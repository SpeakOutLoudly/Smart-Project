#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"
#include "colorful_light.h"

#define RED_LED_PIN_NAME WIFI_IOT_IO_NAME_GPIO_10
#define RED_LED_PIN_FUNCTION WIFI_IOT_IO_FUNC_GPIO_10_GPIO

#define GREEN_LED_PIN_NAME WIFI_IOT_IO_NAME_GPIO_11
#define GREEN_LED_PIN_FUNCTION WIFI_IOT_IO_FUNC_GPIO_11_GPIO

#define BLUE_LED_PIN_NAME WIFI_IOT_IO_NAME_GPIO_12
#define BLUE_LED_PIN_FUNCTION WIFI_IOT_IO_FUNC_GPIO_12_GPIO

#define PWM_DUTY 64000
#define PWM_FREQ_DIVITION 64000
#define ADC_RESOLUTION 4096  // 12位ADC分辨率 (2^12 = 4096)

static int Query_colorful_light_state = 0;

//static int Query_fan_state = 0;

static int Control_Status_Light = 0;
static char *Control_Value_Light = "OFF";

static int Control_Status_Fan = 0;
static char *Control_Value_Fan = "0";

static void InitLightGPIO(void){
    GpioInit();

    IoSetFunc(RED_LED_PIN_NAME, WIFI_IOT_IO_FUNC_GPIO_10_PWM1_OUT);
    IoSetFunc(GREEN_LED_PIN_NAME, WIFI_IOT_IO_FUNC_GPIO_11_PWM2_OUT);
    IoSetFunc(BLUE_LED_PIN_NAME, WIFI_IOT_IO_FUNC_GPIO_12_PWM3_OUT);

    // 初始化 PWM 端口
    PwmInit(WIFI_IOT_PWM_PORT_PWM1); // R
    PwmInit(WIFI_IOT_PWM_PORT_PWM2); // G
    PwmInit(WIFI_IOT_PWM_PORT_PWM3); // B
}

int colorful_light_get_light_status(void){
    return Query_colorful_light_state;
}

static void colorful_light_control(char *value){
    if(strcmp(value, "ON") == 0){
        Query_colorful_light_state = 1;
        printf("colorful_light_control: ON\n");
        PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_DUTY, PWM_FREQ_DIVITION);
        PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_DUTY, PWM_FREQ_DIVITION);
        PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_DUTY, PWM_FREQ_DIVITION);
    }else if(strcmp(value, "OFF") == 0){
        printf("colorful_light_control: OFF\n");
        PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        PwmStop(WIFI_IOT_PWM_PORT_PWM2);
        PwmStop(WIFI_IOT_PWM_PORT_PWM3);
        Query_colorful_light_state = 0;
        Control_Status_Light = 0;
    }
}
/*
static void fan_init(void){

    // 初始化PWM引脚用于风扇调速
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_8, WIFI_IOT_IO_FUNC_GPIO_8_PWM1_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM1);
    
    // 初始化风扇电源控制引脚
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_DIR_OUT);
    
    // 初始状态关闭风扇
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE0);

}

static void fan_hardware(int level)
{
    Query_fan_state = level;
    
    if (level > 0 && level <= 3) {
        // 启动风扇
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE1);
        // PWM控制风扇转速 (level 1-3 对应不同转速)
        PwmStart(WIFI_IOT_PWM_PORT_PWM1, 
                PWM_FREQ_DIVITION / 10 * (level + 7), 
                PWM_FREQ_DIVITION);
        printf("[Bathroom] Fan ON - Level %d\n", level);
    } else {
        // 关闭风扇
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE0);
        PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        printf("[Bathroom] Fan OFF\n");
    }
}
*/
void hardware_control(char *target, char *value){
    if(strcmp(target, "LIGHT") == 0 ){
        printf("主函数匹配成功，Control_Status_Light 设置为1\n");
        Control_Status_Light = 1;
        Control_Value_Light = value;
    }else if(strcmp(target, "FAN") == 0){
        Control_Status_Fan = 1;
        Control_Value_Fan = value;
    }else if(strcmp(target, "STATUS") == 0){
        Status_Query();
    }
}


void Status_Query(void) {
    printf("执行状态查询\n");
    printf("Query_colorful_light_state: %d\n", colorful_light_get_light_status());
    // 这里可以添加具体的状态查询逻辑
}

static void Main_Task(void *arg){
    (void)arg;
    InitLightGPIO();
    
    while(1){
        if(Control_Status_Light == 1){
            colorful_light_control(Control_Value_Light);
            printf("进入手动模式\n");
        } else {
            // 启动三个灯同时常亮
            PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_DUTY, PWM_FREQ_DIVITION); // 红灯全亮
            PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_DUTY, PWM_FREQ_DIVITION); // 绿灯全亮
            PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_DUTY, PWM_FREQ_DIVITION); // 蓝灯全亮
            
            printf("三个RGB灯已开启常亮模式\n");
        }
        // 延时避免CPU占用过高
        usleep(1000000); // 1秒延时
    }
}

void ColorfulLightDemo(void){
    osThreadAttr_t attr;
    attr.name = "Main_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.priority = osPriorityNormal;
    attr.stack_size = 4096;
    attr.stack_mem = NULL;

    if(osThreadNew(Main_Task, NULL, &attr) == NULL){
        printf("[ColorfulLightDemo] Failed to create Main_Task!\n");
    }
}

APP_FEATURE_INIT(ColorfulLightDemo);