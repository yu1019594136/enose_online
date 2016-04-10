#ifndef BEEP_H
#define BEEP_H


#include "GPIO.h"

//蜂鸣器初始化
void beep_init();

//打开蜂鸣器函数
void beep_on();

//关闭蜂鸣器函数
void beep_off();

//移除蜂鸣器
void beep_close();

#endif // BEEP_H
