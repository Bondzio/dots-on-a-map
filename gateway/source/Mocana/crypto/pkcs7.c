/* Version: mss_v6_3 */
/*
 * pkcs7.c
 *
 * PKCS7 Utilities
 *
 * Copyright Mocana Corp 2005-2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#ifdef __ENABLE_MOCANA_PKCS7__

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../crypto/crypto.h"
#include "../crypto/rsa.h"
#include "../crypto/dsa.h"
#include "../crypto/dsa2.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../harness/harness.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/ca_mgmt.h"
#include "../crypto/des.h"
#include "../crypto/aes.h"
#include "../crypto/three_des.h"
#include "../crypto/arc4.h"
#include "../crypto/rc4algo.h"
#include "../crypto/arc2.h"
#include "../crypto/rc2algo.h"
#include "../crypto/aes_keywrap.h"
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../crypto/pkcs_common.h"
#include "../asn1/parsecert.h"
#include "../asn1/derencoder.h"
#include "../crypto/ansix9_63_kdf.h"
#include "../crypto/pkcs7.h"


/* for CMS */
#include "../crypto/cms.h"
#include "../crypto/sec_key.h"


/*--------------------------------------------------------------------------*/

/* private data structures */

typedef struct SignedDataHash
{
    ubyte   hashType;      /* used for signing/verification */
    const ubyte* algoOID;  /* used for signing */
    const   BulkHashAlgo* hashAlgo;
    ubyte*  hashData;
    BulkCtx bulkCtx;
} SignedDataHash;


typedef struct PKCS7_SignatureInfo
{
    ASN1_ITEMPTR    pASN1;
    ubyte4          msgSigDigestLen;
    ubyte           msgSigDigest[CERT_MAXDIGESTSIZE];
} PKCS7_SignatureInfo;

static const ubyte kRFC5758_HASHTYPE_TO_RSA_HASHTYPE[] =
{
    ht_sha224,
    ht_sha256,
    ht_sha384,
    ht_sha512
};

/* static function prototypes */

static MSTATUS PKCS7_GetHashAlgoIdFromHashAlgoOID( ASN1_ITEMPTR pDigestAlgoOID, CStream s,
                                                  ubyte* hashAlgoId);

static MSTATUS PKCS7_GetHashAlgoIdFromHashAlgoOID2( const ubyte* digestAlgoOID,
                                                   ubyte* hashAlgoId);

static MSTATUS PKCS7_ProcessSignerInfo(MOC_RSA_HASH(hwAccelDescr hwAccelCtx)
                                        CStream s, ASN1_ITEMPTR pSignerInfo,
                                        CStream certificatesStream,
                                        ASN1_ITEMPTR pCertificates,
                                        ASN1_ITEMPTR pContentType,
                                        sbyte4 numHashes,
                                        SignedDataHash hashes[/*numHashes*/],
                                        PKCS7_SignatureInfo* pSigInfo,
                                        ASN1_ITEMPTR* ppCertificate);

/* figure out the certificate that corresponds to the signer info */
static MSTATUS PKCS7_GetSignerInfoCertificate( CStream s,
                                                ASN1_ITEMPTR pSignerInfo,
                                                CStream certificatesStream,
                                                ASN1_ITEMPTR pCertificates,
                                                ASN1_ITEMPTR* ppCertificate);

/* this will figure out the chain of certifcates and will call the
 supplied callback to make sure the root of the chain is an accepted
 certificate. Used to verify SignedData */
static MSTATUS PKCS7_ValidateCertificate( CStream s,
                                            ASN1_ITEMPTR pLeafCertificate,
                                            ASN1_ITEMPTR pCertificates,
                                            PKCS7_ValidateRootCertificate valCertFun);


/*
 this routine creates a ContentInfo as a child of pParent.
 contentType is given in payLoadType;
 content is given in payLoad.
 it optionally returns a pointer to the ContentInfo.
ContentInfo ::= SEQUENCE {
  contentType  ContentType,
  content      [0] EXPLICIT CONTENTS.&Type({Contents}{@contentType})
OPTIONAL
}
*/
static MSTATUS
PKCS7_AddContentInfo(DER_ITEMPTR pParent,
               const ubyte* payLoadType, /* OID */
               const ubyte* pPayLoad,
               ubyte4 payLoadLen,
               DER_ITEMPTR *ppContentInfo);

/* this routine creates a node with the provided tag,
 with sets of or sequences of items (indicated by passing
the ASN1 type SET or SEQUENCE) as children.
 it returns a pointer to the node with tag.
[tag] SET (or SEQUENCE) {
item1,
item2,
...
}
*/
static MSTATUS
PKCS7_AddSetOfOrSequenceOfASN1ItemsWithTag(DER_ITEMPTR pParent,
                                           ubyte tag, ubyte4 setOrSequence,
                                           CStream *itemStreams,
                                           ASN1_ITEMPTR *ppRootItems, ubyte4 numItems,
                                           DER_ITEMPTR *ppChild);

/* This routine add an item to parent.
 * it optionally returns the newly added item.
*/
static MSTATUS
PKCS7_AddItem1(DER_ITEMPTR pParent,
              CStream cs, ASN1_ITEMPTR pRootItem,
              DER_ITEMPTR *ppNewItem);

/* This routine add an item given by payLoad to parent.
 * it optionally returns the newly added item.
*/
static MSTATUS
PKCS7_AddItem2( DER_ITEMPTR pParent,
               const ubyte* pPayLoad, ubyte4 payLoadLen,
               DER_ITEMPTR *ppNewItem);


/* this routine creates an Attribute given
 an attribute type (an OID), an attribute value,
 as well as the value type (i.e. id|tag).
 it optionally returns a pointer to it.
Attribute       ::=     SEQUENCE {
type              AttributeType,
values    SET OF AttributeValue }
-- at least one value is required
*/
static MSTATUS
PKCS7_AddAttribute(DER_ITEMPTR pParent, const ubyte* typeOID,
                   const ubyte valueType, const ubyte* value, ubyte4 valueLen,
                   DER_ITEMPTR *ppAttribute);

/* this routine adds an IssuerAndSerialNumber structure to the pParent.
IssuerAndSerialNumber ::= SEQUENCE {
  issuer        Name,
  serialNumber  CertificateSerialNumber
}
*/
static MSTATUS
PKCS7_AddIssuerAndSerialNumber(DER_ITEMPTR pParent,
                               CStream cs,
                               ASN1_ITEMPTR pIssuer,
                               ASN1_ITEMPTR pSerialNumber,
                               DER_ITEMPTR *ppIssuerAndSerialNumber);

/* This routine adds per signerInfo to the parent.
 * pSignerInfo points to the signer info for one signer;
 * md5Hash and sha1Hash contains the message digest for the payload;
 * payLoadType is provided for adding contentType attribute value if appropriate;
 * pDataBuffer is used to keep track of the allocated buffers that needs to be deallocated later.
*/
static MSTATUS
PKCS7_AddPerSignerInfo(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) DER_ITEMPTR pParent,
                       signerInfoPtr pSignerInfo,
                        SignedDataHash* pDataHash,
                        RNGFun rngFun, void* rngFunArg,
                        ubyte* payLoadType,
                        ubyte** pDataBuffer );

#ifdef __ENABLE_MOCANA_ECC__

static MSTATUS PKCS7_ECCEncryptKey(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                    const BulkHashAlgo* pHashAlgo,
                    ECCKey* pECCKey, ConstPFEPtr k,
                    const ubyte* keyWrapOID,
                    const ubyte* ukmData, ubyte4 ukmDataLen,
                    const ubyte* cek, ubyte4 cekLen,
                    ubyte** encryptedKey, ubyte4* encryptedKeyLen);


static MSTATUS PKCS7_ECCDecryptKey(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                    const BulkHashAlgo* pHashAlgo,
                    ECCKey* pECCKey, ConstPFEPtr k,
                    const ubyte* keyWrapOID,
                    const ubyte* ukmData, ubyte4 ukmDataLen,
                    const ubyte* encryptedKey, ubyte4 encryptedKeyLen,
                    ubyte** cek, ubyte4* cekLen);

#endif

/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GetHashAlgoIdFromHashAlgoOID( ASN1_ITEMPTR pDigestAlgoOID, CStream s,
                                    ubyte* hashAlgoId)
{
    ubyte sha2SubType;

    if (OK == ASN1_VerifyOID( pDigestAlgoOID, s, sha1_OID))
    {
        *hashAlgoId = ht_sha1;
    }
    else if ( OK == ASN1_VerifyOID( pDigestAlgoOID, s, md5_OID))
    {
        *hashAlgoId = ht_md5;
    }
    else if (OK == ASN1_VerifyOIDRoot( pDigestAlgoOID, s, sha2_OID, &sha2SubType))
    {
        switch(sha2SubType)
        {
        case sha256Digest:
            *hashAlgoId = ht_sha256;
            break;

        case sha384Digest:
            *hashAlgoId = ht_sha384;
            break;

        case sha512Digest:
            *hashAlgoId = ht_sha512;
            break;

        case sha224Digest:
            *hashAlgoId = ht_sha224;
            break;
        }
    }
    else
    {
        return ERR_PKCS7_UNSUPPORTED_DIGESTALGO;
    }

    return OK;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GetHashAlgoIdFromHashAlgoOID2( const ubyte* digestAlgoOID,
                                     ubyte* hashAlgoId)
{
    /* note we are using pointer comparison */
    if (digestAlgoOID == md5_OID)
    {
        *hashAlgoId = ht_md5;
    }
    else if (digestAlgoOID == sha1_OID)
    {
        *hashAlgoId = ht_sha1;
    }
    else if (digestAlgoOID == sha224_OID)
    {
        *hashAlgoId = ht_sha224;
    }
    else if (digestAlgoOID == sha256_OID)
    {
        *hashAlgoId = ht_sha256;
    }
    else if (digestAlgoOID == sha384_OID)
    {
        *hashAlgoId = ht_sha384;
    }
    else if (digestAlgoOID == sha512_OID)
    {
        *hashAlgoId = ht_sha512;
    }
    else
    {
        return ERR_PKCS7_UNSUPPORTED_DIGESTALGO;
    }
    return OK;
}



/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_DestructHashes( MOC_HASH(hwAccelDescr hwAccelCtx) ubyte4 numHashes, SignedDataHash** ppHashes)
{
    ubyte4 i;
    SignedDataHash* pHashes;

    if (!ppHashes || !(*ppHashes))
        return ERR_NULL_POINTER;

    pHashes = *ppHashes;

    for (i = 0; i < numHashes; ++i)
    {
        if ( pHashes[i].bulkCtx)
        {
            pHashes[i].hashAlgo->freeFunc( MOC_HASH(hwAccelCtx)
                                               &pHashes[i].bulkCtx);
        }
        if ( pHashes[i].hashData)
        {
            CRYPTO_FREE(hwAccelCtx, TRUE, &pHashes[i].hashData);
        }
    }

    FREE(pHashes);
    *ppHashes = 0;

    return OK;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_ConstructHashes(  MOC_HASH(hwAccelDescr hwAccelCtx) ubyte4 hashes,
                        ubyte4* numHashes, SignedDataHash** ppHashes)
{
    MSTATUS status = OK;
    ubyte4 i,j;
    SignedDataHash* pHashes = 0;

    /* compute the hashes here */
    *numHashes = MOC_BITCOUNT( hashes);
    if (0 == *numHashes)
    {
        *ppHashes = 0;
        return OK;
    }

    pHashes = MALLOC( (*numHashes) * sizeof( SignedDataHash));
    if (!pHashes)
    {
        return ERR_MEM_ALLOC_FAIL;
    }

    MOC_MEMSET( (ubyte*) pHashes, 0, (*numHashes) * sizeof( SignedDataHash));

    i = j = 0;
    while (hashes && j < *numHashes)
    {
        if ( 1 & hashes)
        {
            pHashes[j].hashType = i;
            if (OK > (status = CRYPTO_getHashAlgoOID( i, &pHashes[j].algoOID)))
            {
                goto exit;
            }
            if (OK > (status = CRYPTO_getRSAHashAlgo( i, &pHashes[j].hashAlgo)))
            {
               goto exit;
            }
            if (OK > (status = CRYPTO_ALLOC( hwAccelCtx,
                                      pHashes[j].hashAlgo->digestSize,
                                      TRUE, &(pHashes[j].hashData))))
            {
               goto exit;
            }
            pHashes[j].hashAlgo->allocFunc(MOC_HASH(hwAccelCtx)
                                             &pHashes[j].bulkCtx);
            pHashes[j].hashAlgo->initFunc( MOC_HASH(hwAccelCtx)
                                             pHashes[j].bulkCtx);
            ++j;
        }
        hashes >>= 1;
        i++;
    }

    *ppHashes = pHashes;
    pHashes = 0;

exit:

    if (pHashes)
    {
        PKCS7_DestructHashes(MOC_HASH(hwAccelCtx)*numHashes, &pHashes);
    }

    return status;
}


/*-------------------------------------------------------------------------*/

static ubyte4
PKCS7_getNumberChildren( ASN1_ITEMPTR pParent)
{
    ubyte4 retVal = 0;
    ASN1_ITEMPTR pChild;

    pChild = ASN1_FIRST_CHILD( pParent);
    while (pChild)
    {
        ++retVal;
        pChild = ASN1_NEXT_SIBLING(pChild);
    }

    return retVal;
}


/*--------------------------------------------------------------------------*/

/*
    pRootItem is the pointer to an ASN1_ITEM returned by
    ASN1_Parse
*/

extern MSTATUS
PKCS7_GetCertificates(ASN1_ITEM* pRootItem,
           CStream s,
           ASN1_ITEM** ppFirstCertificate)
{
    static WalkerStep pcks7WalkInstructions[] =
    {
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},
        { VerifyOID, 0, (ubyte*) pkcs7_signedData_OID },
        { GoNextSibling, 0, 0},
        { VerifyTag, 0, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0},
        { GoChildWithTag, 0, 0},
        { Complete, 0, 0}
    };

    return ASN1_WalkTree( pRootItem, s, pcks7WalkInstructions, ppFirstCertificate);
}


/*--------------------------------------------------------------------------*/

extern MSTATUS PKCS7_FindCertificate( CStream s,
                                        ASN1_ITEMPTR pIssuer,
                                        ASN1_ITEMPTR pSerialNumber,
                                        CStream certificatesStream,
                                        ASN1_ITEMPTR pCertificates,
                                        ASN1_ITEMPTR* ppCertificate)
{
    ASN1_ITEMPTR pCurrCertificate;

    if ( 0 == ppCertificate || 0 == pIssuer ||
        0 == pSerialNumber || 0 == pCertificates)
    {
        return ERR_NULL_POINTER;
    }

    *ppCertificate = 0;

    /* loop over the certificates */
    pCurrCertificate = pCertificates;
    while ( 0 != pCurrCertificate)
    {
        /* certificate part or TBSCertificate is the first child of certificae */
        ASN1_ITEMPTR pCertificatePart = ASN1_FIRST_CHILD(pCurrCertificate);
        if (OK == CERT_checkCertificateIssuerSerialNumber(  pIssuer,
                                                            pSerialNumber,
                                                            s,
                                                            pCertificatePart,
                                                            certificatesStream))
        {
            *ppCertificate = pCertificatePart;
            break;
        }
        pCurrCertificate = ASN1_NEXT_SIBLING( pCurrCertificate);
    }

    /* always return OK */
    return OK;
}


/*--------------------------------------------------------------------------*/

static MSTATUS PKCS7_GetSignerInfoCertificate( CStream s,
                                                ASN1_ITEMPTR pSignerInfo,
                                                CStream certificatesStream,
                                                ASN1_ITEMPTR pCertificates,
                                                ASN1_ITEMPTR* ppCertificate)
{
    ASN1_ITEMPTR pIssuerSerialNumber;
    ASN1_ITEMPTR pIssuer, pSerialNumber;

    if ( OK > ASN1_GetNthChild( pSignerInfo, 2, &pIssuerSerialNumber) ||
            OK > ASN1_VerifyType( pIssuerSerialNumber, SEQUENCE))
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    pIssuer = ASN1_FIRST_CHILD( pIssuerSerialNumber);
    if ( NULL == pIssuer)
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    pSerialNumber = ASN1_NEXT_SIBLING( pIssuer);
    if ( NULL == pSerialNumber)
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    return PKCS7_FindCertificate( s, pIssuer, pSerialNumber,
                                    certificatesStream, pCertificates,
                                    ppCertificate);
}


/*--------------------------------------------------------------------------*/

static MSTATUS PKCS7_VerifyRSASignature(MOC_RSA(hwAccelDescr hwAccelCtx) CStream s,
                                            ASN1_ITEMPTR pEncryptedDigest,
                                            RSAKey* pRSAKey,
                                            const ubyte* hash,
                                            ubyte4 hashLen,
                                            ubyte4 hashType)
{
    MSTATUS status;
    ubyte* buffer = NULL;
    ubyte4  rsaAlgoId;
    ubyte decryptedSignature[CERT_MAXDIGESTSIZE];
    sbyte4 decryptedSignatureLen, resCmp;

    buffer = (ubyte *)CS_memaccess( s, pEncryptedDigest->dataOffset, pEncryptedDigest->length);
    if (NULL == buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = CERT_decryptRSASignatureBuffer(MOC_RSA(hwAccelCtx) pRSAKey,
                                buffer, pEncryptedDigest->length, decryptedSignature,
                                &decryptedSignatureLen, &rsaAlgoId)))
    {
        goto exit;
    }

    if ( hashType != rsaAlgoId || decryptedSignatureLen != hashLen)
    {
        status = ERR_PKCS7_INVALID_SIGNATURE;
        goto exit;
    }

    if (OK > ( status = MOC_MEMCMP( decryptedSignature, hash, hashLen, &resCmp)))
    {
        goto exit;
    }

    if ( 0 != resCmp)
    {
        status = ERR_PKCS7_INVALID_SIGNATURE;
        goto exit;
    }

exit:
    if ( buffer)
    {
        CS_stopaccess( s, buffer);
    }

    return status;
}

#ifdef __ENABLE_MOCANA_ECC__
/*--------------------------------------------------------------------------*/

static MSTATUS PKCS7_VerifyECDSASignature(CStream s,
                                            ASN1_ITEMPTR pEncryptedDigest,
                                            ECCKey* pECCKey,
                                            const ubyte* hash,
                                            ubyte4 hashLen,
                                            ubyte4 hashType)
{
    MSTATUS status;
    ASN1_ITEMPTR pSequence;

    pSequence = ASN1_FIRST_CHILD( pEncryptedDigest);
    if ( OK > ( status = ASN1_VerifyType( pSequence, SEQUENCE)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* call the exported routine */
    status = CERT_verifyECDSASignature( pSequence, s, pECCKey,
                                        hashLen, hash);
exit:
    return status;
}
#endif

#ifdef __ENABLE_MOCANA_DSA__
/*--------------------------------------------------------------------------*/

static MSTATUS PKCS7_VerifyDSASignature(CStream s,
                                            ASN1_ITEMPTR pEncryptedDigest,
                                            DSAKey* pDSAKey,
                                            const ubyte* hash,
                                            ubyte4 hashLen,
                                            ubyte4 hashType)
{
    MSTATUS status;
    ASN1_ITEMPTR pSequence;

    pSequence = ASN1_FIRST_CHILD( pEncryptedDigest);
    if ( OK > ( status = ASN1_VerifyType( pSequence, SEQUENCE)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* call the exported routine */
    status = CERT_verifyDSASignature( pSequence, s, pDSAKey,
                                        hashLen, hash);
exit:
    return status;
}
#endif

/*--------------------------------------------------------------------------*/

static MSTATUS PKCS7_ValidateCertificate( CStream s,
                                          ASN1_ITEMPTR pLeafCertificate,
                                          ASN1_ITEMPTR pCertificates,
                                          PKCS7_ValidateRootCertificate valCertFun)
{
    MSTATUS status = OK;

    /* TO DO: function needs to be revisited */
    if (valCertFun)
    {
        if (OK > (status = valCertFun(s, pLeafCertificate, pCertificates)))
            status = ERR_FALSE;
    }
    else
    {
        status = ERR_FALSE;
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_ProcessSignerInfo(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) CStream s,
                        ASN1_ITEMPTR pSignerInfo,
                        CStream certificatesStream,
                        ASN1_ITEMPTR pCertificates,
                        ASN1_ITEMPTR pContentType,
                        sbyte4 numHashes, SignedDataHash hashes[/*numHashes*/],
                        PKCS7_SignatureInfo* pSigInfo,
                        ASN1_ITEMPTR* ppCertificate)
{
    ASN1_ITEMPTR pDigestAlgoOID, pDigestAlgo;
    ASN1_ITEMPTR pAuthenticatedAttributes;
    ASN1_ITEMPTR pEncryptedDigest;
    ASN1_ITEMPTR pDigestEncryptionAlgo;
    ASN1_ITEMPTR pCertificate;
    ubyte* attr = 0;
    ubyte4 attrLen = 0;
    const ubyte *pHashResult;
    ubyte *pTempBuf = NULL;
    ubyte *messageDigest = NULL; /* the messageDigest contained inside AuthenticatedAttributes */
    ubyte4 messageDigestLen = 0;
    /* enough space for either digest type */
    sbyte4 cmpResult = -1;
    MSTATUS status = OK;
    DER_ITEMPTR pSetOf = NULL;
    ubyte *dataBuf = NULL;
    ubyte4 dataBufLen;
    sbyte4 i;
    ubyte hashType = 0, subType = 0xFF;
    SignedDataHash *pHashInfo = 0;
    AsymmetricKey certKey;

    if (OK > ASN1_VerifyType( pSignerInfo, SEQUENCE))
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    if (OK > ASN1_GetNthChild( pSignerInfo, 3, &pDigestAlgo) ||
        OK > ASN1_VerifyType( pDigestAlgo, SEQUENCE) )
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    pDigestAlgoOID = ASN1_FIRST_CHILD( pDigestAlgo);
    if ( OK > ASN1_VerifyType( pDigestAlgoOID, OID))
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    /* DigestEncryption and Encrypted Digest are the next siblings
        unless there is Authenticated Attributes identified by tag 0 */
    pDigestEncryptionAlgo = ASN1_NEXT_SIBLING( pDigestAlgo);
    if ( 0 == pDigestEncryptionAlgo)
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    if (OK == ASN1_VerifyTag( pDigestEncryptionAlgo, 0))
    {
        pAuthenticatedAttributes = pDigestEncryptionAlgo;
        pDigestEncryptionAlgo = ASN1_NEXT_SIBLING( pAuthenticatedAttributes);
    }
    else
    {
        pAuthenticatedAttributes = 0;
    }

    if ( OK > ASN1_VerifyType( pDigestEncryptionAlgo, SEQUENCE) ||
        OK > ASN1_VerifyType( ( pEncryptedDigest = ASN1_NEXT_SIBLING( pDigestEncryptionAlgo)),
                                OCTETSTRING))
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    if (OK > ( status = PKCS7_GetSignerInfoCertificate( s, pSignerInfo,
                            certificatesStream, pCertificates, &pCertificate)))
    {
        return status;
    }

    if ( 0 == pCertificate)
    {
        return ERR_PKCS7_NO_CERT_FOR_SIGNER;
    }

   if (OK > (status = CRYPTO_initAsymmetricKey( &certKey)))
        goto exit;

    if ( OK > ( status = CERT_setKeyFromSubjectPublicKeyInfoCert(MOC_RSA(hwAccelCtx)  pCertificate,
                                                                    certificatesStream, &certKey)))
    {
        goto exit;
    }

    if (certKey.type == akt_rsa)
    {
        if ( OK > ASN1_VerifyOID( ASN1_FIRST_CHILD(pDigestEncryptionAlgo), s, rsaEncryption_OID))
        {
            /* could be rsaEncryption + hash -- certicom library does that */
            if (OK > ASN1_VerifyOIDRoot( ASN1_FIRST_CHILD(pDigestEncryptionAlgo), s,
                                         pkcs1_OID, &subType))
            {
                status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
                goto exit;
            }
            /* we will test the subtype to verify it matches the hashType */
        }
    }
#ifdef __ENABLE_MOCANA_DSA__
    else if ( certKey.type == akt_dsa)
    {
        ASN1_ITEMPTR pAlgoOID = ASN1_FIRST_CHILD( pDigestEncryptionAlgo);

        /* certicom uses the dsa_OID, most others dsaWithSHA1 */
        if ( OK > ASN1_VerifyOID( pAlgoOID, s, dsaWithSHA1_OID) &&
             OK > ASN1_VerifyOID( pAlgoOID, s, dsa_OID) )
        {
           if (OK > ASN1_VerifyOIDRoot( pAlgoOID, s, dsaWithSHA2_OID, &subType)
                || 0 == subType || subType > 2)
            {
                status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
                goto exit;
            }
            /* we will match the subtype so convert to hashtype */
            subType = kRFC5758_HASHTYPE_TO_RSA_HASHTYPE[subType-1];          }
        else
        {
            subType = ht_sha1;
        }
    }
#endif
#ifdef __ENABLE_MOCANA_ECC__
    else if ( certKey.type == akt_ecc)
    {
        ASN1_ITEMPTR pAlgoOID = ASN1_FIRST_CHILD( pDigestEncryptionAlgo);

        if ( OK > ASN1_VerifyOID( pAlgoOID, s, ecdsaWithSHA1_OID) )
        {
            if (OK > ASN1_VerifyOIDRoot( pAlgoOID, s, ecdsaWithSHA2_OID, &subType)
                || 0 == subType || subType > 4)
            {
                status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
                goto exit;
            }
            /* we will match the subtype so convert to hashtype */
            subType = kRFC5758_HASHTYPE_TO_RSA_HASHTYPE[subType-1];
        }
        else
        {
            subType = ht_sha1;
        }
    }
#endif
    else
    {
        status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
        goto exit;
    }

    if (OK > ( status = PKCS7_GetHashAlgoIdFromHashAlgoOID( pDigestAlgoOID, s,
                                                            &hashType)))
    {
        goto exit;
    }

    if (subType != 0xFF && subType != hashType)
    {
        status = ERR_PKCS7_MISMATCH_SIG_HASH_ALGO;
        goto exit;
    }

    for (i = 0; i < numHashes; ++i)
    {
        if ( hashes[i].hashType == hashType)
        {
            pHashInfo = hashes+i;
            break;
        }
    }

    if (!pHashInfo)
    {
        status = ERR_PKCS7_INVALID_STRUCT; /*ERR_PKCS7_UNEXPECTED_SIGNER_INFO_HASH;*/
        goto exit;
    }

    if ( pAuthenticatedAttributes )
    {
        /* retrieve the messageDigest value to be compared to the passed in
         * content message digest */
        ASN1_ITEMPTR pMDItem;

        if (OK > (status = ASN1_GetChildWithOID(pAuthenticatedAttributes, s, pkcs9_messageDigest_OID, &pMDItem)))
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }
        if (NULL == pMDItem)
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }
        /* message digest value is 2nd child of the attribute*/
        if (NULL == (pMDItem = ASN1_NEXT_SIBLING(pMDItem)))
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }
        if (NULL == (pMDItem = ASN1_FIRST_CHILD(pMDItem)))
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }

        messageDigestLen = pMDItem->length;
        messageDigest = (ubyte*) CS_memaccess( s, pMDItem->dataOffset, pMDItem->length);

        if (messageDigestLen != pHashInfo->hashAlgo->digestSize)
        {
            status = ERR_PKCS7_INVALID_SIGNATURE;
            goto exit;
        }
        /* compare both content digest and authenticatedAttribute digest */
        MOC_MEMCMP( pHashInfo->hashData, messageDigest, messageDigestLen, &cmpResult);
        if ( cmpResult)
        {
            status = ERR_PKCS7_INVALID_SIGNATURE;
            goto exit;
        }

        /* verify the other mandatory attribute is there */
        if (OK > ASN1_GetChildWithOID( pAuthenticatedAttributes, s, pkcs9_contentType_OID,
                                            &pMDItem))
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }
        if (NULL == pMDItem)
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }
        /* move to attribute value */
        if (NULL == (pMDItem = ASN1_NEXT_SIBLING(pMDItem)))
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }
        if (NULL == (pMDItem = ASN1_FIRST_CHILD(pMDItem)))
        {
            status = ERR_PKCS7_MISSING_AUTH_ATTRIBUTE;
            goto exit;
        }

        /* and it matches the pContentType */
        if (OK > ASN1_CompareItems( pContentType, s, pMDItem, s))
        {
            status = ERR_PKCS7_INVALID_SIGNATURE;
            goto exit;
        }

        /* From PKCS#7: The Attributes value's tag is SET OF,
         * and the DER encoding of the SET OF tag,
         * rather than of the IMPLICIT [0] tag,
         * is to be digested along with the length
         * and contents octets of the Attributes value. */
        dataBufLen = pAuthenticatedAttributes->length;

        dataBuf = (ubyte*) CS_memaccess( s,
                                        pAuthenticatedAttributes->dataOffset,
                                        dataBufLen);
        if ( 0 == dataBuf)
        {
            return ERR_MEM_ALLOC_FAIL;
        }

        /* create a SET OF structure */
        if (OK > (status = DER_AddItem(NULL, CONSTRUCTED|SET, dataBufLen, dataBuf, &pSetOf)))
            goto exit;

        if (OK > (status = DER_Serialize(pSetOf, &attr, &attrLen)))
            goto exit;

        if (OK > (status = CRYPTO_ALLOC(hwAccelCtx,
                                        attrLen + pHashInfo->hashAlgo->digestSize,
                                        TRUE, &pTempBuf)))
        {
            goto exit;
        }

        if (attrLen > 0)
        {
            MOC_MEMCPY(pTempBuf, attr, attrLen);
        }

        pHashInfo->hashAlgo->initFunc( MOC_HASH(hwAccelCtx) pHashInfo->bulkCtx);
        pHashInfo->hashAlgo->updateFunc( MOC_HASH(hwAccelCtx) pHashInfo->bulkCtx,
                                        pTempBuf, attrLen);
        pHashInfo->hashAlgo->finalFunc( MOC_HASH(hwAccelCtx) pHashInfo->bulkCtx,
                                        pTempBuf + attrLen);

        pHashResult = pTempBuf + attrLen;
    }
    else
    {
        pHashResult = pHashInfo->hashData;
    }
    /* verify the signature now */
    if ( certKey.type == akt_rsa)
    {
        if ( OK > ( status = PKCS7_VerifyRSASignature(MOC_RSA(hwAccelCtx) s,
                                            pEncryptedDigest,
                                            certKey.key.pRSA,
                                            pHashResult,
                                            pHashInfo->hashAlgo->digestSize,
                                            pHashInfo->hashType)))
        {
            goto exit;
        }
    }
#ifdef __ENABLE_MOCANA_DSA__
    else if ( certKey.type == akt_dsa)
    {
        if ( OK > ( status = PKCS7_VerifyDSASignature(s,
                                            pEncryptedDigest,
                                            certKey.key.pDSA,
                                            pHashResult,
                                            pHashInfo->hashAlgo->digestSize,
                                            pHashInfo->hashType)))
        {
            goto exit;
        }
    }
#endif
#ifdef __ENABLE_MOCANA_ECC__
    else if ( certKey.type == akt_ecc)
    {
        if ( OK > ( status = PKCS7_VerifyECDSASignature(s,
                                            pEncryptedDigest,
                                            certKey.key.pECC,
                                            pHashResult,
                                            pHashInfo->hashAlgo->digestSize,
                                            pHashInfo->hashType)))
        {
            goto exit;
        }
    }
#endif

    *ppCertificate = pCertificate;

    if (pSigInfo)
    {
        pSigInfo->pASN1 = pSignerInfo;
        pSigInfo->msgSigDigestLen = pHashInfo->hashAlgo->digestSize;
        MOC_MEMCPY( pSigInfo->msgSigDigest, pHashResult,  pHashInfo->hashAlgo->digestSize);
    }

exit:

    if (pTempBuf)
    {
        CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
    }

    if ( messageDigest)
    {
        CS_stopaccess( s, messageDigest);
    }

    if ( dataBuf)
    {
        CS_stopaccess( s, dataBuf);
    }

    if (attr)
    {
        FREE(attr);
    }

    if (pSetOf)
    {
        TREE_DeleteTreeItem((TreeItem*) pSetOf);
    }

    CRYPTO_uninitAsymmetricKey(&certKey, NULL);

    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS7_VerifySignedData(MOC_RSA_HASH(hwAccelDescr hwAccelCtx)  ASN1_ITEM* pSignedData, CStream s,
                       /* getCertFun can be NULL, if certificates
                        * are included in signedData
                       */
                        PKCS7_GetCertificate getCertFun,
                        PKCS7_ValidateRootCertificate valCertFun,
                        sbyte4* numKnownSigners)
{
    return PKCS7_VerifySignedDataEx(MOC_RSA_HASH(hwAccelCtx) pSignedData, s,
                                    getCertFun, valCertFun, NULL, 0,
                                    numKnownSigners);

}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GetDigestAlgorithmHash(ASN1_ITEMPTR pDigestAlgorithm, CStream s,
                                 ubyte4* pHashes)
{
    ASN1_ITEMPTR pDigestAlgorithmOID;
    ubyte hashType = 0;
    const BulkHashAlgo* pBHA;

    if (OK > ASN1_VerifyType( pDigestAlgorithm, SEQUENCE))
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    pDigestAlgorithmOID = ASN1_FIRST_CHILD( pDigestAlgorithm);
    if ( 0 == pDigestAlgorithmOID ||
        OK > ASN1_VerifyType( pDigestAlgorithmOID, OID) )
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    /* get the hash type and make sure it can be instantiated */
    if (OK <= PKCS7_GetHashAlgoIdFromHashAlgoOID( pDigestAlgorithmOID, s,
                                                  &hashType) &&
        OK <= CRYPTO_getRSAHashAlgo( hashType, &pBHA) )
    {
        (*pHashes) |= (1 << hashType);
    }

    return OK;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_CollectHashAlgos(MOC_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEMPTR pDigestAlgorithms,
                       CStream s, ubyte4* numAlgos, SignedDataHash** ppHashes)
{
    MSTATUS status;
    ASN1_ITEMPTR pDigestAlgorithm;
    ubyte4 hashes;

    hashes = 0;
    /* generate hash of content info for each digest algorithms we know about ?*/
    pDigestAlgorithm = ASN1_FIRST_CHILD(pDigestAlgorithms);
    while ( pDigestAlgorithm) /* can be empty */
    {
        if (OK > (status = PKCS7_GetDigestAlgorithmHash(pDigestAlgorithm, s,
                                                        &hashes)))
        {
            goto exit;
        }
        pDigestAlgorithm = ASN1_NEXT_SIBLING( pDigestAlgorithm);
    }

    if (OK > ( status = PKCS7_ConstructHashes( MOC_HASH(hwAccelCtx) hashes,
                                                numAlgos, ppHashes)))
    {
        goto exit;
    }

exit:

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_VerifySignatures(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEMPTR pSignedData, ASN1_ITEMPTR pContentInfo,
                       CStream s, ubyte4 numHashes,
                       SignedDataHash* pSignedDataHash,
                       PKCS7_GetCertificate getCertFun,
                       PKCS7_ValidateRootCertificate valCertFun,
                       ubyte4* pNumSigners, PKCS7_SignatureInfo **ppSigInfos)
{
    MSTATUS status;
    ASN1_ITEMPTR pCertificates;
    CStream certificatesStream;
    ASN1_ITEMPTR pSignerInfos, pSignerInfo, pNextSibling;
    ASN1_ITEMPTR pCertificatesRoot = 0;
    ubyte4 numSigners, numValidSigners = 0;
    PKCS7_SignatureInfo* pSigInfos = 0;

    /* now go to the certificates part if any/need to validate which one we accept */
    /* certificates is an optional part with tag [0] SET or [2] SEQUENCE */
    if (OK > (status = ASN1_GetChildWithTag(pSignedData, 0, &pCertificates)))
        goto exit;

    if ( 0 == pCertificates)
    {
        ASN1_GetChildWithTag( pSignedData, 2, &pCertificates);
    }
    certificatesStream = s;

    /* now go to the signer Info: this is the last child of the sequence */
    pSignerInfos = ASN1_NEXT_SIBLING( pContentInfo);
    if ( 0 == pSignerInfos)
    {
        /* must be at least one */
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    pNextSibling = ASN1_NEXT_SIBLING( pSignerInfos);
    /* if the last child is EOC, it is the second to last child */
    while ( pNextSibling &&
        !(pNextSibling->tag == 0 && pNextSibling->id == 0 && pNextSibling->length==0))
    {
        pSignerInfos = pNextSibling;
        pNextSibling = ASN1_NEXT_SIBLING( pSignerInfos);
    }

    if ( OK > ASN1_VerifyType( pSignerInfos, SEQUENCE) &&
        OK > ASN1_VerifyType( pSignerInfos, SET))
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    /* if the caller requested an array with the signer infos, allocate it */
    if (ppSigInfos)
    {
        numSigners = PKCS7_getNumberChildren( pSignerInfos);
        pSigInfos = (PKCS7_SignatureInfo*) MALLOC( numSigners * sizeof (PKCS7_SignatureInfo));
        if (!pSigInfos)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
    }

    /* go through all of them to see which signer we accept */
    pSignerInfo = ASN1_FIRST_CHILD( pSignerInfos);
    while (pSignerInfo)
    {
        /* ignore end-of-content octets (for interop with Symantec) */
        if(0 == pSignerInfo->tag && 0 == pSignerInfo->length)
        {
            goto skip;
        }

        ASN1_ITEMPTR pSignerInfoCert = 0;
        ASN1_ITEMPTR pCertificatesTemp = 0;
        CStream certificatesStreamTemp;

        if (0 == pCertificates)
        {
            /* signer certificates are not included in signedData,
            * need to load them from other means
            */
            ASN1_ITEMPTR pIssuerAndSerialNumber,
                        pIssuer, pSerialNumber;

            ASN1_GetNthChild(pSignerInfo, 2, &pIssuerAndSerialNumber);
            if (!pIssuerAndSerialNumber)
            {
                status = ERR_PKCS7_INVALID_STRUCT;
                goto exit;
            }
            pIssuer = ASN1_FIRST_CHILD(pIssuerAndSerialNumber);
            pSerialNumber = ASN1_NEXT_SIBLING(pIssuer);
            if ( OK > (status = getCertFun(MOC_RSA(hwAccelCtx) s, pSerialNumber, pIssuer,
                                &pCertificatesRoot, &certificatesStreamTemp)))
            {
                goto exit;
            }
            pCertificatesTemp = ASN1_FIRST_CHILD(pCertificatesRoot);
        }
        else
        {
            pCertificatesTemp = pCertificates;
            certificatesStreamTemp = certificatesStream;
        }
        if ( OK > (status =
                PKCS7_ProcessSignerInfo(MOC_RSA_HASH(hwAccelCtx) s,
                                        pSignerInfo, certificatesStreamTemp,
                                        pCertificatesTemp,
                                        ASN1_FIRST_CHILD(pContentInfo),
                                        numHashes, pSignedDataHash,
                                        (pSigInfos) ? pSigInfos + numValidSigners : NULL,
                                        &pSignerInfoCert)))
        {
            goto exit;
        }

        if (OK == PKCS7_ValidateCertificate(certificatesStreamTemp,
                                            pSignerInfoCert, pCertificates,
                                            valCertFun))
        {
            ++numValidSigners;
        }

skip:
        pSignerInfo = ASN1_NEXT_SIBLING( pSignerInfo);
    }

    *pNumSigners = numValidSigners;
    if (ppSigInfos)
    {
        *ppSigInfos = pSigInfos;
    }
    pSigInfos = 0;

exit:
    if (pSigInfos)
        FREE(pSigInfos);

    if (pCertificatesRoot)
    {
        TREE_DeleteTreeItem((TreeItem*)pCertificatesRoot);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS7_VerifySignedDataEx(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pSignedData, CStream s,
                       /* getCertFun can be NULL, if certificates
                        * are included in signedData
                       */
                        PKCS7_GetCertificate getCertFun,
                        PKCS7_ValidateRootCertificate valCertFun,
                        const ubyte* payLoad, /* for detached signatures */
                        ubyte4 payLoadLen,
                        sbyte4* numKnownSigners)
{
    /*
    SignedData ::= SEQUENCE {
          version         INTEGER {sdVer1(1), sdVer2(2)} (sdVer1 | sdVer2),
          digestAlgorithms
                          DigestAlgorithmIdentifiers,
          contentInfo     ContentInfo,
          certificates CHOICE {
            certSet       [0] IMPLICIT ExtendedCertificatesAndCertificates,
            certSequence  [2] IMPLICIT Certificates
          } OPTIONAL,
          crls CHOICE {
            crlSet        [1] IMPLICIT CertificateRevocationLists,
            crlSequence   [3] IMPLICIT CRLSequence
          } OPTIONAL,
          signerInfos     SignerInfos
        } (WITH COMPONENTS { ..., version (sdVer1),
             digestAlgorithms   (WITH COMPONENTS { ..., daSet PRESENT }),
             certificates       (WITH COMPONENTS { ..., certSequence ABSENT }),
             crls               (WITH COMPONENTS { ..., crlSequence ABSENT }),
             signerInfos        (WITH COMPONENTS { ..., siSet PRESENT })
           } |
           WITH COMPONENTS { ..., version (sdVer2),
              digestAlgorithms  (WITH COMPONENTS { ..., daSequence PRESENT }),
              certificates      (WITH COMPONENTS { ..., certSet ABSENT }),
              crls              (WITH COMPONENTS { ..., crlSet ABSENT }),
              signerInfos       (WITH COMPONENTS { ..., siSequence PRESENT })
        })
    */

    static WalkerStep gotoPKCS7ContentInfoToContent[] =
    {
        { GoNthChild, 2, 0},
        { VerifyTag, 0, 0 },
        { GoFirstChild, 0, 0},
        { Complete, 0, 0}
    };

    ASN1_ITEMPTR pVersion;
    ASN1_ITEMPTR pDigestAlgorithms;
    ASN1_ITEMPTR pContentInfo;

    ASN1_ITEMPTR pContent;
    ubyte* toHash;

    /* we will compute multiple hashes possibly */
    ubyte4 i, numHashes = 0;
    SignedDataHash* pSignedDataHash = 0;
    ubyte *pTempBuf = 0;
    MSTATUS status = OK;

    *numKnownSigners = 0;

    pVersion = ASN1_FIRST_CHILD( pSignedData);
    if ( 0 == pVersion)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    pDigestAlgorithms = ASN1_NEXT_SIBLING( pVersion);
    if ( 0 == pDigestAlgorithms)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }
    pContentInfo = ASN1_NEXT_SIBLING( pDigestAlgorithms);
    if ( 0 == pContentInfo)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    if (OK > ( status = PKCS7_CollectHashAlgos(MOC_HASH(hwAccelCtx)
                                                pDigestAlgorithms, s,
                                                &numHashes, &pSignedDataHash)))
    {
        goto exit;
    }


    if (numHashes)
    {
        status = ASN1_WalkTree( pContentInfo, s, gotoPKCS7ContentInfoToContent,
                                    &pContent);
        if (OK == status)
        {
            if (pContent->indefinite && ASN1_FIRST_CHILD(pContent))
            {
                ASN1_ITEMPTR pTemp;

                /* NOTE:The PKCS#7 EncryptedContent is specified as an octet string, but
                * SCEP entities must also accept a sequence of octet strings as a valid
                * alternate encoding.
                * This alternate encoding must be accepted wherever PKCS #7 Enveloped
                * Data is specified in this document.
                */
                pTemp = ASN1_FIRST_CHILD(pContent);
                /* accumulate octetstring content until reaching EOC */
                while (pTemp->length > 0)
                {
                    toHash = (ubyte*) CS_memaccess( s, pTemp->dataOffset, pTemp->length);
                    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, pTemp->length, TRUE, &pTempBuf)))
                        goto exit;
                    MOC_MEMCPY(pTempBuf, toHash, pTemp->length);
                    for (i = 0; i < numHashes; ++i)
                    {
                        pSignedDataHash[i].hashAlgo->updateFunc( MOC_HASH(hwAccelCtx)
                                                            pSignedDataHash[i].bulkCtx,
                                                            pTempBuf, pTemp->length);
                    }
                    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
                    pTempBuf = NULL;
                    pTemp = ASN1_NEXT_SIBLING(pTemp);
                    CS_stopaccess( s, toHash);
                }
            }
            else
            {
                toHash = (ubyte*) CS_memaccess( s, pContent->dataOffset, pContent->length);
                if (pContent->length > 0)
                {
                    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, pContent->length, TRUE, &pTempBuf)))
                        goto exit;

                    MOC_MEMCPY(pTempBuf, toHash, pContent->length);
                }
                for (i = 0; i < numHashes; ++i)
                {
                    pSignedDataHash[i].hashAlgo->updateFunc( MOC_HASH(hwAccelCtx)
                                                           pSignedDataHash[i].bulkCtx,
                                                           pTempBuf, pContent->length);
                }

                CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
                pTempBuf = NULL;
                CS_stopaccess( s, toHash);
            }
        }
        else
        {
            /* external signature. content should be provided in payLoad */
            if (payLoad && payLoadLen > 0)
            {
                if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, payLoadLen, TRUE, &pTempBuf)))
                    goto exit;

                MOC_MEMCPY(pTempBuf, payLoad, payLoadLen);
                for (i = 0; i < numHashes; ++i)
                {
                    pSignedDataHash[i].hashAlgo->updateFunc( MOC_HASH(hwAccelCtx)
                                                           pSignedDataHash[i].bulkCtx,
                                                           pTempBuf, payLoadLen);

                }
                CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
                pTempBuf = NULL;
            } else
            {
                status = ERR_PKCS7_NO_CONTENT;
                goto exit;
            }
        }

        for (i = 0; i < numHashes; ++i)
        {
            pSignedDataHash[i].hashAlgo->finalFunc( MOC_HASH(hwAccelCtx)
                                                    pSignedDataHash[i].bulkCtx,
                                                    pSignedDataHash[i].hashData);

        }
    }
    else if ( ASN1_FIRST_CHILD(pDigestAlgorithms))
    {
        /* there were some hashes but we didn't recognize/support any! */
        status = ERR_PKCS7_UNSUPPORTED_DIGESTALGO;
        goto exit;
    }

    if (OK > (status = PKCS7_VerifySignatures( MOC_RSA_HASH(hwAccelCtx) pSignedData, pContentInfo, s,
                                               numHashes, pSignedDataHash,
                                               getCertFun, valCertFun,
                                               (ubyte4*)numKnownSigners, NULL)))
    {
        goto exit;
    }

exit:
    if (pTempBuf)
        CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);

    if (numHashes)
        PKCS7_DestructHashes(MOC_HASH(hwAccelCtx) numHashes, &pSignedDataHash);

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GetIssuerSerialNumber( ASN1_ITEMPTR pIssuerSerialNumber,
                            CMSIssuerSerialNumber* pISN)
{
/*
  IssuerAndSerialNumber ::= SEQUENCE {
     issuer Name,
     serialNumber CertificateSerialNumber }
*/

    pISN->pIssuer = ASN1_FIRST_CHILD( pIssuerSerialNumber);
    if (!pISN->pIssuer || OK > ASN1_VerifyType( pISN->pIssuer, SEQUENCE))
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }

    pISN->pSerialNumber = ASN1_NEXT_SIBLING( pISN->pIssuer);
    if (!pISN->pSerialNumber)
    {
        return ERR_PKCS7_INVALID_STRUCT;
    }
    return OK;
}


/*--------------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_ECC__

static MSTATUS
PKCS7_GetOriginatorPublicKey( ASN1_ITEMPTR pSequence,
                              CMSOriginatorPublicKey* pOriginatorKey)
{
    ASN1_ITEMPTR pTemp;
    CStream dummyStream = {0, 0}; /* the walk steps will not access the stream */
/*
  OriginatorPublicKey ::= SEQUENCE {
     algorithm AlgorithmIdentifier,
     publicKey BIT STRING }
  AlgorithmIdentifier  ::=  SEQUENCE  {
     algorithm   OBJECT IDENTIFIER,
     parameters  ANY DEFINED BY algorithm OPTIONAL  }
*/
    static WalkerStep walkInstructions1[] =
    {
        { VerifyType, SEQUENCE, 0 },
        { GoFirstChild, 0, 0},    /* version */
        { VerifyType, SEQUENCE, 0 },
        { GoFirstChild, 0, 0 },
        { VerifyType, OID, 0 },
        { Complete, 0, 0}
    };
    static WalkerStep walkInstructions2[] =
    {
        { GoParent, 0, 0 },
        { GoNextSibling, 0, 0},    /* version */
        { VerifyType, BITSTRING, 0 },
        { Complete, 0, 0}
    };

    if (OK > ASN1_WalkTree( pSequence, dummyStream, walkInstructions1, &pTemp))
        return ERR_PKCS7_INVALID_STRUCT;

    pOriginatorKey->pAlgoOID = pTemp;

    pTemp = ASN1_NEXT_SIBLING(pTemp);

    if (!pTemp) { return ERR_PKCS7_INVALID_STRUCT; }

    pOriginatorKey->pAlgoParameters = pTemp;

    if (OK > ASN1_WalkTree( pTemp, dummyStream, walkInstructions2, &pTemp))
        return ERR_PKCS7_INVALID_STRUCT;

    pOriginatorKey->pPublicKey = pTemp;

    return OK;
}

#endif

/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_ProcessKeyTransRecipientInfo(MOC_RSA(hwAccelDescr hwAccelCtx)
                                   ASN1_ITEMPTR pKeyTransRecipientInfo, CStream s,
                                   PKCS7_GetPrivateKey getPrivateKeyFun,
                                   CMS_GetPrivateKey getPrivateKeyFunEx,
                                   ubyte** ppSymmetricKey, ubyte4* pSymmetricKeyLen)
{
    MSTATUS         status;
    AsymmetricKey   asymmetricKey;
    ubyte*          pSymmetricKey = 0;
    CMSRecipientId  recipientId;
    ASN1_ITEMPTR    pRecipientIdentifier;
    ASN1_ITEMPTR    pEncryptedKey;
    ASN1_ITEMPTR    pTemp;
    ubyte*          cipherText;
    sbyte4          cipherMaxLen;
    RSAKey*         pRSAKey;

    if (OK > ( status = CRYPTO_initAsymmetricKey( &asymmetricKey)))
        goto exit;
/*
  KeyTransRecipientInfo ::= SEQUENCE {
     version CMSVersion,  -- always set to 0 or 2
     rid RecipientIdentifier,
     keyEncryptionAlgorithm KeyEncryptionAlgorithmIdentifier,
     encryptedKey EncryptedKey }

  RecipientIdentifier ::= CHOICE {
     issuerAndSerialNumber IssuerAndSerialNumber,
     subjectKeyIdentifier [0] SubjectKeyIdentifier }

*/

    status = ASN1_GetNthChild( pKeyTransRecipientInfo, 2, &pRecipientIdentifier);
    if ( status < OK) goto exit;

    recipientId.type = NO_TAG;

    if (OK <= ASN1_VerifyType( pRecipientIdentifier, SEQUENCE))
    {
        /* pTemp = issuer and serial number */
        if (OK > PKCS7_GetIssuerSerialNumber(pRecipientIdentifier,
                    &recipientId.ri.ktrid.u.issuerAndSerialNumber))
        {
            goto exit;
        }
        recipientId.ri.ktrid.type = NO_TAG;
    }
    else if (OK <= ASN1_GetTag( pRecipientIdentifier, &recipientId.ri.ktrid.type) &&
                0 == recipientId.ri.ktrid.type)
    {
        /* temp's 1st child = subject key identifier */
        pTemp = ASN1_FIRST_CHILD( pRecipientIdentifier);
        if (OK >  ASN1_VerifyType( pTemp, OCTETSTRING))
        {
            status = ERR_PKCS7_INVALID_STRUCT;
            goto exit;
        }
        recipientId.ri.ktrid.u.subjectKeyIdentifier = pTemp;
    }
    else
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    if (getPrivateKeyFun && NO_TAG == recipientId.ri.ktrid.type)
    {
        if ( OK > (*getPrivateKeyFun)(s,
                    recipientId.ri.ktrid.u.issuerAndSerialNumber.pSerialNumber,
                    recipientId.ri.ktrid.u.issuerAndSerialNumber.pIssuer,
                    &asymmetricKey))
        {
            status = ERR_FALSE;
            goto exit;
        }
    }
    else if (getPrivateKeyFunEx)
    {
        if ( OK > (*getPrivateKeyFunEx)(s, &recipientId, &asymmetricKey))
        {
            status = ERR_FALSE;
            goto exit;
        }
    }
    else
    {
        status = ERR_PKCS7_WRONG_CALLBACK;
        goto exit;
    }

    if ( akt_rsa != asymmetricKey.type)
    {
        status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
        goto exit;
    }

    /* use the RSA key to decrypt the symmetric key */
    pRSAKey = asymmetricKey.key.pRSA;

    status = RSA_getCipherTextLength(pRSAKey, &cipherMaxLen);
    if (status < OK) goto exit;

    pSymmetricKey = (ubyte*) MALLOC( cipherMaxLen);
    if (NULL == pSymmetricKey)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    status =  ASN1_GetNthChild( pKeyTransRecipientInfo, 4, &pEncryptedKey);
    if ( status < OK) goto exit;

    cipherText = (ubyte*) CS_memaccess( s,
                                        pEncryptedKey->dataOffset,
                                        pEncryptedKey->length);
    if ( 0 == cipherText)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* decrypt the encrypted symmetric key with the RSA key */
    status = RSA_decrypt(MOC_RSA( hwAccelCtx)
                     pRSAKey, cipherText,
                     pSymmetricKey, pSymmetricKeyLen,
                     NULL, 0,
                     NULL);

    CS_stopaccess( s, cipherText);

    if (OK <= status)
    {
        *ppSymmetricKey = pSymmetricKey;
        pSymmetricKey = 0;
    }

exit:

    CRYPTO_uninitAsymmetricKey(&asymmetricKey, NULL);

    if (pSymmetricKey)
    {
        FREE(pSymmetricKey);
    }

    return status;
}


#ifdef __ENABLE_MOCANA_ECC__
/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_ProcessKeyAgreeRecipientInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                   ASN1_ITEMPTR pRecipientInfo, CStream s,
                                   PKCS7_GetPrivateKey getPrivateKeyFun,
                                   CMS_GetPrivateKey getPrivateKeyFunEx,
                                   ubyte** ppSymmetricKey,
                                   ubyte4* pSymmetricKeyLen)
{
    MSTATUS status;
    AsymmetricKey privateKey;
    AsymmetricKey ephemeralKey;
    ASN1_ITEMPTR pTemp, pTemp1, pEncryptedKey, pUKM;
    ASN1_ITEMPTR pECCBitString, pKeyEncryptionAlgo;
    CMSRecipientId recipientId;
    const ubyte* ukmData = 0;
    ubyte4 ukmLen = 0;
    const ubyte* point = 0;
    const ubyte* keyWrapOID = 0;
    const ubyte* encryptedKey = 0;
    const BulkHashAlgo* pHashAlgo;

    static WalkerStep ecPublicKeyBitStringWalkInstructions[] =
    {
        { GoFirstChild, 0, 0},    /* version */
        { VerifyInteger, 3, 0},   /* must always be 3 */
        { GoNextSibling, 0, 0},   /* OriginatorIdentifierOrKey */
        { VerifyTag, 0, 0 },      /* [0] */
        { GoFirstChild, 0, 0 },
        { VerifyTag, 1, 0 },      /* [1] -> originatorKey */
        { GoFirstChild, 0, 0 },
        { VerifyType, SEQUENCE, 0 },
        { GoFirstChild, 0, 0 },
        { VerifyOID, 0, (ubyte*) ecPublicKey_OID },
        { GoParent, 0, 0 },
        { GoNextSibling, 0, 0 },
        { VerifyType, BITSTRING, 0 },
        { Complete, 0, 0}
    };

    if (OK > ( status = CRYPTO_initAsymmetricKey( &privateKey)))
        goto exit;

    if (OK > ( status = CRYPTO_initAsymmetricKey( &ephemeralKey)))
        goto exit;

    if (OK > ( status = ASN1_WalkTree(pRecipientInfo, s,
             ecPublicKeyBitStringWalkInstructions, &pECCBitString)))
    {
        goto exit;
    }

    /* check if there's an ukm present */
    if (OK > ( status = ASN1_GetChildWithTag( pRecipientInfo, 1, &pUKM)))
    {
        goto exit;
    }

    /* !!!!!!!!!!! default status for exit !!!!!!!!!!!!!!!*/
    status = ERR_PKCS7_INVALID_STRUCT;

    if ( OK > ASN1_GetNthChild( pRecipientInfo, pUKM ? 4 : 3, &pKeyEncryptionAlgo))
    {
        goto exit;
    }

    pTemp1 = ASN1_NEXT_SIBLING( pKeyEncryptionAlgo);
    if ( !pTemp1)
    {
        goto exit;
    }

    /* pTemp1 is the last child: recipientEncryptedKeys */
    pTemp1 = ASN1_FIRST_CHILD( pTemp1);
    if (!pTemp1)
    {
        goto exit;
    }

    /* pTemp1 -> recipientEncryptedKey */
    pTemp1 = ASN1_FIRST_CHILD( pTemp1);
    if (!pTemp1)
    {
        goto exit;
    }

    recipientId.type = 1;

    if (OK <= ASN1_VerifyType( pTemp1, SEQUENCE))
    {
        /* pTemp1 is IssuerAndSerialNumber */
        if (OK > PKCS7_GetIssuerSerialNumber(pTemp1,
                    &recipientId.ri.karid.u.issuerAndSerialNumber))
        {
            goto exit;
        }
        recipientId.ri.karid.type = NO_TAG;
    }
    else if ( OK <= ASN1_GetTag( pTemp1, &recipientId.ri.karid.type))
    {
        pTemp = ASN1_FIRST_CHILD( pTemp1);
        if (!pTemp)
        {
            goto exit;
        }

        if ( 0 == recipientId.ri.karid.type)
        {
            if (OK > ASN1_VerifyType( pTemp, OCTETSTRING))
            {
                goto exit;
            }
            recipientId.ri.karid.u.subjectKeyIdentifier = pTemp;
        }
        else if ( 1 ==recipientId.ri.karid.type)
        {
            if (OK > PKCS7_GetOriginatorPublicKey( pTemp,
                                    &recipientId.ri.karid.u.originatorKey))
            {
                goto exit;
            }
        }
        else
        {
            status = ERR_FALSE; /* don't understand this type */
            goto exit;
        }
    }
    else
    {
         goto exit;
    }

    pEncryptedKey = ASN1_NEXT_SIBLING( pTemp1);

    if (getPrivateKeyFun && NO_TAG == recipientId.ri.karid.type)
    {
        if ( OK > (*getPrivateKeyFun)(s,
                    recipientId.ri.karid.u.issuerAndSerialNumber.pSerialNumber,
                    recipientId.ri.karid.u.issuerAndSerialNumber.pIssuer,
                    &privateKey))
        {
            status = ERR_FALSE;
            goto exit;
        }
    }
    else if (getPrivateKeyFunEx)
    {
        if ( OK > (*getPrivateKeyFunEx)(s, &recipientId, &privateKey))
        {
            status = ERR_FALSE;
            goto exit;
        }
    }
    else
    {
        status = ERR_PKCS7_WRONG_CALLBACK;
        goto exit;
    }

    if ( akt_ecc != privateKey.type || !(privateKey.key.pECC->privateKey))
    {
        status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
        goto exit;
    }

    /* read the public key -- make sure it's the same curve as the private key */
    point = CS_memaccess( s, pECCBitString->dataOffset, pECCBitString->length);
    if (!point)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = CRYPTO_setECCParameters( &ephemeralKey, CRYPTO_getECCurveId(&privateKey),
                                                point, pECCBitString->length, NULL, 0, NULL)))
    {
        goto exit;
    }

    /* figure out the hash algo and the key wrap OID */
    pTemp1 = ASN1_FIRST_CHILD( pKeyEncryptionAlgo);
    if ( OK <= ASN1_VerifyOID( pTemp1, s, dhSinglePassStdDHSha1KDF_OID))
    {
        if (OK > ( status = CRYPTO_getRSAHashAlgo( ht_sha1, &pHashAlgo)))
            goto exit;
    }
    else if ( OK <= ASN1_VerifyOID( pTemp1, s, dhSinglePassStdDHSha224KDF_OID))
    {
        if (OK > ( status = CRYPTO_getRSAHashAlgo( ht_sha224, &pHashAlgo)))
            goto exit;
    }
    else if ( OK <= ASN1_VerifyOID( pTemp1, s, dhSinglePassStdDHSha256KDF_OID))
    {
        if (OK > ( status = CRYPTO_getRSAHashAlgo( ht_sha256, &pHashAlgo)))
            goto exit;
    }
    else if ( OK <= ASN1_VerifyOID( pTemp1, s, dhSinglePassStdDHSha384KDF_OID))
    {
        if (OK > ( status = CRYPTO_getRSAHashAlgo( ht_sha384, &pHashAlgo)))
            goto exit;
    }
    else if ( OK <= ASN1_VerifyOID( pTemp1, s, dhSinglePassStdDHSha512KDF_OID))
    {
        if (OK > ( status = CRYPTO_getRSAHashAlgo( ht_sha512, &pHashAlgo)))
            goto exit;
    }
    else
    {
        status = ERR_PKCS7_UNSUPPORTED_KDF;
        goto exit;
    }

    /* KeyWrapAlgorithm  ::=  AlgorithmIdentifier */
    pTemp1 = ASN1_NEXT_SIBLING(pTemp1);
    if (!pTemp1)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    pTemp1 = ASN1_FIRST_CHILD( pTemp1);

    if ( OK <= ASN1_VerifyOID( pTemp1, s, aes128Wrap_OID))
    {
        keyWrapOID = aes128Wrap_OID;
    }
    else if ( OK <= ASN1_VerifyOID( pTemp1, s, aes192Wrap_OID))
    {
        keyWrapOID = aes192Wrap_OID;
    }
    else if ( OK <= ASN1_VerifyOID( pTemp1, s, aes256Wrap_OID))
    {
        keyWrapOID = aes256Wrap_OID;
    }
    else
    {
        status = ERR_PKCS7_UNSUPPORTED_KEY_WRAP;
        goto exit;
    }

    if (pUKM)
    {
        ukmLen = pUKM->length;
        ukmData = CS_memaccess( s, pUKM->dataOffset, pUKM->length);
        if (!ukmData)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
    }

    encryptedKey = CS_memaccess( s, pEncryptedKey->dataOffset, pEncryptedKey->length);
    if (!encryptedKey)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > ( status = PKCS7_ECCDecryptKey(MOC_SYM_HASH(hwAccelCtx)
                                            pHashAlgo,
                                            ephemeralKey.key.pECC,
                                            privateKey.key.pECC->k, keyWrapOID,
                                            ukmData, ukmLen,
                                            encryptedKey, pEncryptedKey->length,
                                            ppSymmetricKey, pSymmetricKeyLen)))
    {
        goto exit;
    }

exit:

    if (encryptedKey)
    {
        CS_stopaccess( s, encryptedKey);
    }

    if (point)
    {
        CS_stopaccess( s, point);
    }

    if (ukmData)
    {
        CS_stopaccess( s, ukmData);
    }

    CRYPTO_uninitAsymmetricKey( &privateKey, NULL);
    CRYPTO_uninitAsymmetricKey( &ephemeralKey, NULL);

    return status;
}

#endif


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_ProcessRecipientInfos( MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pRecipientInfos,
                            CStream s,
                            PKCS7_GetPrivateKey getPrivateKeyFun,
                            CMS_GetPrivateKey getPrivateKeyFun2,
                            ubyte** ppSymmetricKey,
                            ubyte4* pSymmetricKeyLen,
                            sbyte4* recipientIndex)
{
    MSTATUS status = OK;
    ASN1_ITEMPTR pRecipientInfo;
    sbyte4 index = 0;

    /* find which recipient info corresponds to us */
    pRecipientInfo = ASN1_FIRST_CHILD( pRecipientInfos);
    if ( !pRecipientInfo) return ERR_PKCS7_INVALID_STRUCT; /* there must be at least one */

    while (pRecipientInfo)
    {
        if (OK <= ASN1_VerifyType( pRecipientInfo, SEQUENCE))
        {
            if (OK <= PKCS7_ProcessKeyTransRecipientInfo(MOC_RSA(hwAccelCtx)
                                                        pRecipientInfo, s,
                                                        getPrivateKeyFun,
                                                        getPrivateKeyFun2,
                                                        ppSymmetricKey,
                                                        pSymmetricKeyLen))
            {
               goto exit; /* found a match */
            }
        }
        else /* must be a tag */
        {
            ubyte4 tag;

            if (OK > ASN1_GetTag( pRecipientInfo, &tag))
            {
                status = ERR_PKCS7_INVALID_STRUCT;
                goto exit;
            }

            /* the sequence is implicit */
            switch( tag)
            {
            case 1:
#ifdef __ENABLE_MOCANA_ECC__
                 if (OK <= PKCS7_ProcessKeyAgreeRecipientInfo(MOC_SYM_HASH(hwAccelCtx)
                                                        pRecipientInfo, s,
                                                        getPrivateKeyFun,
                                                        getPrivateKeyFun2,
                                                        ppSymmetricKey,
                                                        pSymmetricKeyLen))
                {
                    goto exit;
                }
                break;
#endif
            case 2:
            case 3:
            case 4:
            default:
                break;
            }
        }
        pRecipientInfo = ASN1_NEXT_SIBLING( pRecipientInfo);
        ++index;
    }

    status = ERR_PKCS7_NO_RECIPIENT_KEY_MATCH;

exit:
    if (recipientIndex)
    {
        *recipientIndex = (OK > status)? -1 : index;
    }

    return status;
}



/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GetBulkAlgo( MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pContentEncryptAlgo,
                   CStream s,
                   ubyte* pSymmetricKey,
                   ubyte4 symmetricKeyLen,
                   ubyte* iv,
                   BulkCtx* pBulkCtx,
                   const BulkEncryptionAlgo** ppBulkAlgo)
{
    ASN1_ITEM* pEncryptedAlgoOID;
    ubyte encryptionSubType;
#ifdef __ENABLE_ARC2_CIPHERS__
    sbyte4          effectiveKeyBits;
#endif
    MSTATUS status;

    /* first child is the OID identifying the algorithm */
    pEncryptedAlgoOID = ASN1_FIRST_CHILD( pContentEncryptAlgo);
    if ( 0 == pEncryptedAlgoOID ||
            OK > ASN1_VerifyType( pEncryptedAlgoOID, OID))
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    /* determine the algorithm */
    /* we support 3DES_EDE_CBC, RC4, RC2 CBC initially add more as necesssary */
    status = ASN1_VerifyOIDRoot(pEncryptedAlgoOID, s,
                    rsaEncryptionAlgoRoot_OID, &encryptionSubType);

    if (OK == status ) /* match */
    {
        switch ( encryptionSubType)
        {
#ifdef __ENABLE_ARC2_CIPHERS__
        case 2: /* RC2CBC*/
            if (OK > (status = PKCS_GetRC2CBCParams( pEncryptedAlgoOID, s,
                &effectiveKeyBits, iv)))
            {
                goto exit;
            }
            *ppBulkAlgo = &CRYPTO_RC2EffectiveBitsSuite;
            /* special createFunc for RC2 that allows effective keyBits */
            *pBulkCtx = CreateRC2Ctx2(MOC_SYM(hwAccelCtx) pSymmetricKey,
                                            symmetricKeyLen, effectiveKeyBits);
            break;
#endif

#ifndef __DISABLE_ARC4_CIPHERS__
        case 4: /* RC4 */
            /* no parameter */
            *ppBulkAlgo = &CRYPTO_RC4Suite;
            *pBulkCtx = CreateRC4Ctx(MOC_SYM(hwAccelCtx) pSymmetricKey,
                                        symmetricKeyLen, 0);
            break;
#endif

#ifndef __DISABLE_3DES_CIPHERS__
        case 7: /* desEDE3CBC */
            /* iv OCTET STRING (SIZE(8)) */
            if (OK > (status = PKCS_GetCBCParams(pEncryptedAlgoOID, s,
                                                    DES_BLOCK_SIZE, iv)))
            {
                goto exit;
            }
            *ppBulkAlgo = &CRYPTO_TripleDESSuite;
            *pBulkCtx = Create3DESCtx(MOC_SYM(hwAccelCtx) pSymmetricKey,
                                            symmetricKeyLen, 0);

            break;
#endif

        default:
            status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
            goto exit;
            break;
        }
    }
    else
    {
        /* SCEP can use this */
#ifdef __ENABLE_DES_CIPHER__
        if ( OK == ASN1_VerifyOID(pEncryptedAlgoOID, s, desCBC_OID ))
        {
            /* iv OCTET STRING (SIZE(8)) */
            if ( OK > (status = PKCS_GetCBCParams(pEncryptedAlgoOID, s,
                                                    DES_BLOCK_SIZE, iv)))
            {
                goto exit;
            }
            *ppBulkAlgo = &CRYPTO_DESSuite;
            *pBulkCtx = CreateDESCtx(MOC_SYM(hwAccelCtx) pSymmetricKey,
                                            symmetricKeyLen, 0);
        }
        else
#endif
#ifndef __DISABLE_AES_CIPHERS__
        if ((OK == ASN1_VerifyOID(pEncryptedAlgoOID, s, aes128CBC_OID )) ||
            (OK == ASN1_VerifyOID(pEncryptedAlgoOID, s, aes192CBC_OID))  ||
            (OK == ASN1_VerifyOID(pEncryptedAlgoOID, s, aes256CBC_OID )))
        {
            if (OK > (status = PKCS_GetCBCParams(pEncryptedAlgoOID, s, AES_BLOCK_SIZE, iv)))
            {
                goto exit;
            }
            *ppBulkAlgo = &CRYPTO_AESSuite;
            *pBulkCtx = CreateAESCtx(MOC_SYM(hwAccelCtx) pSymmetricKey,
                symmetricKeyLen, 0);
        }
        else
        /* add others here if necessary */
#endif
        {
            status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
            goto exit;
        }
    }

    if (NULL == *pBulkCtx)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

exit:

    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS7_DecryptEnvelopedDataAux(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                  ASN1_ITEM* pEnvelopedData,
                  CStream s,
                  PKCS7_GetPrivateKey getPrivateKeyFun,
                  encryptedContentType* pType,
                  ASN1_ITEM** ppEncryptedContent,
                  BulkCtx* pBulkCtx,
                  const BulkEncryptionAlgo** ppBulkAlgo,
                  ubyte iv[/*16=MAX_IV_SIZE*/])
{
    ASN1_ITEMPTR    pVersion;
    ASN1_ITEMPTR    pRecipientInfos;
    ASN1_ITEMPTR    pEncryptedContentInfo, pContentEncryptAlgo;
    ubyte*          pSymmetricKey = 0;
    ubyte4          symmetricKeyLen = 0;
    MSTATUS         status;

    if ( NULL == pEnvelopedData|| NULL == pBulkCtx || NULL == ppBulkAlgo)
    {
        return ERR_NULL_POINTER;
    }

    pVersion = ASN1_FIRST_CHILD(pEnvelopedData);
    if ( !pVersion)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    pRecipientInfos = ASN1_NEXT_SIBLING( pVersion);
    if (!pRecipientInfos)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    pEncryptedContentInfo = ASN1_NEXT_SIBLING( pRecipientInfos);
    if ( ! pEncryptedContentInfo)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    /* look at the recipient infos and see if we are one of them */
    /* depending on the version of the PKCS#7 it's either a SET OF or a SEQUENCE OF */
    if ( OK > ASN1_VerifyType( pRecipientInfos, SET) &&
        OK > ASN1_VerifyType( pRecipientInfos, SEQUENCE))
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    /* check here the type of EncryptedContentInfo */
    if (OK > ASN1_VerifyType( pEncryptedContentInfo, SEQUENCE))
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    if (OK > ( status = PKCS7_ProcessRecipientInfos( MOC_RSA_SYM_HASH(hwAccelCtx)
                                    pRecipientInfos, s, getPrivateKeyFun, NULL,
                                    &pSymmetricKey, &symmetricKeyLen, NULL)))
    {
        goto exit;
    }

    if (!pSymmetricKey || !symmetricKeyLen)
    {
        status = ERR_PKCS7_NO_RECIPIENT_KEY_MATCH;
        goto exit;
    }

    /* content encryption -- what's encryted differs
     between PKCS#7 v1.5 and v1.6. This implements version 1.5 or RFC2315 */

    status = ASN1_GetNthChild( pEncryptedContentInfo, 2, &pContentEncryptAlgo);
    if ( status < OK) goto exit;

    if (OK > ( status = PKCS7_GetBulkAlgo(MOC_RSA_HASH(hwAccelCtx) pContentEncryptAlgo, s,
                    pSymmetricKey, symmetricKeyLen, iv, pBulkCtx,
                    ppBulkAlgo)))
    {
        goto exit;
    }

    /* now decrypt */
    *ppEncryptedContent = ASN1_NEXT_SIBLING(pContentEncryptAlgo);
    if ( 0 == (*ppEncryptedContent)) /* optional not an error */
    {
        goto exit;
    }

    /* encryptedContent [0] IMPLICIT OCTETSTRING OPTIONAL */
    if ( OK > ASN1_VerifyTag( *ppEncryptedContent, 0))
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    /* from SCEP draft:
    * NOTE:The PKCS#7 EncryptedContent is specified as an octet string, but
    * SCEP entities must also accept a sequence of octet strings as a valid
    * alternate encoding.
    */
    if ((*ppEncryptedContent)->indefinite &&
        ASN1_FIRST_CHILD(*ppEncryptedContent))
    {
        *pType = SCEP;
        *ppEncryptedContent = ASN1_FIRST_CHILD(*ppEncryptedContent);
    }
    else /*the content of [0] tag is the encrypted content */
    {
        *pType = NORMAL;
    }


exit:

    if (pSymmetricKey)
    {
        FREE(pSymmetricKey);
    }

    if (OK > status && NULL != *pBulkCtx)
    {
        /* assert( *ppBulkAlgo) */
        if (NULL != *ppBulkAlgo)
        {
            (*ppBulkAlgo)->deleteFunc(MOC_SYM(hwAccelCtx) pBulkCtx);
        }
    }

    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS PKCS7_DecryptEnvelopedData( MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                            ASN1_ITEM* pEnvelopedData,
                                            CStream s,
                                            PKCS7_GetPrivateKey getPrivateKeyFun,
                                            ubyte** decryptedInfo,
                                            sbyte4* decryptedInfoLen)
{
    ubyte           iv[16] = {0}; /* all supported algos have a 8 byte IV; aes has 16 byte */
    BulkCtx         pBulkCtx = NULL;
    const BulkEncryptionAlgo* pBulkAlgo = NULL;
    encryptedContentType    type;
    ASN1_ITEM* pEncryptedContent;
    MSTATUS         status;

    if ( NULL == pEnvelopedData|| NULL == decryptedInfo || NULL == decryptedInfoLen)
    {
        return ERR_NULL_POINTER;
    }

    if ( OK > ( status = PKCS7_DecryptEnvelopedDataAux( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pEnvelopedData, s, getPrivateKeyFun,
                                            &type,
                                            &pEncryptedContent, &pBulkCtx,
                                            &pBulkAlgo, iv)))
    {
        goto exit;
    }

    /* call the common routine */
    status = PKCS_BulkDecryptEx(MOC_SYM(hwAccelCtx) type, pEncryptedContent, s, pBulkCtx, pBulkAlgo,
                                iv, decryptedInfo, decryptedInfoLen);

exit:

    if (NULL != pBulkCtx)
    {
        /* assert( pBulkAlgo) */
        if (NULL != pBulkAlgo)
            pBulkAlgo->deleteFunc(MOC_SYM(hwAccelCtx) &pBulkCtx);
    }

    return status;
}

#ifdef __ENABLE_MOCANA_ECC__

/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GenerateECCCMSSharedInfo(const ubyte* keyInfoOID,
                               const ubyte* ukmData, ubyte4 ukmDataLen,
                               ubyte4 kekLen,
                               ubyte** sharedInfo, ubyte4 *sharedInfoLen)
{
    MSTATUS status;
    DER_ITEMPTR pTag, pSequence = 0;
    ubyte copyData[MAX_DER_STORAGE];

    if (OK > ( status = DER_AddSequence( NULL, &pSequence)))
    {
        goto exit;
    }

    if (OK > ( status = DER_StoreAlgoOID( pSequence, keyInfoOID, TRUE)))
    {
        goto exit;
    }

    if (ukmData)
    {
        if (OK > ( status = DER_AddTag( pSequence, 0, &pTag)))
        {
            goto exit;
        }
        if (OK > ( status = DER_AddItem( pTag, OCTETSTRING, ukmDataLen, ukmData, NULL)))
        {
            goto exit;
        }
    }

    kekLen = kekLen * 8; /* suppPubInfo length in bits */
    BIGEND32( copyData, kekLen);
    if (OK > ( status = DER_AddTag( pSequence, 2, &pTag)))
    {
        goto exit;
    }
    if (OK > ( status = DER_AddItemCopyData( pTag, OCTETSTRING, 4, copyData, NULL)))
    {
        goto exit;
    }

    /* Serialize */
    if (OK > ( status = DER_Serialize( pSequence, sharedInfo, sharedInfoLen)))
    {
        goto exit;
    }

exit:

    if (pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    return status;
}



/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GenerateECCKeyEncryptionKey(MOC_HASH(hwAccelDescr hwAccelCtx)
                    const BulkHashAlgo* pHashAlgo,
                    ECCKey* pECCKey, ConstPFEPtr k,
                    const ubyte* keyWrapOID,
                    const ubyte* ukmData, ubyte4 ukmDataLen,
                    ubyte4 kekLen, ubyte** p_kek)
{
    MSTATUS status;

    ubyte* sharedInfo = 0;
    ubyte4 sharedInfoLen;
    ubyte* z = 0;
    ubyte4 zLen;
    ubyte* kek = 0;

    /* do the DH operation to get Z shared secret */
    if ( OK > ( status = ECDH_generateSharedSecretAux(pECCKey->pCurve,
                            pECCKey->Qx, pECCKey->Qy, k, &z, (sbyte4*)&zLen, 1)))
    {
        goto exit;
    }

    /* generate the sharedInfo -> DER encoding of ECC-CMS-SharedInfo --
     the kekLen is identical to cekLen -- compatible with RFC 5008 */
    if (OK > ( status = PKCS7_GenerateECCCMSSharedInfo(keyWrapOID,
                               ukmData, ukmDataLen, kekLen,
                               &sharedInfo, &sharedInfoLen)))
    {
        goto exit;
    }

    kek = MALLOC( kekLen);
    if (!kek)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > ( status = ANSIX963KDF_generate( MOC_HASH( hwAccelCtx)
                                                pHashAlgo, z, zLen,
                                                sharedInfo, sharedInfoLen,
                                                kekLen, kek)))
    {
        goto exit;
    }

    *p_kek = kek;
    kek = 0;

exit:

    if ( z)
    {
        FREE( z);
    }

    if ( sharedInfo)
    {
        FREE(sharedInfo);
    }

    if ( kek)
    {
        FREE( kek);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_ECCEncryptKey(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                    const BulkHashAlgo* pHashAlgo,
                    ECCKey* pECCKey, ConstPFEPtr k,
                    const ubyte* keyWrapOID,
                    const ubyte* ukmData, ubyte4 ukmDataLen,
                    const ubyte* cek, ubyte4 cekLen,
                    ubyte** encryptedKey, ubyte4* encryptedKeyLen)
{
    MSTATUS status;

    ubyte* kek = 0;
    ubyte* wrappedKey = 0;

    if (OK > ( status = PKCS7_GenerateECCKeyEncryptionKey(
                    MOC_HASH(hwAccelCtx) pHashAlgo,
                    pECCKey, k, keyWrapOID, ukmData, ukmDataLen,
                    cekLen, &kek)))
    {
        goto exit;
    }

    wrappedKey = MALLOC( cekLen + 8);

    if ( OK > ( status = AESKWRAP_encrypt( MOC_SYM(hwAccelCtx) kek, cekLen,
                  cek, cekLen, wrappedKey)))
    {
        goto exit;
    }

    *encryptedKey = wrappedKey;
    *encryptedKeyLen = cekLen + 8;
    wrappedKey = 0;

exit:

    if ( wrappedKey)
    {
        FREE( wrappedKey);
    }

    if ( kek)
    {
        FREE( kek);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_ECCDecryptKey(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                    const BulkHashAlgo* pHashAlgo,
                    ECCKey* pECCKey, ConstPFEPtr k,
                    const ubyte* keyWrapOID,
                    const ubyte* ukmData, ubyte4 ukmDataLen,
                    const ubyte* encryptedKey, ubyte4 encryptedKeyLen,
                    ubyte** cek, ubyte4* cekLen)
{
    MSTATUS status;
    ubyte* kek = 0;
    ubyte* unwrappedKey = 0;

    *cekLen = encryptedKeyLen - 8;

    if (OK > ( status = PKCS7_GenerateECCKeyEncryptionKey(
                    MOC_HASH(hwAccelCtx) pHashAlgo,
                    pECCKey, k, keyWrapOID, ukmData, ukmDataLen,
                    *cekLen, &kek)))
    {
        goto exit;
    }

    unwrappedKey = MALLOC( *cekLen);

    if ( OK > ( status = AESKWRAP_decrypt( MOC_SYM(hwAccelCtx) kek, *cekLen,
                  encryptedKey, encryptedKeyLen, unwrappedKey)))
    {
        goto exit;
    }

    *cek = unwrappedKey;
    unwrappedKey = 0;

exit:

    if ( unwrappedKey)
    {
        FREE( unwrappedKey);
    }

    if ( kek)
    {
        FREE( kek);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_HashOfEcKey(MOC_HASH(hwAccelDescr hwAccelCtx) ECCKey* pECCKey, ubyte* result,
                  MSTATUS (*completeDigest)(MOC_HASH(hwAccelDescr hwAccelCtx)
                                            const ubyte *pData, ubyte4 dataLen,
                                            ubyte *pDigestOutput))
{
    MSTATUS status;
    ubyte* ptBuf = NULL;
    ubyte* keyBuf = NULL;
    sbyte4 keyBufLen = 0;

    /* generate public key hash */
    if ( OK > ( status = EC_pointToByteString( pECCKey->pCurve,
                                              pECCKey->Qx,
                                              pECCKey->Qy,
                                              &ptBuf,
                                              &keyBufLen)))
    {
        goto exit;
    }

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, keyBufLen,
                                    TRUE, &keyBuf)))
    {
        goto exit;
    }

    MOC_MEMCPY(keyBuf, ptBuf, keyBufLen);

    status = completeDigest(MOC_HASH(hwAccelCtx) keyBuf, keyBufLen, result);

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &keyBuf);

    if (ptBuf)
    {
        FREE(ptBuf);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddEcKeyIdentifier(MOC_HASH(hwAccelDescr hwAccelCtx) DER_ITEMPTR pParent,
                         ECCKey* pECCKey, ubyte4 hashType)
{
    MSTATUS (*completeDigest)(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pDigestOutput);
    MSTATUS status;
    DER_ITEMPTR pTag;
    ubyte* hashResult = NULL;
    ubyte4 digestSize = 0;

    switch(hashType)
    {
        case ht_sha1:
            digestSize = SHA1_RESULT_SIZE;
            completeDigest = SHA1_completeDigest;
            break;

#ifndef __DISABLE_MOCANA_SHA224__
        case ht_sha224:
            digestSize = SHA224_RESULT_SIZE;
            completeDigest = SHA224_completeDigest;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA256__
        case ht_sha256:
            digestSize = SHA256_RESULT_SIZE;
            completeDigest = SHA256_completeDigest;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA384__
        case ht_sha384:
            digestSize = SHA384_RESULT_SIZE;
            completeDigest = SHA384_completeDigest;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA512__
        case ht_sha512:
            digestSize = SHA512_RESULT_SIZE;
            completeDigest = SHA512_completeDigest;
            break;
#endif

        default:
            status = ERR_PKCS7_UNSUPPORTED_DIGESTALGO;
            goto exit;
    }

    hashResult = (ubyte*)MALLOC(digestSize);
    if (!hashResult)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = PKCS7_HashOfEcKey(MOC_HASH(hwAccelCtx) pECCKey,
                                         hashResult, completeDigest)))
    {
        goto exit;
    }

    /* rKeyId [0] IMPLICIT RecipientKeyIdentifier */
    if (OK > (status = DER_AddTag(pParent, 0, &pTag)))
    {
        goto exit;
    }

    status = DER_AddItemOwnData(pTag, OCTETSTRING,
                                digestSize, &hashResult, NULL);

exit:
    if (hashResult)
    {
        FREE(hashResult);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddECDHRecipientInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                            DER_ITEMPTR pRecipientInfos,
                            ECCKey* pECCKey,
                            const BulkEncryptionAlgo* pBulkEncryptionAlgo,
                            const ubyte* ceKey,
                            ubyte4 ceKeyLen,
                            ASN1_ITEMPTR pCertificate,
                            CStream certificateStream,
                            ubyte4 keyIdHashType,
                            RNGFun rngFun,
                            void* rngFunArg)
{
    MSTATUS status;
    const BulkHashAlgo* pHashAlgo;
    ubyte hashType; /* for X9.63 key derivation */
    const ubyte* keyDerivationOID;
    const ubyte* keyWrapOID;
    DER_ITEMPTR pRecipientInfo, pTag, pTemp;
    ASN1_ITEMPTR pIssuer, pSerialNumber;
    ubyte copyData[MAX_DER_STORAGE];
    PFEPtr k = 0, Qx = 0, Qy = 0;
    PrimeFieldPtr pPF;
    ubyte* ukmData = 0;
    ubyte* ukmDataCopy;
    sbyte4 ecdhKeyLen;
    ubyte* ecdhKeyBuffer = 0;
    ubyte4 encryptedKeyBufferLen = 0;
    ubyte* encryptedKeyBuffer = 0;

    pPF = EC_getUnderlyingField(pECCKey->pCurve);

    /* add implicit tag [1] for KeyAgreeRecipientInfo */
    if (OK > ( status = DER_AddTag(pRecipientInfos, 1, &pRecipientInfo)))
        goto exit;

    /* CMSVersion */
    copyData[0] = 3;
    if ( OK > ( status = DER_AddItemCopyData( pRecipientInfo, INTEGER, 1, copyData, NULL)))
        goto exit;

    /* tag [0] for OriginatorIdentifierOrKey */
    if ( OK > ( status = DER_AddTag( pRecipientInfo, 0, &pTag)))
        goto exit;

    /* implicit tag [1] for OriginatorPublicKey */
    if ( OK > ( status = DER_AddTag( pTag, 1, &pTemp)))
        goto exit;

    /* AlgorithmIdentifier -- RFC3278 required NULL parameters, but the
    revision recommends the ecCurve or ABSENT -- test backward compatibility...*/
    if (OK > ( status = DER_StoreAlgoOID( pTemp, ecPublicKey_OID, TRUE)))
        goto exit;

    /* generate an ephemeral ECDH key */
    if (OK > (status = PRIMEFIELD_newElement( pPF, &k)) ||
        OK > (status = PRIMEFIELD_newElement( pPF, &Qx)) ||
        OK > (status = PRIMEFIELD_newElement( pPF, &Qy)))
    {
        goto exit;
    }

    if (OK > (status = EC_generateKeyPair( pECCKey->pCurve, rngFun,
                                            rngFunArg, k, Qx, Qy)))
    {
        goto exit;
    }

    /* allocate a buffer for the key parameter */
    if (OK > (status = EC_getPointByteStringLen( pECCKey->pCurve, &ecdhKeyLen)))
        goto exit;

    if (0 == ecdhKeyLen)
    {
        status = ERR_BAD_KEY;
        goto exit;
    }

    /* add an extra byte = 0 (unused bits) */
    ecdhKeyBuffer = MALLOC( ecdhKeyLen+1);
    if (!ecdhKeyBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    ecdhKeyBuffer[0] = 0; /* unused bits */

    if (OK > ( status = EC_writePointToBuffer( pECCKey->pCurve, Qx, Qy,
                                                ecdhKeyBuffer+1, ecdhKeyLen)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddItemOwnData( pTemp, BITSTRING, ecdhKeyLen+1, &ecdhKeyBuffer, NULL)))
        goto exit;

    /* ukm SHOULD be generated -- we generate encryptKeyLen */
    ukmData = ukmDataCopy = MALLOC( ceKeyLen);
    if(!ukmData)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    rngFun( rngFunArg, ceKeyLen, ukmData);

    if (OK > ( status = DER_AddTag( pRecipientInfo, 1, &pTag)))
        goto exit;

    if (OK > ( status = DER_AddItemOwnData( pTag, OCTETSTRING, ceKeyLen, &ukmData, NULL)))
        goto exit;

    /* keyEncryptionAlgorithmIdentifier */
    if (OK > ( status = DER_AddSequence( pRecipientInfo, &pTemp)))
        goto exit;

    /* we pick up the key derivation function that makes
       sense for the strength of the ECC key --
       also compatible with RFC 5008 */
#ifdef __ENABLE_MOCANA_ECC_P192__
    if (pECCKey->pCurve == EC_P192)
    {
        keyDerivationOID = dhSinglePassStdDHSha1KDF_OID;
        hashType = ht_sha1;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
    if (pECCKey->pCurve == EC_P224)
    {
        keyDerivationOID = dhSinglePassStdDHSha224KDF_OID;
        hashType = ht_sha224;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P256__
    if (pECCKey->pCurve == EC_P256)
    {
        keyDerivationOID = dhSinglePassStdDHSha256KDF_OID;
        hashType = ht_sha256;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
    if (pECCKey->pCurve == EC_P384)
    {
        keyDerivationOID = dhSinglePassStdDHSha384KDF_OID;
        hashType = ht_sha384;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
    if (pECCKey->pCurve == EC_P521)
    {
        keyDerivationOID = dhSinglePassStdDHSha512KDF_OID;
        hashType = ht_sha512;
    }
    else
#endif
    {
        status = ERR_EC_UNSUPPORTED_CURVE;
        goto exit;
    }

    if (OK > ( status = CRYPTO_getRSAHashAlgo(hashType, &pHashAlgo) ))
        goto exit;

    /* KeyEncryptionAlgorithmIdentifier */
    /* OID is the keyDerivation */
    if (OK > ( status = DER_AddOID( pTemp, keyDerivationOID, NULL)))
        goto exit;
    /* and parameters the KeyWrapAlgorithm */
    /* depending on the bulk encryption algo */
    /* for the moment, support only AES (suite B) */
#ifndef __DISABLE_AES_CIPHERS__
    if ( pBulkEncryptionAlgo == &CRYPTO_AESSuite)
    {
        switch (ceKeyLen)
        {
        case 16: /* AES 128 */
            keyWrapOID = aes128Wrap_OID;
            break;
        case 24:
            keyWrapOID = aes192Wrap_OID;
            break;
        case 32:
            keyWrapOID = aes256Wrap_OID;
            break;
        default:
            status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
            goto exit;  /* Should this be a fatal error? */
            break;
        }
    }
    else
#endif
    {
        status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
        goto exit;
    }
    /* KeyWrapAlgorithm  ::=  AlgorithmIdentifier */
    if (OK > ( status = DER_StoreAlgoOID( pTemp, keyWrapOID, TRUE)))
        goto exit;

    /* get the encrypted (wrapped) key */
    if (OK > ( status = PKCS7_ECCEncryptKey(
                    MOC_SYM_HASH(hwAccelCtx)
                    pHashAlgo,
                    pECCKey, k, keyWrapOID,
                    ukmDataCopy, ceKeyLen,
                    ceKey, ceKeyLen,
                    &encryptedKeyBuffer,
                    &encryptedKeyBufferLen)))
    {
        goto exit;
    }

    /* write out the recipientEncryptedKeys */
    if ( OK > ( status = DER_AddSequence( pRecipientInfo, &pTemp)))
    {
        goto exit;
    }

    if ( OK > ( status = DER_AddSequence( pTemp, &pTemp)))
    {
        goto exit;
    }

    if (pCertificate)
    {
        /* add issuerAndSerialNumber */
        /* get issuer and serial number of certificate */
        if ( OK > ( status = CERT_getCertificateIssuerSerialNumber( pCertificate, &pIssuer, &pSerialNumber)))
            goto exit;

        /* isssuerAndSerialNumber */
        if ( OK > ( status = PKCS7_AddIssuerAndSerialNumber( pTemp, certificateStream,
                                                            pIssuer, pSerialNumber, NULL)))
        {
            goto exit;
        }
    }
    else
    {
        if ( OK > ( status = PKCS7_AddEcKeyIdentifier(MOC_HASH(hwAccelCtx) pTemp,
                                                      pECCKey, keyIdHashType)))
        {
            goto exit;
        }
    }

    /* finally add the  encrypted (wrapped key) */
    if (OK > ( status = DER_AddItemOwnData( pTemp, OCTETSTRING,
            encryptedKeyBufferLen, &encryptedKeyBuffer, NULL)))
    {
        goto exit;
    }

exit:

    if ( ecdhKeyBuffer)
    {
        FREE( ecdhKeyBuffer);
    }

    if ( ukmData)
    {
        FREE( ukmData);
    }

    if ( encryptedKeyBuffer)
    {
        FREE( encryptedKeyBuffer);
    }

    PRIMEFIELD_deleteElement( pPF, &k);
    PRIMEFIELD_deleteElement( pPF, &Qx);
    PRIMEFIELD_deleteElement( pPF, &Qy);

    return status;
}
#endif


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddRSARecipientInfo(MOC_RSA(hwAccelDescr hwAccelCtx)
                           DER_ITEMPTR pRecipientInfos,
                           RSAKey* pRSAKey,
                           const ubyte* encryptKey,
                           ubyte4 encryptKeyLen,
                           ASN1_ITEMPTR pCertificate,
                           CStream certificateStream,
                           RNGFun rngFun,
                           void* rngFunArg)
{
    MSTATUS status;
    DER_ITEMPTR pRecipientInfo;
    ubyte copyData[MAX_DER_STORAGE];
    ASN1_ITEMPTR pIssuer, pSerialNumber;
    ubyte* encryptedKey = 0;
    ubyte4 encryptedKeyLen;

    if ( OK > ( status = DER_AddSequence( pRecipientInfos, &pRecipientInfo)))
        goto exit;

    /* recipient info version = 0 */
    copyData[0] = 0;
    if ( OK > ( status = DER_AddItemCopyData( pRecipientInfo, INTEGER, 1, copyData, NULL)))
        goto exit;

    /* get issuer and serial number of certificate */
    if ( OK > ( status = CERT_getCertificateIssuerSerialNumber( pCertificate, &pIssuer, &pSerialNumber)))
        goto exit;

    /* isssuerAndSerialNumber */
    if ( OK > ( status = PKCS7_AddIssuerAndSerialNumber( pRecipientInfo, certificateStream,
                                                        pIssuer, pSerialNumber, NULL)))
    {
        goto exit;
    }

    if  ( OK > (status = DER_StoreAlgoOID( pRecipientInfo, rsaEncryption_OID, TRUE)))
        goto exit;

    /* encrypt key */
    if ( OK > ( status = RSA_getCipherTextLength( pRSAKey, (sbyte4 *)(&encryptedKeyLen))))
        goto exit;

    encryptedKey = MALLOC( encryptedKeyLen);
    if ( !encryptedKey)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* add the encrypted key as an OCTET string */
    if ( OK > ( status = RSA_encrypt(MOC_RSA(hwAccelCtx) pRSAKey,
                                    encryptKey, encryptKeyLen,
                                    encryptedKey,
                                    rngFun, rngFunArg, NULL)))
    {
        goto exit;
    }


    if ( OK > ( status = DER_AddItemOwnData( pRecipientInfo, OCTETSTRING,
                                    encryptedKeyLen, &encryptedKey, NULL)))
    {
        goto exit;
    }
exit:

    if (encryptedKey)
    {
        FREE( encryptedKey);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS7_GetCryptoAlgoParams( const ubyte* encryptAlgoOID,
                          const BulkEncryptionAlgo** ppBulkEncryptionAlgo,
                          sbyte4 *keyLength)
{
    #ifdef __ENABLE_DES_CIPHER__
    if ( EqualOID( desCBC_OID, encryptAlgoOID)) /* SCEP requires this but not sure about OID*/
    {
        *keyLength = DES_KEY_LENGTH;
        *ppBulkEncryptionAlgo = &CRYPTO_DESSuite;

    }
    else
#endif
#ifndef __DISABLE_3DES_CIPHERS__
    if ( EqualOID( desEDE3CBC_OID, encryptAlgoOID))
    {
        *keyLength = THREE_DES_KEY_LENGTH;
        *ppBulkEncryptionAlgo = &CRYPTO_TripleDESSuite;
    }
    else
#endif
#ifndef __DISABLE_AES_CIPHERS__
    if ( EqualOID( aes128CBC_OID, encryptAlgoOID))
    {
        *keyLength = 16;
        *ppBulkEncryptionAlgo = &CRYPTO_AESSuite;
    }
    else
    if ( EqualOID( aes192CBC_OID, encryptAlgoOID))
    {
        *keyLength = 24;
        *ppBulkEncryptionAlgo = &CRYPTO_AESSuite;
    }
    else
    if ( EqualOID( aes256CBC_OID, encryptAlgoOID))
    {
        *keyLength = 32;
        *ppBulkEncryptionAlgo = &CRYPTO_AESSuite;
    }
    else
        /* add others here if necessary */
#endif
    {
        return ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
    }

    return OK;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS7_EnvelopData( MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                        DER_ITEMPTR pStart, /* can be null */
                        DER_ITEMPTR pParent,
                        ASN1_ITEMPTR pCACertificates[/*numCACerts*/],
                        CStream pStreams[/*numCACerts*/],
                        sbyte4 numCACerts, /* only one currently supported */
                        const ubyte* encryptAlgoOID,
                        RNGFun rngFun,
                        void* rngFunArg,
                        const ubyte* pPayLoad,
                        ubyte4 payLoadLen,
                        ubyte** ppEnveloped,
                        ubyte4* pEnvelopedLen)
{
    MSTATUS         status = OK;
    DER_ITEMPTR     pEnvelopedData = 0;
    DER_ITEMPTR     pTemp,
                    pEncryptionAlgo,
                    pEncryptedPayload,
                    pRecipientInfos;
    ubyte*          pPlaceHolderData;
    ubyte           copyData[MAX_DER_STORAGE];
    ubyte           iv[MAX_IV_LENGTH]; /* IV generation */
    ubyte           encryptKey[MAX_ENC_KEY_LENGTH];  /* big enough for AES-256 */
    sbyte4          i, keyLength, padSize = 0;
    const BulkEncryptionAlgo* pBulkEncryptionAlgo;
    AsymmetricKey   key;
    ubyte4          envelopedBufferLen;
    ubyte*          envelopedBuffer = 0;
    ubyte*          pCryptoBuf = NULL;
    ubyte*          pCryptoIv = NULL;

    if ( !pCACertificates || !pPayLoad || !ppEnveloped || !pEnvelopedLen || !rngFun)
    {
        return ERR_NULL_POINTER;
    }

    if ( OK > ( status = CRYPTO_initAsymmetricKey( &key)))
        goto exit;

    if ( OK > ( status = PKCS7_GetCryptoAlgoParams( encryptAlgoOID,
                                                    &pBulkEncryptionAlgo,
                                                    &keyLength)))
    {
        goto exit;
    }

    /* generate key and iv */
    rngFun( rngFunArg, keyLength, encryptKey);
    if ( pBulkEncryptionAlgo->blockSize)
    {
        rngFun( rngFunArg, pBulkEncryptionAlgo->blockSize, iv);
    }

    if ( OK > ( status = DER_AddSequence( pParent, &pEnvelopedData)))
        goto exit;

    /* version = 0 */
    copyData[0] = 0;
    if ( OK > ( status = DER_AddItem( pEnvelopedData, INTEGER, 1, copyData, NULL)))
        goto exit;

    /* recipient information */
    if ( OK > ( status = DER_AddSet( pEnvelopedData, &pRecipientInfos)))
        goto exit;

    /* for each certificate, add a recipient info */
    for ( i = 0; i < numCACerts; ++i)
    {
        if ( OK > (status = CERT_setKeyFromSubjectPublicKeyInfo(
                MOC_RSA(hwAccelCtx) pCACertificates[i], pStreams[i], &key)))
        {
            goto exit;
        }

        if ( akt_rsa == key.type)
        {
            if (OK > ( status = PKCS7_AddRSARecipientInfo(MOC_RSA(hwAccelCtx)
                                    pRecipientInfos, key.key.pRSA,
                                    encryptKey, keyLength,
                                    pCACertificates[i], pStreams[i],
                                    rngFun, rngFunArg)))
            {
                goto exit;
            }
        }
#ifdef __ENABLE_MOCANA_ECC__
        else if ( akt_ecc == key.type)
        {
            /* need to change the version */
            if (copyData[0] < 2)
                copyData[0] = 2;

            if (OK > ( status = PKCS7_AddECDHRecipientInfo(MOC_HASH(hwAccelCtx)
                                    pRecipientInfos,key.key.pECC,
                                    pBulkEncryptionAlgo,
                                    encryptKey, keyLength,
                                    pCACertificates[i], pStreams[i], 0,
                                    rngFun, rngFunArg)))
            {
                goto exit;
            }
        }
#endif
        else
        {
            status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
            goto exit;
        }

        if ( OK > ( status = CRYPTO_uninitAsymmetricKey(&key, NULL)))
            goto exit;
    }

    /* Encrypted Content Info */
    if ( OK > ( status = DER_AddSequence( pEnvelopedData, &pTemp)))
        goto exit;

    /* content type */
    if ( OK > ( status = DER_AddOID( pTemp, pkcs7_data_OID, NULL)))
        goto exit;

    /* encryption algo */
    if ( OK > ( status = DER_AddSequence( pTemp, &pEncryptionAlgo)))
        goto exit;

    if ( OK > ( status = DER_AddOID( pEncryptionAlgo, encryptAlgoOID, NULL)))
        goto exit;

    if ( pBulkEncryptionAlgo->blockSize > 0)
    {
        if ( OK > ( status = DER_AddItem( pEncryptionAlgo, OCTETSTRING,
                                        pBulkEncryptionAlgo->blockSize, iv, NULL)))
        {
            goto exit;
        }
        padSize = pBulkEncryptionAlgo->blockSize - ( payLoadLen % pBulkEncryptionAlgo->blockSize);
        if ( 0 == padSize) padSize = pBulkEncryptionAlgo->blockSize;
    }
    else
    {
        if ( OK > ( status = DER_AddItem( pEncryptionAlgo, NULLTAG, 0, NULL, NULL)))
            goto exit;
        padSize = 0;
    }

    /* now add the encrypted payload tag [0] place holder for now */
    if ( OK > ( status = DER_AddItem( pTemp, (CONTEXT|0), payLoadLen + padSize,
                                        NULL, &pEncryptedPayload )))
    {
        goto exit;
    }
    /* write everything to our buffer */
    if ( OK > ( status = DER_Serialize( pStart ? pStart : pEnvelopedData,
                                        &envelopedBuffer, &envelopedBufferLen)))
    {
        goto exit;
    }

    /* fill-in the place holders */
    /* encrypted payload */
    if  ( OK > ( status = DER_GetSerializedDataPtr( pEncryptedPayload, &pPlaceHolderData)))
        goto exit;

#ifdef __DISABLE_MOCANA_HARDWARE_ACCEL__
    MOC_MEMCPY( pPlaceHolderData, pPayLoad, payLoadLen);
    pCryptoBuf = pPlaceHolderData;
#else
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, payLoadLen + padSize, TRUE, &pCryptoBuf)))
        goto exit;

    MOC_MEMCPY(pCryptoBuf , pPayLoad, payLoadLen);
#endif

    /* add padding */
    for (i = 0; i < padSize; ++i)
    {
        pCryptoBuf[payLoadLen+i] = (ubyte) padSize;
    }

#ifdef __DISABLE_MOCANA_HARDWARE_ACCEL__
    pCryptoIv = iv;
#else
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, 16, TRUE, &pCryptoIv)))
        goto exit;

    MOC_MEMCPY(pCryptoIv , iv, 16);
#endif

    /* encrypt in place */
    if (OK > ( status = CRYPTO_Process(MOC_SYM(hwAccelCtx) pBulkEncryptionAlgo,
                            encryptKey, keyLength, pCryptoIv, pCryptoBuf, payLoadLen + padSize, 1)))
    {
        goto exit;
    }

    /* copy back the encrypted buffer */
    MOC_MEMCPY(pPlaceHolderData, pCryptoBuf, payLoadLen + padSize);

    /* return the buffer */
    *ppEnveloped = envelopedBuffer;
    envelopedBuffer = NULL;
    *pEnvelopedLen = envelopedBufferLen;

exit:
#ifdef __DISABLE_MOCANA_HARDWARE_ACCEL__
    /* nothing to do */
#else
    if (pCryptoBuf)
        CRYPTO_FREE(hwAccelCtx, TRUE, &pCryptoBuf);
    if (pCryptoIv)
        CRYPTO_FREE(hwAccelCtx, TRUE, &pCryptoIv);
#endif

    if ( envelopedBuffer)
    {
        FREE( envelopedBuffer);
    }

    /* if there was no parent specified delete the DER tree */
    if ( !pParent && pEnvelopedData)
    {
        TREE_DeleteTreeItem( (TreeItem*) pEnvelopedData);
    }

    /* clear the key on the stack */
    MOC_MEMSET( encryptKey, 0, MAX_ENC_KEY_LENGTH);

    CRYPTO_uninitAsymmetricKey( &key, NULL);

    return status;
}


/*------------------------------------------------------------------*/
static MSTATUS
PKCS7_AddItem2( DER_ITEMPTR pParent,
               const ubyte* pPayLoad, ubyte4 payLoadLen,
               DER_ITEMPTR *ppNewItem)
{
    MSTATUS status;
    ASN1_ITEMPTR pRootItem, pPayLoadItem;
    CStream payLoadStream;
    const ubyte* payLoadMemAccessBuffer = NULL;
    MemFile memFile;

    /* a. construct the ASN1 item for the payload */
    MF_attach(&memFile, payLoadLen, (ubyte*) pPayLoad);
    CS_AttachMemFile(&payLoadStream, &memFile );

    if (OK > (status = ASN1_Parse(payLoadStream, &pRootItem)))
    {
        goto exit;
    }
    pPayLoadItem = ASN1_FIRST_CHILD(pRootItem);
    if (!pPayLoadItem)
    {
        status = ERR_ASN;
        goto exit;
    }
    /* b. do CS_memaccess on the dataOffset of the ASN1_ITEM: */
    payLoadMemAccessBuffer = CS_memaccess( payLoadStream, pPayLoadItem->dataOffset, pPayLoadItem->length);
    if ( !payLoadMemAccessBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* c. add it with the enclosing tag SEQUENCE to the parent item: */
    if ( OK > ( status = DER_AddItem( pParent, (CONSTRUCTED|SEQUENCE), pPayLoadItem->length, payLoadMemAccessBuffer, ppNewItem)))
        goto exit;

exit:
    if (pRootItem)
    {
        TREE_DeleteTreeItem((TreeItem *) pRootItem);
    }

    /* d. stop memaccess */
    if (payLoadMemAccessBuffer)
    {
        CS_stopaccess( payLoadStream, payLoadMemAccessBuffer);
    }
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddContentInfo(DER_ITEMPTR pParent,
               const ubyte* payLoadType, /* OID, if NULL, degenerate case */
               const ubyte* pPayLoad, /* if payLoadType is not NULL, pPayLoad is NULL, external signatures */
               ubyte4 payLoadLen,
               DER_ITEMPTR *ppContentInfo)
{
    MSTATUS status;
    DER_ITEMPTR pContentInfo, pTempItem;
    sbyte4 cmpResult;

    if ( OK > ( status = DER_AddSequence( pParent, &pContentInfo)))
        goto exit;

    if (payLoadType)
    {
        /* content type */
        if ( OK > ( status = DER_AddOID( pContentInfo, payLoadType, NULL)))
            goto exit;
       if (pPayLoad && payLoadLen > 0)
       {
           /* content */
           if ( OK > ( status = DER_AddTag( pContentInfo, 0, &pTempItem)))
               goto exit;

           MOC_MEMCMP(payLoadType, pkcs7_data_OID, sizeof(payLoadType), &cmpResult);

           if (cmpResult == 0) /* data */
           {
               if ( OK > ( status = DER_AddItem( pTempItem, PRIMITIVE|OCTETSTRING, payLoadLen, pPayLoad, &pTempItem)))
                   goto exit;
           }
           else
           {
               if (OK > (status = PKCS7_AddItem2(pTempItem, pPayLoad, payLoadLen, NULL)))
                   goto exit;
           }
       }
    }
    else
    {
        /* content type will be data in the degenerate case */
        if ( OK > ( status = DER_AddOID( pContentInfo, pkcs7_data_OID, NULL)))
            goto exit;
    }

    if (ppContentInfo)
    {
        *ppContentInfo = pContentInfo;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddItem1(DER_ITEMPTR pParent,
               CStream cs, ASN1_ITEMPTR pRootItem,
               DER_ITEMPTR *ppNewItem)
{
    MSTATUS status;
    const ubyte* memAccessBuffer;
    ASN1_ITEMPTR pItem;

    pItem = ASN1_FIRST_CHILD(pRootItem);
    memAccessBuffer = CS_memaccess(cs, pItem->dataOffset, pItem->length);
    if ( !memAccessBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    if ( OK > ( status = DER_AddItem( pParent, pItem->id|pItem->tag,
        pItem->length, memAccessBuffer, ppNewItem)))
    {
        goto exit;
    }
exit:
    if (memAccessBuffer)
    {
        CS_stopaccess( cs, memAccessBuffer);
    }
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddSetOfOrSequenceOfASN1ItemsWithTag(DER_ITEMPTR pParent,
                                           ubyte tag, ubyte4 setOrSequence,
                                           CStream *itemStreams,
                                           ASN1_ITEMPTR *ppRootItems, ubyte4 numItems,
                                           DER_ITEMPTR *ppChild)
{
    MSTATUS status;
    DER_ITEMPTR pChild, pTempItem = NULL;
    ubyte4 i;

    if (0 == numItems)
    {
        status = ERR_PKCS7;
        goto exit;
    }

    if ( OK > ( status = DER_AddTag( pParent, tag, &pChild)))
        goto exit;

    if (setOrSequence == SET)
    {
        if ( OK > ( status = DER_AddSet( pChild, &pTempItem)))
            goto exit;
    }
    else if (setOrSequence == SEQUENCE)
    {
        if ( OK > ( status = DER_AddSequence( pChild, &pTempItem)))
            goto exit;
    }
    else
    {
        /* IMPLICIT type SET or SEQUENCE */
        pTempItem = pChild;
    }
    for ( i = 0; i < numItems; ++i)
    {
        if (OK > (status = PKCS7_AddItem1(pTempItem, itemStreams[i], ppRootItems[i], NULL)))
            goto exit;
    }
    if (ppChild)
    {
        *ppChild = pChild;
    }
exit:
    return status;
}



/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddAttributeEx(DER_ITEMPTR pParent, const ubyte* typeOID,
                   const ubyte valueType, const ubyte* value, ubyte4 valueLen,
                   intBoolean derBuffer, DER_ITEMPTR *ppAttribute)
{
    MSTATUS status;
    DER_ITEMPTR pAttribute, pTempItem;

    if ( OK > ( status = DER_AddSequence( pParent, &pAttribute)))
        goto exit;

    if ( OK > ( status = DER_AddOID( pAttribute, typeOID, NULL)))
        goto exit;

    if ( OK > ( status = DER_AddSet( pAttribute, &pTempItem)))
        goto exit;

    if (derBuffer)
    {
        if ( OK > ( status = DER_AddDERBuffer( pTempItem, valueLen, value, NULL)))
            goto exit;
    }
    else
    {
        if ( OK > ( status = DER_AddItem( pTempItem, valueType, valueLen, value, NULL)))
            goto exit;
    }

    if (ppAttribute)
    {
        *ppAttribute = pAttribute;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddAttribute(DER_ITEMPTR pParent, const ubyte* typeOID,
                   const ubyte valueType, const ubyte* value, ubyte4 valueLen,
                   DER_ITEMPTR *ppAttribute)
{
    return PKCS7_AddAttributeEx(pParent, typeOID, valueType,
                                value, valueLen, 0, ppAttribute);
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddIssuerAndSerialNumber(DER_ITEMPTR pParent,
                               CStream cs,
                               ASN1_ITEMPTR pIssuer,
                               ASN1_ITEMPTR pSerialNumber,
                               DER_ITEMPTR *ppIssuerAndSerialNumber)
{
    MSTATUS status;
    DER_ITEMPTR pIssuerAndSerialNumber;

    if (OK > (status = DER_AddSequence(pParent, &pIssuerAndSerialNumber)))
        goto exit;

    if ( OK > (status = DER_AddASN1Item( pIssuerAndSerialNumber, pIssuer, cs, NULL)))
        goto exit;

    if ( OK > (status = DER_AddASN1Item( pIssuerAndSerialNumber, pSerialNumber, cs, NULL)))
        goto exit;

    if (ppIssuerAndSerialNumber)
    {
        *ppIssuerAndSerialNumber = pIssuerAndSerialNumber;
    }

exit:

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddRSASignature(MOC_RSA(hwAccelDescr hwAccelCtx) DER_ITEMPTR pSignerInfoItem, const RSAKey* pRSA,
                      const ubyte* digestAlgoOID,
                      const ubyte* digest, ubyte4 digestLen,
                      ubyte** ppSignature, ubyte4* pSignatureLen)
{
    MSTATUS status;
    DER_ITEMPTR pDigestInfo = 0;
    ubyte* pDerDigestInfo = 0;
    ubyte4 derDigestInfoLen;
    ubyte* pEncryptedDigest = 0;
    ubyte4 encryptedDigestLen;

    /* create a DigestInfo */
    if ( OK > ( status = DER_AddSequence ( NULL, &pDigestInfo)))
        goto exit;

    if ( OK > ( status = DER_StoreAlgoOID ( pDigestInfo, digestAlgoOID,
                                            TRUE)))
    {
        goto exit;
    }
    /* if authenticated attributes is present, use second hash; else use pHash->hashData */

    if ( OK > ( status = DER_AddItem( pDigestInfo, OCTETSTRING,
                                      digestLen, digest, NULL)))
    {
        goto exit;
    }

    if ( OK > ( status = DER_Serialize( pDigestInfo, &pDerDigestInfo,
                                        &derDigestInfoLen)))
    {
        goto exit;
    }

    if ( OK > ( status = RSA_getCipherTextLength( pRSA,
                                    (sbyte4 *)(&encryptedDigestLen))))
    {
        goto exit;
    }

    pEncryptedDigest = MALLOC(encryptedDigestLen);
    if ( !pEncryptedDigest)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if ( OK > ( status = RSA_signMessage(MOC_RSA(hwAccelCtx) pRSA,
        pDerDigestInfo, derDigestInfoLen, pEncryptedDigest, NULL)))
    {
        goto exit;
    }

    /* add the encrypted digest as an OCTET string */
    if ( OK > ( status = DER_AddItem( pSignerInfoItem, OCTETSTRING, encryptedDigestLen,
                                        pEncryptedDigest, NULL)))
    {
        goto exit;
    }

    *ppSignature = pEncryptedDigest;
    pEncryptedDigest = 0;
    *pSignatureLen = encryptedDigestLen;

exit:

    if (pEncryptedDigest)
    {
        FREE(pEncryptedDigest);
    }

    if (pDerDigestInfo)
    {
        FREE(pDerDigestInfo);
    }

    if (pDigestInfo)
    {
        TREE_DeleteTreeItem( (TreeItem*) pDigestInfo);
    }

    return status;
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_ECC__
static MSTATUS
PKCS7_AddECDSASignature(DER_ITEMPTR pSignerInfoItem, ECCKey* pECCKey,
                      RNGFun rngFun, void* rngArg,
                      const ubyte* hash, ubyte4 hashLen,
                      ubyte** ppSignature, ubyte4* pSignatureLen)
{
    DER_ITEMPTR pTempItem;
    DER_ITEMPTR pTempSeq = 0;
    PFEPtr sig_r = 0, sig_s = 0;
    PrimeFieldPtr pPF;
    ubyte* pSignatureBuffer = 0;
    sbyte4 elementLen;
    ubyte* pRBuffer;
    ubyte* pSBuffer;
    MSTATUS status;

    pPF = EC_getUnderlyingField( pECCKey->pCurve);

    if (OK > ( status = PRIMEFIELD_newElement( pPF, &sig_r)))
        goto exit;
    if (OK > ( status = PRIMEFIELD_newElement( pPF, &sig_s)))
        goto exit;

    if (OK > ( status = ECDSA_sign( pECCKey->pCurve,
                            pECCKey->k,
                            rngFun, rngArg,
                            hash, hashLen,
                            sig_r, sig_s)))
    {
        goto exit;
    }
    /* add the signature */
    /* allocate buffer for sig_r and sig_s with leading zeroes */
    if ( OK > ( status = PRIMEFIELD_getElementByteStringLen( pPF, &elementLen)))
        goto exit;

    /* allocate 2 extra bytes for the possible zero padding */
    pSignatureBuffer = MALLOC( 2 + 2 * elementLen);
    if (! pSignatureBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    pRBuffer = pSignatureBuffer;
    *pRBuffer = 0x00; /* leading 0 */
    pSBuffer = pSignatureBuffer + 1 + elementLen;
    *pSBuffer = 0x00; /* leading 0 */
    /* write R */
    if ( OK > ( status = PRIMEFIELD_writeByteString( pPF, sig_r, pRBuffer+1, elementLen)))
        goto exit;

    /* write S */
    if ( OK > ( status = PRIMEFIELD_writeByteString( pPF, sig_s, pSBuffer+1, elementLen)))
        goto exit;

    /* create a sequence with the two integer -> signature */
    if (OK > ( status = DER_AddSequence( NULL, &pTempSeq)))
        goto exit;

    if (OK > ( status = DER_AddInteger( pTempSeq, elementLen + 1, pRBuffer, NULL)))
        goto exit;

    if (OK > ( status = DER_AddInteger( pTempSeq, elementLen + 1, pSBuffer, NULL)))
        goto exit;

    /* serialize the sequence */
    if (OK > ( status = DER_Serialize( pTempSeq, ppSignature, pSignatureLen)))
        goto exit;

    /* add the DER encoded buffer -- signature -- as is */
    if (OK > (status = DER_AddItem( pSignerInfoItem, OCTETSTRING, 0, NULL, &pTempItem)))
        goto exit;

    if (OK > ( status = DER_AddDERBuffer( pTempItem, *pSignatureLen, *ppSignature, NULL)))
        goto exit;

exit:

    if (pTempSeq)
    {
        TREE_DeleteTreeItem( (TreeItem*) pTempSeq);
    }

    FREE(pSignatureBuffer);

    PRIMEFIELD_deleteElement( pPF, &sig_r);
    PRIMEFIELD_deleteElement( pPF, &sig_s);

    return status;
}

#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_DSA__
static MSTATUS
PKCS7_AddDSASignature(MOC_DSA(hwAccelDescr hwAccelCtx)
                      DER_ITEMPTR pSignerInfoItem, DSAKey* pDSAKey,
                      RNGFun rngFun, void* rngArg,
                      const ubyte* hash, ubyte4 hashLen,
                      ubyte** ppSignature, ubyte4* pSignatureLen)
{
    DER_ITEMPTR pTempItem;
    DER_ITEMPTR pTempSeq = 0;
    vlong *r, *s;
    ubyte* pSignatureBuffer = 0;
    sbyte4 rLen, sLen;
    ubyte* pRBuffer;
    ubyte* pSBuffer;
    MSTATUS status;

    if (OK > ( status = DSA_computeSignature2(MOC_DSA(hwAccelCtx) rngFun, rngArg,
                                               pDSAKey, hash, hashLen, &r, &s, NULL)))
    {
        goto exit;
    }
    /* add the signature */
    /* allocate buffer for sig_r and sig_s with leading zeroes */
    if (OK > ( status = VLONG_byteStringFromVlong( r, NULL, &rLen)))
        goto exit;
    if (OK > ( status = VLONG_byteStringFromVlong( s, NULL, &sLen)))
        goto exit;

    /* allocate 2 extra bytes for the possible zero padding */
    pSignatureBuffer = MALLOC( 2 + rLen + sLen);
    if (! pSignatureBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    pRBuffer = pSignatureBuffer;
    *pRBuffer = 0x00; /* leading 0 */
    pSBuffer = pSignatureBuffer + 1 + rLen;
    *pSBuffer = 0x00; /* leading 0 */
    /* write R */
    if ( OK > ( status = VLONG_byteStringFromVlong( r, pRBuffer+1, &rLen)))
        goto exit;

    /* write S */
    if ( OK > ( status = VLONG_byteStringFromVlong( s, pSBuffer+1, &sLen)))
        goto exit;

    /* create a sequence with the two integer -> signature */
    if (OK > ( status = DER_AddSequence( NULL, &pTempSeq)))
        goto exit;

    if (OK > ( status = DER_AddInteger( pTempSeq, rLen + 1, pRBuffer, NULL)))
        goto exit;

    if (OK > ( status = DER_AddInteger( pTempSeq, sLen + 1, pSBuffer, NULL)))
        goto exit;

    /* serialize the sequence */
    if (OK > ( status = DER_Serialize( pTempSeq, ppSignature, pSignatureLen)))
        goto exit;

    /* add the DER encoded buffer -- signature -- as is */
    if (OK > (status = DER_AddItem( pSignerInfoItem, OCTETSTRING, 0, NULL, &pTempItem)))
        goto exit;

    if (OK > ( status = DER_AddDERBuffer( pTempItem, *pSignatureLen, *ppSignature, NULL)))
        goto exit;

exit:

    if (pTempSeq)
    {
        TREE_DeleteTreeItem( (TreeItem*) pTempSeq);
    }

    FREE(pSignatureBuffer);

    VLONG_freeVlong(&r, NULL);
    VLONG_freeVlong(&s, NULL);

    return status;
}

#endif /* defined(__ENABLE_MOCANA_DSA__) */

#ifdef __ENABLE_MOCANA_DSA__

/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddDSADigestAlgoId(DER_ITEMPTR pSignerInfoItem, ubyte hashType)
{
    DER_ITEMPTR pOID = 0, pSequence;
    ubyte* oidBuffer = 0;
    ubyte4 oidBufferLen;
    ubyte dsaAlgoOID[1 + MAX_SIG_OID_LEN];
    MSTATUS status;

    if (OK > ( status = CRYPTO_getDSAHashAlgoOID( hashType, dsaAlgoOID)))
        goto exit;
    /* there is no parameters to the AlgoOID (even NULL) */
    if ( OK > ( status = DER_AddSequence( pSignerInfoItem, &pSequence)))
        goto exit;

    /* the memory management and the fact that OID are prefixed by their
        length makes it a bit involved... */
    /* creates a stand alone DER_ITEM with the correct OID */
    if ( OK > ( status = DER_AddOID( NULL, dsaAlgoOID, &pOID)))
        goto exit;

    /* serialize it */
    if (OK > ( status = DER_Serialize( pOID, &oidBuffer, &oidBufferLen)))
    {
        goto exit;
    }
    /* add the DER encoded buffer to pSequence transferring ownership */
    if (OK > ( status = DER_AddDERBufferOwn( pSequence, oidBufferLen, (const ubyte**) &oidBuffer, NULL)))
    {
        goto exit;
    }

exit:
    if (pOID)
    {
        TREE_DeleteTreeItem( (TreeItem*) pOID); /* don't forget to delete the stand alone DER_ITEM */
    }

    if (oidBuffer)
    {
        FREE(oidBuffer);
    }
    return status;
}
#endif


#ifdef __ENABLE_MOCANA_ECC__

/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddECDSADigestAlgoId(DER_ITEMPTR pSignerInfoItem, ubyte hashType)
{
    DER_ITEMPTR pOID = 0, pSequence;
    ubyte* oidBuffer = 0;
    ubyte4 oidBufferLen;
    ubyte ecdsaAlgoOID[1 + MAX_SIG_OID_LEN];
    MSTATUS status;

    if (OK > ( status = CRYPTO_getECDSAHashAlgoOID( hashType, ecdsaAlgoOID)))
        goto exit;
    /* RFC 5008 says that there is no parameters to the AlgoOID (even NULL) */
    if ( OK > ( status = DER_AddSequence( pSignerInfoItem, &pSequence)))
        goto exit;

    /* the memory management and the fact that OID are prefixed by their
        length makes it a bit involved... */
    /* creates a stand alone DER_ITEM with the correct OID */
    if ( OK > ( status = DER_AddOID( NULL, ecdsaAlgoOID, &pOID)))
        goto exit;

    /* serialize it */
    if (OK > ( status = DER_Serialize( pOID, &oidBuffer, &oidBufferLen)))
    {
        goto exit;
    }
    /* add the DER encoded buffer to pSequence transferring ownership */
    if (OK > ( status = DER_AddDERBufferOwn( pSequence, oidBufferLen, (const ubyte**) &oidBuffer, NULL)))
    {
        goto exit;
    }

exit:
    if (pOID)
    {
        TREE_DeleteTreeItem( (TreeItem*) pOID); /* don't forget to delete the stand alone DER_ITEM */
    }

    if (oidBuffer)
    {
        FREE(oidBuffer);
    }
    return status;
}
#endif

/*------------------------------------------------------------------*/

static MSTATUS
PKCS7_AddPerSignerInfo(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) DER_ITEMPTR pSignerInfosItem,
                        signerInfoPtr pSignerInfo, SignedDataHash* pHash,
                        RNGFun rngFun, void* rngArg,
                        ubyte* payLoadType, ubyte** ppDataBuffer )
{
    MSTATUS         status;
    DER_ITEMPTR     pSignerInfoItem, pAttributeItem;
    DER_ITEMPTR     pSetOf = NULL;
    ubyte           copyData[MAX_DER_STORAGE];
    ubyte           *pTempBuf = NULL;

    ubyte*          pDerAttributes = 0;
    ubyte4          derAttributesLen;
    ubyte4          i;

    if ( OK > ( status = DER_AddSequence( pSignerInfosItem, &pSignerInfoItem)))
        goto exit;

    /* signer info version = 1 */
    copyData[0] = 1;
    if ( OK > ( status = DER_AddItemCopyData( pSignerInfoItem, INTEGER, 1, copyData, NULL)))
        goto exit;

    /* isssuerAndSerialNumber */
    if (OK > (status = PKCS7_AddIssuerAndSerialNumber(pSignerInfoItem,
                                    pSignerInfo->cs, pSignerInfo->pIssuer,
                                    pSignerInfo->pSerialNumber, NULL)))
    {
        goto exit;
    }

    /* digestAlgorithm */
    if ( OK > ( status = DER_StoreAlgoOID( pSignerInfoItem, pSignerInfo->digestAlgoOID, TRUE)))
        goto exit;

    /* OPTIONAL authenticatedAttributes */

    if (pSignerInfo->authAttrsLen > 0 ||
        !EqualOID(pkcs7_data_OID, payLoadType) )
    {
        if ( OK > ( status = DER_AddTag( pSignerInfoItem, 0, &pAttributeItem)))
            goto exit;

        /*  from PKCS #7: The Attributes value's tag is SET OF,
        * and the DER encoding of the SET OF tag,
        * rather than of the IMPLICIT [0] tag,
        * is to be digested along with the length and
        * contents octets of the Attributes value. */

        /* pSetOf is a shadow structure used to compute the attribute digest */
        if ( OK > ( status = DER_AddSet(NULL, &pSetOf)))
            goto exit;

        /* mandatory contentType */
        if ( OK > ( status = PKCS7_AddAttribute( pAttributeItem, pkcs9_contentType_OID, OID,  payLoadType+1, payLoadType[0], NULL)))
            goto exit;

        if ( OK > ( status = PKCS7_AddAttribute( pSetOf, pkcs9_contentType_OID, OID,  payLoadType+1, payLoadType[0], NULL)))
            goto exit;


        /* mandatory messageDigest attributes */
        if ( OK > ( status = PKCS7_AddAttribute( pAttributeItem, pkcs9_messageDigest_OID,
                                                 PRIMITIVE|OCTETSTRING, pHash->hashData,
                                                 pHash->hashAlgo->digestSize, NULL)))
        {
            goto exit;
        }

        if ( OK > ( status = PKCS7_AddAttribute( pSetOf, pkcs9_messageDigest_OID,
                                                PRIMITIVE|OCTETSTRING, pHash->hashData,
                                                pHash->hashAlgo->digestSize, NULL)))
        {
            goto exit;
        }

        for (i = 0; i < pSignerInfo->authAttrsLen; i++)
        {
            Attribute *pAttr = pSignerInfo->pAuthAttrs+i;
            if ( OK > ( status = PKCS7_AddAttribute( pAttributeItem, pAttr->typeOID, pAttr->type, pAttr->value, pAttr->valueLen, NULL)))
                goto exit;

            if ( OK > ( status = PKCS7_AddAttribute( pSetOf, pAttr->typeOID, pAttr->type, pAttr->value, pAttr->valueLen, NULL)))
                goto exit;
        }

        if (OK > (status = DER_Serialize(pSetOf, &pDerAttributes, &derAttributesLen)))
            goto exit;

        if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, derAttributesLen + pHash->hashAlgo->digestSize, TRUE, &pTempBuf)))
            goto exit;

        if (derAttributesLen > 0)
        {
            MOC_MEMCPY(pTempBuf, pDerAttributes, derAttributesLen);
        }

        /* compute the second message digest on the authenticated attributes if present */
        pHash->hashAlgo->initFunc( MOC_HASH(hwAccelCtx) pHash->bulkCtx);
        pHash->hashAlgo->updateFunc( MOC_HASH(hwAccelCtx) pHash->bulkCtx, pTempBuf, derAttributesLen);
        pHash->hashAlgo->finalFunc( MOC_HASH(hwAccelCtx) pHash->bulkCtx, pTempBuf + derAttributesLen);
    }

    /* digestEncrytionAlgorithm */
    if ( pSignerInfo->pKey->type == akt_rsa)
    {
        if ( OK > ( status = DER_StoreAlgoOID( pSignerInfoItem, rsaEncryption_OID, TRUE)))
            goto exit;
    }
#ifdef __ENABLE_MOCANA_DSA__
    else if ( pSignerInfo->pKey->type == akt_dsa)
    {
        if ( OK > ( status = PKCS7_AddDSADigestAlgoId(pSignerInfoItem, pHash->hashType)))
            goto exit;
    }
#endif
#ifdef __ENABLE_MOCANA_ECC__
    else if ( pSignerInfo->pKey->type == akt_ecc)
    {
        if (OK > ( status = PKCS7_AddECDSADigestAlgoId(pSignerInfoItem, pHash->hashType)))
            goto exit;
    }
#endif
    else
    {
        status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
        goto exit;
    }

    /* encrypt message digest */
    /* encrypt Der encoded DigestInfo if RSA */
    if ( pSignerInfo->pKey->type == akt_rsa)
    {
        const ubyte* toSign;
        ubyte4 dummy;

       /* if authenticated attributes is present, use second hash; else use pHash->hashData */
        toSign =  (pSignerInfo->authAttrsLen > 0 ||
                    !EqualOID(pkcs7_data_OID, payLoadType) ) ?
                    pTempBuf + derAttributesLen:
                    pHash->hashData;

        if (OK > ( status = PKCS7_AddRSASignature(MOC_RSA(hwAccelCtx) pSignerInfoItem,
                                                    pSignerInfo->pKey->key.pRSA,
                                                    pSignerInfo->digestAlgoOID,
                                                    toSign, pHash->hashAlgo->digestSize,
                                                    ppDataBuffer,
                                                    &dummy)))
        {
            goto exit;
        }
    }
#ifdef __ENABLE_MOCANA_DSA__
    else if ( pSignerInfo->pKey->type == akt_dsa)
    {
        /* if authenticated attributes is present, use second hash; else use pHash->hashData */
        const ubyte* toSign;
        ubyte4 dummy;

        if (pSignerInfo->authAttrsLen > 0 ||
            (pkcs7_data_OID[pkcs7_data_OID[0]] != payLoadType[payLoadType[0]]))
        {
            toSign = pTempBuf + derAttributesLen;
        }
        else
        {
            toSign = pHash->hashData;
        }

        if (OK > ( status = PKCS7_AddDSASignature( pSignerInfoItem,
                                pSignerInfo->pKey->key.pDSA, rngFun, rngArg,
                                toSign, pHash->hashAlgo->digestSize,
                                ppDataBuffer, &dummy)))
        {
            goto exit;
        }
    }
#endif
#ifdef __ENABLE_MOCANA_ECC__
    else if ( pSignerInfo->pKey->type == akt_ecc)
    {
        /* if authenticated attributes is present, use second hash; else use pHash->hashData */
        const ubyte* toSign;
        ubyte4 dummy;

        if (pSignerInfo->authAttrsLen > 0 ||
            (pkcs7_data_OID[pkcs7_data_OID[0]] != payLoadType[payLoadType[0]]))
        {
            toSign = pTempBuf + derAttributesLen;
        }
        else
        {
            toSign = pHash->hashData;
        }

        if (OK > ( status = PKCS7_AddECDSASignature( pSignerInfoItem,
                                pSignerInfo->pKey->key.pECC, rngFun, rngArg,
                                toSign, pHash->hashAlgo->digestSize,
                                ppDataBuffer, &dummy)))
        {
            goto exit;
        }
    }
#endif

    /* OPTIONAL unauthenticatedAttributes */

exit:
    if (pTempBuf)
        CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);

    if (pSetOf)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSetOf);
    }

    if (pDerAttributes)
    {
        FREE(pDerAttributes);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS7_SignData( MOC_RSA_HASH(hwAccelDescr hwAccelCtx)
                        DER_ITEMPTR pStart,
                        DER_ITEMPTR pParent,
                        ASN1_ITEMPTR pCACertificates[/*numCACerts*/],
                        CStream pCAStreams[/*numCACerts*/],
                        sbyte4 numCACerts,
                        ASN1_ITEMPTR pCrls[/*numCrls*/],
                        CStream pCrlStreams[/*numCrls*/],
                        sbyte4 numCrls,
                        signerInfoPtr *pSignerInfos, /* if NULL, degenerate case */
                        ubyte4 numSigners, /* number of signers */
                        const ubyte* payLoadType, /* OID, if NULL, degenerate case*/
                        const ubyte* pPayLoad,
                        ubyte4 payLoadLen,
                        RNGFun rngFun,
                        void* rngFunArg,
                        ubyte** ppSigned,
                        ubyte4* pSignedLen)
{
    return PKCS7_SignDataEx(MOC_RSA_HASH(hwAccelCtx) 0, pStart, pParent, pCACertificates, pCAStreams, numCACerts,
                            pCrls, pCrlStreams, numCrls, pSignerInfos, numSigners,
                            payLoadType, pPayLoad, payLoadLen, rngFun, rngFunArg,
                            ppSigned, pSignedLen);
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS7_SignDataEx( MOC_RSA_HASH(hwAccelDescr hwAccelCtx)
                    ubyte4 flags,
                    DER_ITEMPTR pStart,
                    DER_ITEMPTR pParent,
                    ASN1_ITEMPTR pCACertificates[/*numCACerts*/],
                    CStream pCAStreams[/*numCACerts*/],
                    sbyte4 numCACerts,
                    ASN1_ITEMPTR pCrls[/*numCrls*/],
                    CStream pCrlStreams[/*numCrls*/],
                    sbyte4 numCrls,
                    signerInfoPtr *pSignerInfos, /* if NULL, degenerate case */
                    ubyte4 numSigners, /* number of signers */
                    const ubyte* payLoadType, /* OID, if NULL, degenerate case*/
                    const ubyte* pPayLoad,
                    ubyte4 payLoadLen,
                    RNGFun rngFun,
                    void* rngFunArg,
                    ubyte** ppSigned,
                    ubyte4* pSignedLen)
{
    MSTATUS         status = OK;
    DER_ITEMPTR     pSignedData = NULL;
    DER_ITEMPTR     pTempItem,
                    pSignerInfosItem;
    ubyte**         pDataBuffers = 0; /* keep track of allocated buffers
                                      referenced by added DER_ITEM (signatures) */
    ubyte           copyData[MAX_DER_STORAGE];
    ubyte4          i;
    ubyte4          signedBufferLen;
    ubyte          *signedBuffer = NULL;
    ubyte          *pTempBuf = NULL;
    SignedDataHash *pSignedDataHash = 0;
    ubyte4          numHashes = 0;
    ubyte4          hashes = 0;

    if (!ppSigned || !pSignedLen)
    {
        return ERR_NULL_POINTER;
    }

    if ( OK > ( status = DER_AddSequence( pParent, &pSignedData)))
        goto exit;

    /* version = 1 */
    copyData[0] = 1;
    if ( OK > ( status = DER_AddItemCopyData( pSignedData, INTEGER, 1, copyData, NULL)))
        goto exit;

    /* digestAlgorithms */
    if ( OK > ( status = DER_AddSet( pSignedData, &pTempItem)))
        goto exit;

    /* Add all unique digestAlgos */
    /* NOTE: we are computing digest for each signer. one optimization,
     * when there are multiple signers present, is to compute the digest
     * once for each unique digest algorithm, and reuse the digest for all
     * signers with the same digest algorithm.
     */
    if (numSigners > 0)
    {
        if (payLoadLen > 0)
        {
            if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, payLoadLen, TRUE, &pTempBuf)))
                goto exit;

            MOC_MEMCPY(pTempBuf, pPayLoad, payLoadLen);
        }

        for (i = 0; i < numSigners; i++)
        {
            ubyte hashId;
            if ( OK > (status = PKCS7_GetHashAlgoIdFromHashAlgoOID2(
                                    pSignerInfos[i]->digestAlgoOID,
                                    &hashId)))
            {
                goto exit;
            }

            hashes |= (1 << hashId);
        }

        if (OK > ( PKCS7_ConstructHashes( MOC_HASH(hwAccelCtx) hashes, &numHashes, &pSignedDataHash)))
        {
            goto exit;
        }

        for (i = 0; i < numHashes; ++i)
        {
            if ( OK > ( status = DER_StoreAlgoOID( pTempItem,
                                                   pSignedDataHash[i].algoOID,
                                                   TRUE)))
            {
                goto exit;
            }
            pSignedDataHash[i].hashAlgo->updateFunc( MOC_HASH(hwAccelCtx)
                                                    pSignedDataHash[i].bulkCtx,
                                                    pTempBuf, payLoadLen);
            pSignedDataHash[i].hashAlgo->finalFunc( MOC_HASH(hwAccelCtx)
                                                    pSignedDataHash[i].bulkCtx,
                                                    pSignedDataHash[i].hashData);
        }
        CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
        pTempBuf = NULL;
    }
    /* else the degenerate case will be an empty set */

    /* contentInfo */
    if (flags & PKCS7_EXTERNAL_SIGNATURES)
    {
        if (OK > (status = PKCS7_AddContentInfo(pSignedData, payLoadType, NULL, 0, NULL)))
            goto exit;
    }
    else
    {
        if (OK > (status = PKCS7_AddContentInfo(pSignedData, payLoadType, pPayLoad, payLoadLen, NULL)))
            goto exit;
    }
    /* OPTIONAL certificates, it should be present for ease of verification */
    if (numCACerts > 0)
    {
        if (OK > (status = PKCS7_AddSetOfOrSequenceOfASN1ItemsWithTag(pSignedData,
            0, 0, pCAStreams, pCACertificates, numCACerts, NULL)))
            goto exit;
    }

    /* OPTIONAL crls */
    if (numCrls > 0)
    {
        if (OK > (status = PKCS7_AddSetOfOrSequenceOfASN1ItemsWithTag(pSignedData,
            1, 0, pCrlStreams, pCrls, numCrls, NULL)))
            goto exit;
    }

    if ( OK > ( status = DER_AddSet ( pSignedData, &pSignerInfosItem)))
        goto exit;

    if (numSigners > 0)
    {
        /* allocate space for keeping track of signature buffers */
        if (NULL == (pDataBuffers = MALLOC( numSigners * sizeof( const ubyte*))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        for (i = 0; i < numSigners; i++)
        {
            pDataBuffers[i] = NULL;
        }

        for ( i = 0; i < numSigners; ++i)
        {
            /* figure out the hash for this signer */
            ubyte4 j;
            SignedDataHash* pHash = 0;

            for (j = 0; j< numHashes; ++j)
            {
                if ( EqualOID( pSignerInfos[i]->digestAlgoOID,
                                pSignedDataHash[j].algoOID))
                {
                    pHash = pSignedDataHash+j;
                    break;
                }
            }

            if (!pHash)
            {
                status = ERR_PKCS7_INVALID_STRUCT;
                goto exit;
            }

            if (OK > (status = PKCS7_AddPerSignerInfo(MOC_RSA_HASH(hwAccelCtx) pSignerInfosItem,
                        pSignerInfos[i], pHash, rngFun,  rngFunArg,
                        (ubyte*)payLoadType, &(pDataBuffers[i]) )))
            {
                goto exit;
            }
        }
    }

    /* write everything to our buffer */
    if ( OK > ( status = DER_Serialize( pStart ? pStart : pSignedData,
                                        &signedBuffer, &signedBufferLen)))
    {
        goto exit;
    }

    /* return the buffer */
    *ppSigned = signedBuffer;
    signedBuffer = NULL;
    *pSignedLen = signedBufferLen;

exit:
    if (pTempBuf)
        CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);

    /* delete the buffers holding the encryptedDigest */
    if (pDataBuffers)
    {
        for (i = 0; i < numSigners; i++)
        {
            if (pDataBuffers[i])
            {
                FREE(pDataBuffers[i]);
            }
        }
        FREE(pDataBuffers);
    }

    /* delete the DER tree */
    if (pSignedData)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSignedData);
    }

    if (pSignedDataHash)
    {
        PKCS7_DestructHashes(MOC_HASH(hwAccelCtx) numHashes, &pSignedDataHash);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS7_DigestData( MOC_HASH(hwAccelDescr hwAccelCtx)
                    DER_ITEMPTR pStart, /* can be null */
                    DER_ITEMPTR pParent,
                    const ubyte* payloadType, /* OID */
                    ubyte hashType,
                    const ubyte* pPayload,
                    ubyte4 payloadLen,
                    ubyte** ppDigested,
                    ubyte4* pDigestedLen)
{
    MSTATUS status;
    DER_ITEMPTR pDigested = 0;
    DER_ITEMPTR pDigestedData;
    DER_ITEMPTR pSeq, pTag;
    const BulkHashAlgo* pBHA;
    ubyte* digest = 0;
    const ubyte* oid;
    BulkCtx ctx = 0;

    if (!ppDigested || ! pDigestedLen)
    {
        return ERR_NULL_POINTER;
    }

    if (!payloadType)
    {
        payloadType = pkcs7_data_OID;
    }

    if (OK > ( status = CRYPTO_getHashAlgoOID( hashType, &oid)))
    {
        goto exit;
    }

    /* compute the hash */
    if (OK > ( status = CRYPTO_getRSAHashAlgo( hashType, &pBHA)))
    {
        goto exit;
    }

    if (OK > ( status = CRYPTO_ALLOC( hwAccelCtx, pBHA->digestSize, 1, &digest)))
    {
        goto exit;
    }

    if (OK > ( status = pBHA->allocFunc(MOC_HASH(hwAccelCtx) &ctx)))
    {
        goto exit;
    }
    if (OK > ( status = pBHA->initFunc(MOC_HASH(hwAccelCtx)ctx)))
    {
        goto exit;
    }
    if (OK > ( status = pBHA->updateFunc(MOC_HASH(hwAccelCtx) ctx, pPayload,
                                            payloadLen)))
    {
        goto exit;
    }
    if (OK > ( status = pBHA->finalFunc(MOC_HASH(hwAccelCtx) ctx, digest)))
    {
        goto exit;
    }

    /* build the DER */
    if (OK > ( status = DER_AddSequence(pParent, &pDigested)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddOID(pDigested, pkcs7_digestedData_OID, NULL)))
    {
        goto exit;
    }
    if (OK > ( status = DER_AddTag(pDigested, 0, &pTag)))
    {
        goto exit;
    }
    if (OK > ( status = DER_AddSequence(pTag, &pDigestedData)))
    {
        goto exit;
    }

    /* Version */
    if (OK > ( status = DER_AddIntegerEx(pDigestedData,
                                        EqualOID(payloadType, pkcs7_data_OID) ? 0 : 2,
                                        NULL)))
    {
        goto exit;
    }

    /* AlgorithmIdentifier */
    if (OK > ( status = DER_AddSequence( pDigestedData, &pSeq)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddOID( pSeq, oid, NULL)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddItem( pSeq, NULLTAG, 0, 0, NULL)))
    {
        goto exit;
    }

    /* EncapsulatedContentInfo */
    if (OK > ( status = DER_AddSequence( pDigestedData, &pSeq)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddOID( pSeq, payloadType, NULL)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddTag( pSeq, 0, &pTag)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddItem( pTag, OCTETSTRING, payloadLen, pPayload, NULL)))
    {
        goto exit;
    }
    if (OK > ( status = DER_AddItem( pDigestedData, OCTETSTRING, pBHA->digestSize, digest, NULL)))
    {
        goto exit;
    }

    /* write everything to our buffer */
    if ( OK > ( status = DER_Serialize( pStart ? pStart : pDigested,
                                        ppDigested, pDigestedLen)))
    {
        goto exit;
    }

exit:

    if (ctx)
    {
        pBHA->freeFunc( MOC_HASH(hwAccelCtx) &ctx);
    }

    if (digest)
    {
        CRYPTO_FREE( hwAccelCtx, 1, &digest);
    }

    /* delete the DER tree */
    if (pDigested)
    {
        TREE_DeleteTreeItem( (TreeItem*) pDigested);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS7_GetSignerDigestAlgo( ASN1_ITEMPTR pSignerInfo, CStream cs,
                        ubyte* hashAlgoId)
{
    MSTATUS status;
    ASN1_ITEMPTR pDigestAlgorithm, pOID;

    if (OK > ( status = ASN1_GetNthChild( pSignerInfo, 3, &pDigestAlgorithm)))
    {
        goto exit;
    }

    if (OK > ( status = ASN1_VerifyType( pDigestAlgorithm, SEQUENCE)))
    {
        goto exit;
    }

    pOID = ASN1_FIRST_CHILD( pDigestAlgorithm);

    if (OK > ( status = ASN1_VerifyType( pOID, OID)))
    {
        goto exit;
    }

    if (OK > ( status = PKCS7_GetHashAlgoIdFromHashAlgoOID( pOID, cs, hashAlgoId)))
    {
        goto exit;
    }

exit:

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS7_GetSignerSignatureAlgo( ASN1_ITEMPTR pSignerInfo, CStream cs,
                        ubyte* pubKeyType)
{
    MSTATUS status;
    ASN1_ITEMPTR pSignatureAlgorithm, pOID;
#if defined(__ENABLE_MOCANA_ECC__) || defined(__ENABLE_MOCANA_DSA__)
    ubyte subType;
#endif

    if (OK > ( status = ASN1_GetNthChild( pSignerInfo, 4, &pSignatureAlgorithm)))
    {
        goto exit;
    }

    if (OK <= ASN1_VerifyTag( pSignatureAlgorithm, 0))
    {
        pSignatureAlgorithm = ASN1_NEXT_SIBLING( pSignatureAlgorithm);
    }

    if (OK > ( status = ASN1_VerifyType( pSignatureAlgorithm, SEQUENCE)))
    {
        goto exit;
    }

    pOID = ASN1_FIRST_CHILD( pSignatureAlgorithm);

    if (OK > ( status = ASN1_VerifyType( pOID, OID)))
    {
        goto exit;
    }

    if ( OK <= ASN1_VerifyOID( pOID, cs, rsaEncryption_OID))
    {
        *pubKeyType = akt_rsa;
    }
#ifdef __ENABLE_MOCANA_DSA__
    else if ( OK <= ASN1_VerifyOID( pOID, cs, dsaWithSHA1_OID) ||
            OK <= ASN1_VerifyOIDRoot( pOID, cs, dsaWithSHA2_OID, &subType))
    {
        *pubKeyType = akt_dsa;
    }
#endif
#ifdef __ENABLE_MOCANA_ECC__
    else if ( OK <= ASN1_VerifyOID( pOID, cs, ecdsaWithSHA1_OID) ||
            OK <= ASN1_VerifyOIDRoot( pOID, cs, ecdsaWithSHA2_OID, &subType))
    {
        *pubKeyType = akt_ecc;
    }
#endif
    else
    {
        status = ERR_PKCS7_UNSUPPORTED_ENCRYPTALGO;
        goto exit;
    }

exit:

    return status;
}


/*--------------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
PKCS7_GetSignerSignedAttributes( ASN1_ITEMPTR pSignerInfo,
                        ASN1_ITEMPTR *ppFirstSignedAttribute)
{
    return ASN1_GetChildWithTag( pSignerInfo, 0, ppFirstSignedAttribute);
}


/*--------------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
PKCS7_GetSignerUnsignedAttributes( ASN1_ITEMPTR pSignerInfo,
                        ASN1_ITEMPTR *ppFirstUnsignedAttribute)
{
    return ASN1_GetChildWithTag( pSignerInfo, 0, ppFirstUnsignedAttribute);
}


#ifdef __ENABLE_MOCANA_CMS__

#include "cms.inc"

#endif  /* __ENABLE_MOCANA_PKCS7_STREAM_API__ */

#endif /* __ENABLE_MOCANA_PKCS7__ */
