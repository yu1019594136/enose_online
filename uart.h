/*
 * uart.h
 *
 *  Created on: 2014年10月4日
 *      Author: zhouyu
 */

#ifndef UART_H_
#define UART_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define UART_DEV_PATH       "/dev/ttyO1"

int open_port(void);
void configure_port(int fd);

#ifdef __cplusplus
}
#endif

#endif /* UART_H_ */
