#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "oled_ssd1306.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"

void Oled_init(void){
    GpioInit();
    OledInit();
}

static void OledDemoTask(void *arg){
    (void)arg;
    Oled_init();

    OledFillScreen(0x00);
    OledShowString(0,0,"Hello, World!", 1);
    sleep(1);
    for(int i = 0; i<3;i++){
        OledFillScreen(0x00);
        for(int y=0;y<8;y++){
            static const char text[] = "This ia a Scree Demo prog!";
            OledShowString(0,y,text,1);
        }
        sleep(1);
    }
}

static void OledDemo(void){
    osThreadAttr_t attr;
    attr.name = "OledDemoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    if (osThreadNew(OledDemoTask, NULL, &attr) == NULL){
        printf("[OledDemo] Falied to create OledTask!\n");
    }
}

APP_FEATURE_INIT(OledDemo);