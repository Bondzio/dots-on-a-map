/*
*********************************************************************************************************
*                                              QCA400x
X
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
*                                               QCA400X
*
* Filename      : qca.c
* Version       : V1.00.00
* Programmer(s) : AL
*********************************************************************************************************
* Note(s)       : (1) The qca400x module is essentially a port of the Qualcomm/Atheros driver.
*                     Please see the Qualcomm/Atheros copyright in the files in the "atheros_wifi" folder.
*
*                 (3) Device driver compatible with these hardware:
*
*                     (a) QCA4002
*                     (b) QCA4004
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    QCA400X_MODULE

#include  <Source/os.h>
#include  "qca.h"
#include  "common_src/include/common_api.h"
#include  "custom_src/include/custom_wlan_api.h"
#include  "atheros_wifi/atheros_wifi_api.h"
#include  "atheros_wifi/atheros_wifi_internal.h"
#include  "atheros_wifi/atheros_stack_offload.h"
#include  "custom_src/stack_custom/custom_stack_offload.h"
#include  "common_src/stack_common/common_stack_offload.h"


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  QCA_WAKE_EVENT_NAME                                "QCA400x Wake Event"
#define  QCA_START_EVENT_NAME                               "QCA400x Start Event"
#define  QCA_USER_WAKE_EVENT_NAME                           "QCA400x Wake Event"
#define  QCA_SOCK_SEL_EVENT_NAME                            "QCA400x SockSel Event"
#define  QCA_TASK_NAME                                      "QCA400x Task"


/*
*********************************************************************************************************
*                                      LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

#if (QCA_CFG_HEAP_USAGE_CALC_EN == DEF_ENABLED) 
CPU_SIZE_T QCAStat_HeapUsage;
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  QOSAL_VOID  QCA_Task (void *p_arg);


/*
*********************************************************************************************************
*                                              QCA_Init()
*
* Description : Initialize the QCA400x Driver/Module.
*
* Argument(s) : p_task_cfg  Pointer to the task configuration.
*
*               p_dev_bsp   Pointer to the BSP API.
*
*               p_dev_cfg   Pointer to the Device configuration.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*                               QCA_ERR_NONE                     Operation Successful.
*                               QCA_ERR_MEM_ALLOC                Allocation of Memory failed.
*                               QCA_ERR_POOL_MEM_ALLOC           Allocation of Mem pool failed.
*                               QCA_ERR_INIT                     Initialization failed.
*
* Return(s)   : none.
*
* Caller(s)   : App_TaskStart().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  QCA_Init (const QCA_TASK_CFG    *p_task_cfg,
                const QCA_DEV_BSP     *p_dev_bsp,
                const QCA_DEV_CFG     *p_dev_cfg,
                      QCA_ERR         *p_err)
{
    CPU_SIZE_T             reqd_octets;
    LIB_ERR                lib_err;
    RTOS_ERR               err_rtos;
    
    
#if (QCA_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_task_cfg == DEF_NULL) { 
       *p_err = QCA_ERR_NULL_PTR;
        return;
    }
    if (p_dev_bsp == DEF_NULL) { 
       *p_err = QCA_ERR_NULL_PTR;
        return;
    }
    if (p_dev_cfg == DEF_NULL) { 
       *p_err = QCA_ERR_NULL_PTR;
        return;
    }
    if (p_err == DEF_NULL) { 
       *p_err = QCA_ERR_NULL_PTR;
        return;
    }    
#endif
    
    KAL_Init(DEF_NULL, &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = QCA_ERR_INIT;
        return;
    } 
    
#if (QCA_CFG_HEAP_USAGE_CALC_EN == DEF_ENABLED)   
    QCAStat_HeapUsage = Mem_HeapGetSizeRem(sizeof(CPU_ALIGN),&lib_err);
    if (lib_err != LIB_MEM_ERR_NONE) { 
       *p_err = QCA_ERR_INIT;
        return;    
    }
#endif    
                                                                /* --------------- ALLOCATE QCA CONTEXT --------------- */
    QCA_CtxPtr = Mem_HeapAlloc( sizeof(QCA_CTX),
                                sizeof(CPU_INT32U),
                               &reqd_octets,
                               &lib_err);
    if (QCA_CtxPtr == (void *)DEF_NULL) {
       *p_err = QCA_ERR_MEM_ALLOC;
        return;
    }
    
    QCA_CtxPtr->Tsk_Cfg = p_task_cfg;
    QCA_CtxPtr->Dev_BSP = p_dev_bsp;
    QCA_CtxPtr->Dev_Cfg = p_dev_cfg;
    
                                                                /* ------------ ALLOCATE COMMON DRIVER OBJ ------------ */
    QCA_CtxPtr->CommonCxt = Mem_HeapAlloc( sizeof(A_DRIVER_CONTEXT),
                                           sizeof(CPU_INT32U),
                                          &reqd_octets,
                                          &lib_err);
    if (QCA_CtxPtr->CommonCxt == (void *)DEF_NULL) {
       *p_err = QCA_ERR_MEM_ALLOC;
        return;
    }
                                                                /* -------------- ALLOCATE SCAN INFO OBJ -------------- */
    QCA_CtxPtr->scanOutSize  = ATH_MAX_SCAN_BUFFERS;
    QCA_CtxPtr->scanOutCount = 0u;
    
    QCA_CtxPtr->pScanOut = (CPU_INT08U *)Mem_HeapAlloc((ATH_MAX_SCAN_BUFFERS * sizeof(ATH_SCAN_EXT)),
                                                        sizeof(CPU_INT32U),
                                                       &reqd_octets,
                                                       &lib_err);
    if (QCA_CtxPtr->pScanOut == (void *)DEF_NULL) {
       *p_err = QCA_ERR_MEM_ALLOC;
        return;
    }
    
    QCA_CtxPtr->SocketCustomCxtPtr = (CPU_INT08U *)Mem_HeapAlloc(((MAX_SOCKETS_SUPPORTED + 1) * sizeof(SOCKET_CUSTOM_CONTEXT)),
                                                                   sizeof(CPU_INT32U),
                                                                  &reqd_octets,
                                                                  &lib_err);
    if (QCA_CtxPtr->SocketCustomCxtPtr == (void *)DEF_NULL) {
       *p_err = QCA_ERR_MEM_ALLOC;
        return;
    }   
    QCA_CtxPtr->AthSocketCxtPtr = (CPU_INT08U *)Mem_HeapAlloc(((MAX_SOCKETS_SUPPORTED + 1) * sizeof(ATH_SOCKET_CONTEXT)),
                                                                sizeof(CPU_INT32U),
                                                               &reqd_octets,
                                                               &lib_err);
    if (QCA_CtxPtr->AthSocketCxtPtr == (void *)DEF_NULL) {
       *p_err = QCA_ERR_MEM_ALLOC;
        return;
    }   
    
                                                                /* Create Atheros driver start semaphore.                 */
    QCA_CtxPtr->DriverStartSemHandle = KAL_SemCreate((const  CPU_CHAR *) QCA_START_EVENT_NAME,
                                                                         DEF_NULL,
                                                                        &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = QCA_ERR_INIT;
        return;
    }
                                                                /* ------------ ALLOCATE DATA BUFFER POOLS ------------ */
    QCA_BufPool_Init(QCA_CtxPtr->BufPools,
                     p_dev_cfg->BufPoolCfg,
                     p_dev_cfg->NbBufPool,
                     p_err);
    if (*p_err != QCA_ERR_NONE) {
        return;
    }
  
                                                                /* ------- INITIALIZE EXTERNAL GPIO CONTROLLER -------- */
    p_dev_bsp->CfgGPIO();                                       /* Configure Wireless Controller GPIO (see Note #2).    */

                                                                /* ------------ INITIALIZE SPI CONTROLLER ------------- */
    p_dev_bsp->SPI_Init();                                      /* Configure SPI Controller (see Note #3).              */

                                                                /* ----- INITIALIZE EXTERNAL INTERRUPT CONTROLLER ----- */
    p_dev_bsp->CfgIntCtrl();                                    /* Configure ext int ctrl'r (see Note #4).              */

                                                                /* ------------ DISABLE EXTERNAL INTERRUPT ------------ */
    p_dev_bsp->IntCtrl(DEF_NO);                                 /* Disable ext int ctrl'r (See Note #5)                 */

                                                                /* ---------------- CREATE DRIVER TASK ---------------- */
                                                                /* Get the driver task handle.                          */
    QCA_CtxPtr->DriverTaskHandle = KAL_TaskAlloc((const  CPU_CHAR *) QCA_TASK_NAME,
                                                                     DEF_NULL,
                                                                     p_task_cfg->StkSizeBytes,
                                                                     DEF_NULL,
                                                                    &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = QCA_ERR_MEM_ALLOC;
        return ;
    }
                                                                /* Create the driver task.                              */
    KAL_TaskCreate(          QCA_CtxPtr->DriverTaskHandle ,
                             QCA_Task,
                    (void *) QCA_CtxPtr,
                             p_task_cfg->Prio,
                             DEF_NULL,
                            &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err = QCA_ERR_INIT;
        return ;
    }
    
    Mem_Clr(QCA_CtxPtr->DeviceMacAddr, 6u);
    
    QCA_CtxPtr->connectStateCB = DEF_NULL;
    QCA_CtxPtr->DriverShutdown = DEF_NO;
    QCA_CtxPtr->DriverStarted  = DEF_NO;
    QCA_CtxPtr->promiscuous_cb = DEF_NULL;
    QCA_CtxPtr->scanOutCount   = 0u;
    
#if (QCA_CFG_HEAP_USAGE_CALC_EN == DEF_ENABLED)   
    QCAStat_HeapUsage -= Mem_HeapGetSizeRem (sizeof(CPU_ALIGN),&lib_err);
    if (lib_err != LIB_MEM_ERR_NONE) { 
       *p_err = QCA_ERR_INIT;
        return;    
    }
#endif
    
   *p_err = QCA_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           QCA_ISR_Handler()
*
* Description : Handle the ISR from the QCA400x. 
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : BSP.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  QCA_ISR_Handler (void)
{
    HW_InterruptHandler(QCA_CtxPtr);
}


/*
*********************************************************************************************************
*                                              QCA_Start()
*
* Description : Start the QCA400X Driver/Module.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*                           QCA_ERR_NONE                     Operation Successful.
*                           QCA_ERR_START_FAULT              Driver failed to start.
*
* Return(s)   : none.
*
* Caller(s)   : App_TaskStart().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  QCA_Start (QCA_ERR  *p_err)
{
    RTOS_ERR   err_rtos;
    A_STATUS   status;
    
#if (QCA_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) { 
       *p_err = QCA_ERR_NULL_PTR;
        return;
    }   
#endif
    
    if (QCA_CtxPtr == DEF_NULL) { 
       *p_err = QCA_ERR_NOT_INITIALIZED;
        return;
    }    
                                                                /* ---------- ATHEROS COMMON INITIALIZATION ----------- */
    status = Api_InitStart((QOSAL_VOID*)QCA_CtxPtr);
    if (status != A_OK) {
        *p_err = QCA_ERR_INIT;
         return;
    }
                                                                /* Unblock the QCA Task.                                */
    KAL_SemPost( QCA_CtxPtr->DriverStartSemHandle,
                 KAL_OPT_PEND_NONE,
                &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
       *p_err  = QCA_ERR_START_FAULT ;
        return;
    }
                                                                /* Launch the driver initialization sequence.           */
    status = Driver_Init(QCA_CtxPtr);
    if (status != A_OK) {
       *p_err  = QCA_ERR_START_FAULT ;
        return;
    }
                                                                /* Complete the initialization.                         */
    Api_InitFinish(QCA_CtxPtr);
    Api_WMIInitFinish(QCA_CtxPtr);
    
   *p_err = QCA_ERR_NONE;
}


/*
*********************************************************************************************************
*                                              QCA_Stop()
*
* Description : Stop the QCA400X Driver/Module.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
* Return(s)   : none.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  QCA_Stop (QCA_ERR  *p_err)
{
    CPU_BOOLEAN     is_started;
    CPU_SR_ALLOC();
      
      
#if (QCA_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) { 
       *p_err = QCA_ERR_NULL_PTR;
        return;
    }    
#endif   
    
    if (QCA_CtxPtr == DEF_NULL) { 
       *p_err = QCA_ERR_NOT_INITIALIZED;
        return;
    } 
    
    CPU_CRITICAL_ENTER();
    is_started = QCA_CtxPtr->DriverStarted;
    CPU_CRITICAL_EXIT();
    
    if (is_started == DEF_NO) { 
       *p_err = QCA_ERR_NOT_STARTED;
        return;
    }    
    
    CPU_CRITICAL_ENTER();
    QCA_CtxPtr->DriverShutdown = QOSAL_TRUE;
    CPU_CRITICAL_EXIT();    
    
    DRIVER_WAKE_DRIVER(QCA_CtxPtr);
    
    DRIVER_WAIT_FOR_CONDITION(QCA_CtxPtr, &QCA_CtxPtr->DriverShutdown, QOSAL_FALSE, 15000);
    
    Driver_DeInit(QCA_CtxPtr);
    
    Api_DeInitFinish(QCA_CtxPtr);
    
    
   *p_err = QCA_ERR_NONE;

}


 /*
*********************************************************************************************************
*                                          QCA_BufPool_Init()
*
* Description : Initialize the QCA Buffer Pools.
*
* Argument(s) : p_buf_pool  Pointer to a table of QCA_BUF_POOL available.
*
*               p_cfg       POinter to a configuration of buffer pool
*
*               nb_pool     Number of QCA_BUF_POOL object available in the table
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               QCA_ERR_POOL_MEM_ALLOC              Allocation of Mem pool failed.
* Return(s)   : none.
*
* Caller(s)   : QCA_Init().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void QCA_BufPool_Init (QCA_BUF_POOL  **p_buf_pool,
                       QCA_BUF_CFG    *p_cfg,
                       CPU_INT32U      nb_pool,
                       QCA_ERR        *p_err)
{
    CPU_INT32U  i;
    CPU_SIZE_T  reqd_octets;
    LIB_ERR     lib_err;
    RTOS_ERR    err_rtos;

  
    for (i = 0 ; i < nb_pool; i++) {                            /* For Each buffer pool configured...                   */

                                                                /* ...Allocate a QCA_BUF_POOL object...                 */
        p_buf_pool[i] = (QCA_BUF_POOL *) Mem_HeapAlloc( sizeof(QCA_BUF_POOL),
                                                        sizeof(CPU_INT32U),
                                                       &reqd_octets,
                                                       &lib_err);
        if (lib_err != LIB_MEM_ERR_NONE) {
          *p_err = QCA_ERR_POOL_MEM_ALLOC;
           return;
        }
                                                                /* ...Create the memory pool...                         */
        Mem_PoolCreate (&(p_buf_pool[i]->Pool),
                         DEF_NULL,
                         DEF_NULL,
                         p_cfg[i].BlkNb,
                         p_cfg[i].Size + AR6000_DATA_OFFSET + sizeof(A_NETBUF),
                         sizeof(CPU_ALIGN),
                        &reqd_octets,
                        &lib_err);
        if (lib_err != LIB_MEM_ERR_NONE) {
           *p_err = QCA_ERR_POOL_MEM_ALLOC;
            return;
        }
                                                                /* ...Create the memory block semaphore...              */
        p_buf_pool[i]->SemCnt = KAL_SemCreate((const  CPU_CHAR *) "qca buf",
                                                                  DEF_NULL,
                                                                  &err_rtos);
        if (err_rtos != RTOS_ERR_NONE) {
           *p_err = QCA_ERR_INIT;
            return;
        }
                                                                /* ...and set the semaphore to the nb of block avail.   */
        KAL_SemSet( p_buf_pool[i]->SemCnt ,
                    p_cfg[i].BlkNb,
                   &err_rtos);
        if (err_rtos != RTOS_ERR_NONE) {
           *p_err = QCA_ERR_INIT;
            return;
        }
    }

  *p_err = QCA_ERR_NONE;
}

/*
*********************************************************************************************************
*                                           QCA_BufPoolGet()
*
* Description : Get a QCA_BUF_POOL that contains block with enough space for the memory block size needed.
*
* Argument(s) : p_buf_pool  Pointer to a table of QCA_BUF_POOL available.
*
*               nb_pool     Number of QCA_BUF_POOL object available in the table
*
*               blk_size    Size of the memory block needed.
*
* Return(s)   : none.
*
* Caller(s)   : Custom_GetRxRequest(),
*               a_netbuf_alloc().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

QCA_BUF_POOL *QCA_BufPoolGet (QCA_BUF_POOL  **p_buf_pool,
                              CPU_INT32U      nb_pool,
                              CPU_INT32U      blk_size)
{
    QCA_BUF_POOL  *p_pool;
    CPU_INT32U     i;
    
                                                                /* Add the driver header and the size of A_NETBUF.       */
    blk_size += AR6000_DATA_OFFSET + sizeof(A_NETBUF);
                                                                /* Find a Pool with block of enough size.               */
    for (i = 0; i < nb_pool ; ++i) {
        p_pool = p_buf_pool[i];
        if (blk_size <= p_pool->Pool.BlkSize) {
            break;
        } else { 
            p_pool = DEF_NULL;
        }
    }
    return p_pool;
}


/*
*********************************************************************************************************
*                                            QCA_BufBlkGet()
*
* Description : Get a Buffer from a QCA_BUF_POOL.
*
* Argument(s) : p_pool      Pointer to the QCA_BUF_POOL to use.
*
*               blk_size    Size of the memory block needed.
*
*               block       Indicate if the function must block if no buffer is available.
*
* Return(s)   : If successful, Pointer to the beginning of the memory block.
*               If failed,     DEF_NULL;
*
* Caller(s)   : Custom_GetRxRequest(),
*               a_netbuf_alloc().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void* QCA_BufBlkGet (QCA_BUF_POOL *p_pool,
                     CPU_INT32U    blk_size,
                     CPU_BOOLEAN   block)
{
    KAL_OPT      option;
    void        *p_buf;
    LIB_ERR      err_lib;
    RTOS_ERR      err_rtos;
    
                                                                /* Choose pend option.                                  */
    if (block == DEF_YES) {
        option = KAL_OPT_PEND_BLOCKING;
    } else {
        option = KAL_OPT_PEND_NON_BLOCKING;
    }
                                                                /* Check for the buffer availability                    */
    KAL_SemPend( p_pool->SemCnt,
                 option, 
                 1000u,
                 &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {                         
        return DEF_NULL;
    }  
                                                                /* Add the driver header and the size of A_NETBUF.      */
    blk_size += AR6000_DATA_OFFSET + sizeof(A_NETBUF);
    
                                                                /* Get the memory block.                                */
    p_buf = Mem_PoolBlkGet(&(p_pool->Pool), blk_size, &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return DEF_NULL;
    }
    
    return p_buf;
}


/*
*********************************************************************************************************
*                                           QCA_BufBlkFree()
*
* Description : Free a Buffer from a QCA_BUF_POOL
*
* Argument(s) : p_pool  Pointer to the QCA_BUF_POOL to use.
*
*               p_buf   Pointer to the memory block to free.
*
* Return(s)   : none.
*
* Caller(s)   : a_netbuf_free().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  QCA_BufBlkFree (QCA_BUF_POOL *p_pool,
                      void*         p_buf)
{
    LIB_ERR      err_lib;
    RTOS_ERR      err_rtos;
    

    Mem_PoolBlkFree(&(p_pool->Pool),
                      p_buf,
                    &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        A_ASSERT(0);
    }
    
    KAL_SemPost( p_pool->SemCnt,
                 KAL_OPT_PEND_NONE,
                &err_rtos);
    if (err_rtos != RTOS_ERR_NONE) {
        A_ASSERT(0);
    }
}                                           
                                                        

/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              QCA_Task()
*
* Description : $$$$ Add function description.
*
* Argument(s) : p_arg   $$$$ Add description for 'p_arg'
*
* Return(s)   : none.
*
* Caller(s)   : QCA_Init().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static QOSAL_VOID QCA_Task (void *p_arg)
{
    QOSAL_VOID   *p_cxt;
    QOSAL_BOOL    can_block;
    QOSAL_UINT16  block_msec;
    RTOS_ERR  kal_err;
    CPU_SR_ALLOC();

    can_block = QOSAL_FALSE;
    p_cxt     = (QOSAL_VOID *)p_arg;

    do
    {
                                                                /* ----------- WAIT FOR DRIVER START SIGNAL ----------- */
        KAL_SemPend( GET_DRIVER_CXT(p_cxt)->DriverStartSemHandle,
                     KAL_OPT_PEND_NONE,
                     KAL_TIMEOUT_INFINITE,
                     &kal_err);
        if (kal_err != RTOS_ERR_NONE){
            A_ASSERT(0);
        }
        
        CPU_CRITICAL_ENTER();
        GET_DRIVER_CXT(p_cxt)->DriverStarted = DEF_YES;
        CPU_CRITICAL_EXIT();
                                                                /* --------------- DRIVER TASK HANDLER ---------------- */
        while (DEF_TRUE) {
                                                                /* Check if the Driver task can sleep.                  */
            if (can_block) {
                if (GET_DRIVER_CXT(p_cxt)->DriverShutdown == DEF_YES){
                    break;
                }
                GET_DRIVER_COMMON(p_cxt)->driverSleep = QOSAL_TRUE;

                qosal_wait_for_event(&GET_DRIVER_CXT(p_cxt)->driverWakeEvent, 0x01, A_TRUE, 0, block_msec);
            }

            GET_DRIVER_COMMON(p_cxt)->driverSleep = QOSAL_FALSE;

                                                                /* Check if the driver should shutdown.                        */
            Driver_Main( p_cxt,
                         DRIVER_SCOPE_RX|DRIVER_SCOPE_TX,
                        &can_block,
                        &block_msec);
        }
        
        CPU_CRITICAL_ENTER();
        GET_DRIVER_CXT(p_cxt)->DriverStarted  = DEF_NO; 
        GET_DRIVER_CXT(p_cxt)->DriverShutdown = DEF_NO;
        CPU_CRITICAL_EXIT();
        
                                                              /* Signal that the driver is stopped.                          */
        DRIVER_WAKE_USER(p_cxt);

    } while (DEF_TRUE);
}
