#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include "hi_wifi_api.h"
#include "hi_errno.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"
#include "lwip/sockets.h"

#include "MQTTPacket.h"
#include "transport.h"
#include "bathroom_task.h"
#include "Tools.h"

int toStop = 0;

// 备用WiFi连接函数（当前不使用，主要使用wifi_utils.c中的连接）
int backup_wifi_connect(void) {
    // WiFi配置参数
    hi_wifi_assoc_request assoc_req = {0};
    
    // 配置WiFi参数（实际使用时需要修改为真实的WiFi信息）
    strcpy(assoc_req.ssid, "Line");        // WiFi名称
    strcpy(assoc_req.key, "123456silent");     // WiFi密码
    assoc_req.auth = HI_WIFI_SECURITY_WPA2PSK;       // 安全类型
    
    printf("正在连接WiFi: %s\n", assoc_req.ssid);
    
    // 启动WiFi STA模式
    if (hi_wifi_sta_start(NULL, NULL) != HISI_OK) {
        printf("WiFi STA启动失败\n");
        return -1;
    }
    
    // 连接WiFi
    if (hi_wifi_sta_connect(&assoc_req) != HISI_OK) {
        printf("WiFi连接失败\n");
        return -1;
    }
    
    // 等待连接完成
    usleep(5000000); // 等待5秒
    
    // 检查连接状态
    hi_wifi_status wifi_status;
    if (hi_wifi_sta_get_connect_info(&wifi_status) == HISI_OK) {
        if ((int)wifi_status.status == (int)HI_WIFI_EVT_STA_CONNECTED) {
            printf("WiFi连接成功\n");
            return 0;
        }
    }
    
    printf("WiFi连接超时\n");
    return -1;
}

// 网络状态监控函数
int check_network_status(void) {
    hi_wifi_status wifi_status;
    if (hi_wifi_sta_get_connect_info(&wifi_status) == HISI_OK) {
        return ((int)wifi_status.status == (int)HI_WIFI_EVT_STA_CONNECTED) ? 1 : 0;
    }
    return 0;
}

int mqtt_connect(void){
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    int rc = 0;
    int mysock = 0;
    unsigned char buf[256]; // 存储信息
    int buflen = sizeof(buf);
    int msgid = 1;

    int req_qos = 0;
    MQTTString topicString = MQTTString_initializer;
    char payload[256];                                  //此处是发布的信息
    //int payloadlen = 0;
    int len = 0;

    // MQTT服务器配置 - 使用公共EMQX服务器
    char *host = "47.108.220.55";  // 可改为您的MQTT服务器地址
    int port = 1883;

    printf("尝试连接MQTT服务器: %s:%d\n", host, port);

    // 建立TCP连接
    mysock = transport_open(host, port);
    if (mysock < 0) {
        printf("无法连接到MQTT服务器\n");
        return mysock;
    }

    printf("TCP连接成功，发送CONNECT包\n");

    // 配置MQTT连接参数
    data.clientID.cstring = "bathroom_device_01";  // 浴室设备唯一ID
    data.keepAliveInterval = 20;
    data.cleansession = 1;
    data.username.cstring = "bathroom_user";       // 可选：用户名
    data.password.cstring = "bathroom_pass";       // 可选：密码

    // 发送CONNECT包
    len = MQTTSerialize_connect(buf, buflen, &data);
    rc = transport_sendPacketBuffer(mysock, buf, len);

    // 等待CONNACK
    if (MQTTPacket_read(buf, buflen, transport_getdata) == CONNACK) {
        unsigned char sessionPresent, connack_rc;
        if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 || connack_rc != 0) {
            printf("MQTT连接被拒绝，返回码: %d\n", connack_rc);
            goto exit;
        }
        printf("MQTT连接成功\n");
    } else {
        printf("未收到CONNACK响应\n");
        goto exit;
    }

    // 订阅浴室控制主题
    topicString.cstring = "/sh/envir/status/bathroom";
    len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);
    rc = transport_sendPacketBuffer(mysock, buf, len);

    if (MQTTPacket_read(buf, buflen, transport_getdata) == SUBACK) {
        unsigned short submsgid;
        int subcount;
        int granted_qos;
        rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
        if (granted_qos != 0) {
            printf("订阅QoS不匹配: %d\n", granted_qos);
            goto exit;
        }
        printf("成功订阅主题: %s\n", topicString.cstring);
    } else {
        printf("订阅失败\n");
        goto exit;
    }

    // 主循环：处理消息和发布状态
    while (!toStop) {
        // 检查是否收到控制消息
        if (MQTTPacket_read(buf, buflen, transport_getdata) == PUBLISH) {
            unsigned char dup;
            int qos;
            unsigned char retained;
            unsigned short msgid;
            int payloadlen_in;
            unsigned char *payload_in;
            MQTTString receivedTopic;
            
            rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                                       &payload_in, &payloadlen_in, buf, buflen);
            printf("收到控制命令: %.*s\n", payloadlen_in, payload_in);

            // 解析JSON命令并控制浴室设备
            if (json_parse_and_control(payload_in, (unsigned int)payloadlen_in) == 0) {
                printf("命令执行成功\n");
            } else {
                printf("JSON解析或命令执行失败\n");
            }
        }

        // 发布浴室状态数据
        topicString.cstring = "/sh/envir/status/resp/bathroom";
        // 构造状态数据（JSON格式）

        int bathroom_state = Query_Room_Status();
        //TODO：增加消息发布函数，注意修改设备ID
        publish_param(mysock, buf, buflen, payload, sizeof(payload), topicString, 7, bathroom_state, "infraredRay", "bathroom_state");

        usleep(5000000);  // 5秒发布一次状态
    }

    printf("断开MQTT连接\n");
    len = MQTTSerialize_disconnect(buf, buflen);
    rc = transport_sendPacketBuffer(mysock, buf, len);

exit:
    transport_close(mysock);
    return rc;
}

// 硬件自行联网主函数（假设WiFi已通过wifi_utils.c连接）
void hardware_auto_networking(void) {
    printf("启动MQTT联网功能（WiFi连接已由wifi_utils.c完成）\n");
    
    // 检查WiFi状态
    //if (check_network_status()) {
        printf("WiFi连接正常，开始MQTT连接\n");
        // 直接进行MQTT连接
        if (mqtt_connect() == 0) {
            printf("MQTT连接成功，设备已就绪\n");
        } else {
            printf("MQTT连接失败\n");
        }
    //} else {
    //    printf("WiFi连接状态异常，请检查wifi_utils.c的连接\n");
    //}
}