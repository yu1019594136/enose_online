#ifndef THREAD_SHT21_AIR_H
#define THREAD_SHT21_AIR_H

#include <QByteArray>
#include <QThread>
#include <QString>
#include <QTimer>
#include "qcommon.h"

/*********************串口线程*****************************/
class Sht21_Air_Thread : public QThread
{
    Q_OBJECT
public:
    explicit Sht21_Air_Thread(QObject *parent = 0);
    void stop();

protected:
    void run();

private:
    volatile bool stopped;
    SHT21_AIR_SAMPLE_START sht21_air_sample_start;    //线程自身保存参数的结构体
    QTimer *sht21_timer;
    QTimer *air_timer;

    bool sht21_sample_start;
    bool air_sample_start;
    bool sht21_sample_stop;
    bool air_sample_stop;
    FILE *fp_sht21_data_file, *fp_air_data_file;   //数据文件句柄
    char *sht21_filename, *air_filename;     //保存文件名称
    QByteArray ba1, ba2;

    int task_completed;//采集任务结果报告

signals:
    //通知逻辑线程串口数据采集完成
    void send_to_logic_sht21_air_sample_complete(int Task_completed);

    //通知绘图选项卡开始绘制曲线
    void send_to_plot_sht21_curve();

    //通知绘图选项卡开始绘制曲线
    void send_to_plot_air_curve();

public slots:
    //响应逻辑线程的信号，开始采集数据
    void recei_fro_logicthread_sht21_air_sample_start(SHT21_AIR_SAMPLE_START Sht21_air_sample_start);

    //响应停止串口数据采集的槽函数
    void recei_fro_logicthread_sht21_air_sample_stop();

    //定时器溢出信号的槽函数，用于控制采样频率
    void sht21_timerUpdate();

    void air_timerUpdate();

};


#endif // THREAD_SHT21_AIR_H
