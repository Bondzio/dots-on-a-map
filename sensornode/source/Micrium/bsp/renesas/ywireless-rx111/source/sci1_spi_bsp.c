#include  <os.h>
#include  <bsp_sys.h>
#include  <qca.h>
#include  <m1_bsp.h>
#include  "iodefine.h"


void SCI_BSP_SPI_init() {
    // TODO: generalize selection of gpio pins
    PORT1.PDR.BIT.B7 = DEF_OUT;                                 /* IRQ7                                                 */
    PORT3.PDR.BIT.B0 = DEF_IN;                                  /* SCI1.rx                                              */
    PORT3.PDR.BIT.B1 = DEF_OUT;                                 /* SCI1.SSL                                             */
    PORT2.PDR.BIT.B6 = DEF_OUT;                                 /* SCI1.tx                                              */
    PORT2.PDR.BIT.B7 = DEF_OUT;                                  /* SCI1.SCK                                             */    
    MPC.PWPR.BIT.B0WI  = 0;                                     /* Disable write protection for MPC.                    */
    MPC.PWPR.BIT.PFSWE = 1;
    MPC.P30PFS.BIT.PSEL = 0xA; // RXD1/SMISO1/SSCL1
    MPC.P26PFS.BIT.PSEL = 0xA; // TXD1/SMOSI1/SSDA1
    MPC.P27PFS.BIT.PSEL = 0xA; // SCK1
    MPC.PWPR.BIT.PFSWE = 0;                                     /* Re-enable write protection for MPC.                  */
    MPC.PWPR.BIT.B0WI  = 1;
    PORT3.PMR.BIT.B0 = 1;
    PORT2.PMR.BIT.B6 = 1;
    PORT2.PMR.BIT.B7 = 1;

    // power-on controller
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();                                       /* Disable interrupts so we can ...                     */
    SYSTEM.PRCR.WORD = 0xA507;                                  /* ... disable register protection.                     */

    MSTP(SCI1) = 0u;

    SYSTEM.PRCR.WORD = 0xA500;                                  /* Re-enable register write protection.                 */

    CPU_CRITICAL_EXIT();
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
void  SCI_BSP_SPI_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir)
{
    CPU_INT32U  pclk;
    double brr;
    int n = 0;
    long newFreq;
    static int set = 0;

    SCI1.SCR.BYTE = 0x00; // disable all interrupts and tx/rx
    
    SCI1.SCR.BIT.CKE = 0x0; // internal clock
    SCI1.SIMR1.BIT.IICM = 0; // don't select i2c mode?
    SCI1.SPMR.BIT.CKPH = cpha;
    SCI1.SPMR.BIT.CKPOL = cpol;
    
    SCI1.SEMR.BYTE = 0x00; // SEMR?
    SCI1.SCMR.BIT.SMIF = 0;
    SCI1.SCMR.BIT.SINV = 0;
    SCI1.SCMR.BIT.SDIR = ~(xfer_shift_dir & 0x0001);
    SCI1.SMR.BIT.CM = 1;  // set clock-synchronous / simple spi communication mode
    pclk = BSP_SysPeriphClkFreqGet(CLK_ID_PCLKB);
    brr = (int)((pclk / (4.0 * freq)) - 1);
    if (brr >= 0) {
        newFreq = pclk / ((brr + 1) * 4.0);
        if ((newFreq != freq) && (newFreq > freq)) {
            for (n = 1; n < 4; n++) {
                brr = (int)((pclk / (8.0 * (1 << (2*n-1)) * freq)) - 1);
                if (brr >= 0) {
                    newFreq = pclk / ((brr + 1) * 8.0 * (1 << (2*n-1)));
                    if (newFreq == freq)
                        break;
                }
            }
        } else if (newFreq != freq)
            n = 4;
    }
    if (n == 4)
        BSP_LED_On(2);
    else {
      if (!set) {
        SCI1.SMR.BIT.CKS = n;
        set = 1;
      }
        SCI1.BRR = brr;
    }

}

OS_SEM sci_spi_sem;

void  SCI_BSP_SPI_Lock (void)
{
    OS_ERR err;
    CPU_TS ts;
    OSSemPend(&sci_spi_sem, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
}

void  SCI_BSP_SPI_Unlock (void)
{
    OS_ERR err;
    
    OSSemPost(&sci_spi_sem, OS_OPT_POST_1, &err);
}

void  SCI_BSP_SPI_WrRd (CPU_INT08U  *p_buf_wr,
                                CPU_INT08U  *p_buf_rd,
                                CPU_INT16U   wr_rd_len)
{
    CPU_INT32U              i;
    CPU_INT32U              data;
    CPU_INT08U              ptr_incr_wr;
    CPU_INT08U              ptr_incr_rd;
    CPU_INT32U              dummy_wr;
    CPU_INT32U              dummy_rd;
    CPU_INT32U              xfer_cnt;

                                          /* Determine buffer pointer incrementation size.        */
    ptr_incr_wr = ptr_incr_rd = 1u;

    xfer_cnt = wr_rd_len / ptr_incr_wr;                         /* Determine the number of transfer loops needed.       */

                                                                /* Setup read/write buffer data pointers.               */
    if (p_buf_wr == DEF_NULL) {
        dummy_wr    = 0xFFFFFFFF;                               /* Populate the write buffer with 1's.                  */
        p_buf_wr    = (CPU_INT08U *)&dummy_wr;
        ptr_incr_wr = 0u;
    } else if (p_buf_rd == DEF_NULL) {
        p_buf_rd    = (CPU_INT08U *)&dummy_rd;
        ptr_incr_rd = 0u;
    }

    /* must do all at once
    SCI1.SCR.BIT.RIE = 1; // Receive Interrupt Enable
    SCI1.SCR.BIT.TIE = 1; // Transmit Interrupt Enable
    SCI1.SCR.BIT.RE = 1; // Receive Enable
    SCI1.SCR.BIT.TE = 1; // Transmit Enable
    */
#if 1
    SCI1.SCR.BYTE = SCI1.SCR.BYTE | (0xf0);
#else
    SCI1.SCR.BYTE = SCI1.SCR.BYTE | (0xa0);
#endif
    for (i = 0; i < xfer_cnt; ++i) {
        while (!IR(SCI1, TXI1));
                                                                /* Copy write data into SPI data register.              */
        data              = *(CPU_INT08U *)p_buf_wr;
        SCI1.TDR =  (CPU_INT08U  )data;

        p_buf_wr += ptr_incr_wr;
#if 1
        // check for receive errors
        if (SCI1.SSR.BIT.ORER) {
          BSP_LED_On(2);
          SCI1.SSR.BIT.ORER = 0;
        }
        
        while (!IR(SCI1, RXI1));                         /* Wait for SPRI interrupt.                             */

                                                                /* Read data from the SPI data register.                */
        data                  = (CPU_INT08U)SCI1.RDR;
        *(CPU_INT08U *)p_buf_rd = (CPU_INT08U)data;

        p_buf_rd += ptr_incr_rd;
#endif
    }

    /* must do all at once
    SCI1.SCR.BIT.TIE = 0;
    SCI1.SCR.BIT.RIE = 0;
    SCI1.SCR.BIT.TE = 0;
    SCI1.SCR.BIT.RE = 0;
    SCI1.SCR.BIT.TEIE = 0; //?
    */
#if 1
    SCI1.SCR.BYTE = SCI1.SCR.BYTE & (0x0f);
#else
    SCI1.SCR.BYTE = SCI1.SCR.BYTE & (0x0f);    
#endif
}


void  SCI_BSP_SPI_ChipSelEn (void)
{
    PORT3.PODR.BIT.B1 = 0u;
}

void  SCI_BSP_SPI_ChipSelDis (void)
{
    PORT3.PODR.BIT.B1 = 1u;
}

void SCI_BSP_SPI_Data(void) {
  PORT1.PODR.BIT.B7 = 1;
}

void SCI_BSP_SPI_Command(void) {
  PORT1.PODR.BIT.B7 = 0;
}