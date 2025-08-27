#ifndef __BATHROOM_TASK_H__
#define __BATHROOM_TASK_H__

extern int bathroom_state;
extern int bathroom_light_state;
extern int bathroom_fan_state;
extern int bathroom_light_control;
extern int fan_level;

void bathroom_task(void);
void bathroom_control_set_fan(int);
void bathroom_control_set_light(int);
#endif
