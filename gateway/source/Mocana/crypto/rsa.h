/* Version: mss_v6_3 */
/*
 * rsa.h
 *
 * RSA public key encryption
 *
 * Copyright Mocana Corp 2003-2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file rsa.h RSA cryptographic algorithm developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for RSA cryptographic algorithm operations.

\since 1.41
\version 3.06 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- RSA_byteStringFromKey
- RSA_cloneKey
- RSA_createKey
- RSA_decrypt
- RSA_encrypt
- RSA_equalKey
- RSA_freeKey
- RSA_generateKey
- RSA_getCipherTextLength
- RSA_keyFromByteString
- RSA_prepareKey
- RSA_setPublicKeyParameters
- RSA_setAllKeyParameters
- RSA_signMessage
- RSA_verifySignature

*/


/*------------------------------------------------------------------*/

#ifndef __RSA_H__
#define __RSA_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_RSA_VLONG   (7)
#define NUM_RSA_MODEXP  (2)

#if !defined( __DISABLE_MOCANA_RSA_DECRYPTION__) && !defined(__PSOS_RTOS__)
typedef struct BlindingHelper
{
    RTOS_MUTEX      blindingMutex;
    vlong*          pRE;
    vlong*          pR1;
    ubyte4          counter;
} BlindingHelper;
#endif

typedef enum RSAKeyType
{
    RsaPublicKey = 0,
    RsaPrivateKey = 1,
    RsaProtectedKey = 2 /* HW RSA key */
} RSAKeyType;

struct RSAKey;

typedef MSTATUS (*rsaSignCallback_t)(void* pParams,
                                     const ubyte* pData,
                                     ubyte4 dataLen,
                                     ubyte** ppSignedData,
                                     ubyte4* pSignedLen);

typedef MSTATUS (*rsaDecryptCallback_t)(void* pParams,
                                        const ubyte* pCipherText,
                                        ubyte4 cipherTextLen,
                                        ubyte** ppPlainText,
                                        ubyte4* pPlainTextLen);

typedef MSTATUS (*rsaValidateKeyCallback_t)(void* pParams, struct RSAKey* pKey, byteBoolean* isValid);

typedef MSTATUS (*rsaCopyCookieCallback_t)(void** dstCookie, void* srcCookie);

typedef void (*rsaFreeCookieCallback_t)(void* cookie);

typedef void (*rsaFreeDataCallback_t)(void* data);

typedef struct RSAKey
{
    RSAKeyType      keyType;
#ifdef __ENABLE_MOCANA_HW_SECURITY_MODULE__
    void* pParams;
    rsaSignCallback_t signCallback;
    rsaDecryptCallback_t decryptCallback;
    rsaValidateKeyCallback_t validateCallback;
    rsaCopyCookieCallback_t copyCookieCallback;
    rsaFreeCookieCallback_t freeCookieCallback;
    rsaFreeDataCallback_t freeDataCallback;
#endif
    vlong*          v[NUM_RSA_VLONG];
    ModExpHelper    modExp[NUM_RSA_MODEXP];
#if !defined(__DISABLE_MOCANA_RSA_DECRYPTION__) && !defined( __PSOS_RTOS__)
    BlindingHelper  blinding;
#endif
} RSAKey;

#define RSA_E(k)            ((k)->v[0])
#define RSA_N(k)            ((k)->v[1])  /* n=modulus=size of encrypted data */
#define RSA_P(k)            ((k)->v[2])
#define RSA_Q(k)            ((k)->v[3])
#define RSA_DP(k)           ((k)->v[4])
#define RSA_DQ(k)           ((k)->v[5])
#define RSA_QINV(k)         ((k)->v[6])
#define RSA_MODEXP_P(k)     ((k)->modExp[0])
#define RSA_MODEXP_Q(k)     ((k)->modExp[1])

#define RSA_KEYSIZE(k)	    (VLONG_bitLength(RSA_N(k)))

/*------------------------------------------------------------------*/

/* RSA primitives defined in PKCS#1 version 2.1 */
#if !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
extern MSTATUS RSA_RSADP(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKey, const vlong *pCipherText, vlong **ppMessage, vlong **ppVlongQueue);
#endif
extern MSTATUS RSA_RSAEP(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pPublicRSAKey, const vlong *pMessage, vlong **ppRetCipherText, vlong **ppVlongQueue);

#if (!defined(__DISABLE_MOCANA_RSA_DECRYPTION__) && defined(__RSAINT_HARDWARE__) && defined(__ENABLE_MOCANA_PKCS11_CRYPTO__))
#define RSA_RSASP1 RSAINT_decrypt
#elif (!defined(__DISABLE_MOCANA_RSA_DECRYPTION__))
extern MSTATUS RSA_RSASP1(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKey, const vlong *pMessage, RNGFun rngFun, void* rngFunArg, vlong **ppRetSignature, vlong **ppVlongQueue);
#endif
extern MSTATUS RSA_RSAVP1(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pPublicRSAKey, const vlong *pSignature, vlong **ppRetMessage, vlong **ppVlongQueue);


/*------------------------------------------------------------------*/

/* used for custom RSA blinding implementation */
/*!
\exclude
*/
typedef MSTATUS (*RSADecryptFunc)(MOC_RSA(hwAccelDescr hwAccelCtx)
                                         const RSAKey *pRSAKey,
                                         const vlong *c,
                                         vlong **ppRetDecrypt,
                                         vlong **ppVlongQueue);

/*!
\exclude
*/
typedef MSTATUS (*CustomBlindingFunc)( MOC_RSA(hwAccelDescr hwAccelCtx)
                                        const RSAKey* pRSAKeyInt,
                                        const vlong* pCipher,
                                        RNGFun rngFun, void* rngFunArg,
                                        RSADecryptFunc rsaDecryptPrimitive,
                                        vlong** ppRetDecrypt,
                                        vlong** ppVlongQueue);

/*------------------------------------------------------------------*/

/* exported for FIPS RSA Key Generation Testing */
MOC_EXTERN MSTATUS RSA_generateKeyFipsSteps(MOC_RSA(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, vlong *e, vlong *Xp, vlong *Xp1, vlong *Xp2, intBoolean *doContinue, vlong **p, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS RSA_createKey(RSAKey **pp_RetRSAKey);

MOC_EXTERN MSTATUS RSA_freeKey(RSAKey **ppFreeRSAKey, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS      RSA_cloneKey(RSAKey **ppNew, const RSAKey *pSrc, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS      RSA_equalKey(const RSAKey* key1, const RSAKey* key2, byteBoolean* res);

MOC_EXTERN MSTATUS RSA_setPublicKeyParameters(RSAKey *key,
                                              ubyte4 exponent,
                                              const ubyte* modulus,
                                              ubyte4 modulusLen,
                                              vlong **ppVlongQueue);

MOC_EXTERN MSTATUS RSA_setAllKeyParameters(MOC_RSA(hwAccelDescr hwAccelCtx)
                                           RSAKey *key,
                                           ubyte4 exponent,
                                           const ubyte* modulus,
                                           ubyte4 modulusLen,
                                           const ubyte* prime1,
                                           ubyte4 prime1len,
                                           const ubyte* prime2,
                                           ubyte4 prime2len,
                                           vlong** ppVlongQueue);


MOC_EXTERN MSTATUS   RSA_getCipherTextLength(const RSAKey *key, sbyte4* cipherTextLen);

MOC_EXTERN MSTATUS   RSA_encrypt(MOC_RSA(hwAccelDescr hwAccelCtx)
                             const RSAKey *key,
                             const ubyte* plainText,
                             ubyte4 plainTextLen,
                             ubyte* cipherText,
                             RNGFun rngFun,
                             void* rngFunArg,
                             vlong **ppVlongQueue);

#ifndef __DISABLE_MOCANA_RSA_DECRYPTION__
MOC_EXTERN MSTATUS   RSA_decrypt(MOC_RSA(hwAccelDescr hwAccelCtx)
                             const RSAKey *key,
                             const ubyte* cipherText,
                             ubyte* plainText,
                             ubyte4* plainTextLen,
                             RNGFun rngFun,
                             void* rngFunArg,
                             vlong **ppVlongQueue);
#endif

MOC_EXTERN MSTATUS  RSA_verifySignature(MOC_RSA(hwAccelDescr hwAccelCtx)
                                        const RSAKey *pKey,
                                        const ubyte* cipherText,
                                        ubyte* plainText,
                                        ubyte4* plainTextLen,
                                        vlong **ppVlongQueue);

MOC_EXTERN MSTATUS  RSA_signMessage(MOC_RSA(hwAccelDescr hwAccelCtx)
                                    const RSAKey *pKey,
                                    const ubyte* plainText,
                                    ubyte4 plainTextLen,
                                    ubyte* cipherText,
                                    vlong **ppVlongQueue);

MOC_EXTERN MSTATUS  RSA_generateKeyFIPS(MOC_RSA(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                RSAKey *p_rsaKey, ubyte4 keySize, vlong **Xp, vlong **Xp1, vlong **Xp2,
                vlong **Xq, vlong **Xq1, vlong **Xq2, vlong **ppVlongQueue);

MOC_EXTERN MSTATUS  RSA_generateKey(MOC_RSA(hwAccelDescr hwAccelCtx)
                                    randomContext *pRandomContext,
                                    RSAKey *p_rsaKey,
                                    ubyte4 keySize,
                                    vlong **ppVlongQueue);

MOC_EXTERN MSTATUS  RSA_prepareKey(MOC_RSA(hwAccelDescr hwAccelCtx)
                                   RSAKey *pRSAKey, vlong** ppVlongQueue);

MOC_EXTERN MSTATUS  RSA_keyFromByteString(MOC_RSA(hwAccelDescr hwAccelCtx)
                                            RSAKey **ppKey,
                                            const ubyte* buffer,
                                            ubyte4 bufferLen,
                                            vlong **ppVlongQueue);

MOC_EXTERN MSTATUS  RSA_byteStringFromKey(MOC_RSA(hwAccelDescr hwAccelCtx)
                                          const RSAKey *key, ubyte* buffer,
                                          ubyte4* pRetLen);

#ifdef __cplusplus
}
#endif

#endif
