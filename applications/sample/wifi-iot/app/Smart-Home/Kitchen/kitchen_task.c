#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"
#include "kitchen_task.h"

#include "wifiiot_adc.h"
#include "wifiiot_errno.h"
#include "wifiiot_i2c.h"

//对应管脚PIN11
#include "aht20.h"

// 静态函数前向声明

static void kitchen_fan_control(char *param,char *value);

static void kitchen_fire_alarm_control(char *value);

static void kitchen_waterpump_control(char *param,char *value);

/**
 * 引脚管理
 * 蜂鸣器：
 * Pin9：PWM0 蜂鸣器控制
 * 
 * 风扇板：
 * 风扇：
 * Pin8：PWM1 风扇转速控制
 * Pin7：GPIO 风扇电源控制
 * 
 * 火焰传感器板：
 * 火焰传感器：
 * Pin5：GPIO 火焰传感器

 * 温湿度传感器：
 * Pin13：I2C0 SDA
 * Pin14：I2C0 SCL
 * 
 * LED显示器：
 * Pin13
 * Pin14
 * 
 * 水泵：
 * Pin6：GPIO 水泵控制
 * Pin10：PWM1 水泵转速控制
 */
// PWM频率分频常量定义
#define PWM_DUTY 64000
#define PWM_FREQ_DIVITION 64000
#define AHT20_BAUDRATE 400 * 1000
#define AHT20_I2C_IDX WIFI_IOT_I2C_IDX_0
#define GAS_SENSOR_CHAN_NAME WIFI_IOT_ADC_CHANNEL_5

static int kitchen_fan_state = 0;
static int kitchen_fire_status = 0;
static int kitchen_alarm_status = 0;
static int fan_level = 0;
static int kitchen_water_pump_state = 0;
static int kitchen_water_pump_level = 0;

static float temperature = 0.0f;
static float humidity = 0.0f;
static int gas_sensor_value = 0;



//蜂鸣器
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
//火焰传感器引脚初始化
static void Init_Firesensor_GPIO(void){
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_IO_FUNC_GPIO_5_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_GPIO_DIR_IN);
}

//水泵引脚初始化
#define WATER_PUMP_PIN_NAME WIFI_IOT_IO_NAME_GPIO_10
#define WATER_PUMP_POWER_PIN WIFI_IOT_IO_NAME_GPIO_6

static void Init_WaterPump_GPIO(void){
     // 初始化PWM引脚用于水泵调速
    IoSetFunc(WATER_PUMP_PIN_NAME, WIFI_IOT_IO_FUNC_GPIO_10_PWM1_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM1);

    // 初始化水泵电源控制引脚 原来为5，现在为6
    IoSetFunc(WATER_PUMP_POWER_PIN, WIFI_IOT_IO_FUNC_GPIO_6_GPIO);
    GpioSetDir(WATER_PUMP_POWER_PIN, WIFI_IOT_GPIO_DIR_OUT);

    // 初始状态关闭水泵
    GpioSetOutputVal(WATER_PUMP_POWER_PIN, WIFI_IOT_GPIO_VALUE0);
}



static void kitchen_init(void *arg){
    (void)arg;
    GpioInit();
    Init_Fan_GPIO();
    Init_Firesensor_GPIO();
    Init_WaterPump_GPIO();
    Init_Beeper_GPIO();
    Init_Aht20_GPIO();
    printf("[kitchen] Initialization completed\n");
}


/**
 * @brief 读取aht20传感器数据
 */

 static void kitchen_read_aht20(void){
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

static void kitchen_read_gas_sensor(void){
    unsigned short data = 0;
    if(AdcRead(GAS_SENSOR_CHAN_NAME,&data, WIFI_IOT_ADC_EQU_MODEL_4, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0) == WIFI_IOT_SUCCESS){
        gas_sensor_value = data;
    }
}



/**
 * @brief 读取火焰传感器数据
 */

static void kitchen_read_fire_sensor(void){
    WifiIotGpioValue value;
    GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_5, &value); //获取火焰状态,0表示探测到火焰，1表示未探测到

    //获取到的值转化为火灾状态
    kitchen_fire_status = value ? 0 : 1;
}

// ==================== 硬件主要逻辑入口 ====================

static void kitchen_entry(void *arg){
    (void)arg;
    kitchen_init(NULL);
    printf("[kitchen] Smart kitchen system started in AUTO mode\n");

    //aht20初始化校准
    if(WIFI_IOT_SUCCESS != AHT20_Calibrate()){
        printf("AHT20 sensor init failed!\r\n");
        sleep(3);
    }

    while(1){
        kitchen_read_aht20();
        kitchen_read_fire_sensor();
        kitchen_read_gas_sensor();

        

        //风扇手动控制
        if(kitchen_fan_state == 1){
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE1);
            PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_FREQ_DIVITION / 10 * (fan_level + 7),
            PWM_FREQ_DIVITION);
        } else{
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
            PwmStop(WIFI_IOT_PWM_PORT_PWM1);
            
        }
        sleep(1);

         //水泵手动控制
         if (kitchen_water_pump_level > 0 && kitchen_water_pump_level < 4)
        {
            GpioSetOutputVal(WATER_PUMP_POWER_PIN, WIFI_IOT_GPIO_VALUE1);
            PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_FREQ_DIVITION / 10 * (kitchen_water_pump_level + 7), PWM_FREQ_DIVITION);
        }
        else
        {
            GpioSetOutputVal(WATER_PUMP_POWER_PIN, WIFI_IOT_GPIO_VALUE0);
            PwmStop(WIFI_IOT_PWM_PORT_PWM1);
        }
        sleep(1); //睡眠
    }

   

}





// ==================== 硬件外部查询接口 ====================


float Query_Temperature(void){
    return temperature;
}
float Query_Humidity(void){
    return humidity;
}
int Query_gas_sensor_value(void){
    return gas_sensor_value;
}


// ==================== 系统控制接口 ====================

/**
 * @brief 启动厨房智能控制系统任务
 */
void Main_Task(void){
    osThreadAttr_t attr;
    attr.name = "kitchen_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    if(osThreadNew(kitchen_entry, NULL, &attr) == NULL){
        printf("[kitchen_task] Failed to create kitchen_task!\r\n");
    }
}
// ==================== 硬件控制总接口 ====================

/**
 * @brief 硬件控制总接口
 * @param target 目标设备类型
 * @param value 控制值
 */
void Hardware_Control(char *target, char *param, char *value){   //从设备名解析改为编号解析，具有映射见文档（后续会更改）
    param = param;
    if(strcmp(target, "12") == 0){                    //厨房灯
        kitchen_waterpump_control(param, value);
    }else if(strcmp(target, "13") == 0){
        kitchen_fan_control(param, value);
    }else if(strcmp(target, "8") == 0){
        kitchen_fire_alarm_control(value);
    }
}
/**
 * @brief 控制火焰传感器报警
 */

static void kitchen_fire_alarm_control(char *value){
    uint16_t freqDivisor = 34052;

    if(strcmp(value, "ON") == 0){
        kitchen_alarm_status = 1;
        PwmStart(WIFI_IOT_PWM_PORT_PWM0, freqDivisor / 2, freqDivisor);
    }else if(strcmp(value, "OFF") == 0){
        kitchen_alarm_status = 0;
        PwmStop(WIFI_IOT_PWM_PORT_PWM0);
    }
}

/**
 * @brief 控制风扇
 */

 static void kitchen_fan_control(char *param,char *value)
 {
     param = param;
     int value_int = atoi(value);
     if(value_int < 0 || value_int > 3){
         printf("kitchen_fan_control: 无效的风扇级别 %d（应为0-3）\n", value_int);
         return;
     }else if(value_int == 0){
         printf("关闭风扇\n");

         kitchen_fan_state = 0;
         fan_level = 0;
         // 移除这行：Control_Status_Fan = 0; 让手动控制持续有效
     }else{
         printf("打开风扇\n");
         kitchen_fan_state = 1;
         fan_level = value_int;
 
     }
 }

/**
 * @brief 控制水泵
 */
static void kitchen_waterpump_control(char *param,char *value)
{
    param = param;
    int value_int = atoi(value);
    if(value_int < 0 || value_int > 4){
        printf("kitchen_waterpump_control: 无效的水泵级别 %d（应为0-4）\n", value_int);
         return;
     }else if(value_int == 0){
         printf("关闭水泵\n");

         kitchen_water_pump_state = 0;
         kitchen_water_pump_level = 0;
         // 移除这行：Control_Status_Fan = 0; 让手动控制持续有效
     }else{
         printf("打开水泵\n");
         kitchen_water_pump_state = 1;
         kitchen_water_pump_level = value_int;

     }
 }


