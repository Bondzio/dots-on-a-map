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
*                                             OS TICK BSP
*
* File : bsp_os.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <os.h>
#include  <bsp_int_vect_tbl.h>
#include  <bsp_clk.h>
#include  <bsp_os.h>

#include  "iorx651.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  BSP_CFG_OS_TICK_IPL                3u


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
*                                           LOCAL CONSTANTS
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
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                    INITIALIZE OS TICK INTERRUPT
*
* Description : Initialize the tick interrupt.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none
*********************************************************************************************************
*/

void  BSP_OSTickInit (void)
{
    CPU_INT32U  periph_clk_freq;
    CPU_INT16U  cnt_val;
    CPU_INT16U  pclk_div;


    periph_clk_freq = BSP_ClkFreqGet(BSP_CLK_ID_PCLKB);
    BSP_IntVectSet(VECT_CMT0_CMI0, (CPU_FNCT_VOID)BSP_OSTickISR);

    SYSTEM.PRCR.WORD    = 0xA502;                               /* Disable register write protection.                   */
    MSTP(CMT0)          = 0u;                                   /* Enable CMT0 module.                                  */

    CMT.CMSTR0.BIT.STR0 = 0u;                                   /* Stop timer channel 0.                                */
    CMT0.CMCR.BIT.CKS   = 1u;                                   /* Set peripheral clock divider.                        */

    pclk_div = CMT0.CMCR.BIT.CKS;
    switch (pclk_div) {
        case 0:                                                 /* Get PCLK divider.                                    */
             pclk_div = 8u;
             break;


        case 1:
             pclk_div = 32u;
             break;


        case 2:
             pclk_div = 128u;
             break;


        case 3:
             pclk_div = 512u;
             break;


        default:
             pclk_div = 1u;                                     /* Invalid value.                                       */
             break;
    }

    periph_clk_freq /= pclk_div;

#if (OS_VERSION >= 30000u)
    cnt_val = (periph_clk_freq / OSCfg_TickRate_Hz) - 1u;
#else
    cnt_val = (periph_clk_freq / OS_TICKS_PER_SEC) - 1u;
#endif

    CMT0.CMCOR = cnt_val;                                       /* Set compare-match value.                             */
    CMT0.CMCNT = 0;                                             /* Clear counter register.                              */

    IR(CMT0,  CMI0) = 0u;                                       /* Clear any pending ISR.                               */
    IPR(CMT0, CMI0) = BSP_CFG_OS_TICK_IPL;                      /* Set interrupt priority.                              */
    IEN(CMT0, CMI0) = 1u;                                       /* Enable interrupt source.                             */

    CMT0.CMCR.BIT.CMIE  = 1u;                                   /* Enable interrupt.                                    */
    CMT.CMSTR0.BIT.STR0 = 1u;                                   /* Start timer.                                         */
}


/*
*********************************************************************************************************
*                                    KERNEL TICK INTERRUPT HANDLER
*
* Description : Kernel's tick interrupt handler
*
* Argument(s) : none.
*
* Caller(s)   : tick interrupt.
*
* Return(s)   : none.
*********************************************************************************************************
*/

#if      __RENESAS__
#pragma  interrupt  BSP_OSTickISR
#endif

CPU_ISR  BSP_OSTickISR (void)
{
    OSIntEnter();                                               /* Notify uC/OS-III or uCOS-II of ISR entry             */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */
    OSTimeTick();
    OSIntExit();                                                /* Notify uC/OS-III or uCOS-II of ISR exit              */
}
