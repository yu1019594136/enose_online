#include <QPainter>
#include <QDebug>
#include <qcommon.h>
#include "pru_plot_widget.h"
#include <QImage>

/*-----------------plot-----------------*/
//pru数据的内存块，用于选项卡绘图的数据，该数据块循环更新，为全局变量
extern PLOT_DATA_BUF pru_plot_data_buf;
extern unsigned int AIN_num;


PRU_Plot_Widget::PRU_Plot_Widget(QWidget *parent)
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
    color[10] = Qt::darkGreen;

    PRU_Plot_Data_Buf.p_data = NULL;
    PRU_Plot_Data_Buf.pp_data = NULL;
    PRU_Plot_Data_Buf.index = 0;
    PRU_Plot_Data_Buf.buf_size = 0;
    PRU_Plot_Data_Buf.valid_data_size = 0;
}

//接收来pru线程的数据绘图命令
void PRU_Plot_Widget::recei_fro_pruthread()
{
    /*
     * pru线程和绘图选项卡最好分别有一份自己的绘图数据，pru线程随时对PRU_plot_data_buf进行修改，如果用
     * 户同时点开了绘图选项卡那么绘图事件中将同时不断读取PRU_plot_data_buf的内容，从而容易发生错误，因此两者最好
     * 各自有一份自己的数据
    */
    PRU_Plot_Data_Buf = pru_plot_data_buf;

    qDebug("pru plot para received, index = %lu valid_data_size = %lu, ", PRU_Plot_Data_Buf.index, PRU_Plot_Data_Buf.valid_data_size);

    if(PRU_Plot_Data_Buf.valid_data_size == 0 || PRU_Plot_Data_Buf.pp_data == NULL)//没有有效数据或者指针为空则不应该绘图
    {
        qDebug() << "PRU_Plot_Data_Buf.valid_data_size = 0 or PRU_Plot_Data_Buf.p_PRU_data = NULL";
    }
    else//有数据则应该发起绘图事件
    {
        update();
    }
}

void PRU_Plot_Widget::paintEvent(QPaintEvent *event)
{
    unsigned long int i,j,k;

    QPainter painter(this);

    /* 开机时候有可能点开绘图选项卡，此时会执行绘图事件，所以绘图时间函数中也应该对指针进行判断 */
    if(PRU_Plot_Data_Buf.valid_data_size != 0 && PRU_Plot_Data_Buf.pp_data != NULL)
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
        painter.drawText(rect, Qt::AlignHCenter, PRU_Plot_Data_Buf.filename);//plot_para.pic_name.remove(SYS_FILE_PATH));

        /* 设置视口（逻辑坐标） */
        painter.setWindow(0, 0, PRU_Plot_Data_Buf.buf_size, PRU_DATA_PLOT_HEIGHT);
        /* 坐标系平移 */
        painter.translate(0, PRU_DATA_PLOT_HEIGHT);

        if(PRU_Plot_Data_Buf.valid_data_size < PRU_Plot_Data_Buf.buf_size)//有效数据个数少于内存块大小时painter.drawPoint(i, -PRU_Plot_Data_Buf.pp_data[j][i]);
        {
            for(i = 0; i < AIN_num; i++)
            {
                painter.setPen(QPen(color[i]));

                for(j = 0; j < PRU_Plot_Data_Buf.valid_data_size; j++)
                {
                    painter.drawPoint(j, -PRU_Plot_Data_Buf.pp_data[i][j]);
                }
            }
        }
        else//有效数据个数等于内存块大小时
        {
            for(i = 0; i < AIN_num; i++)
            {
                k = 0;

                painter.setPen(QPen(color[i]));

                for(j = PRU_Plot_Data_Buf.index; j < PRU_Plot_Data_Buf.buf_size; j++)
                {
                    painter.drawPoint(k++, -PRU_Plot_Data_Buf.pp_data[i][j]);
                }

                for(j = 0; j < PRU_Plot_Data_Buf.index; j++)
                {
                    painter.drawPoint(k++, -PRU_Plot_Data_Buf.pp_data[i][j]);
                }
            }
        }
    }
    else
    {
        //qDebug("PRU_Plot_Data_Buf.valid_data_size = 0 or PRU_Plot_Data_Buf.p_PRU_data = NULL, cann't plot!\n");

        /* 不显示数据时，显示一张图片 */
        QImage pic;

        if(pic.load(QString(E_NOSE_ONLINE_LOGO)))
        {
            //qDebug() << "picture size:" << pic.size() << endl;
            painter.drawImage(QPoint(30, 30), pic);
        }


    }
}
