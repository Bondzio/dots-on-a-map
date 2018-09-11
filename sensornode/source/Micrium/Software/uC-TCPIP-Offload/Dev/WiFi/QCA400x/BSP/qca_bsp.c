/*
*********************************************************************************************************
*                                            	QCA400x
*
*                          (c) Copyright 2003-2015; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               QCA400x is provided in source form to registered licensees ONLY.  It is
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
*                            QCA400x BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*                                            
*                                           TEMPLATE FILES
*
* Filename      : qca_bsp.c
* Version       : V1.00
* Programmer(s) : AL
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "qca_bsp.h"
#include  "../bsp.h"
#include  <MK70F12.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  QCA_BSP_Start                     (void);

static  void  QCA_BSP_Stop                      (void);

static  void  QCA_BSP_CfgGPIO                   (void);

static  void  QCA_BSP_CfgExtIntCtrl             (void);

static  void  QCA_BSP_SPI_Cfg                   (void);

static  void  QCA_BSP_SPI_Lock                  (void);

static  void  QCA_BSP_SPI_Unlock                (void);

        void  QCA_BSP_SPI_WrRd                  (CPU_INT08U                  *p_buf_wr,
                                                 CPU_INT08U                  *p_buf_rd,
                                                 CPU_INT16U                   len);

static  void  QCA_BSP_SPI_ChipSelEn             (void);

static  void  QCA_BSP_SPI_ChipSelDis            (void);

static  void  QCA_BSP_SPI_SetCfg                (QCA_CFG_SPI_CLK_FREQ         freq,
                                                 QCA_CFG_SPI_CLK_POL          pol,
                                                 QCA_CFG_SPI_CLK_PHASE        pha,
                                                 QCA_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                                 QCA_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir);

static  void  QCA_BSP_ExtIntCtrl                (CPU_BOOLEAN                  en);

static  void  QCA_BSP_ISR_Handler               (void);



/*
*********************************************************************************************************
*                                    QUALCOMM DEVICE BSP INTERFACE
*********************************************************************************************************
*/

const  QCA_DEV_BSP  QCA_BSP_BoardName_ModuleName = {
                                                    &QCA_BSP_Start,
                                                    &QCA_BSP_Stop,
                                                    &QCA_BSP_CfgGPIO,
                                                    &QCA_BSP_CfgExtIntCtrl,
                                                    &QCA_BSP_ExtIntCtrl,
                                                    &QCA_BSP_SPI_Cfg,
                                                    &QCA_BSP_SPI_Lock,
                                                    &QCA_BSP_SPI_Unlock,
                                                    &QCA_BSP_SPI_WrRd,
                                                    &QCA_BSP_SPI_ChipSelEn,
                                                    &QCA_BSP_SPI_ChipSelDis,
                                                    &QCA_BSP_SPI_SetCfg
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
*                                           QCA_BSP_Start()
*
* Description : Start wireless hardware.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : QCA_Start() via 'p_dev_bsp->Start()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_Start (void)
{

}


/*
*********************************************************************************************************
*                                            QCA_BSP_Stop()
*
* Description : Stop wireless hardware.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : QCA_Stop() via 'p_dev_bsp->Stop()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_Stop (void)
{

}


/*
*********************************************************************************************************
*                                          QCA_BSP_CfgGPIO()
*
* Description : (1) Configure pins:
*
*                   (a) Configure SPI     pins.
*                   (b) Configure EXT ISR pin.
*                   (c) Configure PWR     pin.
*                   (d) Configure RST     pin.
*
* Argument(s) : None.
*
* Return(s)   : none.
*
* Caller(s)   : QCA_Init() via 'p_dev_bsp->CfgGPIO()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  QCA_BSP_CfgGPIO (void)
{
  
}


/*
*********************************************************************************************************
*                                       QCA_BSP_CfgExtIntCtrl()
*
* Description : Assign extern ISR to the ISR Handler.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : QCA400x driver via 'p_dev_bsp->CfgExtIntCtrl()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_CfgExtIntCtrl (void)
{

}


/*
*********************************************************************************************************
*                                          QCA_BSP_SPI_Cfg()
*
* Description : Initialize SPI registers.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : QCA400x driver via 'p_dev_bsp->SPI_Cfg()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) This function is called only when the wireless network interface is added.
*
*               (2) Several aspects of SPI communication may need to be configured :
*
*                   (a) CLOCK POLARITY & PHASE (CPOL & CPHA).  SPI communication takes place in any of
*                       four modes, depending on the clock phase & clock polarity settings :
*
*                       (1) If CPOL = 0, the clock is low  when inactive.
*
*                           If CPOL = 1, the clock is high when inactive.
*
*                       (2) If CPHA = 0, data is 'read'    on the leading   edge of the clock &
*                                                'changed' on the following edge of the clock.
*
*                           If CPHA = 1, data is 'changed' on the following edge of the clock &
*                                                'read'    on the leading   edge of the clock.
*
*                       The driver should configure the SPI for CPOL = 0, CPHA = 0.  This means that
*                       data (i.e., bits) are read when a low->high clock transition occurs & changed
*                       when a high->low clock transition occurs.
*
*                   (b) TRANSFER UNIT LENGTH.  A transfer unit is always 8-bits.
*
*                   (c) SHIFT DIRECTION.  The most significant bit should always be transmitted first.
*
*                   (d) CLOCK FREQUENCY.  See 'NetDev_SPI_SetClkFreq()  Note #1'.
*
*               (3) The CS (Chip Select) MUST be configured as a GPIO output; it should not be controlled
*                   by the CPU's SPI peripheral.  The functions 'NetDev_SPI_ChipSelEn()' and
*                  'NetDev_SPI_ChipSelDis()' manually enable and disable the CS.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_Cfg (void)
{
    
}


/*
*********************************************************************************************************
*                                          QCA_BSP_SPI_Lock()
*
* Description : Acquire SPI lock.
*
* Argument(s) : none.
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

}


/*
*********************************************************************************************************
*                                         QCA_BSP_SPI_Unlock()
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
* Note(s)     : (1) 'QCA_BSP_SPI_Lock()' will be called before the device driver begins to access the SPI.
*                   The application should NOT use the same bus to access another device until the
*                   matching call to this function has been made.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_Unlock (void)
{

}


/*
*********************************************************************************************************
*                                          QCA_BSP_SPI_WrRd()
*
* Description : Write and read from SPI.
*
* Argument(s) : p_buf_wr    Pointer to buffer to write.
*
*               p_buf_rd    Pointer to buffer for data read.
*
*               wr_rd_len   Number of octets to write and read.
*
*
* Return(s)   : none.
*
* Caller(s)   : QCA400x driver via 'p_dev_bsp->SPI_WrRd()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_WrRd (CPU_INT08U   *p_buf_wr,
                                CPU_INT08U   *p_buf_rd,
                                CPU_INT16U    wr_rd_len)
{

}

/*
*********************************************************************************************************
*                                       QCA_BSP_SPI_ChipSelEn()
*
* Description : Enable wireless device chip select.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_ChipSelEn()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'QCA_BSP_SPI_ChipSelEn()' will be called before the device driver begins to access
*                   the SPI.
*
*               (2) The chip select is typically 'active low'; to enable the card, the chip select pin
*                   should be cleared.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_ChipSelEn (void)
{
   
}


/*
*********************************************************************************************************
*                                       QCA_BSP_SPI_ChipSelDis()
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
* Note(s)     : (1) 'QCA_BSP_SPI_ChipSelDis()' will be called when the device driver finished to access
*                   the SPI.
*
*               (1) The chip select is typically 'active low'; to disable the card, the chip select pin
*                   should be set.
*********************************************************************************************************
*/

static  void  QCA_BSP_SPI_ChipSelDis (void)
{
   
}


/*
*********************************************************************************************************
*                                       QCA_BSP_SPI_SetClkFreq()
*
* Description : Set SPI clock frequency.
*
* Argument(s) : freq            Clock frequency, in Hz.
*
*               pol             Clock polarity.
*
*               phase           Clock phase.
*               
*               xfer_unit_len   SPI transfer length unit.
*
*               xfer_shift_dir  SPI transer direction.
*
* Return(s)   : none.
*
* Caller(s)   : QCA400x driver via 'p_dev_bsp->SPI_SetClkFreq()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'QCA_BSP_SPI_SetClkFreq()' will be called before the device driver begins to access
*                   the SPI and after 'QCA_BSP_SPI_Lock()'has been called.
*
*               (2) The effective clock frequency MUST be no more than 'freq'.  If the frequency cannot be
*                   configured equal to 'freq', it should be configured less than 'freq'.
*
*********************************************************************************************************
*/

void  QCA_BSP_SPI_SetCfg (QCA_CFG_SPI_CLK_FREQ         freq,
                          QCA_CFG_SPI_CLK_POL          pol,
                          QCA_CFG_SPI_CLK_PHASE        pha,
                          QCA_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                          QCA_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir)
{
   
}


/*
*********************************************************************************************************
*                                         QCA_BSP_ExtIntCtrl()
*
* Description : Enable or diable external interrupt.
*
* Argument(s) : en      Enable or diable the interrupt.
*
* Return(s)   : none.
*
* Caller(s)   : QCA400x driver via 'p_dev_bsp->ExtIntCtrl()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_ExtIntCtrl (CPU_BOOLEAN   en)
{
    
}


/*
*********************************************************************************************************
*                                        QCA_BSP_ISR_HandlerK70F120M_1()
*
* Description : BSP-level ISR handler for device receive & transmit interrupts.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : CPU &/or device interrupts.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  QCA_BSP_ISR_Handler (void)
{

}
