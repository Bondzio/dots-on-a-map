/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2015; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                            RENESAS RX231
*                                               on the
*                                    YWireless-RX231 Wireless Demo Kit
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : JPC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <stdlib.h>

#include  <os.h>
#include <lib_mem.h>

#include <bsp_spi.h>
#include  "app_wifi_util.h"
#include  "app_util.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

OS_MEM   pktBufferMem;
int PKT_BUFFER_COUNT;
int PKT_BUFFER_SIZE;
CPU_INT08U  * pktBuffers;
CPU_CHAR sensor_buf[1024];
portHandle rspi0;
static int rspiSPIInit(portConfig * config) {
    RSPI_BSP_SPI_Init();
}


static int rspiSPIReadWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int len) {
    BSP_SPI_BUS_CFG cfg = {
        .SClkFreqMax = (CPU_INT32U)config->frequency,
        .CPOL = (CPU_INT08U)config->modeConfig.spi.polarity,
        .CPHA = (CPU_INT08U)config->modeConfig.spi.phase,
        .LSBFirst = DEF_NO,
        .BitsPerFrame = (CPU_INT08U)8,
    };

    // TODO: support CS toggle between bytes as config option
    BSP_SPI_Lock(0);
    BSP_SPI_Cfg(0, &cfg);
    BSP_SPI_ChipSel(0, 3, DEF_ON);
    // TODO: make this return a status so we can indicate errors (timeouts in RSPI while loops?)
    BSP_SPI_Xfer(0, txBuf, rxBuf, len);
    BSP_SPI_ChipSel(0, 3, DEF_OFF);
    BSP_SPI_Unlock(0);
    return len;
}

static int rspiSPIGpioSet(portConfig * config, int pin, char high){
    if(high & 0x1)
        RSPI0_BSP_SPI_Data();
    else
        RSPI0_BSP_SPI_Command();

    return 0;
}

portHandle rspi0;
/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void reportBadPeripheralDriverMessage(char * errorMessage, char * driverMessage);


/*
*********************************************************************************************************
*                                                main()
*
* Description : Gets device configurations then starts up the smart home garage door opener task and
*               provisions WiFi.
*
* Arguments   : Not used
*********************************************************************************************************
*/


int sendDataToM1(char * tagName, char * tagData, int tagDataLength) {
    Str_Cat_N(&sensor_buf[0], "\"", sizeof(sensor_buf) - Str_Len(sensor_buf) - 1);
    Str_Cat_N(&sensor_buf[0], (char *) tagName, Str_Len(tagName));
    Str_Cat_N(&sensor_buf[0], "\": ", sizeof(sensor_buf) - Str_Len(sensor_buf) - 1);
    //base64_encode(tagData, tagDataLength, &sensor_buf[Str_Len(sensor_buf)], sizeof(sensor_buf)-Str_Len(sensor_buf)-1, &tagDataLength);
    Str_Cat_N(&sensor_buf[0], (char *) tagData, tagDataLength);
    Str_Cat_N(&sensor_buf[0], ",", sizeof(sensor_buf) - Str_Len(sensor_buf) - 1);
    return 0;
}


float q_sqrt(float x) {
    int i = *(int*)&x;
    i = 0x1fbd1df5 + (i >> 1); //Thank you based Carmack
    return *(float*)&i;
}


float mag_calc(float x, float y, float z) {
    return q_sqrt(x * x + y * y + z * z);
}


static void flushMsgsToM1() {
    CPU_INT32U  sensor_buf_len;


    sensor_buf_len = Str_Len(sensor_buf);                       /* Replace the last ',' with the null terminator        */
    sensor_buf[sensor_buf_len+1] = '\0';

    App_SHG_PublishMsg((CPU_CHAR*) &sensor_buf);                /* Pass the buffer to the SHG for publishing            */

    sensor_buf[0] = '\0';                                       /* Clear the first character to empty the string        */
}


port ACL;


int App_PreWifiInit() {
    rspi0.init = &rspiSPIInit;
    rspi0.readWrite = &rspiSPIReadWrite;
    rspi0.readAndWrite = NULL;
    rspi0.gpioSet = &rspiSPIGpioSet;
    
    ACL.handle = &rspi0;
    ACL.config.mode = MODE_SPI;
    ACL.config.frequency = 1000000;
    ACL.config.modeConfig.spi.polarity = 0;
    ACL.config.modeConfig.spi.phase = 0;

    acl2PreWifiInit(&ACL);
}

int  App_Vibration (SHG_CFG * SHG_Cfg)
{

    OS_ERR         err_os;
    /*  declaire variables  */
    int ret, aclSensorStatus = 0;

    if (SHG_Cfg->LoopTimeSeconds == -1) {
        SHG_Cfg->LoopTimeSeconds = 10;
        SHG_Cfg->LoopTimeMilliSeconds = 0;
    }

    PKT_BUFFER_COUNT = 20;
    PKT_BUFFER_SIZE = 128;
    pktBuffers = malloc(PKT_BUFFER_COUNT * PKT_BUFFER_SIZE);
    OSMemCreate((OS_MEM    *)&pktBufferMem,
                (CPU_CHAR  *)"pktBuffers",
                (void      *)pktBuffers,
                (OS_MEM_QTY ) PKT_BUFFER_COUNT,
                (OS_MEM_SIZE) PKT_BUFFER_SIZE,
                (OS_ERR    *)&err_os);
    ASSERT(err_os == OS_ERR_NONE);
    decoding_table_init();
    generate_crc_table();

    char aclbuf[4] = {0};
    aclbuf[0] = 0x80 | 0x00;
    ACL.handle->readWrite(&ACL.config, aclbuf, &aclbuf[2], 2);
    aclbuf[0] = 0x80 | 0x24;
    ACL.handle->readWrite(&ACL.config, aclbuf, &aclbuf[2], 2);
    aclbuf[0] = 0x80 | 0x28;
    ACL.handle->readWrite(&ACL.config, aclbuf, &aclbuf[2], 2);

    initACL2(&ACL, &SHG_Cfg); // TODO: handle init error here

    if (!SHG_Cfg->LoopTimeMilliSeconds && !SHG_Cfg->LoopTimeSeconds) {
        SHG_Cfg->LoopTimeSeconds = 10;
        SHG_Cfg->LoopTimeMilliSeconds = 0;
    }

    OS_TICK prev = OSTimeGet(&err_os);
    OS_TICK prev_min = prev;
    OS_TICK curr;
    char ff = 0;
    char act = 0;
    char st = 0;
    char first = 1;
    uint8_t first_flag = 0;
    uint16_t sample_cnt = 0;
    uint8_t x_zero_cross = 0;
    uint8_t y_zero_cross = 0;
    uint8_t z_zero_cross = 0;
    float x_prev_avg = 0;
    float y_prev_avg = 0;
    float z_prev_avg = 0;
    float x_last = 0;
    float y_last = 0;
    float z_last = 0;
    float mag_max = -1000000;
    float mag_min = 1000000;
    float mag_tot = 0;
    float x_max = -1000000;
    float x_min = 1000000;
    float x_tot = 0;
    float y_max = -1000000;
    float y_min = 1000000;
    float y_tot = 0;
    float z_max = -1000000;
    float z_min = 1000000;
    float z_tot = 0;
    char tx_str[16];

    while (DEF_ON) {
        if (App_SHG_IsConnected()) {
            curr = OSTimeGet(&err_os);
            if ((curr - prev) > (SHG_Cfg->LoopTimeSeconds * 1000 + SHG_Cfg->LoopTimeMilliSeconds)) {
                prev = curr;
                if (first_flag && aclSensorStatus >= 0) {
                    x_prev_avg = x_tot / sample_cnt;
                    y_prev_avg = y_tot / sample_cnt;
                    z_prev_avg = z_tot / sample_cnt;
                    snprintf(tx_str, 16, "%f", x_max);
                    sendDataToM1("x_max", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", x_min);
                    sendDataToM1("x_min", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", x_tot / sample_cnt);
                    sendDataToM1("x_avg", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", y_max);
                    sendDataToM1("y_max", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", y_min);
                    sendDataToM1("y_min", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", y_tot / sample_cnt);
                    sendDataToM1("y_avg", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", z_max);
                    sendDataToM1("z_max", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", z_min);
                    sendDataToM1("z_min", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%f", z_tot / sample_cnt);
                    sendDataToM1("z_avg", tx_str, strlen(tx_str));
                    snprintf(tx_str, 16, "%u", sample_cnt);
                    sendDataToM1("sample_cnt", tx_str, strlen(tx_str));
                    sprintf(tx_str, "%u", x_zero_cross);
                    sendDataToM1("x_zero_cross", tx_str, strlen(tx_str));
                    sprintf(tx_str, "%u", y_zero_cross);
                    sendDataToM1("y_zero_cross", tx_str, strlen(tx_str));
                    sprintf(tx_str, "%u", z_zero_cross);
                    sendDataToM1("z_zero_cross", tx_str, strlen(tx_str));
                    flushMsgsToM1();
                    sample_cnt = 0;
                    x_zero_cross = 0;
                    y_zero_cross = 0;
                    z_zero_cross = 0;
                    mag_max = 0;
                    mag_min = 0x7FFFFFFF;
                    mag_tot = 0;
                    x_max = -1000000;
                    x_min = 1000000;
                    x_tot = 0;
                    y_max = -1000000;
                    y_min = 1000000;
                    y_tot = 0;
                    z_max = -1000000;
                    z_min = 1000000;
                    z_tot = 0;
                    first_flag = 0;
                }
                ret = verifyACL2IDs(&ACL);
                if (ret < 0)
                    reportBadPeripheralDriverMessage("Sensor Disconnected", "ACL");
                else {
                    if (aclSensorStatus < 0) {
                        initACL2(&ACL, &SHG_Cfg); // TODO: handle init error here
                    }
                }
                aclSensorStatus = ret;
            }
            if (aclSensorStatus >= 0) {
                char buf[7];
                // read x, y, and z
                aclSensorStatus = acl2MultiReadReg(&ACL, 0x0e, buf, 6);
                if (aclSensorStatus >= 0) {
                    int16_t xAccel = buf[0] | (buf[1] << 8);
                    int16_t yAccel = buf[2] | (buf[3] << 8);
                    int16_t zAccel = buf[4] | (buf[5] << 8);

                    float dScale = 0.004;

                    float fXAccel = xAccel * dScale;
                    float fYAccel = yAccel * dScale;
                    float fZAccel = zAccel * dScale;
                    float mag_accel = mag_calc(fXAccel, fYAccel, fZAccel);
                    if (mag_accel > mag_max) {
                        mag_max = mag_accel;
                    }
                    if (mag_accel < mag_min) {
                        mag_min = mag_accel;
                    }
                    if (fXAccel > x_max) {
                        x_max = fXAccel;
                    }
                    if (fXAccel < x_min) {
                        x_min = fXAccel;
                    }
                    x_tot += fXAccel;
                    if (fYAccel > y_max) {
                        y_max = fYAccel;
                    }
                    if (fYAccel < y_min) {
                        y_min = fYAccel;
                    }
                    y_tot += fYAccel;
                    if (fZAccel > z_max) {
                        z_max = fZAccel;
                    }
                    if (fZAccel < z_min) {
                        z_min = fZAccel;
                    }
                    z_tot += fZAccel;
                    if ((x_last < x_prev_avg && fXAccel > x_prev_avg) || (x_last > x_prev_avg && fXAccel < x_prev_avg)) {
                        x_zero_cross++;
                    }
                    if ((y_last < y_prev_avg && fYAccel > y_prev_avg) || (y_last > y_prev_avg && fYAccel < y_prev_avg)) {
                        y_zero_cross++;
                    }
                    if ((z_last < z_prev_avg && fZAccel > z_prev_avg) || (z_last > z_prev_avg && fZAccel < z_prev_avg)) {
                        z_zero_cross++;
                    }
                    mag_tot += mag_accel;
                    sample_cnt++;
                    first_flag = 1;
                    x_last = fXAccel;
                    y_last = fYAccel;
                    z_last = fZAccel;
                    // interrupts
                    if (aclSensorStatus >= 0)
                        aclSensorStatus = acl2SingleReadReg(&ACL, 0x0b, &buf[6]);
                    if (!ff)
                        ff = (buf[6] >> 2) & 0x1;
                    if (!act)
                        act = (buf[6] >> 4) & 0x1;
                    if (!st)
                        st = (buf[6] >> 6) & 0x1;
                    //char ledString[65];
                    //sprintf(ledString, "x: %+13fy: %+13fz: %+13fFF:%d ACT:%d ST: %d", fXAccel, fYAccel, fZAccel, ff, act, st);
                    //OledPutString(ledString);
                    if (/*ff || */act || st) { // can't rely on free-fall interrupt because z-axis is busted
                        ff = 0;
                        act = 0;
                        st = 0;
                        //sendDataToM1("acceleration", buf, 6);
                        //sendDataToM1("interrupts", &buf[6], 1);
                        //flushMsgsToM1();
                        //OSFlagPost(&SHG_Events, SHG_SENSOR_PUBLISH, OS_OPT_POST_FLAG_SET, &err_os);
                    }
                }
            }
        }
        OSTimeDlyHMSM(0, 0, 0, 10, OS_OPT_NONE, &err_os);
    }
}


static void reportBadPeripheralDriverMessage(char * errorMessage, char * driverMessage) {
    OS_ERR err;

    char * buf = (char *)malloc(Str_Len(errorMessage) + Str_Len(driverMessage) + 3);
    // TODO: check err
    buf[0] = '\0';
    sprintf(buf, "\"%s: %s\"", errorMessage, driverMessage);
    sendDataToM1("error", buf, Str_Len(buf));
    free(buf);
    flushMsgsToM1();
}
