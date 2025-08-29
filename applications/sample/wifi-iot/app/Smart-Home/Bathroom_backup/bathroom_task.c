#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"

#include "bathroom_task.h"
#include <string.h>
#include <stdlib.h>

// PWM频率分频常量定义
#define PWM_FREQ_DIVITION 64000
// 注意：GPIO_12与蓝色LED冲突，这里使用GPIO_5作为风扇电源控制
#define FAN_POWER_GPIO WIFI_IOT_IO_NAME_GPIO_5

// 系统状态变量
static int bathroom_occupancy = 0;      // 浴室占用状态 (0: 无人, 1: 有人)
int bathroom_fan_state = 0;             // 风扇状态 (0: 关闭, 1-3: 档位) - 全局变量
int bathroom_light_state = 0;           // 灯光状态 (0: 关闭, 1: 开启) - 全局变量
int bathroom_state = 0;                 // 兼容性变量，映射到bathroom_occupancy
static int is_dark_environment = 0;     // 环境光线状态 (0: 明亮, 1: 昏暗)
static int control_mode = 0;            // 控制模式 (0: 自动模式, 1: 手动模式)


void Light_init(void *arg){
    (void)arg;
    // 设置 RGB LED IO为输出模式
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_IO_FUNC_GPIO_10_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_DIR_OUT);

    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_FUNC_GPIO_11_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_DIR_OUT);

    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_GPIO); 
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_DIR_OUT);
    
    // 初始状态关闭灯光
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_VALUE0);
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_VALUE0);
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE0);
}

// 灯光硬件控制函数
static void bathroom_set_light_hardware(int on)
{
    WifiIotGpioValue value = on ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, value);  // 红色LED
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, value);  // 绿色LED
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, value);  // 蓝色LED
    bathroom_light_state = on ? 1 : 0;
    printf("[Bathroom] Light %s\n", on ? "ON" : "OFF");
}

void Fan_init(void *arg){
    (void)arg;
    // 初始化PWM引脚用于风扇调速
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_8, WIFI_IOT_IO_FUNC_GPIO_8_PWM1_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM1);
    
    // 初始化风扇电源控制引脚
    IoSetFunc(FAN_POWER_GPIO, WIFI_IOT_IO_FUNC_GPIO_5_GPIO);
    GpioSetDir(FAN_POWER_GPIO, WIFI_IOT_GPIO_DIR_OUT);
    
    // 初始状态关闭风扇
    GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
}

// 风扇硬件控制函数
static void bathroom_set_fan_hardware(int level)
{
    bathroom_fan_state = level;
    
    if (level > 0 && level <= 3) {
        // 启动风扇
        GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE1);
        // PWM控制风扇转速 (level 1-3 对应不同转速)
        PwmStart(WIFI_IOT_PWM_PORT_PWM1, 
                PWM_FREQ_DIVITION / 10 * (level + 7), 
                PWM_FREQ_DIVITION);
        printf("[Bathroom] Fan ON - Level %d\n", level);
    } else {
        // 关闭风扇
        GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
        PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        printf("[Bathroom] Fan OFF\n");
    }
}

void Photoresistor_init(void *arg){
    (void)arg;
    // 初始化光敏电阻传感器
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_IO_FUNC_GPIO_9_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_GPIO_DIR_IN);
}

void Infrared_sensor_init(void *arg){
    (void)arg;
    // 初始化人体红外传感器
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_FUNC_GPIO_7_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_GPIO_DIR_IN);
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_PULL_UP);
}

void bathroom_init(void *arg){
    (void)arg;
    GpioInit();

    Light_init(NULL);
    Photoresistor_init(NULL);
    Infrared_sensor_init(NULL);
    Fan_init(NULL);
    
    printf("[Bathroom] Initialization completed\n");
}

// 传感器读取函数
static int bathroom_read_occupancy_sensor(void)
{
    WifiIotGpioValue value = WIFI_IOT_GPIO_VALUE0;
    if (GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_7, &value) == 0) {
        return (int)value;
    }
    return 0; // 默认无人
}

static int bathroom_read_light_sensor(void)
{
    WifiIotGpioValue value = WIFI_IOT_GPIO_VALUE0;
    if (GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_9, &value) == 0) {
        // 光敏电阻：0=昏暗(需要开灯), 1=明亮(不需要开灯)
        return (value == WIFI_IOT_GPIO_VALUE0) ? 1 : 0; // 返回1表示昏暗
    }
    return 0; // 默认明亮
}

// 智能灯光控制函数
static void bathroom_smart_light_control(void)
{
    int should_turn_on = 0;
    
    if (bathroom_occupancy == 1) {
        // 有人时，根据光线决定是否开灯
        should_turn_on = is_dark_environment;
    } else {
        // 无人时，关闭灯光
        should_turn_on = 0;
    }
    
    if (should_turn_on != bathroom_light_state) {
        bathroom_set_light_hardware(should_turn_on);
    }
}

// 智能风扇控制函数
static void bathroom_smart_fan_control(void)
{
    int target_level = 0;
    
    if (bathroom_occupancy == 1) {
        // 有人时，根据情况启动风扇
        if (is_dark_environment) {
            target_level = 1; // 晚上低速风扇
        } else {
            target_level = 2; // 白天中速风扇
        }
    } else {
        // 无人时，延迟一段时间后关闭风扇（简化处理，直接关闭）
        target_level = 0;
    }
    
    if (target_level != bathroom_fan_state) {
        bathroom_set_fan_hardware(target_level);
    }
}

void bathroom_entry(void *arg){
    (void)arg;

    bathroom_init(NULL);
    
    printf("[Bathroom] Smart bathroom system started in AUTO mode\n");
    
    while(1) {
        // 始终读取传感器数据（供状态查询）
        bathroom_occupancy = bathroom_read_occupancy_sensor();
        is_dark_environment = bathroom_read_light_sensor();
        
        // 根据控制模式执行不同逻辑
        if (control_mode == 0) {
            // 自动模式：基于传感器的智能控制
            bathroom_smart_light_control();
            bathroom_smart_fan_control();
        }
        // 手动模式：不执行自动控制，等待远程指令
        
        // 输出状态信息(可选)
        static int debug_counter = 0;
        if (++debug_counter >= 10) { // 每10秒输出一次状态
            printf("[Bathroom] Mode:%s, Occupancy:%d, Dark:%d, Light:%d, Fan:%d\n", 
                   control_mode == 0 ? "AUTO" : "MANUAL",
                   bathroom_occupancy, is_dark_environment, 
                   bathroom_light_state, bathroom_fan_state);
            debug_counter = 0;
        }
        
        sleep(1); // 1秒检测一次
    }
}

// 浴室智能控制系统任务创建函数
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
    } else {
        printf("[bathroom_task] Successfully created bathroom_task!\r\n");
    }
}

// ==================== 公开接口函数 ====================

// 状态查询接口
int bathroom_get_occupancy_status(void)
{
    return bathroom_occupancy;
}

int bathroom_get_light_status(void)
{
    return bathroom_light_state;
}

int bathroom_get_fan_status(void)
{
    return bathroom_fan_state;
}

int bathroom_get_light_sensor_status(void)
{
    return is_dark_environment;
}

int bathroom_get_control_mode(void)
{
    return control_mode;
}

// 模式控制接口
int bathroom_set_control_mode(int mode)
{
    if (mode < 0 || mode > 1) {
        printf("[Bathroom] Invalid control mode: %d\n", mode);
        return -1;
    }
    
    control_mode = mode;
    printf("[Bathroom] Control mode changed to: %s\n", 
           mode == 0 ? "AUTO" : "MANUAL");
    return 0;
}

// 模式控制（字符串版）：对外导出，供Tools/mqtt调用
void bathroom_Mode_Control(char *value)
{
    if (value == NULL) {
        printf("bathroom_Mode_Control: value is NULL\n");
        return;
    }

    printf("模式控制命令: %s\n", value);

    int mode = -1;

    if (strcmp(value, "AUTO") == 0 || strcmp(value, "auto") == 0 || strcmp(value, "0") == 0) {
        mode = 0;
    } else if (strcmp(value, "MANUAL") == 0 || strcmp(value, "manual") == 0 || strcmp(value, "1") == 0) {
        mode = 1;
    } else {
        printf("bathroom_Mode_Control: 未知模式 %s（支持: AUTO/MANUAL/auto/manual/0/1）\n", value);
        return;
    }

    if (bathroom_set_control_mode(mode) == 0) {
        printf("浴室模式切换成功: %s\n", mode == 0 ? "自动模式" : "手动模式");
    } else {
        printf("浴室模式切换失败\n");
    }
}

// 远程控制接口
int bathroom_remote_control_light(int on)
{
    if (control_mode != 1) {
        printf("[Bathroom] Remote light control failed: not in manual mode\n");
        return -1;
    }
    
    bathroom_set_light_hardware(on);
    printf("[Bathroom] Remote light control: %s\n", on ? "ON" : "OFF");
    return 0;
}

int bathroom_remote_control_fan(int level)
{
    if (control_mode != 1) {
        printf("[Bathroom] Remote fan control failed: not in manual mode\n");
        return -1;
    }
    
    if (level < 0 || level > 3) {
        printf("[Bathroom] Invalid fan level: %d\n", level);
        return -1;
    }
    
    bathroom_set_fan_hardware(level);
    printf("[Bathroom] Remote fan control: Level %d\n", level);
    return 0;
}

/*
 * 硬件控制主函数
 * 根据target选择相应的处理函数，并传入value
 */
void hardware_control(char *target, char *value) {
    if (target == NULL || value == NULL) {
        printf("hardware_control: target或value为NULL\n");
        return;
    }

    printf("执行硬件控制 - 目标: %s, 值: %s\n", target, value);

    // 使用字符串比较来判断目标设备类型
    if (strcmp(target, "FAN") == 0) {
        bathroom_Fan_Control(value);
    }
    else if (strcmp(target, "LIGHT") == 0) {
        bathroom_Light_Control(value);
    }
    else if (strcmp(target, "MODE") == 0) {
        bathroom_Mode_Control(value);
    }
    else if (strcmp(target, "STATUS") == 0) {
        Status_Query();
    }
    else {
        printf("hardware_control: 未知目标设备 %s（支持: FAN/LIGHT/MODE/STATUS）\n", target);
    }
}

// 风扇控制函数
void bathroom_Fan_Control(char *value) {
    if (value == NULL) {
        printf("bathroom_Fan_Control: value is NULL\n");
        return;
    }
    
    printf("风扇控制命令: %s\n", value);
    
    int value_int = atoi(value);
    
    // 检查参数有效性
    if (value_int < 0 || value_int > 3) {
        printf("bathroom_Fan_Control: 无效的风扇级别 %d（应为0-3）\n", value_int);
        return;
    }
    
    // 优先使用新接口，如果失败则使用兼容接口
    if (bathroom_remote_control_fan(value_int) != 0) {
        printf("远程控制失败，使用兼容模式（自动切换到手动模式）\n");
        bathroom_set_control_mode(1);  // 切换到手动模式
        bathroom_set_fan_hardware(value_int);
    }
    
    if (value_int >= 1) {
        printf("开启浴室风扇，级别: %d\n", value_int);
    } else {
        printf("关闭浴室风扇\n");
    }
}

// 灯光控制函数
void bathroom_Light_Control(char *value) {
    if (value == NULL) {
        printf("bathroom_Light_Control: value is NULL\n");
        return;
    }
    
    printf("灯光控制命令: %s\n", value);
    
    int light_on = -1;
    
    if (strcmp(value, "ON") == 0 || strcmp(value, "on") == 0 || strcmp(value, "1") == 0) {
        light_on = 1;
    }
    else if (strcmp(value, "OFF") == 0 || strcmp(value, "off") == 0 || strcmp(value, "0") == 0) {
        light_on = 0;
    }
    else {
        printf("bathroom_Light_Control: 未知值 %s（支持: ON/OFF/on/off/1/0）\n", value);
        return;
    }
    
    // 优先使用新接口，如果失败则使用兼容接口
    if (bathroom_remote_control_light(light_on) != 0) {
        printf("远程控制失败，使用兼容模式（自动切换到手动模式）\n");
        bathroom_set_control_mode(1);  // 切换到手动模式
        bathroom_set_light_hardware(light_on);
    }
    
    printf("%s浴室灯光\n", light_on ? "开启" : "关闭");
}

// 状态查询函数
void Status_Query(void) {
    printf("=== 浴室系统状态 ===\n");
    printf("控制模式: %s\n", bathroom_get_control_mode() == 0 ? "自动" : "手动");
    printf("人员占用: %s\n", bathroom_get_occupancy_status() ? "有人" : "无人");
    printf("环境光线: %s\n", bathroom_get_light_sensor_status() ? "昏暗" : "明亮");
    printf("灯光状态: %s\n", bathroom_get_light_status() ? "开启" : "关闭");
    printf("风扇状态: 档位 %d\n", bathroom_get_fan_status());
    printf("==================\n");
}
