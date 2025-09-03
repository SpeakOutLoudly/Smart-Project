#ifndef __MQTT_TASK_H__
#define __MQTT_TASK_H__

// 函数声明
int backup_wifi_connect(void);  // 备用WiFi连接（不推荐使用）
int check_network_status(void);
int mqtt_connect(void);
//void auto_network_reconnect(void);
void hardware_auto_networking(void);

// 全局变量声明（需要在对应的.c文件中定义）
extern int toStop;

#endif