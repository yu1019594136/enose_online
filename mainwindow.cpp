#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWSServer>
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QMetaType>    //注册元类型

//注册元类型
Q_DECLARE_METATYPE(UART_SAMPLE_START)

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
    //ui->Qtabwidget->addTab(plot_widget, "plot data");

    /* 开机显示时间 */
    QDateTime datetime = QDateTime::currentDateTime();
    ui->datetime->setText(datetime.toString("yyyy.MM.dd hh:mm"));

    /* 实例化一个定时器并启动,用于更新时间 */
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    timer->start(1000 * 60);//一分钟更新一次时间

    /* 注册元类型 */
    qRegisterMetaType <UART_SAMPLE_START>("UART_SAMPLE_START");


    /* 实例化三个线程并启动,将三个子线程相关的信号关联到GUI主线程的槽函数 */
    uartthread = new UartThread ();
    logicthread = new LogicThread ();

    /* 逻辑线程发送信号通知串口线程开始采集数据 */
    connect(logicthread, SIGNAL(send_to_uartthread_sample_start(UART_SAMPLE_START)), uartthread, SLOT(recei_fro_logicthread_sample_start(UART_SAMPLE_START)), Qt::QueuedConnection);

    /* 逻辑线程发送信号通知串口线程停止采集数据 */
    connect(logicthread, SIGNAL(send_to_uartthread_sample_stop()), uartthread, SLOT(recei_fro_logicthread_sample_stop()), Qt::QueuedConnection);

    /* 串口线程通知逻辑线程数据采集完成 */
    connect(uartthread, SIGNAL(send_to_logic_uart_sample_complete(int)), logicthread, SLOT(receive_task_report(int)), Qt::QueuedConnection);

    /* 串口线程通知绘图选项卡进行绘图 */
    connect(uartthread, SIGNAL(send_to_plot_uart_curve()), uart_plot_widget, SLOT(recei_fro_uartthread()), Qt::QueuedConnection);

    uartthread->start();
    logicthread->start();

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


    /* 退出事件循环，结束程序 */
    QApplication *p;
    p->quit();
}

void MainWindow::timerUpdate()
{
    QDateTime datetime = QDateTime::currentDateTime();
    ui->datetime->setText(datetime.toString("yyyy.MM.dd hh:mm"));
}
