/*
*********************************************************************************************************
*                                         BOARD SUPPORT PACKAGE
*
*                            (c) Copyright 2011; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                      Renesas RX Specific code
*
* File      : BSP_TICK_C.C
* By        : PC
*
* LICENSING TERMS:
* ---------------
*             BSP is provided in source form to registered licensees ONLY.  It is
*             illegal to distribute this source code to any third party unless you receive
*             written permission by an authorized Micrium representative.  Knowledge of
*             the source code may NOT be used to develop a similar product.
*
*             Please help us continue to provide the Embedded community with the finest
*             software available.  Your honesty is greatly appreciated.
*
*             You can contact us at www.micrium.com.
*********************************************************************************************************
*/

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *bsp_tick_c__c = "$Id: $";
#endif


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  <bsp_int_vect_tbl.h>
#include  <bsp_sys.h>

#include  "iodefine.h"

#include  "bsp_cfg.h"
#if       BSP_CFG_OS3_EN   > 0u
#include  <os.h>
#endif

#if       BSP_CFG_OS2_EN   > 0u
#include  <ucos_ii.h>
#endif


/*
*********************************************************************************************************
*                                          OS_BSP_TickInit()
*
* Description : Initialize the kernel's tick interrupt.
*
* Argument(s) : none.
*
* Caller(s)   : Application.
*
* Return(s)   : none.
*
* Note(s)     : (1) This function MUST be called after OSStart() & after processor initialization.
*
*               (2) The IPL (Interrupt Priority Level) for the CMT0 Timer Module can be found
*                   and adjusted in the bsp_cfg.h
*********************************************************************************************************
*/

void  OS_BSP_TickInit (void)
{
    CPU_INT32U  periph_clk_freq;
    CPU_INT16U  timeout_val;


    periph_clk_freq = BSP_SysPeriphClkFreqGet(CLK_ID_PCLKB);



    SYSTEM.PRCR.WORD = 0xA507;                                  /* Unlock protection register.                          */
    MSTP(CMT0) = 0;                                             /* Enable CMT0 Module.                                  */
    SYSTEM.PRCR.WORD = 0xA500;                                  /* Relock protection register.                          */

    CMT.CMSTR0.BIT.STR0 = 0;                                    /* Stop Timer Channel 0.                                */

    CMT0.CMCR.BIT.CKS = 1;                                      /* Set Peripheral Clock Divider.                        */
                                                                /* Clock Setup: PCLK/32                                 */

                                                                /* Set Compare-Match Value.                             */
#if (OS_VERSION >= 30000u)
    timeout_val = periph_clk_freq / OSCfg_TickRate_Hz;
#else
    timeout_val = periph_clk_freq / OS_TICKS_PER_SEC;
#endif

    timeout_val /= 32u;
    CMT0.CMCOR = timeout_val - 1u;

    IPR(CMT0, CMI0) = BSP_CFG_OS_TICK_IPL;                      /* Set Interrupt Priority. See Note(2)                  */
    IEN(CMT0, CMI0) = 1;                                        /* Enable Interrupt Source.                             */

    CMT0.CMCR.BIT.CMIE = 1;                                     /* Enable Interrupt.                                    */
    CMT.CMSTR0.BIT.STR0 = 1;                                    /* Start Timer.                                         */
}


/*
*********************************************************************************************************
*                                         OS_BSP_TickISR()
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

#ifdef   __RENESAS__
#pragma  interrupt  OS_BSP_TickISR
#endif

CPU_ISR  OS_BSP_TickISR (void)
{
    OSIntEnter();                            /* Notify uC/OS-III or uC/OS-II of ISR entry            */
    CPU_INT_GLOBAL_EN();                     /* Reenable global interrupts                           */
    OSTimeTick();
    OSIntExit();                             /* Notify uC/OS-III of ISR exit                                           */
}
