#include "thread_logic_contrl.h"
#include <QDebug>
#include <unistd.h>

LogicThread::LogicThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
}

void LogicThread::run()
{
    qDebug("logic thread starts\n");

    UART_SAMPLE uart_sample;
    uart_sample.sample_mode = TIMING;
    uart_sample.sample_time = 30;//单位s
    uart_sample.filename = QString("test");

    while(!stopped)
    {
        usleep(1000000);
        qDebug("thread logic\n");

        emit send_to_uartthread_sample_start(uart_sample);
    }

    stopped = false;
}

void LogicThread::stop()
{
    stopped = true;
}
