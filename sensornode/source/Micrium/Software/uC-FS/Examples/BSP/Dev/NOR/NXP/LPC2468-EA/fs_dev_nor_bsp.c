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
*                                          NOR FLASH DEVICES
*
*                                BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                    NXP LPC2468 ON THE EMBEDDED ARTISTS LPC2468 EVALUATION BOARD
*
* Filename      : fs_dev_nor_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The Embedded Artists LPC2468 evaluation board has a 32-Mb SST SST39VF3201 NOR
*                     located on CS0.  The following NOR configuration parameters should be used :
*
*                         nor_cfg.AddrBase    = 0x80000000;     // Bus address of CS0 memory.
*                         nor_cfg.RegionNbr   = 0;              // SST39 has only one region.
*
*                         nor_cfg.PhyPtr      = (FS_DEV_NOR_PHY_API *)&FSDev_NOR_SST39_1x16;
*
*                         nor_cfg.BusWidth    = 16;             // Bus is 16-bit.
*                         nor_cfg.BusWidthMax = 16;             // SST39 is 16-bit.
*                         nor_cfg.PhyDevCnt   = 1;              // Only one SST39.
*
*                     The remaining NOR configuration parameters are based on application requirements.
*
*                 (2) On the revision 1.0 evaluation boards, the address lines were incorrectly
*                     connected between the LPC2468 & the SST39.  The bus address passed to
*                     'FSDev_NOR_BSP_Rd_16()'/'RdWord_16()'/'WrWord_16()' must be corrected for these platforms.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_NOR_BSP_MODULE
#include  <fs.h>
#include  <fs_dev_nor.h>
#include  <lib_mem.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  LPC2XXX_PERIPH_ID_EMC                            11u

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  LPC2XXX_REG_PCONP                          (*(CPU_REG32 *)(0xE01FC0C4u))

#define  LPC24XX_REG_PINSEL0                        (*(CPU_REG32 *)(0xE002C000u))
#define  LPC24XX_REG_PINSEL1                        (*(CPU_REG32 *)(0xE002C004u))
#define  LPC24XX_REG_PINSEL2                        (*(CPU_REG32 *)(0xE002C008u))
#define  LPC24XX_REG_PINSEL3                        (*(CPU_REG32 *)(0xE002C00Cu))
#define  LPC24XX_REG_PINSEL4                        (*(CPU_REG32 *)(0xE002C010u))
#define  LPC24XX_REG_PINSEL5                        (*(CPU_REG32 *)(0xE002C014u))
#define  LPC24XX_REG_PINSEL6                        (*(CPU_REG32 *)(0xE002C018u))
#define  LPC24XX_REG_PINSEL7                        (*(CPU_REG32 *)(0xE002C01Cu))
#define  LPC24XX_REG_PINSEL8                        (*(CPU_REG32 *)(0xE002C020u))
#define  LPC24XX_REG_PINSEL9                        (*(CPU_REG32 *)(0xE002C024u))

#define  LPC24XX_REG_EMCCONTROL                     (*(CPU_REG32 *)(0xFFE08000u))
#define  LPC24XX_REG_EMCSTATUS                      (*(CPU_REG32 *)(0xFFE08004u))
#define  LPC24XX_REG_EMCCONFIG                      (*(CPU_REG32 *)(0xFFE08008u))

#define  LPC24XX_REG_EMCSTATICCNFG0                 (*(CPU_REG32 *)(0xFFE08200u))
#define  LPC24XX_REG_EMCSTATICWAITWEN0              (*(CPU_REG32 *)(0xFFE08204u))
#define  LPC24XX_REG_EMCSTATICWAITOEN0              (*(CPU_REG32 *)(0xFFE08208u))
#define  LPC24XX_REG_EMCSTATICWAITRD0               (*(CPU_REG32 *)(0xFFE0820Cu))
#define  LPC24XX_REG_EMCSTATICWAITPG0               (*(CPU_REG32 *)(0xFFE08210u))
#define  LPC24XX_REG_EMCSTATICWAITWR0               (*(CPU_REG32 *)(0xFFE08214u))
#define  LPC24XX_REG_EMCSTATICWAITTURN0             (*(CPU_REG32 *)(0xFFE08218u))


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


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      FILE SYSTEM NOR FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        FSDev_NOR_BSP_Open()
*
* Description : Open (initialize) bus for NOR.
*
* Argument(s) : unit_nbr        Unit number of NOR.
*
*               addr_base       Base address of NOR.
*
*               bus_width       Bus width, in bits.
*
*               phy_dev_cnt     Number of devices interleaved.
*
* Return(s)   : DEF_OK,   if interface was opened.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is opened.
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_NOR_BSP_Open (FS_QTY      unit_nbr,
                                 CPU_ADDR    addr_base,
                                 CPU_INT08U  bus_width,
                                 CPU_INT08U  phy_dev_cnt)
{
    CPU_INT32U  pinsel;
    CPU_INT32U  emcstaticcnfg;


    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_NOR_BSP_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

    LPC24XX_REG_EMCCONTROL  = 0x00000001u;                  /* Dis addr mirror.                                     */
                                                            /* Turn on EMC PCLK                                     */
    LPC2XXX_REG_PCONP      |= DEF_BIT(LPC2XXX_PERIPH_ID_EMC);

    pinsel                  =  LPC24XX_REG_PINSEL4;         /* Cfg P2[14] as nCS2, P2[15] as nCS3.                  */
    pinsel                 &= ~0xF0000000u;
    pinsel                 |=  0x50000000u;
    LPC24XX_REG_PINSEL4     =  pinsel;

                                                            /* Cfg P2[16..21] as nCAS, nRAS, CLKOUT0, CLKOU1, ...   */
                                                            /*                   nDYCS0, nDYCS1.                    */
                                                            /* Cfg P2[24..25] as CKEOUT0, CKE0UT1.                  */
    pinsel                  =  LPC24XX_REG_PINSEL5;         /* Cfg P2[28..29] as DQMOUT0, DQMOUT1.                  */
    pinsel                 &= ~0x0F0F0FFFu;
    pinsel                 |=  0x05050555u;
    pinsel                 &= ~0x00000C00u;
    LPC24XX_REG_PINSEL5     =  pinsel;

    LPC24XX_REG_PINSEL6     =  0x55555555u;                 /* Cfg P3[0..15] as D0..15.                             */

    LPC24XX_REG_PINSEL8     =  0x55555555u;                 /* Cfg P4[0..15] as A0..15.                             */

                                                            /* Cfg P4[16..23] as A16..23.                           */
                                                            /* Cfg P4[24..27] as nOE, nWE, BLS0, BLS1.              */
    pinsel                  =  LPC24XX_REG_PINSEL9;         /* Cfg P4[30..31] as nCS0, nCS1.                        */
    pinsel                 &= ~0xF0FFFFFFu;
    pinsel                 |=  0x50555555u;
    LPC24XX_REG_PINSEL9     =  pinsel;

    FS_OS_Dly_ms(100u);

    if (bus_width == 8u) {
        emcstaticcnfg = 0x00000080u;
    } else if (bus_width == 16u) {
        emcstaticcnfg = 0x00000081u;
    } else {
        emcstaticcnfg = 0x00000082u;
    }

    LPC24XX_REG_EMCSTATICCNFG0     = emcstaticcnfg;
    LPC24XX_REG_EMCSTATICWAITWEN0  = 0x02u;
    LPC24XX_REG_EMCSTATICWAITOEN0  = 0x02u;
    LPC24XX_REG_EMCSTATICWAITRD0   = 0x1Fu;
    LPC24XX_REG_EMCSTATICWAITPG0   = 0x1Fu;
    LPC24XX_REG_EMCSTATICWAITWR0   = 0x1Fu;
    LPC24XX_REG_EMCSTATICWAITTURN0 = 0x0Fu;

    FS_OS_Dly_ms(100u);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        FSDev_NOR_BSP_Close()
*
* Description : Close (uninitialize) bus for NOR.
*
* Argument(s) : unit_nbr    Unit number of NOR.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

void  FSDev_NOR_BSP_Close (FS_QTY  unit_nbr)
{
    (void)&unit_nbr;                                            /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                        FSDev_NOR_BSP_Rd_XX()
*
* Description : Read from bus interface.
*
* Argument(s) : unit_nbr    Unit number of NOR.
*
*               p_dest      Pointer to destination memory buffer.
*
*               addr_src    Source address.
*
*               cnt         Number of words to read.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) Data should be read from the bus in words sized to the data bus; for any unit, only
*                   the function with that access width will be called.
*********************************************************************************************************
*/

void  FSDev_NOR_BSP_Rd_08 (FS_QTY       unit_nbr,
                           void        *p_dest,
                           CPU_ADDR     addr_src,
                           CPU_INT32U   cnt)
{

}

void  FSDev_NOR_BSP_Rd_16 (FS_QTY       unit_nbr,
                           void        *p_dest,
                           CPU_ADDR     addr_src,
                           CPU_INT32U   cnt)
{
#ifdef  LPC24XX_EA_VERSION_1_0
    CPU_ADDR     addr_src_phy;
    CPU_INT08U  *p_dest_08;
    CPU_INT08U   datum;


    addr_src_phy = addr_src & 0x0FFFFFu;
    switch (addr_src & 0x300000u) {
        case 0x100000u:
             addr_src_phy |= 0x80400000u;
             break;

        case 0x200000u:
             addr_src_phy |= 0x80200000u;
             break;

        case 0x300000u:
             addr_src_phy |= 0x80600000u;
             break;

        default:
             addr_src_phy |= 0x80000000u;
             break;
    }

    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
         datum = *(CPU_REG16 *)addr_src_phy;
         MEM_VAL_SET_INT16U((void *)p_dest_08, datum);
         p_dest_08    += 2;
         addr_src_phy += 2u;
         cnt--;
    }


#else
    CPU_INT08U  *p_dest_08;
    CPU_INT16U   datum;


    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
         datum = *(CPU_REG16 *)addr_src;
         MEM_VAL_SET_INT16U((void *)p_dest_08, datum);
         p_dest_08 += 2;
         addr_src  += 2u;
         cnt--;
    }
#endif
}


/*
*********************************************************************************************************
*                                      FSDev_NOR_BSP_RdWord_XX()
*
* Description : Read word from bus interface.
*
* Argument(s) : unit_nbr    Unit number of NOR.
*
*               addr_src    Source address.
*
* Return(s)   : Word read.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) Data should be read from the bus in words sized to the data bus; for any unit, only
*                   the function with that access width will be called.
*********************************************************************************************************
*/

CPU_INT08U  FSDev_NOR_BSP_RdWord_08 (FS_QTY    unit_nbr,
                                     CPU_ADDR  addr_src)
{
#ifdef  LPC24XX_EA_VERSION_1_0
    CPU_ADDR    addr_src_phy;
    CPU_INT08U  datum;


    addr_src_phy = addr_src & 0x0FFFFFu;
    switch (addr_src & 0x300000u) {
        case 0x100000u:
             addr_src_phy |= 0x80400000u;
             break;

        case 0x200000u:
             addr_src_phy |= 0x80200000u;
             break;

        case 0x300000u:
             addr_src_phy |= 0x80600000u;
             break;

        default:
             addr_src_phy |= 0x80000000u;
             break;
    }

    datum = *(CPU_REG08 *)addr_src_phy;
    return (datum);


#else
    CPU_INT08U  datum;


    datum = *(CPU_REG08 *)addr_src;
    return (datum);
#endif
}

CPU_INT16U  FSDev_NOR_BSP_RdWord_16 (FS_QTY    unit_nbr,
                                     CPU_ADDR  addr_src)
{
#ifdef  LPC24XX_EA_VERSION_1_0
    CPU_ADDR    addr_src_phy;
    CPU_INT16U  datum;


    addr_src_phy = addr_src & 0x0FFFFFu;
    switch (addr_src & 0x300000u) {
        case 0x100000u:
             addr_src_phy |= 0x80400000u;
             break;

        case 0x200000u:
             addr_src_phy |= 0x80200000u;
             break;

        case 0x300000u:
             addr_src_phy |= 0x80600000u;
             break;

        default:
             addr_src_phy |= 0x80000000u;
             break;
    }

    datum = *(CPU_REG16 *)addr_src_phy;
    return (datum);


#else
    CPU_INT16U  datum;


    datum = *(CPU_REG16 *)addr_src;
    return (datum);
#endif
}


/*
*********************************************************************************************************
*                                      FSDev_NOR_BSP_WrWord_XX()
*
* Description : Write word to bus interface.
*
* Argument(s) : unit_nbr    Unit number of NOR.
*
*               addr_dest   Destination address.
*
*               datum       Word to write.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) Data should be written to the bus in words sized to the data bus; for any unit, only
*                   the function with that access width will be called.
*********************************************************************************************************
*/

void  FSDev_NOR_BSP_WrWord_08 (FS_QTY      unit_nbr,
                               CPU_ADDR    addr_dest,
                               CPU_INT08U  datum)
{

}

void  FSDev_NOR_BSP_WrWord_16 (FS_QTY      unit_nbr,
                               CPU_ADDR    addr_dest,
                               CPU_INT16U  datum)
{
    (void)&unit_nbr;                                            /* Prevent compiler warning.                            */

#ifdef  LPC24XX_EA_VERSION_1_0
    CPU_ADDR  addr_dest_phy;


    addr_dest_phy = addr_dest & 0x0FFFFFu;
    switch (addr_dest & 0x300000u) {
        case 0x100000u:
             addr_dest_phy |= 0x80400000u;
             break;

        case 0x200000u:
             addr_dest_phy |= 0x80200000;
             break;

        case 0x300000u:
             addr_dest_phy |= 0x80600000u;
             break;

        default:
             addr_dest_phy |= 0x80000000u;
             break;
    }

   *(CPU_REG16 *)addr_dest_phy = datum;


#else
   *(CPU_REG16 *)addr_dest     = datum;
#endif
}


/*
*********************************************************************************************************
*                                    FSDev_NOR_BSP_WaitWhileBusy()
*
* Description : Wait while NOR is busy.
*
* Argument(s) : unit_nbr    Unit number of NOR.
*
*               p_phy_data  Pointer to NOR phy data.
*
*               poll_fnct   Pointer to function to poll, if there is no harware ready/busy signal.
*
*               to_us       Timeout, in microseconds.
*
* Return(s)   : DEF_OK,   if NOR became ready.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_NOR_BSP_WaitWhileBusy (FS_QTY                 unit_nbr,
                                          FS_DEV_NOR_PHY_DATA   *p_phy_data,
                                          CPU_BOOLEAN          (*poll_fnct)(FS_DEV_NOR_PHY_DATA  *),
                                          CPU_INT32U             to_us)
{
    CPU_INT32U   time_cur_us;
    CPU_INT32U   time_start_us;
    CPU_BOOLEAN  rdy;


    time_cur_us   = /* $$$$ GET CURRENT TIME, IN MICROSECONDS. */0u;
    time_start_us = time_cur_us;

    while (time_cur_us - time_start_us < to_us) {
        rdy = poll_fnct(p_phy_data);
        if (rdy == DEF_OK) {
            return (DEF_OK);
        }
        time_cur_us = /* $$$$ GET CURRENT TIME, IN MICROSECONDS. */0u;
    }

    return (DEF_FAIL);
}
