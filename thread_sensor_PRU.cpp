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


PRUThread::PRUThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;

}

void PRUThread::run()
{
    qDebug("PRU thread starts\n");

    //PRU初始化，下载PRU代码到Instruction data ram中
    PRU_init_loadcode();

    while(!stopped)
    {

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
    int i = 0, j = 0;

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
    for(i = 0; i < 11; i++)
        AIN_num += pru_sample_start.AIN[i];

    qDebug() << "AIN_num = " << AIN_num;

    j = 0;
    for(i = 0; i < AIN_num; i++)
    {
        if(pru_sample_start.AIN[i])
        {
            spiData[2 + (j++)] = (i << 12) | 0x0c000000;
            //printf("spiData[%d] = 0x%x\n", j+1, spiData[j+1]) ;//
        }
    }

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
//    /* 保存数据到文件 */
//    if(save_data_to_file("/root/qi_enose_online/pru_data.txt", spiData[1] / 2, 11) == SUCCESS)
//        qDebug("Data is saved in /root/qi_enose_online/pru_data.txt");
//    else
//        qDebug("Data save error!\n");
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
     //unsigned short int read_result;
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
