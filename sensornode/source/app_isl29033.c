#include "app_isl29033.h"
#include <stdint.h>

static port * s_light_sensor = NULL;

static unsigned int range_multipliers[] = {
	125, 500, 2000, 8000
};


static int i2cSingleReadReg(port * lightSensor, char addr, unsigned char * data) {
    char i2cbuf[2] = {ISL29033_I2C_ADDR, addr};

    return lightSensor->handle->readAndWrite(&lightSensor->config, i2cbuf, data, 1, 2);
}

static int i2cSingleWriteReg(port * lightSensor, char addr, unsigned char data) {
    char i2cbuf[3] = {ISL29033_I2C_ADDR, addr, data};

    return lightSensor->handle->readAndWrite(&lightSensor->config, i2cbuf, NULL, 0, 3);
}

static uint8_t read8(char register_address) {
    uint8_t data;
    i2cSingleReadReg(s_light_sensor, register_address, &data);
    return data;
}

void isl29033_init(port * light_sensor) {
    s_light_sensor = light_sensor;
    i2cSingleWriteReg(s_light_sensor, 0, 0xA3);
    i2cSingleWriteReg(s_light_sensor, 1, 0x03);
}

int isl29033_read_adc(int * adc) {
    uint8_t cmd2;
	int lsb, msb, range, bitdepth;

    cmd2 = read8(0x01);

	lsb = read8(0x02);

	if (lsb < 0)
        return -1;

	msb = read8(0x03);

	if (msb < 0)
		return -2;

	range = cmd2 & 0x3;
	bitdepth = (4 - ((cmd2 >> 2) & 0x3)) * 4;
	*adc = (((msb << 8) | lsb) * range_multipliers[range]) >> bitdepth;
    return 0;
}
