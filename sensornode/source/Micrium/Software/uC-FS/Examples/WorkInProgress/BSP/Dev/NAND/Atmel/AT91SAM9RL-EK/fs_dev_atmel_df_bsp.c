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
*                                          ATMEL DATAFLASH DEVICE
*
*                                BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                      Atmel AT91SAM9M10 ON THE ATMEL AT91SAM9M10-EVAL BOARD
*
* Filename      : fs_dev_atmel_df_bsp.c
* Version       : v4.07.00
* Programmer(s) : AHFAI
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

#define  PIOA_ID                                    2u
#define  SPI_ID                                    13u

#define  SD_CS_PIN                                 28u
#define  SD_MISO_PIN                               25u
#define  SD_MOSI_PIN                               26u
#define  SD_CLK_PIN                                27u
#define  SPI_CSR                              SPI_CSR0


#define MCLK                              (100045440uL)  /* may depend on PLL */
#define MCLK_SPICLOCK                             MCLK         /* defaults is SD clock = MCLK */


/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/
                                                                            /* ----------------- PIOA ----------------- */
#define  PIOA_BASE                                     0xFFFFF400u
#define  PIOA_PER       *(CPU_REG32*) (PIOA_BASE + 0x00u)         /* Enable register, enables PIO function    */
#define  PIOA_PDR       *(CPU_REG32*) (PIOA_BASE + 0x04u)         /* PIOA disable register                    */
#define  PIOA_OER       *(CPU_REG32*) (PIOA_BASE + 0x10u)         /* Output enable reg., sets to output mode  */
#define  PIOA_ODR       *(CPU_REG32*) (PIOA_BASE + 0x14u)         /* Output disable reg., sets to output mode */
#define  PIOA_SODR      *(CPU_REG32*) (PIOA_BASE + 0x30u)         /* Set output data                          */
#define  PIOA_CODR      *(CPU_REG32*) (PIOA_BASE + 0x34u)         /* Clear output data register               */
#define  PIOA_ODSR      *(CPU_REG32*) (PIOA_BASE + 0x38u)         /* output data status register              */
#define  PIOA_PDSR      *(CPU_REG32*) (PIOA_BASE + 0x3Cu)         /* pin data status register                 */
#define  PIOA_OWER      *(CPU_REG32*) (PIOA_BASE + 0xA0u)         /* Output write enable register             */
#define  PIOA_OWDR      *(CPU_REG32*) (PIOA_BASE + 0xA4u)         /* Output write disable register            */
#define  PIOA_ASR       *(CPU_REG32*) (PIOA_BASE + 0x70u)         /* PIOA "A" peripheral select register      */
#define  PIOA_BSR       *(CPU_REG32*) (PIOA_BASE + 0x74u)         /* PIOA "B" peripheral select register      */

                                                                            /* ------------------ SPI ----------------- */
#define SPI_BASE  0xFFFCC000
#define SPI_CR    *(CPU_REG32*) (SPI_BASE + 0x00)
#define SPI_MR    *(CPU_REG32*) (SPI_BASE + 0x04)
#define SPI_RDR   *(CPU_REG32*) (SPI_BASE + 0x08)
#define SPI_TDR   *(CPU_REG32*) (SPI_BASE + 0x0C)
#define SPI_SR    *(CPU_REG32*) (SPI_BASE + 0x10)
#define SPI_IER   *(CPU_REG32*) (SPI_BASE + 0x14)
#define SPI_IDR   *(CPU_REG32*) (SPI_BASE + 0x18)
#define SPI_IMR   *(CPU_REG32*) (SPI_BASE + 0x1c)
#define SPI_CSR0  *(CPU_REG32*) (SPI_BASE + 0x30)
#define SPI_CSR1  *(CPU_REG32*) (SPI_BASE + 0x34)
#define SPI_CSR2  *(CPU_REG32*) (SPI_BASE + 0x38)
#define SPI_CSR3  *(CPU_REG32*) (SPI_BASE + 0x3c)

                                                                            /* ------------------ PMC ----------------- */
#define PMC_BASE  0xFFFFFC00
#define PMC_SCER  *(CPU_REG32*) (PMC_BASE + 0x00) /* System Clock Enable Register */
#define PMC_SCDR  *(CPU_REG32*) (PMC_BASE + 0x04) /* System Clock Disable Register */
#define PMC_SCSR  *(CPU_REG32*) (PMC_BASE + 0x08) /* System Clock Status Register */
#define PMC_PCER  *(CPU_REG32*) (PMC_BASE + 0x10)  /* Peripheral clock enable register */


/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/


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

CPU_BOOLEAN  FSDev_NAND_BSP_SPI_Open       (FS_QTY                  unit_nbr);   /* Open (initialize) SPI.              */

void         FSDev_NAND_BSP_SPI_Close      (FS_QTY                  unit_nbr);   /* Close (uninitialize) SPI.           */

void         FSDev_NAND_BSP_SPI_Lock       (FS_QTY                  unit_nbr);   /* Acquire SPI lock.                   */

void         FSDev_NAND_BSP_SPI_Unlock     (FS_QTY                  unit_nbr);   /* Release SPI lock.                   */

void         FSDev_NAND_BSP_SPI_Rd         (FS_QTY                  unit_nbr,    /* Read from SPI.                      */
                                            void                   *p_dest,
                                            CPU_SIZE_T              cnt);

void         FSDev_NAND_BSP_SPI_Wr         (FS_QTY                  unit_nbr,    /* Write to SPI.                       */
                                            void                   *p_src,
                                            CPU_SIZE_T              cnt);

void         FSDev_NAND_BSP_SPI_ChipSelEn  (FS_QTY                  unit_nbr);   /* Enable serial flash chip select.    */

void         FSDev_NAND_BSP_SPI_ChipSelDis (FS_QTY                  unit_nbr);   /* Disable serial flash chip select.   */

void         FSDev_NAND_BSP_SPI_SetClkFreq (FS_QTY                  unit_nbr,    /* Set SPI clock frequency.            */
                                            CPU_INT32U              freq);


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
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_NAND_BSP_SPI_Open       (FS_QTY                  unit_nbr);   /* Open (initialize) SPI.               */

void         FSDev_NAND_BSP_SPI_Close      (FS_QTY                  unit_nbr);   /* Close (uninitialize) SPI.            */

void         FSDev_NAND_BSP_SPI_Lock       (FS_QTY                  unit_nbr);   /* Acquire SPI lock.                    */

void         FSDev_NAND_BSP_SPI_Unlock     (FS_QTY                  unit_nbr);   /* Release SPI lock.                    */

void         FSDev_NAND_BSP_SPI_Rd         (FS_QTY                  unit_nbr,    /* Read from SPI.                       */
                                            void                   *p_dest,
                                            CPU_SIZE_T              cnt);

void         FSDev_NAND_BSP_SPI_Wr         (FS_QTY                  unit_nbr,    /* Write to SPI.                        */
                                            void                   *p_src,
                                            CPU_SIZE_T              cnt);

void         FSDev_NAND_BSP_SPI_ChipSelEn  (FS_QTY                  unit_nbr);   /* Enable serial flash chip select.     */

void         FSDev_NAND_BSP_SPI_ChipSelDis (FS_QTY                  unit_nbr);   /* Disable serial flash chip select.    */

void         FSDev_NAND_BSP_SPI_SetClkFreq (FS_QTY                  unit_nbr,    /* Set SPI clock frequency.             */
                                            CPU_INT32U              freq);

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
*                                        FSDev_NAND_BSP_SPI_Open()
*
* Description : Open (initialize) bus for DataFlash device.
*
* Argument(s) : unit_nbr        Unit number of DataFlash device.
*
*               bus_width       Bus width, in bits.
*
* Return(s)   : DEF_OK,   if interface was opened.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is opened.
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_NAND_BSP_SPI_Open (FS_QTY      unit_nbr)
{
    CPU_INT32U  SPIFreq;
    CPU_INT32U  InFreq;
    CPU_INT08U  sbcr;

    /* $$$$ Replace the hardcoded values by defines constant (register bits define) */
    
    /* $$$$ Move SPI Freq setting to the corresponding function  FSDev_NAND_BSP_SPI_SetClkFreq() */


    sbcr    = 0x02;
   /* SPIFreq = 1000 * 12000;
    if (SPIFreq >= 200000) {
        InFreq = 48000000;
    }
    sbcr = (InFreq + SPIFreq - 1) / SPIFreq;*/
  
    
                                                                /* Enable Power for PIOA and SPI block                  */
  
    PMC_PCER = (1 << SPI_ID)
               | (1 << PIOA_ID);
  
                                                                /* Setup Pins                                           */

    PIOA_PER =  0
                | (1 << SD_CS_PIN)
                | (1 << SD_MISO_PIN)
                | (1 << SD_MOSI_PIN)
                | (1 << SD_CLK_PIN)
                ;
      PIOA_SODR = 0
                | (1 << SD_CS_PIN)
                | (1 << SD_MOSI_PIN)
                | (1 << SD_CLK_PIN)
                ;
      PIOA_OER  = 0
                | (1 << SD_CS_PIN)
                | (1 << SD_MOSI_PIN)
                | (1 << SD_CLK_PIN)
                ;
      PIOA_ODR  = 0
                | (1 << SD_MOSI_PIN)
                ;
      PIOA_PDR =  0
                | (1 << SD_MISO_PIN)                            /* SPI-MISO                                             */
                | (1 << SD_MOSI_PIN)                            /* SPI-MOSI                                             */
                | (1 << SD_CLK_PIN)                             /* SPI-Clock                                            */
                ;
      PIOA_ASR  = 0
                |(1 << SD_MISO_PIN)                             /* SPI-MISO                                             */
                |(1 << SD_MOSI_PIN)                             /* SPI-MOSI                                             */
                |(1 << SD_CLK_PIN)                              /* SPI-Clock                                            */
                ;

                                                                /* SPI                                                  */
      SPI_CR    = (1 << 7);                                     /* Software reset                                       */
      SPI_MR    = 0
                |(1 << 0)                                       /* 1 : Master mode                                      */
                |(0 << 1)                                       /* 0 : Fixed chip select                                */
                |(0 << 2)                                       /* Chip select                                          */
                |(0 << 3)                                       /* 0: Use MCLK as clock                                 */
                |(1 << 4)                                       /* 1: Fault detection disable                           */
                |(0 << 7)                                       /* 1: Loopback                                          */
                |(0 << 16)                                      /* 0000b: Use CS0                                       */
                ;
      
      SPI_CSR   = 0
                |(1 << 0)                                       /* 1 : Clock polarity of idle is high                   */
                |(0 << 1)                                       /* Clock hase sel                                       */
                |(1 << 3)                                       /* Leave CS0 stay low                                   */
                |(0 << 4)                                       /* 0000b: 8 bits per transfer                           */
                |(sbcr<< 8)                                     /* 8..15: SCBR: Baud rate divider                       */
                |(0x100000);
                  ;
      SPI_CR    = (1 << 0);                                     /* Enable SPI                                           */  

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

void  FSDev_NAND_BSP_SPI_Close (FS_QTY  unit_nbr)
{
    ;
}

/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_SPI_Lock()
*
* Description : 
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

void  FSDev_NAND_BSP_SPI_Lock (FS_QTY  unit_nbr)
{
    ;
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_SPI_Unlock()
*
* Description : 
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

void  FSDev_NAND_BSP_SPI_Unlock (FS_QTY  unit_nbr)
{
    ;
}

/*
*********************************************************************************************************
*                                         FSDev_NAND_BSP_SPI_Rd()
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

void  FSDev_NAND_BSP_SPI_Rd (FS_QTY       unit_nbr,
                             void        *p_dest,
                             CPU_SIZE_T   cnt)
{
    CPU_INT08U  *p_dest_08;

    /* $$$$ Replace the hardcoded values by defines constant (register bits define) */
    p_dest_08 = (CPU_INT08U *)p_dest;
    do {
        SPI_TDR = 0xff;
        while ((SPI_SR & (1 << 9)) == 0);
        while ((SPI_SR & (1 << 0)) == 0);
       *p_dest_08 = SPI_RDR;
        p_dest_08++;
        cnt--;
    } while (cnt); 
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

void  FSDev_NAND_BSP_SPI_Wr (FS_QTY       unit_nbr,
                             void        *p_src,
                             CPU_SIZE_T   cnt)
{
    CPU_INT08U  *p_src_08;

    /* $$$$ Replace the hardcoded values by defines constant (register bits define) */
    p_src_08 = (CPU_INT08U *)p_src;
    do {
        SPI_TDR = *p_src_08;
        while ((SPI_SR & (1 << 9)) == 0);
        cnt--;
        p_src_08++;
    } while (cnt);    
    
}


/*
*********************************************************************************************************
*                                     FSDev_NAND_BSP_SPI_ChipSelEn()
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

void  FSDev_NAND_BSP_SPI_ChipSelEn (FS_QTY  unit_nbr)
{
    PIOA_CODR  = (1 <<  SD_CS_PIN);       /* CS0 on eval board */   
}


/*
*********************************************************************************************************
*                                     FSDev_NAND_BSP_SPI_ChipSelDis()
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

void  FSDev_NAND_BSP_SPI_ChipSelDis (FS_QTY  unit_nbr)
{
    PIOA_SODR  = (1 <<  SD_CS_PIN);       /* CS0 on eval board */  
}


/*
*********************************************************************************************************
*                                     FSDev_NAND_BSP_SPI_SetClkFreq()
*
* Description : Set SPI clock frequency.
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

void  FSDev_NAND_BSP_SPI_SetClkFreq (FS_QTY      unit_nbr,
                                     CPU_INT32U  freq)
{
    
}

