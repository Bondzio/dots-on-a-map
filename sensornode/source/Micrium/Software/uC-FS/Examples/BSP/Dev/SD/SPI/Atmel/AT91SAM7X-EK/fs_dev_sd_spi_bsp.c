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
*                       ATMEL AT91SAM7X ON THE ATMEL AT91SAM7X-EK EVALUATION BOARD
*
* Filename      : fs_dev_sd_spi_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The ATM91SAM7X SPI peripheral is described in documentation from NXP :
*
*                         AT91SAM7X512, AT91SAM7X256, AT91SAM7X128 Preliminary.  6120F-ATARM-03-Oct-06.
*                         "28. Serial Peripheral Interface (SPI)".
*
*                (2) AT91SAM7X_FREQ_MCK must be set to the MCK frequency.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_SD_SPI_BSP_MODULE
#include  <fs_dev_sd_spi.h>
#include  <fs.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  AT91SAM7X_FREQ_MCK                       48000000u

#define  AT91SAM7X_PIN_SPI0_NPCS1               DEF_BIT_13      /* PA13                                                 */
#define  AT91SAM7X_PIN_SPI0_MISO                DEF_BIT_16      /* PA16                                                 */
#define  AT91SAM7X_PIN_SPI0_MOSI                DEF_BIT_17      /* PA17                                                 */
#define  AT91SAM7X_PIN_SPI0_SPCK                DEF_BIT_18      /* PA18                                                 */

#define  AT91SAM7X_PERIPH_ID_PIOA               DEF_BIT_02
#define  AT91SAM7X_PERIPH_ID_SPI0               DEF_BIT_04

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  AT91SAM7X_REG_PIOA_BASE                               0xFFFFF400uL
#define  AT91SAM7X_REG_PIOA_PER                (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x000u))
#define  AT91SAM7X_REG_PIOA_PDR                (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x004u))
#define  AT91SAM7X_REG_PIOA_OER                (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x010u))
#define  AT91SAM7X_REG_PIOA_ODR                (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x014u))
#define  AT91SAM7X_REG_PIOA_SODR               (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x030u))
#define  AT91SAM7X_REG_PIOA_CODR               (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x034u))
#define  AT91SAM7X_REG_PIOA_PDSR               (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x03Cu))
#define  AT91SAM7X_REG_PIOA_MDDR               (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x054u))
#define  AT91SAM7X_REG_PIOA_PPUDR              (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x060u))
#define  AT91SAM7X_REG_PIOA_ASR                (*(CPU_REG32 *)(AT91SAM7X_REG_PIOA_BASE + 0x070u))

#define  AT91SAM7X_REG_PMC_BASE                               0xFFFFFC00uL
#define  AT91SAM7X_REG_PMC_PCER                (*(CPU_REG32 *)(AT91SAM7X_REG_PMC_BASE  + 0x010u))
#define  AT91SAM7X_REG_PMC_PCDR                (*(CPU_REG32 *)(AT91SAM7X_REG_PMC_BASE  + 0x014u))

#define  AT91SAM7X_REG_SPI0_BASE                               0xFFFE0000uL
#define  AT91SAM7X_REG_SPI0_CR                 (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x000u))
#define  AT91SAM7X_REG_SPI0_MR                 (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x004u))
#define  AT91SAM7X_REG_SPI0_RDR                (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x008u))
#define  AT91SAM7X_REG_SPI0_TDR                (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x00Cu))
#define  AT91SAM7X_REG_SPI0_SR                 (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x010u))
#define  AT91SAM7X_REG_SPI0_IER                (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x014u))
#define  AT91SAM7X_REG_SPI0_IDR                (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x018u))
#define  AT91SAM7X_REG_SPI0_IMR                (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x01Cu))
#define  AT91SAM7X_REG_SPI0_CSR0               (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x030u))
#define  AT91SAM7X_REG_SPI0_CSR1               (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x034u))
#define  AT91SAM7X_REG_SPI0_CSR2               (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x038u))
#define  AT91SAM7X_REG_SPI0_CSR3               (*(CPU_REG32 *)(AT91SAM7X_REG_SPI0_BASE + 0x03Cu))

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  AT91SAM7X_BIT_SPI0_CR_SPIEN            DEF_BIT_00
#define  AT91SAM7X_BIT_SPI0_CR_SPIDIS           DEF_BIT_01
#define  AT91SAM7X_BIT_SPI0_CR_SWRST            DEF_BIT_07
#define  AT91SAM7X_BIT_SPI0_CR_LASTXFER         DEF_BIT_08

#define  AT91SAM7X_BIT_SPI0_MR_MSTR             DEF_BIT_00
#define  AT91SAM7X_BIT_SPI0_MR_PS               DEF_BIT_01
#define  AT91SAM7X_BIT_SPI0_MR_PCSDEC           DEF_BIT_02
#define  AT91SAM7X_BIT_SPI0_MR_MODFDIS          DEF_BIT_04
#define  AT91SAM7X_BIT_SPI0_MR_LLB              DEF_BIT_07
#define  AT91SAM7X_BIT_SPI0_MR_PCS_MASK         0x000F0000u
#define  AT91SAM7X_BIT_SPI0_MR_PCS_NPCS0        0x000E0000u
#define  AT91SAM7X_BIT_SPI0_MR_PCS_NPCS1        0x000D0000u
#define  AT91SAM7X_BIT_SPI0_MR_PCS_NPCS2        0x000B0000u
#define  AT91SAM7X_BIT_SPI0_MR_PCS_NPCS3        0x00070000u
#define  AT91SAM7X_BIT_SPI0_MR_DLYBCS_MASK      0xFF000000u

#define  AT91SAM7X_BIT_SPI0_INT_RDRF            DEF_BIT_00
#define  AT91SAM7X_BIT_SPI0_INT_TDRE            DEF_BIT_01
#define  AT91SAM7X_BIT_SPI0_INT_MODF            DEF_BIT_02
#define  AT91SAM7X_BIT_SPI0_INT_OVRES           DEF_BIT_03
#define  AT91SAM7X_BIT_SPI0_INT_ENDRX           DEF_BIT_04
#define  AT91SAM7X_BIT_SPI0_INT_ENDTX           DEF_BIT_05
#define  AT91SAM7X_BIT_SPI0_INT_RXBUFF          DEF_BIT_06
#define  AT91SAM7X_BIT_SPI0_INT_TXBUFE          DEF_BIT_07
#define  AT91SAM7X_BIT_SPI0_INT_NSSR            DEF_BIT_08
#define  AT91SAM7X_BIT_SPI0_INT_TXEMPTY         DEF_BIT_09
#define  AT91SAM7X_BIT_SPI0_INT_ALL            (AT91SAM7X_BIT_SPI0_INT_RDRF   | AT91SAM7X_BIT_SPI0_INT_TDRE   | AT91SAM7X_BIT_SPI0_INT_MODF  | \
                                                AT91SAM7X_BIT_SPI0_INT_OVRES  | AT91SAM7X_BIT_SPI0_INT_ENDRX  | AT91SAM7X_BIT_SPI0_INT_ENDTX | \
                                                AT91SAM7X_BIT_SPI0_INT_RXBUFF | AT91SAM7X_BIT_SPI0_INT_TXBUFE | AT91SAM7X_BIT_SPI0_INT_NSSR  | \
                                                AT91SAM7X_BIT_SPI0_INT_TXEMPTY)

#define  AT91SAM7X_BIT_SPI0_SR_SPIENS           DEF_BIT_16

#define  AT91SAM7X_BIT_SPI0_CSR_CPOL            DEF_BIT_00
#define  AT91SAM7X_BIT_SPI0_CSR_NCPHA           DEF_BIT_01
#define  AT91SAM7X_BIT_SPI0_CSR_CSAAT           DEF_BIT_03
#define  AT91SAM7X_BIT_SPI0_CSR_BITS_08         DEF_BIT_NONE

#define  AT91SAM7X_BIT_SPI0_TDR_PCS_NPCS0       0x000E0000u
#define  AT91SAM7X_BIT_SPI0_TDR_PCS_NPCS1       0x000D0000u
#define  AT91SAM7X_BIT_SPI0_TDR_PCS_NPCS2       0x000B0000u
#define  AT91SAM7X_BIT_SPI0_TDR_PCS_NPCS3       0x00070000u

#define  AT91SAM7X_BIT_SPI0_RDR_PCS_NPCS0       0x000E0000u
#define  AT91SAM7X_BIT_SPI0_RDR_PCS_NPCS1       0x000D0000u
#define  AT91SAM7X_BIT_SPI0_RDR_PCS_NPCS2       0x000B0000u
#define  AT91SAM7X_BIT_SPI0_RDR_PCS_NPCS3       0x00070000u


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
*                   --------------------------------------------------
*                   | AT91SAM7X NAME | AT91SAM7X PIO # | SD/MMC NAME |
*                   --------------------------------------------------
*                   |   SPI0_NPCS1   |      PA[13]     | CS  (pin 1) |
*                   |   SPI0_MOSI    |      PA[17]     | DI  (pin 2) |
*                   |   SPI0_SPCK    |      PA[18]     | CLK (pin 5) |
*                   |   SPI0_MISO    |      PA[16]     | D0  (pin 7) |
*                   --------------------------------------------------
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FSDev_BSP_SPI_Open (FS_QTY  unit_nbr)
{
    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_BSP_SPI_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }


                                                                /* ---------------------- CFG HW ---------------------- */
    AT91SAM7X_REG_PMC_PCER |= AT91SAM7X_PERIPH_ID_PIOA | AT91SAM7X_PERIPH_ID_SPI0;

    AT91SAM7X_REG_PIOA_PER  = AT91SAM7X_PIN_SPI0_NPCS1;         /* Cfg PA13 as GPIO.                                    */
    AT91SAM7X_REG_PIOA_OER  = AT91SAM7X_PIN_SPI0_NPCS1;
    AT91SAM7X_REG_PIOA_SODR = AT91SAM7X_PIN_SPI0_NPCS1;

                                                                /* Cfg PA16, 17, 18 as MISO, MOSI, SPCK.                */
    AT91SAM7X_REG_PIOA_PDR  = AT91SAM7X_PIN_SPI0_MISO | AT91SAM7X_PIN_SPI0_MOSI | AT91SAM7X_PIN_SPI0_SPCK;
    AT91SAM7X_REG_PIOA_ASR  = AT91SAM7X_PIN_SPI0_MISO | AT91SAM7X_PIN_SPI0_MOSI | AT91SAM7X_PIN_SPI0_SPCK;



                                                                /* ---------------------- CFG SPI --------------------- */
    AT91SAM7X_REG_SPI0_CR   = AT91SAM7X_BIT_SPI0_CR_SWRST;      /* Reset SPI0.                                          */
    AT91SAM7X_REG_SPI0_IDR  = AT91SAM7X_BIT_SPI0_INT_ALL;       /* Dis all ints.                                        */
    AT91SAM7X_REG_SPI0_MR   = AT91SAM7X_BIT_SPI0_MR_MSTR
                            | AT91SAM7X_BIT_SPI0_MR_MODFDIS
                            | AT91SAM7X_BIT_SPI0_MR_PCS_NPCS1;
    AT91SAM7X_REG_SPI0_CSR1 = AT91SAM7X_BIT_SPI0_CSR_NCPHA
                            | AT91SAM7X_BIT_SPI0_CSR_BITS_08
                            | (1u << 8);

    FSDev_BSP_SPI_SetClkFreq(unit_nbr, FS_DEV_SD_DFLT_CLK_SPD);

    AT91SAM7X_REG_SPI0_CR   = AT91SAM7X_BIT_SPI0_CR_SPIEN;

    while (DEF_BIT_IS_CLR(AT91SAM7X_REG_SPI0_SR, AT91SAM7X_BIT_SPI0_SR_SPIENS) == DEF_YES) {
        ;
    }

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
    CPU_BOOLEAN   rdy;
    CPU_BOOLEAN   rxd;
    CPU_INT08U    datum;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
        do{
            rdy = DEF_BIT_IS_SET(AT91SAM7X_REG_SPI0_SR, AT91SAM7X_BIT_SPI0_INT_TDRE);
        } while (rdy == DEF_NO);

        AT91SAM7X_REG_SPI0_TDR = 0xFFu;                         /* Tx dummy byte.                                       */

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(AT91SAM7X_REG_SPI0_SR, AT91SAM7X_BIT_SPI0_INT_RDRF);
        } while (rxd == DEF_NO);

        datum     = AT91SAM7X_REG_SPI0_RDR;                     /* Rd rx'd byte.                                        */
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
    CPU_BOOLEAN   rdy;
    CPU_BOOLEAN   rxd;
    CPU_INT08U    datum;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    p_src_08 = (CPU_INT08U *)p_src;
    while (cnt > 0u) {
        do{
            rdy = DEF_BIT_IS_SET(AT91SAM7X_REG_SPI0_SR, AT91SAM7X_BIT_SPI0_INT_TDRE);
        } while (rdy == DEF_NO);

        datum                  = *p_src_08;
        AT91SAM7X_REG_SPI0_TDR =  datum;                        /* Tx byte.                                             */
        p_src_08++;

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(AT91SAM7X_REG_SPI0_SR, AT91SAM7X_BIT_SPI0_INT_RDRF);
        } while (rxd == DEF_NO);

        datum = AT91SAM7X_REG_SPI0_RDR;                         /* Rd rx'd dummy byte.                                  */
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

    AT91SAM7X_REG_PIOA_CODR = AT91SAM7X_PIN_SPI0_NPCS1;
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

    AT91SAM7X_REG_PIOA_SODR = AT91SAM7X_PIN_SPI0_NPCS1;
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
    CPU_INT32U  csr;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    clk_div = (AT91SAM7X_FREQ_MCK + freq - 1u) / freq;
    if (clk_div == 0u) {
        clk_div =  1u;
    }
    if (clk_div >  255u) {
        clk_div =  255u;
    }

    csr                      =  AT91SAM7X_REG_SPI0_CSR1;
    csr                     &= ~0x0000FF00u;
    csr                     |= (clk_div << 8);
    AT91SAM7X_REG_SPI0_CSR1  =  csr;
}
