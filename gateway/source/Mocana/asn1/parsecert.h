/* Version: mss_v6_3 */
/*
 * parsecert.h
 *
 * X.509v3 Certificate Parser
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PARSECERT_HEADER__
#define __PARSECERT_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------*/

/* used to indicate options for checking certificate policies */
enum
{
    keyUsageMandatory = 0x01,
    rejectUnknownCriticalExtensions = 0x02
    /* checkNameConstraints = 0x04, */
    /* checkPolicy = 0x08 -> not implemented yet */
};

/* bits used for Key Usage extensions */
enum
{
    digitalSignature = 0,
    nonRepudiation = 1,
    keyEncipherment = 2,
    dataEncipherment = 3,
    keyAgreement = 4,
    keyCertSign = 5,
    cRLSign = 6,
    encipherOnly = 7,
    decipherOnly = 8
};


    
struct RSAKey;
#if (defined(__ENABLE_MOCANA_ECC__))
struct ECCKey;
#endif

struct AsymmetricKey;

/* callback function used for enumeration -- should return !=OK to stop
the enumeration */
typedef MSTATUS (*EnumCallbackFun)( ASN1_ITEM* pItem, CStream cs, void* userArg);

/* exported routines */
MOC_EXTERN MSTATUS CERT_decryptRSASignatureBuffer(MOC_RSA(hwAccelDescr hwAccelCtx) struct RSAKey* pRSAKey,
                                           const ubyte* pSignature, ubyte4 signatureLen,
                                           ubyte hash[/*CERT_MAXDIGESTSIZE*/], sbyte4 *pHashLen,
                                           ubyte4* rsaAlgoIdSubType);
#if (defined(__ENABLE_MOCANA_DSA__))
MOC_EXTERN MSTATUS CERT_verifyDSASignature( ASN1_ITEM* pSequenceSignature, CStream s, struct DSAKey* pECCKey,
                                            sbyte4 computedHashLen,
                                            const ubyte computedHash[/*computedHashLen*/]);
MOC_EXTERN MSTATUS CERT_extractDSAKey( ASN1_ITEM* pSubjectKeyInfo,
                                      CStream s, AsymmetricKey* pKey);
#endif

#if (defined(__ENABLE_MOCANA_ECC__))
MOC_EXTERN MSTATUS CERT_verifyECDSASignature( ASN1_ITEM* pSequenceSignature, CStream s, struct ECCKey* pECCKey,
                                            sbyte4 computedHashLen,
                                            const ubyte computedHash[/*computedHashLen*/]);

MOC_EXTERN MSTATUS CERT_extractECCKey(ASN1_ITEM* pSubjectKeyInfo,
                                      CStream s, AsymmetricKey* pKey);
#endif

MOC_EXTERN MSTATUS CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEM* rootItem,
                                                       CStream s, struct AsymmetricKey* pPubKey);
MOC_EXTERN MSTATUS CERT_setKeyFromSubjectPublicKeyInfoCert(MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEM* pTBSCertificate,
                                                           CStream s, struct AsymmetricKey* pPubKey);
MOC_EXTERN MSTATUS CERT_extractRSAKey(MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEM* pSubjectKeyInfo, CStream s, AsymmetricKey* pKey);

MOC_EXTERN MSTATUS CERT_CompSubjectCommonName(ASN1_ITEM* rootItem, CStream s, const sbyte* nameToMatch);
MOC_EXTERN MSTATUS CERT_CompSubjectCommonName2(ASN1_ITEM* pCertificate, CStream s, const sbyte* nameToMatch);

MOC_EXTERN MSTATUS CERT_CompSubjectAltNames(ASN1_ITEM* rootItem, CStream s,
                                        const sbyte* nameToMatch, ubyte4 tagMask);
MOC_EXTERN MSTATUS CERT_CompSubjectAltNames2(ASN1_ITEM* pCertificate, CStream s,
                                             const sbyte* nameToMatch, ubyte4 tagMask);
MOC_EXTERN MSTATUS CERT_CompSubjectAltNamesExact(ASN1_ITEM* rootItem, CStream s,
                                                 const sbyte* nameToMatch, ubyte4 tagMask);
MOC_EXTERN MSTATUS CERT_CompSubjectAltNamesEx( ASN1_ITEM* rootItem, CStream s,
                                            const CNMatchInfo* namesToMatch,
                                            ubyte4 tagMask);
MOC_EXTERN MSTATUS CERT_CompSubjectAltNamesEx2( ASN1_ITEM* pCertificate, CStream s,
                                            const CNMatchInfo* namesToMatch,
                                            ubyte4 tagMask);

#if (defined(__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__))
MOC_EXTERN MSTATUS CERT_CompSubjectCommonNameEx(ASN1_ITEM* rootItem, CStream s, const CNMatchInfo* namesToMatch);
MOC_EXTERN MSTATUS CERT_CompSubjectCommonNameEx2(ASN1_ITEM* pCertificate, CStream s, const CNMatchInfo* namesToMatch);
#endif
MOC_EXTERN MSTATUS CERT_VerifyValidityTime(ASN1_ITEM* rootItem, CStream s);
MOC_EXTERN MSTATUS CERT_VerifyValidityTimeWithConf(ASN1_ITEM* rootItem, CStream s, intBoolean chkBefore, intBoolean chkAfter);
MOC_EXTERN MSTATUS CERT_VerifyValidityTime2(ASN1_ITEM* pCertificate, CStream s);

MOC_EXTERN MSTATUS CERT_ComputeBufferHash(MOC_HASH(hwAccelDescr hwAccelCtx) ubyte* buffer, ubyte4 bytesToHash,
                                      ubyte hash[/*CERT_MAXDIGESTSIZE*/], sbyte4* hashSize, ubyte4 hashType);

MOC_EXTERN MSTATUS CERT_VerifyCertificatePolicies(ASN1_ITEM* rootItem, CStream s, sbyte4 checkOptions,
                                              sbyte4 chainLength, intBoolean rootCertificate);
MOC_EXTERN MSTATUS CERT_VerifyCertificatePolicies2(ASN1_ITEM* pCertificate, CStream s, sbyte4 checkOptions,
                                              sbyte4 chainLength, intBoolean rootCertificate);


MOC_EXTERN MSTATUS CERT_getCertificateKeyUsage(ASN1_ITEM* rootItem, CStream s,
                                               sbyte4 checkOptions, ASN1_ITEM** ppKeyUsage);
MOC_EXTERN MSTATUS CERT_getCertificateKeyUsage2(ASN1_ITEM* pCertificate, CStream s,
                                                sbyte4 checkOptions, ASN1_ITEM** ppKeyUsage);

MOC_EXTERN MSTATUS CERT_validateCertificate(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pPrevCertificate,
                                        CStream pPrevCertStream, ASN1_ITEM* pCertificate, CStream pCertStream,
                                        sbyte4 checkOptions, sbyte4 chainLength, intBoolean rootCertificate);

MOC_EXTERN MSTATUS CERT_validateCertificateWithConf(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pPrevCertificate,
                                 CStream pPrevCertStream, ASN1_ITEM* pCertificate, CStream pCertStream,
                                 sbyte4 checkOptions, sbyte4 chainLength, intBoolean rootCertificate,
                                 intBoolean chkValidityBefore, intBoolean chkValidityAfter);

MOC_EXTERN MSTATUS CERT_extractDistinguishedNames(ASN1_ITEM* rootItem, CStream s,
                                                  intBoolean isSubject, 
                                                  struct certDistinguishedName *pRetDN);
MOC_EXTERN MSTATUS CERT_extractDistinguishedNames2(ASN1_ITEM* pCertificate, CStream s,
                                                  intBoolean isSubject, 
                                                   struct certDistinguishedName *pRetDN);

MOC_EXTERN MSTATUS CERT_extractDistinguishedNamesFromName(ASN1_ITEM* pName, CStream s,
                                       struct certDistinguishedName *pRetDN);

MOC_EXTERN MSTATUS CERT_extractVersion(ASN1_ITEM* startItem, CStream s, intBoolean startIsRoot, sbyte4 *pRetVersion);

MOC_EXTERN MSTATUS CERT_getSubjectCommonName( ASN1_ITEM* rootItem, CStream s, ASN1_ITEM** ppCommonNameItem);
MOC_EXTERN MSTATUS CERT_getSubjectCommonName2( ASN1_ITEM* pCertificate, CStream s, ASN1_ITEM** ppCommonNameItem);

MOC_EXTERN MSTATUS CERT_getSubjectEntryByOID( ASN1_ITEM* rootItem, CStream s, const ubyte* oid, ASN1_ITEM** ppEntryItem);
MOC_EXTERN MSTATUS CERT_getSubjectEntryByOID2( ASN1_ITEM* pCertificate, CStream s, const ubyte* oid, ASN1_ITEM** ppEntryItem);

MOC_EXTERN MSTATUS CERT_checkCertificateIssuer(ASN1_ITEM* pPrevRootItem, CStream pPrevCertStream, ASN1_ITEM* pRootItem,
                                           CStream pCertStream);

MOC_EXTERN MSTATUS CERT_GetCertTime( ASN1_ITEMPTR pTime, CStream s, TimeDate* pGMTTime);
MOC_EXTERN MSTATUS CERT_verifySignature( MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEM *pCertOrCRL, CStream cs, AsymmetricKey *pIsuerPubKey);

MOC_EXTERN MSTATUS CERT_extractValidityTime(ASN1_ITEM* rootItem, CStream s, 
                                            struct certDistinguishedName *pRetDN);
MOC_EXTERN MSTATUS CERT_extractValidityTime2(ASN1_ITEM* pCertificate, CStream s, 
                                             struct certDistinguishedName *pRetDN);
MOC_EXTERN MSTATUS CERT_getValidityTime(ASN1_ITEM* startItem, intBoolean startIsRoot, ASN1_ITEMPTR* pRetStart, ASN1_ITEMPTR* pRetEnd);

MOC_EXTERN MSTATUS CERT_rawVerifyOID(ASN1_ITEM* rootItem, CStream s, const ubyte *pOidItem, const ubyte *pOidValue,
                                 intBoolean *pIsPresent);
MOC_EXTERN MSTATUS CERT_rawVerifyOID2(ASN1_ITEM* pCertificate, CStream s, const ubyte *pOidItem, const ubyte *pOidValue,
                                 intBoolean *pIsPresent);

MOC_EXTERN MSTATUS CERT_extractSerialNum(ASN1_ITEM* rootItem, CStream s, ubyte** ppRetSerialNum, ubyte4 *pRetSerialNumLength);
MOC_EXTERN MSTATUS CERT_extractSerialNum2(ASN1_ITEM* rootItem, CStream s, ubyte** ppRetSerialNum, ubyte4 *pRetSerialNumLength);

/* use this function to go through all the crlDistributionPoints -- pCurrItem is the current item -- it should be NULL
in the first call or to reset the enumeration; the function will return ERR_FALSE if there not any more items */
MOC_EXTERN MSTATUS CERT_enumerateCRL( ASN1_ITEM* rootItem, CStream s, EnumCallbackFun ecf, void* userArg);
MOC_EXTERN MSTATUS CERT_enumerateCRL2( ASN1_ITEM* pCertificate, CStream s, EnumCallbackFun ecf, void* userArg);

MOC_EXTERN MSTATUS CERT_enumerateAltName( ASN1_ITEM* rootItem, CStream s, sbyte4 isSubject, EnumCallbackFun ecf, void* userArg);
MOC_EXTERN MSTATUS CERT_enumerateAltName2( ASN1_ITEM* pCertificate, CStream s, sbyte4 isSubject, EnumCallbackFun ecf, void* userArg);

MOC_EXTERN MSTATUS CERT_checkCertificateIssuerSerialNumber( ASN1_ITEM* pIssuer, ASN1_ITEM* pSerialNumber,
                                                        CStream pIssuerStream,
                                                        ASN1_ITEM* pCertificate, CStream pCertStream);
MOC_EXTERN MSTATUS CERT_getCertificateIssuerSerialNumber( ASN1_ITEM* rootItem, ASN1_ITEM** ppIssuer, ASN1_ITEM** ppSerialNumber);
MOC_EXTERN MSTATUS CERT_getCertificateIssuerSerialNumber2( ASN1_ITEM* pCertificate, ASN1_ITEM** ppIssuer, ASN1_ITEM** ppSerialNumber);

MOC_EXTERN MSTATUS CERT_getCertificateSubject( ASN1_ITEM* rootItem, ASN1_ITEM** ppSubject);
MOC_EXTERN MSTATUS CERT_getCertificateSubject2( ASN1_ITEM* pCertificate, ASN1_ITEM** ppSubject);

MOC_EXTERN MSTATUS CERT_getRSASignatureAlgo( ASN1_ITEM* rootItem, CStream certStream, ubyte* signAlgo);

MOC_EXTERN MSTATUS CERT_isRootCertificate(ASN1_ITEM* rootItem, CStream s);
MOC_EXTERN MSTATUS CERT_isRootCertificate2(ASN1_ITEM* pCertificate, CStream s);

#ifdef __ENABLE_MOCANA_EXTRACT_CERT_BLOB__
MOC_EXTERN MSTATUS CERT_extractDistinguishedNamesBlob(ASN1_ITEM* rootItem, CStream s, intBoolean isSubject, ubyte **ppRetDistinguishedName, ubyte4 *pRetDistinguishedNameLen);
#endif

extern MSTATUS CERT_getCertExtension( ASN1_ITEMPTR pExtensionsSeq, CStream s,
                            const ubyte* whichOID, intBoolean* critical,
                            ASN1_ITEMPTR* ppExtension);

extern MSTATUS CERT_getCertificateExtensions(ASN1_ITEM* rootItem,
                                ASN1_ITEM** ppExtensions);
extern MSTATUS CERT_getCertificateExtensions2(ASN1_ITEM* pCertificate,
                                ASN1_ITEM** ppExtensions);

extern MSTATUS CERT_getCertSignAlgoType(ASN1_ITEM* pSignAlgoId, CStream s, ubyte4* hashType, ubyte4* pubKeyType);

extern MSTATUS CERT_getSignatureItem(ASN1_ITEM* rootItem, CStream s, ASN1_ITEM** ppSignature);


#ifdef __cplusplus
}
#endif

#endif /* __PARSECERT_HEADER__ */
