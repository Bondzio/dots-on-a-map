/* Version: mss_v6_3 */
/*
 * cms.h
 *
 * CMS Parser and utilities routines
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __CMS_HEADER__
#define __CMS_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


/* Content type of a received CMS cf. CMS_getContentType() */
typedef enum CMS_ContentType
{
    E_CMS_undetermined = 0,
    E_CMS_data = 1,
    E_CMS_signedData = 2,
    E_CMS_envelopedData = 3,
    /* E_PCKS7S_signedAndEnvelopedData = 4, */
    E_CMS_digestedData = 5,
    E_CMS_encryptedData = 6,
    E_CMS_ct_authData = 102,
} CMS_ContentType;

typedef void* CMS_context;  /* opaque structure used when parsing a CMS */

/* opaque structures used when creating a CMS */
typedef void* CMS_signedDataContext;
typedef void* CMS_signerInfo;
typedef void* CMS_envelopedDataContext;


#define NO_TAG (0xFFFFFFFF)

/* data structures used in callbacks */
typedef struct CMSIssuerSerialNumber
{
    ASN1_ITEMPTR pIssuer;
    ASN1_ITEMPTR pSerialNumber;
} CMSIssuerSerialNumber;

typedef struct CMSKeyTransRecipientId
{
    ubyte4 type;
    union
    {
        CMSIssuerSerialNumber issuerAndSerialNumber; /* type = NO_TAG */
        ASN1_ITEMPTR          subjectKeyIdentifier;  /* type = 0 OCTETSTRING */
    } u;
} CMSKeyTransRecipientId;


typedef struct CMSOriginatorPublicKey
{
    ASN1_ITEMPTR pAlgoOID;  /* AlgorithmIdentifier: algorithm OID */
    ASN1_ITEMPTR pAlgoParameters; /* AlgorithmIdentifier: parameters ANY */
    ASN1_ITEMPTR pPublicKey; /* BIT STRING */
} CMSOriginatorPublicKey;

typedef struct CMSKeyAgreeRecipientId
{
    ubyte4 type;
    union
    {
        CMSIssuerSerialNumber   issuerAndSerialNumber;  /* type = NO_TAG */
        ASN1_ITEMPTR            subjectKeyIdentifier;   /* type = 0 OCTETSTRING */
        CMSOriginatorPublicKey  originatorKey;          /* type = 1 */
    } u;
} CMSKeyAgreeRecipientId;

#if 0  /* this is not supported yet */
typedef struct CMSKEKRecipientId
{
    ASN1_ITEMPTR keyIdentifer;
    ASN1_ITEMPTR date;          /* can be NULL */
    ASN1_ITEMPTR other;         /* can be NULL */
} CMSKEKRecipientId;
#endif

/* data structure used in the CMS_GetPrivateKey callback. The callback implementer
should use the content of this structure to determine which key is requested */
typedef struct CMSRecipientId
{
    ubyte4 type;
    union
    {
        CMSKeyTransRecipientId    ktrid;   /* type = NO_TAG */
        CMSKeyAgreeRecipientId    karid;   /* type = 1 */
#if 0
        CMSKEKRecipientId         kekrid;  /* type = 2 */
        CMSPasswordRecipientId    pwrdi;   /* type = 3 */
        CMSOtherRecipientId       orid;    /* type = 4 */
#endif
    } ri;
} CMSRecipientId;

/* this callback is used to retrieve the private key that */
/* corresponds to a CMSRecipientId */
typedef MSTATUS (*CMS_GetPrivateKey)(CStream cs,
                                        const CMSRecipientId* pRecipientId,
                                        AsymmetricKey* pKey);

/* this callback is used to verify that this certificate is recognized
as valid */
typedef MSTATUS (*CMS_ValidateRootCertificate)(CStream cs,
                                                 ASN1_ITEM* pLeafCertificate,
                                                 ASN1_ITEM* pCertificate);

/* this callback is used to get a certificate given the issuer name and
serial number */
typedef MSTATUS (*CMS_GetCertificate)(CStream cs,
                                        ASN1_ITEM* pSerialNumber,
                                        ASN1_ITEM* pIssuerName,
                                        ASN1_ITEMPTR* ppCertificate,
                                        CStream *pCertStream);

/* all the callbacks that the CMS parser might need. */
typedef struct CMS_Callbacks
{
    CMS_GetPrivateKey             getPrivKeyFun;
    CMS_ValidateRootCertificate   valCertFun;
    CMS_GetCertificate            getCertFun;

} CMS_Callbacks;

/* create a new context to parse a CMS */
MOC_EXTERN MSTATUS CMS_newContext(CMS_context* pNewContext,
                                  const CMS_Callbacks* pCallbacks);

/* add data to the context -- the decrypted data (if any) is returned in newly allocated buffers output --
 if the logical end of the processing has been reached (i.e. no more data is necessary), *pFinished is not FALSE */
MOC_EXTERN MSTATUS CMS_updateContext( CMS_context context, const ubyte* input,
                                        ubyte4 inputLen, ubyte** output,
                                        ubyte4* pOutputLen, intBoolean* pFinished);

/* delete the context */
MOC_EXTERN MSTATUS CMS_deleteContext( CMS_context* pContext);

/*======== other functions to query the context for more information =======*/

/* These functions can return OK (success), ERR_EOF (more data must be provided by calling
CMS_updateContext) or some other error message (invalid data) */

/* return the content type cf.CMS_ContentType */
MOC_EXTERN MSTATUS CMS_getContentType( CMS_context context, CMS_ContentType* cmsContentType);

/* return the OID (with length prefix) for the encapsulated content type - the buffer is
newly allocated and must be freed */
MOC_EXTERN MSTATUS CMS_getEncapContentType( CMS_context context, ubyte** ppOID);

/********* EnvelopedData recipients **********/

/* return the number of recipients -- if they have all not all been seen
it will return ERR_EOF */
MOC_EXTERN MSTATUS CMS_getNumRecipients( CMS_context context,
                                          sbyte4* numRecipients);

/* return the recipient -- if they have all not all been seen
it will return ERR_EOF */
MOC_EXTERN MSTATUS CMS_getRecipientInfo( CMS_context context,
                                          sbyte4 recipientIndexZeroBased,
                                          const ASN1_ITEM** pRecipientInfo,
                                          CStream* pCS);

/* will return the index of the decrypting recipient -- can return ERR_EOF
or some other error */
MOC_EXTERN MSTATUS CMS_getDecryptingRecipient( CMS_context context,
                                          sbyte4* recipientIndexZeroBased);

/* return the OID (with length prefix) for the encryption algo- the buffer is
newly allocated and must be freed */
MOC_EXTERN MSTATUS CMS_getEncryptionAlgo( CMS_context context,
                                         ubyte** ppEncryptionAlgoOID);

/******* SignedData signers **********/

/* return the number of signers that could be verified. If the data was detached
and was not provided by using CMS_setDetachedSignatureData, it will return the error
ERR_PKCS7_DETACHED_DATA. */
MOC_EXTERN MSTATUS CMS_getNumSigners( CMS_context context,
                                        sbyte4* numSigners);

/* access the signers that could be verified [0..numSigners-1] */
MOC_EXTERN MSTATUS CMS_getSignerInfo( CMS_context context,
                                      sbyte4 signerIndexZeroBased,
                                      const ASN1_ITEM** pSignerInfo,
                                      CStream* pCS);

/* Extract receipt info
This will be called after the whole signedData has been parsed and signature is verified;
The encapsulated Content Type (cf. CMS_getEncapContentType()) is id-ct-receipt
The signed data (built by accretion of all the CMS_updateContext's returned buffers)is the receipt.
The pointers returned point to data inside the receipt buffer sent as input: they should not be freed */
MOC_EXTERN MSTATUS CMS_getReceiptInfo( const ubyte* receipt, ubyte4 receiptLen,
                                        const ubyte** messageId, ubyte4* messageIdLen,
                                        const ubyte** signature, ubyte4* signatureLen);

/* returne the message digest in the receipt */
MOC_EXTERN MSTATUS CMS_getReceiptMsgDigest( CMS_context context,
                                           const ubyte** digest, ubyte4* digestLen);

/* certificates */
MOC_EXTERN MSTATUS CMS_getFirstCertificate( CMS_context context,
                                      const ASN1_ITEM** ppCertificate,
                                      CStream* pCS);
/* to get the next ones -- just use ASN1_NEXT_SIBLING */

/* Does this CMS represent a detached signature? If yes, the data was not part of the
CMS and must be provided by using CMS_setDetachedSignatureData before the signature can
be verified. */
MOC_EXTERN MSTATUS CMS_detachedSignature(CMS_context context, intBoolean* detached);

/* use this function to provide the data to verify the signature if this was a detached signature */
MOC_EXTERN MSTATUS CMS_setDetachedSignatureData( MOC_HASH(hwAccelDescr hwAccelCtx) CMS_context context, const ubyte* payload,
                                                ubyte4 payloadLen, intBoolean done);

/* generate a signed receipt for the message and the signer */
MOC_EXTERN MSTATUS CMS_createSignedReceipt( CMS_context context,
                                            sbyte4 signerIndexZeroBased,
                                            RNGFun rngFun, void* rngFunArg,
                                            const ubyte* signerCert, ubyte4 signerCertLen,
                                            const AsymmetricKey* pKey, const ubyte* hashAlgoOID,
                                            ubyte** ppReceipt, ubyte4* pReceiptLen);

/************ CREATING CMS API **************************************/
/* 2 distinct APIs are provided for creating CMS, one for signed data and the other
for enveloped data */
/************************* creating signed data */
MOC_EXTERN MSTATUS CMS_signedNewContext( CMS_signedDataContext* pNewCtx,
                                           const ubyte* payloadTypeOID,
                                           intBoolean detached, RNGFun rngFun,
                                           void* rngFunArg);
/* add a certificate to the signed CMS -- cf. also CMS_addSigner -- this can be used to add
an intermediate certificate */
MOC_EXTERN MSTATUS CMS_signedAddCertificate( CMS_signedDataContext myCtx, const ubyte* cert,
                                        ubyte4 certLen);

/* add a CRL to the signed CMS */
MOC_EXTERN MSTATUS CMS_signedAddCRL( CMS_signedDataContext myCtx, const ubyte* crl,
                                        ubyte4 crlLen);

/* flags in CMS_signedAddSigner are a combination of the following values */
enum {
    e_cms_signer_addCert = 0x0001,       /* add the certificate to the CMS */
    e_cms_signer_forceAuthAttr = 0x0002  /* this signer wants to add some authenticated attributes */
};

/* add a signer to the CMS */
MOC_EXTERN MSTATUS CMS_signedAddSigner( CMS_signedDataContext myCtx,
                                        const ubyte* cert,
                                        ubyte4 certLen,
                                        const AsymmetricKey* pKey,
                                        const ubyte* digestAlgoOID,
                                        ubyte4 flags,
                                        CMS_signerInfo* pNewSignerInfo);

/* add an authenticated attribute for this signer. If the signerInfo is 0 (NULL),
the authenticated attribute is added to all signers */
MOC_EXTERN MSTATUS CMS_signedAddSignerAttribute( CMS_signedDataContext myCtx,
                                            CMS_signerInfo signerInfo,
                                            const ubyte* typeOID,
                                            ubyte4 type, /* id|tag */
                                            const ubyte* value,
                                            ubyte4 valueLen,
                                            intBoolean authenticated);

/* Request a receipt for this message */
MOC_EXTERN MSTATUS CMS_signedAddReceiptRequest( CMS_signedDataContext myCtx,
                        const ubyte** receiptFrom,  /* Array of recipient email addresses from which receipts are requested*/
                        sbyte4 numReceiptFrom,      /* -1 for all, 0 for not on mailing list or > 0 to use the receiptFrom arg */
                        const ubyte** receiptTo,    /* Array of email addresses that receipts are to be sent to */
                        sbyte4 numReceiptTo);

/* Extract receipt request info. This returns the information that should be saved to process the
receipt when it arrives.
This function should only be called after request was created by CMS_signedAddReceiptRequest
and after the last CMS_signedUpdateContext (finished arg was not FALSE).
The pointers returned point to data managed by the CMS_signedDataContext:
they should not be freed, and will become invalid if the CMS_signedDataContext is deleted */
MOC_EXTERN MSTATUS CMS_signedGetRequestInfo( CMS_signedDataContext myCtx,
                                        CMS_signerInfo signerInfo,
                                        const ubyte** messageId, ubyte4* messageIdLen,
                                        const ubyte** digest, ubyte4* digestLen,
                                        const ubyte** signature, ubyte4* signatureLen);

/* Update the context with new data
Note that we need the output ASAP in streaming mode. Set the finished flag to not FALSE
to indicate this was the last data and that the CMS can be completely generated */
MOC_EXTERN MSTATUS CMS_signedUpdateContext( CMS_signedDataContext myCtx,
                                           const ubyte* data, ubyte4 dataLen,
                                           ubyte** output, ubyte4* pOutputLen,
                                           intBoolean finished);

/* free the context for creating signed data */
MOC_EXTERN MSTATUS CMS_signedDeleteContext(CMS_signedDataContext* pNewCtx);


/************************************** creating enveloped data */
MOC_EXTERN MSTATUS CMS_envelopedNewContext( CMS_envelopedDataContext* pNewCtx,
                                                  const ubyte* encryptAlgoOID,
                                                  RNGFun rngFun, void* rngFunArg);


/* This actually adds a recipient with his encryption cert */
MOC_EXTERN MSTATUS CMS_envelopedAddRecipient( CMS_envelopedDataContext myCtx,
                                              const ubyte* cert, ubyte4 certLen);


/* Adds an unauthentifcate attribute */
MOC_EXTERN MSTATUS CMS_envelopedAddUnauthAttribute( CMS_envelopedDataContext myCtx,
                                            const ubyte* typeOID,
                                            ubyte4 type, /* id|tag */
                                            const ubyte* value,
                                            ubyte4 valueLen);

/* Update the context with new data. cf CMS_signedUpdateContext */
/* Note that we need the output ASAP in streaming mode */
MOC_EXTERN MSTATUS CMS_envelopedUpdateContext( CMS_envelopedDataContext myCtx,
                                              const ubyte* data, ubyte4 dataLen,
                                              ubyte** output, ubyte4* pOutputLen,
                                              intBoolean finished);

/* Release all memory associated with the context to create Enveloped Data */
MOC_EXTERN MSTATUS CMS_envelopedDeleteContext(CMS_envelopedDataContext* pNewCtx);


#ifdef __cplusplus
}
#endif

#endif  /*#ifndef __CMS_HEADER__ */
