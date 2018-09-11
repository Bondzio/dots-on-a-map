/*
************************************************************************************************************************
*                                                      uC/OS-III
*                                                 The Real-Time Kernel
*
*                                     (c) Copyright 2012; Micrium, Inc.; Weston, FL
*                           All rights reserved.  Protected by international copyright laws.
*
*                                                   Microchip OSAL 
*
* File    : OSAL_uCOS3.H
* By      : JJL
* Version : V3.03.02
*
* LICENSING TERMS:
* ---------------
*           uC/OS-III is provided in source form for FREE short-term evaluation, for educational use or 
*           for peaceful research.  If you plan or intend to use uC/OS-III in a commercial application/
*           product then, you need to contact Micrium to properly license uC/OS-III for its use in your 
*           application/product.   We provide ALL the source code for your convenience and to help you 
*           experience uC/OS-III.  The fact that the source is provided does NOT mean that you can use 
*           it commercially without paying a licensing fee.
*
*           Knowledge of the source code may NOT be used to develop a similar product.
*
*           Please help us continue to provide the embedded community with the finest software available.
*           Your honesty is greatly appreciated.
*
*           You can contact us at www.micrium.com, or by phone at +1 (954) 217-2036.
************************************************************************************************************************
*/

#ifndef _OSAL_MICRIUM_H
#define _OSAL_MICRIUM_H

#define  OSAL_USE_RTOS

#ifdef __cplusplus
extern "C" {
#endif

/*
------------------------------------------------------------------------------------------------------------------------
*                                                    INCLUDE FILES
------------------------------------------------------------------------------------------------------------------------
*/

#include <xc.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os.h"

  
/*
------------------------------------------------------------------------------------------------------------------------
*                                                     CONSTANTS
------------------------------------------------------------------------------------------------------------------------
*/

#ifndef OSAL_DAEMON_STACK_SIZE
#if defined(__PIC32MX__)
#define OSAL_DAEMON_STACK_SIZE  800
#elif defined(__PIC24__)
#define OSAL_DAEMON_STACK_SIZE  200
#endif
#endif

#ifndef OSAL_DAEMON_PRIORITY
#define OSAL_DAEMON_PRIORITY    ((OSAL_MAX_PRIORITIES)-(3))
#endif

/*
------------------------------------------------------------------------------------------------------------------------
*                                                     DATA TYPES
------------------------------------------------------------------------------------------------------------------------
*/

struct  osal_q {
    void         *QBaseAddrPtr;                 /* Base address of queue's storage area                               */
    void         *QEndAddrPtr;                  /* One entry past the end of the queue storage area                   */
    void         *QInPtr;                       /* Pointer to where the next queue element will be placed             */
    void         *QOutPtr;                      /* Pointer to the next queue entry to extract                         */
    OS_SEM        QPostSem;                     /* Semaphore used to block the posting task if the queue is full      */
    OS_SEM        QPendSem;                     /* Semaphore used to block the pending task if the queue is empty     */
    CPU_INT16U    QEntries;                     /* Current number of entries in the queue                             */
    CPU_INT16U    QEntriesMax;                  /* Maximum number of entries that can be placed in the queue          */
    CPU_INT16U    QEntrySize;                   /* Number of bytes for each entry in the queue                        */
};

typedef  struct  osal_q  OSAL_Q;                /* OSAL message queue type                                            */

#define OSAL_SEM_HANDLE_TYPE            OS_SEM*
#define OSAL_MUTEX_HANDLE_TYPE          OS_MUTEX*
#define OSAL_QUEUE_HANDLE_TYPE          OSAL_Q*
#define OSAL_THREAD_HANDLE_TYPE         OS_TCB*

#define OSAL_WAIT_FOREVER               (OS_TICK)0xFFFF

/*
------------------------------------------------------------------------------------------------------------------------
*                                          INCLUDE 'osal.h' AFTER DATA TYPES
------------------------------------------------------------------------------------------------------------------------
*/

#include "osal.h"


/*
------------------------------------------------------------------------------------------------------------------------
*                                                 FUNCTION PROTOTYPES
------------------------------------------------------------------------------------------------------------------------
*/

#define OSAL_ISR_Enter()    OSIntEnter()

#define OSAL_ISR_Exit()     OSIntExit()

#ifdef __cplusplus
}
#endif

#endif
