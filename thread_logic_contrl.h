#ifndef THREAD_LOGIC_CONTRL_H
#define THREAD_LOGIC_CONTRL_H


#include <QThread>
#include <QString>
#include <QTimer>
#include "qcommon.h"
#include <QDateTime>
#include "queue.h"
#include <QProcess>

/*********************串口线程*****************************/
class LogicThread : public QThread
{
    Q_OBJECT
public:
    explicit LogicThread(QObject *parent = 0);
    void stop();

protected:
    void run();
    void record_GUI_para_to_file();

private:
    volatile bool stopped;
    bool upload_flag;
    QTimer *logictimer; //用于逻辑控制的定时器，定时模式使用

    bool beep_on_flag;
    unsigned int beep_counts;
    unsigned int beep_time;
    QTimer *beeptimer;

    QDateTime datetime;
    bool record_para_to_file;

    int task_result;//该变量用于记录任务完成状况
    //当bit1 = 1,表示串口采集数据完成
    //当bit2 = 1, 表示PRU数据采集完成
    //当bit3 = 1, 表示SPI数据采集完成
    //当bit4 = 1，表示SHT21数据采集完成

    UART_SAMPLE_START uart_sample_start;
    PRU_SAMPLE_START pru_sample_start;
    SHT21_AIR_SAMPLE_START sht21_air_sample_start;

    QProcess *compress_process;
    QProcess *upload_process;

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

    //通知sht21_air线程停止采集温湿度数据和空气质量数据
    void send_to_sht21_air_thread_sample_stop();

    //通知sht21_air线程开始采集温湿度数据和空气质量数据
    void send_to_sht21_air_thread_sample_start(SHT21_AIR_SAMPLE_START Sht21_air_sample_start);

    //通知GUI线程退出应用程序
    void send_to_GUI_quit_application();

    void terminate_compress_process();
    void terminate_upload_process();

    void start_compress_data();
    void start_upload_file();

public slots:
    //接受各个线程的采集任务结果报告
    void receive_task_report(int Task_finished_report);

    //接收和解析界面参数
    void recei_parse_GUI_data();

    void compress_queue_check();
    void upload_queue_check();

private slots:
    //定时器时间到达时用于发送其他线程通知其停止数据采集，或者更新本定时器定时时间
    void logictimer_timeout();

    //beep on
    void beeptimer_timeout();

    /*
    将压缩数据和上传文件分开成两个步骤，分别用进程实现的原因，因为开发板空间有限，所
    以压缩数据必须要进行；而上传任务在室外环境下无法实现，但此时压缩数据功能仍然需要。
    */
    void compress_process_started();
    void compress_process_error(QProcess::ProcessError processerror);
    void compress_process_readyreadoutput();
    void compress_process_finished(int i,QProcess::ExitStatus exitstate);

    void upload_process_started();
    void upload_process_error(QProcess::ProcessError processerror);
    void upload_process_readyreadoutput();
    void upload_process_finished(int i,QProcess::ExitStatus exitstate);

};

//从文件读取任务，将任务放入队列管理
void Queue_Init(Queue **queue, QString filepath);

void Queue_Save(Queue **queue, QString filepath);

#endif // THREAD_LOGIC_CONTRL_H
