;
;********************************************************************************************************
;                                                uC/OS-III
;                                          The Real-Time Kernel
;
;
;                           (c) Copyright 2009-2015; Micrium, Inc.; Weston, FL
;                    All rights reserved.  Protected by international copyright laws.
;
;                                     Generic ARM Cortex-A50 Port
;
; File      : os_cpu_a.s
; Version   : V3.05.01
; By        : JBL
;
; For       : ARM Cortex-A50
; Mode      : AArch64
; Toolchain : ARM Compiler Toolchain
;********************************************************************************************************
;

;********************************************************************************************************
;                                          PUBLIC FUNCTIONS
;********************************************************************************************************
                                                                ; External references.
    IMPORT  OSRunning
    IMPORT  OSPrioCur
    IMPORT  OSPrioHighRdy
    IMPORT  OSTCBCurPtr
    IMPORT  OSTCBHighRdyPtr
    IMPORT  OSIntNestingCtr
    IMPORT  OSIntExit
    IMPORT  OSTaskSwHook

    IMPORT  OS_CPU_ExceptStkBase

                                                                ; Functions declared in this file.
    EXPORT  OSStartHighRdy
    EXPORT  OSCtxSw
    EXPORT  OSIntCtxSw
    EXPORT  OS_CPU_SPSRGet
    EXPORT  OS_CPU_SIMDGet


;********************************************************************************************************
;                                     CODE GENERATION DIRECTIVES
;********************************************************************************************************

    REQUIRE8
    PRESERVE8

    AREA CODE, CODE, READONLY, ALIGN = 3

    GBLL OS_CPU_EL3
    IF :DEF: OPT_RUN_EL3
    IF OPT_RUN_EL3 == {TRUE}
OS_CPU_EL3 SETL {TRUE}
    ENDIF
    ENDIF


    GBLL OS_CPU_SIMD
    IF :DEF: OPT_SIMD_DIS
        IF OPT_SIMD_DIS == {FALSE}
OS_CPU_SIMD SETL {TRUE}
        ENDIF
    ELSE
OS_CPU_SIMD SETL {TRUE}
    ENDIF


;********************************************************************************************************
;                                   SIMD REGISTERS MACROS
;********************************************************************************************************


    MACRO
    OS_CPU_ARM_REG_POP

        IF OS_CPU_SIMD == {TRUE}
        LDP  q0, q1, [sp], #32
        LDP  q2, q3, [sp], #32
        LDP  q4, q5, [sp], #32
        LDP  q6, q7, [sp], #32
        LDP  q8, q9, [sp], #32
        LDP  q10, q11, [sp], #32
        LDP  q12, q13, [sp], #32
        LDP  q14, q15, [sp], #32
        LDP  q16, q17, [sp], #32
        LDP  q18, q19, [sp], #32
        LDP  q20, q21, [sp], #32
        LDP  q22, q23, [sp], #32
        LDP  q24, q25, [sp], #32
        LDP  q26, q27, [sp], #32
        LDP  q28, q29, [sp], #32
        LDP  q30, q31, [sp], #32

        LDP  x28, x29, [sp], #16
        MSR  FPSR, x28
        MSR  FPCR, x29
        ENDIF

	    LDP  x0, x1, [sp], #16
	    IF OS_CPU_EL3 == {TRUE}
	    MSR  SPSR_EL3, x1
	    ELSE
	    MSR  SPSR_EL1, x1
	    ENDIF

	    LDP  x30, x0, [sp], #16
        IF OS_CPU_EL3 == {TRUE}
	    MSR  ELR_EL3, x0
	    ELSE
	    MSR  ELR_EL1, x0
	    ENDIF

	    LDP  x0, x1, [sp], #16
	    LDP  x2, x3, [sp], #16
	    LDP  x4, x5, [sp], #16
	    LDP  x6, x7, [sp], #16
	    LDP  x8, x9, [sp], #16
	    LDP  x10, x11, [sp], #16
	    LDP  x12, x13, [sp], #16
	    LDP  x14, x15, [sp], #16
	    LDP  x16, x17, [sp], #16
	    LDP  x18, x19, [sp], #16
	    LDP  x20, x21, [sp], #16
	    LDP  x22, x23, [sp], #16
	    LDP  x24, x25, [sp], #16
	    LDP  x26, x27, [sp], #16
	    LDP  x28, x29, [sp], #16
    MEND

    MACRO
    OS_CPU_ARM_REG_PUSH
        STP  x28, x29, [sp, #-16]!
	    STP  x26, x27, [sp, #-16]!
	    STP  x24, x25, [sp, #-16]!
	    STP  x22, x23, [sp, #-16]!
	    STP  x20, x21, [sp, #-16]!
	    STP  x18, x19, [sp, #-16]!
	    STP  x16, x17, [sp, #-16]!
	    STP  x14, x15, [sp, #-16]!
	    STP  x12, x13, [sp, #-16]!
	    STP  x10, x11, [sp, #-16]!
	    STP  x8, x9, [sp, #-16]!
	    STP  x6, x7, [sp, #-16]!
	    STP  x4, x5, [sp, #-16]!
	    STP  x2, x3, [sp, #-16]!
	    STP  x0, x1, [sp, #-16]!

        IF OS_CPU_EL3 == {TRUE}
	    MRS  x0, ELR_EL3
	    ELSE
	    MRS  x0, ELR_EL1
	    ENDIF

	    STP  x30, x0, [sp, #-16]!

        IF OS_CPU_EL3 == {TRUE}
	    MRS  x0, SPSR_EL3
	    ELSE
	    MRS  x0, SPSR_EL1
	    ENDIF

	    MOV  x1, x0
	    STP  x0, x1, [sp, #-16]!

        IF OS_CPU_SIMD == {TRUE}
        MRS  x28, FPSR
        MRS  x29, FPCR
        STP  x28, x29, [sp, #-16]!

	    STP  q30, q31, [sp, #-32]!
        STP  q28, q29, [sp, #-32]!
        STP  q26, q27, [sp, #-32]!
        STP  q24, q25, [sp, #-32]!
        STP  q22, q23, [sp, #-32]!
        STP  q20, q21, [sp, #-32]!
        STP  q18, q19, [sp, #-32]!
        STP  q16, q17, [sp, #-32]!
        STP  q14, q15, [sp, #-32]!
        STP  q12, q13, [sp, #-32]!
        STP  q10, q11, [sp, #-32]!
        STP  q8, q9, [sp, #-32]!
        STP  q6, q7, [sp, #-32]!
        STP  q4, q5, [sp, #-32]!
        STP  q2, q3, [sp, #-32]!
        STP  q0, q1, [sp, #-32]!
        ENDIF
    MEND

    MACRO
    OS_CPU_ARM_REG_PUSHF
	    STP  x28, x29, [sp, #-16]!
	    STP  x26, x27, [sp, #-16]!
	    STP  x24, x25, [sp, #-16]!
	    STP  x22, x23, [sp, #-16]!
	    STP  x20, x21, [sp, #-16]!
	    STP  x18, x19, [sp, #-16]!
        SUB  sp, sp, #144

	    MOV  x0, x30
	    STP  x30, x0, [sp, #-16]!

        IF OS_CPU_EL3 == {TRUE}
        MOV  x0, #0x0000020D
        ELSE
        MOV  x0, #0x00000205
        ENDIF
	    MOV  x1, x0
	    STP  x0, x1, [sp, #-16]!

        IF OS_CPU_SIMD == {TRUE}
        MRS  x28, FPSR
        MRS  x29, FPCR
        STP  x28, x29, [sp, #-16]!

        SUB  sp, sp, #256
        STP  q14, q15, [sp, #-32]!
        STP  q12, q13, [sp, #-32]!
        STP  q10, q11, [sp, #-32]!
        STP  q8, q9, [sp, #-32]!
        SUB  sp, sp, #128
        ENDIF
    MEND

;********************************************************************************************************
;                                         START MULTITASKING
;                                      void OSStartHighRdy(void)
;
; Note(s) : 1) OSStartHighRdy() MUST:
;              a) Call OSTaskSwHook() then,
;              b) Set OSRunning to OS_STATE_OS_RUNNING,
;              c) Switch to the highest priority task.
;********************************************************************************************************

OSStartHighRdy

    BL   OSTaskSwHook

    LDR  x0, =OSTCBHighRdyPtr
    LDR  x1, [x0]
    LDR  x2, [x1]
    MOV  sp, x2

    IF OS_CPU_SIMD == {TRUE}
    LDP  q0, q1, [sp], #32
    LDP  q2, q3, [sp], #32
    LDP  q4, q5, [sp], #32
    LDP  q6, q7, [sp], #32
    LDP  q8, q9, [sp], #32
    LDP  q10, q11, [sp], #32
    LDP  q12, q13, [sp], #32
    LDP  q14, q15, [sp], #32
    LDP  q16, q17, [sp], #32
    LDP  q18, q19, [sp], #32
    LDP  q20, q21, [sp], #32
    LDP  q22, q23, [sp], #32
    LDP  q24, q25, [sp], #32
    LDP  q26, q27, [sp], #32
    LDP  q28, q29, [sp], #32
    LDP  q30, q31, [sp], #32

    LDP  x28, x29, [sp], #16
    MSR  FPSR, x28
    MSR  FPCR, x29
    ENDIF


    LDP  x0, x1, [sp], #16
    LDP  x2, x3, [sp], #16

    MRS  x4, CurrentEL
    LSR  x4, x4, #2
    CMP  x4, #3 ; EL3
    B.EQ  OSStartHighRdy_EL3
    CMP  x4, #2 ; EL2
    B.EQ  OSStartHighRdy_EL2
    CMP  x4, #1 ; EL1
    B.EQ  OSStartHighRdy_EL1

    B . ; Can't run the kernel from EL0

OSStartHighRdy_EL3
    MSR  SPSR_EL3, x1
    MSR  ELR_EL3, x3
    B  OSStartHighRdy_Restore

OSStartHighRdy_EL2
    MSR  SPSR_EL2, x1
    MSR  ELR_EL2, x3
    B  OSStartHighRdy_Restore

OSStartHighRdy_EL1
    MSR  SPSR_EL1, x1
    MSR  ELR_EL1, x3
    B  OSStartHighRdy_Restore

OSStartHighRdy_Restore
    IF OS_CPU_EL3 == {FALSE}
    MOV  x0, sp
    SUB  x0, x0, #240
    MSR  SP_EL1, x0
    ENDIF

    LDP  x0, x1, [sp], #16
    LDP  x2, x3, [sp], #16
    LDP  x4, x5, [sp], #16
    LDP  x6, x7, [sp], #16
    LDP  x8, x9, [sp], #16
    LDP  x10, x11, [sp], #16
    LDP  x12, x13, [sp], #16
    LDP  x14, x15, [sp], #16
    LDP  x16, x17, [sp], #16
    LDP  x18, x19, [sp], #16
    LDP  x20, x21, [sp], #16
    LDP  x22, x23, [sp], #16
    LDP  x24, x25, [sp], #16
    LDP  x26, x27, [sp], #16
    LDP  x28, x29, [sp], #16
    ERET


;********************************************************************************************************
;                       PERFORM A CONTEXT SWITCH (From task level) - OSCtxSw()
;
; Note(s) : 1) OSCtxSw() is called with interrupts DISABLED.
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) Save the current task's context onto the current task's stack,
;              b) OSTCBCurPtr->StkPtr = SP;
;              c) OSTaskSwHook();
;              d) OSPrioCur           = OSPrioHighRdy;
;              e) OSTCBCurPtr         = OSTCBHighRdyPtr;
;              f) SP                  = OSTCBHighRdyPtr->StkPtr;
;              g) Restore the new task's context from the new task's stack,
;              h) Return to new task's code.
;
;           3) Upon entry:
;              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
;              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
;********************************************************************************************************

OSCtxSw

    OS_CPU_ARM_REG_PUSHF

    LDR x0, =OSTCBCurPtr
    LDR x1, [x0]
    MOV x2, sp
    STR x2, [x1]


    BL OSTaskSwHook


    LDR x0, =OSPrioCur
    LDR x1, =OSPrioHighRdy
    LDRB w2, [x1]
    STRB w2, [x0]


    LDR x0, =OSTCBCurPtr
    LDR x1, =OSTCBHighRdyPtr
    LDR x2, [x1]
    STR x2, [x0]


    LDR x0, [x2]
    MOV sp, x0

    OS_CPU_ARM_REG_POP
    ERET


;********************************************************************************************************
;                   PERFORM A CONTEXT SWITCH (From interrupt level) - OSIntCtxSw()
;
; Note(s) : 1) OSIntCtxSw() is called with interrupts DISABLED.
;
;           2) The pseudo-code for OSCtxSw() is:
;              a) OSTaskSwHook();
;              b) OSPrioCur   = OSPrioHighRdy;
;              c) OSTCBCurPtr = OSTCBHighRdyPtr;
;              d) SP          = OSTCBHighRdyPtr->OSTCBStkPtr;
;              e) Restore the new task's context from the new task's stack,
;              f) Return to new task's code.
;
;           3) Upon entry:
;              OSTCBCurPtr      points to the OS_TCB of the task to suspend,
;              OSTCBHighRdyPtr  points to the OS_TCB of the task to resume.
;********************************************************************************************************

OSIntCtxSw
    BL OSTaskSwHook

    LDR x0, =OSPrioCur
    LDR x1, =OSPrioHighRdy
    LDRB w2, [x1]
    STRB w2, [x0]


    LDR x0, =OSTCBCurPtr
    LDR x1, =OSTCBHighRdyPtr
    LDR x2, [x1]
    STR x2, [x0]


    LDR x0, [x2]
    MOV sp, x0

    OS_CPU_ARM_REG_POP

    ERET


    EXPORT OS_CPU_ARM_ExceptIrqHndlr

OS_CPU_ARM_ExceptIrqHndlr

    OS_CPU_ARM_REG_PUSH

    LDR  x0, =OSIntNestingCtr
    LDRB w1, [x0]
    ADD  w1, w1, #1
    STRB w1, [x0]
    CMP  w1, #1
    BNE  OS_CPU_ARM_ExceptHndlr_BreakExcept

    LDR  x0, =OSTCBCurPtr
    LDR  x1, [x0]
    MOV  x2, sp
    STR  x2, [x1]

    LDR  x0, =OS_CPU_ExceptStkBase
    LDR  x1, [x0]
    MOV  sp, x1

    IMPORT  OS_CPU_ExceptHndlr
    BL   OS_CPU_ExceptHndlr

    BL   OSIntExit

    LDR  x0, =OSTCBCurPtr
    LDR  x1, [x0]
    LDR  x2, [x1]
    MOV  sp, x2

    OS_CPU_ARM_REG_POP

    ERET


OS_CPU_ARM_ExceptHndlr_BreakExcept
    IMPORT  OS_CPU_ExceptHndlr
    BL   OS_CPU_ExceptHndlr

    LDR  x0, =OSIntNestingCtr
    LDRB w1, [x0]
    SUB  w1, w1, #1
    STRB w1, [x0]

    OS_CPU_ARM_REG_POP

    ERET


OS_CPU_SPSRGet
    IF OS_CPU_EL3 == {TRUE}
    MOV x0, #0x0000000D
    ELSE
    MOV x0, #0x00000005
    ENDIF

    RET


OS_CPU_SIMDGet
    IF OS_CPU_SIMD == {TRUE}
    MOV x0, #1
    ELSE
    MOV x0, #0
    ENDIF

    RET

    END
