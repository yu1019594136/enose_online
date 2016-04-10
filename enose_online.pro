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
target.path = /root/qi_enose_online
INSTALLS = target

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
    thread_logic_control.cpp

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
    thread_logic_contrl.h

FORMS    += mainwindow.ui \
    myinputpanelform.ui
