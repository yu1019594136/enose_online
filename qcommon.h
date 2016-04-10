#ifndef QCOMMON_H
#define QCOMMON_H


#include <QString>

/* 程序运行是所需要的一些配置文件存放路径 */

//控制蜂鸣器的IO口编号
#define BEEP_PORT               30


//数据采集模式
enum SAMPLE_MODE{
    TIMING = 0,
    MONITOR
};

//逻辑线程通知串口线程开始采集数据
typedef struct{
    int sample_mode;    //采样方式，定时或者长期监测
    int sample_time;    //定时方式下的采样时间长度
    QString filename;   //保存数据文件名称
} UART_SAMPLE;


#endif // QCOMMON_H
