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
*                                             DATA FLASH
*
* Filename      : df_rx111.c
* Version       : v4.07.00
* Programmer(s) : JPC
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <KAL/kal.h>
#include  <bsp_sys.h>
#include  <iodefine.h>                                          /* MUST be in BSP, as this is MCU-specific.             */
#include  "fs_dev_dataflash.h"



#define  DATAFLASH_BASE_ADDR                     0x00100000u
#define  DATAFLASH_SEC_SIZE                            1024u
#define  DATAFLASH_SEC_CNT                                8u

#define  FLASH_WAIT_TIMEOUT                          0xFFFFu

#ifdef RX231
#define  DATAFLASH_OFFSET                        0x000FE000000u
#else
#define  DATAFLASH_OFFSET                        0x000F1000u
#endif

#define  DF_CMD_PROGRAM                                0x81u
#define  DF_CMD_BLANK_CHK                              0x83u
#define  DF_CMD_BLK_ERASE                              0x84u


#define  IS_VALID_DATAFLASH_ADDR(addr)        (((CPU_ADDR)(addr) >=  DATAFLASH_BASE_ADDR \
                                            &&  (CPU_ADDR)(addr) <  (DATAFLASH_BASE_ADDR + (DATAFLASH_SEC_SIZE * DATAFLASH_SEC_CNT) - 1)) \
                                            && ((CPU_ADDR)(addr) %   DATAFLASH_SEC_SIZE) == 0)

static  int          DF_Erase         (CPU_INT08U  *p_base);


void  DF_AccessEn (void)
{
    FLASH.DFLCTL.BIT.DFLEN = 1;

    ///@todo wait tDSTOP (see ch. 36) using a h/w delay
    KAL_Dly(1);

                                                                /* Reset the controller.                                */
    FLASH.FRESETR.BIT.FRESET = 1;
    FLASH.FRESETR.BIT.FRESET = 0;
}


void  DF_AccessDis (void)
{
    FLASH.DFLCTL.BIT.DFLEN = 0;

    ///@todo wait tDSTOP (see ch. 36) using a h/w delay
    KAL_Dly(1);
}


CPU_INT08U  *DF_BaseAddrGet (void)
{
    return (CPU_INT08U *)DATAFLASH_BASE_ADDR;
}


FS_SEC_SIZE  DF_SecSizeGet (void)
{
    return (FS_SEC_SIZE)DATAFLASH_SEC_SIZE;
}


FS_SEC_QTY  DF_SecCntGet (void)
{
    return (FS_SEC_QTY)DATAFLASH_SEC_CNT;
}



int  DF_ProgramEraseModeEnter (void)
{
    FLASH.FENTRYR.WORD = 0xAA80;                                /* Set FEKEY to 0xAA enable writing.                    */

    if (SYSTEM.OPCCR.BIT.OPCM == 0) {                           /* High-speed operating mode.                           */
        FLASH.FPR        = 0xA5;
        FLASH.FPMCR.BYTE = 0x10;
        FLASH.FPMCR.BYTE = 0xEF;
        FLASH.FPMCR.BYTE = 0x10;
    } else {                                                    /* Middle-speed operating mode.                         */
        FLASH.FPR        = 0xA5;
        FLASH.FPMCR.BYTE = 0x50;
        FLASH.FPMCR.BYTE = 0xAF;
        FLASH.FPMCR.BYTE = 0x50;
    }

    if (FLASH.FPSR.BIT.PERR == 1) {                             /* Ensure device was successfully put in P/E mode.      */
        return -1;
    }

                                                                /* Set FCLK frequency.                                  */
    DF_FClkFreqSet();

    return 0;
}


int  DF_ProgramEraseModeExit (void)
{
    CPU_INT32U  timeout;


    FLASH.FPR        = 0xA5;
    FLASH.FPMCR.BYTE = 0x08;
    FLASH.FPMCR.BYTE = 0xF7;
    FLASH.FPMCR.BYTE = 0x08;

    timeout = FLASH_WAIT_TIMEOUT;
    while (--timeout);


    FLASH.FENTRYR.WORD = 0xAA00;                                /* Set FEKEY to 0xAA enable writing.                    */

    timeout = FLASH_WAIT_TIMEOUT;
    while (FLASH.FENTRYR.WORD != 0x0000 && timeout != 0) {
        --timeout;
    }

    return 0;
}


void  DF_FClkFreqSet (void)
{
    CPU_INT32U  fclk_freq_mhz;

                                                                /* Get the peripheral clock frequency in MHz.           */
    fclk_freq_mhz = BSP_SysPeriphClkFreqGet(CLK_ID_FCLK) / 1000000;
                                                                /* See table 35.3 in h/w manual.                        */
    FLASH.FISR.BIT.PCKA = (fclk_freq_mhz - 1);
}


int  DF_Program(CPU_INT08U  *p_dst,
                CPU_INT08U  *p_src,
                CPU_SIZE_T   cnt_octets)
{
    int                  i;
    int                  err;
    CPU_REG16            addr_high;
    CPU_REG16            addr_low;
    CPU_REG16            prog_data;
    CPU_ADDR             flash_addr;
    volatile  CPU_INT32U timeout;


    if ( ! IS_VALID_DATAFLASH_ADDR(p_dst)) {
        return -1;
    }
                                                                /* Erase before reprogramming.                          */
    err = DF_Erase(p_dst);
    if (err < 0) {
        return -1;
    }

    err = 0;
                                                                /* Select user/data area for flash programming.         */
    FLASH.FASR.BIT.EXS = 0;

    for (i = 0; i < cnt_octets; ++i) {
        flash_addr = ((CPU_ADDR)p_dst & 0xFFFFF) + DATAFLASH_OFFSET;
#ifdef RX231
                                                                /* Set start address HIGH register (top 8 and bottom 4 ...   */
        addr_high  = (flash_addr >> 16) & 0xFF0F;
#else
                                                                /* Set start address HIGH register (only bottom 4 ...   */
        addr_high  = (flash_addr >> 16) & 0x0F;
#endif
                                                                /* Set start address LOW register.                      */
        addr_low   =  flash_addr        & 0xFFFF;

        FLASH.FSARH = addr_high;
        FLASH.FSARL = addr_low;

                                                                /* Set the write buffer data register (lower 8 bits).   */
        prog_data  = *p_src;
#ifdef RX231
        FLASH.FWB0 =  0;
        FLASH.FWB0 =  prog_data & 0x00FF;
#else
        FLASH.FWBL =  0;
        FLASH.FWBL =  prog_data & 0x00FF;
#endif

                                                                /* Start programming.                                   */
        FLASH.FCR.BYTE = DF_CMD_PROGRAM;

        timeout = FLASH_WAIT_TIMEOUT;
        while (FLASH.FSTATR1.BIT.FRDY != 1 && timeout != 0) {
            --timeout;
        }
                                                                /* Return -1 if timeout or error has occurred.          */
        if (timeout == 0) {
            err = -1;
            break;
        }

        FLASH.FCR.BYTE = 0x00;

        timeout = FLASH_WAIT_TIMEOUT;
        while (FLASH.FSTATR1.BIT.FRDY != 0 && timeout != 0) {
            --timeout;
        }
                                                                /* Return -1 if timeout or error has occurred.          */
        if (timeout == 0 || FLASH.FSTATR0.BIT.ILGLERR == 1 || FLASH.FSTATR0.BIT.PRGERR == 1) {
            err = -1;
            break;
        }

        ++p_dst;
        ++p_src;
    }

                                                                /* Reset the controller.                                */
    FLASH.FRESETR.BIT.FRESET = 1;
    FLASH.FRESETR.BIT.FRESET = 0;

    return err;
}


static  int  DF_Erase (CPU_INT08U  *p_base)
{
    CPU_REG16            start_addr_high;
    CPU_REG16            start_addr_low;
    CPU_REG08            end_addr_high;
    CPU_REG16            end_addr_low;
    CPU_REG16            prog_data;
    CPU_ADDR             flash_addr;
    volatile  CPU_INT32U timeout;

                                                                /* Select user/data area for flash programming.         */
    FLASH.FASR.BIT.EXS = 0;

    flash_addr = ((CPU_ADDR)p_base & 0xFFFFF) + DATAFLASH_OFFSET;
                                                                /* Set start address HIGH register.                     */
#ifdef RX231
    start_addr_high  = (flash_addr >> 16) & 0xFF0F;
#else
    start_addr_high  = (flash_addr >> 16) & 0x0F;
#endif
    FLASH.FSARH      =  start_addr_high;
                                                                /* Set start address LOW register.                      */
    start_addr_low   =  flash_addr & 0xFFFF;
    FLASH.FSARL      =  start_addr_low;

                                                                /* Set end address HIGH register.                       */
    end_addr_high   =  start_addr_high;
    FLASH.FEARH     =  end_addr_high;
                                                                /* Set end address LOW register.                        */
    end_addr_low    = (start_addr_low + DATAFLASH_SEC_SIZE) - 1;
    FLASH.FEARL     =  end_addr_low;



                                                            /* Start programming.                                   */
    FLASH.FCR.BYTE = DF_CMD_BLK_ERASE;

    timeout = FLASH_WAIT_TIMEOUT;
    while (FLASH.FSTATR1.BIT.FRDY != 1 && timeout != 0) {
        --timeout;
    }
                                                            /* Return -1 if timeout or error has occurred.          */
    if (timeout == 0) {
        return -1;
    }

    FLASH.FCR.BYTE = 0x00;

    timeout = FLASH_WAIT_TIMEOUT;
    while (FLASH.FSTATR1.BIT.FRDY != 0 && timeout != 0) {
        --timeout;
    }
                                                            /* Return -1 if timeout or error has occurred.          */
    if (timeout == 0 || FLASH.FSTATR0.BIT.ILGLERR == 1 || FLASH.FSTATR0.BIT.PRGERR == 1) {
        return -1;
    }

                                                                /* Reset the controller.                                */
    FLASH.FRESETR.BIT.FRESET = 1;
    FLASH.FRESETR.BIT.FRESET = 0;


    return 0;
}
