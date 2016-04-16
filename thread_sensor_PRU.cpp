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


void PRUThread::recei_fro_logicthread_pru_sample_start(PRU_SAMPLE_START Pru_sample_start)
{
    pru_sample_start = Pru_sample_start;

    qDebug() << "pru_sample_start.display_size = " << pru_sample_start.display_size;
    qDebug() << "pru_sample_start.filename = " << pru_sample_start.filename;
    qDebug() << "pru_sample_start.sample_time_hours = " << pru_sample_start.sample_time_hours;
    qDebug() << "pru_sample_start.sample_time_minutes = " << pru_sample_start.sample_time_minutes;
    qDebug() << "pru_sample_start.sample_time_seconds = " << pru_sample_start.sample_time_seconds;
    qDebug() << "pru_sample_start.sample_freq = " << pru_sample_start.sample_freq;
    qDebug() << "pru_sample_start.AIN[0] = " << pru_sample_start.AIN[0];
    qDebug() << "pru_sample_start.AIN[1] = " << pru_sample_start.AIN[1];
    qDebug() << "pru_sample_start.AIN[2] = " << pru_sample_start.AIN[2];
    qDebug() << "pru_sample_start.AIN[3] = " << pru_sample_start.AIN[3];
    qDebug() << "pru_sample_start.AIN[4] = " << pru_sample_start.AIN[4];
    qDebug() << "pru_sample_start.AIN[5] = " << pru_sample_start.AIN[5];
    qDebug() << "pru_sample_start.AIN[6] = " << pru_sample_start.AIN[6];
    qDebug() << "pru_sample_start.AIN[7] = " << pru_sample_start.AIN[7];
    qDebug() << "pru_sample_start.AIN[8] = " << pru_sample_start.AIN[8];
    qDebug() << "pru_sample_start.AIN[9] = " << pru_sample_start.AIN[9];
    qDebug() << "pru_sample_start.AIN[10] = " << pru_sample_start.AIN[10];

}
