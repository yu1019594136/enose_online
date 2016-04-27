#ifndef QCOMMON_H
#define QCOMMON_H


#include <QString>

/* 程序运行是所需要的一些配置文件存放路径 */
#define E_NOSE_ONLINE_LOGO                  "/root/qi_enose_online/E-nose_online_Logo.png"
#define PRUADC_BIN                          "/root/qi_enose_online/PRU_Code/PRUADC.bin"
#define PRUClock_BIN                        "/root/qi_enose_online/PRU_Code/PRUClock.bin"


#define PRU_PLOT_TIME_SPAN                  100 //pru绘图曲线的时间跨度，表示整条曲线从最左端的采样点到最右端采样点之间的时间间隔, per second
#define UART_DATA_PLOT_HEIGHT               400 //
#define PRU_MEMORY_SIZE                     8000000 //PRU允许使用的内存空间大小，注意：最大不能超过8000000 bytes

//数据采集模式
enum SAMPLE_MODE{
    TIMING = 0,
    MONITOR
};

//数据采集模式
enum SAMPLE_STATUS{
    START = 0,
    STOP
};

//数据保存模式
enum DATA_SAVE{
    LOCAL_HOST = 0,
    USB_DEVICE = 1,
    UPLOAD
};

//采集任务类型
enum TASK_TYPE{
    UNDEFINED       = 0,
    UART_COMPLETED  = 1,
    PRU_COMPLETED   = 2,
    SPI_COMPLETED   = 4,
    SHT21_COMPLETED = 8
};

//逻辑线程通知串口线程开始采集数据
typedef struct{
    unsigned long display_size; //显示数据点个数
    QString filename;   //保存数据文件名称
} UART_SAMPLE_START;

//串口数据的内存块，用于选项卡绘图的数据，该数据块循环更新，为全局变量
typedef struct{
    unsigned short int *p_data;    //内存块首地址
    unsigned short int **pp_data;    //
    unsigned long int buf_size;        //内存块大小
    unsigned long int index;            //当前最新的数据项索引，循环更新时需要用到的索引,index指向最旧的数据，index-1表示最新的一个数据,index在0-data_size之间循环
    unsigned long int valid_data_size;  //数据块中有效数据的个数,有效数据个数从0开始逐渐增长，最后保持在data_size大小，
    QString filename;   //绘图时在绘图选项卡上面标明数据文件名称
} PLOT_DATA_BUF;

//逻辑线程通知PRU线程开始采集数据
typedef struct{
    unsigned long display_size; //显示数据点个数
    QString filename;   //保存数据文件名称, 仅仅需要时间戳和userstring, 文件序号需要PRU线程自己计算和添加
    float sample_freq;//每个通道的采样频率
    bool AIN[11];//sensor采样通道情况,true表示采集, false表示不采集
    unsigned int sample_time_hours;//每个通道的时间长度
    unsigned int sample_time_minutes;//每个通道的时间长度
    unsigned int sample_time_seconds;//每个通道的时间长度
} PRU_SAMPLE_START;


typedef struct{
    //采样时间
    int sample_time_hours;
    int sample_time_minutes;
    int sample_time_seconds;

    //绘图选项卡的显示数据个数
    int plot_data_num_dust;
    int plot_data_num_air;
    int plot_data_num_sht21;
    int plot_data_num_sensor;

    //采样周期/频率
    unsigned int sample_period_air_ms;
    unsigned int sample_period_sht21_s;
    double sample_freq_sensor;

    QString user_string;
    int data_save_mode;

    //sensor采样通道情况,true表示采集, false表示不采集
    bool AIN[11];

    int sample_mode;

} GUI_Para;

//系统配置参数，保存于文件
typedef struct{
    unsigned int time_span_pru_plot;//绘制PRU数据曲线时，数据点横跨的时间尺度，此参数用于计算降频频率
    unsigned int pru_memory_size; //PRU允许使用的内存空间大小，注意：最大不能超过8000000 bytes

} SYS_Para;

#endif // QCOMMON_H
