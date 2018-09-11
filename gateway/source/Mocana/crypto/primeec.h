/* Version: mss_v6_3 */
/*
 * primeec.h
 *
 * Finite Field Elliptic Curve Header
 *
 * Copyright Mocana Corp 2006-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */
/*! \file primeec.h Finite Field Elliptic Curve developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for EC operations.

\since 3.06
\version 5.3 and later

To enable any of this file's functions, the following flags must be defined in
moptions.h:
- $__ENABLE_MOCANA_ECC__$

! External Functions
This file contains the following public ($extern$) function declarations:
- EC_byteStringToPoint
- EC_pointToByteString
- ECDSA_sign
- ECDSA_verifySignature

*/



/*------------------------------------------------------------------*/

#ifndef __PRIMEEC_HEADER__
#define __PRIMEEC_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(__ENABLE_MOCANA_ECC__))
/* Support for Finite Field Elliptic Curve Operations */

typedef const struct PrimeEllipticCurve* PEllipticCurvePtr;

typedef struct ECCKey
{
    intBoolean          privateKey;
    PFEPtr              Qx;         /* public */
    PFEPtr              Qy;         /* public */
    PFEPtr              k;          /* private*/
    PEllipticCurvePtr   pCurve;     /* curve */
} ECCKey;

/* NIST curves */
#ifndef __ENABLE_MOCANA_ECC_P192__
#define NUM_EC_P192 (0)
#else
#define NUM_EC_P192 (1)
MOC_EXTERN const PEllipticCurvePtr EC_P192;
#endif

#ifdef __DISABLE_MOCANA_ECC_P224__
#define NUM_EC_P224 (0)
#else
#define NUM_EC_P224 (1)
MOC_EXTERN const PEllipticCurvePtr EC_P224;
#endif

#ifdef __DISABLE_MOCANA_ECC_P256__
#define NUM_EC_P256 (0)
#else
#define NUM_EC_P256 (1)
MOC_EXTERN const PEllipticCurvePtr EC_P256;
#endif

#ifdef __DISABLE_MOCANA_ECC_P384__
#define NUM_EC_P384 (0)
#else
#define NUM_EC_P384 (1)
MOC_EXTERN const PEllipticCurvePtr EC_P384;
#endif

#ifdef __DISABLE_MOCANA_ECC_P521__
#define NUM_EC_P521 (0)
#else
#define NUM_EC_P521 (1)
MOC_EXTERN const PEllipticCurvePtr EC_P521;
#endif

#define NUM_ECC_PCURVES    ((NUM_EC_P192) + (NUM_EC_P224) + (NUM_EC_P256) + \
                                        (NUM_EC_P384) + (NUM_EC_P521))

MOC_EXTERN PrimeFieldPtr EC_getUnderlyingField(PEllipticCurvePtr pEC);

/* operations on points */
/* k (X,Y) */
MOC_EXTERN MSTATUS EC_multiplyPoint(PrimeFieldPtr pPF, PFEPtr pResX, PFEPtr pResY,
                                    ConstPFEPtr k, ConstPFEPtr pX, ConstPFEPtr pY);

/* (AddedX, AddedY) + k (X,Y) */
MOC_EXTERN MSTATUS EC_addMultiplyPoint(PrimeFieldPtr pPF, PFEPtr pResX, PFEPtr pResY,
                                        ConstPFEPtr pAddedX, ConstPFEPtr pAddedY,
                                        ConstPFEPtr k, ConstPFEPtr pX, ConstPFEPtr pY);

/* key management */
MOC_EXTERN MSTATUS EC_newKey(PEllipticCurvePtr pEC, ECCKey** ppNewKey);
MOC_EXTERN MSTATUS EC_deleteKey(ECCKey** ppKey);
MOC_EXTERN MSTATUS EC_cloneKey(ECCKey** ppNew, const ECCKey* pSrc);
MOC_EXTERN MSTATUS EC_equalKey(const ECCKey* pKey1, const ECCKey* pKey2,
                               byteBoolean* res);
MOC_EXTERN MSTATUS EC_setKeyParameters(ECCKey* pKey,
                                       const ubyte* point, ubyte4 pointLen,
                                       const ubyte* scalar, ubyte4 scalarLen);

MOC_EXTERN MSTATUS EC_verifyKeyPair(PEllipticCurvePtr pEC, ConstPFEPtr k,
                                    ConstPFEPtr pQx, ConstPFEPtr pQy);

MOC_EXTERN MSTATUS EC_generateKeyPair(PEllipticCurvePtr pEC, RNGFun rngFun, void* rngArg,
                                      PFEPtr k, PFEPtr pQx, PFEPtr pQy);

MOC_EXTERN MSTATUS EC_verifyPublicKey(PEllipticCurvePtr pEC,
                                      ConstPFEPtr pQx, ConstPFEPtr pQy);

MOC_EXTERN MSTATUS EC_pointToByteString(PEllipticCurvePtr pEC,
                                        ConstPFEPtr pX, ConstPFEPtr pY,
                                        ubyte** s, sbyte4* pLen);

MOC_EXTERN MSTATUS EC_getPointByteStringLen(PEllipticCurvePtr pEC, sbyte4 *pLen);

MOC_EXTERN MSTATUS EC_writePointToBuffer(PEllipticCurvePtr pEC,
                                         ConstPFEPtr pX, ConstPFEPtr pY,
                                         ubyte* s, sbyte4 len);

MOC_EXTERN MSTATUS EC_byteStringToPoint(PEllipticCurvePtr pEC,
                                        const ubyte* s, sbyte4 len,
                                        PFEPtr* ppX, PFEPtr* ppY);
#ifdef OPENSSL_ENGINE
MOC_EXTERN MSTATUS MOC_ECDSA_sign(PEllipticCurvePtr pEC, ConstPFEPtr d,
                              RNGFun rngFun, void* rngArg,
                              const ubyte* hash, ubyte4 hashLen,
                              PFEPtr r, PFEPtr s);
#else
MOC_EXTERN MSTATUS ECDSA_sign(PEllipticCurvePtr pEC, ConstPFEPtr d,
                              RNGFun rngFun, void* rngArg,
                              const ubyte* hash, ubyte4 hashLen,
                              PFEPtr r, PFEPtr s);
#endif
MOC_EXTERN MSTATUS ECDSA_verifySignature(PEllipticCurvePtr pEC,
                                         ConstPFEPtr pPublicKeyX, ConstPFEPtr pPublicKeyY,
                                         const ubyte* hash, ubyte4 hashLen,
                                         ConstPFEPtr r, ConstPFEPtr s);

MOC_EXTERN MSTATUS ECDH_generateSharedSecretAux(PEllipticCurvePtr pEC,
                                                ConstPFEPtr pX, ConstPFEPtr pY,
                                                ConstPFEPtr scalarMultiplier,
                                                ubyte** sharedSecret,
                                                sbyte4* sharedSecretLen,
                                                sbyte4 flag);

MOC_EXTERN MSTATUS ECDH_generateSharedSecret(PEllipticCurvePtr pEC,
                                             const ubyte* pointByteString,
                                             sbyte4 pointByteStringLen,
                                             ConstPFEPtr scalarMultiplier,
                                             ubyte** sharedSecret,
                                             sbyte4* sharedSecretLen);

/* comb multiply is disabled for small code foot print unless
   __ENABLE_MOCANA_ECC_COMB__ is defined */
#if defined(__ENABLE_MOCANA_ECC_COMB__) || !defined( __ENABLE_MOCANA_SMALL_CODE_FOOTPRINT__)

/* size of the precomputed block in pf_unit = (2^w - 2) * 2 * n */
MSTATUS EC_combSize( PrimeFieldPtr pPF, sbyte4 windowSize, sbyte4* size);

/* comb multiplication routines */
MOC_EXTERN MSTATUS EC_precomputeComb(PrimeFieldPtr pPF, ConstPFEPtr pQx,
                                     ConstPFEPtr pQy, sbyte4 windowSize,
                                     PFEPtr* pPrecomputed);

MOC_EXTERN MSTATUS EC_precomputeCombOfCurve(PEllipticCurvePtr pEC, sbyte4 windowSize,
                                            PFEPtr* pCurvePrecomputed);

#endif /* __ENABLE_MOCANA_ECC_COMB__ !__ENABLE_MOCANA_SMALL_CODE_FOOTPRINT__ */


MOC_EXTERN MSTATUS EC_multiplyPointEx(PrimeFieldPtr pPF, PFEPtr pResX, PFEPtr pResY,
                                      ConstPFEPtr k, ConstPFEPtr pX, ConstPFEPtr pY,
                                      sbyte4 windowSize, ConstPFEPtr pPrecomp);

MOC_EXTERN MSTATUS EC_addMultiplyPointEx(PrimeFieldPtr pPF, PFEPtr pResX, PFEPtr pResY,
                                        ConstPFEPtr pAddedX, ConstPFEPtr pAddedY,
                                        ConstPFEPtr k, ConstPFEPtr pX, ConstPFEPtr pY,
                                        sbyte4 windowSize, ConstPFEPtr pPrecomp);


MOC_EXTERN MSTATUS EC_verifyKeyPairEx(PEllipticCurvePtr pEC, ConstPFEPtr k,
                                      ConstPFEPtr pQx, ConstPFEPtr pQy,
                                      sbyte4 windowSize, ConstPFEPtr pCurvePrecomputed);

MOC_EXTERN MSTATUS EC_generateKeyPairEx(PEllipticCurvePtr pEC, RNGFun rngFun, void* rngArg,
                                        sbyte4 windowSize, ConstPFEPtr pCurvePrecomputed,
                                        PFEPtr k, PFEPtr pQx, PFEPtr pQy);

MOC_EXTERN MSTATUS ECDSA_signEx(PEllipticCurvePtr pEC, ConstPFEPtr d,
                                RNGFun rngFun, void* rngArg,
                                const ubyte* hash, ubyte4 hashLen,
                                sbyte4 windowSize, ConstPFEPtr pCurvePrecomp,
                                PFEPtr r, PFEPtr s);

MOC_EXTERN MSTATUS ECDSA_verifySignatureEx(PEllipticCurvePtr pEC,
                                           ConstPFEPtr pPublicKeyX, ConstPFEPtr pPublicKeyY,
                                           const ubyte* hash, ubyte4 hashLen,
                                           sbyte4 curveWinSize, ConstPFEPtr pCurvePrecomp,
                                           sbyte4 pubKeyWinSize, ConstPFEPtr pPubKeyPrecomp,
                                           ConstPFEPtr r, ConstPFEPtr s);

#endif /* __ENABLE_MOCANA_ECC__  */

#ifdef __cplusplus
}
#endif

#endif /* __PRIMEFLD_HEADER__ */

