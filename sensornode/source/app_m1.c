/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2016; Micrium, Inc.; Weston, FL
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
*                                            RENESAS RX111
*                                               on the
*                                    YWireless-RX111 Wireless Demo Kit
*
* Filename      : app_shg.c
* Version       : V1.00
* Programmer(s) : MTM
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES                                             *
*********************************************************************************************************
*/

#include  <os.h>
#include  <lib_mem.h>
#include  <bsp_led.h>
#include  <bsp_switch.h>
#include  <bsp_sys.h>
#include  <bsp_spi.h>


#include  "app_m1.h"
#include  "app_ams.h"
#include  "app_wifi.h"


/*
*********************************************************************************************************
*                                                DEFINES                                                *
*********************************************************************************************************
*/

#define  SHG_TASK_PUBLISH_PRIO                  11u
#define  SHG_TASK_PUBLISH_STK_SIZE              1024u


#define  SHG_FIRMWARE_ID_STR                    "YWireless RX111 Smart Home Garage"
#define  SHG_FIRMWARE_VERSION_MAJOR             0
#define  SHG_FIRMWARE_VERSION_MINOR             1
#define  SHG_FIRMWARE_REVISION_NBR              0               /* TODO: Use variables and generate w/ release script   */

#define  SHG_TASK_SUBSCRIBE_MSG_POLL_PRIO       11u
#define  SHG_TASK_SUBSCRIBE_MSG_POLL_STK_SIZE   512u

#define  SHG_MQTT_CLIENT_ID_PREFIX              "micrium-"
#define  SHG_JSON_BUF_SIZE                      1024u
#define  SHG_MQTT_RX_BUF_SIZE                   128u

#define  SHG_SWITCH_NBR_GARAGE                  2u
#define  SHG_SWITCH_NBR_MOTION_DETECT           3u
#define  SHG_SWITCH_NBR_DIAGNOSTICS             4u

#define  SHG_NTP_TIMEOUT_S                      15u
#define  SHG_RECONN_TIMEOUT_S                   15u

#define  SHG_LED_INDICATOR                      BSP_LED_ORANGE


/*
*********************************************************************************************************
*                                               CONSTANTS                                               *
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               DATA TYPES                                              *
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES                                          *
*********************************************************************************************************
*/

static  void         SHG_TaskSubscribeMsgPoll (void  *p_arg);

static  void         SHG_SetConnected (CPU_BOOLEAN val);

static  CPU_BOOLEAN  SHG_OnMotionDetected (CPU_INT08U  switch_nbr, SWITCH_EVENT  type);

static  CPU_BOOLEAN  SHG_OnGarageDoorSwitchEvent (CPU_INT08U  switch_nbr, SWITCH_EVENT  type);

static  CPU_BOOLEAN  SHG_OnDiagnosticsSwitchEvent (CPU_INT08U  switch_nbr, SWITCH_EVENT  type);


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES                                           *
*********************************************************************************************************
*/
static  CPU_BOOLEAN   mqtt_connected = DEF_FALSE;

static  OS_FLAG_GRP   SHG_Events;
static  OS_MUTEX      SHG_SensorMutex;
static  OS_MUTEX      SHG_ConnectedMutex;

static  OS_TCB        SHG_TaskPublishTCB;
static  CPU_STK       SHG_TaskPublishStk[SHG_TASK_PUBLISH_STK_SIZE];

static  OS_TCB        SHG_TaskSubscribeMsgPollTCB;
static  CPU_STK       SHG_TaskSubscribeMsgPollStk[SHG_TASK_SUBSCRIBE_MSG_POLL_STK_SIZE];

static  CPU_BOOLEAN   SHG_GarageIsOpen      = DEF_NO;

static  CPU_CHAR      SHG_JSONBuf[SHG_JSON_BUF_SIZE];
static  CPU_CHAR      SHG_SensorBuf[AMS_SENSOR_BUF_LEN];


/*
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS                                            *
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LOCAL FUNCTIONS                                            *
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          App_SHG_Init()
*
* Description : Initialize the Smart Home Garage task
*
* Arguments   : SHG_Cfg : Pointer to the confguration file
*
* Returns     : DEF_OK on success, DEF_FAIL on failure.
*
* Notes       : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  App_SHG_Init (SHG_CFG *SHG_Cfg)
{
    OS_ERR       err_os;
    CPU_BOOLEAN  ret_val = DEF_FAIL;

    do {
        OSMutexCreate(&SHG_SensorMutex,                         /* Create the SHG sensor mutex                          */
                      "SHG Sensor Mutex",
                      &err_os);
        if(err_os != OS_ERR_NONE) {
            break;
        }

        OSMutexCreate(&SHG_ConnectedMutex,                      /* Create the SHG connected mutex                       */
                      "SHG Connected Mutex",
                      &err_os);
        if(err_os != OS_ERR_NONE) {
            break;
        }

        OSFlagCreate(&SHG_Events,                               /* Create the SHG Events flag group                     */
                     "Garage Door Buttons Events",
                      0,
                     &err_os);
        if(err_os != OS_ERR_NONE) {
            break;
        }

        ret_val = DEF_OK;                                       /* Everything initialized ok, return DEF_OK            */

    } while(0);

    return ret_val;
}


/*
*********************************************************************************************************
*                                          App_SHG_PublishMsg()
*
* Description : Post a sensor message to MediumOne
*
* Arguments   : sensor_msg :    The sensor message string to post
*
* Returns     : none.
*
* Notes       : In an effort to save space, we can only send one sensor message at a time. This is not
*               a huge deal since most sensor messages are sent infrequently. Once the sensor message
*               has been copied into an MQTT buffer this one becomes available.
*
*********************************************************************************************************
*/

void App_SHG_PublishMsg(CPU_CHAR* sensor_msg)
{
    OS_ERR err_os;


    OSMutexPend(&SHG_SensorMutex,                               /* Wait for the sensor message buffer to be free        */
                 0,
                 OS_OPT_NONE,
                 0,
                &err_os);

    Str_Copy((CPU_CHAR*) &SHG_SensorBuf,                        /* Copy the sensor message into the local buffer        */
                          sensor_msg);

    OSFlagPost(&SHG_Events,                                     /* Signal for the sensor message to be published        */
                SHG_SENSOR_PUBLISH,
                OS_OPT_POST_FLAG_SET,
               &err_os);
}


/*
*********************************************************************************************************
*                                          App_SHG_IsConnected()
*
* Description : Returns the current state of the connection with the MQTT broker
*
* Arguments   : none.
*
* Returns     : DEF_TRUE if connected, DEF_FALSE if not connected.
*
* Notes       : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  App_SHG_IsConnected (void)
{
    OS_ERR err_os;
    CPU_BOOLEAN ret_val;


    OSMutexPend(&SHG_SensorMutex,                               /* Wait for the connected variable to be available      */
                 0,
                 OS_OPT_NONE,
                 0,
                &err_os);

    ret_val = mqtt_connected;


    OSMutexPost(&SHG_ConnectedMutex,                            /* Free connected variable                              */
                 OS_OPT_NONE,
                &err_os);

    return ret_val;
}


/*
*********************************************************************************************************
*                                        SHG_OnGarageDoorSwitchEvent()
*
* Description :
*
* Arguments   :
*
* Returns     : none.
*
* Notes       : none.
*
*********************************************************************************************************
*/

static  CPU_BOOLEAN  SHG_OnGarageDoorSwitchEvent (CPU_INT08U    switch_nbr,
                                                  SWITCH_EVENT  type)
{
    OS_ERR       err_os;
    CPU_BOOLEAN  is_oneshot;


    (void)type; // not yet implemented
    (void)switch_nbr;

    is_oneshot = DEF_NO;

    OSFlagPost(&SHG_Events, SHG_EVENT_GARAGE_SWITCH_PRESSED, OS_OPT_NONE, &err_os);

    return is_oneshot;
}


/*
*********************************************************************************************************
*                                          SHG_OnMotionDetected()
*
* Description :
*
* Arguments   :
*
* Returns     : none.
*
* Notes       : none.
*
*********************************************************************************************************
*/

static  CPU_BOOLEAN  SHG_OnMotionDetected (CPU_INT08U    switch_nbr,
                                           SWITCH_EVENT  type)
{
    OS_ERR       err_os;
    CPU_BOOLEAN  is_oneshot;


    (void)type; // not yet implemented
    (void)switch_nbr;

    is_oneshot = DEF_NO;

    OSFlagPost(&SHG_Events, SHG_EVENT_MOTION_DETECTED, OS_OPT_NONE, &err_os);

    return is_oneshot;
}


/*
*********************************************************************************************************
*                                    SHG_OnDiagnosticsSwitchEvent()
*
* Description :
*
* Arguments   :
*
* Returns     : none.
*
* Notes       : none.
*
*********************************************************************************************************
*/

static  CPU_BOOLEAN  SHG_OnDiagnosticsSwitchEvent (CPU_INT08U    switch_nbr,
                                                   SWITCH_EVENT  type)
{
    OS_ERR       err_os;
    CPU_BOOLEAN  is_oneshot;


    (void)type; // not yet implemented
    (void)switch_nbr;

    is_oneshot = DEF_NO;

    OSFlagPost(&SHG_Events,
                SHG_EVENT_DIAGNOSTICS_SWITCH_PRESSED,
                OS_OPT_NONE,
               &err_os);

    return is_oneshot;
}



/***              peripheral port finctions              ***/
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


static int sci1SPIInit(portConfig * config) {
    SCI_BSP_SPI_init();
}


static int sci1SPIReadWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int len) {
    SCI_BSP_SPI_SetCfg(config->frequency, 0, 0, NULL, 0);
    SCI_BSP_SPI_Lock();
    SCI_BSP_SPI_ChipSelEn();
    int ret = SCI_BSP_SPI_WrRd(txBuf, rxBuf, len);
    SCI_BSP_SPI_ChipSelDis();
    SCI_BSP_SPI_Unlock();
    return ret;
}


static int sci1SPIGpioSet(portConfig * config, int pin, char high) {
    if (high & 0x1)
        SCI_BSP_SPI_Data();
    else
        SCI_BSP_SPI_Command();
    return 0;
}


static int sci1I2CInit(portConfig * config) {
    SCI_BSP_I2C_init();
}


static int sci1I2CReadAndWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int readLength, int writeLength) {
    OS_ERR err;
    int ret = 0;
#if 0
    if (DEMO_Type == DEMO_ENV_SENSOR) {
        CPU_TS ts;
        OSSemPend(&sci1Lock, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
    }
#endif
    int address;

    SCI_BSP_I2C_SetCfg(config->frequency, 0, 1, 0, 0);
    if (writeLength >= 1) { // required, at minimum we need device address
        address = *txBuf++;
        writeLength--;
        if (writeLength && readLength) // we are writing some data (maybe register address) and reading some data
            ret = SCI_BSP_I2C_WrAndRd(address, txBuf, writeLength, rxBuf, readLength);
        else if (readLength) // just reading
            ret = SCI_BSP_I2C_WrRd(address, NULL, rxBuf, readLength);
        else // just writing
            ret = SCI_BSP_I2C_WrRd(address, txBuf, NULL, writeLength);
    } else
        ret = -1;
#if 0
    if(DEMO_Type == DEMO_ENV_SENSOR) {
        OSSemPost(&sci1Lock, OS_OPT_POST_1, &err);
    }
#endif
    return ret;
}


static int riicI2CInit(portConfig * config) {
    RIIC_BSP_I2C_init();
}


static int riicI2CReadAndWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int readLength, int writeLength) {
    int address;

    RIIC_BSP_I2C_SetCfg(config->frequency, 0, 1, 0, 0);
    if (writeLength >= 1) { // required, at minimum we need device address
        address = *txBuf++;
        writeLength--;
        if (writeLength && readLength) // we are writing some data (maybe register address) and reading some data
            return RIIC_BSP_I2C_WrAndRd(address, txBuf, writeLength, rxBuf, readLength);
        else if (readLength) // just reading
            return RIIC_BSP_I2C_WrRd(address, NULL, rxBuf, readLength);
        else // just writing
            return RIIC_BSP_I2C_WrRd(address, txBuf, NULL, writeLength);
    }
}


static int sci1UARTInit(portConfig * config) {
    SCI_BSP_UART_init(0);
    if(config->frequency)
        SCI_BSP_UART_SetCfg(config->frequency, 0, 0, 0, 0);
}


static int sci1UARTReadWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int len) {
    return SCI_BSP_UART_WrRd(txBuf, rxBuf, len);
}


static int sci1GPIO(portConfig * config, int pin, char val) {
    OS_ERR  err_os;
    SCI_BSP_UART_GPIO(val);
    OSTimeDlyHMSM(0, 0, 0, 150, OS_OPT_NONE, &err_os);   // for sonar. sonar needs 49ms of calibration time and 100ms of waiting time
}

const portHandle portMap[][PMOD_PORT_PROTOCOLS_SUPPORTED] = {
  {
    {
      .init = &sci1SPIInit,
      .readWrite = &sci1SPIReadWrite,
      .readAndWrite = NULL,
      .gpioSet = &sci1SPIGpioSet,
    },
    {
      .init = &sci1I2CInit,
      .readWrite = NULL,
      .readAndWrite = &sci1I2CReadAndWrite,
      .gpioSet = NULL,
    },
    {
      .init = &sci1UARTInit,
      .readWrite = &sci1UARTReadWrite,
      .readAndWrite = NULL,
      .gpioSet = &sci1GPIO,
    }
  },
  {
    {
      .init = &rspiSPIInit,
      .readWrite = &rspiSPIReadWrite,
      .readAndWrite = NULL,
      .gpioSet = &rspiSPIGpioSet,
    },
    {
      .init = NULL,
      .readWrite = NULL,
      .readAndWrite = NULL,
      .gpioSet = NULL,
    },
    {
      .init = NULL,
      .readWrite = NULL,
      .readAndWrite = NULL,
      .gpioSet = NULL,
    }
  },
  {
    {
      .init = NULL,
      .readWrite = NULL,
      .readAndWrite = NULL,
      .gpioSet = NULL,
    },
    {
      .init = &riicI2CInit,
      .readWrite = NULL,
      .readAndWrite = &riicI2CReadAndWrite,
      .gpioSet = NULL,
    },
    {
      .init = NULL,
      .readWrite = NULL,
      .readAndWrite = NULL,
      .gpioSet = NULL,
    }
  }
};

