#ifndef THREAD_LOGIC_CONTRL_H
#define THREAD_LOGIC_CONTRL_H


#include <QThread>
#include <QString>
#include "qcommon.h"

/*********************串口线程*****************************/
class LogicThread : public QThread
{
    Q_OBJECT
public:
    explicit LogicThread(QObject *parent = 0);
    void stop();

protected:
    void run();

private:
    volatile bool stopped;

signals:
    //逻辑线程发送此信号给串口线程通知串口线程开始数据采集
    void send_to_uartthread_sample_start(UART_SAMPLE_START Uart_sample_start);

    //逻辑线程发送此信号给串口线程通知串口线程停止数据采集
    void send_to_uartthread_sample_stop();

public slots:

};

#endif // THREAD_LOGIC_CONTRL_H
