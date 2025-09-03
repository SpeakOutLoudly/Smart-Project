#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_pwm.h"
#include "livingroom_task.h"

#include "wifiiot_adc.h"
#include "wifiiot_errno.h"
#include "wifiiot_i2c.h"

//对应管脚PIN11
#include "aht20.h"


// 静态函数前向声明
//static void livingroom_light_control(char *value);
static void livingroom_fan_control(char *param, char *value);
static void livingroom_fire_alarm_control(char *value);
static void livingroom_Biglight_control(char *value);
static void livingroom_Walllight_control(char *value);
static void livingroom_Smalllight_control(char *value);
static void livingroom_WaterPump_control(char *param, char *value);

/**
 * 引脚管理
 * 红绿灯板：
 * 灯光：
 * Pin10：GPIO 红灯控制
 * Pin6：GPIO 绿灯控制    由于与mq2冲突，改为GPIO6
 * Pin12：GPIO 蓝灯控制
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
 * Pin5：GPIO 火焰传感器(待测试)

 * 温湿度传感器：
 * Pin13：I2C0 SDA
 * Pin14：I2C0 SCL
 * Pin11：ADC 天然气传感器
 * 
 * LED显示器：
 * Pin13
 * Pin14
 * 
 * 水泵(弃用)：
 * Pin0：PWM3 速率控制（原来10）
 * Pin1：GPIO 电源（原来5）
 */
// PWM频率分频常量定义
#define PWM_DUTY 64000
#define PWM_FREQ_DIVITION 64000
#define AHT20_BAUDRATE 400 * 1000
#define AHT20_I2C_IDX WIFI_IOT_I2C_IDX_0
#define GAS_SENSOR_CHAN_NAME WIFI_IOT_ADC_CHANNEL_5

static int livingroom_fan_state = 0;
static int livingroom_light_state = 0;
static int livingroom_fire_status = 0;
static int livingroom_alarm_status = 0;
static int livingroom_WaterPump_state = 0;
static int livingroom_WaterPump_level = 0;
static int fan_level = 0;


static int gas_sensor_value = 0;
static float temperature = 0.0f;
static float humidity = 0.0f;



//红绿灯引脚初始化
static void Init_Light_GPIO(void){
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_IO_FUNC_GPIO_10_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_DIR_OUT);

    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_6, WIFI_IOT_IO_FUNC_GPIO_6_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_6, WIFI_IOT_GPIO_DIR_OUT);

    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_IO_FUNC_GPIO_12_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_DIR_OUT);

}

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

//水泵引脚初始化
static void Init_WaterPump_GPIO(void){
    //速率控制
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_0, WIFI_IOT_IO_FUNC_GPIO_0_PWM3_OUT);
    PwmInit(WIFI_IOT_PWM_PORT_PWM3);

    //电源控制
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_1, WIFI_IOT_IO_FUNC_GPIO_1_GPIO);
    GpioSetDir(WIFI_IOT_IO_NAME_GPIO_1, WIFI_IOT_GPIO_DIR_OUT);
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


static void livingroom_init(void *arg){
    (void)arg;
    GpioInit();
    Init_Light_GPIO();
    Init_Fan_GPIO();
    Init_Firesensor_GPIO();
    Init_Beeper_GPIO();
    Init_Aht20_GPIO();
    Init_WaterPump_GPIO();
    printf("[livingroom] Initialization completed\n");
}


/**
 * @brief 读取aht20传感器数据
 */

 static void livingroom_read_aht20(void){
    uint32_t retval = 0;
    retval = AHT20_StartMeasure();
    printf("AHT20_StartMeasure: %d\n", retval);
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

static void livingroom_read_gas_sensor(void){
    unsigned short data = 0;
    if(AdcRead(GAS_SENSOR_CHAN_NAME,&data, WIFI_IOT_ADC_EQU_MODEL_4, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0) == WIFI_IOT_SUCCESS){
        gas_sensor_value = data;
    }
}


/**
 * @brief 读取火焰传感器数据
 */

static void livingroom_read_fire_sensor(void){
    WifiIotGpioValue value;
    GpioGetInputVal(WIFI_IOT_IO_NAME_GPIO_5, &value); //获取火焰状态,0表示探测到火焰，1表示未探测到

    //获取到的值转化为火灾状态
    livingroom_fire_status = value ? 0 : 1;
}

// ==================== 硬件主要逻辑入口 ====================

static void livingroom_entry(void *arg){
    (void)arg;
    livingroom_init(NULL);
    printf("[livingroom] Smart livingroom system started in AUTO mode\n");

    //aht20初始化校准
    if(WIFI_IOT_SUCCESS != AHT20_Calibrate()){
        printf("AHT20 sensor init failed!\r\n");
        sleep(3);
    }

    while(1){
        livingroom_read_aht20();
        livingroom_read_fire_sensor();
        livingroom_read_gas_sensor();
        //灯光手动控制

        //风扇手动控制
        if(livingroom_fan_state == 1){
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE1);
            PwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_FREQ_DIVITION / 10 * (fan_level + 7),
            PWM_FREQ_DIVITION);
        } else{
            GpioSetOutputVal(FAN_POWER_GPIO, WIFI_IOT_GPIO_VALUE0);
            PwmStop(WIFI_IOT_PWM_PORT_PWM1);   
        }

        //水泵手动控制
        if(livingroom_WaterPump_state == 1){
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_1, WIFI_IOT_GPIO_VALUE1);
            PwmStart(WIFI_IOT_PWM_PORT_PWM3, PWM_FREQ_DIVITION / 10 * (livingroom_WaterPump_level + 7),
            PWM_FREQ_DIVITION);
        } else{
            GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_1, WIFI_IOT_GPIO_VALUE0);
            PwmStop(WIFI_IOT_PWM_PORT_PWM3);   
        }
        sleep(2);
    }
}



// ==================== 硬件外部查询接口 ====================

float Query_Temperature(void){
    return temperature;
}
float Query_Humidity(void){
    return humidity;
}
int Query_Gas_Sensor_Value(void){
    return gas_sensor_value;
}


// ==================== 系统控制接口 ====================

/**
 * @brief 启动浴室智能控制系统任务
 */
void Main_Task(void){
    osThreadAttr_t attr;
    attr.name = "livingroom_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    if(osThreadNew(livingroom_entry, NULL, &attr) == NULL){
        printf("[livingroom_task] Failed to create livingroom_task!\r\n");
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
    if(strcmp(target, "1") == 0){                    //1号灯
        livingroom_Biglight_control(value);            
    }else if(strcmp(target, "9") == 0){              //2号灯     
        livingroom_Walllight_control(value);
    }else if(strcmp(target, "10") == 0){             //3号灯     
        livingroom_Smalllight_control(value);
    }else if(strcmp(target, "3") == 0){              //风扇
        livingroom_fan_control(param, value);
    }else if(strcmp(target, "8") == 0){              //火焰报警
        livingroom_fire_alarm_control(value);
    }else if(strcmp(target, "999") == 0){             //舍弃
        livingroom_WaterPump_control(param, value);
    }
}


/**
 * @brief 控制火焰传感器报警
 */

static void livingroom_fire_alarm_control(char *value){
    uint16_t freqDivisor = 34052;

    if(strcmp(value, "1") == 0){
        livingroom_alarm_status = 1;
        PwmStart(WIFI_IOT_PWM_PORT_PWM0, freqDivisor / 2, freqDivisor);
    }else if(strcmp(value, "0") == 0){
        livingroom_alarm_status = 0;
        PwmStop(WIFI_IOT_PWM_PORT_PWM0);
    }
}

/**
 * @brief 控制风扇
 */

 static void livingroom_fan_control(char *param, char *value)
 {
    param = param;
     int value_int = atoi(value);
     if(value_int < 0 || value_int > 3){
         printf("livingroom_fan_control: 无效的风扇级别 %d（应为0-3）\n", value_int);
         return;
     }else if(value_int == 0){
         printf("关闭风扇\n");
 
         livingroom_fan_state = 0;
         fan_level = 0;
         // 移除这行：Control_Status_Fan = 0; 让手动控制持续有效
     }else{
         printf("打开风扇\n");
         livingroom_fan_state = 1;
         fan_level = value_int;
 
     }
 }
 /**
  * @brief 控制灯光
  */
static void livingroom_Biglight_control(char *value){
    int light_num = atoi(value);  // 将字符串转换为整数
    switch(light_num) {
    case 1:
        printf("打开1号灯光\n");
        livingroom_light_state = 1;
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_VALUE1);
        break;
        
    case 0:
        printf("关闭1号灯光\n");
        livingroom_light_state = 0;
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_10, WIFI_IOT_GPIO_VALUE0);
        break;

    default:
        printf("无效的灯光控制值: %s\n", value);
        break;
    }
}

static void livingroom_Walllight_control(char *value){
    int light_num = atoi(value);  // 将字符串转换为整数
    switch(light_num) {
    case 1:
        printf("打开2号灯光\n");
        livingroom_light_state = 1;
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_6, WIFI_IOT_GPIO_VALUE1);
        break;
        
    case 0:
        printf("关闭2号灯光\n");
        livingroom_light_state = 0;
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_6, WIFI_IOT_GPIO_VALUE0);
        break;

    default:
        printf("无效的灯光控制值: %s\n", value);
        break;
    }
}

static void livingroom_Smalllight_control(char *value){
    int light_num = atoi(value);  // 将字符串转换为整数
    switch(light_num) {
    case 1:
        printf("打开3号灯光\n");
        livingroom_light_state = 1;
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE1);
        break;
        
    case 0:
        printf("关闭3号灯光\n");
        livingroom_light_state = 0;
        GpioSetOutputVal(WIFI_IOT_IO_NAME_GPIO_12, WIFI_IOT_GPIO_VALUE0);
        break;

    default:
        printf("无效的灯光控制值: %s\n", value);
        break;
    }
}

static void livingroom_WaterPump_control(char *param, char *value){
    param = param;
    int value_int = atoi(value);
    if(value_int < 0 || value_int > 3){
        printf("livingroom_WaterPump_control: 无效的空气净化器级别 %d（应为0-3）\n", value_int);
        return;
    } 
    if(value_int == 0){
        printf("关闭空气净化器\n");
        livingroom_WaterPump_state = 0;

    }else{
        printf("打开空气净化器\n");
        livingroom_WaterPump_state = 1;
        livingroom_WaterPump_level = value_int;
    }
}
