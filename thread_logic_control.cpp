#include "thread_logic_contrl.h"
#include <QDebug>
#include <unistd.h>
#include <QDateTime>

LogicThread::LogicThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
}

void LogicThread::run()
{
    qDebug("logic thread starts\n");

    UART_SAMPLE_START uart_sample_start;

    uart_sample_start.sample_mode = MONITOR;
    uart_sample_start.sample_time = 15;//单位s
    uart_sample_start.display_size = 40;

    QDateTime datetime = QDateTime::currentDateTime();
    uart_sample_start.filename = datetime.toString("yyyy.MM.dd-hh_mm_ss_") + QString("test") + ".txt";

    //等待其他线程准备好
    usleep(100000);
    qDebug() << "100ms passed!";

    //emit send_to_uartthread_sample_start(uart_sample_start);

    //usleep(60 * 1000000);
    //emit send_to_uartthread_sample_stop();

    while(!stopped)
    {
    }

    stopped = false;

    qDebug("logic thread stopped");
}

void LogicThread::stop()
{
    stopped = true;
}
