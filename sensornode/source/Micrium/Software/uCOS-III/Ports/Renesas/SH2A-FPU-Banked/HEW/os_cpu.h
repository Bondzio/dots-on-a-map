/*
************************************************************************************************************************
*                                                      uC/OS-III
*                                                The Real-Time Kernel
*
*
*                                      (c) Copyright 2010-2015, Micrium, Weston, FL
*                                                 All Rights Reserved
*
*                                           Renesas SH2A-FPU Specific code
*                                          Renesas SH SERIES C/C++ Compiler
*                                          
*
* File      : OS_CPU.H
* Version   : V3.05.01
* By        : HMS
*********************************************************************************************************
*/

#ifndef  OS_CPU_H
#define  OS_CPU_H


#ifdef   OS_CPU_GLOBALS
#define  OS_CPU_EXT
#else
#define  OS_CPU_EXT  extern
#endif

#include <machine.h>


#ifdef __cplusplus
extern  "C" {
#endif

/*
************************************************************************************************************************
*                                                CONFIGURATION DEFAULTS
************************************************************************************************************************
*/

#ifndef  OS_CPU_FPU_EN
#define  OS_CPU_FPU_EN    1u                                /* Harware floating point support enabled by default      */
#endif

/*
************************************************************************************************************************
*                                                        MACROS
************************************************************************************************************************
*/

#define  OS_TASK_SW_VECT           33u                      /* Interrupt vector # used for context switch             */

#define  OS_TASK_SW()              trapa(OS_TASK_SW_VECT);

#if      OS_CFG_TS_EN == 1u
#define  OS_TS_GET()               (CPU_TS)CPU_TS_Get32()
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif

/*
************************************************************************************************************************
*                                                 FUNCTION PROTOTYPES
************************************************************************************************************************
*/

CPU_INT32U  OS_Get_GBR       (void);
void        OS_C_ISR_Save    (void);
void        OS_C_ISR_Restore (void);
void        OSCtxSw          (void);
void        OSIntCtxSw       (void);
void        OSStartHighRdy   (void);
void        OSTickISR        (void);


#ifdef __cplusplus
}
#endif

#endif
