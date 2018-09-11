/*
*********************************************************************************************************
*                                     MICIRUM BOARD SUPPORT PACKAGE
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
*                                         BOARD SUPPORT PACKAGE
*
*                                            Renesas RX111
*                                                on the
*                                           YWIRELESS-RX111
*
* Filename      : bsp.c
* Version       : V1.00
* Programmer(s) : DC
*                 JJL
*                 SB
*                 JPC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>
#include  <cpu.h>
#include  <bsp.h>
#include  <bsp_sys.h>
#include  <bsp_led.h>
#include  <bsp_switch.h>
#include  "bsp_spi.h"
#include  "iodefine.h"


/*
*********************************************************************************************************
*                                         BSP INITIALIZATION
*
* Description : This function should be called by the application code before using any functions in this
*               module.
*********************************************************************************************************
*/

void  BSP_Init (void)
{
    BSP_SysInit();                                              /* Initialize system clocks                             */

    OS_BSP_TickInit();                                          /* Init kernel tick                                     */

    BSP_LED_Init();                                             /* Initialize LEDs                                      */

    BSP_SwitchInit();                                           /* Initialize switches                                  */
}
