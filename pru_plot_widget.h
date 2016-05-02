#ifndef PRU_PLOT_WIDGET_H
#define PRU_PLOT_WIDGET_H

#include <QWidget>
#include "common.h"
#include "qcommon.h"

#define PRU_DATA_PLOT_HEIGHT 65536

class PRU_Plot_Widget : public QWidget
{
    Q_OBJECT

public:
    PRU_Plot_Widget(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QColor color[11];
    PLOT_DATA_BUF PRU_Plot_Data_Buf;
    QImage pic;

public slots:
    /* 接收来串口线程的数据绘图命令 */
    void recei_fro_pruthread();
};

#endif // PRU_PLOT_WIDGET_H
