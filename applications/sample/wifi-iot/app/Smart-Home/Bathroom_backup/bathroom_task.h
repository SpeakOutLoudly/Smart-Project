/**
 * @file bathroom_task.h
 * @brief 浴室智能控制系统头文件
 * @details 仅暴露查询接口、模式控制接口和硬件控制总入口
 */

#ifndef __BATHROOM_TASK_H__
#define __BATHROOM_TASK_H__

// ==================== 系统状态查询接口 ====================

/**
 * @brief 获取浴室占用状态
 * @return int 0-无人，1-有人
 */
int bathroom_get_occupancy_status(void);

/**
 * @brief 获取浴室灯光状态
 * @return int 0-关闭，1-开启
 */
int bathroom_get_light_status(void);

/**
 * @brief 获取浴室风扇状态
 * @return int 0-关闭，1-3-风扇档位
 */
int bathroom_get_fan_status(void);

/**
 * @brief 获取环境光线状态
 * @return int 0-明亮，1-昏暗
 */
int bathroom_get_light_sensor_status(void);

/**
 * @brief 获取当前控制模式
 * @return int 0-自动模式，1-手动模式
 */
int bathroom_get_control_mode(void);

void Status_Query(void);

// ==================== 模式控制接口 ====================

/**
 * @brief 设置浴室控制模式
 * @param mode 0-自动模式，1-手动模式
 * @return int 0-成功，-1-失败
 */
int bathroom_set_control_mode(int mode);

/**
 * @brief 模式控制（字符串版）
 * @param value "AUTO"/"MANUAL"/"0"/"1"（大小写均可）
 */
void bathroom_Mode_Control(char *value);

/**
 * @brief 风扇控制（字符串版）
 * @param value 风扇档位："0"-关闭，"1"-"3"-对应档位
 */
void bathroom_Fan_Control(char *value);

/**
 * @brief 灯光控制（字符串版）
 * @param value 灯光状态："ON"/"OFF"/"1"/"0"（大小写均可）
 */
void bathroom_Light_Control(char *value);

// ==================== 系统控制接口 ====================

/**
 * @brief 启动浴室智能控制系统任务
 */
void bathroom_task(void);

// ==================== 硬件控制总接口 ====================

/**
 * @brief 硬件控制总接口
 * @param target 目标设备类型
 * @param value 控制值
 */
void hardware_control(char *target, char *value);

#endif /* __BATHROOM_TASK_H__ */