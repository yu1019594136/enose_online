#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWSServer>
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QMetaType>    //注册元类型

//注册元类型
Q_DECLARE_METATYPE(UART_SAMPLE_START)
Q_DECLARE_METATYPE(PRU_SAMPLE_START)

//读取界面参数并发通知逻辑线程开始数据采集
GUI_Para gui_para;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 隐藏鼠标 */
    QWSServer::setCursorVisible(false);

    /* 启动程序时总是显示第一个选项卡 */
    ui->tabWidget->setCurrentIndex(0);

    /* 实例化一个tab页用于绘图 */
    uart_plot_widget = new Uart_Plot_Widget(this);
    ui->tabWidget->addTab(uart_plot_widget, "uart curve");

    /* 实例化一个tab页用于绘图 */
    pru_plot_widget = new PRU_Plot_Widget(this);
    ui->tabWidget->addTab(pru_plot_widget, "pru curve");

    /* 开机显示时间 */
    QDateTime datetime = QDateTime::currentDateTime();
    ui->datetime->setText(datetime.toString("yyyy.MM.dd hh:mm"));

    /* 实例化一个定时器并启动,用于更新时间 */
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    timer->start(1000 * 60);//一分钟更新一次时间

    //doublespinbox补充配置代码
    ui->doubleSpinBox->setMinimum(0.0);
    ui->doubleSpinBox->setMaximum(5000);
    ui->doubleSpinBox->setSingleStep(0.01);
    ui->doubleSpinBox->setValue(100);

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

    /* 注册元类型 */
    qRegisterMetaType <UART_SAMPLE_START>("UART_SAMPLE_START");
    qRegisterMetaType <PRU_SAMPLE_START>("PRU_SAMPLE_START");


    /* 实例化三个线程并启动,将三个子线程相关的信号关联到GUI主线程的槽函数 */
    uartthread = new UartThread ();
    logicthread = new LogicThread ();
    pruthread = new PRUThread ();

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

    /* 逻辑线程通知pru线程开始采集数据 */
    connect(logicthread, SIGNAL(send_to_pruthread_pru_sample_start(PRU_SAMPLE_START)), pruthread, SLOT(recei_fro_logicthread_pru_sample_start(PRU_SAMPLE_START)), Qt::QueuedConnection);

    uartthread->start();
    logicthread->start();
    pruthread->start();

}

MainWindow::~MainWindow()
{
    delete ui;
    delete uartthread;
    delete logicthread;
}

void MainWindow::on_quit_clicked()
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

    //checkbox不需要再次读取，槽函数中已经进行设置
//    for(int i = 0; i < 11; i++)
//    {
//        qDebug() << "AIN[" << i << "] = " << gui_para.AIN[i];
//    }

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
