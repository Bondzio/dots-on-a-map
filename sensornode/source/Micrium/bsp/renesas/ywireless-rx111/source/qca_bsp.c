/*
*********************************************************************************************************
*                                            uC/TCP-IP V2
*                                      The Embedded TCP/IP Suite
*                                            EXAMPLE CODE
*
*                          (c) Copyright 2003-2014; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/TCP-IP is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.  However,
*               please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                            NETWORK BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                                            Renesas RX111
*                                                on the
*                                           YWIRELESS-RX111
*
* Filename      : qca_bsp.c
* Version       : V1.00.00
* Programmer(s) : JB
*                 DC
*                 JPC
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>
#include  <bsp_sys.h>
#include  <qca.h>
#include  "iodefine.h"
#include  "qca_bsp.h"
#include  "bsp_spi.h"
#include  "m1_bsp.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
                                                                /* GPIO pin direction setting defines.                  */
#define  DEF_IN                                     0u
#define  DEF_OUT                                    1u

#define  QCA_BSP_PIN_POWER_EN                       27u
#define  QCA_BSP_PIN_INT                            5u
#define  QCA_BSP_PIN_CS                             15u

#define  QCA_BSP_SPI_QDR_QSPI_CS0                   0x01u

#define  QCA_BSP_SPI_TFFF_TIMEOUT_VAL               0xFFFFu
#define  QCA_BSP_SPI_RFDF_TIMEOUT_VAL               0xFFFFu

#define  GPIO_PIN_MASK                              0x1Fu

#define  BSP_QCA_SCLK_FREQ_INIT                     400000      /* 400 KHz                                              */

/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

#define  SPI_CTAR_FMSZ_GET(x)                   (((x) & SPI_CTAR_FMSZ_MASK)   >> SPI_CTAR_FMSZ_SHIFT)
#define  SPI_POPR_RXDATA_GET(x)                 (((x) & SPI_POPR_RXDATA_MASK) >> SPI_POPR_RXDATA_SHIFT)
#define  SPI_PUSHR_PCS_GET(x)                   (((x) & SPI_PUSHR_PCS_MASK)   >> SPI_PUSHR_PCS_SHIFT)

#define  GPIO_PIN(x)                            (((1u) << ((x) & GPIO_PIN_MASK)))

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  QCA_BSP_Start         (void);

static  void  QCA_BSP_Stop          (void);

static  void  QCA_BSP_CfgGPIO       (void);

static  void  QCA_BSP_CfgExtIntCtrl (void);

static  void  QCA_BSP_SPI_Init      (void);

static  void  QCA_BSP_SPI_Lock      (void);

static  void  QCA_BSP_SPI_Unlock    (void);

static  void  QCA_BSP_SPI_WrRd      (CPU_INT08U                  *p_buf_wr,
                                     CPU_INT08U                  *p_buf_rd,
                                     CPU_INT16U                   len);

static  void  QCA_BSP_SPI_ChipSelEn (void);

static  void  QCA_BSP_SPI_ChipSelDis(void);

static  void  QCA_BSP_SPI_SetCfg    (QCA_CFG_SPI_CLK_FREQ         freq,
                                     QCA_CFG_SPI_CLK_POL          pol,
                                     QCA_CFG_SPI_CLK_PHASE        pha,
                                     QCA_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                     QCA_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir);

static  void  QCA_BSP_ExtIntCtrl    (CPU_BOOLEAN                  en);


/*
*********************************************************************************************************
*                                    QUALCOMM DEVICE BSP INTERFACE
*********************************************************************************************************
*/

QCA_DEV_BSP  const  QCA_BSP_YWIRELESS_RX111_GT202 = {
    QCA_BSP_Start,
    QCA_BSP_Stop,
    QCA_BSP_CfgGPIO,
    QCA_BSP_CfgExtIntCtrl,
    QCA_BSP_ExtIntCtrl,
    QCA_BSP_SPI_Init,
    QCA_BSP_SPI_Lock,
    QCA_BSP_SPI_Unlock,
    QCA_BSP_SPI_WrRd,
    QCA_BSP_SPI_ChipSelEn,
    QCA_BSP_SPI_ChipSelDis,
    QCA_BSP_SPI_SetCfg
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      SPI DEVICE DRIVER FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           NetDev_Start()
*
* Description : Start wireless hardware.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Start()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  void  QCA_BSP_Start (void)
{
    BSP_PMOD_Reset();
}


/*
*********************************************************************************************************
*                                            NetDev_Stop()
*
* Description : Stop wireless hardware.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Stop()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_Stop (void)
{
    PMODRST_PODR = 0;                                      /* De-assert the reset signal.                          */
}


/*
*********************************************************************************************************
*                                          NetDev_CfgGPIO()
*
* Description : (1) Configure pins:
*
*                   (a) Configure SPI     pins.
*                   (b) Configure EXT ISR pin.
*                   (c) Configure PWR     pin.
*                   (d) Configure RST     pin.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->CfgGPIO()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none
*********************************************************************************************************
*/

// for PMOD2 (wifi)
static  void  QCA_BSP_CfgGPIO (void)
{
                                                                /* Configure I/O pin directions.                        */
    PMOD2_PIN7_PDR = DEF_IN;                                  /* IRQ6                                                 */
    PMOD2_PIN1_PDR = DEF_OUT;                                 /* SSLA0                                                */

    MPC.PWPR.BIT.B0WI  = 0;                                     /* Disable write protection for MPC.                    */
    MPC.PWPR.BIT.PFSWE = 1;

    PMOD2_PIN7_PFS_ISEL = 1;                                    /* Enable IRQ5   on           PA[4]                     */

    MPC.PWPR.BIT.PFSWE = 0;                                     /* Re-enable write protection for MPC.                  */
    MPC.PWPR.BIT.B0WI  = 1;

                                                                /* Configure pin modes. 0 = GPIO, 1 = Peripheral.       */
    PMOD2_PIN1_PMR = 0;                                       /* PC[4] as GPIO for PMOD2 chip select                  */

                                                                /* Set chip selects to inactive.                        */
    PMOD2_PIN1_PODR = 1;
}


/*
*********************************************************************************************************
*                                       NetDev_CfgExtIntCtrl()
*
* Description : Assign extern ISR to the ISR Handler.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->CfgExtIntCtrl()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_CfgExtIntCtrl (void)
{
#ifdef RX231
    ICU.IRQCR[0].BIT.IRQMD = 0x1;                               /* Falling-edge trigger.                                */
    IPR(ICU, IRQ0)         = 0x1;                               /* Set IPL to lowest setting.                           */
    IR(ICU,  IRQ0)         =   0;                               /* Clear any pending interrupt.                         */
#else
    ICU.IRQCR[5].BIT.IRQMD = 0x1;                               /* Falling-edge trigger.                                */
    IPR(ICU, IRQ5)         = 0x1;                               /* Set IPL to lowest setting.                           */
    IR(ICU,  IRQ5)         =   0;                               /* Clear any pending interrupt.                         */
#endif
}


/*
*********************************************************************************************************
*                                        QCA_BSP_SPI_Init()
* Description : no-op
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_Init (void)
{
}


/*
*********************************************************************************************************
*                                          NetDev_SPI_Lock()
*
* Description : Acquire SPI lock.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Lock()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) This function will be called before the device driver begins to access the SPI.  The
*                   application should NOT use the same bus to access another device until the matching
*                   call to 'NetDev_SPI_Unlock()' has been made.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_Lock (void)
{
    BSP_SPI_Lock(0);
}


/*
*********************************************************************************************************
*                                         NetDev_SPI_Unlock()
*
* Description : Release SPI lock.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Unlock()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_Lock()' will be called before the device driver begins to access the SPI.
*                   The application should NOT use the same bus to access another device until the
*                   matching call to this function has been made.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_Unlock (void)
{
    BSP_SPI_Unlock(0);
}


/*
*********************************************************************************************************
*                                          NetDev_SPI_WrRd()
*
* Description : Write and read from SPI.
*
* Argument(s) : p_if        Pointer to interface to start the hardware.
*               ----        Argument validated in NetIF_Add().
*
*               p_buf_wr    Pointer to buffer to write.
*               --------    Argument validated in $$$$.
*
*               p_buf_rd    Pointer to buffer for data read.
*               --------    Argument validated in $$$$.
*
*               wr_rd_len   Number of octets to write and read.
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_WrRd()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_WrRd (CPU_INT08U  *p_buf_wr,
                                CPU_INT08U  *p_buf_rd,
                                CPU_INT16U   wr_rd_len)
{
    BSP_SPI_Xfer(0, p_buf_wr, p_buf_rd, wr_rd_len);

}

/*
*********************************************************************************************************
*                                       NetDev_SPI_ChipSelEn()
*
* Description : Enable wireless device chip select.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_ChipSelEn()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_ChipSelEn()' will be called before the device driver begins to access
*                   the SPI.
*
*               (2) The chip select is typically 'active low'; to enable the card, the chip select pin
*                   should be cleared.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_ChipSelEn (void)
{
    BSP_SPI_ChipSel(0, BSP_SPI_SLAVE_ID_PMOD2, DEF_ON);
}


/*
*********************************************************************************************************
*                                       NetDev_SPI_ChipSelDis()
*
* Description : Disable wireless device chip select.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_ChipSelDis()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_ChipSelDis()' will be called when the device driver finished to access
*                   the SPI.
*
*               (1) The chip select is typically 'active low'; to disable the card, the chip select pin
*                   should be set.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_ChipSelDis (void)
{
    BSP_SPI_ChipSel(0, BSP_SPI_SLAVE_ID_PMOD2, DEF_OFF);
}


/*
*********************************************************************************************************
*                                       QCA_BSP_SPI_SetClkFreq()
*
* Description : Set SPI clock frequency.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               freq    Clock frequency, in Hz.
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_SetClkFreq()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_SetClkFreq()' will be called before the device driver begins to access
*                   the SPI and after 'NetDev_SPI_Lock()' has been called.
*
*               (2) The effective clock frequency MUST be no more than 'freq'.  If the frequency cannot be
*                   configured equal to 'freq', it should be configured less than 'freq'.
*
*               (3) The Baud Rate Scaler is only used in master mode. the Prescaled system clk is divided
*                   by the Baud Rate Scaler to generate the Freq of SCK. The baud rate is computed accord.
*                   to the following equation:
*                                        fSYS     (1 + DBR)         Where fSYS = Peripheral Clk.
*                       SCK Baud Rate = ------ * -----------                        (60 MHz)
*                                        PBR          BR
*********************************************************************************************************
*/

void  QCA_BSP_SPI_SetCfg (QCA_CFG_SPI_CLK_FREQ         freq,
                          QCA_CFG_SPI_CLK_POL          cpol,
                          QCA_CFG_SPI_CLK_PHASE        cpha,
                          QCA_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                          QCA_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir)
{
    BSP_SPI_BUS_CFG  spi_cfg = {
        .SClkFreqMax = freq,
        .CPHA = cpha,
        .CPOL = cpol,
        .BitsPerFrame = xfer_unit_len,
        .LSBFirst = xfer_shift_dir
    };


    BSP_SPI_Cfg(0, &spi_cfg);
}


/*
*********************************************************************************************************
*                                         NetDev_ExtIntCtrl()
*
* Description : Enable or diable external interrupt.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               en      Enable or diable the interrupt.
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->ExtIntCtrl()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_ExtIntCtrl (CPU_BOOLEAN   en)
{
#ifdef RX231
    IEN(ICU, IRQ0) = (en == DEF_YES ? 1u : 0u);
#else
    IEN(ICU, IRQ5) = (en == DEF_YES ? 1u : 0u);
#endif
}


/*
*********************************************************************************************************
*                                        NetBSP_ISR_HandlerK70F120M_1()
*
* Description : BSP-level ISR handler for device receive & transmit interrupts.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : CPU &/or device interrupts.
*
* Note(s)     : (1) (a) Each device interrupt, or set of device interrupts, MUST be handled by a
*                       unique BSP-level ISR handler which maps each specific device interrupt to
*                       its corresponding network interface ISR handler.
*
*                   (b) BSP-level device ISR handlers SHOULD be named using the following convention :
*
*                           NetDev_ISR_Handler<Dev><Type>[Nbr]()
*
*                               where
*                                   (1) Dev         Network device name or type
*                                   (2) Type        Network device interrupt type (optional,
*                                                       if generic interrupt type or unknown)
*                                   (3) Nbr         Network device number (optional;
*                                                       see 'NETWORK DEVICE BSP INTERFACE  Note #2a3')
*********************************************************************************************************
*/
#define GT202_HACK
#ifdef GT202_HACK
volatile long temp;
#endif
CPU_ISR  QCA_BSP_ISR_YWireless_RX111_GT202 (void)
{
#ifdef GT202_HACK
    temp = 0;
    while (temp < 50000) temp++;
#endif
    OSIntEnter();                                               /* Notify uC/OS-III of ISR entry                        */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */

    QCA_ISR_Handler();

    OSIntExit();                                                /* Notify uC/OS-III of ISR exit                         */
}
