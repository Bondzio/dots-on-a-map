/*
*********************************************************************************************************
*                                                uC/OS-III
*                                          The Real-Time Kernel
*
*
*                           (c) Copyright 2009-2010; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                      IAR DLIB MULTITHREADED SUPPORT
* 
*
* File      : os_dlib.c
* Version   : V3.02.0
* By        : FT
*
* LICENSING TERMS:
* ---------------
*             uC/OS-III is provided in source form to registered licensees ONLY.  It is 
*             illegal to distribute this source code to any third party unless you receive 
*             written permission by an authorized Micrium representative.  Knowledge of 
*             the source code may NOT be used to develop a similar product.
*
*             Please help us continue to provide the Embedded community with the finest
*             software available.  Your honesty is greatly appreciated.
* 
*             You can find our product's user manual, API reference, release notes and
*             more information at https://doc.micrium.com.
*             You can contact us at www.micrium.com.
*
* Toolchain : IAR EWARM V6.xx and higher
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>
#include  <yvals.h>
#include  <app_cfg.h>


/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define  OS_DLIB_LOCK_FILE_STREAM_NBR                     8u
#define  OS_DLIB_LOCK_HEAP_MGMT_NBR                       5u


/*
*********************************************************************************************************
*                                            DEFAULT VALUES
*********************************************************************************************************
*/

#if  ((!defined(OS_CFG_DLIB_REG_ID_TLS) )  && \
      ( defined(OS_CFG_TASK_REG_TBL_SIZE)) && \
      ( OS_CFG_TASK_REG_TBL_SIZE > 0u   ))
#define  OS_CFG_DLIB_REG_ID_TLS                 OS_CFG_TASK_REG_TBL_SIZE - 1u
#endif

#if (_DLIB_FILE_DESCRIPTOR > 0) && \
    (_FILE_OP_LOCKS        > 0)
#define  OS_CFG_DLIB_LOCK_MAX_NBR              (OS_DLIB_LOCK_HEAP_MGMT_NBR + \
                                                OS_DLIB_LOCK_FILE_STREAM_NBR)
#else
#define  OS_CFG_DLIB_LOCK_MAX_NBR               OS_DLIB_LOCK_HEAP_MGMT_NBR
#endif


/*
*********************************************************************************************************
*                                        LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef  OS_CFG_MUTEX_EN 
#error  "OS_CFG_MUTEX_EN                     not #define'd in 'os_cfg.h'     "
#error  "                                [MUST be 1]                         "

#elif   (OS_CFG_MUTEX_EN == 0u)
#error  "OS_CFG_MUTEX_EN               illegally #define'd in 'os_cfg.h'     "
#error  "                                [MUST be 1]                         "
#endif

#ifndef  OS_CFG_DLIB_LOCK_MAX_NBR
#error  "OS_CFG_DLIB_LOCK_MAX_NBR           not #define'd in 'os_cfg_app.h'  "
#endif

#ifndef  OS_CFG_TASK_REG_TBL_SIZE 
#error  "OS_CFG_TASK_REG_TBL_SIZE           not #define'd in 'os_cfg.h'      "
#error  "                                [MUST be > 0]                       "

#elif   (OS_CFG_TASK_REG_TBL_SIZE == 0u)
#error  "OS_CFG_TASK_REG_TBL_SIZE      illegally #define'd in 'os_cfg.h'     "
#error  "                                [MUST be > 0]                       "
#endif

#ifndef OS_CFG_DLIB_REG_ID_TLS
#error  "OS_CFG_DLIB_REG_ID_TLS             not #define'd in 'os_cfg_app.h'  "

#elif   (OS_CFG_DLIB_REG_ID_TLS > OS_CFG_TASK_REG_TBL_SIZE - 1u)
#error  "OS_CFG_DLIB_REG_ID_TLS     illegally #define'd in 'os_cfg_app.h'    "
#error  "                                [MUST be < OS_CFG_TASK_REG_TBL_SIZE]"
#endif


/*
*********************************************************************************************************
*                                            LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  struct os_dlib_lock  OS_DLIB_LOCK;

struct os_dlib_lock {                                           
    OS_MUTEX       Mutex;                                       /* OS Mutex object.                                   */
    OS_DLIB_LOCK  *NextPtr;                                     /* Pointer to the next object in the pool.            */
};


/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static  OS_DLIB_LOCK   OS_DLIB_PoolData[OS_CFG_DLIB_LOCK_MAX_NBR];
static  OS_DLIB_LOCK  *OS_DLIB_PoolHeadPtr = (OS_DLIB_LOCK *)0; /* Pointer to the head of 'OS_DLIB_LOCK' link list.   */


/*
*********************************************************************************************************
*                                              MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     DLIB SYSTEM LOCKS INTERFACE
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      OS DLIB LOCK INITIALIZATION
*
* Description : Create a list of free available 'OS_DLIB_LOCK' objects.
*
* Argument(s) : None.
*
* Return(s)   : Return the current task TLS pointer.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) 'OS_CFG_DLIB_LOCK_MAX_NBR' configure the maximum number of locks used by the 
*                    application.
*********************************************************************************************************
*/

void  OS_DLIB_LockInit (void)
{
    CPU_INT16U     pool_data_ix;
    OS_DLIB_LOCK  *p_lock;
    OS_DLIB_LOCK  *p_lock_next;
    CPU_SR_ALLOC();
    
    
    CPU_CRITICAL_ENTER();    
                                                                /* Create the link list of OS_DLIB_LOCK objects.        */
    for (pool_data_ix = 0u; pool_data_ix < OS_CFG_DLIB_LOCK_MAX_NBR - 1u; pool_data_ix++) {
        p_lock          = &OS_DLIB_PoolData[pool_data_ix];
        p_lock_next     = &OS_DLIB_PoolData[pool_data_ix + 1u];
        p_lock->NextPtr =  p_lock_next;
    }
    
    OS_DLIB_PoolHeadPtr = &OS_DLIB_PoolData[0];                 /* Initialize the list head pointer.                    */
    p_lock              = &OS_DLIB_PoolData[OS_CFG_DLIB_LOCK_MAX_NBR - 1u];
    p_lock->NextPtr     = (OS_DLIB_LOCK *)0;                    /* Last node points to 'NULL'                           */

    CPU_CRITICAL_EXIT();    
}


/*
*********************************************************************************************************
*                                          OS DLIB LOCK CREATE
* 
* Description : Allocate  a new 'OS_DLIB_LOCK' object from the free pool and 
*               and create a 'OS_MUTEX' in the kernel.
*
* Argument(s) : p_lock  Pointer to the DLIB lock handler.
*
* Return(s)   : None.
*
* Caller(s)   : __iar_system_Mtxinit()
*               __iar_file_Mtxinit()
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OS_DLIB_LockCreate (void  **p_lock)
{
    OS_DLIB_LOCK  *p_lock_new;
    OS_ERR         os_err;
    CPU_SR_ALLOC();

    
    if (p_lock == (void **)0) {
        return;
    }

    if (OS_DLIB_PoolHeadPtr == (OS_DLIB_LOCK *)0) {             /* If 'OS_DLIB_LOCK' object pool is empty?              */
        *p_lock = (void *)0;                                    /*   return a 'NULL' pointer.                           */
        return;
    }

    p_lock_new = OS_DLIB_PoolHeadPtr;                           /* Get the first object in the list.                    */

    if (OSRunning == OS_STATE_OS_RUNNING) {                     /* Create a MUTEX only if the kernel has started.       */
        OSMutexCreate(               &p_lock_new->Mutex,        /* Create the mutex in the kernel.                      */
                      (CPU_CHAR    *) 0,
                                     &os_err);
                                 
        if (os_err != OS_ERR_NONE) {                            /* If the mutex create funtion fail?                    */
            *p_lock = (void *)0;                                /* ... return a 'NULL' pointer.                         */
             return;        
        }
    }        

    CPU_CRITICAL_ENTER();
    OS_DLIB_PoolHeadPtr = p_lock_new->NextPtr;                  /* Move HEAD pointer to the next object in the list.    */
    CPU_CRITICAL_EXIT();
    
    *p_lock = (void *)p_lock_new;                               /* Return the new 'OS_DLIB_LOCK' object pointer.        */
}


/*
*********************************************************************************************************
*                                       OS DLIB LOCK DELETE
*
* Description : Delete a 'OS_MUTEX' from the kernel and return the allocated 'OS_DLIB_LOCK'
*               to the free pool.
*
* Argument(s) : p_lock  DLIB lock handler.
*
* Return(s)   : Return the current task TLS pointer.
*
* Caller(s)   : __iar_system_Mtxdst()
*               __iar_file_Mtxdst()
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OS_DLIB_LockDel (void  *p_lock)
{
    OS_DLIB_LOCK   *p_lock_cur;
    OS_ERR         os_err;
    CPU_SR_ALLOC();

    
    if (p_lock == (void *)0) {
        return;
    }
    
    p_lock_cur = (OS_DLIB_LOCK *)p_lock;


    if (OSRunning == OS_STATE_OS_RUNNING) {                     /* Delete a MUTEX only if the kernel has started.       */
        
        (void)OSMutexDel(&p_lock_cur->Mutex,
                         OS_OPT_DEL_ALWAYS,
                         &os_err);
        (void)&os_err;
    }

    CPU_CRITICAL_ENTER();
                                                                /* Return the 'OS_DLIB_LOCK' in front of the link list. */
    if (OS_DLIB_PoolHeadPtr == (OS_DLIB_LOCK *)0) {
        p_lock_cur->NextPtr  = (OS_DLIB_LOCK *)0;
    } else {
        p_lock_cur->NextPtr = OS_DLIB_PoolHeadPtr;
    }
    OS_DLIB_PoolHeadPtr  = p_lock_cur;                      

    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                         OS DLIB LOCK PEND
*
* Description : Wait indefinitely until the lock become available
*
* Arguments   : p_lock  DLIB lock handler.
*
* Return(s)   : Return the current task TLS pointer.
*
* Caller(s)   : __iar_system_Mtxlock()
*               __iar_file_Mtxlock()

* Note(s)     : None.
*********************************************************************************************************
*/

void  OS_DLIB_LockPend (void  *p_lock)
{
    OS_DLIB_LOCK  *p_lock_cur;
    OS_ERR          os_err;    
    
    
    if ((p_lock    == (void *)0          ) ||                   /* Return if the lock handler is 'NULL' or the ...      */
        (OSRunning != OS_STATE_OS_RUNNING)) {                   /* ... kernel is not running.                           */
        return;
    }
    
    p_lock_cur = (OS_DLIB_LOCK *)p_lock;
    OSMutexPend(           &p_lock_cur->Mutex,
                (OS_TICK  ) 0u,
                            OS_OPT_PEND_BLOCKING,
                (CPU_TS  *) 0,
                           &os_err);
    (void)&os_err;
}


/*
*********************************************************************************************************
*                                         OS DLIB LOCK POST
*
* Description : Signal the lock.
*
* Argument(s) : p_lock  DLIB lock handler.
*
* Return(s)   : Return the current task TLS pointer.
*
* Caller(s)   : __iar_system_Mtxunlock()
*               __iar_file_Mtxunlock()
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OS_DLIB_LockPost (void  *p_lock)
{
    OS_DLIB_LOCK  *p_lock_cur;
    OS_ERR         os_err;    
    
    
    if ((p_lock    == (void *)0          ) ||                   /* Return if the lock handler is 'NULL' or the ...      */
        (OSRunning != OS_STATE_OS_RUNNING)) {                   /* ... kernel is not running.                           */
        return;
    }
    
    p_lock_cur = (OS_DLIB_LOCK *)p_lock;
    OSMutexPost(&p_lock_cur->Mutex,
                 OS_OPT_POST_NONE,
                &os_err);

    (void)&os_err;
}


/*
*********************************************************************************************************
*                                         OS DLIB TLS GET
*
* Description : Get TLS segment pointer.
*
* Argument(s) : p_thread   Thread handler.
*
* Return(s)   : Return the current task TLS pointer.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  *OS_DLIB_TLS_Get (void *p_thread)
{
    void    *p_tls;
    OS_TCB  *p_tcb;
    OS_ERR   os_err;

    
    if (OSRunning != OS_STATE_OS_RUNNING) {                     /* If the kernel is not runnin yet?                     */
        p_tls = (void *)__segment_begin("__DLIB_PERTHREAD");    /* ... return the pointer to the main TLS segment.      */
    }
    
    p_tcb = (OS_TCB *)p_thread;                                 /* Get the pointer from the task's register.           */
    p_tls = (void   *)OSTaskRegGet((OS_TCB    *) p_tcb,         
                                   (OS_REG_ID  ) OS_CFG_DLIB_REG_ID_TLS,
                                                &os_err);
    
    if (os_err != OS_ERR_NONE) {
        return ((void *)0);
    }           

    return (p_tls);
}


/*
*********************************************************************************************************
*                                         OS DLIB TLS SET
*
* Description : Set the current TLS segment pointer.
*
* Argument(s) : None.
*
* Return(s)   : Return the current task TLS pointer.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OS_DLIB_TLS_Set (void  *p_thread,
                       void  *p_tls)
{
    OS_ERR   os_err;
    OS_TCB  *p_tcb;

    
    if (p_tls == (void *)0) {
        return;
    }

    p_tcb = (OS_TCB *)p_thread;    
    
    OSTaskRegSet((OS_TCB    *) p_tcb,
                 (OS_REG_ID  ) OS_CFG_DLIB_REG_ID_TLS,
                 (OS_REG     ) p_tls,
                 (OS_ERR    *)&os_err);
    
    (void)os_err;
    
}


/*
*********************************************************************************************************
*                                         OS DLIB TLS START
*
* Description : Allocate and initialize TLS support.
*
* Argument(s) : p_thread   Pointer to the thread handler.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OS_DLIB_TLS_Start (void  *p_thread)
{
    void  *p_tls;
    
    
    p_tls = (void *)__iar_dlib_perthread_allocate();            /* Get TLS segment from the HEAP.                       */
    
    __iar_dlib_perthread_initialize(p_tls);                     /* Initialize the TLS segment.                          */
    
    OS_DLIB_TLS_Set(p_thread, p_tls);                           /* Set the TLS segment pointer in the task.             */
}


/*
*********************************************************************************************************
*                                         OS DLIB TLS END
*
* Description : Stop and de-allocate TLS support.
*
* Argument(s) : p_thread   Pointer to the thread handler.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  OS_DLIB_TLS_End (void  *p_thread)
{
    void  *p_tls;

    
    p_tls = OS_DLIB_TLS_Get(p_thread);                          /* Get TLS segment pointer.                              */
    
    __iar_dlib_perthread_destroy();
    __iar_dlib_perthread_deallocate(p_tls);                     /* De-allocate the TLS segment.                          */
    
    OS_DLIB_TLS_Set(p_thread, (void *)0);                       /* Remove the TLS segment pointer from the task.         */
}
