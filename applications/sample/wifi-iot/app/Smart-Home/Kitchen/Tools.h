#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "MQTTPacket.h"

int json_parse_and_control(unsigned char *payload_in, unsigned int payload_len);
int publish_param(int mysock, 
                 unsigned char *buf, int buflen, 
                 char *payload, size_t payload_size,
                 MQTTString topicString,
                 int deviceId, int value, char *paramName,
                 const char *desc);

#endif