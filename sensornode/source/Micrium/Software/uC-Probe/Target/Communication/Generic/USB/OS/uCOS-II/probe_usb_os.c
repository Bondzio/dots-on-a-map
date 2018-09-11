/*
*********************************************************************************************************
*                                       uC/Probe Communication
*
*                         (c) Copyright 2007-2014; Micrium, Inc.; Weston, FL
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
*                                       Micrium uC/OS-II PORT
*
* Filename      : probe_usb_os.c
* Version       : V1.01
* Programmer(s) : JPB
*********************************************************************************************************
* Note(s)       : (1) This file is the uC/OS-II layer for the uC/Probe USB Communication Module.
*
*                 (2) Assumes uC/OS-II V2.92+ is included in the project build.
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
#include  <ucos_ii.h>


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
                                                                /* Stack for USB task.                                  */
static  OS_STK  ProbeUSB_OS_TaskStk[PROBE_USB_CFG_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  ProbeUSB_OS_Task(void  *p_arg);                   /* Probe USB task.                                      */


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
    INT8U  err;


#if (OS_TASK_CREATE_EXT_EN > 0)
    #if (OS_STK_GROWTH == 1)
    err = OSTaskCreateExt( ProbeUSB_OS_Task,
                           (void *)0,
                          &ProbeUSB_OS_TaskStk[PROBE_USB_CFG_TASK_STK_SIZE - 1],/* Set Top-Of-Stack.                    */
                           PROBE_USB_CFG_TASK_PRIO,
                           PROBE_USB_CFG_TASK_PRIO,
                          &ProbeUSB_OS_TaskStk[0],                              /* Set Bottom-Of-Stack.                 */
                           PROBE_USB_CFG_TASK_STK_SIZE,
                           (void *)0,                                           /* No TCB extension.                    */
                           OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);          /* Enable stack checking + clear stack. */
    #else
    err = OSTaskCreateExt( ProbeUSB_OS_Task,
                           (void *)0,
                          &ProbeUSB_OS_TaskStk[0],                              /* Set Top-Of-Stack.                    */
                           PROBE_USB_CFG_TASK_PRIO,
                           PROBE_USB_CFG_TASK_PRIO,
                          &ProbeUSB_OS_TaskStk[PROBE_USB_CFG_TASK_STK_SIZE - 1],/* Set Bottom-Of-Stack.                 */
                           PROBE_USB_CFG_TASK_STK_SIZE,
                           (void *)0,                                           /* No TCB extension.                    */
                           OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);          /* Enable stack checking + clear stack. */
    #endif
#else
    #if (OS_STK_GROWTH == 1)
    err = OSTaskCreate( ProbeUSB_OS_Task,
                        (void *)0,
                       &ProbeUSB_OS_TaskStk[PROBE_USB_CFG_TASK_STK_SIZE - 1],
                        PROBE_USB_CFG_TASK_PRIO);
    #else
    err = OSTaskCreate( ProbeUSB_OS_Task,
                        (void *)0,
                       &ProbeUSB_OS_TaskStk[0],
                        PROBE_USB_CFG_TASK_PRIO);
    #endif
#endif

#if (OS_TASK_NAME_EN   > 0)
    OSTaskNameSet(PROBE_USB_CFG_TASK_PRIO, (INT8U *)"Probe USB", &err);
#endif
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
    INT32U  dly_ticks;


    dly_ticks = OS_TICKS_PER_SEC * ((INT32U)ms + 500L / OS_TICKS_PER_SEC) / 1000L;
    OSTimeDly(dly_ticks);
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
    CPU_INT08U  class_nbr;


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
