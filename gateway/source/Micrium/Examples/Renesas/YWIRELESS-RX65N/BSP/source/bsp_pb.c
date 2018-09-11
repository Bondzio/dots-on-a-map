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
*                                            PUSH BUTTONS BSP
*
* File : bsp_pb.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  "bsp_pb.h"


/*
*********************************************************************************************************
*                                          LOCAL DEFINES
*********************************************************************************************************
*/

#define  BSP_PB_1                  PORT0.PIDR.BIT.B0
#define  BSP_PB_2                  PORT0.PIDR.BIT.B1
#define  BSP_PB_3                  PORT0.PIDR.BIT.B7


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     PUSH BUTTON INITIALIZATION
*
* Description: This function is used to initialize the push buttons.
*
* Argument(s): None.
*
* Return(s)  : None.
*
* Caller(s)  : BSP_Init()
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  BSP_PB_Init (void)
{                                                               /* Set pins to input.                                   */
    PORT0.PDR.BIT.B0 = DEF_CLR;                                 /* PB1                                                  */
    PORT0.PDR.BIT.B1 = DEF_CLR;                                 /* PB2                                                  */
    PORT0.PDR.BIT.B7 = DEF_CLR;                                 /* PB3                                                  */
}


/*
*********************************************************************************************************
*                                          PUSH BUTTON READ
*
* Description: This function is used to read the values of user push buttons.
*
* Arguments  : pushbutton
*                   The push button to read.
*
* Return(s)  : DEF_ON  if the switch is pressed
*              DEF_OFF if the switch is released
*
* Caller(s)  : Application.
*
* Note(s)    : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_PB_Read (BSP_PB  pushbutton)
{                                                               /* A closed switch grounds the pin to logical-0.        */
    switch(pushbutton) {
        case 0u:
             return (BSP_PB_1 ? DEF_OFF : DEF_ON);


        case 1u:
             return (BSP_PB_2 ? DEF_OFF : DEF_ON);


        case 2u:
             return (BSP_PB_3 ? DEF_OFF : DEF_ON);


        default:
             return DEF_OFF;
    }
}
