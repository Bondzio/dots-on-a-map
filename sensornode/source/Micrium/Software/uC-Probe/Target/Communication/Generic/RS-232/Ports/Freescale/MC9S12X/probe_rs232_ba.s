;********************************************************************************************************
;                                               uC/OS-II
;                                         The Real-Time Kernel
;
;                         (c) Copyright 2002, Jean J. Labrosse, Weston, FL
;                                          All Rights Reserved
;
;
;                                       PAGED S12X Specific code
;                                            (CODEWARRIOR)
;
; File         : probe_rs232_ba.s
; By           : Eric Shufro
;
; Notes        : THIS FILE *MUST* BE LINKED INTO NON_BANKED MEMORY!
;********************************************************************************************************

NON_BANKED:       section


;********************************************************************************************************
;                                           I/O PORT ADDRESSES
;********************************************************************************************************

PPAGE:            equ    $0015         ; Addres of PPAGE register (assuming MC9S12XEP100 part)
RPAGE:            equ    $0016         ; Addres of RPAGE register (assuming MC9S12XEP100 part)
EPAGE:            equ    $0017         ; Addres of EPAGE register (assuming MC9S12XEP100 part)
GPAGE:            equ    $0010         ; Addres of GPAGE register (assuming MC9S12XEP100 part)


;********************************************************************************************************
;                                          PUBLIC DECLARATIONS
;********************************************************************************************************

    xdef   ProbeRS232_RxTxISR


;********************************************************************************************************
;                                         EXTERNAL DECLARATIONS
;********************************************************************************************************

    xref   OSIntExit
    xref   OSIntNesting
    xref   OSTCBCur

    xref   ProbeRS232_RxTxISRHandler


;********************************************************************************************************
;                                           uC/Probe Rx ISR
;
; Description : This routine is the uC/Probe Rx interrupt service routine
;
; Arguments   : none
;
; Notes       : 1) All USER interrupts should be modeled EXACTLY like this where the only
;                  line to be modified is the call to your ISR_Handler and perhaps the call to
;                  the label name ProbeRS232_RxISRHandler.
;********************************************************************************************************

ProbeRS232_RxTxISR:
    ldaa   GPAGE                       ; Get current value of GPAGE register
    psha                               ; Push GPAGE register onto current task's stack

    ldaa   EPAGE                       ; Get current value of EPAGE register
    psha                               ; Push EPAGE register onto current task's stack

    ldaa   RPAGE                       ; Get current value of RPAGE register
    psha                               ; Push RPAGE register onto current task's stack

    ldaa   PPAGE                       ; Get current value of PPAGE register
    psha                               ; Push PPAGE register onto current task's stack

    inc    OSIntNesting                ; Notify uC/OS-II about ISR

    ldab   OSIntNesting                ; if (OSIntNesting == 1) {
    cmpb   #$01
    bne    ProbeRS232_RxTxISR1

    ldy    OSTCBCur                    ;     OSTCBCur->OSTCBStkPtr = Stack Pointer
    sts    0,y                         ; }


ProbeRS232_RxTxISR1:
    call   ProbeRS232_RxTxISRHandler  ; Call Rx ISR handler. (See probe_rs232c.c)

;   cli                               ; Optionally enable interrupts to allow interrupt nesting

    call   OSIntExit                  ; Notify uC/OS-II about end of ISR, a context switch may occur from within OSIntExit().

    pula                               ; Get value of PPAGE register
    staa   PPAGE                       ; Store into CPU's PPAGE register

    pula                               ; Get value of RPAGE register
    staa   RPAGE                       ; Store into CPU's RPAGE register

    pula                               ; Get value of EPAGE register
    staa   EPAGE                       ; Store into CPU's EPAGE register

    pula                               ; Get value of GPAGE register
    staa   GPAGE                       ; Store into CPU's GPAGE register

    rti                               ; Return from interrupt to interrupted task.


