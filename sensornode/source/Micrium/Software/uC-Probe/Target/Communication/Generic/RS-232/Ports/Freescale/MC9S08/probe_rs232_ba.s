;********************************************************************************************************
;                                               uC/OS-II
;                                         The Real-Time Kernel
;
;                         (c) Copyright 2002, Jean J. Labrosse, Weston, FL
;                                          All Rights Reserved
;
;
;                                       Banked MC9S08 Specific code
;                                            (Codewarrior)
;
; File         : probe_rs232_ba.s
; By           : Eric Shufro
; Notes        : THIS FILE *MUST* BE LINKED INTO NON_BANKED MEMORY!
;********************************************************************************************************

NON_BANKED:       section  

;********************************************************************************************************
;                                           I/O PORT ADDRESSES
;********************************************************************************************************

PPAGE:            equ    $0078         ; Addres of PPAGE register (Using MC9S08QE128 as a reference)

;********************************************************************************************************
;                                          PUBLIC DECLARATIONS
;********************************************************************************************************

    xdef   ProbeRS232_RxISR   
    xdef   ProbeRS232_TxISR   
    xdef   ProbeRS232_ErrISR           
                      
;********************************************************************************************************
;                                         EXTERNAL DECLARATIONS
;********************************************************************************************************
   
    xref   OSIntExit
    xref   OSIntNesting  
    xref   OSTCBCur     
   
    xref   ProbeRS232_RxISRHandler 
    xref   ProbeRS232_TxISRHandler 
    xref   ProbeRS232_ErrISRHandler             
    
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

ProbeRS232_RxISR:
    lda    PPAGE                      ; Get current value of PPAGE register                                
    psha                              ; Push PPAGE register onto current task's stack

    pshh                              ; Push the H register on to the stack

    lda    OSIntNesting               ; 
    add    #1                         ; Notify uC/OS-II about the interrupt   
    sta    OSIntNesting
    
    cmpa   #$01                       ; if (OSIntNesting == 1) {    
    bne    ProbeRS232_RxISR1            ;
    
    tsx                               ;     Copy the stack pointer into HX
    pshx                              ;     Store the stack pointer on the stack
    pshh
    ldhx   OSTCBCur                   ;     Obtain OSTCBCur in HX

    pula                              ;     OSTCBCur->OSTCBStkPtr = Stack Pointer     
    sta    0, x                       
    pula
    sta    1, x                       ; }
        
ProbeRS232_RxISR1:
    call   ProbeRS232_RxISRHandler    ; Call Rx ISR handler. (See probe_rs232c.c)

    call   OSIntExit                  ; Notify uC/OS-II about end of ISR, a context switch may occur from within OSIntExit().
    
    pulh                              ; No context switch necessary, restore the H register

    pula                              ; Get value of PPAGE register
    sta    PPAGE                      ; Store into CPU's PPAGE register                                
    
    rti                               ; Return from interrupt to interrupted task.
    
;********************************************************************************************************
;                                           uC/Probe Tx ISR
;
; Description : This routine is the uC/Probe Tx interrupt service routine
;
; Arguments   : none
;
; Notes       : 1) All USER interrupts should be modeled EXACTLY like this where the only
;                  line to be modified is the call to your ISR_Handler and perhaps the call to
;                  the label name ProbeRS232_TxISRHandler.
;********************************************************************************************************

ProbeRS232_TxISR:
    lda    PPAGE                      ; Get current value of PPAGE register                                
    psha                              ; Push PPAGE register onto current task's stack

    pshh                              ; Push the H register on to the stack

    lda    OSIntNesting               ; 
    add    #1                         ; Notify uC/OS-II about the interrupt   
    sta    OSIntNesting
    
    cmpa   #$01                       ; if (OSIntNesting == 1) {    
    bne    ProbeRS232_TxISR1            ;
    
    tsx                               ;     Copy the stack pointer into HX
    pshx                              ;     Store the stack pointer on the stack
    pshh
    ldhx   OSTCBCur                   ;     Obtain OSTCBCur in HX

    pula                              ;     OSTCBCur->OSTCBStkPtr = Stack Pointer     
    sta    0, x                       
    pula
    sta    1, x                       ; }
        
ProbeRS232_TxISR1:
    call   ProbeRS232_TxISRHandler    ; Call Tx ISR handler. (See probe_rs232c.c)

    call   OSIntExit                  ; Notify uC/OS-II about end of ISR, a context switch may occur from within OSIntExit().
    
    pulh                              ; No context switch necessary, restore the H register
 
    pula                              ; Get value of PPAGE register
    sta    PPAGE                      ; Store into CPU's PPAGE register                                
   
    rti                               ; Return from interrupt to interrupted task.
    
;********************************************************************************************************
;                                           uC/Probe SCI Error ISR
;
; Description : This routine is the SCI Error interrupt service routine
;
; Arguments   : none
;
; Notes       : 1) All USER interrupts should be modeled EXACTLY like this where the only
;                  line to be modified is the call to your ISR_Handler and perhaps the call to
;                  the label name ProbeRS232_ErrISRHandler.
;********************************************************************************************************

ProbeRS232_ErrISR:
    lda    PPAGE                      ; Get current value of PPAGE register                                
    psha                              ; Push PPAGE register onto current task's stack

    pshh                              ; Push the H register on to the stack

    lda    OSIntNesting               ; 
    add    #1                         ; Notify uC/OS-II about the interrupt   
    sta    OSIntNesting
    
    cmpa   #$01                       ; if (OSIntNesting == 1) {    
    bne    ProbeRS232_ErrISR1            ;
    
    tsx                               ;     Copy the stack pointer into HX
    pshx                              ;     Store the stack pointer on the stack
    pshh
    ldhx   OSTCBCur                   ;     Obtain OSTCBCur in HX

    pula                              ;     OSTCBCur->OSTCBStkPtr = Stack Pointer     
    sta    0, x                       
    pula
    sta    1, x                       ; }
        
ProbeRS232_ErrISR1:
    call   ProbeRS232_ErrISRHandler   ; Call the SCI Error handler (see probe_RS232c.c)

    call   OSIntExit                  ; Notify uC/OS-II about end of ISR, a context switch may occur from within OSIntExit().
    
    pulh                              ; No context switch necessary, restore the H register

    pula                              ; Get value of PPAGE register
    sta    PPAGE                      ; Store into CPU's PPAGE register                                
    
    rti                               ; Return from interrupt to interrupted task.
    

    
