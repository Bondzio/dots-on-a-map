/*
*********************************************************************************************************
*                                            uC/USB-Device
*                                       The Embedded USB Stack
*
*                         (c) Copyright 2004-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/USB-Device is provided in source form to registered licensees ONLY.  It is
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
*                                   USB DEVICE OPERATING SYSTEM LAYER
*                                          Micrium Template
*
* File          : usbd_msc_os.c
* Version       : V4.05.00
* Programmer(s) : PW
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/


#include  "../../usbd_msc.h"
#include  "../../usbd_msc_os.h"
#include  <app_cfg.h>


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef USBD_MSC_OS_CFG_TASK_STK_SIZE
#error  "USBD_MSC_OS_CFG_TASK_STK_SIZE not #define'd in 'app_cfg.h' [MUST be > 0]"
#endif

#ifndef USBD_MSC_OS_CFG_TASK_PRIO
#error  USBD_MSC_OS_CFG_TASK_PRIO not #define'd in 'app_cfg.h' [MUST be > 0]"
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


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
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB   USBD_MSC_OS_TaskTCB;

static  CPU_STK  USBD_MSC_OS_TaskStk[USBD_MSC_OS_CFG_TASK_STK_SIZE];

static  OS_SEM   USBD_MSC_OS_TASK_SemTbl[USBD_MSC_CFG_MAX_NBR_DEV];

static  OS_SEM   USBD_MSC_OS_EnumSignal;


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

static  void  USBD_MSC_OS_Task(void  *p_arg);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           USBD_MSC_OS_Init()
*
* Description : Initialize MSC OS interface.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE       OS initialization successful.
*                               USBD_ERR_OS_FAIL    OS objects NOT successfully initialized.
*
* Return(s)   : None.
*
* Caller(s)   : USBD_OS_Init().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_MSC_OS_Init (USBD_ERR  *p_err)
{
    /* $$$$ Insert code to create all the required semaphores. */

    /* $$$$ Insert code to create the MSC task, USBD_MSC_OS_Task(). */

   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         USBD_MSC_OS_Task()
*
* Description : OS-dependent shell task to process MSC task
*
* Argument(s) : p_arg       Pointer to task initialization argument (required by uC/OS-II).
*
* Return(s)   : None.
*
* Created by  : USBD_OS_Init().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBD_MSC_OS_Task (void  *p_arg)
{
    CPU_INT08U  class_nbr;


    class_nbr = (CPU_INT08U)(CPU_ADDR)p_arg;

    USBD_MSC_TaskHandler(class_nbr);
}


/*
*********************************************************************************************************
*                                          USBD_MSC_OS_CommSignalPost()
*
* Description : Post a semaphore used for MSC communication.
*
* Argument(s) : class_nbr   MSC instance class number
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE       OS signal     successfully posted.
*                               USBD_ERR_OS_FAIL    OS signal NOT successfully posted.
*
* Return(s)   : None.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_MSC_OS_CommSignalPost (CPU_INT08U   class_nbr,
                                  USBD_ERR    *p_err)
{
    /* $$$$ Insert code to post a semaphore used for MSC communication. */
   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          USBD_MSC_OS_CommSignalPend()
*
* Description : Wait on a semaphore to become available for MSC communication.
*
* Argument(s) : class_nbr   MSC instance class number
*
*               timeout     Timeout in milliseconds.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*                               USBD_ERR_NONE          The call was successful and your task owns the resource
*                                                       or, the event you are waiting for occurred.
*                               USBD_ERR_OS_TIMEOUT    The semaphore was not received within the specified timeout.
*                               USBD_ERR_OS_FAIL       otherwise.
*
* Return(s)   : None.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_MSC_OS_CommSignalPend (CPU_INT08U   class_nbr,
                                  CPU_INT32U   timeout,
                                  USBD_ERR    *p_err)
{
    /* $$$$ Insert code to wait on a semaphore to become available for MSC communication. */
    /* A timeout parameter is available to implement a wait forever or with a timeout.    */

   *p_err = USBD_ERR_OS_NONE;
}


/*
*********************************************************************************************************
*                                         USBD_MSC_OS_CommSignalDel()
*
* Description : Delete a semaphore if no tasks are waiting on it for MSC communication.
*
* Argument(s) : class_nbr   MSC instance class number
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*                               USBD_ERR_NONE          The call was successful and the semaphore was destroyed
*                               USBD_ERR_OS_FAIL       otherwise.
*
* Return(s)   : None.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_MSC_OS_CommSignalDel (CPU_INT08U   class_nbr,
                                 USBD_ERR    *p_err)
{
   /* $$$$ Insert code to delete a semaphore if no tasks are waiting on it for MSC communication. */
   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          USBD_MSC_OS_EnumSignalPost()
*
* Description : Post a semaphore for MSC enumeration process.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE       OS signal     successfully posted.
*                               USBD_ERR_OS_FAIL    OS signal NOT successfully posted.
*
* Return(s)   : None.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_MSC_OS_EnumSignalPost (USBD_ERR  *p_err)
{
    /* $$$$ Insert code to post a semaphore for MSC enumeration process. */
   *p_err = USBD_ERR_NONE;

}


/*
*********************************************************************************************************
*                                       USBD_MSC_OS_EnumSignalPend()
*
* Description : Wait on a semaphore to become available for MSC enumeration process.
*
* Argument(s) : timeout     Timeout in milliseconds.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*                               USBD_ERR_NONE          The call was successful and your task owns the resource
*                                                       or, the event you are waiting for occurred.
*                               USBD_ERR_OS_TIMEOUT    The semaphore was not received within the specified timeout.
*                               USBD_ERR_OS_FAIL       otherwise.
*
* Return(s)   : None.
*
* Caller(s)   : Various.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_MSC_OS_EnumSignalPend (CPU_INT32U  timeout,
                                  USBD_ERR   *p_err)
{
    /* $$$$ Insert code to wait on a semaphore to become available for MSC enumeration process. */
   *p_err = USBD_ERR_NONE;
}
