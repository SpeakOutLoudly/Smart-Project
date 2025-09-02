#include "cmsis_os2.h"
#include <unistd.h>
#include <stdio.h>
#include "wifi_utils.h"
#include "mqtt_utils.h"

static void publish_thread(void *arg){
    (void)arg;

    // 1. 使用wifi_utils进行可靠的WiFi连接
    connect_wifi();
    
    // 2. 等待WiFi连接完成
    sleep(5);
    
    // 3. 检查网络状态并进行MQTT连接
    hardware_auto_networking();

}

void publish_task(void){
    osThreadAttr_t attr;
    attr.name = "publish_thread";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = 36;


    if(osThreadNew((osThreadFunc_t)publish_thread, NULL, &attr) == NULL){
        printf("[publish_task] Failed to create publish_thread\n");
    }
}