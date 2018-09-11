/* Version: mss_v6_3 */
/*
 * prime.h
 *
 * Prime Factory Header
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __PRIME_HEADER__
#define __PRIME_HEADER__

typedef enum
{
    prime_DSA,
    prime_RSA,
    prime_Legacy
} PrimeTestType;

MOC_EXTERN MSTATUS PRIME_doPrimeTestsEx(MOC_MOD(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, vlong *pPrime, PrimeTestType type, intBoolean *pIsPrime, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS PRIME_doPrimeTests(MOC_MOD(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, vlong *pPrime, intBoolean *pIsPrime, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS PRIME_doDualPrimeTests(MOC_MOD(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, sbyte4 startingIndex, vlong *pPrimeA, vlong *pPrimeB, intBoolean *pIsBothPrime, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS PRIME_simplePrimeTest(MOC_MOD(hwAccelDescr hwAccelCtx) vlong *pPrime, ubyte4 startingIndex, ubyte4 endingIndex, intBoolean *pRetIsPrime, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS PRIME_generateSizedPrime(MOC_PRIME(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, vlong **ppRetPrime, ubyte4 numBitsLong, vlong **ppVlongQueue);

#endif /* __PRIME_HEADER__ */
