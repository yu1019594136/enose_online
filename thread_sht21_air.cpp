#include "thread_sht21_air.h"
#include <stdio.h>
#include <stdint.h>
#include <QDebug>
#include <unistd.h>
#include <malloc.h>
#include "sht21.h"
#include "stm32_spislave.h"

//sht21_plot_data_buf用于存储温湿度绘图数据, air_plot_data_buf;用于存储空气质量传感器绘图数据
PLOT_DATA_BUF sht21_plot_data_buf, air_plot_data_buf;

//spi spi通信
uint16_t txbuf[5] = {0xfe00, 0xfe01, 0xfe02, 0xfe03, 0xffff};	//SPI通信发送缓冲区
uint16_t rxbuf[5] = {0};	//SPI通信接收缓冲区

Sht21_Air_Thread::Sht21_Air_Thread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
    task_completed = UNDEFINED;
    sht21_sample_start = false;
    air_sample_start = false;

    sht21_sample_stop = false;
    air_sample_stop = false;

    fp_sht21_data_file = NULL;
    fp_air_data_file = NULL;

    //串口数据的内存块，初始化
    sht21_plot_data_buf.pp_data = NULL;
    sht21_plot_data_buf.p_data = NULL;
    sht21_plot_data_buf.buf_size = 0;
    sht21_plot_data_buf.index = 0;
    sht21_plot_data_buf.valid_data_size = 0;
    sht21_plot_data_buf.pp_data_float = NULL;
    sht21_plot_data_buf.pp_data_int = NULL;

    air_plot_data_buf.pp_data = NULL;
    air_plot_data_buf.p_data = NULL;
    air_plot_data_buf.buf_size = 0;
    air_plot_data_buf.index = 0;
    air_plot_data_buf.valid_data_size = 0;
    air_plot_data_buf.pp_data_float = NULL;
    air_plot_data_buf.pp_data_int = NULL;

    sht21_timer = new QTimer(this);
    connect(sht21_timer, SIGNAL(timeout()), this, SLOT(sht21_timerUpdate()));

    air_timer = new QTimer(this);
    connect(air_timer, SIGNAL(timeout()), this, SLOT(air_timerUpdate()));
}

void Sht21_Air_Thread::run()
{
    int i = 0;
    float temp = 0.0, humidity = 0.0;
    unsigned int duty1 = 0, duty2 = 0;

    stm32_Init();

    qDebug("Sht21_Air_thread starts");

    while(!stopped)
    {
        if(sht21_sample_start)
        {
            temp = sht21_get_temp_float();
            humidity = sht21_get_humidity_float();

            fprintf(fp_sht21_data_file, "%f\t%f\n", temp, humidity);
            sht21_plot_data_buf.pp_data_float[0][sht21_plot_data_buf.index] = temp;
            sht21_plot_data_buf.pp_data_float[1][sht21_plot_data_buf.index++] = humidity;

            if(sht21_plot_data_buf.index == sht21_plot_data_buf.buf_size)
                sht21_plot_data_buf.index = 0;

            if(sht21_plot_data_buf.valid_data_size < sht21_plot_data_buf.buf_size)
                sht21_plot_data_buf.valid_data_size++;

            //plot
            emit send_to_plot_sht21_curve();

            sht21_sample_start = false;
        }

        if(sht21_sample_stop)
        {
            if(fp_sht21_data_file)
            {
                fclose(fp_sht21_data_file);
                fp_sht21_data_file = NULL;
                qDebug("fp_sht21_data_file is closed, data is saved in %s.", sht21_filename);
            }
            sht21_sample_stop = false;
        }

        if(air_sample_start)
        {
            for(i = 0; i < 5; i++)
            {
                stm32_Transfer(txbuf + i, rxbuf + i, 2);
            }
            duty1 = rxbuf[1] * 65536 + rxbuf[2];
            duty2 = rxbuf[3] * 65536 + rxbuf[4];

//            if(duty1 > 100000)
//                duty1 = duty1 >> 1;
//            if(duty2 > 100000)
//                duty2 = duty2 >> 1;

            fprintf(fp_air_data_file, "%u\t%u\n", duty1, duty2);

            air_plot_data_buf.pp_data_int[0][air_plot_data_buf.index] = duty1;
            air_plot_data_buf.pp_data_int[1][air_plot_data_buf.index++] = duty2;

            if(air_plot_data_buf.index == air_plot_data_buf.buf_size)
                air_plot_data_buf.index = 0;

            if(air_plot_data_buf.valid_data_size < air_plot_data_buf.buf_size)
                air_plot_data_buf.valid_data_size++;

            emit send_to_plot_air_curve();

            air_sample_start = false;
        }

        if(air_sample_stop)
        {
            if(fp_air_data_file)
            {
                fclose(fp_air_data_file);
                fp_air_data_file = NULL;
                qDebug("fp_air_data_file is closed, data is saved in %s.", air_filename);
            }
            air_sample_stop = false;
        }
    }

    stopped = false;

    //突然停止线程时必须释放申请的内存空间
    if(sht21_plot_data_buf.pp_data_float != NULL)
    {
        for(i = 0; i < 2; i++)//释放原来的AIN_num条内存块
        {
            free(sht21_plot_data_buf.pp_data_float[i]);
            sht21_plot_data_buf.pp_data_float[i] = NULL;
        }

        free(sht21_plot_data_buf.pp_data_float);
        sht21_plot_data_buf.pp_data_float = NULL;
        qDebug() << "sht21 free last memory size";
    }

    //突然停止线程时必须释放申请的内存空间
    if(air_plot_data_buf.pp_data_int != NULL)
    {
        for(i = 0; i < 2; i++)//释放原来的AIN_num条内存块
        {
            free(air_plot_data_buf.pp_data_int[i]);
            air_plot_data_buf.pp_data_int[i] = NULL;
        }

        free(air_plot_data_buf.pp_data_int);
        air_plot_data_buf.pp_data_int = NULL;
        qDebug() << "air free last memory size";
    }

    stm32_Close();

    qDebug("Sht21_Air_thread stopped");
}

void Sht21_Air_Thread::stop()
{
    //结束线程
    stopped = true;
}

//响应逻辑线程的信号，开始采集数据
void Sht21_Air_Thread::recei_fro_logicthread_sht21_air_sample_start(SHT21_AIR_SAMPLE_START Sht21_air_sample_start)
{
    int i;

    sht21_air_sample_start = Sht21_air_sample_start;
    qDebug() << "sht21_air_sample_start.air_display_size = " << sht21_air_sample_start.air_display_size;
    qDebug() << "sht21_air_sample_start.air_filename = " << sht21_air_sample_start.air_filename;
    qDebug() << "sht21_air_sample_start.air_period = " << sht21_air_sample_start.air_period;

    qDebug() << "sht21_air_sample_start.sht21_display_size = " << sht21_air_sample_start.sht21_display_size;
    qDebug() << "sht21_air_sample_start.sht21_filename = " << sht21_air_sample_start.sht21_filename;
    qDebug() << "sht21_air_sample_start.sht21_period = " << sht21_air_sample_start.sht21_period;

    /*=============================for sht21============================================*/
    /* 检查是否重新分配空间 */

    //逻辑线程要求的绘图尺寸和串口内存数据块大小不一致时必须重新分配内存块大小
    if(sht21_air_sample_start.sht21_display_size != sht21_plot_data_buf.buf_size)
    {
        //释放原来的内存空间
        if(sht21_plot_data_buf.pp_data_float != NULL)
        {
            for(i = 0; i < 2; i++)//释放原来的AIN_num条内存块
            {
                free(sht21_plot_data_buf.pp_data_float[i]);
                sht21_plot_data_buf.pp_data_float[i] = NULL;
            }

            free(sht21_plot_data_buf.pp_data_float);
            sht21_plot_data_buf.pp_data_float = NULL;
            qDebug() << "sht21 free last memory size";
        }

        //申请新的内存空间
        sht21_plot_data_buf.pp_data_float = (float **)malloc(sizeof(float *) * 2);//one for temperature, one for humidity
        if(sht21_plot_data_buf.pp_data_float)
        {
            for(i = 0; i < 2; i++)
            {
                sht21_plot_data_buf.pp_data_float[i] = (float *)malloc(sizeof(float) * sht21_air_sample_start.sht21_display_size);
                memset(sht21_plot_data_buf.pp_data_float[i], 0, sizeof(float) * sht21_air_sample_start.sht21_display_size);
            }
            qDebug("get new memory for sht21 plot buf");
        }
    }
    else//若大小相等，则内存块不需要重新申请，只需要将原来的内存空间写0即可，索引归0
    {
        //memset(sht21_plot_data_buf.p_data, 0, sizeof(unsigned short int) * uart_sample_start.display_size);//将分配的内存空间初始化为0
        for(i = 0; i < 2; i++)
        {
            memset(sht21_plot_data_buf.pp_data_float[i], 0, sizeof(float) * sht21_air_sample_start.sht21_display_size);
        }
        qDebug() << "sht21 memory size equals!";
    }

    sht21_plot_data_buf.filename = sht21_air_sample_start.sht21_filename;
    sht21_plot_data_buf.valid_data_size = 0;
    sht21_plot_data_buf.index = 0;
    sht21_plot_data_buf.buf_size = sht21_air_sample_start.sht21_display_size;

    /*=============================for air============================================*/

    //逻辑线程要求的绘图尺寸和串口内存数据块大小不一致时必须重新分配内存块大小
    if(sht21_air_sample_start.air_display_size != air_plot_data_buf.buf_size)
    {
        //释放原来的内存空间
        if(air_plot_data_buf.pp_data_int != NULL)
        {
            for(i = 0; i < 2; i++)//释放原来的AIN_num条内存块
            {
                free(air_plot_data_buf.pp_data_int[i]);
                air_plot_data_buf.pp_data_int[i] = NULL;
            }

            free(air_plot_data_buf.pp_data_int);
            air_plot_data_buf.pp_data_int = NULL;
            qDebug() << "air free last memory size";
        }

        //申请新的内存空间
        air_plot_data_buf.pp_data_int = (unsigned int **)malloc(sizeof(unsigned int *) * 2);//one for sensor1, one for sensor2
        if(air_plot_data_buf.pp_data_int)
        {
            for(i = 0; i < 2; i++)
            {
                air_plot_data_buf.pp_data_int[i] = (unsigned int *)malloc(sizeof(unsigned int) * sht21_air_sample_start.air_display_size);
                memset(air_plot_data_buf.pp_data_int[i], 0, sizeof(unsigned int) * sht21_air_sample_start.air_display_size);
            }
            qDebug("get new memory for air plot buf");
        }
    }
    else//若大小相等，则内存块不需要重新申请，只需要将原来的内存空间写0即可，索引归0
    {
        for(i = 0; i < 2; i++)
        {
            memset(air_plot_data_buf.pp_data_int[i], 0, sizeof(float) * sht21_air_sample_start.air_display_size);
        }
        qDebug() << "air memory size equals!";
    }

    air_plot_data_buf.filename = sht21_air_sample_start.air_filename;
    air_plot_data_buf.valid_data_size = 0;
    air_plot_data_buf.index = 0;
    air_plot_data_buf.buf_size = sht21_air_sample_start.air_display_size;

    /*=============================end============================================*/

    //准备数据文件名
    //准备数据保存文件名char
    ba1 = sht21_air_sample_start.sht21_filename.toLatin1();
    sht21_filename = ba1.data();
    fp_sht21_data_file = fopen(sht21_filename , "w");

    ba2 = sht21_air_sample_start.air_filename.toLatin1();
    air_filename = ba2.data();
    fp_air_data_file = fopen(air_filename , "w");

    task_completed = UNDEFINED;
    sht21_sample_stop = false;
    air_sample_stop = false;

    //启动定时器开始采样
    air_timer->start(sht21_air_sample_start.air_period);
    sht21_timer->start(sht21_air_sample_start.sht21_period * 1000);

}


//响应停止串口数据采集的槽函数
void Sht21_Air_Thread::recei_fro_logicthread_sht21_air_sample_stop()
{
    //关闭定时器结束采样
    if(sht21_timer->isActive())
    {
        sht21_timer->stop();
        qDebug("sht21_timer stopped");
    }

    if(air_timer->isActive())
    {
        air_timer->stop();
        qDebug("air_timer stopped");
    }

    sht21_sample_stop = true;
    air_sample_stop = true;

    task_completed = SHT21_AIR_COMPLETED;
    emit send_to_logic_sht21_air_sample_complete(task_completed);

}

//定时器溢出信号的槽函数，用于控制采样频率
void Sht21_Air_Thread::sht21_timerUpdate()
{
    sht21_sample_start = true;
}

void Sht21_Air_Thread::air_timerUpdate()
{
    air_sample_start = true;
}
