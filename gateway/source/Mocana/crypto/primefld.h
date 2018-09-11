/* Version: mss_v6_3 */
/*
 * primefld.h
 *
 * Prime Field Header
 *
 * Copyright Mocana Corp 2006-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file primefld.h Prime Field (EC) developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for EC prime field management.

\since 3.06
\version 5.05 and later

To enable any of this file's functions, the following flags must be defined in
moptions.h:
- $__ENABLE_MOCANA_SSH_FTP_SERVER__$

! External Functions
This file contains the following public ($extern$) function declarations:
- PRIMEFIELD_newElement
- PRIMEFIELD_copyElement
- PRIMEFIELD_deleteElement

*/


/*------------------------------------------------------------------*/

#ifndef __PRIMEFLD_HEADER__
#define __PRIMEFLD_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(__ENABLE_MOCANA_ECC__))
/* Support for Finite Field Operations */

typedef const struct PrimeField* PrimeFieldPtr;


typedef const struct PFE    *ConstPFEPtr;
typedef struct PFE          *PFEPtr;

/* NIST curves */
#ifdef __ENABLE_MOCANA_ECC_P192__
MOC_EXTERN const PrimeFieldPtr PF_p192;
#endif

#ifndef __DISABLE_MOCANA_ECC_P224__
MOC_EXTERN const PrimeFieldPtr PF_p224;
#endif

#ifndef __DISABLE_MOCANA_ECC_P256__
MOC_EXTERN const PrimeFieldPtr PF_p256;
#endif

#ifndef __DISABLE_MOCANA_ECC_P384__
MOC_EXTERN const PrimeFieldPtr PF_p384;
#endif

#ifndef __DISABLE_MOCANA_ECC_P521__
MOC_EXTERN const PrimeFieldPtr PF_p521;
#endif

MOC_EXTERN MSTATUS PRIMEFIELD_newElement( PrimeFieldPtr pField, PFEPtr* ppNewElem);

MOC_EXTERN MSTATUS PRIMEFIELD_copyElement( PrimeFieldPtr pField, PFEPtr pDestElem, ConstPFEPtr pSrcElem);

MOC_EXTERN MSTATUS PRIMEFIELD_deleteElement( PrimeFieldPtr pField, PFEPtr* ppDeleteElem);

MOC_EXTERN MSTATUS PRIMEFIELD_add( PrimeFieldPtr pField, PFEPtr pSumAndValue, ConstPFEPtr pAddend);
MOC_EXTERN MSTATUS PRIMEFIELD_subtract( PrimeFieldPtr pField, PFEPtr pResultAndValue, ConstPFEPtr pSubtract);
MOC_EXTERN sbyte4 PRIMEFIELD_xor(PrimeFieldPtr pField, PFEPtr pA, ConstPFEPtr pB);
MOC_EXTERN MSTATUS PRIMEFIELD_multiply( PrimeFieldPtr pField, PFEPtr pProduct, ConstPFEPtr pA, ConstPFEPtr pB);
MOC_EXTERN MSTATUS PRIMEFIELD_shiftR( PrimeFieldPtr pField, PFEPtr pA);
MOC_EXTERN MSTATUS PRIMEFIELD_getBit( PrimeFieldPtr pField, ConstPFEPtr pA, ubyte4 bitNum, ubyte* bit);
MOC_EXTERN MSTATUS PRIMEFIELD_inverse( PrimeFieldPtr pField, PFEPtr pInverse, ConstPFEPtr pA);
MOC_EXTERN MSTATUS PRIMEFIELD_divide( PrimeFieldPtr pField, PFEPtr pResult, ConstPFEPtr pA, ConstPFEPtr pDivisor);
MOC_EXTERN sbyte4 PRIMEFIELD_cmpToUnsigned(PrimeFieldPtr pField, ConstPFEPtr pA, ubyte4 val);
MOC_EXTERN MSTATUS PRIMEFIELD_setToUnsigned(PrimeFieldPtr pField, PFEPtr pA, ubyte4 val);
MOC_EXTERN MSTATUS PRIMEFIELD_setToByteString( PrimeFieldPtr pField, PFEPtr pA, const ubyte* b, sbyte4 len);
MOC_EXTERN MSTATUS PRIMEFIELD_getAsByteString( PrimeFieldPtr pField, ConstPFEPtr pA, ubyte** b, sbyte4* len);
MOC_EXTERN MSTATUS PRIMEFIELD_getAsByteString2( PrimeFieldPtr pField, ConstPFEPtr pA, ConstPFEPtr pB, ubyte** b, sbyte4* len);
MOC_EXTERN MSTATUS PRIMEFIELD_writeByteString( PrimeFieldPtr pField, ConstPFEPtr pA, ubyte* b, sbyte4 len);
MOC_EXTERN MSTATUS PRIMEFIELD_getElementByteStringLen(PrimeFieldPtr pField, sbyte4* len);
MOC_EXTERN sbyte4 PRIMEFIELD_cmp(PrimeFieldPtr pField, ConstPFEPtr pA, ConstPFEPtr pB);

MOC_EXTERN MSTATUS PRIMEFIELD_barrettMultiply( PrimeFieldPtr pField, PFEPtr pProduct, ConstPFEPtr pA,
                                          ConstPFEPtr pB, ConstPFEPtr pModulo, ConstPFEPtr pMu);
MOC_EXTERN MSTATUS PRIMEFIELD_addAux( PrimeFieldPtr pField, PFEPtr pSumAndValue, ConstPFEPtr pAddend,
                                    ConstPFEPtr pModulus); /* specify modular reduction */
MOC_EXTERN MSTATUS PRIMEFIELD_inverseAux( sbyte4 k, PFEPtr pInverse, ConstPFEPtr pA, ConstPFEPtr pModulus);

#if (defined(__ENABLE_MOCANA_VLONG_ECC_CONVERSION__))
/* useful conversion functions */
MOC_EXTERN MSTATUS PRIMEFIELD_newElementFromVlong( PrimeFieldPtr pField, const vlong* pV,
                                              PFEPtr* ppNewElem);
MOC_EXTERN MSTATUS PRIMEFIELD_newVlongFromElement( PrimeFieldPtr pField, ConstPFEPtr pElem,
                                              vlong** ppNewElem, vlong** ppQueue);
MOC_EXTERN MSTATUS PRIMEFIELD_getPrime( PrimeFieldPtr pField, vlong** ppPrime);

MOC_EXTERN MSTATUS PRIMEFIELD_newMpintFromElement(PrimeFieldPtr pField, ConstPFEPtr pElem, ubyte** ppNewMpint, sbyte4 *pRetMpintLength, vlong** ppVlongQueue);
MOC_EXTERN MSTATUS PRIMEFIELD_newElementFromMpint(const ubyte* pBuffer, ubyte4 bufSize, ubyte4 *pBufIndex, PrimeFieldPtr pField, PFEPtr* ppNewElem);
#endif

#endif /* __ENABLE_MOCANA_ECC__  */

#ifdef __cplusplus
}
#endif

#endif /* __PRIMEFLD_HEADER__ */

