/* Version: mss_v6_3 */
/*
 * pkcs7.h
 *
 * PKCS#7 Parser and utilities routines
 *
 * Copyright Mocana Corp 2005-2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PKCS7_HEADER__
#define __PKCS7_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------*/

/* PKCS7_signData flags */
#define PKCS7_EXTERNAL_SIGNATURES 0x01

/* type definitions */

typedef struct Attribute
{
    const ubyte* typeOID;
    ubyte4 type; /* id|tag */
    ubyte* value;
    ubyte4 valueLen;
} Attribute;

typedef struct signerInfo {
    ASN1_ITEMPTR pIssuer; /* signer certificate's issuer */
    ASN1_ITEMPTR pSerialNumber; /* signer certificate's issuer specific serial number */
    CStream cs; /* common stream for both issuer and serial number */
    AsymmetricKey* pKey; /* private key */
    const ubyte* digestAlgoOID; /* must point to one of the constants in oiddefs.h */
    const ubyte* unused;
    Attribute* pAuthAttrs;
    ubyte4 authAttrsLen;
    Attribute* pUnauthAttrs;
    ubyte4 unauthAttrsLen;
} signerInfo;

typedef struct signerInfo *signerInfoPtr;

/* this callback is used to retrieve the private key that */
/* corresponds to an issuer and serial number */
typedef MSTATUS (*PKCS7_GetPrivateKey)(CStream cs,
                                        ASN1_ITEM* pSerialNumber,
                                        ASN1_ITEM* pIssuerName,
                                        AsymmetricKey* pKey);

/* this callback is used to verify that this certificate is recognized
as valid */
typedef MSTATUS (*PKCS7_ValidateRootCertificate)(CStream cs,
                                                 ASN1_ITEM* pLeafCertificate,
                                                 ASN1_ITEM* pCertificate);

/* this callback is used to get a certificate given the issuer name and
serial number */
typedef MSTATUS (*PKCS7_GetCertificate)(MOC_RSA(hwAccelDescr hwAccelCtx)
                                        CStream cs,
                                        ASN1_ITEM* pSerialNumber,
                                        ASN1_ITEM* pIssuerName,
                                        ASN1_ITEMPTR* ppCertificate,
                                        CStream *pCertStream);

typedef struct PKCS7_Callbacks
{
    PKCS7_GetPrivateKey             getPrivKeyFun;
    PKCS7_ValidateRootCertificate   valCertFun;
    PKCS7_GetCertificate            getCertFun;
} PKCS7_Callbacks;

/*------------------------------------------------------------------*/
/* exported routines */
#ifdef __ENABLE_MOCANA_PKCS7__

/* this routine takes a pointer to the root item of a parsed PKCS7
    message (by ASN1_Parse) and returns the pointer to the first
    certificate in the message. If the PKCS7 contains several
    certificates, they are the siblings of the first one */
MOC_EXTERN MSTATUS PKCS7_GetCertificates(ASN1_ITEM* pRootItem,
                                        CStream s,
                                        ASN1_ITEM** ppFirstCertificate);
/* given a SET or SEQUENCE of certificates, will return
 the certificate with the matching Issuer and SerialNumber */
MOC_EXTERN MSTATUS PKCS7_FindCertificate( CStream s,
                                        ASN1_ITEMPTR pIssuer,
                                        ASN1_ITEMPTR pSerialNumber,
                                        CStream certificatesStream,
                                        ASN1_ITEMPTR pCertificates,
                                        ASN1_ITEMPTR* ppCertificate);

/* this routine verifies the signature on a PKCS7 signed data struct
it uses a user provided call back to verify the certificate chain
it extensively checks the structure and the signature for all signers
and returns the number of known signers (whose certificate could be
validated using the valCertFun call back */
MOC_EXTERN MSTATUS PKCS7_VerifySignedData(MOC_RSA_HASH(hwAccelDescr hwAccelCtx)  ASN1_ITEM* pSignedData,
                                        CStream s,
                                        PKCS7_GetCertificate getCertFun,
                                        PKCS7_ValidateRootCertificate valCertFun,
                                        sbyte4* numKnownSigners);

/* same as PKCS7_VerifySignedData routine, in addition, this routine can be used
to verify external signatures by passing in the content in payLoad */
MOC_EXTERN MSTATUS
PKCS7_VerifySignedDataEx(MOC_RSA_HASH(hwAccelDescr hwAccelCtx)  ASN1_ITEM* pSignedData, CStream s,
                       /* getCertFun can be NULL, if certificates
                        * are included in signedData
                       */
                        PKCS7_GetCertificate getCertFun,
                        PKCS7_ValidateRootCertificate valCertFun,
                        const ubyte* payLoad, /* for detached signatures */
                        ubyte4 payLoadLen,
                        sbyte4* numKnownSigners);

MOC_EXTERN MSTATUS PKCS7_DecryptEnvelopedDataAux( MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                                ASN1_ITEM* pEnvelopedData,
                                                CStream s,
                                                PKCS7_GetPrivateKey getPrivateKeyFun,
                                                encryptedContentType* pType,
                                                ASN1_ITEM** ppEncryptedContent,
                                                BulkCtx* ppBulkCtx,
                                                const BulkEncryptionAlgo** ppBulkAlgo,
                                                ubyte iv[/*16=MAX_IV_SIZE*/]);

MOC_EXTERN MSTATUS PKCS7_DecryptEnvelopedData( MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                            ASN1_ITEM* pEnvelopedData,
                                            CStream s,
                                            PKCS7_GetPrivateKey getPrivateKeyFun,
                                            ubyte** decryptedInfo,
                                            sbyte4* decryptedInfoLen);

MOC_EXTERN MSTATUS PKCS7_EnvelopData( MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                                    DER_ITEMPTR pStart, /* can be null */
                                    DER_ITEMPTR pParent, /* can be null */
                                    ASN1_ITEMPTR pCACertificates[/*numCACerts*/],
                                    CStream pStreams[/*numCACerts*/],
                                    sbyte4 numCACerts,
                                    const ubyte* encryptAlgoOID,
                                    RNGFun rngFun,
                                    void* rngFunArg,
                                    const ubyte* pPayLoad,
                                    ubyte4 payLoadLen,
                                    ubyte** ppEnveloped,
                                    ubyte4* pEnvelopedLen);

MOC_EXTERN MSTATUS
PKCS7_SignData( MOC_RSA_HASH(hwAccelDescr hwAccelCtx)
                DER_ITEMPTR pStart, /* can be null */
                DER_ITEMPTR pParent,
                ASN1_ITEMPTR pCACertificates[/*numCACerts*/], /* can be null */
                CStream pCAStreams[/*numCACerts*/],
                sbyte4 numCACerts,
                ASN1_ITEMPTR pCrls[/*numCrls*/], /* can be null */
                CStream pCrlStreams[/*numCrls*/],
                sbyte4 numCrls,
                signerInfoPtr *pSignerInfos, /* if NULL, will create degenerate SignedData */
                ubyte4 numSigners, /* number of signers */
                const ubyte* payLoadType, /* if NULL, will create degenerate SignedData */
                const ubyte* pPayLoad,
                ubyte4 payLoadLen,
                RNGFun rngFun,           /* this can be NULL for degenerate SignedData */
                void* rngFunArg,         /* this can be NULL for degenerate SignedData */
                ubyte** ppSigned,
                ubyte4* pSignedLen);

/* same as PKCS7_SignData, in addition, this routine can be used to
control how signedData structure is generated, by passing in the flags */
MOC_EXTERN MSTATUS
PKCS7_SignDataEx( MOC_RSA_HASH(hwAccelDescr hwAccelCtx)
                    ubyte4 flags,
                    DER_ITEMPTR pStart, /* can be null */
                    DER_ITEMPTR pParent,
                    ASN1_ITEMPTR pCACertificates[/*numCACerts*/], /* can be null */
                    CStream pCAStreams[/*numCACerts*/],
                    sbyte4 numCACerts,
                    ASN1_ITEMPTR pCrls[/*numCrls*/], /* can be null */
                    CStream pCrlStreams[/*numCrls*/],
                    sbyte4 numCrls,
                    signerInfoPtr *pSignerInfos, /* if NULL, will create degenerate SignedData */
                    ubyte4 numSigners, /* number of signers */
                    const ubyte* payLoadType, /* if NULL, will create degenerate SignedData */
                    const ubyte* pPayLoad,
                    ubyte4 payLoadLen,
                    RNGFun rngFun,             /* this can be NULL for degenerate SignedData */
                    void* rngFunArg,           /* this can be NULL for degenerate SignedData */
                    ubyte** ppSigned,
                    ubyte4* pSignedLen);

MOC_EXTERN MSTATUS
PKCS7_DigestData( MOC_HASH(hwAccelDescr hwAccelCtx)
                    DER_ITEMPTR pStart, /* can be null */
                    DER_ITEMPTR pParent,
                    const ubyte* payLoadType, /* OID can be null then will used pkcs7_data_OID */
                    ubyte hashType,
                    const ubyte* pPayLoad,
                    ubyte4 payLoadLen,
                    ubyte** ppDigested,
                    ubyte4* pDigestedLen);


/* return the hash used for this signer (i.e. ht_md5, ht_sha1, etc..) */
MOC_EXTERN MSTATUS
PKCS7_GetSignerDigestAlgo( ASN1_ITEMPTR pSignerInfo, CStream cs, ubyte* hashAlgoId);

/* return the hash used for this signer (i.e. akt_rsa, akt_ecc, etc..) */
MOC_EXTERN MSTATUS
PKCS7_GetSignerSignatureAlgo( ASN1_ITEMPTR pSignerInfo, CStream cs, ubyte* pubKeyAlgoId);

MOC_EXTERN MSTATUS
PKCS7_GetSignerSignedAttributes( ASN1_ITEMPTR pSignerInfo,
                        ASN1_ITEMPTR *ppFirstSignedAttribute);

MOC_EXTERN MSTATUS
PKCS7_GetSignerUnsignedAttributes( ASN1_ITEMPTR pSignerInfo,
                        ASN1_ITEMPTR *ppFirstUnsignedAttribute);


#endif  /*#ifdef __ENABLE_MOCANA_PKCS7__*/

#ifdef __cplusplus
}
#endif

#endif  /*#ifndef __PKCS7_HEADER__ */
