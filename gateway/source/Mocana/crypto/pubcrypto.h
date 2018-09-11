/* Version: mss_v6_3 */
/*
 * pubcrypto.h
 *
 * General Public Crypto Definitions & Types Header
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __PUBCRYPTO_HEADER__
#define __PUBCRYPTO_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(__ENABLE_MOCANA_ECC__))
struct ECCKey;
struct PrimeEllipticCurve;
#endif

struct RSAKey;

#if (defined(__ENABLE_MOCANA_DSA__))
struct DSAKey;
#endif
struct AsymmetricKey;

typedef struct AsymmetricKey
{
    ubyte4 type;
    union
    {
        struct RSAKey* pRSA;
#if (defined(__ENABLE_MOCANA_ECC__))
        struct ECCKey* pECC;
#endif
#if (defined(__ENABLE_MOCANA_DSA__))
        struct DSAKey* pDSA;
#endif
    } key;
} AsymmetricKey;

MOC_EXTERN MSTATUS  CRYPTO_initAsymmetricKey(AsymmetricKey* pKey);
MOC_EXTERN MSTATUS  CRYPTO_copyAsymmetricKey(AsymmetricKey* pNew, const AsymmetricKey* pSrc);
MOC_EXTERN MSTATUS  CRYPTO_matchPublicKey(const AsymmetricKey* pKey1, const AsymmetricKey* pKey2);
MOC_EXTERN MSTATUS  CRYPTO_uninitAsymmetricKey(AsymmetricKey* pKey, vlong** ppVlongQueue);
#if (defined(__ENABLE_MOCANA_ECC__))
MOC_EXTERN ubyte4   CRYPTO_getECCurveId( const AsymmetricKey* pKey);
MOC_EXTERN MSTATUS  CRYPTO_getECCurveOID( const struct ECCKey* pKey, const ubyte* *pCurveOID);
#endif

/* RSA key */
MOC_EXTERN MSTATUS  CRYPTO_createRSAKey( AsymmetricKey* pKey, vlong** ppVlongQueue);
MOC_EXTERN MSTATUS  CRYPTO_setRSAParameters(MOC_RSA(hwAccelDescr hwAccelCtx)
                                        AsymmetricKey* pKey,
                                        ubyte4 exponent,
                                        const ubyte* modulus,
                                        ubyte4 modulusLen,
                                        const ubyte* p,
                                        ubyte4 pLen,
                                        const ubyte* q,
                                        ubyte4 qLen,
                                        vlong **ppVlongQueue);
/* ECC key */
#if (defined(__ENABLE_MOCANA_ECC__))
MOC_EXTERN MSTATUS  CRYPTO_createECCKey( AsymmetricKey* pKey,
                                        const struct PrimeEllipticCurve* pCurve,
                                        vlong** ppVlongQueue);
MOC_EXTERN MSTATUS  CRYPTO_setECCParameters( AsymmetricKey* pKey,
                                        ubyte4 curveId,
                                        const ubyte* point,
                                        ubyte4 pointLen,
                                        const ubyte* scalar,
                                        ubyte4 scalarLen,
                                        vlong** ppVlongQueue);

#endif

#if (defined(__ENABLE_MOCANA_DSA__))
/* DSA/DSS key */
MOC_EXTERN MSTATUS  CRYPTO_createDSAKey( AsymmetricKey* pKey, vlong** ppVlongQueue);
MOC_EXTERN MSTATUS  CRYPTO_setDSAParameters( MOC_DSA(hwAccelDescr hwAccelCtx) AsymmetricKey* pKey,
                                        const ubyte* p,
                                        ubyte4 pLen,
                                        const ubyte* q,
                                        ubyte4 qLen,
                                        const ubyte* g,
                                        ubyte4 gLen,
                                        const ubyte* y,
                                        ubyte4 yLen,
                                        const ubyte* x,
                                        ubyte4 xLen,
                                        vlong **ppVlongQueue);
#endif

/* Abstraction between software and secmod operations */
#if (defined(__ENABLE_MOCANA_HW_SECURITY_MODULE__) && defined(__ENABLE_MOCANA_TPM__))
MOC_EXTERN MSTATUS  CRYPTO_createTPMRSAKey(secModDescr* secModContext, AsymmetricKey* pKey);
#endif

MOC_EXTERN MSTATUS  CRYPTO_serializeIn(ubyte* pKeyblob, ubyte4 keyBlobLen, AsymmetricKey* pKey);

MOC_EXTERN MSTATUS  CRYPTO_serializeOut(AsymmetricKey* pKey, ubyte** ppKeyBlob, ubyte4* pKeyBlobLen);

/*end abstraction between software and secmod operations */

#ifdef __cplusplus
}
#endif
        
#endif /* __PUBCRYPTO_HEADER__ */
