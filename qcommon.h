#ifndef QCOMMON_H
#define QCOMMON_H


#include <QString>

/* 程序运行是所需要的一些配置文件存放路径 */
#define E_NOSE_ONLINE_LOGO_UART             "/root/qi_enose_online/image/E-nose_online_Logo_uart.png"
#define E_NOSE_ONLINE_LOGO_PRU              "/root/qi_enose_online/image/E-nose_online_Logo_pru.png"
#define E_NOSE_ONLINE_LOGO_AIR              "/root/qi_enose_online/image/E-nose_online_Logo_air.png"
#define E_NOSE_ONLINE_LOGO_SHT21            "/root/qi_enose_online/image/E-nose_online_Logo_sht21.png"
#define PRUADC_BIN                          "/root/qi_enose_online/PRU_Code/PRUADC.bin"
#define PRUClock_BIN                        "/root/qi_enose_online/PRU_Code/PRUClock.bin"
#define FILEPATH                            "/root/qi_enose_online/data/"
#define DEFAULT_PARA_FILE                   "/root/qi_enose_online/config/default_para.txt"
#define CONFIG_PARA_FILE                    "/root/qi_enose_online/config/config_para.txt"
#define COMPRESS_TASK_FILE                  "/root/qi_enose_online/config/compress_task.txt"
#define UPLOAD_TASK_FILE                    "/root/qi_enose_online/config/upload_task.txt"
#define COMPRESS_SCRIPTS                    "/root/qi_enose_online/scripts/compress_data.sh"
#define UPLOAD_SCRIPTS                      "/root/qi_enose_online/scripts/upload_file.sh"

#define PRU_PLOT_TIME_SPAN                  300 //pru绘图曲线的时间跨度，表示整条曲线从最左端的采样点到最右端采样点之间的时间间隔, per second
#define UART_DATA_PLOT_HEIGHT               600 //
#define AIR_DATA_PLOT_HEIGHT                100000 //空气质量原始数据为占空比，周期100ms，高电平时间越长空气质量越差
#define SHT21_DATA_PLOT_HEIGHT              100 //温湿度数据中，温度显示范围0-100°，湿度显示范围0%-100%
#define BEEP_COUNTS                         3 //鸣叫次数
#define BEEP_TIME                           500 //unit ms

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
};

//采集任务类型
enum TASK_TYPE{
    UNDEFINED               = 0,
    UART_COMPLETED          = 1,
    PRU_COMPLETED           = 2,
    SHT21_AIR_COMPLETED     = 4
};

//upload_file.sh运行模式
enum UPLOAD_SCRIPT_RUN_MODE{
    UPLOAD_OPEN = 0,
    UPLOAD_FILE = 1,
    UPLOAD_CLOSE
};

//逻辑线程通知串口线程开始采集数据
typedef struct{
    unsigned long display_size; //显示数据点个数
    QString filename;   //保存数据文件名称
} UART_SAMPLE_START;

//串口数据的内存块，用于选项卡绘图的数据，该数据块循环更新，为全局变量
typedef struct{
    unsigned short int *p_data;    //一维,内存块首地址
    unsigned short int **pp_data;    //二维内存块首地址
    float **pp_data_float;          //for float
    unsigned int **pp_data_int;     //for unsigned int
    unsigned long int buf_size;        //行内存块大小
    unsigned long int index;            //当前最新的数据项索引，循环更新时需要用到的索引,index指向最旧的数据，index-1表示最新的一个数据,index在0-data_size之间循环
    unsigned long int valid_data_size;  //数据块中每行有效数据的个数,有效数据个数从0开始逐渐增长，最后保持在data_size大小，
    QString filename;   //绘图时在绘图选项卡上面标明数据文件名称
} PLOT_DATA_BUF;

//逻辑线程通知PRU线程开始采集数据
typedef struct{
    unsigned long display_size; //显示数据点个数
    QString filename;   //保存数据文件名称, 仅仅需要时间戳和userstring, 文件序号需要PRU线程自己计算和添加
    float sample_freq;//每个通道的采样频率
    unsigned int AIN[11];//sensor采样通道情况,true表示采集, false表示不采集
    unsigned int sample_time_hours;//每个通道的时间长度
    unsigned int sample_time_minutes;//每个通道的时间长度
    unsigned int sample_time_seconds;//每个通道的时间长度
    int sample_mode;
} PRU_SAMPLE_START;

//sht21_air线程采样参数
typedef struct{
    unsigned long sht21_display_size; //显示数据点个数
    unsigned long air_display_size; //显示数据点个数
    unsigned long sht21_period;//单位ms，以100ms最小单位递增
    unsigned long air_period;//单位s，以1s最小单位递增
    QString sht21_filename;   //保存数据文件名称
    QString air_filename;   //保存数据文件名称
} SHT21_AIR_SAMPLE_START;

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
    int sample_period_air_ms;
    int sample_period_sht21_s;
    double sample_freq_sensor;

    QString user_string;
    int data_save_mode;

    //sensor采样通道情况,true表示采集, false表示不采集
    unsigned int AIN[11];

    int sample_mode;

} GUI_Para;

//系统配置参数，保存于文件
typedef struct{
    QString filepath;//数据文件保存的目录
    unsigned int uart_data_plot_height;
    unsigned int pru_plot_time_span;//绘制PRU数据曲线时，数据点横跨的时间尺度，此参数用于计算降频频率
    unsigned int air_data_plot_height;
    unsigned int sht21_data_plot_height;
    unsigned int beep_counts;
    unsigned int beep_time;
    QString USB_path;
} SYS_Para;

#endif // QCOMMON_H
