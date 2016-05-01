#include "thread_logic_contrl.h"
#include <QDebug>
#include <unistd.h>
#include <QDateTime>

//读取界面参数并发通知逻辑线程开始数据采集
extern GUI_Para gui_para;

//系统配置参数，保存于文件
SYS_Para sys_para;

LogicThread::LogicThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
    task_result = 0;

    logictimer = new QTimer (this);
    connect(logictimer, SIGNAL(timeout()), this, SLOT(logictimer_timeout()));

}

void LogicThread::run()
{
    qDebug("logic thread starts");

    /* 从系统参数文件中读取配置参数，保存到sys_para */
    // do something....

    usleep(100000);

    while(!stopped)
    {

    }

    stopped = false;

    qDebug("logic thread stopped");
}

void LogicThread::stop()
{
    stopped = true;
}

//逻辑定时器溢出后，会给其他工作线程发送信号（pru线程除外)，并等待其他线程的结束工作汇报。
//定时器溢出或者stop被按下都将产生相同的动作：结束各个采样任务，因此定时器溢出的槽函数和stop按钮按下后触发的逻辑线程槽函数可以为同一个
void LogicThread::logictimer_timeout()
{

    if(logictimer->isActive())
    {
        qDebug() << "logictimer time out!";
        logictimer->stop();
    }

    //emit send_to_uartthread_sample_stop();//通知串口线程停止数据采集

    //emit send_to_pruthread_sample_stop();//通知pru线程停止采集数据

    emit send_to_sht21_air_thread_sample_stop();//通知sht21_air线程停止采集温湿度数据和空气质量数据
}

//其他线程的结束工作汇报。
void LogicThread::receive_task_report(int Task_finished_report)
{
    if(Task_finished_report == UART_COMPLETED)
    {
        task_result = task_result | Task_finished_report;
        qDebug() << "UART task is completed. Task_finished_report = " << Task_finished_report;
    }
    else if(Task_finished_report == PRU_COMPLETED)
    {
        task_result = task_result | Task_finished_report;
        qDebug() << "PRU task is completed. Task_finished_report = " << Task_finished_report;
    }
    else if(Task_finished_report == SHT21_AIR_COMPLETED)
    {
        task_result = task_result | Task_finished_report;
        qDebug() << "SHT21_AIR task is completed. Task_finished_report = " << Task_finished_report;
    }
    else
    {
        qDebug() << "a wrong report is received by logic";
    }

    task_result = task_result | UART_COMPLETED;
    task_result = task_result | PRU_COMPLETED;
    if((task_result & UART_COMPLETED) && (task_result & PRU_COMPLETED) && (task_result & SHT21_AIR_COMPLETED))//任务都完成了
    {
        emit send_to_GUI_enbale_start();
    }

}

//接收和解析界面参数，开始通知其他线程采集数据
void LogicThread::recei_parse_GUI_data()
{
    QDateTime datetime = QDateTime::currentDateTime();

    //通知串口线程开始采集粉尘传感器数据
    uart_sample_start.display_size = gui_para.plot_data_num_dust;
    uart_sample_start.filename = datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string + QString("_dust.txt");

    //通知PRU线程开始采集SENSOR数据
    pru_sample_start.display_size = gui_para.plot_data_num_sensor;
    pru_sample_start.filename = datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string + QString("_sensor_");//PRU线程内部再追加序号尾缀
    pru_sample_start.sample_time_hours = gui_para.sample_time_hours;
    pru_sample_start.sample_time_minutes = gui_para.sample_time_minutes;
    pru_sample_start.sample_time_seconds = gui_para.sample_time_seconds;
    pru_sample_start.sample_freq = gui_para.sample_freq_sensor;
    pru_sample_start.AIN[0] = gui_para.AIN[0];
    pru_sample_start.AIN[1] = gui_para.AIN[1];
    pru_sample_start.AIN[2] = gui_para.AIN[2];
    pru_sample_start.AIN[3] = gui_para.AIN[3];
    pru_sample_start.AIN[4] = gui_para.AIN[4];
    pru_sample_start.AIN[5] = gui_para.AIN[5];
    pru_sample_start.AIN[6] = gui_para.AIN[6];
    pru_sample_start.AIN[7] = gui_para.AIN[7];
    pru_sample_start.AIN[8] = gui_para.AIN[8];
    pru_sample_start.AIN[9] = gui_para.AIN[9];
    pru_sample_start.AIN[10] = gui_para.AIN[10];
    pru_sample_start.sample_mode = gui_para.sample_mode;

    //通知sht21_air线程开始采集温湿度数据和空气质量数据
    sht21_air_sample_start.air_display_size = gui_para.plot_data_num_air;
    sht21_air_sample_start.air_period = gui_para.sample_period_air_ms;
    sht21_air_sample_start.air_filename = datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string +QString("_air_qulity.txt");

    sht21_air_sample_start.sht21_display_size = gui_para.plot_data_num_sht21;
    sht21_air_sample_start.sht21_period = gui_para.sample_period_sht21_s;
    sht21_air_sample_start.sht21_filename = datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string +QString("_sht21.txt");;

    //emit send_to_uartthread_sample_start(uart_sample_start);//通知串口线程开始数据采集

    //emit send_to_pruthread_pru_sample_start(pru_sample_start);//通知pru线程开始采集数据

    emit send_to_sht21_air_thread_sample_start(sht21_air_sample_start);//通知sht21_air线程开始采集温湿度数据和空气质量数据

    if(gui_para.sample_mode == TIMING)//定时模式下开启计时
        logictimer->start(1000 * (gui_para.sample_time_hours * 3600 + gui_para.sample_time_minutes * 60 + gui_para.sample_time_seconds));//逻辑线程启动定时器开始计时
    else//监测模式下
    {
        if(logictimer->isActive())
            logictimer->stop();
    }

    //任务结果状态回归
    task_result = UNDEFINED;
}
