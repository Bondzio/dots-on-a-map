/*---------------------------------------------------------------------------*/
/*                            - cstartup.s -                                 */
/*                                                                           */
/*       This module contains the code executed before the C "main"          */
/*       function is called.                                                 */
/*       The code can be tailored to suit special hardware needs.            */
/*                                                                           */
/*       Copyright 2009-2014 IAR Systems AB.                                 */
/*                                                                           */
/*       Entries:                                                            */
/*         __iar_program_start       C entry point                           */
/*                                                                           */
/*       $Revision: 3046 $                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/


#ifndef __ROPI__
#error __ROPI__ not defined
#endif

;------------------------------------------------------------------------------
; Reset vector.
;------------------------------------------------------------------------------
; The C startup sequence.
;
; Calls __low_level_init to perform early user initialization, after which the
; static data areas are initialized if necessary. Then 'main' and finally 'exit'
; are called.
;------------------------------------------------------------------------------
        module    _CSTARTUP
        section   ISTACK:DATA:NOROOT
        section   USTACK:DATA:NOROOT
        section   .inttable:CONST:NOROOT
        section   .text:CODE:NOROOT(2)

        extern    _BSP_IntVectTbl
        extern    _HardwareInit
        extern    _SoftwareInit
        extern    ___iar_data_init2
        public    __iar_program_start
        require   __cinit
__iar_program_start:
;------------------------------------------------------------------------------
; After a reset, execution will start here.
;------------------------------------------------------------------------------
        code
        mvtc      #(SFE ISTACK),    ISP     ; interrupt stack pointer
        mvtc      #(SFE USTACK),    USP     ; interrupt stack pointer
        mvtc      #_BSP_IntVectTbl, INTB    ; vector address
        bsr       _HardwareInit             ; pre-crt init
        bsr       __cinit                   ; crt init
        bsr       _SoftwareInit             ; software init - SHOULD NOT RETURN
        brk                                 ; trigger NMI


;------------------------------------------------------------------------------
; If the application swithes to user mode, the user stack pointer will
; need initialization.
;------------------------------------------------------------------------------
        ;mvtc      #(SFE USTACK), USP  ; stack pointer

;------------------------------------------------------------------------------
; If the application does not enable interrupts in low_level_init, then the
; initialization of INTB can wait until just before the call to main. This
; allows interrupt handlers and the vector to be placed in RAM.
;------------------------------------------------------------------------------
#if __ROPI__
        ; set up INTB
#else
#endif

        section   .text:CODE:NOROOT
        public    __iar_subn_handling
        code
__iar_subn_handling:
#if __CORE__ != __RX200__
        mvtc      #0, fpsw            ; enable subnormal handling
#endif
        section   .text:CODE:NOROOT
        public    __iar_call_low_level_init
        extern    ___low_level_init
        code

;------------------------------------------------------------------------------
; If hardware must be initiated from assembly or if interrupts
; should be on when reaching main, this is the place to insert
; such code.
;------------------------------------------------------------------------------

__iar_call_low_level_init:
        bsr.a   ___low_level_init
        cmp     #0, r1
        beq     __iar_main_call

;------------------------------------------------------------------------------
; Data initialization.
;
; Initialize the static data areas.
;------------------------------------------------------------------------------
        section .text:CODE:NOROOT
        code
__cinit:
        bsr     ___iar_data_init2

;------------------------------------------------------------------------------
; Call C++ static constructors.
;------------------------------------------------------------------------------

        section .text:CODE:NOROOT
        public  __iar_cstart_call_ctors
        extern  ___call_ctors
        extern  SHT$$PREINIT_ARRAY$$Base
        extern  SHT$$INIT_ARRAY$$Limit
        code
__iar_init$$done1:
__iar_cstart_call_ctors:
#if __ROPI__
        add     #SHT$$PREINIT_ARRAY$$Base - __PID_TOP, r13, r1
        add     #SHT$$INIT_ARRAY$$Limit - __PID_TOP, r13, r2
#else
        mov.l   #SHT$$PREINIT_ARRAY$$Base, r1
        mov.l   #SHT$$INIT_ARRAY$$Limit, r2
#endif
        bsr.a   ___call_ctors

;------------------------------------------------------------------------------
; Call 'main' C function.
;------------------------------------------------------------------------------

        section .text:CODE:NOROOT
        extern PID$$Base
        extern PID$$Limit

;------------------------------------------------------------------------------
; Switch to user mode here if desired.
;------------------------------------------------------------------------------

__iar_init$$done2:
__iar_main_call:
        code
#if __ROPI__
#else
        extern    __iar_reset_vector
        extern    _vector_table
        require   _vector_table
        require   __iar_reset_vector
#endif
        end
