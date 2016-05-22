#-------------------------------------------------
#
# Project created by QtCreator 2016-04-09T03:13:37
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = enose_online
TEMPLATE = app

target.files = enose_online
target.path = /root/qi_enose_online/bin
INSTALLS = target

#关闭release中qDebug的输出
DEFINES += QT_NO_DEBUG_OUTPUT

SOURCES += main.cpp\
        mainwindow.cpp \
    common.c \
    GPIO.c \
    i2c.c \
    sht21.c \
    spidev.c \
    uart.c \
    myinputpanel.cpp \
    myinputpanelcontext.cpp \
    qcommon.cpp \
    stm32_spislave.c \
    beep.cpp \
    thread_dust_uart.cpp \
    thread_logic_control.cpp \
    uart_plot_widget.cpp \
    thread_sensor_PRU.cpp \
    pru_plot_widget.cpp \
    sht21_plot_widget.cpp \
    air_quality_plot_widget.cpp \
    thread_sht21_air.cpp \
    queue.cpp

HEADERS  += mainwindow.h \
    common.h \
    GPIO.h \
    i2c.h \
    sht21.h \
    spidev.h \
    uart.h \
    myinputpanel.h \
    myinputpanelcontext.h \
    qcommon.h \
    stm32_spislave.h \
    beep.h \
    thread_dust_uart.h \
    thread_logic_contrl.h \
    uart_plot_widget.h \
    thread_sensor_PRU.h \
    pru_plot_widget.h \
    sht21_plot_widget.h \
    air_quality_plot_widget.h \
    thread_sht21_air.h \
    queue.h

FORMS    += mainwindow.ui \
    myinputpanelform.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/am335x_pru_package-master/pru_sw/app_loader/lib/release/ -lprussdrvd
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/am335x_pru_package-master/pru_sw/app_loader/lib/debug/ -lprussdrvd
else:unix: LIBS += -L$$PWD/am335x_pru_package-master/pru_sw/app_loader/lib/ -lprussdrvd

INCLUDEPATH += $$PWD/am335x_pru_package-master/pru_sw/app_loader/include
DEPENDPATH += $$PWD/am335x_pru_package-master/pru_sw/app_loader/include
