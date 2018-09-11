/* Version: mss_v6_3 */
/*
 * dsa2.h
 *
 * DSA with hashes other than SHA-1 (FIPS 186-3)
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/


#ifndef __DSA2_HEADER__
#define __DSA2_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

MOC_EXTERN MSTATUS DSA_computeSignature2(MOC_DSA(hwAccelDescr hwAccelCtx)
                                          RNGFun rngfun, void* rngArg,
                                          const DSAKey *p_dsaDescr,
                                          const ubyte* msg, ubyte4 msgLen,
                                          vlong **ppR, vlong **ppS, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_verifySignature2(MOC_DSA(hwAccelDescr hwAccelCtx)
                                        const DSAKey *p_dsaDescr,
                                        const ubyte *msg, ubyte4 msgLen,
                                        vlong *pR, vlong *pS,
                                        intBoolean *isGoodSignature,
                                        vlong **ppVlongQueue);


#ifdef __cplusplus
}
#endif

#endif /* __DSA2_HEADER__ */
