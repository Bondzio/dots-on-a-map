#include  <os.h>
#include  <bsp_sys.h>
#include  "iodefine.h"
#include  <m1_bsp.h>


void SCI_BSP_I2C_init() {
#ifdef RX231
    // P17 controls the green wifi status LED on the RSK...
#else
    PORT1.PDR.BIT.B7 = DEF_IN;                                 /* IRQ7                                                 */
#endif
    PORT1.PDR.BIT.B5 = DEF_IN;                                  /* SCI1.rx                                              */
    PORT1.PDR.BIT.B6 = DEF_IN;                                 /* SCI1.tx                                              */
    PORT1.ODR1.BIT.B2 = 0x1;
    PORT1.ODR1.BIT.B4 = 0x1;
    MPC.PWPR.BIT.B0WI  = 0;                                     /* Disable write protection for MPC.                    */
    MPC.PWPR.BIT.PFSWE = 1;
    MPC.P15PFS.BIT.PSEL = 0xA; // RXD1/SMISO1/SSCL1
    MPC.P16PFS.BIT.PSEL = 0xA; // TXD1/SMOSI1/SSDA1
    MPC.PWPR.BIT.PFSWE = 0;                                     /* Re-enable write protection for MPC.                  */
    MPC.PWPR.BIT.B0WI  = 1;
    PORT1.PMR.BIT.B5 = 1;
    PORT1.PMR.BIT.B6 = 1;

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
void  SCI_BSP_I2C_SetCfg (long         freq,
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

    SCI1.SCR.BYTE = 0x00; // disable all interrupts and tx/rx
    SCI1.SIMR3.BIT.IICSDAS = 0x3;
    SCI1.SIMR3.BIT.IICSCLS = 0x3;

    SCI1.SMR.BYTE = 0x00;
    SCI1.SCMR.BIT.SMIF = 0;
    SCI1.SCMR.BIT.SINV = 0;
    SCI1.SCMR.BIT.SDIR = 1;
    pclk = BSP_SysPeriphClkFreqGet(CLK_ID_PCLKB);
    brr_n = 32;
    int cks_value = 0;
#if 1
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

    SCI1.SMR.BIT.CKS = cks_value;
    SCI1.BRR = brr_value;
#else
    // ~100 kbps
    SCI1.SMR.BIT.CKS = 0x1;
    SCI1.BRR = 0x1;
#endif
    SCI1.SEMR.BIT.NFEN = 0x1;
    SCI1.SNFR.BIT.NFCS = 0x1;
    SCI1.SIMR1.BIT.IICM = 0x1;
    SCI1.SIMR1.BIT.IICDL = cpha;
    SCI1.SIMR2.BIT.IICACKT = 0x1;
    SCI1.SIMR2.BIT.IICCSC = 0x1;
    SCI1.SIMR2.BIT.IICINTM = 0x1;
    SCI1.SPMR.BYTE = 0x00;
    SCI1.SCR.BYTE = SCI1.SCR.BYTE | 0x30; // TIE RIE TE RE MPIE TEIE CKE[2]
    SCI1.SCR.BYTE = SCI1.SCR.BYTE | 0xC4; // TIE RIE TE RE MPIE TEIE CKE[2]
}

OS_SEM sci_i2c_sem;

void  SCI_BSP_I2C_Lock (void)
{
    OS_ERR err;
    CPU_TS ts;
    OSSemPend(&sci_i2c_sem, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
}

void  SCI_BSP_I2C_Unlock (void)
{
    OS_ERR err;

    OSSemPost(&sci_i2c_sem, OS_OPT_POST_1, &err);
}

static void _SCI_BSP_I2C_Start() {
    OS_ERR err_os;

    // generate start condition
    SCI1.SCR.BIT.RIE = 0x0;
    IR(SCI1, TEI1) = 0;
    SCI1.SIMR3.BYTE |= 0x51; // IICCSCLS[2] IICSDAS[2] x x IICRSTAREQ IICSTAREQ
    /*
    SIMR3.IICSTAREQ bit to
1 and the SIMR3.IICSCLS[1:0] and
IICSDAS[1:0] bits to 01b
    */
    // start condition completed
    while (!IR(SCI1, TEI1) && !SCI1.SIMR3.BIT.IICSTIF);
    IR(SCI1, TEI1) = 0;
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    SCI1.SIMR3.BIT.IICSTIF = 0x0;
    SCI1.SIMR3.BIT.IICSDAS = 0x0;
    SCI1.SIMR3.BIT.IICSCLS = 0x0;
}

static void _SCI_BSP_I2C_Restart() {
    OS_ERR err_os;

    // generate start condition
    SCI1.SCR.BIT.RIE = 0x0;
    IR(SCI1, TEI1) = 0;
    SCI1.SIMR3.BYTE |= 0x52; // IICCSCLS[2] IICSDAS[2] IICSTIF IICSTPREQ IICRSTAREQ IICSTAREQ
    /*
    SIMR3.IICSTAREQ bit to
1 and the SIMR3.IICSCLS[1:0] and
IICSDAS[1:0] bits to 01b
    */
    // start condition completed
    while (!IR(SCI1, TEI1) && !SCI1.SIMR3.BIT.IICSTIF);
    IR(SCI1, TEI1) = 0;
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    SCI1.SIMR3.BIT.IICSTIF = 0x0;
    SCI1.SIMR3.BIT.IICSDAS = 0x0;
    SCI1.SIMR3.BIT.IICSCLS = 0x0;
}

static void _SCI_BSP_I2C_Stop() {
    OS_ERR err_os;

    // generate stop condition
    SCI1.SCR.BIT.RIE = 0x0;
    IR(SCI1, TEI1) = 0;
    SCI1.SIMR3.BYTE |= 0x54; // IICCSCLS[2] IICSDAS[2] IICSTIF IICSTPREQ IICRSTAREQ IICSTAREQ
    /*
    SIMR3.IICSTAREQ bit to
1 and the SIMR3.IICSCLS[1:0] and
IICSDAS[1:0] bits to 01b
    */
    // start condition completed
    while (!IR(SCI1, TEI1) && !SCI1.SIMR3.BIT.IICSTIF);
    IR(SCI1, TEI1) = 0;
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    SCI1.SIMR3.BIT.IICSTIF = 0x0;
    SCI1.SIMR3.BIT.IICSDAS = 0x3;
    SCI1.SIMR3.BIT.IICSCLS = 0x3;
}

int SCI_BSP_I2C_WrRd(int address, CPU_INT08U * data, CPU_INT08U * read, int bytes) {
    int i;
    int rdwr = (data == DEF_NULL) ? 1 : 0;
    int ret = 0;

    _SCI_BSP_I2C_Start();
    // write slave address and r/w bit
    SCI1.TDR =  ((CPU_INT08U )address << 1) | rdwr;

    // wait for txi interrupt
    while (!IR(SCI1, TXI1));
    IR(SCI1, TXI1) = 0;
    // check for ack
    if (!SCI1.SISR.BIT.IICACKR) {
        if (!rdwr) { // write
            for (i = 0; i < bytes; ++i) {
                // write data
                SCI1.TDR =  *data++;
                // wait for txi interrupt
                while (!IR(SCI1, TXI1));
                IR(SCI1, TXI1) = 0;
            }
        } else { // read
            SCI1.SCR.BIT.RIE = 0x1;
            for (i = 0; i < bytes; ++i) {
                SCI1.SIMR2.BIT.IICACKT = 0x0;
                if (i == (bytes - 1))
                    SCI1.SIMR2.BIT.IICACKT = 0x1;
                // write data
                SCI1.TDR =  0xff;
                // wait for rxi interrupt
                while (!IR(SCI1, RXI1));
                IR(SCI1, RXI1) = 0;
                *read++ = SCI1.RDR;
                // wait for txi interrupt
                while (!IR(SCI1, TXI1));
                IR(SCI1, TXI1) = 0;
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
int SCI_BSP_I2C_WrAndRd(int address, CPU_INT08U * data, int writeBytes, CPU_INT08U * read, int readBytes) {
  // TODO: assert writeBytes >= 1?
    int i;
    int ret = 0;

    _SCI_BSP_I2C_Start();
    // write slave address and r/w bit
    SCI1.TDR =  ((CPU_INT08U )address << 1) | 0;

    // wait for txi interrupt
    while (!IR(SCI1, TXI1));
    IR(SCI1, TXI1) = 0;
    // check for ack
    if (!SCI1.SISR.BIT.IICACKR) {
        for (i = 0; i < writeBytes; ++i) {
            // write data
            SCI1.TDR =  *data++;
            // wait for txi interrupt
            while (!IR(SCI1, TXI1));
            IR(SCI1, TXI1) = 0;
        }
        _SCI_BSP_I2C_Restart();
        // write slave address and r/w bit
        SCI1.TDR =  ((CPU_INT08U )address << 1) | 1;
        // wait for txi interrupt
        while (!IR(SCI1, TXI1));
        IR(SCI1, TXI1) = 0;
        // check for ack
        if (!SCI1.SISR.BIT.IICACKR) {
            SCI1.SCR.BIT.RIE = 0x1;
            for (i = 0; i < readBytes; ++i) {
                SCI1.SIMR2.BIT.IICACKT = 0x0;
                if (i == (readBytes - 1))
                    SCI1.SIMR2.BIT.IICACKT = 0x1;
                // write data
                SCI1.TDR =  0xff;
                // wait for rxi interrupt
                while (!IR(SCI1, RXI1));
                IR(SCI1, RXI1) = 0;
                *read++ = SCI1.RDR;
                // wait for txi interrupt
                while (!IR(SCI1, TXI1));
                IR(SCI1, TXI1) = 0;
            }
        } else
            ret = -2;
    } else
        ret = -1;

    _SCI_BSP_I2C_Stop();
    return ret;
}
