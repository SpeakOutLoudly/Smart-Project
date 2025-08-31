#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "livingroom_task.h"
#include "cJSON.h"


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
    Hardware_Control(target, value);
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

