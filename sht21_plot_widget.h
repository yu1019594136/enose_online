#ifndef SHT21_PLOT_WIDGET_H
#define SHT21_PLOT_WIDGET_H

#include <QWidget>
#include "common.h"
#include "qcommon.h"

//#define WIDTH 1000      //将宽度等分成WIDTH份
//#define HEIGHT 65536     //将高度等分成HEIGH份

class Sht21_Plot_Widget : public QWidget
{
    Q_OBJECT

public:
    Sht21_Plot_Widget(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QColor color[10];
    PLOT_DATA_BUF Sht21_Plot_Data_Buf;
    unsigned int sht21_data_plot_height;

public slots:
    /* 接收来串口线程的数据绘图命令 */
    void recei_fro_sht21_air_thread_sht21_plot();
};

#endif // SHT21_PLOT_WIDGET_H
