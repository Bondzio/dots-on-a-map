/* Version: mss_v6_3 */
/*
 * math_arm.h
 *
 * Inline assembly macros for ARM Processor
 *
 *
 */

#if defined( __GNUC__ ) &&!defined(__thumb__) && !defined(__MOCANA_DISABLE_INLINE_ASSEMBLY__)



/*------------------------------------------------------------------*/

#define MULT_ADDC(a,b,indexA,indexB,res0,res1,res2)                         \
__asm__ __volatile__(                                                       \
        "umull  r6,r7,%6,%7  \n"                                            \
        "adds   %0,%0,r6     \n"                                            \
        "adcs   %1,%1,r7     \n"                                            \
        "adcs   %2,%2,#0     \n"                                            \
        : "=r"(res0), "=r"(res1), "=r"(res2)                                \
        : "0"(res0), "1"(res1), "2"(res2), "r"(a[indexA]), "r"(b[indexB])   \
        : "r6", "r7" );


/*------------------------------------------------------------------*/

#define MULT_ADDC1(a,b,indexA,indexB,res0,res1)                   \
__asm__ __volatile__(                                             \
        "umlal  %0,%1,%4,%5  \n"                                  \
        : "=r"(res0), "=r"(res1)                                  \
        : "0"(res0), "1"(res1), "r"(a[indexA]), "r"(b[indexB]));


/*------------------------------------------------------------------*/


#define ADD_DOUBLE(result0, result1, result2, half0, half1, half2) \
__asm __volatile( "adds %6, %6, %6                     \n\t"       \
                  "adcs %7, %7, %7                     \n\t"       \
                  "adcs %8, %8, %8                     \n\t"       \
                  "adds %0, %6, %0                     \n\t"       \
                  "adcs %1, %7, %1                     \n\t"       \
                  "adcs %2, %8, %2                     \n\t"       \
       : "=r"(result0), "=r"(result1), "=r"(result2)               \
       : "0"(result0), "1"(result1), "2"(result2), "r"(half0),     \
       "r"(half1), "r"(half2)                                      \
       );                                                          \

#endif


/* always used */
#define MONT_MULT_MULTIPLY(a,b,n,r,N,t)  \
    ARM_MONT_MULT_MULTIPLY(a,b,n,r,N)

#define MONT_MULT_SQR(a,n,r,N,t)  \
    ARM_MONT_MULT_MULTIPLY(a,a,n,r,N)

extern int ARM_MONT_MULT_MULTIPLY( ubyte4* a, ubyte4* b, const ubyte4* n, 
                                   ubyte4 rho, ubyte4 N);

#define MONT_MULT_REDUCTION ARM_MONT_MULT_REDUCTION

extern ubyte4 MONT_MULT_REDUCTION( ubyte4* t, const ubyte4* n, ubyte4 rho, ubyte4 N, ubyte4 *a);


#if defined(__ARM_NEON__)

MOC_EXTERN void NEON_multiply( ubyte4* pResult, ubyte4* pFactorA, ubyte4* pFactorB,
                           ubyte4 i_limit, ubyte4 j_limit, ubyte4 x_limit);


#define MACRO_MULTIPLICATION_LOOP(a,b,c,d,e,f) NEON_multiply(a,b,c,d,e,f)

MOC_EXTERN void NEON_square( ubyte4* pResult, const ubyte4* pFactorA,
                         ubyte4 numUnitsA, ubyte4 numUnitsR);


#define MACRO_SQR_LOOP(a,b,c,d) NEON_square(a,b,c,d)

#elif defined(__ARM_V6__)

typedef struct mul_parm_T
{
    ubyte4 *pResult;  /* pointer to the result array */
    ubyte4 *pFactorA; /* pointer to first multiplication factor array */
    ubyte4 *pFactorB; /* pointer to second multipication factor array */
    ubyte4 i_limit;   /* zero based index to the last entry in pFactorA array */
    ubyte4 j_limit;   /* zero based index to the last entry in pFactorB array */
    ubyte4 x_limit;   /* size of the pResult array in words */
} mul_parm_T;


MOC_EXTERN void MATH_ARMV6_MULT(mul_parm_T *pstruct);

#define MACRO_MULTIPLICATION_LOOP(r,a,b,i,j,x)                     \
    {                                                              \
        mul_parm_T P;                                              \
        if ( 0 ==((i) & 1)) a[(i)+1] = 0;                          \
        if ( 0 ==((j) & 1)) b[(j)+1] = 0;                          \
        P.pResult = r; P.pFactorA = a; P.pFactorB = b;             \
        P.i_limit = ((i)>>1); P.j_limit = ((j)>>1);                \
        P.x_limit = (((x)+1) >> 1); MATH_ARMV6_MULT(&P); } 


typedef struct mul_sqr_T
{
    ubyte4 *pResult;  /* pointer to the result array */
    ubyte4 *pFactorA; /* pointer to first multiplication factor array */
    ubyte4 i_limit;   /* zero based index to the last entry in pFactorA array */
    ubyte4 x_limit;   /* size of the pResult array in words */
} mul_sqr_T;

MOC_EXTERN void MATH_ARMV6_SQR(mul_sqr_T *pstruct);

#define MACRO_SQR_LOOP(r,a,i,x)                                    \
    {                                                              \
        mul_sqr_T P;                                               \
        if ( 0 ==((i) & 1)) a[(i)+1] = 0;                          \
        P.pResult = r; P.pFactorA = a;                             \
        P.i_limit = ((i)>>1);                                      \
        P.x_limit = (((x)+1) >> 1); MATH_ARMV6_SQR(&P);    }


#define ASM_MATH_8x8_DEFINED

MOC_EXTERN void MATH_ARMV6_8x8(ubyte4 *pProduct, const ubyte4 *pFactorA,
                           const ubyte4 *pFactorB);

#define MATH_8x8(p,a,b) MATH_ARMV6_8x8( p, a, b)
#define MATH_SQR8(p,a) MATH_ARMV6_8x8( p, a, a)

#endif 

#ifdef WINCE

#define ASM_MATH_8x8_DEFINED

MOC_EXTERN void MATH_ARM_MULT_8x8(ubyte4 *pProduct, const ubyte4 *pFactorA,
                           const ubyte4 *pFactorB);

MOC_EXTERN void MATH_ARM_SQR_8x8(ubyte4 *pProduct, const ubyte4 *pFactorA);

#define MATH_8x8(p,a,b) MATH_ARM_MULT_8x8( p, a, b)
#define MATH_SQR8(p,a) MATH_ARM_SQR_8x8( p, a)

#endif



