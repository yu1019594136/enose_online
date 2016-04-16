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
    PRU_SAMPLE_START pru_sample_start;

signals:

public slots:
    //接收逻辑线程发过来的参数
    void recei_fro_logicthread_pru_sample_start(PRU_SAMPLE_START Pru_sample_start);

private slots:

};

#endif // THREAD_SENSOR_PRU_H
