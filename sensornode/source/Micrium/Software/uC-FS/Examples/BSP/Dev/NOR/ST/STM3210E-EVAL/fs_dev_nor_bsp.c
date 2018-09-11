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
*                      ST STM32F103VE ON THE ST STM3210E-EVAL EVALUATION BOARD
*
* Filename      : fs_dev_nor_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The ST STM3210E-EVAL evaluation board has a 128-Mb ST M29W128GL NOR located on
*                     Bank 1 NOR/PSRAM 2.  The following NOR configuration parameters should be used :
*
*                         nor_cfg.AddrBase    = 0x64000000;     // Bus address of Bank 1, NOR/PSRAM 2 memory.
*                         nor_cfg.RegionNbr   = 0;              // M29W128GL has only one region.
*
*                         nor_cfg.PhyPtr      = (FS_DEV_NOR_PHY_API *)&FSDev_NOR_AMD_1x16;
*
*                         nor_cfg.BusWidth    = 16;             // Bus is 16-bit.
*                         nor_cfg.BusWidthMax = 16;             // M29W128GL is 16-bit.
*                         nor_cfg.PhyDevCnt   = 1;              // Only one M29W128GL.
*
*                     The remaining NOR configuration parameters are based on application requirements.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_NOR_BSP_MODULE
#include  <fs_dev_nor.h>
#include  <stm32f10x_fsmc.h>
#include  <stm32f10x_rcc.h>
#include  <stm32f10x_gpio.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

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
    GPIO_InitTypeDef               gpio_init;
    FSMC_NORSRAMInitTypeDef        nor_init;
    FSMC_NORSRAMTimingInitTypeDef  p;


    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_NOR_BSP_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);

                                                                /* ---------------------- CFG GPIO -------------------- */
                                                                /* NOR data lines configuration.                        */
    gpio_init.GPIO_Pin   = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_7  | GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11 |
                           GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOE, &gpio_init);

                                                                /* NOR address lines configuration.                     */
    gpio_init.GPIO_Pin   = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  | GPIO_Pin_3  | GPIO_Pin_4 |
                           GPIO_Pin_5  | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOF, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  | GPIO_Pin_3  | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_Init(GPIOG, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
    GPIO_Init(GPIOD, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_3  | GPIO_Pin_4  | GPIO_Pin_5  | GPIO_Pin_6;
    GPIO_Init(GPIOE, &gpio_init);

                                                                /* NOE and NWE configuration.                           */
    gpio_init.GPIO_Pin   = GPIO_Pin_4  | GPIO_Pin_5;
    GPIO_Init(GPIOD, &gpio_init);

                                                                /* NE2 configuration.                                   */
    gpio_init.GPIO_Pin   = GPIO_Pin_9;
    GPIO_Init(GPIOG, &gpio_init);

                                                                /* ---------------------- CFG FSMC -------------------- */
    p.FSMC_AddressSetupTime             = 0x05;
    p.FSMC_AddressHoldTime              = 0x00;
    p.FSMC_DataSetupTime                = 0x07;
    p.FSMC_BusTurnAroundDuration        = 0x00;
    p.FSMC_CLKDivision                  = 0x00;
    p.FSMC_DataLatency                  = 0x00;
    p.FSMC_AccessMode                   = FSMC_AccessMode_B;

    nor_init.FSMC_Bank                  = FSMC_Bank1_NORSRAM2;
    nor_init.FSMC_DataAddressMux        = FSMC_DataAddressMux_Disable;
    nor_init.FSMC_MemoryType            = FSMC_MemoryType_NOR;
    nor_init.FSMC_MemoryDataWidth       = FSMC_MemoryDataWidth_16b;
    nor_init.FSMC_BurstAccessMode       = FSMC_BurstAccessMode_Disable;
    nor_init.FSMC_WaitSignalPolarity    = FSMC_WaitSignalPolarity_Low;
    nor_init.FSMC_WrapMode              = FSMC_WrapMode_Disable;
    nor_init.FSMC_WaitSignalActive      = FSMC_WaitSignalActive_BeforeWaitState;
    nor_init.FSMC_WriteOperation        = FSMC_WriteOperation_Enable;
    nor_init.FSMC_WaitSignal            = FSMC_WaitSignal_Disable;
    nor_init.FSMC_ExtendedMode          = FSMC_ExtendedMode_Disable;
    nor_init.FSMC_WriteBurst            = FSMC_WriteBurst_Disable;
    nor_init.FSMC_ReadWriteTimingStruct = &p;
    nor_init.FSMC_WriteTimingStruct     = &p;

    FSMC_NORSRAMInit(&nor_init);

                                                                /* Enable FSMC Bank1_NOR Bank.                          */
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM2, ENABLE);

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
    CPU_INT08U  *p_dest_08;
    CPU_INT16U   datum;


    (void)&unit_nbr;                                            /* Prevent compiler warning.                            */

    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
         datum = *(CPU_REG16 *)addr_src;
         MEM_VAL_SET_INT16U((void *)p_dest_08, datum);
         p_dest_08 += 2;
         addr_src  += 2u;
         cnt--;
    }
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
    CPU_INT08U  datum;


    (void)&unit_nbr;                                            /* Prevent compiler warning.                            */

    datum = *(CPU_REG08 *)addr_src;
    return (datum);
}

CPU_INT16U  FSDev_NOR_BSP_RdWord_16 (FS_QTY    unit_nbr,
                                     CPU_ADDR  addr_src)
{
    CPU_INT16U  datum;


    (void)&unit_nbr;                                            /* Prevent compiler warning.                            */

    datum = *(CPU_REG16 *)addr_src;
    return (datum);
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
    *(CPU_REG08 *)addr_dest = datum;
}

void  FSDev_NOR_BSP_WrWord_16 (FS_QTY      unit_nbr,
                               CPU_ADDR    addr_dest,
                               CPU_INT16U  datum)
{
    (void)&unit_nbr;                                            /* Prevent compiler warning.                            */

    *(CPU_REG16 *)addr_dest = datum;
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
