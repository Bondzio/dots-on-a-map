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
* Filename      : app_shg.h
* Version       : V1.00
* Programmer(s) : MTM
*********************************************************************************************************
*/

#ifndef  APP_SHG_H_
#define  APP_SHG_H_

/*
*********************************************************************************************************
*                                             INCLUDE FILES                                             *
*********************************************************************************************************
*/

#include  "app_wifi_util.h"


/*
*********************************************************************************************************
*                                                DEFINES                                                *
*********************************************************************************************************
*/

#define  SHG_EVENT_GARAGE_SWITCH_PRESSED        DEF_BIT_00
#define  SHG_EVENT_GARAGE_DOOR_CLOSE            DEF_BIT_01
#define  SHG_EVENT_MOTION_DETECTED              DEF_BIT_02
#define  SHG_EVENT_DIAGNOSTICS_SWITCH_PRESSED   DEF_BIT_03
#define  SHG_EVENT_SUBSCRIBE_MSG_RECV           DEF_BIT_04
#define  SHG_EVENT_SUBSCRIBE_MSG_RECV_ERR       DEF_BIT_05
#define  SHG_SENSOR_PUBLISH                     DEF_BIT_06

#define  SHG_FIRMWARE_VERSION_MAJOR             1
#define  SHG_FIRMWARE_VERSION_MINOR             0
#define  SHG_FIRMWARE_REVISION_NBR              0

#define  XSTR(s) #s

#define  JOIN(a, b, c) XSTR(a)"."XSTR(b)"."XSTR(c)

#define  VERSION "\""JOIN(SHG_FIRMWARE_VERSION_MAJOR, SHG_FIRMWARE_VERSION_MINOR, SHG_FIRMWARE_REVISION_NBR)"\""
#define PMOD_PORT_PROTOCOLS_SUPPORTED 3
#define PORT_PMOD1 0
#define PORT_PMOD3 1
#define PORT_PMOD4 2


/*
*********************************************************************************************************
*                                               CONSTANTS                                               *
*********************************************************************************************************
*/

extern const portHandle portMap[][PMOD_PORT_PROTOCOLS_SUPPORTED];
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

/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES                                           *
*********************************************************************************************************
*/



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


CPU_BOOLEAN  App_SHG_Init           (SHG_CFG *SHG_Cfg);
CPU_BOOLEAN  App_SHG_IsConnected    (void);
void         App_SHG_PublishMsg     (CPU_CHAR *sensor_msg);

#endif
