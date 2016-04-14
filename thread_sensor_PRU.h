#ifndef THREAD_SENSOR_PRU_H
#define THREAD_SENSOR_PRU_H


#include <QThread>
#include <QString>
#include <QTimer>
#include "qcommon.h"

/*********************串口线程*****************************/
class PRUThread : public QThread
{
    Q_OBJECT
public:
    explicit PRUThread(QObject *parent = 0);
    void stop();

protected:
    void run();

private:
    volatile bool stopped;

signals:


public slots:

private slots:

};

#endif // THREAD_SENSOR_PRU_H
