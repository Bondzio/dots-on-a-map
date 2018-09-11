/* Version: mss_v6_3 */
/*
 * math_gcc_386.h
 *
 * Inline assembly macros for 80386+ Processor using GCC
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#if defined(__ENABLE_MOCANA_64_BIT__) && defined(__LP64__)

/*---------------------------------------------------------------------------*/

#if 0
/* this version leads to slower execution and is also triggering
   a gcc bug when -funswitch-loops:
    in VLONG_unsignedDivide: some registers are overwritten.
    This can be fixed by making the following variables
    volatile:
        vlong_unit borrow;
#if __GNUC__ == 3 && defined(__OPTIMIZE__)
        volatile
#endif
        vlong_unit r0,r1,r2,r3;
        ubyte4 index0;

    But then the code is slower... It's difficult to extract
    the relevant part to submit as a GCC bug since it depends on
    a lot of functions.
*/
#define MULT_ADDC(base_a,base_b,index0,index1,result0,result1,result2)      \
{                                                                           \
    __asm __volatile (  "mulq   %4                          \n\t"            \
                        "addq   %3, %0                   \n\t"            \
                        "adcq   %%rdx, %1                   \n\t"            \
                        "adcq   $0x00, %2                   \n\t"            \
                        : "=r"(result0),"=r"(result1),"=r"(result2)          \
                        : "a"(base_a[index0]),"r"(base_b[index1]),           \
                        "0"(result0),"1"(result1),"2"(result2)               \
                        : "rdx", "cc" );                              \
}
#endif

#define MULT_ADDC(base_a,base_b,index0,index1,result0,result1,result2)      \
{                                                                           \
    __asm __volatile (  "movq   %3, %%rax                   \n\t"            \
                        "mulq   %4                          \n\t"            \
                        "addq   %%rax, %0                   \n\t"            \
                        "adcq   %%rdx, %1                   \n\t"            \
                        "adcq   $0x00, %2                   \n\t"            \
                        : "=r"(result0),"=r"(result1),"=r"(result2)          \
                        : "r"(base_a[index0]),"r"(base_b[index1]),           \
                        "0"(result0),"1"(result1),"2"(result2)               \
                        : "rax", "rdx", "cc" );                              \
}


#define MULT_ADDC1(base_a,base_b,index0,index1,result0,result1)             \
{                                                                           \
    __asm __volatile (  "mul   %3                          \n\t"            \
                        "add   %2, %0                      \n\t"            \
                        "adc   %%rdx, %1                   \n\t"            \
                        : "=r"(result0),"=r"(result1)                       \
                        : "a"(base_a[index0]),"r"(base_b[index1]),          \
                        "0"(result0),"1"(result1)                           \
                        : "rdx", "cc");                                     \
}

/*------------------------------------------------------------------*/


#define ADD_DOUBLE(result0, result1, result2, half0, half1, half2) \
{                                                                  \
__asm __volatile( "add %6, %6                          \n\t"       \
                  "adc %7, %7                          \n\t"       \
                  "adc %8, %8                          \n\t"       \
                  "add %6, %0                          \n\t"       \
                  "adc %7, %1                          \n\t"       \
                  "adc %8, %2                          \n\t"       \
       : "=r"(result0), "=r"(result1), "=r"(result2)               \
       : "0"(result0), "1"(result1), "2"(result2), "r"(half0),     \
       "r"(half1), "r"(half2)                                      \
       : "cc");                                                    \
}


/*------------------------------------------------------------------*/

#define ASM_BIT_LENGTH( aWord, bitlen)                              \
{                                                                   \
__asm __volatile("bsr %1, %0                        \n\t"           \
                 "inc  %0                           \n\t"           \
                 : "=r"(bitlen)                                     \
                 : "r"(aWord)                                       \
                 : "cc"); }



/*------------------------------------------------------------------*/

#define MULT_ADDC32(base_a,base_b,index0,index1,result0,result1,result2) \
{                                                                        \
__asm __volatile (  "movl  %3, %%eax                   \n\t"           \
                    "mull  %4                          \n\t"           \
                    "addl  %%eax, %0                   \n\t"           \
                    "adcl  %%edx, %1                   \n\t"           \
                    "adcl  $0x00, %2                   \n\t"           \
                  : "=r"(result0),"=r"(result1),"=r"(result2)          \
                  : "r"(base_a[index0]),"r"(base_b[index1]),           \
                    "0"(result0),"1"(result1),"2"(result2)             \
                    : "eax", "edx", "cc");                             \
}



/* shift right assembly is no longer improving noticably the performance
   since the division is no longer implemented using it */

#else /*! __ENABLE_MOCANA_64_BIT__ || ! __LP64__ */


/* 64 bit compiler but no __ENABLE_MOCANA_64_BIT__ */
#if defined(__LP64__)


/*------------------------------------------------------------------*/

#define MULT_ADDC(base_a,base_b,index0,index1,result0,result1,result2) \
{                                                                      \
__asm __volatile (  "movl  %3, %%eax                   \n\t"           \
                    "mull  %4                          \n\t"           \
                    "addl  %%eax, %0                   \n\t"           \
                    "adcl  %%edx, %1                   \n\t"           \
                    "adcl  $0x00, %2                   \n\t"           \
                  : "=r"(result0),"=r"(result1),"=r"(result2)          \
                  : "r"(base_a[index0]),"r"(base_b[index1]),           \
                    "0"(result0),"1"(result1),"2"(result2)             \
                    : "eax", "edx", "cc");                             \
}



/*------------------------------------------------------------------*/

#define ADD_DOUBLE(result0, result1, result2, half0, half1, half2) \
{                                                                  \
__asm __volatile( "addl %6, %6                          \n\t"      \
                  "adcl %7, %7                          \n\t"      \
                  "adcl %8, %8                          \n\t"      \
                  "addl %6, %0                          \n\t"      \
                  "adcl %7, %1                          \n\t"      \
                  "adcl %8, %2                          \n\t"      \
       : "=r"(result0), "=r"(result1), "=r"(result2)               \
       : "0"(result0), "1"(result1), "2"(result2),                 \
         "r"(half0), "r"(half1), "r"(half2)                        \
       : "cc");                                                    \
}


/* this does not work properly  when optimization turned to -O3 with
   GCC 4.1. Works with GCC 4.2 */

/*------------------------------------------------------------------*/

#define ASM_BIT_LENGTH( aWord, bitlen)                              \
{                                                                   \
__asm __volatile("bsrl %1, %0                        \n\t"          \
                 "incl %0                            \n\t"          \
                 : "=r"(bitlen)                                     \
                 : "r"(aWord)                                       \
                 : "cc" ); }


/*------------------------------------------------------------------*/



#else /*! __LP64__ */

/* GCC inline assembly is unstable, not to say buggy. Attempts to 
report bugs to the GCC team were rebuffed. One should always
run some sanity tests to make sure the compiler works properly 
*/

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 3

/* this version works well with gcc 4.3 and over (tested up to 4.6).
It seems a bit slower and also leads to compile errors if optimization
level is 0 and the compiler is in the 3.x series */

/*
#define MULT_ADDC(base_a,base_b,index0,index1,result0,result1,result2) \
{                                                                      \
__asm __volatile (  "movl  %3, %%eax                   \n\t"           \
                    "mull  %4                          \n\t"           \
                    "addl  %%eax, %0                   \n\t"           \
                    "adcl  %%edx, %1                   \n\t"           \
                    "adcl  $0x00, %2                   \n\t"           \
                  : "=r"(result0),"=r"(result1),"=r"(result2)          \
                  : "r"(base_a[index0]),"r"(base_b[index1]),           \
                    "0"(result0),"1"(result1),"2"(result2)             \
                    : "eax", "edx", "cc");                             \
}
*/

/* above version causes compile error when -fPIC is used, possibly
 due to shortage of register. this version saves %eax onto the stack
 and restores it afterward, so there is extra to spare. */

#define MULT_ADDC(base_a,base_b,index0,index1,result0,result1,result2) \
{                                                                      \
__asm __volatile (  "pushl  %%eax                       \n\t"          \
                    "mull   %4                          \n\t"          \
                    "addl   %3, %0                      \n\t"          \
                    "adcl   %%edx, %1                   \n\t"          \
                    "adcl   $0x00, %2                   \n\t"          \
                    "popl   %%eax                       \n\t"          \
                  : "=r"(result0),"=r"(result1),"=r"(result2)          \
                  : "a"(base_a[index0]),"r"(base_b[index1]),           \
                    "0"(result0),"1"(result1),"2"(result2)             \
                  : "edx", "cc");                                      \
}

#else

/* this version is faster and works well if gcc < 4.3 */

/*------------------------------------------------------------------*/

#define MULT_ADDC(base_a,base_b,index0,index1,result0,result1,result2) \
{                                                                      \
__asm __volatile (  "mull   %4                          \n\t"          \
                    "addl   %3, %0                      \n\t"          \
                    "adcl   %%edx, %1                   \n\t"          \
                    "adcl   $0x00, %2                   \n\t"          \
                  : "=r"(result0),"=r"(result1),"=r"(result2)          \
                  : "a"(base_a[index0]),"r"(base_b[index1]),           \
                    "0"(result0),"1"(result1),"2"(result2)             \
                  : "edx", "cc");                                      \
}

#endif

/*------------------------------------------------------------------*/

#define MULT_ADDC1(base_a,base_b,index0,index1,result0,result1)        \
{                                                                      \
__asm __volatile (  "mull   %3                          \n\t"          \
                    "addl   %2, %0                      \n\t"          \
                    "adcl   %%edx, %1                   \n\t"          \
                  : "=r"(result0),"=r"(result1)                        \
                  : "a"(base_a[index0]),"r"(base_b[index1]),           \
                    "0"(result0),"1"(result1)                          \
                    : "edx", "cc");                                    \
}


/*------------------------------------------------------------------*/

#define ADD_DOUBLE(result0, result1, result2, half0, half1, half2) \
{                                                                  \
__asm __volatile( "addl %6, %6                         \n\t"       \
                  "adcl %7, %7                         \n\t"       \
                  "adcl %8, %8                         \n\t"       \
                  "addl %6, %0                         \n\t"       \
                  "adcl %7, %1                         \n\t"       \
                  "adcl %8, %2                         \n\t"       \
       : "=r"(result0), "=r"(result1), "=r"(result2)               \
       : "0"(result0), "1"(result1), "2"(result2),                 \
         "r"(half0), "r"(half1), "r"(half2)                        \
       : "cc" );                                                   \
}




/*------------------------------------------------------------------*/

#define ASM_BIT_LENGTH( aWord, bitlen)                              \
{                                                                   \
__asm __volatile("bsrl %1, %0                        \n\t"          \
                 "incl  %0                           \n\t"          \
                 : "=r"(bitlen)                                     \
                 : "r"(aWord)                                       \
                 : "cc"); }

/* shift right assembly is no longer improving noticably the performance
   since the division is no longer implemented using it */

#ifdef __SSE2__

MOC_EXTERN void SSE2_multiply( ubyte4* pResult, ubyte4* pFactorA, ubyte4* pFactorB,
                           ubyte4 i_limit, ubyte4 j_limit, ubyte4 x_limit);


#define MACRO_MULTIPLICATION_LOOP(a,b,c,d,e,f) SSE2_multiply( a,b,c,d,e,f);


MOC_EXTERN void SSE2_square( ubyte4* pResult, ubyte4* pFactorA, ubyte4 i_limit,
                         ubyte4 x_limit);

#define MACRO_SQR_LOOP(a,b,c,d) SSE2_square(a,b,c,d)

#endif



#endif /* !__LP64__ */

#endif /* !__ENABLE_MOCANA_64_BIT__ || !__LP64__ */
