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
*                                             SD/MMC CARD
*                                              CARD MODE
*
*                                BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                    NXP LPC3180 ON THE PHYTECH PHYCORE LPC3180 EVALUATION BOARD
*
* Filename      : fs_dev_sd_card_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The LPC3180 SD card interface is described in documentation from NXP :
*
*                         UM10198.  LPC3180 User manual.  Rev.02 -- 27 July 2007.  "Chapter 16:
*                         SD card interface".
*
*                 (2) (a) LPC3180_CLK_FREQ should be set to the value of the LPC3180's core clock frequency.
*
*                     (b) A function, LPC3180_SD_Card_BSP_IntVectSet(), must be defined in fs_bsp.c/.h :
*
*                             void  LPC3180_SD_Card_BSP_IntVectSet (CPU_FNCT_VOID  handler);
*
*                         This function should register 'handler' as the ISR for the SD0 interrupt on the
*                         main interrupt controller (MIC).
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_SD_CARD_BSP_MODULE
#include  <fs_dev_sd_card.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  LPC3180_CLK_FREQ                          208000000u    /* See 'FSDev_SD_Card_BSP_SetClkFreq()'.        */

#define  LPC3180_RAM_BUF_ADDR                     ((CPU_INT32U)&LPC3180_RAM_Buf[0])
#define  LPC3180_RAM_BUF_SIZE                           (sizeof(LPC3180_RAM_Buf))

#define  LPC3180_INT_ID_SD0                               15u
#define  LPC3180_INT_ID_SD1                               13u
#define  LPC3180_INT_ID_DMA                               28u

#define  LPC2XXX_WAIT_TIMEOUT_ms                       1000u

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  LPC3180_REG_MS_CTRL                        (*(CPU_REG32 *)(0x40004080u))
#define  LPC3180_REG_DMACLK_CTRL                    (*(CPU_REG32 *)(0x400040E8u))

#define  LPC3180_REG_MIC_ER                         (*(CPU_REG32 *)(0x40008000u))
#define  LPC3180_REG_MIC_APR                        (*(CPU_REG32 *)(0x4000800Cu))
#define  LPC3180_REG_MIC_ATR                        (*(CPU_REG32 *)(0x40008010u))
#define  LPC3180_REG_MIC_IPR                        (*(CPU_REG32 *)(0x40008014u))

#define  LPC2XXX_REG_MCI_BASE                              0x20098000u
#define  LPC2XXX_REG_MCIPOWER                       (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x000u))
#define  LPC2XXX_REG_MCICLOCK                       (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x004u))
#define  LPC2XXX_REG_MCIARGUMENT                    (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x008u))
#define  LPC2XXX_REG_MCICOMMAND                     (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x00Cu))
#define  LPC2XXX_REG_MCIRESPCMD                     (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x010u))
#define  LPC2XXX_REG_MCIRESPONSE0                   (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x014u))
#define  LPC2XXX_REG_MCIRESPONSE1                   (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x018u))
#define  LPC2XXX_REG_MCIRESPONSE2                   (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x01Cu))
#define  LPC2XXX_REG_MCIRESPONSE3                   (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x020u))
#define  LPC2XXX_REG_MCIDATATIMER                   (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x024u))
#define  LPC2XXX_REG_MCIDATALENGTH                  (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x028u))
#define  LPC2XXX_REG_MCIDATACTRL                    (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x02Cu))
#define  LPC2XXX_REG_MCIDATACNT                     (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x030u))
#define  LPC2XXX_REG_MCISTATUS                      (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x034u))
#define  LPC2XXX_REG_MCICLEAR                       (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x038u))
#define  LPC2XXX_REG_MCIMASK0                       (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x03Cu))
#define  LPC2XXX_REG_MCIMASK1                       (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x040u))
#define  LPC2XXX_REG_MCIFIFOCNT                     (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x048u))
#define  LPC2XXX_REG_MCIFIFO                        (*(CPU_REG32 *)(LPC2XXX_REG_MCI_BASE   + 0x080u))
#define  LPC2XXX_ADDR_MCIFIFO                                      (LPC2XXX_REG_MCI_BASE   + 0x080u)

#define  LPC2XXX_REG_GPDMA_BASE                            0x31000000u
#define  LPC2XXX_REG_GPDMA_DMACINTSTATUS            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x000u))
#define  LPC2XXX_REG_GPDMA_DMACINTTCSTATUS          (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x004u))
#define  LPC2XXX_REG_GPDMA_DMACINTTCCLEAR           (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x008u))
#define  LPC2XXX_REG_GPDMA_DMACINTERRORSTATUS       (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x00Cu))
#define  LPC2XXX_REG_GPDMA_DMACINTERRCLR            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x010u))
#define  LPC2XXX_REG_GPDMA_DMACRAWINTTCSTATUS       (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x014u))
#define  LPC2XXX_REG_GPDMA_DMACRAWINTERRORSTATUS    (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x018u))
#define  LPC2XXX_REG_GPDMA_DMACENBLDCHNS            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x01Cu))
#define  LPC2XXX_REG_GPDMA_DMACSOFTBREQ             (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x020u))
#define  LPC2XXX_REG_GPDMA_DMACSOFTSREQ             (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x024u))
#define  LPC2XXX_REG_GPDMA_DMACSOFTLBREQ            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x028u))
#define  LPC2XXX_REG_GPDMA_DMACSOFTLSREQ            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x02Cu))
#define  LPC2XXX_REG_GPDMA_DMACCONFIGURATION        (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x030u))
#define  LPC2XXX_REG_GPDMA_DMACSYNC                 (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x034u))
#define  LPC2XXX_REG_GPDMA_DMACC0SRCADDR            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x100u))
#define  LPC2XXX_REG_GPDMA_DMACC0DESTADDR           (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x104u))
#define  LPC2XXX_REG_GPDMA_DMACC0LLI                (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x108u))
#define  LPC2XXX_REG_GPDMA_DMACC0CONTROL            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x10Cu))
#define  LPC2XXX_REG_GPDMA_DMACC0CONFIGURATION      (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x110u))
#define  LPC2XXX_REG_GPDMA_DMACC1SRCADDR            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x120u))
#define  LPC2XXX_REG_GPDMA_DMACC1DESTADDR           (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x124u))
#define  LPC2XXX_REG_GPDMA_DMACC1LLI                (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x128u))
#define  LPC2XXX_REG_GPDMA_DMACC1CONTROL            (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x12Cu))
#define  LPC2XXX_REG_GPDMA_DMACC1CONFIGURATION      (*(CPU_REG32 *)(LPC2XXX_REG_GPDMA_BASE + 0x130u))

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  LPC3180_BIT_MS_CTRL_MSSDIO_PULLUP          DEF_BIT_09
#define  LPC3180_BIT_MS_CTRL_MSSDIO23_PULLUP        DEF_BIT_08
#define  LPC3180_BIT_MS_CTRL_MSSDIO1_PULLUP         DEF_BIT_07
#define  LPC3180_BIT_MS_CTRL_MSSDIO0_PULLUP         DEF_BIT_06
#define  LPC3180_BIT_MS_CTRL_CLK                    DEF_BIT_05
#define  LPC3180_BIT_MS_CTRL_DIV_MASK               0x0000000Fu
#define  LPC3180_BIT_MS_CTRL_DIV_04                 0x00000004u

#define  LPC3180_BIT_DMACLK_CTRL_EN                 DEF_BIT_00

#define  LPC2XXX_BIT_MCIPOWER_CTRL_POWEROFF             0x00u
#define  LPC2XXX_BIT_MCIPOWER_CTRL_POWERUP              0x02u
#define  LPC2XXX_BIT_MCIPOWER_CTRL_POWERON              0x03u
#define  LPC2XXX_BIT_MCIPOWER_CTRL_POWER_MASK           0x03u
#define  LPC2XXX_BIT_MCIPOWER_CTRL_OPENDRAIN        DEF_BIT_06
#define  LPC2XXX_BIT_MCIPOWER_CTRL_ROD              DEF_BIT_07

#define  LPC2XXX_BIT_MCICLOCK_CLKDIV_MASK             0x00FFu
#define  LPC2XXX_BIT_MCICLOCK_ENABLE                DEF_BIT_08
#define  LPC2XXX_BIT_MCICLOCK_PWRSAVE               DEF_BIT_09
#define  LPC2XXX_BIT_MCICLOCK_BYPASS                DEF_BIT_10
#define  LPC2XXX_BIT_MCICLOCK_WIDEBUS               DEF_BIT_11

#define  LPC2XXX_BIT_MCICOMMAND_CMDINDEX_MASK         0x003Fu
#define  LPC2XXX_BIT_MCICOMMAND_RESPONSE            DEF_BIT_06
#define  LPC2XXX_BIT_MCICOMMAND_LONGRSP             DEF_BIT_07
#define  LPC2XXX_BIT_MCICOMMAND_INTERRUPT           DEF_BIT_08
#define  LPC2XXX_BIT_MCICOMMAND_PENDING             DEF_BIT_09
#define  LPC2XXX_BIT_MCICOMMAND_ENABLE              DEF_BIT_10

#define  LPC2XXX_BIT_MCIRESPCMD_RESPCMD_MASK         0x003Fu

#define  LPC2XXX_BIT_MCIDATALENGTH_DATALENGTH_MASK   0xFFFFu

#define  LPC2XXX_BIT_MCIDATACTRL_ENABLE             DEF_BIT_00
#define  LPC2XXX_BIT_MCIDATACTRL_DIRECTION          DEF_BIT_01  /* Set = from card to controller.                       */
#define  LPC2XXX_BIT_MCIDATACTRL_MODE               DEF_BIT_02  /* Set = stream data transfer.                          */
#define  LPC2XXX_BIT_MCIDATACTRL_DMAENABLE          DEF_BIT_03
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0001_BYTE    0x00u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0002_BYTE    0x10u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0004_BYTE    0x20u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0008_BYTE    0x30u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0016_BYTE    0x40u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0032_BYTE    0x50u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0064_BYTE    0x60u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0128_BYTE    0x70u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0256_BYTE    0x80u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0512_BYTE    0x90u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_1024_BYTE    0xA0u
#define  LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_2048_BYTE    0xB0u

#define  LPC2XXX_BIT_MCIDATACNT_DATACOUNT_MASK        0xFFFFu

#define  LPC2XXX_BIT_MCISTATUS_CMDCRCFAIL           DEF_BIT_00
#define  LPC2XXX_BIT_MCISTATUS_DATACRCFAIL          DEF_BIT_01
#define  LPC2XXX_BIT_MCISTATUS_CMDTIMEOUT           DEF_BIT_02
#define  LPC2XXX_BIT_MCISTATUS_DATATIMEOUT          DEF_BIT_03
#define  LPC2XXX_BIT_MCISTATUS_TXUNDERRUN           DEF_BIT_04
#define  LPC2XXX_BIT_MCISTATUS_RXOVERRUN            DEF_BIT_05
#define  LPC2XXX_BIT_MCISTATUS_CMDRESPEND           DEF_BIT_06
#define  LPC2XXX_BIT_MCISTATUS_CMDSENT              DEF_BIT_07
#define  LPC2XXX_BIT_MCISTATUS_DATAEND              DEF_BIT_08
#define  LPC2XXX_BIT_MCISTATUS_STARTBITERR          DEF_BIT_09
#define  LPC2XXX_BIT_MCISTATUS_DATABLOCKEND         DEF_BIT_10
#define  LPC2XXX_BIT_MCISTATUS_CMDACTIVE            DEF_BIT_11
#define  LPC2XXX_BIT_MCISTATUS_TXACTIVE             DEF_BIT_12
#define  LPC2XXX_BIT_MCISTATUS_RXACTIVE             DEF_BIT_13
#define  LPC2XXX_BIT_MCISTATUS_TXFIFOHALFEMPTY      DEF_BIT_14
#define  LPC2XXX_BIT_MCISTATUS_RXFIFOHALFFULL       DEF_BIT_15
#define  LPC2XXX_BIT_MCISTATUS_TXFIFOFULL           DEF_BIT_16
#define  LPC2XXX_BIT_MCISTATUS_RXFIFOFULL           DEF_BIT_17
#define  LPC2XXX_BIT_MCISTATUS_TXFIFOEMPTY          DEF_BIT_18
#define  LPC2XXX_BIT_MCISTATUS_RXFIFOEMPTY          DEF_BIT_19
#define  LPC2XXX_BIT_MCISTATUS_TXDATAVLBL           DEF_BIT_20
#define  LPC2XXX_BIT_MCISTATUS_RXDATAAVLBL          DEF_BIT_21
#define  LPC2XXX_BIT_MCISTATUS_ERR                 (LPC2XXX_BIT_MCISTATUS_DATACRCFAIL | LPC2XXX_BIT_MCISTATUS_DATATIMEOUT | LPC2XXX_BIT_MCISTATUS_TXUNDERRUN | \
                                                    LPC2XXX_BIT_MCISTATUS_RXOVERRUN   | LPC2XXX_BIT_MCISTATUS_STARTBITERR)

#define  LPC2XXX_BIT_MCICLEAR_CMDCRCFAILCLR         DEF_BIT_00
#define  LPC2XXX_BIT_MCICLEAR_DATACRCFAILCLR        DEF_BIT_01
#define  LPC2XXX_BIT_MCICLEAR_CMDTIMEOUTCLR         DEF_BIT_02
#define  LPC2XXX_BIT_MCICLEAR_DATATIMEOUTCLR        DEF_BIT_03
#define  LPC2XXX_BIT_MCICLEAR_TXUNDERRUNCLR         DEF_BIT_04
#define  LPC2XXX_BIT_MCICLEAR_RXOVERRUNCLR          DEF_BIT_05
#define  LPC2XXX_BIT_MCICLEAR_CMDRESPENDCLR         DEF_BIT_06
#define  LPC2XXX_BIT_MCICLEAR_CMDSENTCLR            DEF_BIT_07
#define  LPC2XXX_BIT_MCICLEAR_DATAENDCLR            DEF_BIT_08
#define  LPC2XXX_BIT_MCICLEAR_STARTBITERRCLR        DEF_BIT_09
#define  LPC2XXX_BIT_MCICLEAR_DATABLOCKENDCLR       DEF_BIT_10
#define  LPC2XXX_BIT_MCICLEAR_ALLCLR               (LPC2XXX_BIT_MCICLEAR_CMDCRCFAILCLR  | LPC2XXX_BIT_MCICLEAR_DATACRCFAILCLR | LPC2XXX_BIT_MCICLEAR_CMDTIMEOUTCLR | \
                                                    LPC2XXX_BIT_MCICLEAR_DATATIMEOUTCLR | LPC2XXX_BIT_MCICLEAR_TXUNDERRUNCLR  | LPC2XXX_BIT_MCICLEAR_RXOVERRUNCLR  | \
                                                    LPC2XXX_BIT_MCICLEAR_CMDRESPENDCLR  | LPC2XXX_BIT_MCICLEAR_CMDSENTCLR     | LPC2XXX_BIT_MCICLEAR_DATAENDCLR    | \
                                                    LPC2XXX_BIT_MCICLEAR_STARTBITERRCLR | LPC2XXX_BIT_MCICLEAR_DATABLOCKENDCLR)


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

static  FS_ERR      FSDev_SD_Card_BSP_Err;
static  FS_OS_SEM   FSDev_SD_Card_OS_Sem;

static  CPU_INT32U  LPC3180_RAM_Buf[(512 * 8) / 4];


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

static  void  FSDev_SD_Card_BSP_ISR_Handler(void);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                    FILE SYSTEM SD CARD FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      FSDev_SD_Card_BSP_Open()
*
* Description : Open (initialize) SD/MMC card interface.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : DEF_OK,   if interface was opened.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FSDev_SD_Card_Refresh().
*
* Note(s)     : (1) This function will be called EVERY time the device is opened.
*
*               (2) The SD/MMC slot is connected to the following pins :
*
*                   ----------------------------------------------
*                   | LPC3180 NAME | LPC3180 PIO # | SD/MMC NAME |
*                   ----------------------------------------------
*                   |   MS_DIO3    |     -----     |     DAT3    |
*                   |   MS_BS      |     -----     |     CMD     |
*                   |   MS_SCLK    |     -----     |     CLK     |
*                   |   MS_DIO1    |     -----     |     DAT1    |
*                   |   MS_DIO2    |     -----     |     DAT2    |
*                   |   MS_DIO0    |     -----     |     DAT0    |
*                   ----------------------------------------------
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_SD_Card_BSP_Open (FS_QTY  unit_nbr)
{
    CPU_INT32U   i;
    CPU_BOOLEAN  ok;


    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

    ok = FS_OS_SemCreate(&FSDev_SD_Card_OS_Sem, 0u);
    if (ok == DEF_FAIL) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Open(): Could not create sem.\r\n"));
        return (DEF_FAIL);
    }



                                                                /* ---------------------- EN DMA ---------------------- */
    LPC3180_REG_DMACLK_CTRL |=  LPC3180_BIT_DMACLK_CTRL_EN;     /* En periph clk.                                       */
    LPC3180_REG_MIC_ER      &= ~DEF_BIT(LPC3180_INT_ID_DMA);    /* Dis int.                                             */

                                                                /* Clear all interrupts on channel 0 and 1.             */
    LPC2XXX_REG_GPDMA_DMACINTTCCLEAR    = 0x03u;
    LPC2XXX_REG_GPDMA_DMACINTERRCLR     = 0x03u;

    LPC2XXX_REG_GPDMA_DMACCONFIGURATION = 0x01u;                /* Enable DMA channels, little endian.                  */
    while (DEF_BIT_IS_CLR(LPC2XXX_REG_GPDMA_DMACCONFIGURATION, 0x01u) == DEF_YES) {
        ;
    }



                                                                /* ---------------------- EN MCI ---------------------- */
    LPC3180_REG_MS_CTRL = LPC3180_BIT_MS_CTRL_DIV_04            /* En periph clk.                                       */
                        | LPC3180_BIT_MS_CTRL_CLK
                        | LPC3180_BIT_MS_CTRL_MSSDIO_PULLUP;

    if (DEF_BIT_IS_SET(LPC2XXX_REG_MCICLOCK, LPC2XXX_BIT_MCICLOCK_ENABLE) == DEF_YES) {
        DEF_BIT_CLR(LPC2XXX_REG_MCICLOCK, LPC2XXX_BIT_MCICLOCK_ENABLE);
    }

    if ((LPC2XXX_REG_MCIPOWER & LPC2XXX_BIT_MCIPOWER_CTRL_POWER_MASK) != LPC2XXX_BIT_MCIPOWER_CTRL_POWEROFF) {
        LPC2XXX_REG_MCIPOWER = LPC2XXX_BIT_MCIPOWER_CTRL_POWEROFF;
    }

    for (i = 0u; i < 1000u; i++) {
        ;
    }

    LPC2XXX_REG_MCIMASK0    = 0u;                               /* Dis all ints.                                        */
    LPC2XXX_REG_MCIMASK1    = 0u;
    LPC3180_REG_MIC_ER     &= ~DEF_BIT(LPC3180_INT_ID_SD0);
    LPC3180_REG_MIC_ER     &= ~DEF_BIT(LPC3180_INT_ID_SD1);

    LPC2XXX_REG_MCICOMMAND  = 0u;
    LPC2XXX_REG_MCIDATACTRL = 0u;
    LPC2XXX_REG_MCICLEAR    = LPC2XXX_BIT_MCICLEAR_ALLCLR;


    LPC2XXX_REG_MCIPOWER    = LPC2XXX_BIT_MCIPOWER_CTRL_POWERUP;/* Power up.                                            */
    while ((LPC2XXX_REG_MCIPOWER & LPC2XXX_BIT_MCIPOWER_CTRL_POWER_MASK) == LPC2XXX_BIT_MCIPOWER_CTRL_POWEROFF) {
        ;
    }
    for (i = 0u; i < 1000u; i++) {
        ;
    }


                                                                /* Set dflt clk freq.                                   */
    FSDev_SD_Card_BSP_SetClkFreq(unit_nbr, FS_DEV_SD_DFLT_CLK_SPD);


    LPC2XXX_REG_MCIPOWER   |= LPC2XXX_BIT_MCIPOWER_CTRL_POWERON;/* Power on.                                            */
    for (i = 0u; i < 1000u; i++) {
        ;
    }

                                                                /* Set int handler.                                     */
    LPC3180_SD_Card_BSP_IntVectSet(FSDev_SD_Card_BSP_ISR_Handler);
    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      FSDev_SD_Card_BSP_Close()
*
* Description : Close (unitialize) SD/MMC card interface.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_Close().
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_Close (FS_QTY  unit_nbr)
{
    CPU_BOOLEAN  ok;


    (void)&unit_nbr;                                            /* Prevent compiler warning.                            */

    ok = FS_OS_SemDel(&FSDev_SD_Card_OS_Sem);                 /* Del sem.                                             */
    if (ok == DEF_FAIL) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Close(): Could not del sem.\r\n"));
    }

    LPC3180_REG_MIC_ER      &= ~DEF_BIT(LPC3180_INT_ID_SD0);
    LPC3180_REG_MIC_ER      &= ~DEF_BIT(LPC3180_INT_ID_SD1);

    LPC3180_REG_MS_CTRL     &= ~LPC3180_BIT_MS_CTRL_CLK;
    LPC3180_REG_DMACLK_CTRL &= ~LPC3180_BIT_DMACLK_CTRL_EN;
}


/*
*********************************************************************************************************
*                                      FSDev_SD_Card_BSP_Lock()
*
* Description : Acquire SD/MMC card bus lock.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) This function will be called before the SD/MMC driver begins to access the SD/MMC
*                   card bus.  The application should NOT use the same bus to access another device until
*                   the matching call to 'FSDev_SD_Card_BSP_Unlock()' has been made.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_Lock (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                     FSDev_SD_Card_BSP_Unlock()
*
* Description : Release SD/MMC card bus lock.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) 'FSDev_SD_Card_BSP_Lock()' will be called before the SD/MMC driver begins to access
*                   the SD/MMC card bus.  The application should NOT use the same bus to access another
*                   device until the matching call to this function has been made.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_Unlock (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
}


/*
*********************************************************************************************************
*                                      FSDev_SD_Card_BSP_CmdStart()
*
* Description : Start a command.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               p_cmd       Pointer to command to transmit (see Note #2).
*
*               p_data      Pointer to buffer address for DMA transfer (see Note #3).
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_DEV_SD_CARD_ERR_NONE       No error.
*                               FS_DEV_SD_CARD_ERR_NO_CARD    No card present.
*                               FS_DEV_SD_CARD_ERR_BUSY       Controller is busy.
*                               FS_DEV_SD_CARD_ERR_UNKNOWN    Unknown or other error.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_CmdStart (FS_QTY               unit_nbr,
                                  FS_DEV_SD_CARD_CMD  *p_cmd,
                                  void                *p_data,
                                  FS_ERR              *p_err)
{
    CPU_INT32U  status;
    CPU_INT32U  command;
    CPU_INT32U  i;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
   (void)&p_data;

    LPC2XXX_REG_MCICOMMAND = 0u;
    LPC2XXX_REG_MCICLEAR   = LPC2XXX_BIT_MCICLEAR_ALLCLR;       /* Clr all ints.                                        */

    for (i = 0u; i < 32u; i++) {
        ;
    }

    status = LPC2XXX_REG_MCISTATUS;                             /* Chk if controller busy.                              */
    if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_CMDACTIVE) == DEF_YES) {
        LPC2XXX_REG_MCICOMMAND = 0u;
        LPC2XXX_REG_MCICLEAR   = status;
    }

    status = LPC2XXX_REG_MCISTATUS;
    if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_CMDACTIVE) == DEF_YES) {
       *p_err = FS_DEV_SD_CARD_ERR_BUSY;
        return;
    }

    for (i = 0u; i < 32u; i++) {
        ;
    }


                                                                /* -------------------- SET COMMAND ------------------- */
    command = (p_cmd->Cmd) | LPC2XXX_BIT_MCICOMMAND_ENABLE;     /* Set cmd ix.                                          */
    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
        command |= LPC2XXX_BIT_MCICOMMAND_RESPONSE;
        if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
            command |= LPC2XXX_BIT_MCICOMMAND_LONGRSP;
        }
    }



                                                                /* -------------------- SET COMMAND ------------------- */
    LPC2XXX_REG_MCIARGUMENT = p_cmd->Arg;
    LPC2XXX_REG_MCICOMMAND  = command;

   *p_err = FS_DEV_SD_CARD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   FSDev_SD_Card_BSP_CmdWaitEnd()
*
* Description : Wait for command to end & get command response.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               p_cmd       Pointer to command that is ending.
*
*               p_resp      Pointer to buffer that will receive command response, if any.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_DEV_SD_CARD_ERR_NONE            No error.
*                               FS_DEV_SD_CARD_ERR_NO_CARD         No card present.
*                               FS_DEV_SD_CARD_ERR_UNKNOWN         Unknown or other error.
*                               FS_DEV_SD_CARD_ERR_WAIT_TIMEOUT    Timeout in waiting for command response.
*                               FS_DEV_SD_CARD_ERR_RESP_TIMEOUT    Timeout in receiving command response.
*                               FS_DEV_SD_CARD_ERR_RESP_CHKSUM     Error in response checksum.
*                               FS_DEV_SD_CARD_ERR_RESP_CMD_IX     Response command index error.
*                               FS_DEV_SD_CARD_ERR_RESP_END_BIT    Response end bit error.
*                               FS_DEV_SD_CARD_ERR_RESP            Other response error.
*                               FS_DEV_SD_CARD_ERR_DATA            Other data err.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_CmdWaitEnd (FS_QTY               unit_nbr,
                                    FS_DEV_SD_CARD_CMD  *p_cmd,
                                    CPU_INT32U          *p_resp,
                                    FS_ERR              *p_err)
{
    CPU_INT32U   status;
    CPU_BOOLEAN  done;
    CPU_INT08U   cmd_resp;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

                                                                /* ----------------- WAIT FOR CMD END ----------------- */
    done = DEF_NO;
    while (done == DEF_NO) {
        status = LPC2XXX_REG_MCISTATUS;
                                                                /* If cmd timeout ...                                   */
        if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_CMDTIMEOUT) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR    = status;
            LPC2XXX_REG_MCICOMMAND  = 0u;
            LPC2XXX_REG_MCIARGUMENT = 0xFFFFFFFFu;
           *p_err = FS_DEV_SD_CARD_ERR_RESP_TIMEOUT;            /* ... rtn err.                                         */
            return;
        }
                                                                /* If CRC chk failed           ...                      */
        if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_CMDCRCFAIL) == DEF_YES) {
                                                                /* ... but CRC should be valid ...                      */
            if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_CRC_VALID) == DEF_YES) {
                LPC2XXX_REG_MCICLEAR    = status;
                LPC2XXX_REG_MCICOMMAND  = 0u;
                LPC2XXX_REG_MCIARGUMENT = 0xFFFFFFFFu;
               *p_err = FS_DEV_SD_CARD_ERR_RESP_CHKSUM;         /* ... rtn err.                                         */
                return;

            } else {
                LPC2XXX_REG_MCICLEAR = status;
                done                 = DEF_YES;
            }

        } else if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_CMDRESPEND) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR = status;
            done                 = DEF_YES;


        } else if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_CMDSENT) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR    = status;
            LPC2XXX_REG_MCICOMMAND  = 0u;
            LPC2XXX_REG_MCIARGUMENT = 0xFFFFFFFFu;

            if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
               *p_err = FS_DEV_SD_CARD_ERR_RESP;
            } else {
               *p_err = FS_DEV_SD_CARD_ERR_NONE;
            }

            return;
        }
    }



                                                                /* ------------------- GET CMD RESP ------------------- */
                                                                /* If resp cmd ix not correct ...                       */
    cmd_resp = LPC2XXX_REG_MCIRESPCMD & LPC2XXX_BIT_MCIRESPCMD_RESPCMD_MASK;
    if (cmd_resp != p_cmd->Cmd) {
                                                                /* ... but should be          ...                       */
        if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_IX_VALID) == DEF_YES) {
            LPC2XXX_REG_MCICOMMAND  = 0u;
            LPC2XXX_REG_MCIARGUMENT = 0xFFFFFFFFu;
           *p_err = FS_DEV_SD_CARD_ERR_RESP_CMD_IX;             /* ... rtn err.                                         */
            return;
        }
    }


                                                                /* If no resp expected ...                              */
    if (DEF_BIT_IS_CLR(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
        LPC2XXX_REG_MCICOMMAND  = 0u;
        LPC2XXX_REG_MCIARGUMENT = 0xFFFFFFFFu;
       *p_err = FS_DEV_SD_CARD_ERR_NONE;                        /* ... rtn w/o err.                                     */
        return;
    }

                                                                /* Rd resp.                                             */
    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
       *p_resp = LPC2XXX_REG_MCIRESPONSE0;
        p_resp++;
       *p_resp = LPC2XXX_REG_MCIRESPONSE1;
        p_resp++;
       *p_resp = LPC2XXX_REG_MCIRESPONSE2;
        p_resp++;
       *p_resp = LPC2XXX_REG_MCIRESPONSE3;
    } else {
       *p_resp = LPC2XXX_REG_MCIRESPONSE0;
    }

    LPC2XXX_REG_MCICOMMAND  = 0u;
    LPC2XXX_REG_MCIARGUMENT = 0xFFFFFFFFu;
   *p_err = FS_DEV_SD_CARD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    FSDev_SD_Card_BSP_CmdDataRd()
*
* Description : Read data following command.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               p_cmd       Pointer to command that was started.
*
*               p_dest      Pointer to destination buffer.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_DEV_SD_CARD_ERR_NONE              No error.
*                               FS_DEV_SD_CARD_ERR_NO_CARD           No card present.
*                               FS_DEV_SD_CARD_ERR_UNKNOWN           Unknown or other error.
*                               FS_DEV_SD_CARD_ERR_WAIT_TIMEOUT      Timeout in waiting for data.
*                               FS_DEV_SD_CARD_ERR_DATA_OVERRUN      Data overrun.
*                               FS_DEV_SD_CARD_ERR_DATA_TIMEOUT      Timeout in receiving data.
*                               FS_DEV_SD_CARD_ERR_DATA_CHKSUM       Error in data checksum.
*                               FS_DEV_SD_CARD_ERR_DATA_START_BIT    Data start bit error.
*                               FS_DEV_SD_CARD_ERR_DATA              Other data error.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_RdData().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_CmdDataRd (FS_QTY               unit_nbr,
                                   FS_DEV_SD_CARD_CMD  *p_cmd,
                                   void                *p_dest,
                                   FS_ERR              *p_err)
{
    CPU_INT32U   datactrl;
    CPU_INT32U   i;
    CPU_INT32U   rd_size;
    CPU_BOOLEAN  ok;
    CPU_SR_ALLOC();


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

                                                                /* ------------------- SET DATA CTRL ------------------ */
    datactrl = LPC2XXX_BIT_MCIDATACTRL_ENABLE                   /* Set data dir.                                        */
             | LPC2XXX_BIT_MCIDATACTRL_DIRECTION
             | LPC2XXX_BIT_MCIDATACTRL_DMAENABLE;

    switch (p_cmd->BlkSize) {                                   /* Set blk size.                                        */
        case 1u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0001_BYTE;    break;
        case 2u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0002_BYTE;    break;
        case 4u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0004_BYTE;    break;
        case 8u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0008_BYTE;    break;
        case 16u:    datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0016_BYTE;    break;
        case 32u:    datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0032_BYTE;    break;
        case 64u:    datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0064_BYTE;    break;
        case 128u:   datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0128_BYTE;    break;
        case 256u:   datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0256_BYTE;    break;
        case 1024u:  datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_1024_BYTE;    break;
        case 2048u:  datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_2048_BYTE;    break;

        default:
        case 512u:   datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0512_BYTE;    break;
    }

    rd_size                               = p_cmd->BlkSize * p_cmd->BlkCnt;

    CPU_CRITICAL_ENTER();
    LPC2XXX_REG_MCIDATALENGTH             = rd_size;

    LPC2XXX_REG_GPDMA_DMACINTTCCLEAR      = DEF_BIT_01;
    LPC2XXX_REG_GPDMA_DMACINTERRCLR       = DEF_BIT_01;

    LPC2XXX_REG_GPDMA_DMACC1SRCADDR       = LPC2XXX_ADDR_MCIFIFO;   /* Ch1 set for P2M transfer from MCI_FIFO to mem.   */
    LPC2XXX_REG_GPDMA_DMACC1DESTADDR      = LPC3180_RAM_BUF_ADDR;
                                                                    /* The burst size is set to 8, size is 8 bit too.   */
    LPC2XXX_REG_GPDMA_DMACC1CONTROL       = (p_cmd->BlkSize & 0x0FFFu) | (0x02u << 12) | (0x02u << 15) | (0x02u << 18) | (0x02u << 21) | DEF_BIT_24 | DEF_BIT_27 | DEF_BIT_31;
    LPC2XXX_REG_GPDMA_DMACC1CONFIGURATION|= 0x10001u | (0x04u << 1) | (0x00u << 6) | (0x06u << 11);
    LPC2XXX_REG_GPDMA_DMACCONFIGURATION   = DEF_BIT_00;

    while (DEF_BIT_IS_CLR(LPC2XXX_REG_GPDMA_DMACCONFIGURATION, DEF_BIT_00) == DEF_YES) {
        ;
    }

    LPC2XXX_REG_MCIDATACTRL               = datactrl;

    for (i = 0u; i < 32u; i++) {
        ;
    }



                                                                /* ---------------- WAIT FOR COMPLETION --------------- */
                                                                /* En ints.                                             */
    LPC2XXX_REG_MCIMASK0  |= LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR;
    LPC2XXX_REG_MCIMASK1  |= LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR;
    LPC3180_REG_MIC_ER    |= DEF_BIT(LPC3180_INT_ID_SD0);
    CPU_CRITICAL_EXIT();

                                                                /* Wait for completion.                                 */
    ok = FS_OS_SemPend(&FSDev_SD_Card_OS_Sem, LPC2XXX_WAIT_TIMEOUT_ms);

                                                                /* Chk for DMA completion.                              */
    while (DEF_BIT_IS_CLR(LPC2XXX_REG_GPDMA_DMACRAWINTTCSTATUS, DEF_BIT_01) == DEF_YES) {
        ;
    }



                                                                /* --------------------- CHK & RTN -------------------- */
    LPC3180_REG_MIC_ER   &= ~DEF_BIT(LPC3180_INT_ID_SD0);       /* Dis ints.                                            */
    LPC2XXX_REG_MCIMASK0 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
    LPC2XXX_REG_MCIMASK1 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
    LPC2XXX_REG_MCICLEAR  =   LPC2XXX_BIT_MCICLEAR_ALLCLR;

    LPC2XXX_REG_GPDMA_DMACINTTCCLEAR      = DEF_BIT_01;         /* Stop DMA.                                            */
    LPC2XXX_REG_GPDMA_DMACINTERRCLR       = DEF_BIT_01;

    LPC2XXX_REG_GPDMA_DMACC1CONTROL       = 0u;
    LPC2XXX_REG_GPDMA_DMACC1CONFIGURATION = 0u;

    if (ok == DEF_YES) {
       *p_err = FSDev_SD_Card_BSP_Err;
        if (*p_err == FS_DEV_SD_CARD_ERR_NONE) {
            Mem_Copy(p_dest, (void *)LPC3180_RAM_BUF_ADDR, rd_size);
        }

    } else {
       *p_err = FS_DEV_SD_CARD_ERR_WAIT_TIMEOUT;
    }
}


/*
*********************************************************************************************************
*                                    FSDev_SD_Card_BSP_CmdDataWr()
*
* Description : Write data following command.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               p_cmd       Pointer to command that was started.
*
*               p_src       Pointer to source buffer.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_DEV_SD_CARD_ERR_NONE              No error.
*                               FS_DEV_SD_CARD_ERR_NO_CARD           No card present.
*                               FS_DEV_SD_CARD_ERR_UNKNOWN           Unknown or other error.
*                               FS_DEV_SD_CARD_ERR_WAIT_TIMEOUT      Timeout in waiting for data.
*                               FS_DEV_SD_CARD_ERR_DATA_UNDERRUN     Data underrun.
*                               FS_DEV_SD_CARD_ERR_DATA_CHKSUM       Err in data checksum.
*                               FS_DEV_SD_CARD_ERR_DATA_START_BIT    Data start bit error.
*                               FS_DEV_SD_CARD_ERR_DATA              Other data error.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_WrData().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_CmdDataWr (FS_QTY               unit_nbr,
                                   FS_DEV_SD_CARD_CMD  *p_cmd,
                                   void                *p_src,
                                   FS_ERR              *p_err)
{
    CPU_INT32U   datactrl;
    CPU_INT32U   i;
    CPU_INT32U   wr_size;
    CPU_BOOLEAN  ok;
    CPU_SR_ALLOC();


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

                                                                /* ------------------- SET DATA CTRL ------------------ */
    wr_size = p_cmd->BlkSize * p_cmd->BlkCnt;
    Mem_Copy((void *)LPC3180_RAM_BUF_ADDR, p_src, wr_size);

    datactrl = LPC2XXX_BIT_MCIDATACTRL_ENABLE                   /* Set data dir.                                        */
             | LPC2XXX_BIT_MCIDATACTRL_DMAENABLE;

    switch (p_cmd->BlkSize) {                                   /* Set blk size.                                        */
        case 1u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0001_BYTE;    break;
        case 2u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0002_BYTE;    break;
        case 4u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0004_BYTE;    break;
        case 8u:     datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0008_BYTE;    break;
        case 16u:    datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0016_BYTE;    break;
        case 32u:    datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0032_BYTE;    break;
        case 64u:    datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0064_BYTE;    break;
        case 128u:   datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0128_BYTE;    break;
        case 256u:   datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0256_BYTE;    break;
        case 1024u:  datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_1024_BYTE;    break;
        case 2048u:  datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_2048_BYTE;    break;

        default:
        case 512u:   datactrl |= LPC2XXX_BIT_MCIDATACTRL_BLOCKSIZE_0512_BYTE;    break;
    }

    CPU_CRITICAL_ENTER();
    LPC2XXX_REG_MCIDATALENGTH             = wr_size;

    LPC2XXX_REG_GPDMA_DMACINTTCCLEAR      = DEF_BIT_01;
    LPC2XXX_REG_GPDMA_DMACINTERRCLR       = DEF_BIT_01;

    LPC2XXX_REG_GPDMA_DMACC1SRCADDR       = LPC3180_RAM_BUF_ADDR;   /* Ch1 set for M2P transfer from mem to MCI_FIFO.   */
    LPC2XXX_REG_GPDMA_DMACC1DESTADDR      = LPC2XXX_ADDR_MCIFIFO;
                                                                    /* The burst size is set to 8, size is 8 bit too.   */
    LPC2XXX_REG_GPDMA_DMACC1CONTROL       = (p_cmd->BlkSize & 0x0FFFu) | (0x02u << 12) | (0x02u << 15) | (0x02u << 18) | (0x02u << 21) | DEF_BIT_24 | DEF_BIT_26 | DEF_BIT_31;
    LPC2XXX_REG_GPDMA_DMACC1CONFIGURATION|= 0x10001u | (0x00u << 1) | (0x04u << 6) | (0x05u << 11);
    LPC2XXX_REG_GPDMA_DMACCONFIGURATION   =  DEF_BIT_00;

    while (DEF_BIT_IS_CLR(LPC2XXX_REG_GPDMA_DMACCONFIGURATION, DEF_BIT_00) == DEF_YES) {
        ;
    }

    LPC2XXX_REG_MCIDATACTRL               = datactrl;

    for (i = 0u; i < 32u; i++) {
        ;
    }


                                                                /* ---------------- WAIT FOR COMPLETION --------------- */
                                                                /* En ints.                                             */
    LPC2XXX_REG_MCIMASK0  |= LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR;
    LPC2XXX_REG_MCIMASK1  |= LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR;
    LPC3180_REG_MIC_ER    |= DEF_BIT(LPC3180_INT_ID_SD0);
    CPU_CRITICAL_EXIT();

                                                                /* Wait for completion.                                 */
    ok = FS_OS_SemPend(&FSDev_SD_Card_OS_Sem, LPC2XXX_WAIT_TIMEOUT_ms);

                                                                /* Chk for DMA completion.                              */
    while (DEF_BIT_IS_CLR(LPC2XXX_REG_GPDMA_DMACRAWINTTCSTATUS, DEF_BIT_01) == DEF_YES) {
        ;
    }



                                                                /* --------------------- CHK & RTN -------------------- */
    LPC3180_REG_MIC_ER   &= ~DEF_BIT(LPC3180_INT_ID_SD0);       /* Dis ints.                                            */
    LPC2XXX_REG_MCIMASK0 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
    LPC2XXX_REG_MCIMASK1 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
    LPC2XXX_REG_MCICLEAR  =   LPC2XXX_BIT_MCICLEAR_ALLCLR;

    LPC2XXX_REG_GPDMA_DMACINTTCCLEAR      = DEF_BIT_01;         /* Stop DMA.                                            */
    LPC2XXX_REG_GPDMA_DMACINTERRCLR       = DEF_BIT_01;

    LPC2XXX_REG_GPDMA_DMACC1CONTROL       = 0u;
    LPC2XXX_REG_GPDMA_DMACC1CONFIGURATION = 0u;


    if (ok == DEF_YES) {
       *p_err = FSDev_SD_Card_BSP_Err;

    } else {
       *p_err = FS_DEV_SD_CARD_ERR_WAIT_TIMEOUT;
    }
}


/*
*********************************************************************************************************
*                                  FSDev_SD_Card_BSP_GetBlkCntMax()
*
* Description : Get maximum number of blocks that can be transferred with a multiple read or multiple
*               write command.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               blk_size    Block size, in octets.
*
* Return(s)   : Maximum number of blocks.
*
* Caller(s)   : FSDev_SD_Card_Refresh().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  FSDev_SD_Card_BSP_GetBlkCntMax (FS_QTY      unit_nbr,
                                            CPU_INT32U  blk_size)
{
    CPU_INT32U  blk_cnt;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    blk_cnt = LPC3180_RAM_BUF_SIZE / blk_size;
    return (blk_cnt);
}


/*
*********************************************************************************************************
*                                 FSDev_SD_Card_BSP_GetBusWidthMax()
*
* Description : Get maximum bus width, in bits.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
* Return(s)   : Maximum bus width.
*
* Caller(s)   : FSDev_SD_Card_Refresh().
*
* Note(s)     : (1) The MMC interface is capable of 1- & 4-bit operation.
*********************************************************************************************************
*/

CPU_INT08U  FSDev_SD_Card_BSP_GetBusWidthMax (FS_QTY  unit_nbr)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    return (4u);
}


/*
*********************************************************************************************************
*                                   FSDev_SD_Card_BSP_SetBusWidth()
*
* Description : Set bus width.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               width       Bus width, in bits.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_Refresh(),
*               FSDev_SD_Card_SetBusWidth().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetBusWidth (FS_QTY      unit_nbr,
                                     CPU_INT08U  width)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    if (width == 4u) {
        LPC2XXX_REG_MCICLOCK |=  LPC2XXX_BIT_MCICLOCK_WIDEBUS;
    } else {
        LPC2XXX_REG_MCICLOCK &= ~LPC2XXX_BIT_MCICLOCK_WIDEBUS;
    }
}


/*
*********************************************************************************************************
*                                   FSDev_SD_Card_BSP_SetClkFreq()
*
* Description : Set clock frequency.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               freq        Clock frequency, in Hz.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_Refresh().
*
* Note(s)     : (1) LPC3180_CLK_FREQ should be set to the value of the LPC3180's core clock frequency.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetClkFreq (FS_QTY      unit_nbr,
                                    CPU_INT32U  freq)
{
    CPU_INT32U  clk_freq;
    CPU_INT32U  clk_div;
    CPU_INT32U  mciclock;
    CPU_INT32U  i;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    clk_freq              = LPC3180_CLK_FREQ / LPC3180_BIT_MS_CTRL_DIV_04;
    clk_div               = (clk_freq + freq - 1u) / freq;
    clk_div               = (clk_div + 1u) / 2u;
    if (clk_div > 255u) {
        clk_div = 255u;
    }

    mciclock              =  LPC2XXX_REG_MCICLOCK;
    mciclock             &= ~LPC2XXX_BIT_MCICLOCK_CLKDIV_MASK;
    mciclock             |=  clk_div | LPC2XXX_BIT_MCICLOCK_ENABLE;
    LPC2XXX_REG_MCICLOCK  =  mciclock;

    for (i = 0u; i < 32u; i++) {
        ;
    }
}


/*
*********************************************************************************************************
*                                 FSDev_SD_Card_BSP_SetTimeoutData()
*
* Description : Set data timeout.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               to_clks     Timeout, in clocks.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_Refresh().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetTimeoutData (FS_QTY      unit_nbr,
                                        CPU_INT32U  to_clks)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    LPC2XXX_REG_MCIDATATIMER = to_clks;
}


/*
*********************************************************************************************************
*                                 FSDev_SD_Card_BSP_SetTimeoutResp()
*
* Description : Set response timeout.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               to_ms       Timeout, in milliseconds.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_Refresh().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetTimeoutResp (FS_QTY      unit_nbr,
                                        CPU_INT32U  to_ms)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
   (void)&to_ms;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                     FSDev_SD_Card_BSP_ISR_Handler()
*
* Description : Handle MCI interrupt.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_SD_Card_BSP_ISR_Handler (void)
{
    CPU_INT32U  status;


    status = LPC2XXX_REG_MCISTATUS;
    if ((status & LPC2XXX_BIT_MCISTATUS_ERR) != 0u) {
        FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA;
        if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_DATACRCFAIL) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR = LPC2XXX_BIT_MCICLEAR_DATACRCFAILCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_CHKSUM;
        }
        if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_DATATIMEOUT) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR = LPC2XXX_BIT_MCICLEAR_TXUNDERRUNCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_TIMEOUT;
        }
        if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_TXUNDERRUN) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR = LPC2XXX_BIT_MCICLEAR_TXUNDERRUNCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_UNDERRUN;
       }
        if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_RXOVERRUN) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR = LPC2XXX_BIT_MCISTATUS_RXOVERRUN;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_OVERRUN;
        }
        if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_STARTBITERR) == DEF_YES) {
            LPC2XXX_REG_MCICLEAR = LPC2XXX_BIT_MCICLEAR_STARTBITERRCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_START_BIT;
        }

        LPC2XXX_REG_MCIMASK0 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
        LPC2XXX_REG_MCIMASK1 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
       (void)FS_OS_SemPost(&FSDev_SD_Card_OS_Sem);
        return;
    }

    if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_DATAEND) == DEF_YES) {
        FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_NONE;
        LPC2XXX_REG_MCICLEAR = LPC2XXX_BIT_MCICLEAR_DATAENDCLR;
        LPC2XXX_REG_MCIMASK0 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
        LPC2XXX_REG_MCIMASK1 &= ~(LPC2XXX_BIT_MCISTATUS_DATAEND | LPC2XXX_BIT_MCISTATUS_DATABLOCKEND | LPC2XXX_BIT_MCISTATUS_ERR);
       (void)FS_OS_SemPost(&FSDev_SD_Card_OS_Sem);
        return;
    }

    if (DEF_BIT_IS_SET(status, LPC2XXX_BIT_MCISTATUS_DATABLOCKEND) == DEF_YES) {
        LPC2XXX_REG_MCICLEAR  = LPC2XXX_BIT_MCICLEAR_DATABLOCKENDCLR;
        return;
    }
}
