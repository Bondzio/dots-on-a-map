/*
*********************************************************************************************************
*                                             uC/OS-III
*                                        The Real-Time Kernel
*
*                          (c) Copyright 2010-2015; Micrium, Inc.; Weston, FL
*                         
*               All rights reserved. Protected by international copyright laws.
*
*               uC/OS-III is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*             You can find our product's user manual, API reference, release notes and
*             more information at https://doc.micrium.com.
*             You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                        Generic Coldfire with EMAC Port for CodeWarrior Compiler
*
*                                          Micrium uC/OS-III
*
* Filename      : OS_CPU.H 
* Version       : V3.05.01
* Programmer(s) : FGK
*                 JD
*********************************************************************************************************
* Note(s)       : None
*********************************************************************************************************
*/

#ifndef OS_CPU_H
#define OS_CPU_H

#include  <cpu.h>

#ifdef    OS_CPU_GLOBALS
#define   OS_CPU_EXT
#else
#define   OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/*
**********************************************************************************************************
*                                          Miscellaneous
**********************************************************************************************************
*/

#define  OS_TASK_SW()          asm(TRAP #14;)                           /* Use Trap #14 to perform a Task Level Context Switch */
                          
#define  OS_STK_GROWTH             1                                    /* Define stack growth: 1 = Down, 0 = Up               */


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

#if      CPU_CFG_TS_TMR_EN == DEF_ENABLED  
#define  OS_TS_GET()               (CPU_TS)CPU_TS_TmrRd()               /* See Note #2a.                                          */
#else
#define  OS_TS_GET()                  (CPU_TS)0
#endif


/*
************************************************************************************************************************
*                                                   GLOBAL VARIABLES
************************************************************************************************************************
*/

OS_CPU_EXT  CPU_STK  *OS_CPU_ExceptStkBase;


/* 
*********************************************************************************************************
*                                          ColdFire Specifics
*********************************************************************************************************
*/

#define  OS_INITIAL_SR        0x2000                                    /* Supervisor mode, interrupts enabled                 */

#define  OS_TRAP_NBR              14                                    /* OSCtxSw() invoked through TRAP #14                  */


/*
**********************************************************************************************************
*                                         Function Prototypes
**********************************************************************************************************
*/

void  OSStartHighRdy(void); 
void  OSIntCtxSw    (void); 
void  OSCtxSw       (void);


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif                                                          /* End of CPU cfg module inclusion.                     */
