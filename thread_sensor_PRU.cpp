#include "thread_sensor_PRU.h"
#include <QDebug>
#include <unistd.h>
#include <QDateTime>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <error.h>

/*--------------PRU header-------------*/
#include "prussdrv.h"
#include "pruss_intc_mapping.h"

#include <ctype.h>
#include <termios.h>
#include <sys/mman.h>

/* DDR地址 */
unsigned int addr;
unsigned int dataSize;// units byte

/* 采样配置相关数据 */
unsigned int spiData[14];
unsigned int timerData[2];
unsigned int PRU_shared_mem_cfg[4] = {0};//用于配置PRU SHARED MEM
unsigned int AIN_num;
unsigned int AIN_num_temp;
unsigned int sample_clock_period_per_seconds;//时间切割单元
unsigned int sample_clock_time_integer;//整数部分
unsigned int sample_clock_time_remainder;//余数部分
float real_sample_freq;//由于延时的整数关系，有时候输入的频率会被修正

void *sharedMem = NULL;
unsigned int *sharedMem_int;
unsigned int PRUADC_stop = 1;
unsigned int PRU_shared_mem_result[4] = {0};//用于保存PRU SHARED MEM的读取结果


//pru数据的内存块，用于选项卡绘图的数据，该数据块循环更新，为全局变量
PLOT_DATA_BUF pru_plot_data_buf;

//快速拷贝时的临时存储区域
unsigned short int mem_buffer[MAX_BUFFER_SIZE] = {0};

//保存从文件中读取的配置参数
extern SYS_Para sys_para;
//pru绘图曲线的时间跨度，表示整条曲线从最左端的采样点到最右端采样点之间的时间间隔
unsigned int pru_plot_time_span;
unsigned int pru_memory_size;

PRUThread::PRUThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
    pru_sample_flag = false;
    pru_sample_end = false;
    Task_completed = UNDEFINED;

    //pru数据的内存块，初始化
    pru_plot_data_buf.p_data = NULL;
    pru_plot_data_buf.pp_data = NULL;
    pru_plot_data_buf.buf_size = 0;
    pru_plot_data_buf.index = 0;
    pru_plot_data_buf.valid_data_size = 0;
    pru_plot_data_buf.pp_data_float = NULL;
    pru_plot_data_buf.pp_data_int = NULL;

    AIN_num = 0;
    AIN_num_temp = 0;

    filename = NULL;
    fp_data_file = NULL;
    serial = 0;
    sample_time_per_section = 0;
    last_sample_time_per_section = 0;
    last_spiData_1 = 0;

    pru_plot_time_span = PRU_PLOT_TIME_SPAN;// = sys_para.time_span_pru_plot
}

void PRUThread::run()
{
    int event_num = 0;

    qDebug("PRU thread starts");

    //PRU初始化，下载PRU代码到Instruction data ram中
    PRU_init_loadcode();

    while(!stopped)
    {
        while(pru_sample_flag)
        {
            /* 1、计算本次将要采样的数据量, 确定spiData[1]的空间大小。即确定单次采样时间 */
            if(sample_clock_time_integer == 0 && sample_clock_time_remainder == 0)//采样已经完成，应该跳出循环
            {
                pru_sample_flag = false;
                pru_sample_end = true;
            }
            else if(sample_clock_time_integer != 0)//还有计时单元
            {
                spiData[1] = dataSize;
                qDebug("this is %u clock period", sample_clock_time_integer);
                if(pru_sample_start.sample_mode == TIMING)
                {
                    sample_clock_time_integer--;
                }
                else if(pru_sample_start.sample_mode == MONITOR)//监测模式下保证sample_clock_time_integer不为0或者不递减即可,直到逻辑线程发来终结信号
                {
                    sample_clock_time_integer = 10000;//
                }
                sample_time_per_section = sample_clock_period_per_seconds;
            }
            else if(sample_clock_time_remainder != 0)//只有最后一段计时余数
            {
                qDebug("there is %u seconds, less than a clock period", sample_clock_time_remainder);
                spiData[1] = sample_clock_time_remainder * real_sample_freq * AIN_num * 2;
                sample_time_per_section = sample_clock_time_remainder;
                sample_clock_time_remainder = 0;
            }

            if(!pru_sample_end)
            {
                /* 2、将配置数据写入PRU的DATA RAM，启动采样 */
                /* 写入配置数据到DATA RAM */
                prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, spiData, sizeof(spiData));  // spi config
                prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, timerData, sizeof(timerData)); // sample config
                prussdrv_pru_write_memory(PRUSS0_SHARED_DATARAM, 0, PRU_shared_mem_cfg, sizeof(PRU_shared_mem_cfg));//PRU_shared_mem的前四个DWORD全写0,

                /* 若线程运行到此处时接到来自逻辑线程的停止采样信号，那么执行完槽函数后有可能产生只有一个数据的文件，为了避免此种现象，增加对last_spiData_1进行判断 */

                /* 使能PRU运行采样程序 */
                if(prussdrv_pru_enable(ADC_PRU_NUM) == 0)
                     qDebug("The ADCPRU enable succeed!\n");
                if(prussdrv_pru_enable(CLK_PRU_NUM) == 0)
                     qDebug("The CLKPRU enable succeed!\n ");
            }

            if(serial > 0)
            {
                /* 生成数据文件名称 */
                pru_plot_data_buf.filename = pru_sample_start.filename + QString::number(serial) + QString(".txt");
                qDebug() << "pru_plot_data_buf.filename = " << pru_plot_data_buf.filename;

                //准备数据保存文件名char
                ba = pru_plot_data_buf.filename.toLatin1();
                filename = ba.data();

                if(last_spiData_1 > 0)
                {
                    /* 根据上一次的实际采样情况进行数据拷贝,保存数据到文件, 并将绘图数据拷贝到内存块 */
                    if(save_and_plot_data(filename, last_spiData_1 >> 1, AIN_num, mem_buffer, last_sample_time_per_section) == SUCCESS)
                        qDebug("Data is saved in %s.\n", filename);
                    else
                        qDebug("Data save error!\n");

                    /* 发送信号驱动绘图选项卡开始绘图 */
                    emit send_to_plot_pru_curve();
                }
                else
                {
                    qDebug("Save_and_plot_data() was cancelled, there are no data in temp mem");
                }

                //sample task end ?
                if(pru_sample_end)
                {
                    qDebug("time is up! pru sample task is completed!");
                    Task_completed = PRU_COMPLETED;
                    emit send_to_logic_pru_sample_complete(Task_completed);//采样结束，发送信号通知逻辑线程
                    break;
                }
                else
                {
                    qDebug("sample times: serial = %u", serial);
                }
            }
            else
            {
                qDebug("No data to store for the first time.");
            }

            if(pru_sample_flag)
            {
                event_num = prussdrv_pru_wait_event (PRU_EVTOUT_0);//等待采样结束, 阻塞函数
                qDebug("ADC PRU0 program completed, event number %d.\n", event_num);

                if(prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT) == 0)
                    qDebug("prussdrv_pru_clear_event success\n");
                else
                    qDebug("prussdrv_pru_clear_event failed!\n");

                if(prussdrv_pru_disable(ADC_PRU_NUM) == 0)
                    qDebug("The ADCPRU disable succeed!\n");
                if(prussdrv_pru_disable(CLK_PRU_NUM) == 0)
                    qDebug("The CLKPRU disable succeed!\n ");

                //读取存储空间的数据个数
                PRU_shared_mem_result[0] = *sharedMem_int;
                PRU_shared_mem_result[1] = *(sharedMem_int + 1);
                PRU_shared_mem_result[2] = *(sharedMem_int + 2);
                PRU_shared_mem_result[3] = *(sharedMem_int + 3);

                qDebug("sharedMem1 = %u\nsharedMem2 = %u\nsharedMem3 = 0x%x\nsharedMem4 = %u.\n", PRU_shared_mem_result[0], PRU_shared_mem_result[1], PRU_shared_mem_result[2], PRU_shared_mem_result[3]);
                qDebug("%u data was sampled", spiData[1] - PRU_shared_mem_result[3]);

                //sample_time_per_section和spiData[1]主要用于这次的采样配置
                //last_sample_time_per_section和last_spiData_1主要用于上一次的数据拷贝处理
                last_sample_time_per_section = sample_time_per_section;
                last_spiData_1 = spiData[1] - PRU_shared_mem_result[3];//根据PRU SHARED MEM空间返回的剩余空间计算消耗的空间，即计算采样字节数（bytes）
                last_spiData_1 = last_spiData_1 - last_spiData_1 % (AIN_num * 2);//last_spiData_1表示行数
                qDebug("last_spiData_1 = %u (the data less than one line were discarded.)", last_spiData_1);
                qDebug("last_sample_time_per_section = %u", last_sample_time_per_section);

                //fast memroy copy, prepare parameters for file saving
                if(last_spiData_1 > 0)
                {
                    if(copy_data_to_buf(mem_buffer, last_spiData_1) == SUCCESS)//spiData[1] unit bytes,不足一行的部分丢弃
                        qDebug("%u (bytes) data is copied.", last_spiData_1);
                }
                else
                {
                    qDebug("memcpy() was cancelled, there are no data in temp mem");
                }

                serial++;
            }

        }

    }

    stopped = false;

    //停止线程时必须释放申请的内存空间
    if(pru_plot_data_buf.pp_data != NULL)
    {
        for(unsigned int i = 0; i < AIN_num; i++)//释放原来的AIN_num条内存块
        {
            free(pru_plot_data_buf.pp_data[i]);
            pru_plot_data_buf.pp_data[i] = NULL;
        }

        free(pru_plot_data_buf.pp_data);
        pru_plot_data_buf.pp_data = NULL;
        qDebug() << "pru free memory before pruthread stop";
    }

    //Releases PRUSS clocks and disables prussdrv module.
    prussdrv_exit();

    qDebug("PRU thread stopped");
}

void PRUThread::stop()
{
    //有时候stop信号还没发过来，或者计时时间还未到，用户突然按下quit要退出程序，此时应该在退出程序前把未保存的数据（虽然不完整）任然保存到文件
    qDebug("pru is ready to stop sample task when quit button was pressed");

    //停止当前片段的pru采集
    PRUADC_stop = 1;
    prussdrv_pru_write_memory(PRUSS0_SHARED_DATARAM, 1, &PRUADC_stop, sizeof(unsigned int));

    //结束采样循环
    sample_clock_time_integer = 0;
    sample_clock_time_remainder = 0;

    stopped = true;
}


void PRUThread::recei_fro_logicthread_pru_sample_start(PRU_SAMPLE_START Pru_sample_start)
{
    unsigned int i = 0, j = 0;

    pru_sample_start = Pru_sample_start;

//    qDebug() << "pru_sample_start.display_size = " << pru_sample_start.display_size;
//    qDebug() << "pru_sample_start.filename = " << pru_sample_start.filename;
//    qDebug() << "pru_sample_start.sample_time_hours = " << pru_sample_start.sample_time_hours;
//    qDebug() << "pru_sample_start.sample_time_minutes = " << pru_sample_start.sample_time_minutes;
//    qDebug() << "pru_sample_start.sample_time_seconds = " << pru_sample_start.sample_time_seconds;
//    qDebug() << "pru_sample_start.sample_freq = " << pru_sample_start.sample_freq;
//    qDebug() << "pru_sample_start.AIN[0] = " << pru_sample_start.AIN[0];
//    qDebug() << "pru_sample_start.AIN[1] = " << pru_sample_start.AIN[1];
//    qDebug() << "pru_sample_start.AIN[2] = " << pru_sample_start.AIN[2];
//    qDebug() << "pru_sample_start.AIN[3] = " << pru_sample_start.AIN[3];
//    qDebug() << "pru_sample_start.AIN[4] = " << pru_sample_start.AIN[4];
//    qDebug() << "pru_sample_start.AIN[5] = " << pru_sample_start.AIN[5];
//    qDebug() << "pru_sample_start.AIN[6] = " << pru_sample_start.AIN[6];
//    qDebug() << "pru_sample_start.AIN[7] = " << pru_sample_start.AIN[7];
//    qDebug() << "pru_sample_start.AIN[8] = " << pru_sample_start.AIN[8];
//    qDebug() << "pru_sample_start.AIN[9] = " << pru_sample_start.AIN[9];
//    qDebug() << "pru_sample_start.AIN[10] = " << pru_sample_start.AIN[10];

    /* 解析采样通道情况 */
    AIN_num_temp = 0;
    for(i = 0; i < sizeof(pru_sample_start.AIN)/sizeof(bool); i++)
        AIN_num_temp += pru_sample_start.AIN[i];

    qDebug() << "total sample channels = " << AIN_num_temp;

    j = 0;
    for(i = 0; i < sizeof(pru_sample_start.AIN)/sizeof(bool); i++)
    {
        if(pru_sample_start.AIN[i])
        {
            spiData[2 + (j++)] = (i << 28) | 0x0c000000;
            //qDebug("spiData[%d] = 0x%x\n", j+1, spiData[j+1]) ;//
        }
    }
    spiData[2+j] = 0;//截止通道标记
    //qDebug("spiData[%d] = 0x%x\n", j+2, spiData[j+2]);

    //逻辑线程要求的绘图尺寸和pru内存数据块大小不一致时必须重新分配内存块大小
    if(pru_sample_start.display_size != pru_plot_data_buf.buf_size || AIN_num_temp != AIN_num)
    {
        //释放原来的内存空间
        if(pru_plot_data_buf.pp_data != NULL)
        {
            for(i = 0; i < AIN_num; i++)//释放原来的AIN_num条内存块
            {
                free(pru_plot_data_buf.pp_data[i]);
                pru_plot_data_buf.pp_data[i] = NULL;
            }

            free(pru_plot_data_buf.pp_data);
            pru_plot_data_buf.pp_data = NULL;
            qDebug() << "pru free last memory size";
        }

        //申请新的空间
        pru_plot_data_buf.pp_data = (unsigned short int **)malloc(sizeof(unsigned short int *) * AIN_num_temp);
        if(pru_plot_data_buf.pp_data)
        {
            for(i = 0; i < AIN_num_temp; i++)
            {
                pru_plot_data_buf.pp_data[i] = (unsigned short int *)malloc(sizeof(unsigned short int) * pru_sample_start.display_size);
                memset(pru_plot_data_buf.pp_data[i], 0, sizeof(unsigned short int) * pru_sample_start.display_size);
            }
            qDebug("get new memory for pru plot buf");
        }
        else
        {
            qDebug("memory allocate again failed");
        }
    }
    else//若大小相等，则内存块不需要重新申请，只需要将原来的内存空间写0即可，索引归0
    {
        for(i = 0; i < AIN_num_temp; i++)
        {
            memset(pru_plot_data_buf.pp_data[i], 0, sizeof(unsigned short int) * pru_sample_start.display_size);
        }
        qDebug() << "pru memory size equals!";
    }

    AIN_num = AIN_num_temp;
    pru_plot_data_buf.buf_size = pru_sample_start.display_size;
    pru_plot_data_buf.index = 0;
    pru_plot_data_buf.valid_data_size = 0;
    pru_plot_data_buf.filename = pru_sample_start.filename;

    /* 主要用于更新绘图选项卡中自带的PRU_Plot_Data_Buf数据 */
    emit send_to_plot_pru_curve();

    /* 配置采样频率 */
    timerData[0] = (5 * 10000000) / (pru_sample_start.sample_freq * AIN_num) - 3;
    real_sample_freq = (5.0 * 10000000) / ((timerData[0] + 3) * AIN_num);
    qDebug("The expect sample rate:\t%f\tHz (each channel)\n", pru_sample_start.sample_freq);
    qDebug("The real sample rate:  \t%f\tHz (each channel)\n", real_sample_freq);

    //4000000 16bit 总存储空间大小除以最终的采样频率，得出计时时间单元
    sample_clock_period_per_seconds = dataSize / (real_sample_freq * AIN_num * 2);

    if(pru_sample_start.sample_mode == TIMING)//定时模式下计算采样周期
    {
        sample_clock_time_integer = (pru_sample_start.sample_time_hours * 3600 + pru_sample_start.sample_time_minutes * 60 + pru_sample_start.sample_time_seconds) / sample_clock_period_per_seconds;
        sample_clock_time_remainder = (pru_sample_start.sample_time_hours * 3600 + pru_sample_start.sample_time_minutes * 60 + pru_sample_start.sample_time_seconds) % sample_clock_period_per_seconds;
    }
    else if(pru_sample_start.sample_mode == MONITOR)//监测模式下保证sample_clock_time_integer不为0或者不递减即可,直到逻辑线程发来终结信号
    {
        sample_clock_time_integer = 10000;
        sample_clock_time_remainder = 0;
    }

    qDebug("sample_clock_period_per_seconds = %u", sample_clock_period_per_seconds);
    qDebug("there are(is) %u sample periods, and %u (s) sample remainder", sample_clock_time_integer, sample_clock_time_remainder);
    qDebug("the total time is %u.", sample_clock_time_integer * sample_clock_period_per_seconds + sample_clock_time_remainder);

    Task_completed = UNDEFINED;
    filename = NULL;
    fp_data_file = NULL;
    serial = 0;
    sample_time_per_section = 0;
    last_sample_time_per_section = 0;
    last_spiData_1 = 0;
    spiData[1] = 0;
    PRU_shared_mem_result[3] = 0;

    //开始采样
    pru_sample_flag = true;
    pru_sample_end = false;
}

void PRUThread::recei_fro_logicthread_pru_sample_stop()
{
    qDebug("pru is ready to stop sample task");

    //停止当前片段的pru采集
    PRUADC_stop = 1;
    prussdrv_pru_write_memory(PRUSS0_SHARED_DATARAM, 1, &PRUADC_stop, sizeof(unsigned int));

    //结束采样循环
    sample_clock_time_integer = 0;
    sample_clock_time_remainder = 0;

}

/* PRU初始化，下载PRU代码到Instruction data ram中 */
void PRU_init_loadcode()
{
    addr = readFileValue(MMAP1_LOC "addr");
    dataSize = readFileValue(MMAP1_LOC "size");

    // Initialize structure used by prussdrv_pruintc_intc
    // PRUSS_INTC_INITDATA is found in pruss_intc_mapping.h
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    // Read in the location and address of the shared memory. This value changes
    // each time a new block of memory is allocated.
    timerData[0] = FREQ_50kHz;
    timerData[1] = RUNNING;
    qDebug("The PRU clock state is set as period: %d (0x%x) and state: %d\n", timerData[0], timerData[0], timerData[1]);
    unsigned int PRU_data_addr = readFileValue(MMAP0_LOC "addr");
    qDebug("-> the PRUClock memory is mapped at the base address: %x\n", (PRU_data_addr + 0x2000));
    qDebug("-> the PRUClock on/off state is mapped at address: %x\n", (PRU_data_addr + 0x10000));

    // data for PRU0 based on the TLC2543 datasheet
    spiData[0] = addr;//readFileValue(MMAP1_LOC "addr")
    spiData[1] = 22;//单位字节，2个字节存储一次采样数据, 采样10次作为测试，主要是为了提前将PRU代码写入Instruction data ram中
    spiData[2] = 0x0c000000;//0x0c000000
    spiData[3] = 0x1c000000;//0x1c000000
    spiData[4] = 0x2c000000;//0x2c000000
    spiData[5] = 0x3c000000;
    spiData[6] = 0x4c000000;
    spiData[7] = 0x5c000000;
    spiData[8] = 0x6c000000;
    spiData[9] = 0x7c000000;
    spiData[10] = 0x8c000000;
    spiData[11] = 0x9c000000;
    spiData[12] = 0xac000000;
    spiData[13] = 0;

    qDebug("The DDR External Memory pool has location: 0x%x and size: 0x%x bytes\n", addr, dataSize);
    qDebug("-> this space has capacity to store %d 16-bit samples (max)\n", dataSize / 2);

    qDebug("Sending the SPI Control Data: 0x%x\n", spiData[2]);
    qDebug("This is the test for sampling and load PRU code into the instruction ram.\n");

    // Allocate and initialize memory
    prussdrv_init ();
    prussdrv_open (PRU_EVTOUT_0);
    // Map the PRU's interrupts
    prussdrv_pruintc_init(&pruss_intc_initdata);

    // Write the address and size into PRU0 Data RAM0. You can edit the value to
    // PRUSS0_PRU1_DATARAM if you wish to write to PRU1
    prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, spiData, sizeof(spiData));  // spi code
    prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, timerData, sizeof(timerData)); // sample clock
    prussdrv_pru_write_memory(PRUSS0_SHARED_DATARAM, 0, PRU_shared_mem_cfg, sizeof(PRU_shared_mem_cfg));//PRU_shared_mem的前四个DWORD全写0,

    //映射PRU SHARED MEM空间，获取空间首地址
    prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &sharedMem);
    sharedMem_int = (unsigned int*) sharedMem;
    qDebug("INIT:sharedMem1 = %u, sharedMem2 = %u, sharedMem3 = 0x%x, sharedMem4 = %u.\n", *sharedMem_int, *(sharedMem_int + 1), *(sharedMem_int + 2), *(sharedMem_int + 3));

    // Load and execute the PRU program on the PRU
    prussdrv_exec_program (ADC_PRU_NUM, PRUADC_BIN);
    prussdrv_exec_program (CLK_PRU_NUM, PRUClock_BIN);
    qDebug("EBBClock PRU1 program now running (%d )\n", timerData[0]);

    // Wait for event completion from PRU, returns the PRU_EVTOUT_0 number
    int n = prussdrv_pru_wait_event (PRU_EVTOUT_0);
    qDebug("EBBADC PRU0 program test completed, event number %d.\n", n);

    if(prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT) == 0)
         qDebug("prussdrv_pru_clear_event success\n");
    else
        qDebug("prussdrv_pru_clear_event failed!\n");

    // Disable PRU
    prussdrv_pru_disable(ADC_PRU_NUM);
    prussdrv_pru_disable(CLK_PRU_NUM);

    /* 测试不需要保存数据 */

}

// Short function to load a single unsigned int from a sysfs entry
unsigned int readFileValue(char filename[])
{
   FILE* fp;
   unsigned int value = 0;
   fp = fopen(filename, "rt");
   fscanf(fp, "%x", &value);
   fclose(fp);
   return value;
}

/* 保存数据到文件 */
int save_and_plot_data(char * filename, unsigned int numberOutputSamples, unsigned int AIN_NUM, unsigned short int *P_Data, unsigned int Sample_time_per_section)
{
    /*--------------------------保存数据相关------------------------------------------*/
     FILE *fp_data_file;
     unsigned int i = 0, j = 0, start_index = 0;
     unsigned int num = 0;

   //保存数据到文件
   fp_data_file = fopen(filename, "w");
   num = 0;
   for(i = 0; i < numberOutputSamples; i++)
   {
       num++;
       if(num % AIN_NUM == 0)
           fprintf(fp_data_file, "%u\n", P_Data[i]);
       else
           fprintf(fp_data_file, "%u\t", P_Data[i]);
   }
   fclose(fp_data_file);

   //选取数据到内存块
   unsigned int factor = 0;//分频因子

   //计算分频因子
   qDebug("Sample_time_per_section = %u", Sample_time_per_section);
   factor = (pru_plot_time_span * numberOutputSamples) / (Sample_time_per_section * pru_plot_data_buf.buf_size * AIN_NUM);
   qDebug("factor = %d", factor);

   if(pru_plot_time_span >= Sample_time_per_section)
   {
        for(i = 0; i < numberOutputSamples / AIN_NUM; i++)
        {
            if(i % factor == 0)//如果i能被factor整除，则
            {
                for(j = 0; j < AIN_NUM; j++)//copy data
                {
                    pru_plot_data_buf.pp_data[j][pru_plot_data_buf.index] = P_Data[i * AIN_NUM + j];

                }

                pru_plot_data_buf.index++;
                if(pru_plot_data_buf.index == pru_plot_data_buf.buf_size)
                    pru_plot_data_buf.index = 0;

                if(pru_plot_data_buf.valid_data_size < pru_plot_data_buf.buf_size)
                    pru_plot_data_buf.valid_data_size++;
            }
        }
   }
   else
   {
       start_index = ((Sample_time_per_section - pru_plot_time_span) * (numberOutputSamples / AIN_NUM)) / Sample_time_per_section;
       //qDebug("start_index = %u", start_index);

       for(i = start_index; i < numberOutputSamples / AIN_NUM; i++)
       {
           if(i % factor == 0)//如果i能被factor整除，则
           {
               for(j = 0; j < AIN_NUM; j++)//copy data
               {
                   pru_plot_data_buf.pp_data[j][pru_plot_data_buf.index] = P_Data[i * AIN_NUM + j];

               }

               pru_plot_data_buf.index++;
               if(pru_plot_data_buf.index == pru_plot_data_buf.buf_size)
                   pru_plot_data_buf.index = 0;

               if(pru_plot_data_buf.valid_data_size < pru_plot_data_buf.buf_size)
                   pru_plot_data_buf.valid_data_size++;
           }
       }

   }

   qDebug("plot data memory is ready");

   return SUCCESS;
}


/* 拷贝数据到临时内存 */
int copy_data_to_buf(unsigned short int *p_data, unsigned int Size_byte)
{
    /*--------------------------保存数据相关------------------------------------------*/
     int mmap_fd;
     void *map_base;
     off_t target = addr;

     /*--------------------------保存数据相关------------------------------------------*/
     if((mmap_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
     {
         qDebug("Failed to open memory!");
         return ERROR;
     }

   map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_fd, target & ~MAP_MASK);
   if(map_base == (void *) -1) {
      qDebug("Failed to map base address");
      return ERROR;
   }

   //target = addr;
   memcpy((void *)p_data, map_base + (target & MAP_MASK), Size_byte);//size unit unsigned short int

   if(munmap(map_base, MAP_SIZE) == -1)
   {
      qDebug("Failed to unmap memory");
      return ERROR;
   }

   close(mmap_fd);

    return SUCCESS;
}
