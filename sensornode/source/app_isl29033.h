#ifndef APP_ISL29501_H_
#define APP_ISL29501_H_

#include <m1_bsp.h>

#define ISL29033_I2C_ADDR 0x44


void isl29033_init(port * light_sensor);
int isl29033_read_adc(int * adc);

#endif
