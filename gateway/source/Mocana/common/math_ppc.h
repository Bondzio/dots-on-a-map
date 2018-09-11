/* Version: mss_v6_3 */
/*
 * math_ppc.h
 *
 * Assembly optimization for PowerPC Processor
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/


#if defined( __LP64__) && defined(__ENABLE_MOCANA_64_BIT__)

#ifdef __ALTIVEC__
/* Altivec with a 64 bit CPU is slower: use it only after testing */
MOC_EXTERN MSTATUS ALTIVEC_multiply( ubyte8* pResult, const ubyte8* pFactorA,
                                 const ubyte8* pFactorB, ubyte4  i_limit,
                                 ubyte4 j_limit, ubyte4 x_limit);

#define MACRO_MULTIPLICATION_LOOP(a,b,c,d,e,f) \
ALTIVEC_multiply( a,b,c,d,e,f);

MOC_EXTERN MSTATUS ALTIVEC_square( ubyte8* pResult, const ubyte8* pFactorA,
                               ubyte4 numUnitsA, ubyte4 numUnitsR);


#define MACRO_SQR_LOOP(a,b,c,d) ALTIVEC_square(a,b,c,d)


#endif /* __ALTIVEC__ */

#if defined( __GNUC__ )

#if defined(__APPLE__)
#define MULT_ADDC( a, b, indexA, indexB, r0, r1, r2)               \
    __asm __volatile( "mulld   r16, %6, %7   \n\t"                 \
                      "addc    %0, %0, r16   \n\t"                 \
                      "mulhdu  r16, %6, %7   \n\t"                 \
                      "adde    %1, %1, r16   \n\t"                 \
                      "addze   %2, %2        \n\t"                 \
                      : "=r"(r0), "=r"(r1), "=r"(r2)               \
                      : "0"(r0), "1"(r1), "2"(r2),                 \
                        "r"(a[indexA]), "r"(b[indexB])             \
                      : "r16");

#define MULT_ADDC1( a, b, indexA, indexB, r0, r1)                  \
    __asm __volatile( "mulld   r16, %4, %5   \n\t"                 \
                      "addc    %0, %0, r16   \n\t"                 \
                      "mulhdu  r16, %4, %5   \n\t"                 \
                      "adde    %1, %1, r16   \n\t"                 \
                      : "=r"(r0), "=r"(r1)                         \
                      : "0"(r0), "1"(r1),                          \
                        "r"(a[indexA]), "r"(b[indexB])             \
                      : "r16");

#else /* !__APPLE__ */
#define MULT_ADDC( a, b, indexA, indexB, r0, r1, r2)               \
    __asm __volatile( "mulld   16, %6, %7    \n\t"                 \
                      "addc    %0, %0, 16    \n\t"                 \
                      "mulhdu  16, %6, %7    \n\t"                 \
                      "adde    %1, %1, 16    \n\t"                 \
                      "addze   %2, %2        \n\t"                 \
                      : "=r"(r0), "=r"(r1), "=r"(r2)               \
                      : "0"(r0), "1"(r1), "2"(r2),                 \
                        "r"(a[indexA]), "r"(b[indexB])             \
                      : "16");

#define MULT_ADDC1( a, b, indexA, indexB, r0, r1)                  \
    __asm __volatile( "mulld   16, %4, %5    \n\t"                 \
                      "addc    %0, %0, 16    \n\t"                 \
                      "mulhdu  16, %4, %5    \n\t"                 \
                      "adde    %1, %1, 16    \n\t"                 \
                      : "=r"(r0), "=r"(r1)                         \
                      : "0"(r0), "1"(r1),                          \
                        "r"(a[indexA]), "r"(b[indexB])             \
                      : "16");

#endif

#define ADD_DOUBLE( r0, r1, r2, h0, h1, h2)                     \
    __asm __volatile( "addc    %6, %6, %6  \n\t"                \
                      "adde    %7, %7, %7  \n\t"                \
                      "adde    %8, %8, %8  \n\t"                \
                      "addc    %0, %0, %6  \n\t"                \
                      "adde    %1, %1, %7  \n\t"                \
                      "adde    %2, %2, %8  \n\t"                \
                      : "=r"(r0), "=r"(r1), "=r"(r2)            \
                      : "0"(r0), "1"(r1), "2"(r2),              \
                        "r"(h0), "r"(h1), "r"(h2)               \
                      );


#define ASM_BIT_LENGTH( aWord, bitLen)         \
    __asm __volatile("cntlzd %0, %1     \n\t"  \
                     "subfic %0, %0, 64 \n\t"  \
                     : "=r"(bitLen)            \
                     : "r"(aWord)              \
        );

/* Note: this is used only for the ECC code which has not yet
been ported to a 64 bit environment. This is slower than the
32 bit code which uses the whole loop coded in assembly. But this
is still faster than the C implementation. Doing the shifts in
assembly *decreases* the performance! */
#ifdef __APPLE__
#define MULT_ADDC32( a, b, indexA, indexB, r0, r1, r2)                          \
    {  ubyte8 _r0r1 = (((ubyte8) r1) << 32) | ((ubyte8) r0);                    \
        __asm __volatile( "mulld   r16, %4, %5    \n\t"                          \
                          "addc    %0, %0, r16    \n\t"                          \
                          "addze   %1, %1        \n\t"                          \
                          : "=r"(_r0r1),"=r"(r2)                                \
                          : "0"(_r0r1), "1"(r2), "r"(a[indexA]), "r"(b[indexB]) \
                          : "r16");                                              \
        r1 = (ubyte4) (_r0r1>>32); r0 = (ubyte4) _r0r1; }

#else
#define MULT_ADDC32( a, b, indexA, indexB, r0, r1, r2)                          \
    {  ubyte8 _r0r1 = (((ubyte8) r1) << 32) | ((ubyte8) r0);                    \
        __asm __volatile( "mulld   16, %4, %5    \n\t"                          \
                          "addc    %0, %0, 16    \n\t"                          \
                          "addze   %1, %1        \n\t"                          \
                          : "=r"(_r0r1),"=r"(r2)                                \
                          : "0"(_r0r1), "1"(r2), "r"(a[indexA]), "r"(b[indexB]) \
                          : "16");                                              \
        r1 = (ubyte4) (_r0r1>>32); r0 = (ubyte4) _r0r1; }
#endif

#endif /* defined( __GNUC__ ) */


#elif !defined(__LP64__) && !defined(__ENABLE_MOCANA_64_BIT__)

#ifdef __ALTIVEC__

MOC_EXTERN MSTATUS ALTIVEC_multiply( ubyte4* pResult, const ubyte4* pFactorA,
                                 const ubyte4* pFactorB, ubyte4  i_limit,
                                 ubyte4 j_limit, ubyte4 x_limit);

#define MACRO_MULTIPLICATION_LOOP(a,b,c,d,e,f) \
ALTIVEC_multiply( a,b,c,d,e,f);

MOC_EXTERN MSTATUS ALTIVEC_square( ubyte4* pResult, const ubyte4* pFactorA,
                               ubyte4 numUnitsA, ubyte4 numUnitsR);


#define MACRO_SQR_LOOP(a,b,c,d) ALTIVEC_square(a,b,c,d)


#else

MOC_EXTERN void MATH_PPC_MULT( ubyte4* pResult, const ubyte4* pFactorA,
                           const ubyte4* pFactorB, ubyte4  i_limit,
                           ubyte4 j_limit, ubyte4 x_limit);

#define MACRO_MULTIPLICATION_LOOP(a, b, c, d, e, f) \
    MATH_PPC_MULT(a,b,c,d,e,f)

MOC_EXTERN void MATH_PPC_SQR(ubyte4* pResult, const ubyte4* pFactor,
                          ubyte4 i_limit, ubyte4 x_limit);

#define MACRO_SQR_LOOP(a,b,c,d) MATH_PPC_SQR(a,b,c,d)

#endif  /* ALTIVEC */


MOC_EXTERN void MATH_PPC_SHR( ubyte4* pUnits, ubyte4 numUnits, ubyte4 shift);

#define MACRO_SHIFT_RIGHT(u,n) MATH_PPC_SHR(u,n,1)

#define MACRO_SHIFT_RIGHT_N(u,n,s) MATH_PPC_SHR(u,n,s)


MOC_EXTERN ubyte4 MATH_PPC_SHL( ubyte4* pUnits, ubyte4 numUnits);

#define MACRO_SHIFT_LEFT(u,n) MATH_PPC_SHL(u,n)


#if defined( __GNUC__ )

#if defined(__APPLE__)

#define MULT_ADDC( a, b, indexA, indexB, r0, r1, r2)               \
    __asm __volatile( "mullw   r16, %6, %7   \n\t"                 \
                      "addc    %0, %0, r16   \n\t"                 \
                      "mulhwu  r16, %6, %7   \n\t"                 \
                      "adde    %1, %1, r16   \n\t"                 \
                      "addze   %2, %2        \n\t"                 \
    : "=r"(r0), "=r"(r1), "=r"(r2)                                 \
    : "0"(r0), "1"(r1), "2"(r2), "r"(a[indexA]), "r"(b[indexB])    \
    : "r16");


#define MULT_ADDC1( a, b, indexA, indexB, r0, r1)                  \
    __asm __volatile( "mullw   r16, %4, %5   \n\t"                 \
                      "addc    %0, %0, r16   \n\t"                 \
                      "mulhwu  r16, %4, %5   \n\t"                 \
                      "adde    %1, %1, r16   \n\t"                 \
    : "=r"(r0), "=r"(r1)                                           \
    : "0"(r0), "1"(r1), "r"(a[indexA]), "r"(b[indexB])             \
    : "r16");

#else

#define MULT_ADDC( a, b, indexA, indexB, r0, r1, r2)               \
    __asm __volatile( "mullw   16, %6, %7    \n\t"                 \
                      "addc    %0, %0, 16    \n\t"                 \
                      "mulhwu  16, %6, %7    \n\t"                 \
                      "adde    %1, %1, 16    \n\t"                 \
                      "addze   %2, %2        \n\t"                 \
    : "=r"(r0), "=r"(r1), "=r"(r2)                                 \
    : "0"(r0), "1"(r1), "2"(r2), "r"(a[indexA]), "r"(b[indexB])    \
    : "16");


#define MULT_ADDC1( a, b, indexA, indexB, r0, r1)                  \
    __asm __volatile( "mullw   16, %4, %5    \n\t"                 \
                      "addc    %0, %0, 16    \n\t"                 \
                      "mulhwu  16, %4, %5    \n\t"                 \
                      "adde    %1, %1, 16    \n\t"                 \
    : "=r"(r0), "=r"(r1)                                           \
    : "0"(r0), "1"(r1), "r"(a[indexA]), "r"(b[indexB])             \
    : "16");

#endif

#define ADD_DOUBLE( r0, r1, r2, h0, h1, h2)                     \
    __asm __volatile( "addc    %6, %6, %6  \n\t"                \
                      "adde    %7, %7, %7  \n\t"                \
                      "adde    %8, %8, %8  \n\t"                \
                      "addc    %0, %0, %6  \n\t"                \
                      "adde    %1, %1, %7  \n\t"                \
                      "adde    %2, %2, %8  \n\t"                \
       : "=r"(r0), "=r"(r1), "=r"(r2)                           \
       : "0"(r0), "1"(r1), "2"(r2), "r"(h0), "r"(h1), "r"(h2)   \
       );


#define ASM_BIT_LENGTH( aWord, bitLen)         \
    __asm __volatile("cntlzw %0, %1     \n\t"  \
                     "subfic %0, %0, 32 \n\t"  \
                    : "=r"(bitLen)             \
                    : "r"(aWord)               \
                    );

#endif /* defined( __GNUC__ ) */


MOC_EXTERN ubyte4 MATH_PPC_ADD( ubyte4* pSumAndValue, const ubyte4* pValue,
                            ubyte4 numUnits);

#define ASM_ADD( a, b, k) MATH_PPC_ADD(a,b,k)

MOC_EXTERN ubyte4 MATH_PPC_ADD3(ubyte4* pResult, const ubyte4* pA,
                            const ubyte4* pB, ubyte4 numUnits);

#define ASM_ADD3( r, a, b, k) MATH_PPC_ADD3(r,a,b,k)

MOC_EXTERN void MATH_PPC_SUB( ubyte4 *pSumAndValue, const ubyte4* pValue,
                          ubyte4 numUnits);

#define ASM_SUBTRACT( a, b, k) MATH_PPC_SUB(a,b,k)

MOC_EXTERN ubyte4 MATH_PPC_SUB3(ubyte4* pResult, const ubyte4* pA,
                            const ubyte4* pB, ubyte4 numUnits);

#define ASM_SUB3(r,a,b,k) MATH_PPC_SUB3(r,a,b,k)


#endif

