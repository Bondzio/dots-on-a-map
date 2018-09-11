#include  <os.h>
#include  <bsp_sys.h>
#include  "iodefine.h"
#include  <m1_bsp.h>
#include <riic_i2c_bsp.h>


void RIIC_BSP_I2C_init() {
    PORT1.PDR.BIT.B2 = DEF_IN;                                  /* RI2C.rx                                              */
    PORT1.PDR.BIT.B3 = DEF_IN;                                  /* RI2C.tx                                              */
    PORT1.ODR0.BIT.B4 = 0x1;
    PORT1.ODR0.BIT.B6 = 0x1;
    MPC.PWPR.BIT.B0WI  = 0;                                     /* Disable write protection for MPC.                    */
    MPC.PWPR.BIT.PFSWE = 1;
    MPC.P12PFS.BIT.PSEL = 0x0f; // SCL
    MPC.P13PFS.BIT.PSEL = 0x0f; // SDA
    MPC.PWPR.BIT.PFSWE = 0;                                     /* Re-enable write protection for MPC.                  */
    MPC.PWPR.BIT.B0WI  = 1;
    PORT1.PMR.BIT.B3 = 1;
    PORT1.PMR.BIT.B2 = 1;

    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();                                       /* Disable interrupts so we can ...                     */
    SYSTEM.PRCR.WORD = 0xA507;                                  /* ... disable register protection.                     */

    MSTP(RIIC0) = 0u;                                           

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
void  RIIC_BSP_I2C_SetCfg (long         freq,
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

    RIIC0.ICCR1.BIT.ICE = 0;
    RIIC0.ICCR1.BIT.IICRST = 1;    
    RIIC0.ICCR1.BIT.ICE = 1;
    RIIC0.ICMR1.BIT.CKS = 2;
    RIIC0.ICBRH.BIT.BRH = 24;
    RIIC0.ICBRL.BIT.BRL = 29;
    RIIC0.ICMR3.BIT.ACKBR = 1;
    RIIC0.ICMR3.BIT.ACKWP = 1;
    RIIC0.ICMR3.BIT.ACKBT = 0;
    //RIIC0.ICMR2.BIT.TMWE = 1;
    RIIC0.ICMR2.BIT.SDDL = cpha;
    //timeout stuff
    RIIC0.ICFER.BIT.MALE = 0;
    //ICIER = ?;
    RIIC0.ICCR1.BIT.IICRST = 0;
}

OS_SEM riic_i2c_sem;

void  RIIC_BSP_I2C_Lock (void)
{
    OS_ERR err;
    CPU_TS ts;
    OSSemPend(&riic_i2c_sem, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
}

void  RIIC_BSP_I2C_Unlock (void)
{
    OS_ERR err;
    
    OSSemPost(&riic_i2c_sem, OS_OPT_POST_1, &err);
}

static void RIIC_BSP_I2C_Stop() {
    RIIC0.ICSR2.BIT.STOP = 0;
    RIIC0.ICCR2.BIT.SP = 1;
    while (!RIIC0.ICSR2.BIT.STOP);
    RIIC0.ICSR2.BIT.NACKF = 0;
    RIIC0.ICSR2.BIT.STOP = 0;
}

static int RIIC_BSP_I2C_Start() {
    if (RIIC0.ICCR2.BIT.BBSY)
        return -1;
    RIIC0.ICCR2.BIT.ST = 1;
    return 0;
}

static int _RIIC_BSP_I2C_Wr(int address, CPU_INT08U * data, int bytes) {
    do {
        if (RIIC0.ICSR2.BIT.NACKF) {
            return -1;
        }
    } while (!RIIC0.ICSR2.BIT.TDRE);
    RIIC0.ICDRT = (address << 1) | 0;
    
    for (; bytes > 0; --bytes) {
        do {
            if (RIIC0.ICSR2.BIT.NACKF) {
                return -2;
            }
        } while (!RIIC0.ICSR2.BIT.TDRE);
        RIIC0.ICDRT = *data++;
    }
    int timeout = 20000;
    while (!RIIC0.ICSR2.BIT.TEND) {
        if (!timeout--)
            return -1;
    }
    return 0;
}

static int RIIC_BSP_I2C_Wr(int address, CPU_INT08U * data, int bytes) {
    int ret = 0;
    
    ret = RIIC_BSP_I2C_Start();
    if (ret)
        return ret;
    ret = _RIIC_BSP_I2C_Wr(address, data, bytes);
    
    RIIC_BSP_I2C_Stop();
    
    return ret;
}

static int _RIIC_BSP_I2C_Rd_Send_Address(int address) {
    int temp;
    OS_ERR err_os;
    
    while (!RIIC0.ICSR2.BIT.TDRE);
    OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    // TODO: HACK why do we need delay here?
    RIIC0.ICDRT = (address << 1) | 1;
    while (!RIIC0.ICSR2.BIT.RDRF);
    if (RIIC0.ICSR2.BIT.NACKF) {
        RIIC0.ICSR2.BIT.STOP = 0;
        RIIC0.ICCR2.BIT.SP = 1;
        temp = RIIC0.ICDRR; // dummy read
        return -1;
    }
    return 0;
}

static int _RIIC_BSP_I2C_Rd_1_2(int address, CPU_INT08U * read, int bytes) {
    int temp;
    int ret = 0;

    ret = _RIIC_BSP_I2C_Rd_Send_Address(address);
    if (!ret) {
        RIIC0.ICMR3.BIT.WAIT = 1;
        if (bytes == 2) {
            temp = RIIC0.ICDRR; // dummy read
            while (!RIIC0.ICSR2.BIT.RDRF);
        }
        RIIC0.ICMR3.BIT.RDRFS = 1;
        RIIC0.ICMR3.BIT.ACKBT = 1;
        if (bytes == 1)
            temp = RIIC0.ICDRR;
        else
            *read++ = RIIC0.ICDRR;
        while (!RIIC0.ICSR2.BIT.RDRF);
        RIIC0.ICSR2.BIT.STOP = 0;
        RIIC0.ICCR2.BIT.SP = 1;
        *read++ = RIIC0.ICDRR;
        RIIC0.ICMR3.BIT.ACKBT = 1;
        RIIC0.ICMR3.BIT.WAIT = 0;
    }
    while (!RIIC0.ICSR2.BIT.STOP);    
    RIIC0.ICMR3.BIT.RDRFS = 0;
    RIIC0.ICMR3.BIT.ACKBT = 0;
    RIIC0.ICSR2.BIT.NACKF = 0;
    RIIC0.ICSR2.BIT.STOP = 0;
    
    return ret;
}

static int RIIC_BSP_I2C_Rd_1_2(int address, CPU_INT08U * read, int bytes) {
    int ret = 0;

    ret = RIIC_BSP_I2C_Start();
    if (ret)
        return ret;
    
    return _RIIC_BSP_I2C_Rd_1_2(address, read, bytes);
}

static int _RIIC_BSP_I2C_Rd_3_plus(int address, CPU_INT08U * read, int bytes) {
    int temp;
    int i;
    int ret = 0;
    
    ret = _RIIC_BSP_I2C_Rd_Send_Address(address);
    if (!ret) {
        temp = RIIC0.ICDRR; // dummy read
        for (i = 0; i < bytes - 2; i++) {
            while (!RIIC0.ICSR2.BIT.RDRF);
            if (i == (bytes - 3))
                RIIC0.ICMR3.BIT.WAIT = 1;
            *read++ = RIIC0.ICDRR;
        }
        while (!RIIC0.ICSR2.BIT.RDRF);  // ?
        RIIC0.ICMR3.BIT.RDRFS = 1;
        RIIC0.ICMR3.BIT.ACKBT = 1;
        *read++ = RIIC0.ICDRR;
        while (!RIIC0.ICSR2.BIT.RDRF);
        RIIC0.ICSR2.BIT.STOP = 0;
        RIIC0.ICCR2.BIT.SP = 1;
        *read++ = RIIC0.ICDRR;
        RIIC0.ICMR3.BIT.ACKBT = 1;
        RIIC0.ICMR3.BIT.WAIT = 0;
    }
    while (!RIIC0.ICSR2.BIT.STOP);    
    RIIC0.ICMR3.BIT.RDRFS = 0;
    RIIC0.ICMR3.BIT.ACKBT = 0;
    RIIC0.ICSR2.BIT.NACKF = 0;
    RIIC0.ICSR2.BIT.STOP = 0;
    
    return ret;
}

static int RIIC_BSP_I2C_Rd_3_plus(int address, CPU_INT08U * read, int bytes) {
    int ret = 0;

    ret = RIIC_BSP_I2C_Start();
    if (ret)
        return ret;
    
    return _RIIC_BSP_I2C_Rd_3_plus(address, read, bytes);
}

int RIIC_BSP_I2C_WrAndRd(int address, CPU_INT08U * data, int writeBytes, CPU_INT08U * read, int readBytes) {
    int ret = 0;

    ret = RIIC_BSP_I2C_Start();
    if (ret)
        return ret;
    ret = _RIIC_BSP_I2C_Wr(address, data, writeBytes);
    
    if (ret) {
        RIIC_BSP_I2C_Stop();
        return ret;
    }

    if (RIIC0.ICSR2.BIT.START)
        RIIC0.ICSR2.BIT.START = 0;
    RIIC0.ICCR2.BIT.RS = 1;
    
    if (readBytes > 2)
        return _RIIC_BSP_I2C_Rd_3_plus(address, read, readBytes);
    else
        return _RIIC_BSP_I2C_Rd_1_2(address, read, readBytes);
}

static int RIIC_BSP_I2C_Rd(int address, CPU_INT08U * read, int bytes) {
    if (bytes <= 2) {
        return RIIC_BSP_I2C_Rd_1_2(address, read, bytes);
    }
    return RIIC_BSP_I2C_Rd_3_plus(address, read, bytes);
}

int RIIC_BSP_I2C_WrRd(int address, CPU_INT08U * data, CPU_INT08U * read, int bytes) {
    if (data == DEF_NULL)
        return RIIC_BSP_I2C_Rd(address, read, bytes);
    else if (read == DEF_NULL)
        return RIIC_BSP_I2C_Wr(address, data, bytes);
    return 0;
}
