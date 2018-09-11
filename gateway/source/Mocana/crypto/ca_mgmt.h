/* Version: mss_v6_3 */
/*
 * ca_mgmt.h
 *
 * Certificate Authority Management Factory
 *
 * Copyright Mocana Corp 2003-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file ca_mgmt.h Certificate authority management factory.
This header file contains structures, enumerations, and function declarations
used for certificate management.

\since 1.41
\version 5.3 and later

! Flags
Whether the following flags are defined determines which structures and
enumerations are defined:
- $__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__$
- $__ENABLE_MOCANA_ECC__$

Whether the following flags are defined determines which function declarations are enabled:
- $__ENABLE_MOCANA_EXTRACT_CERT_BLOB__$
- $__PUBCRYPTO_HEADER__$

*/


/*------------------------------------------------------------------*/

#ifndef __CA_MGMT_HEADER__
#define __CA_MGMT_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------*/

/* these values are serialized -- add but don't modify */
/* valid values for the AsymmetricKey type field */
enum
{
    akt_undefined = 0, /* keep it 0 -> static var are correctly initialized */
                       /* as undefined */
    akt_rsa = 1,
    akt_ecc = 2,
    akt_dsa = 3,
#ifdef __ENABLE_MOCANA_HW_SECURITY_MODULE__

#ifdef __ENABLE_MOCANA_TPM__
    akt_rsa_tpm = 17,  /*0x11*/
#endif

#endif
};

#if (defined(__ENABLE_MOCANA_ECC__))
/* curveId: they actually match the suffix of the OID for these curves */
enum
{
    cid_EC_P192 = 1,
    cid_EC_P256 = 7,
    cid_EC_P224 = 33,
    cid_EC_P384 = 34,
    cid_EC_P521 = 35
};
#endif

/*------------------------------------------------------------------*/

struct AsymmetricKey;

/* Certificate context.
This structure encapsulates the required context of a
certificate. An entity using a certificate for authentication must have access
to the corresponding public and private key pair used to sign the certificate.
Mocana provides API functions for generating and importing certificates; those
API fucntions use the following structure for passing certificate information.

\since 1.41
\version 1.41 and later

*/
typedef struct certDescriptor
{
    /* Pointer to certificate.
    Pointer to certificate.
    */
    ubyte*  pCertificate;

    /* Number of bytes in $pCertificate$.
    Number of bytes in $pCertificate$.
    */
    ubyte4  certLength;

    /* Pointer to key blob value.
    Pointer to key blob value.
    */
    ubyte*  pKeyBlob;

    /* Number of bytes in $pKeyBlob$.
    Number of bytes in $pKeyBlob$
    */
    ubyte4  keyBlobLength;

    /* if defined, use this instead of pKeyBlob */
    struct AsymmetricKey* pKey;

#if !(defined __ENABLE_MOCANA_64_BIT__)
    /* Application-specific %cookie.
    Application-specific %cookie.

    \note If the $__ENABLE_MOCANA_64_BIT__$ flag is defined, only the $ubyte4
    %cookie$ (not the $ubyte8 %cookie$) is included in the structure. If the flag
    is not defined$, only the $ubyte8 %cookie$ (not the $ubyte4 %cookie$) is
    included.
    */
    ubyte4  cookie;
#else
    /* Application-specific %cookie.
    Application-specific %cookie.

    \note If the $__ENABLE_MOCANA_64_BIT__$ flag is defined, only the $ubyte4
    %cookie$ (not the $ubyte8 %cookie$) is included in the structure. If the flag
    is not defined$, only the $ubyte8 %cookie$ (not the $ubyte4 %cookie$) is
    included.
    */
    ubyte8  cookie;
#endif
} certDescriptor;

/* old structure */

/* Certificate generation support.
This structure is provided for backward compatability to
support earlier Mocana certificate generation and authentication functions.
*/
typedef struct nameAttr
{
    /* Attribute's OID (object identifier).
    Attributes's OID (object identifier).
    */
    const ubyte *oid; /* the OID of the attribute */
    /* (Optional) Choice, such as UTF8String.
    Choice, such as UTF8String.
    */
    ubyte type; /* optional, if included, it indicates one of the choices such as UTF8String rather than PrintableString */
    /* Name %value buffer.
    Name %value buffer.
    */
    ubyte* value;
    /* Number of bytes in the name value buffer ($value$).
    Number of bytes in the name value buffer ($value$).
    */
    ubyte4 valueLen;
} nameAttr;

typedef struct relativeDN /* RDN */
{
    /* ! Array of nameAttr of length dnCount.
    This field identifies AttributeTypeAndValues that make up a RDN field.
    */
    nameAttr *pNameAttr;
    ubyte4   nameAttrCount;
} relativeDN;

/* Distinguished name data to support certificate generation.
This structure contains a list of relative distinguished names for a given
certificate. The order relfelcts the sequence of RDN.

\note Internet Explorer limits certificate lifetimes to 30 years.
*/
typedef struct certDistinguishedName
{
    /* Relative distinguished name.
    Relative distinguished name.
    */
    relativeDN *pDistinguishedName;
    /* Number of relative distinguished names for the certificate.
    Number of relative distinguished names for the certificate.
    */
    ubyte4      dnCount;

    /* String identifying certificate's start date, in the format yymmddhhmmss; for example, "030526000126Z" specifies May 26th, 2003 12:01:26 AM.
    String identifying certificate's start date, in the format yymmddhhmmss; for
    example, "030526000126Z" specifies May 26th, 2003 12:01:26 AM.
    */
    sbyte*          pStartDate;                 /* 030526000126Z */

    /* String identifying certificate's end date, in the format yymmddhhmmss; for example, "330524230347Z" specifies May 24th, 2033 11:03:47 PM.
    String identifying certificate's end date, in the format yymmddhhmmss; for
    example, "330524230347Z" specifies May 24th, 2033 11:03:47 PM.
    */
    sbyte*          pEndDate;                   /* 330524230347Z */

} certDistinguishedName;

/* Version 3 certificate or CRL extension (as defined in RFC&nbsp;3280).
This structure is used to specify a version 3 certificate or CRL extension
(as defined in RFC&nbsp;3280).

\since 3.06
\version 3.06 and later

*/
typedef struct extensions
{
    /* Extension Id: an OID defined in oiddefs.h.
    Extension Id: an OID defined in oiddefs.h.
    */
    ubyte* oid;

    /* $TRUE$ if extension is critical; otherwise $FALSE$.
    $TRUE$ if extension is critical; otherwise $FALSE$.
    */
    byteBoolean isCritical;

    /* DER-encoded extension %value.
    DER-encoded extension %value.
    */
    ubyte* value;

    /* Number of bytes in the DER-encoded extension %value ($value$).
    Number of bytes in the DER-encoded extension %value ($value$).
    */
    ubyte4 valueLen;
} extensions;

/* Container for a certificate's version 3 %extensions.
This structure specifies a certificate's version 3 %extensions. For more
information, refer to RFC&nbsp;3280
(ftp://ftp.rfc-editor.org/in-notes/pdfrfc/rfc3280.txt.pdf).

\since 1.41
\version 3.06 and later
*/
typedef struct certExtensions
{
    /* $TRUE$ specifies that the certificate contains a basicConstraints extension; $FALSE$ otherwise.
    $TRUE$ specifies that the certificate contains a basicConstraints extension;
    $FALSE$ otherwise.
    */
    byteBoolean    hasBasicConstraints;

    /* $TRUE$ specifies that the basicConstraints is a CA value; $FALSE$ otherwise.
    $TRUE$ specifies that the basicConstraints is a CA value; $FALSE$
    otherwise.
    */
    byteBoolean    isCA;

    /* Number of certificates in the certificate chain; if negative, it's omitted from the basicConstraints. (This field corresponds to the $pathLenConstraint$ referenced in RFC&nbsp;3280.
    Number of certificates in the certificate chain; if negative, it's omitted
    from the basicConstraints. (This field corresponds to the
    $pathLenConstraint$ referenced in RFC&nbsp;3280.
    */
    sbyte          certPathLen; /* if negative omit this */

    /* $TRUE$ specifies that the certificate contains a $keyUsage$ extension; $FALSE$ otherwise.
    $TRUE$ specifies that the certificate contains a $keyUsage$ extension;
    $FALSE$ otherwise.
    */
    /* key usage */
    byteBoolean    hasKeyUsage;

    /* Bit string representing the desired version 3 certificate extensions; click $keyUsage$ for details about setting this value.
    $
    %keyUsage ::= BIT STRING \{\n
    &nbsp;&nbsp;digitalSignature(0), nonRepudiation(1), keyEncipherment(2),\n
    &nbsp;&nbsp;dataEncipherment(3), keyAgreement(4), keyCertSign(5), cRLSign(6),\n
    &nbsp;&nbsp;encipherOnly(7), decipherOnly(8)\}$

    For example, to set the key usage extension to "digital signature, certificate
    signing, CRL signing," use the following code:

    $%keyUsage = (1 << 0) + (1 << 5) + (1 << 6)$
    */
    ubyte2         keyUsage;

    /* Pointer to array of version 3 %extensions.
    Pointer to array of version 3 %extensions.
    */
    extensions *otherExts;

    /* Number of %extensions in the extensions array.
    Number of %extensions in the extensions array.
    */
    ubyte4      otherExtCount;
} certExtensions;

enum matchFlag
{
    matchFlagSuffix = 0x01,     /* match only the last part "server1.acme.com" matches "acme.com" */
    noWildcardMatch = 0x02,     /* name is not following rules... */
    matchFlagNoWildcard = 0x02,
    matchFlagDotSuffix = 0x04
    /* others tbd */
};


/*
\exclude
*/
typedef struct CNMatchInfo
{
    ubyte4          flags;
    const sbyte*    name;
} CNMatchInfo;



/*------------------------------------------------------------------*/

/* common server (certificate & key related methods) */
/* signAlgo is now the last digit of the PKCS1 OID ex: md5withRSAEncryption */
/* more complex versions of these -- specify extensions and parent certificate */
MOC_EXTERN sbyte4 CA_MGMT_generateCertificateEx( certDescriptor *pRetCertificate, ubyte4 keySize,
                                            const certDistinguishedName *pCertInfo, ubyte signAlgorithm,
                                            const certExtensions* pExtensions,
                                            const certDescriptor* pParentCertificate);

MOC_EXTERN sbyte4 CA_MGMT_generateCertificateEx2( certDescriptor *pRetCertificate, struct AsymmetricKey* key,
const certDistinguishedName *pCertInfo, ubyte signAlgorithm);


MOC_EXTERN sbyte4  CA_MGMT_freeCertificate(certDescriptor *pRetCertificateDescr);

#if 0
MOC_EXTERN sbyte4  CA_MGMT_returnPublicKey(certDescriptor *pCertificateDescr, ubyte **ppRetPublicKey, ubyte4 *pRetPublicKeyLength);
MOC_EXTERN sbyte4  CA_MGMT_returnPublicKeyBitLength(certDescriptor *pCertificateDescr, ubyte4 *pRetPublicKeyLengthInBits);
MOC_EXTERN sbyte4  CA_MGMT_freePublicKey(ubyte **ppRetPublicKey);
#endif

MOC_EXTERN sbyte4  CA_MGMT_allocCertDistinguishedName(certDistinguishedName **ppNewCertDistName);
MOC_EXTERN sbyte4  CA_MGMT_extractCertDistinguishedName(ubyte *pCertificate, ubyte4 certificateLength, sbyte4 isSubject, certDistinguishedName *pRetDN);
MOC_EXTERN sbyte4  CA_MGMT_compareCertDistinguishedName(certDistinguishedName *pName1, certDistinguishedName *pName2);
MOC_EXTERN sbyte4  CA_MGMT_returnCertificatePrints(ubyte *pCertificate, ubyte4 certLength, ubyte *pShaFingerPrint, ubyte *pMD5FingerPrint);
MOC_EXTERN sbyte4  CA_MGMT_freeCertDistinguishedName(certDistinguishedName **ppFreeCertDistName);

MOC_EXTERN sbyte4  CA_MGMT_extractCertASN1Name(const ubyte *pCertificate, ubyte4 certificateLength,
                                           sbyte4 isSubject, sbyte4 includeASN1SeqHeader, ubyte4* pASN1NameOffset, ubyte4* pASN1NameLen);

MOC_EXTERN sbyte4  CA_MGMT_convertKeyDER(ubyte *pDerRsaKey, ubyte4 derRsaKeyLength, ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength);
MOC_EXTERN sbyte4  CA_MGMT_convertKeyPEM(ubyte *pPemRsaKey, ubyte4 pemRsaKeyLength, ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength);
MOC_EXTERN MSTATUS CA_MGMT_keyBlobToDER(const ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength);
MOC_EXTERN MSTATUS CA_MGMT_keyBlobToPEM(const ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte **ppRetKeyPEM, ubyte4 *pRetKeyPEMLength);

MOC_EXTERN sbyte4  CA_MGMT_verifyCertWithKeyBlob(certDescriptor *pCertificateDescr, sbyte4 *pIsGood);

MOC_EXTERN sbyte4  CA_MGMT_extractCertTimes(ubyte *pCertificate, ubyte4 certificateLength, certDistinguishedName *pRetDN);
MOC_EXTERN sbyte4  CA_MGMT_decodeCertificate(ubyte* pKeyFile, ubyte4 fileSize, ubyte** ppDecodeFile, ubyte4 *pDecodedLength);
MOC_EXTERN sbyte4  CA_MGMT_decodeCertificateBundle(ubyte* pBundleFile, ubyte4 fileSize, certDescriptor **pCertificates, ubyte4 *pNumCerts);
MOC_EXTERN sbyte4  CA_MGMT_freeCertificateBundle(certDescriptor **pCertificates, ubyte4 numCerts);

#ifdef __ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__
MOC_EXTERN sbyte4  CA_MGMT_extractSerialNum (ubyte* pCertificate, ubyte4 certificateLength, ubyte** ppRetSerialNum, ubyte4* pRetSerialNumLength);
MOC_EXTERN sbyte4  CA_MGMT_freeSearchDetails(ubyte** ppFreeData);

/*!
 exclude
*/
typedef sbyte4 (*CA_MGMT_EnumItemCBFun)( const ubyte* pContent, ubyte4 contentLen, ubyte4 contentType,
                                        ubyte4 index, void* userArg);
MOC_EXTERN sbyte4  CA_MGMT_enumCrl(ubyte* pCertificate, ubyte4 certificateLength,
                                         CA_MGMT_EnumItemCBFun callbackFunc, void* userArg);

MOC_EXTERN sbyte4  CA_MGMT_enumAltName( ubyte* pCertificate, ubyte4 certificateLength, sbyte4 isSubject,
                                         CA_MGMT_EnumItemCBFun callbackFunc, void* userArg);
#endif

#ifdef __PUBCRYPTO_HEADER__
MOC_EXTERN MSTATUS CA_MGMT_makeKeyBlobEx(AsymmetricKey *pKey, ubyte **ppRetKeyBlob, ubyte4 *pRetKeyLength);
MOC_EXTERN MSTATUS CA_MGMT_extractKeyBlobEx(const ubyte *pKeyBlob, ubyte4 keyBlobLength, AsymmetricKey* pKey);
MOC_EXTERN MSTATUS CA_MGMT_extractKeyBlobTypeEx(const ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte4 *pRetKeyType);
MOC_EXTERN MSTATUS CA_MGMT_extractPublicKey(const ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte **ppRetPublicKeyBlob, ubyte4 *pRetPublicKeyBlobLength, ubyte4 *pRetKeyType);
#endif /* __PUBCRYPTO_HEADER__ */

#ifdef __ENABLE_MOCANA_EXTRACT_CERT_BLOB__
MOC_EXTERN sbyte4 CA_MGMT_findCertDistinguishedName(ubyte *pCertificate, ubyte4 certificateLength, intBoolean isSubject, ubyte **ppRetDistinguishedName, ubyte4 *pRetDistinguishedNameLen);
#endif

MOC_EXTERN sbyte4 CA_MGMT_generateNakedKey(ubyte4 keyType, ubyte4 keySize, ubyte **ppRetNewKeyBlob, ubyte4 *pRetNewKeyBlobLength);
MOC_EXTERN sbyte4 CA_MGMT_freeNakedKey(ubyte **ppFreeKeyBlob);

MOC_EXTERN sbyte4 CA_MGMT_convertPKCS8KeyToKeyBlob(const ubyte* pPKCS8DER, ubyte4 pkcs8DERLen, ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength);
MOC_EXTERN sbyte4 CA_MGMT_convertProtectedPKCS8KeyToKeyBlob(const ubyte* pPKCS8DER, ubyte4 pkcs8DERLen, ubyte *pPassword, ubyte4 passwordLen, ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength);
#ifdef __PKCS_KEY_HEADER__
MOC_EXTERN sbyte4 CA_MGMT_convertKeyBlobToPKCS8Key(const ubyte *pKeyBlob, ubyte4 keyBlobLength, enum PKCS8EncryptionType encType, const ubyte *pPassword, ubyte4 passwordLen, ubyte **ppRetPKCS8DER, ubyte4 *pRetPkcs8DERLen);
#endif
MOC_EXTERN sbyte4 CA_MGMT_cloneKeyBlob(const ubyte* pKeyBlob, ubyte4 keyBlobLen, ubyte** ppRetKeyBlob, ubyte4 *pRetKeyBlobLen);
MOC_EXTERN sbyte4 CA_MGMT_freeKeyBlob(ubyte **ppFreeThisKeyBlob);

#if !(defined(__DISABLE_MOCANA_KEY_GENERATION__)) && !(defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__))
MOC_EXTERN sbyte4 CA_MGMT_extractPublicKeyInfo(ubyte *pCertificate, ubyte4 certificateLen, ubyte** ppRetKeyBlob, ubyte4 *pRetKeyBlobLen);
#endif

MOC_EXTERN sbyte4 CA_MGMT_verifySignature(const ubyte* pIssuerCertBlob, ubyte4 issuerCertBlobLen, ubyte* pCertificate, ubyte4 certLen);

MOC_EXTERN sbyte4 CA_MGMT_extractSignature(ubyte* pCertificate, ubyte4 certificateLen, ubyte** ppSignature, ubyte4* pSignatureLen);

MOC_EXTERN sbyte4 CA_MGMT_extractBasicConstraint(ubyte* pCertificate, ubyte4 certificateLen, intBoolean* pIsCritical, certExtensions* pCertExtensions);

MOC_EXTERN MSTATUS CA_MGMT_getCertSignAlgoType(ubyte *pCertificate, ubyte4 certificateLen, ubyte4* pHashType, ubyte4* pPubKeyType);

MOC_EXTERN sbyte4 CA_MGMT_reorderChain(certDescriptor* pCertChain, sbyte4 numCertsInChain,sbyte* numReorderdCerts);

#ifdef __cplusplus
}
#endif

#endif /* __CA_MGMT_HEADER__ */
