/*
*********************************************************************************************************
*                                                uC/OS-III
*                                          The Real-Time Kernel
*
*                              (c) Copyright 2011-2015, Micrium, Inc., Weston, FL
*                                           All Rights Reserved
*
*                                               TI MSP430X
*                                               MSP430x5xx
*
*
* File         : OS_CPU.H
* By           : Jian Chen (yenger@hotmail.com)
*                Jean J. Labrosse
*********************************************************************************************************
*/

#ifndef  OS_CPU_H
#define  OS_CPU_H

#ifdef  OS_CPU_GLOBALS
#define OS_CPU_EXT
#else
#define OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                             VARIABLES
*********************************************************************************************************
*/

OS_CPU_EXT  CPU_STK  *OSISRStk;

/*
*********************************************************************************************************
*                                             MSP430
*********************************************************************************************************
*/

#define  OS_TASK_SW()         OSCtxSw()               /* Task level context switch routine             */

/*
*********************************************************************************************************
*                                       TIMESTAMP CONFIGURATION
*
* Note(s) : (1) OS_TS_GET() is generally defined as CPU_TS_Get32() to allow CPU timestamp timer to be of
*               any data type size.
*
*           (2) For architectures that provide 32-bit or higher precision free running counters 
*               (i.e. cycle count registers):
*
*               (a) OS_TS_GET() may be defined as CPU_TS_TmrRd() to improve performance when retrieving
*                   the timestamp.
*
*               (b) CPU_TS_TmrRd() MUST be configured to be greater or equal to 32-bits to avoid
*                   truncation of TS.
*********************************************************************************************************
*/

#if      OS_CFG_TS_EN == 1u
#define  OS_TS_GET()               (CPU_TS)CPU_TS_Get32()   /* See Note #2a.                                          */
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif

/*
*********************************************************************************************************
*                                           PROTOTYPES
*********************************************************************************************************
*/

void  OSCtxSw        (void);
void  OSIntCtxSw     (void);
void  OSStartHighRdy (void);


#ifdef __cplusplus
}
#endif

#endif
