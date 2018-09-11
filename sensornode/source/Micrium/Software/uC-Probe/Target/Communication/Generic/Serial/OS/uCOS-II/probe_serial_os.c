/*
*********************************************************************************************************
*                                         uC/Probe Communication
*
*                         (c) Copyright 2007-2008; Micrium, Inc.; Weston, FL
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
*                                              uC/Probe
*
*                                        Communication: Serial
*
* Filename      : probe_usb_os.c
* Version       : V2.20
* Programmer(s) : BAN
* Note(s)       : (1) This file is the uC/OS-II layer for the uC/Probe Serial Communication Module.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <probe_com.h>
#include  <probe_serial.h>
#include  <ucos_ii.h>

#if (PROBE_COM_CFG_SERIAL_EN == DEF_ENABLED)

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

                                                                /* Probe serial task stack.                             */
static  OS_STK  ProbeSerial_OS_TaskStk[PROBE_SERIAL_CFG_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  ProbeSerial_OS_Task (void *p_arg);                /* Probe serial task.                                   */


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            ProbeSerial_OS_Init()
*
* Description : Initialize the serial task for Probe communication.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : ProbeSerial_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  ProbeSerial_OS_Init (void)
{
    INT8U  err;


#if (OS_TASK_CREATE_EXT_EN > 0)
    #if (OS_STK_GROWTH == 1)
    err = OSTaskCreateExt((void (*)(void *)) ProbeSerial_OS_Task,
                          (void          * ) 0,
                          (OS_STK        * )&ProbeSerial_OS_TaskStk[PROBE_SERIAL_CFG_TASK_STK_SIZE - 1],
                          (INT8U           ) PROBE_SERIAL_CFG_TASK_PRIO,
                          (INT16U          ) PROBE_SERIAL_CFG_TASK_PRIO,
                          (OS_STK        * )&ProbeSerial_OS_TaskStk[0],
                          (INT32U          ) PROBE_SERIAL_CFG_TASK_STK_SIZE,
                          (void          * ) 0,
                          (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
    #else
    err = OSTaskCreateExt((void (*)(void *)) ProbeSerial_OS_Task,
                          (void          * ) 0,
                          (OS_STK        * )&ProbeSerial_OS_TaskStk[0],
                          (INT8U           ) PROBE_SERIAL_CFG_TASK_PRIO,
                          (INT16U          ) PROBE_SERIAL_CFG_TASK_PRIO,
                          (OS_STK        * )&ProbeSerial_OS_TaskStk[PROBE_SERIAL_CFG_TASK_STK_SIZE - 1],
                          (INT32U          ) PROBE_SERIAL_CFG_TASK_STK_SIZE,
                          (void          * ) 0,
                          (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
    #endif
#else
    #if (OS_STK_GROWTH == 1)
    err = OSTaskCreate((void (*)(void *)) ProbeSerial_OS_Task,
                       (void          * ) 0,
                       (OS_STK        * )&ProbeSerial_OS_TaskStk[PROBE_SERIAL_CFG_TASK_STK_SIZE - 1],
                       (INT8U           ) PROBE_SERIAL_CFG_TASK_PRIO);
    #else
    err = OSTaskCreate((void (*)(void *)) ProbeSerial_OS_Task,
                       (void          * ) 0,
                       (OS_STK        * )&ProbeSerial_OS_TaskStk[0],
                       (INT8U           ) PROBE_SERIAL_CFG_TASK_PRIO);
    #endif
#endif

#if (OS_VERSION < 287)
#if (OS_TASK_NAME_SIZE > 10)
    OSTaskNameSet(PROBE_SERIAL_CFG_TASK_PRIO, (INT8U *)"Probe Serial", &err);
#endif
#else
#if (OS_TASK_NAME_EN   > 0)
    OSTaskNameSet(PROBE_SERIAL_CFG_TASK_PRIO, (INT8U *)"Probe Serial", &err);
#endif
#endif
}



/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          ProbeSerial_OS_Task()
*
* Description : Task which waits for packets to be received, formalates responses, and begins transmission.
*
* Argument(s) : p_arg        Argument passed to 'ProbeSerial_OS_Task()' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  ProbeSerial_OS_Task (void *p_arg)
{
    ProbeSerial_Task(p_arg);
}

#endif
