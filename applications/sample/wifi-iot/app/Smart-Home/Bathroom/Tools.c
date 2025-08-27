#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bathroom_task.h"
#include "cJSON.h"

/*
 * 硬件控制
 * 接收解析后的命令，并根据命令执行不同的硬件控制
 * 
 * 后续可在此函数增加控制
 * 
*/

// 风扇控制函数
void bathroom_Fan_Control(char *value) {
    int value_int = atoi(value);
    if (value_int == 0) {
        printf("bathroom_Fan_Control: value is NULL\n");
        return;
    }
    
    printf("风扇控制命令: %s\n", value);
    
    if (value_int >= 1) {
        printf("开启浴室风扇\n");
        bathroom_control_set_fan(value_int);
    }
    else if (value_int == 0) {
        printf("关闭浴室风扇\n");
        bathroom_control_set_fan(value_int);
    }
    else {
        printf("bathroom_Fan_Control: 未知值 %s\n", value);
    }
}

// 灯光控制函数
void bathroom_Light_Control(char *value) {
    if (value == NULL) {
        printf("bathroom_Light_Control: value is NULL\n");
        return;
    }
    
    printf("灯光控制命令: %s\n", value);
    
    if (strcmp(value, "ON") == 0) {
        printf("开启浴室灯光\n");
        bathroom_control_set_light(1);
    }
    else if (strcmp(value, "OFF") == 0) {
        printf("关闭浴室灯光\n");
        bathroom_control_set_light(0);
    }
    else {
        printf("bathroom_Light_Control: 未知值 %s\n", value);
    }
}

/*
 * 硬件控制主函数
 * 根据target选择相应的处理函数，并传入value
 */
void hardware_control(char *target, char *value) {
    if (target == NULL || value == NULL) {
        printf("hardware_control: target或value为NULL\n");
        return;
    }

    printf("执行硬件控制 - 目标: %s, 值: %s\n", target, value);

    // 使用字符串比较来判断目标设备类型
    if (strcmp(target, "FAN") == 0) {
        bathroom_Fan_Control(value);
    }
    else if (strcmp(target, "LIGHT") == 0) {
        bathroom_Light_Control(value);
    }
    else {
        printf("hardware_control: 未知目标设备 %s\n", target);
    }
}

/*
 * JSON解析函数
 * 解析格式: {"msg": "LIGHT_ON"}
 * 输入: payload_in - 接收到的字符数组
 *       payload_len - 数据长度
 * 输出: 解析出的命令字符串，如果解析失败返回NULL
 */
/*
 * 新的JSON解析和处理函数
 * 解析格式: {"target": "FAN", "value": "ON"}
 * 输入: payload_in - 接收到的字符数组
 *       payload_len - 数据长度
 * 输出: 0表示成功，-1表示失败
 */
int json_parse_and_control(unsigned char *payload_in, unsigned int payload_len) {
    char *buf = NULL;
    cJSON *root = NULL;
    cJSON *target_item = NULL;
    cJSON *value_item = NULL;
    char *target = NULL;
    char *value = NULL;
    int ret = -1;
    
    // 检查输入参数
    if (payload_in == NULL || payload_len <= 0) {
        printf("json_parse_and_control: 无效的输入参数\n");
        return -1;
    }
    
    // 分配内存并复制数据
    buf = (char *)malloc(payload_len + 1);
    if (buf == NULL) {
        printf("json_parse_and_control: 内存分配失败\n");
        return -1;
    }
    
    memcpy(buf, payload_in, payload_len);
    buf[payload_len] = '\0';
    
    printf("解析JSON: %s\n", buf);
    
    // 解析JSON
    root = cJSON_Parse(buf);
    if (root == NULL) {
        printf("json_parse_and_control: JSON解析失败\n");
        goto cleanup;
    }
    
    // 获取target字段
    target_item = cJSON_GetObjectItem(root, "target");
    if (target_item == NULL || !cJSON_IsString(target_item)) {
        printf("json_parse_and_control: 未找到'target'字段或类型错误\n");
        goto cleanup;
    }
    target = target_item->valuestring;
    
    // 获取value字段
    value_item = cJSON_GetObjectItem(root, "value");
    if (value_item == NULL || !cJSON_IsString(value_item)) {
        printf("json_parse_and_control: 未找到'value'字段或类型错误\n");
        goto cleanup;
    }
    value = value_item->valuestring;
    
    printf("解析成功 - target: %s, value: %s\n", target, value);
    
    // 调用硬件控制函数
    hardware_control(target, value);
    ret = 0;
    
cleanup:
    if (root) {
        cJSON_Delete(root);
    }
    if (buf) {
        free(buf);
    }
    
    return ret;
}

/*
 * 兼容性函数 - 保持旧接口
 * 已弃用，建议使用json_parse_and_control
 */
char* json_parse_command(unsigned char *payload_in, unsigned int payload_len) {
    printf("json_parse_command: 此函数已弃用，请使用json_parse_and_control\n");
    
    // 调用新函数进行处理
    if (json_parse_and_control(payload_in, payload_len) == 0) {
        return "SUCCESS"; // 返回成功标识
    }
    return NULL;
}