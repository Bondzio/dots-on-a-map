/*
*********************************************************************************************************
*                                               QCA400X
*
*                         (c) Copyright 2004-2015; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  QCA400X is provided in source form to registered licensees ONLY.  It is
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
* Filename      : qca_type.h
* Version       : V1.00.00
* Programmer(s) : AL
*********************************************************************************************************
* Note(s)       : (1) The qca400x module is essentially a port of the Qualcomm/Atheros driver.
*                     Please see the Qualcomm/Atheros copyright in the files in the "atheros_wifi" folder.
*
*                 (2) Device driver compatible with these hardware:
*
*                     (a) QCA4002
*                     (b) QCA4004
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#ifndef  QCA400X_TYPE_MODULE_PRESENT
#define  QCA400X_TYPE_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <custom_src/include/a_types.h>
#include  <include/athdefs.h>
#include  <kal.h>


/*
*********************************************************************************************************
*                                      QCA POOL MANAGEMENT TYPE
*********************************************************************************************************
*/

                                                                /* ---------------- QCA BUF POOL TYPE ----------------- */
typedef struct qca_buf_pool {
    MEM_POOL        Pool;
    KAL_SEM_HANDLE  SemCnt;
} QCA_BUF_POOL;

                                                                /* ----------------- QCA BUF POOL CFG ----------------- */
typedef struct qca_buf_cfg { 
    CPU_INT32U    Size;
    CPU_INT32U    BlkNb;   
} QCA_BUF_CFG;


/*
*********************************************************************************************************
*                                QCA400x TASK CONFIGURATION DATA TYPE
*
* Note(s): (1) When the Stack pointer is defined as null pointer (DEF_NULL), the task's stack should be
*              automatically allowed on the heap of uC/LIB.
*********************************************************************************************************
*/

typedef  struct  qca_task_cfg {
    CPU_INT32U   Prio;                                          /* Task priority.                                       */
    CPU_INT32U   StkSizeBytes;                                  /* Size of the stack.                                   */
    void        *StkPtr;                                        /* Pointer to base of the stack (see Note #1).          */
} QCA_TASK_CFG;


/*
*********************************************************************************************************
*                                   DEVICE SPI CFG VALUE DATA TYPE
*********************************************************************************************************
*/

typedef  CPU_INT32U   QCA_CFG_SPI_CLK_FREQ;


typedef  CPU_BOOLEAN  QCA_CFG_SPI_CLK_POL;

#define  QCA_SPI_CLK_POL_INACTIVE_LOW                  0u   /* The clk is low  when inactive.                         */
#define  QCA_SPI_CLK_POL_INACTIVE_HIGH                 1u   /* The clk is high when inactive.                         */



typedef  CPU_BOOLEAN  QCA_CFG_SPI_CLK_PHASE;

#define  QCA_SPI_CLK_PHASE_FALLING_EDGE                0u   /*Data is 'read'    on the leading   edge of the clk & .. */
                                                            /* ...    'changed' on the following edge of the clk.     */
#define  QCA_SPI_CLK_PHASE_RISING_EDGE                 1u   /*Data is 'changed' on the following edge of the clk & .. */
                                                            /* ...    'read'    on the leading   edge of the clk.     */


typedef  CPU_INT08U   QCA_CFG_SPI_XFER_UNIT_LEN;

#define  QCA_SPI_XFER_UNIT_LEN_8_BITS                  8u   /* Xfer unit len is  8 bits.                              */
#define  QCA_SPI_XFER_UNIT_LEN_16_BITS                16u   /* Xfer unit len is 16 bits.                              */
#define  QCA_SPI_XFER_UNIT_LEN_32_BITS                32u   /* Xfer unit len is 32 bits.                              */
#define  QCA_SPI_XFER_UNIT_LEN_64_BITS                64u   /* Xfer unit len is 64 bits.                              */



typedef  CPU_BOOLEAN  QCA_CFG_SPI_XFER_SHIFT_DIR;

#define  QCA_SPI_XFER_SHIFT_DIR_FIRST_MSB              0u   /* Xfer MSB first.                                        */
#define  QCA_SPI_XFER_SHIFT_DIR_FIRST_LSB              1u   /* Xfer LSB first.                                        */

#define  QCA_CFG_FLAG_NONE                             0u

typedef  CPU_INT08U   QCA_BAND;

#define  QCA_BAND_NONE                                 0u
#define  QCA_BAND_2_4_GHZ                              1u
#define  QCA_BAND_5_0_GHZ                              2u
#define  QCA_BAND_DUAL                                 3u


/*
*********************************************************************************************************
*                                      QCA400X CONFIGURATION STRUCTURE
*********************************************************************************************************
*/


typedef  struct  qca_dev_cfg {
  
    CPU_INT32U                  NbBufPool;

    QCA_BUF_CFG                *BufPoolCfg;
    
    QCA_BAND                    Band;                   /* Wireless Band to use by the device.                              */
                                                        /*   NET_DEV_2_4_GHZ        Wireless band is 2.4Ghz.                */
                                                        /*   NET_DEV_5_0_GHZ        Wireless band is 5.0Ghz.                */
                                                        /*   NET_DEV_DUAL           Wireless band is dual (2.4 & 5.0 Ghz).  */

    QCA_CFG_SPI_CLK_FREQ        SPI_ClkFreq;            /* SPI Clk freq (in Hz).                                            */

    QCA_CFG_SPI_CLK_POL         SPI_ClkPol;             /* SPI Clk pol:                                                     */
                                                        /*   NET_DEV_SPI_CLK_POL_ACTIVE_LOW      clk is low  when inactive. */
                                                        /*   NET_DEV_SPI_CLK_POL_ACTIVE_HIGH     clk is high when inactive. */

    QCA_CFG_SPI_CLK_PHASE       SPI_ClkPhase;           /* SPI Clk phase:                                                   */
                                                        /*   NET_DEV_SPI_CLK_PHASE_FALLING_EDGE  Data availables on  ...    */
                                                        /*                                          ... failling edge.      */
                                                        /*   NET_DEV_SPI_CLK_PHASE_RASING_EDGE   Data availables on  ...    */
                                                        /*                                          ... rasing edge.        */

    QCA_CFG_SPI_XFER_UNIT_LEN   SPI_XferUnitLen;        /* SPI xfer unit length:                                            */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_8_BITS    Unit len of  8 bits.       */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_16_BITS   Unit len of 16 bits.       */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_32_BITS   Unit len of 32 bits.       */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_64_BITS   Unit len of 64 bits.       */

    QCA_CFG_SPI_XFER_SHIFT_DIR  SPI_XferShiftDir;       /* SPI xfer shift dir:                                              */
                                                        /*   NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_MSB   Xfer MSB first.         */
                                                        /*   NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_LSB   Xfer LSB first.         */
} QCA_DEV_CFG;


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif

