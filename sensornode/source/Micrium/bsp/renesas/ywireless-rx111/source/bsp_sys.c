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
* Filename      : bsp_sys.c
* Version       : V1.00
* Programmer(s) : DC
*                 JJL
*                 JPC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <cpu_core.h>
#include  <lib_def.h>
#include  "iodefine.h"
#include  "bsp_sys.h"


/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#ifdef RX231
#define  EXT_CLK_FREQ           8000000u
#define  PLL_MULT_REG           0x17u                            /* multiply freq by 12                                   */
#else
#define  EXT_CLK_FREQ           12000000u
#define  PLL_MULT_REG           0xFu                            /* multiply freq by 8                                   */
#endif
#define  PLL_DIV_REG            0x1u                            /* divide   freq by 2                                   */
#define  PLL_OUTPUT_FREQ        48000000u                       /* PLL output == (EXT_CLK_FREQ * mult / div) == 48 MHz */

#define  ICLK_DIV_REG           1u                              /* divide all periph clocks by 2 (output = 24 MHz)      */
#ifdef RX231
#define  BCLK_DIV_REG           1u
#define  PCLKA_DIV_REG          1u
#endif
#define  PCLKB_DIV_REG          1u
#define  PCLKD_DIV_REG          1u
#define  FCLK_DIV_REG           1u

#define  CLK_SRC_LOCO           0u
#define  CLK_SRC_HOCO           1u
#define  CLK_SRC_MCO            2u
#define  CLK_SRC_SCO            3u
#define  CLK_SRC_PLL            4u


/*
*********************************************************************************************************
*                                         BSP SYSTEM INITIALIZATION
*
* Description: Initializes main oscillator circuit and sets PLL as system clock source
*********************************************************************************************************
*/

void  BSP_SysInit (void)
{
    SYSTEM.PRCR.WORD = 0xA503;
                                                                /* Enable main clock oscillator                         */
    SYSTEM.MOFCR.BIT.MODRV21 = 1;                               /* MCO drive capability is within [10, 20] MHz          */
    SYSTEM.MOSCCR.BIT.MOSTP  = 0;
    while (SYSTEM.OSCOVFSR.BIT.MOOVF != 1);                     /* Wait for main oscillator to stabilize                */

                                                                /* Configure PLL to output freq                         */
    SYSTEM.PLLCR.BIT.STC    = PLL_MULT_REG;
    SYSTEM.PLLCR.BIT.PLIDIV = PLL_DIV_REG;

    SYSTEM.PLLCR2.BIT.PLLEN = 0;                                /* Enable PLL                                           */
    while (SYSTEM.OSCOVFSR.BIT.PLOVF != 1);                     /* Wait for PLL to stabilize                            */

                                                                /* Setup system clock dividers                          */
    SYSTEM.SCKCR.BIT.ICK  = ICLK_DIV_REG;
#ifdef RX231
    SYSTEM.SCKCR.BIT.BCK  = BCLK_DIV_REG;
    SYSTEM.SCKCR.BIT.PCKA = PCLKA_DIV_REG;
#endif
    SYSTEM.SCKCR.BIT.PCKB = PCLKB_DIV_REG;
    SYSTEM.SCKCR.BIT.PCKD = PCLKD_DIV_REG;
    SYSTEM.SCKCR.BIT.FCK  = FCLK_DIV_REG;

    SYSTEM.SCKCR3.BIT.CKSEL = CLK_SRC_PLL;                      /* Set PLL circuit as system clock source               */

    SYSTEM.SOSCCR.BIT.SOSTP = 1;
    RTC.RCR3.BIT.RTCEN = 0;
    SYSTEM.PRCR.WORD = 0xA500;
}


/*
*********************************************************************************************************
*                                     PERIPHERAL CLOCK FREQUENCY
*
* Description: This function is used to retrieve peripheral clock frequency.
*
* Arguments  : none
*
* Return     : Peripheral clock frequency in cycles.
*********************************************************************************************************
*/

CPU_INT32U  BSP_SysPeriphClkFreqGet (int clk_id)
{
    CPU_INT32S  div_pow;
    CPU_INT32U  periph_clk;


    switch (clk_id) {
        case CLK_ID_ICLK:                                       /* System Clock.                                        */
             div_pow = SYSTEM.SCKCR.BIT.ICK;
             break;
        case CLK_ID_FCLK:                                       /* Flash clock.                                         */
             div_pow = SYSTEM.SCKCR.BIT.FCK;
             break;
        case CLK_ID_PCLKB:
             div_pow = SYSTEM.SCKCR.BIT.PCKB;
             break;
        case CLK_ID_PCLKD:
             div_pow = SYSTEM.SCKCR.BIT.PCKD;
             break;
             /* TODO: add clk div getters for other clock sources */
        default:
             div_pow = -1;
             break;
    }

                                                                /* divider value = 2 ^ div                              */
    if (div_pow != -1) {
        periph_clk = PLL_OUTPUT_FREQ / (1u << div_pow);
    } else {
        periph_clk = 0u;
    }

    return (periph_clk);
}


/*
*********************************************************************************************************
*                                               BSP_SysReset()
*
* Description: Resets the MCU
*
* Arguments  : none
*
* Return     : none
*********************************************************************************************************
*/


void  BSP_SysReset (void)
{
    SYSTEM.PRCR.WORD = 0xA502;                                  /* Unlock protection register.                          */
#ifdef RX231
    SYSTEM.SWRR.BIT.SWRR = 0xA501;                                  /* Software reset.                                      */
#else
    SYSTEM.SWRR = 0xA501;                                  /* Software reset.                                      */
#endif
}


/*
*********************************************************************************************************
*                                               BSP_SysBrk()
*
* Description: Generates a non-maskable interrupt (NMI) using the BRK instruction
*
* Arguments  : none
*
* Return     : none
*********************************************************************************************************
*/


void  BSP_SysBrk (void)
{
#ifdef  __IAR_SYSTEMS_ICC__
    asm("    BRK");
#else
# error "Compiler not supported. Inline assembly for the BRK instruction is not present."
#endif
}
