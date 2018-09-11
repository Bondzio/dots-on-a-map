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
*                                         BOARD SUPPORT PACKAGE
*
*                                            Renesas RX111
*                                                on the
*                                           YWIRELESS-RX111
*
* Filename      : bsp_switch.c
* Version       : V1.00
* Programmer(s) : JPC
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  "iodefine.h"
#include  "bsp_switch.h"
#include  "bsp_sys.h"


/*
*********************************************************************************************************
*                                      DEFAULT CONFIGURATION
*********************************************************************************************************
*/

#ifndef  BSP_SWITCH_CALLBACK_ARRAY_SIZE
#define BSP_SWITCH_CALLBACK_ARRAY_SIZE     3
#endif


/*
*********************************************************************************************************
*                                          LOCAL DEFINES
*********************************************************************************************************
*/

#ifdef RX231
// on the ywireless-rx231 kit, switch 2 is pb 1, switch 3 is pb 2, and switch 4 is pb 3
#define  BSP_SWITCH_2               PORT3.PIDR.BIT.B5
#define  BSP_SWITCH_2_PDR           // TODO: NMI
#define  BSP_SWITCH_2_PFS_ISEL      // TODO: NMI
#define  BSP_SWITCH_2_IRQ           // TODO: NMI
#define  BSP_SWITCH_2_IRQ_NUM       // TODO: NMI
#define  BSP_SWITCH_3               PORTE.PIDR.BIT.B7
#define  BSP_SWITCH_3_PDR           PORTE.PDR.BIT.B7
#define  BSP_SWITCH_3_PFS_ISEL      MPC.PE7PFS.BIT.ISEL
#define  BSP_SWITCH_3_IRQ_NUM       7
#define  BSP_SWITCH_4               PORTE.PIDR.BIT.B6
#define  BSP_SWITCH_4_PDR           PORTE.PDR.BIT.B6
#define  BSP_SWITCH_4_PFS_ISEL      MPC.PE6PFS.BIT.ISEL
#define  BSP_SWITCH_4_IRQ_NUM       6
#else
#define  BSP_SWITCH_2               PORTB.PIDR.BIT.B1
#define  BSP_SWITCH_2_PDR           PORTB.PDR.BIT.B1
#define  BSP_SWITCH_2_PFS_ISEL      MPC.PB1PFS.BIT.ISEL
#define  BSP_SWITCH_2_IRQ_NUM       4
#define  BSP_SWITCH_3               PORTE.PIDR.BIT.B3
#define  BSP_SWITCH_3_PDR           PORTE.PDR.BIT.B3
#define  BSP_SWITCH_3_PFS_ISEL      MPC.PE3PFS.BIT.ISEL
#define  BSP_SWITCH_3_IRQ_NUM       3
#define  BSP_SWITCH_4               PORT3.PIDR.BIT.B2
#define  BSP_SWITCH_4_PDR           PORT3.PDR.BIT.B2
#define  BSP_SWITCH_4_PFS_ISEL      MPC.P32PFS.BIT.ISEL
#define  BSP_SWITCH_4_IRQ_NUM       2
#endif

#define  INPUT                      0
#define _NUM_TO_IRQ(n) IRQ ## n
#define NUM_TO_IRQ(n) _NUM_TO_IRQ(n)
#define _IEN_ICU_MACRO(x) IEN(ICU, x)
#define IEN_ICU_MACRO(x) _IEN_ICU_MACRO(x)
#define IPR_ICU(x) IPR(ICU, x)
#define IR_ICU(x) IR(ICU, x)

/*
*********************************************************************************************************
*                                           LOCAL GLOBALS
*********************************************************************************************************
*/

static  SWITCH_INT_FNCT  SwitchCallbacks[3][BSP_SWITCH_CALLBACK_ARRAY_SIZE];



/*
*********************************************************************************************************
*                                          SWS INITIALIZATION
*
* Description: This function is used to initialize switches.
*
* Argument(s): None.
*
* Return(s)  : None.
*
* Caller(s)  : BSP_MiscInit()
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  BSP_SwitchInit (void)
{
                                                               /* Set pins to input.                                   */
#ifdef RX231
#else
    BSP_SWITCH_2_PDR = INPUT;                                   /* SW2                                                  */
#endif
    BSP_SWITCH_3_PDR = INPUT;                                   /* SW3                                                  */
    BSP_SWITCH_4_PDR = INPUT;                                   /* SW4                                                  */

                                                                /* Enable interrupt generation for these pins           */
    MPC.PWPR.BIT.B0WI  = 0;                                     /* Disable write protection for MPC.                    */
    MPC.PWPR.BIT.PFSWE = 1;

#ifdef RX231
#else
    BSP_SWITCH_2_PFS_ISEL = 1;                                    /* IRQ4                                                 */
#endif
    BSP_SWITCH_3_PFS_ISEL = 1;                                    /* IRQ3                                                 */
    BSP_SWITCH_4_PFS_ISEL = 1;                                    /* IRQ2                                                 */

    MPC.PWPR.BIT.PFSWE = 0;
    MPC.PWPR.BIT.B0WI  = 1;                                     /* Enable write protection for MPC.                    */

    ICU.IRQCR[BSP_SWITCH_4_IRQ_NUM].BIT.IRQMD  = 0x1;                              /* Rising and falling edge triggered  TODO              */
    ICU.IRQFLTE0.BIT.FLTEN2 =   1;                              /* Enable digital filter                                */
    ICU.IRQFLTC0.BIT.FCLKSEL2 = 3;
    IPR_ICU(NUM_TO_IRQ(BSP_SWITCH_4_IRQ_NUM)) = 0x1;                              /* Set IPL to lowest setting.                           */
    IR_ICU(NUM_TO_IRQ(BSP_SWITCH_4_IRQ_NUM)) =   0;                              /* Clear any pending interrupt.                         */

    ICU.IRQCR[BSP_SWITCH_3_IRQ_NUM].BIT.IRQMD  = 0x1;
    ICU.IRQFLTE0.BIT.FLTEN3 =   1;
    ICU.IRQFLTC0.BIT.FCLKSEL3 = 3;
    IPR_ICU(NUM_TO_IRQ(BSP_SWITCH_3_IRQ_NUM)) = 0x1;
    IR_ICU(NUM_TO_IRQ(BSP_SWITCH_3_IRQ_NUM)) =   0;

#ifdef RX231
#else
    ICU.IRQCR[BSP_SWITCH_2_IRQ_NUM].BIT.IRQMD  = 0x1;
    ICU.IRQFLTE0.BIT.FLTEN4 =   1;
    ICU.IRQFLTC0.BIT.FCLKSEL4 = 3;
    IPR_ICU(NUM_TO_IRQ(BSP_SWITCH_2_IRQ_NUM)) = 0x1;
    IR_ICU(NUM_TO_IRQ(BSP_SWITCH_2_IRQ_NUM)) =   0;
#endif
}


/*
*********************************************************************************************************
*                                             SWITCH READ
*
* Description: Polls the current state of a switch
*
* Arguments  : switch_nbr
*                   The switch to read.
*
* Return(s)  : DEF_ON  if the switch is pressed
*              DEF_OFF if the switch is released
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_SwitchRd (CPU_INT08U  switch_nbr)
{                                                               /* A closed switch grounds the pin to logical-0.        */
    switch (switch_nbr) {
    case 2:
        return (BSP_SWITCH_2 ? DEF_OFF : DEF_ON);

    case 3:
        return (BSP_SWITCH_3 ? DEF_OFF : DEF_ON);

    case 4:
        return (BSP_SWITCH_4 ? DEF_OFF : DEF_ON);

    default:
        return DEF_OFF;
    }
}


/*
*********************************************************************************************************
*                                       INTERRUPT REGISTRATION
*
* Description: Registers a callback function to be invoked when a switch is either pressed, released, or
*              both.
*
* Arguments  : switch_nbr
*                   The switch to read.
*
* Return(s)  : DEF_ON  if the switch is pressed
*              DEF_OFF if the switch is released
*********************************************************************************************************
*/

int  BSP_SwitchIntReg (CPU_INT08U  switch_nbr, SWITCH_INT_FNCT  callback)
{
    int ret;
    CPU_SR_ALLOC();


    ret = -1;
    if ((switch_nbr >= 2) && (switch_nbr <= 4) && (callback)) {
        CPU_CRITICAL_ENTER();
                                                                /* Find an empty slot for this callback                 */
        for (ret = 0; ret < BSP_SWITCH_CALLBACK_ARRAY_SIZE; ++ret) {
            if (SwitchCallbacks[switch_nbr - 2][ret] == 0) {
                SwitchCallbacks[switch_nbr - 2][ret] = callback;
                break;
            }
        }

        CPU_CRITICAL_EXIT();

        if (ret == BSP_SWITCH_CALLBACK_ARRAY_SIZE) {
            ret = -1;
        }
                                                                /* Make sure interrupts are enabled for this switch     */
        switch (switch_nbr) {
        case 2:
#ifdef RX231
#else
            IEN_ICU_MACRO(NUM_TO_IRQ(BSP_SWITCH_2_IRQ_NUM)) = 1;
#endif
            break;
        case 3:
            IEN_ICU_MACRO(NUM_TO_IRQ(BSP_SWITCH_3_IRQ_NUM)) = 1;
            break;
        case 4:
            IEN_ICU_MACRO(NUM_TO_IRQ(BSP_SWITCH_4_IRQ_NUM)) = 1;
            break;
        default:
            break;
        }
    }


    return ret;
}



CPU_ISR  BSP_SwitchIRQ2 (void)
{
    int i = 0;
    SWITCH_INT_FNCT callback;

    for (i = 0; i < BSP_SWITCH_CALLBACK_ARRAY_SIZE; ++i) {
        callback = SwitchCallbacks[2][i];
        if (callback != 0) {
            callback(4, SWITCH_EVENT_OFF_TO_ON);
        }
    }
}


CPU_ISR  BSP_SwitchIRQ3 (void)
{
    int i = 0;
    SWITCH_INT_FNCT callback;

    for (i = 0; i < BSP_SWITCH_CALLBACK_ARRAY_SIZE; ++i) {
        callback = SwitchCallbacks[1][i];
        if (callback != 0) {
            callback(3, SWITCH_EVENT_OFF_TO_ON);
        }
    }
}

CPU_ISR  BSP_SwitchIRQ4 (void)
{
    int i = 0;
    SWITCH_INT_FNCT callback;

    for (i = 0; i < BSP_SWITCH_CALLBACK_ARRAY_SIZE; ++i) {
        callback = SwitchCallbacks[0][i];
        if (callback != 0) {
            callback(2, SWITCH_EVENT_OFF_TO_ON);
        }
    }
}
