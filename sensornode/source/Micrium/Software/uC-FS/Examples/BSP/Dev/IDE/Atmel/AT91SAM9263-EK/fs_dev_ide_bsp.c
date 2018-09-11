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
*                                             IDE DEVICES
*
*                                BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                      ATMEL AT91SAM9263 ON THE AT91SAM9263-EK EVALUATION BOARD
*
* Filename      : fs_dev_ide_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The AT91SAM9263 SD/MMC card interface is described in documentation from Atmel :
*
*                         AT91SAM9263.  6258G-ATARM-06-Jan-09.  "20: AT91SAM9263 Bus Matrix".
*                                                               "21: External Bus Interface (EBI)".
*                                                               "22: Static Memory Controller (SMC)".
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_IDE_BSP_MODULE
#include  <fs_dev_ide.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  AT91SAM9263_BASE_CS4                    0x60000000uL
#define  AT91SAM9263_BASE_CS5                    0x60000000uL
#define  AT91SAM9263_OFF_TASK_FILE               0x00C00000uL
#define  AT91SAM9263_OFF_CTRL_REG                0x00E00000uL

#define  AT91SAM9263_PIN_INTRQ                   DEF_BIT_02     /* PD2                                                  */
#define  AT91SAM9263_PIN_IORDY                   DEF_BIT_03     /* PD3                                                  */
#define  AT91SAM9263_PIN_CFCS0                   DEF_BIT_06     /* PD6                                                  */
#define  AT91SAM9263_PIN_CFCS1                   DEF_BIT_07     /* PD7                                                  */
#define  AT91SAM9263_PIN_CFCE1                   DEF_BIT_08     /* PD8                                                  */
#define  AT91SAM9263_PIN_CFCE2                   DEF_BIT_09     /* PD9                                                  */
#define  AT91SAM9263_PIN_CFRNW                   DEF_BIT_25     /* PD25                                                 */

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  AT91SAM9263_REG_PIOB_BASE                             0xFFFFF400uL
#define  AT91SAM9263_REG_PIOB_PDR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x004u))
#define  AT91SAM9263_REG_PIOB_PER                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x000u))
#define  AT91SAM9263_REG_PIOB_OER                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x010u))
#define  AT91SAM9263_REG_PIOB_ODR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x014u))
#define  AT91SAM9263_REG_PIOB_SODR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x030u))
#define  AT91SAM9263_REG_PIOB_CODR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x034u))
#define  AT91SAM9263_REG_PIOB_PDSR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x03Cu))
#define  AT91SAM9263_REG_PIOB_MDDR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x054u))
#define  AT91SAM9263_REG_PIOB_PPUDR                   (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x060u))
#define  AT91SAM9263_REG_PIOB_ASR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOB_BASE + 0x070u))

#define  AT91SAM9263_REG_PIOC_BASE                             0xFFFFF600uL
#define  AT91SAM9263_REG_PIOC_PDR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x004u))
#define  AT91SAM9263_REG_PIOC_PER                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x000u))
#define  AT91SAM9263_REG_PIOC_OER                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x010u))
#define  AT91SAM9263_REG_PIOC_ODR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x014u))
#define  AT91SAM9263_REG_PIOC_SODR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x030u))
#define  AT91SAM9263_REG_PIOC_CODR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x034u))
#define  AT91SAM9263_REG_PIOC_PDSR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x03Cu))
#define  AT91SAM9263_REG_PIOC_MDDR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x054u))
#define  AT91SAM9263_REG_PIOC_PPUDR                   (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x060u))
#define  AT91SAM9263_REG_PIOC_ASR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOC_BASE + 0x070u))

#define  AT91SAM9263_REG_PIOD_BASE                             0xFFFFF800uL
#define  AT91SAM9263_REG_PIOD_PDR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x004u))
#define  AT91SAM9263_REG_PIOD_PER                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x000u))
#define  AT91SAM9263_REG_PIOD_OER                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x010u))
#define  AT91SAM9263_REG_PIOD_ODR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x014u))
#define  AT91SAM9263_REG_PIOD_SODR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x030u))
#define  AT91SAM9263_REG_PIOD_CODR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x034u))
#define  AT91SAM9263_REG_PIOD_PDSR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x03Cu))
#define  AT91SAM9263_REG_PIOD_MDDR                    (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x054u))
#define  AT91SAM9263_REG_PIOD_PPUDR                   (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x060u))
#define  AT91SAM9263_REG_PIOD_ASR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PIOD_BASE + 0x070u))

#define  AT91SAM9263_REG_PMC_BASE                              0xFFFFFC00uL
#define  AT91SAM9263_REG_PMC_PCER                     (*(CPU_REG32 *)(AT91SAM9263_REG_PMC_BASE  + 0x010u))
#define  AT91SAM9263_REG_PMC_PCDR                     (*(CPU_REG32 *)(AT91SAM9263_REG_PMC_BASE  + 0x014u))

#define  AT91SAM9263_REG_AIC_BASE                              0xFFFFF000uL
#define  AT91SAM9263_REG_AIC_SMR(n)                   (*(CPU_REG32 *)(AT91SAM9263_REG_AIC_BASE  + 0x000u + (n) * 4))
#define  AT91SAM9263_REG_AIC_SVR(n)                   (*(CPU_REG32 *)(AT91SAM9263_REG_AIC_BASE  + 0x080u + (n) * 4))
#define  AT91SAM9263_REG_AIC_IVR                      (*(CPU_REG32 *)(AT91SAM9263_REG_AIC_BASE  + 0x100u))
#define  AT91SAM9263_REG_AIC_IECR                     (*(CPU_REG32 *)(AT91SAM9263_REG_AIC_BASE  + 0x120u))
#define  AT91SAM9263_REG_AIC_IDCR                     (*(CPU_REG32 *)(AT91SAM9263_REG_AIC_BASE  + 0x124u))
#define  AT91SAM9263_REG_AIC_ICCR                     (*(CPU_REG32 *)(AT91SAM9263_REG_AIC_BASE  + 0x128u))

#define  AT91SAM9263_REG_SMC_BASE                              0xFFFFE400uL
#define  AT91SAM9263_REG_SMC_SETUP4                   (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x040u))
#define  AT91SAM9263_REG_SMC_PULSE4                   (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x044u))
#define  AT91SAM9263_REG_SMC_CYCLE4                   (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x048u))
#define  AT91SAM9263_REG_SMC_CTRL4                    (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x04Cu))
#define  AT91SAM9263_REG_SMC_SETUP5                   (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x050u))
#define  AT91SAM9263_REG_SMC_PULSE5                   (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x054u))
#define  AT91SAM9263_REG_SMC_CYCLE5                   (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x058u))
#define  AT91SAM9263_REG_SMC_CTRL5                    (*(CPU_REG32 *)(AT91SAM9263_REG_SMC_BASE  + 0x05Cu))

#define  AT91SAM9263_REG_CCFG_BASE                             0xFFFFED10uL
#define  AT91SAM9263_REG_CCFG_TCMR                    (*(CPU_REG32 *)(AT91SAM9263_REG_CCFG_BASE + 0x004u))
#define  AT91SAM9263_REG_CCFG_EBI0CSA                 (*(CPU_REG32 *)(AT91SAM9263_REG_CCFG_BASE + 0x010u))
#define  AT91SAM9263_REG_CCFG_EBI1CSA                 (*(CPU_REG32 *)(AT91SAM9263_REG_CCFG_BASE + 0x010u))


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

#define  AT91SAM9264_SET_CS4_BW08()   AT91SAM9263_REG_SMC_CTRL4 = DEF_BIT_05 | DEF_BIT_NONE | (15u << 16);
#define  AT91SAM9264_SET_CS4_BW16()   AT91SAM9263_REG_SMC_CTRL4 = DEF_BIT_05 | DEF_BIT_12   | (15u << 16);

#define  AT91SAM9264_SET_CS5_BW08()   AT91SAM9263_REG_SMC_CTRL5 = DEF_BIT_05 | DEF_BIT_NONE | (15u << 16);
#define  AT91SAM9264_SET_CS5_BW16()   AT91SAM9263_REG_SMC_CTRL5 = DEF_BIT_05 | DEF_BIT_12   | (15u << 16);


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
*                                      FILE SYSTEM IDE FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        FSDev_IDE_BSP_Open()
*
* Description : Open (initialize) hardware.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : DEF_OK,   if interface was opened.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FSDev_IDE_Refresh().
*
* Note(s)     : (1) This function will be called EVERY time the device is opened.
*
*               (2) (a) The AT91SAM9263-EK evaluation board has a 1.8" hard drive connector, with the
*                       following connections :
*
*                           ----------------------------------------------------------------
*                           | AT91SAM9263 NAME | AT91SAM9263 PIO # | HDD NAME | HDD PIN #  |
*                           ----------------------------------------------------------------
*                           |     NRST         |        ----       | RESET-   | 1          |
*                           |     A0-A2        |        ----       |          | 35, 33, 36 |
*                           |     D0-D15       |        ----       |          | 3-17       |
*                           |     EBI0_NBS1    |        PD6        | DIOR-    | 25         |
*                           |     EBI0_NBS3    |        PD7        | DIOW-    | 23         |
*                           |     EBI0_CFCE1   |        PD8        | CS0-     | 37         |
*                           |     EBI0_CFCE2   |        PD9        | CS1-     | 38         |
*                           |     ---------    |        PD2        | INTRQ    | 31         |
*                           |     ---------    |        PD3        | IORDY    | 27         |
*                           ----------------------------------------------------------------
*
*                   (b) The hard drive may be addressed using EBI0 CS4 or CS5.  This port uses CS5;
*                       CS5 is addressed in the memory address range 0x60000000-0x6FFFFFFF.
*
*                   (c) A21-A23 determine the mode of device access :
*                       (1) If {A21, A22, A23} = {0, 1, 1}, the access mode is True IDE Mode.  Normal
*                           task file registers & data are accessed in this mode.  In this mode,
*                           CFCE1, CFCE2 (CS0-, CS1-) are 0, 1.
*                       (2) If {A21, A22, A23} = {1, 1, 1}, the access mode is True IDE Alternate Mode.
*                           The device control & alternate status registers are accessed in this mode.
*                           In this mode, CFCE1, CFCE2 (CS0-, CS1-) are 1, 0.
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_IDE_BSP_Open (FS_QTY  unit_nbr)
{
    CPU_INT32U  ebi0_csa;


    if (unit_nbr != 0u) {
        return (DEF_FAIL);
    }

    AT91SAM9263_REG_PIOD_PDR = AT91SAM9263_PIN_CFCE1 | AT91SAM9263_PIN_CFCE2 | AT91SAM9263_PIN_CFCS0 | AT91SAM9263_PIN_CFCS1 | AT91SAM9263_PIN_CFRNW;
    AT91SAM9263_REG_PIOD_ASR = AT91SAM9263_PIN_CFCE1 | AT91SAM9263_PIN_CFCE2 | AT91SAM9263_PIN_CFCS0 | AT91SAM9263_PIN_CFCS1 | AT91SAM9263_PIN_CFRNW;

    AT91SAM9263_REG_PIOD_PER = AT91SAM9263_PIN_INTRQ | AT91SAM9263_PIN_IORDY;
    AT91SAM9263_REG_PIOD_ODR = AT91SAM9263_PIN_INTRQ | AT91SAM9263_PIN_IORDY;

    ebi0_csa                      = AT91SAM9263_REG_CCFG_EBI0CSA;
    ebi0_csa                     |= DEF_BIT_05;
    AT91SAM9263_REG_CCFG_EBI0CSA  = ebi0_csa;

    AT91SAM9263_REG_SMC_SETUP5 =  (8u << 0) |  (8u << 16);
    AT91SAM9263_REG_SMC_PULSE5 = (30u << 0) |  (39u << 8) | (30u << 16) | (39u << 24);
    AT91SAM9263_REG_SMC_CYCLE5 = (61u << 0) | (61u << 16);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        FSDev_IDE_BSP_Close()
*
* Description : Close (uninitialize) hardware.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_IDE_Close().
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_Close (FS_QTY  unit_nbr)
{

}


/*
*********************************************************************************************************
*                                        FSDev_IDE_BSP_Lock()
*
* Description : Acquire IDE card bus lock.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) This function will be called before the IDE driver begins to access the IDE bus.
*                   The application should NOT use the same bus to access another device until the
*                   matching call to 'FSDev_IDE_BSP_Unlock()' has been made.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_Lock (FS_QTY  unit_nbr)
{

}


/*
*********************************************************************************************************
*                                       FSDev_IDE_BSP_Unlock()
*
* Description : Release IDE bus lock.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) 'FSDev_IDE_BSP_Lock()' will be called before the IDE driver begins to access the IDE
*                   bus.  The application should NOT use the same bus to access another device until the
*                   matching call to this function has been made.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_Unlock (FS_QTY  unit_nbr)
{

}


/*
*********************************************************************************************************
*                                        FSDev_IDE_BSP_Reset()
*
* Description : Hardware-reset IDE device.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_IDE_Refresh().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_Reset (FS_QTY  unit_nbr)
{

}


/*
*********************************************************************************************************
*                                        FSDev_IDE_BSP_RegRd()
*
* Description : Read from IDE device register.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               reg         Register to read :
*
*                               FS_DEV_IDE_REG_ERR          Error Register
*                               FS_DEV_IDE_REG_SC           Sector Count Registers
*                               FS_DEV_IDE_REG_SN           Sector Number Registers
*                               FS_DEV_IDE_REG_CYL          Cylinder Low Registers
*                               FS_DEV_IDE_REG_CYH          Cylinder High Registers
*                               FS_DEV_IDE_REG_DH           Card/Drive/Head Register
*                               FS_DEV_IDE_REG_STATUS       Status Register
*                               FS_DEV_IDE_REG_ALTSTATUS    Alternate Status
*
* Return(s)   : Register value.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  FSDev_IDE_BSP_RegRd (FS_QTY      unit_nbr,
                                 CPU_INT08U  reg)
{
    CPU_INT08U  val;


    AT91SAM9264_SET_CS5_BW08();

    if (reg == FS_DEV_IDE_REG_ALTSTATUS) {
        val = *(CPU_REG08 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_CTRL_REG  + 6u);

    } else {
        val = *(CPU_REG08 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE + reg);
    }

    return (val);
}


/*
*********************************************************************************************************
*                                        FSDev_IDE_BSP_RegWr()
*
* Description : Write to IDE device register.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               reg         Register to write :
*
*                               FS_DEV_IDE_REG_FR         Features Register
*                               FS_DEV_IDE_REG_SC         Sector Count Register
*                               FS_DEV_IDE_REG_SN         Sector Number Register
*                               FS_DEV_IDE_REG_CYL        Cylinder Low Register
*                               FS_DEV_IDE_REG_CYH        Cylinder High Register
*                               FS_DEV_IDE_REG_DH         Card/Drive/Head Register
*                               FS_DEV_IDE_REG_CMD        Command  Register
*                               FS_DEV_IDE_REG_DEVCTRL    Device Control
*
*               val         Value to write into register.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_RegWr (FS_QTY      unit_nbr,
                           CPU_INT08U  reg,
                           CPU_INT08U  val)
{
    AT91SAM9264_SET_CS5_BW08();

    if (reg == FS_DEV_IDE_REG_DEVCTRL) {
        *(CPU_REG08 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_CTRL_REG  + 6u)  = val;
    } else {
        *(CPU_REG08 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE + reg) = val;
    }
}


/*
*********************************************************************************************************
*                                        FSDev_IDE_BSP_CmdWr()
*
* Description : Write 7-byte command to IDE device registers.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               cmd         Array holding command.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_IDE_RdData(),
*               FSDev_IDE_WrData().
*
* Note(s)     : (1) The 7-byte of the command should be written to IDE device registers as follows :
*
*                       REG_FR  = Byte 0
*                       REG_SC  = Byte 1
*                       REG_SN  = Byte 2
*                       REG_CYL = Byte 3
*                       REG_CYH = Byte 4
*                       REG_DH  = Byte 5
*                       REG_CMD = Byte 6
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_CmdWr (FS_QTY      unit_nbr,
                           CPU_INT08U  cmd[])
{
    CPU_INT16U  val;


    AT91SAM9264_SET_CS5_BW08();
    *(CPU_REG08 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE + 1u) = cmd[0];

    val = cmd[1] | (CPU_INT16U)((CPU_INT16U)cmd[2] << 8);
    *(CPU_REG16 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE + 2u) = val;

    val = cmd[3] | (CPU_INT16U)((CPU_INT16U)cmd[4] << 8);
    *(CPU_REG16 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE + 4u) = val;

    val = cmd[5] | (CPU_INT16U)((CPU_INT16U)cmd[6] << 8);
    *(CPU_REG16 *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE + 6u) = val;
}


/*
*********************************************************************************************************
*                                          FSDev_IDE_BSP_DataRd()
*
* Description : Read data from IDE device.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               p_dest      Pointer to destination memory buffer.
*
*               cnt         Number of octets to read.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_IDE_RdData().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_DataRd (FS_QTY       unit_nbr,
                            void        *p_dest,
                            CPU_SIZE_T   cnt)
{
    CPU_SIZE_T             cnt_words;
    volatile  CPU_INT16U  *p_reg_data;
    CPU_INT16U            *p_dest_16;


    AT91SAM9264_SET_CS5_BW16();

    cnt_words  = (cnt + 1u) / sizeof(CPU_INT16U);
    p_reg_data = (CPU_REG16  *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE);
    p_dest_16  = (CPU_INT16U *)p_dest;

    while (cnt_words > 0u) {
       *p_dest_16 = *p_reg_data;

        p_dest_16++;
        cnt_words--;
    }
}


/*
*********************************************************************************************************
*                                          FSDev_IDE_BSP_DataWr()
*
* Description : Write data to IDE device.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               p_src       Pointer to source memory buffer.
*
*               cnt         Number of octets to write.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_IDE_WrData().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_DataWr (FS_QTY       unit_nbr,
                            void        *p_src,
                            CPU_SIZE_T   cnt)
{
    CPU_SIZE_T             cnt_words;
    volatile  CPU_INT16U  *p_reg_data;
    CPU_INT16U            *p_src_16;
    CPU_INT32U             pdsr;
    CPU_BOOLEAN            ok;


    AT91SAM9264_SET_CS5_BW16();

    cnt_words  = (cnt + 1u) / sizeof(CPU_INT16U);
    p_reg_data = (CPU_REG16  *)(AT91SAM9263_BASE_CS5 + AT91SAM9263_OFF_TASK_FILE);
    p_src_16   = (CPU_INT16U *)p_src;

    while (cnt_words > 0u) {
       *p_reg_data = *p_src_16;

        p_src_16++;
        cnt_words--;

        do {
            pdsr = AT91SAM9263_REG_PIOD_PDSR;
            ok   = DEF_BIT_IS_SET(pdsr, AT91SAM9263_PIN_IORDY);
        } while (ok == DEF_NO);
    }
}


/*
*********************************************************************************************************
*                                      FSDev_IDE_BSP_DMA_Start()
*
* Description : Setup DMA for command (initialize channel).
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               p_data      Pointer to memory buffer.
*
*               cnt         Number of octets to transfer.
*
*               mode_type   Transfer mode type :
*
*                               FS_DEV_IDE_MODE_TYPE_DMA     Multiword DMA mode.
*                               FS_DEV_IDE_MODE_TYPE_UDMA    Ultra-DMA mode.
*
*               rd          Indicates whether transfer is read or write :
*
*                               DEF_YES    Transfer is read.
*                               DEF_NO     Transfer is write.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_IDE_RdData(),
*               FSDev_IDE_WrData().
*
* Note(s)     : (1) DMA setup occurs before the command is executed (in 'FSDev_IDE_BSP_CmdWr()'.
*					Afterwards, data transmission completion must be confirmed (in 'FSDev_IDE_BSP_DMA_End()')
*					before the driver checks the command status.
*
*               (2) If the return value of 'FSDev_IDE_BSP_GetModesSupported()' does NOT include
*                  'FS_DEV_IDE_MODE_TYPE_DMA' or 'FS_DEV_IDE_MODE_TYPE_UDMA', this function need not
*                   be implemented; it will never be called.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_DMA_Start (FS_QTY        unit_nbr,
                               void         *p_data,
                               CPU_SIZE_T    cnt,
                               FS_FLAGS      mode_type,
                               CPU_BOOLEAN   rd)
{

}


/*
*********************************************************************************************************
*                                       FSDev_IDE_BSP_DMA_End()
*
* Description : End DMA transfer (& uninitialize channel).
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               p_data      Pointer to memory buffer.
*
*               cnt         Number of octets that should have been transfered.
*
*               rd          Indicates whether transfer was read or write :
*
*                               DEF_YES    Transfer was read.
*                               DEF_NO     Transfer was write.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_IDE_RdData(),
*               FSDev_IDE_WrData().
*
* Note(s)     : (1) See 'FSDev_IDE_BSP_DMA_Start()  Note #1'.
*
*               (2) When this function returns, the host controller should be setup to transmit commands
*                   in PIO mode.
*
*               (3) If the return value of 'FSDev_IDE_BSP_GetModesSupported()' does NOT include
*                  'FS_DEV_IDE_MODE_TYPE_DMA' or 'FS_DEV_IDE_MODE_TYPE_UDMA', this function need not
*                   be implemented; it will never be called.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_DMA_End (FS_QTY        unit_nbr,
                             void         *p_data,
                             CPU_SIZE_T    cnt,
                             CPU_BOOLEAN   rd)
{

}


/*
*********************************************************************************************************
*                                      FSDev_IDE_BSP_GetDrvNbr()
*
* Description : Get IDE drive number.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : Drive number (0 or 1).
*
* Caller(s)   : FSDev_IDE_Refresh().
*
* Note(s)     : (1) Two IDE devices may be accessed on the same bus by setting the DRV bit of the
*                   drive/head register.  If the bit should be clear, this function should return 0; if
*                   the bit should be set, this function should return 1.
*********************************************************************************************************
*/

CPU_INT08U  FSDev_IDE_BSP_GetDrvNbr (FS_QTY  unit_nbr)
{
    CPU_INT08U  drv_nbr;


    drv_nbr = 0u;
    return (drv_nbr);
}


/*
*********************************************************************************************************
*                                  FSDev_IDE_BSP_GetModesSupported()
*
* Description : Get supported transfer modes.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : Bit-mapped variable indicating supported transfer mode(s); should be the bitwise OR of
*               one or more of :
*
*                   FS_DEV_IDE_MODE_TYPE_PIO     PIO mode supported.
*                   FS_DEV_IDE_MODE_TYPE_DMA     Multiword DMA mode supported.
*                   FS_DEV_IDE_MODE_TYPE_UDMA    Ultra-DMA mode supported.
*
* Caller(s)   : FSDev_IDE_Refresh().
*
* Note(s)     : (1) See 'fs_dev_ide.h  MODE DEFINES'.
*********************************************************************************************************
*/

FS_FLAGS   FSDev_IDE_BSP_GetModesSupported (FS_QTY  unit_nbr)
{
    CPU_INT08U  mode_types;


    mode_types = FS_DEV_IDE_MODE_TYPE_PIO;
    return (mode_types);
}


/*
*********************************************************************************************************
*                                       FSDev_IDE_BSP_SetMode()
*
* Description : Set transfer mode timings.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
*               mode_type   Transfer mode type :
*
*                               FS_DEV_IDE_MODE_TYPE_PIO     PIO mode.
*                               FS_DEV_IDE_MODE_TYPE_DMA     Multiword DMA mode.
*                               FS_DEV_IDE_MODE_TYPE_UDMA    Ultra-DMA mode.
*
*               mode        Transfer mode, between 0 and maximum mode supported for mode type by device
*                           (inclusive).
*
* Return(s)   : Mode selected; should be between 0 and 'mode', inclusive.
*
* Caller(s)   : FSDev_IDE_Refresh().
*
* Note(s)     : (1) If DMA is supported, two transfer modes will be setup.  The first will be a PIO mode;
*                   the second will be a Multiword DMA or Ultra-DMA mode.  Thereafter, the host controller
*                   or bus interface must be capable of both PIO and DMA transfers.
*********************************************************************************************************
*/

CPU_INT08U  FSDev_IDE_BSP_SetMode (FS_QTY      unit_nbr,
                                   FS_FLAGS    mode_type,
                                   CPU_INT08U  mode)
{
    CPU_INT08U  mode_sel;


    mode_sel = mode;

    return (mode_sel);
}


/*
*********************************************************************************************************
*                                          FSDev_IDE_BSP_Dly400_ns()
*
* Description : Delay for 400 ns.
*
* Argument(s) : unit_nbr    Unit number of IDE device.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_IDE_BSP_Dly400_ns (FS_QTY  unit_nbr)
{
    volatile  CPU_INT32U  i;


    for (i = 0u; i < 30u; i++) {
        ;
    }

    (void)&i;
}
