#ifndef THREAD_LOGIC_CONTRL_H
#define THREAD_LOGIC_CONTRL_H


#include <QThread>
#include <QString>
#include <QTimer>
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
    QTimer *logictimer; //用于逻辑控制的定时器，定时模式使用

signals:
    //逻辑线程发送此信号给串口线程通知串口线程开始数据采集
    void send_to_uartthread_sample_start(UART_SAMPLE_START Uart_sample_start);

    //逻辑线程发送此信号给串口线程通知串口线程停止数据采集
    void send_to_uartthread_sample_stop();

public slots:

private slots:
    //定时器时间到达时用于发送其他线程通知其停止数据采集，或者更新本定时器定时时间
    void logictimer_timeout();

};

#endif // THREAD_LOGIC_CONTRL_H
