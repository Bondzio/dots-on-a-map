/* Version: mss_v6_3 */
/*
 * pkcs10.h
 *
 * PKCS #10 Header
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */
/* \file pkcs10.h PKCS10 API header.
This header file contains definitions, enumerations, structures, and function
declarations used by PKCS10 functions.

\since 1.41
\version 5.3 and later

! Flags
To build products using this header file, the following flag must be defined
in moptions.h:
- $__ENABLE_MOCANA_PKCS10__$

! External Functions
This file contains the following public ($extern$) function declarations:
- PKCS10_GenerateSelfSignedCert
- PKCS10_generateCSR
- PKCS10_generateCSREx

*/


/*------------------------------------------------------------------*/

#ifndef __PKCS10_HEADER__
#define __PKCS10_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/* PKCS#10 certificate request attributes as defined in PKCS#9 */
typedef struct requestAttributes
{
    sbyte*      pChallengePwd;             /* ChallengePassword */
    ubyte4      challengePwdLength;

    certExtensions *pExtensions;
} requestAttributes;

/* outbound CSR BER encoded use FREE to delete */

#ifdef __ENABLE_MOCANA_PKCS10__
MOC_EXTERN MSTATUS PKCS10_GenerateCertReq(AsymmetricKey* pPubKey, certDistinguishedName *pCertInfo,
                       ubyte** ppCertReq, ubyte4* pCertReqLength);

MOC_EXTERN MSTATUS PKCS10_GenerateCertReqEx(AsymmetricKey* pPubKey, certDistinguishedName *pCertInfo,
                       requestAttributes *reqAttrs, ubyte** ppCertReq, ubyte4* pCertReqLength);

MOC_EXTERN MSTATUS PKCS10_GenerateCertReqEx2(AsymmetricKey* pPubKey, certDistinguishedName *pCertInfo,
                        const ubyte* signAlgoOID, requestAttributes *pReqAttrs,
                        ubyte** ppCertReq, ubyte4* pCertReqLength);


MOC_EXTERN MSTATUS PKCS10_GenerateCertReqFromASN1( AsymmetricKey* pPubKey, const ubyte* pASN1Name,
                                              ubyte4 asn1NameLen, ubyte** ppCertReq,
                                              ubyte4* pCertReqLength);
/* This routine generates a self signed X509v3 certificate based on
 * the key pair, certificate subject Name, certificate extensions,
 * and a signing algorithm */
MOC_EXTERN MSTATUS PKCS10_GenerateSelfSignedCert(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) AsymmetricKey* pKey,
                                             const certDistinguishedName *pSubjectInfo,
                                             const certExtensions* pExtensions,
                                             ubyte signAlgo, RNGFun rngFun, void* rngFunArg,
                                             ubyte** ppSelfSignedCert, ubyte4* pSelfSignedCertLength);


/* outbound CSR base 64 encoded */
MOC_EXTERN MSTATUS PKCS10_generateCSR(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte **ppRetCsr, ubyte4 *p_retCsrLength, certDistinguishedName *pCertInfo);
MOC_EXTERN MSTATUS PKCS10_generateCSREx(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte **ppRetCsr, ubyte4 *p_retCsrLength, certDistinguishedName *pCertInfo, requestAttributes *reqAttrs);
MOC_EXTERN MSTATUS PKCS10_generateCSRFromASN1(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte **ppRetCsr, ubyte4 *p_retCsrLength,
                                            const ubyte* pASN1Name, ubyte4 asn1NameLen);

MOC_EXTERN MSTATUS PKCS10_freeCSR(ubyte **ppCsrToFree);

/* inbound CSR response from a CA */
MOC_EXTERN MSTATUS PKCS10_decodeSignedCertificate(ubyte* pCertBase64, ubyte4 certLengthBase64, ubyte** ppCertificate, ubyte4 *pRetCertificateLen);
MOC_EXTERN MSTATUS PKCS10_freeDecodedSignedCertificate(ubyte** ppFreeCertificate);
#endif /* __ENABLE_MOCANA_PKCS10__ */

#ifdef __cplusplus
}
#endif

#endif /* __PKCS10_HEADER__ */
