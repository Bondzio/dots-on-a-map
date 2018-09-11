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
*                    NXP LPC2468 ON THE EMBEDDED ARTISTS LPC2468 EVALUATION BOARD
*
* Filename      : fs_dev_nand_bsp.c
* Version       : v4.07.00
* Programmer(s) : FBJ
*********************************************************************************************************
* Note(s)       : (1) The Embedded Artists LPC2468 evaluation board has a 128-Mb Samsung K9F1G08U0A-P NAND
*                     located on CS1, using the external memory controller (EMC). 
*
*                     Busy pin is connected on P2[12]. Jumper must be set accordingly. 
*                     CLE pin is connected on A20
*                     CLE pin is connecter on A19 
*
*                     Memory address starts at 0x81000000
*
*                     Everything else is managed by the EMC
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
/* 
*********************************************************************************************************
* Note(s)       : (1) ALE and CLE pins are respectively connected to A19 and A20 of the EMC. 
*                     Hence, they are set/cleared by masking the 19th and 20th bit of the memory address.
*
*                 (2) To enable data read/write, both ALE and CLE pins on device must be set low.
*                     Bits 19 and 20 of the base address must be cleared.
*
*                 (3) To enable command read/write, CLE must be set high and ALE must be low. 
*                     Bit 19 must be cleared and bit 20 must be set.
*
*                 (4) To enable address read/write, CLE must be set low and ALE must be set high. 
*                     Bit 19 must be set and bit 20 must be cleared.
*
*********************************************************************************************************
*/
#define  LPC2XXX_PERIPH_ID_EMC                            11u

#define  LPC2468_EA_NAND_BASE_ADDR                0x81000000u
#define  LPC2468_EA_NAND_DATA_ADDR               (LPC2468_EA_NAND_BASE_ADDR & 0xFFE7FFFFu)                  /* See note (2) */
#define  LPC2468_EA_NAND_CMD_ADDR               ((LPC2468_EA_NAND_BASE_ADDR & 0xFFF7FFFFu) | 0x00100000u)   /* See note (3) */
#define  LPC2468_EA_NAND_ADDR_ADDR              ((LPC2468_EA_NAND_BASE_ADDR & 0xFFEFFFFFu) | 0x00080000u)   /* See note (4) */

#define  FS_DEV_NAND_BSP_TS_RESOLUTION                   22u


/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  LPC2XXX_REG_PCONP                          (*(volatile CPU_REG32 *)(0xE01FC0C4u))

#define  LPC24XX_REG_PINSEL0                        (*(volatile CPU_REG32 *)(0xE002C000u))
#define  LPC24XX_REG_PINSEL1                        (*(volatile CPU_REG32 *)(0xE002C004u))
#define  LPC24XX_REG_PINSEL2                        (*(volatile CPU_REG32 *)(0xE002C008u))
#define  LPC24XX_REG_PINSEL3                        (*(volatile CPU_REG32 *)(0xE002C00Cu))
#define  LPC24XX_REG_PINSEL4                        (*(volatile CPU_REG32 *)(0xE002C010u))
#define  LPC24XX_REG_PINSEL5                        (*(volatile CPU_REG32 *)(0xE002C014u))
#define  LPC24XX_REG_PINSEL6                        (*(volatile CPU_REG32 *)(0xE002C018u))
#define  LPC24XX_REG_PINSEL7                        (*(volatile CPU_REG32 *)(0xE002C01Cu))
#define  LPC24XX_REG_PINSEL8                        (*(volatile CPU_REG32 *)(0xE002C020u))
#define  LPC24XX_REG_PINSEL9                        (*(volatile CPU_REG32 *)(0xE002C024u))

#define  LPC24XX_REG_PINMODE0                       (*(volatile CPU_REG32 *)(0xE002C040u))
#define  LPC24XX_REG_PINMODE1                       (*(volatile CPU_REG32 *)(0xE002C044u))
#define  LPC24XX_REG_PINMODE2                       (*(volatile CPU_REG32 *)(0xE002C048u))
#define  LPC24XX_REG_PINMODE3                       (*(volatile CPU_REG32 *)(0xE002C04Cu))
#define  LPC24XX_REG_PINMODE4                       (*(volatile CPU_REG32 *)(0xE002C050u))
#define  LPC24XX_REG_PINMODE5                       (*(volatile CPU_REG32 *)(0xE002C054u))
#define  LPC24XX_REG_PINMODE6                       (*(volatile CPU_REG32 *)(0xE002C058u))
#define  LPC24XX_REG_PINMODE7                       (*(volatile CPU_REG32 *)(0xE002C05Cu))
#define  LPC24XX_REG_PINMODE8                       (*(volatile CPU_REG32 *)(0xE002C060u))
#define  LPC24XX_REG_PINMODE9                       (*(volatile CPU_REG32 *)(0xE002C064u))

#define  LPC24XX_REG_FIO2PIN                        (*(volatile CPU_REG32 *)(0x3FFFC054u))

#define  LPC24XX_REG_FIO2DIR                        (*(volatile CPU_REG32 *)(0x3FFFC040u))
#define  LPC24XX_REG_FIO3DIR                        (*(volatile CPU_REG32 *)(0x3FFFC060u))
#define  LPC24XX_REG_FIO4DIR                        (*(volatile CPU_REG32 *)(0x3FFFC080u))

#define  LPC24XX_REG_EMCCONTROL                     (*(volatile CPU_REG32 *)(0xFFE08000u))
#define  LPC24XX_REG_EMCSTATUS                      (*(volatile CPU_REG32 *)(0xFFE08004u))
#define  LPC24XX_REG_EMCCONFIG                      (*(volatile CPU_REG32 *)(0xFFE08008u))

#define  LPC24XX_REG_EMCSTATICCNFG1                 (*(volatile CPU_REG32 *)(0xFFE08220u))
#define  LPC24XX_REG_EMCSTATICWAITWEN1              (*(volatile CPU_REG32 *)(0xFFE08224u))
#define  LPC24XX_REG_EMCSTATICWAITOEN1              (*(volatile CPU_REG32 *)(0xFFE08228u))
#define  LPC24XX_REG_EMCSTATICWAITRD1               (*(volatile CPU_REG32 *)(0xFFE0822Cu))
#define  LPC24XX_REG_EMCSTATICWAITPG1               (*(volatile CPU_REG32 *)(0xFFE08230u))
#define  LPC24XX_REG_EMCSTATICWAITWR1               (*(volatile CPU_REG32 *)(0xFFE08234u))
#define  LPC24XX_REG_EMCSTATICWAITTURN1             (*(volatile CPU_REG32 *)(0xFFE08238u))

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

#define  FS_DEV_NAND_BSP_DATA_RD()             *(volatile CPU_INT08U *)LPC2468_EA_NAND_DATA_ADDR            /* $$$$ Rd  data.               */
#define  FS_DEV_NAND_BSP_DATA_WR(datum)        *(volatile CPU_INT08U *)LPC2468_EA_NAND_DATA_ADDR = (datum)  /* $$$$ Wr  data.               */

#define  FS_DEV_NAND_BSP_ADDR_RD()             *(volatile CPU_INT08U *)LPC2468_EA_NAND_ADDR_ADDR            /* $$$$ Rd  data.               */
#define  FS_DEV_NAND_BSP_ADDR_WR(datum)        *(volatile CPU_INT08U *)LPC2468_EA_NAND_ADDR_ADDR = (datum)  /* $$$$ Wr  data.               */

#define  FS_DEV_NAND_BSP_CMD_RD()              *(volatile CPU_INT08U *)LPC2468_EA_NAND_CMD_ADDR             /* $$$$ Rd  data.               */
#define  FS_DEV_NAND_BSP_CMD_WR(datum)         *(volatile CPU_INT08U *)LPC2468_EA_NAND_CMD_ADDR = (datum)   /* $$$$ Wr  data.               */

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
    CPU_INT32U  pinsel;
    CPU_INT32U  pinmode;
    CPU_INT32U  fiodir;
    CPU_INT32U  emcstaticcnfg;


    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_NAND_BSP_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

    LPC24XX_REG_EMCCONTROL  = 0x00000001u;                  /* Dis addr mirror.                                     */
                                                            /* Turn on EMC PCLK                                     */
    LPC2XXX_REG_PCONP      |= DEF_BIT(LPC2XXX_PERIPH_ID_EMC);

    pinsel                  =  LPC24XX_REG_PINSEL4;         /* Cfg P2[12] as GPIO                                   */
    pinsel                 &= ~0x03000000u;
    LPC24XX_REG_PINSEL4     =  pinsel;

    pinmode                 =  LPC24XX_REG_PINMODE4;
    pinmode                &= ~0x03000000u;
    LPC24XX_REG_PINMODE4   |=  0x02000000u;

    fiodir                  =  LPC24XX_REG_FIO2DIR;         /* Cfg P2[12] as input                                  */
    fiodir                 &= ~0x00001000u;
    LPC24XX_REG_FIO2DIR     =  fiodir;

    pinsel                  =  LPC24XX_REG_PINSEL6;         /* Cfg P3[0..7] as D0..7.                               */
    pinsel                 &= ~0x0000FFFFu;
    pinsel                 |=  0x00005555u;
    LPC24XX_REG_PINSEL6     =  pinsel;

    pinmode                 =  LPC24XX_REG_PINMODE6;
    pinmode                &= ~0x0000FFFFu;
    LPC24XX_REG_PINMODE6   |=  0x0000AAAAu;
                                                            /* Cfg P4[19..20] as A19..20.                           */
                                                            /* Cfg P4[24..25] as nOE, nWE                           */
    pinsel                  =  LPC24XX_REG_PINSEL9;         /* Cfg P4[31] as nCS1.                                  */
    pinsel                 &= ~0x300F03C0u;
    pinsel                 |=  0x10050140u;
    LPC24XX_REG_PINSEL9     =  pinsel;

    pinmode                 =  LPC24XX_REG_PINMODE9;
    pinmode                &= ~0x303F03C0u;
    LPC24XX_REG_PINMODE9   |=  0x202A0280u;
                                                            /* Cfg P4[31] as output                                 */
    fiodir                  =  LPC24XX_REG_FIO4DIR;
    fiodir                 |=  0x80000000u;
    LPC24XX_REG_FIO4DIR     =  fiodir;

    FS_OS_Dly_ms(100u);

    if (bus_width == 8u) {
        emcstaticcnfg = 0x00000080u;
    } else if (bus_width == 16u) {
        emcstaticcnfg = 0x00000081u;
    } else {
        emcstaticcnfg = 0x00000082u;
    }

    LPC24XX_REG_EMCSTATICCNFG1     = emcstaticcnfg;
    LPC24XX_REG_EMCSTATICWAITWEN1  = 0u;
    LPC24XX_REG_EMCSTATICWAITOEN1  = 0u;
    LPC24XX_REG_EMCSTATICWAITRD1   = 1u;
    LPC24XX_REG_EMCSTATICWAITPG1   = 0u;
    LPC24XX_REG_EMCSTATICWAITWR1   = 0u;
    LPC24XX_REG_EMCSTATICWAITTURN1 = 0u;

    FS_OS_Dly_ms(100u);

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
    (void)&unit_nbr;                                        /* Prevent compiler warning.                            */
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
                                                            /* Chip select handled by external memory controller    */
    (void)&unit_nbr;                                        /* Prevent compiler warning                             */
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
                                                            /* Chip select handled by external memory controller    */
    (void)&unit_nbr;                                        /* Prevent compiler warning                             */
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
    volatile  CPU_INT08U  *p_dest_08;


    (void)&unit_nbr;                                        /* Prevent compiler warning                             */

    p_dest_08 = (volatile  CPU_INT08U *)p_dest;

    while (cnt > 0) {
       *p_dest_08 = FS_DEV_NAND_BSP_DATA_RD();

        p_dest_08++;
        cnt--;
    }
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
    volatile  CPU_INT08U  *p_addr_08;


    (void)&unit_nbr;                                        /* Prevent compiler warning                             */
    
    p_addr_08 = (volatile CPU_INT08U *)p_addr;
    
    while (cnt > 0) {
        FS_DEV_NAND_BSP_ADDR_WR(*p_addr_08);

        p_addr_08++;
        cnt--;
    }
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
    volatile  CPU_INT08U  *p_cmd_08;


    (void)&unit_nbr;                                        /* Prevent compiler warning                             */
    
    p_cmd_08 = (volatile CPU_INT08U *)p_cmd;

    while (cnt > 0) {
        FS_DEV_NAND_BSP_CMD_WR(*p_cmd_08);

        p_cmd_08++;
        cnt--;
    }
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
    volatile  CPU_INT08U  *p_src_08;


    (void)&unit_nbr;                                        /* Prevent compiler warning                             */
    
    p_src_08 = (CPU_INT08U *)p_src;

    while (cnt > 0) {
        FS_DEV_NAND_BSP_DATA_WR(*p_src_08);

        p_src_08++;
        cnt--;
    }
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
    CPU_BOOLEAN  mem_rdy;


    (void)&unit_nbr;                                        /* Prevent compiler warning                             */

    mem_rdy = DEF_FAIL;

                                                            /* Adjust timeout more loosely to avoid errors          */
    to_us   = (CPU_INT32U)((to_us * 1.17) + 25u);

    time_cur_us   = Clk_GetTS() * 1000u;
    time_start_us = time_cur_us;
                                                            /* Allow wait time error within resolution              */
    while (time_cur_us - time_start_us < (to_us + FS_DEV_NAND_BSP_TS_RESOLUTION)) {
        if (mem_rdy == DEF_OK) {
            return (DEF_OK);
        }

        mem_rdy = DEF_BIT_IS_SET(LPC24XX_REG_FIO2PIN, DEF_BIT_12);

        time_cur_us = Clk_GetTS() * 1000u;
    }

    return (DEF_FAIL);
}
