#ifndef __BATHROOM_TASK_H__
#define __BATHROOM_TASK_H__

extern int bathroom_state;
extern int bathroom_light_state;
extern int bathroom_fan_state;
extern int bathroom_light_control;
extern int fan_level;
extern int fan_target_level;
extern int fan_current_duty;
extern int fan_error_count;

// 基本控制函数
void bathroom_task(void);
void bathroom_control_set_fan(int level);
void bathroom_control_set_light(int on);

// 风扇高级控制函数
void fan_power_control(int power_on);
void fan_speed_control(int level);
void fan_emergency_stop(void);
void get_fan_status_info(char *status_buffer, int buffer_size);

#endif
