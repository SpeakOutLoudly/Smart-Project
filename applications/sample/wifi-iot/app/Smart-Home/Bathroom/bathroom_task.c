#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"

// PWM频率分频常量定义
#define PWM_FREQ_DIVITION 64000
#define FAN_POWER_GPIO WIFI_IOT_IO_NAME_GPIO_5

int bathroom_state = 0;
int bathroom_fan_state = 0;
int bathroom_light_state = 0;
int bathroom_light_control = 0;
int fan_level = 1;

void bathroom_init(void *arg){
    (void)arg;
    GpioInit();

    //设置 红绿蓝 LED IO为输出
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_IO_FUNC_GPIO_10_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_DIR_OUT);

    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_DIR_OUT);

    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_GPIO); 
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_DIR_OUT);
    
    //光敏电阻
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_IO_FUNC_GPIO_9_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_GPIO_DIR_IN);
    
    //人体红外感应
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_FUNC_GPIO_7_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_GPIO_DIR_IN);
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_PULL_UP);

    //风扇
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_8, WIFI_IOT_IO_FUNC_GPIO_8_PWM1_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM1);
    IoSetFunc(FAN_POWER_GPIO, WIFI_IOT_IO_FUNC_GPIO_5_GPIO);
    GpioSetDir(FAN_POWER_GPIO, WIFI_IOT_GPIO_DIR_OUT);
    //FAN__PWOER要的GPIO12和蓝色LED冲突，此处先省略

}

void bathroom_control_set_light(int on)
{
    int value = on ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, value);
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, value);
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, value);
    bathroom_light_control = on ? 1 : 0;
    bathroom_light_state = on ? 1 : 0;
}

void bathroom_control_set_fan(int on)
{
    /* 示例：若接到具体风机GPIO请替换为对应IO */
    /* 这里复用一根灯的GPIO作为演示 */
    // int value = on ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    bathroom_fan_state = on ? 1 : 0;
    fan_level = on;
}

void bathroom_entry(void *arg){
    (void)arg;

    bathroom_init(NULL);
    WifiIotGpioValue rel = 0;
    while(1){

        //读取人体红外传感器，
        GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_7, &rel);

        bathroom_state = rel;
        //读取光敏电阻
        GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_9, &rel);

        //如果有人
        if (bathroom_state)
        {

            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, (int)rel);
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, (int)rel);
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, (int)rel);
            bathroom_light_state = rel;
        }
        else
        {
            if(bathroom_light_control == 1){
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, (int)rel);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, (int)rel);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, (int)rel);
                bathroom_light_state = rel;
            } else{
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_VALUE0);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_VALUE0);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE0);
                bathroom_light_state = 0;
            }
        }
        //风扇模块
        if (fan_level > 0 && fan_level < 4 && bathroom_fan_state == 1){
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE1);
            PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_FREQ_DIVITION / 10 * (fan_level + 7),
            PWM_FREQ_DIVITION);
        }
        else{
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
            PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        }
        sleep(1);
    }
}

void bathroom_task(void){
    osThreadAttr_t attr;
    attr.name = "bathroom_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    if(osThreadNew(bathroom_entry, NULL, &attr) == NULL){
        printf("[bathroom_task] Failed to create bathroom_task!\r\n");
    }
}