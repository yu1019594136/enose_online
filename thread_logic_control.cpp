#include "thread_logic_contrl.h"
#include "beep.h"
#include <QDebug>
#include <unistd.h>
#include <QDateTime>

//读取界面参数并发通知逻辑线程开始数据采集
extern GUI_Para gui_para;

//系统配置参数，保存于文件
SYS_Para sys_para;

//该全局变量向逻辑线程表明，当前应该发send_to_GUI_enbale_start()通知GUI
//线程使能start按钮，准备下一次采样，还是发send_to_GUI_quit_application()
//通知GUI线程退出应用程序。
extern bool quit_flag;

LogicThread::LogicThread(QObject *parent) :
    QThread(parent)
{
    stopped = false;
    task_result = 0;
    quit_flag = false;
    record_para_to_file = false;
    beep_on_flag = false;

    logictimer = new QTimer (this);
    connect(logictimer, SIGNAL(timeout()), this, SLOT(logictimer_timeout()));

    beeptimer = new QTimer (this);
    connect(beeptimer, SIGNAL(timeout()), this, SLOT(beeptimer_timeout()));
}

void LogicThread::run()
{
    qDebug("logic thread starts");

    beep_init();

    usleep(100000);

    while(!stopped)
    {
        if(beep_on_flag)
        {
            beeptimer->start(sys_para.beep_time);
            sys_para.beep_counts = sys_para.beep_counts * 2 - 1;//算上鸣叫和不鸣叫的定时总次数

            beep_on_flag = false;
        }

        if(record_para_to_file)
        {
            record_para_to_file = false;
            record_GUI_para_to_file();
        }

    }

    stopped = false;

    beep_close();

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
    if(gui_para.sample_mode == TIMING)
    {
        if(logictimer->isActive())
        {
            qDebug() << "logictimer time out!";
            logictimer->stop();
        }
    }
    else if(gui_para.sample_mode == MONITOR)//仅仅监测模式下逻辑线程需要发信号去停止PRU线程
    {
        emit send_to_pruthread_sample_stop();//通知pru线程停止采集数据
    }

    emit send_to_uartthread_sample_stop();//通知串口线程停止数据采集

    emit send_to_sht21_air_thread_sample_stop();//通知sht21_air线程停止采集温湿度数据和空气质量数据
}

void LogicThread::beeptimer_timeout()
{
    if(sys_para.beep_counts == 0)
    {
        beeptimer->stop();
        beep_off();
    }
    else
    {
        if(sys_para.beep_counts % 2 == 0)
            beep_off();
        else
            beep_on();

        sys_para.beep_counts--;
    }

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

    if((task_result & UART_COMPLETED) && (task_result & PRU_COMPLETED) && (task_result & SHT21_AIR_COMPLETED))//任务都完成了
    {
        if(quit_flag)//采集任务中途直接按下quit按钮
        {
            emit send_to_GUI_quit_application();
        }
        else//采集任务结束后按下stop按钮
        {
            emit send_to_GUI_enbale_start();
            quit_flag = false;

            //驱动蜂鸣器
            qDebug("all sample task are completed, beep on!");
            beep_on_flag = true;
        }

    }

}

//接收和解析界面参数，开始通知其他线程采集数据
void LogicThread::recei_parse_GUI_data()
{
    FILE *fp_plot_para = NULL;

    datetime = QDateTime::currentDateTime();

    //读取绘图相关配置参数
    if((fp_plot_para = fopen(CONFIG_PARA_FILE, "r")) != NULL)
    {
        fscanf(fp_plot_para, "sys_para.pru_plot_time_span =\t%u\n\n", &sys_para.pru_plot_time_span);
        fscanf(fp_plot_para, "sys_para.uart_data_plot_height =\t%u\n\n", &sys_para.uart_data_plot_height);
        fscanf(fp_plot_para, "sys_para.sht21_data_plot_height =\t%u\n\n", &sys_para.sht21_data_plot_height);
        fscanf(fp_plot_para, "sys_para.air_data_plot_height =\t%u\n\n", &sys_para.air_data_plot_height);
        fscanf(fp_plot_para, "sys_para.beep_counts =\t%u\n\n", &sys_para.beep_counts);
        fscanf(fp_plot_para, "sys_para.beep_time =\t%u\n\n", &sys_para.beep_time);
        sys_para.filepath = FILEPATH;

        fclose(fp_plot_para);
        fp_plot_para = NULL;
    }
    else
    {
        sys_para.pru_plot_time_span = PRU_PLOT_TIME_SPAN;
        sys_para.uart_data_plot_height = UART_DATA_PLOT_HEIGHT;
        sys_para.sht21_data_plot_height = SHT21_DATA_PLOT_HEIGHT;
        sys_para.air_data_plot_height = AIR_DATA_PLOT_HEIGHT;
        sys_para.beep_counts = BEEP_COUNTS;
        sys_para.beep_time = BEEP_TIME;
        sys_para.filepath = FILEPATH;
    }

    //通知串口线程开始采集粉尘传感器数据
    uart_sample_start.display_size = gui_para.plot_data_num_dust;
    uart_sample_start.filename = sys_para.filepath + datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string + QString("_dust.txt");

    //通知PRU线程开始采集SENSOR数据
    pru_sample_start.display_size = gui_para.plot_data_num_sensor;
    pru_sample_start.filename = sys_para.filepath + datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string + QString("_sensor_");//PRU线程内部再追加序号尾缀
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
    sht21_air_sample_start.air_filename = sys_para.filepath + datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string +QString("_air_qulity.txt");

    sht21_air_sample_start.sht21_display_size = gui_para.plot_data_num_sht21;
    sht21_air_sample_start.sht21_period = gui_para.sample_period_sht21_s;
    sht21_air_sample_start.sht21_filename = sys_para.filepath + datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string +QString("_sht21.txt");

    emit send_to_uartthread_sample_start(uart_sample_start);//通知串口线程开始数据采集

    emit send_to_pruthread_pru_sample_start(pru_sample_start);//通知pru线程开始采集数据

    emit send_to_sht21_air_thread_sample_start(sht21_air_sample_start);//通知sht21_air线程开始采集温湿度数据和空气质量数据

    if(gui_para.sample_mode == TIMING)//定时模式下开启计时
        logictimer->start(1000 * (gui_para.sample_time_hours * 3600 + gui_para.sample_time_minutes * 60 + gui_para.sample_time_seconds));//逻辑线程启动定时器开始计时
    else//监测模式下
    {
        if(logictimer->isActive())
            logictimer->stop();
    }

    beep_on_flag = false;
    record_para_to_file = true;

    //任务结果状态回归
    task_result = UNDEFINED;
}

void LogicThread::record_GUI_para_to_file()
{
    FILE *fp_record_para = NULL;
    QString filename_QString = sys_para.filepath + datetime.toString("yyyy.MM.dd-hh_mm_ss_") + QString("para.txt");
    char *filename_char, *str;
    QByteArray ba_filename, ba_str;
    unsigned int i = 0;

    ba_filename = filename_QString.toLatin1();
    filename_char = ba_filename.data();

    if((fp_record_para = fopen(filename_char, "w")) != NULL)
    {
        fprintf(fp_record_para, "sample_time_hours =\t%d\n", gui_para.sample_time_hours);
        fprintf(fp_record_para, "sample_time_minutes =\t%d\n", gui_para.sample_time_minutes);
        fprintf(fp_record_para, "sample_time_seconds =\t%d\n\n", gui_para.sample_time_seconds);

        fprintf(fp_record_para, "plot_data_num_dust =\t%d\n", gui_para.plot_data_num_dust);
        fprintf(fp_record_para, "plot_data_num_air =\t%d\n", gui_para.plot_data_num_air);
        fprintf(fp_record_para, "plot_data_num_sht21 =\t%d\n", gui_para.plot_data_num_sht21);
        fprintf(fp_record_para, "plot_data_num_sensor =\t%d\n\n", gui_para.plot_data_num_sensor);

        fprintf(fp_record_para, "sample_period_air_ms =\t%d\n", gui_para.sample_period_air_ms);
        fprintf(fp_record_para, "sample_period_sht21_s =\t%d\n", gui_para.sample_period_sht21_s);
        fprintf(fp_record_para, "sample_freq_sensor =\t%lf\n\n", gui_para.sample_freq_sensor);

        ba_str = gui_para.user_string.toLatin1();
        str = ba_str.data();
        fprintf(fp_record_para, "user_string =\t%s\n", str);

        if(gui_para.data_save_mode == LOCAL_HOST)
            fprintf(fp_record_para, "data_save_mode =\tLOCAL_HOST\n\n");
        else if(gui_para.data_save_mode == USB_DEVICE)
            fprintf(fp_record_para, "data_save_mode =\tUSB_DEVICE\n\n");
        else if(gui_para.data_save_mode == UPLOAD)
            fprintf(fp_record_para, "data_save_mode =\tUPLOAD\n\n");

        for(i = 0; i < 11; i++)
        {
            fprintf(fp_record_para, "AIN[%u] =\t%u\n", i, gui_para.AIN[i]);
        }

        if(gui_para.sample_mode == TIMING)
            fprintf(fp_record_para, "\nsample_mode =\tTIMING\n");
        else if(gui_para.sample_mode == MONITOR)
            fprintf(fp_record_para, "\nsample_mode =\tMONITOR\n");

        fclose(fp_record_para);
        fp_record_para = NULL;
    }
}
