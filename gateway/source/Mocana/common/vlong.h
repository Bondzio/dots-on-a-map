/* Version: mss_v6_3 */
/*
 * vlong.h
 *
 * Very Long Integer Library
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __VLONG_HEADER__
#define __VLONG_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


#include "../crypto/hw_accel.h"

#ifdef __ENABLE_MOCANA_64_BIT__
typedef ubyte8 vlong_unit;
#else
typedef ubyte4 vlong_unit;
#endif


typedef struct vlong /* Provides storage allocation and index checking */
{

    vlong_unit* pUnits;                /* array of units */
    ubyte4       numUnitsAllocated;      /* units allocated */
    ubyte4       numUnitsUsed;           /* used units */
    intBoolean   negative;

    struct vlong*    pNextVlong;        /* used to chain vlong variables */

} vlong;


/*------------------------------------------------------------------*/

/* manipulate vlongs */
MOC_EXTERN MSTATUS VLONG_modexp(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *x, const vlong *e, const vlong *m, vlong **ppRetModExp, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_greatestCommonDenominator(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *pValueX, const vlong *pValueY, vlong **ppGcd, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_modularInverse(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *pA, const vlong *pModulus, vlong **ppRetModularInverse, vlong **ppVlongQueue);
MOC_EXTERN ubyte4  VLONG_bitLength(const vlong *pThis);
MOC_EXTERN MSTATUS VLONG_shrVlong(vlong *pThis);
MOC_EXTERN MSTATUS VLONG_shlVlong(vlong *pThis);
MOC_EXTERN MSTATUS VLONG_shlXvlong(vlong *pThis, ubyte4 numBits);
MOC_EXTERN MSTATUS VLONG_shrXvlong(vlong *pThis, ubyte4 numBits);
MOC_EXTERN MSTATUS VLONG_decrement(vlong *pThis, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_increment(vlong *pThis, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_addImmediate(vlong *pThis, ubyte4 immVal, vlong **ppVlongQueue);
MOC_EXTERN intBoolean VLONG_isVlongZero(const vlong *pThis);
MOC_EXTERN sbyte4  VLONG_compareSignedVlongs(const vlong *pValueX, const vlong* pValueY);
MOC_EXTERN sbyte4  VLONG_compareUnsigned(const vlong* pTest, vlong_unit immValue);
MOC_EXTERN intBoolean VLONG_isVlongBitSet(const vlong *pThis, ubyte4 testBit);
MOC_EXTERN MSTATUS VLONG_setVlongBit(vlong *pThis, ubyte4 setBit);
MOC_EXTERN MSTATUS VLONG_unsignedDivide( vlong* pQuotient, const vlong* pDividend,
                                        const vlong* pDivisor, vlong* pRemainder,
                                        vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong* pDividend, const vlong* pDivisor, vlong **ppRemainder, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_operatorDivideSignedVlongs(const vlong* pDividend, const vlong* pDivisor, vlong **ppQuotient, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_addSignedVlongs(vlong *pSumAndValue, const vlong *pValue, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_subtractSignedVlongs(vlong *pResultAndValue, const vlong *pValue, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_copySignedValue(vlong *pDest, const vlong *pSource);
MOC_EXTERN MSTATUS VLONG_vlongSignedMultiply(vlong *pProduct, const vlong *pFactorX, const vlong *pFactorY);
MOC_EXTERN MSTATUS VLONG_unsignedMultiply(vlong *pProduct, const vlong *pFactorX, const vlong *pFactorY);
MOC_EXTERN MSTATUS VLONG_vlongSignedSquare(vlong *pProduct, const vlong *pFactor);
MOC_EXTERN MSTATUS VLONG_clearVlong(vlong *pThis);
MOC_EXTERN vlong_unit  VLONG_getVlongUnit(const vlong *pThis, ubyte4 index);
MOC_EXTERN MSTATUS VLONG_setVlongUnit(vlong *pThis, ubyte4 index, vlong_unit unitValue);

/* vlong constructors */
MOC_EXTERN MSTATUS VLONG_allocVlong(vlong **ppRetVlongValue, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_reallocVlong(vlong *pThis, ubyte4 vlongNewLength);
MOC_EXTERN MSTATUS VLONG_vlongFromByteString(const ubyte* byteString, sbyte4 len, vlong **ppRetVlong,
                                         vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_byteStringFromVlong(const vlong* pValue, ubyte* pDest, sbyte4* pRetLen);
MOC_EXTERN MSTATUS VLONG_newFromMpintBytes(const ubyte *pArray, ubyte4 bytesAvailable, vlong **ppNewVlong,
                                       ubyte4 *pRetNumBytesUsed);
MOC_EXTERN MSTATUS VLONG_vlongFromUByte4String(const ubyte4 *pUByte4Str, ubyte4 len, vlong **ppNewVlong);
MOC_EXTERN MSTATUS VLONG_makeVlongFromUnsignedValue(vlong_unit value, vlong **ppRetVlong, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_makeVlongFromVlong(const vlong* pValue, vlong **ppRetVlong, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_mpintByteStringFromVlong(const vlong* pValue, ubyte** ppDest, sbyte4* pRetLen);
MOC_EXTERN MSTATUS VLONG_fixedByteStringFromVlong(vlong* pValue, ubyte* pDest, sbyte4 fixedLength);


MOC_EXTERN MSTATUS VLONG_makeRandomVlong(void *pRandomContext, vlong **ppRetPrime, ubyte4 numBitsLong, vlong **ppVlongQueue);

/* vlong destructor */
MOC_EXTERN MSTATUS VLONG_freeVlong(vlong **ppFreeVlong, vlong **ppVlongQueue);

/* vlong queue destructor */

MOC_EXTERN MSTATUS VLONG_freeVlongQueue(vlong **ppVlongQueue);

/* Modular Exponentiation Caches */
typedef const struct MontgomeryCtx* CModExpHelper;
typedef struct MontgomeryCtx* ModExpHelper;

MOC_EXTERN MSTATUS VLONG_newModExpHelper(MOC_MOD(hwAccelDescr hwAccelCtx) ModExpHelper* pMEH, const vlong* m, vlong** ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_deleteModExpHelper( ModExpHelper* pMEH, vlong** ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_modExp(MOC_MOD(hwAccelDescr hwAccelCtx) CModExpHelper meh,  const vlong *x, const vlong *e, vlong **ppRetModExp, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS VLONG_makeModExpHelperFromModExpHelper( CModExpHelper meh,  ModExpHelper* pMEH, vlong **ppVlongQueue);

#if 0
MOC_EXTERN MSTATUS VLONG_byteStringFromModExpHelper( CModExpHelper, ubyte* pDest, sbyte4* pRetLen);
MOC_EXTERN MSTATUS VLONG_modExpHelperFromByteString( const ubyte* byteString, sbyte4 len, ModExpHelper* pMEH, vlong** ppVlongQueue);
#endif

/* Barrett reduction */
MOC_EXTERN MSTATUS VLONG_newBarrettMu( vlong** pMu, const vlong* m, vlong** ppVlongQueue);
MOC_EXTERN MSTATUS VLONG_barrettMultiply( vlong* pResult, const vlong* pX, const vlong* pY,
                                      const vlong* pM, const vlong* pMu, vlong** ppVlongQueue);

#ifdef __cplusplus
}
#endif

#endif /* __VLONG_HEADER__ */
