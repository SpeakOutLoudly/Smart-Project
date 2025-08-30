#ifndef __COLORFUL_LIGHT_H__
#define __COLORFUL_LIGHT_H__

void hardware_control(char *target, char *value);

int colorful_light_get_light_status(void);
void Status_Query(void);

void ColorfulLightDemo(void);

#endif