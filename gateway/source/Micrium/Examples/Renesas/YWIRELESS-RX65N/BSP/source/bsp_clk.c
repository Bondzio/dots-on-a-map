/*
*********************************************************************************************************
*                                            EXAMPLE CODE
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
*                                              CLOCK BSP
*
* File : bsp_clk.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <bsp_clk.h>
#include  <bsp.h>
#include  <lib_def.h>
#include  "iorx651.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* --------------- PLL CONTROL REGISTER --------------- */
#define  BSP_SYS_PLLCR_PLLDIV_1             0x00u
#define  BSP_SYS_PLLCR_PLLDIV_1_2           0x01u
#define  BSP_SYS_PLLCR_PLLDIV_1_3           0x02u

#define  BSP_SYS_PLLCR_STC_x10              0x13u
#define  BSP_SYS_PLLCR_STC_x10_5            0x14u
#define  BSP_SYS_PLLCR_STC_x11              0x15u
#define  BSP_SYS_PLLCR_STC_x11_5            0x16u
#define  BSP_SYS_PLLCR_STC_x12              0x17u
#define  BSP_SYS_PLLCR_STC_x12_5            0x18u
#define  BSP_SYS_PLLCR_STC_x13              0x19u
#define  BSP_SYS_PLLCR_STC_x13_5            0x1Au
#define  BSP_SYS_PLLCR_STC_x14              0x1Bu
#define  BSP_SYS_PLLCR_STC_x14_5            0x1Cu
#define  BSP_SYS_PLLCR_STC_x15              0x1Du
#define  BSP_SYS_PLLCR_STC_x15_5            0x1Eu
#define  BSP_SYS_PLLCR_STC_x16              0x1Fu
#define  BSP_SYS_PLLCR_STC_x16_5            0x20u
#define  BSP_SYS_PLLCR_STC_x17              0x21u
#define  BSP_SYS_PLLCR_STC_x17_5            0x22u
#define  BSP_SYS_PLLCR_STC_x18              0x23u
#define  BSP_SYS_PLLCR_STC_x18_5            0x24u
#define  BSP_SYS_PLLCR_STC_x19              0x25u
#define  BSP_SYS_PLLCR_STC_x19_5            0x26u
#define  BSP_SYS_PLLCR_STC_x20              0x27u
#define  BSP_SYS_PLLCR_STC_x20_5            0x28u
#define  BSP_SYS_PLLCR_STC_x21              0x29u
#define  BSP_SYS_PLLCR_STC_x21_5            0x2Au
#define  BSP_SYS_PLLCR_STC_x22              0x2Bu
#define  BSP_SYS_PLLCR_STC_x22_5            0x2Cu
#define  BSP_SYS_PLLCR_STC_x23              0x2Du
#define  BSP_SYS_PLLCR_STC_x23_5            0x2Eu
#define  BSP_SYS_PLLCR_STC_x24              0x2Fu
#define  BSP_SYS_PLLCR_STC_x24_5            0x30u
#define  BSP_SYS_PLLCR_STC_x25              0x31u
#define  BSP_SYS_PLLCR_STC_x25_5            0x32u
#define  BSP_SYS_PLLCR_STC_x26              0x33u
#define  BSP_SYS_PLLCR_STC_x26_5            0x34u
#define  BSP_SYS_PLLCR_STC_x27              0x35u
#define  BSP_SYS_PLLCR_STC_x27_5            0x36u
#define  BSP_SYS_PLLCR_STC_x28              0x37u
#define  BSP_SYS_PLLCR_STC_x28_5            0x38u
#define  BSP_SYS_PLLCR_STC_x29              0x39u
#define  BSP_SYS_PLLCR_STC_x29_5            0x3Au
#define  BSP_SYS_PLLCR_STC_x30              0x3Bu


#define  SCKCR_FREQ_DIV1                    0x00u
#define  SCKCR_FREQ_DIV2                    0x01u
#define  SCKCR_FREQ_DIV4                    0x02u
#define  SCKCR_FREQ_DIV8                    0x03u
#define  SCKCR_FREQ_DIV16                   0x04u
#define  SCKCR_FREQ_DIV32                   0x05u
#define  SCKCR_FREQ_DIV64                   0x06u


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
*                                   BSP SYSTEM CLOCK INITIALIZATION
*
* Description : Sets up PLL to oscillate at 192 MHz, and configures peripheral clock dividers
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : 1) Restrictions on Setting of ROMWT[1:0] bits:
*                  a) if          ICLK <=  50Mhz, then ROMWT[1:0] = 00b or 01b or 10b
*                  b) if  50Mhz < ICLK <= 100Mhz, then ROMWT[1:0] = 01b or 10b
*                  c) if 100Mhz < ICLK <= 120Mhz, then ROMWT[1:0] = 10b
*
*********************************************************************************************************
*/

void  BSP_ClkCfg (void)
{
    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_ENABLED);        /* Enable writing to Clock Generation registers.        */

                                                                /* -------------- SELECT CLOCK SOURCE ----------------- */
                                                                /* Main clock will not oscillate in software standby ...*/
                                                                /* ... or deep software standby modes.                  */
    SYSTEM.MOFCR.BIT.MOFXIN = 0;                                /* Oscillator will not be controlled by this bit        */
    SYSTEM.MOFCR.BIT.MOSEL  = 0;                                /* Resonator is used for clock input.                   */

    SYSTEM.HOCOCR.BYTE      = 0x01;                             /* Stop HOCO.                                           */
    SYSTEM.HOCOPCR.BYTE     = 0x01;                             /* Turn off the power supply of HOCO.                   */

    SYSTEM.MOFCR.BIT.MODRV2 = 0u;                               /* Set Main Clk Osc. drive ability to 20-24 MHz         */

    SYSTEM.MOSCWTCR.BYTE    = 0x29;                             /* Main Clk Osc Wait Time.                              */
    SYSTEM.MOSCCR.BYTE      = 0u;                               /* Set Main Clk Osc to Operation Mode.                  */

    while ((0 == SYSTEM.OSCOVFSR.BIT.MOOVF)) {                  /* Wait for Main Clk Osc to be stable for Sys Clk use.  */
        ;
    }

    SYSTEM.ROMWT.BIT.ROMWT = 0x2u;                              /* See Note 1!!!.                                       */

    SYSTEM.SOSCCR.BYTE = 0x01;                                  /* Set the sub-clock to stopped.                        */

                                                                /* ---------------- PLL CONFIGURATION ----------------- */
    SYSTEM.PLLCR.BIT.PLIDIV    = BSP_SYS_PLLCR_PLLDIV_1;        /*      - PLIDIV = 1/1                                  */
    SYSTEM.PLLCR.BIT.PLLSRCSEL = 0u;                            /* Set main Clk Osc as PLL Clk Source.                  */
    SYSTEM.PLLCR.BIT.STC       = BSP_SYS_PLLCR_STC_x10;         /*      - STC    = 10                                   */
                                                                /*      - PLL    = (XTAL_FREQ * PLIDIV) * STC           */
                                                                /*               = (24000000 * 1/1) * 10                */
                                                                /*               = 240MHz                               */

    SYSTEM.PLLCR2.BYTE = 0x00;                                  /* Set the PLL to operating.                            */

    while ((0 == SYSTEM.OSCOVFSR.BIT.PLOVF)) {                  /* Wait for PLL clock to stabilize.                     */
        ;
    }

                                                                /* ---------------- SYSTEM CONFIGURATION -------------- */
    SYSTEM.SCKCR.LONG = (SCKCR_FREQ_DIV4 << 28u)  |             /* ... FCLK  = PLL * 1/4 =  60Mhz (FlashIF clock)       */
                        (SCKCR_FREQ_DIV2 << 24u)  |             /* ... ICLK  = PLL * 1/2 = 120Mhz (System clock)        */
                        (SCKCR_FREQ_DIV2 << 16u)  |             /* ... BCLK  = PLL * 1/2 = 120Mhz (External bus clock)  */
                        (SCKCR_FREQ_DIV2 << 12u)  |             /* ... PCLKA = PLL * 1/2 = 120Mhz (Peripheral clock A)  */
                        (SCKCR_FREQ_DIV4 <<  8u)  |             /* ... PCLKB = PLL * 1/4 =  60Mhz (Peripheral clock B)  */
                        (SCKCR_FREQ_DIV4 <<  4u)  |             /* ... PCLKC = PLL * 1/4 =  60Mhz (Peripheral clock C)  */
                        (SCKCR_FREQ_DIV4 <<  0u);               /* ... PCLKD = PLL * 1/4 =  60Mhz (Peripheral clock D)  */

    SYSTEM.SCKCR2.WORD = 0x41;                                  /* ... UCLK  = PLL * 1/5 = 48Mhz                        */

    SYSTEM.SCKCR3.WORD = 0x0400;                                /* Switch to PLL clock                                  */

    SYSTEM.LOCOCR.BYTE = 0x01;                                  /* Turn LOCO off since it is not going to be used.      */

    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_DISABLED);       /* Disable writing to Clock Generation registers.       */
}


/*
*********************************************************************************************************
*                                       SYSTEM CLOCK FREQUENCY
*
* Description : Returns the clock frequency of a periphal clock, specified by 'clk_id'
*
* Argument(s) : clk_id      System clock identifiers:

*                               BSP_CLK_ID_ICLK,
*                               BSP_CLK_ID_PCLKA,
*                               BSP_CLK_ID_PCLKB,
*                               BSP_CLK_ID_FCLK,
*                               BSP_CLK_ID_BCLK,
*                               BSP_CLK_ID_BCLK_PIN,
*                               BSP_CLK_ID_SDCLK,
*                               BSP_CLK_ID_UCLK,
*                               BSP_CLK_ID_CANMCLK,
*                               BSP_CLK_ID_RTCSCLK,
*                               BSP_CLK_ID_RTCMCLK,
*                               BSP_CLK_ID_IWDTCLK,
*                               BSP_CLK_ID_JTAGTCK
*
* Return(s)   : clock frequency in Hertz.
*
* Caller(s)   : Application.
*********************************************************************************************************
*/

CPU_INT32U  BSP_ClkFreqGet (BSP_CLK_ID  clk_id)
{
    CPU_INT32U  pll_stc;
    CPU_INT32U  pll_stc_frac;
    CPU_INT32U  pll_div;
    CPU_INT32U  pll_freq;
    CPU_INT32U  periph_div;
    CPU_INT32U  clk_freq;
    CPU_INT16U  clk_div;


    clk_freq = 0u;

    if ((clk_id == BSP_CLK_ID_CANMCLK) ||
        (clk_id == BSP_CLK_ID_RTCMCLK)) {
        clk_freq = BSP_CFG_SYS_EXT_CLK_FREQ;

    } else if (clk_id == BSP_CLK_ID_RTCSCLK) {
        clk_freq = BSP_CFG_SYS_EXT_SUB_CLK_FREQ;

    } else if (clk_id == BSP_CLK_ID_IWDTCLK) {
        clk_freq = BSP_SYS_IWDTCLK_FREQ;

    } else {
        switch (clk_id) {
            case BSP_CLK_ID_ICLK:
                 clk_div = SYSTEM.SCKCR.BIT.ICK;
                 break;


            case BSP_CLK_ID_PCLKA:
                 clk_div = SYSTEM.SCKCR.BIT.PCKA;
                 break;


            case BSP_CLK_ID_PCLKB:
                 clk_div = SYSTEM.SCKCR.BIT.PCKB;
                 break;


            case BSP_CLK_ID_FCLK:
                 clk_div = SYSTEM.SCKCR.BIT.FCK;
                 break;


            case BSP_CLK_ID_BCLK:
            case BSP_CLK_ID_BCLK_PIN:
            case BSP_CLK_ID_SDCLK:
                 clk_div = SYSTEM.SCKCR.BIT.BCK;
                 break;


            case BSP_CLK_ID_UCLK:
                 clk_div = SYSTEM.SCKCR2.BIT.UCK + 1u;
                 break;


            default:
                 clk_div = 0xFFFFu;
                 break;
        }

        if (clk_div != 0xFFFFu) {
            pll_stc = SYSTEM.PLLCR.BIT.STC;
            pll_stc_frac = (CPU_INT32U)(pll_stc + 1u) % 2u;     /* 1 if there is a fractional component to the      ... */
                                                                /* ... multiplier. 0 otherwise.                         */

            if (pll_stc % 2u == 1u) {                           /* Calculate the integer portion of the PLL multiplier. */
                pll_stc++;
            }
            pll_stc = pll_stc / 2u;
            pll_div  = (1u << SYSTEM.PLLCR.BIT.PLIDIV);         /* PLL divider = 2 ^ PLIDIV                         */
            pll_freq = (BSP_CFG_SYS_EXT_CLK_FREQ * pll_stc + pll_stc_frac * (BSP_CFG_SYS_EXT_CLK_FREQ / 2u)) / pll_div;

            if (clk_id == BSP_CLK_ID_UCLK) {
                periph_div = clk_div;                           /* Peripheral divider = clk_div                     */
            } else {
                periph_div = (1u << clk_div);                   /* Peripheral divider = 2 ^ clk_div                 */
            }

            clk_freq = pll_freq / periph_div;
        }
    }

    return (clk_freq);
}
