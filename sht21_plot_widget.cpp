#include <QPainter>
#include <QDebug>
#include <qcommon.h>
#include "sht21_plot_widget.h"
#include <QImage>

/*-----------------plot-----------------*/
//sht21_plot_data_buf;用于存储温湿度绘图数据
extern PLOT_DATA_BUF sht21_plot_data_buf;

//系统配置参数，保存于文件
extern SYS_Para sys_para;

Sht21_Plot_Widget::Sht21_Plot_Widget(QWidget *parent)
{
    /* Qt color */
/*
Constant        Value       Description
Qt::white       3           White (#ffffff)
Qt::black       2           Black (#000000)
Qt::red         7           Red (#ff0000)
Qt::darkRed     13          Dark red (#800000)
Qt::green       8           Green (#00ff00)
Qt::darkGreen	14          Dark green (#008000)
Qt::blue        9           Blue (#0000ff)
Qt::darkBlue	15          Dark blue (#000080)
Qt::cyan        10          Cyan (#00ffff)
Qt::darkCyan	16          Dark cyan (#008080)
Qt::magenta     11          Magenta (#ff00ff)
Qt::darkMagenta	17          Dark magenta (#800080)
Qt::yellow      12          Yellow (#ffff00)
Qt::darkYellow	18          Dark yellow (#808000)
Qt::gray        5           Gray (#a0a0a4)
Qt::darkGray	4           Dark gray (#808080)
Qt::lightGray	6           Light gray (#c0c0c0)
Qt::transparent	19          a transparent black value (i.e., QColor(0, 0, 0, 0))
Qt::color0      0           0 pixel value (for bitmaps)
Qt::color1      1           1 pixel value (for bitmaps)
*/
    color[0] = Qt::black;
    color[1] = Qt::red;
    color[2] = Qt::green;
    color[3] = Qt::blue;
    color[4] = Qt::cyan;
    color[5] = Qt::magenta;
    color[6] = Qt::yellow;
    color[7] = Qt::gray;
    color[8] = Qt::darkBlue;
    color[9] = Qt::darkRed;

    Sht21_Plot_Data_Buf.pp_data = NULL;
    Sht21_Plot_Data_Buf.p_data = NULL;
    Sht21_Plot_Data_Buf.index = 0;
    Sht21_Plot_Data_Buf.buf_size = 0;
    Sht21_Plot_Data_Buf.valid_data_size = 0;
    Sht21_Plot_Data_Buf.pp_data_float = NULL;
    Sht21_Plot_Data_Buf.pp_data_int = NULL;

    pic.load(QString(E_NOSE_ONLINE_LOGO_SHT21));
}

//接收来sht21_air线程的数据绘图命令
void Sht21_Plot_Widget::recei_fro_sht21_air_thread_sht21_plot()
{
    /*
     * 串口线程和绘图选项卡最好分别有一份自己的绘图数据，串口线程随时对uart_plot_data_buf进行修改，如果用
     * 户同时点开了绘图选项卡那么绘图事件中将同时不断读取uart_plot_data_buf的内容，从而容易发生错误，因此两者最好
     * 各自有一份自己的数据
    */
    Sht21_Plot_Data_Buf = sht21_plot_data_buf;

    if(Sht21_Plot_Data_Buf.valid_data_size == 0 || Sht21_Plot_Data_Buf.pp_data_float == NULL)//没有有效数据或者指针为空则不应该绘图
    {
        qDebug() << "Sht21_Plot_Data_Buf.valid_data_size = 0 or Sht21_Plot_Data_Buf.pp_uart_data = NULL";
    }
    else//有数据则应该发起绘图事件
    {
        update();
    }
}

void Sht21_Plot_Widget::paintEvent(QPaintEvent *event)
{
    unsigned long int i,j;

    QPainter painter(this);

    /* 开机时候有可能点开绘图选项卡，此时会执行绘图事件，所以绘图时间函数中也应该对指针进行判断 */
    if(Sht21_Plot_Data_Buf.valid_data_size != 0 && Sht21_Plot_Data_Buf.pp_data_float != NULL)
    {
/*
("Bitstream Charter", "Clean", "Clearlyu", "Clearlyu Alternate Glyphs", "Clearlyu Arabic",
"Clearlyu Arabic Extra", "Clearlyu Devanagari", "Clearlyu Devangari Extra", "Clearlyu Ligature",
"Clearlyu Pua", "Courier 10 Pitch", "Fangsong Ti", "Fixed [Jis]", "Fixed [Misc]", "Fixed [Sony]",
"Gothic", "Mincho", "Newspaper", "Nil", "Song Ti", "Standard Symbols L")

*/
        /* 先画数据文件名称，在转换坐标 */
        QRect rect(10, 10, 460, 20);
        QFont font("Clearlyu", 12);
        painter.setFont(font);
        painter.drawText(rect, Qt::AlignHCenter, Sht21_Plot_Data_Buf.filename.remove(sys_para.filepath));//plot_para.pic_name.remove(SYS_FILE_PATH));

        /* 设置视口（逻辑坐标） */
        painter.setWindow(0, 0, Sht21_Plot_Data_Buf.buf_size, sys_para.sht21_data_plot_height);
        /* 坐标系平移 */
        painter.translate(0, sys_para.sht21_data_plot_height);

        //qDebug("height = %u, width = %lu\n", UART_DATA_PLOT_HEIGHT, Uart_Plot_Data_Buf.data_size);

        //QPainterPath path;

        if(Sht21_Plot_Data_Buf.valid_data_size < Sht21_Plot_Data_Buf.buf_size)//有效数据个数少于内存块大小时
        {
            painter.setPen(QPen(color[1]));
            for(i = 0; i < Sht21_Plot_Data_Buf.valid_data_size; i++)
            {
                painter.drawPoint(QPointF(i, -Sht21_Plot_Data_Buf.pp_data_float[0][i]));
            }

            painter.setPen(QPen(color[3]));
            for(i = 0; i < Sht21_Plot_Data_Buf.valid_data_size; i++)
            {
                painter.drawPoint(QPointF(i, -Sht21_Plot_Data_Buf.pp_data_float[1][i]));
            }

        }
        else//有效数据个数等于内存块大小时
        {
            j = 0;
            painter.setPen(QPen(color[1]));
            for(i = Sht21_Plot_Data_Buf.index; i < Sht21_Plot_Data_Buf.buf_size; i++)
            {
                painter.drawPoint(QPointF(j++, -Sht21_Plot_Data_Buf.pp_data_float[0][i]));
            }
            for(i = 0; i < Sht21_Plot_Data_Buf.index; i++)
            {
                painter.drawPoint(QPointF(j++, -Sht21_Plot_Data_Buf.pp_data_float[0][i]));
            }

            j = 0;
            painter.setPen(QPen(color[3]));
            for(i = Sht21_Plot_Data_Buf.index; i < Sht21_Plot_Data_Buf.buf_size; i++)
            {
                painter.drawPoint(QPointF(j++, -Sht21_Plot_Data_Buf.pp_data_float[1][i]));
            }
            for(i = 0; i < Sht21_Plot_Data_Buf.index; i++)
            {
                painter.drawPoint(QPointF(j++, -Sht21_Plot_Data_Buf.pp_data_float[1][i]));
            }
        }


    }
    else
    {
        //qDebug("Uart_Plot_Data_Buf.valid_data_size = 0 or Uart_Plot_Data_Buf.p_uart_data = NULL, cann't plot!\n");
        painter.drawImage(QPoint(30, 30), pic);

    }
}


