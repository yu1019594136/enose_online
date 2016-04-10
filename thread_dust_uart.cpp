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
