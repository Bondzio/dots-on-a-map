#include <stdlib.h>
#include <stdint.h>
#include  <bsp.h>
#include  <bsp_int_vect_tbl.h>
#include  <bsp_cfg.h>
#include  <os.h>
#include  <iorx651.h>
#include  <bsp_os.h>
#include  <bsp_clk.h>
#include  <bsp_uart.h>
#include  <lib_str.h>


#define COUNT_TIMEOUT 200000L
#define NUM_DIVISORS_ASYNC  (9)


typedef struct st_baud_divisor
{
    int16_t     divisor;    // clock divisor
    uint8_t     abcs;       // abcs value to get divisor
    uint8_t     bgdm;       // bdgm value to get divisor
    uint8_t     cks;        // cks  value to get divisor (cks = n)
} baud_divisor_t;


/* NOTE: diff than SCI async baud table, but should provide same results */
const baud_divisor_t async_baud[NUM_DIVISORS_ASYNC]=
{
    /* divisor result, abcs, bgdm, n */
    {8,    1, 1, 0},
    {16,   0, 1, 0},
    {32,   0, 0, 0},
    {64,   0, 1, 1},
    {128,  0, 0, 1},
    {256,  0, 1, 2},
    {512,  0, 0, 2},
    {1024, 0, 1, 3},
    {2048, 0, 0, 3}
};

static char hwFlowControl = 0;
static char s_provisioning = 0;
#define DEF_IN 0
#define DEF_OUT 1
extern OS_FLAG_GRP sonar_grp;

void SCI_BSP_UART_init(char hwFlowControl, char prov) {
    OS_ERR err_os;

    s_provisioning = prov;
//    PORTD.PDR.BIT.B7 = DEF_IN;                                  /* IRQ7                                                 */
    PORT9.PDR.BIT.B2 = DEF_IN;                                  /* SCI7.rx                                              */
//    PORT9.PDR.BIT.B3 = DEF_IN;                                  /* SCI7.SSL                                             */
    PORT9.PDR.BIT.B0 = DEF_OUT;                                 /* SCI7.tx                                              */
//    PORT9.PDR.BIT.B1 = DEF_IN;                                  /* SCI7.SCK                                             */

    BSP_IO_Protect(WRITE_ENABLED);
    MPC.P92PFS.BIT.PSEL = 0xA; // RXD1/SMISO1/SSCL1
    MPC.P90PFS.BIT.PSEL = 0xA; // TXD1/SMOSI1/SSDA1

    if (hwFlowControl)
        MPC.P93PFS.BIT.PSEL = 0xB; // CTS1#/RTS1#/SS1#

    BSP_IO_Protect(WRITE_DISABLED);                             /* Disable writing to MPC registers                     */

    PORT9.PMR.BIT.B0 = 1;
    PORT9.PMR.BIT.B2 = 1;
    if (hwFlowControl)
        PORT9.PMR.BIT.B3 = 1; // if using rts/cts


    CPU_SR_ALLOC();

    CPU_CRITICAL_ENTER();                                       /* Disable interrupts so we can ...                     */

    SYSTEM.PRCR.WORD = 0xA507;                                  /* ... disable register protection.                     */

    MSTP(SCI7) = 0u;                                           /* Wake RSPI1 unit from standby mode                    */

    SYSTEM.PRCR.WORD = 0xA500;                                  /* ... disable register protection.                     */

    CPU_CRITICAL_EXIT();

    IPR(SCI7, RXI7)         = 5;                               /* Set IPL to the next highest setting (more important than wifi) */
    IR (SCI7, RXI7)         = 0;                               /* Clear any pending interrupt.                         */
    IEN(SCI7, RXI7)         = 1u;
    BSP_IntVectSet(98, (CPU_FNCT_VOID)SCI_BSP_UART_Rd_handler);

#if 1
    IPR(ICU, GROUPBL0)         = 4;                               /* Set IPL to the next highest setting (more important than wifi) */
    IR (ICU, GROUPBL0)         = 0;                               /* Clear any pending interrupt.                         */
    IEN(ICU, GROUPBL0)         = 1u;
    ICU.GENBL0.BIT.EN14 = 1;
    ICU.GENBL0.BIT.EN15 = 1;
    BSP_IntVectSet(110, (CPU_FNCT_VOID)SCI_BSP_UART_Err_handler);
#endif
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

void  SCI_BSP_UART_SetCfg (long         baud,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir) {
    CPU_INT32U  pclk;
    uint32_t i;
    uint32_t num_divisors = NUM_DIVISORS_ASYNC;
    uint32_t ratio;
    uint32_t tmp;
    baud_divisor_t const *p_baud_info=async_baud;

    uint32_t divisor;
    uint32_t int_M;
    float    float_M;
    float    error;
    float    abs_error;

    SCI7.SCR.BYTE = 0x00; // disable all interrupts and tx/rx
    SCI7.SCR.BIT.CKE = 0x00; // baud rate on-chip, no clock output
    SCI7.SIMR1.BIT.IICM = 0x0;
    SCI7.SPMR.BIT.CKPH = 0;
    SCI7.SPMR.BIT.CKPOL = 0;
    SCI7.SPMR.BIT.CTSE = hwFlowControl;  // not yet supported
    SCI7.SMR.BYTE = 0;
    SCI7.SMR.BIT.CM = 0; //
    SCI7.SMR.BIT.STOP = 0; // 1 stop bit
    SCI7.SMR.BIT.PE = 0; // no parity
    SCI7.SMR.BIT.CHR = 0; // 8 bit data
    SCI7.SCMR.BIT.SINV = 0;
    SCI7.SCMR.BIT.SMIF = 0;
    SCI7.SCMR.BIT.SDIR = 0;
    SCI7.BRR = 0xFF;
    SCI7.SEMR.BIT.BRME    = 0;
    SCI7.SEMR.BIT.ABCS    = 0;
    SCI7.SEMR.BIT.NFEN    = 0;
    SCI7.SEMR.BIT.BGDM    = 0;
    SCI7.SEMR.BIT.RXDESEL = 0;
    SCI7.SNFR.BYTE = 0;

    pclk = BSP_ClkFreqGet(BSP_CLK_ID_PCLKB);
    /* FIND DIVISOR; table has associated ABCS, BGDM and CKS values */
    /* BRR must be 255 or less */
    /* the "- 1" is ignored in some steps for approximations */
    /* BRR = (PCLK/(divisor * baud)) - 1 */
    /* BRR = (ratio / divisor) - 1 */
    ratio = pclk/baud;
    for(i=0; i < num_divisors; i++)
    {
        if (ratio < (uint32_t)(p_baud_info[i].divisor * 256))
        {
            break;
        }
    }

    /* RETURN IF BRR WILL BE >255 OR LESS THAN 0 */
    if (i == num_divisors)
    {
        return;           // impossible baud rate requested; return 100% error
    }

    divisor = (uint32_t)p_baud_info[i].divisor;
    tmp = ratio/(divisor);      // tmp = PCLK/(baud * divisor) = BRR+1 = N+1
    if(0 == tmp)
    {
        return;            // illegal value; return 100% error
    }

    /* SET BRR, ABCS, BDGM, and CKS */
    tmp = ratio / (divisor/2);  // divide by half the divisor

    /* if odd, "round up" by ignoring -1; divide by 2 again for rest of divisor */
    SCI7.BRR = (uint8_t)((tmp & 0x01) ? (tmp/2) : ((tmp/2)-1));
    SCI7.SEMR.BIT.ABCS = p_baud_info[i].abcs;
    SCI7.SEMR.BIT.BGDM = p_baud_info[i].bgdm;
    SCI7.SMR.BIT.CKS = p_baud_info[i].cks;

    /* CALCULATE BIT RATE ERROR.
     * RETURN IF ERROR LESS THAN 1% OR IF IN SYNCHRONOUS/SSPI MODE.
     */
    tmp = ratio/(divisor);      // tmp = PCLK/(baud * divisor) = BRR+1 = N+1
    error = ( ((float)pclk / ((baud * divisor) * tmp)) - 1) * 100;
    abs_error  = (error < 0) ? (-error) : error;

    if (abs_error <= 1.0)
    {
        SCI7.SEMR.BIT.BRME = 0;          // disable MDDR
        SCI7.SEMR.BIT.NFEN = 1;
        SCI7.SEMR.BIT.RXDESEL = 1; //?
        SCI7.SCR.BIT.RIE = 1;
        SCI7.SCR.BIT.RE = 1;
        return ;
    }

    /* CALCULATE M ASSUMING A 0% ERROR then WRITE REGISTER */
    SCI7.BRR = (uint8_t)(tmp-1);
    float_M = ((float)((baud * divisor) * 256) * tmp) / pclk;
    float_M *= 2;
    int_M = (uint32_t)float_M;
    int_M = (int_M & 0x01) ? ((int_M/2) + 1) : (int_M/2);

    SCI7.MDDR = (uint8_t)int_M;      // write M
    SCI7.SEMR.BIT.BRME = 1;          // enable MDDR
    error = (( (float)(pclk) / (((divisor * tmp) * baud) * ((float)(256)/int_M)) ) - 1) * 100;


    SCI7.SEMR.BIT.NFEN = 1;
    SCI7.SEMR.BIT.RXDESEL = 1; //?
    SCI7.SCR.BIT.RIE = 1;
    SCI7.SCR.BIT.RE = 1;
}

volatile static char s_tx_in_progress = 0;

int SCI_BSP_UART_WrRd(CPU_INT08U * data, void * ignored, int bytes) {
    int i;
    int ret = 0;

    SCI7.SCR.BIT.TEIE = 0;
    s_tx_in_progress = 1;
    SCI7.SCR.BIT.TIE = 1;
    SCI7.SCR.BIT.TE = 1;
    for (i = 0; i < bytes; i++) {
        while (!IR(SCI7, TXI7));
        SCI7.TDR = *data++;
        IR(SCI7, TXI7) = 0;
    }
    SCI7.SCR.BIT.TIE = 0;
    SCI7.SCR.BIT.TEIE = 1;

    return ret;
}

volatile uint8_t current_sonar_reading = 0;
volatile char g_cli_cmd[65];
volatile char g_cli_processing = 0;

/*
* this function is called by the uart rx ISR.
* this function is NOT re-entrant.
* so, this function must be called while interrupts are disabled.
* so, there is a race condition that we can miss an interrupt
* that occurs after leaving this function but before re-enabling interrupts?
*/
#pragma  interrupt   SCI_BSP_UART_Rd_handler
CPU_ISR SCI_BSP_UART_Rd_handler() {
    OS_ERR err_os;
    static char sonar[5];
    static char cli[65];
    static int i = 0;

    OSIntEnter();                                               /* Notify uC/OS-III or uC/OS-II of ISR entry            */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */

    IEN(SCI7, RXI7) = 0;                                        /* Enable interrupt source.                             */

    if (!s_provisioning) {
        sonar[i] = SCI7.RDR;
        if (SCI7.SSR.BIT.ORER || SCI7.SSR.BIT.PER || SCI7.SSR.BIT.FER) {
            SCI7.SSR.BIT.ORER = 0;
            SCI7.SSR.BIT.PER = 0;
            SCI7.SSR.BIT.FER = 0;
        } else {
            if (!i) {
              if (sonar[i] == 'R') {
                i++;
              }
            } else {
              if (sonar[i] == '\r') {
                  if (i == 4) {
                      current_sonar_reading = sonar[1] * 100 + sonar[2] * 10 + sonar[3] - '0' * 111;
                      if (current_sonar_reading == 255) {
                          current_sonar_reading = 0;
                      } else
                          OSFlagPost(&sonar_grp, SONAR_GROUP_READING_READY, OS_OPT_POST_FLAG_SET, &err_os);
                  }
                i = 0;
              } else {
                i++;
                if (i == 5)
                  i = 0;
              }
            }
        }
    } else {
        cli[i++] = SCI7.RDR;
        if (g_cli_processing)
            i = 0;
        else {
            if (cli[i - 1] == '\r') {
                if (!s_tx_in_progress)
                    SCI_BSP_UART_WrRd("\r\n", NULL, 2);
                cli[i - 1] = '\0';
                Str_Copy((char *)g_cli_cmd, cli);
                g_cli_processing = 1;
                i = 0;
                OSFlagPost(&sonar_grp, SONAR_GROUP_READING_READY, OS_OPT_POST_FLAG_SET, &err_os);
            } else {
                if (!s_tx_in_progress) {
                    // transform mac "del" to windows "bs"
                    if (cli[i - 1] == '\x7f')
                        cli[i - 1] = '\x08';
                    // handle backspace
                    if (cli[i - 1] == '\x08') {
                        if (i > 1) {
                            i -= 2;
                            SCI_BSP_UART_WrRd("\x08\x7f", NULL, 2);
                        } else
                            i--;
                    } else
                        SCI_BSP_UART_WrRd((CPU_INT08U *)&cli[i - 1], NULL, 1);
                }
            }
            if (i == 65)
                i--;
        }
    }

    IR(SCI7, RXI7)  = 0;                                        /* Clear any pending ISR.                               */
    IEN(SCI7, RXI7) = 1;                                        /* Enable interrupt source.                             */

    OSIntExit();                                                /* Notify uC/OS-III or uC/OS-II of ISR exit             */
}

#pragma interrupt  SCI_BSP_UART_Err_handler
CPU_ISR SCI_BSP_UART_Err_handler() {
    OSIntEnter();                                               /* Notify uC/OS-III or uC/OS-II of ISR entry            */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */

    IEN(ICU, GROUPBL0) = 0;                                        /* Enable interrupt source.                             */

    if (ICU.GRPBL0.BIT.IS14) {
        SCI7.SCR.BIT.TEIE = 0;
        SCI7.SCR.BIT.TE = 0;
        SCI7.SCR.BIT.TIE = 0;
        s_tx_in_progress = 0;
    }
    if (ICU.GRPBL0.BIT.IS15) {
        char c = SCI7.RDR;  // dummy read of overflow data to clear rdr
        SCI7.SSR.BIT.ORER = 0;
        SCI7.SSR.BIT.PER = 0;
        SCI7.SSR.BIT.FER = 0;
    }

    IR(ICU, GROUPBL0)  = 0;                                        /* Clear any pending ISR.                               */
    IEN(ICU, GROUPBL0) = 1;                                        /* Enable interrupt source.                             */

    OSIntExit();                                                /* Notify uC/OS-III or uC/OS-II of ISR exit             */
}
