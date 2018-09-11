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
*                       ATMEL AT91SAM9XE ON THE AT91SAM9261-EK EVALUATION BOARD
*
* Filename      : fs_dev_sd_card_bsp.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (1) The AT91SAM9261 SD/MMC card interface is described in documentation from Atmel :
*
*                         AT91SAM9161.  6062M-ATARM-23-Mar-09.  "35: MultiMedia Card Interface (MCI)".
*
*                 (2) AT91SAM9_MCLK_FREQ should be set to the value of the AT91SAM9261's MCLK frequency.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_SD_CARD_BSP_MODULE
#include  <fs_dev_sd_card.h>
#include  <fs_os.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  AT91SAM9_MCLK_FREQ                         96000000u

#define  AT91SAM9_GPIO_MC_DA0                     DEF_BIT_00   /* PA0 (peripheral B).                                  */
#define  AT91SAM9_GPIO_MC_DA1                     DEF_BIT_04   /* PA4 (peripheral B).                                  */
#define  AT91SAM9_GPIO_MC_DA2                     DEF_BIT_05   /* PA5 (peripheral B).                                  */
#define  AT91SAM9_GPIO_MC_DA3                     DEF_BIT_06   /* PA6 (peripheral B).                                  */
#define  AT91SAM9_GPIO_MC_CDA                     DEF_BIT_01   /* PA1 (peripheral B).                                  */
#define  AT91SAM9_GPIO_MC_CK                      DEF_BIT_02   /* PA2 (peripheral B).                                  */

#define  AT91SAM9_INT_ID_MCI                               9u
#define  AT91SAM9_PERIPH_ID_MCI                            9u

#define  AT91SAM9_PERIPH_ID_PIOA                           2u

#define  AT91SAM9_WAIT_TIMEOUT_ms                       1000u

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  AT91SAM9_REG_PIOA_BASE                             0xFFFFF400u
#define  AT91SAM9_REG_PIOA_PDR              (*(CPU_REG32 *)(AT91SAM9_REG_PIOA_BASE + 0x004u))
#define  AT91SAM9_REG_PIOA_MDDR             (*(CPU_REG32 *)(AT91SAM9_REG_PIOA_BASE + 0x054u))
#define  AT91SAM9_REG_PIOA_PPUDR            (*(CPU_REG32 *)(AT91SAM9_REG_PIOA_BASE + 0x060u))
#define  AT91SAM9_REG_PIOA_ASR              (*(CPU_REG32 *)(AT91SAM9_REG_PIOA_BASE + 0x070u))
#define  AT91SAM9_REG_PIOA_BSR              (*(CPU_REG32 *)(AT91SAM9_REG_PIOA_BASE + 0x074u))

#define  AT91SAM9_REG_PMC_BASE                              0xFFFFFC00u
#define  AT91SAM9_REG_PMC_PCER              (*(CPU_REG32 *)(AT91SAM9_REG_PMC_BASE  + 0x010u))
#define  AT91SAM9_REG_PMC_PCDR              (*(CPU_REG32 *)(AT91SAM9_REG_PMC_BASE  + 0x014u))

#define  AT91SAM9_REG_AIC_BASE                              0xFFFFF000u
#define  AT91SAM9_REG_AIC_SMR(n)            (*(CPU_REG32 *)(AT91SAM9_REG_AIC_BASE  + 0x000u + (n) * 4u))
#define  AT91SAM9_REG_AIC_SVR(n)            (*(CPU_REG32 *)(AT91SAM9_REG_AIC_BASE  + 0x080u + (n) * 4u))
#define  AT91SAM9_REG_AIC_IVR               (*(CPU_REG32 *)(AT91SAM9_REG_AIC_BASE  + 0x100u))
#define  AT91SAM9_REG_AIC_IECR              (*(CPU_REG32 *)(AT91SAM9_REG_AIC_BASE  + 0x120u))
#define  AT91SAM9_REG_AIC_IDCR              (*(CPU_REG32 *)(AT91SAM9_REG_AIC_BASE  + 0x124u))
#define  AT91SAM9_REG_AIC_ICCR              (*(CPU_REG32 *)(AT91SAM9_REG_AIC_BASE  + 0x128u))

#define  AT91SAM9_REG_MCI_BASE                               0xFFFA8000u

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

                                                                /* ---------------------- MCI CR ---------------------- */
#define  AT91SAM9_BIT_MCI_CR_MCIEN                DEF_BIT_00    /* Multi-Media Interface Enable.                        */
#define  AT91SAM9_BIT_MCI_CR_MCIDIS               DEF_BIT_01    /* Multi-Media Interface Disable.                       */
#define  AT91SAM9_BIT_MCI_CR_PWSEN                DEF_BIT_02    /* Power Save Mode Enable.                              */
#define  AT91SAM9_BIT_MCI_CR_PWSDIS               DEF_BIT_03    /* Power Save Mode Disable.                             */
#define  AT91SAM9_BIT_MCI_CR_SWRST                DEF_BIT_07    /* Software Reset.                                      */


                                                                /* ---------------------- MCI MR ---------------------- */
#define  AT91SAM9_BIT_MCI_MR_CLKDIV_MASK          0x000000FFu   /* Clock Divider.                                       */
#define  AT91SAM9_BIT_MCI_MR_PWSDIV_MASK          0x00000700u   /* Pwer Saving Divider.                                 */
#define  AT91SAM9_BIT_MCI_MR_RDPROOF              DEF_BIT_11    /* Read Proof Enable.                                   */
#define  AT91SAM9_BIT_MCI_MR_WRPROOF              DEF_BIT_12    /* Write Proof Enable.                                  */
#define  AT91SAM9_BIT_MCI_MR_PDCFBYTE             DEF_BIT_13    /* PDC Force Byte Transfer.                             */
#define  AT91SAM9_BIT_MCI_MR_PDCPADV              DEF_BIT_14    /* PDC Padding Value.                                   */
#define  AT91SAM9_BIT_MCI_MR_PDCMODE              DEF_BIT_15    /* PDC-oriented Mode.                                   */
#define  AT91SAM9_BIT_MCI_MR_BLKLEN_MASK          0xFFFF0000u   /* Data Block Length.                                   */


                                                                /* --------------------- MCI DTOR --------------------- */
#define  AT91SAM9_BIT_MCI_DTOR_DTOCYC_MASK        0x0000000Fu   /* Data Timeout Cycle Number.                           */

#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_MASK        0x00000070u   /* Data Timeout Multiplier.                             */
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_1           0x00000000u
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_16          0x00000010u
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_128         0x00000020u
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_256         0x00000030u
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_1024        0x00000040u
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_4096        0x00000050u
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_65536       0x00000060u
#define  AT91SAM9_BIT_MCI_DTOR_DTOMUL_1048576     0x00000070u


                                                                /* --------------------- MCI SDCR --------------------- */
#define  AT91SAM9_BIT_MCI_SDCR_SDCSEL_MASK        0x00000003u   /* SDCard/SDIO Slot.                                    */
#define  AT91SAM9_BIT_MCI_SDCR_SCSEL_A            0x00000000u
#define  AT91SAM9_BIT_MCI_SDCR_SCSEL_B            0x00000001u
#define  AT91SAM9_BIT_MCI_SDCR_SDCBUS             DEF_BIT_07    /* SDCard/SDIO Bus Width.                               */


                                                                /* --------------------- MCI CMDR --------------------- */
#define  AT91SAM9_BIT_MCI_CMDR_CMDNB_MASK         0x0000003Fu   /* Command Number.                                      */

#define  AT91SAM9_BIT_MCI_CMDR_RSPTYP_MASK        0x000000C0u   /* Response Type.                                       */
#define  AT91SAM9_BIT_MCI_CMDR_RSPTYP_NONE        0x00000000u   /*   No response.                                       */
#define  AT91SAM9_BIT_MCI_CMDR_RSPTYP_48          0x00000040u   /*   48-bit response.                                   */
#define  AT91SAM9_BIT_MCI_CMDR_RSPTYP_136         0x00000080u   /*   136-bit response.                                  */

#define  AT91SAM9_BIT_MCI_CMDR_SPCMD_MASK         0x00000700u   /* Special Command.                                     */
#define  AT91SAM9_BIT_MCI_CMDR_SPCMD_NOT          0x00000000u   /*   Not a special CMD.                                 */
#define  AT91SAM9_BIT_MCI_CMDR_SPCMD_INIT         0x00000100u   /*   Initialization CMD.                                */
#define  AT91SAM9_BIT_MCI_CMDR_SPCMD_SYNC         0x00000200u   /*   Synchronized CMD.                                  */
#define  AT91SAM9_BIT_MCI_CMDR_SPCMD_INTCMD       0x00000400u   /*   Interrupt command.                                 */
#define  AT91SAM9_BIT_MCI_CMDR_SPCMD_INTRESP      0x00000500u   /*   Interrupt response.                                */

#define  AT91SAM9_BIT_MCI_CMDR_OPDCMD             DEF_BIT_11    /* Open Drain Command.                                  */
#define  AT91SAM9_BIT_MCI_CMDR_MAXLAT             DEF_BIT_12    /* Max Latency for Command to Response (1 = 64-cycle).  */

#define  AT91SAM9_BIT_MCI_CMDR_TRCMD_MASK         0x00030000u   /* Transfer Command.                                    */
#define  AT91SAM9_BIT_MCI_CMDR_TRCMD_NONE         0x00000000u   /*   No data transfer.                                  */
#define  AT91SAM9_BIT_MCI_CMDR_TRCMD_START        0x00010000u   /*   Start data transfer.                               */
#define  AT91SAM9_BIT_MCI_CMDR_TRCMD_STOP         0x00020000u   /*   Stop data transfer.                                */

#define  AT91SAM9_BIT_MCI_CMDR_TRDIR              DEF_BIT_18    /* Transfer Direction (1 = Read).                       */

#define  AT91SAM9_BIT_MCI_CMDR_TRTYP_MASK         0x00380000u   /* Transfer Type.                                       */
#define  AT91SAM9_BIT_MCI_CMDR_TRTYP_SINGLEBLK    0x00000000u   /*   MMC/SDCard Single Block.                           */
#define  AT91SAM9_BIT_MCI_CMDR_TRTYP_MULTBLK      0x00080000u   /*   MMC/SDCard Multiple Block.                         */
#define  AT91SAM9_BIT_MCI_CMDR_TRTYP_STREAM       0x00100000u   /*   MMC Stream.                                        */
#define  AT91SAM9_BIT_MCI_CMDR_TRTYP_SDIOBYTE     0x00200000u   /*   SDIO Byte.                                         */
#define  AT91SAM9_BIT_MCI_CMDR_TRTYP_SDIOBLK      0x00280000u   /*   SDIO Block.                                        */

#define  AT91SAM9_BIT_MCI_CMDR_IOSPCMD_MASK       0x03000000u   /* SDIO Special Command.                                */
#define  AT91SAM9_BIT_MCI_CMDR_IOSPCMD_NOT        0x00000000u   /*   Not a SDIO Special Command.                        */
#define  AT91SAM9_BIT_MCI_CMDR_IOSPCMD_SUSPEND    0x01000000u   /*   SDIO Suspend Command.                              */
#define  AT91SAM9_BIT_MCI_CMDR_IOSPCMD_RESUME     0x02000000u   /*   SDIO Resume  Command.                              */


                                                                /* --------------------- MCI BLKR --------------------- */
#define  AT91SAM9_BIT_MCI_BLKR_BCNT_MASK          0x0000FFFFu   /* MMC/SDIO Block Count - SDIO Byte Count.              */
#define  AT91SAM9_BIT_MCI_BLKR_BLKLEN_MASK        0xFFFF0000u   /* Data Block Length.                                   */


                                                                /* -------------- MCI SR / IER / IDR / IMR ------------ */
#define  AT91SAM9_BIT_MCI_INT_CMDRDY              DEF_BIT_00    /* Command Ready Interrupt Enable.                      */
#define  AT91SAM9_BIT_MCI_INT_RXRDY               DEF_BIT_01    /* Receiver Ready Interrupt Enable.                     */
#define  AT91SAM9_BIT_MCI_INT_TXRDY               DEF_BIT_02    /* Transmit Ready Interrupt Enable.                     */
#define  AT91SAM9_BIT_MCI_INT_BLKE                DEF_BIT_03    /* Data Block Ended Interrupt Enable.                   */
#define  AT91SAM9_BIT_MCI_INT_DTIP                DEF_BIT_04    /* Data Transfer In Progress Interrupt Enable.          */
#define  AT91SAM9_BIT_MCI_INT_NOTBUSY             DEF_BIT_05    /* MCI Not Busy Interrupt Enable.                       */
#define  AT91SAM9_BIT_MCI_INT_ENDRX               DEF_BIT_06    /* End of RX Buffer Interrupt Enable.                   */
#define  AT91SAM9_BIT_MCI_INT_ENDTX               DEF_BIT_07    /* End of TX Buffer Interrupt Enable.                   */
#define  AT91SAM9_BIT_MCI_INT_SDIOIRQA            DEF_BIT_08    /* SDIO Interrupt for Slot A Interrupt Enable.          */
#define  AT91SAM9_BIT_MCI_INT_SDIOIRQB            DEF_BIT_09    /* SDIO Interrupt for Slot B Interrupt Enable.          */
#define  AT91SAM9_BIT_MCI_INT_RXBUFF              DEF_BIT_14    /* RX Buffer Full Interrupt Enable.                     */
#define  AT91SAM9_BIT_MCI_INT_TXBUFE              DEF_BIT_15    /* TX Buffer Empty Interrupt Enable.                    */
#define  AT91SAM9_BIT_MCI_INT_RINDE               DEF_BIT_16    /* Response Index Error Interrupt Enable.               */
#define  AT91SAM9_BIT_MCI_INT_RDIRE               DEF_BIT_17    /* Response Direction Error Interrupt Enable.           */
#define  AT91SAM9_BIT_MCI_INT_RCRCE               DEF_BIT_18    /* Response CRC Error Interrupt Enable.                 */
#define  AT91SAM9_BIT_MCI_INT_RENDE               DEF_BIT_19    /* Response End Bit Error Interrupt Enable.             */
#define  AT91SAM9_BIT_MCI_INT_RTOE                DEF_BIT_20    /* Response Time-out Error Interrupt Enable.            */
#define  AT91SAM9_BIT_MCI_INT_DCRCE               DEF_BIT_21    /* Data CRC Error Interrupt Enable.                     */
#define  AT91SAM9_BIT_MCI_INT_DTOE                DEF_BIT_22    /* Data Time-out Error Interrupt Enable.                */
#define  AT91SAM9_BIT_MCI_INT_OVRE                DEF_BIT_30    /* Overrun Interrupt Enable.                            */
#define  AT91SAM9_BIT_MCI_INT_UNRE                DEF_BIT_31    /* Underrun Interrupt Enable.                           */
#define  AT91SAM9_BIT_MCI_INT_ALL                (AT91SAM9_BIT_MCI_INT_CMDRDY   | AT91SAM9_BIT_MCI_INT_RXRDY  | AT91SAM9_BIT_MCI_INT_TXRDY    | \
                                                  AT91SAM9_BIT_MCI_INT_BLKE     | AT91SAM9_BIT_MCI_INT_DTIP   | AT91SAM9_BIT_MCI_INT_NOTBUSY  | \
                                                  AT91SAM9_BIT_MCI_INT_ENDRX    | AT91SAM9_BIT_MCI_INT_ENDTX  | AT91SAM9_BIT_MCI_INT_SDIOIRQA | \
                                                  AT91SAM9_BIT_MCI_INT_SDIOIRQB | AT91SAM9_BIT_MCI_INT_RXBUFF | AT91SAM9_BIT_MCI_INT_TXBUFE   | \
                                                  AT91SAM9_BIT_MCI_INT_RINDE    | AT91SAM9_BIT_MCI_INT_RDIRE  | AT91SAM9_BIT_MCI_INT_RCRCE    | \
                                                  AT91SAM9_BIT_MCI_INT_RENDE    | AT91SAM9_BIT_MCI_INT_RTOE   | AT91SAM9_BIT_MCI_INT_DCRCE    | \
                                                  AT91SAM9_BIT_MCI_INT_DTOE     | AT91SAM9_BIT_MCI_INT_OVRE   | AT91SAM9_BIT_MCI_INT_UNRE)
#define  AT91SAM9_BIT_MCI_INT_ERR                (AT91SAM9_BIT_MCI_INT_RINDE    | AT91SAM9_BIT_MCI_INT_RDIRE  | AT91SAM9_BIT_MCI_INT_RCRCE    | \
                                                  AT91SAM9_BIT_MCI_INT_RENDE    | AT91SAM9_BIT_MCI_INT_RTOE   | AT91SAM9_BIT_MCI_INT_DCRCE    | \
                                                  AT91SAM9_BIT_MCI_INT_DTOE     | AT91SAM9_BIT_MCI_INT_OVRE   | AT91SAM9_BIT_MCI_INT_UNRE)


                                                                /* --------------------- MCI PTCR --------------------- */
#define  AT91SAM9_BIT_MCI_PTCR_RXTEN              DEF_BIT_00    /* Receiver Transfer Enable.                            */
#define  AT91SAM9_BIT_MCI_PTCR_RXTDIS             DEF_BIT_01    /* Receiver Transfer Disable.                           */
#define  AT91SAM9_BIT_MCI_PTCR_TXTEN              DEF_BIT_08    /* Transmitter Transfer Enable.                         */
#define  AT91SAM9_BIT_MCI_PTCR_TXTDIS             DEF_BIT_09    /* Transmitter Transfer Disable.                        */


                                                                /* --------------------- MCI PTSR --------------------- */
#define  AT91SAM9_BIT_MCI_PTSR_RXTEN              DEF_BIT_00    /* Receiver Transfer Enable.                            */
#define  AT91SAM9_BIT_MCI_PTSR_TXTEN              DEF_BIT_08    /* Transmitter Transfer Enable.                         */


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

typedef  struct at91sam9_struct_mci {
    CPU_REG32  MCI_CR;                                          /* MCI Control Register                                 */
    CPU_REG32  MCI_MR;                                          /* MCI Mode Register                                    */
    CPU_REG32  MCI_DTOR;                                        /* MCI Data Timeout Register                            */
    CPU_REG32  MCI_SDCR;                                        /* MCI SD Card Register                                 */
    CPU_REG32  MCI_ARGR;                                        /* MCI Argument Register                                */
    CPU_REG32  MCI_CMDR;                                        /* MCI Command Register                                 */
    CPU_REG32  MCI_BLKR;                                        /* MCI Block Register                                   */
    CPU_REG32  Reserved0[1];
    CPU_REG32  MCI_RSPR[4];                                     /* MCI Response Register                                */
    CPU_REG32  MCI_RDR;                                         /* MCI Receive Data Register                            */
    CPU_REG32  MCI_TDR;                                         /* MCI Transmit Data Register                           */
    CPU_REG32  Reserved1[2];
    CPU_REG32  MCI_SR;                                          /* MCI Status Register                                  */
    CPU_REG32  MCI_IER;                                         /* MCI Interrupt Enable Register                        */
    CPU_REG32  MCI_IDR;                                         /* MCI Interrupt Disable Register                       */
    CPU_REG32  MCI_IMR;                                         /* MCI Interrupt Mask Register                          */
    CPU_REG32  Reserved2[43];
    CPU_REG32  MCI_VR;                                          /* MCI Version Register                                 */
    CPU_REG32  MCI_RPR;                                         /* Receive Pointer Register                             */
    CPU_REG32  MCI_RCR;                                         /* Receive Counter Register                             */
    CPU_REG32  MCI_TPR;                                         /* Transmit Pointer Register                            */
    CPU_REG32  MCI_TCR;                                         /* Transmit Counter Register                            */
    CPU_REG32  MCI_RNPR;                                        /* Receive Next Pointer Register                        */
    CPU_REG32  MCI_RNCR;                                        /* Receive Next Counter Register                        */
    CPU_REG32  MCI_TNPR;                                        /* Transmit Next Pointer Register                       */
    CPU_REG32  MCI_TNCR;                                        /* Transmit Next Counter Register                       */
    CPU_REG32  MCI_PTCR;                                        /* PDC Transfer Control Register                        */
    CPU_REG32  MCI_PTSR;                                        /* PDC Transfer Status Register                         */
} AT91SAM9_STRUCT_MCI;


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
*               (2) A SD/MMC slot is connected to the AT91SAM9261 using the following pins :
*
*                   ------------------------------------------------------
*                   | AT91SAM9261 NAME | AT91SAM9261 PIO # | SD/MMC NAME |
*                   ------------------------------------------------------
*                   |      MC_DA0      |        PA0        |     DAT0    |
*                   |      MC_DA1      |        PA4        |     DAT1    |
*                   |      MC_DA2      |        PA5        |     DAT2    |
*                   |      MC_DA3      |        PA6        |     DAT3    |
*                   |      MC_CDA      |        PA1        |     CMD     |
*                   |      MC_CK       |        PA2        |     CLK     |
*                   ------------------------------------------------------
*
*                   (a) The jumper which connects PA3 to either the on-board AT45 DataFlash or the SD/MMC
*                       CLK pin must NOT be configure to connect PA3 to SD/MMC CLK pin; it must be connect
*                       the other pins or be unfitted.
*********************************************************************************************************
*/

CPU_BOOLEAN  FSDev_SD_Card_BSP_Open (FS_QTY  unit_nbr)
{
    AT91SAM9_STRUCT_MCI  *p_mci;
    CPU_INT32U            mr;
    CPU_BOOLEAN           ok;


    if (unit_nbr != 0u) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Open(): Invalid unit nbr: %d.\r\n", unit_nbr));
        return (DEF_FAIL);
    }

    ok = FS_OS_SemCreate(&FSDev_SD_Card_OS_Sem, 0u);
    if (ok == DEF_FAIL) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Open(): Could not create sem.\r\n"));
        return (DEF_FAIL);
    }
    p_mci = (AT91SAM9_STRUCT_MCI *)AT91SAM9_REG_MCI_BASE;



                                                                                    /* ------------ EN MCI ------------ */
                                                                                    /* Cfg GPIO.                        */
    AT91SAM9_REG_PMC_PCER   = DEF_BIT(AT91SAM9_PERIPH_ID_PIOA);

    AT91SAM9_REG_PIOA_PDR   = AT91SAM9_GPIO_MC_DA0 | AT91SAM9_GPIO_MC_DA1 | AT91SAM9_GPIO_MC_DA2 | AT91SAM9_GPIO_MC_DA3 | AT91SAM9_GPIO_MC_CDA | AT91SAM9_GPIO_MC_CK;
    AT91SAM9_REG_PIOA_BSR   = AT91SAM9_GPIO_MC_DA0 | AT91SAM9_GPIO_MC_DA1 | AT91SAM9_GPIO_MC_DA2 | AT91SAM9_GPIO_MC_DA3 | AT91SAM9_GPIO_MC_CDA | AT91SAM9_GPIO_MC_CK;
    AT91SAM9_REG_PIOA_PPUDR = AT91SAM9_GPIO_MC_DA0 | AT91SAM9_GPIO_MC_DA1 | AT91SAM9_GPIO_MC_DA2 | AT91SAM9_GPIO_MC_DA3 | AT91SAM9_GPIO_MC_CDA | AT91SAM9_GPIO_MC_CK;
    AT91SAM9_REG_PIOA_MDDR  = AT91SAM9_GPIO_MC_DA0 | AT91SAM9_GPIO_MC_DA1 | AT91SAM9_GPIO_MC_DA2 | AT91SAM9_GPIO_MC_DA3 | AT91SAM9_GPIO_MC_CDA | AT91SAM9_GPIO_MC_CK;

    AT91SAM9_REG_PMC_PCER   = DEF_BIT(AT91SAM9_PERIPH_ID_MCI);                      /* En periph clk.                   */
    AT91SAM9_REG_AIC_IDCR   = DEF_BIT(AT91SAM9_INT_ID_MCI);



    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_SWRST;                                    /* Reset MCI.                       */
    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_MCIDIS | AT91SAM9_BIT_MCI_CR_PWSDIS;      /* Dis MCI.                         */

    p_mci->MCI_IDR  = AT91SAM9_BIT_MCI_INT_ALL;                                     /* Dis all ints.                    */

    p_mci->MCI_DTOR = 0x0Fu | AT91SAM9_BIT_MCI_DTOR_DTOMUL_1048576;                 /* Set max DTOR.                    */

    FSDev_SD_Card_BSP_SetClkFreq(unit_nbr, FS_DEV_SD_DFLT_CLK_SPD);                 /* Init clk freq.                   */

    p_mci->MCI_SDCR = AT91SAM9_BIT_MCI_SDCR_SCSEL_A;                                /* Set init mode.                   */

    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_MCIEN;                                    /* En MCI.                          */

    AT91SAM9_REG_PMC_PCDR = DEF_BIT(AT91SAM9_PERIPH_ID_MCI);                        /* Dis periph clk.                  */


                                                                                    /* ------------ CFG INT ----------- */
                                                                                    /* Set int handler.                 */
    AT91SAM9_REG_AIC_SVR(AT91SAM9_INT_ID_MCI) = (CPU_INT32U)FSDev_SD_Card_BSP_ISR_Handler;
    AT91SAM9_REG_AIC_SMR(AT91SAM9_INT_ID_MCI) =  0u;
    AT91SAM9_REG_AIC_IECR                     =  DEF_BIT(AT91SAM9_INT_ID_MCI);


                                                                                    /* ---------- START CARD ---------- */
    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_MCIDIS;                                   /* Dis MCI.                         */
    mr              = p_mci->MCI_MR;                                                /* Cfg MR.                          */
    mr             &= ~(AT91SAM9_BIT_MCI_MR_WRPROOF | AT91SAM9_BIT_MCI_MR_RDPROOF | AT91SAM9_BIT_MCI_MR_BLKLEN_MASK | AT91SAM9_BIT_MCI_MR_PDCMODE);
    p_mci->MCI_MR   = mr;
    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_MCIEN;                                    /* En MCI.                          */
    p_mci->MCI_ARGR = 0u;                                                           /* Send cmd.                        */
    p_mci->MCI_CMDR = AT91SAM9_BIT_MCI_CMDR_SPCMD_INIT | AT91SAM9_BIT_MCI_CMDR_OPDCMD;
    p_mci->MCI_IER  = AT91SAM9_BIT_MCI_INT_CMDRDY | AT91SAM9_BIT_MCI_INT_ERR;       /* En ints.                         */

   (void)FS_OS_SemPend(&FSDev_SD_Card_OS_Sem, AT91SAM9_WAIT_TIMEOUT_ms);          /* Wait for completion.             */

    p_mci->MCI_IDR  = AT91SAM9_BIT_MCI_INT_ALL;
    p_mci->MCI_BLKR = 0u;
    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_MCIDIS;
    p_mci->MCI_CMDR = 0u;

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


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    ok = FS_OS_SemDel(&FSDev_SD_Card_OS_Sem);                 /* Del sem.                                             */
    if (ok == DEF_FAIL) {
        FS_TRACE_INFO(("FSDev_SD_Card_BSP_Close(): Could not del sem.\r\n"));
    }
    AT91SAM9_REG_AIC_IDCR = DEF_BIT(AT91SAM9_INT_ID_MCI);       /* Dis MCI int.                                         */
    AT91SAM9_REG_PMC_PCDR = DEF_BIT(AT91SAM9_PERIPH_ID_MCI);    /* Dis periph clk.                                      */
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
    AT91SAM9_STRUCT_MCI  *p_mci;
    CPU_INT32U            mr;
    CPU_INT32U            ier;
    CPU_INT32U            cmdr;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    p_mci = (AT91SAM9_STRUCT_MCI *)AT91SAM9_REG_MCI_BASE;
    AT91SAM9_REG_PMC_PCER = DEF_BIT(AT91SAM9_PERIPH_ID_MCI);    /* En periph clk.                                       */

    p_mci->MCI_CR = AT91SAM9_BIT_MCI_CR_MCIDIS;                 /* Dis MCI.                                             */


                                                                /* --------------------- SET UP PDC ------------------- */
    switch (p_cmd->DataDir) {                                   /* En ex'er or tx'er.                                   */
        case FS_DEV_SD_CARD_DATA_DIR_CARD_TO_HOST:
             p_mci->MCI_PTCR = AT91SAM9_BIT_MCI_PTCR_RXTEN;
             break;

        case FS_DEV_SD_CARD_DATA_DIR_HOST_TO_CARD:
             p_mci->MCI_PTCR = AT91SAM9_BIT_MCI_PTCR_TXTEN;
             break;

        default:
        case FS_DEV_SD_CARD_DATA_DIR_NONE:
             break;
    }

                                                                /* Dis tx'er & rx'er.                                   */
    p_mci->MCI_PTCR = AT91SAM9_BIT_MCI_PTCR_RXTDIS | AT91SAM9_BIT_MCI_PTCR_TXTDIS;

    mr              = p_mci->MCI_MR;                            /* Cfg MR.                                              */
    mr             &= ~(AT91SAM9_BIT_MCI_MR_WRPROOF | AT91SAM9_BIT_MCI_MR_RDPROOF | AT91SAM9_BIT_MCI_MR_BLKLEN_MASK | AT91SAM9_BIT_MCI_MR_PDCMODE);

    switch (p_cmd->DataDir) {                                   /* Set up PDC.                                          */
        case FS_DEV_SD_CARD_DATA_DIR_CARD_TO_HOST:
             p_mci->MCI_MR   = mr | AT91SAM9_BIT_MCI_MR_WRPROOF | AT91SAM9_BIT_MCI_MR_RDPROOF | AT91SAM9_BIT_MCI_MR_PDCMODE | (p_cmd->BlkSize << 16);
             p_mci->MCI_RPR  = (CPU_INT32U)p_data;
             p_mci->MCI_RCR  = (p_cmd->BlkSize * p_cmd->BlkCnt) / 4u;
             p_mci->MCI_PTCR = AT91SAM9_BIT_MCI_PTCR_RXTEN;
             p_mci->MCI_BLKR = (p_cmd->BlkSize << 16) | p_cmd->BlkCnt;
             ier             = AT91SAM9_BIT_MCI_INT_ENDRX | AT91SAM9_BIT_MCI_INT_ERR;
             break;

        case FS_DEV_SD_CARD_DATA_DIR_HOST_TO_CARD:
             p_mci->MCI_MR   = mr | AT91SAM9_BIT_MCI_MR_WRPROOF | AT91SAM9_BIT_MCI_MR_RDPROOF | AT91SAM9_BIT_MCI_MR_PDCMODE | (p_cmd->BlkSize << 16);
             p_mci->MCI_TPR  = (CPU_INT32U)p_data;
             p_mci->MCI_TCR  = (p_cmd->BlkSize * p_cmd->BlkCnt) / 4u;
             p_mci->MCI_BLKR = (p_cmd->BlkSize << 16) | p_cmd->BlkCnt;
             ier             = AT91SAM9_BIT_MCI_INT_BLKE | AT91SAM9_BIT_MCI_INT_ERR;
             break;

        default:
        case FS_DEV_SD_CARD_DATA_DIR_NONE:
             p_mci->MCI_MR   = mr;
             ier             = AT91SAM9_BIT_MCI_INT_CMDRDY | AT91SAM9_BIT_MCI_INT_ERR;
             break;
    }



                                                                /* ----------------------- WR CMD --------------------- */
    cmdr = (CPU_INT32U)p_cmd->Cmd | AT91SAM9_BIT_MCI_CMDR_MAXLAT;
                                                                /* Set resp type.                                       */
    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_RSPTYP_136;
    } else if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_RSPTYP_48;
    }
                                                                /* Set transfer cmd type.                               */
    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_DATA_START) == DEF_YES) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_TRCMD_START;
    } else if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_DATA_STOP) == DEF_YES) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_TRCMD_STOP;
    }

    if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_OPEN_DRAIN) == DEF_YES) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_OPDCMD;
    }
                                                                /* Set data type.                                       */
    if (p_cmd->DataType == FS_DEV_SD_CARD_DATA_TYPE_MULTI_BLOCK) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_TRTYP_MULTBLK;
    } else if (p_cmd->DataType == FS_DEV_SD_CARD_DATA_TYPE_STREAM) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_TRTYP_STREAM;
    }
                                                                /* Set data dir.                                        */
    if (p_cmd->DataDir == FS_DEV_SD_CARD_DATA_DIR_CARD_TO_HOST) {
        cmdr |= AT91SAM9_BIT_MCI_CMDR_TRDIR;
    }

    if (DEF_BIT_IS_CLR(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_CRC_VALID) == DEF_YES) {
        ier &= ~AT91SAM9_BIT_MCI_INT_RCRCE;
    }
    if (DEF_BIT_IS_CLR(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_IX_VALID) == DEF_YES) {
        ier &= ~AT91SAM9_BIT_MCI_INT_RINDE;
    }

    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_MCIEN;                /* En MCI.                                              */
    p_mci->MCI_ARGR = p_cmd->Arg;                               /* Send cmd.                                            */
    p_mci->MCI_CMDR = cmdr;

                                                                /* If wr to card ...                                    */
    if (p_cmd->DataDir == FS_DEV_SD_CARD_DATA_DIR_HOST_TO_CARD) {
        p_mci->MCI_PTCR = AT91SAM9_BIT_MCI_PTCR_TXTEN;          /* ... en PDC after cmd.                                */
    }

    p_mci->MCI_IER  = ier;                                      /* En ints.                                             */

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
    AT91SAM9_STRUCT_MCI   *p_mci;
    CPU_BOOLEAN            ok;
    CPU_BOOLEAN            not_busy;
    volatile  CPU_INT32U   timeout;


                                                                /* ------------------- WAIT CMD END ------------------- */
    p_mci = (AT91SAM9_STRUCT_MCI *)AT91SAM9_REG_MCI_BASE;
                                                                /* Wait for completion.                                 */
    ok    = FS_OS_SemPend(&FSDev_SD_Card_OS_Sem, AT91SAM9_WAIT_TIMEOUT_ms);

    timeout = 150u;
    while (timeout > 0u) {
        timeout--;
    }

    p_mci->MCI_IDR = p_mci->MCI_IMR;



                                                                /* ---------------------- RD RESP --------------------- */
    if (ok == DEF_OK) {
       *p_err = FSDev_SD_Card_BSP_Err;
                                                                /* If stop cmd ...                                      */
        if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_DATA_STOP) == DEF_YES) {
            timeout = 10000u;
            do {                                                /* ... wait until card not busy.                        */
                not_busy = DEF_BIT_IS_SET(p_mci->MCI_SR, AT91SAM9_BIT_MCI_INT_NOTBUSY);
                timeout--;
            } while ((timeout  >  0u) &&
                     (not_busy == DEF_NO));
            if (not_busy == DEF_NO) {                           /* If still busy ...                                    */
               *p_err = FS_DEV_SD_CARD_ERR_DATA;                /* ... rtn err.                                         */
            }
        }

        if (*p_err == FS_DEV_SD_CARD_ERR_NONE) {                /* If no err, rd resp.                                  */
            if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP_LONG) == DEF_YES) {
               *p_resp = p_mci->MCI_RSPR[0];
                p_resp++;
               *p_resp = p_mci->MCI_RSPR[1];
                p_resp++;
               *p_resp = p_mci->MCI_RSPR[2];
                p_resp++;
               *p_resp = p_mci->MCI_RSPR[3];
            } else if (DEF_BIT_IS_SET(p_cmd->Flags, FS_DEV_SD_CARD_CMD_FLAG_RESP) == DEF_YES) {
               *p_resp = p_mci->MCI_RSPR[0];
            }
        }
    } else {
       *p_err = FS_DEV_SD_CARD_ERR_WAIT_TIMEOUT;
    }

    p_mci->MCI_BLKR = 0u;                                       /* Reset regs.                                          */
    p_mci->MCI_CR   = AT91SAM9_BIT_MCI_CR_MCIDIS;
    p_mci->MCI_CMDR = 0u;
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
* Note(s)     : (1) The data transfer will have completed prior to the return of 'FSDev_SD_CardBSP_CmdWaitEnd()'.
*                   If this function is called, then the transfer was successful, so no error should be
*                   returned.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_CmdDataRd (FS_QTY               unit_nbr,
                                   FS_DEV_SD_CARD_CMD  *p_cmd,
                                   void                *p_dest,
                                   FS_DEV_SD_CARD_ERR  *p_err)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
   (void)&p_cmd;
   (void)&p_dest;

   *p_err = FS_DEV_SD_CARD_ERR_NONE;
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
* Note(s)     : (1) The data transfer will have completed prior to the return of 'FSDev_SD_CardBSP_CmdWaitEnd()'.
*                   If this function is called, then the transfer was successful, so no error should be
*                   returned.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_CmdDataWr (FS_QTY               unit_nbr,
                                   FS_DEV_SD_CARD_CMD  *p_cmd,
                                   void                *p_src,
                                   FS_DEV_SD_CARD_ERR  *p_err)
{
   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */
   (void)&p_cmd;
   (void)&p_src;

   *p_err = FS_DEV_SD_CARD_ERR_NONE;
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
   (void)&blk_size;

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
* Note(s)     : (1) The MCI interface is capable of 1- & 4-bit operation.
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
    AT91SAM9_STRUCT_MCI  *p_mci;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

   p_mci = (AT91SAM9_STRUCT_MCI *)AT91SAM9_REG_MCI_BASE;

    if (width == 4u) {
        p_mci->MCI_SDCR |=  AT91SAM9_BIT_MCI_SDCR_SDCBUS;
    } else {
        p_mci->MCI_SDCR &= ~AT91SAM9_BIT_MCI_SDCR_SDCBUS;
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
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSDev_SD_Card_BSP_SetClkFreq (FS_QTY      unit_nbr,
                                    CPU_INT32U  freq)
{
    AT91SAM9_STRUCT_MCI  *p_mci;
    CPU_INT32U            clk_div;
    CPU_INT32U            freq_actual;
    CPU_INT32U            mclk_freq;
    CPU_INT32U            mr;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    mclk_freq      = AT91SAM9_MCLK_FREQ;
    clk_div        = mclk_freq / (freq * 2u);
    if (clk_div > 0u) {
        clk_div--;
    }

    freq_actual    = mclk_freq / (2u * (clk_div + 1u));         /* Chk setting.                                         */
    if (freq_actual > freq) {
        clk_div++;
    }

    p_mci = (AT91SAM9_STRUCT_MCI *)AT91SAM9_REG_MCI_BASE;

    mr             =  p_mci->MCI_MR;
    mr            &= ~AT91SAM9_BIT_MCI_MR_CLKDIV_MASK;
    mr            |=  clk_div;
    mr            |=  AT91SAM9_BIT_MCI_MR_PWSDIV_MASK;
    p_mci->MCI_MR  =  mr;
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
    AT91SAM9_STRUCT_MCI  *p_mci;
    CPU_INT32U            dtocyc;
    CPU_INT32U            dtomul;


   (void)&unit_nbr;                                             /* Prevent compiler warning.                            */

    if (to_clks < 16u) {
        dtocyc = to_clks / 1u;
        dtomul = AT91SAM9_BIT_MCI_DTOR_DTOMUL_1;
    } else if (to_clks < 256u) {
        dtocyc = to_clks / 16u;
        dtomul = AT91SAM9_BIT_MCI_DTOR_DTOMUL_16;
    } else if (to_clks < 4096u) {
        dtocyc = to_clks / 256u;
        dtomul = AT91SAM9_BIT_MCI_DTOR_DTOMUL_256;
    } else if (to_clks < 65536u) {
        dtocyc = to_clks / 4096u;
        dtomul = AT91SAM9_BIT_MCI_DTOR_DTOMUL_4096;
    } else if (to_clks < 1048576u) {
        dtocyc = to_clks / 65536u;
        dtomul = AT91SAM9_BIT_MCI_DTOR_DTOMUL_65536;
    } else {
        dtocyc = to_clks / 1048576u;
        dtomul = AT91SAM9_BIT_MCI_DTOR_DTOMUL_1048576;
    }

    if (dtocyc > 0xFu) {
        dtocyc = 0xFu;
    }

    p_mci           = (AT91SAM9_STRUCT_MCI *)AT91SAM9_REG_MCI_BASE;
    p_mci->MCI_DTOR =  dtocyc | dtomul;
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
*                                  FSDev_SD_Card_BSP_ISR_Handler()
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
    AT91SAM9_STRUCT_MCI  *p_mci;
    CPU_INT32U            sr;
    CPU_INT32U            imr;
    FS_ERR                err;
    CPU_BOOLEAN           post;


    p_mci = (AT91SAM9_STRUCT_MCI *)AT91SAM9_REG_MCI_BASE;

    sr    = p_mci->MCI_SR;
    imr   = p_mci->MCI_IMR;
    sr   &= imr;


    err  = FS_DEV_SD_CARD_ERR_NONE;
    post = DEF_NO;
    if ((sr & AT91SAM9_BIT_MCI_INT_ERR) != 0u) {
        err = FS_DEV_SD_CARD_ERR_DATA;
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_DCRCE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_DATA_CHKSUM;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_DTOE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_DATA_TIMEOUT;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_UNRE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_DATA_UNDERRUN;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_OVRE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_DATA_OVERRUN;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_RTOE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_RESP_TIMEOUT;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_RCRCE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_RESP_CHKSUM;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_RENDE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_RESP_END_BIT;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_RINDE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_RESP_CMD_IX;
        }
        if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_RDIRE) == DEF_YES) {
            err = FS_DEV_SD_CARD_ERR_DATA_START_BIT;
        }
        p_mci->MCI_IDR = imr;
        post           = DEF_YES;

    } else if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_BLKE) == DEF_YES) {
        err            = FS_DEV_SD_CARD_ERR_NONE;
        p_mci->MCI_IDR = imr;
        post           = DEF_YES;

    } else if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_ENDRX) == DEF_YES) {
        err            = FS_DEV_SD_CARD_ERR_NONE;
        p_mci->MCI_IDR = imr;
        post           = DEF_YES;

    } else if (DEF_BIT_IS_SET(sr, AT91SAM9_BIT_MCI_INT_CMDRDY) == DEF_YES) {
        err            = FS_DEV_SD_CARD_ERR_NONE;
        p_mci->MCI_IDR = imr;
        post           = DEF_YES;
    }

    FSDev_SD_Card_BSP_Err = err;

    if (post == DEF_YES) {
       (void)FS_OS_SemPost(&FSDev_SD_Card_OS_Sem);
    }

    AT91SAM9_REG_AIC_IVR  = 0u;
    AT91SAM9_REG_AIC_ICCR = DEF_BIT(AT91SAM9_INT_ID_MCI);
}
