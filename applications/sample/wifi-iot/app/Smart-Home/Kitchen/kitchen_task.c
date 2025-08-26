#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"

//气体报警阈值
#define GAS_THRESHOLD 500

//MQ2传感器通道
#define GAS_SENSOR_CHAN_NAME WIFI_IOT_ADC_CHANNEL_5

void bath_init(void *arg){
    (void)arg;

    GpioInit();
    IoSetFunc()
}

void kitchen_entry(void *arg){
    (void)arg;

    bath_init();
    while(1){


    }

}

void kitchen_task(){
    osThreadAttr_t attr;
    attr.name = "kitchen_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    if(osThreadNew(kitchen_entry, NULL, &attr) == NULL){
        printf("[kitchen_task] Falied to create kitchen_task!\r\n");
    }
}
