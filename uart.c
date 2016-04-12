#include "uart.h"
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */


struct termios options;//配置串口

int open_port(void)
{
    int fd;
    fd = open(UART_DEV_PATH, O_RDWR | O_NOCTTY | O_NDELAY);
    //O_NOCTTY表示不想成为控制终端，本程序仅仅不需要键盘鼠标等作为输入
    //O_NDELAY表示本程序不关心串行通信中的另一端是否在线，不关心DCD控制线状态

    if(fd < 0)
    {
        perror("open_port:Unable to open /dev/ttyO1\n");
    }
    else
        fcntl(fd, F_SETFL, 0);//阻塞读取
        //fcntl(fd, F_SETFL, FNDELAY);//非阻塞读取

    return fd;
}

void configure_port(int fd)
{
    /* 获取当前的串口配置 */
    tcgetattr(fd, &options);

    /* 设置串口波特率 */
    cfsetispeed(&options, B9600);//接受波特率
    cfsetospeed(&options, B9600);//发送波特率

    /* 设置串口帧格式 8N1 */
    options.c_cflag &= ~CSIZE; /* Mask the character size bits */
    options.c_cflag |= CS8;    /* Select 8 data bits */
    options.c_cflag &= ~PARENB;//无校验
    options.c_cflag &= ~CSTOPB;//1位停止位

    /* 禁能硬件流控制 */
    options.c_cflag &= ~CRTSCTS;
    //options.c_cflag |= CRTSCTS;//使能硬件流控制

    /* 使能接收模式和本地模式 */
    options.c_cflag |= (CLOCAL | CREAD);

    /* 标准输入和原始输入 */
    options.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);

    /* 原始输出 */
    options.c_oflag &= ~OPOST;

    options.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT );

    /* 阻塞，直到读取到一个字符 */
    options.c_cc[VMIN] = 1;

     /* 不使用字符间的计时器 */
    options.c_cc[VTIME] = 0;

    /* 配置生效 */
    tcsetattr(fd, TCSANOW, &options);
    //TCSANOW立即生效，即使还有数据没完成接收或者发送
    //TCSADRAIN,等发送完成设置才生效
    //TCSAFLUSH，刷新输入和输出缓冲区后设置生效
}
