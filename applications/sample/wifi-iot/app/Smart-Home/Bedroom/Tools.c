#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MQTTPacket.h"
#include "bedroom_task.h"
#include "cJSON.h"
#include "transport.h"


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
    char *param = NULL;
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
    if (target_item == NULL || !cJSON_IsNumber(target_item)) {
        printf("json_parse_and_control: 未找到'target'字段或类型错误\n");
        goto cleanup;
    }
    target = target_item->valuestring;
    
    // 获取param字段
    cJSON *param_item = cJSON_GetObjectItem(root, "param");
    if (param_item == NULL || !cJSON_IsString(param_item)) {
        printf("json_parse_and_control: 未找到'param'字段或类型错误\n");
        goto cleanup;
    }
    param = param_item->valuestring;

    // 获取value字段
    value_item = cJSON_GetObjectItem(root, "value");
    if (value_item == NULL || !cJSON_IsString(value_item)) {
        printf("json_parse_and_control: 未找到'value'字段或类型错误\n");
        goto cleanup;
    }
    value = value_item->valuestring;
    
    printf("解析成功 - target: %s, value: %s\n", target, value);
    
    // 调用硬件控制函数
    Hardware_Control(target, param, value);
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

// 生成：{"deviceId": <id>, "param": <value>}
static int json_Submit(int deviceId, int value, char *payload, size_t payload_size, char *paraName) {
    int n = snprintf(payload, payload_size, "{\"deviceId\":%d,\"%s\":%d}", deviceId, paraName, value);
    if (n < 0 || (size_t)n >= payload_size) return -1; // 防止溢出
    return 0;
}

// 封装单个参数的发布流程
int publish_param(int mysock, 
                         unsigned char *buf, int buflen, 
                         char *payload, size_t payload_size,
                         MQTTString topicString,
                         int deviceId, int value, char *paraName,
                         const char *desc)
{
    int payloadlen, len, rc;

    if (json_Submit(deviceId, value, payload, payload_size, paraName) == 0) {
        payloadlen = strlen(payload);

        len = MQTTSerialize_publish(buf, buflen,
                                    0,   // dup = 0
                                    0,   // qos = 0
                                    0,   // retained = 0
                                    0,   // msgid = 0
                                    topicString,
                                    (unsigned char*)payload,
                                    payloadlen);

        rc = transport_sendPacketBuffer(mysock, buf, len);
        printf("发布状态(%s): %s\n", desc, payload);
        return rc;
    } else {
        printf("构造 JSON 失败: %s\n", desc);
        return -1;
    }
}