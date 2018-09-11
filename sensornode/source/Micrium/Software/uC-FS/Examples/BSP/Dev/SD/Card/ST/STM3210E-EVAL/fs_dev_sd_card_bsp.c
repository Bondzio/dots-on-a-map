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
*                      ST STM32F103VE ON THE ST STM3210E-EVAL EVALUATION BOARD
*
* Filename      : fs_dev_sd_card_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*                 EJ
*********************************************************************************************************
* Note(s)       : (1) The STM32 SD/MMC card interface is described in documentation from ST Micro :
*
*                         RM0008.  Reference Manual.  Low-, medium- and high-density STM32F101xx,
*                         STM32F102xx and STM32F103xx advanced ARM-based 32-bit MCUs. Rev 6, September
*                         2008.  "19  SDIO interface (SDIO)".
*
*                (2) (a) STM32_SDIOCLK_CLK_FREQ_HZ must be set to the SDIOCLK frequency.
*
*                    (b) FSDev_SD_Card_BSP_ISR_Handler() must be placed in the SDIO interrupt vector slot
*                        or called by the handler placed in that slot, if an RTOS integrated into the
*                        demands a special interrupt enter/exit procedure.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_SD_CARD_BSP_MODULE
#include  <fs_dev_sd_card.h>
#include  <bsp.h>
#include  <fs.h>
#include  <fs_os.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  STM32_SDIOCLK_CLK_FREQ_HZ                  72000000uL

#define  STM32_INT_ID_SDIO                                49u
#define  STM32_INT_ID_SDIO_GRP                 (STM32_INT_ID_SDIO / 32u)
#define  STM32_INT_ID_SDIO_BIT         (DEF_BIT(STM32_INT_ID_SDIO % 32u))

#define  STM32_WAIT_TIMEOUT_ms                          1000u

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  STM32_REG_NVIC_SETEN(n)   (*((CPU_REG32 *)(0xE000E100uL + (n) * 4u)))
#define  STM32_REG_NVIC_CLREN(n)   (*((CPU_REG32 *)(0xE000E180uL + (n) * 4u)))

#define  STM32_REG_RCC_BASE                         0x40021000uL
#define  STM32_REG_RCC_CR           (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x000u))
#define  STM32_REG_RCC_CFGR         (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x004u))
#define  STM32_REG_RCC_CIR          (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x008u))
#define  STM32_REG_RCC_APB2RSTR     (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x00Cu))
#define  STM32_REG_RCC_APB1RSTR     (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x010u))
#define  STM32_REG_RCC_AHBENR       (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x014u))
#define  STM32_REG_RCC_APB2ENR      (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x018u))
#define  STM32_REG_RCC_APB1ENR      (*(CPU_REG32 *)(STM32_REG_RCC_BASE   + 0x01Cu))

#define  STM32_REG_DMA_BASE                         0x40020400uL
#define  STM32_REG_DMA_ISR          (*(CPU_REG32 *)(STM32_REG_DMA_BASE   + 0x000u))
#define  STM32_REG_DMA_IFCR         (*(CPU_REG32 *)(STM32_REG_DMA_BASE   + 0x004u))

#define  STM32_REG_DMAC4_BASE                       0x40020444uL
#define  STM32_REG_DMAC4_CCR        (*(CPU_REG32 *)(STM32_REG_DMAC4_BASE + 0x000u))
#define  STM32_REG_DMAC4_CNDTR      (*(CPU_REG32 *)(STM32_REG_DMAC4_BASE + 0x004u))
#define  STM32_REG_DMAC4_CPAR       (*(CPU_REG32 *)(STM32_REG_DMAC4_BASE + 0x008u))
#define  STM32_REG_DMAC4_CMAR       (*(CPU_REG32 *)(STM32_REG_DMAC4_BASE + 0x00Cu))

#define  STM32_REG_GPIOC_BASE                       0x40011000uL
#define  STM32_REG_GPIOC_CRL        (*(CPU_REG32 *)(STM32_REG_GPIOC_BASE + 0x000u))
#define  STM32_REG_GPIOC_CRH        (*(CPU_REG32 *)(STM32_REG_GPIOC_BASE + 0x004u))
#define  STM32_REG_GPIOC_IDR        (*(CPU_REG32 *)(STM32_REG_GPIOC_BASE + 0x008u))
#define  STM32_REG_GPIOC_ODR        (*(CPU_REG32 *)(STM32_REG_GPIOC_BASE + 0x00Cu))
#define  STM32_REG_GPIOC_BSRR       (*(CPU_REG32 *)(STM32_REG_GPIOC_BASE + 0x010u))
#define  STM32_REG_GPIOC_BRR        (*(CPU_REG32 *)(STM32_REG_GPIOC_BASE + 0x014u))
#define  STM32_REG_GPIOC_LCKR       (*(CPU_REG32 *)(STM32_REG_GPIOC_BASE + 0x018u))

#define  STM32_REG_GPIOD_BASE                       0x40011400uL
#define  STM32_REG_GPIOD_CRL        (*(CPU_REG32 *)(STM32_REG_GPIOD_BASE + 0x000u))
#define  STM32_REG_GPIOD_CRH        (*(CPU_REG32 *)(STM32_REG_GPIOD_BASE + 0x004u))
#define  STM32_REG_GPIOD_IDR        (*(CPU_REG32 *)(STM32_REG_GPIOD_BASE + 0x008u))
#define  STM32_REG_GPIOD_ODR        (*(CPU_REG32 *)(STM32_REG_GPIOD_BASE + 0x00Cu))
#define  STM32_REG_GPIOD_BSRR       (*(CPU_REG32 *)(STM32_REG_GPIOD_BASE + 0x010u))
#define  STM32_REG_GPIOD_BRR        (*(CPU_REG32 *)(STM32_REG_GPIOD_BASE + 0x014u))
#define  STM32_REG_GPIOD_LCKR       (*(CPU_REG32 *)(STM32_REG_GPIOD_BASE + 0x018u))

#define  STM32_REG_SDIO_BASE                        0x40018000uL
#define  STM32_REG_SDIO_POWER       (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x000u))
#define  STM32_REG_SDIO_CLKCR       (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x004u))
#define  STM32_REG_SDIO_ARG         (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x008u))
#define  STM32_REG_SDIO_CMD         (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x00Cu))
#define  STM32_REG_SDIO_RESPCMD     (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x010u))
#define  STM32_REG_SDIO_RESP0       (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x014u))
#define  STM32_REG_SDIO_RESP1       (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x018u))
#define  STM32_REG_SDIO_RESP2       (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x01Cu))
#define  STM32_REG_SDIO_RESP3       (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x020u))
#define  STM32_REG_SDIO_DTIMER      (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x024u))
#define  STM32_REG_SDIO_DLEN        (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x028u))
#define  STM32_REG_SDIO_DCTRL       (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x02Cu))
#define  STM32_REG_SDIO_DCOUNT      (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x030u))
#define  STM32_REG_SDIO_STA         (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x034u))
#define  STM32_REG_SDIO_ICR         (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x038u))
#define  STM32_REG_SDIO_MASK        (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x03Cu))
#define  STM32_REG_SDIO_FIFOCNT     (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x048u))
#define  STM32_REG_SDIO_FIFO        (*(CPU_REG32 *)(STM32_REG_SDIO_BASE  + 0x080u))
#define  STM32_ADDR_SDIO_FIFO                      (STM32_REG_SDIO_BASE  + 0x080u)

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  STM32_BIT_SDIO_POWER_PWRCTRL_POWEROFF          0x00u
#define  STM32_BIT_SDIO_POWER_PWRCTRL_POWERUP           0x02u
#define  STM32_BIT_SDIO_POWER_PWRCTRL_POWERON           0x03u
#define  STM32_BIT_SDIO_POWER_PWRCTRL_MASK              0x03u

#define  STM32_BIT_SDIO_CLKCR_CLKDIV_MASK             0x00FFu
#define  STM32_BIT_SDIO_CLKCR_CLKEN                 DEF_BIT_08
#define  STM32_BIT_SDIO_CLKCR_PWRSAV                DEF_BIT_09
#define  STM32_BIT_SDIO_CLKCR_BYPASS                DEF_BIT_10
#define  STM32_BIT_SDIO_CLKCR_WIDBUS_MASK             0x1800u
#define  STM32_BIT_SDIO_CLKCR_WIDBUS_4BIT           DEF_BIT_11
#define  STM32_BIT_SDIO_CLKCR_WIDBUS_8BIT           DEF_BIT_12
#define  STM32_BIT_SDIO_CLKCR_NEGEDGE               DEF_BIT_13
#define  STM32_BIT_SDIO_CLKCR_HWFC_EN               DEF_BIT_14

#define  STM32_BIT_SDIO_CMD_CMDINDEX_MASK             0x003Fu
#define  STM32_BIT_SDIO_CMD_RESPONSE                DEF_BIT_06
#define  STM32_BIT_SDIO_CMD_LONGRSP                 DEF_BIT_07
#define  STM32_BIT_SDIO_CMD_WAITINT                 DEF_BIT_08
#define  STM32_BIT_SDIO_CMD_WAITPEND                DEF_BIT_09
#define  STM32_BIT_SDIO_CMD_CPSMEN                  DEF_BIT_10
#define  STM32_BIT_SDIO_CMD_SDIOSUSPEND             DEF_BIT_11
#define  STM32_BIT_SDIO_CMD_ENCMDCOMPL              DEF_BIT_12
#define  STM32_BIT_SDIO_CMD_NIEN                    DEF_BIT_13
#define  STM32_BIT_SDIO_CMD_ATACMD                  DEF_BIT_14

#define  STM32_BIT_SDIO_RESPCMD_RESPCMD_MASK         0x003Fu

#define  STM32_BIT_SDIO_DLEN_DATALENGTH_MASK         0xFFFFu

#define  STM32_BIT_SDIO_DCTRL_DTEN                  DEF_BIT_00
#define  STM32_BIT_SDIO_DCTRL_DTDIR                 DEF_BIT_01  /* Set = from card to controller.                       */
#define  STM32_BIT_SDIO_DCTRL_DTMODE                DEF_BIT_02  /* Set = stream data transfer.                          */
#define  STM32_BIT_SDIO_DCTRL_DMAEN                 DEF_BIT_03
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0001_BYTE      0x00u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0002_BYTE      0x10u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0004_BYTE      0x20u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0008_BYTE      0x30u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0016_BYTE      0x40u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0032_BYTE      0x50u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0064_BYTE      0x60u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0128_BYTE      0x70u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0256_BYTE      0x80u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0512_BYTE      0x90u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_1024_BYTE      0xA0u
#define  STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_2048_BYTE      0xB0u
#define  STM32_BIT_SDIO_DCTRL_RWSTART               DEF_BIT_08
#define  STM32_BIT_SDIO_DCTRL_RWSTOP                DEF_BIT_09
#define  STM32_BIT_SDIO_DCTRL_RWMOD                 DEF_BIT_10
#define  STM32_BIT_SDIO_DCTRL_SDIOEN                DEF_BIT_11

#define  STM32_BIT_SDIO_DCOUNT_DATACOUNT_MASK         0xFFFFu

#define  STM32_BIT_SDIO_STA_CCRCFAIL                DEF_BIT_00
#define  STM32_BIT_SDIO_STA_DCRCFAIL                DEF_BIT_01
#define  STM32_BIT_SDIO_STA_CTIMEOUT                DEF_BIT_02
#define  STM32_BIT_SDIO_STA_DTIMEOUT                DEF_BIT_03
#define  STM32_BIT_SDIO_STA_TXUNDERR                DEF_BIT_04
#define  STM32_BIT_SDIO_STA_RXOVERR                 DEF_BIT_05
#define  STM32_BIT_SDIO_STA_CMDREND                 DEF_BIT_06
#define  STM32_BIT_SDIO_STA_CMDSENT                 DEF_BIT_07
#define  STM32_BIT_SDIO_STA_DATAEND                 DEF_BIT_08
#define  STM32_BIT_SDIO_STA_SBITERR                 DEF_BIT_09
#define  STM32_BIT_SDIO_STA_DBCKEND                 DEF_BIT_10
#define  STM32_BIT_SDIO_STA_CMDACT                  DEF_BIT_11
#define  STM32_BIT_SDIO_STA_TXACT                   DEF_BIT_12
#define  STM32_BIT_SDIO_STA_RXACT                   DEF_BIT_13
#define  STM32_BIT_SDIO_STA_TXFIFOHE                DEF_BIT_14
#define  STM32_BIT_SDIO_STA_RXFIFOHF                DEF_BIT_15
#define  STM32_BIT_SDIO_STA_TXFIFOF                 DEF_BIT_16
#define  STM32_BIT_SDIO_STA_RXFIFOF                 DEF_BIT_17
#define  STM32_BIT_SDIO_STA_TXFIFOE                 DEF_BIT_18
#define  STM32_BIT_SDIO_STA_RXFIFOE                 DEF_BIT_19
#define  STM32_BIT_SDIO_STA_TXDAVL                  DEF_BIT_20
#define  STM32_BIT_SDIO_STA_RXDAVL                  DEF_BIT_21
#define  STM32_BIT_SDIO_STA_SDIOIT                  DEF_BIT_22
#define  STM32_BIT_SDIO_STA_CEATAEND                DEF_BIT_23
#define  STM32_BIT_SDIO_STA_ERR                    (STM32_BIT_SDIO_STA_DCRCFAIL | STM32_BIT_SDIO_STA_DTIMEOUT | STM32_BIT_SDIO_STA_TXUNDERR | \
                                                    STM32_BIT_SDIO_STA_RXOVERR  | STM32_BIT_SDIO_STA_SBITERR)

#define  STM32_BIT_SDIO_ICR_CCRCFAILCLR             DEF_BIT_00
#define  STM32_BIT_SDIO_ICR_DCRCFAILCLR             DEF_BIT_01
#define  STM32_BIT_SDIO_ICR_CTIMEOUTCLR             DEF_BIT_02
#define  STM32_BIT_SDIO_ICR_DTIMEOUTCLR             DEF_BIT_03
#define  STM32_BIT_SDIO_ICR_TXUNDERRCLR             DEF_BIT_04
#define  STM32_BIT_SDIO_ICR_RXOVERRCLR              DEF_BIT_05
#define  STM32_BIT_SDIO_ICR_CMDRENDCLR              DEF_BIT_06
#define  STM32_BIT_SDIO_ICR_CMDSENTCLR              DEF_BIT_07
#define  STM32_BIT_SDIO_ICR_DATAENDCLR              DEF_BIT_08
#define  STM32_BIT_SDIO_ICR_SBITERRCLR              DEF_BIT_09
#define  STM32_BIT_SDIO_ICR_DBCKENDCLR              DEF_BIT_10
#define  STM32_BIT_SDIO_ICR_SDIOITC                 DEF_BIT_22
#define  STM32_BIT_SDIO_ICR_CEATAENDC               DEF_BIT_23
#define  STM32_BIT_SDIO_ICR_ALLCLR                 (STM32_BIT_SDIO_ICR_CCRCFAILCLR | STM32_BIT_SDIO_ICR_DCRCFAILCLR | STM32_BIT_SDIO_ICR_CTIMEOUTCLR | \
                                                    STM32_BIT_SDIO_ICR_DTIMEOUTCLR | STM32_BIT_SDIO_ICR_TXUNDERRCLR | STM32_BIT_SDIO_ICR_RXOVERRCLR  | \
                                                    STM32_BIT_SDIO_ICR_CMDRENDCLR  | STM32_BIT_SDIO_ICR_CMDSENTCLR  | STM32_BIT_SDIO_ICR_DATAENDCLR  | \
                                                    STM32_BIT_SDIO_ICR_SBITERRCLR  | STM32_BIT_SDIO_ICR_DBCKENDCLR)

#define  STM32_BIT_DMA_FLAG_GL4                     0x00001000u
#define  STM32_BIT_DMA_FLAG_TC4                     0x00002000u
#define  STM32_BIT_DMA_FLAG_HT4                     0x00004000u
#define  STM32_BIT_DMA_FLAG_TE4                     0x00008000u


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

static  FS_ERR     FSDev_SD_Card_BSP_Err;
static  FS_OS_SEM  FSDev_SD_Card_OS_Sem;


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

static  void  FSDev_SD_Card_BSP_CfgRx(CPU_INT32U  *p_dest,
                                      CPU_INT32U   size);

static  void  FSDev_SD_Card_BSP_CfgTx(CPU_INT32U  *p_src,
                                      CPU_INT32U   size);


static  void  FSDev_SD_Card_BSP_ISR_Handler (void);


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
*                   ----------------------------------------------------
*                   |     STM32 NAME     |  STM32 PIO #  | SD/MMC NAME |
*                   ----------------------------------------------------
*                   |   MicroSDCard_D0   |     PC8       |     DAT0    |
*                   |   MicroSDCard_D1   |     PC9       |     DAT1    |
*                   |   MicroSDCard_D2   |     PC10      |     DAT2    |
*                   |   MicroSDCard_D3   |     PC11      |     DAT3    |
*                   |   MicroSDCard_CLK  |     PC12      |     CLK     |
*                   |   MicroSDCard_CMD  |     PD2       |     CMD     |
*                   |   Card Detect      |     PF11      |    -----    |
*                   ----------------------------------------------------
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_SD_Card_BSP_Open (FS_QTY  unit_nbr)
{
    CPU_INT32U   i;
    CPU_INT32U   temp;
    CPU_BOOLEAN  ok;


    if (unit_nbr != 0) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

    ok = FS_OS_SemCreate(&FSDev_SD_Card_OS_Sem, 0u);
    if (ok == DEF_FAIL) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Open(): Could not create sem.\r\n"));
        return (DEF_FAIL);
    }

                                                                        /* ------------------ EN GPIO ----------------- */
    STM32_REG_RCC_APB2ENR |= DEF_BIT_04 | DEF_BIT_05;                   /* En GPIOC & GPIOD periph clks.                */

    temp                 = STM32_REG_GPIOC_CRH;                         /* Cfg PC8-12 as AF push-pull, 50MHz.           */
    temp                &= 0xFFF00000u;
    temp                |= 0x000BBBBBu;
    STM32_REG_GPIOC_CRH  = temp;

    temp                 = STM32_REG_GPIOD_CRL;                         /* Cfg PD2    as AF push-pull, 50MHz.           */
    temp                &= 0xFFFFF0FFu;
    temp                |= 0x00000B00u;
    STM32_REG_GPIOD_CRL  = temp;


                                                                        /* ------------------ EN DMA ------------------ */
    STM32_REG_RCC_AHBENR |= DEF_BIT_01;                                 /* En DMA2 periph clk.                          */



                                                                        /* ------------------ EN MCI ------------------ */
    STM32_REG_RCC_AHBENR |= DEF_BIT_10;                                 /* En SDIO periph clk.                          */

    if (DEF_BIT_IS_SET(STM32_REG_SDIO_CLKCR, STM32_BIT_SDIO_CLKCR_CLKEN) == DEF_YES) {
        DEF_BIT_CLR(STM32_REG_SDIO_CLKCR, STM32_BIT_SDIO_CLKCR_CLKEN);
    }

    if ((STM32_REG_SDIO_POWER & STM32_BIT_SDIO_POWER_PWRCTRL_MASK) != STM32_BIT_SDIO_POWER_PWRCTRL_POWEROFF) {
        STM32_REG_SDIO_POWER = STM32_BIT_SDIO_POWER_PWRCTRL_POWEROFF;
    }

    for (i = 0u; i < 1000u; i++) {
        ;
    }

    STM32_REG_SDIO_MASK  = 0u;                                          /* Dis all ints.                                */
    STM32_REG_NVIC_CLREN(STM32_INT_ID_SDIO_GRP) = STM32_INT_ID_SDIO_BIT;

    STM32_REG_SDIO_CMD   = 0u;
    STM32_REG_SDIO_DCTRL = 0u;
    STM32_REG_SDIO_ICR   = STM32_BIT_SDIO_ICR_ALLCLR;

                                                                        /* Install ISR.                                 */
    BSP_IntVectSet(BSP_INT_ID_SDIO, (CPU_FNCT_VOID)FSDev_SD_Card_BSP_ISR_Handler);
    BSP_IntPrioSet(BSP_INT_ID_SDIO, 10u);


    STM32_REG_SDIO_POWER = STM32_BIT_SDIO_POWER_PWRCTRL_POWERUP;        /* Power up.                                    */
    while ((STM32_REG_SDIO_POWER & STM32_BIT_SDIO_POWER_PWRCTRL_MASK) == STM32_BIT_SDIO_POWER_PWRCTRL_POWEROFF) {
        ;
    }
    for (i = 0u; i < 1000u; i++) {
        ;
    }


    FSDev_SD_Card_BSP_SetClkFreq(unit_nbr, 400000u);                    /* Set dflt clk freq.                           */


    STM32_REG_SDIO_POWER |= STM32_BIT_SDIO_POWER_PWRCTRL_POWERON;       /* Power on.                                    */
    for (i = 0u; i < 1000u; i++) {
        ;
    }

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
    CPU_BOOLEAN  ok;;


    (void)&unit_nbr;                                                /* Prevent compiler warning.                        */

    ok = FS_OS_SemDel(&FSDev_SD_Card_OS_Sem);                     /* Del sem.                                         */
    if (ok == DEF_FAIL) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Close(): Could not del sem.\r\n"));
    }

                                                                    /* Dis SDIO int.                                    */
    STM32_REG_NVIC_CLREN(STM32_INT_ID_SDIO_GRP) = STM32_INT_ID_SDIO_BIT;
    STM32_REG_RCC_AHBENR &= ~DEF_BIT_10;                            /* Dis SDIO periph clk.                             */
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
                                  FS_DEV_SD_CARD_ERR  *p_err)
{
    CPU_INT32U  status;
    CPU_INT32U  command;
    CPU_INT32U  i;
    CPU_INT32U  datactrl;
    CPU_INT32U  rd_size;
    CPU_SR_ALLOC();

   (void)&p_data;

    STM32_REG_SDIO_CMD = 0u;
    STM32_REG_SDIO_ICR = STM32_BIT_SDIO_ICR_ALLCLR;             /* Clr all ints.                                        */

    for (i = 0u; i < 32u; i++) {
        ;
    }

    status = STM32_REG_SDIO_STA;                                /* Chk if controller busy.                              */
    if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_CMDACT) == DEF_YES) {
        STM32_REG_SDIO_CMD = 0u;
        STM32_REG_SDIO_ICR = status;
    }

    status = STM32_REG_SDIO_STA;
    if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_CMDACT) == DEF_YES) {
       *p_err = FS_DEV_SD_CARD_ERR_BUSY;
        return;
    }

    for (i = 0u; i < 32u; i++) {
        ;
    }


    if(p_cmd->DataDir == FS_DEV_SD_CARD_DATA_DIR_CARD_TO_HOST)  {
                                                                /* ------------------- SET DATA CTRL ------------------ */
    datactrl = STM32_BIT_SDIO_DCTRL_DTEN                        /* Set data dir.                                        */
             | STM32_BIT_SDIO_DCTRL_DTDIR
             | STM32_BIT_SDIO_DCTRL_DMAEN;

    switch (p_cmd->BlkSize) {                                   /* Set blk size.                                        */
        case 1u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0001_BYTE;    break;
        case 2u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0002_BYTE;    break;
        case 4u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0004_BYTE;    break;
        case 8u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0008_BYTE;    break;
        case 16u:    datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0016_BYTE;    break;
        case 32u:    datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0032_BYTE;    break;
        case 64u:    datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0064_BYTE;    break;
        case 128u:   datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0128_BYTE;    break;
        case 256u:   datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0256_BYTE;    break;
        case 1024u:  datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_1024_BYTE;    break;
        case 2048u:  datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_2048_BYTE;    break;

        default:
        case 512u:   datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0512_BYTE;    break;
    }

    rd_size              = p_cmd->BlkSize * p_cmd->BlkCnt;

    CPU_CRITICAL_ENTER();
    STM32_REG_SDIO_DLEN  = rd_size;
    FSDev_SD_Card_BSP_CfgRx(p_data, rd_size);
    STM32_REG_SDIO_DCTRL = datactrl;
    CPU_CRITICAL_EXIT();

    for (i = 0u; i < 32u; i++) {
        ;
    }

                                                                /* En ints.                                             */
    STM32_REG_SDIO_MASK |= STM32_BIT_SDIO_STA_DATAEND | STM32_BIT_SDIO_STA_DBCKEND | STM32_BIT_SDIO_STA_ERR;
    STM32_REG_NVIC_SETEN(STM32_INT_ID_SDIO_GRP) = STM32_INT_ID_SDIO_BIT;
    }


                                                                /* -------------------- SET COMMAND ------------------- */
    command = (p_cmd->Cmd) | STM32_BIT_SDIO_CMD_CPSMEN;         /* Set cmd ix.                                          */
    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
        command |= STM32_BIT_SDIO_CMD_RESPONSE;
        if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
            command |= STM32_BIT_SDIO_CMD_LONGRSP;
        }
    }



                                                                /* -------------------- SET COMMAND ------------------- */
    STM32_REG_SDIO_ARG = p_cmd->Arg;
    STM32_REG_SDIO_CMD = command;

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
                                    FS_DEV_SD_CARD_ERR  *p_err)
{
    CPU_INT32U   status;
    CPU_BOOLEAN  done;
    CPU_INT08U   cmd_resp;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

                                                                /* ----------------- WAIT FOR CMD END ----------------- */
    done = DEF_NO;
    while (done == DEF_NO) {
        status = STM32_REG_SDIO_STA;
                                                                /* If cmd timeout ...                                   */
        if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_CTIMEOUT) == DEF_YES) {
            STM32_REG_SDIO_ICR = status;
            STM32_REG_SDIO_CMD = 0u;
            STM32_REG_SDIO_ARG = 0xFFFFFFFFu;
           *p_err = FS_DEV_SD_CARD_ERR_RESP_TIMEOUT;            /* ... rtn err.                                         */
            return;
        }
                                                                /* If CRC chk failed           ...                      */
        if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_CCRCFAIL) == DEF_YES) {
                                                                /* ... but CRC should be valid ...                      */
            if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_CRC_VALID) == DEF_YES) {
                STM32_REG_SDIO_ICR = status;
                STM32_REG_SDIO_CMD = 0u;
                STM32_REG_SDIO_ARG = 0xFFFFFFFF;
               *p_err = FS_DEV_SD_CARD_ERR_RESP_CHKSUM;         /* ... rtn err.                                         */
                return;

            } else {
                STM32_REG_SDIO_ICR = status;
                done               = DEF_YES;
            }

        } else if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_CMDREND) == DEF_YES) {
            STM32_REG_SDIO_ICR = status;
            done               = DEF_YES;


        } else if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_CMDSENT) == DEF_YES) {
            STM32_REG_SDIO_ICR = status;
            STM32_REG_SDIO_CMD = 0u;
            STM32_REG_SDIO_ARG = 0xFFFFFFFFu;

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
    cmd_resp = STM32_REG_SDIO_RESPCMD & STM32_BIT_SDIO_RESPCMD_RESPCMD_MASK;
    if (cmd_resp != p_cmd->Cmd) {
                                                                /* ... but should be          ...                       */
        if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_IX_VALID) == DEF_YES) {
            STM32_REG_SDIO_CMD = 0u;
            STM32_REG_SDIO_ARG = 0xFFFFFFFFu;
           *p_err = FS_DEV_SD_CARD_ERR_RESP_CMD_IX;             /* ... rtn err.                                         */
            return;
        }
    }


                                                                /* If no resp expected ...                              */
    if (DEF_BIT_IS_CLR(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
        STM32_REG_SDIO_CMD = 0u;
        STM32_REG_SDIO_ARG = 0xFFFFFFFFu;
       *p_err = FS_DEV_SD_CARD_ERR_NONE;                        /* ... rtn w/o err.                                     */
        return;
    }

                                                                /* Rd resp.                                             */
    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
       *p_resp = STM32_REG_SDIO_RESP0;
        p_resp++;
       *p_resp = STM32_REG_SDIO_RESP1;
        p_resp++;
       *p_resp = STM32_REG_SDIO_RESP2;
        p_resp++;
       *p_resp = STM32_REG_SDIO_RESP3;
    } else {
       *p_resp = STM32_REG_SDIO_RESP0;
    }

    STM32_REG_SDIO_CMD = 0u;
    STM32_REG_SDIO_ARG = 0xFFFFFFFFu;
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
                                   FS_DEV_SD_CARD_ERR  *p_err)
{
    CPU_BOOLEAN  ok;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */


                                                                /* Wait for completion.                                 */
    ok = FS_OS_SemPend(&FSDev_SD_Card_OS_Sem, STM32_WAIT_TIMEOUT_ms);


                                                                /* --------------------- CHK & RTN -------------------- */
                                                                /* Dis ints.                                            */
    STM32_REG_NVIC_CLREN(STM32_INT_ID_SDIO_GRP) = STM32_INT_ID_SDIO_BIT;
    STM32_REG_SDIO_MASK &= ~(STM32_BIT_SDIO_STA_DATAEND | STM32_BIT_SDIO_STA_DBCKEND | STM32_BIT_SDIO_STA_ERR);
    STM32_REG_SDIO_ICR   =   STM32_BIT_SDIO_ICR_ALLCLR;


    if (ok == DEF_OK) {
       *p_err = FSDev_SD_Card_BSP_Err;
        if (*p_err == FS_DEV_SD_CARD_ERR_NONE) {
                                                                /* Make sure DMA is finished.                           */
            while (DEF_BIT_IS_CLR(STM32_REG_DMA_ISR, STM32_BIT_DMA_FLAG_TC4) == DEF_YES) {
                ;
            }
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
                                   FS_DEV_SD_CARD_ERR  *p_err)
{
    CPU_INT32U   datactrl;
    CPU_INT32U   wr_size;
    CPU_BOOLEAN  ok;
    CPU_SR_ALLOC();


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

                                                                /* ------------------- SET DATA CTRL ------------------ */
    wr_size  = p_cmd->BlkSize * p_cmd->BlkCnt;
    datactrl = STM32_BIT_SDIO_DCTRL_DMAEN;                      /* Set data dir.                                        */


    switch (p_cmd->BlkSize) {                                   /* Set blk size.                                        */
        case 1u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0001_BYTE;    break;
        case 2u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0002_BYTE;    break;
        case 4u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0004_BYTE;    break;
        case 8u:     datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0008_BYTE;    break;
        case 16u:    datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0016_BYTE;    break;
        case 32u:    datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0032_BYTE;    break;
        case 64u:    datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0064_BYTE;    break;
        case 128u:   datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0128_BYTE;    break;
        case 256u:   datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0256_BYTE;    break;
        case 1024u:  datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_1024_BYTE;    break;
        case 2048u:  datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_2048_BYTE;    break;

        default:
        case 512u:   datactrl |= STM32_BIT_SDIO_DCTRL_DBLOCKSIZE_0512_BYTE;    break;
    }

    CPU_CRITICAL_ENTER();
    STM32_REG_SDIO_DLEN   = wr_size;
    STM32_REG_SDIO_DCTRL  = datactrl;
    FSDev_SD_Card_BSP_CfgTx(p_src, wr_size);
                                                                /* ---------------- WAIT FOR COMPLETION --------------- */
                                                                /* En ints.                                             */
    STM32_REG_SDIO_MASK |= STM32_BIT_SDIO_STA_DATAEND | STM32_BIT_SDIO_STA_DBCKEND | STM32_BIT_SDIO_STA_ERR;
    STM32_REG_NVIC_SETEN(STM32_INT_ID_SDIO_GRP) = STM32_INT_ID_SDIO_BIT;

    STM32_REG_SDIO_DCTRL |= STM32_BIT_SDIO_DCTRL_DTEN;
    CPU_CRITICAL_EXIT();

                                                                /* Wait for completion.                                 */
    ok = FS_OS_SemPend(&FSDev_SD_Card_OS_Sem, STM32_WAIT_TIMEOUT_ms);


                                                                /* --------------------- CHK & RTN -------------------- */
                                                                /* Dis ints.                                            */
    STM32_REG_NVIC_CLREN(STM32_INT_ID_SDIO_GRP) = STM32_INT_ID_SDIO_BIT;
    STM32_REG_SDIO_MASK &= ~(STM32_BIT_SDIO_STA_DATAEND | STM32_BIT_SDIO_STA_DBCKEND | STM32_BIT_SDIO_STA_ERR);
    STM32_REG_SDIO_ICR   =   STM32_BIT_SDIO_ICR_ALLCLR;

    if (ok == DEF_OK) {
       *p_err = FSDev_SD_Card_BSP_Err;
        if (*p_err == FS_DEV_SD_CARD_ERR_NONE) {
                                                                /* Make sure DMA is finished.                           */
            while (DEF_BIT_IS_CLR(STM32_REG_DMA_ISR, STM32_BIT_DMA_FLAG_TC4) == DEF_YES) {
                ;
            }
        }
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
* Note(s)     : (1) The controller has no limit on the number of blocks in a multiple block read or
*                   write, so 'DEF_INT_32U_MAX_VAL' is returned.
*********************************************************************************************************
*/

CPU_INT32U  FSDev_SD_Card_BSP_GetBlkCntMax (FS_QTY      unit_nbr,
                                            CPU_INT32U  blk_size)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    return (DEF_INT_32U_MAX_VAL);
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
        STM32_REG_SDIO_CLKCR |=  STM32_BIT_SDIO_CLKCR_WIDBUS_4BIT;
    } else {
        STM32_REG_SDIO_CLKCR &= ~STM32_BIT_SDIO_CLKCR_WIDBUS_4BIT;
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
* Note(s)     : (1) STM32_SDIOCLK_CLK_FREQ_HZ should be set to the value of the STM32's SDIO clock
*                   frequency.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetClkFreq (FS_QTY      unit_nbr,
                                    CPU_INT32U  freq)
{
    CPU_INT32U  clk_freq;
    CPU_INT32U  clk_div;
    CPU_INT32U  clkcr;
    CPU_INT32U  i;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    clk_freq              =  STM32_SDIOCLK_CLK_FREQ_HZ;
    clk_div               = (clk_freq + freq - 1u) / freq;
    clk_div               = (clk_div + 1u) / 2u;
    if (clk_div > 255u) {
        clk_div = 255u;
    }

    clkcr                 =  STM32_REG_SDIO_CLKCR;
    clkcr                &= ~STM32_BIT_SDIO_CLKCR_CLKDIV_MASK;
    clkcr                |=  clk_div
                          | STM32_BIT_SDIO_CLKCR_HWFC_EN
                          | STM32_BIT_SDIO_CLKCR_CLKEN;
    STM32_REG_SDIO_CLKCR  =  clkcr;

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

    STM32_REG_SDIO_DTIMER = to_clks;
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

void  FSDev_SD_Card_BSP_ISR_Handler (void)
{
    CPU_INT32U  status;


    status = STM32_REG_SDIO_STA;
    if ((status & STM32_BIT_SDIO_STA_ERR) != 0u) {
        FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA;
        if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_DCRCFAIL) == DEF_YES) {
            STM32_REG_SDIO_ICR    = STM32_BIT_SDIO_ICR_DCRCFAILCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_CHKSUM;
        }
        if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_DTIMEOUT) == DEF_YES) {
            STM32_REG_SDIO_ICR    = STM32_BIT_SDIO_ICR_TXUNDERRCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_TIMEOUT;
        }
        if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_TXUNDERR) == DEF_YES) {
            STM32_REG_SDIO_ICR    = STM32_BIT_SDIO_ICR_TXUNDERRCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_UNDERRUN;
       }
        if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_RXOVERR) == DEF_YES) {
            STM32_REG_SDIO_ICR    = STM32_BIT_SDIO_STA_RXOVERR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_OVERRUN;
        }
        if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_SBITERR) == DEF_YES) {
            STM32_REG_SDIO_ICR    = STM32_BIT_SDIO_ICR_SBITERRCLR;
            FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_DATA_START_BIT;
        }

        STM32_REG_SDIO_MASK &= ~(STM32_BIT_SDIO_STA_DATAEND | STM32_BIT_SDIO_STA_DBCKEND | STM32_BIT_SDIO_STA_ERR);
       (void)FS_OS_SemPost(&FSDev_SD_Card_OS_Sem);
        return;
    }

    if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_DATAEND) == DEF_YES) {
        FSDev_SD_Card_BSP_Err = FS_DEV_SD_CARD_ERR_NONE;
        STM32_REG_SDIO_ICR    = STM32_BIT_SDIO_ICR_DATAENDCLR;
        STM32_REG_SDIO_MASK  &= ~(STM32_BIT_SDIO_STA_DATAEND | STM32_BIT_SDIO_STA_DBCKEND | STM32_BIT_SDIO_STA_ERR);
       (void)FS_OS_SemPost(&FSDev_SD_Card_OS_Sem);
        return;
    }

    if (DEF_BIT_IS_SET(status, STM32_BIT_SDIO_STA_DBCKEND) == DEF_YES) {
        STM32_REG_SDIO_ICR  = STM32_BIT_SDIO_ICR_DBCKENDCLR;
        return;
    }
}


/*
*********************************************************************************************************
*                                      FSDev_SD_Card_BSP_CfgRx()
*
* Description : Configure DMA reception.
*
* Argument(s) : p_dest      Point to destination buffer.
*
*               size        Size of source buffer, in octets.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_BSP_CmdDataRd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_SD_Card_BSP_CfgRx (CPU_INT32U  *p_dest,
                                       CPU_INT32U   size)
{
    CPU_INT32U  temp;


    STM32_REG_DMA_IFCR     =  STM32_BIT_DMA_FLAG_GL4 | STM32_BIT_DMA_FLAG_TC4 | STM32_BIT_DMA_FLAG_HT4 | STM32_BIT_DMA_FLAG_TE4;
    STM32_REG_DMAC4_CCR   &= ~DEF_BIT_00;

    temp                   = STM32_REG_DMAC4_CCR;
    temp                  &= 0xFFFF800Fu;
    temp                  |= 0x00000080u | 0x00000200u | 0x00000800u | 0x00002000u;
    STM32_REG_DMAC4_CCR    = temp;
    STM32_REG_DMAC4_CNDTR  = size / 4u;
    STM32_REG_DMAC4_CPAR   = STM32_ADDR_SDIO_FIFO;
    STM32_REG_DMAC4_CMAR   = (CPU_INT32U)p_dest;

    STM32_REG_DMAC4_CCR   |= DEF_BIT_00;
}


/*
*********************************************************************************************************
*                                      FSDev_SD_Card_BSP_CfgTx()
*
* Description : Configure DMA transmission.
*
* Argument(s) : p_src       Point to source buffer.
*
*               size        Size of source buffer, in octets.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_BSP_CmdDataWr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_SD_Card_BSP_CfgTx (CPU_INT32U  *p_src,
                                       CPU_INT32U   size)
{
    CPU_INT32U  temp;


    STM32_REG_DMA_IFCR     =  STM32_BIT_DMA_FLAG_GL4 | STM32_BIT_DMA_FLAG_TC4 | STM32_BIT_DMA_FLAG_HT4 | STM32_BIT_DMA_FLAG_TE4;
    STM32_REG_DMAC4_CCR   &= ~DEF_BIT_00;

    temp                   = STM32_REG_DMAC4_CCR;
    temp                  &= 0xFFFF800Fu;
    temp                  |= 0x00000010u | 0x00000080u | 0x00000200u | 0x00000800u | 0x00003000u;
    STM32_REG_DMAC4_CCR    = temp;
    STM32_REG_DMAC4_CNDTR  = size / 4u;
    STM32_REG_DMAC4_CPAR   = STM32_ADDR_SDIO_FIFO;
    STM32_REG_DMAC4_CMAR   = (CPU_INT32U)p_src;

    STM32_REG_DMAC4_CCR   |= DEF_BIT_00;
}

