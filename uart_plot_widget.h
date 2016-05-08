#ifndef UART_PLOT_WIDGET_H
#define UART_PLOT_WIDGET_H

#include <QWidget>
#include "common.h"
#include "qcommon.h"

#define WIDTH 1000      //将宽度等分成WIDTH份
#define HEIGHT 65536     //将高度等分成HEIGH份

class Uart_Plot_Widget : public QWidget
{
    Q_OBJECT

public:
    Uart_Plot_Widget(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QColor color[10];
    PLOT_DATA_BUF Uart_Plot_Data_Buf;

    /* 不显示数据时，显示一张图片 */
    QImage pic;

public slots:
    /* 接收来串口线程的数据绘图命令 */
    void recei_fro_uartthread();
};

#endif // UART_PLOT_WIDGET_H
