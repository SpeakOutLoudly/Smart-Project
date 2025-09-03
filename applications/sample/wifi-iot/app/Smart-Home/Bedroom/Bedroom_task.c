#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"
#include "bedroom_task.h"
#include "wifiiot_adc.h"
#include "wifiiot_errno.h"
#include "wifiiot_i2c.h"

// PWM频率分频常量定义
#define PWM_DUTY 64000
#define PWM_Gear_1 19200
#define PWM_Gear_2 38400
#define PWM_Gear_3 64000
#define PWM_FREQ_DIVITION 64000
#define AHT20_BAUDRATE 400 * 1000
#define AHT20_I2C_IDX WIFI_IOT_I2C_IDX_0
#define GAS_SENSOR_CHAN_NAME WIFI_IOT_ADC_CHANNEL_5


static int bedroom_state = 0; // 0:无人, 1:有人
static int bedroom_fan_state = 0;
// static int bedroom_light_state = 0;
static int fan_level = 1;
static int bedroom_alarm_status = 0;
static float temperature = 0.0f;
static float humidity = 0.0f;
// 添加灯光状态数组声明
// static int lights_stat[3] = {0}; // 假设有3个灯光通道
// static int Control_Status_Light = 0;  // 0:自动模式, 1:待执行手动操作, 2:手动模式(已执行)
// static char *Control_Value_Light = "OFF";


//对应管脚PIN11
#include "aht20.h"

// 静态函数前向声明

static void bedroom_fan_control(char *param,char *value);

static void bedroom_fan_control(char *param,char *value);


/**
 * 引脚管理
 * 蜂鸣器：
 * Pin9：PWM0 蜂鸣器控制
 * 
 * 风扇板：
 * 风扇：
 * Pin8：PWM1 风扇转速控制
 * Pin3：GPIO 风扇电源控制  原本是12
 * 
 *灯光：
 * Pin10：PWM2 灯光控制
 * Pin2：PWM3 灯光控制
 * Pin12：PWM4 灯光控制
 *
 * 光敏电阻：
 * Pin5：GPIO 光敏电阻   原本为9
 * 
 * 人体红外传感器：
 * Pin7：GPIO 人体红外传感器
 * 
 * 温湿度传感器：
 * Pin13：I2C0 SDA
 * Pin14：I2C0 SCL
 * 
 * gas:
 * Pin11
 * 
 * LED显示器：
 * Pin13
 * Pin14
 * 

 */



//蜂鸣器   GPIO 9,13,14
static void Init_Beeper_GPIO(void){
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_IO_FUNC_GPIO_9_PWM0_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM0);
}

static void Init_Aht20_GPIO(void){
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_13, WIFI_IOT_IO_FUNC_GPIO_13_I2C0_SDA);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_14, WIFI_IOT_IO_FUNC_GPIO_14_I2C0_SCL);
    I2cInit(AHT20_I2C_IDX,AHT20_BAUDRATE);
}

//风扇引脚初始化   
#define FAN_POWER_GPIO WIFI_IOT_IO_NAME_GPIO_7

static void Init_Fan_GPIO(void){
    // 初始化PWM引脚用于风扇调速 原来为8，现在为8
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_8, WIFI_IOT_IO_FUNC_GPIO_8_PWM1_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM1);
    
    // 初始化风扇电源控制引脚 原来为12，现在为7
    IoSetFunc(FAN_POWER_GPIO, WIFI_IOT_IO_FUNC_GPIO_7_GPIO);
    GpioSetDir(FAN_POWER_GPIO, WIFI_IOT_GPIO_DIR_OUT);
    
    // 初始状态关闭风扇
    GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
}


// 灯光引脚初始化  GPIO 10,2,12
// static int light_pin[3] = {
//     WIFI_IOT_PWM_PORT_PWM1, // R
//     WIFI_IOT_PWM_PORT_PWM2, // G
//     WIFI_IOT_PWM_PORT_PWM3  // B
// };

// static void Init_Light_GPIO(void){
//     IoSetFunc(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_IO_FUNC_GPIO_10_PWM1_OUT);
//     IoSetFunc(WIFI_IOT_IO_NAME_GPIO_2, WIFI_IOT_IO_FUNC_GPIO_2_PWM2_OUT);
//     IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_PWM3_OUT);

//     // 初始化 PWM 端口
//     PwmInit(WIFI_IOT_PWM_PORT_PWM1); // R
//     PwmInit(WIFI_IOT_PWM_PORT_PWM2); // G
//     PwmInit(WIFI_IOT_PWM_PORT_PWM3); // B
    
//     // 初始化灯光状态
//     lights_stat[0] = 0;
//     lights_stat[1] = 0;
//     lights_stat[2] = 0;
// }

// //光敏电阻，人体红外传感器引脚初始化  GPIO5

// void Init_Photoresistor_GPIO(void){
//     // 初始化光敏电阻传感器  5,原本为9
//     IoSetFunc(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_IO_FUNC_GPIO_5_GPIO);
//     GpioSetDir(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_GPIO_DIR_IN);
// }

// void Init_Infrared_sensor_GPIO(void){
//     // 初始化人体红外传感器   GPIO7   
//     IoSetFunc(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_FUNC_GPIO_7_GPIO);
//     GpioSetDir(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_GPIO_DIR_IN);
//     IoSetPull(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_PULL_UP);
// }




void bedroom_init(void *arg){
    (void)arg;
    GpioInit();

    // Init_Light_GPIO();
    Init_Fan_GPIO();
    //Init_Photoresistor_GPIO();
    //Init_Infrared_sensor_GPIO();
    Init_Beeper_GPIO();
    Init_Aht20_GPIO();

    printf("[Bedroom] Initialization completed\n");
}



/**
 * @brief 读取aht20传感器数据
 */

 static void bedroom_read_aht20(void){
    uint32_t retval = 0;
    retval = AHT20_StartMeasure();
    if (retval != WIFI_IOT_SUCCESS)
    {
        printf("trigger measure failed!\r\n");
    }
    else
    {
        // 接收测量结果，拼接转换为标准值
        retval = AHT20_GetMeasureResult(&temperature, &humidity);
        printf("temp: %.2f,  humi: %.2f\n", temperature, humidity);
    }
}

/**
 * @brief 读取天然气传感器数据
 */

static void bedroom_read_gas_sensor(void){
    unsigned short data = 0;
    if(AdcRead(GAS_SENSOR_CHAN_NAME,&data, WIFI_IOT_ADC_EQU_MODEL_4, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0) == WIFI_IOT_SUCCESS){
        printf("gas: %d\n", data);
    }
}



  

/**
 * @brief 控制卧室灯光
 */
// static void bedroom_control_set_light(char *value)
// {   
//     int light_num = atoi(value);
//     printf("进入bedroom_control_set_light函数\n");
    
//     // 更新灯光状态
//     if (light_num > 0) {
//         lights_stat[0] = 1;
//         lights_stat[1] = 1;
//         lights_stat[2] = 1;
//     } else {
//         lights_stat[0] = 0;
//         lights_stat[1] = 0;
//         lights_stat[2] = 0;
//     }
    
//     if (light_num == 1){
//         printf("打开卧室灯光\n");
//         PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_Gear_1, PWM_FREQ_DIVITION);
//         PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_Gear_1, PWM_FREQ_DIVITION);
//         PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_Gear_1, PWM_FREQ_DIVITION);
//         bedroom_light_state = 1;
//     }else if(light_num == 2){
//         printf("打开卧室灯光\n");
//         PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_Gear_2, PWM_FREQ_DIVITION);
//         PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_Gear_2, PWM_FREQ_DIVITION);
//         PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_Gear_2, PWM_FREQ_DIVITION);
//         bedroom_light_state = 1;
//     }else if(light_num == 3){
//         printf("打开卧室灯光\n");
//         PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_Gear_3, PWM_FREQ_DIVITION);
//         PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_Gear_3, PWM_FREQ_DIVITION);
//         PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_Gear_3, PWM_FREQ_DIVITION);
//         bedroom_light_state = 1;
//     }else if (light_num == 0){
//         printf("关闭卧室灯光\n");
//         PwmStop(WIFI_IOT_PWM_PORT_PWM1);
//         PwmStop(WIFI_IOT_PWM_PORT_PWM2);
//         PwmStop(WIFI_IOT_PWM_PORT_PWM3);
//         bedroom_light_state = 0;
//     }
//     else{
//         printf("无效的灯光控制值: %s\n", value);
//     }
// }


// // 重置为自动模式的函数
// static void reset_to_auto_mode(char *value) {
//     if (strcmp(value, "LIGHT") == 0) {
//         Control_Status_Light = 0;
//         printf("卧室灯光已重置为自动模式\n");
//     } else {
//         printf("reset_to_auto_mode: 未知设备类型 %s\n", value);
//     }
// }

// // 添加 bedroom_thread 函数
// static void bedroom_thread(void)
// {
//     for (int i = 0; i < 3; i++) {
//         // 根据 lights_stat 的状态设置 GPIO 输出
//         // 注意：这里需要根据实际的硬件连接来调整
//         if (lights_stat[i]) {
//             // 打开灯光
//             // 这里可能需要使用 PwmStart 而不是 GpioSetOutputVal
//             PwmStart(light_pin[i], PWM_DUTY, PWM_FREQ_DIVITION);
//         } else {
//             // 关闭灯光
//             PwmStop(light_pin[i]);
//         }
//     }
// }




// ==================== 硬件主要逻辑入口 ====================
void bedroom_entry(void *arg){
    (void)arg;

    bedroom_init(NULL);
    // WifiIotGpioValue rel = 0;
    //aht20初始化校准
    if(WIFI_IOT_SUCCESS != AHT20_Calibrate()){
        printf("AHT20 sensor init failed!\r\n");
        sleep(3);
    }

    while(1){
        // 执行灯光控制线程
        // bedroom_thread();
        bedroom_read_aht20();

        bedroom_read_gas_sensor();

        // 灯光控制逻辑
        // if(Control_Status_Light == 1){
        //     printf("卧室灯光进入手动模式\n");
        //     bedroom_control_set_light(Control_Value_Light);
        //     Control_Status_Light = 2;
        // } else if (Control_Status_Light == 0) {
        //     // 自动模式：读取人体红外传感器
        //     GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_5, &rel);
        //     bedroom_state = rel;
            
        //     //读取光敏电阻
        //     GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_9, &rel);

        //     //如果有人
        //     if (bedroom_state){
        //         if (bedroom_light_state == 0) {
        //             printf("卧室有人 - 自动开灯\n");
        //             PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_DUTY, PWM_FREQ_DIVITION);
        //             PwmStart(WIFI_IOT_PWM_PORT_PWM2, PWM_DUTY, PWM_FREQ_DIVITION);
        //             PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_DUTY, PWM_FREQ_DIVITION);
        //             bedroom_light_state = 1;
        //             // 更新灯光状态
        //             lights_stat[0] = 1;
        //             lights_stat[1] = 1;
        //             lights_stat[2] = 1;
        //         }
        //     }
        //     else
        //     {
        //         if (bedroom_light_state == 1) {
        //             printf("卧室无人 - 自动关灯\n");
        //             PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        //             PwmStop(WIFI_IOT_PWM_PORT_PWM2);
        //             PwmStop(WIFI_IOT_PWM_PORT_PWM3);
        //             bedroom_light_state = 0;
        //             // 更新灯光状态
        //             lights_stat[0] = 0;
        //             lights_stat[1] = 0;
        //             lights_stat[2] = 0;
        //         }
        //     }
        // }
        // sleep(1);

         //风扇手动控制
        if(bedroom_fan_state == 1){
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE1);
            PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_FREQ_DIVITION / 10 * (fan_level + 7),
            PWM_FREQ_DIVITION);
        } else{
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
            PwmStop(WIFI_IOT_PWM_PORT_PWM1);
            
        }
        sleep(1);

    }
}



// ==================== 硬件外部查询接口 ====================
void Status_Query(void) {
    printf("执行卧室状态查询\n");
    // printf("bedroom_light_state: %d\n", bedroom_light_state);
    printf("bedroom_fan_state: %d\n", bedroom_fan_state);
}

int Query_Room_Status(void){
    return bedroom_state;
}

// int Query_Light_Status(void){
//     return bedroom_light_state;
// }


int Query_Fan_Level(void){
    return fan_level;
}

int Query_Alarm_Status(void){
    return bedroom_alarm_status;
}


float Query_Temperature(void){
    return temperature;
}
float Query_Humidity(void){
    return humidity;
}


// ==================== 系统控制接口 ====================
void Main_Task(void){
    osThreadAttr_t attr;
    attr.name = "bedroom_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    if(osThreadNew(bedroom_entry, NULL, &attr) == NULL){
        printf("[bedroom_task] Failed to create bedroom_task!\r\n");
    }
}

// ==================== 硬件外部控制接口 ====================
void Hardware_Control(char *target, char *param, char *value) {
    param = param;
    if (target == NULL || value == NULL) {
        printf("hardware_control: target或value为NULL\n");
        return;
    }

    printf("执行卧室硬件控制 - 目标: %s, 参数: %s, 值: %s\n", target, param, value);

    if (strcmp(target, "FAN") == 0) {
        bedroom_fan_control(param, value);
    }
    // else if (strcmp(target, "LIGHT") == 0 && strcmp(param, "BRIGHTNESS") == 0) {
    //     Control_Status_Light = 1;
    //     Control_Value_Light = value;
    // }
    // else if (strcmp(target, "LIGHT") == 0 && strcmp(param, "MODE") == 0) {
    //     reset_to_auto_mode(value);
    // }
    else if (strcmp(target, "STATUS") == 0) {
        Status_Query();
    }
   
    else {
        printf("hardware_control: 未知目标设备 %s\n", target);
    }
}



/**
 * @brief 控制风扇
 */

 static void bedroom_fan_control(char *param,char *value)
 {
     param = param;
     int value_int = atoi(value);
     if(value_int < 0 || value_int > 3){
         printf("bedroom_fan_control: 无效的风扇级别 %d（应为0-3）\n", value_int);
         return;
     }else if(value_int == 0){
         printf("关闭风扇\n");

         bedroom_fan_state = 0;
         fan_level = 0;
         // 移除这行：Control_Status_Fan = 0; 让手动控制持续有效
     }else{
         printf("打开风扇\n");
         bedroom_fan_state = 1;
         fan_level = value_int;
 
     }
 }

