/*
*********************************************************************************************************
*                                               QCA400x
*
*                         (c) Copyright 2004-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  QCA400x is provided in source form to registered licensees ONLY.  It is
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
*                                      QCA400x CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename : qca_cfg.c
* Version  : V1.00.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    QCA_DEV_CFG_MODULE
#include   <qca.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   QCA400x BUFFER POOLS CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define QCA_DFLT_NB_BUF_POOL        3u                          /* Size of the QCA_BUF_CFG array.                       */

#define QCA_DFLT_SMALL_BUF_NB       4u                          /* Memory Pool Configuration for the SMALL buffer.      */
#define QCA_DFLT_SMALL_BUF_SZ       64u

#define QCA_DFLT_MEDIUM_BUF_NB      3u                          /* Memory Pool Configuration for the MEDIUM buffer.     */
#define QCA_DFLT_MEDIUM_BUF_SZ      256u

#define QCA_DFLT_LARGE_BUF_NB       2u                          /* Memory Pool Configuration for the LARGE buffer.      */
#define QCA_DFLT_LARGE_BUF_SZ       1520u
                                                                /* Array of Memory Pool Configuration.                  */
QCA_BUF_CFG  QCA_BufCfgDflt[QCA_DFLT_NB_BUF_POOL] = {
 
    {QCA_DFLT_SMALL_BUF_SZ  , QCA_DFLT_SMALL_BUF_NB },          /* Small  Buffer Cfg.                                   */
    {QCA_DFLT_MEDIUM_BUF_SZ , QCA_DFLT_MEDIUM_BUF_NB},          /* Medium Buffer Cfg.                                   */
    {QCA_DFLT_LARGE_BUF_SZ  , QCA_DFLT_LARGE_BUF_NB },          /* Large  Buffer Cfg.                                  */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   QCA400x DEVICE CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/


const QCA_DEV_CFG QCA_DevCfg = {
                
    QCA_DFLT_NB_BUF_POOL,                   /* Number of buffer pools.                                                  */

    QCA_BufCfgDflt,                         /* Buffer Pools Configuration pointer.                                      */
    
    QCA_BAND_2_4_GHZ ,                      /* Wireless band:                                                           */
                                            /*   QCA_BAND_2_4_GHZ     the wireless band to use is 2.4         Ghz.      */
                                            /*   QCA_BAND_5_0_GHZ     the wireless band to use is 5.0         Ghz.      */
                                            /*   QCA_BAND_DUAL        the wireless band to use is 2.4 and 5.0 Ghz.      */

    16000000L,                              /* SPI Clock frequency (in Hertz).                                          */

    QCA_SPI_CLK_POL_INACTIVE_HIGH,          /* SPI Clock polarity:                                                      */
                                            /*   QCA_SPI_CLK_POL_INACTIVE_LOW     the clock is low  when inactive.      */
                                            /*   QCA_SPI_CLK_POL_INACTIVE_HIGH    the clock is high when inactive.      */

    QCA_SPI_CLK_PHASE_FALLING_EDGE,         /* SPI Clock phase:                                                         */
                                            /*   QCA_SPI_CLK_PHASE_FALLING_EDGE  Data availables on failling edge.      */
                                            /*   QCA_SPI_CLK_PHASE_RASING_EDGE   Data availables on rasing   edge.      */

    QCA_SPI_XFER_UNIT_LEN_8_BITS,           /* SPI transfert unit length:                                               */
                                            /*   QCA_SPI_XFER_UNIT_LEN_8_BITS    Unit length of  8 bits.                */
                                            /*   QCA_SPI_XFER_UNIT_LEN_16_BITS   Unit length of 16 bits.                */
                                            /*   QCA_SPI_XFER_UNIT_LEN_32_BITS   Unit length of 32 bits.                */
                                            /*   QCA_SPI_XFER_UNIT_LEN_64_BITS   Unit length of 64 bits.                */

    QCA_SPI_XFER_SHIFT_DIR_FIRST_MSB,       /* SPI transfer shift direction:                                            */
                                            /*   QCA_SPI_XFER_SHIFT_DIR_FIRST_MSB   Transfer MSB first.                 */
                                            /*   QCA_SPI_XFER_SHIFT_DIR_FIRST_LSB   Transfer LSB first.                 */
};



/*
*********************************************************************************************************
*********************************************************************************************************
*                                      QCA400x TASK CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  QCA_OS_CFG_TASK_PRIO
#define  QCA_OS_CFG_TASK_PRIO                4u
#endif

#ifndef  QCA_OS_CFG_TASK_STK_SIZE
#define  QCA_OS_CFG_TASK_STK_SIZE            1024
#endif

const QCA_TASK_CFG QCA_TaskCfg ={
    QCA_OS_CFG_TASK_PRIO,                                      
    QCA_OS_CFG_TASK_STK_SIZE ,                              
    DEF_NULL                                  
};