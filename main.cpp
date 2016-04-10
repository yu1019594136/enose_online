#include "beep.h"
#include <unistd.h>         //usleep()
#include <stdio.h>
#include "sht21.h"

//界面相关头文件
#include "mainwindow.h"
#include <QApplication>
#include "myinputpanelcontext.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MyInputPanelContext *ic = new MyInputPanelContext;
    app.setInputContext(ic);

    MainWindow mainwindow;
    mainwindow.showFullScreen();
    return app.exec();
}
