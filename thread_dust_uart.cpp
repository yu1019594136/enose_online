#include "thread_dust_uart.h"
#include <stdio.h>
#include <QDebug>
#include <unistd.h>
#include <malloc.h>
#include "uart.h"

//串口数据的内存块，用于选项卡绘图的数据，该数据块循环更新，为全局变量
UART_PLOT_DATA_BUF uart_plot_data_buf;

UartThread::UartThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
    uart_sample_flag = false;
    fd_close_flag = false;
    start_point_captured = false;

    filename = NULL;
    fp_data_file = NULL;
    fd_uart = -1;

    //串口数据的内存块，初始化
    uart_plot_data_buf.p_uart_data = NULL;
    uart_plot_data_buf.data_size = 0;
    uart_plot_data_buf.index = 0;

    //缓冲区初始化
//    for(i = 0; i < 10; i++)
//        uart_data_buffer[i] = 0;
    memset(uart_data_buffer, 0, sizeof(char) * 5);

    //实例化定时器
    uart_timer = new QTimer(this);
    connect(uart_timer, SIGNAL(timeout()), this, SLOT(recei_fro_logicthread_sample_stop()));
}

void UartThread::run()
{
    qDebug("uart thread starts\n");

    while(!stopped)
    {
        if(uart_sample_flag)//开始采集数据
        {
            /* 1,确定数据起点 */
            while(!start_point_captured)
            {
                /* 数据起始点测试原理：
                   准备好四个char型存储单元，假设首地址为p
                   1、每次读取到数据后先存放到p+3;
                   2、将后三个数据依次往前移动一位，原来的p位置数据将被覆盖掉;
                   3、检测第p, p+1, p+2位置的数据是否分别为0xFF, 0X18,0X00;
                   4、如果是，则结束循环，数据起始点确定；否则开始读取下一个数据，并跳转步骤1
                */

                //读取到数据后先存放到p+3
                read(fd_uart, uart_data_buffer+3, 1);
                qDebug("uart_data_buffer[3] = 0x%x", uart_data_buffer[3]);

                //后三个数据依次往前移动一位
                uart_data_buffer[0] = uart_data_buffer[1];
                uart_data_buffer[1] = uart_data_buffer[2];
                uart_data_buffer[2] = uart_data_buffer[3];

                //检测第p, p+1, p+2位置的数据是否分别为0xFF, 0X18,0X00;
                if(uart_data_buffer[0] == 0xff && uart_data_buffer[1] == 0x18 && uart_data_buffer[2] == 0x00)
                {
                    start_point_captured = true;
                    qDebug("start point captured");
                }
            }

            /*
                数据转换关系：第一个uchar数据为小数点之前的有效数据，第二个uchar数据为小数点之后的有效数据
                为了避免操作浮点数，将第一个数据乘以100再加上第二个数据，因此转换后的数据为原始数据的100倍大小
            */
            read(fd_uart, uart_data_buffer+3, 1);//有效数据
            uart_plot_data_buf.p_uart_data[uart_plot_data_buf.index] = uart_data_buffer[3] * 100;
            read(fd_uart, uart_data_buffer+4, 1);//有效数据
            uart_plot_data_buf.p_uart_data[uart_plot_data_buf.index++] += uart_data_buffer[4];

            //2，保存数据到文件和链表
            fprintf(fp_data_file, "%u\n", uart_plot_data_buf.p_uart_data[uart_plot_data_buf.index]);

            //qDebug("data[%lu] = %u", uart_plot_data_buf.index, uart_plot_data_buf.p_uart_data[uart_plot_data_buf.index]);

            //循环更新，防止越界访问
            if(uart_plot_data_buf.index == uart_sample_start.display_size)
            {
                uart_plot_data_buf.index = 0;
                qDebug("cycle...........");
            }

            //3,发送信号更新绘图

            //无效数据丢弃
            read(fd_uart, uart_data_buffer, 1);
            read(fd_uart, uart_data_buffer, 1);
            read(fd_uart, uart_data_buffer, 1);
            read(fd_uart, uart_data_buffer, 1);
            read(fd_uart, uart_data_buffer, 1);
            read(fd_uart, uart_data_buffer, 1);
            read(fd_uart, uart_data_buffer, 1);

            //test
//            read(fd_uart, uart_data_buffer, 1);
//            qDebug("uart_data_buffer[0] = 0x%x", uart_data_buffer[0]);

        }

        if(fd_close_flag)
        {
            /* 采样结束后应该关闭文件句柄 */
            //关闭设备文件句柄
            if(fd_uart != -1)
            {
                close(fd_uart);
                fd_uart = -1;

                qDebug() << "close(fd_uart)";
            }
            //关闭文件名char句柄
            if(fp_data_file != NULL)
            {
                fclose(fp_data_file);
                fp_data_file = NULL;

                qDebug() << "fclose(fp_data_file)";
            }

            fd_close_flag = false;//无需重复关闭,该开关，在stop槽函数中变为true
        }

    }

    stopped = false;

    //停止线程时必须释放申请的内存空间
    if(uart_plot_data_buf.p_uart_data != NULL)
    {
        qDebug("clear memory");
        free(uart_plot_data_buf.p_uart_data);
    }
    uart_plot_data_buf.p_uart_data = NULL;

    //关闭设备文件句柄
    //有时候stop信号还没发过来，或者计时时间还未到，用户突然按下quit要退出程序，此时应该在退出程序前把未保存的数据（虽然不完整）任然保存到文件
    if(fd_uart != -1)
    {
        close(fd_uart);
        fd_uart = -1;

        qDebug() << "close(fd_uart), stop buttom is pressed when sample task is running";
    }
    //关闭文件名char句柄
    if(fp_data_file != NULL)
    {
        fclose(fp_data_file);
        fp_data_file = NULL;

        qDebug() << "fclose(fp_data_file), stop buttom is pressed when sample task is running";
    }

}

void UartThread::stop()
{
    //结束线程
    stopped = true;
}

//槽函数,响应逻辑线程的信号，开始采集数据
void UartThread::recei_fro_logicthread_sample_start(UART_SAMPLE_START Uart_sample_start)
{
    qDebug() << "Uart_sample.sample_mode = " << Uart_sample_start.sample_mode << "\n";
    qDebug() << "Uart_sample.sample_time = " << Uart_sample_start.sample_time << "\n";
    qDebug() << "Uart_sample.display_size = " << Uart_sample_start.display_size << "\n";
    qDebug() << "Uart_sample.filename = " << Uart_sample_start.filename << "\n";

    uart_sample_start = Uart_sample_start;

    /* 检查是否重新分配链表空间 */

    //逻辑线程要求的绘图尺寸和串口内存数据块大小不一致时必须重新分配内存块大小
    if(uart_sample_start.display_size != uart_plot_data_buf.data_size)
    {
        //释放原来的内存空间
        if(uart_plot_data_buf.p_uart_data != NULL)
        {
            free(uart_plot_data_buf.p_uart_data);
            qDebug() << "free last memory size";
        }

        //申请新的内存空间
        uart_plot_data_buf.p_uart_data = (unsigned short int *)malloc(sizeof(unsigned short int) * uart_sample_start.display_size);
        memset(uart_plot_data_buf.p_uart_data, 0, sizeof(unsigned short int) * uart_sample_start.display_size);//将分配的内存空间初始化为0

        uart_plot_data_buf.data_size = uart_sample_start.display_size;
        uart_plot_data_buf.index = 0;

        qDebug() << "get new memory size";
    }
    else//若大小相等，则内存块不需要重新申请，只需要将原来的内存空间写0即可，索引归0
    {
        memset(uart_plot_data_buf.p_uart_data, 0, sizeof(unsigned short int) * uart_sample_start.display_size);//将分配的内存空间初始化为0
        uart_plot_data_buf.index = 0;
        qDebug() << "memory size equals!";
    }

    //准备数据保存文件名char
    ba = uart_sample_start.filename.toLatin1();
    filename = ba.data();
    fp_data_file = fopen(filename , "w");

    //打开设备文件句柄,并进行配置,波特率，数据格式
    fd_uart = open_port();
    configure_port(fd_uart);

    //定时模式下设定定时器
    if(uart_sample_start.sample_mode == TIMING)
    {
        uart_timer->start(uart_sample_start.sample_time * 1000);
        //qDebug() << "TIMING mode, uart_timer start";
    }
//    else
//    {
//        qDebug() << "MONITOR mode, uart_timer do not start";
//    }

    //启动采样
    uart_sample_flag = true;
    fd_close_flag = false;
    start_point_captured = false;

    qDebug() << "slot function: uart sample start";

}

//槽函数,监测模式下响应逻辑线程的信号，停止采集数据
//      定时模式下，响应定时器溢出信号，停止数据采集
void UartThread::recei_fro_logicthread_sample_stop()
{
    //定时模式下关闭定时器
    if(uart_sample_start.sample_mode == TIMING)
    {
        if(uart_timer->isActive())
        {
            uart_timer->stop();
            //qDebug() << "uart timer stop";
        }

    }

    //停止采集数据
    uart_sample_flag = false;
    fd_close_flag = true;
    start_point_captured = true;

    qDebug() << "slot function: uart sample stop";
}
