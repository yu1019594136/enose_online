#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWSServer>
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QMetaType>    //注册元类型

//注册元类型
Q_DECLARE_METATYPE(UART_SAMPLE)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 隐藏鼠标 */
    QWSServer::setCursorVisible(false);

    /* 启动程序时总是显示第一个选项卡 */
    ui->tabWidget->setCurrentIndex(0);
    //Qtabwidget->setCurrentIndex(3);

    /* 开机显示时间 */
    QDateTime datetime = QDateTime::currentDateTime();
    ui->datetime->setText(datetime.toString("yyyy.MM.dd hh:mm"));

    /* 实例化一个定时器并启动,用于更新时间 */
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    timer->start(1000 * 60);//一分钟更新一次时间

    /* 注册元类型 */
    qRegisterMetaType <UART_SAMPLE>("UART_SAMPLE");


    /* 实例化三个线程并启动,将三个子线程相关的信号关联到GUI主线程的槽函数 */
    uartthread = new UartThread ();
    logicthread = new LogicThread ();

    /* 逻辑线程发送信号通知串口线程开始采集数据 */
    connect(logicthread, SIGNAL(send_to_uartthread_sample_start(UART_SAMPLE)), uartthread, SLOT(recei_fro_logicthread_sample_start(UART_SAMPLE)), Qt::QueuedConnection);

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
    //检查线程是否为运行状态
    if(uartthread->isRunning())
        uartthread->stop();
    while(!uartthread->isFinished());//确认线程是否停止

    if(logicthread->isRunning())
        logicthread->stop();
    while(!logicthread->isFinished());

    /* 退出事件循环，结束程序 */
    QApplication *p;
    p->quit();
}

void MainWindow::timerUpdate()
{
    QDateTime datetime = QDateTime::currentDateTime();
    ui->datetime->setText(datetime.toString("yyyy.MM.dd hh:mm"));
}
