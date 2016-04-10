#include "beep.h"
#include "qcommon.h"

GPIO_Init_Struct gpio_Beep;//蜂鸣器，用于声音提示

//蜂鸣器初始化
void beep_init()
{
    gpio_Beep.pin = BEEP_PORT;
    gpio_Beep.dir = OUTPUT_PIN;
    GPIO_Init(&gpio_Beep);
}

//打开蜂鸣器函数
void beep_on()
{
    gpio_set_value(&gpio_Beep,HIGH);
}

//关闭蜂鸣器函数
void beep_off()
{
    gpio_set_value(&gpio_Beep,LOW);
}

//移除蜂鸣器
void beep_close()
{
    GPIO_Close(&gpio_Beep);
}
