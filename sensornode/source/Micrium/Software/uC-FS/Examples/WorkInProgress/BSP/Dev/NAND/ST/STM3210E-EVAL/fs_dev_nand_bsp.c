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
*                      ST STM32F103VE ON THE ST STM3210E-EVAL EVALUATION BOARD
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
#include  <fs_dev_nand_stm32.h>
#include  <stm32f10x.h>
#include  <ecc.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  STM32_ADDR_DATA                           0x70000000u
#define  STM32_ADDR_ADDR                           0x70020000u
#define  STM32_ADDR_CMD                            0x70010000u

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  STM32_REG_FSMC_BCR1        (*(CPU_REG32 *)0xA0000000u)
#define  STM32_REG_FSMC_BCR2        (*(CPU_REG32 *)0xA0000008u)
#define  STM32_REG_FSMC_BCR3        (*(CPU_REG32 *)0xA0000010u)
#define  STM32_REG_FSMC_BCR4        (*(CPU_REG32 *)0xA0000018u)

#define  STM32_REG_FSMC_BTR1        (*(CPU_REG32 *)0xA0000004u)
#define  STM32_REG_FSMC_BTR2        (*(CPU_REG32 *)0xA000000Cu)
#define  STM32_REG_FSMC_BTR3        (*(CPU_REG32 *)0xA0000014u)
#define  STM32_REG_FSMC_BTR4        (*(CPU_REG32 *)0xA000001Cu)

#define  STM32_REG_FSMC_BWTR1       (*(CPU_REG32 *)0xA0000100u)
#define  STM32_REG_FSMC_BWTR2       (*(CPU_REG32 *)0xA0000104u)
#define  STM32_REG_FSMC_BWTR3       (*(CPU_REG32 *)0xA0000114u)
#define  STM32_REG_FSMC_BWTR4       (*(CPU_REG32 *)0xA000011Cu)

#define  STM32_REG_FSMC_PCR2        (*(CPU_REG32 *)0xA0000060u)
#define  STM32_REG_FSMC_PCR3        (*(CPU_REG32 *)0xA0000080u)
#define  STM32_REG_FSMC_PCR4        (*(CPU_REG32 *)0xA00000A0u)

#define  STM32_REG_FSMC_SR2         (*(CPU_REG32 *)0xA0000064u)
#define  STM32_REG_FSMC_SR3         (*(CPU_REG32 *)0xA0000084u)
#define  STM32_REG_FSMC_SR4         (*(CPU_REG32 *)0xA00000A4u)

#define  STM32_REG_FSMC_PMEM2       (*(CPU_REG32 *)0xA0000068u)
#define  STM32_REG_FSMC_PMEM3       (*(CPU_REG32 *)0xA0000088u)
#define  STM32_REG_FSMC_PMEM4       (*(CPU_REG32 *)0xA00000A8u)

#define  STM32_REG_FSMC_PATT2       (*(CPU_REG32 *)0xA000006Cu)
#define  STM32_REG_FSMC_PATT3       (*(CPU_REG32 *)0xA000008Cu)
#define  STM32_REG_FSMC_PATT4       (*(CPU_REG32 *)0xA00000ACu)

#define  STM32_REG_FSMC_PIO4        (*(CPU_REG32 *)0xA00000B0u)

#define  STM32_REG_FSMC_ECCR2       (*(CPU_REG32 *)0xA0000074u)
#define  STM32_REG_FSMC_ECCR3       (*(CPU_REG32 *)0xA0000094u)

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  STM32_BIT_FSMC_PCR_ECCPS_0256          0x00000000u
#define  STM32_BIT_FSMC_PCR_ECCPS_0512          0x00020000u
#define  STM32_BIT_FSMC_PCR_ECCPS_1024          0x00040000u
#define  STM32_BIT_FSMC_PCR_ECCPS_2048          0x00060000u
#define  STM32_BIT_FSMC_PCR_ECCPS_4096          0x00080000u
#define  STM32_BIT_FSMC_PCR_ECCPS_8192          0x000A0000u
#define  STM32_BIT_FSMC_PCR_ECCPS_MASK          0x000E0000u

#define  STM32_BIT_FSMC_PCR_ECCEN               DEF_BIT_06

#define  STM32_BIT_FSMC_ECCR_MASK_0256          0x003FFFFFu
#define  STM32_BIT_FSMC_ECCR_MASK_0512          0x00FFFFFFu
#define  STM32_BIT_FSMC_ECCR_MASK_1024          0x03FFFFFFu
#define  STM32_BIT_FSMC_ECCR_MASK_2048          0x0FFFFFFFu
#define  STM32_BIT_FSMC_ECCR_MASK_4096          0x3FFFFFFFu
#define  STM32_BIT_FSMC_ECCR_MASK_8192          0xFFFFFFFFu


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
    GPIO_InitTypeDef                   gpio_init;
    FSMC_NANDInitTypeDef               nand_init;
    FSMC_NAND_PCCARDTimingInitTypeDef  p;


    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);

                                                                /* ---------------------- CFG GPIO -------------------- */
                                                                /* CLE, ALE, D0..3, NOW, NWE & NCE2 NAND pin cfg.       */
    gpio_init.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_14 | GPIO_Pin_15 |
                           GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_4  | GPIO_Pin_5  | GPIO_Pin_7;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;

    GPIO_Init(GPIOD, &gpio_init);

                                                                /* D4..7 NAND pin configuration.                        */
    gpio_init.GPIO_Pin   = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;

    GPIO_Init(GPIOE, &gpio_init);


                                                                /* NWAIT NAND pin configuration.                        */
    gpio_init.GPIO_Pin   = GPIO_Pin_6;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_IPU;

    GPIO_Init(GPIOD, &gpio_init);

                                                                /* INT2 NAND pin configuration.                         */
    gpio_init.GPIO_Pin   = GPIO_Pin_6;
    GPIO_Init(GPIOG, &gpio_init);

                                                                /* ---------------------- CFG FSMC -------------------- */
    p.FSMC_SetupTime                          = 0x1;
    p.FSMC_WaitSetupTime                      = 0x3;
    p.FSMC_HoldSetupTime                      = 0x2;
    p.FSMC_HiZSetupTime                       = 0x1;

    nand_init.FSMC_Bank                       = FSMC_Bank2_NAND;
    nand_init.FSMC_Waitfeature                = FSMC_Waitfeature_Enable;
    nand_init.FSMC_MemoryDataWidth            = FSMC_MemoryDataWidth_8b;
    nand_init.FSMC_ECC                        = FSMC_ECC_Enable;
    nand_init.FSMC_ECCPageSize                = FSMC_ECCPageSize_512Bytes;
    nand_init.FSMC_TCLRSetupTime              = 0x00;
    nand_init.FSMC_TARSetupTime               = 0x00;
    nand_init.FSMC_CommonSpaceTimingStruct    = &p;
    nand_init.FSMC_AttributeSpaceTimingStruct = &p;

    FSMC_NANDInit(&nand_init);

    FSMC_NANDCmd(FSMC_Bank2_NAND, ENABLE);                      /* FSMC NAND bank cmd test.                             */

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
    CPU_INT08U  *p_dest_08;
    CPU_INT08U   datum_08;


    p_dest_08 = (CPU_INT08U *)p_dest;
    while (cnt > 0u) {
        datum_08  = *(CPU_REG08 *)STM32_ADDR_DATA;
       *p_dest_08 =   datum_08;

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
    CPU_INT08U  datum;


    while (cnt > 0u) {
        datum                         = *p_addr;
        *(CPU_REG08 *)STM32_ADDR_ADDR =  datum;

        p_addr++;
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
    CPU_INT08U  datum;


    while (cnt > 0u) {
        datum                        = *p_cmd;
        *(CPU_REG08 *)STM32_ADDR_CMD =  datum;

        p_cmd++;
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
    CPU_INT08U  *p_src_08;
    CPU_INT08U   datum;


    p_src_08 = (CPU_INT08U *)p_src;
    while (cnt > 0u) {
        datum                         = *p_src_08;
        *(CPU_REG08 *)STM32_ADDR_DATA =  datum;

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


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        STM32 NAND FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          STM32_ECC_Init()
*
* Description : Initialize ECC generator (for 256-byte page size).
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  STM32_ECC_Init (void)
{
    CPU_INT32U  reg_val;


    reg_val              =  STM32_REG_FSMC_PCR2;
    reg_val             &= ~STM32_BIT_FSMC_PCR_ECCPS_MASK;
    reg_val             |=  STM32_BIT_FSMC_PCR_ECCPS_0256;
    reg_val             &= ~STM32_BIT_FSMC_PCR_ECCEN;
    STM32_REG_FSMC_PCR2  =  reg_val;
}


/*
*********************************************************************************************************
*                                          STM32_ECC_Start()
*
* Description : Start ECC computation (for 256-byte page size).
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  STM32_ECC_Start (void)
{
    STM32_REG_FSMC_PCR2 |= STM32_BIT_FSMC_PCR_ECCEN;
}


/*
*********************************************************************************************************
*                                           STM32_ECC_Get()
*
* Description : Get ECC value (for 256-byte page size).
*
* Argument(s) : none.
*
* Return(s)   : 21-bit ECC in 32-bit value.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  STM32_ECC_Get (void)
{
    CPU_INT32U  ecc;


    ecc  = STM32_REG_FSMC_ECCR2;
    ecc &= STM32_BIT_FSMC_ECCR_MASK_0256;

    STM32_REG_FSMC_PCR2 &= ~STM32_BIT_FSMC_PCR_ECCEN;

    return (ecc);
}


/*
*********************************************************************************************************
*                                         STM32_ECC_Correct()
*
* Description : Correct error using ECC (for 256-byte page size).
*
* Argument(s) : ecc         ECC for data buffer.
*
*               ecc_chk     ECC for original data buffer.
*
*               p_data      Pointer to data buffer.
*
* Return(s)   : ECC_ERR_NONE,            if no error.
*               ECC_ERR_CORRECTABLE,     if correctable error detected in data.
*               ECC_ERR_ECC_CORRECTABLE, if correctable error detected in ECC.
*               ECC_ERR_UNCORRECTABLE,   if uncorrectable error detected in data.
*
* Caller(s)   : Physical-layer driver.
*
* Note(s)     : (1) The 22-bit ECC has 6 bit (or column) parity bits & 16 word (or line) parity bits :
*
*                       W128 W128' W064 W064' W032 W032' W016 W016' W008 W008' W004 W004' W002 W002' W001 W001' B04 B04' B02 B02' B01 B01'
*
*                   (a) If there is no error, the ECC and check ECC will be identical.
*
*                   (b) If there is a single-bit error in the data, then, in the XOR of the ECC & check
*                       ECC, for each n, either Wn or Wn' will be set, & for each m, either Bm or Bm' will
*                       be set.  The bit error location can be extracted from this XOR :
*
*                           BIT  ADDRESS = {B04, B02, B01}
*
*                           BYTE ADDRESS = {W128, W064, W032, W016, W008, W004, W002, W001}
*
*
*                   (c) If there is a single bit error, then only one bit will be set in the XOR of the
*                       ECC & check ECC.
*
*                   (d) Otherwise, there is a multiple-bit error.
*********************************************************************************************************
*/

CPU_ERR  STM32_ECC_Correct (CPU_INT32U   ecc,
                            CPU_INT32U   ecc_chk,
                            CPU_INT08U  *p_data)
{
    CPU_INT08U  addr_bit;
    CPU_SIZE_T  addr_octet;
    CPU_INT08U  bit_ix;
    CPU_INT08U  bit_set_cnt;
    CPU_INT32U  temp;


    ecc     &= STM32_BIT_FSMC_ECCR_MASK_0256;
    ecc_chk &= STM32_BIT_FSMC_ECCR_MASK_0256;

    if (ecc == ecc_chk) {
        return (ECC_ERR_NONE);
    }

    ecc     ^= ecc_chk;
    temp     = ecc ^ (ecc >> 1);
    temp    &= 0x00155555u;

    if (temp == 0x00155555u) {
        addr_bit = 0u;
        for (bit_ix = 0u; bit_ix < 3u; bit_ix++) {
            if (DEF_BIT_IS_SET(ecc, DEF_BIT(2u * bit_ix + 1u)) == DEF_YES) {
                addr_bit |= DEF_BIT(bit_ix);
            }
        }

        addr_octet = 0u;
        for (bit_ix = 0u; bit_ix < 8u; bit_ix++) {
            if (DEF_BIT_IS_SET(ecc, DEF_BIT(2u * bit_ix + 7u)) == DEF_YES) {
                addr_octet |= DEF_BIT(bit_ix);
            }
        }

        p_data[addr_octet] ^= DEF_BIT(addr_bit);
        return (ECC_ERR_CORRECTABLE);

    } else {
        bit_set_cnt = 0u;
        for (bit_ix = 0u; bit_ix < DEF_INT_16_NBR_BITS; bit_ix++) {
            if (DEF_BIT_IS_SET(ecc, DEF_BIT(bit_ix)) == DEF_YES) {
                bit_set_cnt++;
                if (bit_set_cnt > 1u) {
                    return (ECC_ERR_UNCORRECTABLE);
                }
            }
        }
        return (ECC_ERR_ECC_CORRECTABLE);
    }
}
