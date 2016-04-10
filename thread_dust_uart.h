#ifndef THREAD_DUST_UART_H
#define THREAD_DUST_UART_H

#include <QThread>
#include <QString>

/*********************串口线程*****************************/
class UartThread : public QThread
{
    Q_OBJECT
public:
    explicit UartThread(QObject *parent = 0);
    void stop();

protected:
    void run();

private:
    volatile bool stopped;

signals:

public slots:

};


#endif // THREAD_DUST_UART_H
