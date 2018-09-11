/*
*********************************************************************************************************
*                                         BOARD SUPPORT PACKAGE
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
* Filename      : bsp_led.c
* Version       : V1.01
* Programmer(s) : SB
*                 JPC
*********************************************************************************************************
*/


#include  <cpu.h>
#include  <lib_def.h>
#include  "bsp.h"
#include  "bsp_led.h"
#include  "iodefine.h"
#include  "bsp_cfg.h"


/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

                                                                        /* Port Output Data Registers.                  */
#ifdef RX231
#define  BSP_LED_1              PORT4.PODR.BIT.B7  // green
#define  BSP_LED_1_PDR          PORT4.PDR.BIT.B7  // green
#define  BSP_LED_2              PORT4.PODR.BIT.B6  // orange
#define  BSP_LED_2_PDR          PORT4.PDR.BIT.B6  // orange
#define  BSP_LED_3              PORT4.PODR.BIT.B5  // red
#define  BSP_LED_3_PDR          PORT4.PDR.BIT.B5  // red
#define  BSP_LED_4              PORT4.PODR.BIT.B5  // hb
#define  BSP_LED_4_PDR          PORT4.PDR.BIT.B5  // hb
#define LED_ON 1
#define LED_OFF 0
#else
#define  BSP_LED_1              PORT0.PODR.BIT.B5
#define  BSP_LED_1_PDR          PORT0.PDR.BIT.B5
#define  BSP_LED_2              PORT0.PODR.BIT.B3
#define  BSP_LED_2_PDR          PORT0.PDR.BIT.B3
#define  BSP_LED_3              PORTB.PODR.BIT.B7
#define  BSP_LED_3_PDR          PORTB.PDR.BIT.B7
#define  BSP_LED_4              PORTB.PODR.BIT.B6
#define  BSP_LED_4_PDR          PORTB.PDR.BIT.B6
#define LED_ON 1
#define LED_OFF 0
#endif


/*
*********************************************************************************************************
*                                       LED INITIALIZATION
*
* Description: This function is used to initialize the LEDs on the board.
*
* Arguments  : none
*********************************************************************************************************
*/

void  BSP_LED_Init (void)
{
    BSP_LED_Off(BSP_LED_ALL);                                   /* Turn OFF all LEDs.                                   */

                                                                /* Set pins to output.                                  */
    BSP_LED_1_PDR = DEF_SET;                                 /* LED 1                                                */
    BSP_LED_2_PDR = DEF_SET;                                 /* LED 2                                                */
    BSP_LED_3_PDR = DEF_SET;                                 /* LED 3                                                */
    BSP_LED_4_PDR = DEF_SET;                                 /* Heartbeat LED                                        */
}


/*
*********************************************************************************************************
*                                               LED ON
*
* Description: This function is used to control any or all the LEDs on the board.
*
* Arguments  : led    is the number of the LED to control
*                     0    indicates that you want ALL the LEDs to be ON
*                     1    turns ON LED0 on the board
*                     .
*                     .
*                     4    turns ON LED3 on the board
*********************************************************************************************************
*/

void  BSP_LED_On (CPU_INT08S  led)
{
    switch (led) {
        case BSP_LED_ALL:
            BSP_LED_1 = LED_ON;
            BSP_LED_2 = LED_ON;
            BSP_LED_3 = LED_ON;
            break;

        case BSP_LED_GREEN:
            BSP_LED_1 = LED_ON;
            break;

        case BSP_LED_ORANGE:
            BSP_LED_2 = LED_ON;
            break;

        case BSP_LED_RED:
            BSP_LED_3 = LED_ON;
            break;

        case BSP_LED_HEARTBEAT:
            BSP_LED_4 = LED_ON;
            break;

        default:
            break;
    }
}


/*
*********************************************************************************************************
*                                               LED OFF
*
* Description: This function is used to control any or all the LEDs on the board.
*
* Arguments  : led    is the number of the LED to turn OFF
*                     0    indicates that you want ALL the LEDs to be OFF
*                     1    turns OFF LED0 on the board
*                     .
*                     .
*                     4    turns OFF LED3 on the board
*********************************************************************************************************
*/

void  BSP_LED_Off (CPU_INT08S  led)
{
    switch (led) {
        case BSP_LED_ALL:
            BSP_LED_1 = LED_OFF;
            BSP_LED_2 = LED_OFF;
            BSP_LED_3 = LED_OFF;
            break;

        case BSP_LED_GREEN:
            BSP_LED_1 = LED_OFF;
            break;

        case BSP_LED_ORANGE:
            BSP_LED_2 = LED_OFF;
            break;

        case BSP_LED_RED:
            BSP_LED_3 = LED_OFF;
            break;

        case BSP_LED_HEARTBEAT:
            BSP_LED_4 = LED_OFF;
            break;

        default:
            break;
    }
}


/*
*********************************************************************************************************
*                                             LED TOGGLE
*
* Description: This function is used to toggle the state of any or all the LEDs on the board.
*
* Arguments  : led    is the number of the LED to toggle
*                     0    indicates that you want ALL the LEDs to toggle
*                     1    Toggle LED0 on the board
*                     .
*                     .
*                     4    Toggle LED3 on the board
*********************************************************************************************************
*/

void  BSP_LED_Toggle (CPU_INT08S  led)
{
    switch (led) {
        case BSP_LED_ALL:
            BSP_LED_1 = ~(CPU_INT32U)BSP_LED_1;
            BSP_LED_2 = ~(CPU_INT32U)BSP_LED_2;
            BSP_LED_3 = ~(CPU_INT32U)BSP_LED_3;
            break;

        case BSP_LED_GREEN:
            BSP_LED_1 = ~(CPU_INT32U)BSP_LED_1;
            break;

        case BSP_LED_ORANGE:
            BSP_LED_2 = ~(CPU_INT32U)BSP_LED_2;
            break;

        case BSP_LED_RED:
            BSP_LED_3 = ~(CPU_INT32U)BSP_LED_3;
            break;

        case BSP_LED_HEARTBEAT:
            BSP_LED_4 = ~(CPU_INT32U)BSP_LED_4;
            break;

        default:
            break;
    }
}
