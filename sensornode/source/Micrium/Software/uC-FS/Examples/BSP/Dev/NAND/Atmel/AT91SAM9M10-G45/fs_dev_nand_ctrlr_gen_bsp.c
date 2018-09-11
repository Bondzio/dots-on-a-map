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
*                                          NAND FLASH DEVICE
*
*                             BOARD SUPPORT PACKAGE (BSP) FUNCTIONS EXAMPLE
*
*                   MICRON MT29F2G08ABD ON THE ATMEL AT91SAM9M10-G45-EK EVALUATION BOARD
*
* Filename      : fs_dev_nand_ctrlr_generic_bsp.c
* Version       : v4.07.00
* Programmer(s) : EJ
*                 FBJ
*
* Note(s)       : (1) This file is an example to get started to write your own BSP. Micrium does not
*                     provide any guarantee that this design will be functional when applied to your
*                     design and/or your application.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  <fs_dev_nand_ctrlr_gen.h>

#include  <fs_cfg.h>


/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*
* Note(s) : (1) According to the documentation, the ALE (Address Latch Enable) is connected to A21 (bit 21
*               of the address bus, and the CLE (Command Latch Enable) is connected to A22 (bit 22 of the
*               address bus. Adding respectively 0x200000 and 0x400000 to the address written sets
*               those bit high in the external bus interface (EBI).
*********************************************************************************************************
*/

#define SAM9M10_NAND_BASE_ADDR            (CPU_ADDR    )(0x40000000)
#define SAM9M10_NAND_DATA                 (CPU_INT16U *)(SAM9M10_NAND_BASE_ADDR + 0x000000)
#define SAM9M10_NAND_ADDR                 (CPU_INT16U *)(SAM9M10_NAND_BASE_ADDR + 0x200000)
#define SAM9M10_NAND_CMD                  (CPU_INT16U *)(SAM9M10_NAND_BASE_ADDR + 0x400000)


/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  SAM9M10_REG_MATRIX_BASE           (CPU_ADDR   )(0xFFFFEA00u)
#define  SAM9M10_REG_MATRIX_CCFG_EBICSA  (*(CPU_REG32 *)(SAM9M10_REG_MATRIX_BASE + 0x128u))

#define  SAM9M10_REG_SMC_BASE              (CPU_ADDR   )(0xFFFFE800u)
#define  SAM9M10_REG_SMC_SETUP(cs_nbr)   (*(CPU_REG32 *)(SAM9M10_REG_SMC_BASE + (cs_nbr * 0x10u) + 0x00u))
#define  SAM9M10_REG_SMC_PULSE(cs_nbr)   (*(CPU_REG32 *)(SAM9M10_REG_SMC_BASE + (cs_nbr * 0x10u) + 0x04u))
#define  SAM9M10_REG_SMC_CYCLE(cs_nbr)   (*(CPU_REG32 *)(SAM9M10_REG_SMC_BASE + (cs_nbr * 0x10u) + 0x08u))
#define  SAM9M10_REG_SMC_MODE(cs_nbr)    (*(CPU_REG32 *)(SAM9M10_REG_SMC_BASE + (cs_nbr * 0x10u) + 0x0Cu))

#define  SAM9M10_REG_PIOC_BASE             (CPU_ADDR   )(0xFFFFF600)
#define  SAM9M10_REG_PIOC_PER            (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x00u))
#define  SAM9M10_REG_PIOC_PDR            (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x04u))
#define  SAM9M10_REG_PIOC_PSR            (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x08u))
#define  SAM9M10_REG_PIOC_OER            (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x10u))
#define  SAM9M10_REG_PIOC_ODR            (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x14u))
#define  SAM9M10_REG_PIOC_IFDR           (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x24u))
#define  SAM9M10_REG_PIOC_SODR           (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x30u))
#define  SAM9M10_REG_PIOC_CODR           (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x34u))
#define  SAM9M10_REG_PIOC_PDSR           (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x3Cu))
#define  SAM9M10_REG_PIOC_IDR            (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x44u))
#define  SAM9M10_REG_PIOC_MDDR           (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x54u))
#define  SAM9M10_REG_PIOC_PUDR           (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x60u))
#define  SAM9M10_REG_PIOC_PUER           (*(CPU_REG32 *)(SAM9M10_REG_PIOC_BASE + 0x64u))

#define  SAM9M10_REG_PMC_BASE              (CPU_ADDR   )(0xFFFFFC00)
#define  SAM9M10_REG_PMC_PCER            (*(CPU_REG32 *)(SAM9M10_REG_PMC_BASE  + 0x10u))


/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  SAM9M10_BIT_MATRIX_CCFG_EBI_CS3A           DEF_BIT_03

#define  SAM9M10_BIT_SMC_MODE_READ_MODE             DEF_BIT_00
#define  SAM9M10_BIT_SMC_MODE_WRITE_MODE            DEF_BIT_01
#define  SAM9M10_BIT_SMC_MODE_NWAIT_DISABLED        DEF_BIT_NONE
#define  SAM9M10_BIT_SMC_MODE_DBW_8BIT              DEF_BIT_NONE
#define  SAM9M10_BIT_SMC_MODE_TDF_CYCLES_MASK       DEF_BIT_MASK(0xFu, 16u)

#define  SAM9M10_BIT_PIOC_P08                       DEF_BIT_08
#define  SAM9M10_BIT_PIOC_P14                       DEF_BIT_14

#define  SAM9M10_BIT_PID_PIOC                       4u


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
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


static  void  FS_NAND_BSP_Open         (FS_ERR        *p_err);

static  void  FS_NAND_BSP_Close        (void);

static  void  FS_NAND_BSP_ChipSelEn    (void);

static  void  FS_NAND_BSP_ChipSelDis   (void);

static  void  FS_NAND_BSP_CmdWr        (CPU_INT08U    *p_cmd,
                                        CPU_SIZE_T     cnt,
                                        FS_ERR        *p_err);

static  void  FS_NAND_BSP_AddrWr       (CPU_INT08U    *p_addr,
                                        CPU_SIZE_T     cnt,
                                        FS_ERR        *p_err);

static  void  FS_NAND_BSP_DataWr       (void          *p_src,
                                        CPU_SIZE_T     cnt,
                                        CPU_INT08U     width,
                                        FS_ERR        *p_err);

static  void  FS_NAND_BSP_DataRd       (void          *p_dest,
                                        CPU_SIZE_T     cnt,
                                        CPU_INT08U     width,
                                        FS_ERR        *p_err);

static  void  FS_NAND_BSP_WaitWhileBusy(void          *p_ctrlr_data,
                                        CPU_BOOLEAN  (*poll_fcnt)(void  *),
                                        CPU_INT32U     to_us,
                                        FS_ERR        *p_err);


/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_NAND_CTRLR_GEN_BSP_API  FS_NAND_BSP_SAM9M10 = {
    FS_NAND_BSP_Open,
    FS_NAND_BSP_Close,
    FS_NAND_BSP_ChipSelEn,
    FS_NAND_BSP_ChipSelDis,
    FS_NAND_BSP_CmdWr,
    FS_NAND_BSP_AddrWr,
    FS_NAND_BSP_DataWr,
    FS_NAND_BSP_DataRd,
    FS_NAND_BSP_WaitWhileBusy
};

/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         FS_NAND_BSP_Open()
*
* Description : Open (initialize) bus for NAND.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           FS_ERR_NONE     No error.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is opened.
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_Open (FS_ERR  *p_err)
{
                                                                /* Config EBI.                                          */
    SAM9M10_REG_MATRIX_CCFG_EBICSA |=  SAM9M10_BIT_MATRIX_CCFG_EBI_CS3A;

                                                                /* Config SMC.                                          */
    SAM9M10_REG_SMC_SETUP(3)        =  0x00010001u;
    SAM9M10_REG_SMC_PULSE(3)        =  0x04030302u;
    SAM9M10_REG_SMC_CYCLE(3)        =  0x00070004u;

    SAM9M10_REG_SMC_MODE(3)         =  SAM9M10_BIT_SMC_MODE_READ_MODE
                                    |  SAM9M10_BIT_SMC_MODE_WRITE_MODE
                                    |  SAM9M10_BIT_SMC_MODE_NWAIT_DISABLED
                                    |  SAM9M10_BIT_SMC_MODE_DBW_8BIT
                                    |  ((0x1 << 16) & SAM9M10_BIT_SMC_MODE_TDF_CYCLES_MASK);

                                                                /* Config PIO.                                          */
    SAM9M10_REG_PIOC_IDR            =  SAM9M10_BIT_PIOC_P14;
    SAM9M10_REG_PIOC_PUDR           =  SAM9M10_BIT_PIOC_P14;
    SAM9M10_REG_PIOC_MDDR           =  SAM9M10_BIT_PIOC_P14;
    SAM9M10_REG_PIOC_SODR           =  SAM9M10_BIT_PIOC_P14;
    SAM9M10_REG_PIOC_OER            =  SAM9M10_BIT_PIOC_P14;
    SAM9M10_REG_PIOC_PER            =  SAM9M10_BIT_PIOC_P14;

    SAM9M10_REG_PMC_PCER            = (1 << SAM9M10_BIT_PID_PIOC);

    SAM9M10_REG_PIOC_IDR            =  SAM9M10_BIT_PIOC_P08;
    SAM9M10_REG_PIOC_PUER           =  SAM9M10_BIT_PIOC_P08;
    SAM9M10_REG_PIOC_IFDR           =  SAM9M10_BIT_PIOC_P08;
    SAM9M10_REG_PIOC_ODR            =  SAM9M10_BIT_PIOC_P08;
    SAM9M10_REG_PIOC_PER            =  SAM9M10_BIT_PIOC_P08;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         FS_NAND_BSP_Close()
*
* Description : Close (uninitialize) bus for NAND.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_Close (void)
{

}


/*
*********************************************************************************************************
*                                        FS_NAND_BSP_ChipSelEn()
*
* Description : Enable NAND chip select/enable.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s)     : (1) If you are sharing the bus/hardware with other software, this function MUST get an
*                   exclusive resource lock and configure any peripheral that could have been setup
*                   differently by the other software.
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_ChipSelEn (void)
{
    SAM9M10_REG_PIOC_CODR = (1 << 14);
}


/*
*********************************************************************************************************
*                                       FS_NAND_BSP_ChipSelDis()
*
* Description : Disable NAND chip select/enable.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s)     : (1) If you are sharing the bus/hardware with other software, this function MUST release
*                   the exclusive resource lock obtained in function FS_NAND_BSP_ChipSelEn().
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_ChipSelDis (void)
{
    SAM9M10_REG_PIOC_SODR = (1 << 14);
}


/*
*********************************************************************************************************
*                                       FS_NAND_BSP_CmdWr()
*
* Description : Write command to NAND.
*
* Argument(s) : p_cmd   Pointer to buffer that holds command.
*
*               cnt     Number of octets to write.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           FS_ERR_NONE     No error.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s) : (1) According to the documentation, the CLE (Command Latch Enable) is connected to A22 (bit 22
*               of the address bus. Writing at the address SAM9M10_NAND_CMD sets this bit high in the
*               external bus interface (EBI).
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_CmdWr (CPU_INT08U  *p_cmd,
                                 CPU_SIZE_T   cnt,
                                 FS_ERR      *p_err)
{
    Mem_Copy(SAM9M10_NAND_CMD, p_cmd, cnt);
   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        FS_NAND_BSP_AddrWr()
*
* Description : Write address to NAND.
*
* Argument(s) : p_addr  Pointer to buffer that holds address.
*
*               cnt     Number of octets to write.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           FS_ERR_NONE     No error.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s) : (1) According to the documentation, the ALE (Address Latch Enable) is connected to A21 (bit 21
*               of the address bus. Writing at the address SAM9M10_NAND_ADDR sets this bit high in the
*               external bus interface (EBI).
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_AddrWr (CPU_INT08U  *p_addr,
                                  CPU_SIZE_T   cnt,
                                  FS_ERR      *p_err)
{
    Mem_Copy(SAM9M10_NAND_ADDR, p_addr, cnt);
   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        FS_NAND_BSP_DataWr()
*
* Description : Write data to NAND.
*
* Argument(s) : p_src   Pointer to source memory buffer.
*
*               cnt     Number of octets to write.
*
*               width   Write access width, in bits.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           FS_ERR_NONE     No error.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s)     : (1) If 'cnt' is larger than 0x1FFFFF, the function will not work as it will set the ALE.
*                   Since this is highly improbable, a simple Mem_Copy() is still used to avoid a more
*                   complex code.
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_DataWr (void        *p_src,
                                  CPU_SIZE_T   cnt,
                                  CPU_INT08U   width,
                                  FS_ERR      *p_err)
{
#if (FS_CFG_ERR_ARG_CHK_DBG_EN == DEF_ENABLED)
    if (width != 8u) {
       *p_err = FS_ERR_INVALID_ARG;
        return;
    }
#endif

    Mem_Copy(SAM9M10_NAND_DATA, p_src, cnt);
   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         FS_NAND_BSP_DataRd()
*
* Description : Read data from NAND.
*
* Argument(s) : p_dest  Pointer to destination memory buffer.
*
*               cnt     Number of octets to read.
*
*               width   Read access width, in bits.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           FS_ERR_NONE     No error.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s)     : (1) If 'cnt' is larger than 0x1FFFFF, the function will not work as it will set the ALE.
*                   Since this is highly improbable, a simple Mem_Copy() is still used to avoid a more
*                   complex code.
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_DataRd (void        *p_dest,
                                  CPU_SIZE_T   cnt,
                                  CPU_INT08U   width,
                                  FS_ERR      *p_err)
{
#if (FS_CFG_ERR_ARG_CHK_DBG_EN == DEF_ENABLED)
    if (width != 8u) {
       *p_err = FS_ERR_INVALID_ARG;
        return;
    }
#endif

    Mem_Copy(p_dest, SAM9M10_NAND_DATA, cnt);
   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   FS_NAND_BSP_WaitWhileBusy()
*
* Description : Wait while NAND is busy.
*
* Argument(s) : p_ctrlr_data    Pointer to NAND controller data.
*
*               poll_fnct       Pointer to function to poll, if there is no hardware ready/busy signal.
*
*               to_us           Timeout, in microseconds.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   FS_ERR_NONE     No error.
*
* Return(s)   : none.
*
* Caller(s)   : Controller-layer driver.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FS_NAND_BSP_WaitWhileBusy (void          *p_ctrlr_data,
                                         CPU_BOOLEAN  (*poll_fcnt)(void  *),
                                         CPU_INT32U     to_us,
                                         FS_ERR        *p_err)
{
    while((SAM9M10_REG_PIOC_PDSR & (1 << 8)) == 0u);
   *p_err = FS_ERR_NONE;
}

