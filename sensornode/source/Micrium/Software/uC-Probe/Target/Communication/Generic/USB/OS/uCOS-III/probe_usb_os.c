/*
*********************************************************************************************************
*                                       uC/Probe Communication
*
*                         (c) Copyright 2007-2013; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                         COMMUNICATION: USB
*                                       Micrium uC/OS-III PORT
*
* Filename      : probe_usb_os.c
* Version       : V1.01
* Programmer(s) : JPB
*********************************************************************************************************
* Note(s)       : (1) This file is the uC/OS-III layer for the uC/Probe USB Communication Module.
*
*                 (2) Assumes uC/OS-III V3.04.00 is included in the project build.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <probe_com.h>
#include  <probe_usb.h>
#include  <probe_com_cfg.h>
#include  <os.h>


/*
*********************************************************************************************************
*                                               ENABLE
*
* Note(s) : (1) See 'probe_usb.h  ENABLE'.
*********************************************************************************************************
*/

#if (PROBE_COM_CFG_USB_EN == DEF_ENABLED)                       /* See Note #1.                                         */


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

static  OS_TCB   ProbeUSB_OS_TaskTCB;
static  CPU_STK  ProbeUSB_OS_TaskStk[PROBE_USB_CFG_TASK_STK_SIZE];   /* Stack for USB task.                             */


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  ProbeUSB_OS_Task(void  *p_arg);                        /* Probe USB task.                                 */


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          ProbeUSB_OS_Init()
*
* Description : Create RTOS objects for USB communication.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : ProbeUSB_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  ProbeUSB_OS_Init (CPU_INT08U  class_nbr)
{
    OS_ERR  err;


    OSTaskCreate((OS_TCB     *)&ProbeUSB_OS_TaskTCB,
                 (CPU_CHAR   *)"Probe USB",
                 (OS_TASK_PTR ) ProbeUSB_OS_Task,
                 (void       *) class_nbr,
                 (OS_PRIO     ) PROBE_USB_CFG_TASK_PRIO,
                 (CPU_STK    *)&ProbeUSB_OS_TaskStk[0],
                 (CPU_STK_SIZE)(PROBE_USB_CFG_TASK_STK_SIZE / 10u),
                 (CPU_STK_SIZE) PROBE_USB_CFG_TASK_STK_SIZE,
                 (OS_MSG_QTY  ) 0u,
                 (OS_TICK     ) 0u,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    (void)&err;
}


/*
*********************************************************************************************************
*                                          ProbeUSB_OS_Dly()
*
* Description : Delay the USB task.
*
* Argument(s) : ms          Number of milliseconds for which the USB task should be delayed.
*
* Return(s)   : none.
*
* Caller(s)   : ProbeUSB_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  ProbeUSB_OS_Dly (CPU_INT16U  ms)
{
    OS_ERR      err;

    
    OSTimeDlyHMSM((CPU_INT16U) 0u,
                  (CPU_INT16U) 0u,
                  (CPU_INT16U) 0u,
                  (CPU_INT32U) ms,
                  (OS_OPT    ) OS_OPT_TIME_HMSM_NON_STRICT,
                  (OS_ERR   *)&err);
}


/*
*********************************************************************************************************
*********************************************************************************************************
**                                          LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          ProbeUSB_OS_Task()
*
* Description : Task which waits for packets to be received, formulates responses, and begins transmission.
*
* Argument(s) : p_arg       Argument passed to ProbeUSB_OS_Task() by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  ProbeUSB_OS_Task (void *p_arg)
{
    CPU_INT08U   class_nbr;


    class_nbr = (CPU_INT08U)(CPU_ADDR)p_arg;

    ProbeUSB_Task(class_nbr);
}


/*
*********************************************************************************************************
*                                              ENABLE END
*
* Note(s) : See 'ENABLE  Note #1'.
*********************************************************************************************************
*/

#endif
