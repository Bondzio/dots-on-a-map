/*
*********************************************************************************************************
*                                             EXAMPLE CODE
*********************************************************************************************************
* Licensing terms:
*   This file is provided as an example on how to use Micrium products. It has not necessarily been
*   tested under every possible condition and is only offered as a reference, without any guarantee.
*
*   Please feel free to use any application code labeled as 'EXAMPLE CODE' in your application products.
*   Example code may be used as is, in whole or in part, or may be used as a reference only. This file
*   can be modified as required.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: http://www.micrium.com
*
*   Please help us continue to provide the Embedded community with the finest software available.
*
*   Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                               LED BSP
*
* File : bsp_led.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <bsp_led.h>
#include  <lib_def.h>
#include  "iorx651.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
**                                         GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          LED INITIALIZATION
*
* Description : Board Support package LED initialization.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_LED_Init (void)
{
                                                                /* Select I/O pins as output.                           */
    PORT0.PDR.BIT.B3 = DEF_SET;
    PORT0.PDR.BIT.B5 = DEF_SET;
    PORT7.PDR.BIT.B3 = DEF_SET;
    PORTJ.PDR.BIT.B5 = DEF_SET;

    BSP_LED_Off(BSP_LED_ALL);                                   /* Turn off all LEDs on board.                          */
}


/*
*********************************************************************************************************
*                                            BSP_LED_Off()
*
* Description : Turn OFF any or all the LEDs on the board.
*
* Argument(s) : led    The LED to control.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_LED_Off (BSP_LED  led)
{
    switch (led) {
        case 0u:
             PORT0.PODR.BIT.B3 = DEF_SET;
             PORT0.PODR.BIT.B5 = DEF_SET;
             PORT7.PODR.BIT.B3 = DEF_SET;
             PORTJ.PODR.BIT.B5 = DEF_CLR;
             break;


        case 1u:
             PORT0.PODR.BIT.B3 = DEF_SET;
             break;


        case 2u:
             PORT0.PODR.BIT.B5 = DEF_SET;
             break;


        case 3u:
             PORT7.PODR.BIT.B3 = DEF_SET;
             break;


        case 4u:
             PORTJ.PODR.BIT.B5 = DEF_CLR;
             break;


        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                            BSP_LED_On()
*
* Description : Turn ON any or all the LEDs on the board.
*
* Argument(s) : led     The LED to control.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_LED_On (BSP_LED  led)
{
    switch (led) {
        case 0u:
             PORT0.PODR.BIT.B3 = DEF_CLR;
             PORT0.PODR.BIT.B5 = DEF_CLR;
             PORT7.PODR.BIT.B3 = DEF_CLR;
             PORTJ.PODR.BIT.B5 = DEF_SET;
             break;


        case 1u:
             PORT0.PODR.BIT.B3 = DEF_CLR;
             break;


        case 2u:
             PORT0.PODR.BIT.B5 = DEF_CLR;
             break;


        case 3u:
             PORT7.PODR.BIT.B3 = DEF_CLR;
             break;


        case 4u:
             PORTJ.PODR.BIT.B5 = DEF_SET;
             break;


        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                          BSP_LED_Toggle()
*
* Description : Toggles any or all the LEDs on the board.
*
* Argument(s) : led   The LED to control.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_LED_Toggle (BSP_LED  led)
{
    switch (led) {
        case 0u:
             PORT0.PODR.BIT.B3 = PORT0.PODR.BIT.B3 ^ DEF_SET;
             PORT0.PODR.BIT.B5 = PORT0.PODR.BIT.B5 ^ DEF_SET;
             PORT7.PODR.BIT.B3 = PORT7.PODR.BIT.B3 ^ DEF_SET;
             PORTJ.PODR.BIT.B5 = PORTJ.PODR.BIT.B5 ^ DEF_SET;
             break;


        case 1u:
             PORT0.PODR.BIT.B3 = PORT0.PODR.BIT.B3 ^ DEF_SET;
             break;


        case 2u:
             PORT0.PODR.BIT.B5 = PORT0.PODR.BIT.B5 ^ DEF_SET;
             break;


        case 3u:
             PORT7.PODR.BIT.B3 = PORT7.PODR.BIT.B3 ^ DEF_SET;
             break;


        case 4u:
             PORTJ.PODR.BIT.B5 = PORTJ.PODR.BIT.B5 ^ DEF_SET;
             break;


        default:
             break;
    }
}
