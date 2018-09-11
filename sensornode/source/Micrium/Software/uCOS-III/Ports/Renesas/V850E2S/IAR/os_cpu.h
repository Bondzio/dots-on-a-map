/*
*********************************************************************************************************
*                                                uC/OS-III
*                                          The Real-Time Kernel
*
*
*                           (c) Copyright 2009-2015; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                          Renesas V850E2S Port
*
* File      : OS_CPU.H
* Version   : V3.05.01
* By        : FF
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
*           You can find our product's user manual, API reference, release notes and
*           more information at https://doc.micrium.com.
*           You can contact us at www.micrium.com.
*
* For       : Renesas V850E2S
* Toolchain : IAR EWV850 v3.7x and 3.8x
*********************************************************************************************************
*/

#ifndef  OS_CPU_H
#define  OS_CPU_H


#ifdef   OS_CPU_GLOBALS
#define  OS_CPU_EXT
#else
#define  OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                               MACROS
* Note(s) : (1) Ensure that OSCtxSw is registered in the vector address of EITRAP0.
*********************************************************************************************************
*/

#define  OS_TASK_SW()           asm("trap 0x00")          /* See Note #1.                                          */

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
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                        FUNCTION PROTOTYPES
* Note(s) :  (1) OS_CPU_TickInit() must be implemented by the user to initialize the timer, which will
*                be used as the tick interrupt. Moreover, OS_CPU_TickHandler() must be registered in the 
*                proper vector address of timer that will be used as the tick.
*********************************************************************************************************
*/

void       OSCtxSw               (void);
void       OSIntCtxSw            (void);
void       OSStartHighRdy        (void);
                                                             /* See Note # 1.                                        */      
void       OS_CPU_TickHandler    (void);
void       OS_CPU_TickInit       (CPU_INT32U  tick_per_sec);


#ifdef __cplusplus
}
#endif

#endif
