/*
*********************************************************************************************************
*                                                uC/OS-III
*                                          The Real-Time Kernel
*
*
*                           (c) Copyright 2009-2015; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                            Generic ARM Cortex-R4 Port
*
* File      : OS_CPU.H
* Version   : V3.05.01
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
* For       : ARM Cortex-R4 (ARMv7)
* Mode      : ARM or Thumb
* Toolchain : Code Composer Studio (CCS) V4.xx and higher.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                               WARNING - DEPRECATION NOTICE - WARNING
* May 2015
* This file is part of a deprecated port and will be removed in a future release.
* The functionalities of this port were replaced by the generic ARM-Cortex-A port.
*********************************************************************************************************
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
*                                       CONFIGURATION DEFAULTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#if  ((defined(__TI_VFP_SUPPORT__     ) && (__TI_VFP_SUPPORT__      == 1))   || \
      (defined(__TI_VFPV3_SUPPORT__   ) && (__TI_VFPV3_SUPPORT__    == 1))   || \
      (defined(__TI_VFPV3D16_SUPPORT__) && (__TI_VFPV3D16_SUPPORT__ == 1)))
#define  OS_CPU_ARM_FP_EN                              DEF_ENABLED
#else
#define  OS_CPU_ARM_FP_EN                              DEF_DISABLED
#endif
#define  OS_CPU_ARM_FP_REG_NBR                           32u


/*
*********************************************************************************************************
*                                       ARM EXCEPTION DEFINES
*********************************************************************************************************
*/

                                                            /* ARM exception IDs                                      */
#define  OS_CPU_ARM_EXCEPT_RST                         0x00u
#define  OS_CPU_ARM_EXCEPT_UND                         0x01u
#define  OS_CPU_ARM_EXCEPT_SWI                         0x02u
#define  OS_CPU_ARM_EXCEPT_ABORT_PREFETCH              0x03u
#define  OS_CPU_ARM_EXCEPT_ABORT_DATA                  0x04u
#define  OS_CPU_ARM_EXCEPT_RSVD                        0x05u
#define  OS_CPU_ARM_EXCEPT_IRQ                         0x06u
#define  OS_CPU_ARM_EXCEPT_FIQ                         0x07u


                                                            /* ARM exception vectors addresses                        */
#define  OS_CPU_ARM_EXCEPT_VECT_ADDR_RST                (OS_CPU_ARM_EXCEPT_RST            * 0x04u + 0x00u)
#define  OS_CPU_ARM_EXCEPT_VECT_ADDR_UND                (OS_CPU_ARM_EXCEPT_UND            * 0x04u + 0x00u)
#define  OS_CPU_ARM_EXCEPT_VECT_ADDR_SWI                (OS_CPU_ARM_EXCEPT_SWI            * 0x04u + 0x00u)
#define  OS_CPU_ARM_EXCEPT_VECT_ADDR_ABORT_PREFETCH     (OS_CPU_ARM_EXCEPT_ABORT_PREFETCH * 0x04u + 0x00u)
#define  OS_CPU_ARM_EXCEPT_VECT_ADDR_ABORT_DATA         (OS_CPU_ARM_EXCEPT_ABORT_DATA     * 0x04u + 0x00u)
#define  OS_CPU_ARM_EXCEPT_VECT_ADDR_IRQ                (OS_CPU_ARM_EXCEPT_IRQ            * 0x04u + 0x00u)
#define  OS_CPU_ARM_EXCEPT_VECT_ADDR_FIQ                (OS_CPU_ARM_EXCEPT_FIQ            * 0x04u + 0x00u)

                                                            /* ARM exception handlers addresses                       */
#define  OS_CPU_ARM_EXCEPT_HANDLER_ADDR_RST             (OS_CPU_ARM_EXCEPT_RST            * 0x04u + 0x20u)
#define  OS_CPU_ARM_EXCEPT_HANDLER_ADDR_UND             (OS_CPU_ARM_EXCEPT_UND            * 0x04u + 0x20u)
#define  OS_CPU_ARM_EXCEPT_HANDLER_ADDR_SWI             (OS_CPU_ARM_EXCEPT_SWI            * 0x04u + 0x20u)
#define  OS_CPU_ARM_EXCEPT_HANDLER_ADDR_ABORT_PREFETCH  (OS_CPU_ARM_EXCEPT_ABORT_PREFETCH * 0x04u + 0x20u)
#define  OS_CPU_ARM_EXCEPT_HANDLER_ADDR_ABORT_DATA      (OS_CPU_ARM_EXCEPT_ABORT_DATA     * 0x04u + 0x20u)
#define  OS_CPU_ARM_EXCEPT_HANDLER_ADDR_IRQ             (OS_CPU_ARM_EXCEPT_IRQ            * 0x04u + 0x20u)
#define  OS_CPU_ARM_EXCEPT_HANDLER_ADDR_FIQ             (OS_CPU_ARM_EXCEPT_FIQ            * 0x04u + 0x20u)

                                                            /* ARM "Jump To Self" asm instruction                     */
#define  OS_CPU_ARM_INSTR_JUMP_TO_SELF                 0xEAFFFFFEu
                                                            /* ARM "Jump To Exception Handler" asm instruction        */
#define  OS_CPU_ARM_INSTR_JUMP_TO_HANDLER              0xE59FF018u

/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

#define  OS_TASK_SW()               OSCtxSw()

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
#define  OS_TS_GET()               (CPU_TS)CPU_TS_TmrRd()   /* See Note #2a.                                          */
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

OS_CPU_EXT  CPU_STK  *OS_CPU_ExceptStkBase;
OS_CPU_EXT  CPU_STK  *OS_CPU_ExceptStkPtr;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/


void  OSCtxSw                            (void);
void  OSIntCtxSw                         (void);
void  OSStartHighRdy                     (void);
void  OS_CPU_InitExceptVect              (void);

void  OS_CPU_ARM_ExceptUndefInstrHndlr   (void);
void  OS_CPU_ARM_ExceptSwiHndlr          (void);
void  OS_CPU_ARM_ExceptPrefetchAbortHndlr(void);
void  OS_CPU_ARM_ExceptDataAbortHndlr    (void);
void  OS_CPU_ARM_ExceptIrqHndlr          (void);
void  OS_CPU_ARM_ExceptFiqHndlr          (void);

void  OS_CPU_IntHandler                  (CPU_INT32U  src_id);


#ifdef __cplusplus
}
#endif

#endif
