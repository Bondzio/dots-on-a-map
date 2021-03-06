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
* Filename      : app_ams.h
* Version       : V1.00
* Programmer(s) : MTM
*********************************************************************************************************
*/

#ifndef  APP_AMS_H_
#define  APP_AMS_H_

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

#define  AMS_SENSOR_BUF_LEN                     1024u
#define  AMS_VERSION                           "6.0-201611081316"


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


void  App_AMS (SHG_CFG *SHG_Cfg);

#endif
