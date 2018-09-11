/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                          (c) Copyright 2003-2015; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       uC/OS-III Application Code
*
* Filename      : app_cfg.h
* Version       : V1.00
* Programmer(s) : JPC
*********************************************************************************************************
*/

#ifndef  APP_CFG_H_
#define  APP_CFG_H_


/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*********************************************************************************************************
*/

#define  APP_CFG_TASK_START_PRIO                               10u
#define  USBD_MSC_OS_CFG_TASK_PRIO                              6u
#define  USBD_OS_CFG_CORE_TASK_PRIO                             7u
#define  CLK_OS_CFG_TASK_PRIO                                   4u
#define  SMART_HOME_GWY_APP_TASK_PRIO                          10u
#define  APP_CFG_TASK_MQTT_CLIENT_SUB_PRIO                     11u
#define  APP_CFG_TASK_MQTT_CLIENT_PUB_PRIO                     12u


/*
*********************************************************************************************************
*                                            TASK STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/

#define  APP_CFG_TASK_START_STK_SIZE                          512u
#define  USBD_MSC_OS_CFG_TASK_STK_SIZE                        512u
#define  USBD_OS_CFG_CORE_TASK_STK_SIZE                       512u
#define  CLK_OS_CFG_TASK_STK_SIZE                             128u
#define  APP_CFG_TASK_MQTT_CLIENT_PUB_STK_SIZE                512u
#define  APP_CFG_TASK_MQTT_CLIENT_SUB_STK_SIZE                512u
#define  SMART_HOME_GWY_APP_TASK_STK_SIZE                     256u


/*
*********************************************************************************************************
*                                       TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/
#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                               0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                              1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                               2
#endif


#define  APP_CFG_TRACE_LEVEL                        TRACE_LEVEL_OFF
#include <stdio.h>
#define  APP_CFG_TRACE                              printf

#define  APP_TRACE_INFO(x)               ((APP_CFG_TRACE_LEVEL >= TRACE_LEVEL_INFO)  ? (void)(APP_CFG_TRACE x) : (void)0)
#define  APP_TRACE_DBG(x)                ((APP_CFG_TRACE_LEVEL >= TRACE_LEVEL_DBG)   ? (void)(APP_CFG_TRACE x) : (void)0)


#endif
