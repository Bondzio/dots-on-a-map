/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           uC/DHCP-c
*                                       Application Code
*
* Filename      : app_dhcp-c.h
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
*/

#ifndef  APP_DHCP_C_H
#define  APP_DHCP_C_H

/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/


#include <stdarg.h>
#include <stdio.h>

#include <cpu.h>

#include <lib_def.h>
#include <app_cfg.h>

/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/

#ifdef   APP_DHCP_C_MODULE
#define  APP_DHCP_C_MODULE_EXT
#else
#define  APP_DHCP_C_MODULE_EXT  extern
#endif


/*
*********************************************************************************************************
*                                        DEFAULT CONFIGURATION
*********************************************************************************************************
*/

#ifndef  APP_CFG_DHCP_C_EN
#define  APP_CFG_DHCP_C_EN                       DEF_ENABLED
#endif


/*
*********************************************************************************************************
*                                      CONDITIONAL INCLUDE FILES
*********************************************************************************************************
*/

#if (APP_CFG_DHCP_C_EN == DEF_ENABLED)
#include  <Source/dhcp-c.h>
#endif


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

#if (APP_CFG_DHCP_C_EN == DEF_ENABLED)
CPU_BOOLEAN  AppDHCPc_Init (NET_IF_NBR  if_nbr);
#endif


/*
*********************************************************************************************************
*                                          CONFIGURATION ERRORS
*********************************************************************************************************
*/


#endif
