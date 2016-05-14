#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>
#include <QWSServer>
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QMetaType>    //注册元类型

//注册元类型
Q_DECLARE_METATYPE(UART_SAMPLE_START)
Q_DECLARE_METATYPE(PRU_SAMPLE_START)
Q_DECLARE_METATYPE(SHT21_AIR_SAMPLE_START)

//读取界面参数并发通知逻辑线程开始数据采集
GUI_Para gui_para;

//该全局变量向逻辑线程表明，当前应该发send_to_GUI_enbale_start()通知GUI
//线程使能start按钮，准备下一次采样，还是发send_to_GUI_quit_application()
//通知GUI线程退出应用程序。
bool quit_flag = false;

//队列最大长度
extern unsigned int compress_data_queue_max_len;
extern unsigned int upload_file_queue_max_len;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //界面控件初始化
    GUI_init();

    /* 隐藏鼠标 */
    QWSServer::setCursorVisible(false);

    /* 启动程序时总是显示第一个选项卡 */
    ui->tabWidget->setCurrentIndex(0);

    /* 实例化一个tab页用于绘图 */
    uart_plot_widget = new Uart_Plot_Widget(this);
    ui->tabWidget->addTab(uart_plot_widget, "uart");

    /* 实例化一个tab页用于绘图 */
    pru_plot_widget = new PRU_Plot_Widget(this);
    ui->tabWidget->addTab(pru_plot_widget, "pru");

    /* 实例化一个tab页用于绘图 */
    air_plot_widget = new Air_Plot_Widget(this);
    ui->tabWidget->addTab(air_plot_widget, "air");

    /* 实例化一个tab页用于绘图 */
    sht21_plot_widget = new Sht21_Plot_Widget(this);
    ui->tabWidget->addTab(sht21_plot_widget, "sht21");

    /* 开机显示时间 */
    QDateTime datetime = QDateTime::currentDateTime();
    ui->datetime->setText(datetime.toString("yyyy.MM.dd hh:mm"));

    /* 实例化一个定时器并启动,用于更新时间 */
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    timer->start(1000 * 60);//一分钟更新一次时间

    //将参数界面2中的checkbox的槽函数连接
    connect(ui->checkBox, SIGNAL(stateChanged(int)), SLOT(checkbox_changed(int)));
    connect(ui->checkBox_2, SIGNAL(stateChanged(int)), SLOT(checkbox_2_changed(int)));
    connect(ui->checkBox_3, SIGNAL(stateChanged(int)), SLOT(checkbox_3_changed(int)));
    connect(ui->checkBox_4, SIGNAL(stateChanged(int)), SLOT(checkbox_4_changed(int)));
    connect(ui->checkBox_5, SIGNAL(stateChanged(int)), SLOT(checkbox_5_changed(int)));

    connect(ui->checkBox_6, SIGNAL(stateChanged(int)), SLOT(checkbox_6_changed(int)));
    connect(ui->checkBox_7, SIGNAL(stateChanged(int)), SLOT(checkbox_7_changed(int)));
    connect(ui->checkBox_8, SIGNAL(stateChanged(int)), SLOT(checkbox_8_changed(int)));
    connect(ui->checkBox_9, SIGNAL(stateChanged(int)), SLOT(checkbox_9_changed(int)));
    connect(ui->checkBox_10, SIGNAL(stateChanged(int)), SLOT(checkbox_10_changed(int)));
    connect(ui->checkBox_11, SIGNAL(stateChanged(int)), SLOT(checkbox_11_changed(int)));

    //使能START按钮，禁能STOP按钮
    ui->pushButton->setEnabled(true);
    ui->pushButton_2->setEnabled(false);

    /* 注册元类型 */
    qRegisterMetaType <UART_SAMPLE_START>("UART_SAMPLE_START");
    qRegisterMetaType <PRU_SAMPLE_START>("PRU_SAMPLE_START");
    qRegisterMetaType <SHT21_AIR_SAMPLE_START>("SHT21_AIR_SAMPLE_START");


    /* 实例化三个线程并启动,将三个子线程相关的信号关联到GUI主线程的槽函数 */
    uartthread = new UartThread ();
    logicthread = new LogicThread ();
    pruthread = new PRUThread ();
    sht21_air_thread = new Sht21_Air_Thread ();//用于采集温湿度数据和空气质量数据

    /* GUI线程发送信号通知逻辑线程开始解析数据并启动其他线程的采集任务 */
    connect(this, SIGNAL(send_to_logic_GUI_data()), logicthread, SLOT(recei_parse_GUI_data()), Qt::QueuedConnection);

    /* 逻辑线程发送信号通知串口线程开始采集数据 */
    connect(logicthread, SIGNAL(send_to_uartthread_sample_start(UART_SAMPLE_START)), uartthread, SLOT(recei_fro_logicthread_sample_start(UART_SAMPLE_START)), Qt::QueuedConnection);

    /* 逻辑线程发送信号通知串口线程停止采集数据 */
    connect(logicthread, SIGNAL(send_to_uartthread_sample_stop()), uartthread, SLOT(recei_fro_logicthread_sample_stop()), Qt::QueuedConnection);

    /* 串口线程通知逻辑线程数据采集完成 */
    connect(uartthread, SIGNAL(send_to_logic_uart_sample_complete(int)), logicthread, SLOT(receive_task_report(int)), Qt::QueuedConnection);
    connect(pruthread, SIGNAL(send_to_logic_pru_sample_complete(int)), logicthread, SLOT(receive_task_report(int)), Qt::QueuedConnection);

    /* 串口线程通知绘图选项卡进行绘图 */
    connect(uartthread, SIGNAL(send_to_plot_uart_curve()), uart_plot_widget, SLOT(recei_fro_uartthread()), Qt::QueuedConnection);

    /* pru线程通知绘图选项卡进行绘图 */
    connect(pruthread, SIGNAL(send_to_plot_pru_curve()), pru_plot_widget, SLOT(recei_fro_pruthread()), Qt::QueuedConnection);

    /* sht21_air线程通知sht21绘图选项卡进行绘图 */
    connect(sht21_air_thread, SIGNAL(send_to_plot_sht21_curve()), sht21_plot_widget, SLOT(recei_fro_sht21_air_thread_sht21_plot()), Qt::QueuedConnection);
    connect(sht21_air_thread, SIGNAL(send_to_plot_air_curve()), air_plot_widget, SLOT(recei_fro_sht21_air_thread_air_plot()), Qt::QueuedConnection);

    /* 逻辑线程通知pru线程开始采集数据 */
    connect(logicthread, SIGNAL(send_to_pruthread_pru_sample_start(PRU_SAMPLE_START)), pruthread, SLOT(recei_fro_logicthread_pru_sample_start(PRU_SAMPLE_START)), Qt::QueuedConnection);

    /* GUI线程通知逻辑线程stop按钮被按下 */
    connect(this, SIGNAL(send_to_logic_stop_pressed()), logicthread, SLOT(logictimer_timeout()), Qt::QueuedConnection);

    /* pru线程收到逻辑线程的信号后，执行结束pru采样任务的槽函数 */
    connect(logicthread, SIGNAL(send_to_pruthread_sample_stop()), pruthread, SLOT(recei_fro_logicthread_pru_sample_stop()), Qt::QueuedConnection);

    /* 逻辑线程发送信号给GUI线程，所有任务已经结束，并通知其使能start按钮 */
    connect(logicthread, SIGNAL(send_to_GUI_enbale_start()), this, SLOT(recei_fro_logicthread_enable_start()), Qt::QueuedConnection);

    /* 逻辑线程通知sht21_air线程开始采集数据 */
    connect(logicthread, SIGNAL(send_to_sht21_air_thread_sample_start(SHT21_AIR_SAMPLE_START)), sht21_air_thread, SLOT(recei_fro_logicthread_sht21_air_sample_start(SHT21_AIR_SAMPLE_START)), Qt::QueuedConnection);

    /* 逻辑线程通知sht21_air线程停止采集数据 */
    connect(logicthread, SIGNAL(send_to_sht21_air_thread_sample_stop()), sht21_air_thread, SLOT(recei_fro_logicthread_sht21_air_sample_stop()), Qt::QueuedConnection);

    /* sht21_air线程通知逻辑线程采样结束 */
    connect(sht21_air_thread, SIGNAL(send_to_logic_sht21_air_sample_complete(int)), logicthread, SLOT(receive_task_report(int)), Qt::QueuedConnection);

    /* quit按钮按下通知逻辑线程结束其他线程的采集任务 */
    connect(this, SIGNAL(send_to_logicthread_abort_sample_tasks()), logicthread, SLOT(logictimer_timeout()), Qt::QueuedConnection);
    connect(logicthread, SIGNAL(send_to_GUI_quit_application()), this, SLOT(recei_fro_logicthread_quit_application()), Qt::QueuedConnection);
    connect(this, SIGNAL(quit_application()), this, SLOT(recei_fro_logicthread_quit_application()), Qt::QueuedConnection);

    uartthread->start();
    logicthread->start();
    pruthread->start();
    sht21_air_thread->start();

}

MainWindow::~MainWindow()
{
    delete ui;
    delete uartthread;
    delete logicthread;
    delete sht21_air_thread;
}

void MainWindow::on_quit_clicked()
{
    ui->quit->setEnabled(false);

    if(ui->pushButton->isEnabled())//如果start按钮处于使能状态说明，当前没有采样任务，quit按钮按下后可以直接退出程序
    {
        emit quit_application();
        qDebug("quit application");
    }
    else//如果start按钮处于禁能状态说明，当前有采样任务正在进行，quit按钮按下后应该先通知逻辑线程终止各采集任务
    {
        quit_flag = true;
        emit send_to_logicthread_abort_sample_tasks();
        qDebug("abort sample tasks, then quit application");
    }

}

void MainWindow::recei_fro_logicthread_quit_application()
{
    if(logicthread->isRunning())
        logicthread->stop();
    while(!logicthread->isFinished());

    //检查线程是否为运行状态
    if(uartthread->isRunning())
        uartthread->stop();
    while(!uartthread->isFinished());//确认线程是否停止

    if(pruthread->isRunning())
        pruthread->stop();
    while(!pruthread->isFinished());

    if(sht21_air_thread->isRunning())
        sht21_air_thread->stop();
    while(!sht21_air_thread->isFinished());

    pid_t new_pid;
    char *const arg_shutdown[] = {"shutdown","-h","now",NULL};
    if(ui->checkBox_12->isChecked())//如果勾选了quit按钮旁边的复选框则退出应用程序后会自动关机
    {
        new_pid = fork();

        if(new_pid < 0)
            qDebug("fork() failed!\n");
        else if(new_pid == 0)//子进程运行内容
        {
            sleep(5);//seconds秒钟后关机
            if(execve("/sbin/shutdown", arg_shutdown, NULL) == -1)
            {
                qDebug("shutdown in 5 seconds");
            }
        }
        else//主进程运行内容
        {
            qDebug("Application quit!\n");
        }
    }

    /* 退出事件循环，结束程序 */
    QApplication *p;
    p->quit();
}

void MainWindow::timerUpdate()
{
    QDateTime datetime = QDateTime::currentDateTime();
    ui->datetime->setText(datetime.toString("yyyy.MM.dd hh:mm"));
}

//参数解析,读取界面参数并发通知逻辑线程开始数据采集
void MainWindow::on_pushButton_clicked()
{
    //采样模式解析
    if(ui->radioButton_3->isChecked())
    {
        gui_para.sample_mode = TIMING;
        qDebug() << "sample mode  = TIMING";
    }
    if(ui->radioButton_4->isChecked())
    {
        gui_para.sample_mode = MONITOR;
        qDebug() << "sample mode  = MONITOR";
    }

    gui_para.sample_time_hours = ui->spinBox->value();
    gui_para.sample_time_minutes = ui->spinBox_3->value();
    gui_para.sample_time_seconds = ui->spinBox_2->value();
    qDebug() << "sample time: "
             << gui_para.sample_time_hours << "hours "
             << gui_para.sample_time_minutes << "minutes "
             << gui_para.sample_time_seconds << "seconds";

    gui_para.plot_data_num_dust = ui->spinBox_8->value();
    gui_para.plot_data_num_air = ui->spinBox_9->value();
    gui_para.plot_data_num_sht21 = ui->spinBox_10->value();
    gui_para.plot_data_num_sensor = ui->spinBox_11->value();
    qDebug() << "plot_data_num_dust = " << gui_para.plot_data_num_dust;
    qDebug() << "plot_data_num_air = " << gui_para.plot_data_num_air;
    qDebug() << "plot_data_num_sht21 = " << gui_para.plot_data_num_sht21;
    qDebug() << "plot_data_num_sensor = " << gui_para.plot_data_num_sensor;

    gui_para.sample_period_air_ms = ui->spinBox_5->value();
    gui_para.sample_period_sht21_s = ui->spinBox_6->value();
    gui_para.sample_freq_sensor = ui->doubleSpinBox->value();
    qDebug() << "sample_period_air_ms = " << gui_para.sample_period_air_ms;
    qDebug() << "sample_period_sht21_s = " << gui_para.sample_period_sht21_s;
    qDebug() << "sample_freq_sensor = " << gui_para.sample_freq_sensor;

    gui_para.user_string = ui->lineEdit->text();
    qDebug() << "user_string = " << gui_para.user_string;

    if(ui->radioButton_5->isChecked())
    {
        gui_para.data_save_mode = LOCAL_HOST;
        qDebug() << "data save mode = LOCAL_HOST";
    }
    if(ui->radioButton->isChecked())
    {
        gui_para.data_save_mode = USB_DEVICE;
        qDebug() << "data save mode = USB_DEVICE";
    }
    if(ui->radioButton_2->isChecked())
    {
        gui_para.data_save_mode = UPLOAD;
        qDebug() << "data save mode = UPLOAD";
    }

    emit send_to_logic_GUI_data();

    ui->pushButton->setEnabled(false);//禁能start按钮
    ui->pushButton_2->setEnabled(true);//使能stop按钮
}

//如果选择了定时模式，则要把hours minutes, seconds输入框使能
void MainWindow::on_radioButton_3_clicked()
{
    qDebug() << "H:M:S is enabled";
    ui->spinBox->setEnabled(true);
    ui->spinBox_2->setEnabled(true);
    ui->spinBox_3->setEnabled(true);
}

//如果选择了监测模式，则要把hours minutes, seconds输入框禁能
void MainWindow::on_radioButton_4_clicked()
{
    qDebug() << "H:M:S is disabled";
    ui->spinBox->setEnabled(false);
    ui->spinBox_2->setEnabled(false);
    ui->spinBox_3->setEnabled(false);
}

//将参数界面2中的checkbox的槽函数连接
void MainWindow::checkbox_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[7] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[7] = true;
    }
}

void MainWindow::checkbox_2_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[5] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[5] = true;
    }
}

void MainWindow::checkbox_3_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[0] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[0] = true;
    }
}

void MainWindow::checkbox_4_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[1] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[1] = true;
    }
}

void MainWindow::checkbox_5_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[2] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[2] = true;
    }
}

void MainWindow::checkbox_6_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[6] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[6] = true;
    }
}

void MainWindow::checkbox_7_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[3] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[3] = true;
    }
}

void MainWindow::checkbox_8_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[4] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[4] = true;
    }
}

void MainWindow::checkbox_9_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[8] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[8] = true;
    }
}

void MainWindow::checkbox_10_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[9] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[9] = true;
    }
}

void MainWindow::checkbox_11_changed(int state)
{
    if(state == Qt::Unchecked)
    {
        gui_para.AIN[10] = false;
    }
    else if(state == Qt::Checked)
    {
        gui_para.AIN[10] = true;
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    //通知逻辑线程stop按钮被按下
    emit send_to_logic_stop_pressed();

    //同时禁能stop按钮
    ui->pushButton_2->setEnabled(false);
}

void MainWindow::recei_fro_logicthread_enable_start()
{
    //使能start按钮
    ui->pushButton->setEnabled(true);
    ui->pushButton_2->setEnabled(false);
    qDebug("enable start button, and disable stop button");
}


void MainWindow::GUI_init()//从默认参数文件读取参数初始化界面
{
    FILE *fp_default_para = NULL;
    char temp_str[50] = {0};
    char str[1024] = {0};
    unsigned int i = 0;

    //1、程序使用内置参数配置GUI_para或者使用默认参数文件来配置GUI_para
    if((fp_default_para = fopen(DEFAULT_PARA_FILE, "r")) != NULL)//使用默认参数文件来配置GUI_para
    {
        fscanf(fp_default_para, "sample_time_hours =\t%d\n", &gui_para.sample_time_hours);
        fscanf(fp_default_para, "sample_time_minutes =\t%d\n", &gui_para.sample_time_minutes);
        fscanf(fp_default_para, "sample_time_seconds =\t%d\n\n", &gui_para.sample_time_seconds);

        fscanf(fp_default_para, "plot_data_num_dust =\t%d\n", &gui_para.plot_data_num_dust);
        fscanf(fp_default_para, "plot_data_num_air =\t%d\n", &gui_para.plot_data_num_air);
        fscanf(fp_default_para, "plot_data_num_sht21 =\t%d\n", &gui_para.plot_data_num_sht21);
        fscanf(fp_default_para, "plot_data_num_sensor =\t%d\n\n", &gui_para.plot_data_num_sensor);

        fscanf(fp_default_para, "sample_period_air_ms =\t%d\n", &gui_para.sample_period_air_ms);
        fscanf(fp_default_para, "sample_period_sht21_s =\t%d\n", &gui_para.sample_period_sht21_s);
        fscanf(fp_default_para, "sample_freq_sensor =\t%lf\n\n", &gui_para.sample_freq_sensor);

        fscanf(fp_default_para, "user_string =\t%s\n", str);
        gui_para.user_string = str;
        fscanf(fp_default_para, "data_save_mode =\t%d\n\n", &gui_para.data_save_mode);

        for(i = 0; i < 11; i++)
        {
            fscanf(fp_default_para, "%s =\t%u\n", temp_str, &gui_para.AIN[i]);
        }

        fscanf(fp_default_para, "\nsample_mode =\t%d\n\n", &gui_para.sample_mode);

        fscanf(fp_default_para, "compress_data_queue_max_len =\t%u\n", &compress_data_queue_max_len);
        fscanf(fp_default_para, "upload_file_queue_max_len =\t%u\n\n", &upload_file_queue_max_len);

        fclose(fp_default_para);
    }
    else//程序使用内置参数配置GUI_para
    {
        gui_para.sample_time_hours = 0;
        gui_para.sample_time_minutes = 3;
        gui_para.sample_time_seconds = 5;

        gui_para.plot_data_num_dust = 300;
        gui_para.plot_data_num_air = 600;
        gui_para.plot_data_num_sht21 = 100;
        gui_para.plot_data_num_sensor = 1000;

        gui_para.sample_period_air_ms = 100;
        gui_para.sample_period_sht21_s = 1;
        gui_para.sample_freq_sensor = 5000.0;

        gui_para.user_string = QString("zhouyu");
        gui_para.data_save_mode = 0;

        gui_para.AIN[0] = 0;
        gui_para.AIN[1] = 0;
        gui_para.AIN[2] = 1;
        gui_para.AIN[3] = 1;
        gui_para.AIN[4] = 1;
        gui_para.AIN[5] = 1;
        gui_para.AIN[6] = 1;
        gui_para.AIN[7] = 1;
        gui_para.AIN[8] = 1;
        gui_para.AIN[9] = 0;
        gui_para.AIN[10] = 1;

        gui_para.sample_mode = 0;

        compress_data_queue_max_len = 1;
        upload_file_queue_max_len = 5;

    }

    //2、根据GUI_para来设置GUI控件参数
    ui->spinBox->setValue(gui_para.sample_time_hours);
    ui->spinBox_3->setValue(gui_para.sample_time_minutes);
    ui->spinBox_2->setValue(gui_para.sample_time_seconds);

    ui->spinBox_8->setValue(gui_para.plot_data_num_dust);
    ui->spinBox_9->setValue(gui_para.plot_data_num_air);
    ui->spinBox_10->setValue(gui_para.plot_data_num_sht21);
    ui->spinBox_11->setValue(gui_para.plot_data_num_sensor);

    ui->spinBox_5->setValue(gui_para.sample_period_air_ms);
    ui->spinBox_6->setValue(gui_para.sample_period_sht21_s);

    //doublespinbox补充配置代码
    ui->doubleSpinBox->setMinimum(0.0);
    ui->doubleSpinBox->setMaximum(5000);
    ui->doubleSpinBox->setSingleStep(0.01);
    ui->doubleSpinBox->setValue(gui_para.sample_freq_sensor);

    ui->lineEdit->setText(gui_para.user_string);

    if(gui_para.data_save_mode == LOCAL_HOST)
    {
        ui->radioButton_5->setChecked(true);
        ui->radioButton->setChecked(false);
        ui->radioButton_2->setChecked(false);
    }
    if(gui_para.data_save_mode == USB_DEVICE)
    {
        ui->radioButton_5->setChecked(false);
        ui->radioButton->setChecked(true);
        ui->radioButton_2->setChecked(false);
    }
    if(gui_para.data_save_mode == UPLOAD)
    {
        ui->radioButton_5->setChecked(false);
        ui->radioButton->setChecked(false);
        ui->radioButton_2->setChecked(true);
    }

    if(gui_para.AIN[0] == 1)
        ui->checkBox_3->setChecked(true);
    else
        ui->checkBox_3->setChecked(false);

    if(gui_para.AIN[1] == 1)
        ui->checkBox_4->setChecked(true);
    else
        ui->checkBox_4->setChecked(false);

    if(gui_para.AIN[2] == 1)
        ui->checkBox_5->setChecked(true);
    else
        ui->checkBox_5->setChecked(false);

    if(gui_para.AIN[3] == 1)
        ui->checkBox_7->setChecked(true);
    else
        ui->checkBox_7->setChecked(false);

    if(gui_para.AIN[4] == 1)
        ui->checkBox_8->setChecked(true);
    else
        ui->checkBox_8->setChecked(false);

    if(gui_para.AIN[5] == 1)
        ui->checkBox_2->setChecked(true);
    else
        ui->checkBox_2->setChecked(false);

    if(gui_para.AIN[6] == 1)
        ui->checkBox_6->setChecked(true);
    else
        ui->checkBox_6->setChecked(false);

    if(gui_para.AIN[7] == 1)
        ui->checkBox->setChecked(true);
    else
        ui->checkBox->setChecked(false);

    if(gui_para.AIN[8] == 1)
        ui->checkBox_9->setChecked(true);
    else
        ui->checkBox_9->setChecked(false);

    if(gui_para.AIN[9] == 1)
        ui->checkBox_10->setChecked(true);
    else
        ui->checkBox_10->setChecked(false);

    if(gui_para.AIN[10] == 1)
        ui->checkBox_11->setChecked(true);
    else
        ui->checkBox_11->setChecked(false);

    if(gui_para.sample_mode == TIMING)
    {
        ui->radioButton_3->setChecked(true);
        ui->radioButton_4->setChecked(false);
    }
    if(gui_para.sample_mode == MONITOR)
    {
        ui->radioButton_3->setChecked(false);
        ui->radioButton_4->setChecked(true);
    }

}
