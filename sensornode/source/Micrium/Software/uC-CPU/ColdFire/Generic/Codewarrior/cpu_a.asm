
;********************************************************************************************************
;                                                uC/CPU
;                                    CPU CONFIGURATION & PORT LAYER
;
;                          (c) Copyright 2004-2015; Micrium, Inc.; Weston, FL
;
;               All rights reserved.  Protected by international copyright laws.
;
;               uC/CPU is provided in source form to registered licensees ONLY.  It is 
;               illegal to distribute this source code to any third party unless you receive 
;               written permission by an authorized Micrium representative.  Knowledge of 
;               the source code may NOT be used to develop a similar product.
;
;               Please help us continue to provide the Embedded community with the finest 
;               software available.  Your honesty is greatly appreciated.
;
;               You can find our product's user manual, API reference, release notes and
;               more information at https://doc.micrium.com.
;               You can contact us at www.micrium.com.
;********************************************************************************************************
;


;********************************************************************************************************
;
;                                            CPU PORT FILE
;
;                                              ColdFire
;                                             Codewarrior
;
; Filename      : cpu_a.asm
; Version       : V1.30.02.00
; Programmer(s) : JJL
;                 FGK
;********************************************************************************************************



;********************************************************************************************************
;                                         PUBLIC DECLARATIONS
;********************************************************************************************************


        .global  _CPU_VectInit

        .global  _CPU_SR_Save
        .global  _CPU_SR_Restore


;********************************************************************************************************
;                                         EXTERNAL DECLARATIONS
;********************************************************************************************************


        .extern  _CPU_VBR_Ptr
        .text

/*
;********************************************************************************************************
;                                 VECTOR BASE REGISTER INITIALIZATION
;
; Description : This function is called to set the Vector Base Register to the value specified in
;               the function argument.
;
; Argument(s) : VBR         Desired vector base address.
;
; Return(s)   : none.
;
; Note(s)     : 'CPU_VBR_Ptr' keeps the current vector base address.
;********************************************************************************************************
*/

_CPU_VectInit:

        MOVE.L  D0,-(A7)                                        /* Save D0                                              */
        MOVE.L  8(A7),D0                                        /* Retrieve 'vbr' parameter from stack                  */
        MOVE.L  D0,_CPU_VBR_Ptr                                 /* Save 'vbr' into CPU_VBR_Ptr                          */
        MOVEC   D0,VBR
        MOVE.L  (A7)+,D0                                        /* Restore D0                                           */
        RTS


/*
;********************************************************************************************************
;                               CPU_SR_Save() for OS_CRITICAL_METHOD #3
;
; Description : This functions implements the OS_CRITICAL_METHOD #3 function to preserve the state of the
;               interrupt disable flag in order to be able to restore it later.
;
; Argument(s) : none.
;
; Return(s)   : It is assumed that the return value is placed in the D0 register as expected by the
;               compiler.
;
; Note(s)     : none.
;********************************************************************************************************
*/

_CPU_SR_Save:

        MOVE.W  SR,D0                                           /* Copy SR into D0                                      */
        MOVE.L  D0,-(A7)                                        /* Save D0                                              */
        ORI.L   #0x0700,D0                                      /* Disable interrupts                                   */
        MOVE.W  D0,SR                                           /* Restore SR state with interrupts disabled            */
        MOVE.L  (A7)+,D0                                        /* Restore D0                                           */
        RTS


/*
;********************************************************************************************************
;                             CPU_SR_Restore() for OS_CRITICAL_METHOD #3
;
; Description : This functions implements the OS_CRITICAL_METHOD #function to restore the state of the
;               interrupt flag.
;
; Argument(s) : cpu_sr      Contents of the SR to restore.  It is assumed that 'cpu_sr' is passed in the stack.
;
; Return(s)   : none.
;
; Note(s)     : none.
;********************************************************************************************************
*/

_CPU_SR_Restore:

        MOVE.L  D0,-(A7)                                        /* Save D0                                              */
        MOVE.W  10(A7),D0                                       /* Retrieve cpu_sr parameter from stack                 */
        MOVE.W  D0,SR                                           /* Restore SR previous state                            */
        MOVE.L  (A7)+,D0                                        /* Restore D0                                           */
        RTS


/*
;********************************************************************************************************
;                                     CPU ASSEMBLY PORT FILE END
;********************************************************************************************************
*/

        .end

