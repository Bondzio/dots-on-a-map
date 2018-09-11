/* Version: mss_v6_3 */
/*
 * pkcs.h
 *
 * PKCS routines
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PKCS_HEADER__
#define __PKCS_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------*/

/* exported routines */

#ifdef __ENABLE_MOCANA_PKCS7__

/* This API returns an DER encoded PKCS7 message that contains the
payload enveloped using the provided certificate. This is just a
high level wrapper, with less flexibility of PKCS7_EnvelopData */
MOC_EXTERN MSTATUS PKCS7_EnvelopWithCertificate( const ubyte* cert,
                                    ubyte4 certLen,
                                    const ubyte* encryptAlgoOID,
                                    const ubyte* pPayLoad,
                                    ubyte4 payLoadLen,
                                    ubyte** ppEnveloped,
                                    ubyte4* pEnvelopedLen);

/* same but enveloped for several recipients described by their
certificate */
MOC_EXTERN MSTATUS PKCS7_EnvelopWithCertificates( ubyte4 numCerts,
                                    const ubyte* certs[/*numCerts*/],
                                    ubyte4 certLens[/*numCerts*/],
                                    const ubyte* encryptAlgoOID,
                                    const ubyte* pPayLoad,
                                    ubyte4 payLoadLen,
                                    ubyte** ppEnveloped,
                                    ubyte4* pEnvelopedLen);

/* This API decrypts the Enveloped Data part of a PKCS7 message
This is a high level wrapper for PKCS7_DecryptEnvelopedData */
MOC_EXTERN MSTATUS PKCS7_DecryptEnvelopedDataPart( const ubyte* pkcs7Msg,
                                            ubyte4 pkcs7MsgLen,
                                            PKCS7_GetPrivateKey getPrivateKeyFun,
                                            ubyte** decryptedInfo,
                                            sbyte4* decryptedInfoLen);

MOC_EXTERN MSTATUS
PKCS7_SignWithCertificateAndKeyBlob( const ubyte* cert,
                                    ubyte4 certLen,
                                    const ubyte* keyBlob,
                                    ubyte4 keyBlobLen,
                                    ASN1_ITEMPTR pCACertificates[/*numCACerts*/],
                                    CStream pCAStreams[/*numCACerts*/],
                                    sbyte4 numCACerts,
                                    ASN1_ITEMPTR pCrls[/*numCrls*/],
                                    CStream pCrlStreams[/*numCrls*/],
                                    sbyte4 numCrls,
                                    const ubyte* digestAlgoOID,
                                    const ubyte* payLoadType,
                                    ubyte* pPayLoad, /* removed const to get rid of compiler warning */
                                    ubyte4 payLoadLen,
                                    Attribute* pAuthAttrs,
                                    ubyte4 authAttrsLen,
                                    RNGFun rngFun,
                                    void* rngFunArg,
                                    ubyte** ppSigned,
                                    ubyte4* pSignedLen);

#endif /*__ENABLE_MOCANA_PKCS7__ */

#ifdef __cplusplus
}
#endif


#endif  /*#ifndef __PKCS7_HEADER__ */
