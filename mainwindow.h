#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "thread_dust_uart.h"
#include "thread_logic_contrl.h"
#include "uart_plot_widget.h"
#include "thread_sensor_PRU.h"

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

public slots:

signals:
    //发送信号通知控制线程，界面参数已经读取
    void send_to_logic_GUI_data();


private:
    Ui::MainWindow *ui;
    QTimer *timer;//用于更新时间
    UartThread *uartthread;//用于读取串口数据
    LogicThread *logicthread;//用于逻辑控制
    PRUThread *pruthread;//控制PRU采集数据

    Uart_Plot_Widget *uart_plot_widget;//串口数据绘图选项卡

};

#endif // MAINWINDOW_H
