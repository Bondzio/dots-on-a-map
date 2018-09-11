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
*                                       ATMEL DATAFLASH DEVICE
*
*                                BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                       Atmel AT91SAM9M10 ON THE ATMEL AT91SAM9M10-G45-EK BOARD
*
* Filename      : fs_dev_atmel_df_bsp.c
* Version       : v4.07.00
* Programmer(s) : FBJ
*                 AHFAI
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_ATMEL_DF_BSP_MODULE
#include  <fs_dev_nand.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  SAM9M10_FREQ_MCK                        100000000u

#define  SAM9M10_PIN_SPI0_NPCS1                 DEF_BIT_03      /* PB03                                                 */
#define  SAM9M10_PIN_SPI0_MISO                  DEF_BIT_00      /* PB00                                                 */
#define  SAM9M10_PIN_SPI0_MOSI                  DEF_BIT_01      /* PB01                                                 */
#define  SAM9M10_PIN_SPI0_SPCK                  DEF_BIT_02      /* PB02                                                 */

#define  SAM9M10_PERIPH_ID_PIOB                 DEF_BIT_03
#define  SAM9M10_PERIPH_ID_SPI0                 DEF_BIT_14

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

                                                                /* ----------------- GPIOB REGISTERS ------------------ */
#define  SAM9M10_REG_PIOB_BASE                      (CPU_ADDR)(0xFFFFF400u)
#define  SAM9M10_REG_PIOB_PER                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x00u))
#define  SAM9M10_REG_PIOB_PDR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x04u))
#define  SAM9M10_REG_PIOB_PSR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x08u))
#define  SAM9M10_REG_PIOB_OER                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x10u))
#define  SAM9M10_REG_PIOB_ODR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x14u))
#define  SAM9M10_REG_PIOB_OSR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x18u))
#define  SAM9M10_REG_PIOB_IFER                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x20u))
#define  SAM9M10_REG_PIOB_IFDR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x24u))
#define  SAM9M10_REG_PIOB_IFSR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x28u))
#define  SAM9M10_REG_PIOB_SODR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x30u))
#define  SAM9M10_REG_PIOB_CODR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x34u))
#define  SAM9M10_REG_PIOB_ODSR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x38u))
#define  SAM9M10_REG_PIOB_PDSR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x3Cu))
#define  SAM9M10_REG_PIOB_IER                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x40u))
#define  SAM9M10_REG_PIOB_IDR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x44u))
#define  SAM9M10_REG_PIOB_IMR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x48u))
#define  SAM9M10_REG_PIOB_ISR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x4Cu))
#define  SAM9M10_REG_PIOB_MDER                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x50u))
#define  SAM9M10_REG_PIOB_MDDR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x54u))
#define  SAM9M10_REG_PIOB_MDSR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x58u))
#define  SAM9M10_REG_PIOB_PUDR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x60u))
#define  SAM9M10_REG_PIOB_PUER                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x64u))
#define  SAM9M10_REG_PIOB_PUSR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x68u))
#define  SAM9M10_REG_PIOB_ASR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x70u))
#define  SAM9M10_REG_PIOB_BSR                  (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x74u))
#define  SAM9M10_REG_PIOB_ABSR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0x78u))
#define  SAM9M10_REG_PIOB_OWER                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0xA0u))
#define  SAM9M10_REG_PIOB_OWDR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0xA4u))
#define  SAM9M10_REG_PIOB_OWSR                 (*(CPU_REG32 *)(SAM9M10_REG_PIOB_BASE + 0xA8u))


                                                                /* ------------------ SPI REGISTERS ------------------- */
#define  SAM9M10_REG_SPI0_BASE                      (CPU_ADDR)(0xFFFA4000u)
#define  SAM9M10_REG_SPI0_CR                   (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x00u))
#define  SAM9M10_REG_SPI0_MR                   (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x04u))
#define  SAM9M10_REG_SPI0_RDR                  (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x08u))
#define  SAM9M10_REG_SPI0_TDR                  (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x0Cu))
#define  SAM9M10_REG_SPI0_SR                   (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x10u))
#define  SAM9M10_REG_SPI0_IER                  (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x14u))
#define  SAM9M10_REG_SPI0_IDR                  (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x18u))
#define  SAM9M10_REG_SPI0_IMR                  (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x1Cu))
#define  SAM9M10_REG_SPI0_CSR0                 (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x30u))
#define  SAM9M10_REG_SPI0_CSR1                 (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x34u))
#define  SAM9M10_REG_SPI0_CSR2                 (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x38u))
#define  SAM9M10_REG_SPI0_CSR3                 (*(CPU_REG32 *)(SAM9M10_REG_SPI0_BASE + 0x3Cu))


                                                                /* ------------ POWER MANAGEMENT CONTROLLER ----------- */
#define  SAM9M10_REG_PMC_BASE                       (CPU_ADDR)(0xFFFFFC00u)
#define  SAM9M10_REG_PMC_SCER                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x00u))
#define  SAM9M10_REG_PMC_SCDR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x04u))
#define  SAM9M10_REG_PMC_SCSR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x08u))
#define  SAM9M10_REG_PMC_PCER                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x10u))
#define  SAM9M10_REG_PMC_PCDR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x14u))
#define  SAM9M10_REG_PMC_PCSR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x18u))
#define  SAM9M10_REG_PMC_MOR                   (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x20u))
#define  SAM9M10_REG_PMC_MCFR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x24u))
#define  SAM9M10_REG_PMC_PLLR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x2Cu))
#define  SAM9M10_REG_PMC_MCKR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x30u))
#define  SAM9M10_REG_PMC_PCKR                  (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x40u))
#define  SAM9M10_REG_PMC_IER                   (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x60u))
#define  SAM9M10_REG_PMC_IDR                   (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x64u))
#define  SAM9M10_REG_PMC_SR                    (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x68u))
#define  SAM9M10_REG_PMC_IMR                   (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE + 0x6Cu))

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  SAM9M10_BIT_SPI0_CR_SPIEN               DEF_BIT_00
#define  SAM9M10_BIT_SPI0_CR_SPIDIS              DEF_BIT_01
#define  SAM9M10_BIT_SPI0_CR_SWRST               DEF_BIT_07
#define  SAM9M10_BIT_SPI0_CR_LASTXFER            DEF_BIT_24

#define  SAM9M10_BIT_SPI0_MR_MSTR                DEF_BIT_00
#define  SAM9M10_BIT_SPI0_MR_PS                  DEF_BIT_01
#define  SAM9M10_BIT_SPI0_MR_PCSDEC              DEF_BIT_02
#define  SAM9M10_BIT_SPI0_MR_MODFDIS             DEF_BIT_04
#define  SAM9M10_BIT_SPI0_MR_LLB                 DEF_BIT_07
#define  SAM9M10_BIT_SPI0_MR_PCS_MASK            0x000F0000u
#define  SAM9M10_BIT_SPI0_MR_PCS_NPCS0           0x000E0000u
#define  SAM9M10_BIT_SPI0_MR_PCS_NPCS1           0x000D0000u
#define  SAM9M10_BIT_SPI0_MR_PCS_NPCS2           0x000B0000u
#define  SAM9M10_BIT_SPI0_MR_PCS_NPCS3           0x00070000u
#define  SAM9M10_BIT_SPI0_MR_DLYBCS_MASK         0xFF000000u

#define  SAM9M10_BIT_SPI0_RDR_PCS_NPCS0          0x000E0000u
#define  SAM9M10_BIT_SPI0_RDR_PCS_NPCS1          0x000D0000u
#define  SAM9M10_BIT_SPI0_RDR_PCS_NPCS2          0x000B0000u
#define  SAM9M10_BIT_SPI0_RDR_PCS_NPCS3          0x00070000u

#define  SAM9M10_BIT_SPI0_TDR_PCS_NPCS0          0x000E0000u
#define  SAM9M10_BIT_SPI0_TDR_PCS_NPCS1          0x000D0000u
#define  SAM9M10_BIT_SPI0_TDR_PCS_NPCS2          0x000B0000u
#define  SAM9M10_BIT_SPI0_TDR_PCS_NPCS3          0x00070000u

#define  SAM9M10_BIT_SPI0_SR_SPIENS              DEF_BIT_16

#define  SAM9M10_BIT_SPI0_INT_RDRF               DEF_BIT_00
#define  SAM9M10_BIT_SPI0_INT_TDRE               DEF_BIT_01
#define  SAM9M10_BIT_SPI0_INT_MODF               DEF_BIT_02
#define  SAM9M10_BIT_SPI0_INT_OVRES              DEF_BIT_03
#define  SAM9M10_BIT_SPI0_INT_NSSR               DEF_BIT_08
#define  SAM9M10_BIT_SPI0_INT_TXEMPTY            DEF_BIT_09
#define  SAM9M10_BIT_SPI0_INT_ALL               (SAM9M10_BIT_SPI0_INT_RDRF   | SAM9M10_BIT_SPI0_INT_TDRE   | SAM9M10_BIT_SPI0_INT_MODF  | \
                                                 SAM9M10_BIT_SPI0_INT_OVRES  | SAM9M10_BIT_SPI0_INT_NSSR   | SAM9M10_BIT_SPI0_INT_TXEMPTY)

#define  SAM9M10_BIT_SPI0_CSR_CPOL               DEF_BIT_00
#define  SAM9M10_BIT_SPI0_CSR_NCPHA              DEF_BIT_01
#define  SAM9M10_BIT_SPI0_CSR_CSAAT              DEF_BIT_03
#define  SAM9M10_BIT_SPI0_CSR_BITS_08            DEF_BIT_NONE

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
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                            /* ------------- SPI API FNCTS ------------ */
static  CPU_BOOLEAN  FSDev_NAND_BSP_SPI_Open      (FS_QTY       unit_nbr);  /* Open    (initialize) SPI.                */

static  void         FSDev_NAND_BSP_SPI_Close     (FS_QTY       unit_nbr);  /* Close (uninitialize) SPI.                */

static  void         FSDev_NAND_BSP_SPI_Lock      (FS_QTY       unit_nbr);  /* Acquire SPI lock.                        */

static  void         FSDev_NAND_BSP_SPI_Unlock    (FS_QTY       unit_nbr);  /* Release SPI lock.                        */

static  void         FSDev_NAND_BSP_SPI_Rd        (FS_QTY       unit_nbr,   /* Read from SPI.                           */
                                                   void        *p_dest,
                                                   CPU_SIZE_T   cnt);

static  void         FSDev_NAND_BSP_SPI_Wr        (FS_QTY       unit_nbr,   /* Write to  SPI.                           */
                                                   void        *p_src,
                                                   CPU_SIZE_T   cnt);

static  void         FSDev_NAND_BSP_SPI_ChipSelEn (FS_QTY       unit_nbr);  /* Enable  serial flash chip select.        */

static  void         FSDev_NAND_BSP_SPI_ChipSelDis(FS_QTY       unit_nbr);  /* Disable serial flash chip select.        */

static  void         FSDev_NAND_BSP_SPI_SetClkFreq(FS_QTY       unit_nbr,   /* Set SPI clock frequency.                 */
                                                   CPU_INT32U   freq);


/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_SPI_API  FSDev_NAND_BSP_SPI = {
    FSDev_NAND_BSP_SPI_Open,
    FSDev_NAND_BSP_SPI_Close,
    FSDev_NAND_BSP_SPI_Lock,
    FSDev_NAND_BSP_SPI_Unlock,
    FSDev_NAND_BSP_SPI_Rd,
    FSDev_NAND_BSP_SPI_Wr,
    FSDev_NAND_BSP_SPI_ChipSelEn,
    FSDev_NAND_BSP_SPI_ChipSelDis,
    FSDev_NAND_BSP_SPI_SetClkFreq
};

/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     FILE SYSTEM NAND FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      FSDev_NAND_BSP_SPI_Open()
*
* Description : Open (initialize) bus for DataFlash device.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
*               bus_width   Bus width, in bits.
*
* Return(s)   : DEF_OK,   if interface was successfully opened.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Physical-layer driver.
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
*               (4) A Atmel Dataflash (NAND) device is connected to the following pins :
*
*                   ----------------------------------------------
*                   | SAM9M10 NAME | SAM9M10 PIO # |  NAND NAME  |
*                   ----------------------------------------------
*                   |  SPI0_NPCS1  |     PB[03]    | CS  (pin 4) |
*                   |  SPI0_MOSI   |     PB[00]    | SI  (pin 1) |
*                   |  SPI0_SPCK   |     PB[01]    | SCK (pin 2) |
*                   |  SPI0_MISO   |     PB[02]    | SO  (pin 8) |
*                   ----------------------------------------------
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FSDev_NAND_BSP_SPI_Open (FS_QTY  unit_nbr)
{
    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_NAND_BSP_SPI_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

                                                                /* ---------------------- CFG HW ---------------------- */
    SAM9M10_REG_PMC_PCER |= SAM9M10_PERIPH_ID_SPI0
                         |  SAM9M10_PERIPH_ID_PIOB;
  
    SAM9M10_REG_PIOB_PER  = SAM9M10_PIN_SPI0_NPCS1
                          | SAM9M10_PIN_SPI0_MISO
                          | SAM9M10_PIN_SPI0_MOSI
                          | SAM9M10_PIN_SPI0_SPCK;

    SAM9M10_REG_PIOB_SODR = SAM9M10_PIN_SPI0_NPCS1
                          | SAM9M10_PIN_SPI0_MOSI
                          | SAM9M10_PIN_SPI0_SPCK;

    SAM9M10_REG_PIOB_OER  = SAM9M10_PIN_SPI0_NPCS1
                          | SAM9M10_PIN_SPI0_MOSI
                          | SAM9M10_PIN_SPI0_SPCK;

    SAM9M10_REG_PIOB_ODR  = SAM9M10_PIN_SPI0_MOSI;

    SAM9M10_REG_PIOB_PDR  = SAM9M10_PIN_SPI0_MISO
                          | SAM9M10_PIN_SPI0_MOSI
                          | SAM9M10_PIN_SPI0_SPCK;

    SAM9M10_REG_PIOB_ASR  = SAM9M10_PIN_SPI0_MISO
                          | SAM9M10_PIN_SPI0_MOSI
                          | SAM9M10_PIN_SPI0_SPCK;


                                                                /* ---------------------- CFG SPI --------------------- */


    SAM9M10_REG_SPI0_CR   = SAM9M10_BIT_SPI0_CR_SWRST;          /* Reset SPI0.                                          */

    SAM9M10_REG_SPI0_MR   = SAM9M10_BIT_SPI0_MR_MSTR
                          | SAM9M10_BIT_SPI0_MR_MODFDIS;

    SAM9M10_REG_SPI0_CSR0 = SAM9M10_BIT_SPI0_CSR_CPOL
                          | SAM9M10_BIT_SPI0_CSR_CSAAT;

    FSDev_NAND_BSP_SPI_SetClkFreq(unit_nbr, 10000000u);

    SAM9M10_REG_SPI0_CR   = SAM9M10_BIT_SPI0_CR_SPIEN;

    while (DEF_BIT_IS_CLR(SAM9M10_REG_SPI0_SR, SAM9M10_BIT_SPI0_SR_SPIENS) == DEF_YES) {
        ;
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_Close()
*
* Description : Close (uninitialize) bus for DataFlash device.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_Close (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                      FSDev_NAND_BSP_SPI_Lock()
*
* Description : Acquire SPI lock.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_Lock (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                      FSDev_NAND_BSP_SPI_Unlock()
*
* Description : Release SPI lock.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_Unlock (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_SPI_Rd()
*
* Description : Read data from DataFlash.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
*               p_dest      Pointer to destination memory buffer.
*
*               cnt         Number of octets to read.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_Rd (FS_QTY       unit_nbr,
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
            rdy = DEF_BIT_IS_SET(SAM9M10_REG_SPI0_SR, SAM9M10_BIT_SPI0_INT_TDRE);
        } while (rdy == DEF_NO);

        SAM9M10_REG_SPI0_TDR = 0xFFu;                           /* Tx dummy byte.                                       */

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(SAM9M10_REG_SPI0_SR, SAM9M10_BIT_SPI0_INT_RDRF);
        } while (rxd == DEF_NO);

        datum     = SAM9M10_REG_SPI0_RDR;                       /* Rd rx'd byte.                                        */
       *p_dest_08 = datum;
        p_dest_08++;
        cnt--;
    }
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_SPI_Wr()
*
* Description : Write data to DataFlash.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
*               p_src       Pointer to source memory buffer.
*
*               cnt         Number of octets to write.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_Wr (FS_QTY       unit_nbr,
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
            rdy = DEF_BIT_IS_SET(SAM9M10_REG_SPI0_SR, SAM9M10_BIT_SPI0_INT_TDRE);
        } while (rdy == DEF_NO);

        datum                  = *p_src_08;
        SAM9M10_REG_SPI0_TDR =  datum;                          /* Tx byte.                                             */
        p_src_08++;

                                                                /* Wait to rx byte.                                     */
        do{
            rxd = DEF_BIT_IS_SET(SAM9M10_REG_SPI0_SR, SAM9M10_BIT_SPI0_INT_RDRF);
        } while (rxd == DEF_NO);

        datum = SAM9M10_REG_SPI0_RDR;                           /* Rd rx'd dummy byte.                                  */
       (void)&datum;

        cnt--;
    }
}


/*
*********************************************************************************************************
*                                   FSDev_NAND_BSP_SPI_ChipSelEn()
*
* Description : Enable DataFlash chip select.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_ChipSelEn (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    SAM9M10_REG_PIOB_CODR = SAM9M10_PIN_SPI0_NPCS1;
}


/*
*********************************************************************************************************
*                                   FSDev_NAND_BSP_SPI_ChipSelDis()
*
* Description : Disable DataFlash chip select.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_ChipSelDis (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    SAM9M10_REG_PIOB_SODR = SAM9M10_PIN_SPI0_NPCS1;
}


/*
*********************************************************************************************************
*                                   FSDev_NAND_BSP_SPI_SetClkFreq()
*
* Description : Set SPI clock frequency.
*
* Argument(s) : unit_nbr    Unit number of DataFlash device.
*
*               freq        Clock frequency, in Hz.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) The effective clock frequency MUST be no more than 'freq'.  If the frequency cannot be
*                   configured equal to 'freq', it should be configured less than 'freq'.
*********************************************************************************************************
*/

static  void  FSDev_NAND_BSP_SPI_SetClkFreq (FS_QTY      unit_nbr,
                                             CPU_INT32U  freq)
{
    CPU_INT32U  clk_div;
    CPU_INT32U  csr;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    clk_div = (SAM9M10_FREQ_MCK + freq - 1u) / freq;
    if (clk_div == 0u) {
        clk_div =  1u;
    }
    if (clk_div >  255u) {
        clk_div =  255u;
    }

    csr                    =  SAM9M10_REG_SPI0_CSR0;
    csr                   &= ~0x0000FF00u;
    csr                   |= (clk_div << 8);
    SAM9M10_REG_SPI0_CSR0  =  csr;
}

