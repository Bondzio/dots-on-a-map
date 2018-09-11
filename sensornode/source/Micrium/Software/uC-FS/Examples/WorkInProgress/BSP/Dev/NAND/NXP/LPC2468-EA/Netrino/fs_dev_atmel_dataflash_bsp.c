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
#include <includes.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  LPC2XXX_PERIPH_ID_SPI                          8u

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

                                                                            /* ------------------ SPI ----------------- */
#define SPI_BASE  0xE0020000
#define SPI_CR    *(CPU_REG32*) (SPI_BASE + 0x00)                 /* Control register.                        */
#define SPI_SR    *(CPU_REG32*) (SPI_BASE + 0x04)                 /* Status register.                         */
#define SPI_DR    *(CPU_REG16*) (SPI_BASE + 0x08)                 /* Data register.                           */
#define SPI_MCKR  *(CPU_REG32*) (SPI_BASE + 0x0C)                 /* Clock counter register.                  */

#define  LPC2XXX_REG_PCONP                          (*(CPU_REG32 *)(0xE01FC0C4u))

#define  LPC2XXX_REG_PCLKSEL0                       (*(CPU_REG32 *)(0xE01FC1A8u))
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
    CPU_INT32U  pinsel;
    CPU_INT32U   pclksel;

    
  
    
                                                                /* Configure P0.15 for SPI Clk                    */
    pinsel   = PINSEL0;
    pinsel  |= 0xC0000000;
    PINSEL0  = pinsel;

                                                                 /* Configure P0.16, P0.17, P0.18 for SSEL MISO & MOSI  */
    pinsel   = PINSEL1;
    pinsel  |= 0x0000003C;
    PINSEL1  = pinsel;
    
   
 
    
                                                                 /* ---------------------- EN SPI ---------------------- */
    LPC2XXX_REG_PCONP |= DEF_BIT(LPC2XXX_PERIPH_ID_SPI);        /* En periph clk.                                       */
   
   
    
    
    SPI_CR   = (0 << 2) |                           /* 8 bits of data per transfer                          */
               (0 << 3) |
               (0 << 4) |                           /* Clk active high                                      */  
               (1 << 5) |                           /* SPI in master mode                                   */
               (0 << 7);                            /* Interrupts disabled                                  */
   
    
    /* Set direction */
    IO0DIR = 1 << 16;
    IO0SET = 1 << 16;                           /* SSEL goes high: disable CE */
    
    
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
        SPI_DR = 0xff;
        while ((SPI_SR & (1 << 7)) == 0);
      
        
       *p_dest_08 = SPI_DR & 0xFF;
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
        SPI_DR = *p_src_08;
        while((SPI_SR & (1 << 7)) == 0);
        
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
     IO0CLR = 1 << 16;                        /* SSEL goes low: enablee CE */
    
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
    IO0SET  = 1 << 16;                           /* SSEL goes high: disable CE */
   
   
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
    /* $$$$$$ Value to set in SPI_MCKR need to be correctly calculated */
    SPI_MCKR = 8;                      /* MCU doc. states that this value should be an even number >= 8 for master mode */
}


