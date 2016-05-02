#ifndef AIR_QUALITY_PLOT_WIDGET_H
#define AIR_QUALITY_PLOT_WIDGET_H

#include <QWidget>
#include "common.h"
#include "qcommon.h"

//#define WIDTH 1000      //将宽度等分成WIDTH份
//#define HEIGHT 65536     //将高度等分成HEIGH份

class Air_Plot_Widget : public QWidget
{
    Q_OBJECT

public:
    Air_Plot_Widget(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QColor color[10];
    PLOT_DATA_BUF Air_Plot_Data_Buf;
    unsigned int air_data_plot_height;

    /* 不显示数据时，显示一张图片 */
    QImage pic;

public slots:
    /* 接收来串口线程的数据绘图命令 */
    void recei_fro_sht21_air_thread_air_plot();
};

#endif // AIR_QUALITY_PLOT_WIDGET_H
