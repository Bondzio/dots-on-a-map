/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*                          (c) Copyright 2016; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               This BSP is provided in source form to registered licensees ONLY.  It is
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
*                                          SPI BUS FUNCTIONS
*
* Filename      : bsp_spi.c
* Version       : V1.00.00
* Programmer(s) : JPC
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <iorx651.h>
#include <bsp.h>
#include  <bsp_clk.h>
#include  <bsp_spi.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
                                                                /* IO Pin Directions                                    */
#define  DEF_IN                 0
#define  DEF_OUT                1



/*
*********************************************************************************************************
*********************************************************************************************************
*                                      SPI DEVICE DRIVER FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

// general I/O pin configuration for all PMOD
void  BSP_PMOD_General_Init(void)
{
    PMODRST_PDR = DEF_OUT;                                 /* PMODRST                                              */
    PMOD_PIN9_PDR = DEF_OUT;                                 /* PMOD9                                                */
    PMOD_PIN10_PDR = DEF_OUT;                                 /* PMOD10                                               */

    PMOD_PIN9_PODR = 1;                                      /* Set PMOD9 high so LL part doesn't go to bootloader   */
}

// reset
void BSP_PMOD_Reset(void)
{
    PMODRST_PODR = 0;                                      /* De-assert chip PWD.                                  */
    KAL_Dly(1);

    PMODRST_PODR = 1;                                      /* Assert chip PWD.                                     */
    KAL_Dly(100);
}

// for PMOD3
void  RSPI_BSP_SPI_Init(void)
{
                                                                /* Configure I/O pin directions.                        */
    PMOD3_PIN7_PDR = DEF_OUT;                                 /* IRQ6                                                 */
    PMOD3_PIN1_PDR = DEF_OUT;                                 /* SSLA1                                                */

                                                                /* Configure pin modes. 0 = GPIO, 1 = Peripheral.       */
    PMOD3_PIN1_PMR = 0;                                       /* PA[0] as GPIO for PMOD2 chip select                  */

                                                                /* Set chip selects to inactive.                        */
    PMOD3_PIN1_PODR = 1;
}

// for PMOD2,3 general setup
void  BSP_RSPI_General_Init (void)
{
                                                                /* Configure I/O pin directions.                        */
    RSPI0_MOSIA_PDR = DEF_OUT;                                 /* MOSIA                                                */
    RSPI0_MISOA_PDR = DEF_IN;                                  /* MISOA                                                */
    RSPI0_RSPCKA_PDR = DEF_OUT;                                 /* RSPCKA                                               */

    BSP_IO_Protect(WRITE_ENABLED);                              /* Enable writing to Multi-Function Pin Controller      */

    RSPI0_MOSIA_PFS_PSEL = 0xD;                                  /* Select MOSIA  function for PC[6].                    */
    RSPI0_MISOA_PFS_PSEL = 0xD;                                  /* Select MISOA  function for PC[7].                    */
    RSPI0_RSPCKA_PFS_PSEL = 0xD;                                  /* Select RSPCKA function for PC[5].                    */

    BSP_IO_Protect(WRITE_DISABLED);                             /* Disable writing to MPC registers                     */

    
                                                                /* Configure pin modes. 0 = GPIO, 1 = Peripheral.       */
    RSPI0_MOSIA_PMR = 1;
    RSPI0_MISOA_PMR = 1;
    RSPI0_RSPCKA_PMR = 1;

    
    CPU_SR_ALLOC();
    CPU_CRITICAL_ENTER();
    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_ENABLED);        /* Enable writing to Clock Generation registers.        */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_ENABLED);         /* Enable writing to Operation Mode registers.          */

    MSTP(RSPI0) = 0u;                                           /* Wake RSPI0 unit from standby mode                    */
    MSTP(CRC) = 0u;                                           /* Wake RSPI0 unit from standby mode                    */

    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_DISABLED);       /* Disable writing to Clock Generation registers.       */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_DISABLED);        /* Disable writing to Operation Mode registers.         */
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                          BSP_SPI_ChipSel()
*
* Description : Enable chip select on an SPI slave.
*
* Argument(s) :
*
* Return(s)   : none.
*********************************************************************************************************
*/

void  BSP_SPI_ChipSel (CPU_INT08U  bus_id, CPU_INT08U  slave_id, CPU_BOOLEAN  en)
{
    CPU_INT08U  pin_val;


    //ASSERT(bus_id == 0);
    //ASSERT(slave_id == BSP_SPI_SLAVE_ID_PMOD3);

                                                                /* Set chip select pin value                            */
    pin_val = (en ? 0 : 1);

    switch (slave_id) {
        case BSP_SPI_SLAVE_ID_PMOD3:
             PMOD3_PIN1_PODR = pin_val;
             break;
    }
}


void  BSP_SPI_Cfg (CPU_INT08U               bus_id,
                   BSP_SPI_BUS_CFG  const  *p_cfg)
{
    CPU_INT32U  pclk;
    CPU_INT08U  spbr;

                                                                /* Set MOSI idle transfer value to '1'.                 */
    RSPI0.SPPCR.BIT.MOIFV = 1;
    RSPI0.SPPCR.BIT.MOIFE = 1;

    RSPI0.SPCR.BIT.SPE  = 0;                                    /* Disable SPI module during initialization.            */
    RSPI0.SPCR.BIT.MSTR = 1;                                    /* Enable Master mode.                                  */

                                                                /* Set baud rate dividers                               */
    RSPI0.SPCMD0.BIT.BRDV = 0;                                  /* N = 0                                                */

    pclk = BSP_ClkFreqGet(BSP_CLK_ID_PCLKA);
    spbr = (CPU_INT08U)(((double)pclk / (p_cfg->SClkFreqMax * 2)) - 1);
    RSPI0.SPBR = spbr;


    if ( p_cfg->CPHA == 0) {
        RSPI0.SPCMD0.BIT.CPHA = 0;                              /* Data variation on odd edge, data sampling on even edge. */
    } else {
        RSPI0.SPCMD0.BIT.CPHA = 1;                              /* Data variation on even edge, data sampling on odd edge. */
    }

    if (p_cfg->CPOL == 0) {
        RSPI0.SPCMD0.BIT.CPOL = 0;                              /* RSPCK is high when idle.                             */
    } else {
        RSPI0.SPCMD0.BIT.CPOL = 1;                              /* RSPCK is high when idle.                             */
    }

                                                                /* Select transfer unit length.                         */
    switch (p_cfg->BitsPerFrame) {
        case 8:
             RSPI0.SPCMD0.BIT.SPB = 0x4;                        /* Data length = 8 bits.                                */
             RSPI0.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case 16:
             RSPI0.SPCMD0.BIT.SPB = 0xF;
             RSPI0.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case 32:
             RSPI0.SPCMD0.BIT.SPB = 0x2;
             RSPI0.SPDCR.BIT.SPLW = 1;                          /* SPDR is accessed in longwords.                       */
             break;

        default:
             break;
    }

                                                                /* Select transfer shift direction.                     */
    switch (p_cfg->LSBFirst) {
        case 0:
             RSPI0.SPCMD0.BIT.LSBF = 0;
             break;

        case 1:
             RSPI0.SPCMD0.BIT.LSBF = 1;
             break;

        default:
             break;
    }

    RSPI0.SPSCR.BIT.SPSLN = 0x0;                                /* Just use Command Register 0 (SPCMD0).                */

    RSPI0.SPCR2.BIT.SCKASE = 0;
}


void  BSP_SPI_Xfer (CPU_INT08U          bus_id,
                    CPU_INT08U  const  *p_buf_wr,
                    CPU_INT08U         *p_buf_rd,
                    CPU_INT16U          wr_rd_len)
{
    CPU_INT32U              i;
    CPU_INT32U              data;
    CPU_INT08U              ptr_incr_wr;
    CPU_INT08U              ptr_incr_rd;
    CPU_INT32U              dummy_wr;
    CPU_INT32U              dummy_rd;
    CPU_INT32U              xfer_cnt;
    CPU_SR_ALLOC();
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

    RSPI0.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */
    RSPI0.SPCR.BIT.SPTIE  = 1;                                  /* Enable transmit interrupt request.                   */
    RSPI0.SPCR.BIT.SPRIE  = 1;                                  /* Enable receive  interrupt request.                   */
    RSPI0.SPCR.BIT.SPE    = 1;                                  /* Enable RSPI.                                         */

    CPU_CRITICAL_ENTER();
    for (i = 0; i < xfer_cnt; ++i) {
        IR(RSPI0, SPTI0) = 0u;
        IR(RSPI0, SPRI0) = 0u;

                                                                /* Copy write data into SPI data register.              */
        data              = *(CPU_INT08U *)p_buf_wr;
        RSPI0.SPDR.WORD.H =  (CPU_INT08U  )data;

        p_buf_wr += ptr_incr_wr;

        while (IR(RSPI0, SPTI0) != 1u);                         /* Wait for SPTI interrupt.                             */
        while (IR(RSPI0, SPRI0) != 1u);                         /* Wait for SPRI interrupt.                             */

                                                                /* Read data from the SPI data register.                */
        data                  = (CPU_INT32U)RSPI0.SPDR.WORD.H;
      *(CPU_INT08U *)p_buf_rd = (CPU_INT08U)data;

        p_buf_rd += ptr_incr_rd;
    }
    CPU_CRITICAL_EXIT();

    RSPI0.SPCR.BIT.SPTIE  = 0;                                  /* Disable transmit interrupt request.                  */
    RSPI0.SPCR.BIT.SPRIE  = 0;                                  /* Disable receive  interrupt request.                  */

    RSPI0.SPCR2.BIT.SPIIE = 1;                                  /* Re-enable idle interrupt.                            */
    while ((RSPI0.SPSR.BIT.IDLNF) != 0u);                       /* Wait until SPI transfer is complete.                 */

    RSPI0.SPCR.BIT.SPE = 0;                                     /* Disable RSPI.                                        */
    RSPI0.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */
}

void  RSPI0_BSP_SPI_Data (void)
{
    PMOD3_PIN7_PODR = 1;
}

void  RSPI0_BSP_SPI_Command (void)
{
    PMOD3_PIN7_PODR = 0;
}
