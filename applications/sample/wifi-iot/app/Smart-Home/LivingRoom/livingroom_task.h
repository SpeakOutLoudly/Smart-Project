#ifndef __LIVINGROOM_TASK_H__
#define __LIVINGROOM_TASK_H__


//应该保留的接口文档，其余不用的写成静态接口即可

// ==================== 系统状态查询接口 ====================

/**
 * @brief 相关状态查询接口
 * 接口名：
 * Query_Device_Status
 */
int Query_Fire_Status(void);
int Query_Light_Status(void);
int Query_Fan_Status(void);
int Query_Fan_Level(void);
int Query_Alarm_Status(void);
float Query_Temperature(void);
float Query_Humidity(void);

void Status_Query(void);
// ==================== 系统控制接口 ====================

/**
 * @brief 启动客厅智能控制系统任务
 */
void Main_Task(void);

// ==================== 硬件控制总接口 ====================

/**
 * @brief 客厅硬件控制总接口
 * @param target 目标设备类型
 * @param value 控制值
 */
void Hardware_Control(char *target, char *param, char *value);

#endif
