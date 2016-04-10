#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "thread_dust_uart.h"
#include "thread_logic_contrl.h"

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

public slots:


private:
    Ui::MainWindow *ui;
    QTimer *timer;//用于更新时间
    UartThread *uartthread;//用于读取串口数据
    LogicThread *logicthread;//用于逻辑控制

};

#endif // MAINWINDOW_H
