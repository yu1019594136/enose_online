#ifndef THREAD_SENSOR_PRU_H
#define THREAD_SENSOR_PRU_H

#include <QThread>
#include <QString>
#include <QTimer>
#include "qcommon.h"
#include "common.h"


/*-----------------pru-----------------*/
#define ADC_PRU_NUM	   0   // using PRU0 for the ADC capture
#define CLK_PRU_NUM	   1   // using PRU1 for the sample clock
#define MMAP0_LOC   "/sys/class/uio/uio0/maps/map0/"
#define MMAP1_LOC   "/sys/class/uio/uio0/maps/map1/"

#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)

#define MAX_BUFFER_SIZE 4000000 //unsigned short int

/* FREQ_xHz = delay
如何根据所需要的Hz，计算出n？
delay + 3 = (5 * 10^7) / f

或者
f = (5 * 10^7) / (delay + 3)

f单位是Hz
*/
enum FREQUENCY {    // measured and calibrated, but can be calculated
    FREQ_12_5MHz =  1,
    FREQ_6_25MHz =  5,
    FREQ_5MHz    =  7,
    FREQ_3_85MHz = 10,
    FREQ_1MHz   =  47,
    FREQ_500kHz =  97,
    FREQ_250kHz = 245,
    FREQ_100kHz = 497,
    FREQ_50kHz = 997,
    FREQ_40kHz = 1247,
    FREQ_25kHz = 1997,
    FREQ_10kHz = 4997,
    FREQ_5kHz =  9997,
    FREQ_4kHz = 12497,
    FREQ_2kHz = 24997,
    FREQ_1kHz = 49997
};

enum CONTROL {
    PAUSED = 0,
    RUNNING = 1,
    UPDATE = 3
};


/*********************串口线程*****************************/
class PRUThread : public QThread
{
    Q_OBJECT
public:
    explicit PRUThread(QObject *parent = 0);
    void stop();

protected:
    void run();

private:
    volatile bool stopped;
    volatile bool pru_sample_flag;
    PRU_SAMPLE_START pru_sample_start;
    int Task_completed;

    FILE *fp_data_file;   //数据文件句柄
    char *filename;     //保存文件名称
    QByteArray ba;
    //QString filename_serial;//
    //record sample times when task starts
    unsigned int serial;

    volatile bool pru_sample_end;

    //每阶段的采样时间长度
    unsigned int sample_time_per_section;

    unsigned int last_sample_time_per_section;
    unsigned int last_spiData_1;

signals:
    void send_to_logic_pru_sample_complete(int task_completed);

    //通知绘图选项卡开始绘制曲线
    void send_to_plot_pru_curve();

public slots:
    //接收逻辑线程发过来的参数
    void recei_fro_logicthread_pru_sample_start(PRU_SAMPLE_START Pru_sample_start);

private slots:

};


/* PRU初始化，下载PRU代码到Instruction data ram中 */
void PRU_init_loadcode();

// Short function to load a single unsigned int from a sysfs entry
unsigned int readFileValue(char filename[]);

/* 保存数据到文件 */
int save_and_plot_data(char * filename, unsigned int numberOutputSamples, unsigned int AIN_NUM, unsigned short int *P_Data, unsigned int Sample_time_per_section);

/* 拷贝数据到临时内存 */
int copy_data_to_buf(unsigned short int *p_data, unsigned int Size_byte);

#endif // THREAD_SENSOR_PRU_H
