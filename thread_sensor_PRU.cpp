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
unsigned int dataSize;

/* 采样配置相关数据 */
unsigned int spiData[14];
unsigned int timerData[2];
unsigned int AIN_num;
unsigned int sample_clock_period_per_seconds;//时间切割单元
unsigned int sample_clock_time_integer;//整数部分
unsigned int sample_clock_time_remainder;//余数部分
float real_sample_freq;//由于延时的整数关系，有时候输入的频率会被修正


PRUThread::PRUThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
    pru_sample_flag = false;
    Task_completed = UNDEFINED;

    filename = NULL;
    fp_data_file = NULL;
    serial = 0;
}

void PRUThread::run()
{
    int event_num = 0;

    qDebug("PRU thread starts\n");

    //PRU初始化，下载PRU代码到Instruction data ram中
    PRU_init_loadcode();

    while(!stopped)
    {
        while(pru_sample_flag)
        {
            /* 1、确定spiData[1]的空间大小。即确定单次采样时间 */
            if(sample_clock_time_integer == 0 && sample_clock_time_remainder == 0)//采样已经完成，应该跳出循环
            {
                qDebug("time is up! pru sample task is completed!");

                pru_sample_flag = false;
                Task_completed = PRU_COMPLETED;
                emit send_to_logic_pru_sample_complete(Task_completed);

                break;
            }
            else if(sample_clock_time_integer != 0)//还有计时单元
            {
                spiData[1] = dataSize;
                qDebug("this is %u clock period", sample_clock_time_integer);
                sample_clock_time_integer--;
                serial++;
            }
            else if(sample_clock_time_remainder != 0)//只有最后一段计时余数
            {
                qDebug("there is %u seconds, less than a clock period", sample_clock_time_remainder);
                spiData[1] = sample_clock_time_remainder * real_sample_freq * AIN_num * 2;
                sample_clock_time_remainder = 0;
                serial++;
            }

            /* 2、将配置数据写入PRU的DATA RAM，启动采样 */
            /* 写入配置数据到DATA RAM */
            prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, spiData, sizeof(spiData));  // spi config
            prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, timerData, sizeof(timerData)); // sample config

            /* 使能PRU运行采样程序 */
            if(prussdrv_pru_enable(ADC_PRU_NUM) == 0)
                 qDebug("The ADCPRU enable succeed!\n");
            if(prussdrv_pru_enable(CLK_PRU_NUM) == 0)
                 qDebug("The CLKPRU enable succeed!\n ");

            /* 生成数据文件名称 */
            filename_serial = pru_sample_start.filename + QString::number(serial) + QString(".txt");
            qDebug() << "filename_serial = " << filename_serial;

            //准备数据保存文件名char
            ba = filename_serial.toLatin1();
            filename = ba.data();

            event_num = prussdrv_pru_wait_event (PRU_EVTOUT_0);//等待采样结束, 阻塞函数
            qDebug("ADC PRU0 program completed, event number %d.\n", event_num);

            if(prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT) == 0)
                qDebug("prussdrv_pru_clear_event success\n");
            else
                qDebug("prussdrv_pru_clear_event failed!\n");

            if(prussdrv_pru_disable(ADC_PRU_NUM) == 0)
                 printf("The ADCPRU disable succeed!\n");
            if(prussdrv_pru_disable(CLK_PRU_NUM) == 0)
                 printf("The CLKPRU disable succeed!\n ");

            /* 3、保存数据到文件 */
            if(save_data_to_file(filename, spiData[1] / 2, AIN_num) == SUCCESS)
                qDebug("Data is saved in %s.\n", filename);
            else
                qDebug("Data save error!\n");

            /* 4、将数据文件读取到内存块中，准备绘图 */
        }

    }

    stopped = false;

    qDebug("PRU thread stopped");
}

void PRUThread::stop()
{
    stopped = true;
}


void PRUThread::recei_fro_logicthread_pru_sample_start(PRU_SAMPLE_START Pru_sample_start)
{
    unsigned int i = 0, j = 0;

    pru_sample_start = Pru_sample_start;

    qDebug() << "pru_sample_start.display_size = " << pru_sample_start.display_size;
    qDebug() << "pru_sample_start.filename = " << pru_sample_start.filename;
    qDebug() << "pru_sample_start.sample_time_hours = " << pru_sample_start.sample_time_hours;
    qDebug() << "pru_sample_start.sample_time_minutes = " << pru_sample_start.sample_time_minutes;
    qDebug() << "pru_sample_start.sample_time_seconds = " << pru_sample_start.sample_time_seconds;
    qDebug() << "pru_sample_start.sample_freq = " << pru_sample_start.sample_freq;
    qDebug() << "pru_sample_start.AIN[0] = " << pru_sample_start.AIN[0];
    qDebug() << "pru_sample_start.AIN[1] = " << pru_sample_start.AIN[1];
    qDebug() << "pru_sample_start.AIN[2] = " << pru_sample_start.AIN[2];
    qDebug() << "pru_sample_start.AIN[3] = " << pru_sample_start.AIN[3];
    qDebug() << "pru_sample_start.AIN[4] = " << pru_sample_start.AIN[4];
    qDebug() << "pru_sample_start.AIN[5] = " << pru_sample_start.AIN[5];
    qDebug() << "pru_sample_start.AIN[6] = " << pru_sample_start.AIN[6];
    qDebug() << "pru_sample_start.AIN[7] = " << pru_sample_start.AIN[7];
    qDebug() << "pru_sample_start.AIN[8] = " << pru_sample_start.AIN[8];
    qDebug() << "pru_sample_start.AIN[9] = " << pru_sample_start.AIN[9];
    qDebug() << "pru_sample_start.AIN[10] = " << pru_sample_start.AIN[10];

    AIN_num = 0;
    for(i = 0; i < sizeof(pru_sample_start.AIN)/sizeof(bool); i++)
        AIN_num += pru_sample_start.AIN[i];

    qDebug() << "sample channels = " << AIN_num;

    j = 0;
    for(i = 0; i < sizeof(pru_sample_start.AIN)/sizeof(bool); i++)
    {
        if(pru_sample_start.AIN[i])
        {
            spiData[2 + (j++)] = (i << 28) | 0x0c000000;
            qDebug("spiData[%d] = 0x%x\n", j+1, spiData[j+1]) ;//
        }
    }
    spiData[2+j] = 0;//截止通道标记
    qDebug("spiData[%d] = 0x%x\n", j+2, spiData[j+2]);

    /* 配置采样频率 */
    timerData[0] = (5 * 10000000) / (pru_sample_start.sample_freq * AIN_num) - 3;
    real_sample_freq = (5.0 * 10000000) / ((timerData[0] + 3) * AIN_num);
    qDebug("The expect sample rate:\t%f\tHz (each channel)\n", pru_sample_start.sample_freq);
    qDebug("The real sample rate:  \t%f\tHz (each channel)\n", real_sample_freq);

    //4000000 16bit 总存储空间大小除以最终的采样频率，得出计时时间单元
    sample_clock_period_per_seconds = dataSize / (real_sample_freq * AIN_num * 2);
    sample_clock_time_integer = (pru_sample_start.sample_time_hours * 3600 + pru_sample_start.sample_time_minutes * 60 + pru_sample_start.sample_time_seconds) / sample_clock_period_per_seconds;
    sample_clock_time_remainder = (pru_sample_start.sample_time_hours * 3600 + pru_sample_start.sample_time_minutes * 60 + pru_sample_start.sample_time_seconds) % sample_clock_period_per_seconds;
    qDebug("sample_clock_period_per_seconds = %u", sample_clock_period_per_seconds);
    qDebug("there are(is) %u sample periods, and %u (s) sample remainder", sample_clock_time_integer, sample_clock_time_remainder);
    qDebug("the total time is %u.", sample_clock_time_integer * sample_clock_period_per_seconds + sample_clock_time_remainder);

    Task_completed = UNDEFINED;
    filename = NULL;
    fp_data_file = NULL;
    serial = 0;

    //开始采样
    pru_sample_flag = true;

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
int save_data_to_file(char * filename, unsigned int numberOutputSamples, unsigned int AIN_NUM)
{
    /*--------------------------保存数据相关------------------------------------------*/
     int mmap_fd;
     FILE *fp_data_file;
     unsigned int i = 0;
     void *map_base, *virt_addr;
     unsigned int num = 0;
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

   fp_data_file = fopen(filename, "w");

   for(i = 0; i < numberOutputSamples; i++)
   {
       virt_addr = map_base + (target & MAP_MASK);
       //read_result = *((uint16_t *) virt_addr);
       num++;
       if(num % AIN_NUM == 0)
           fprintf(fp_data_file, "%u\n", *((uint16_t *) virt_addr));
       else
           fprintf(fp_data_file, "%u\t", *((uint16_t *) virt_addr));

       target+=2;// 2 bytes per sample
   }

   if(munmap(map_base, MAP_SIZE) == -1)
   {
      qDebug("Failed to unmap memory");
      return ERROR;
   }
   close(mmap_fd);

   fclose(fp_data_file);

   return SUCCESS;
}
