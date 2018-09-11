/* Version: mss_v6_3 */
/*
 * asn1cert.h
 *
 * ASN.1 Certificate Encoding Header
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __ASN1CERT_HEADER__
#define __ASN1CERT_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

MOC_EXTERN MSTATUS ASN1CERT_generateCertificate(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) AsymmetricKey *pCertKey,
                                            const certDistinguishedName *pSubjectInfo,
                                            AsymmetricKey *pSignKey,
                                            const certDistinguishedName *pIssuerInfo,
                                            ubyte signAlgo, const certExtensions* pExtensions,
                                            RNGFun rngFun, void* rngFunArg,
                                            ubyte **ppRetCertificate, ubyte4 *pRetCertLength);

/* identical to ASN1CERT_generateCertificate except the issuer info is already ASN1 encoded */
MOC_EXTERN MSTATUS ASN1CERT_generateLeafCertificate(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) AsymmetricKey *pCertKey,
                                            const certDistinguishedName *pSubjectInfo,
                                            AsymmetricKey *pSignKey, const ASN1_ITEM *pIssuerInfo,
                                            CStream cs, ubyte signAlgo, const certExtensions* pExtensions,
                                            RNGFun rngFun, void* rngFunArg,
                                            ubyte **ppRetCertificate, ubyte4 *pRetCertLength);

/* used by PKCS 10 */
MOC_EXTERN MSTATUS ASN1CERT_StoreDistinguishedName( DER_ITEMPTR pRoot,
                                               const certDistinguishedName* pCertInfo);

MOC_EXTERN MSTATUS ASN1CERT_storePublicKeyInfo( const AsymmetricKey* pPublicKey, DER_ITEMPTR pCertificate);

MOC_EXTERN MSTATUS ASN1CERT_sign(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) DER_ITEMPTR pSignedHead,
                                AsymmetricKey* pSignKey, const ubyte* signAlgoOID,
                                RNGFun rngFun, void* rngFunArg,
                                ubyte **ppRetDEREncoding, ubyte4 *pRetDEREncodingLen);
/* add certificate extensions */
MOC_EXTERN MSTATUS ASN1CERT_AddExtensions(DER_ITEMPTR pExtensionTag, const certExtensions* pExtensions,
                                      DER_ITEMPTR *ppExtsItem);

#if (defined(__ENABLE_MOCANA_ECC__))
MOC_EXTERN MSTATUS ASN1CERT_storeECCPublicKeyInfo( const ECCKey *pECCKey, DER_ITEMPTR pCertificate);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ASN1CERT_HEADER__ */

