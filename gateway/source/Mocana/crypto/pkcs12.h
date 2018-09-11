/* Version: mss_v6_3 */
/*
 * pkcs12.h
 *
 * PKCS#12 Parser
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PKCS12_HEADER__
#define __PKCS12_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_PKCS12__
/* public constants */
MOC_EXTERN const ubyte pkcs12_bagtypes_root_OID[]; /* 1.2.840.113549.1.12.10.1 */
MOC_EXTERN const ubyte pkcs12_Pbe_root_OID[]; /* 1.2.840.113549.1.12.1 */

typedef enum
{
    KEYINFO, CERT, CRL
} contentTypes;

typedef enum
{
#ifndef __DISABLE_MOCANA_PKCS12_X509_CERTTYPE_DEFINITION__
    X509 = 1,
#endif
    SDSI=2
} certTypes;

/*PKCS12 Encryption and Integrity Modes */
typedef enum ePKCS12Mode
{
    PKCS12Mode_Privacy_none = 0,
    PKCS12Mode_Privacy_data,
    PKCS12Mode_Privacy_password,
    PKCS12Mode_Privacy_pubKey,
    PKCS12Mode_Integrity_password,
    PKCS12Mode_Integrity_pubKey
} ePKCS12Mode;

/* enum for pkcs12 attribute */
typedef enum ePKCS12AttributeType
{
    PKCS12_AttributeType_friendlyName = 0,
    PKCS12_AttributeType_localKeyId
} ePKCS12AttributeType;

/* User configuration related structures */

/*
 * PKCS12AttributeUserValue
 * Section 4.2 PKCS12AttrSet : If the user desires to assign nicknames and identifiers to keys etc
 */
typedef struct PKCS12AttributeUserValue
{
    ePKCS12AttributeType             eAttrType;    /* One of the above mentioned Attribute type */
    ubyte*                           pValue;       /* Holds the Value */
    ubyte4                           valueLen;     /* length of Value */
} PKCS12AttributeUserValue;

/*
 * PKCS12DataObject
 * Contents that need to be published within the PKCS#12 file along with the privacy mode
 */
typedef struct PKCS12DataObject
{
    ePKCS12Mode                  privacyMode;          /* Privacy Mode: Indicates how the user wants to encrypt the data given below.
                                                        * for PKCS12Mode_Privacy_none: In this case the mode would default to PKCS12Mode_Privacy_password
                                                        */
    ubyte4                       encKeyType;           /* enum PKCS8EncryptionType : PKCS#8 Encrytion Key type */
    const ubyte*                 pKeyPassword;         /* Password for Key encryption
                                                        * Option 1: if NULL then PKCS#8PrivateKeyInfo is used
                                                        * Option 2: if set then PKCS#8ShroudedKey is used.
                                                        */
    ubyte4                       keyPasswordLen;       /* Indicates the length of the password in bytes */
    AsymmetricKey*               pPrivateKey;          /* Private key that needs to be published in PKCS#12 */
    certTypes                    eCertType;            /* Type of certificate from certTypes enum */
    ubyte*                       pCertificate;         /* DER formatted certificate file to be published in PKCS#12 */
    ubyte4                       certificateLen;       /* Length of certificate file */
    ubyte*                       pCrl;                 /* Stream that hold the crl to be published in PKCS#12 */
    ubyte4                       crlLen;               /* Length of CRL format */
    PKCS12AttributeUserValue**   ppPKCS12AttrValue;    /* Stores 1 or more instances, if no parameters need to passed assign NULL / 0 */
    ubyte4                       numPKCS12AttrValue;   /* Indicates number of PKCS12AtttributeUserValue instance/s, 0 if none */
} PKCS12DataObject;

/*
 * PKCS12PrivacyModeConfig
 */
typedef struct PKCS12PrivacyModeConfig
{
    /* Privacy Mode : Password */
    const ubyte*                   pPrivacyPassword;             /* Password for privacy mode, if its NULL/0 then password from Integrity password mode is used */
    ubyte4                         privacyPasswordLen;           /* Length of the password in bytes */
    ubyte4                         pkcs12EncryptionType;
    /* Privacy Mode : Public Key */
    const ubyte*                   pEncryptionAlgoOID;
    CStream**                      ppCSDestPubKeyStream;         /* public key stream/s*/
    ubyte4                         numPubKeyStream;              /* number of public key stream/s */
} PKCS12PrivacyModeConfig;

/* End of User configuration realted structures */

/*
 * when type is KEYINFO, content contains the DER encoded PKCS#8 PrivateKeyInfo;
 * when type is CERT, and extraInfo is X509, content contains the DER encoded X.509 certificate;
 * when type is CERT, and extraInfo is SDSI, content contains the BASE64 encoded SDSI certificate;
 * when type is CRL, and extraInfo is X509, content contains the DER encoded X.509 CRL.
*/
typedef MSTATUS (*PKCS12_contentHandler)( contentTypes type, ubyte4 extraInfo, const ubyte* content, ubyte4 contentLen);

typedef MSTATUS (*PKCS12_contentHandlerEx)(void* context, contentTypes type, 
                                           ubyte4 extraInfo, 
                                           const ubyte* content, 
                                           ubyte4 contentLen);

/*------------------------------------------------------------------*/

/* these routines take a pointer to the root item of a parsed PKCS12
    message (by ASN1_Parse) */
MOC_EXTERN MSTATUS PKCS12_ExtractInfo(MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                                        ASN1_ITEM* pRootItem,
                                        CStream s,
                                        const ubyte* uniPassword,
                                        sbyte4 uniPassLen,
                                        PKCS7_Callbacks* pkcs7CBs,
                                        PKCS12_contentHandler handler);
    
MOC_EXTERN MSTATUS PKCS12_ExtractInfoEx(MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                                          ASN1_ITEM* pRootItem,
                                          CStream s,
                                          const ubyte* uniPassword,
                                          sbyte4 uniPassLen,
                                          PKCS7_Callbacks* pkcs7CBs,
                                          PKCS12_contentHandlerEx handler,
                                          void* handlerContext);
    

/* NOTE for PKCS12_decrypt/PKCS12_encrypt: password argument can be unicode or not */

MOC_EXTERN MSTATUS PKCS12_decrypt(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
               ASN1_ITEMPTR pEncryptedData,
               ASN1_ITEMPTR pAlgoIdentifier,
               CStream s, const ubyte* password,
               sbyte4 passwordLen,
               ubyte** decryptedInfo,
               sbyte4* decryptedInfoLen);

MOC_EXTERN MSTATUS PKCS12_encrypt(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
               ubyte pbeSubType,
               const ubyte* password, sbyte4 passwordLen,
               const ubyte* salt, sbyte4 saltLen, ubyte4 iterCount,
               ubyte* plainText, sbyte4 plainTextLen);

MOC_EXTERN const BulkEncryptionAlgo* PKCS12_GetEncryptionAlgo( ubyte pbeSubType);

MOC_EXTERN MSTATUS
PKCS12_EncryptPFXPdu(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                     randomContext* pRandomContext,
                     ubyte4 integrityMode,
                     /* Password Integrity Mode */
                     const ubyte* pIntegrityPswd,
                     ubyte4 integrityPswdLen,
                     /* Pub Key Integrity Mode */
                     AsymmetricKey* pVsrcSigK,
                     const ubyte* pDigestAlgoOID,
                     CStream csSignerCertificate[],
                     ubyte4  numSignerCerts,
                     /* PKCS Privacy Mode Configuration and Data */
                     const PKCS12PrivacyModeConfig *pPkcs12PrivacyModeConfig,
                     /* Data to be encrypted */
                     PKCS12DataObject pkcs12DataObject[/*numPKCS12DataObj*/],
                     ubyte4 numPKCS12DataObj,
                     /* return PKCS#12 certificate */
                     ubyte** ppRetPkcs12CertDer, ubyte4* pRetPkcs12CertDerLen);

#endif /* __ENABLE_MOCANA_PKCS12__ */


#ifdef __cplusplus
}
#endif

#endif  /*#ifndef __PKCS12_HEADER__ */
