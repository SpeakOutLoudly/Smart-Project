#ifndef __BATHROOM_TASK_H__
#define __BATHROOM_TASK_H__


//应该保留的接口文档，其余不用的写成静态接口即可

// ==================== 系统状态查询接口 ====================

/**
 * @brief 相关状态查询接口
 * 接口名：
 * Query_Device_Status
 */
int Query_Room_Status(void);
int Query_Light_Status(void);
int Query_Fan_Status(void);
int Query_Fan_Level(void);
void Status_Query(void);
// ==================== 系统控制接口 ====================

/**
 * @brief 启动浴室智能控制系统任务
 */
void Main_Task(void);

// ==================== 硬件控制总接口 ====================

/**
 * @brief 硬件控制总接口
 * @param target 目标设备类型
 * @param value 控制值
 */
void Hardware_Control(char *target, char *value);

#endif
