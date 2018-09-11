/* Version: mss_v6_3 */
/*
 * pkcs1.h
 *
 * PKCS#1 Version 2.1 Header
 *
 * Copyright Mocana Corp 2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __PKCS1_HEADER__
#define __PKCS1_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------*/

/* exported routines */
#ifdef __ENABLE_MOCANA_PKCS1__

/*!
\exclude
*/
typedef MSTATUS(*mgfFunc)(hwAccelDescr hwAccelCtx, const ubyte *mgfSeed, ubyte4 mgfSeedLen, ubyte4 maskLen, BulkHashAlgo *H, ubyte **ppRetMask);


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS PKCS1_rsaesOaepEncrypt(hwAccelDescr hwAccelCtx, randomContext *pRandomContext, const RSAKey *pRSAKey, ubyte H_rsaAlgoId, mgfFunc MGF, const ubyte *M, ubyte4 mLen, const ubyte *L, ubyte4 lLen, ubyte **ppRetEncrypt, ubyte4 *pRetEncryptLen);
#if (!defined(__DISABLE_MOCANA_RSA_DECRYPTION__))
MOC_EXTERN MSTATUS PKCS1_rsaesOaepDecrypt(hwAccelDescr hwAccelCtx, const RSAKey *pRSAKey, ubyte H_rsaAlgoId, mgfFunc MGF, const ubyte *C, ubyte4 cLen, const ubyte *L, ubyte4 lLen, ubyte **ppRetDecrypt, ubyte4 *pRetDecryptLength);
#endif

#if (!defined(__DISABLE_MOCANA_RSA_DECRYPTION__))
MOC_EXTERN MSTATUS PKCS1_rsassaPssSign(hwAccelDescr hwAccelCtx, randomContext *pRandomContext, const RSAKey *pRSAKey, ubyte H_rsaAlgoId, mgfFunc MGF, const ubyte *pMessage, ubyte4 mesgLen, ubyte4 saltLen, ubyte **ppRetSignature, ubyte4 *pRetSignatureLen);
MOC_EXTERN MSTATUS PKCS1_rsassaFreePssSign(hwAccelDescr hwAccelCtx, ubyte **ppSignature);
#endif
MOC_EXTERN MSTATUS PKCS1_rsassaPssVerify(hwAccelDescr hwAccelCtx, const RSAKey *pRSAKey, ubyte H_rsaAlgoId, mgfFunc MGF, const ubyte * const pMessage, ubyte4 mesgLen, const ubyte *pSignature, ubyte4 signatureLen, ubyte4 saltLen, intBoolean *pRetIsSignatureValid);

/* helper function */
#ifdef OPENSSL_ENGINE
MOC_EXTERN MSTATUS MOC_PKCS1_MGF1(hwAccelDescr hwAccelCtx, const ubyte *mgfSeed, ubyte4 mgfSeedLen, ubyte4 maskLen, BulkHashAlgo *H, ubyte **ppRetMask);
#else
MOC_EXTERN MSTATUS PKCS1_MGF1(hwAccelDescr hwAccelCtx, const ubyte *mgfSeed, ubyte4 mgfSeedLen, ubyte4 maskLen, BulkHashAlgo *H, ubyte **ppRetMask);
#endif
MOC_EXTERN MSTATUS PKCS1_MGF1(hwAccelDescr hwAccelCtx, const ubyte *mgfSeed, ubyte4 mgfSeedLen, ubyte4 maskLen, BulkHashAlgo *H, ubyte **ppRetMask);
MOC_EXTERN MSTATUS PKCS1_OS2IP(const ubyte *pMessage, ubyte4 mesgLen, vlong **ppRetM);
MOC_EXTERN MSTATUS PKCS1_I2OSP(vlong *pValue, ubyte4 fixedLength, ubyte **ppRetString);

#endif /*__ENABLE_MOCANA_PKCS1__ */

#ifdef __cplusplus
}
#endif


#endif  /* __PKCS1_HEADER__ */
