/* Version: mss_v6_3 */
/*
 * asn1cert.c
 *
 * ASN.1 Certificate Encoding
 *
 * Copyright Mocana Corp 2003-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifndef __DISABLE_MOCANA_CERTIFICATE_GENERATION__

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/random.h"
#include "../common/vlong.h"
#include "../crypto/md5.h"
#include "../crypto/md2.h"
#include "../crypto/md4.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/crypto.h"
#include "../crypto/ca_mgmt.h"
#include "../crypto/rsa.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../harness/harness.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../asn1/parsecert.h"
#include "../asn1/derencoder.h"
#include "../crypto/asn1cert.h"


/*------------------------------------------------------------------*/

static MSTATUS
ASN1CERT_StoreNamePart( DER_ITEMPTR pRoot, const ubyte* oid,
                       const sbyte* namePart, ubyte4 namePartLen, ubyte type)
{
    DER_ITEMPTR pTemp;
    MSTATUS status;

    if ( OK > ( status = DER_AddSet( pRoot, &pTemp)))
        return status;

    if ( OK > ( status = DER_AddSequence( pTemp, &pTemp)))
        return status;

    if ( OK > ( status = DER_AddOID( pTemp, oid, NULL)))
        return status;

    if ( OK > ( status = DER_AddItem( pTemp, type, namePartLen, (ubyte *)namePart, NULL)))
        return status;

    return OK;
}


/*------------------------------------------------------------------*/

MSTATUS
ASN1CERT_StoreDistinguishedName( DER_ITEMPTR pRoot, const certDistinguishedName* pCertInfo)
{
    MSTATUS status;
    relativeDN *pDistinguishedName;
    relativeDN *pRDN;
    ubyte4 i;
    ubyte4 j;
    DER_ITEMPTR pSequence;

    if (NULL == pCertInfo || NULL == pCertInfo->pDistinguishedName)
    {
        return ERR_NULL_POINTER;
    }

    pDistinguishedName = pCertInfo->pDistinguishedName;

    if (OK > (status = DER_AddSequence( pRoot, &pSequence)))
        return status;

    for (i = 0, pRDN = pDistinguishedName; i < pCertInfo->dnCount; i++, pRDN = (pDistinguishedName+i))
    {
        nameAttr *pNameComponent;
        for (j = 0, pNameComponent = pRDN->pNameAttr; j < pRDN->nameAttrCount; j++, pNameComponent = (pRDN->pNameAttr+j))
        {
            if (!pNameComponent || !pNameComponent->value || !pNameComponent->oid)
            {
                FREE(pSequence);
                return ERR_NULL_POINTER;
            }

            if (pNameComponent->oid == pkcs9_emailAddress_OID) /* add domainComponent later */
            {
                if (OK > (status = ASN1CERT_StoreNamePart( pSequence, pNameComponent->oid,
                    (sbyte *)pNameComponent->value, pNameComponent->valueLen,
                    IA5STRING)))
                    return status;
            } else
            {
                if (OK > (status = ASN1CERT_StoreNamePart( pSequence, pNameComponent->oid,
                    (sbyte *)pNameComponent->value, pNameComponent->valueLen,
                    (pNameComponent->type == 0? PRINTABLESTRING: pNameComponent->type))))
                    return status;
            }
        }
    }

    return OK;
}


/*------------------------------------------------------------------*/

static MSTATUS
ASN1CERT_AddExtensionsToTBSCertificate(DER_ITEMPTR pCertificate,
                       const certExtensions* pExtensions)
{
    MSTATUS         status;
    DER_ITEMPTR     pTempItem;

    if ( !pExtensions)
        return ERR_NULL_POINTER;

    if ( !pExtensions->hasBasicConstraints && !pExtensions->hasKeyUsage && !pExtensions->otherExts)
        return OK; /* nothing to do */

    /* add the tag for extensions [3] and a sequence */
    if (OK > ( status = DER_AddTag( pCertificate, 3, &pTempItem)))
        goto exit;

    if (OK > ( status = ASN1CERT_AddExtensions(pTempItem, pExtensions, NULL)))
        goto exit;
exit:
    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
ASN1CERT_AddExtensions(DER_ITEMPTR pExtensionTag,
                          const certExtensions* pExtensions, DER_ITEMPTR *ppExtsItem)
{
    MSTATUS status;
    DER_ITEMPTR    pExtsItem, pTempItem;
    ubyte           copyData[MAX_DER_STORAGE];

    if ( !pExtensions)
        return ERR_NULL_POINTER;

    if ( !pExtensions->hasBasicConstraints && !pExtensions->hasKeyUsage && !pExtensions->otherExts)
        return OK; /* nothing to do */

    if (OK > ( status = DER_AddSequence( pExtensionTag, &pExtsItem)))
        goto exit;

    if (ppExtsItem)
    {
        *ppExtsItem = pExtsItem;
    }

    /* add basicConstraints if there */
    if (pExtensions->hasBasicConstraints)
    {
        if (OK > ( status = DER_AddSequence( pExtsItem, &pTempItem)))
            goto exit;

        if (OK > (status = DER_AddOID( pTempItem, basicConstraints_OID, NULL)))
            goto exit;

        /*
         * Section 4.2.1.10  Basic Constraints: This extension MUST appear as
         * a critical extension in all CA certificates.
         */
        if (pExtensions->isCA)
        {
            copyData[0] = 0xFF;
            if (OK > ( status = DER_AddItemCopyData( pTempItem, BOOLEAN, 1, copyData, NULL)))
                goto exit;
        }

        if (OK > (status = DER_AddItem( pTempItem, OCTETSTRING, 0, NULL, &pTempItem)))
            goto exit;

        if (OK > ( status = DER_AddSequence( pTempItem, &pTempItem)))
            goto exit;

        copyData[0] = pExtensions->isCA ? 0xFF : 0x00;
        if (OK > ( status = DER_AddItemCopyData( pTempItem, BOOLEAN, 1, copyData, NULL)))
            goto exit;

        /* add OPTIONAL certPathLen if positive */
        if ( pExtensions->certPathLen >= 0)
        {
            copyData[0] = pExtensions->certPathLen;
            if (OK > ( status = DER_AddItemCopyData(pTempItem, INTEGER, 1, copyData, NULL)))
                goto exit;
        }
    }

    /* add key usage if there */
    if (pExtensions->hasKeyUsage)
    {
        if (OK > ( status = DER_AddSequence( pExtsItem, &pTempItem)))
            goto exit;

        if (OK > (status = DER_AddOID( pTempItem, keyUsage_OID, NULL)))
            goto exit;

        /*
         * Section 4.2.1.3 Key Usage: When used, this extension SHOULD be marked critical.
         */
        copyData[0] = 0xFF;
        if (OK > ( status = DER_AddItemCopyData( pTempItem, BOOLEAN, 1, copyData, NULL)))
            goto exit;

        if (OK > (status = DER_AddItem( pTempItem, OCTETSTRING, 0, NULL, &pTempItem)))
            goto exit;

        /* but the data in little endian order: least significant bit is always first  */
        copyData[1] = (ubyte) (pExtensions->keyUsage >> 8);
        copyData[0] = (ubyte) (pExtensions->keyUsage);
        if ( OK > ( status = DER_AddBitString( pTempItem, 2, copyData, NULL)))
            goto exit;
    }

    if (pExtensions->otherExts)
    {
        ubyte4 i;
        extensions *pExt;
        for (i = 0, pExt = pExtensions->otherExts; i < pExtensions->otherExtCount; i++, pExt = pExtensions->otherExts + i)
        {
            /* add a single extension */
            if ( OK > (status = DER_AddSequence( pExtsItem, &pTempItem)))
                goto exit;
            /* extnID */
            if ( OK > (status = DER_AddOID(pTempItem, pExt->oid, NULL)))
                goto exit;
            /* if crital == TRUE, add critical */
            if (pExt->isCritical)
            {
                copyData[0] = 0xFF;
                if (OK > ( status = DER_AddItemCopyData( pTempItem, BOOLEAN, 1, copyData, NULL)))
                    goto exit;
            }
            /* extnValue */
            if (OK > (status = DER_AddItem(pTempItem, OCTETSTRING, pExt->valueLen, pExt->value, NULL)))
                goto exit;
        }
    }
exit:

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
ASN1CERT_rsaSignAux( MOC_RSA_HASH(hwAccelDescr hwAccelCtx) RSAKey *pRSAKey,
                DER_ITEMPTR pToSign, DER_ITEMPTR pSignature, ubyte signAlgo)
{
    ubyte*          pHash = NULL;
    ubyte*          pTempBuf = NULL;
    ubyte*          pData;
    ubyte4          dataLen;
    ubyte4          digestSize = 0;
    DER_ITEMPTR     pSequence = 0;
    ubyte*          pBuffer = 0;
    ubyte*          serializedBitStr;
    const ubyte*    hashAlgoOID = 0;
    MSTATUS         status = OK;

    if (OK > ( status = DER_GetASNBufferInfo( pToSign, &pData, &dataLen)))
       goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, CERT_MAXDIGESTSIZE, TRUE, &pHash)))
        goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, dataLen, TRUE, &pTempBuf)))
        goto exit;

    MOC_MEMCPY(pTempBuf, pData, dataLen);

    switch ( signAlgo )
    {
        case md5withRSAEncryption:
            digestSize = MD5_RESULT_SIZE;
            hashAlgoOID = md5_OID;
            status = MD5_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;

        case sha1withRSAEncryption:
            digestSize = SHA1_RESULT_SIZE;
            hashAlgoOID = sha1_OID;
            status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;

#ifndef __DISABLE_MOCANA_SHA224__
        case sha224withRSAEncryption:
            digestSize = SHA224_RESULT_SIZE;
            hashAlgoOID = sha224_OID;
            status = SHA224_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA256__
        case sha256withRSAEncryption:
            digestSize = SHA256_RESULT_SIZE;
            hashAlgoOID = sha256_OID;
            status = SHA256_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA384__
        case sha384withRSAEncryption:
            digestSize = SHA384_RESULT_SIZE;
            hashAlgoOID = sha384_OID;
            status = SHA384_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA512__
        case sha512withRSAEncryption:
            digestSize = SHA512_RESULT_SIZE;
            hashAlgoOID = sha512_OID;
            status = SHA512_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

        default:
            status = ERR_CERT_AUTH_BAD_SIGN_ALGO;
            break;
    }

    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
    pTempBuf = NULL;
    if (OK > status) goto exit;

    /* now construct a new ASN.1 DER encoding with this */
    if ( OK > (status = DER_AddSequence( NULL, &pSequence)))
        goto exit;

    if ( OK > ( status = DER_StoreAlgoOID( pSequence, hashAlgoOID, TRUE)))
       goto exit;

    if ( OK > ( status = DER_AddItem( pSequence, OCTETSTRING, digestSize, pHash, NULL)))
       goto exit;

    if ( OK > ( status = DER_Serialize( pSequence, &pBuffer, &dataLen)))
        goto exit;

    /* put the signature directly into the BITSTRING data */
    if ( OK > ( status = DER_GetSerializedDataPtr( pSignature, &serializedBitStr)))
        goto exit;

    if ( OK > ( status = RSA_signMessage(MOC_RSA(hwAccelCtx) pRSAKey, pBuffer, dataLen,
                                            serializedBitStr + 1, NULL)))
        goto exit;

    /* then make sure unused bits is set to 0 */
    *serializedBitStr = 0x00;

exit:
    if (pHash)
    {
        CRYPTO_FREE(hwAccelCtx, TRUE, &pHash);
    }
    if (pTempBuf)
    {
        CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
    }
    if ( pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    if ( pBuffer)
    {
        FREE( pBuffer);
    }

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
ASN1CERT_rsaSign(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) DER_ITEMPTR pSignedHead,
                 RSAKey* pRSAKey, const ubyte* signAlgoOID,
                 ubyte **ppRetDEREncoding, ubyte4 *pRetDEREncodingLen)
{
    /* this function is more complicated so that we do one and only
    one memory allocation to save memory and to avoid fragmentation */

    MSTATUS status = OK;
    ubyte* pRetDEREncoding = 0;
    ubyte4 retDEREncodingLen;
    sbyte4 signatureLen, cmpRes;
    DER_ITEMPTR pBitString;

    if ((0 == pSignedHead) || (0 == pRSAKey) || (0 == signAlgoOID) ||
        (0 == ppRetDEREncoding) || ( 0 == pRetDEREncodingLen))
    {
        return ERR_NULL_POINTER;
    }

    /* verify this is a signAlgo we support */
    if ( PKCS1_OID_LEN+1 != signAlgoOID[0])
    {
        return ERR_CERT_AUTH_BAD_SIGN_ALGO;
    }

    MOC_MEMCMP( signAlgoOID + 1, pkcs1_OID + 1, PKCS1_OID_LEN, &cmpRes);
    if ( 0 != cmpRes )
    {
        return ERR_CERT_AUTH_BAD_SIGN_ALGO;
    }

    /* signature algo */
    if ( OK > (status = DER_StoreAlgoOID( pSignedHead, signAlgoOID, TRUE)))
        goto exit;

    /* signature (place holder for the moment) */
    if ( OK > ( status = RSA_getCipherTextLength( pRSAKey, &signatureLen)))
        goto exit;

    if ( OK > ( status = DER_AddItem( pSignedHead, BITSTRING, signatureLen + 1, /* +1 unused bits octets */
                                        NULL, &pBitString)))
        goto exit;

    /* write the whole thing */
    if ( OK > ( status = DER_Serialize( pSignedHead, &pRetDEREncoding, &retDEREncodingLen)))
        goto exit;

    /* now generate the signature */
    if ( OK > ( status = ASN1CERT_rsaSignAux( MOC_RSA_HASH(hwAccelCtx) pRSAKey, DER_FIRST_CHILD(pSignedHead),
                                            pBitString, signAlgoOID[ 1 + PKCS1_OID_LEN])))
        goto exit;

    *pRetDEREncodingLen = retDEREncodingLen;
    *ppRetDEREncoding = pRetDEREncoding;
    pRetDEREncoding = NULL;

exit:

    if ( pRetDEREncoding)
    {
        FREE (pRetDEREncoding);
    }
    return status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static MSTATUS
ASN1CERT_eccSignAux(MOC_HASH(hwAccelDescr hwAccelCtx) ECCKey* pSignKey,
                    PrimeFieldPtr pPF, DER_ITEMPTR pToSign, ubyte signAlgo,
                    RNGFun rngFun, void* rngFunArg,
                    ubyte* pR, ubyte* pS, sbyte4 buffLen)
{
    ubyte*          pHash = NULL;
    ubyte*          pTempBuf = NULL;
    ubyte*          pData;
    ubyte4          dataLen;
    ubyte4          digestSize = 0;
    MSTATUS         status = OK;
    PFEPtr          r = 0, s = 0;

    if (OK > ( status = DER_GetASNBufferInfo( pToSign, &pData, &dataLen)))
        goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, CERT_MAXDIGESTSIZE, TRUE, &pHash)))
        goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, dataLen, TRUE, &pTempBuf)))
        goto exit;

    MOC_MEMCPY(pTempBuf, pData, dataLen);

    switch ( signAlgo )
    {
        case sha1withRSAEncryption:
            digestSize = SHA1_RESULT_SIZE;
            status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;

#ifndef __DISABLE_MOCANA_SHA224__
        case sha224withRSAEncryption:
            digestSize = SHA224_RESULT_SIZE;
            status = SHA224_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA256__
        case sha256withRSAEncryption:
            digestSize = SHA256_RESULT_SIZE;
            status = SHA256_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA384__
        case sha384withRSAEncryption:
            digestSize = SHA384_RESULT_SIZE;
            status = SHA384_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA512__
        case sha512withRSAEncryption:
            digestSize = SHA512_RESULT_SIZE;
            status = SHA512_completeDigest(MOC_HASH(hwAccelCtx) pTempBuf, dataLen, pHash);
            break;
#endif

        default:
            status = ERR_CERT_AUTH_BAD_SIGN_ALGO;
            break;
    }

    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
    pTempBuf = NULL;
    if (OK > status) goto exit;

    if (OK > ( status = PRIMEFIELD_newElement( pPF, &r)))
        goto exit;

    if (OK > (status = PRIMEFIELD_newElement( pPF, &s)))
        goto exit;

    if (OK > ( status =  ECDSA_sign( pSignKey->pCurve, pSignKey->k, rngFun,
                                        rngFunArg, pHash, digestSize, r, s)))
    {
        goto exit;
    }

    /* write R */
    if ( OK > ( status = PRIMEFIELD_writeByteString( pPF, r, pR, buffLen)))
        goto exit;

    /* write S */
    if ( OK > ( status = PRIMEFIELD_writeByteString( pPF, s, pS, buffLen)))
        goto exit;

exit:

    if (pHash)
    {
        CRYPTO_FREE(hwAccelCtx, TRUE, &pHash);
    }
    if (pTempBuf)
    {
        CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);
    }

    PRIMEFIELD_deleteElement( pPF, &r);
    PRIMEFIELD_deleteElement( pPF, &s);

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_ECC__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static MSTATUS
ASN1CERT_eccSign(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) DER_ITEMPTR pSignedHead,
              ECCKey* pSignKey, const ubyte* signAlgoOID,
              RNGFun rngFun, void* rngFunArg,
              ubyte **ppRetDEREncoding, ubyte4 *pRetDEREncodingLen)
{
    MSTATUS         status;
    ubyte*          pRetDEREncoding = 0;
    ubyte4          retDEREncodingLen;
    sbyte4          cmpRes;
    sbyte4          elementLen;
    ubyte           copyData[MAX_DER_STORAGE];
    DER_ITEMPTR     pTemp, pR, pS;
    PrimeFieldPtr   pPF;
    ubyte*          pSignatureBuffer = 0;
    ubyte*          pRBuffer;
    ubyte*          pSBuffer;
    ubyte4          offset;
    ubyte           signAlgo;

    /* this function is more complicated so that we do one and only
    one memory allocation to save memory and to avoid fragmentation */

    if ((0 == pSignedHead) || (0 == pSignKey) || (0 == signAlgoOID) ||
        (0 == ppRetDEREncoding) || ( 0 == pRetDEREncodingLen))
    {
        return ERR_NULL_POINTER;
    }

    /* verify this is a signAlgo we support */
    status = ERR_CERT_AUTH_BAD_SIGN_ALGO;

    if ( *ecdsaWithSHA1_OID == signAlgoOID[0])
    {
        MOC_MEMCMP( signAlgoOID + 1, ecdsaWithSHA1_OID + 1, *ecdsaWithSHA1_OID, &cmpRes);
        if ( 0 == cmpRes )
        {
            signAlgo = ht_sha1;
            status = OK;
        }
    }

    if ( OK > status && *ecdsaWithSHA2_OID == signAlgoOID[0] - 1)
    {
        MOC_MEMCMP( signAlgoOID + 1, ecdsaWithSHA2_OID + 1, *ecdsaWithSHA2_OID, &cmpRes);
        if ( 0 == cmpRes )
        {
            switch (signAlgoOID[ signAlgoOID[0]])
            {
            case 1:
                signAlgo = ht_sha224;
                status = OK;
                break;

            case 2:
                signAlgo = ht_sha256;
                status = OK;
                break;

            case 3:
                signAlgo = ht_sha384;
                status = OK;
                break;

            case 4:
                signAlgo = ht_sha512;
                status = OK;
                break;

            default:
                break;
            }
        }
    }

    /* if OK >  status -> the algorithm OID did not match anything */
    if (OK > status)
    {
        goto exit;
    }

    /* signature algo */
    if ( OK > (status = DER_AddSequence( pSignedHead, &pTemp)))
        goto exit;

    if ( OK > (status = DER_AddOID( pTemp, signAlgoOID, NULL)))
        goto exit;

    /* signature is a BITSTRING that encapsulate a sequence of
    2 INTEGERS -- might need to be padded with 0 on left so that
    INTEGER is not negative */
    pPF = EC_getUnderlyingField( pSignKey->pCurve);

    copyData[0] = 0; /* unused bits */
    if (OK > ( status = DER_AddItemCopyData( pSignedHead, BITSTRING, 1, copyData, &pTemp)))
        goto exit;

    /* allocate a single buffer for the signature parameter */
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

    if (OK > ( status = DER_AddSequence( pTemp, &pTemp)))
        goto exit;

    /* add the two integers --use maximum size i.e. assume leading zero */
    if (OK > ( status = DER_AddItem( pTemp, INTEGER, 1 + elementLen, pRBuffer, &pR)))
        goto exit;

    if (OK > ( status = DER_AddItem( pTemp, INTEGER, 1 + elementLen, pSBuffer, &pS)))
        goto exit;

    /* write the whole thing */
    if ( OK > ( status = DER_Serialize( pSignedHead, &pRetDEREncoding, &retDEREncodingLen)))
        goto exit;

    /* now generate the signature in the buffers after the leading zero */
    if ( OK > ( status = ASN1CERT_eccSignAux( MOC_HASH(hwAccelCtx) pSignKey, pPF,
                                                DER_FIRST_CHILD(pSignedHead),
                                                signAlgo, rngFun, rngFunArg,
                                                pRBuffer + 1, pSBuffer + 1,
                                                elementLen)))
    {
        goto exit;
    }

    /* format the buffers for proper INTEGER DER encoding */
    if ( OK > ( status = DER_GetIntegerEncodingOffset( 1 + elementLen, pRBuffer, &offset)))
        goto exit;
    if (OK > (status = DER_SetItemData( pR, elementLen + 1 - offset, pRBuffer + offset)))
        goto exit;

    if ( OK > ( status = DER_GetIntegerEncodingOffset( 1 + elementLen, pSBuffer, &offset)))
        goto exit;
    if (OK > (status = DER_SetItemData( pS, elementLen + 1 - offset, pSBuffer + offset)))
        goto exit;

    /* in every case, we need to rewrite the certificate in the allocated buffer */
    if (OK > ( status = DER_SerializeInto( pSignedHead, pRetDEREncoding, &retDEREncodingLen)))
        goto exit;

    *pRetDEREncodingLen = retDEREncodingLen;
    *ppRetDEREncoding = pRetDEREncoding;
    pRetDEREncoding = NULL;

exit:

    if ( pSignatureBuffer)
    {
        FREE( pSignatureBuffer);
    }


    if ( pRetDEREncoding)
    {
        FREE (pRetDEREncoding);
    }
    return status;
}
#endif /* (defined(__ENABLE_MOCANA_ECC__)) */


/*------------------------------------------------------------------*/

MSTATUS
ASN1CERT_sign(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) DER_ITEMPTR pSignedHead,
              AsymmetricKey* pSignKey, const ubyte* signAlgoOID,
              RNGFun rngFun, void* rngFunArg,
              ubyte **ppRetDEREncoding, ubyte4 *pRetDEREncodingLen)
{
    switch ( pSignKey->type)
    {
        case akt_rsa:
            return ASN1CERT_rsaSign(MOC_RSA_HASH(hwAccelCtx) pSignedHead,
                                    pSignKey->key.pRSA, signAlgoOID,
                                    ppRetDEREncoding, pRetDEREncodingLen);
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
            return ASN1CERT_eccSign(MOC_HASH(hwAccelCtx) pSignedHead,
                                    pSignKey->key.pECC, signAlgoOID,
                                    rngFun, rngFunArg,
                                    ppRetDEREncoding, pRetDEREncodingLen);
#endif

    default:
        break;
    }

    return ERR_BAD_KEY_TYPE;
}


/*------------------------------------------------------------------*/

static MSTATUS
ASN1CERT_storeRSAPublicKeyInfo( RSAKey* pRSAKey, DER_ITEMPTR pCertificate)
{
    DER_ITEMPTR pTemp;
    MSTATUS status;
    ubyte       copyData[MAX_DER_STORAGE];
    sbyte4      expLen, modulusLen;
    ubyte*      expModulusBuffer = 0;
    ubyte*      pE;
    ubyte*      pN;

    if ( (0 == pRSAKey) || (0 == pCertificate) )
    {
        return ERR_NULL_POINTER;
    }

    /* subject public key */
    if (OK > ( status = DER_AddSequence( pCertificate, &pTemp)))
        goto exit;

    if (OK > ( status = DER_StoreAlgoOID( pTemp, rsaEncryption_OID, TRUE)))
        goto exit;

    copyData[0] = 0; /* unused bits */
    if (OK > ( status = DER_AddItemCopyData( pTemp, BITSTRING, 1, copyData, &pTemp)))
        goto exit;


    /* allocate a single buffer for the key parameter */
    if (OK > (status = VLONG_byteStringFromVlong(RSA_N(pRSAKey), NULL, &modulusLen)))
        goto exit;

    if (OK > (status = VLONG_byteStringFromVlong(RSA_E(pRSAKey), NULL, &expLen)))
        goto exit;

    if ((0 == modulusLen) || (0 == expLen))
    {
        status = ERR_BAD_KEY;
        goto exit;
    }

    /* add 2 bytes to prevent negative values */
    expModulusBuffer = MALLOC( modulusLen + expLen + 2);
    if ( !expModulusBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    pE = expModulusBuffer;
    *pE = 0x00; /* leading 0 */
    pN = expModulusBuffer + 1 + expLen;
    *pN = 0x00; /* leading 0 */
    if (OK > (status = VLONG_byteStringFromVlong(RSA_N(pRSAKey), pN + 1, &modulusLen)))
        goto exit;

    /* make the sequence the owner of the buffer but set length to 0 so that it is not
        written as the sequence data! */
    if (OK > ( status = DER_AddItemOwnData( pTemp, (CONSTRUCTED|SEQUENCE), 0, &expModulusBuffer, &pTemp)))
        goto exit;

    /* add the modulus with the leading zero -- DER_AddInteger will do the right thing */
    if (OK > ( status = DER_AddInteger( pTemp, modulusLen + 1, pN, NULL)))
        goto exit;

    if (OK > (status = VLONG_byteStringFromVlong(RSA_E(pRSAKey), pE + 1, &expLen)))
        goto exit;

    /* add the exponent */
    if (OK > ( status = DER_AddInteger( pTemp, expLen + 1, pE, NULL)))
        goto exit;

exit:

    if ( expModulusBuffer)
    {
        FREE( expModulusBuffer);
    }

    return status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
extern MSTATUS
ASN1CERT_storeECCPublicKeyInfo( const ECCKey *pECCKey, DER_ITEMPTR pCertificate)
{
    MSTATUS         status;
    DER_ITEMPTR     pTemp;
    DER_ITEMPTR     pAlgoID;
    const ubyte*    curveOID;
    sbyte4          keyLen;
    ubyte*          keyBuffer = 0;

    if ( (0 == pECCKey) || (0 == pCertificate) )
    {
        return ERR_NULL_POINTER;
    }

    if (OK > ( status = CRYPTO_getECCurveOID(pECCKey, &curveOID)))
        goto exit;

    /* subject public key */
    if (OK > ( status = DER_AddSequence( pCertificate, &pTemp)))
        goto exit;

    /* add the algorithm identifier sequence */
    if ( OK > ( status = DER_AddSequence( pTemp, &pAlgoID)))
        return status;

    if ( OK > ( status = DER_AddOID( pAlgoID, ecPublicKey_OID, NULL)))
        return status;

    if  (OK > ( status = DER_AddOID( pAlgoID, curveOID, NULL)))
        return status;

    /* allocate a buffer for the key parameter */
    if (OK > (status = EC_getPointByteStringLen( pECCKey->pCurve, &keyLen)))
        goto exit;

    if (0 == keyLen)
    {
        status = ERR_BAD_KEY;
        goto exit;
    }

    /* add an extra byte = 0 (unused bits) */
    keyBuffer = MALLOC( keyLen+1);
    if (!keyBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    keyBuffer[0] = 0; /* unused bits */

    if (OK > ( status = EC_writePointToBuffer( pECCKey->pCurve, pECCKey->Qx,
                                                pECCKey->Qy, keyBuffer+1, keyLen)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddItemOwnData( pTemp, BITSTRING, keyLen+1, &keyBuffer, NULL)))
        goto exit;

exit:

    if ( keyBuffer)
    {
        FREE( keyBuffer);
    }

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_ECC__)) */


/*------------------------------------------------------------------*/

MSTATUS
ASN1CERT_storePublicKeyInfo( const AsymmetricKey* pPublicKey, DER_ITEMPTR pCertificate)
{
    switch (pPublicKey->type)
    {
        case akt_rsa:
        {
            return ASN1CERT_storeRSAPublicKeyInfo(pPublicKey->key.pRSA, pCertificate);
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            return ASN1CERT_storeECCPublicKeyInfo(pPublicKey->key.pECC, pCertificate);
        }
#endif

        default:
        {
            break;
        }
    }

    return ERR_BAD_KEY_TYPE;
}


/*------------------------------------------------------------------*/

extern MSTATUS
ASN1CERT_generateLeafCertificate(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) AsymmetricKey *pCertKey,
                                    const certDistinguishedName *pSubjectInfo,
                                    AsymmetricKey *pSignKey, const ASN1_ITEM *pIssuerInfo,
                                    CStream cs, ubyte signAlgo, const certExtensions* pExtensions,
                                    RNGFun rngFun, void* rngFunArg,
                                    ubyte **ppRetCertificate, ubyte4 *pRetCertLength)
{
    DER_ITEMPTR     pCertificate = 0;
    DER_ITEMPTR     pSignedCertificate = 0;
    DER_ITEMPTR     pTemp = 0;
    ubyte           copyData[MAX_DER_STORAGE];
    ubyte           serialNumber[SHA1_RESULT_SIZE];
    ubyte           signAlgoOID[2+MAX_SIG_OID_LEN];
#if (defined(__ENABLE_MOCANA_ECC__))
    ubyte*          ptBuffer = 0;
    sbyte4          ptBufferLen;
#endif
    const ubyte*    issuerMemAccessBuffer = 0;
    MSTATUS         status;

    if ( (0 == pCertKey) || (0 == pSubjectInfo) ||
            (0 == pSignKey) || (0 == pIssuerInfo) ||
            (0 == ppRetCertificate) || ( 0 == pRetCertLength))
    {
        return ERR_NULL_POINTER;
    }

    switch ( pSignKey->type)
    {
        case akt_rsa:
        {
            if (OK > ( status = CRYPTO_getRSAHashAlgoOID( signAlgo, signAlgoOID)))
                goto exit;
            break;
        }

#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            if (OK > ( status = CRYPTO_getECDSAHashAlgoOID( signAlgo, signAlgoOID)))
                goto exit;
            break;
        }
#endif

        default:
        {
            status = ERR_BAD_KEY_TYPE;
            goto exit;
        }
    }

    /* build certificate */
    if ( OK > (status = DER_AddSequence( NULL, &pSignedCertificate)))
        goto exit;

    if ( OK > (status = DER_AddSequence( pSignedCertificate, &pCertificate)))
        goto exit;

    /* version  tag [0] + integer (2)*/
    if ( OK > (status = DER_AddTag( pCertificate, 0,  &pTemp)))
        goto exit;

    copyData[0] = 2;
    if ( OK > ( status = DER_AddItemCopyData( pTemp, INTEGER, 1, copyData, NULL)))
        goto exit;

    switch ( pCertKey->type)
    {
        case akt_rsa:
        {
            /* serial number -> generated by SHA-1 hash of the RSA key modulus */
            if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx)
                                    (ubyte *)RSA_N(pCertKey->key.pRSA)->pUnits,
                                    sizeof(vlong_unit) * RSA_N(pCertKey->key.pRSA)->numUnitsUsed, serialNumber)))
            {
                goto exit;
            }
            break;
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            /* serial number -> generated by SHA-1 hash of the point */
            if ( OK > ( status = EC_pointToByteString( pCertKey->key.pECC->pCurve,
                                                    pCertKey->key.pECC->Qx,
                                                    pCertKey->key.pECC->Qy,
                                                    &ptBuffer,
                                                    &ptBufferLen)))
            {
                goto exit;
            }

            if ( OK > ( status = SHA1_completeDigest( MOC_HASH(hwAccelCtx)
                                    ptBuffer, ptBufferLen, serialNumber)))
            {
                goto exit;
            }
            break;
        }
#endif

        default:
        {
            status = ERR_BAD_KEY_TYPE;
            goto exit;
            break;
        }
    }

    /* serial number must be 20 bytes long at most and must not be negative: clear the sign bit */
    serialNumber[0] &= 0x7F;
    if ( OK > (status = DER_AddItem( pCertificate, INTEGER, SHA1_RESULT_SIZE, serialNumber, NULL)))
        goto exit;

    /* signature */
    if ( OK > ( status = DER_StoreAlgoOID( pCertificate, signAlgoOID, (akt_rsa == pCertKey->type))))
        goto exit;

    /* issuer */
    issuerMemAccessBuffer = CS_memaccess( cs, pIssuerInfo->dataOffset, pIssuerInfo->length);
    if ( !issuerMemAccessBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    if ( OK > ( status = DER_AddItem( pCertificate, (CONSTRUCTED|SEQUENCE), pIssuerInfo->length, issuerMemAccessBuffer, NULL)))
        goto exit;

    /* validity */
    if ( OK > ( status = DER_AddSequence( pCertificate, &pTemp)))
        goto exit;

    if ( OK > ( status = DER_AddItem( pTemp, UTCTIME, MOC_STRLEN((sbyte *)pSubjectInfo->pStartDate), (ubyte *)pSubjectInfo->pStartDate, NULL)))
        goto exit;

    if ( OK > ( status = DER_AddItem( pTemp, UTCTIME, MOC_STRLEN((sbyte *)pSubjectInfo->pEndDate), (ubyte *)pSubjectInfo->pEndDate, NULL)))
        goto exit;

    /* subject */
    if ( OK > ( status = ASN1CERT_StoreDistinguishedName( pCertificate, pSubjectInfo)))
        goto exit;

    /* subject public key info */
    if ( OK > ( status = ASN1CERT_storePublicKeyInfo( pCertKey, pCertificate)))
        goto exit;

    if ( pExtensions)
    {
        if ( OK > ( status = ASN1CERT_AddExtensionsToTBSCertificate( pCertificate,  pExtensions)))
            goto exit;
    }

    /* now sign */
    if  (OK > ( status = ASN1CERT_sign( MOC_RSA_HASH(hwAccelCtx) pSignedCertificate, pSignKey, signAlgoOID,
                                        rngFun, rngFunArg, ppRetCertificate, pRetCertLength)))
    {
        goto exit;
    }

exit:
#if (defined(__ENABLE_MOCANA_ECC__))
    if (ptBuffer)
    {
        FREE(ptBuffer);
    }
#endif

    if (issuerMemAccessBuffer)
    {
        CS_stopaccess( cs, issuerMemAccessBuffer);
    }

    if ( pSignedCertificate)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSignedCertificate);
    }

    return status;

} /* ASN1CERT_generateLeafCertificate */


/*------------------------------------------------------------------*/

extern MSTATUS
ASN1CERT_generateCertificate(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) AsymmetricKey *pCertKey,
                             const certDistinguishedName *pSubjectInfo,
                             AsymmetricKey *pSignKey,
                             const certDistinguishedName *pIssuerInfo,
                             ubyte signAlgo, const certExtensions* pExtensions,
                             RNGFun rngFun, void* rngFunArg,
                             ubyte **ppRetCertificate, ubyte4 *pRetCertLength)
{
    DER_ITEMPTR pCertificate = 0;
    DER_ITEMPTR pSignedCertificate = 0;
    DER_ITEMPTR pTemp = 0;
#if (defined(__ENABLE_MOCANA_ECC__))
    ubyte*      ptBuffer = 0;
    sbyte4      ptBufferLen;
#endif
    ubyte       copyData[MAX_DER_STORAGE];
    ubyte*      pSerialNumber = NULL;
    ubyte       signAlgoOID[2 + MAX_SIG_OID_LEN];
    ubyte*      pTempBuf = NULL;
    MSTATUS     status;

    if ( (0 == pCertKey) || (0 == pSubjectInfo) ||
            (0 == pSignKey) || (0 == pIssuerInfo) ||
            (0 == ppRetCertificate) || ( 0 == pRetCertLength))
    {
        return ERR_NULL_POINTER;
    }

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, SHA_HASH_RESULT_SIZE, TRUE, &pSerialNumber)))
        goto exit;

    switch ( pSignKey->type)
    {
        case akt_rsa:
        {
            if (OK > ( status = CRYPTO_getRSAHashAlgoOID( signAlgo, signAlgoOID)))
                goto exit;
            break;
        }

#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            if (OK > ( status = CRYPTO_getECDSAHashAlgoOID( signAlgo, signAlgoOID)))
                goto exit;
            break;
        }
#endif

        default:
        {
            status = ERR_BAD_KEY_TYPE;
            goto exit;
        }
    }

    /* build certificate */
    if ( OK > (status = DER_AddSequence( NULL, &pSignedCertificate)))
        goto exit;

    if ( OK > (status = DER_AddSequence( pSignedCertificate, &pCertificate)))
        goto exit;

    /* version  tag [0] + integer (2)*/
    if ( OK > (status = DER_AddTag( pCertificate, 0,  &pTemp)))
        goto exit;

    copyData[0] = 2;
    if ( OK > ( status = DER_AddItemCopyData( pTemp, INTEGER, 1, copyData, NULL)))
        goto exit;

    switch ( pCertKey->type)
    {
        case akt_rsa:
        {
            if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, (sizeof(vlong_unit) * RSA_N(pCertKey->key.pRSA)->numUnitsUsed), TRUE, &pTempBuf)))
                goto exit;

            MOC_MEMCPY(pTempBuf, (ubyte *)(RSA_N(pCertKey->key.pRSA)->pUnits),
                       (sizeof(vlong_unit) * RSA_N(pCertKey->key.pRSA)->numUnitsUsed));

            /* serial number -> generated by SHA-1 hash of the RSA key modulus */
            if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx)
                                                   pTempBuf,
                                                   sizeof(vlong_unit) * RSA_N(pCertKey->key.pRSA)->numUnitsUsed,
                                                   pSerialNumber)))
            {
                goto exit;
            }
            break;
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            /* serial number -> generated by SHA-1 hash of the point */
            if ( OK > ( status = EC_pointToByteString( pCertKey->key.pECC->pCurve,
                                                    pCertKey->key.pECC->Qx,
                                                    pCertKey->key.pECC->Qy,
                                                    &ptBuffer,
                                                    &ptBufferLen)))
            {
                goto exit;
            }

            if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, ptBufferLen, TRUE, &pTempBuf)))
                goto exit;

            MOC_MEMCPY(pTempBuf, ptBuffer, ptBufferLen);

            if ( OK > ( status = SHA1_completeDigest( MOC_HASH(hwAccelCtx)
                                    pTempBuf, ptBufferLen, pSerialNumber)))
            {
                goto exit;
            }
            break;
        }
#endif
        default:
        {
            status = ERR_BAD_KEY_TYPE;
            goto exit;
            break;
        }
    }

    /* serial number must be 20 bytes long at most and must not be negative: clear the sign bit */
    pSerialNumber[0] &= 0x7F;
    if ( OK > (status = DER_AddItem( pCertificate, INTEGER, SHA1_RESULT_SIZE, pSerialNumber, NULL)))
        goto exit;

    /* signature */
    if ( OK > ( status = DER_StoreAlgoOID( pCertificate, signAlgoOID, (akt_rsa == pCertKey->type))))
        goto exit;

    /* issuer */
    if ( OK > ( status = ASN1CERT_StoreDistinguishedName( pCertificate, pIssuerInfo)))
        goto exit;

    /* validity */
    if ( OK > ( status = DER_AddSequence( pCertificate, &pTemp)))
        goto exit;

    if ( OK > ( status = DER_AddItem( pTemp, UTCTIME, MOC_STRLEN((sbyte *)pSubjectInfo->pStartDate), (ubyte *)pSubjectInfo->pStartDate, NULL)))
        goto exit;

    if ( OK > ( status = DER_AddItem( pTemp, UTCTIME, MOC_STRLEN((sbyte *)pSubjectInfo->pEndDate), (ubyte *)pSubjectInfo->pEndDate, NULL)))
        goto exit;

    /* subject */
    if ( OK > ( status = ASN1CERT_StoreDistinguishedName( pCertificate, pSubjectInfo)))
        goto exit;

    /* subject public key info */
    if ( OK > ( status = ASN1CERT_storePublicKeyInfo( pCertKey, pCertificate)))
        goto exit;

    if ( pExtensions)
    {
        if ( OK > ( status = ASN1CERT_AddExtensionsToTBSCertificate( pCertificate,  pExtensions)))
            goto exit;
    }

    /* now sign */
    if  (OK > ( status = ASN1CERT_sign( MOC_RSA_HASH(hwAccelCtx) pSignedCertificate, pSignKey, signAlgoOID,
                                            rngFun, rngFunArg, ppRetCertificate, pRetCertLength)))
    {
        goto exit;
    }

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &pSerialNumber);
    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempBuf);

#if (defined(__ENABLE_MOCANA_ECC__))
    if (ptBuffer)
    {
        FREE(ptBuffer);
    }
#endif

    if ( pSignedCertificate)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSignedCertificate);
    }

    return status;

} /* ASN1CERT_generateCertificate */

#endif /* __DISABLE_MOCANA_CERTIFICATE_GENERATION__ */
