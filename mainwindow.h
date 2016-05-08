#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "thread_dust_uart.h"
#include "thread_logic_contrl.h"
#include "thread_sensor_PRU.h"
#include "thread_sht21_air.h"
#include "uart_plot_widget.h"
#include "pru_plot_widget.h"
#include "sht21_plot_widget.h"
#include "air_quality_plot_widget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_quit_clicked();

    void timerUpdate();//用于更新时间

    void on_pushButton_clicked();

    void on_radioButton_3_clicked();

    void on_radioButton_4_clicked();

    void checkbox_changed(int state);
    void checkbox_2_changed(int state);
    void checkbox_3_changed(int state);
    void checkbox_4_changed(int state);
    void checkbox_5_changed(int state);
    void checkbox_6_changed(int state);
    void checkbox_7_changed(int state);
    void checkbox_8_changed(int state);
    void checkbox_9_changed(int state);
    void checkbox_10_changed(int state);
    void checkbox_11_changed(int state);

    void on_pushButton_2_clicked();

public slots:
    //逻辑线程发送信号给GUI线程，所有任务已经结束
    void recei_fro_logicthread_enable_start();

    //执行退出应用程序的槽函数
    void recei_fro_logicthread_quit_application();

signals:
    //发送信号通知控制线程，界面参数已经读取
    void send_to_logic_GUI_data();

    //stop button is pressed
    void send_to_logic_stop_pressed();

    //通知逻辑线程结束其他线程的采集任务
    void send_to_logicthread_abort_sample_tasks();

    //quit application
    void quit_application();


private:
    Ui::MainWindow *ui;
    QTimer *timer;//用于更新时间
    UartThread *uartthread;//用于读取串口数据
    LogicThread *logicthread;//用于逻辑控制
    PRUThread *pruthread;//控制PRU采集数据
    Sht21_Air_Thread *sht21_air_thread;//用于采集温湿度数据和空气质量数据

    Uart_Plot_Widget *uart_plot_widget;//串口数据绘图选项卡
    PRU_Plot_Widget *pru_plot_widget;//pru数据绘图选项卡
    Air_Plot_Widget *air_plot_widget;//用于存储空气质量传感器绘图
    Sht21_Plot_Widget *sht21_plot_widget;//用于温湿度绘图

    void GUI_init();//从默认参数文件读取参数初始化界面

};

#endif // MAINWINDOW_H
