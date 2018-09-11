#include <m1_bsp.h>
#include "app_wifi_util.h"
#include <os.h>


static int spiSingleReadReg(port * aclSensor, char addr, unsigned char * data) {
    unsigned char buf[3] = {0x0B, addr & 0x3f, 0};
    int ret = aclSensor->handle->readWrite(&aclSensor->config, buf, buf, 3);
    *data = buf[2];
    return ret;
}


static int spiMultiReadReg(port * aclSensor, char addr, unsigned char * data, int n) {
    unsigned char * buf = (char *)malloc(n + 2);
    buf[0] = 0x0B;
    buf[1] = addr & 0x3f;
    int ret = aclSensor->handle->readWrite(&aclSensor->config, buf, buf, n + 2);
    memcpy(data, &buf[2], n);
    free(buf);
    return ret;
}


static int spiSingleWriteReg(port * aclSensor, char addr, unsigned char data) {
    unsigned char buf[3] = {0x0A, addr & 0x3f, data};
    return aclSensor->handle->readWrite(&aclSensor->config, buf, NULL, 3);
}


static int spiMultiWriteReg(port * aclSensor, char addr, unsigned char * data, int n) {
    unsigned char * buf = (char *)malloc(n + 2);
    buf[0] = 0x0A;
    buf[1] = addr & 0x3f;
    memcpy(&buf[2], data, n);
    int ret = aclSensor->handle->readWrite(&aclSensor->config, buf, NULL, n + 2);
    free(buf);
    return ret;
}


int acl2SingleWriteReg(port * aclSensor, char addr, unsigned char data) {
    if (aclSensor->config.mode == MODE_SPI)
        return spiSingleWriteReg(aclSensor, addr, data);
    else
        return -1;
}


int acl2MultiWriteReg(port * aclSensor, char addr, unsigned char * data, int n) {
    if (aclSensor->config.mode == MODE_SPI)
        return spiMultiWriteReg(aclSensor, addr, data, n);
    else
        return -1;
}


int acl2SingleReadReg(port * aclSensor, char addr, unsigned char * data) {
    if (aclSensor->config.mode == MODE_SPI)
        return spiSingleReadReg(aclSensor, addr, data);
    else
        return -1;
}


int acl2MultiReadReg(port * aclSensor, char addr, unsigned char * data, int n) {
    if (aclSensor->config.mode == MODE_SPI)
        return spiMultiReadReg(aclSensor, addr, data, n);
    else
        return -1;
}


int verifyACL2IDs(port * aclSensor) {
    unsigned char buf;
    int aclSensorStatus;

    // read AD devid
    aclSensorStatus = acl2SingleReadReg(aclSensor, 0x00, &buf);
    if (buf != 0xAD)
        return -1;
    // read MEMS devid
    aclSensorStatus = acl2SingleReadReg(aclSensor, 0x01, &buf);
    if (buf != 0x1D)
        return -2;
    // read part id
    aclSensorStatus = acl2SingleReadReg(aclSensor, 0x02, &buf);
    if (buf != 0xF2)
        return -3;
    
    return 0;
}


int initACL2(port * aclSensor, SHG_CFG * SHG_Cfg) {
    unsigned char buf;
    int aclSensorStatus;

    aclSensorStatus = verifyACL2IDs(aclSensor);
    if (aclSensorStatus < 0)
        return -1;

    // set range
    if (aclSensorStatus >= 0)
        aclSensorStatus = acl2SingleWriteReg(aclSensor, 0x2c, 0x93);  // +/-8mg/LSB
    // set measure mode
    if (aclSensorStatus >= 0)
        aclSensorStatus = acl2SingleWriteReg(aclSensor, 0x2d, 0x02);

    return aclSensorStatus;
}

int acl2PreWifiInit(port * aclSensor) {
    OS_ERR err_os;

    // issue soft Reset
    int ret = acl2SingleWriteReg(aclSensor, 0x1F, 0x52);
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    return ret;
}
