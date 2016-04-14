#include "thread_logic_contrl.h"
#include <QDebug>
#include <unistd.h>
#include <QDateTime>

LogicThread::LogicThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;

    logictimer = new QTimer (this);
    connect(logictimer, SIGNAL(timeout()), this, SLOT(logictimer_timeout()));

}

void LogicThread::run()
{
    qDebug("logic thread starts\n");

    UART_SAMPLE_START uart_sample_start;

    uart_sample_start.display_size = 20;
    QDateTime datetime = QDateTime::currentDateTime();
    uart_sample_start.filename = datetime.toString("yyyy.MM.dd-hh_mm_ss_") + QString("test") + ".txt";

    //等待其他线程准备好
    usleep(100000);
    qDebug() << "100ms passed!";

    emit send_to_uartthread_sample_start(uart_sample_start);//通知串口线程开始数据采集
    logictimer->start(1000 * 30);//逻辑线程启动定时器开始计时

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

void LogicThread::logictimer_timeout()
{

    if(logictimer->isActive())
    {
        qDebug() << "logictimer time out!";
        logictimer->stop();
    }
    emit send_to_uartthread_sample_stop();
}
