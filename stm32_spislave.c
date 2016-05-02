/*
 * stm32_spislave.c
 *
 *  Created on: 2014年10月20日
 *      Author: zhouyu
 */


#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <spidev.h>
#include <stm32_spislave.h>

static int spi_fd;						//spi设备文件句柄
static int spibus_num = 1;				//SPI设备总线号
static int spichipselect_num = 0;		//SPI片选号，a SPI device with chipselect C on bus B, you should see /dev/spidevB.C

static uint32_t mode = 0;				//（POL，PHA），MODE0=（0,0）MODE1=（0,1）MODE2=（1,0）MODE3=（1,1）,POL=0表示时钟空闲为低电平，1为高电平；PHA=0表示采样发生在时钟的第一个跳变边沿，1表示第二个跳变边沿
static uint8_t bits = 16;				//每次通信的比特长度，这个长度依从设备而不同
static uint32_t speed = 9000000;		//SCLK的频率，单位Hz
static uint16_t delay = 0;				//当有多个从设备时，本次通信传输完成后在取消选中本片选线之前，延长多长时间（这个时间过后接着取消本片选线，选中其他片选线开始下一次传输）

static struct spi_ioc_transfer tr;		//用于进行全双工通信配置，具体参考spidev.h

void stm32_Init()
{
	int ret;
	char temp_str[50] = {0};

	/* 获取设备文件指针 */
	sprintf(temp_str, "/dev/spidev%d.%d",spibus_num, spichipselect_num);//将整形参数转换成字符串
	spi_fd = open(temp_str, O_RDWR);
	if (spi_fd < 0)
		printf("can't open device");

	/* 通信格式参数配置,依从设备而不同 */
	/*
	 * spi mode
	 */
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		printf("can't set spi mode");

	ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		printf("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't set bits per word");

	ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't set max speed hz");

	ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't get max speed hz");

	/* spi_ioc_transfer相关配置 */
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP))
	{
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}
}

void stm32_Transfer(uint16_t *TxBuf, uint16_t *RxBuf, int len_in_bytes)
{
	/* 为每一次SPI Transfer进行配置*/
	tr.tx_buf = (unsigned long)TxBuf;
	tr.rx_buf = (unsigned long)RxBuf;
	tr.len = len_in_bytes;

	/* 双向传输数据 */
	SPI_Transfer(spi_fd, 1, &tr);

}

void stm32_Close()
{
	/* 关闭SPI设备的文件指针 */
	close(spi_fd);
}
