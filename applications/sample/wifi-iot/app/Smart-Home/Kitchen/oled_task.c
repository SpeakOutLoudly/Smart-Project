#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "oled_ssd1306.h"
#include "kitchen_task.h"

void Oled_init(void){
    GpioInit();
    OledInit();
}

static void OledDemoTask(void *arg){
    (void)arg;
    (void)arg;
    Oled_init();       //初始化
    OledFillScreen(0x00); //清屏，
    //在左上角位置显示字符串Hello, HarmonyOS
    OledShowString(0, 0, "smart home 2.0", 1);
    // printf("oled thread running ,led level:%d\r\n", fan_level);
    char line[32] = {0};

    while (1)
    {

        //组装显示气体的字符串 单位是百万分之  ，检测浓度：300-10000ppm(可燃气体)
        snprintf(line, sizeof(line), "fire_alarm : %3s", Query_gas_sensor_value() ? "on" : "off");
        OledShowString(0, 2, line, 1); //在（0，3）位置显示组装后的气体字符串

        //组装显示湿度的字符串
        snprintf(line, sizeof(line), "fire_state : %2d", Query_Humidity());
        OledShowString(0, 3, line, 1); //在（0，3）位置显示组装后的气体字符串

        //组装显示温度的字符串
        snprintf(line, sizeof(line), "temp: %.1f deg", Query_Temperature());
        OledShowString(0, 4, line, 1); //在（0，1）位置显示组装后的温度字符串


        sleep(1); //睡眠
    }
}


void OledDemo(void){
    osThreadAttr_t attr;
    attr.name = "OledDemoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    if(osThreadNew(OledDemoTask, NULL, &attr) == NULL){
        printf("[OledDemo] Falied to create OledDemoTask!\r\n");
    }
}