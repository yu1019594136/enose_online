#ifndef THREAD_DUST_UART_H
#define THREAD_DUST_UART_H

#include <QByteArray>
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
    volatile bool uart_sample_flag;//是否开始采集数据
    volatile bool fd_close_flag;//是否关闭文件句柄
    volatile bool start_point_captured;//是否捕获到数据起始点

    FILE *fp_data_file;   //数据文件句柄
    int fd_uart;        //设备文件句柄
    char *filename;     //保存文件名称
    QByteArray ba;
    UART_SAMPLE_START uart_sample_start;    //线程自身保存参数的结构体
    unsigned char uart_data_buffer[5];  //数据缓冲区，用于确定采样起始以及提取有效数据

    int Task_completed;//采集任务结果报告

signals:
    //通知逻辑线程串口数据采集完成
    void send_to_logic_uart_sample_complete(int);

    //通知绘图选项卡开始绘制曲线
    void send_to_plot_uart_curve();

public slots:
    //响应逻辑线程的信号，开始采集数据
    void recei_fro_logicthread_sample_start(UART_SAMPLE_START Uart_sample_start);

    //响应停止串口数据采集的槽函数
    void recei_fro_logicthread_sample_stop();

};


#endif // THREAD_DUST_UART_H
