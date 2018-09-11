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
*                                 FREESCALE i.MX27 ON THE i.MX27 ADS
*
* Filename      : fs_dev_ide_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
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

#define  FS_DEV_IDE_PIO_MAX_MODE                           4u
#define  FS_DEV_IDE_DMA_MAX_MODE                           2u

#define  IMX27_CLK_FREQ_ATA                             100000000uL

#define  IMX27_GPIO_ATA_DATA_00_14                      0x0001FFFCu     /* PD2..PD16.                                   */
#define  IMX27_GPIO_ATA_DATA_15                         0x00800000u     /* PF23.                                        */

#define  IMX27_GPIO_ATA_DIOR                            DEF_BIT_20      /* PF20.                                        */
#define  IMX27_GPIO_ATA_DIOW                            DEF_BIT_19      /* PF19.                                        */
#define  IMX27_GPIO_ATA_CS1                             DEF_BIT_18      /* PF18.                                        */
#define  IMX27_GPIO_ATA_CS0                             DEF_BIT_17      /* PF17.                                        */
#define  IMX27_GPIO_ATA_DA2                             DEF_BIT_16      /* PF16.                                        */
#define  IMX27_GPIO_ATA_DA1                             DEF_BIT_14      /* PF14.                                        */
#define  IMX27_GPIO_ATA_DA0                             DEF_BIT_13      /* PF13.                                        */
#define  IMX27_GPIO_ATA_DMARQ                           DEF_BIT_12      /* PF12.                                        */
#define  IMX27_GPIO_ATA_DMACK                           DEF_BIT_11      /* PF11.                                        */
#define  IMX27_GPIO_ATA_RESET_B                         DEF_BIT_10      /* PF10.                                        */
#define  IMX27_GPIO_ATA_INTRQ                           DEF_BIT_09      /* PF09.                                        */
#define  IMX27_GPIO_ATA_IORDY                           DEF_BIT_08      /* PF08.                                        */
#define  IMX27_GPIO_ATA_BUFFER_EN                       DEF_BIT_07      /* PF07.                                        */

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

                                                                        /* -------------- CPLD REGISTERS -------------- */
#define  IMX27_REG_CPLD_BCTRL1_SET      (*(CPU_REG16 *)(0xD4000008u))   /* Ctrl Set    reg 1.                           */
#define  IMX27_REG_CPLD_BCTRL1_CLR      (*(CPU_REG16 *)(0xD400000Cu))   /* Ctrl Clear  reg 1.                           */
#define  IMX27_REG_CPLD_BCTRL2_SET      (*(CPU_REG16 *)(0xD4000010u))   /* Ctrl Set    reg 2.                           */
#define  IMX27_REG_CPLD_BCTRL2_CLR      (*(CPU_REG16 *)(0xD4000014u))   /* Ctrl Clear  reg 2.                           */
#define  IMX27_REG_CPLD_BCTRL3_SET      (*(CPU_REG16 *)(0xD4000018u))   /* Ctrl Set    reg 3.                           */
#define  IMX27_REG_CPLD_BCTRL3_CLR      (*(CPU_REG16 *)(0xD400001Cu))   /* Ctrl Clear  reg 3.                           */
#define  IMX27_REG_CPLD_BCTRL4_SET      (*(CPU_REG16 *)(0xD4000020u))   /* Ctrl Set    reg 4.                           */
#define  IMX27_REG_CPLD_CTRL4_CLR       (*(CPU_REG16 *)(0xD4000024u))   /* Ctrl Clear  reg 4.                           */
#define  IMX27_REG_CPLD_BSTAT1          (*(CPU_REG16 *)(0xD4000028u))   /*      Status reg 1.                           */


                                                                        /* ------- PERIPHERAL CONTROL REGISTERS ------- */
#define  IMX27_REG_PCCR0                (*(CPU_REG32 *)(0x10027020u))   /* Peripheral Clk Ctrl Reg 0.                   */
#define  IMX27_REG_PCCR1                (*(CPU_REG32 *)(0x10027024u))   /* Peripheral Clk Ctrl Reg 1.                   */


                                                                        /* -------------- GPIO REGISTERS -------------- */
#define  IMX27_REG_PTD_GIUS             (*(CPU_REG32 *)(0x10015320u))   /* Port D General Purpose In Use reg.           */
#define  IMX27_REG_PTD_GPR              (*(CPU_REG32 *)(0x10015338u))   /* Port D General Purpose reg.                  */
#define  IMX27_REG_PTD_PUEN             (*(CPU_REG32 *)(0x10015340u))   /* Port D Pull Up Enable reg.                   */

#define  IMX27_REG_PTF_GIUS             (*(CPU_REG32 *)(0x10015520u))   /* Port F General Purpose In Use reg.           */
#define  IMX27_REG_PTF_GPR              (*(CPU_REG32 *)(0x10015538u))   /* Port F General Purpose reg.                  */
#define  IMX27_REG_PTF_PUEN             (*(CPU_REG32 *)(0x10015540u))   /* Port F Pull Up Enable reg.                   */


                                                                        /* --------------- ATA REGISTERS -------------- */
#define  IMX27_REG_TIME_CONFIG0         (*(CPU_REG32 *)(0x80001000u))
#define  IMX27_REG_TIME_CONFIG1         (*(CPU_REG32 *)(0x80001004u))
#define  IMX27_REG_TIME_CONFIG2         (*(CPU_REG32 *)(0x80001008u))
#define  IMX27_REG_TIME_CONFIG3         (*(CPU_REG32 *)(0x8000100Cu))
#define  IMX27_REG_TIME_CONFIG4         (*(CPU_REG32 *)(0x80001010u))
#define  IMX27_REG_TIME_CONFIG5         (*(CPU_REG32 *)(0x80001014u))
#define  IMX27_REG_FIFO_DATA_32         (*(CPU_REG32 *)(0x80001018u))
#define  IMX27_REG_FIFO_DATA_16         (*(CPU_REG16 *)(0x8000101Cu))
#define  IMX27_REG_FIFO_FILL            (*(CPU_REG32 *)(0x80001020u))
#define  IMX27_REG_ATA_CONTROL          (*(CPU_REG32 *)(0x80001024u))
#define  IMX27_REG_INT_PENDING          (*(CPU_REG32 *)(0x80001028u))
#define  IMX27_REG_INT_ENABLE           (*(CPU_REG32 *)(0x8000102Cu))
#define  IMX27_REG_INT_CLEAR            (*(CPU_REG32 *)(0x80001030u))
#define  IMX27_REG_FIFO_ALARM           (*(CPU_REG32 *)(0x80001034u))
#define  IMX27_ADDR_FIFO                                0x80001018u


                                                                        /* ------------ ATA DRIVE REGISTERS ----------- */
#define  IMX27_REG_DDTR                 (*(CPU_REG16 *)(0x800010A0u))   /* Drive Data Register.                         */
#define  IMX27_REG_DFTR                 (*(CPU_REG08 *)(0x800010A4u))   /* Drive Features Register.                     */
#define  IMX27_REG_DSCR                 (*(CPU_REG08 *)(0x800010A8u))   /* Drive Sector Count Register.                 */
#define  IMX27_REG_DSNR                 (*(CPU_REG08 *)(0x800010ACu))   /* Drive Sector Number Register.                */
#define  IMX27_REG_DCLR                 (*(CPU_REG08 *)(0x800010B0u))   /* Drive Cylinder Low Register.                 */
#define  IMX27_REG_DCHR                 (*(CPU_REG08 *)(0x800010B4u))   /* Drive Cylinder High Register.                */
#define  IMX27_REG_DDHR                 (*(CPU_REG08 *)(0x800010B8u))   /* Drive Device Head Register.                  */
#define  IMX27_REG_DCDR                 (*(CPU_REG08 *)(0x800010BCu))   /* Drive Command / Status Register.             */
#define  IMX27_REG_DCTR                 (*(CPU_REG08 *)(0x800010D8u))   /* Drive Alternate Status / Counter Register.   */

                                                                        /* -------------- DMAC REGISTERS -------------- */
#define  IMX27_REG_DMAC_DCR             (*(CPU_REG32 *)(0x10001000u))   /* DMA Control Register.                        */
#define  IMX27_REG_DMAC_DISR            (*(CPU_REG32 *)(0x10001004u))   /* DMA Interrupt Status Register.               */
#define  IMX27_REG_DMAC_DIMR            (*(CPU_REG32 *)(0x10001008u))   /* DMA Interrupt Mask Register.                 */
#define  IMX27_REG_DMAC_DBTOSR          (*(CPU_REG32 *)(0x1000100Cu))   /* DMA Burst Time-Out Status Register.          */
#define  IMX27_REG_DMAC_DRTOSR          (*(CPU_REG32 *)(0x10001010u))   /* DMA Request Time-Out Status Register.        */
#define  IMX27_REG_DMAC_DSESR           (*(CPU_REG32 *)(0x10001014u))   /* DMA Transfer Error Status Register.          */
#define  IMX27_REG_DMAC_DBOSR           (*(CPU_REG32 *)(0x10001018u))   /* DMA Buffer Overflow Status Register.         */
#define  IMX27_REG_DMAC_DBTOCR          (*(CPU_REG32 *)(0x1000101Cu))   /* DMA Burst Time-Out Control Register.         */
#define  IMX27_REG_DMAC_WSRA            (*(CPU_REG32 *)(0x10001040u))   /* W-Size Register A.                           */
#define  IMX27_REG_DMAC_XSRA            (*(CPU_REG32 *)(0x10001044u))   /* X-Size Register A.                           */
#define  IMX27_REG_DMAC_YSRA            (*(CPU_REG32 *)(0x10001048u))   /* Y-Size Register A.                           */
#define  IMX27_REG_DMAC_WSRB            (*(CPU_REG32 *)(0x1000104Cu))   /* W-Size Register B.                           */
#define  IMX27_REG_DMAC_XSRB            (*(CPU_REG32 *)(0x10001050u))   /* X-Size Register B.                           */
#define  IMX27_REG_DMAC_YSRB            (*(CPU_REG32 *)(0x10001054u))   /* Y-Size Register B.                           */

#define  IMX27_REG_DMAC_SAR(n)          (*(CPU_REG32 *)(0x10001080u + ((n) * 0x40u)))   /* Ch Source Address Reg.       */
#define  IMX27_REG_DMAC_DAR(n)          (*(CPU_REG32 *)(0x10001084u + ((n) * 0x40u)))   /* Ch Destination Address Reg.  */
#define  IMX27_REG_DMAC_CNTR(n)         (*(CPU_REG32 *)(0x10001088u + ((n) * 0x40u)))   /* Ch Count Reg.                */
#define  IMX27_REG_DMAC_CCR(n)          (*(CPU_REG32 *)(0x1000108Cu + ((n) * 0x40u)))   /* Ch Control Reg.              */
#define  IMX27_REG_DMAC_RSSR(n)         (*(CPU_REG32 *)(0x10001090u + ((n) * 0x40u)))   /* Ch Request Src Select Reg.   */
#define  IMX27_REG_DMAC_BLR(n)          (*(CPU_REG32 *)(0x10001094u + ((n) * 0x40u)))   /* Ch Burst Length Reg.         */
#define  IMX27_REG_DMAC_RTOR(n)         (*(CPU_REG32 *)(0x10001098u + ((n) * 0x40u)))   /* Ch Request Time-Out Reg.     */
#define  IMX27_REG_DMAC_BUCR(n)         (*(CPU_REG32 *)(0x10001098u + ((n) * 0x40u)))   /* Ch Bus Utilization Ctrl Reg. */
#define  IMX27_REG_DMAC_CCNR(n)         (*(CPU_REG32 *)(0x1000109Cu + ((n) * 0x40u)))   /* Ch Channel Counter Reg.      */

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

                                                                        /* ------------------- CPLD ------------------- */
#define  IMX27_BIT_CPLD_ATA_FEC_EN                          DEF_BIT_04  /* Enable the ATA/FEC data path.                */
#define  IMX27_BIT_CPLD_ATA_FEC_SEL                         DEF_BIT_05  /* Select the ATA/FEC data.                     */
#define  IMX27_BIT_CPLD_ATA_EN                              DEF_BIT_06  /* Enable the ATA data path.                    */

                                                                        /* ------------------- DMAC ------------------- */
#define  IMX27_BIT_DMAC_RSSR_DMA_REQ_ATA_RCV_FIFO                   29u /* DMA request channel for ATA rx FIFO.         */
#define  IMX27_BIT_DMAC_RSSR_DMA_REQ_ATA_TX_FIFO                    28u /* DMA request channel for ATA tx FIFO.         */

                                                                        /* ------------------- PCCR1 ------------------ */
#define  IMX27_BIT_PCCR0_GPIO_EN                            DEF_BIT_25
#define  IMX27_BIT_PCCR0_DMA_EN                             DEF_BIT_28


                                                                        /* ------------------- PCCR1 ------------------ */
#define  IMX27_BIT_PCCR1_HCLK_ATA                           DEF_BIT_23


                                                                        /* ---------------- ATA_CONTROL --------------- */
#define  IMX27_BIT_ATA_CONTROL_FIFO_RST_B                   DEF_BIT_07
#define  IMX27_BIT_ATA_CONTROL_ATA_RST_B                    DEF_BIT_06
#define  IMX27_BIT_ATA_CONTROL_FIFO_TX_EN                   DEF_BIT_05
#define  IMX27_BIT_ATA_CONTROL_FIFO_RCV_EN                  DEF_BIT_04
#define  IMX27_BIT_ATA_CONTROL_DMA_PENDING                  DEF_BIT_03
#define  IMX27_BIT_ATA_CONTROL_DMA_ULTRA_SELECTED           DEF_BIT_02
#define  IMX27_BIT_ATA_CONTROL_DMA_WRITE                    DEF_BIT_01
#define  IMX27_BIT_ATA_CONTROL_IORDY_EN                     DEF_BIT_00


                                                                        /* ---------------- INT_PENDING --------------- */
                                                                        /* ---------------- INT_ENABLE  --------------- */
                                                                        /* ---------------- INT_CLEAR   --------------- */
#define  IMX27_BIT_ATA_INT_ATA_INTRQ1                       DEF_BIT_07
#define  IMX27_BIT_ATA_INT_FIFO_UNDERFLOW                   DEF_BIT_06
#define  IMX27_BIT_ATA_INT_FIFO_OVERFLOW                    DEF_BIT_05
#define  IMX27_BIT_ATA_INT_CONTROLLER_IDLE                  DEF_BIT_04
#define  IMX27_BIT_ATA_INT_ATA_INTRQ2                       DEF_BIT_03


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
*                                             PIO TIMINGS
*
* Note(s) : (1) True IDE mode PIO timings are given in "CF+ & CF SPECIFICATION REV. 4.1", Table 22.
*               All timings are in nanoseconds.
*********************************************************************************************************
*/

typedef  struct  fs_dev_ide_timing_pio {
    CPU_INT16U  t0;
    CPU_INT08U  t1;
    CPU_INT08U  t2_16;
    CPU_INT16U  t2_08;
    CPU_INT08U  t2i;
    CPU_INT08U  t3;
    CPU_INT08U  t4;
    CPU_INT08U  t5;
    CPU_INT08U  t6;
    CPU_INT08U  t6z;
    CPU_INT08U  t7;
    CPU_INT08U  t8;
    CPU_INT08U  t9;
    CPU_INT08U  tRD;
    CPU_INT08U  tA;
    CPU_INT16U  tB;
    CPU_INT08U  tC;
} FS_DEV_IDE_TIMING_PIO;

   /*  t0,    t1,   t2_16,   t2_8,    t2i,   t3,   t4,    t5,   t6,   t6z,    t7,    t8,    t9,  tRD,    tA,     tB,   tC */
const  FS_DEV_IDE_TIMING_PIO  FSDev_IDE_Timings_PIO[5] = {
    {600u,   70u,   165u,   290u,    0u,   60u,   30u,   50u,   5u,   30u,   90u,   60u,   20u,   0u,   35u,   1250u,   5u},
    {383u,   50u,   125u,   290u,    0u,   45u,   20u,   35u,   5u,   30u,   50u,   45u,   15u,   0u,   35u,   1250u,   5u},
    {240u,   30u,   100u,   290u,    0u,   30u,   15u,   20u,   5u,   30u,   30u,   30u,   10u,   0u,   35u,   1250u,   5u},
    {180u,   30u,    80u,    80u,   70u,   30u,   10u,   20u,   5u,   30u,    0u,    0u,   10u,   0u,   35u,   1250u,   5u},
    {120u,   25u,    70u,    70u,   25u,   20u,   10u,   20u,   5u,   30u,    0u,    0u,   10u,   0u,   35u,   1250u,   5u},
};

/*
*********************************************************************************************************
*                                        MULTIWORD DMA TIMINGS
*
* Note(s) : (1) True IDE mode Multiword DMA timings are given in "CF+ & CF SPECIFICATION REV. 4.1", Table
*               23. All timings are in nanoseconds.
*********************************************************************************************************
*/

typedef  struct  fs_dev_ide_timing_dma {
    CPU_INT16U  t0;
    CPU_INT08U  tD;
    CPU_INT08U  tE;
    CPU_INT08U  tF;
    CPU_INT08U  tG;
    CPU_INT08U  tH;
    CPU_INT08U  tI;
    CPU_INT08U  tJ;
    CPU_INT08U  tKR;
    CPU_INT08U  tKW;
    CPU_INT08U  tLR;
    CPU_INT08U  tLW;
    CPU_INT08U  tM;
    CPU_INT08U  tN;
    CPU_INT08U  tZ;
} FS_DEV_IDE_TIMING_DMA;

   /*  t0,     tD,     tE,   tF,     tG,    tH,   tI,    tJ,   tKR,    tKW,    tLR,   tLW,    tM,    tN,   tZ */
const  FS_DEV_IDE_TIMING_DMA  FSDev_IDE_Timings_DMA[3] = {
    {480u,   215u,   150u,   5u,   100u,   20u,   0u,   20u,   50u,   215u,   120u,   40u,   50u,   15u,  20u},
    {150u,    80u,    60u,   5u,    30u,   15u,   0u,    5u,   50u,    50u,    40u,   40u,   30u,   10u,  25u},
    {120u,    70u,    50u,   5u,    20u,   10u,   0u,    5u,   25u,    25u,    35u,   35u,   25u,   10u,  25u},
};


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
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_IDE_BSP_Open (FS_QTY  unit_nbr)
{
    if ((unit_nbr != 0u) &&
        (unit_nbr != 1u)) {
        return (DEF_FAIL);
    }

                                                                    /* -------------------- EN ATA -------------------- */
    IMX27_REG_PCCR0          |=   IMX27_BIT_PCCR0_GPIO_EN           /* En GPIO & DMA.                                   */
                              |   IMX27_BIT_PCCR0_DMA_EN;

    IMX27_REG_PCCR1          |=   IMX27_BIT_PCCR1_HCLK_ATA;         /* En ATA HCLK.                                     */

    IMX27_REG_CPLD_BCTRL2_CLR =   IMX27_BIT_CPLD_ATA_FEC_EN         /* En ATA data path in CPLD.                        */
                              |   IMX27_BIT_CPLD_ATA_FEC_SEL
                              |   IMX27_BIT_CPLD_ATA_EN;

    IMX27_REG_PTD_GIUS       &=  ~IMX27_GPIO_ATA_DATA_00_14;        /* En ATA data bus I/Os.                            */
    IMX27_REG_PTD_GPR        &=  ~IMX27_GPIO_ATA_DATA_00_14;
    IMX27_REG_PTD_PUEN       |=   IMX27_GPIO_ATA_DATA_00_14;
                                                                    /* En other I/Os.                                   */
    IMX27_REG_PTF_GIUS       &= ~(IMX27_GPIO_ATA_DIOR  | IMX27_GPIO_ATA_DIOW  | IMX27_GPIO_ATA_CS1     | IMX27_GPIO_ATA_CS0   | IMX27_GPIO_ATA_DA2   | IMX27_GPIO_ATA_DA1       | IMX27_GPIO_ATA_DA0    |
                                  IMX27_GPIO_ATA_DMARQ | IMX27_GPIO_ATA_DMACK | IMX27_GPIO_ATA_RESET_B | IMX27_GPIO_ATA_INTRQ | IMX27_GPIO_ATA_IORDY | IMX27_GPIO_ATA_BUFFER_EN | IMX27_GPIO_ATA_DATA_15);
    IMX27_REG_PTF_GPR        |=  (IMX27_GPIO_ATA_DIOR  | IMX27_GPIO_ATA_DIOW  | IMX27_GPIO_ATA_CS1     | IMX27_GPIO_ATA_CS0   | IMX27_GPIO_ATA_DA2   | IMX27_GPIO_ATA_DA1       | IMX27_GPIO_ATA_DA0    |
                                  IMX27_GPIO_ATA_DMARQ | IMX27_GPIO_ATA_DMACK | IMX27_GPIO_ATA_RESET_B | IMX27_GPIO_ATA_INTRQ | IMX27_GPIO_ATA_IORDY | IMX27_GPIO_ATA_BUFFER_EN );
    IMX27_REG_PTF_GPR        &= ~(IMX27_GPIO_ATA_DATA_15);
    IMX27_REG_PTF_PUEN       |=  (IMX27_GPIO_ATA_DIOR  | IMX27_GPIO_ATA_DIOW  | IMX27_GPIO_ATA_CS1     | IMX27_GPIO_ATA_CS0   | IMX27_GPIO_ATA_DA2   | IMX27_GPIO_ATA_DA1       | IMX27_GPIO_ATA_DA0    |
                                  IMX27_GPIO_ATA_DMARQ | IMX27_GPIO_ATA_DMACK | IMX27_GPIO_ATA_RESET_B | IMX27_GPIO_ATA_INTRQ | IMX27_GPIO_ATA_IORDY | IMX27_GPIO_ATA_BUFFER_EN | IMX27_GPIO_ATA_DATA_15);


                                                                    /* -------------------- CFG ATA ------------------- */
    IMX27_REG_ATA_CONTROL     =   IMX27_BIT_ATA_CONTROL_ATA_RST_B   /* Remove ATA from reset.                           */
                              |   IMX27_BIT_ATA_CONTROL_IORDY_EN;

    FSDev_IDE_BSP_SetMode(unit_nbr,                                 /* Cfg timing.                                      */
                          FS_DEV_IDE_MODE_TYPE_PIO,
                          0u);

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
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
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
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
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
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
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
    IMX27_REG_ATA_CONTROL &= ~IMX27_BIT_ATA_CONTROL_ATA_RST_B;

    FS_OS_Dly_ms(1u);

    IMX27_REG_ATA_CONTROL |=  IMX27_BIT_ATA_CONTROL_ATA_RST_B;

    FS_OS_Dly_ms(2u);
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
    CPU_INT08U  reg_val;


    switch (reg) {
        case FS_DEV_IDE_REG_ERR:
             reg_val = IMX27_REG_DFTR;
             break;

        case FS_DEV_IDE_REG_SC:
             reg_val = IMX27_REG_DSCR;
             break;

        case FS_DEV_IDE_REG_SN:
             reg_val = IMX27_REG_DSNR;
             break;

        case FS_DEV_IDE_REG_CYL:
             reg_val = IMX27_REG_DCLR;
             break;

        case FS_DEV_IDE_REG_CYH:
             reg_val = IMX27_REG_DCHR;
             break;

        case FS_DEV_IDE_REG_DH:
             reg_val = IMX27_REG_DDHR;
             break;

        case FS_DEV_IDE_REG_STATUS:
             reg_val = IMX27_REG_DCDR;
             break;

        default:
        case FS_DEV_IDE_REG_ALTSTATUS:
             reg_val = IMX27_REG_DCTR;
             break;
    }

    return (reg_val);
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
    switch (reg) {
        case FS_DEV_IDE_REG_FR:
             IMX27_REG_DFTR = val;
             break;

        case FS_DEV_IDE_REG_SC:
             IMX27_REG_DSCR = val;
             break;

        case FS_DEV_IDE_REG_SN:
             IMX27_REG_DSNR = val;
             break;

        case FS_DEV_IDE_REG_CYL:
             IMX27_REG_DCLR = val;
             break;

        case FS_DEV_IDE_REG_CYH:
             IMX27_REG_DCHR = val;
             break;

        case FS_DEV_IDE_REG_DH:
             IMX27_REG_DDHR = val;
             break;

        case FS_DEV_IDE_REG_CMD:
             IMX27_REG_DCDR = val;
             break;

        default:
        case FS_DEV_IDE_REG_ALTSTATUS:
             IMX27_REG_DCTR = val;
             break;
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
    IMX27_REG_DFTR = cmd[0];
    IMX27_REG_DSCR = cmd[1];
    IMX27_REG_DSNR = cmd[2];
    IMX27_REG_DCLR = cmd[3];
    IMX27_REG_DCHR = cmd[4];
    IMX27_REG_DDHR = cmd[5];
    IMX27_REG_DCDR = cmd[6];
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
    CPU_SIZE_T   cnt_words;
    CPU_INT16U  *p_dest_16;


    cnt_words = (cnt + 1u) / sizeof(CPU_INT16U);
    p_dest_16 = (CPU_INT16U *)p_dest;

    while (cnt_words > 0u) {
       *p_dest_16 = IMX27_REG_DDTR;
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
    CPU_SIZE_T   cnt_words;
    CPU_INT16U  *p_src_16;


    cnt_words = (cnt + 1u) / sizeof(CPU_INT16U);
    p_src_16  = (CPU_INT16U *)p_src;

    while (cnt_words > 0u) {
        IMX27_REG_DDTR = *p_src_16;
        p_src_16++;
        cnt_words--;
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
    IMX27_REG_DMAC_DISR    = DEF_BIT(7);                                    /* Clr all status reg bits.                 */
    IMX27_REG_DMAC_DIMR   |= DEF_BIT(7);
    IMX27_REG_DMAC_DBTOSR  = DEF_BIT(7);
    IMX27_REG_DMAC_DSESR   = DEF_BIT(7);
    IMX27_REG_DMAC_DRTOSR  = DEF_BIT(7);
    IMX27_REG_DMAC_DBOSR   = DEF_BIT(7);

    IMX27_REG_DMAC_CCR(7u)  = DEF_BIT_NONE;                                  /* Dis ch.                                  */

    IMX27_REG_ATA_CONTROL &= ~IMX27_BIT_ATA_CONTROL_FIFO_RST_B;             /* Reset FIFO.                              */
    IMX27_REG_ATA_CONTROL |=  IMX27_BIT_ATA_CONTROL_FIFO_RST_B;

    if (rd == DEF_YES) {
        IMX27_REG_DMAC_SAR(7u)  =  IMX27_ADDR_FIFO;                         /* Set ATA FIFO as src addr.                */
        IMX27_REG_DMAC_DAR(7u)  = (CPU_INT32U)p_data;                       /* Set data buf as dest addr.               */
        IMX27_REG_DMAC_BLR(7u)  =  32u;
        IMX27_REG_DMAC_CNTR(7u) =  cnt;                                     /* DMA Transfer Count                       */
        IMX27_REG_DMAC_RSSR(7u) =  IMX27_BIT_DMAC_RSSR_DMA_REQ_ATA_RCV_FIFO;/* Req source                               */
        IMX27_REG_DMAC_CCR(7u)  =  DEF_BIT_11 | DEF_BIT_03;                 /* Src = FIFO, dest = line, en DMA req sig. */
        IMX27_REG_DMAC_CCR(7u) |=  DEF_BIT_00;                              /* Start DMA Transmission                   */
        IMX27_REG_FIFO_ALARM    =  16u;
        IMX27_REG_ATA_CONTROL  |= IMX27_BIT_ATA_CONTROL_DMA_PENDING | IMX27_BIT_ATA_CONTROL_FIFO_RCV_EN;


    } else {
        IMX27_REG_DMAC_SAR(7u)  = (CPU_INT32U)p_data;                       /* Set data buf as src addr.                */
        IMX27_REG_DMAC_DAR(7u)  =  IMX27_ADDR_FIFO;                         /* Set ATA FIFO as dest addr.               */
        IMX27_REG_DMAC_BLR(7u)  =  32u;
        IMX27_REG_DMAC_CNTR(7u) =  cnt;                                     /* DMA Transfer Count                       */
        IMX27_REG_DMAC_RSSR(7u) =  IMX27_BIT_DMAC_RSSR_DMA_REQ_ATA_TX_FIFO; /* Req source                               */
        IMX27_REG_DMAC_CCR(7u)  =  DEF_BIT_13 | DEF_BIT_03;                 /* Dest = FIFO, src = line, en DMA req sig. */
        IMX27_REG_DMAC_CCR(7u) |=  DEF_BIT_00;                              /* Start DMA Transmission                   */
        IMX27_REG_FIFO_ALARM    =  48u;
        IMX27_REG_ATA_CONTROL  |= IMX27_BIT_ATA_CONTROL_DMA_PENDING | IMX27_BIT_ATA_CONTROL_FIFO_TX_EN | IMX27_BIT_ATA_CONTROL_DMA_WRITE;
    }
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
    CPU_INT32U   timeout;
    CPU_BOOLEAN  rdy;



    if (rd == DEF_YES) {                                        /* ---------------------- END RD ---------------------- */
                                                                /* Wait for rd to complete.                             */
        timeout = 0u;
        do {
            rdy = DEF_BIT_IS_SET(IMX27_REG_DMAC_DISR, DEF_BIT_07);
            timeout++;
        } while ((rdy == DEF_NO) && (timeout < 10000000uL));



    } else {                                                    /* ---------------------- END WR ---------------------- */
                                                                /* Wait for wr to complete.                             */
        timeout = 0u;
        do {
            rdy = DEF_BIT_IS_SET(IMX27_REG_DMAC_DISR, DEF_BIT_07);
            timeout++;
        } while ((rdy == DEF_NO) && (timeout < 10000000uL));

        FSDev_IDE_BSP_Dly400_ns(unit_nbr);                      /* !!!! Provide short delay.                            */
        FSDev_IDE_BSP_Dly400_ns(unit_nbr);
    }



                                                                /* ---------------------- DIS DMA --------------------- */
    IMX27_REG_ATA_CONTROL &= ~(IMX27_BIT_ATA_CONTROL_FIFO_TX_EN  |
                               IMX27_BIT_ATA_CONTROL_FIFO_RCV_EN |
                               IMX27_BIT_ATA_CONTROL_DMA_PENDING |
                               IMX27_BIT_ATA_CONTROL_DMA_WRITE   );
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


    drv_nbr = (unit_nbr == 0u) ? 0u : 1u;
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


    mode_types = FS_DEV_IDE_MODE_TYPE_PIO | FS_DEV_IDE_MODE_TYPE_DMA;
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
    CPU_INT32U  config;
    CPU_INT32U  clk_freq;
    CPU_INT32U  ns_per_clk;


    clk_freq   = IMX27_CLK_FREQ_ATA;
    ns_per_clk = 1000000000uL / clk_freq;


    switch (mode_type) {
        case FS_DEV_IDE_MODE_TYPE_DMA:                          /* ------------------ CFG DMA TIMINGS ----------------- */
             mode_sel = DEF_MAX(mode, FS_DEV_IDE_DMA_MAX_MODE);

             config                  =    IMX27_REG_TIME_CONFIG2;
             config                 &= ~((CPU_INT32U) (DEF_OCTET_MASK                                        ) << (3u * DEF_OCTET_NBR_BITS)
                                     |   (CPU_INT32U) (DEF_OCTET_MASK                                        ) << (2u * DEF_OCTET_NBR_BITS)
                                     |   (CPU_INT32U) (DEF_OCTET_MASK                                        ) << (1u * DEF_OCTET_NBR_BITS));
             IMX27_REG_TIME_CONFIG2  =    config
                                     |  ((CPU_INT32U)((FSDev_IDE_Timings_DMA[mode_sel].tD  / ns_per_clk) + 1u) << (3u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)((FSDev_IDE_Timings_DMA[mode_sel].tH  / ns_per_clk) + 1u) << (2u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)((FSDev_IDE_Timings_DMA[mode_sel].tM  / ns_per_clk) + 1u) << (1u * DEF_OCTET_NBR_BITS));

             config                  =    IMX27_REG_TIME_CONFIG3;
             config                 &= ~((CPU_INT32U) (DEF_OCTET_MASK                                        ) << (0u * DEF_OCTET_NBR_BITS));
             IMX27_REG_TIME_CONFIG3  =    config
                                     |   (CPU_INT32U)((FSDev_IDE_Timings_DMA[mode_sel].tKW / ns_per_clk) + 1u) << (0u * DEF_OCTET_NBR_BITS);
             break;



        default:                                                /* ------------------ CFG PIO TIMINGS ----------------- */
        case FS_DEV_IDE_MODE_TYPE_PIO:
             mode_sel = DEF_MAX(mode, FS_DEV_IDE_PIO_MAX_MODE);


             IMX27_REG_TIME_CONFIG0  =  ((CPU_INT32U)((FSDev_IDE_Timings_PIO[mode_sel].t2_08 / ns_per_clk) + 1u) << (3u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)((FSDev_IDE_Timings_PIO[mode_sel].t1    / ns_per_clk) + 1u) << (2u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)( 0x03u                                                   ) << (1u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)( 0x03u                                                   ) << (0u * DEF_OCTET_NBR_BITS));

             IMX27_REG_TIME_CONFIG1  =  ((CPU_INT32U)((FSDev_IDE_Timings_PIO[mode_sel].t4    / ns_per_clk) + 1u) << (3u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)((FSDev_IDE_Timings_PIO[mode_sel].tRD   / ns_per_clk) + 1u) << (2u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)((FSDev_IDE_Timings_PIO[mode_sel].tA    / ns_per_clk) + 1u) << (1u * DEF_OCTET_NBR_BITS))
                                     |  ((CPU_INT32U)((FSDev_IDE_Timings_PIO[mode_sel].t2_08 / ns_per_clk) + 1u) << (0u * DEF_OCTET_NBR_BITS));

             config                  =    IMX27_REG_TIME_CONFIG2;
             config                 &= ~((CPU_INT32U) (DEF_OCTET_MASK                                          ) << (0u * DEF_OCTET_NBR_BITS));
             IMX27_REG_TIME_CONFIG2  =    config
                                     |   (CPU_INT32U)((FSDev_IDE_Timings_PIO[mode_sel].t9    / ns_per_clk) + 1u) << (0u * DEF_OCTET_NBR_BITS);
             break;
    }

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


    for (i = 0u; i < 10u; i++) {
        ;
    }

    (void)&i;
}
