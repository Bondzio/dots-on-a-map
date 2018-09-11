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
*                                       LUMINARY MICRO LM3Sxxxx
*
* Filename      : fs_dev_sd_spi_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) (a) This ports maps SSI0 to device "sd:0:" & SSI1 to "sd:1:".
*
*                     (b) LM3S_FSYS_CLK_FREQ_MHZ should be set to the FSYS frequency.
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

#define  LM3S_FSYS_CLK_FREQ_MHZ                       50000000uL

#define  LM3S_PIN_SSI0CLK                           DEF_BIT_02  /* PA2                                                  */
#define  LM3S_PIN_SSI0FSS                           DEF_BIT_03  /* PA3                                                  */
#define  LM3S_PIN_SSI0RX                            DEF_BIT_04  /* PA4                                                  */
#define  LM3S_PIN_SSI0TX                            DEF_BIT_05  /* PA5                                                  */

#define  LM3S_PIN_SSI1CLK                           DEF_BIT_00  /* PE0                                                  */
#define  LM3S_PIN_SSI1FSS                           DEF_BIT_01  /* PE1                                                  */
#define  LM3S_PIN_SSI1RX                            DEF_BIT_02  /* PE2                                                  */
#define  LM3S_PIN_SSI1TX                            DEF_BIT_03  /* PE3                                                  */

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  LM3S_REG_SSI0_BASE                        0x40008000uL
#define  LM3S_REG_SSI1_BASE                        0x40009000uL

#define  LM3S_REG_RCGC1            (*(CPU_REG32 *)(0x400FE104uL))
#define  LM3S_REG_RCGC2            (*(CPU_REG32 *)(0x400FE108uL))

#define  LM3S_REG_GPIOA_BASE                       0x40004000uL
#define  LM3S_REG_GPIOA_DATA_ADDR  ( (CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x000u))
#define  LM3S_REG_GPIOA_DIR        (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x400u))
#define  LM3S_REG_GPIOA_AFSEL      (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x420u))
#define  LM3S_REG_GPIOA_ODR        (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x50Cu))
#define  LM3S_REG_GPIOA_PUR        (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x510u))
#define  LM3S_REG_GPIOA_PDR        (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x514u))
#define  LM3S_REG_GPIOA_DEN        (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x51Cu))
#define  LM3S_REG_GPIOA_LOCK       (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x520u))
#define  LM3S_REG_GPIOA_CR         (*(CPU_REG32 *)(LM3S_REG_GPIOA_BASE + 0x524u))

#define  LM3S_REG_GPIOE_BASE                       0x40024000uL
#define  LM3S_REG_GPIOE_DATA_ADDR  ( (CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x000u))
#define  LM3S_REG_GPIOE_DIR        (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x400u))
#define  LM3S_REG_GPIOE_AFSEL      (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x420u))
#define  LM3S_REG_GPIOE_ODR        (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x50Cu))
#define  LM3S_REG_GPIOE_PUR        (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x510u))
#define  LM3S_REG_GPIOE_PDR        (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x514u))
#define  LM3S_REG_GPIOE_DEN        (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x51Cu))
#define  LM3S_REG_GPIOE_LOCK       (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x520u))
#define  LM3S_REG_GPIOE_CR         (*(CPU_REG32 *)(LM3S_REG_GPIOE_BASE + 0x524u))

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

                                                                /* ------------------- SSI CR0 BITS ------------------- */
#define  LM3S_BIT_SSICR0_SCR_MASK                   0xFF00u     /* SSI Serial Clock Rate.                               */
#define  LM3S_BIT_SSICR0_SPH                    DEF_BIT_07      /* SSI Serial Clock Phase.                              */
#define  LM3S_BIT_SSICR0_SPO                    DEF_BIT_06      /* SSI Serial Clock Polarity.                           */
#define  LM3S_BIT_SSICR0_FRF_MASK                   0x0030u     /* SSI Frame Format Select.                             */
#define  LM3S_BIT_SSICR0_DSS_MASK                   0x000Fu     /* SSI Data Size Select.                                */
#define  LM3S_BIT_SSICR0_DSS_04BIT                  0x0003u
#define  LM3S_BIT_SSICR0_DSS_05BIT                  0x0004u
#define  LM3S_BIT_SSICR0_DSS_06BIT                  0x0005u
#define  LM3S_BIT_SSICR0_DSS_07BIT                  0x0006u
#define  LM3S_BIT_SSICR0_DSS_08BIT                  0x0007u
#define  LM3S_BIT_SSICR0_DSS_09BIT                  0x0008u
#define  LM3S_BIT_SSICR0_DSS_10BIT                  0x0009u
#define  LM3S_BIT_SSICR0_DSS_11BIT                  0x000Au
#define  LM3S_BIT_SSICR0_DSS_12BIT                  0x000Bu
#define  LM3S_BIT_SSICR0_DSS_13BIT                  0x000Cu
#define  LM3S_BIT_SSICR0_DSS_14BIT                  0x000Du
#define  LM3S_BIT_SSICR0_DSS_15BIT                  0x000Eu
#define  LM3S_BIT_SSICR0_DSS_16BIT                  0x000Fu


                                                                /* ------------------- SSI CR1 BITS ------------------- */
#define  LM3S_BIT_SSICR1_SOD                    DEF_BIT_03      /* SSI Slave Mode Output Disable.                       */
#define  LM3S_BIT_SSICR1_MS                     DEF_BIT_02      /* SSI Master/Slave Select.                             */
#define  LM3S_BIT_SSICR1_SSE                    DEF_BIT_01      /* SSI Synchronous Serial Port Enable.                  */
#define  LM3S_BIT_SSICR1_LBM                    DEF_BIT_00      /* SSI Loopback Mode.                                   */


                                                                /* -------------------- SSI SR BITS ------------------- */
#define  LM3S_BIT_SSISR_BSY                     DEF_BIT_04      /* SSI Busy Bit.                                        */
#define  LM3S_BIT_SSISR_RFF                     DEF_BIT_03      /* SSI Receive FIFO Full.                               */
#define  LM3S_BIT_SSISR_RNE                     DEF_BIT_02      /* SSI Receive FIFO Not Empty.                          */
#define  LM3S_BIT_SSISR_TNF                     DEF_BIT_01      /* SSI Transmit FIFO Not Full.                          */
#define  LM3S_BIT_SSISR_TFE                     DEF_BIT_00      /* SSI Transmit FIFO Empty.                             */


                                                                /* -------------------- RCGC1 BITS -------------------- */
#define  LM3S_BIT_RCGC1_SSI0                    DEF_BIT_04
#define  LM3S_BIT_RCGC1_SSI1                    DEF_BIT_05

                                                                /* -------------------- RCGC1 BITS -------------------- */
#define  LM3S_BIT_RCGC2_PORTA                   DEF_BIT_00
#define  LM3S_BIT_RCGC2_PORTE                   DEF_BIT_04


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

typedef  struct  lm3s_struct_ssi {
    CPU_REG32  SSICR0;
    CPU_REG32  SSICR1;
    CPU_REG32  SSIDR;
    CPU_REG32  SSISR;
    CPU_REG32  SSICPSR;
    CPU_REG32  SSIIM;
    CPU_REG32  SSIRIS;
    CPU_REG32  SSIMIS;
    CPU_REG32  SSICR;
} LM3S_STRUCT_SSI;


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
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FSDev_BSP_SPI_Open (FS_QTY  unit_nbr)
{
    LM3S_STRUCT_SSI  *p_ssi;


    if ((unit_nbr != 0u) &&
        (unit_nbr != 1u)) {
        FS_TRACE_INFO(("FSDev_BSP_SPI_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

                                                                /* --------------------- CFG GPIO --------------------- */
    if (unit_nbr == 0u) {
        LM3S_REG_RCGC1       |=  LM3S_BIT_RCGC1_SSI0;
        LM3S_REG_RCGC2       |=  LM3S_BIT_RCGC2_PORTA;
        LM3S_REG_GPIOA_DEN   |=  LM3S_PIN_SSI0CLK | LM3S_PIN_SSI0RX | LM3S_PIN_SSI0TX | LM3S_PIN_SSI0FSS;

        LM3S_REG_GPIOA_LOCK   =  0x1ACCE551u;
        LM3S_REG_GPIOA_CR     =  LM3S_PIN_SSI0CLK | LM3S_PIN_SSI0RX | LM3S_PIN_SSI0TX;
        LM3S_REG_GPIOA_AFSEL |=  LM3S_PIN_SSI0CLK | LM3S_PIN_SSI0RX | LM3S_PIN_SSI0TX;

        LM3S_REG_GPIOA_LOCK   =  0x1ACCE551u;
        LM3S_REG_GPIOA_CR     =  LM3S_PIN_SSI0FSS;
        LM3S_REG_GPIOA_AFSEL &= ~LM3S_PIN_SSI0FSS;
        LM3S_REG_GPIOA_DIR   |=  LM3S_PIN_SSI0FSS;
        FSDev_BSP_SPI_ChipSelDis(unit_nbr);
    } else {
        LM3S_REG_RCGC1       |=  LM3S_BIT_RCGC1_SSI1;
        LM3S_REG_RCGC2       |=  LM3S_BIT_RCGC2_PORTE;
        LM3S_REG_GPIOE_DEN   |=  LM3S_PIN_SSI1CLK | LM3S_PIN_SSI1RX | LM3S_PIN_SSI1TX | LM3S_PIN_SSI1FSS;

        LM3S_REG_GPIOE_LOCK   =  0x1ACCE551u;
        LM3S_REG_GPIOE_CR     =  LM3S_PIN_SSI1CLK | LM3S_PIN_SSI1RX | LM3S_PIN_SSI1TX;
        LM3S_REG_GPIOE_AFSEL |=  LM3S_PIN_SSI1CLK | LM3S_PIN_SSI1RX | LM3S_PIN_SSI1TX;

        LM3S_REG_GPIOE_LOCK   =  0x1ACCE551u;
        LM3S_REG_GPIOE_CR     =  LM3S_PIN_SSI1FSS;
        LM3S_REG_GPIOE_AFSEL &= ~LM3S_PIN_SSI1FSS;
        LM3S_REG_GPIOE_DIR   |=  LM3S_PIN_SSI1FSS;
        FSDev_BSP_SPI_ChipSelDis(unit_nbr);
    }


                                                                /* ---------------------- INIT SPI -------------------- */
    if (unit_nbr == 0u) {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI0_BASE;
    } else {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI1_BASE;
    }

    p_ssi->SSICR1 = 0u;                                         /* En SSP.                                              */
    p_ssi->SSICR0 = (1u << 8)                                   /* 8-bit data, presaler div = 1.                        */
                  | LM3S_BIT_SSICR0_DSS_08BIT;
    p_ssi->SSICR1 = LM3S_BIT_SSICR1_SSE;                        /* En SSP.                                              */

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
    LM3S_STRUCT_SSI  *p_ssi;
    CPU_INT08U       *p_dest_08;
    CPU_BOOLEAN       rxd;
    CPU_INT08U        datum;


    if (unit_nbr == 0u) {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI0_BASE;
    } else {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI1_BASE;
    }

    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
        p_ssi->SSIDR = 0xFFu;                                   /* Tx dummy byte.                                       */

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(p_ssi->SSISR, LM3S_BIT_SSISR_RNE);
        } while (rxd == DEF_NO);

        datum     = p_ssi->SSIDR;                               /* Rd rx'd byte.                                        */
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
    LM3S_STRUCT_SSI  *p_ssi;
    CPU_INT08U       *p_src_08;
    CPU_BOOLEAN       rxd;
    CPU_INT08U        datum;


    if (unit_nbr == 0u) {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI0_BASE;
    } else {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI1_BASE;
    }

    p_src_08 = (CPU_INT08U *)p_src;
    while (cnt > 0u) {
        datum        = *p_src_08;
        p_ssi->SSIDR =  datum;                                  /* Tx byte.                                             */
        p_src_08++;

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(p_ssi->SSISR, LM3S_BIT_SSISR_RNE);
        } while (rxd == DEF_NO);

        datum = p_ssi->SSIDR;                                   /* Rd rx'd dummy byte.                                  */
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
    if (unit_nbr == 0u) {
      *(LM3S_REG_GPIOA_DATA_ADDR + LM3S_PIN_SSI0FSS) = 0u;
    } else {
      *(LM3S_REG_GPIOE_DATA_ADDR + LM3S_PIN_SSI1FSS) = 0u;
    }
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
    if (unit_nbr == 0u) {
      *(LM3S_REG_GPIOA_DATA_ADDR + LM3S_PIN_SSI0FSS) = LM3S_PIN_SSI0FSS;
    } else {
      *(LM3S_REG_GPIOE_DATA_ADDR + LM3S_PIN_SSI1FSS) = LM3S_PIN_SSI1FSS;
    }
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
    LM3S_STRUCT_SSI  *p_ssi;
    CPU_INT32U        clk_div;
    CPU_INT32U        clk_freq;


    if (unit_nbr == 0u) {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI0_BASE;
    } else {
        p_ssi = (LM3S_STRUCT_SSI *)LM3S_REG_SSI1_BASE;
    }

    clk_freq       =  LM3S_FSYS_CLK_FREQ_MHZ;
    clk_div        = (clk_freq + freq - 1u) / freq;
    if (clk_div < 2u) {                                         /* The div must be at least 2.                          */
        clk_div = 2u;
    }
    if ((clk_div % 2u) != 0u) {                                 /* The div must be even.                                */
        clk_div++;
    }
    if (clk_div > 255u) {                                       /* The div is an 8-bit value.                           */
        clk_div = 254u;
    }

    p_ssi->SSICPSR =  clk_div;
}
