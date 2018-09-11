/* Version: mss_v6_3 */
/*
 * dsa.h
 *
 * DSA Factory Header
 *
 * Copyright Mocana Corp 2003-2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/* \file dsa.h DSA cryptographic algorithm developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for DSA cryptographic algorithm operations.

\since 1.41
\version 5.3 and later

! Flags
No flag definitions are required to use this file.

! External Functions
This file contains the following public ($extern$) function declarations:
- DSA_cloneKey
- DSA_computeKeyPair
- DSA_computeSignature
- DSA_createKey
- DSA_equalKey
- DSA_extractKeyBlob
- DSA_freeKey
- DSA_generateKey
- DSA_generateKeyEx
- DSA_makeKeyBlob
- DSA_verifyKeys
- DSA_verifyKeysEx
- DSA_verifySignature

*/
/*------------------------------------------------------------------*/


#ifndef __DSA_HEADER__
#define __DSA_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------*/

#define DSA_CONTEXT(X)          (X)->p_dsaDescr
#define PRIVATE_KEY_BYTE_SIZE   (20)
#define DSA_SIGNATURE_LEN       (2 * PRIVATE_KEY_BYTE_SIZE)


/*------------------------------------------------------------------*/

struct vlong;

#define NUM_DSA_VLONG   (5)
#define NUM_DSA_MODEXP  (2)

typedef struct DSAKey
{
    vlong*          dsaVlong[NUM_DSA_VLONG];
} DSAKey;

typedef enum
{
    DSA_sha1,
    DSA_sha224,
    DSA_sha256,
    DSA_sha384,
    DSA_sha512,
} DSAHashType;

typedef enum
{
    DSA_186_2,
    DSA_186_4
} DSAKeyType;

#define DSA_P(k)            ((k)->dsaVlong[0])
#define DSA_Q(k)            ((k)->dsaVlong[1])
#define DSA_G(k)            ((k)->dsaVlong[2])
#define DSA_Y(k)            ((k)->dsaVlong[3])
#define DSA_X(k)            ((k)->dsaVlong[4])


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS DSA_createKey(DSAKey **pp_dsaDescr);

MOC_EXTERN MSTATUS DSA_cloneKey(DSAKey** ppNew, const DSAKey* pSrc);

MOC_EXTERN MSTATUS DSA_freeKey(DSAKey **pp_dsaDescr, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_generateKey(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, DSAKey *p_dsaDescr, ubyte4 keySize, ubyte4 *pRetC, ubyte *pRetSeed, vlong **ppRetH, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_generateKeyEx(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, DSAKey *p_dsaDescr, ubyte4 keySize, ubyte4 qSize, DSAHashType hashType, ubyte4 *pRetC, ubyte *pRetSeed, vlong **ppRetH, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_computeKeyPair(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, DSAKey *p_dsaDescr, vlong **ppVlongQueue);

/* These 3 functions must be used with SHA1 hashes only */
MOC_EXTERN MSTATUS DSA_computeSignature(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, const DSAKey *p_dsaDescr,
                                        vlong* m, intBoolean *pVerifySignature, vlong **ppR, vlong **ppS, vlong **ppVlongQueue);

/* same as DSA_computeSignature but can use any random number generator */
MOC_EXTERN MSTATUS DSA_computeSignatureEx(MOC_DSA(hwAccelDescr hwAccelCtx)
                                          RNGFun rngfun, void* rngArg,
                                          const DSAKey *p_dsaDescr, vlong* m,
                                          intBoolean *pVerifySignature,
                                          vlong **ppR, vlong **ppS, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_verifySignature(MOC_DSA(hwAccelDescr hwAccelCtx) const DSAKey *p_dsaDescr,
                                       vlong *m, vlong *pR, vlong *pS, intBoolean *isGoodSignature, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_verifyKeys(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, ubyte *pSeed, const DSAKey *p_dsaDescr, ubyte4 C, vlong *pH, intBoolean *isGoodKeys, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_verifyKeysEx(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, ubyte *pSeed, ubyte4 seedSize, const DSAKey *p_dsaDescr, DSAHashType hashType, DSAKeyType keyType, ubyte4 C, vlong *pH, intBoolean *isGoodKeys, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_verifyPQ(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx,
            DSAKey *p_dsaDescr, ubyte4 L, ubyte4 Nin, DSAHashType hashType, DSAKeyType keyType, ubyte4 C,
            ubyte *pSeed, ubyte4 seedSize, intBoolean *pIsPrimePQ, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_verifyG(MOC_DSA(hwAccelDescr hwAccelCtx) vlong *pP, vlong *pQ, vlong *pG, intBoolean *isValid, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_makeKeyBlob(const DSAKey *p_dsaDescr, ubyte *pKeyBlob, ubyte4 *pRetKeyBlobLength);

MOC_EXTERN MSTATUS DSA_extractKeyBlob(DSAKey **pp_RetNewDsaDescr, const ubyte *pKeyBlob, ubyte4 keyBlobLength);

MOC_EXTERN MSTATUS DSA_equalKey(const DSAKey *pKey1, const DSAKey *pKey2, byteBoolean* pResult);

MOC_EXTERN MSTATUS DSA_setAllKeyParameters(MOC_DSA(hwAccelDescr hwAccelCtx) DSAKey* pKey,  const ubyte* p, ubyte4 pLen,
                                            const ubyte* q, ubyte4 qLen,
                                            const ubyte* g, ubyte4 gLen,
                                            const ubyte* x, ubyte4 xLen,
                                            vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_setPublicKeyParameters( DSAKey* pKey,  const ubyte* p, ubyte4 pLen,
                                            const ubyte* q, ubyte4 qLen,
                                            const ubyte* g, ubyte4 gLen,
                                            const ubyte* y, ubyte4 yLen,
                                            vlong **ppVlongQueue);

MOC_EXTERN MSTATUS DSA_getCipherTextLength(const DSAKey *pKey, sbyte4* cipherTextLen);

MOC_EXTERN MSTATUS generatePQ(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx,
                                            DSAKey *p_dsaDescr, ubyte4 L,
                                            ubyte4 Nin, DSAHashType hashType,
                                            ubyte4 *pRetC, ubyte *pRetSeed,
                                            vlong **ppVlongQueue);

MOC_EXTERN MSTATUS generateG(MOC_DSA(hwAccelDescr hwAccelCtx) DSAKey *p_dsaDescr,
                                            vlong **ppRetH, vlong **ppVlongQueue);

#ifdef __cplusplus
}
#endif

#endif /* __DSA_HEADER__ */
