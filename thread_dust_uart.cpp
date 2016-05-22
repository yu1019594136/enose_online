#include "thread_dust_uart.h"
#include <stdio.h>
#include <QDebug>
#include <unistd.h>
#include <malloc.h>
#include "uart.h"
#include "queue.h"
#include <QMutex>

extern QMutex mutex_compress;
//压缩任务队列
extern Queue *compress_queue;

//串口数据的内存块，用于选项卡绘图的数据，该数据块循环更新，为全局变量
PLOT_DATA_BUF uart_plot_data_buf;

UartThread::UartThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
    uart_sample_flag = false;
    fd_close_flag = false;
    start_point_captured = false;
    Task_completed = UNDEFINED;

    filename = NULL;
    fp_data_file = NULL;
    fd_uart = -1;

    //串口数据的内存块，初始化
    uart_plot_data_buf.p_data = NULL;
    uart_plot_data_buf.buf_size = 0;
    uart_plot_data_buf.index = 0;
    uart_plot_data_buf.valid_data_size = 0;
    uart_plot_data_buf.pp_data = NULL;
    uart_plot_data_buf.pp_data_float = NULL;
    uart_plot_data_buf.pp_data_int = NULL;

    //缓冲区初始化
    memset(uart_data_buffer, 0, sizeof(char) * 5);

}

void UartThread::run()
{
    qDebug("uart thread starts");

    while(!stopped)
    {
        if(uart_sample_flag)//开始采集数据
        {
            /* 1,确定数据起点 */
            while(!start_point_captured)
            {
                /* 数据起始点测试原理：
                   准备好四个char型存储单元，假设首地址为p
                   (1)、每次读取到数据后先存放到p+3;
                   (2)、将后三个数据依次往前移动一位，原来的p位置数据将被覆盖掉;
                   (3)、检测第p, p+1, p+2位置的数据是否分别为0xFF, 0X18,0X00;
                   (4)、如果是，则结束循环，数据起始点确定；否则开始读取下一个数据，并跳转步骤1
                */

                //读取到数据后先存放到p+3
                read(fd_uart, uart_data_buffer+3, 1);
                //qDebug("uart_data_buffer[3] = 0x%x", uart_data_buffer[3]);

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
            uart_plot_data_buf.p_data[uart_plot_data_buf.index] = uart_data_buffer[3] * 100;
            read(fd_uart, uart_data_buffer+4, 1);//有效数据
            uart_plot_data_buf.p_data[uart_plot_data_buf.index] += uart_data_buffer[4];

            read(fd_uart, uart_data_buffer, 1);//将所有的数据处理穿插在连续的阻塞读取中，充分利用等待时间

            //2，保存数据到文件和链表
            fprintf(fp_data_file, "%u\n", uart_plot_data_buf.p_data[uart_plot_data_buf.index++]);//index++, 不断更新有效数据起点,index指向最旧的数据

            //qDebug("data[%lu] = %u", uart_plot_data_buf.index, uart_plot_data_buf.p_uart_data[uart_plot_data_buf.index]);

            read(fd_uart, uart_data_buffer, 1);//将所有的数据处理穿插在连续的阻塞读取中，充分利用等待时间

            //循环更新，防止越界访问
            if(uart_plot_data_buf.index == uart_sample_start.display_size)
            {
                uart_plot_data_buf.index = 0;
                //qDebug("a display_size cycle.......");
            }

            read(fd_uart, uart_data_buffer, 1);//将所有的数据处理穿插在连续的阻塞读取中，充分利用等待时间

            //3,发送信号更新绘图
            if(uart_plot_data_buf.valid_data_size < uart_plot_data_buf.buf_size)//不断更新有效数据大小，确定绘图数据个数
                uart_plot_data_buf.valid_data_size++;

            read(fd_uart, uart_data_buffer, 1);//将所有的数据处理穿插在连续的阻塞读取中，充分利用等待时间

            //4, 发送信号，驱动绘图选项卡进行绘图
            emit send_to_plot_uart_curve();

            //无效数据丢弃
            read(fd_uart, uart_data_buffer, 1);
            read(fd_uart, uart_data_buffer + 1, 1);
            read(fd_uart, uart_data_buffer + 2, 1);

            //检测第p, p+1, p+2位置的数据是否分别为0xFF, 0X18,0X00;
            //读取完一次有效数据后，再一次校对数据起始点，如果校对正确则继续采样，否则进去最开始的死循环，重新捕获数据起始点
            if(uart_data_buffer[0] == 0xff && uart_data_buffer[1] == 0x18 && uart_data_buffer[2] == 0x00)
            {
                //qDebug("start point captured");
            }
            else
            {
                uart_plot_data_buf.index--;//将索引回退一个，去掉上一次的数据
                start_point_captured = false;//再次进入起始点捕获循环
                qDebug("start point lost, capture again");
            }

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

                qDebug() << "uart data is saved in " << uart_sample_start.filename;
            }

            fd_close_flag = false;//无需重复关闭,该开关，在stop槽函数中变为true

            mutex_compress.lock();
            InsertQueue(compress_queue, uart_sample_start.filename);
            mutex_compress.unlock();

            //通知逻辑线程串口数据采集完毕
            Task_completed = UART_COMPLETED;
            emit send_to_logic_uart_sample_complete(Task_completed);

        }

    }

    stopped = false;

    //停止线程时必须释放申请的内存空间
    if(uart_plot_data_buf.p_data != NULL)
    {
        qDebug("clear memory for uart plot when uart thread stop");
        free(uart_plot_data_buf.p_data);
    }
    uart_plot_data_buf.p_data = NULL;

    qDebug("uart thread stopped");

}

void UartThread::stop()
{
    //结束线程
    stopped = true;
}

//槽函数,响应逻辑线程的信号，开始采集数据
void UartThread::recei_fro_logicthread_sample_start(UART_SAMPLE_START Uart_sample_start)
{
    uart_sample_start = Uart_sample_start;
    qDebug() << "uart_sample.display_size = " << uart_sample_start.display_size << "\n";
    qDebug() << "uart_sample.filename = " << uart_sample_start.filename << "\n";

    /* 检查是否重新分配空间 */

    //逻辑线程要求的绘图尺寸和串口内存数据块大小不一致时必须重新分配内存块大小
    if(uart_sample_start.display_size != uart_plot_data_buf.buf_size)
    {
        //释放原来的内存空间
        if(uart_plot_data_buf.p_data != NULL)
        {
            free(uart_plot_data_buf.p_data);
            qDebug() << "uart free last memory size";
        }

        //申请新的内存空间
        uart_plot_data_buf.p_data = (unsigned short int *)malloc(sizeof(unsigned short int) * uart_sample_start.display_size);
        memset(uart_plot_data_buf.p_data, 0, sizeof(unsigned short int) * uart_sample_start.display_size);//将分配的内存空间初始化为0

        qDebug() << "uart get new memory size";
    }
    else//若大小相等，则内存块不需要重新申请，只需要将原来的内存空间写0即可，索引归0
    {
        memset(uart_plot_data_buf.p_data, 0, sizeof(unsigned short int) * uart_sample_start.display_size);//将分配的内存空间初始化为0
        qDebug() << "uart memory size equals!";
    }

    uart_plot_data_buf.filename = uart_sample_start.filename;
    uart_plot_data_buf.valid_data_size = 0;
    uart_plot_data_buf.index = 0;
    uart_plot_data_buf.buf_size = uart_sample_start.display_size;

    //准备数据保存文件名char
    ba = uart_sample_start.filename.toLatin1();
    filename = ba.data();
    fp_data_file = fopen(filename , "w");

    //打开设备文件句柄,并进行配置,波特率，数据格式
    fd_uart = open_port();
    configure_port(fd_uart);

    //启动采样
    uart_sample_flag = true;
    fd_close_flag = false;
    start_point_captured = false;
    Task_completed = UNDEFINED;

    qDebug() << "slot function: uart sample start";

}

//槽函数,监测模式下响应逻辑线程的信号，停止采集数据
//      定时模式下，响应定时器溢出信号，停止数据采集
void UartThread::recei_fro_logicthread_sample_stop()
{
    //停止采集数据
    uart_sample_flag = false;
    fd_close_flag = true;
    start_point_captured = true;

    qDebug() << "slot function: uart sample stop";
}
