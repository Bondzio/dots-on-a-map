;********************************************************************************************************
;                                                uC/OS-III
;                                          The Real-Time Kernel
;
;
;                           (c) Copyright 2009-2015; Micrium, Inc.; Weston, FL
;                    All rights reserved.  Protected by international copyright laws.
;
;                                              Renesas SH2A
;                            Renesas SH SERIES C/C++ Compiler (V.9.00.03.006)
;
; File    : OS_CPU_A.INC
; Version : V3.05.01
;********************************************************************************************************


;********************************************************************************************************
;                                             OS_CTX_SAVE
;                                      Save CPU and FPU Context
;********************************************************************************************************
        .MACRO    OS_CTX_SAVE                 
        MOV.L     R0,    @-R15				  ; Save TBR
        STC.L     TBR,   R0                   
        MOV.L     R0,    @-R15

        STC.L     GBR,   @-R15                ; Save GBR
        STS.L     PR,    @-R15                ; Save PR

        STS.L     MACL,  @-R15                ; Save DSP registers
        STS.L     MACH,  @-R15

        MOVML.L   R14,   @-R15                ; Save Integer registers
        .ENDM

;********************************************************************************************************
;                                          OS_CTX_SAVE_ISR
;********************************************************************************************************
        .MACRO    OS_CTX_SAVE_ISR              
        MOV.L     R0,    @-R15				  ; Save TBR
        STC.L     TBR,   R0                  
        MOV.L     R0,    @-R15
        .ENDM

;********************************************************************************************************
;*                                          OS_C_ISR_SAVE
;********************************************************************************************************
        .MACRO    OS_C_ISR_SAVE
        MOV.L     R0,    @-R15				  ; Save R0
		STC.L     TBR,   R0                   ; Save TBR
        MOV.L     R0,    @-R15
        .ENDM

;********************************************************************************************************
;                                           OS_CTX_RESTORE
;                                     Restore CPU and FPU Context
;********************************************************************************************************

        .MACRO    OS_CTX_RESTORE              
        MOVML.L   @R15+, R14                  ; Restore Integer registers

        LDS.L     @R15+, MACH                 ; Restore DSP registers
        LDS.L     @R15+, MACL

        LDS.L     @R15+, PR                   ; Restore PR
        LDC.L     @R15+, GBR                  ; Restore GBR

        MOV.L     @R15+, R0                   ; Restore TBR
        LDC       R0,    TBR
        MOV.L     @R15+, R0
        .ENDM
  
;********************************************************************************************************
;                                          OS_CTX_RESTORE_ISR
;********************************************************************************************************

        .MACRO    OS_CTX_RESTORE_ISR           
        MOV.L     @R15+, R0                   ; Restore TBR
        LDC       R0,    TBR
        MOV.L     @R15+, R0
        .ENDM

;********************************************************************************************************
;                                           OS_C_ISR_RESTORE
;********************************************************************************************************

        .MACRO    OS_C_ISR_RESTORE
        MOV.L     @R15+, R0                   ; Restore TBR
        LDC       R0,    TBR
        MOV.L     @R15+, R0
        .ENDM

