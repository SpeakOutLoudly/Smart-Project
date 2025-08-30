#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include "wifi_device.h"
#include "lwip/netifapi.h"
// #include "lwip/api_shell.h" // 移除可能有问题的头文件包含
#include "wifi_utils.h"
#include "mqtt_utils.h"
#include "bathroom_task.h"
#include "publish_task.h"

static void main_entry(void *arg){
    (void)arg;
    sleep(2);
    printf("Smart Home BathroomPart Running...\r\n");

    Main_Task();
    publish_task();

}

void main_task(void){
    osThreadAttr_t attr;
    attr.name = "main_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;

    // osThreadEntry_t是一个函数指针类型，表示线程入口函数的类型（即void (*osThreadEntry_t)(void *)）。
    // 这里将main_entry强制类型转换为osThreadEntry_t，是为了确保传递给osThreadNew的参数类型正确，避免编译器类型不匹配的警告或错误。
    if(osThreadNew((osThreadFunc_t)main_entry, NULL, &attr) == NULL){
        printf("[main_task] Failed to create main_task!\r\n");
    } 
}

APP_FEATURE_INIT(main_task);