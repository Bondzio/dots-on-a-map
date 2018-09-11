/*
*********************************************************************************************************
*                                             uC/FS V4
*                                     The Embedded File System
*
*                         (c) Copyright 2008-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/FS is provided in source form to registered licensees ONLY.  It is
*                  illegal to distribute this source code to any third party unless you receive
*                  written permission by an authorized Micrium representative.  Knowledge of
*                  the source code may NOT be used to develop a similar product.
*
*                  Please help us continue to provide the Embedded community with the finest
*                  software available.  Your honesty is greatly appreciated.
*
*                  You can find our product's user manual, API reference, release notes and
*                  more information at: https://doc.micrium.com
*
*                  You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                      FILE SYSTEM DEVICE DRIVER
*
*                                             SD/MMC CARD
*                                               SPI MODE
*
*                                BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                          NXP LPC21488 ON THE IAR LPC2148 EVALUATION BOARD
*
* Filename      : fs_dev_sd_spi_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The LPC214X SPI peripheral is described in documentation from NXP :
*
*                         UM10139_2.  LPC214x User manual.  Rev.02 -- 25 July 2006.  "Chapter 12:
*                         SPI Interface (SPI0)".
*
*                (2) (a) LPC214X_FREQ_PCLK_HZ must be set to the PCLK frequency.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_SD_SPI_BSP_MODULE
#include  <fs_dev_sd_spi.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  LPC214X_FREQ_PCLK_HZ                       24000000u

#define  LPC2XXX_REG_PINSEL0                        (*(CPU_REG32 *)(0xE002C000u))
#define  LPC2XXX_REG_PINSEL1                        (*(CPU_REG32 *)(0xE002C004u))
#define  LPC2XXX_REG_PINSEL2                        (*(CPU_REG32 *)(0xE002C008u))

#define  LPC2XXX_REG_PCONP                          (*(CPU_REG32 *)(0xE01FC0C4u))

#define  LPC2XXX_REG_FIO0DIR                        (*(CPU_REG32 *)(0x3FFFC000u))
#define  LPC2XXX_REG_FIO0MASK                       (*(CPU_REG32 *)(0x3FFFC010u))
#define  LPC2XXX_REG_FIO0PIN                        (*(CPU_REG32 *)(0x3FFFC014u))
#define  LPC2XXX_REG_FIO0SET                        (*(CPU_REG32 *)(0x3FFFC018u))
#define  LPC2XXX_REG_FIO0CLR                        (*(CPU_REG32 *)(0x3FFFC01Cu))

#define  LPC2XXX_REG_SSP_BASE                                       0xE0068000u
#define  LPC2XXX_REG_SSPCR0                         (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x000u))
#define  LPC2XXX_REG_SSPCR1                         (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x004u))
#define  LPC2XXX_REG_SSPDR                          (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x008u))
#define  LPC2XXX_REG_SSPSR                          (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x00Cu))
#define  LPC2XXX_REG_SSPCPSR                        (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x010u))
#define  LPC2XXX_REG_SSPIMSC                        (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x014u))
#define  LPC2XXX_REG_SSPRIS                         (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x018u))
#define  LPC2XXX_REG_SSPMIS                         (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x01Cu))
#define  LPC2XXX_REG_SSPICR                         (*(CPU_REG32 *)(LPC2XXX_REG_SSP_BASE + 0x020u))

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

                                                                /* ---------------------- SSPCR0 ---------------------- */
#define  LPC2XXX_BIT_SSPCR0_DSS_MASK                0x0000000Fu /* Data Size Select.                                    */
#define  LPC2XXX_BIT_SSPCR0_FRF_MASK                0x00000030u
#define  LPC2XXX_BIT_SSPCR0_FRF_SPI                 0x00000000u /* Frame Format.                                        */
#define  LPC2XXX_BIT_SSPCR0_FRF_TI                  0x00000010u
#define  LPC2XXX_BIT_SSPCR0_FRF_MICROWIRE           0x00000020u
#define  LPC2XXX_BIT_SSPCR0_CPOL                    DEF_BIT_06  /* Clock Out Polarity.                                  */
#define  LPC2XXX_BIT_SSPCR0_CPHA                    DEF_BIT_07  /* Clock Out Phase.                                     */
#define  LPC2XXX_BIT_SSPCR0_SRC_MASK                0x0000FF00u /* Serial Clock Rate.                                   */


                                                                /* ---------------------- SSPCR1 ---------------------- */
#define  LPC2XXX_BIT_SSPCR1_LBM                     DEF_BIT_00  /* Loop Back Mode.                                      */
#define  LPC2XXX_BIT_SSPCR1_SSE                     DEF_BIT_01  /* SSP Enable.                                          */
#define  LPC2XXX_BIT_SSPCR1_MS                      DEF_BIT_02  /* Master/Slave Mode.                                   */
#define  LPC2XXX_BIT_SSPCR1_SOD                     DEF_BIT_03  /* Slave Output Disable.                                */


                                                                /* ----------------------- SSPSR ---------------------- */
#define  LPC2XXX_BIT_SSPSR_TFE                      DEF_BIT_00  /* Transmit FIFO Empty.                                 */
#define  LPC2XXX_BIT_SSPSR_TNF                      DEF_BIT_01  /* Transmit FIFO Not Full.                              */
#define  LPC2XXX_BIT_SSPSR_RNE                      DEF_BIT_02  /* Receive FIFO Not Empty.                              */
#define  LPC2XXX_BIT_SSPSR_RFF                      DEF_BIT_03  /* Recieve FIFO Full.                                   */
#define  LPC2XXX_BIT_SSPSR_BSY                      DEF_BIT_04  /* Busy.                                                */


                                                                /* ---------------------- SSPIMSC --------------------- */
                                                                /* ---------------------- SSPRIS  --------------------- */
                                                                /* ---------------------- SSPMIS  --------------------- */
                                                                /* ---------------------- SSPICR  --------------------- */
#define  LPC2XXX_BIT_SSPINT_ROR                     DEF_BIT_00  /* Receive Overrun.                                     */
#define  LPC2XXX_BIT_SSPINT_RT                      DEF_BIT_01  /* Receive Timeout.                                     */
#define  LPC2XXX_BIT_SSPINT_RX                      DEF_BIT_02  /* Receive FIFO Half Full.                              */
#define  LPC2XXX_BIT_SSPINT_TX                      DEF_BIT_03  /* Transmit FIFO Half Empty.                            */


                                                                /* --------------------- SSPDMACR --------------------- */
#define  LPC2XXX_BIT_DMACR_RXDMAE                   DEF_BIT_00  /* Receive DMA Enable.                                  */
#define  LPC2XXX_BIT_DMACR_TXDMAE                   DEF_BIT_01  /* Transmit DMA Enable.                                 */


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                        /* --------------- SPI API FNCTS -------------- */
static  CPU_BOOLEAN  FSDev_BSP_SPI_Open      (FS_QTY       unit_nbr);   /* Open (initialize) SPI.                       */

static  void         FSDev_BSP_SPI_Close     (FS_QTY       unit_nbr);   /* Close (uninitialize) SPI.                    */

static  void         FSDev_BSP_SPI_Lock      (FS_QTY       unit_nbr);   /* Acquire SPI lock.                            */

static  void         FSDev_BSP_SPI_Unlock    (FS_QTY       unit_nbr);   /* Release SPI lock.                            */

static  void         FSDev_BSP_SPI_Rd        (FS_QTY       unit_nbr,    /* Rd from SPI.                                 */
                                              void        *p_dest,
                                              CPU_SIZE_T   cnt);

static  void         FSDev_BSP_SPI_Wr        (FS_QTY       unit_nbr,    /* Wr to SPI.                                   */
                                              void        *p_src,
                                              CPU_SIZE_T   cnt);

static  void         FSDev_BSP_SPI_ChipSelEn (FS_QTY       unit_nbr);   /* En SD/MMC chip sel.                          */

static  void         FSDev_BSP_SPI_ChipSelDis(FS_QTY       unit_nbr);   /* Dis SD/MMC chip sel.                         */

static  void         FSDev_BSP_SPI_SetClkFreq(FS_QTY       unit_nbr,    /* Set SPI clk freq.                            */
                                              CPU_INT32U   freq);


/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_SPI_API  FSDev_SD_SPI_BSP_SPI = {
    FSDev_BSP_SPI_Open,
    FSDev_BSP_SPI_Close,
    FSDev_BSP_SPI_Lock,
    FSDev_BSP_SPI_Unlock,
    FSDev_BSP_SPI_Rd,
    FSDev_BSP_SPI_Wr,
    FSDev_BSP_SPI_ChipSelEn,
    FSDev_BSP_SPI_ChipSelDis,
    FSDev_BSP_SPI_SetClkFreq
};


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                    FILE SYSTEM SD SPI FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        FSDev_BSP_SPI_Open()
*
* Description : Open (initialize) SPI for SD/MMC.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : DEF_OK,   if interface was opened.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FSDev_SD_SPI_Refresh().
*
* Note(s)     : (1) This function will be called EVERY time the device is opened.
*
*               (2) A SD/MMC slot is connected to the following pins :
*
*                   -----------------------------------------------
*                   |  LPC2148 NAME | LPC2148 PIO # | SD/MMC NAME |
*                   -----------------------------------------------
*                   |    SSEL1      |     P0[20]    | CS  (pin 1) |
*                   |    MOSI1      |     P0[19]    | DI  (pin 2) |
*                   |    SCK1       |     P0[17]    | CLK (pin 5) |
*                   |    MISO1      |     P0[18]    | D0  (pin 7) |
*                   -----------------------------------------------
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FSDev_BSP_SPI_Open (FS_QTY  unit_nbr)
{
    CPU_INT32U  pinsel;
    CPU_SR_ALLOC();


    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_BSP_SPI_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }


                                                                /* ---------------------- CFG HW ---------------------- */
    CPU_CRITICAL_ENTER();
    pinsel                = LPC2XXX_REG_PINSEL1;                /* Cfg P0[17..20] as SCK, MISO, MOSI, GPIO (SSEL).      */
    pinsel               &= 0xFFFFFC03u;
    pinsel               |= 0x000000A8u;
    LPC2XXX_REG_PINSEL1   = pinsel;

    LPC2XXX_REG_FIO0DIR  |=  DEF_BIT_20;                        /* Cfg P0[20] as output.                                */
    LPC2XXX_REG_FIO0MASK &= ~DEF_BIT_20;
    LPC2XXX_REG_FIO0SET   =  DEF_BIT_20;                        /* Desel card.                                          */
    CPU_CRITICAL_EXIT();


                                                                /* ---------------------- CFG SPI --------------------- */
    LPC2XXX_REG_PCONP    |= DEF_BIT_10;                         /* En peripheral pwr.                                   */
    LPC2XXX_REG_SSPCR0    = 0x00000107u;                        /* 8-bit data, presaler div = 1.                        */
    LPC2XXX_REG_SSPCPSR   = 2u;
    LPC2XXX_REG_SSPCR1    = LPC2XXX_BIT_SSPCR1_SSE;             /* En SSP.                                              */

    FSDev_BSP_SPI_SetClkFreq(unit_nbr, FS_DEV_SD_DFLT_CLK_SPD);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        FSDev_BSP_SPI_Close()
*
* Description : Close (uninitialize) SPI for SD/MMC.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_SPI_Close().
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_Close (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    LPC2XXX_REG_PCONP &= ~DEF_BIT_10;                           /* Dis peripheral pwr.                                  */
}


/*
*********************************************************************************************************
*                                        FSDev_BSP_SPI_Lock()
*
* Description : Acquire SPI lock.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) This function will be called before the SD/MMC driver begins to access the SPI.  The
*                   application should NOT use the same bus to access another device until the matching
*                   call to 'FSDev_BSP_SPI_Unlock()' has been made.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_Lock (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                       FSDev_BSP_SPI_Unlock()
*
* Description : Release SPI lock.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) 'FSDev_BSP_SPI_Lock()' will be called before the SD/MMC driver begins to access the SPI.
*                   The application should NOT use the same bus to access another device until the
*                   matching call to this function has been made.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_Unlock (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                         FSDev_BSP_SPI_Rd()
*
* Description : Read from SPI.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               p_dest      Pointer to destination memory buffer.
*
*               cnt         Number of octets to read.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_Rd (FS_QTY       unit_nbr,
                                void        *p_dest,
                                CPU_SIZE_T   cnt)
{
    CPU_INT08U   *p_dest_08;
    CPU_BOOLEAN   rxd;
    CPU_INT08U    datum;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
        LPC2XXX_REG_SSPDR = 0xFFu;                              /* Tx dummy byte.                                       */

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(LPC2XXX_REG_SSPSR, LPC2XXX_BIT_SSPSR_RNE);
        } while (rxd == DEF_NO);

        datum     = LPC2XXX_REG_SSPDR;                          /* Rd rx'd byte.                                        */
       *p_dest_08 = datum;
        p_dest_08++;
        cnt--;
    }
}


/*
*********************************************************************************************************
*                                         FSDev_BSP_SPI_Wr()
*
* Description : Write to SPI.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               p_src       Pointer to source memory buffer.
*
*               cnt         Number of octets to write.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_Wr (FS_QTY       unit_nbr,
                                void        *p_src,
                                CPU_SIZE_T   cnt)
{
    CPU_INT08U   *p_src_08;
    CPU_BOOLEAN   rxd;
    CPU_INT08U    datum;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    p_src_08 = (CPU_INT08U *)p_src;
    while (cnt > 0u) {
        datum             = *p_src_08;
        LPC2XXX_REG_SSPDR =  datum;                             /* Tx byte.                                             */
        p_src_08++;

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(LPC2XXX_REG_SSPSR, LPC2XXX_BIT_SSPSR_RNE);
        } while (rxd == DEF_NO);

        datum = LPC2XXX_REG_SSPDR;                              /* Rd rx'd dummy byte.                                  */
       (void)&datum;

        cnt--;
    }
}


/*
*********************************************************************************************************
*                                      FSDev_BSP_SPI_ChipSelEn()
*
* Description : Enable SD/MMC chip select.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) The chip select is typically 'active low'; to enable the card, the chip select pin
*                   should be cleared.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_ChipSelEn (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    LPC2XXX_REG_FIO0CLR = DEF_BIT_20;
}


/*
*********************************************************************************************************
*                                     FSDev_BSP_SPI_ChipSelDis()
*
* Description : Disable SD/MMC chip select.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) The chip select is typically 'active low'; to disable the card, the chip select pin
*                   should be set.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_ChipSelDis (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    LPC2XXX_REG_FIO0SET = DEF_BIT_20;
}


/*
*********************************************************************************************************
*                                     FSDev_BSP_SPI_SetClkFreq()
*
* Description : Set SPI clock frequency.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               freq        Clock frequency, in Hz.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_SPI_Open().
*
* Note(s)     : (1) The effective clock frequency MUST be no more than 'freq'.  If the frequency cannot be
*                   configured equal to 'freq', it should be configured less than 'freq'.
*********************************************************************************************************
*/

static  void  FSDev_BSP_SPI_SetClkFreq (FS_QTY      unit_nbr,
                                        CPU_INT32U  freq)
{
    CPU_INT32U  clk_div;
    CPU_INT32U  clk_freq;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    clk_freq            =  LPC214X_FREQ_PCLK_HZ;
    clk_div             = (clk_freq + freq - 1u) / freq;
    if (clk_div < 2u) {                                         /* The div must be at least 2.                          */
        clk_div = 2u;
    }
    if ((clk_div % 2u) != 0u) {                                 /* The div must be even.                                */
        clk_div++;
    }
    if (clk_div > 255u) {                                       /* The div is an 8-bit value.                           */
        clk_div = 254u;
    }

    LPC2XXX_REG_SSPCPSR =  clk_div;
}
