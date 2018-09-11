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
*                                          NAND FLASH DEVICES
*                                           PARALLEL DEVICE
*
*                                BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                      Atmel AT91SAM9M10 ON THE ATMEL AT91SAM9M10-EVAL BOARD
*
* Filename      : fs_dev_nand_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_NAND_BSP_MODULE
#include  <fs_dev_nand.h>

#include    "at91sam9m10.h"
#include <board.h>
#include <board_memories.h>
#include <pio.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define NAND_BASE_ADDR          0x40000000
#define NAND_DATA               (CPU_INT16U *)(NAND_BASE_ADDR + 0x000000)
#define NAND_ADDR               (CPU_INT16U *)(NAND_BASE_ADDR + 0x200000)
#define NAND_CMD                (CPU_INT16U *)(NAND_BASE_ADDR + 0x400000)

/*********************************************************************
*
*       #define sfrs
*
**********************************************************************
*/
#if 1
#define SMC0_BASE_ADDR    0xFFFFE400
#define SMC0_SETUP3       (*(volatile CPU_INT32U*)(SMC0_BASE_ADDR + 3 * 0x10 + 0x00))  /* SMC CS3 Setup Register */
#define SMC0_PULSE3       (*(volatile CPU_INT32U*)(SMC0_BASE_ADDR + 3 * 0x10 + 0x04))  /* SMC CS3 Pulse Register */
#define SMC0_CYCLE3       (*(volatile CPU_INT32U*)(SMC0_BASE_ADDR + 3 * 0x10 + 0x08))  /* SMC CS3 Cycle Register */
#define SMC0_CTRL3        (*(volatile CPU_INT32U*)(SMC0_BASE_ADDR + 3 * 0x10 + 0x0C))  /* SMC CS3 Mode Register */
/*      MATRIX + EBI interface */
#define MATRIX_BASE_ADDR (0xFFFFEC00)                                 /* MATRIX Base Address */
#define MATRIX_MCFG      (*(volatile CPU_INT32U*)(MATRIX_BASE_ADDR + 0x00))  /* MATRIX Master configuration register */
#define MATRIX_EBICSA    (*(volatile CPU_INT32U*)(MATRIX_BASE_ADDR + 0x120)) /* MATRIX EBI Chip Select Assignment register */

#define PMC_BASE_ADDR    0xFFFFFC00
#define PMC_PCER         (*(volatile CPU_INT32U *)(PMC_BASE_ADDR + 0x10)) /* (PMC) Peripheral Clock Enable Register */
#define PMC_PCDR         (*(volatile CPU_INT32U *)(PMC_BASE_ADDR + 0x14)) /* (PMC) Peripheral Clock Disable Register */
#endif


#if 1
/* ========== Register definition for PIOA peripheral ========== */
#define PIOA_BASE        0xFFFFF200
#define PIOA_PER         (*(volatile CPU_INT32U *)(PIOA_BASE + 0x00)) /* (PIOA) PIO Enable Register */
#define PIOA_ODR         (*(volatile CPU_INT32U *)(PIOA_BASE + 0x14)) /* (PIOA) Output Disable Registerr */
#define PIOA_IFDR        (*(volatile CPU_INT32U *)(PIOA_BASE + 0x24)) /* (PIOA) Input Filter Disable Register */
#define PIOA_IDR         (*(volatile CPU_INT32U *)(PIOA_BASE + 0x44)) /* (PIOA) Interrupt Disable Register */
#define PIOA_PPUDR       (*(volatile CPU_INT32U *)(PIOA_BASE + 0x60)) /* (PIOA) Pull-up Disable Register */
#define PIOA_ODSR        (*(volatile CPU_INT32U *)(PIOA_BASE + 0x38)) /* (PIOA) Output Pin Data Status Register */
#define PIOA_PDSR        (*(volatile CPU_INT32U *)(PIOA_BASE + 0x3C)) /* (PIOA) Pin Data Status Register */
#define PIOA_PUER        (*(volatile CPU_INT32U *)(PIOA_BASE + 0x64))
#endif


/* ========== Register definition for PIOC peripheral ========== */
#define PIOC_BASE        0xFFFFF600
#define PIOC_PER         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x00)) /* (PIOC) PIO Enable Register */
#define PIOC_PDR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x04)) /* (PIOC) PIO Disable Register */
#define PIOC_PSR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x08)) /* (PIOC) PIO Status Register */
#define PIOC_OER         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x10)) /* (PIOC) Output Enable Register */
#define PIOC_ODR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x14)) /* (PIOC) Output Disable Registerr */
#define PIOC_OSR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x18)) /* (PIOC) Output Status Register */
#define PIOC_IFER        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x20)) /* (PIOC) Input Filter Enable Register */
#define PIOC_IFDR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x24)) /* (PIOC) Input Filter Disable Register */
#define PIOC_IFSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x28)) /* (PIOC) Input Filter Status Register */
#define PIOC_SODR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x30)) /* (PIOC) Set Output Data Register */
#define PIOC_CODR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x34)) /* (PIOC) Clear Output Data Register */
#define PIOC_ODSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x38)) /* (PIOC) Output Data Status Register */
#define PIOC_PDSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x3C)) /* (PIOC) Pin Data Status Register */
#define PIOC_IER         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x40)) /* (PIOC) Interrupt Enable Register */
#define PIOC_IDR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x44)) /* (PIOC) Interrupt Disable Register */
#define PIOC_IMR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x48)) /* (PIOC) Interrupt Mask Register */
#define PIOC_ISR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x4C)) /* (PIOC) Interrupt Status Register */
#define PIOC_MDER        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x50)) /* (PIOC) Multi-driver Enable Register */
#define PIOC_MDDR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x54)) /* (PIOC) Multi-driver Disable Register */
#define PIOC_MDSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x58)) /* (PIOC) Multi-driver Status Register */
#define PIOC_PPUDR       (*(volatile CPU_INT32U *)(PIOC_BASE + 0x60)) /* (PIOC) Pull-up Disable Register */
#define PIOC_PPUER       (*(volatile CPU_INT32U *)(PIOC_BASE + 0x64)) /* (PIOC) Pull-up Enable Register */
#define PIOC_PPUSR       (*(volatile CPU_INT32U *)(PIOC_BASE + 0x68)) /* (PIOC) Pull-up Status Register */
#define PIOC_ASR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x70)) /* (PIOC) Select A Register */
#define PIOC_BSR         (*(volatile CPU_INT32U *)(PIOC_BASE + 0x74)) /* (PIOC) Select B Register */
#define PIOC_ABSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x78)) /* (PIOC) AB Select Status Register */
#define PIOC_OWER        (*(volatile CPU_INT32U *)(PIOC_BASE + 0xA0)) /* (PIOC) Output Write Enable Register */
#define PIOC_OWDR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0xA4)) /* (PIOC) Output Write Disable Register */
#define PIOC_OWSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0xA8)) /* (PIOC) Output Write Status Register */

#if 1
#define PERIPHAL_ID_PIOA        (2)  /* Parallel IO Controller A */
#define PERIPHAL_ID_PIOC        (4)  /* Parallel IO Controller C, D, E */
#endif

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/



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
static const Pin pPinsNf[] = {PINS_NANDFLASH};

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

static CPU_INT16U * _pCurrentNANDAddr;

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
*                                        FSDev_NAND_BSP_Open()
*
* Description : Open (initialize) bus for NAND.
*
* Argument(s) : unit_nbr        Unit number of NAND.
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

CPU_BOOLEAN  FSDev_NAND_BSP_Open (FS_QTY      unit_nbr,
                                  CPU_INT08U  bus_width)
{
   #if 0
  /* */
  /* Update external bus interface, static memory controller */
  /* */
  MATRIX_EBICSA |= (1 << 3); /* Assign CS3 for use with NAND flash */
  SMC0_SETUP3 = 0x00000101;
  SMC0_PULSE3 = 0x03030303;
  SMC0_CYCLE3 = 0x00050005;
  SMC0_CTRL3  = (0 << 12)     /* DBW: 0: 8-bit NAND, 1: 16-bit NAND */
             |  (3 <<  0)     /* Use NRD & NWE signals for read / write */
             |  (2 << 16)     /* Add 2 cycles for data float time */
             |  (0 << 20)
                 ;
  /* */
  /* Enable clocks for PIOA, PIOD */
  /* */
  PMC_PCER = ((1 << PERIPHAL_ID_PIOC) |  (1 << PERIPHAL_ID_PIOA));
  /* */
  /*  Set PIOA pin 22 as port pin, for use as NAND Ready/Busy line. */
  /* */
  PIOA_ODR  = 1 << 22;    /* Configure input */
  PIOA_PER  = 1 << 22;    /* Set pin as port pin */

  /* */
  /*  Set PIOD pin 15 as port pin (output), for use as NAND CS. */
  /* */
  PIOC_SODR = 1 << 14;    /* Set pin high */
  PIOC_OER  = 1 << 14;    /* Configure as output */
  PIOC_PER  = 1 << 14;    /* Set pin as port pin */
#endif
    BOARD_ConfigureNandFlash(8); 
    PIO_Configure(pPinsNf, PIO_LISTSIZE(pPinsNf));

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_Close()
*
* Description : Close (uninitialize) bus for NAND.
*
* Argument(s) : unit_nbr    Unit number of NAND.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

void  FSDev_NAND_BSP_Close (FS_QTY  unit_nbr)
{

}


/*
*********************************************************************************************************
*                                     FSDev_NAND_BSP_ChipSelEn()
*
* Description : Enable NAND chip select/enable.
*
* Argument(s) : unit_nbr    Unit number of NAND.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_NAND_BSP_ChipSelEn (FS_QTY  unit_nbr)
{
    PIOC_CODR = (1 << 14); /* Enable NAND CE */
}


/*
*********************************************************************************************************
*                                     FSDev_NAND_BSP_ChipSelDis()
*
* Description : Disable NAND chip select/enable.
*
* Argument(s) : unit_nbr    Unit number of NAND.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_NAND_BSP_ChipSelDis (FS_QTY  unit_nbr)
{
    PIOC_SODR = (1 << 14); /* Disable NAND CE */
}

/*********************************************************************
*
*       FS_NAND_HW_X_SetData
*/
void FS_NAND_HW_X_SetDataMode(CPU_INT08U Unit) {
  
  /* CLE low, ALE low */
  _pCurrentNANDAddr = NAND_DATA;
}


/*********************************************************************
*
*       FS_NAND_HW_X_SetCmd
*/
void FS_NAND_HW_X_SetCmdMode(CPU_INT08U Unit) {
  
  /*CLE high, ALE low */
  _pCurrentNANDAddr = NAND_CMD;
}

/*********************************************************************
*
*       FS_NAND_HW_X_SetAddr
*/
void FS_NAND_HW_X_SetAddrMode(CPU_INT08U Unit) {
  
  /* CLE low, ALE high */
  _pCurrentNANDAddr = NAND_ADDR;
}


/*
*********************************************************************************************************
*                                         FSDev_NAND_BSP_Rd()
*
* Description : Read data from NAND.
*
* Argument(s) : unit_nbr    Unit number of NAND.
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

void  FSDev_NAND_BSP_RdData (FS_QTY       unit_nbr,
                             void        *p_dest,
                             CPU_SIZE_T   cnt)
{

  FS_NAND_HW_X_SetDataMode(unit_nbr);  
  Mem_Copy(p_dest, _pCurrentNANDAddr, cnt);
  
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_WrAddr()
*
* Description : Write address to NAND.
*
* Argument(s) : unit_nbr    Unit number of NAND.
*
*               p_addr      Pointer to buffer that holds address.
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

void  FSDev_NAND_BSP_WrAddr (FS_QTY       unit_nbr,
                             CPU_INT08U  *p_addr,
                             CPU_SIZE_T   cnt)
{
    FS_NAND_HW_X_SetAddrMode(unit_nbr); 
    Mem_Copy(_pCurrentNANDAddr, p_addr, cnt);
     FS_NAND_HW_X_SetDataMode(unit_nbr); 
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_WrCmd()
*
* Description : Write command to NAND.
*
* Argument(s) : unit_nbr    Unit number of NAND.
*
*               p_cmd       Pointer to buffer that holds command.
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

void  FSDev_NAND_BSP_WrCmd (FS_QTY       unit_nbr,
                            CPU_INT08U  *p_cmd,
                            CPU_SIZE_T   cnt)
{
    FS_NAND_HW_X_SetCmdMode(unit_nbr); 
    Mem_Copy(_pCurrentNANDAddr, p_cmd, cnt);
     FS_NAND_HW_X_SetDataMode(unit_nbr); 
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_BSP_WrData()
*
* Description : Write data to NAND.
*
* Argument(s) : unit_nbr    Unit number of NAND.
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

void  FSDev_NAND_BSP_WrData (FS_QTY       unit_nbr,
                             void        *p_src,
                             CPU_SIZE_T   cnt)
{
    FS_NAND_HW_X_SetDataMode(unit_nbr); 
    Mem_Copy(_pCurrentNANDAddr, p_src, cnt);  
}


/*
*********************************************************************************************************
*                                   FSDev_NAND_BSP_WaitWhileBusy()
*
* Description : Wait while NAND is busy.
*
* Argument(s) : unit_nbr    Unit number of NAND.
*
*               p_phy_data  Pointer to NAND phy data.
*
*               poll_fnct   Pointer to function to poll, if there is no harware ready/busy signal.
*
*               to_us       Timeout, in microseconds.
*
* Return(s)   : DEF_OK,   if NAND became ready.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_NAND_BSP_WaitWhileBusy (FS_QTY                  unit_nbr,
                                           FS_DEV_NAND_PHY_DATA   *p_phy_data,
                                           CPU_BOOLEAN           (*poll_fnct)(FS_DEV_NAND_PHY_DATA  *),
                                           CPU_INT32U              to_us)
{
    CPU_INT32U   time_cur_us;
    CPU_INT32U   time_start_us;
    CPU_BOOLEAN  rdy;

#if 0

    time_cur_us   = /* $$$$ GET CURRENT TIME, IN MICROSECONDS. */0u;
    time_start_us = time_cur_us;

    while (time_cur_us - time_start_us < to_us) {
        rdy = poll_fnct(p_phy_data);
        if (rdy == DEF_OK) {
            return (DEF_OK);
        }
        time_cur_us = /* $$$$ GET CURRENT TIME, IN MICROSECONDS. */0u;
    }
#endif
    while((PIOC_PDSR & (1 << 8)) == 0);
  
    return (DEF_OK);
}
