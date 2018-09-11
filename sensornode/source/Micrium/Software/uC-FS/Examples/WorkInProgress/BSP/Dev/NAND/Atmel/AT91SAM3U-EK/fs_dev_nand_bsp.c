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
*                                              TEMPLATE
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


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define NAND_BASE_ADDR                          0x60000000
#define NAND_DATA                       (CPU_INT16U *)(NAND_BASE_ADDR + 0x1000000)
#define NAND_ADDR                       (CPU_INT16U *)(NAND_BASE_ADDR + 0x1200000)
#define NAND_CMD                        (CPU_INT16U *)(NAND_BASE_ADDR + 0x1400000)

#define NAND_CE_PIN                                     12
#define NAND_RB_PIN                                     24

#define PERIPHAL_ID_SMC         ( 9)  /* Static memory controller */
#define PERIPHAL_ID_PIOA        (10)  /* Parallel IO Controller A */
#define PERIPHAL_ID_PIOB        (11)  /* Parallel IO Controller B */
#define PERIPHAL_ID_PIOC        (12)  /* Parallel IO Controller C */

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define SMC_BASE_ADDR    0x400E0000
#define SMC_SETUP_CS1       (*(volatile CPU_INT32U*)(SMC_BASE_ADDR + 1 * 0x14 + 0x70 + 0x00))  /* SMC CS1 Setup Register */
#define SMC_PULSE_CS1       (*(volatile CPU_INT32U*)(SMC_BASE_ADDR + 1 * 0x14 + 0x70 + 0x04))  /* SMC CS1 Pulse Register */
#define SMC_CYCLE_CS1       (*(volatile CPU_INT32U*)(SMC_BASE_ADDR + 1 * 0x14 + 0x70 + 0x08))  /* SMC CS1 Cycle Register */
#define SMC_TIMINGS_CS1     (*(volatile CPU_INT32U*)(SMC_BASE_ADDR + 1 * 0x14 + 0x70 + 0x0C))  /* SMC CS1 Timing Register */
#define SMC_MODE_CS1        (*(volatile CPU_INT32U*)(SMC_BASE_ADDR + 1 * 0x14 + 0x70 + 0x10))  /* SMC CS1 Mode  Register */

#define PMC_BASE_ADDR    0x400E0400
#define PMC_PCER         (*(volatile CPU_INT32U *)(PMC_BASE_ADDR + 0x10)) /* (PMC) Peripheral Clock Enable Register */
#define PMC_PCDR         (*(volatile CPU_INT32U *)(PMC_BASE_ADDR + 0x14)) /* (PMC) Peripheral Clock Disable Register */

/* ========== Register definition for PIOB peripheral ========== */
#define PIOB_BASE        0x400E0E00
#define PIOB_PER         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x00)) /* (PIOB) PIO Enable Register */
#define PIOB_PDR         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x04)) /* (PIOB) PIO Disable Register */
#define PIOB_PSR         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x08)) /* (PIOB) PIO Status Register */
#define PIOB_OER         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x10)) /* (PIOB) Output Enable Register */
#define PIOB_ODR         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x14)) /* (PIOB) Output Disable Registerr */
#define PIOB_OSR         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x18)) /* (PIOB) Output Status Register */
#define PIOB_IFER        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x20)) /* (PIOB) Input Filter Enable Register */
#define PIOB_IFDR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x24)) /* (PIOB) Input Filter Disable Register */
#define PIOB_IFSR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x28)) /* (PIOB) Input Filter Status Register */
#define PIOB_SODR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x30)) /* (PIOB) Set Output Data Register */
#define PIOB_CODR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x34)) /* (PIOB) Clear Output Data Register */
#define PIOB_ODSR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x38)) /* (PIOB) Output Data Status Register */
#define PIOB_PDSR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x3C)) /* (PIOB) Pin Data Status Register */
#define PIOB_IER         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x40)) /* (PIOB) Interrupt Enable Register */
#define PIOB_IDR         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x44)) /* (PIOB) Interrupt Disable Register */
#define PIOB_IMR         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x48)) /* (PIOB) Interrupt Mask Register */
#define PIOB_ISR         (*(volatile CPU_INT32U *)(PIOB_BASE + 0x4C)) /* (PIOB) Interrupt Status Register */
#define PIOB_MDER        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x50)) /* (PIOB) Multi-driver Enable Register */
#define PIOB_MDDR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x54)) /* (PIOB) Multi-driver Disable Register */
#define PIOB_MDSR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x58)) /* (PIOB) Multi-driver Status Register */
#define PIOB_PPUDR       (*(volatile CPU_INT32U *)(PIOB_BASE + 0x60)) /* (PIOB) Pull-up Disable Register */
#define PIOB_PPUER       (*(volatile CPU_INT32U *)(PIOB_BASE + 0x64)) /* (PIOB) Pull-up Enable Register */
#define PIOB_PPUSR       (*(volatile CPU_INT32U *)(PIOB_BASE + 0x68)) /* (PIOB) Pull-up Status Register */
#define PIOB_ABSR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0x70)) /* (PIOB) AB Select Status Register */
#define PIOB_OWER        (*(volatile CPU_INT32U *)(PIOB_BASE + 0xA0)) /* (PIOB) Output Write Enable Register */
#define PIOB_OWDR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0xA4)) /* (PIOB) Output Write Disable Register */
#define PIOB_OWSR        (*(volatile CPU_INT32U *)(PIOB_BASE + 0xA8)) /* (PIOB) Output Write Status Register */


/* ========== Register definition for PIOC peripheral ========== */
#define PIOC_BASE        0x400E1000
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
#define PIOC_ABSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0x70)) /* (PIOC) AB Select Status Register */
#define PIOC_OWER        (*(volatile CPU_INT32U *)(PIOC_BASE + 0xA0)) /* (PIOC) Output Write Enable Register */
#define PIOC_OWDR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0xA4)) /* (PIOC) Output Write Disable Register */
#define PIOC_OWSR        (*(volatile CPU_INT32U *)(PIOC_BASE + 0xA8)) /* (PIOC) Output Write Status Register */


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
    /* */
    /* Enable clock for SMC */
    /* */
    PMC_PCER        = (1 << PERIPHAL_ID_SMC);
    SMC_SETUP_CS1   = 0x00010001;
    SMC_PULSE_CS1   = 0x04030302;
    SMC_CYCLE_CS1   = 0x00070005;
    SMC_TIMINGS_CS1 = (1 <<  0)                      /* CLE to REN */
                      | (2 <<  4)                      /* ALE to Data */
                      | (1 <<  8)                      /* ALE to REN */
                      | (1 << 16)                      /* Ready to REN */
                      | (2 << 24)                      /* WEN to REN */
                      | (7 << 28)                      /* Ready/Busy Line Selection */
                      | (1 << 31)                      /* Use Nand Flash Timing */
                      ;
    SMC_MODE_CS1    =  (0 << 12)     /* DBW: 0: 8-bit NAND, 1: 16-bit NAND */
                      |  (3 <<  0);    /* Use NRD & NWE signals for read / write */
    
    /* */
    /* Enable clocks for PIOA, PIOD */
    /* */
    PMC_PCER = ((1 << PERIPHAL_ID_PIOB) |  (1 << PERIPHAL_ID_PIOC));
    
    /* */
    /*  Set PIO pin x as port pin, for use as NAND Ready/Busy line. */
    /* */
    PIOB_ODR  = 1 << NAND_RB_PIN;    /* Configure input */
    PIOB_PER  = 1 << NAND_RB_PIN;    /* Set pin as port pin */
    
    /* */
    /*  Set PIO pin x as port pin (output), for use as NAND CS. */
    /* */
    PIOC_SODR = 1 << NAND_CE_PIN;    /* Set pin high */
    PIOC_OER  = 1 << NAND_CE_PIN;    /* Configure as output */
    PIOC_PER  = 1 << NAND_CE_PIN;    /* Set pin as port pin */
    
    /* */
    /*  Set different port pins to alternate function */
    /* */
    PIOB_ABSR &= ~((1 << 17)              /* PIOx.pin17  -> NAND_OE */
                  |  (1 << 18)            /* PIOx.pin18  -> NAND_WE */
                  |  (1 << 21)            /* PIOx.pin21  -> NAND_ALE */
                  |  (1 << 22)            /* PIOx.pin22  -> NAND_CLE */
                  )
               ;
    PIOB_PDR =  (1 << 17)            /* PIOC.pin17  -> NAND_OE */
             |  (1 << 18)            /* PIOx.pin18  -> NAND_WE */
             |  (1 << 21)            /* PIOx.pin21  -> NAND_ALE */
             |  (1 << 22)            /* PIOx.pin22  -> NAND_CLE */
             ;
    PIOB_ABSR &= ~0xfe01fe00;           /* Set pins to alternate function A -> databus */
    PIOB_ABSR = (1 << 6);             /* Set pins to alternate function B -> databus */
    PIOB_PDR =  0xfe01fe00           /* Disable port pin function */
             |  (1 <<  6)            /* Disable port pin function */
             ;
  
    SMC_MODE_CS1  = (1 << 12)     /* DBW: 0: 8-bit NAND, 1: 16-bit NAND */
                | (3 << 0);     /* Use NRD & NWE signals for read / write */
    
    return DEF_OK;
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
    PIOC_CODR = (1 << NAND_CE_PIN); /* Enable NAND CE */
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
    PIOC_SODR = (1 << NAND_CE_PIN); /* Disable NAND CE */
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
    /* Set Data Mode */
    /* CLE low, ALE low */
    _pCurrentNANDAddr = NAND_DATA;

   // CPU_INT16U *tmp = _pCurrentNANDAddr;
    
   // printf (" %02X \r\n",*_pCurrentNANDAddr);
    Mem_Copy(p_dest, _pCurrentNANDAddr, cnt);
  //  CPU_INT16U tmp[512];
  // Mem_Copy(tmp, p_dest, cnt);
   
#if 0  
   CPU_INT16U * p;

  p = (CPU_INT16U*) p_dest;
  cnt >>= 1;
  while (cnt--) {
    *p++ = *_pCurrentNANDAddr;
  }
#endif
  
}

void FSDev_NAND_BSP_WrBytes(void *addr, CPU_SIZE_T cnt)
{
  // CPU_INT16U tmp[512];
 //  Mem_Copy(tmp, addr, cnt);
   
   Mem_Copy(_pCurrentNANDAddr, addr, cnt); 
#if 0   
   CPU_INT16U * p;

  p = (CPU_INT16U*) addr;
  cnt++;
  cnt >>= 1;
  while (cnt--) {
    *_pCurrentNANDAddr = *p++;
  }
#endif
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
    /* Set Addr Mode */
    /* CLE low, ALE high */
    _pCurrentNANDAddr = NAND_ADDR;
    
    FSDev_NAND_BSP_WrBytes((void*)p_addr, cnt);  
    _pCurrentNANDAddr = NAND_DATA;

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
    /* Set Cmd mode */ 
    /*CLE high, ALE low */
    _pCurrentNANDAddr = NAND_CMD;
    
     FSDev_NAND_BSP_WrBytes((void*)p_cmd, cnt);
    _pCurrentNANDAddr = NAND_DATA;

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
     /* Set Data Mode */
    /* CLE low, ALE low */
    _pCurrentNANDAddr = NAND_DATA;
     
    FSDev_NAND_BSP_WrBytes((void*)p_src, cnt);
   

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

    while ((PIOB_PDSR & (1 << NAND_RB_PIN)) == 0);
  
    
#if 0
    time_cur_us   = /* $$$$ GET CURRENT TIME, IN MICROSECONDS. */;
    time_start_us = time_cur_us;

    while (time_cur_us - time_start_us < to_us) {
        rdy = poll_fnct(p_phy_data);
        if (rdy == DEF_OK) {
            return (DEF_OK);
        }
        time_cur_us = /* $$$$ GET CURRENT TIME, IN MICROSECONDS. */;
    }
#endif
    return (DEF_OK);
}
