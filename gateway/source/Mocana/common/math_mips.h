/* Version: mss_v6_3 */
/*
 * math_mips.h
 *
 * Assembly optimization for MIPS Processor
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_64_BIT__)

#if defined(__mips64) && defined(__GNUC__)

#define MULT_ADDC(a,b,indexA,indexB,r0,r1,r2)                              \
   __asm __volatile( "dmultu %6, %7          \n\t"                         \
                     "mflo   $8              \n\t"                         \
                     "mfhi   $9              \n\t"                         \
                     "daddu  %0,  %0,  $8    \n\t"                         \
                     "sltu  $10,  %0,  $8    \n\t"                         \
                     "daddu  %1,  %1,  $10   \n\t"                         \
                     "sltu  $10,  %1,  $10   \n\t"                         \
                     "daddu  %2,  %2,  $10   \n\t"                         \
                     "daddu  %1,  %1,  $9    \n\t"                         \
                     "sltu  $10,  %1,  $9    \n\t"                         \
                     "daddu  %2,  %2,  $10   \n\t"                         \
       : "=r"(r0), "=r"(r1), "=r"(r2)                                      \
       : "0"(r0), "1"(r1), "2"(r2), "r"(a[indexA]), "r"(b[indexB])         \
       : "$8", "$9", "$10" );

#define MULT_ADDC1(a,b,indexA,indexB,r0,r1)                                \
   __asm __volatile( "dmultu %4, %5          \n\t"                         \
                     "mflo   $8              \n\t"                         \
                     "mfhi   $9              \n\t"                         \
                     "daddu  %0,  %0,  $8    \n\t"                         \
                     "sltu  $10,  %0,  $8    \n\t"                         \
                     "daddu  %1,  %1,  $10   \n\t"                         \
                     "daddu  %1,  %1,  $9    \n\t"                         \
       : "=r"(r0), "=r"(r1)                                                \
       : "0"(r0), "1"(r1), "r"(a[indexA]), "r"(b[indexB])                  \
       : "$8", "$9", "$10" );

#endif

#define MULT_ADDC32(a,b,indexA,indexB, r0, r1, r2)                  \
    __asm __volatile( "multu %6, %7          \n\t"                  \
                      "mflo  $8              \n\t"                  \
                      "mfhi  $9              \n\t"                  \
                      "addu  %0,  %0,  $8    \n\t"                  \
                      "sltu  $10,  %0,  $8   \n\t"                  \
                      "addu  %1,  %1,  $10   \n\t"                  \
                      "sltu  $10,  %1,  $10  \n\t"                  \
                      "addu  %2,  %2,  $10   \n\t"                  \
                      "addu  %1,  %1,  $9    \n\t"                  \
                      "sltu  $10,  %1,  $9   \n\t"                  \
                      "addu  %2,  %2,  $10   \n\t"                  \
       : "=r"(r0), "=r"(r1), "=r"(r2)                               \
       : "0"(r0), "1"(r1), "2"(r2), "r"(a[indexA]), "r"(b[indexB])  \
       : "$8", "$9", "$10" );


#else  /* __ENABLE_MOCANA_64_BIT__ not defined */

#if defined( __GNUC__)

#define MULT_ADDC(a,b,indexA,indexB, r0, r1, r2)                    \
    __asm __volatile( "multu %6, %7          \n\t"                  \
                      "mflo  $8              \n\t"                  \
                      "mfhi  $9              \n\t"                  \
                      "addu  %0,  %0,  $8    \n\t"                  \
                      "sltu  $10,  %0,  $8   \n\t"                  \
                      "addu  %1,  %1,  $10   \n\t"                  \
                      "sltu  $10,  %1,  $10  \n\t"                  \
                      "addu  %2,  %2,  $10   \n\t"                  \
                      "addu  %1,  %1,  $9    \n\t"                  \
                      "sltu  $10,  %1,  $9   \n\t"                  \
                      "addu  %2,  %2,  $10   \n\t"                  \
       : "=r"(r0), "=r"(r1), "=r"(r2)                               \
       : "0"(r0), "1"(r1), "2"(r2), "r"(a[indexA]), "r"(b[indexB])  \
       : "$8", "$9", "$10" );

#define MULT_ADDC1(a,b,indexA,indexB,r0,r1)                                \
   __asm __volatile( "multu %4, %5           \n\t"                         \
                     "mflo   $8              \n\t"                         \
                     "mfhi   $9              \n\t"                         \
                     "addu  %0,  %0,  $8     \n\t"                         \
                     "sltu  $10,  %0, $8     \n\t"                         \
                     "addu  %1,  %1,  $10    \n\t"                         \
                     "addu  %1,  %1,  $9     \n\t"                         \
       : "=r"(r0), "=r"(r1)                                                \
       : "0"(r0), "1"(r1), "r"(a[indexA]), "r"(b[indexB])                  \
       : "$8", "$9", "$10" );

#endif  /* __GNUC__ */

MOC_EXTERN void MATH_MIPS_SHR( ubyte4* pUnits, ubyte4 numUnits);

#define MACRO_SHIFT_RIGHT(u,n) MATH_MIPS_SHR(u,n)

MOC_EXTERN ubyte4 MATH_MIPS_SHL( ubyte4 * pUnits, ubyte4 numUnits);

#define MACRO_SHIFT_LEFT(u,n) MATH_MIPS_SHL(u, n)

#endif
