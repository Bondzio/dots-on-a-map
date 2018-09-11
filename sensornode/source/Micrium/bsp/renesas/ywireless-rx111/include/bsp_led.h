/*
*********************************************************************************************************
*                                           BOARD SUPPORT PACKAGE
*
*                            (c) Copyright 2015; Micrium, Inc.; Weston, FL
*
*               All rights reserved. Protected by international copyright laws.
*
*               BSP is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         BOARD SUPPORT PACKAGE
*
*                                            Renesas RX111
*                                                on the
*                                           YWIRELESS-RX111
*
* Filename      : bsp_led.h
* Version       : V1.00
* Programmer(s) : SB
*                 JPC
*********************************************************************************************************
*/

#ifndef  _BSP_LED_H_
#define  _BSP_LED_H_


/*
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*/

typedef enum {
    BSP_LED_ALL         = -1,
    BSP_LED_GREEN       =  1,
    BSP_LED_ORANGE      =  2,
    BSP_LED_RED         =  3,
    BSP_LED_HEARTBEAT   =  5
} BSP_LED;


/*
*********************************************************************************************************
*                                            PROTOTYPES
*********************************************************************************************************
*/

void    BSP_LED_Init    (void);
void    BSP_LED_On      (CPU_INT08S  led);
void    BSP_LED_Off     (CPU_INT08S  led);
void    BSP_LED_Toggle  (CPU_INT08S  led);


#endif
