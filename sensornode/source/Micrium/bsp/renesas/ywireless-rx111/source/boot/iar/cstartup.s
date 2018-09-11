/*---------------------------------------------------------------------------*/
/*                            - cstartup.s -                                 */
/*                                                                           */
/*       This module contains the code executed before the C "main"          */
/*       function is called.                                                 */
/*       The code can be tailored to suit special hardware needs.            */
/*                                                                           */
/*       Copyright 2009-2010 IAR Systems AB.                                 */
/*                                                                           */
/*       Entries:                                                            */
/*         __iar_program_start       C entry point                           */
/*                                                                           */
/*       $Revision: 4548 $                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                         RX111 RESET VECTOR
*                                                 &
*                                        SYSTEM INITIALIZATION
*
* Filename      : cstartup.s
* Version       : V1.00
* Programmer(s) : JPC
*********************************************************************************************************
*/

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
        extern    _main
        extern    _exit
        extern    ___iar_data_init2
        public    __iar_program_start
        require   __iar_main_call
#if __ROPI__
        extern    __PID_TOP
#endif
        require   __cinit
__iar_program_start:
;------------------------------------------------------------------------------
; After a reset, execution will start here.
;------------------------------------------------------------------------------
        code
        mvtc      #(SFE ISTACK), ISP  ; istack pointer

;------------------------------------------------------------------------------
; If the application swithes to user mode, the user stack pointer will
; need initialization.
;------------------------------------------------------------------------------
        ;mvtc      #(SFE USTACK), USP  ; stack pointer

;------------------------------------------------------------------------------
; If the application moves the exception vector, for example when in ropi mode,
; initialize EXTB here (Core V2 only).
;------------------------------------------------------------------------------
        ;mvtc      #(SFE EXTVEC),EXTB  ; exception vector base pointer

;------------------------------------------------------------------------------
; If the application does not enable interrupts in low_level_init, then the
; initialization of INTB can wait until just before the call to main. This
; allows interrupt handlers and the vector to be placed in RAM.
;------------------------------------------------------------------------------
#if __ROPI__
        ; set up INTB
#else
        mvtc      #(SFB .inttable), INTB ; vector address
#endif

        section   .text:CODE:NOROOT
        public    __iar_init_FINTV
        extern    ___fast_interrupt
        code
__iar_init_FINTV:
        mvtc      #___fast_interrupt:32,fintv  ; setup the fast interrupt

        section   .text:CODE:NOROOT
        public    __iar_subn_handling
        code
__iar_subn_handling:
#if __FPU__
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
        public  __iar_cstart_end
        public  __iar_main_call

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
;------------------------------------------------------------------------------
; Init INTB here if interrupt handlers should be placed in RAM.
;------------------------------------------------------------------------------
        ;mvtc      #(SFB .inttable), INTB
        bsr.a     _main
        bra.a     _exit
__iar_cstart_end:

        end
