#ifndef QCOMMON_H
#define QCOMMON_H


#include <QString>

/* 程序运行是所需要的一些配置文件存放路径 */


//数据采集模式
enum SAMPLE_MODE{
    TIMING = 0,
    MONITOR
};

//数据采集
enum SAMPLE_STATUS{
    START = 0,
    STOP
};

//逻辑线程通知串口线程开始采集数据
typedef struct{
    int sample_mode;    //采样方式，定时或者长期监测
    int sample_time;    //定时方式下的采样时间长度
    unsigned long display_size; //显示数据点个数
    QString filename;   //保存数据文件名称
} UART_SAMPLE_START;

//串口数据的内存块，用于选项卡绘图的数据，该数据块循环更新，为全局变量
typedef struct{
    unsigned short int *p_uart_data;    //内存块首地址
    unsigned long int data_size;            //内存块中数据个数
    unsigned long int index;                //当前最新的数据项索引，循环更新时需要用到的索引
} UART_PLOT_DATA_BUF;


#endif // QCOMMON_H
