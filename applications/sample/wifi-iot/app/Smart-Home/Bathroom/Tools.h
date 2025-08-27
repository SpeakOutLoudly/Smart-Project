#ifndef __TOOLS_H__
#define __TOOLS_H__

void hardware_control(char *target, char *value);
void bathroom_Fan_Control(char *value);
void bathroom_Light_Control(char *value);
int json_parse_and_control(unsigned char *payload_in, unsigned int payload_len);
char* json_parse_command(unsigned char *payload_in, unsigned int payload_len);

#endif