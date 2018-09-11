/* Version: mss_v6_3 */
/*
 * math_h8s.h
 *
 * Assembly optimization for Renesas H8S Processor
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

typedef struct
{
    ubyte4* pResult;
    ubyte4* pFactorA;
    ubyte4* pFactorB;
    ubyte4  i_limit;
    ubyte4  j_limit;
    ubyte4  x_limit;

} mul_parm_T;

MOC_EXTERN void MATH_H8S_MULT(mul_parm_T *pStruct);

#define MACRO_MULTIPLICATION_LOOP(a, b, c, d, e, f) \
{\
    mul_parm_T  P; \
    P.pResult = a; \
    P.pFactorA = b; \
    P.pFactorB = c; \
    P.i_limit = d; \
    P.j_limit = e; \
    P.x_limit = f; \
    MATH_H8S_MULT(&P); \
}

MOC_EXTERN void MATH_H8S_SHR( ubyte4* pUnits, ubyte4 numUnits);

#define MACRO_SHIFT_RIGHT(u,n) MATH_H8S_SHR(u,n)

