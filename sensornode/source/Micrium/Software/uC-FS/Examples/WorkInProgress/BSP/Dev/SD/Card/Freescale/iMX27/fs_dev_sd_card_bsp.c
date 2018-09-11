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
*                                 FREESCALE i.MX27 ON THE i.MX27 ADS
*
* Filename      : fs_dev_sd_card_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*                 EFS
*********************************************************************************************************
* Note(s)       : (1) The i.MX27 SD/MMC card interface is described in documentation from Freescale :
*
*                         MCIMX27 Multimedia Applications Processor Reference Manual.  Rev. 03. 12/2008.
*                        "Chapter 27 Secured Digital Host Controller (SDHC)".
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


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

                                                                /* ---------------- SD/MMC CARD IDS ------------------- */
#define  IMX27_SDHC_ID_01                                  1u   /* SD/MMC card 1.                                       */
#define  IMX27_SDHC_ID_02                                  2u   /* SD/MMC card 2.                                       */


                                                                /* ----------------- SDHC UNIT 1 PINS ----------------- */
#define  IMX27_ADS_SD_01_D0                       DEF_BIT_18    /* E18 SD Data 0 pin.                                   */
#define  IMX27_ADS_SD_01_D1                       DEF_BIT_19    /* E19 SD Data 1 pin.                                   */
#define  IMX27_ADS_SD_01_D2                       DEF_BIT_20    /* E20 SD Data 2 pin.                                   */
#define  IMX27_ADS_SD_01_D3                       DEF_BIT_21    /* E21 SD Data 3 pin.                                   */
#define  IMX27_ADS_SD_01_CMD                      DEF_BIT_22    /* E22 SD CMD pin.                                      */
#define  IMX27_ADS_SD_01_CLK                      DEF_BIT_23    /* E23 SD CLK pin.                                      */

#define  IMX27_ADS_SD_01_PINS            (IMX27_ADS_SD_01_D0  | IMX27_ADS_SD_01_D1  | IMX27_ADS_SD_01_D2 | \
                                          IMX27_ADS_SD_01_D3  | IMX27_ADS_SD_01_CMD | IMX27_ADS_SD_01_CLK)


                                                                /* ----------------- SDHC UNIT 2 PINS ----------------- */
#define  IMX27_ADS_SD_02_D0                       DEF_BIT_04    /* B04 SD Data 0 pin.                                   */
#define  IMX27_ADS_SD_02_D1                       DEF_BIT_05    /* B05 SD Data 1 pin.                                   */
#define  IMX27_ADS_SD_02_D2                       DEF_BIT_06    /* B06 SD Data 2 pin.                                   */
#define  IMX27_ADS_SD_02_D3                       DEF_BIT_07    /* B07 SD Data 3 pin.                                   */
#define  IMX27_ADS_SD_02_CMD                      DEF_BIT_08    /* B08 SD CMD pin.                                      */
#define  IMX27_ADS_SD_02_CLK                      DEF_BIT_09    /* B09 SD CLK pin.                                      */

#define  IMX27_ADS_SD_02_PINS            (IMX27_ADS_SD_02_D0  | IMX27_ADS_SD_02_D1  | IMX27_ADS_SD_02_D2 | \
                                          IMX27_ADS_SD_02_D3  | IMX27_ADS_SD_02_CMD | IMX27_ADS_SD_02_CLK)

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  IMX27_REG_SDHC1_BASE_ADDR                 0x10013000u
#define  IMX27_REG_SDHC2_BASE_ADDR                 0x10014000u

#define  IMX27_REG_SDHC1_BUFF_ACCESS               0x10013038u
#define  IMX27_REG_SDHC2_BUFF_ACCESS               0x10014038u


                                                                /* ------------------ CPLD REGISTERS ------------------ */
#define  IMX27_REG_CPLD_BCTRL1_SET (*(CPU_REG16 *)(0xD4000008)) /* Ctrl Set    reg 1.                                   */
#define  IMX27_REG_CPLD_BCTRL1_CLR (*(CPU_REG16 *)(0xD400000C)) /* Ctrl Clear  reg 1.                                   */
#define  IMX27_REG_CPLD_BCTRL2_SET (*(CPU_REG16 *)(0xD4000010)) /* Ctrl Set    reg 2.                                   */
#define  IMX27_REG_CPLD_BCTRL2_CLR (*(CPU_REG16 *)(0xD4000014)) /* Ctrl Clear  reg 2.                                   */
#define  IMX27_REG_CPLD_BCTRL3_SET (*(CPU_REG16 *)(0xD4000018)) /* Ctrl Set    reg 3.                                   */
#define  IMX27_REG_CPLD_BCTRL3_CLR (*(CPU_REG16 *)(0xD400001C)) /* Ctrl Clear  reg 3.                                   */
#define  IMX27_REG_CPLD_BCTRL4_SET (*(CPU_REG16 *)(0xD4000020)) /* Ctrl Set    reg 4.                                   */
#define  IMX27_REG_CPLD_CTRL4_CLR  (*(CPU_REG16 *)(0xD4000024)) /* Ctrl Clear  reg 4.                                   */
#define  IMX27_REG_CPLD_BSTAT1     (*(CPU_REG16 *)(0xD4000028)) /*      Status reg 1.                                   */


                                                                /* ----------- PERIPHERAL CONTROL REGISTERS ----------- */
#define  IMX27_REG_PCCR0           (*(CPU_REG32 *)(0x10027020)) /* Peripheral Clk Ctrl Reg.                             */


                                                                /* ------------------ GPIO REGISTERS ------------------ */
#define  IMX27_REG_PTB_GIUS        (*(CPU_REG32 *)(0x10015120)) /* Port B General Purpose In Use reg.                   */
#define  IMX27_REG_PTB_GPR         (*(CPU_REG32 *)(0x10015138)) /* Port B General Purpose reg.                          */
#define  IMX27_REG_PTB_PUEN        (*(CPU_REG32 *)(0x10015140)) /* Port B Pull Up Enable reg.                           */

#define  IMX27_REG_PTE_GIUS        (*(CPU_REG32 *)(0x10015420)) /* Port E General Purpose In Use reg.                   */
#define  IMX27_REG_PTE_GPR         (*(CPU_REG32 *)(0x10015438)) /* Port E General Purpose reg.                          */
#define  IMX27_REG_PTE_PUEN        (*(CPU_REG32 *)(0x10015440)) /* Port E Pull Up Enable reg.                           */


                                                                /* --------- DRIVE STRENGTH CONTROL REGISTERS --------- */
#define  IMX27_REG_DSCR1           (*(CPU_REG32 *)(0x10027820)) /* Drive Strength Ctrl reg 1.                           */


                                                                /* ------------------ DMAC REGISTERS ------------------ */
#define  IMX27_REG_DMAC_DCR        (*(CPU_REG32 *)(0x10001000)) /* DMA Control Register.                                */
#define  IMX27_REG_DMAC_DISR       (*(CPU_REG32 *)(0x10001004)) /* DMA Interrupt Status Register.                       */
#define  IMX27_REG_DMAC_DIMR       (*(CPU_REG32 *)(0x10001008)) /* DMA Interrupt Mask Register.                         */
#define  IMX27_REG_DMAC_DBTOSR     (*(CPU_REG32 *)(0x1000100C)) /* DMA Burst Time-Out Status Register.                  */
#define  IMX27_REG_DMAC_DRTOSR     (*(CPU_REG32 *)(0x10001010)) /* DMA Request Time-Out Status Register.                */
#define  IMX27_REG_DMAC_DSESR      (*(CPU_REG32 *)(0x10001014)) /* DMA Transfer Error Status Register.                  */
#define  IMX27_REG_DMAC_DBOSR      (*(CPU_REG32 *)(0x10001018)) /* DMA Buffer Overflow Status Register.                 */
#define  IMX27_REG_DMAC_DBTOCR     (*(CPU_REG32 *)(0x1000101C)) /* DMA Burst Time-Out Control Register.                 */
#define  IMX27_REG_DMAC_WSRA       (*(CPU_REG32 *)(0x10001040)) /* W-Size Register A.                                   */
#define  IMX27_REG_DMAC_XSRA       (*(CPU_REG32 *)(0x10001044)) /* X-Size Register A.                                   */
#define  IMX27_REG_DMAC_YSRA       (*(CPU_REG32 *)(0x10001048)) /* Y-Size Register A.                                   */
#define  IMX27_REG_DMAC_WSRB       (*(CPU_REG32 *)(0x1000104C)) /* W-Size Register B.                                   */
#define  IMX27_REG_DMAC_XSRB       (*(CPU_REG32 *)(0x10001050)) /* X-Size Register B.                                   */
#define  IMX27_REG_DMAC_YSRB       (*(CPU_REG32 *)(0x10001054)) /* Y-Size Register B.                                   */

#define  IMX27_REG_DMAC_SAR(n)     (*(CPU_REG32 *)(0x10001080u+ ((n) * 0x40u))) /* Ch Source Address Register.          */
#define  IMX27_REG_DMAC_DAR(n)     (*(CPU_REG32 *)(0x10001084u+ ((n) * 0x40u))) /* Ch Destination Address Register.     */
#define  IMX27_REG_DMAC_CNTR(n)    (*(CPU_REG32 *)(0x10001088u+ ((n) * 0x40u))) /* Ch Count Register.                   */
#define  IMX27_REG_DMAC_CCR(n)     (*(CPU_REG32 *)(0x1000108Cu+ ((n) * 0x40u))) /* Ch Control Register.                 */
#define  IMX27_REG_DMAC_RSSR(n)    (*(CPU_REG32 *)(0x10001090u+ ((n) * 0x40u))) /* Ch Request Source Select Register.   */
#define  IMX27_REG_DMAC_BLR(n)     (*(CPU_REG32 *)(0x10001094u+ ((n) * 0x40u))) /* Ch Burst Length Register.            */
#define  IMX27_REG_DMAC_RTOR(n)    (*(CPU_REG32 *)(0x10001098u+ ((n) * 0x40u))) /* Ch Request Time-Out Register.        */
#define  IMX27_REG_DMAC_BUCR(n)    (*(CPU_REG32 *)(0x10001098u+ ((n) * 0x40u))) /* Ch Bus Utilization Control Register. */
#define  IMX27_REG_DMAC_CCNR(n)    (*(CPU_REG32 *)(0x1000109Cu+ ((n) * 0x40u))) /* Ch Channel Counter Register.         */


/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

                                                                            /* ----------- SDHC STR_STP_CLK ----------- */
#define  IMX27_BIT_STR_STP_CLK_SDHC_RESET                   DEF_BIT_03
#define  IMX27_BIT_STR_STP_CLK_START_CLK                    DEF_BIT_01
#define  IMX27_BIT_STR_STP_CLK_STOP_CLK                     DEF_BIT_00


                                                                            /* -------------- SDHC STATUS ------------- */
#define  IMX27_BIT_STATUS_CARD_INSERTION                    DEF_BIT_31
#define  IMX27_BIT_STATUS_CARD_REMOVAL                      DEF_BIT_30
#define  IMX27_BIT_STATUS_YBUF_EMPTY                        DEF_BIT_29
#define  IMX27_BIT_STATUS_XBUF_EMPTY                        DEF_BIT_28
#define  IMX27_BIT_STATUS_YBUF_FULL                         DEF_BIT_27
#define  IMX27_BIT_STATUS_XBUF_FULL                         DEF_BIT_26
#define  IMX27_BIT_STATUS_BUF_UND_RUN                       DEF_BIT_25
#define  IMX27_BIT_STATUS_BUF_OVFL                          DEF_BIT_24
#define  IMX27_BIT_STATUS_SDIO_INT_ACTIVE                   DEF_BIT_14
#define  IMX27_BIT_STATUS_END_CMD_RESP                      DEF_BIT_13
#define  IMX27_BIT_STATUS_WRITE_OP_DONE                     DEF_BIT_12
#define  IMX27_BIT_STATUS_READ_OP_DONE                      DEF_BIT_11
#define  IMX27_BIT_STATUS_WR_CRC_ERROR_CODE                (DEF_BIT_10 | DEF_BIT_09)
#define  IMX27_BIT_STATUS_CARD_BUS_CLK_RUN                  DEF_BIT_08
#define  IMX27_BIT_STATUS_BUF_READ_READY                    DEF_BIT_07
#define  IMX27_BIT_STATUS_BUF_WRITE_READY                   DEF_BIT_06
#define  IMX27_BIT_STATUS_RESP_CRC_ERR                      DEF_BIT_05
#define  IMX27_BIT_STATUS_READ_CRC_ERR                      DEF_BIT_03
#define  IMX27_BIT_STATUS_WRITE_CRC_ERR                     DEF_BIT_02
#define  IMX27_BIT_STATUS_TIME_OUT_RESP                     DEF_BIT_01
#define  IMX27_BIT_STATUS_TIME_OUT_READ                     DEF_BIT_00
#define  IMX27_BIT_STATUS_ALL_MASK                         (IMX27_BIT_STATUS_CARD_INSERTION | IMX27_BIT_STATUS_CARD_REMOVAL    | IMX27_BIT_STATUS_BUF_UND_RUN       | \
                                                            IMX27_BIT_STATUS_BUF_OVFL       | IMX27_BIT_STATUS_SDIO_INT_ACTIVE | IMX27_BIT_STATUS_END_CMD_RESP      | \
                                                            IMX27_BIT_STATUS_WRITE_OP_DONE  | IMX27_BIT_STATUS_READ_OP_DONE    | IMX27_BIT_STATUS_WR_CRC_ERROR_CODE | \
                                                            IMX27_BIT_STATUS_RESP_CRC_ERR   | IMX27_BIT_STATUS_READ_CRC_ERR    | IMX27_BIT_STATUS_WRITE_CRC_ERR     | \
                                                            IMX27_BIT_STATUS_TIME_OUT_RESP  | IMX27_BIT_STATUS_TIME_OUT_READ   )


                                                                            /* ------------- SDHC CLK_RATE ------------ */
#define  IMX27_BIT_CLK_RATE_CLK_DIVIDER_MASK                0x0000000Fu
#define  IMX27_BIT_CLK_RATE_CLK_PRESCALER_MASK              0x0000FFF0u


                                                                            /* ----------- SDHC CMD_DAT_CONT ---------- */
#define  IMX27_BIT_CMD_DAT_CONT_CMD_RESUME                  DEF_BIT_15
#define  IMX27_BIT_CMD_DAT_CONT_CMD_RESP_LONG_OFF           DEF_BIT_12
#define  IMX27_BIT_CMD_DAT_CONT_STOP_READWAIT               DEF_BIT_11
#define  IMX27_BIT_CMD_DAT_CONT_START_READWAIT              DEF_BIT_10
#define  IMX27_BIT_CMD_DAT_CONT_BUS_WIDTH_1BIT              DEF_BIT_NONE
#define  IMX27_BIT_CMD_DAT_CONT_BUS_WIDTH_4BIT              DEF_BIT_09
#define  IMX27_BIT_CMD_DAT_CONT_INIT                        DEF_BIT_07
#define  IMX27_BIT_CMD_DAT_CONT_WRITE_READ                  DEF_BIT_04
#define  IMX27_BIT_CMD_DAT_CONT_DATA_ENABLE                 DEF_BIT_03
#define  IMX27_BIT_CMD_DAT_CONT_FORMAT_OF_RESPONSE_NONE     DEF_BIT_NONE
#define  IMX27_BIT_CMD_DAT_CONT_FORMAT_OF_RESPONSE_48_CRC   DEF_BIT_00
#define  IMX27_BIT_CMD_DAT_CONT_FORMAT_OF_RESPONSE_136      DEF_BIT_01
#define  IMX27_BIT_CMD_DAT_CONT_FORMAT_OF_RESPONSE_48      (DEF_BIT_01 | DEF_BIT_00)


                                                                            /* -------------- SDHC RES_TO ------------- */
#define  IMX27_BIT_RES_TO_RESPONSE_TIME_OUT_MASK            0x000000FFu


                                                                            /* ------------- SDHC READ_TO ------------- */
#define  IMX27_BIT_READ_TO_DATA_READ_TIME_OUT_MASK          0x0000FFFFu


                                                                            /* ----------------- CPLD ----------------- */
#define  IMX27_BIT_CPLD_SDIO_PWR_EN                         DEF_BIT_03      /* Select 3.15V for the SD/MMC2 power.      */

#define  IMX27_BIT_CPLD_SD1_DET                             DEF_BIT_13      /* SD Card 1 card detected bit.             */
#define  IMX27_BIT_CPLD_SD1_WP                              DEF_BIT_10      /* SD Card 1 write protected bit.           */

#define  IMX27_BIT_CPLD_SD2_DET                             DEF_BIT_12      /* SD Card 2 card detected bit.             */
#define  IMX27_BIT_CPLD_SD2_WP                              DEF_BIT_09      /* SD Card 2 write protected bit.           */

#define  IMX27_BIT_CPLD_SD3_DET                             DEF_BIT_11      /* SD Card 3 card detected bit.             */
#define  IMX27_BIT_CPLD_SD3_WP                              DEF_BIT_08      /* SD Card 3 write protected bit.           */


                                                                            /* ----------------- DMAC ----------------- */
#define  IMX27_BIT_DMAC_BL_16                                       16u     /* 16-bit block length (1-bit mode).        */
#define  IMX27_BIT_DMAC_BL_64                                        0u     /* 64-bit block length (4-bit mode).        */

#define  IMX27_BIT_DMAC_RSSR_DMA_REQ_SDHC1                           7u     /* DMA request channel for SDHC1.           */
#define  IMX27_BIT_DMAC_RSSR_DMA_REQ_SDHC2                           6u     /* DMA request channel for SDHC2.           */


                                                                            /* ----------------- PCCR0 ---------------- */
#define  IMX27_BIT_PCCR0_SDHC3_EN                           DEF_BIT_03
#define  IMX27_BIT_PCCR0_SDHC2_EN                           DEF_BIT_04
#define  IMX27_BIT_PCCR0_SDHC1_EN                           DEF_BIT_05
#define  IMX27_BIT_PCCR0_GPIO_EN                            DEF_BIT_25
#define  IMX27_BIT_PCCR0_DMA_EN                             DEF_BIT_28


                                                                            /* ----------------- DSCR ----------------- */
#define  IMX27_BIT_DSCR_SLOW02_MASK                         0x0000000Cu     /* Drive strength 02 mask.                  */
#define  IMX27_BIT_DSCR_SLOW10_MASK                         0x000C0000u     /* Drive strength 10 mask.                  */
#define  IMX27_BIT_DSCR_SLOW02_MAX_HI                       0x0000000Cu     /* Drive strength 02 mask.                  */
#define  IMX27_BIT_DSCR_SLOW10_MAX_HI                       0x000C0000u     /* Drive strength 10 mask.                  */


                                                                            /* --------------- DMAC DCR --------------- */
#define  IMX27_BIT_DMAC_DCR_DEN                             DEF_BIT_00


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

typedef  struct  imx27_struct_sdhc {
    CPU_REG32  STR_STP_CLK;
    CPU_REG32  STATUS;
    CPU_REG32  CLK_RATE;
    CPU_REG32  CMD_DAT_CONT;
    CPU_REG32  RES_TO;
    CPU_REG32  READ_TO;
    CPU_REG32  BLK_LEN;
    CPU_REG32  NOB;
    CPU_REG32  REV_NO;
    CPU_REG32  INT_CTRL;
    CPU_REG32  CMD;
    CPU_REG32  ARG;
    CPU_REG32  RESERVED1;
    CPU_REG32  RES_FIFO;
    CPU_REG32  BUFFER_ACCESS;
} IMX27_STRUCT_SDHC;


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

static  CPU_BOOLEAN  FSDev_SD_Card_SDHC1_BusWidth;

static  CPU_BOOLEAN  FSDev_SD_Card_SDHC2_BusWidth;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  FSDev_SD_Card_DMA_Start(FS_QTY        unit_nbr,
                                      void         *p_data,
                                      CPU_INT32U    blk_cnt,
                                      CPU_INT32U    blk_size,
                                      CPU_BOOLEAN   rd);


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
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_SD_Card_BSP_Open (FS_QTY  unit_nbr)
{
    IMX27_STRUCT_SDHC  *p_sdhc;


    switch (unit_nbr) {
        case IMX27_SDHC_ID_01:                                          /* ------------------ SDHC 1 ------------------ */
             IMX27_REG_PCCR0     |=   IMX27_BIT_PCCR0_GPIO_EN;          /* En GPIO module clk.                          */
             IMX27_REG_PCCR0     |=   IMX27_BIT_PCCR0_SDHC1_EN;         /* En SDHC module clk.                          */
             IMX27_REG_PTE_GIUS  &= ~(IMX27_ADS_SD_01_PINS);            /* Set SD card pins to multiplexed periph mode  */
             IMX27_REG_PTE_GPR   &= ~(IMX27_ADS_SD_01_PINS);            /* Set SD card pins to act as primary function. */
             IMX27_REG_PTE_PUEN  &= ~(IMX27_ADS_SD_01_PINS);            /* Dis pull-up resistors for all pins.          */
             IMX27_REG_PTE_PUEN  |=  (IMX27_ADS_SD_01_CMD);             /* En  pull-up resistor  for CMD pin.           */

                                                                        /* I/O drive strength set to 6mA.               */
             IMX27_REG_DSCR1     &= ~IMX27_BIT_DSCR_SLOW10_MASK;
             IMX27_REG_DSCR1     |=  IMX27_BIT_DSCR_SLOW10_MAX_HI;
             p_sdhc               = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC1_BASE_ADDR;

             FSDev_SD_Card_SDHC1_BusWidth = 1u;                         /* Dflt, 1-bit bus.                             */
             break;


        case IMX27_SDHC_ID_02:                                          /* ------------------ SDHC 2 ------------------ */
             IMX27_REG_PCCR0           |=   IMX27_BIT_PCCR0_GPIO_EN;    /* En GPIO module clk.                          */
             IMX27_REG_PCCR0           |=   IMX27_BIT_PCCR0_SDHC2_EN;   /* En SDHC module clk.                          */
             IMX27_REG_PTB_GIUS        &= ~(IMX27_ADS_SD_02_PINS);      /* Set SD card pins to multiplexed periph mode. */
             IMX27_REG_PTB_GPR         &= ~(IMX27_ADS_SD_02_PINS);      /* Set SD card pins to act as primary function. */
             IMX27_REG_PTB_PUEN        &= ~(IMX27_ADS_SD_02_PINS);      /* Dis pull-up resistors for all pins.          */
             IMX27_REG_PTB_PUEN        |=  (IMX27_ADS_SD_02_CMD);       /* En  pull-up resistor  for CMD pin.           */

                                                                        /* I/O drive strength set to 6mA.               */
             IMX27_REG_DSCR1           &= ~IMX27_BIT_DSCR_SLOW02_MASK;
             IMX27_REG_DSCR1           |=  IMX27_BIT_DSCR_SLOW02_MAX_HI;

             IMX27_REG_CPLD_BCTRL1_SET  =  IMX27_BIT_CPLD_SDIO_PWR_EN;  /* Enable SD/MMC2 on the ADS board.             */
             p_sdhc                     = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC2_BASE_ADDR;

             FSDev_SD_Card_SDHC2_BusWidth = 1u;                         /* Dflt, 1-bit bus.                             */
             break;


        default:                                                        /* --------------- UNSUPPORTED ---------------- */
            FS_TRACE_INFO(("FSDev_SD_Card_BSP_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
            return (DEF_FAIL);
    }


                                                                        /* ----------------- INIT SDHC ---------------- */
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_SDHC_RESET;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_SDHC_RESET             /* Apply reset with a stop clk cmd.             */
                         | IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;              /* Issue 8 stop clk instr to fill reset cycle.  */
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->STR_STP_CLK  = IMX27_BIT_STR_STP_CLK_STOP_CLK;
    p_sdhc->CLK_RATE     = 0x3Fu;                                       /* Cfg SDHC clk reg divider bits.               */
    p_sdhc->RES_TO       = 0xFFu;
    p_sdhc->BLK_LEN      = 512u;
    p_sdhc->NOB          = 1u;

    p_sdhc->STR_STP_CLK |= IMX27_BIT_STR_STP_CLK_STOP_CLK;
    while (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_CARD_BUS_CLK_RUN) == DEF_YES) {
        ;
    }

    IMX27_REG_PCCR0    |= IMX27_BIT_PCCR0_DMA_EN;                       /* Init DMA module                              */
    IMX27_REG_DMAC_DCR |= IMX27_BIT_DMAC_DCR_DEN;

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
                                                                /* ---------------- DIS SDHC MODULE CLK --------------- */
    switch (unit_nbr) {
        case IMX27_SDHC_ID_01:
             IMX27_REG_PCCR0 &= ~IMX27_BIT_PCCR0_SDHC1_EN;
             break;

        case IMX27_SDHC_ID_02:
             IMX27_REG_PCCR0 &= ~IMX27_BIT_PCCR0_SDHC2_EN;
             break;

        default:
             break;
    }
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
    IMX27_STRUCT_SDHC  *p_sdhc;
    CPU_INT32U          cmd_dat_cont;


    cmd_dat_cont = 0u;
    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
        if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
            cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_FORMAT_OF_RESPONSE_136;
        } else {
            if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_CRC_VALID) == DEF_YES) {
                cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_FORMAT_OF_RESPONSE_48_CRC;
            } else {
                cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_FORMAT_OF_RESPONSE_48;
            }
        }
    }

    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_DATA_START) == DEF_YES) {
        cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_DATA_ENABLE;     /* Set the data transfer enable bit.                        */
    }

    if (p_cmd->DataDir == FS_DEV_SD_CARD_DATA_DIR_HOST_TO_CARD) {
        cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_WRITE_READ;      /* Set WRITE bit.                                           */
    }

    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_INIT) == DEF_YES) {
        cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_INIT;            /* Set the INIT bit, adds 80 clock cycles before each cmd.  */
    }

    if (unit_nbr == IMX27_SDHC_ID_01) {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC1_BASE_ADDR;
        if (FSDev_SD_Card_SDHC1_BusWidth == 4u) {
           cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_BUS_WIDTH_4BIT;
        }
    } else {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC2_BASE_ADDR;
        if (FSDev_SD_Card_SDHC2_BusWidth == 4u) {
           cmd_dat_cont |= IMX27_BIT_CMD_DAT_CONT_BUS_WIDTH_4BIT;
        }
    }

    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_DATA_START) == DEF_YES) {
        p_sdhc->NOB     = p_cmd->BlkCnt;
        p_sdhc->BLK_LEN = p_cmd->BlkSize;
    }

    p_sdhc->STATUS       = IMX27_BIT_STATUS_ALL_MASK;
    p_sdhc->CMD          = p_cmd->Cmd;                 /* Write cmd.                                               */
    p_sdhc->ARG          = p_cmd->Arg;                 /* Write arg.                                               */
    p_sdhc->CMD_DAT_CONT = cmd_dat_cont;               /* Write the cmd & data ctrl reg, start cmd.                */

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
    IMX27_STRUCT_SDHC  *p_sdhc;
    CPU_INT32U          resp_half;
    CPU_INT32U          resp_word;


    if (unit_nbr == IMX27_SDHC_ID_01) {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC1_BASE_ADDR;
    } else {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC2_BASE_ADDR;
    }

    while (DEF_TRUE) {
        if (DEF_BIT_IS_CLR(p_sdhc->STATUS, IMX27_BIT_STATUS_CARD_BUS_CLK_RUN) == DEF_YES) {
            p_sdhc->STR_STP_CLK |= IMX27_BIT_STR_STP_CLK_START_CLK;
            while (DEF_BIT_IS_CLR(p_sdhc->STATUS, IMX27_BIT_STATUS_CARD_BUS_CLK_RUN) == DEF_YES) {
                ;
            }
        }

        if (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_END_CMD_RESP) == DEF_YES) {
            if (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_TIME_OUT_RESP) == DEF_YES) {
                p_sdhc->STATUS |= IMX27_BIT_STATUS_TIME_OUT_RESP;
               *p_err = FS_DEV_SD_CARD_ERR_RESP_TIMEOUT;
                return;
            }

            if (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_RESP_CRC_ERR) == DEF_YES) {
                p_sdhc->STATUS |= IMX27_BIT_STATUS_RESP_CRC_ERR;
               *p_err = FS_DEV_SD_CARD_ERR_RESP_CHKSUM;
                return;
            }

            if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word  = (resp_half << 16);
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word |=  resp_half;
               *p_resp     =  resp_word;
                p_resp++;

                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word  = (resp_half << 16);
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word |=  resp_half;
               *p_resp     =  resp_word;
                p_resp++;

                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word  = (resp_half << 16);
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word |=  resp_half;
               *p_resp     =  resp_word;
                p_resp++;

                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word  = (resp_half << 16);
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word |=  resp_half;
               *p_resp     =  resp_word;
                p_resp++;

            } else if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word  = (resp_half << 24);
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word |= (resp_half <<  8);
                resp_half  =  p_sdhc->RES_FIFO & DEF_INT_16_MASK;
                resp_word |= (resp_half >>  8);
               *p_resp     = resp_word;
            }

            p_sdhc->STATUS |= IMX27_BIT_STATUS_END_CMD_RESP;
           *p_err = FS_DEV_SD_CARD_ERR_NONE;
            return;
        }
    }

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
    IMX27_STRUCT_SDHC  *p_sdhc;
    CPU_INT32U          tot_octets;
    CPU_INT32U          tot_rd_octets;


    FSDev_SD_Card_DMA_Start(unit_nbr,
                            p_dest,
                            p_cmd->BlkCnt,
                            p_cmd->BlkSize,
                            DEF_YES);

    if (unit_nbr == IMX27_SDHC_ID_01) {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC1_BASE_ADDR;
    } else {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC2_BASE_ADDR;
    }

    while (DEF_BIT_IS_CLR(p_sdhc->STATUS, IMX27_BIT_STATUS_READ_OP_DONE) == DEF_YES) {
        ;
    }

    tot_octets    =  p_cmd->BlkCnt * p_cmd->BlkSize;
    tot_rd_octets = (unit_nbr == IMX27_SDHC_ID_01) ? IMX27_REG_DMAC_CCNR(8u) : IMX27_REG_DMAC_CCNR(9u);

    if (tot_octets != tot_rd_octets) {
       *p_err = FS_DEV_SD_CARD_ERR_DATA_OVERRUN;

    } else if (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_READ_CRC_ERR) == DEF_YES) {
       *p_err = FS_DEV_SD_CARD_ERR_DATA_CHKSUM;

    } else if (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_TIME_OUT_READ) == DEF_YES) {
       *p_err = FS_DEV_SD_CARD_ERR_DATA_TIMEOUT;

    } else if (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_BUF_OVFL) == DEF_YES) {
       *p_err = FS_DEV_SD_CARD_ERR_DATA_OVERRUN;

    } else {
       *p_err = FS_DEV_SD_CARD_ERR_NONE;
    }

    p_sdhc->STATUS = IMX27_BIT_STATUS_READ_OP_DONE;
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
    IMX27_STRUCT_SDHC  *p_sdhc;
    CPU_INT32U          tot_octets;
    CPU_INT32U          tot_wr_octets;


    FSDev_SD_Card_DMA_Start(unit_nbr,
                            p_src,
                            p_cmd->BlkCnt,
                            p_cmd->BlkSize,
                            DEF_NO);

    if (unit_nbr == IMX27_SDHC_ID_01) {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC1_BASE_ADDR;
    } else {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC2_BASE_ADDR;
    }

    while (DEF_BIT_IS_CLR(p_sdhc->STATUS, IMX27_BIT_STATUS_WRITE_OP_DONE) == DEF_YES) {
        ;
    }

    tot_octets    =  p_cmd->BlkCnt * p_cmd->BlkSize;
    tot_wr_octets = (unit_nbr == IMX27_SDHC_ID_01) ? IMX27_REG_DMAC_CCNR(8u) : IMX27_REG_DMAC_CCNR(9u);

    if (tot_octets != tot_wr_octets) {
       *p_err = FS_DEV_SD_CARD_ERR_DATA_UNDERRUN;

    } else if (DEF_BIT_IS_SET(p_sdhc->STATUS, IMX27_BIT_STATUS_WRITE_CRC_ERR) == DEF_YES) {
       *p_err = FS_DEV_SD_CARD_ERR_DATA_CHKSUM;

    } else {
       *p_err = FS_DEV_SD_CARD_ERR_NONE;
    }

    p_sdhc->STATUS = IMX27_BIT_STATUS_WRITE_OP_DONE;
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
* Note(s)     : (1) The SDHC interface is capable of 1- & 4-bit operation.
*********************************************************************************************************
*/

CPU_INT08U  FSDev_SD_Card_BSP_GetBusWidthMax (FS_QTY  unit_nbr)
{
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
* Note(s)     : (1) The bus width setting is applied in FSDev_SD_Card_BSP_CmdStart() upon command
*                   execution based on the value of FSDev_SD_Card_SDHCx_BusWidth.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetBusWidth (FS_QTY      unit_nbr,
                                     CPU_INT08U  width)
{
    if (unit_nbr == IMX27_SDHC_ID_01) {
        FSDev_SD_Card_SDHC1_BusWidth = width;
    } else {
        FSDev_SD_Card_SDHC2_BusWidth = width;
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
* Note(s)     : (1) The effective clock frequency MUST be no more than 'freq'.  If the frequency cannot be
*                   configured equal to 'freq', it should be configured less than 'freq'.
*
*                   (a) When separating the clock frequency divider into clock divider & clock prescaler,
*                       the result of each division-by-two is rounded up.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetClkFreq (FS_QTY      unit_nbr,
                                    CPU_INT32U  freq)
{
    IMX27_STRUCT_SDHC  *p_sdhc;
    CPU_INT32U          clk_div;
    CPU_INT32U          clk_div_log;
    CPU_INT32U          clk_freq;
    CPU_INT32U          divider;
    CPU_INT32U          prescaler;


    clk_freq = BSP_ClkFreq(BSP_PERCLK2);
    clk_div  = (clk_freq + freq - 1u) / freq;

    if (clk_div <= 2u) {
        prescaler = 0u;
        divider   = 1u;

    } else {
        clk_div_log = 0u;
        while (clk_div > 16u) {
            clk_div = (clk_div + 1u) / 2u;                      /* Divide by 2, rounding up (see Note #1a).             */
            clk_div_log++;
        }

        prescaler = clk_div_log;
        divider   = clk_div - 1u;
    }

    if (unit_nbr == IMX27_SDHC_ID_01) {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC1_BASE_ADDR;
    } else {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC2_BASE_ADDR;
    }

    p_sdhc->CLK_RATE = (prescaler << 4) | divider;
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
    IMX27_STRUCT_SDHC  *p_sdhc;


    if (unit_nbr == IMX27_SDHC_ID_01) {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC1_BASE_ADDR;
    } else {
        p_sdhc = (IMX27_STRUCT_SDHC *)IMX27_REG_SDHC2_BASE_ADDR;
    }

    p_sdhc->RES_TO = to_clks;
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
*                                      FSDev_SD_Card_DMA_Start()
*
* Description : Start DMA execution.
*
* Argument(s) : unit_nbr    Unit number of SD/MMC card.
*
*               p_data      Pointer to data buffer.
*
*               blk_cnt     Number of blocks to read or write.
*
*               blk_size    Size of each block, in octets.
*
*               rd          Indicates whether operation is read or write :
*
*                               DEF_YES, operation is read  (card to host).
*                               DEF_NO,  operation is write (host to card).
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_SD_Card_BSP_CmdDataRd(),
*               FSDev_SD_Card_BSP_CmdDataWr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_SD_Card_DMA_Start (FS_QTY        unit_nbr,
                                       void         *p_data,
                                       CPU_INT32U    blk_cnt,
                                       CPU_INT32U    blk_size,
                                       CPU_BOOLEAN   rd)
{
    CPU_INT32U  tot_octets;


    tot_octets = blk_cnt * blk_size;

    if (unit_nbr == IMX27_SDHC_ID_01) {                                     /* ---------------- SDHC 1 ---------------- */
        IMX27_REG_DMAC_DISR    = DEF_BIT_08;                                /* Clr all status reg bits.                 */
        IMX27_REG_DMAC_DIMR   |= DEF_BIT_08;
        IMX27_REG_DMAC_DBTOSR  = DEF_BIT_08;
        IMX27_REG_DMAC_DSESR   = DEF_BIT_08;
        IMX27_REG_DMAC_DRTOSR  = DEF_BIT_08;
        IMX27_REG_DMAC_DBOSR   = DEF_BIT_08;

        IMX27_REG_DMAC_CCR(8u) = DEF_BIT_NONE;                              /* Dis ch.                                  */

        if (rd == DEF_YES ) {
            IMX27_REG_DMAC_SAR(8u)  =  IMX27_REG_SDHC1_BUFF_ACCESS;         /* Set SDHC buf as src addr.                */
            IMX27_REG_DMAC_DAR(8u)  = (CPU_INT32U)p_data;                   /* Set the data buf as dest addr            */
            IMX27_REG_DMAC_BLR(8u)  = (FSDev_SD_Card_SDHC1_BusWidth == 4u) ? IMX27_BIT_DMAC_BL_64 : IMX27_BIT_DMAC_BL_16;
            IMX27_REG_DMAC_CNTR(8u) =  tot_octets;                          /* DMA Transfer Count                       */
            IMX27_REG_DMAC_RSSR(8u) =  IMX27_BIT_DMAC_RSSR_DMA_REQ_SDHC1;   /* SDHC1 Request source                     */
            IMX27_REG_DMAC_CCR(8u)  =  DEF_BIT_11 | DEF_BIT_03;             /* Src = FIFO, dest = line, en DMA req sig. */
            IMX27_REG_DMAC_CCR(8u) |=  DEF_BIT_00;                          /* Start DMA Transmission                   */


        } else {
            IMX27_REG_DMAC_SAR(8u)  = (CPU_INT32U)p_data;                   /* Set the data buf as src addr             */
            IMX27_REG_DMAC_DAR(8u)  =  IMX27_REG_SDHC1_BUFF_ACCESS;         /* Set SDHC buf as dest addr.               */
            IMX27_REG_DMAC_BLR(8u)  = (FSDev_SD_Card_SDHC1_BusWidth == 4u) ? IMX27_BIT_DMAC_BL_64 : IMX27_BIT_DMAC_BL_16;
            IMX27_REG_DMAC_CNTR(8u) =  tot_octets;                          /* DMA Transfer Count                       */
            IMX27_REG_DMAC_RSSR(8u) =  IMX27_BIT_DMAC_RSSR_DMA_REQ_SDHC1;   /* SDHC1 Request source                     */
            IMX27_REG_DMAC_CCR(8u)  =  DEF_BIT_13 | DEF_BIT_03;             /* Dest = FIFO, src = line, en DMA req sig. */
            IMX27_REG_DMAC_CCR(8u) |=  DEF_BIT_00;                          /* Start DMA Transmission                   */
        }



    } else {                                                                /* ---------------- SDHC 2 ---------------- */
        IMX27_REG_DMAC_DISR    = DEF_BIT_09;                                /* Clr all status reg bits.                 */
        IMX27_REG_DMAC_DIMR   |= DEF_BIT_09;
        IMX27_REG_DMAC_DBTOSR  = DEF_BIT_09;
        IMX27_REG_DMAC_DSESR   = DEF_BIT_09;
        IMX27_REG_DMAC_DRTOSR  = DEF_BIT_09;
        IMX27_REG_DMAC_DBOSR   = DEF_BIT_09;

        IMX27_REG_DMAC_CCR(9u) = DEF_BIT_NONE;                              /* Dis ch.                                  */

        if (rd == DEF_YES ) {
            IMX27_REG_DMAC_SAR(9u)  =  IMX27_REG_SDHC2_BUFF_ACCESS;         /* Set SDHC buf as src addr.                */
            IMX27_REG_DMAC_DAR(9u)  = (CPU_INT32U)p_data;                   /* Set the data buf as dest addr            */
            IMX27_REG_DMAC_BLR(9u)  = (FSDev_SD_Card_SDHC2_BusWidth == 4u) ? IMX27_BIT_DMAC_BL_64 : IMX27_BIT_DMAC_BL_16;
            IMX27_REG_DMAC_CNTR(9u) =  tot_octets;                          /* DMA Transfer Count                       */
            IMX27_REG_DMAC_RSSR(9u) =  IMX27_BIT_DMAC_RSSR_DMA_REQ_SDHC2;   /* SDHC1 Request source                     */
            IMX27_REG_DMAC_CCR(9u)  =  DEF_BIT_11 | DEF_BIT_03;             /* Src = FIFO, dest = line, en DMA req sig. */
            IMX27_REG_DMAC_CCR(9u) |=  DEF_BIT_00;                          /* Start DMA Transmission                   */


        } else {
            IMX27_REG_DMAC_SAR(9u)  = (CPU_INT32U)p_data;                   /* Set the data buf as src addr             */
            IMX27_REG_DMAC_DAR(9u)  =  IMX27_REG_SDHC2_BUFF_ACCESS;         /* Set SDHC buf as dest addr.               */
            IMX27_REG_DMAC_BLR(9u)  = (FSDev_SD_Card_SDHC2_BusWidth == 4u) ? IMX27_BIT_DMAC_BL_64 : IMX27_BIT_DMAC_BL_16;
            IMX27_REG_DMAC_CNTR(9u) =  tot_octets;                          /* DMA Transfer Count                       */
            IMX27_REG_DMAC_RSSR(9u) =  IMX27_BIT_DMAC_RSSR_DMA_REQ_SDHC2;   /* SDHC1 Request source                     */
            IMX27_REG_DMAC_CCR(9u)  =  DEF_BIT_13 | DEF_BIT_03;             /* Dest = FIFO, src = line, en DMA req sig. */
            IMX27_REG_DMAC_CCR(9u) |=  DEF_BIT_00;                          /* Start DMA Transmission                   */
        }
    }
}
