#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"

int bathroom_state = 0;
int bathroom_fan_state = 0;
int bathroom_light_state = 0;

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
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_VALUE0);
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_VALUE0);
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE0);
            bathroom_light_state = 0;
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