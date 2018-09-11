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
*                          ST STR912 ON THE IAR STR912-SK EVALUATION BOARD
*
* Filename      : fs_dev_sd_spi_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The STR912 SSP peripheral is described in documentation from ST :
*
*                         UM0216 Reference Manual.  STR91xF ARM9-based microcontroller family.  Rev 1,
*                         April 2006.  "10 Synchronous serial peripheral (SSP)".
*
*                 (2) STR912_PCLK_FREQ_MHZ must be set to the PCLK frequency used for SSP0.
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

#define  STR912_PCLK_FREQ_MHZ                         36000000uL

#define  STR912_PIN_SSP0_NSS                         DEF_BIT_07 /* P5[7]                                                */
#define  STR912_PIN_SSP0_SCLK                        DEF_BIT_04 /* P5[4]                                                */
#define  STR912_PIN_SSP0_MISO                        DEF_BIT_06 /* P5[6]                                                */
#define  STR912_PIN_SSP0_MOSI                        DEF_BIT_05 /* P5[5]                                                */

#define  STR912_PIN_SD_WP                            DEF_BIT_00 /* P9[0]                                                */
#define  STR912_PIN_SD_CD                            DEF_BIT_01 /* P9[1]                                                */
#define  STR912_PIN_SD_CS                            DEF_BIT_03 /* P9[3]                                                */

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  STR912_REG_SCU_BASE                        0x5C002000u
#define  STR912_REG_SCU_PCGR1       (*(CPU_REG32 *)(STR912_REG_SCU_BASE   + 0x018u))
#define  STR912_REG_SCU_PRR1        (*(CPU_REG32 *)(STR912_REG_SCU_BASE   + 0x020u))
#define  STR912_REG_SCU_GPIOOUT5    (*(CPU_REG32 *)(STR912_REG_SCU_BASE   + 0x058u))
#define  STR912_REG_SCU_GPIOIN5     (*(CPU_REG32 *)(STR912_REG_SCU_BASE   + 0x078u))
#define  STR912_REG_SCU_GPIOTYPE5   (*(CPU_REG32 *)(STR912_REG_SCU_BASE   + 0x098u))
#define  STR912_REG_SCU_GPIOEMI     (*(CPU_REG32 *)(STR912_REG_SCU_BASE   + 0x0ACu))

#define  STR912_REG_GPIO5_BASE                      0x5800B000u
#define  STR912_REG_GPIO5_DATA_ADDR ( (CPU_REG32 *)(STR912_REG_GPIO5_BASE + 0x000u))
#define  STR912_REG_GPIO5_DIR       (*(CPU_REG32 *)(STR912_REG_GPIO5_BASE + 0x400u))
#define  STR912_REG_GPIO5_SEL       (*(CPU_REG32 *)(STR912_REG_GPIO5_BASE + 0x420u))

#define  STR912_REG_GPIO9_BASE                      0x5800F000u
#define  STR912_REG_GPIO9_DATA_ADDR ( (CPU_REG32 *)(STR912_REG_GPIO9_BASE + 0x000u))
#define  STR912_REG_GPIO9_DIR       (*(CPU_REG32 *)(STR912_REG_GPIO9_BASE + 0x400u))
#define  STR912_REG_GPIO9_SEL       (*(CPU_REG32 *)(STR912_REG_GPIO9_BASE + 0x420u))

#define  STR912_REG_SSP0_BASE                       0x5C007000u
#define  STR912_REG_SSP0_CR0        (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x000u))
#define  STR912_REG_SSP0_CR1        (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x004u))
#define  STR912_REG_SSP0_DR         (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x008u))
#define  STR912_REG_SSP0_SR         (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x00Cu))
#define  STR912_REG_SSP0_PR         (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x010u))
#define  STR912_REG_SSP0_IMSCR      (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x014u))
#define  STR912_REG_SSP0_RISR       (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x018u))
#define  STR912_REG_SSP0_MISR       (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x01Cu))
#define  STR912_REG_SSP0_ICR        (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x020u))
#define  STR912_REG_SSP0_DMACR      (*(CPU_REG16 *)(STR912_REG_SSP0_BASE  + 0x024u))

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  STR912_BIT_SCU_PCGR1_SSP0              DEF_BIT_08
#define  STR912_BIT_SCU_PCGR1_GPIO5             DEF_BIT_23

#define  STR912_BIT_SCU_PRR1_SSP0               DEF_BIT_08
#define  STR912_BIT_SCU_PRR1_GPIO5              DEF_BIT_23

                                                                /* ------------------- SSI CR0 BITS ------------------- */
#define  STR912_BIT_SSP_CR0_SCR_MASK                0xFF00u     /* SSI Serial Clock Rate.                               */
#define  STR912_BIT_SSP_CR0_SPH                 DEF_BIT_07      /* SSI Serial Clock Phase.                              */
#define  STR912_BIT_SSP_CR0_SPO                 DEF_BIT_06      /* SSI Serial Clock Polarity.                           */
#define  STR912_BIT_SSP_CR0_FRF_MASK                0x0030u     /* SSI Frame Format Select.                             */
#define  STR912_BIT_SSP_CR0_DSS_MASK                0x000Fu     /* SSI Data Size Select.                                */
#define  STR912_BIT_SSP_CR0_DSS_04BIT               0x0003u
#define  STR912_BIT_SSP_CR0_DSS_05BIT               0x0004u
#define  STR912_BIT_SSP_CR0_DSS_06BIT               0x0005u
#define  STR912_BIT_SSP_CR0_DSS_07BIT               0x0006u
#define  STR912_BIT_SSP_CR0_DSS_08BIT               0x0007u
#define  STR912_BIT_SSP_CR0_DSS_09BIT               0x0008u
#define  STR912_BIT_SSP_CR0_DSS_10BIT               0x0009u
#define  STR912_BIT_SSP_CR0_DSS_11BIT               0x000Au
#define  STR912_BIT_SSP_CR0_DSS_12BIT               0x000Bu
#define  STR912_BIT_SSP_CR0_DSS_13BIT               0x000Cu
#define  STR912_BIT_SSP_CR0_DSS_14BIT               0x000Du
#define  STR912_BIT_SSP_CR0_DSS_15BIT               0x000Eu
#define  STR912_BIT_SSP_CR0_DSS_16BIT               0x000Fu

                                                                /* ------------------- SSI CR1 BITS ------------------- */
#define  STR912_BIT_SSP_CR1_SOD                 DEF_BIT_03      /* SSI Slave Mode Output Disable.                       */
#define  STR912_BIT_SSP_CR1_MS                  DEF_BIT_02      /* SSI Master/Slave Select.                             */
#define  STR912_BIT_SSP_CR1_SSE                 DEF_BIT_01      /* SSI Synchronous Serial Port Enable.                  */
#define  STR912_BIT_SSP_CR1_LBM                 DEF_BIT_00      /* SSI Loopback Mode.                                   */

                                                                /* -------------------- SSI SR BITS ------------------- */
#define  STR912_BIT_SSP_SR_BSY                  DEF_BIT_04      /* SSI Busy Bit.                                        */
#define  STR912_BIT_SSP_SR_RFF                  DEF_BIT_03      /* SSI Receive FIFO Full.                               */
#define  STR912_BIT_SSP_SR_RNE                  DEF_BIT_02      /* SSI Receive FIFO Not Empty.                          */
#define  STR912_BIT_SSP_SR_TNF                  DEF_BIT_01      /* SSI Transmit FIFO Not Full.                          */
#define  STR912_BIT_SSP_SR_TFE                  DEF_BIT_00      /* SSI Transmit FIFO Empty.                             */


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
*               (2) Several aspects of SPI communication may need to be configured :
*
*                   (a) CLOCK POLARITY & PHASE (CPOL & CPHA).  SPI communication takes place in any of
*                       four modes, depending on the clock phase & clock polarity settings :
*
*                       (1) If CPOL = 0, the clock is low  when inactive.
*
*                           If CPOL = 1, the clock is high when inactive.
*
*                       (2) If CPHA = 0, data is 'read'    on the leading   edge of the clock &
*                                                'changed' on the following edge of the clock.
*
*                           If CPHA = 1, data is 'changed' on the following edge of the clock &
*                                                'read'    on the leading   edge of the clock.
*
*                       The driver should configure the SPI for CPOL = 0, CPHA = 0.  This means that
*                       data (i.e., bits) are read when a low->high clock transition occurs & changed
*                       when a high->low clock transition occurs.
*
*                   (b) TRANSFER UNIT LENGTH.  A transfer unit is always 8-bits.
*
*                   (c) SHIFT DIRECTION.  The most significant bit should always be transmitted first.
*
*                   (d) CLOCK FREQUENCY.  See 'FSDev_BSP_SPI_SetClkFreq()  Note #1'.
*
*               (3) The CS (Chip Select) MUST be configured as a GPIO output; it should not be controlled
*                   by the CPU's SPI peripheral.  The functions 'FSDev_BSP_SPI_ChipSelEn()' and
*                  'FSDev_BSP_SPI_ChipSelDis()' manually enable and disable the CS.
*
*               (4) A SD/MMC slot is connected to the following pins :
*
*                   -----------------------------------------------
*                   |  STR912 NAME  |  STR912 PIO # | SD/MMC NAME |
*                   -----------------------------------------------
*                   |    ---------  |     P9[3]     | CS  (pin 1) |
*                   |    SSP0_MOSI  |     P5[5]     | DI  (pin 2) |
*                   |    SSP0_SCLK  |     P5[4]     | CLK (pin 5) |
*                   |    SSP0_MISO  |     P5[6]     | D0  (pin 7) |
*                   -----------------------------------------------
*
*                   (a) In addition, SSP0_NSS (P5[7]) & P9[2] are connected to the chip select.  Both
*                       of those pins are configured as GPIO outuputs, but P9[3] is the only pin actually
*                       used for that purpose.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FSDev_BSP_SPI_Open (FS_QTY  unit_nbr)
{
    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_SD_SPI_BSP_SPI_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }


                                                                /* En periph clks.                                      */
    STR912_REG_SCU_PCGR1   |=  STR912_BIT_SCU_PCGR1_SSP0 | STR912_BIT_SCU_PCGR1_GPIO5;

    STR912_REG_SCU_PRR1    &= ~STR912_BIT_SCU_PRR1_SSP0;        /* Reset SSP.                                           */
    STR912_REG_SCU_PRR1    |=  STR912_BIT_SCU_PRR1_GPIO5;
    STR912_REG_SCU_PRR1    |=  STR912_BIT_SCU_PRR1_SSP0;



                                                                /* --------------------- CFG GPIO --------------------- */
    STR912_REG_GPIO5_DIR   |=  STR912_PIN_SSP0_NSS;             /* Cfg NSS as general purpose output.                   */
    STR912_REG_GPIO5_SEL   &= ~STR912_PIN_SSP0_NSS;

                                                                /* Cfg SCK, MOSO, MISO as alt fnct push-pull.           */
    STR912_REG_SCU_GPIOOUT5  &=   0x00FFu;
    STR912_REG_SCU_GPIOOUT5  |=   0x4A00u;
    STR912_REG_SCU_GPIOIN5   |=   STR912_PIN_SSP0_MISO;
    STR912_REG_SCU_GPIOTYPE5 &= ~(STR912_PIN_SSP0_SCLK | STR912_PIN_SSP0_MISO | STR912_PIN_SSP0_MOSI);

    STR912_REG_SCU_GPIOEMI   &= ~DEF_BIT_00;                    /* Cfg WP, CD as general purpose input.                 */
    STR912_REG_GPIO9_DIR     &= ~(STR912_PIN_SD_WP | STR912_PIN_SD_CD);
    STR912_REG_GPIO9_DIR     |=   STR912_PIN_SD_CS | DEF_BIT_02;
    STR912_REG_GPIO9_SEL     &= ~(STR912_PIN_SD_WP | STR912_PIN_SD_CD | STR912_PIN_SD_CS | DEF_BIT_02);

    FSDev_BSP_SPI_ChipSelDis(unit_nbr);



                                                                /* ---------------------- INIT SPI -------------------- */
    STR912_REG_SSP0_CR1 = 0u;                                   /* En SSP.                                              */
    STR912_REG_SSP0_CR0 = (1u << 8)                             /* 8-bit data, presaler div = 1.                        */
                        | STR912_BIT_SSP_CR0_DSS_08BIT;
    STR912_REG_SSP0_CR1 = STR912_BIT_SSP_CR1_SSE;               /* En SSP.                                              */

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
    CPU_INT08U   *p_dest_08;
    CPU_BOOLEAN   rxd;
    CPU_INT08U    datum;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
        STR912_REG_SSP0_DR = 0xFFu;                             /* Tx dummy byte.                                       */

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(STR912_REG_SSP0_SR, STR912_BIT_SSP_SR_RNE);
        } while (rxd == DEF_NO);

        datum     = STR912_REG_SSP0_DR;                         /* Rd rx'd byte.                                        */
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
        datum              = *p_src_08;
        STR912_REG_SSP0_DR =  datum;                            /* Tx byte.                                             */
        p_src_08++;

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(STR912_REG_SSP0_SR, STR912_BIT_SSP_SR_RNE);
        } while (rxd == DEF_NO);

        datum = STR912_REG_SSP0_DR;                             /* Rd rx'd dummy byte.                                  */
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

   *(STR912_REG_GPIO9_DATA_ADDR + STR912_PIN_SD_CS) = 0u;
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

   *(STR912_REG_GPIO9_DATA_ADDR + STR912_PIN_SD_CS) = STR912_PIN_SD_CS;
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


    clk_freq           =  STR912_PCLK_FREQ_MHZ;
    clk_div            = (clk_freq + freq - 1u) / freq;
    if (clk_div < 2u) {                                         /* The div must be at least 2.                          */
        clk_div = 2u;
    }
    if ((clk_div % 2u) != 0u) {                                 /* The div must be even.                                */
        clk_div++;
    }
    if (clk_div > 255u) {                                       /* The div is an 8-bit value.                           */
        clk_div = 254u;
    }

    STR912_REG_SSP0_PR =  clk_div;
}
