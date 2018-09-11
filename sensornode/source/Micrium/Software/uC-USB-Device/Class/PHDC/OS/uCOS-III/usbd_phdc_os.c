/*
*********************************************************************************************************
*                                            uC/USB-Device
*                                       The Embedded USB Stack
*
*                         (c) Copyright 2004-2013; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/USB is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                 USB PHDC CLASS OPERATING SYSTEM LAYER
*                                          Micrium uC/OS-III
*
* File          : usbd_phdc_os.c
* Version       : V4.05.00
* Programmer(s) : JFD
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#include  <app_cfg.h>
#include  "../../usbd_phdc.h"
#include  "../../usbd_phdc_os.h"
#include  <Source/os.h>


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef USBD_PHDC_OS_CFG_SCHED_EN
#error  "USBD_PHDC_OS_CFG_SCHED_EN not #define'd in 'usbd_cfg.h' [MUST be DEF_ENABLED or DEF_DISABLED]"
#endif

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
#ifndef USBD_PHDC_OS_CFG_SCHED_TASK_STK_SIZE
#error  "USBD_PHDC_OS_CFG_SCHED_TASK_STK_SIZE not #define'd in 'app_cfg.h' [MUST be > 0u]"
#endif

#ifndef USBD_PHDC_OS_CFG_SCHED_TASK_PRIO
#error  "USBD_PHDC_OS_CFG_SCHED_TASK_PRIO not #define'd in 'app_cfg.h' [MUST be > 0]"
#endif
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
#define  USBD_PHDC_OS_BULK_WR_PRIO_MAX                     5u
#else
#define  USBD_PHDC_OS_BULK_WR_PRIO_MAX                     1u
#endif


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          PHDC OS CTRL INFO
*********************************************************************************************************
*/

typedef struct usbd_phdc_os_ctrl {
    OS_SEM       WrIntrSem;                                     /* Lock that protect wr intr EP.                        */
    OS_SEM       RdSem;                                         /* Lock that protect rd bulk EP.                        */
    OS_SEM       WrBulkSem[USBD_PHDC_OS_BULK_WR_PRIO_MAX];      /* Sem that unlock bulk write of given prio.            */

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    CPU_INT08U   WrBulkSemCtr[USBD_PHDC_OS_BULK_WR_PRIO_MAX];   /* Count of lock performed on given prio.               */
    OS_SEM       WrBulkSchedLock;                               /* Sem used to lock sched.                              */
    OS_SEM       WrBulkSchedRelease;                            /* Sem used to release sched.                           */

    CPU_BOOLEAN  ReleasedSched;                                 /* Indicate if sched is released.                       */
    CPU_INT08U   WrBulkLockCnt;                                 /* Count of call to WrBulkLock.                         */
#endif
} USBD_PHDC_OS_CTRL;

/*
*********************************************************************************************************
*                                          PHDC OS PEND DATA
*********************************************************************************************************
*/

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
typedef struct usbd_phdc_os_sched_pend_data {
    OS_PEND_DATA  SchedLockSem;
    OS_PEND_DATA  SchedReleaseSem;
} USBD_PHDC_OS_SCHED_PEND_DATA;
#endif


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  USBD_PHDC_OS_CTRL             USBD_PHDC_OS_CtrlTbl[USBD_PHDC_CFG_MAX_NBR_DEV];
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
static  USBD_PHDC_OS_SCHED_PEND_DATA  USBD_PHDC_OS_PendData[USBD_PHDC_CFG_MAX_NBR_DEV];
static  CPU_STK                       USBD_PHDC_OS_WrBulkSchedTaskStk[USBD_PHDC_OS_CFG_SCHED_TASK_STK_SIZE];
static  OS_TCB                        USBD_PHDC_OS_WrBulkSchedTaskTCB;
#endif


/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
static  void  USBD_PHDC_OS_WrBulkSchedTask(void  *p_arg);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBD_PHDC_OS_Init()
*
* Description : Initialize PHDC OS interface.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE               OS initialization successful.
*                               USBD_ERR_OS_SIGNAL_CREATE   OS semaphore NOT successfully initialized.
*                               USBD_ERR_OS_INIT_FAIL       OS task      NOT successfully initialized.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_Init (USBD_ERR  *p_err)
{
    OS_ERR                         err_os;
    USBD_PHDC_OS_CTRL             *p_os_ctrl;
    CPU_INT08U                     ix;
    CPU_INT08U                     cnt;
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    USBD_PHDC_OS_SCHED_PEND_DATA  *p_os_pend_data;
#endif


    for (cnt = 0; cnt < USBD_PHDC_CFG_MAX_NBR_DEV; cnt ++) {
        p_os_ctrl = &USBD_PHDC_OS_CtrlTbl[cnt];

        OSSemCreate(&p_os_ctrl->WrIntrSem,
                    "Write intr semaphore",
                     1u,
                    &err_os);

        if (err_os != OS_ERR_NONE) {
           *p_err = USBD_ERR_OS_SIGNAL_CREATE;
            return;
        }

        OSSemCreate(&p_os_ctrl->RdSem,
                    "Read semaphore",
                     1u,
                    &err_os);

        if (err_os != OS_ERR_NONE) {
           *p_err = USBD_ERR_OS_SIGNAL_CREATE;
            return;
        }

        for (ix = 0u; ix < USBD_PHDC_OS_BULK_WR_PRIO_MAX; ix++) {
            OSSemCreate(&p_os_ctrl->WrBulkSem[ix],
                        "Bulk priority semaphore",
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
                         0u,
#else
                         1u,
#endif
                        &err_os);

            if (err_os != OS_ERR_NONE) {
               *p_err = USBD_ERR_OS_SIGNAL_CREATE;
                return;
            }
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
            p_os_ctrl->WrBulkSemCtr[ix] = 0u;
#endif
        }

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
        OSSemCreate(&p_os_ctrl->WrBulkSchedLock,
                    "Bulk scheduler lock",
                     0u,
                    &err_os);

        if (err_os != OS_ERR_NONE) {
           *p_err = USBD_ERR_OS_SIGNAL_CREATE;
            return;
        }

        OSSemCreate(&p_os_ctrl->WrBulkSchedRelease,
                    "Bulk scheduler release",
                     0u,
                    &err_os);

        if (err_os != OS_ERR_NONE) {
           *p_err = USBD_ERR_OS_SIGNAL_CREATE;
            return;
        }


        p_os_ctrl->ReleasedSched = DEF_YES;                     /* Sched is released by dflt.                           */
        p_os_ctrl->WrBulkLockCnt = 0;

        p_os_pend_data                             = &USBD_PHDC_OS_PendData[cnt];
        p_os_pend_data->SchedLockSem.PendObjPtr    = (OS_PEND_OBJ *)&p_os_ctrl->WrBulkSchedLock;
        p_os_pend_data->SchedReleaseSem.PendObjPtr = (OS_PEND_OBJ *)&p_os_ctrl->WrBulkSchedRelease;
#endif
    }

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    OSTaskCreate(        &USBD_PHDC_OS_WrBulkSchedTaskTCB,
                         "USB PHDC Scheduler",
                          USBD_PHDC_OS_WrBulkSchedTask,
                 (void *) 0,
                          USBD_PHDC_OS_CFG_SCHED_TASK_PRIO,
                         &USBD_PHDC_OS_WrBulkSchedTaskStk[0],
                          USBD_PHDC_OS_CFG_SCHED_TASK_STK_SIZE / 10u,
                          USBD_PHDC_OS_CFG_SCHED_TASK_STK_SIZE,
                          0u,
                          0u,
                 (void *) 0,
                          OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                         &err_os);

    if (err_os != OS_ERR_NONE) {
       *p_err = USBD_ERR_OS_INIT_FAIL;
        return;
    }
#endif

   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBD_PHDC_OS_RdLock()
*
* Description : Lock PHDC read pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
*               timeout_ms  Timeout, in ms.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS signal     successfully acquired.
*                               USBD_OS_ERR_TIMEOUT     OS signal NOT successfully acquired in the time
*                                                         specified by 'timeout_ms'.
*                               USBD_OS_ERR_ABORT       OS signal aborted.
*                               USBD_OS_ERR_FAIL        OS signal not acquired because another error.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Rd(),
*               USBD_PHDC_PreambleRd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_RdLock (CPU_INT08U   class_nbr,
                           CPU_INT16U   timeout,
                           USBD_ERR    *p_err)
{
    OS_SEM             *p_sem;
    OS_ERR              err_os;
    USBD_PHDC_OS_CTRL  *p_os_ctrl;
    CPU_INT32U          timeout_ticks;


    p_os_ctrl     = &USBD_PHDC_OS_CtrlTbl[class_nbr];
    p_sem         = &p_os_ctrl->RdSem;
    timeout_ticks = ((((OS_TICK)timeout * OSCfg_TickRate_Hz)  + 1000u - 1u) / 1000u);

    OSSemPend(          p_sem,
                        timeout_ticks,
                        OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
                       &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
            *p_err = USBD_ERR_NONE;
             break;

        case OS_ERR_TIMEOUT:
            *p_err = USBD_ERR_OS_TIMEOUT;
             break;

        case OS_ERR_PEND_ABORT:
            *p_err = USBD_ERR_OS_ABORT;
             break;

        case OS_ERR_OBJ_TYPE:
        case OS_ERR_OBJ_DEL:
        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_PEND_ISR:
        case OS_ERR_SCHED_LOCKED:
        case OS_ERR_PEND_WOULD_BLOCK:
        default:
            *p_err = USBD_ERR_OS_FAIL;
             break;
    }
}


/*
*********************************************************************************************************
*                                       USBD_PHDC_OS_RdUnlock()
*
* Description : Unlock PHDC read pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Rd(),
*               USBD_PHDC_PreambleRd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_RdUnlock (CPU_INT08U  class_nbr)
{
    OS_SEM             *p_sem;
    OS_ERR              err_os;
    USBD_PHDC_OS_CTRL  *p_os_ctrl;


    p_os_ctrl = &USBD_PHDC_OS_CtrlTbl[class_nbr];
    p_sem     = &p_os_ctrl->RdSem;

    OSSemPost(p_sem,
              OS_OPT_POST_1,
             &err_os);
}


/*
*********************************************************************************************************
*                                      USBD_PHDC_OS_WrIntrLock()
*
* Description : Lock PHDC write interrupt pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
*               timeout_ms  Timeout, in ms.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS signal     successfully acquired.
*                               USBD_OS_ERR_TIMEOUT     OS signal NOT successfully acquired in the time
*                                                       specified by 'timeout_ms'.
*                               USBD_OS_ERR_ABORT       OS signal aborted.
*                               USBD_OS_ERR_FAIL        OS signal not acquired because another error.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrIntrLock (CPU_INT08U   class_nbr,
                               CPU_INT16U   timeout,
                               USBD_ERR    *p_err)
{
    OS_SEM             *p_sem;
    OS_ERR              err_os;
    USBD_PHDC_OS_CTRL  *p_os_ctrl;
    CPU_INT32U          timeout_ticks;


    p_os_ctrl     = &USBD_PHDC_OS_CtrlTbl[class_nbr];
    p_sem         = &p_os_ctrl->WrIntrSem;
    timeout_ticks = ((((OS_TICK)timeout * OSCfg_TickRate_Hz)  + 1000u - 1u) / 1000u);

    OSSemPend(          p_sem,
                        timeout_ticks,
                        OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
                       &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
            *p_err = USBD_ERR_NONE;
             break;

        case OS_ERR_TIMEOUT:
            *p_err = USBD_ERR_OS_TIMEOUT;
             break;

        case OS_ERR_PEND_ABORT:
            *p_err = USBD_ERR_OS_ABORT;
             break;

        case OS_ERR_OBJ_TYPE:
        case OS_ERR_OBJ_DEL:
        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_PEND_ISR:
        case OS_ERR_SCHED_LOCKED:
        case OS_ERR_PEND_WOULD_BLOCK:
        default:
            *p_err = USBD_ERR_OS_FAIL;
             break;
    }
}


/*
*********************************************************************************************************
*                                     USBD_PHDC_OS_WrIntrUnlock()
*
* Description : Unlock PHDC write interrupt pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrIntrUnlock (CPU_INT08U  class_nbr)
{
    OS_SEM             *p_sem;
    OS_ERR              err_os;
    USBD_PHDC_OS_CTRL  *p_os_ctrl;


    p_os_ctrl = &USBD_PHDC_OS_CtrlTbl[class_nbr];
    p_sem     = &p_os_ctrl->WrIntrSem;

    OSSemPost(p_sem,
              OS_OPT_POST_1,
             &err_os);
}


/*
*********************************************************************************************************
*                                      USBD_PHDC_OS_WrBulkLock()
*
* Description : Lock PHDC write bulk pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
*               prio        Priority of the transfer, based on its QoS.
*
*               timeout_ms  Timeout, in ms.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS signal     successfully acquired.
*                               USBD_OS_ERR_TIMEOUT     OS signal NOT successfully acquired in the time
*                                                         specified by 'timeout_ms'.
*                               USBD_OS_ERR_ABORT       OS signal aborted.
*                               USBD_OS_ERR_FAIL        OS signal not acquired because another error.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr(),
*               USBD_PHDC_PreambleWr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrBulkLock (CPU_INT08U   class_nbr,
                               CPU_INT08U   prio,
                               CPU_INT16U   timeout,
                               USBD_ERR    *p_err)
{
    OS_SEM             *p_sem;
    OS_ERR              err_os;
    USBD_PHDC_OS_CTRL  *p_os_ctrl;
    CPU_INT32U          timeout_ticks;
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    CPU_SR_ALLOC();
#else

    (void)&prio;
#endif


    p_os_ctrl     = &USBD_PHDC_OS_CtrlTbl[class_nbr];
    timeout_ticks = ((((OS_TICK)timeout * OSCfg_TickRate_Hz)  + 1000u - 1u) / 1000u);

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    p_sem = &p_os_ctrl->WrBulkSem[prio];

    CPU_CRITICAL_ENTER();
    p_os_ctrl->WrBulkSemCtr[prio]++;
    CPU_CRITICAL_EXIT();

    OSSemPost(&p_os_ctrl->WrBulkSchedLock,
               OS_OPT_POST_1,
              &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
             break;

        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_OBJ_TYPE:
        case OS_ERR_SEM_OVF:
        default:
            *p_err = USBD_ERR_OS_FAIL;
             return;
    }
#else
    p_sem = &p_os_ctrl->WrBulkSem[0];
#endif

    OSSemPend(          p_sem,                                  /* Wait for sched to unlock xfer.                       */
                        timeout_ticks,
                        OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
                       &err_os);

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    CPU_CRITICAL_ENTER();
    if (p_os_ctrl->WrBulkSemCtr[prio] > 0) {
        p_os_ctrl->WrBulkSemCtr[prio]--;
    }
    CPU_CRITICAL_EXIT();
#endif

    switch (err_os) {
        case OS_ERR_NONE:
            *p_err = USBD_ERR_NONE;
             break;

        case OS_ERR_TIMEOUT:
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
             CPU_CRITICAL_ENTER();
             if (p_os_ctrl->WrBulkLockCnt > 0) {
                 p_os_ctrl->WrBulkLockCnt--;
             }
             CPU_CRITICAL_EXIT();
#endif
            *p_err = USBD_ERR_OS_TIMEOUT;
             break;

        case OS_ERR_PEND_ABORT:
            *p_err = USBD_ERR_OS_ABORT;
             break;

        case OS_ERR_OBJ_TYPE:
        case OS_ERR_OBJ_DEL:
        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_PEND_ISR:
        case OS_ERR_SCHED_LOCKED:
        case OS_ERR_PEND_WOULD_BLOCK:
        default:
            *p_err = USBD_ERR_OS_FAIL;
             break;
    }
}


/*
*********************************************************************************************************
*                                     USBD_PHDC_OS_WrBulkUnlock()
*
* Description : Unlock PHDC write bulk pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr(),
*               USBD_PHDC_PreambleWr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrBulkUnlock (CPU_INT08U  class_nbr)
{
    OS_SEM             *p_sem;
    OS_ERR              err_os;
    USBD_PHDC_OS_CTRL  *p_os_ctrl;


    p_os_ctrl = &USBD_PHDC_OS_CtrlTbl[class_nbr];
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    p_sem = &p_os_ctrl->WrBulkSchedRelease;
#else
    p_sem = &p_os_ctrl->WrBulkSem[0];
#endif

    OSSemPost(p_sem,
              OS_OPT_POST_1,
             &err_os);
}


/*
*********************************************************************************************************
*                                        USBD_PHDC_OS_Reset()
*
* Description : Reset PHDC OS layer for given instance.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Reset().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_Reset (CPU_INT08U  class_nbr)
{
    USBD_PHDC_OS_CTRL  *p_os_ctrl;
    OS_ERR              err;
    CPU_INT08U          cnt;
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    CPU_SR_ALLOC();
#endif


    p_os_ctrl = &USBD_PHDC_OS_CtrlTbl[class_nbr];

    OSSemPendAbort(&p_os_ctrl->WrIntrSem,                       /* Resume all task pending on sem.                      */
                    OS_OPT_PEND_ABORT_ALL,
                   &err);

    OSSemPendAbort(&p_os_ctrl->RdSem,
                    OS_OPT_PEND_ABORT_ALL,
                   &err);

    for (cnt = 0; cnt < USBD_PHDC_OS_BULK_WR_PRIO_MAX; cnt++) {
        OSSemPendAbort(&p_os_ctrl->WrBulkSem[cnt],
                        OS_OPT_PEND_ABORT_ALL,
                       &err);
    }

#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
    CPU_CRITICAL_ENTER();
    p_os_ctrl->WrBulkLockCnt = 0;
    p_os_ctrl->ReleasedSched = DEF_YES;

    for (cnt = 0; cnt < USBD_PHDC_OS_BULK_WR_PRIO_MAX; cnt++) {
        p_os_ctrl->WrBulkSemCtr[cnt] = 0;
    }
    CPU_CRITICAL_EXIT();
#endif
}


/*
*********************************************************************************************************
*                                   USBD_PHDC_OS_WrBulkSchedTask()
*
* Description : OS-dependent shell task to schedule bulk transfers in function of their priority.
*
* Argument(s) : p_arg       Pointer to task initialization argument (required by uC/OS-III).
*
* Return(s)   : none.
*
* Created by  : USBD_PHDC_OS_Init().
*
* Note(s)     : (1) Only one task handle all class instances bulk write scheduling.
*********************************************************************************************************
*/
#if USBD_PHDC_OS_CFG_SCHED_EN == DEF_ENABLED
static  void  USBD_PHDC_OS_WrBulkSchedTask (void *p_arg)
{
    OS_ERR                         err_os;
    USBD_PHDC_OS_CTRL             *p_os_ctrl;
    USBD_PHDC_OS_SCHED_PEND_DATA  *p_os_pend_data;
    CPU_INT08U                     prio;
    CPU_INT08U                     cnt;
    CPU_SR_ALLOC();


    (void)&p_arg;

    while (DEF_ON) {
                                                                /* Pend on release/bulk lock sem of all class instances.*/
        (void)OSPendMulti((OS_PEND_DATA *)&USBD_PHDC_OS_PendData,
                                           USBD_PHDC_CFG_MAX_NBR_DEV * 2,
                                           0,
                                           OS_OPT_PEND_BLOCKING,
                                          &err_os);

        if (err_os == OS_ERR_NONE) {
            for (cnt = 0; cnt < USBD_PHDC_CFG_MAX_NBR_DEV; cnt++) {
                p_os_pend_data = &USBD_PHDC_OS_PendData[cnt];
                p_os_ctrl      = &USBD_PHDC_OS_CtrlTbl[cnt];

                if (p_os_pend_data->SchedLockSem.RdyObjPtr != (OS_PEND_OBJ *)0) {
                    p_os_ctrl->WrBulkLockCnt++;
                }
                if (p_os_pend_data->SchedReleaseSem.RdyObjPtr != (OS_PEND_OBJ *)0) {
                    p_os_ctrl->ReleasedSched = DEF_YES;
                }

                if ((p_os_ctrl->WrBulkLockCnt  > 0      ) &&
                    (p_os_ctrl->ReleasedSched == DEF_YES)) {
                    prio = 0u;

                    CPU_CRITICAL_ENTER();
                    p_os_ctrl->WrBulkLockCnt--;
                    p_os_ctrl->ReleasedSched = DEF_NO;

                    while (prio < USBD_PHDC_OS_BULK_WR_PRIO_MAX) {
                        if (p_os_ctrl->WrBulkSemCtr[prio] > 0) {
                            break;
                        }
                        prio++;
                    }
                    CPU_CRITICAL_EXIT();

                    if (prio < USBD_PHDC_OS_BULK_WR_PRIO_MAX) {
                        OSSemPost(&p_os_ctrl->WrBulkSem[prio],
                                   OS_OPT_POST_1,
                                  &err_os);
                    }
                }
            }
        }
    }
}
#endif
