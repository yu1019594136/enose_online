#ifndef THREAD_DUST_UART_H
#define THREAD_DUST_UART_H

#include <QThread>
#include <QString>
#include "qcommon.h"

/*********************串口线程*****************************/
class UartThread : public QThread
{
    Q_OBJECT
public:
    explicit UartThread(QObject *parent = 0);
    void stop();

protected:
    void run();

private:
    volatile bool stopped;

signals:

public slots:
    //响应逻辑线程的信号，开始采集数据
    void recei_fro_logicthread_sample_start(UART_SAMPLE Uart_sample);

};


#endif // THREAD_DUST_UART_H
