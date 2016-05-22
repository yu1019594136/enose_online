#include "thread_logic_contrl.h"
#include "beep.h"
#include <QDebug>
#include <unistd.h>
#include <QDateTime>

#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QMessageBox>
#include <QStringList>

#include <QMutex>

QMutex mutex_compress;

//压缩任务队列
Queue *compress_queue = NULL;
int compress_data_queue_max_len;//队列最大长度

//上传任务队列
Queue *upload_queue = NULL;
int upload_file_queue_max_len;//队列最大长度

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
    upload_flag = true;

    logictimer = new QTimer (this);
    connect(logictimer, SIGNAL(timeout()), this, SLOT(logictimer_timeout()));

    beeptimer = new QTimer (this);
    connect(beeptimer, SIGNAL(timeout()), this, SLOT(beeptimer_timeout()));

    //压缩任务队列初始化
    Queue_Init(&compress_queue, QString(COMPRESS_TASK_FILE));
    //上传任务队列初始化
    Queue_Init(&upload_queue, QString(UPLOAD_TASK_FILE));

    compress_process = new QProcess();
    compress_process->setReadChannelMode(QProcess::SeparateChannels);
    compress_process->setReadChannel(QProcess::StandardOutput);
    connect(compress_process, SIGNAL(started()), this, SLOT(compress_process_started()));
    connect(compress_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(compress_process_error(QProcess::ProcessError)));
    connect(compress_process, SIGNAL(readyReadStandardOutput()), this, SLOT(compress_process_readyreadoutput()));
    connect(compress_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(compress_process_finished(int,QProcess::ExitStatus)));
    connect(this, SIGNAL(terminate_compress_process()), compress_process, SLOT(terminate()));

    connect(this, SIGNAL(start_compress_data()), this, SLOT(compress_queue_check()), Qt::QueuedConnection);

    upload_process = new QProcess();
    upload_process->setReadChannelMode(QProcess::SeparateChannels);
    upload_process->setReadChannel(QProcess::StandardOutput);
    connect(upload_process, SIGNAL(started()), this, SLOT(upload_process_started()));
    connect(upload_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(upload_process_error(QProcess::ProcessError)));
    connect(upload_process, SIGNAL(readyReadStandardOutput()), this, SLOT(upload_process_readyreadoutput()));
    connect(upload_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(upload_process_finished(int,QProcess::ExitStatus)));
    connect(this, SIGNAL(terminate_upload_process()), upload_process, SLOT(terminate()));

    connect(this, SIGNAL(start_upload_file(int)), this, SLOT(upload_queue_check(int)), Qt::QueuedConnection);
}

void LogicThread::run()
{
    qDebug("logic thread starts");

    beep_init();

    //程序启动后自动启动进程查看队列中是否有未完成任务
    emit start_compress_data();

    int mode = UPLOAD_OPEN;//UPLOAD_OPEN for net test
    emit start_upload_file(mode);

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

    //将队列保存到文件，以便下次程序启动时可以恢复本次未完成的任务;释放队列本身
    Queue_Save(&compress_queue, COMPRESS_TASK_FILE);
    Queue_Save(&upload_queue, UPLOAD_TASK_FILE);

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

        if(quit_flag)//定时模式下，如果中途按下quit按钮，pru线程也需要发信号停止
        {
            emit send_to_pruthread_sample_stop();//通知pru线程停止采集数据
        }
    }
    else if(gui_para.sample_mode == MONITOR)//监测模式下逻辑线程需要发信号去停止PRU线程
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
        //采集任务中途直接按下quit按钮,
        //中途按下quit按钮时不再进行数据压缩和文件上传，未完成的任务将在程序下次启动是自动继续进行
        if(quit_flag)
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

            //进行数据压缩
            emit start_compress_data();
        }

    }

}

//接收和解析界面参数，开始通知其他线程采集数据
void LogicThread::recei_parse_GUI_data()
{
    FILE *fp_plot_para = NULL;
    char usb_path[1024] = {0};

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
        fscanf(fp_plot_para, "sys_para.USB_path =\t%s\n\n", usb_path);
        sys_para.USB_path = usb_path;

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
        sys_para.USB_path = FILEPATH;
        sys_para.filepath = FILEPATH;
    }

    if(gui_para.data_save_mode == USB_DEVICE)
    {
        sys_para.filepath = sys_para.USB_path;
        qDebug() << "sys_para.USB_path = " << sys_para.filepath;
    }
    else
        sys_para.filepath = FILEPATH;

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
    QString filename_QString = sys_para.filepath + datetime.toString("yyyy.MM.dd-hh_mm_ss_") + gui_para.user_string + QString("_para.txt");
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

        //将参数文件列入压缩队列
        mutex_compress.lock();
        InsertQueue(compress_queue, filename_QString);
        mutex_compress.unlock();
    }
}

void Queue_Init(Queue **queue, QString filepath)
{
    *queue = InitQueue();

    QFile file(filepath);
    //方式：Append为追加，WriteOnly，ReadOnly
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug() << "can not find file:" << filepath << "\nqueue is empty.";
    }
    else
    {
        QTextStream in(&file);

        while(!in.atEnd())
        {
            //insert to queue
            InsertQueue(*queue, in.readLine());
        }
        //show queue length
        qDebug() << "queue was initiated by file " << filepath << "\nlen = " << GetQueueLength(*queue);

        file.close();
    }
}

void Queue_Save(Queue **queue, QString filepath)
{
    QFile file(filepath);
    //方式：Append为追加，WriteOnly，ReadOnly
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        qDebug() << "can not find file:" << filepath << "\nqueue can not be saved.";
    }
    else
    {
        QTextStream out(&file);
        int len = GetQueueLength(*queue);

        while(len > 0)
        {
            out << *GetQueueHead(*queue) << "\n";
            DelQueue(*queue);
            len--;
        }

        qDebug() << "A queue is saved to file " << filepath;

        file.close();
    }

}

/****************************************************/
void LogicThread::compress_process_started()
{
    qDebug() << "compress_process_started";
}

void LogicThread::compress_process_error(QProcess::ProcessError processerror)
{
    qDebug() << "compress_process error = " << processerror << endl;
    emit terminate_compress_process();
    qDebug() << "terminate the failed compress process";
}

void LogicThread::compress_process_readyreadoutput()
{
    qDebug() << compress_process->readAllStandardOutput();
}

void LogicThread::compress_process_finished(int i,QProcess::ExitStatus exitstate)
{
    QString filename;
    int mode;

    qDebug() << "compress_process finish, exit code = " << i << endl;

    if(exitstate == QProcess::NormalExit)//返回代码为0表示正常退出
    {
        qDebug() << "compress process exited normally.";

        //从队列中移除已经完成的任务，并将其转移到upload_file_queue队列
        mutex_compress.lock();
        for(int i = 0; i < compress_data_queue_max_len; i++)
        {
            filename = *GetQueueHead(compress_queue) + ".tar.gz";
            DelQueue(compress_queue);
            InsertQueue(upload_queue, filename);
        }
        mutex_compress.unlock();

        //start new compress task
        emit start_compress_data();

        //start upload task
        qDebug() << "upload queue len = " << GetQueueLength(upload_queue);
        mode = UPLOAD_FILE;
        emit start_upload_file(mode);
    }

}

/****************************************************/
void LogicThread::upload_process_started()
{
    qDebug() << "upload_process_started";
}

void LogicThread::upload_process_error(QProcess::ProcessError processerror)
{
    qDebug() << "upload_process error = " << processerror << endl;
    emit terminate_upload_process();
    qDebug() << "terminate the failed upload process";
}

void LogicThread::upload_process_readyreadoutput()
{
    qDebug() << upload_process->readAllStandardOutput();
}

void LogicThread::upload_process_finished(int i,QProcess::ExitStatus exitstate)
{
    int mode;

    qDebug() << "upload_process finish, exit code = " << i << endl;

    //if(exitstate == QProcess::NormalExit)
    if(i == 0)
    {
        qDebug() << "upload process exited normally.";

        //从队列中移除已经完成的任务
        for(int i = 0; i < upload_file_queue_max_len; i++)
        {
            DelQueue(upload_queue);
        }

        //start upload task
        qDebug() << "upload queue len = " << GetQueueLength(upload_queue);
        mode = UPLOAD_FILE;
        emit start_upload_file(mode);//本次传输成功，紧接着启动一下一次传输
    }
    else if(i == 1)//IP不存在
    {
        upload_flag = false;

        qDebug() << "Host is unreachable";
        //此时不应该继续启动上传文件脚本，应该等用户配置好之后手动点击复选框时再来触发启动脚本
    }
    else if(i == 2)//IP存在, 某文件名对应的文件不存在
    {
        upload_flag = false;
        qDebug() << "File is not exits";
        //此时不应该继续启动上传文件脚本，应该等用户配置好之后手动点击复选框时再来触发启动脚本
    }
    else if(i == 3)//IP存在, 路径不可达
    {
        upload_flag = false;
        qDebug() << "ACC or FilePath is wrong";
        //此时不应该继续启动上传文件脚本，应该等用户配置好之后手动点击复选框时再来触发启动脚本
    }
    else if(i == 4)//网络测试成功
    {
        upload_flag = true;
        qDebug() << "Net test succeed";
        mode = UPLOAD_FILE;
        emit start_upload_file(mode);//网络测试成功紧接着启动一下一次传输
    }

    //向GUI线程报告脚本运行结果
    emit send_to_GUI_netreport(i);
}

void LogicThread::compress_queue_check()
{
    if(compress_process->state() == QProcess::NotRunning)//进程状态是空闲
    {
        mutex_compress.lock();
        int len = GetQueueLength(compress_queue);

        if(len < compress_data_queue_max_len)//检查队列长度是否达到上限
        {
            qDebug("compress queue len(%d) is smaller than %d", len, compress_data_queue_max_len);
        }
        else//若同时满足，则将队列任务取出来并启动进程
        {
            QStringList arguments;
            arguments = GetQueue_ahead_element(compress_queue, compress_data_queue_max_len);
            //qDebug() << arguments;

            compress_process->start(COMPRESS_SCRIPTS, arguments);
            qDebug("%d compress task in compress queue\n%d compress task ahead were submitted to compress_data.sh\n", len, compress_data_queue_max_len);
        }
        mutex_compress.unlock();
    }
    else
    {
        qDebug() << "compress_process state is Runing, wait ....";
    }
}

void LogicThread::upload_queue_check(int operation_code)
{
    if(operation_code == UPLOAD_CLOSE)
    {
        upload_flag = false;
    }
    else if(operation_code == UPLOAD_FILE)
    {
        if(upload_flag)//
        {
            if(upload_process->state() == QProcess::NotRunning)//进程状态是空闲
            {
                int len = GetQueueLength(upload_queue);

                if(len < upload_file_queue_max_len)//检查队列长度是否达到上限
                {
                    qDebug("upload queue len(%d) is smaller than %d", len, upload_file_queue_max_len);
                }
                else//若同时满足，则将队列任务取出来并启动进程
                {
                    QStringList arguments;
                    arguments = GetQueue_ahead_element(upload_queue, upload_file_queue_max_len);
                    //qDebug() << arguments;

                    upload_process->start(UPLOAD_SCRIPTS, arguments);
                    qDebug("%d upload task in upload queue\n%d upload task ahead were submitted to upload_data.sh\n", len, upload_file_queue_max_len);
                }
            }
            else
            {
                qDebug() << "upload_process state is Runing, wait ....";
            }
        }
        else//upload_flag = false
        {
            /*
             * upload_flag = false
                有可能是内部上传任务失败而将其置为false
                有可能是外部用户取消upload复选框的勾选而将其置为false
                最后上传任务取消
            */
            qDebug() << "upload_flag = false. upload task is cancelled.";
        }

    }
    else if(operation_code == UPLOAD_OPEN)//just for net test
    {
        upload_flag = true;

        if(upload_process->state() == QProcess::NotRunning)//进程状态是空闲
        {
            upload_process->start(UPLOAD_SCRIPTS);//不传入文件名，仅仅做网络ping测试
        }
        else
        {
            qDebug() << "upload_process state is Runing, wait for net test ....";
        }
    }
}
