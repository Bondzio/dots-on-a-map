#include  <os.h>
#include  <bsp_sys.h>
#include  "iodefine.h"
#include  <m1_bsp.h>
#include <stdlib.h>

#define COUNT_TIMEOUT 200000L


static char hwFlowControl = 0;

static rbd_t _rbd;
static char _rbmem[RING_BUFFER_SIZE];

void SCI_BSP_UART_init(char hwFC) {
    OS_ERR err_os;
    rb_attr_t attr = {sizeof(_rbmem[0]), ARRAY_SIZE(_rbmem), _rbmem};
 
    /* Initialize the ring buffer */
    ASSERT(ring_buffer_init(&_rbd, &attr) == 0);

    hwFlowControl = hwFC;
//    PORT1.PDR.BIT.B7 = DEF_IN;                                  /* IRQ7                                                 */
    PORTE.PDR.BIT.B2 = DEF_IN;                                  /* SCI1.rx                                              */
    PORTE.PDR.BIT.B3 = DEF_IN;                                  /* SCI1.SSL                                             */
    PORTE.PDR.BIT.B1 = DEF_OUT;                                 /* SCI1.tx                                              */
//    PORT2.PDR.BIT.B7 = DEF_IN;                                  /* SCI1.SCK                                             */    
    MPC.PWPR.BIT.B0WI  = 0;                                     /* Disable write protection for MPC.                    */
    MPC.PWPR.BIT.PFSWE = 1;
    MPC.PE2PFS.BIT.PSEL = 0xC; // RXD12/SMISO12/SSCL12
    MPC.PE1PFS.BIT.PSEL = 0xC; // TXD12/SMOSI12/SSDA12
    if (hwFlowControl)
        MPC.PE3PFS.BIT.PSEL = 0xC; // CTS12#/RTS12#/SS12#
    MPC.PWPR.BIT.PFSWE = 0;                                     /* Re-enable write protection for MPC.                  */
    MPC.PWPR.BIT.B0WI  = 1;
    PORTE.PMR.BIT.B2 = 1;
    PORTE.PMR.BIT.B1 = 1;
    if (hwFlowControl)
        PORTE.PMR.BIT.B3 = 1; // if using rts/cts
#if 0
    OSQCreate(&RxUARTQ, "RxUART Queue", RX_UART_BUFFER_COUNT, &err_os);
    OSMemCreate((OS_MEM    *)&RxUARTBufferPool,                   
                (CPU_CHAR  *)"rxUARTBuffers",                 
                (void      *)&rxUARTBuffers[0][0],      
                (OS_MEM_QTY ) RX_UART_BUFFER_COUNT,                      
                (OS_MEM_SIZE) RX_UART_BUFFER_SIZE,                            
                (OS_ERR    *)&err_os);
#endif
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();                                       /* Disable interrupts so we can ...                     */
    SYSTEM.PRCR.WORD = 0xA507;                                  /* ... disable register protection.                     */

    MSTP(SCI12) = 0u;

    SYSTEM.PRCR.WORD = 0xA500;                                  /* Re-enable register write protection.                 */

    CPU_CRITICAL_EXIT();
    
#if 1 || (defined(LORA_DEMO) && defined(LORA_EP)) // EP

    IPR(SCI12, RXI12)         = 2;                               /* Set IPL to the next highest setting (more important than wifi) */
    IR (SCI12, RXI12)         =   0;                               /* Clear any pending interrupt.                         */
    IEN(SCI12, RXI12) = 1u;

    IPR(SCI12, ERI12)         = 2;                               /* Set IPL to the next highest setting (more important than wifi) */
    IR (SCI12, ERI12)         =   0;                               /* Clear any pending interrupt.                         */
    IEN(SCI12, ERI12) = 1u;
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
void  SCI_BSP_UART_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir)
{
    CPU_INT32U  pclk;
    int brr_n;
    int n = 0, i;
    long newFreq;
    static int set = 0;

    SCI12.SCR.BYTE = 0x00; // disable all interrupts and tx/rx
    SCI12.SCR.BIT.CKE = 0x00; // baud rate on-chip, no clock output
    SCI12.SIMR1.BIT.IICM = 0x0;
    SCI12.SPMR.BIT.CKPH = 0;
    SCI12.SPMR.BIT.CKPOL = 0;
    SCI12.SPMR.BIT.CTSE = hwFlowControl;  // not yet supported
    SCI12.SMR.BIT.CM = 0; // 
    SCI12.SMR.BIT.STOP = 0; // 1 stop bit
    SCI12.SMR.BIT.PE = 0; // no parity
    SCI12.SMR.BIT.CHR = 0; // 8 bit data
    
    pclk = BSP_SysPeriphClkFreqGet(CLK_ID_PCLKB);
    long ratio = pclk/freq;

    for (i=0; i < NUM_DIVISORS_ASYNC; i++) {
        if (ratio < (long)(async_baud[i].divisor * 256))
            break;
    }

    /* RETURN IF BRR WILL BE >255 OR LESS THAN 0 */

    if (i == NUM_DIVISORS_ASYNC)
        return;           // impossible baud rate requested; return 100% error

    /* ROUND UP/DOWN BRR AND SET BAUD RATE RELATED REGISTERS */

    /* divide ratio by only half the divisor and see if odd number */

    long tmp = ratio/((long)async_baud[i].divisor/2);

    /* if odd, "round up" by ignoring -1; divide by 2 again for rest of divisor */

    SCI12.BRR = (unsigned char)((tmp & 0x01) ? (tmp/2) : ((tmp/2)-1));

    SCI12.SEMR.BIT.ABCS = async_baud[i].abcs;

    SCI12.SMR.BIT.CKS = async_baud[i].cks;

    /* CALCULATE AND RETURN BIT RATE ERROR */
    // back calculate what PCLK should be:
    long pclk_calc = (long)(SCI12.BRR +1) * (long)async_baud[i].divisor * freq;

    /* error represented in 0.1% increments */
    int bit_err = (long)(((pclk - pclk_calc) * 1000L) / pclk_calc);
    
    
    SCI12.SEMR.BIT.NFEN = 1;
    SCI12.SEMR.BIT.RXDESEL = 1; //?
    SCI12.SCR.BIT.RIE = 1;
    SCI12.SCR.BIT.RE = 1;
}

/*
* this function is called by the uart rx ISR.
* this function is NOT re-entrant.
* so, this function must be called while interrupts are disabled.
* so, there is a race condition that we can miss an interrupt 
* that occurs after leaving this function but before re-enabling interrupts?
*/
void SCI_BSP_UART_Rd_handler() {
    char c;
    
    IEN(SCI12, RXI12) = 0;                                        /* Enable interrupt source.                             */

    // IR is already cleared...
    if (SCI12.SSR.BIT.ORER || SCI12.SSR.BIT.PER || SCI12.SSR.BIT.FER) {
        if (SCI12.SSR.BIT.ORER)
            c = SCI12.RDR;  // dummy read of overflow data to clear rdr
        SCI12.SSR.BIT.ORER = 0;
        SCI12.SSR.BIT.PER = 0;
        SCI12.SSR.BIT.FER = 0;
    } else {
        c = SCI12.RDR;
        ring_buffer_put(_rbd, &c);
//        parse_NMEA(c);
    }
#if 1
     while (IR(SCI12, RXI12)) {
        if (SCI12.SSR.BIT.ORER || SCI12.SSR.BIT.PER || SCI12.SSR.BIT.FER) {
            if (SCI12.SSR.BIT.ORER)
                c = SCI12.RDR;  // dummy read of overflow data to clear rdr
            SCI12.SSR.BIT.ORER = 0;
            SCI12.SSR.BIT.PER = 0;
            SCI12.SSR.BIT.FER = 0;
        } else {
            c = SCI12.RDR;
        ring_buffer_put(_rbd, &c);
//            parse_NMEA(c);
        }
    }
#endif
    IR(SCI12, RXI12) = 0;
    IEN(SCI12, RXI12) = 1;                                        /* Enable interrupt source.                             */
}

unsigned char SCI_BSP_Status() {
    return SCI12.SSR.BYTE;
}

void SCI_BSP_Clear_Status() {
    char c = SCI12.RDR;  // dummy read of overflow data to clear rdr
    IEN(SCI12, ERI12) = 0;                                        /* Enable interrupt source.                             */

    SCI12.SSR.BIT.ORER = 0;
    SCI12.SSR.BIT.PER = 0;
    SCI12.SSR.BIT.FER = 0;

    IR(SCI12, ERI12)  = 0;                                        /* Clear any pending ISR.                               */
    IEN(SCI12, ERI12) = 1;                                        /* Enable interrupt source.                             */
}

int SCI_BSP_UART_WrRd(CPU_INT08U * data, CPU_INT08U * read, int bytes) {
    int i;
    int rdwr = (data == DEF_NULL) ? 1 : 0;
    int ret = 0;
    long count = 0;
    OS_ERR err;
    char * buf;
    int bytes_read = 0;
    static char * currBuff = 0;
    static int buffOffset = 0;
    static OS_MSG_SIZE msg_size;
    char c;

    if (rdwr) {
        while (bytes_read < bytes) {
            if (!ring_buffer_get(_rbd, &c)) {
                *read++ = c;
                bytes_read++;
            } else 
              return bytes_read ? bytes_read : -1;
        }
        ret = bytes_read;
    } else {
        SCI12.SCR.BIT.TIE = 1;
        SCI12.SCR.BIT.TE = 1;
        for (i = 0; i < bytes; i++) {
            while (!IR(SCI12, TXI12));
            SCI12.TDR = *data++;
            IR(SCI12, TXI12) = 0;
        }
        SCI12.SCR.BIT.TIE = 0;
        SCI12.SCR.BIT.TEIE = 1;
        while (!IR(SCI12, TEI12));
        SCI12.SCR.BIT.TEIE = 0;
        SCI12.SCR.BIT.TE = 0;
        SCI12.SCR.BIT.TIE = 0;
    }

    return ret;
}

void SCI_BSP_UART_GPIO(int val) {
    PORT2.PODR.BIT.B6 = val;
}

char SCI_BSP_UART_GPIO_read(char pin) {
    switch (pin) {
    case 7:
        return PORT1.PIDR.BIT.B7;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    default:
        return 0;
    }
}                           