#include  <os.h>
#include  <bsp_sys.h>
#include  "iodefine.h"
#include  <m1_bsp.h>


void SCI6_BSP_I2C_init() {
#ifdef RX231
    // P17 controls the green wifi status LED on the RSK...
#else
    PORT1.PDR.BIT.B7 = DEF_IN;                                 /* IRQ7                                                 */
#endif
    PORT3.PDR.BIT.B3 = DEF_IN;                                  /* SCI6.rx                                              */
    PORT3.PDR.BIT.B2 = DEF_IN;                                 /* SCI6.tx                                              */
    PORT3.ODR0.BIT.B6 = 0x1;
    PORT3.ODR0.BIT.B4 = 0x1;
    MPC.PWPR.BIT.B0WI  = 0;                                     /* Disable write protection for MPC.                    */
    MPC.PWPR.BIT.PFSWE = 1;
    MPC.P33PFS.BIT.PSEL = 0xB; // RXD1/SMISO1/SSCL1
    MPC.P32PFS.BIT.PSEL = 0xB; // TXD1/SMOSI1/SSDA1
    MPC.PWPR.BIT.PFSWE = 0;                                     /* Re-enable write protection for MPC.                  */
    MPC.PWPR.BIT.B0WI  = 1;
    PORT3.PMR.BIT.B3 = 1;
    PORT3.PMR.BIT.B2 = 1;

    // power-on controller
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();                                       /* Disable interrupts so we can ...                     */
    SYSTEM.PRCR.WORD = 0xA507;                                  /* ... disable register protection.                     */

    MSTP(SCI6) = 0u;

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
void  SCI6_BSP_I2C_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir)
{
    CPU_INT32U  pclk;
    int brr_n;
    int n = 0;
    long newFreq;
    static int set = 0;

    SCI6.SCR.BYTE = 0x00; // disable all interrupts and tx/rx
    SCI6.SIMR3.BIT.IICSDAS = 0x3;
    SCI6.SIMR3.BIT.IICSCLS = 0x3;

    SCI6.SMR.BYTE = 0x00;
    SCI6.SCMR.BIT.SMIF = 0;
    SCI6.SCMR.BIT.SINV = 0;
    SCI6.SCMR.BIT.SDIR = 1;
    pclk = BSP_SysPeriphClkFreqGet(CLK_ID_PCLKB);
    brr_n = 32;
    int cks_value = 0;

    /*  ---- Calculation BRR ---- */
    int brr_value = (int)((double)((double)((double)pclk / (brr_n * freq))) - 0.1);

    /* Set clock source */
    while (brr_value > 255)
    {
        /* Counter over 0xff */
        switch (brr_n)
        {
            case 32:
                brr_n = 64;         /* 64*(2^(2*1-1)) */
                cks_value = 1;      /* clock select: PLCK/4 */
                break;
            case 64:
                brr_n  = 256;       /* 64*(2^(2*2-1))  */
                cks_value = 2;      /* clock select: PLCK/16 */
                break;
            case 256:
                brr_n = 1024;       /* 64*(2^(2*3-1)) */
                cks_value = 3;      /* clock select: PLCK/64 */
                break;
            default:
                /* nothing to do */
                break;
        }

        /* ---- Calculation BRR ---- */
        brr_value = (int)((double)(((double)pclk / (brr_n * freq))) - 0.1);

        /* When the clock source of the on-chip baud rate generator is PLCK/64 and when the value
           of brr_value is greater than 255 */
        if ((cks_value == 3) && (brr_value > 255))
        {
            brr_value = 255;
        }
    }

    SCI6.SMR.BIT.CKS = cks_value;
    SCI6.BRR = brr_value;

    SCI6.SEMR.BIT.NFEN = 0x1;
    SCI6.SNFR.BIT.NFCS = 0x1;
    SCI6.SIMR1.BIT.IICM = 0x1;
    SCI6.SIMR1.BIT.IICDL = cpha;
    SCI6.SIMR2.BIT.IICACKT = 0x1;
    SCI6.SIMR2.BIT.IICCSC = 0x1;
    SCI6.SIMR2.BIT.IICINTM = 0x1;
    SCI6.SPMR.BYTE = 0x00;
    SCI6.SCR.BYTE = SCI6.SCR.BYTE | 0x30; // TIE RIE TE RE MPIE TEIE CKE[2]
    SCI6.SCR.BYTE = SCI6.SCR.BYTE | 0xC4; // TIE RIE TE RE MPIE TEIE CKE[2]
}

void  SCI6_BSP_I2C_Lock (void)
{
}

void  SCI6_BSP_I2C_Unlock (void)
{
}

static void _SCI_BSP_I2C_Start() {
    OS_ERR err_os;

    // generate start condition
    SCI6.SCR.BIT.RIE = 0x0;
    IR(SCI6, TEI6) = 0;
    SCI6.SIMR3.BYTE |= 0x51; // IICCSCLS[2] IICSDAS[2] x x IICRSTAREQ IICSTAREQ
    /*
    SIMR3.IICSTAREQ bit to
1 and the SIMR3.IICSCLS[1:0] and
IICSDAS[1:0] bits to 01b
    */
    // start condition completed
    while (!IR(SCI6, TEI6) && !SCI6.SIMR3.BIT.IICSTIF);
    IR(SCI6, TEI6) = 0;
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    SCI6.SIMR3.BIT.IICSTIF = 0x0;
    SCI6.SIMR3.BIT.IICSDAS = 0x0;
    SCI6.SIMR3.BIT.IICSCLS = 0x0;
}

static void _SCI_BSP_I2C_Restart() {
    OS_ERR err_os;

    // generate start condition
    SCI6.SCR.BIT.RIE = 0x0;
    IR(SCI6, TEI6) = 0;
    SCI6.SIMR3.BYTE |= 0x52; // IICCSCLS[2] IICSDAS[2] IICSTIF IICSTPREQ IICRSTAREQ IICSTAREQ
    /*
    SIMR3.IICSTAREQ bit to
1 and the SIMR3.IICSCLS[1:0] and
IICSDAS[1:0] bits to 01b
    */
    // start condition completed
    while (!IR(SCI6, TEI6) && !SCI6.SIMR3.BIT.IICSTIF);
    IR(SCI6, TEI6) = 0;
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    SCI6.SIMR3.BIT.IICSTIF = 0x0;
    SCI6.SIMR3.BIT.IICSDAS = 0x0;
    SCI6.SIMR3.BIT.IICSCLS = 0x0;
}

static void _SCI_BSP_I2C_Stop() {
    OS_ERR err_os;

    // generate stop condition
    SCI6.SCR.BIT.RIE = 0x0;
    IR(SCI6, TEI6) = 0;
    SCI6.SIMR3.BYTE |= 0x54; // IICCSCLS[2] IICSDAS[2] IICSTIF IICSTPREQ IICRSTAREQ IICSTAREQ
    /*
    SIMR3.IICSTAREQ bit to
1 and the SIMR3.IICSCLS[1:0] and
IICSDAS[1:0] bits to 01b
    */
    // start condition completed
    while (!IR(SCI6, TEI6) && !SCI6.SIMR3.BIT.IICSTIF);
    IR(SCI6, TEI6) = 0;
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    SCI6.SIMR3.BIT.IICSTIF = 0x0;
    SCI6.SIMR3.BIT.IICSDAS = 0x3;
    SCI6.SIMR3.BIT.IICSCLS = 0x3;
}

int SCI6_BSP_I2C_WrRd(int address, CPU_INT08U * data, CPU_INT08U * read, int bytes) {
    int i;
    int rdwr = (data == DEF_NULL) ? 1 : 0;
    int ret = 0;

    _SCI_BSP_I2C_Start();
    // write slave address and r/w bit
    SCI6.TDR =  ((CPU_INT08U )address << 1) | rdwr;

    // wait for txi interrupt
    while (!IR(SCI6, TXI6));
    IR(SCI6, TXI6) = 0;
    // check for ack
    if (!SCI6.SISR.BIT.IICACKR) {
        if (!rdwr) { // write
            for (i = 0; i < bytes; ++i) {
                // write data
                SCI6.TDR =  *data++;
                // wait for txi interrupt
                while (!IR(SCI6, TXI6));
                IR(SCI6, TXI6) = 0;
            }
        } else { // read
            SCI6.SCR.BIT.RIE = 0x1;
            for (i = 0; i < bytes; ++i) {
                SCI6.SIMR2.BIT.IICACKT = 0x0;
                if (i == (bytes - 1))
                    SCI6.SIMR2.BIT.IICACKT = 0x1;
                // write data
                SCI6.TDR =  0xff;
                // wait for rxi interrupt
                while (!IR(SCI6, RXI6));
                IR(SCI6, RXI6) = 0;
                *read++ = SCI6.RDR;
                // wait for txi interrupt
                while (!IR(SCI6, TXI6));
                IR(SCI6, TXI6) = 0;
            }
        }
    } else
        ret = -1;

    _SCI_BSP_I2C_Stop();
    return ret;
}

/*
This function performs START-WRITES-RESTART-READS-STOP
Typically used for register-based device access
*/
int SCI6_BSP_I2C_WrAndRd(int address, CPU_INT08U * data, int writeBytes, CPU_INT08U * read, int readBytes) {
  // TODO: assert writeBytes >= 1?
    int i;
    int ret = 0;

    _SCI_BSP_I2C_Start();
    // write slave address and r/w bit
    SCI6.TDR =  ((CPU_INT08U )address << 1) | 0;

    // wait for txi interrupt
    while (!IR(SCI6, TXI6));
    IR(SCI6, TXI6) = 0;
    // check for ack
    if (!SCI6.SISR.BIT.IICACKR) {
        for (i = 0; i < writeBytes; ++i) {
            // write data
            SCI6.TDR =  *data++;
            // wait for txi interrupt
            while (!IR(SCI6, TXI6));
            IR(SCI6, TXI6) = 0;
        }
        _SCI_BSP_I2C_Restart();
        // write slave address and r/w bit
        SCI6.TDR =  ((CPU_INT08U )address << 1) | 1;
        // wait for txi interrupt
        while (!IR(SCI6, TXI6));
        IR(SCI6, TXI6) = 0;
        // check for ack
        if (!SCI6.SISR.BIT.IICACKR) {
            SCI6.SCR.BIT.RIE = 0x1;
            for (i = 0; i < readBytes; ++i) {
                SCI6.SIMR2.BIT.IICACKT = 0x0;
                if (i == (readBytes - 1))
                    SCI6.SIMR2.BIT.IICACKT = 0x1;
                // write data
                SCI6.TDR =  0xff;
                // wait for rxi interrupt
                while (!IR(SCI6, RXI6));
                IR(SCI6, RXI6) = 0;
                *read++ = SCI6.RDR;
                // wait for txi interrupt
                while (!IR(SCI6, TXI6));
                IR(SCI6, TXI6) = 0;
            }
        } else
            ret = -2;
    } else
        ret = -1;

    _SCI_BSP_I2C_Stop();
    return ret;
}
