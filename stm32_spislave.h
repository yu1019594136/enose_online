/*
 * stm32_spislave.h
 *
 *  Created on: 2014年10月20日
 *      Author: zhouyu
 */

#ifndef STM32_SPISLAVE_H_
#define STM32_SPISLAVE_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* 高8位表示发送指令COMMAND给STM32，STM32回传数据，低八位字节表示回传8个uint16_t个数据 */
#define COMMAND 0xfe04

void stm32_Init(int gpiocs);
void stm32_Transfer(uint16_t *TxBuf, uint16_t *RxBuf, int len);
//void stm32_Transfer_half_duplex(uint8_t *TxBuf, uint8_t *RxBuf, int len);
void getdata_stm32(uint16_t command, uint16_t *RxBuf);
void stm32_Close();

#ifdef __cplusplus
}
#endif

#endif /* STM32_SPISLAVE_H_ */
