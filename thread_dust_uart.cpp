#include "thread_dust_uart.h"
#include <QDebug>
#include <unistd.h>

UartThread::UartThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
}

void UartThread::run()
{
    qDebug("uart thread starts\n");

    while(!stopped)
    {
        usleep(500000);
        qDebug("thread uart\n");
    }

    stopped = false;
}

void UartThread::stop()
{
    stopped = true;
}

//槽函数,响应逻辑线程的信号，开始采集数据
void UartThread::recei_fro_logicthread_sample_start(UART_SAMPLE Uart_sample)
{
    qDebug("Uart_sample.sample_mode = %d\nUart_sample.sample_time = %d\n", Uart_sample.sample_mode, Uart_sample.sample_time);
    qDebug() << "Uart_sample.filename = " << Uart_sample.filename << "\n";
}
