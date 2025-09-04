#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"
#include "bathroom_task.h"

// PWM频率分频常量定义
#define PWM_DUTY 64000
#define PWM_Gear_1 19200
#define PWM_Gear_2 38400
#define PWM_Gear_3 64000
#define PWM_FREQ_DIVITION 64000
#define FAN_POWER_GPIO WIFI_IOT_IO_NAME_GPIO_8

/**
 *  炫彩灯板：
 * 
 * 
 * 
 */

static int bathroom_state = 0;
static int bathroom_fan_state = 0;
static int bathroom_light_state = 0;
static int fan_level = 0;


static int Control_Status_Light = 0;  // 0:自动模式, 1:待执行手动操作, 2:手动模式(已执行)
static char *Control_Value_Light = "OFF";


// 引脚初始化函数

static void Init_Light_GPIO(void){
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_IO_FUNC_GPIO_10_PWM1_OUT);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_PWM2_OUT);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_PWM3_OUT);

    // 初始化 PWM 端口
    PwmInit(WIFI_IOT_PWM_PORT_PWM1); // R
    PwmInit(WIFI_IOT_PWM_PORT_PWM2); // G
    PwmInit(WIFI_IOT_PWM_PORT_PWM3); // B
}

static void Init_Fan_GPIO(void){
    // 初始化PWM引脚用于风扇调速 原来为8，现在为1
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_1, WIFI_IOT_IO_FUNC_GPIO_1_PWM4_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM4);
    
    // 初始化风扇电源控制引脚 原来为12，现在为8
    IoSetFunc(FAN_POWER_GPIO, WIFI_IOT_IO_FUNC_GPIO_8_GPIO);
    GpioSetDir(FAN_POWER_GPIO, WIFI_IOT_GPIO_DIR_OUT);
    
    // 初始状态关闭风扇
    GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);

}

void Init_Photoresistor_GPIO(void){

    // 初始化光敏电阻传感器
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_IO_FUNC_GPIO_9_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_GPIO_DIR_IN);
}

void Init_Infrared_sensor_GPIO(void){

    // 初始化人体红外传感器
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_FUNC_GPIO_7_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_GPIO_DIR_IN);
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_PULL_UP);
}

void bathroom_init(void *arg){
    (void)arg;
    GpioInit();

    Init_Light_GPIO();
    Init_Fan_GPIO();
    Init_Photoresistor_GPIO();
    Init_Infrared_sensor_GPIO();

    printf("[Bathroom] Initialization completed\n");

}

static void bathroom_control_set_light(char *value)
{   
    int light_num = atoi(value);  // 将字符串转换为整数
    printf("进入bathroom_control_set_light函数\n");
    if (light_num == 1){
        printf("打开灯光\n");
        PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_Gear_1, PWM_FREQ_DIVITION); // 红灯30%
        PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_Gear_1, PWM_FREQ_DIVITION); // 绿灯30%
        PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_Gear_1, PWM_FREQ_DIVITION); // 蓝灯30%
        bathroom_light_state = 1;
    }else if(light_num == 2){
        printf("打开灯光\n");
        PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_Gear_2, PWM_FREQ_DIVITION); // 红灯60%
        PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_Gear_2, PWM_FREQ_DIVITION); // 绿灯60%
        PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_Gear_2, PWM_FREQ_DIVITION); // 蓝灯60%
        bathroom_light_state = 1;
    }else if(light_num == 3){
        printf("打开灯光\n");
        PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_Gear_3, PWM_FREQ_DIVITION); // 红灯100%
        PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_Gear_3, PWM_FREQ_DIVITION); // 绿灯100%
        PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_Gear_3, PWM_FREQ_DIVITION); // 蓝灯100%
        bathroom_light_state = 1;
    }else if (light_num == 0){
        printf("关闭灯光\n");
        PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        PwmStop(WIFI_IOT_PWM_PORT_PWM2);
        PwmStop(WIFI_IOT_PWM_PORT_PWM3);
        bathroom_light_state = 0;
        Control_Status_Light = 0;
    }
    else{
        printf("无效的灯光控制值: %s\n", value);
    }
}

static void bathroom_control_set_fan(char *value)
{
    /* 示例：若接到具体风机GPIO请替换为对应IO */
    /* 这里复用一根灯的GPIO作为演示 */
    // int value = on ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    int value_int = atoi(value);
    if(value_int < 0 || value_int > 3){
        printf("bathroom_control_set_fan: 无效的风扇级别 %d（应为0-3）\n", value_int);
        return;
    }else if(value_int == 0){
        printf("关闭风扇\n");
        bathroom_fan_state = 0;
        fan_level = 0;
        // 移除这行：Control_Status_Fan = 0; 让手动控制持续有效
    }else{
        printf("打开风扇\n");
        bathroom_fan_state = 1;
        fan_level = value_int;
    }
}

/*
 * 硬件主要逻辑入口
 */

void bathroom_entry(void *arg){
    (void)arg;

    bathroom_init(NULL);
    WifiIotGpioValue rel = 0;
    while(1){
        // 灯光控制逻辑
        if(Control_Status_Light == 1){
            // 有待执行的手动操作
            printf("灯光进入手动模式\n");
            bathroom_control_set_light(Control_Value_Light);
        } else if (Control_Status_Light == 0) {
            // 自动模式：读取人体红外传感器
            GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_7, &rel);
            bathroom_state = rel;
            //读取光敏电阻
            GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_9, &rel);

            //如果有人
            if (bathroom_state){
                if (bathroom_light_state == 0) { // 避免重复开启
                    printf("有人 - 自动开灯\n");
                    PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_DUTY, PWM_FREQ_DIVITION);
                    PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_DUTY, PWM_FREQ_DIVITION);
                    PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_DUTY, PWM_FREQ_DIVITION);
                    bathroom_light_state = 1;
                }
            }
            else
            {
                if (bathroom_light_state == 1) { // 避免重复关闭
                    printf("无人 - 自动关灯\n");
                    PwmStop(WIFI_IOT_PWM_PORT_PWM1);
                    PwmStop(WIFI_IOT_PWM_PORT_PWM2);
                    PwmStop(WIFI_IOT_PWM_PORT_PWM3);
                    bathroom_light_state = 0;
                }
            }
        }
        // Control_Status_Light == 2 时，保持手动模式，不做任何操作
        /*
        // 风扇控制逻辑
        if(Control_Status_Fan == 1){
            // 有待执行的手动操作
            printf("风扇进入手动模式\n");
            bathroom_control_set_fan(Control_Value_Fan);
            Control_Status_Fan = 2; // 标记为已执行手动操作
        }*/
        
        // 风扇硬件控制（无论手动还是自动模式都需要执行硬件操作）
        if (fan_level > 0 && fan_level < 4 && bathroom_fan_state == 1){
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE1);
            PwmStart(WIFI_IOT_PWM_PORT_PWM4, PWM_FREQ_DIVITION / 10 * (fan_level + 7),
            PWM_FREQ_DIVITION);
        }
        else{
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
            PwmStop(WIFI_IOT_PWM_PORT_PWM4);
        }
        
        sleep(1);
    }
}

void Main_Task(void){
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

// ==================== 硬件外部控制接口 ====================
//以下是对硬件的远程控制部分

//TODO：增加硬件控制函数，注意修改设备ID
/*
 * 硬件控制主函数
 * 根据target选择相应的处理函数，并传入value
 */
void Hardware_Control(char *target, char *param, char *value) {
    param = param;
    if (target == NULL || value == NULL) {
        printf("hardware_control: target或value为NULL\n");
        return;
    }

    printf("执行硬件控制 - 目标: %s, 参数: %s, 值: %s\n", target, param, value);

    // 使用字符串比较来判断目标设备类型
    if (strcmp(target, "14") == 0) {
        bathroom_control_set_fan(value);
    }
    else if (strcmp(target, "11") == 0 && strcmp(param, "BRIGHTNESS") == 0) {

        Control_Status_Light = 1;
        Control_Value_Light = value;
    }
    else {
        printf("hardware_control: 未知目标设备 %s\n", target);
    }
}

// ==================== 硬件外部查询接口 ====================

int Query_Room_Status(void){
    return bathroom_state;
}

