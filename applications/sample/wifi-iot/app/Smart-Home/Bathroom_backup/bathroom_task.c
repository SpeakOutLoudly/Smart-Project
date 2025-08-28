#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"

// PWM频率分频常量定义 - 调整为20-25kHz避免啸叫
#define PWM_FREQ_DIVITION 40000  // 约20kHz (40MHz/40000 = 1kHz base * 20 = 20kHz)

// 风扇控制相关常量
#define FAN_LEVEL_MIN 0
#define FAN_LEVEL_MAX 3
#define FAN_PWM_MIN_DUTY 20     // 最小占空比20%，确保风扇能启动
#define FAN_PWM_MAX_DUTY 95     // 最大占空比95%，留有余量
#define FAN_SOFT_START_STEPS 10 // 软启动步数
#define FAN_STARTUP_DELAY_MS 50 // 软启动间隔

// GPIO引脚定义
#define FAN_PWM_GPIO    WIFI_IOT_IO_NAME_GPIO_8   // PWM调速引脚
#define FAN_POWER_GPIO  WIFI_IOT_IO_NAME_GPIO_5   // 电源控制引脚

int bathroom_state = 0;
int bathroom_fan_state = 0;
int bathroom_light_state = 0;
int bathroom_light_control = 0;
int fan_level = 0;              // 默认关闭
int fan_target_level = 0;       // 目标级别
int fan_current_duty = 0;       // 当前占空比
int fan_error_count = 0;        // 错误计数器

void bathroom_init(void *arg){
    (void)arg;
    GpioInit();

    //设置 红绿蓝 LED IO为输出 (GPIO10, GPIO11, GPIO12)
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

    // 风扇控制系统初始化
    // GPIO8: PWM调速输出
    IoSetFunc(FAN_PWM_GPIO, WIFI_IOT_IO_FUNC_GPIO_8_PWM1_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM1);
    
    // GPIO5: 风扇电源控制输出
    IoSetFunc(FAN_POWER_GPIO, WIFI_IOT_IO_FUNC_GPIO_5_GPIO);
    GpioSetDir(FAN_POWER_GPIO, WIFI_IOT_GPIO_DIR_OUT);
    
    // 初始化时确保风扇完全关闭
    PwmStop(WIFI_IOT_PWM_PORT_PWM1);
    GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0); // 关闭电源
    
    printf("风扇控制系统初始化完成: GPIO5(电源) + GPIO8(PWM)\n");

}

void bathroom_control_set_light(int on)
{
    int value = on ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, value);
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, value);
    GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, value); // 现在GPIO12可以正常用于RGB LED
    bathroom_light_control = on ? 1 : 0;
    bathroom_light_state = on ? 1 : 0;
}

/*
 * 风扇电源控制函数
 * power_on: 1=开启电源, 0=关闭电源
 */
void fan_power_control(int power_on) {
    WifiIotGpioValue gpio_value = power_on ? WIFI_IOT_GPIO_VALUE1 : WIFI_IOT_GPIO_VALUE0;
    GpioSetOutputVal(FAN_POWER_GPIO, gpio_value);
    
    printf("风扇电源控制: %s\n", power_on ? "开启" : "关闭");
    
    if (!power_on) {
        // 关闭电源时，确保PWM也停止
        PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        fan_current_duty = 0;
    }
}

/*
 * 风扇PWM调速控制函数
 * level: 0=关闭, 1-3=不同速度级别
 */
void fan_speed_control(int level) {
    if (level <= 0) {
        // 停止PWM输出
        PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        fan_current_duty = 0;
        printf("风扇PWM停止\n");
        return;
    }
    
    // 限制级别范围
    if (level > FAN_LEVEL_MAX) {
        level = FAN_LEVEL_MAX;
    }
    
    // 计算PWM占空比: level 1-3 -> 30%-90%占空比
    unsigned short duty_cycle = PWM_FREQ_DIVITION * level * 30 / 100;
    
    // 启动PWM，频率约20kHz，避免啸叫
    PwmStart(WIFI_IOT_PWM_PORT_PWM1, duty_cycle, PWM_FREQ_DIVITION);
    fan_current_duty = duty_cycle;
    
    printf("风扇PWM设置: 级别%d, 占空比%d%%\n", level, level * 30);
}

void bathroom_control_set_fan(int level)
{
    /*
     * 风扇控制函数
     * level: 0=关闭, 1-3=不同速度级别
     * 
     * 硬件连接:
     * - 风扇电源: GPIO5控制 (可通过软件开关)
     * - 调速信号: GPIO8 PWM输出 -> 风扇PWM输入
     * - 工作模式: 电源控制 + PWM调速双重控制
     * 
     * 控制逻辑:
     * 1. level = 0: 关闭电源 + 停止PWM
     * 2. level > 0: 开启电源 + 设置PWM占空比
     */
    
    // 参数验证
    if (level < FAN_LEVEL_MIN || level > FAN_LEVEL_MAX) {
        printf("错误: 风扇级别超出范围 (0-3): %d\n", level);
        fan_error_count++;
        return;
    }
    
    bathroom_fan_state = (level > 0) ? 1 : 0;
    fan_level = level;
    
    if (level == 0) {
        // 关闭风扇：先停止PWM，再关闭电源
        fan_speed_control(0);
        usleep(10000); // 延时10ms确保PWM停止
        fan_power_control(0);
        fan_error_count = 0; // 重置错误计数
        printf("风扇完全关闭\n");
    } else {
        // 开启风扇：先开启电源，再设置PWM
        fan_power_control(1);
        usleep(50000); // 延时50ms让电源稳定
        fan_speed_control(level);
        printf("风扇启动: 级别%d (占空比%d%%)\n", level, level * 30);
    }
}

/*
 * 风扇紧急停止函数
 * 立即停止风扇运行，用于异常情况
 */
void fan_emergency_stop(void) {
    printf("风扇紧急停止!\n");
    
    // 立即停止PWM和电源
    PwmStop(WIFI_IOT_PWM_PORT_PWM1);
    fan_power_control(0);
    
    // 重置所有状态
    fan_level = 0;
    fan_target_level = 0;
    fan_current_duty = 0;
    bathroom_fan_state = 0;
    fan_error_count = 0;
}

/*
 * 获取风扇状态信息
 * 返回格式化的状态字符串
 */
void get_fan_status_info(char *status_buffer, int buffer_size) {
    // 简化的占空比计算
    int duty_percent = (fan_level > 0) ? (fan_level * 30) : 0;
    
    snprintf(status_buffer, buffer_size,
             "Fan[Level:%d,Duty:%d%%,State:%s,Errors:%d]",
             fan_level,
             duty_percent,
             bathroom_fan_state ? "ON" : "OFF",
             fan_error_count);
}

void bathroom_entry(void *arg){
    (void)arg;

    bathroom_init(NULL);
    WifiIotGpioValue rel = 0;
    while(1){
        
        //读取人体红外传感器
        GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_7, &rel);
        bathroom_state = rel;
        
        //读取光敏电阻
        GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_9, &rel);

        //如果有人
        if (bathroom_state) {
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, (int)rel);
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, (int)rel);
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, (int)rel);
            bathroom_light_state = rel;
        } else {
            if(bathroom_light_control == 1){
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, (int)rel);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, (int)rel);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, (int)rel);
                bathroom_light_state = rel;
            } else {
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_VALUE0);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_GPIO_VALUE0);
                GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE0);
                bathroom_light_state = 0;
            }
        }
        
        // 风扇PWM控制模块 - 使用MOSFET控制
        if (fan_level > 0 && fan_level <= 3 && bathroom_fan_state == 1){
            // 计算PWM占空比: level 1-3 对应不同的占空比
            // level 1: 30% 占空比, level 2: 60%, level 3: 90%
            unsigned short duty_cycle = PWM_FREQ_DIVITION * fan_level * 30 / 100;
            
            // 启动PWM，频率约20kHz，避免啸叫
            PwmStart(WIFI_IOT_PWM_PORT_PWM1, duty_cycle, PWM_FREQ_DIVITION);
        } else {
            // 关闭风扇
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