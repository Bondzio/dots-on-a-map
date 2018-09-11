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
* Filename      : qca.h
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

#ifndef  QCA400X_MODULE_PRESENT
#define  QCA400X_MODULE_PRESENT

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <custom_src/include/a_types.h>
#include  <include/athdefs.h>
#include  <kal.h>
#include  <qca_cfg.h>


#ifdef   QCA400X_MODULE
#define  QCA400X_EXT
#else
#define  QCA400X_EXT  extern
#endif


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  QCA_MAX_NB_BUF_POOL     10

#define  QCA_CONNECT_FAILURE     0u
#define  QCA_CONNECT_SUCCESS     1u
#define  QCA_RSNA_AUTH_FAILURE   10u
#define  QCA_RSNA_AUTH_SUCCESS   16u


/*
*********************************************************************************************************
*                                          QCA ERROR CODES
*********************************************************************************************************
*/

typedef enum qca_err {
    QCA_ERR_NONE                        = 0u,
    
    QCA_ERR_NULL_PTR                    = 100u,
    
    QCA_ERR_INIT                        = 200u,
    QCA_ERR_MEM_ALLOC                   = 201u,
    QCA_ERR_POOL_MEM_ALLOC              = 202u,
    
    QCA_ERR_NOT_INITIALIZED             = 300u,
    QCA_ERR_START_FAULT                 = 301u,
    QCA_ERR_NOT_STARTED                 = 302u,
    
} QCA_ERR;


/*
*********************************************************************************************************
*                                  QCA400X DEVICE BSP INTERFACE DATA TYPE
*********************************************************************************************************
*/

typedef  struct  qca_bsp_spi {
                                                                /* Start                                                */
    void        (*Start)            (void);

                                                                /* Stop                                                 */
    void        (*Stop )            (void);

                                                                /* Cfg GPIO                                             */
    void        (*CfgGPIO)          (void);

                                                                /* Cfg ISR                                              */
    void        (*CfgIntCtrl)       (void);

                                                                /* Enable/Disable ISR                                   */
    void        (*IntCtrl)          (CPU_BOOLEAN                  en);

                                                                /* Cfg SPI                                              */
    void        (*SPI_Init)         (void);

                                                                /* Lock                                                 */
    void        (*SPI_Lock)         (void);

                                                                /* Unlock                                               */
    void        (*SPI_Unlock)       (void);

                                                                /* Wr & Rd                                              */
    void        (*SPI_WrRd)         (CPU_INT08U                  *p_buf_wr,
                                     CPU_INT08U                  *p_buf_rd,
                                     CPU_INT16U                   len);

                                                                /* Chip Select                                          */
    void        (*SPI_ChipSelEn)    (void);

                                                                /* Chip Unselect                                        */
    void        (*SPI_ChipSelDis)   (void);

                                                                /* Set SPI Cfg                                          */
    void        (*SPI_SetCfg)       (QCA_CFG_SPI_CLK_FREQ         freq,
                                     QCA_CFG_SPI_CLK_POL          pol,
                                     QCA_CFG_SPI_CLK_PHASE        pha,
                                     QCA_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                     QCA_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir);
} QCA_DEV_BSP;


/*
*********************************************************************************************************
*                                        QCA400X CONTEXT DATA STRUCTURE
*********************************************************************************************************
*/


typedef  struct  qca_ctx {
    
   const  QCA_DEV_BSP      *Dev_BSP;
   const  QCA_DEV_CFG      *Dev_Cfg;
   const  QCA_TASK_CFG     *Tsk_Cfg;

          QCA_BUF_POOL     *BufPools[QCA_MAX_NB_BUF_POOL];

          CPU_INT08U        DeviceMacAddr[6];
          CPU_INT32U        SoftVersion;

          KAL_LOCK_HANDLE   UtilityMutex;

          KAL_TASK_HANDLE   DriverTaskHandle;
          
          KAL_SEM_HANDLE    DriverStartSemHandle;
          KAL_SEM_HANDLE    userWakeEvent;
          KAL_SEM_HANDLE    driverWakeEvent;
          KAL_SEM_HANDLE    sockSelectWakeEvent;
          
          QOSAL_VOID       *CommonCxt;
          QOSAL_BOOL        DriverStarted;
          QOSAL_BOOL        DriverShutdown;

          QOSAL_VOID       *connectStateCB;
          QOSAL_VOID       *promiscuous_cb;

          QOSAL_UINT8      *pScanOut;
          QOSAL_UINT16      scanOutSize;
          QOSAL_UINT16      scanOutCount;
          
          void             *AthSocketCxtPtr;
          void             *SocketCustomCxtPtr;
          
          QOSAL_VOID       *otaCB;
          QOSAL_VOID       *probeReqCB;
          QOSAL_VOID       *httpPostCb;
          QOSAL_VOID       *httpPostCbCxt ;

} QCA_CTX;

/*
*********************************************************************************************************
*                                       QCA400X CONTEXT POINTER
*********************************************************************************************************
*/

QCA400X_EXT  QCA_CTX  *QCA_CtxPtr;


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void          QCA_ISR_Handler  (void);

void          QCA_Init         (const QCA_TASK_CFG      *p_task_cfg,
                                const QCA_DEV_BSP       *p_dev_bsp,
                                const QCA_DEV_CFG       *p_dev_cfg,
                                      QCA_ERR           *p_err);

void          QCA_Start        (      QCA_ERR           *p_err);

void          QCA_Stop         (      QCA_ERR           *p_err);

void          QCA_BufPool_Init (      QCA_BUF_POOL     **p_buf_pool,
                                      QCA_BUF_CFG       *p_cfg,
                                      CPU_INT32U         nb_pool,
                                      QCA_ERR           *p_err);

QCA_BUF_POOL *QCA_BufPoolGet   (      QCA_BUF_POOL     **p_buf_pool,
                                      CPU_INT32U         nb_pool,
                                      CPU_INT32U         blk_size);

void         *QCA_BufBlkGet    (      QCA_BUF_POOL      *p_pool,
                                      CPU_INT32U         blk_size,
                                      CPU_BOOLEAN        block);

void          QCA_BufBlkFree   (      QCA_BUF_POOL      *p_pool,
                                      void              *p_buf);


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif

