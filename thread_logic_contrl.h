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
    int task_result;//该变量用于记录任务完成状况
    //当bit1 = 1,表示串口采集数据完成
    //当bit2 = 1, 表示PRU数据采集完成
    //当bit3 = 1, 表示SPI数据采集完成
    //当bit4 = 1，表示SHT21数据采集完成

    UART_SAMPLE_START uart_sample_start;
    PRU_SAMPLE_START pru_sample_start;

signals:
    //逻辑线程发送此信号给串口线程通知串口线程开始数据采集
    void send_to_uartthread_sample_start(UART_SAMPLE_START Uart_sample_start);

    //逻辑线程发送此信号给串口线程通知串口线程停止数据采集
    void send_to_uartthread_sample_stop();

    //逻辑线程通知pru线程开始采集数据
    void send_to_pruthread_pru_sample_start(PRU_SAMPLE_START Pru_sample_start);

    //逻辑线程发送此信号给pru线程通知pru线程停止数据采集
    void send_to_pruthread_sample_stop();

    //逻辑线程发送信号给GUI线程，所有任务已经结束，并通知其使能start按钮
    void send_to_GUI_enbale_start();

public slots:
    //接受各个线程的采集任务结果报告
    void receive_task_report(int Task_finished_report);

    //接收和解析界面参数
    void recei_parse_GUI_data();

private slots:
    //定时器时间到达时用于发送其他线程通知其停止数据采集，或者更新本定时器定时时间
    void logictimer_timeout();

};

#endif // THREAD_LOGIC_CONTRL_H
