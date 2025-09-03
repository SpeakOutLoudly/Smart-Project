/******************************************************************************\
**  版    权 :  深圳开鸿数字产业发展有限公司（2021）
**  文件名称 :  fire.c
**  功能描述 :  火焰检测
**  作    者 :  王滨泉
**  日    期 :  2021.10.10
**  版    本 :  V0.0.1
**  变更记录 :  V0.0.1/2021.10.10
                1 首次创建                 
\******************************************************************************/

/******************************************************************************\
                                 Includes
\******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <securec.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "ohos_types.h"
#include "wifiiot_errno.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "Beep.h"

/******************************************************************************\
                             Variables definitions
\******************************************************************************/

#define ADC_TASK_STACK_SIZE      4096                   //任务栈大小
#define ADC_TASK_PRIO            10                     //任务优先等级
#define BEEP_PIN                 9                      //蜂鸣器引脚
#define FIRE_DO_PIN              WIFI_IOT_IO_NAME_GPIO_5                      //火焰检测引脚，低电平触发火焰警报
//------------------------------------------------------------------------------
// 火焰状态
typedef enum
{
    FIRE_WHITOUT,                 //无火
    FIRE_HAVE,                    //有火 
}TS_FIRE_STATE;

static TS_FIRE_STATE s_Fire_State;
/******************************************************************************\
                             Functions definitions
\******************************************************************************/
/*
* 函数名称 : Get_Fire_State
* 功能描述 : 获取火焰状态
* 参    数 : 空
* 返回值   : 火焰状态
* 示    例 : State = Get_Fire_State();
*/
/******************************************************************************/ 
uint8_t Get_Fire_State(void)
/******************************************************************************/
{           
    return s_Fire_State;    //传递火焰状态
}
/*
* 函数名称 : Init_FireSensor
* 功能描述 : 初始化火焰传感器IO
* 参    数 : 空
* 返回值   : 空
* 示    例 : Init_FireSensor();
*/
/******************************************************************************/ 
void Init_FireSensor(void)
/******************************************************************************/
{
    GpioInit();
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_IO_FUNC_GPIO_5_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_GPIO_DIR_IN);
}
/*
* 函数名称 : Fire_Detection_Task
* 功能描述 : 火焰检测任务
* 参    数 : 空
* 返回值   : 空
* 示    例 : Fire_Detection_Task(&argument);
*/
/******************************************************************************/ 
void Fire_Detection_Task(void *argument)
/******************************************************************************/ 
{
    (void)argument;

    Init_FireSensor();                          //硬件初始化
    WifiIotGpioValue value = WIFI_IOT_GPIO_VALUE1;
    while (1)
    {
        usleep(10*1000);
        GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_5,&value);           //获取火焰状态
        if(value == WIFI_IOT_GPIO_VALUE0)
        {
 
                printf("=======================================\r\n");
                printf("****************Fire alarm*************\r\n");
                printf("=======================================\r\n");

        }
    }
}
/*
* 函数名称 : Fire_Detection_Demo
* 功能描述 : 创建火焰检测任务
* 参    数 : 空
* 返回值   : 空
* 示    例 : Fire_Detection_Demo(void);
*/
/******************************************************************************/ 
void Fire_Detection_Demo(void)
/******************************************************************************/ 
{
    osThreadAttr_t attr;

    attr.name       = "Fire-Detection-Task";
    attr.attr_bits  = 0U;
    attr.cb_mem     = NULL;
    attr.cb_size    = 0U;
    attr.stack_mem  = NULL;
    attr.stack_size = ADC_TASK_STACK_SIZE;
    attr.priority   = ADC_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)Fire_Detection_Task, NULL, &attr) == NULL) {
        printf("\r\n[Fire_Detection_Demo] Falied to create fire detection task!\n");
    }else{
        printf("\r\n[Fire_Detection_Demo] Succ to create fire detection task!\n");
    }
}

APP_FEATURE_INIT(Fire_Detection_Demo);