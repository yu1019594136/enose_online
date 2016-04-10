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

    while(!stopped)
    {
        usleep(500000);
        qDebug("thread logic\n");
    }

    stopped = false;
}

void LogicThread::stop()
{
    stopped = true;
}
