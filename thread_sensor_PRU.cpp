#include "thread_sensor_PRU.h"
#include <QDebug>
#include <unistd.h>
#include <QDateTime>

PRUThread::PRUThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;

}

void PRUThread::run()
{
    qDebug("PRU thread starts\n");

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


