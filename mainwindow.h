#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

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
};

#endif // MAINWINDOW_H
