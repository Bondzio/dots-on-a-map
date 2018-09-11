/* Version: mss_v6_3 */
/*
 * ca_mgmt.c
 *
 * Certificate Authority Management Factory
 *
 * Copyright Mocana Corp 2004-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file ca_mgmt.c Mocana certificate authority management factory.
This file contains certificate management functions.

\since 1.41
\version 5.3 and later

! Flags
Whether the following flags are defined determines which functions are enabled:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$
- $__DISABLE_MOCANA_CERTIFICATE_PARSING__$
- $__DISABLE_MOCANA_KEY_GENERATION__$
- $__ENABLE_MOCANA_DSA__$
- $__ENABLE_MOCANA_ECC__$
- $__ENABLE_MOCANA_EXTRACT_CERT_BLOB__$
- $__ENABLE_MOCANA_PEM_CONVERSION__$
- $__ENABLE_MOCANA_PKCS10__$

! External Functions
This file contains the following public ($extern$) functions:
- CA_MGMT_allocCertDistinguishedName
- CA_MGMT_convertKeyDER
- CA_MGMT_convertKeyPEM
- CA_MGMT_convertPKCS8KeyToKeyBlob
- CA_MGMT_convertProtectedPKCS8KeyToKeyBlob
- CA_MGMT_decodeCertificate
- CA_MGMT_enumAltName
- CA_MGMT_enumCrl
- CA_MGMT_extractCertASN1Name
- CA_MGMT_extractCertDistinguishedName
- CA_MGMT_extractCertTimes
- CA_MGMT_freeCertDistinguishedName
- CA_MGMT_freeCertificate
- CA_MGMT_freeKeyBlob
- CA_MGMT_freeNakedKey
- CA_MGMT_freePublicKey
- CA_MGMT_generateCertificateEx
- CA_MGMT_generateNakedKey
- CA_MGMT_keyBlobToDER
- CA_MGMT_keyBlobToPEM
- CA_MGMT_convertKeyBlobToPKCS8Key
- CA_MGMT_returnCertificatePrints
- CA_MGMT_returnPublicKey
- CA_MGMT_returnPublicKeyBitLength
- CA_MGMT_reorderChain
- CA_MGMT_verifyCertWithKeyBlob

*/

#include "../common/moptions.h"

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mrtos.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/mstdlib.h"
#include "../common/random.h"
#include "../common/vlong.h"
#include "../common/memory_debug.h"
#include "../crypto/base64.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/crypto.h"
#ifdef __ENABLE_MOCANA_DSA__
#include "../crypto/dsa.h"
#endif
#include "../crypto/rsa.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../crypto/pkcs_key.h"
#include "../crypto/ca_mgmt.h"
#include "../harness/harness.h"
#include "../asn1/parseasn1.h"
#include "../asn1/parsecert.h"
#include "../asn1/derencoder.h"
#include "../asn1/oiddefs.h"
#include "../crypto/asn1cert.h"
#include "../crypto/sec_key.h"
#include "../common/utils.h"


/*------------------------------------------------------------------*/
#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
typedef struct __certBasket
{
    ASN1_ITEM           *pCert;
    ubyte               *pCertificate;
    sbyte4              certLength;
    struct __certBasket *pPrev;
    struct __certBasket *pNext;
}certBasket;
#endif
#if (defined(__ENABLE_MOCANA_PKCS10__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__))

static void
findNextLine(ubyte *pSrc, ubyte4 *pSrcIndex, ubyte4 srcLength)
{
    pSrc += (*pSrcIndex);

    /* seek CR or LF */
    while ((*pSrcIndex < srcLength) && ((0x0d != *pSrc) && (0x0a != *pSrc)))
    {
        (*pSrcIndex)++;
        pSrc++;
    }

    /* skip CR and LF */
    while ((*pSrcIndex < srcLength) && ((0x0d == *pSrc) || (0x0a == *pSrc)))
    {
        (*pSrcIndex)++;
        pSrc++;
    }
} /* findNextLine */

/*------------------------------------------------------------------*/

static void
fetchLine(ubyte *pSrc,  ubyte4 *pSrcIndex, ubyte4 srcLength,
          ubyte *pDest, ubyte4 *pDestIndex)
{
    pSrc += (*pSrcIndex);

    if ('-' == *pSrc)
    {
        /* handle '---- XXX ----' lines */
        /* seek CR or LF */
        while ((*pSrcIndex < srcLength) && ((0x0d != *pSrc) && (0x0a != *pSrc)))
        {
            (*pSrcIndex)++;
            pSrc++;
        }

        /* skip CR and LF */
        while ((*pSrcIndex < srcLength) && ((0x0d == *pSrc) || (0x0a == *pSrc)))
        {
            (*pSrcIndex)++;
            pSrc++;
        }
    }
    else
    {
        pDest += (*pDestIndex);

        /* handle base64 encoded data line */
        while ((*pSrcIndex < srcLength) &&
               ((0x20 != *pSrc) && (0x0d != *pSrc) && (0x0a != *pSrc)))
        {
            *pDest = *pSrc;

            (*pSrcIndex)++;
            (*pDestIndex)++;
            pSrc++;
            pDest++;
        }

        /* skip to next line */
        while ((*pSrcIndex < srcLength) &&
               ((0x20 == *pSrc) || (0x0d == *pSrc) || (0x0a == *pSrc) || (0x09 == *pSrc)))
        {
            (*pSrcIndex)++;
            pSrc++;
        }
    }
} /* fetchLine */

#endif /* (defined(__ENABLE_MOCANA_PKCS10__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_PKCS10__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__))

/* Convert PEM-encoded certificate to DER-encoded certificate.
This function converts a PEM-encoded certificate to a DER-encoded certificate.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_PKCS10__$
- $__ENABLE_MOCANA_PEM_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pKeyFile Pointer to certificate file to decode.
\param fileSize Number of bytes in certificate file.
\param ppDecodeFile On return, pointer to address of buffer containing DER-decoded certificate.
\param pDecodedLength On return, pointer to number of bytes in $ppDecodeFile$.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_decodeCertificate(ubyte*  pKeyFile, ubyte4 fileSize,
                          ubyte** ppDecodeFile, ubyte4 *pDecodedLength)
{
    /* misleading name: decode any PEM message */
    ubyte*  pBase64Mesg = NULL;
    ubyte4  srcIndex    = 0;
    ubyte4  destIndex   = 0;
    sbyte4  numDelimiters = 0;
    MSTATUS status;

    if ((!pKeyFile) || (!ppDecodeFile) || (!pDecodedLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == fileSize)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    if (NULL == (pBase64Mesg = MALLOC( fileSize)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    while (fileSize > srcIndex && numDelimiters < 2)
    {
        if ('-' == pKeyFile[srcIndex])
        {
            ++numDelimiters;
        }
        fetchLine(pKeyFile, &srcIndex, fileSize, pBase64Mesg, &destIndex);
    }

    status = BASE64_decodeMessage((ubyte *)pBase64Mesg, destIndex, ppDecodeFile, pDecodedLength);

exit:
    if (NULL != pBase64Mesg)
        FREE(pBase64Mesg);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/
static MSTATUS
findNextDelimiter(ubyte* pFileData, ubyte4 *index, ubyte4 dataSize)
{
    MSTATUS status = OK;

    while (dataSize > *index)
    {
        findNextLine(pFileData, index, dataSize);

        if ('-' == pFileData[*index])
            break;
    }

    return status;
}

static MSTATUS
findFirstDelimiter(ubyte* pFileData, ubyte4 *index, ubyte4 dataSize)
{
    if ('-' == pFileData[*index])
        return OK;

    return findNextDelimiter(pFileData, index, dataSize);
}

/*------------------------------------------------------------------*/

extern sbyte4
CA_MGMT_decodeCertificateBundle(ubyte* pBundleFile, ubyte4 fileSize, certDescriptor **pCertificates, ubyte4 *pNumCerts)
{
    MSTATUS status       = OK;
    ubyte4 numDelimiters = 0;
    ubyte4 srcIndex      = 0;
    ubyte4 endIndex;
    ubyte4 numCerts;
    certDescriptor *certs = NULL;

    if ((NULL == pBundleFile) || (NULL == pCertificates) || (NULL == pNumCerts))
        return ERR_NULL_POINTER;

    /* Compute the number of delimiters, which should be numCerts * 2 */
    while (fileSize > srcIndex)
    {
        if ('-' == pBundleFile[srcIndex])
        {
            ++numDelimiters;
        }
        findNextLine(pBundleFile, &srcIndex, fileSize);
    }

    /* Make sure numDelimeters is even */
    if (1 == numDelimiters % 2)
        return ERR_INVALID_ARG;

    /* Compute numCerts */
    numCerts = numDelimiters / 2;

    /* Create the array */
    if (NULL == (certs = MALLOC(numCerts * sizeof(*certs))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    MOC_MEMSET((ubyte*)certs, 0, numCerts * sizeof(*certs));

    /* Iterate over each certificate */
    numCerts = 0;
    srcIndex = 0;
    findFirstDelimiter(pBundleFile, &srcIndex, fileSize);

    while (srcIndex < fileSize)
    {
        endIndex = srcIndex;
        findNextDelimiter(pBundleFile, &endIndex, fileSize);

        if (OK > (status = CA_MGMT_decodeCertificate(pBundleFile + srcIndex, endIndex - srcIndex, &(certs[numCerts].pCertificate), &(certs[numCerts].certLength))))
            goto exit;

        numCerts++;
        srcIndex = endIndex;
        findNextDelimiter(pBundleFile, &srcIndex, fileSize);
    }

    /* Return the results */
    *pNumCerts = numCerts;
    *pCertificates = certs; certs = NULL;

 exit:
    if (certs)
        CA_MGMT_freeCertificateBundle(&certs, numCerts);

    return status;
}

/*------------------------------------------------------------------*/

extern sbyte4
CA_MGMT_freeCertificateBundle(certDescriptor **pCertificates, ubyte4 numCerts)
{
    ubyte4 i;

    if ((NULL == pCertificates) || (NULL == *pCertificates))
        return ERR_NULL_POINTER;

    for (i = 0; i < numCerts; i++)
    {
        CA_MGMT_freeCertificate(&((*pCertificates)[i]));
    }

    FREE(*pCertificates);
    *pCertificates = NULL;
    return OK;
}


#endif /* (defined(__ENABLE_MOCANA_PKCS10__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__)) */


#if !(defined(__DISABLE_MOCANA_KEY_GENERATION__)) || defined(__ENABLE_MOCANA_PEM_CONVERSION__) || defined(__ENABLE_MOCANA_DER_CONVERSION__)
/* Key Blob format
Version 0: supports only RSA keys
            p,q,n,e stored with 4-byte length prefix
Version 0: supports only DSA keys
            two files: 1) p,q,g,y stored with 4-byte length prefix and 2) x (private key) with 4-byte length prefix
Version 1:

4 bytes: all zero
4 bytes: version number 0x00000001
4 bytes: key type       0x00000001 : RSA
                        0x00000002 : ECC
                        0x00000003 : DSA

switch (keytype)
    case RSA:
        4 bytes length of e string
        n bytes length of e byte string
        4 bytes length of n string
        n bytes length of n byte string
        4 bytes length of p string
        n bytes length of p byte string
        4 bytes length of q string
        n bytes length of q byte string
        4 bytes length of private string #1
        n bytes length of private byte string #1
        4 bytes length of private string #2
        n bytes length of private byte string #2
        4 bytes length of private string #3
        n bytes length of private byte string #3
        4 bytes length of private string #4
        n bytes length of private byte string #4
        4 bytes length of private string #5
        n bytes length of private byte string #5

    case ECC:
        1 byte OID suffix identifying the curve
        4 bytes length of Point string
        n bytes length of Point byte string (uncompressed X9-62 format)
        4 bytes length of Scalar string
        n bytes length of Scalar byte string

    case DSA:
        4 bytes length of p string
        n bytes length of p byte string
        4 bytes length of q string
        n bytes length of q byte string
        4 bytes length of g string
        n bytes length of g byte string
        4 bytes length of y string
        n bytes length of y byte string
        4 bytes length of x string
        n bytes length of x byte string

*/


/*------------------------------------------------------------------*/

static MSTATUS
CA_MGMT_makeRSAKeyBlob(MOC_RSA(hwAccelDescr hwAccelCtx)
                       RSAKey *pRSAContext, ubyte **ppRetKeyBlob,
                       ubyte4 *pRetKeyLength)
{
    MSTATUS status;
    ubyte4 bufferLength;
    ubyte* buffer = NULL;

    if (!pRSAContext || !ppRetKeyBlob || !pRetKeyLength)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > ( status = RSA_byteStringFromKey(MOC_RSA(hwAccelCtx)
                                pRSAContext, 0, &bufferLength)))
        goto exit;

    buffer = MALLOC(bufferLength+12);
    if (!buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    /* headers */
    MOC_MEMSET( buffer, 0x00, 12);
    buffer[7] = 1; /* version */
    buffer[11] = akt_rsa;  /* key type */

    if (OK > (status = RSA_byteStringFromKey(MOC_RSA(hwAccelCtx)
                                pRSAContext, buffer+12, &bufferLength)))
        goto exit;

    *ppRetKeyBlob = buffer;
    buffer = NULL;
    *pRetKeyLength = bufferLength + 12;

exit:

    if (NULL != buffer)
        FREE(buffer);

    return status;

} /* CA_MGMT_makeRSAKeyBlob */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
static MSTATUS
CA_MGMT_makeDSAKeyBlob(MOC_RSA(hwAccelDescr hwAccelCtx)
                       DSAKey *pDSAContext, ubyte **ppRetKeyBlob,
                       ubyte4 *pRetKeyLength)
{
    ubyte4  bufferLength;
    ubyte*  buffer = NULL;
    MSTATUS status;

    if (!pDSAContext || !ppRetKeyBlob || !pRetKeyLength)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = DSA_makeKeyBlob(pDSAContext, NULL, &bufferLength)))
        goto exit;

    buffer = MALLOC(bufferLength+12);
    if (!buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* headers */
    MOC_MEMSET( buffer, 0x00, 12);
    buffer[7] = 1; /* version */
    buffer[11] = akt_dsa;  /* key type */

    if (OK > (status = DSA_makeKeyBlob(pDSAContext, buffer+12, &bufferLength)))
        goto exit;

    *ppRetKeyBlob = buffer;
    buffer = NULL;
    *pRetKeyLength = bufferLength + 12;

exit:
    if (NULL != buffer)
        FREE(buffer);

    return status;

} /* CA_MGMT_makeDSAKeyBlob */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static MSTATUS
CA_MGMT_makeECCKeyBlob(ECCKey *pECCKey, ubyte4 curveId,
                       ubyte **ppRetKeyBlob, ubyte4 *pRetKeyLength)
{
    ubyte*              pKeyBlob        = NULL;
    PrimeFieldPtr       pPF;
    MSTATUS             status;
    sbyte4              scalarLen;
    sbyte4              pointLen;
    sbyte4              blobSize;
    sbyte4              index;

    if (!pECCKey || !ppRetKeyBlob || !pRetKeyLength)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* get total length */
    pPF = EC_getUnderlyingField( pECCKey->pCurve);

    if ( OK > (status = EC_getPointByteStringLen( pECCKey->pCurve, &pointLen)))
        goto exit;
    if (pECCKey->privateKey)
    {
        if ( OK > (status = PRIMEFIELD_getElementByteStringLen(pPF, &scalarLen)))
            goto exit;
    }
    else
    {
        scalarLen = 0;
    }

    blobSize = 12 + 4 + 4 + pointLen + 4 + scalarLen;

    if (NULL == (pKeyBlob = MALLOC(blobSize)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* headers */
    MOC_MEMSET( pKeyBlob, 0, 12);
    pKeyBlob[7] = 1; /* version */
    pKeyBlob[11] = akt_ecc; /* key type */

    index = 12;
    BIGEND32( pKeyBlob + index, curveId);
    index += 4;

    BIGEND32( pKeyBlob + index, pointLen);
    index += 4;
    if (OK > (status = EC_writePointToBuffer( pECCKey->pCurve, pECCKey->Qx, pECCKey->Qy,
                                                pKeyBlob + index, pointLen)))
    {
        goto exit;
    }
    index += pointLen;

    if (pECCKey->privateKey)
    {
        BIGEND32( pKeyBlob + index, scalarLen);
        index += 4;

        if (OK > ( status = PRIMEFIELD_writeByteString( pPF, pECCKey->k,
                                                        pKeyBlob + index, scalarLen)))
        {
            goto exit;
        }
    }

    *ppRetKeyBlob  = pKeyBlob;  pKeyBlob = NULL;
    *pRetKeyLength = blobSize;

exit:
    if (NULL != pKeyBlob)
        FREE(pKeyBlob);

    return status;

} /* CA_MGMT_makeECCKeyBlob */
#endif


/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern MSTATUS
CA_MGMT_makeKeyBlobEx(AsymmetricKey *pKey,
                      ubyte **ppRetKeyBlob, ubyte4 *pRetKeyLength)
{
    hwAccelDescr    hwAccelCtx;
    MSTATUS         status;

    if (!pKey || !ppRetKeyBlob || !pRetKeyLength)
        return ERR_NULL_POINTER;

    if (OK > (status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    switch (pKey->type)
    {
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            status = CA_MGMT_makeECCKeyBlob(pKey->key.pECC, CRYPTO_getECCurveId(pKey), ppRetKeyBlob, pRetKeyLength);
            break;
        }
#endif
#if (defined(__ENABLE_MOCANA_DSA__))
        case akt_dsa:
        {
            status = CA_MGMT_makeDSAKeyBlob(MOC_RSA(hwAccelCtx) pKey->key.pDSA, ppRetKeyBlob, pRetKeyLength);
            break;
        }
#endif
        case akt_rsa:
        {
            status = CA_MGMT_makeRSAKeyBlob(MOC_RSA(hwAccelCtx) pKey->key.pRSA, ppRetKeyBlob, pRetKeyLength);
            break;
        }
        default:
        {
            status = ERR_BAD_KEY_TYPE;
            break;
        }
    }

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;
}

#endif


/*------------------------------------------------------------------*/

static MSTATUS
CA_MGMT_extractOldRSAKeyBlob(MOC_RSA(hwAccelDescr hwAccelCtx)
                             const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                             RSAKey *p_rsaContext)
{
    ubyte4  index;
    MSTATUS status;

    if (0 == keyBlobLength)
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* p */
    index = 0;
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &RSA_P(p_rsaContext), &index)))
        goto exit;

    pKeyBlob += index;
    if (0 >= (sbyte4)(keyBlobLength = (keyBlobLength - index)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* q */
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &RSA_Q(p_rsaContext), &index)))
        goto exit;

    pKeyBlob += index;
    if (0 >= (sbyte4)(keyBlobLength = (keyBlobLength - index)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* n */
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &RSA_N(p_rsaContext), &index)))
        goto exit;

    pKeyBlob += index;
    if (0 >= (sbyte4)(keyBlobLength = (keyBlobLength - index)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* e */
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &RSA_E(p_rsaContext), &index)))
        goto exit;

    if (0 != keyBlobLength - index)
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    p_rsaContext->keyType = RsaPrivateKey;

    status = RSA_prepareKey(MOC_RSA(hwAccelCtx) p_rsaContext, 0);


exit:
    return status;

} /* CA_MGMT_extractOldRSAKeyBlob */


/*------------------------------------------------------------------*/

static MSTATUS
CA_MGMT_readOldRSAKeyBlob(MOC_RSA(hwAccelDescr hwAccelCtx)
                         const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                         AsymmetricKey* pKey)
{
    MSTATUS status;

    if ( OK > ( status = RSA_createKey( &pKey->key.pRSA)))
        return status;
    pKey->type = akt_rsa;

    return CA_MGMT_extractOldRSAKeyBlob(MOC_RSA(hwAccelCtx) pKeyBlob, keyBlobLength, pKey->key.pRSA);
}


/*------------------------------------------------------------------*/

static MSTATUS
CA_MGMT_readRSAKeyPart(MOC_RSA(hwAccelDescr hwAccelCtx)
                       const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                       AsymmetricKey* pKey)
{
    MSTATUS status;

    status = RSA_keyFromByteString(MOC_RSA(hwAccelCtx) &pKey->key.pRSA, pKeyBlob, keyBlobLength, 0);
    if (OK <= status)
    {
        pKey->type = akt_rsa;
    }
    return status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
static MSTATUS
CA_MGMT_readDSAKeyPart(const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                       AsymmetricKey* pKey)
{
    MSTATUS status;

    status = DSA_extractKeyBlob(&pKey->key.pDSA, pKeyBlob, keyBlobLength);

    if (OK <= status)
        pKey->type = akt_dsa;

    return status;
}
#endif


/*------------------------------------------------------------------*/
#if (defined(__ENABLE_MOCANA_ECC__))
static MSTATUS
CA_MGMT_readECCKeyPart(const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                         AsymmetricKey* pKey)
{
    ubyte4 curveId;
    ubyte4 pointLen, scalarLen;
    const ubyte* pPoint;
    const ubyte* pScalar;

    if ( keyBlobLength < 8)
    {
        return ERR_BAD_KEY_BLOB;
    }

    /* read curve id */
    curveId =  ((ubyte4)(*pKeyBlob++) << 24);
    curveId |= ((ubyte4)(*pKeyBlob++) << 16);
    curveId |= ((ubyte4)(*pKeyBlob++) << 8);
    curveId |= ((ubyte4)(*pKeyBlob++) );

    /* read point len */
    pointLen =  ((ubyte4)(*pKeyBlob++) << 24);
    pointLen |= ((ubyte4)(*pKeyBlob++) << 16);
    pointLen |= ((ubyte4)(*pKeyBlob++) << 8);
    pointLen |= ((ubyte4)(*pKeyBlob++) );
    keyBlobLength-=8;

    if ( keyBlobLength < pointLen)
    {
        return ERR_BAD_KEY_BLOB;
    }
    pPoint = pKeyBlob;
    keyBlobLength -= pointLen;
    pKeyBlob += pointLen;

    if ( keyBlobLength > 4)
    {
        scalarLen =  ((ubyte4)(*pKeyBlob++) << 24);
        scalarLen |= ((ubyte4)(*pKeyBlob++) << 16);
        scalarLen |= ((ubyte4)(*pKeyBlob++) << 8);
        scalarLen |= ((ubyte4)(*pKeyBlob++) );
        keyBlobLength -= 4;

        if ( keyBlobLength < scalarLen)
        {
            return ERR_BAD_KEY_BLOB;
        }

        pScalar = pKeyBlob;
    }
    else
    {
        pScalar = 0;
        scalarLen = 0;
    }

    return CRYPTO_setECCParameters( pKey, curveId, pPoint, pointLen,
                                    pScalar, scalarLen, NULL);
}
#endif

/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern MSTATUS
CA_MGMT_extractKeyBlobTypeEx(const ubyte *pKeyBlob, ubyte4 keyBlobLength, ubyte4 *pRetKeyType)
{
    MSTATUS status = OK;
    sbyte4  i;

    if ((NULL == pKeyBlob) || (NULL == pRetKeyType))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* look at the first 4 bytes */
    for (i = 0; i < 4; ++i)
    {
        if (pKeyBlob[i])
        {
            /* if not all 0 -> old format */
            *pRetKeyType = akt_rsa;
            goto exit;
        }
    }

    /* jump over the first 4 bytes */
    pKeyBlob += 4;
    if (4 > (sbyte4)(keyBlobLength = (keyBlobLength - 4)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* version */
    for (i = 0; i < 3; ++i)
    {
        if (*pKeyBlob++)
        {
            status = ERR_BAD_KEY_BLOB_VERSION;
            goto exit;
        }
    }

    if (*pKeyBlob++ != 1)
    {
        status = ERR_BAD_KEY_BLOB_VERSION;
        goto exit;
    }
    if (4 > (sbyte4)(keyBlobLength - 4))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* key type */
    for (i = 0; i < 3; ++i)
    {
        if (*pKeyBlob++)
        {
            status = ERR_BAD_KEY_BLOB;
            goto exit;
        }
    }

    switch (*pKeyBlob)
    {
        case akt_rsa: /* RSA key */
        case akt_ecc: /* ECC key */
        case akt_dsa: /* DSA key */
        {
            *pRetKeyType = *pKeyBlob;
            break;
        }

        default:
        {
            status = ERR_BAD_KEY_BLOB;
            break;
        }
    }

exit:
    return status;

} /* CA_MGMT_extractKeyBlobTypeEx */


/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern MSTATUS
CA_MGMT_extractKeyBlobEx(const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                         AsymmetricKey* pKey)
{
    hwAccelDescr    hwAccelCtx;
    sbyte4          i;
    MSTATUS         status;

    if ((NULL == pKeyBlob) || (NULL == pKey))
        return ERR_NULL_POINTER;

    if (OK > (status = (MSTATUS) HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    CRYPTO_uninitAsymmetricKey( pKey, NULL);

    /* look at the first 4 bytes */
    for ( i = 0; i < 4; ++i)
    {
        if (pKeyBlob[i])
        {
            /* if not all 0 -> old format */
            status = CA_MGMT_readOldRSAKeyBlob(MOC_RSA(hwAccelCtx) pKeyBlob, keyBlobLength, pKey);
            goto exit;
        }
    }

    /* jump over the first 4 bytes */
    pKeyBlob += 4;
    if (4 > (sbyte4)(keyBlobLength = (keyBlobLength - 4)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }
    /* version */
    for ( i = 0; i < 3; ++i)
    {
        if (*pKeyBlob++)
        {
            status = ERR_BAD_KEY_BLOB_VERSION;
            goto exit;
        }
    }

    if (*pKeyBlob++ != 1)
    {
        status = ERR_BAD_KEY_BLOB_VERSION;
        goto exit;
    }
    if (4 > (sbyte4)(keyBlobLength = (keyBlobLength - 4)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* key type */
    for ( i = 0; i < 3; ++i)
    {
        if (*pKeyBlob++)
        {
            status = ERR_BAD_KEY_BLOB;
            goto exit;
        }
    }

    switch (*pKeyBlob++)
    {
        case akt_rsa: /* a RSA key */
        {
            status = CA_MGMT_readRSAKeyPart(MOC_RSA(hwAccelCtx) pKeyBlob, keyBlobLength - 4, pKey);
            break;
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc: /* an ECC key */
        {
            status = CA_MGMT_readECCKeyPart(pKeyBlob, keyBlobLength - 4, pKey);
            break;
        }
#endif
#if (defined(__ENABLE_MOCANA_DSA__))
        case akt_dsa: /* an DSA key */
        {
            status = CA_MGMT_readDSAKeyPart(pKeyBlob, keyBlobLength - 4, pKey);
            break;
        }
#endif
        default:
        {
            status = ERR_BAD_KEY_BLOB;
            break;
        }
    }

exit:
    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;
}


/*------------------------------------------------------------------*/

#if (!defined(__DISABLE_MOCANA_KEY_GENERATION__))
/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern MSTATUS
CA_MGMT_extractPublicKey(const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                         ubyte **ppRetPublicKeyBlob, ubyte4 *pRetPublicKeyBlobLength,
                         ubyte4 *pRetKeyType)
{
    AsymmetricKey       key;
    MSTATUS             status;

    if ((NULL == ppRetPublicKeyBlob) || (NULL == pRetPublicKeyBlobLength) || (NULL == pRetKeyType))
        return ERR_NULL_POINTER;

    *pRetKeyType = akt_undefined;

    CRYPTO_initAsymmetricKey(&key);

    if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &key)))
        goto exit;

    if (akt_ecc == key.type)
    {
#if (defined(__ENABLE_MOCANA_ECC__))
        key.key.pECC->privateKey = 0;

#else
        status = ERR_CRYPTO_ECC_DISABLED;
        goto exit;
#endif
    }
    else if (akt_rsa == key.type)
    {
        key.key.pRSA->keyType = RsaPublicKey;
    }
    else if (akt_dsa == key.type)
    {
#if (defined(__ENABLE_MOCANA_DSA__))
        VLONG_freeVlong( &DSA_X(key.key.pDSA), NULL); /* clear the X (private value) */
#else
        status = ERR_CRYPTO_DSA_DISABLED;
        goto exit;
#endif
    }
    else
    {
        status = ERR_CRYPTO_BAD_KEY_TYPE;
        goto exit;
    }

    if (OK > ( status = CA_MGMT_makeKeyBlobEx( &key, ppRetPublicKeyBlob, pRetPublicKeyBlobLength)))
        goto exit;

    *pRetKeyType = key.type;

exit:
    CRYPTO_uninitAsymmetricKey(&key, NULL);

    return status;
}
#endif


/*------------------------------------------------------------------*/

#if (!defined(__DISABLE_MOCANA_CERTIFICATE_GENERATION__) && !defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__))
#if (!defined(__DISABLE_MOCANA_KEY_GENERATION__))
/* Generate a self-signed X.509 certificate and public/private key pair.
This function generates a self-signed X.509 certificate and public/private key pair.
(Self-signed certificates are typically used during application development and
testing.) You should store the certificate and key blob generated by this
function to persistent storage. Your application can also use certificates and
keys generated from other sources.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, the following flags must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$
- $__DISABLE_MOCANA_CERTIFICATE_PARSING__$
- $__DISABLE_MOCANA_KEY_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pRetCertificate      On return, structure referenced will contain a certificate and key blob.
\param keySize              Number of bits in the key. If %keySize is 192, 224, 256, 384, or
521, an EEC certificate is generated with a key of the specified size. If %keySize% is 1024, 2048,
or 4096, an RSA certificate is generated with a key of the specified size.
\param pCertInfo            Pointer to distinguished name information for creating the certificate.
\param signAlgorithm        Hash algorithm to use to sign the certificate.\n
\n
The following enumerated values (defined in crypto.h) are supported:\n
\n
&bull; $ht_md5$\n
&bull; $ht_sha1$\n
&bull; $ht_sha256$\n
&bull; $ht_sha3846$\n
&bull; $ht_sha512$\n
&bull; $ht_sha224$
\param pExtensions          $NULL$ or pointer to a bit string representing the desired
version 3 certificate extensions. For details on forming the bit string, see the
$certExtensions$ documentation.
\param pParentCertificate   $NULL$ or pointer to a parent certificate. If
$NULL$, a self-signed certificate is generated. Otherwise, a child certificate
(signed using the parent certificate) is generated.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it is not used by Mocana internal code.

\example
sbyte status = 0;
certDescriptor newCertificate;
certDistinguishedName certSubjectName;

// omitted code: initialize all fields of newCertificate to 0,
// and set the fields of certSubjectName to the desired value

// generate a self-signed certificate for the EEC curve P-256 with no extensions
if (0 > (status = CA_MGMT_generateCertificateEx(&newCertificate, 256, &certSubjectName, ht_sha256, NULL, NULL)))
{
    goto exit;
}
\endexample

*/
extern sbyte4
CA_MGMT_generateCertificateEx( certDescriptor *pRetCertificate, ubyte4 keySize,
                                const certDistinguishedName *pCertInfo, ubyte signAlgorithm,
                                const certExtensions* pExtensions,
                                const certDescriptor* pParentCertificate)
{
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   key, signKey;
    vlong*          pVlongQueue = NULL;
#if (defined(__ENABLE_MOCANA_ECC__))
    PEllipticCurvePtr   pCurve = 0;
#endif
    ASN1_ITEMPTR    pRootItem = 0;
    MSTATUS         status;

    if ( !pRetCertificate || !pCertInfo) /* the others can be null */
        return ERR_NULL_POINTER;

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
       return status;

    /* initialize results */
    pRetCertificate->pCertificate  = NULL;
    pRetCertificate->certLength    = 0;
    pRetCertificate->pKeyBlob   = NULL;
    pRetCertificate->keyBlobLength = 0;

    CRYPTO_initAsymmetricKey( &key);

    CRYPTO_initAsymmetricKey( &signKey);

    switch (keySize)
    {
#if (defined(__ENABLE_MOCANA_ECC__))
#ifdef __ENABLE_MOCANA_ECC_P192__
    case 192:
        pCurve = EC_P192;
        break;
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
    case 224:
        pCurve = EC_P224;
        break;
#endif
#ifndef __DISABLE_MOCANA_ECC_P256__
    case 256:
        pCurve = EC_P256;
        break;
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
    case 384:
        pCurve = EC_P384;
        break;
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
    case 521:
        pCurve = EC_P521;
        break;
#endif
#endif /* defined(__ENABLE_MOCANA_ECC__) */

    case 1024: case 1536:
    case 2048: case 4096:
        break;

    default:
        status = ERR_EC_UNSUPPORTED_CURVE;
        goto exit;
        break;
    }
#if (defined(__ENABLE_MOCANA_ECC__))
    if (pCurve) /* ECC */
    {
        ECCKey* pECCKey;

        if (OK > (status = CRYPTO_createECCKey(&key, pCurve, &pVlongQueue)))
            goto exit;

        pECCKey = key.key.pECC;

        if (OK > (status = EC_generateKeyPair(pCurve, RANDOM_rngFun, g_pRandomContext,
                                                pECCKey->k, pECCKey->Qx, pECCKey->Qy)))
        {
            goto exit;
        }
        pECCKey->privateKey = TRUE;
    }
    else /* RSA */
#endif /* __ENABLE_MOCANA_ECC__ */
    {
        if (OK > (status = CRYPTO_createRSAKey(&key, &pVlongQueue)))
            goto exit;

        if (OK > (status = RSA_generateKey(MOC_RSA(hwAccelCtx)
                        g_pRandomContext, key.key.pRSA, keySize, &pVlongQueue)))
            goto exit;
    }

    if (OK > (status = CA_MGMT_makeKeyBlobEx(
                        &key, &pRetCertificate->pKeyBlob, &pRetCertificate->keyBlobLength)))
        goto exit;

    /* is there a signing certificate or are we self-signed ? */
    if (pParentCertificate)
    {
        /* need to extract some info */
        ASN1_ITEMPTR pIssuerInfo;
        CStream cs;
        MemFile mf;

        MF_attach( &mf, pParentCertificate->certLength, pParentCertificate->pCertificate);
        CS_AttachMemFile( &cs, &mf);

        if ( OK > ( status = ASN1_Parse( cs, &pRootItem)))
            goto exit;

        if ( OK > ( status = CERT_getCertificateSubject( pRootItem, &pIssuerInfo)))
            goto exit;

        /* extract the key now */
        if (OK > ( status = CA_MGMT_extractKeyBlobEx(
                                        pParentCertificate->pKeyBlob,
                                        pParentCertificate->keyBlobLength,
                                        &signKey)))
        {
            goto exit;
        }

        /* generate the leaf certificate */
        if (OK > ( status = ASN1CERT_generateLeafCertificate( MOC_RSA_HASH(hwAccelCtx) &key, pCertInfo,
                                                                &signKey, pIssuerInfo, cs, signAlgorithm,
                                                                pExtensions, RANDOM_rngFun, g_pRandomContext,
                                                                &pRetCertificate->pCertificate,
                                                                &pRetCertificate->certLength)))
        {
            goto exit;
        }
    }
    else
    {
        if (OK > (status = ASN1CERT_generateCertificate(MOC_RSA_HASH(hwAccelCtx) &key, pCertInfo,
                                                        &key, pCertInfo, signAlgorithm, pExtensions,
                                                        RANDOM_rngFun, g_pRandomContext,
                                                        &pRetCertificate->pCertificate,
                                                        &pRetCertificate->certLength)))
        {
            goto exit;
        }
    }

exit:
    if (OK > status)
    {
        if (NULL != pRetCertificate->pCertificate)
        {
            FREE(pRetCertificate->pCertificate);   pRetCertificate->pCertificate = NULL;
        }

        if (NULL != pRetCertificate->pKeyBlob)
        {
            FREE(pRetCertificate->pKeyBlob);    pRetCertificate->pKeyBlob = NULL;
        }
    }

    if (pRootItem)
    {
        TREE_DeleteTreeItem( (TreeItem*) pRootItem);
    }

    CRYPTO_uninitAsymmetricKey(&signKey, &pVlongQueue);
    CRYPTO_uninitAsymmetricKey(&key, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return (sbyte4)status;
}

extern sbyte4
CA_MGMT_generateCertificateEx2( certDescriptor *pRetCertificate, struct AsymmetricKey* key,
        const certDistinguishedName *pCertInfo, ubyte signAlgorithm)
{
    hwAccelDescr    hwAccelCtx;
    ASN1_ITEMPTR    pRootItem = 0;
    MSTATUS         status;

    if ( !pRetCertificate || !pCertInfo) /* the others can be null */
        return ERR_NULL_POINTER;

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    /* initialize results */
    pRetCertificate->pCertificate  = NULL;
    pRetCertificate->certLength    = 0;
    pRetCertificate->pKeyBlob   = NULL;
    pRetCertificate->keyBlobLength = 0;

    if (OK > (status = ASN1CERT_generateCertificate(MOC_RSA_HASH(hwAccelCtx) key, pCertInfo,
            key, pCertInfo, signAlgorithm, NULL,
            RANDOM_rngFun, g_pRandomContext,
            &pRetCertificate->pCertificate,
            &pRetCertificate->certLength)))
    {
        goto exit;
    }

    exit:
    if (OK > status)
    {
        if (NULL != pRetCertificate->pCertificate)
        {
            FREE(pRetCertificate->pCertificate);   pRetCertificate->pCertificate = NULL;
        }

        if (NULL != pRetCertificate->pKeyBlob)
        {
            FREE(pRetCertificate->pKeyBlob);    pRetCertificate->pKeyBlob = NULL;
        }
    }

    if (pRootItem)
    {
        TREE_DeleteTreeItem( (TreeItem*) pRootItem);
    }

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return (sbyte4)status;
}

#endif /* __DISABLE_MOCANA_KEY_GENERATION__ */


/*------------------------------------------------------------------*/

/* Free memory allocated by CA_MGMT_generateCertificate.
This function frees the memory in the specified buffer that was previously
allocated by a call to CA_MGMT_generateCertificate.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pFreeCertificateDescr    Pointer to the X.509 certificate and key blob to free.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it is not used by Mocana internal code.

\example
sbyte4 status = 0;

status = CA_MGMT_freeCertificate(pFreeThisCertDescr);
\endexample

*/
extern sbyte4
CA_MGMT_freeCertificate(certDescriptor *pFreeCertificateDescr)
{
    if (NULL == pFreeCertificateDescr)
        return (sbyte4)ERR_NULL_POINTER;

    if (NULL != pFreeCertificateDescr->pCertificate)
    {
        FREE(pFreeCertificateDescr->pCertificate);   pFreeCertificateDescr->pCertificate = NULL;
    }

    if (NULL != pFreeCertificateDescr->pKeyBlob)
    {
        FREE(pFreeCertificateDescr->pKeyBlob);    pFreeCertificateDescr->pKeyBlob = NULL;
    }

    return (sbyte4)OK;
}


#if 0
/*------------------------------------------------------------------*/

/* Get a copy of a key blob's public key.
This function gets a copy of the public key in a key blob returned by
CA_MGMT_generateCertificate.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificateDescr Pointer to the certificate descriptor containing the X.509 certificate and key blob generated public key.
\param ppRetPublicKey On return, pointer to the extracted public key.
\param pRetPublicKeyLength On return, pointer to number of bytes in the generated public key.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

\example
sbyte4 status = 0;

status = CA_MGMT_returnPublicKey(pCertificateDescr, &pRetPublicKey, &retPublicKeyLength);
\endexample

*/
extern sbyte4
CA_MGMT_returnPublicKey(certDescriptor *pCertificateDescr,
                        ubyte **ppRetPublicKey, ubyte4 *pRetPublicKeyLength)
{
    AsymmetricKey   tempKey;
    vlong*          pVlongQueue = NULL;
    MSTATUS         status;

    if ((NULL == pCertificateDescr) || (NULL == ppRetPublicKey) || (NULL == pRetPublicKeyLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CRYPTO_initAsymmetricKey(&tempKey)))
        goto exit;

    if (OK > (status = CA_MGMT_extractKeyBlobEx(pCertificateDescr->pKeyBlob, pCertificateDescr->keyBlobLength, &tempKey)))
        goto exit;

    /* FIXME: only deal with RSA certificates */
    if (akt_rsa != tempKey.type)
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    /* get the size of the buffer required */
    if (OK > (status = VLONG_byteStringFromVlong(RSA_N(tempKey.key.pRSA), NULL, (sbyte4 *)pRetPublicKeyLength)))
        goto exit;

    /* allocate the buffer */
    if (NULL == (*ppRetPublicKey = MALLOC(*pRetPublicKeyLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* copy public key to buffer */
    status = VLONG_byteStringFromVlong(RSA_N(tempKey.key.pRSA), *ppRetPublicKey, (sbyte4 *)pRetPublicKeyLength);

exit:
    CRYPTO_uninitAsymmetricKey( &tempKey, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Get number of bytes in a certificate's public key.
This function gets the number of bytes in a specified certificate's public key.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificateDescr Pointer to the certificate descriptor containing the X.509 certificate and key blob generated public key.
\param pRetPublicKeyLengthInBits On return, pointer to length (in bits) of the generated public key.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_returnPublicKeyBitLength(certDescriptor *pCertificateDescr,
                                 ubyte4 *pRetPublicKeyLengthInBits)
{
    AsymmetricKey   tempKey;
    vlong*          pVlongQueue = NULL;
    MSTATUS         status;

    if ((NULL == pCertificateDescr) || (NULL == pRetPublicKeyLengthInBits))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pRetPublicKeyLengthInBits = 0;

    if (OK > (status = CRYPTO_initAsymmetricKey(&tempKey)))
        goto exit;

    if (OK > (status = CA_MGMT_extractKeyBlobEx(pCertificateDescr->pKeyBlob, pCertificateDescr->keyBlobLength, &tempKey)))
        goto exit;

    /* FIXME: only deal with RSA certificates */
    if (akt_rsa != tempKey.type)
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    *pRetPublicKeyLengthInBits = VLONG_bitLength(RSA_N(tempKey.key.pRSA));

exit:
    CRYPTO_uninitAsymmetricKey( &tempKey, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Free memory allocated by CA_MGMT_returnPublicKey.
This function frees the memory in the specified buffer that was previously
allocated by a call to CA_MGMT__returnPublicKey.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param ppRetPublicKey Pointer to the public key to free.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\example
sbyte4 status = 0;

status = CA_MGMT_freePublicKey(&pRetPublicKey);
\endexample

*/
extern sbyte4
CA_MGMT_freePublicKey(ubyte **ppRetPublicKey)
{
    MSTATUS status = ERR_NULL_POINTER;

    if ((NULL != ppRetPublicKey) && (NULL != *ppRetPublicKey))
    {
        FREE(*ppRetPublicKey);
        *ppRetPublicKey = NULL;

        status = OK;
    }

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

/* Get an X.509 certificate's SHA-1 and MD5 fingerprints.
This function gets an X.509 certificate's SHA-1 and MD5
}fingerprints}&mdash;message digests (output of hash functions).

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificate Pointer to the X.509 certificate of interest.
\param certLength Length of the certificate, in bytes.
\param pShaFingerPrint On return, pointer to the generated SHA-1 fingerprint.
\param pMD5FingerPrint On return, pointer to the generated MD5 fingerprint.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

\example
sbyte4 status = 0;

status = CA_MGMT_returnCertificatePrints(pCertificate, certLength, &ShaFgrBuf, &MD5FgrBuf);
\endexample

*/
extern sbyte4
CA_MGMT_returnCertificatePrints(ubyte *pCertificate, ubyte4 certLength,
                                ubyte *pShaFingerPrint, ubyte *pMD5FingerPrint)
{
    hwAccelDescr    hwAccelCtx;
    MSTATUS         status;

    if ((NULL == pCertificate) || (NULL == pShaFingerPrint) || (NULL == pMD5FingerPrint))
        return ERR_NULL_POINTER;

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    /* generate an MD5 hash of the certificate */
    if (OK > (status = MD5_completeDigest(MOC_HASH(hwAccelCtx) pCertificate, certLength, pMD5FingerPrint)))
        goto exit;

    /* generate a SHA hash of the certificate */
    if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) pCertificate, certLength, pShaFingerPrint)))
        goto exit;

exit:
    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Get an X.509 certificate's subject or issuer ASN.1 name.
This function gets an X.509 certificate's subject or issuer ASN.1 name. The
subject is typically used to generate a PKCS-10 request.

\since 1.41
\version 5.3 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificate         Pointer to the X.509 certificate of interest.
\param certificateLength    Length of the certificate, in bytes.
\param isSubject            $TRUE1$ to return the subject's distinguished name;
$FALSE$ to return the issuer's distinguished name.
\param includeASN1SeqHeader $TRUE$ to include the ASN.1 sequence; otherwise
$FALSE$.
\param pASN1NameOffset      On return, pointer to certificate's ASN.1 name field.
\param pASN1NameLen         On return, pointer to number of bytes in
certificate's ASN.1 name.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_extractCertASN1Name(const ubyte *pCertificate, ubyte4 certificateLength,
                              sbyte4 isSubject, sbyte4 includeASN1SeqHeader,
                              ubyte4* pASN1NameOffset, ubyte4* pASN1NameLen)
{
    /* this function is particularly useful when used with PKCS#10 */

    ASN1_ITEMPTR    pCert           = NULL;
    ASN1_ITEMPTR    pSubject;
    MemFile         certMemFile;
    MSTATUS         status;
    CStream         cs;

    if (!pCertificate || !pASN1NameOffset || !pASN1NameLen)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, (ubyte*) pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    /* fetch the data we want to grab */
    if (isSubject)
    {
        if ( OK > ( status = CERT_getCertificateSubject(pCert, &pSubject)))
            goto exit;
    }
    else
    {
        if ( OK > ( status = CERT_getCertificateIssuerSerialNumber(pCert, &pSubject, NULL)))
            goto exit;
    }

    *pASN1NameOffset = (ubyte4) pSubject->dataOffset;
    *pASN1NameLen = pSubject->length;

    if (includeASN1SeqHeader)
    {
        *pASN1NameOffset -= pSubject->headerSize;
        *pASN1NameLen    += pSubject->headerSize;
    }

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Get an X.509 certificate's subject or issuer distinguished name.
This function gets an X.509 certificate's subject or issuer distinguished name.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificate Pointer to the X.509 certificate of interest.
\param certificateLength Length of the certificate, in bytes.
\param isSubject $1$ = subject, $0$ = issuer; distinguished name to be returned.
\param pRetDN On return, pointer to the requested distinguished name.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

\example
certDistinguishedName distName;
sbyte4 status;

status = CA_MGMT_extractCertDistinguishedName(pCertificate, certificateLength, 1, &distName);
\endexample

*/
extern sbyte4
CA_MGMT_extractCertDistinguishedName(ubyte *pCertificate, ubyte4 certificateLength,
                                     intBoolean isSubject, certDistinguishedName *pRetDN)
{
    ASN1_ITEM*  pCert           = NULL;
    MemFile     certMemFile;
    MSTATUS status;
    CStream     cs;

    if ((NULL == pCertificate) || (NULL == pRetDN))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    /* fetch the data we want to grab */
    status = CERT_extractDistinguishedNames(pCert, cs, isSubject, pRetDN);

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_EXTRACT_CERT_BLOB__
/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern sbyte4
CA_MGMT_findCertDistinguishedName(ubyte *pCertificate, ubyte4 certificateLength,
                                  intBoolean isSubject,
                                  ubyte **ppRetDistinguishedName, ubyte4 *pRetDistinguishedNameLen)
{
    ASN1_ITEM*  pCert           = NULL;
    MemFile     certMemFile;
    MSTATUS status;
    CStream     cs;

    if ((NULL == pCertificate) || (NULL == ppRetDistinguishedName) || (NULL == pRetDistinguishedNameLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    /* fetch the data we want to grab */
    status = CERT_extractDistinguishedNamesBlob(pCert, cs, isSubject, ppRetDistinguishedName, pRetDistinguishedNameLen);

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}
#endif /* __ENABLE_MOCANA_EXTRACT_CERT_BLOB__ */

/*------------------------------------------------------------------*/

/* Compare X.509 certificate's subject or issuer distinguished name.
This function compare an X.509 certificate's subject or issuer distinguished name.

\since 5.7
\version 5.7 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pName1 pointer to the first distinguished name to compare.
\param pName2 pointer to the second distinguished name to compare.

\return TRUE if pName1 and pName2 are same, otherwise returns FALSE.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.
*/
extern intBoolean
CA_MGMT_compareCertDistinguishedName(certDistinguishedName *pName1, certDistinguishedName *pName2)
{
    ubyte4 i, j;
    intBoolean result = FALSE;

    if (!pName1 && !pName2)
    {
        result = TRUE;
        goto exit;
    }
    
    if (!pName1 || !pName2)
    {
        goto exit;
    } 

    if (pName1->dnCount != pName2->dnCount)
    {
        goto exit;
    }

    for (i = 0; i < pName1->dnCount; i++)
    {
        relativeDN *pDn1 = &pName1->pDistinguishedName[i];
        relativeDN *pDn2 = &pName2->pDistinguishedName[i];
        
        if (!pDn1 && !pDn2)
        {
            continue;
        }
        if ((!pDn1 && pDn2) || (pDn1 && !pDn2) || (pDn1->nameAttrCount != pDn2->nameAttrCount))
        {
            goto exit;
        }

        for (j = 0; j < pDn1->nameAttrCount; j++)
        {
            sbyte4 result1;
            nameAttr *pNameAttr1 = &pDn1->pNameAttr[j];
            nameAttr *pNameAttr2 = &pDn2->pNameAttr[j];

            if (!pNameAttr1 && !pNameAttr2)
            {
                continue;
            }
            if ((!pNameAttr1 && pNameAttr2) || (pNameAttr1 && !pNameAttr2))
            {
                goto exit;
            }
            if ((pNameAttr1->oid[0] != pNameAttr2->oid[0]) || (pNameAttr1->type != pNameAttr2->type) || (pNameAttr1->valueLen != pNameAttr2->valueLen))
            {
                goto exit;
            }
            MOC_MEMCMP(pNameAttr1->oid+1, pNameAttr2->oid+1, pNameAttr1->oid[0], &result1);
            if (result1 != 0)
            {
                goto exit;
            }

            MOC_MEMCMP(pNameAttr1->value, pNameAttr2->value, pNameAttr1->valueLen, &result1);
            if (result1 != 0)
            {
                goto exit;
            }

        }

    }
    result = TRUE;
exit:
    return result;
}


/*------------------------------------------------------------------*/

/* Get a certificate's start and expiration times and dates.
This function gets a certificate's start and expiration times and dates.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificate Pointer to the X.509 certificate of interest.
\param certificateLength Length of the certificate, in bytes.
\param pRetDN On return, pointer to the certificates start and end dates.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_extractCertTimes(ubyte *pCertificate, ubyte4 certificateLength,
                         certDistinguishedName *pRetDN)
{
    ASN1_ITEM*  pCert           = NULL;
    MemFile     certMemFile;
    MSTATUS status;
    CStream     cs;

    if ((NULL == pCertificate) || (NULL == pRetDN))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    /* fetch the data we want to grab */
    status = CERT_extractValidityTime(pCert, cs, pRetDN);

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Verify correspondence of key blob and certificate.
This function verifies correspondence of a certificate descriptor's key blob and
X.509 certificate.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificateDescr Pointer to the certificate descriptor containing the
X.509 certificate and key blob generated public key.
\param pIsGood Pointer to buffer that on return contains $TRUE$ if the
certificate's key blob properly corresponds to its key, or $FALSE$ if there is no correspondence.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_verifyCertWithKeyBlob(certDescriptor *pCertificateDescr, sbyte4 *pIsGood)
{
    hwAccelDescr    hwAccelCtx;
    ASN1_ITEM*      pCert           = NULL;
    MemFile         certMemFile;
    AsymmetricKey   pBlobKey;
    vlong*          pN              = NULL;
    AsymmetricKey   certKey;
    MSTATUS         status;
    CStream         cs;

    CRYPTO_initAsymmetricKey(&certKey);
    CRYPTO_initAsymmetricKey(&pBlobKey);

    if ((NULL == pCertificateDescr) || (NULL == pCertificateDescr->pCertificate) || (NULL == pIsGood))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pIsGood = FALSE;


    if (OK > (status = CA_MGMT_extractKeyBlobEx(pCertificateDescr->pKeyBlob, pCertificateDescr->keyBlobLength, &pBlobKey)))
        goto exit;

    if (akt_rsa != pBlobKey.type && akt_ecc != pBlobKey.type)
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    /* extract the public key of the certificate */
    if (0 == pCertificateDescr->certLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, pCertificateDescr->certLength, pCertificateDescr->pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse( cs, &pCert)))
    {
        goto exit;
    }

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
       goto exit;

    if (OK > (status = CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(hwAccelCtx) pCert, cs, &certKey)))
    {
        goto exit;
    }

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    if ( akt_rsa == certKey.type)
    {
        /* verify key blob and certificate public keys match */
        if ((0 != VLONG_compareSignedVlongs(RSA_N(certKey.key.pRSA), RSA_N(pBlobKey.key.pRSA))) ||
            (0 != VLONG_compareSignedVlongs(RSA_E(certKey.key.pRSA), RSA_E(pBlobKey.key.pRSA))))
        {
            status = ERR_CERT_AUTH_MISMATCH_PUBLIC_KEYS;
            goto exit;
        }

        /* verify key blob private key matches public key */
        if (OK > (status = VLONG_allocVlong(&pN, NULL)))
            goto exit;

        DEBUG_RELABEL_MEMORY(pN);

        if (OK > (status = VLONG_vlongSignedMultiply(pN, RSA_P(pBlobKey.key.pRSA), RSA_Q(pBlobKey.key.pRSA))))
            goto exit;

        if (0 != VLONG_compareSignedVlongs(pN, RSA_N(pBlobKey.key.pRSA)))
        {
            status = ERR_CERT_AUTH_KEY_BLOB_CORRUPT;
            goto exit;
        }
    }
#ifdef __ENABLE_MOCANA_ECC__
    else if ( akt_ecc == certKey.type)
    {
        byteBoolean cmpRes = FALSE;

        if (OK > (status = EC_equalKey(pBlobKey.key.pECC, certKey.key.pECC, &cmpRes)))
            goto exit;

        if (!cmpRes)
        {
            status = ERR_CERT_AUTH_MISMATCH_PUBLIC_KEYS;
            goto exit;
        }

        /* verify key blob private key matches public key */
        if (OK > (status = EC_verifyKeyPair(pBlobKey.key.pECC->pCurve, pBlobKey.key.pECC->k,
                                            pBlobKey.key.pECC->Qx, pBlobKey.key.pECC->Qy)))
        {
            status = ERR_CERT_AUTH_KEY_BLOB_CORRUPT;
            goto exit;
        }
    }
#endif
    else
    {
        status = ERR_CERT_AUTH_MISMATCH_PUBLIC_KEYS;
        goto exit;
    }

    *pIsGood = TRUE;

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    CRYPTO_uninitAsymmetricKey( &pBlobKey, NULL);
    VLONG_freeVlong(&pN, NULL);
    CRYPTO_uninitAsymmetricKey( &certKey, NULL);

    return (sbyte4)status;
}

#endif /* !defined(__DISABLE_MOCANA_CERTIFICATE_GENERATION__) && !defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__) */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_DER_CONVERSION__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__)

/* Convert DER file key information to Mocana NanoCert key blob.
This function converts key information contained in a }DER
file}&mdash;distinguished encoding rules file, which some applications, such as
open SSL, use to store their key information&mdash;to a Mocana NanoCert key blob.

\since 1.41
\version 5.3 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_DER_CONVERSION__$
- $__ENABLE_MOCANA_PEM_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pDerRsaKey           Pointer to the DER key.
\param derRsaKeyLength      Number of bytes in the DER key.
\param ppRetKeyBlob         On return, pointer to the NanoCert key blob.
\param pRetKeyBlobLength    On return, pointer to number of bytes in the
NanoCert key blob ($pRegKeyBlob$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_convertKeyDER(ubyte *pDerRsaKey, ubyte4 derRsaKeyLength,
                      ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength)
{
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   key;
    MSTATUS         status;

    CRYPTO_initAsymmetricKey(&key);

    /* check input */
    if ((NULL == pDerRsaKey) || (NULL == ppRetKeyBlob) || (NULL == pRetKeyBlobLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
       goto exit;

    status = PKCS_getPKCS1Key(MOC_HASH(hwAccelCtx) pDerRsaKey, derRsaKeyLength, &key);

#ifdef __ENABLE_MOCANA_ECC__
    if (OK > status)
        status = SEC_getKey(pDerRsaKey, derRsaKeyLength, &key);
#endif

#ifdef __ENABLE_MOCANA_DSA__
    if (OK > status)
        status = PKCS_getDSAKey(pDerRsaKey, derRsaKeyLength, &key);
#endif

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    if (OK > status)
        goto exit;

    status = CA_MGMT_makeKeyBlobEx(&key, ppRetKeyBlob, pRetKeyBlobLength);

exit:
    CRYPTO_uninitAsymmetricKey(&key, NULL);

    return (sbyte4)status;

} /* CA_MGMT_convertKeyDER */


/*------------------------------------------------------------------*/

/* Convert NanoCert key blob to DER key.
This function converts a Mocana NanoCert key blob containing RSA private key
information to a PKCS\#1 DER key.

\since 3.2
\version 5.3 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_DER_CONVERSION__$
- $__ENABLE_MOCANA_PEM_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pKeyBlob         Pointer to the NanoCert key blob.
\param keyBlobLength    Number of bytes in the NanoCert key blob.
\param ppRetKeyDER      On return, pointer to the DER key.
\param pRetKeyDERLength On return, pointer to the number of bytes in the DER
key ($ppRetKeyDER$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern MSTATUS
CA_MGMT_keyBlobToDER(const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                     ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    /* Convert RSA keyblob to PKCS1 DER */
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   key;
#ifdef __ENABLE_MOCANA_OPENSSL_PUBKEY_COMPATIBILITY__
    DER_ITEMPTR     pRoot = 0;
#endif
    DER_ITEMPTR     pDummySequence = 0;

    MSTATUS         status = OK;

    CRYPTO_initAsymmetricKey(&key);

    /* check input */
    if ((NULL == pKeyBlob) || (NULL == ppRetKeyDER) || (NULL == pRetKeyDERLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &key)))
        goto exit;

#ifdef __ENABLE_MOCANA_ECC__
    if (akt_ecc == key.type)
    {
        status = SEC_setKey(&key, ppRetKeyDER, pRetKeyDERLength);
        goto exit;
    }
#endif

#ifdef __ENABLE_MOCANA_DSA__
    if (akt_dsa == key.type)
    {
        status = PKCS_setDsaDerKey(&key, ppRetKeyDER, pRetKeyDERLength);
        goto exit;
    }
#endif

    if (akt_rsa != key.type)
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
       goto exit;

#ifdef __ENABLE_MOCANA_OPENSSL_PUBKEY_COMPATIBILITY__
    if (key.key.pRSA->keyType == RsaPrivateKey)
#endif
    {
        status = PKCS_setPKCS1Key(MOC_HASH(hwAccelCtx) &key, ppRetKeyDER, pRetKeyDERLength);
    }
#ifdef __ENABLE_MOCANA_OPENSSL_PUBKEY_COMPATIBILITY__
    else
    {
        if (OK > ( status = DER_AddSequence( NULL, &pDummySequence)))
            goto exit;

        if (OK > (status = ASN1CERT_storePublicKeyInfo(&key, pDummySequence)))
            goto exit;

        if(NULL == (pRoot = DER_FIRST_CHILD(pDummySequence)))
            goto exit;

        if ( OK > ( status = DER_Serialize(pRoot, ppRetKeyDER, pRetKeyDERLength)))
            goto exit;
    }
#endif

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

exit:
    CRYPTO_uninitAsymmetricKey(&key, NULL);

    if ( pDummySequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pDummySequence);
    }

    return status;
} /* CA_MGMT_keyBlobToDER */


#endif /* defined(__ENABLE_MOCANA_DER_CONVERSION__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__) */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_PEM_CONVERSION__

/* Convert PEM file key information to NanoCert key blob.
This function converts key information contained in a }PEM (Privacy Enhanced Mail)
file}&mdash;a DER file that is base 64 encoded&mdash;to a Mocana NanoCert key blob.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_PEM_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pPemRsaKey Pointer to the PEM key.
\param pemRsaKeyLength Length of the PEM key.
\param ppRetKeyBlob On return, pointer to the Mocana key blob.
\param pRetKeyBlobLength On return, pointer to the size of the Mocana key blob.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_convertKeyPEM(ubyte *pPemRsaKey, ubyte4 pemRsaKeyLength,
                      ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength)
{
    ubyte*      pDerRsaKey = 0;
    ubyte4      derRsaKeyLength;
    MSTATUS     status;

    /* check input */
    if ((NULL == pPemRsaKey) || (NULL == ppRetKeyBlob) || (NULL == pRetKeyBlobLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* decode the base64 encoded message */
    if (OK > (status = CA_MGMT_decodeCertificate(pPemRsaKey, pemRsaKeyLength, &pDerRsaKey, &derRsaKeyLength)))
        goto exit;

    status = CA_MGMT_convertKeyDER(pDerRsaKey, derRsaKeyLength, ppRetKeyBlob, pRetKeyBlobLength);

exit:
    if (NULL != pDerRsaKey)
        FREE(pDerRsaKey);

    return (sbyte4)status;
} /* CA_MGMT_convertKeyPEM */


/*------------------------------------------------------------------*/

/* Convert NanoCert key blob to PEM file key information.
This function converts a Mocana NanoCert key blob containing RSA private key
information to a }PEM (Privacy Enhanced Mail) file}&mdash;a DER file that is
base 64 encoded.

\since 3.2
\version 5.3 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_PEM_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pKeyBlob         Pointer to the NanoCert key blob.
\param keyBlobLength    Number of bytes in the NanoCert key blob.
\param ppRetKeyPEM      On return, pointer to the PEM key.
\param pRetKeyPEMLength On return, pointer to the number of bytes in the PEM
key ($ppRetKeyPEM$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern MSTATUS
CA_MGMT_keyBlobToPEM(const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                     ubyte **ppRetKeyPEM, ubyte4 *pRetKeyPEMLength)
{
    static sbyte begin_rsa_priv_line[] = "-----BEGIN RSA PRIVATE KEY-----\n";
    static sbyte end_rsa_priv_line[] = "\n-----END RSA PRIVATE KEY-----\n";

    static sbyte begin_rsa_pub_line[] = "-----BEGIN PUBLIC KEY-----\n";
    static sbyte end_rsa_pub_line[] = "\n-----END PUBLIC KEY-----\n";

#if (defined(__ENABLE_MOCANA_ECC__) || defined(__ENABLE_MOCANA_DSA__))

#ifdef __ENABLE_MOCANA_ECC__
    static sbyte begin_ec_line[] = "-----BEGIN EC PRIVATE KEY-----\n";
    static sbyte end_ec_line[] = "\n-----END EC PRIVATE KEY-----\n";
#endif

#ifdef __ENABLE_MOCANA_DSA__
    static sbyte begin_dsa_line[] = "-----BEGIN DSA PRIVATE KEY-----\n";
    static sbyte end_dsa_line[] = "\n-----END DSA PRIVATE KEY-----\n";
#endif
#endif
    ubyte4      n;
    ubyte4      i;
    ubyte4      rem;
    ubyte       *pTrav = NULL;
    AsymmetricKey key;
    sbyte       *pHdr, *pFtr;
    sbyte4      hdrLength, ftrLength;
    ubyte*      pBase64Mesg = NULL;
    ubyte*      pKeyDER = NULL;
    ubyte4      keyDERLength;
    MSTATUS     status;

    ubyte*      pTmp;

    /* check input */
    if ((NULL == pKeyBlob) || (NULL == ppRetKeyPEM) || (NULL == pRetKeyPEMLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* Convert RSA keyblob to PKCS1 DER */
    if (OK > (status = CA_MGMT_keyBlobToDER(pKeyBlob, keyBlobLength, &pKeyDER, &keyDERLength)))
        goto exit;

    /* get key type */
    CRYPTO_initAsymmetricKey(&key);

    if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &key)))
        goto exit;

    if (akt_rsa == key.type)
    {
        if (key.key.pRSA->keyType == RsaPrivateKey)
        {
            pHdr = begin_rsa_priv_line; pFtr = end_rsa_priv_line;
            hdrLength = sizeof(begin_rsa_priv_line) - 1;
            ftrLength = sizeof(end_rsa_priv_line) - 1;
        }
        else
        {
            pHdr = begin_rsa_pub_line; pFtr = end_rsa_pub_line;
            hdrLength = sizeof(begin_rsa_pub_line) - 1;
            ftrLength = sizeof(end_rsa_pub_line) - 1;
        }
    }
#if (defined(__ENABLE_MOCANA_ECC__) || defined(__ENABLE_MOCANA_DSA__))
#ifdef __ENABLE_MOCANA_ECC__
    else if (akt_ecc == key.type)
    {
        pHdr = begin_ec_line; pFtr = end_ec_line;
        hdrLength = sizeof(begin_ec_line) - 1;
        ftrLength = sizeof(end_ec_line) - 1;
    }
#endif
#ifdef __ENABLE_MOCANA_DSA__
    else if (akt_dsa == key.type)
    {
        pHdr = begin_dsa_line; pFtr = end_dsa_line;
        hdrLength = sizeof(begin_dsa_line) - 1;
        ftrLength = sizeof(end_dsa_line) - 1;
    }
#endif
#endif
    else
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    CRYPTO_uninitAsymmetricKey(&key, NULL);

    /* encode into base64 message */
    if (OK > (status = BASE64_encodeMessage(pKeyDER, keyDERLength, &pBase64Mesg, pRetKeyPEMLength)))
        goto exit;

    /* add '---- XXX ----' lines */
    n = *pRetKeyPEMLength/64;
    rem = (*pRetKeyPEMLength)%64;

    if (NULL == (pTmp = *ppRetKeyPEM = MALLOC(*pRetKeyPEMLength + hdrLength + ftrLength + n)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pTmp, pHdr, hdrLength);
    pTmp += hdrLength;

    pTrav = pBase64Mesg;

    for (i=0; i < n; i++)
    {
        MOC_MEMCPY(pTmp, pTrav, 64);
        pTmp += 64;
        pTrav += 64;
        *pTmp++ = '\n';
    }
    if (rem)
    {
        MOC_MEMCPY(pTmp, pTrav, rem);
        pTmp += rem;
    }

    MOC_MEMCPY(pTmp, pFtr, ftrLength);

    *pRetKeyPEMLength += hdrLength + ftrLength + n;

exit:
    if (NULL != pBase64Mesg)
        FREE(pBase64Mesg);

    if (NULL != pKeyDER)
        FREE(pKeyDER);

    return status;
} /* CA_MGMT_keyBlobToPEM */


#endif /* __ENABLE_MOCANA_PEM_CONVERSION__ */


/*------------------------------------------------------------------*/

/* experimental, do not use. */
#if 0
#ifdef __ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__
extern sbyte4
CA_MGMT_rawVerifyOID(ubyte *pCertificate, ubyte4 certificateLength,
                     const ubyte *pOidItem, const ubyte *pOidValue, ubyte4 *pIsPresent)
{
    ASN1_ITEM*  pCert           = NULL;
    MemFile     certMemFile;
    CStream     cs;
    MSTATUS status;

    if ((NULL == pCertificate) || (NULL == pIsPresent) ||
        (NULL == pOidItem) || (NULL == pOidValue))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    /* fetch the data we want to grab */
    status = CERT_rawVerifyOID(pCert, cs, pOidItem, pOidValue, (intBoolean *)pIsPresent);

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}
#endif
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__
extern sbyte4
CA_MGMT_extractSerialNum(ubyte*  pCertificate,   ubyte4  certificateLength,
                         ubyte** ppRetSerialNum, ubyte4* pRetSerialNumLength)
{
    ASN1_ITEM*  pCert           = NULL;
    MemFile     certMemFile;
    CStream     cs;
    MSTATUS status;

    if ((NULL == pCertificate) || (NULL == ppRetSerialNum) || (NULL == pRetSerialNumLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    status = CERT_extractSerialNum(pCert, cs, ppRetSerialNum, pRetSerialNumLength);

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__
extern sbyte4
CA_MGMT_freeSearchDetails(ubyte** ppFreeData)
{
    MSTATUS status = OK;

    if ((NULL == ppFreeData) || (NULL == *ppFreeData))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    FREE(*ppFreeData);
    *ppFreeData = NULL;

exit:
    return (sbyte4)status;
}
#endif



/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__

/*!
\exclude
*/
typedef struct CA_MGMT_enumItemInfo
{
    void*                   userArg;
    CA_MGMT_EnumItemCBFun   userCb;
    ubyte4                  counter;
} CA_MGMT_enumItemInfo;


static MSTATUS
EnumItemCallbackFun( ASN1_ITEM* pItem, CStream cs, void* userArg)
{
    MSTATUS status;
    CA_MGMT_enumItemInfo* pInfo = (CA_MGMT_enumItemInfo*) userArg;
    const ubyte* value;

    /* extract all info for the user call back */
    value = CS_memaccess( cs, pItem->dataOffset, pItem->length);
    if ( !value)
    {
        return ERR_MEM_ALLOC_FAIL;
    }
    status = pInfo->userCb( value, pItem->length, pItem->tag, pInfo->counter, pInfo->userArg);
    CS_stopaccess( cs, value);

    ++(pInfo->counter);
   return status;
}


/* Enumerate the CRLs (certificate revocation lists) in a certificate.
This function enumerates the CRLs (certificate revocation lists) in a
certificate.

\since 2.02
\version 4.0 and later

The $callbackFunc$ parameter's callback function must have the following method
signature:

$sbyte4 userFunc(const ubyte *pContent, ubyte4 contentLen, ubyte4 contentType, ubyte4 index, void *userArg);$

!!! Syntax/parameters
||Name||Type||Description
|$\<pContent\>$|$const ubyte *$|Pointer to CRL buffer.
|$\<contentLen\>$|$ubyte4$|Number of bytes in CRL buffer ($pContent$).
|$\<contentType\>$|$ubyte4$|ASN.1 tag associated with the CRL, which indicates how to interpret the CRL's contents. For details, refer to the description of the GeneralName type http://www.itu.int/ITU-T/asn1/database/itu-t/x/x509/1997/CertificateExtensions.html#CertificateExtensions.GeneralName.
|$\<index\>$|$ubyte4$|0-based index of this CRL's location in the CRL list.
|$\<userArg\>$|$void *$|Value of the $userArg$ parameter when the $%CA_MGMT_enumCrl$ function was called.
|&nbsp;|&nbsp;|&nbsp;\n
||Return value|$sbyte4$|Negative number to stop CRL enumeration; otherwise a number >= 0 to continue CRL enumeration.

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificate         Pointer to DER-encoded certificate.
\param certificateLength    Number of bytes in the certificate ($pCertificate$).
\param callbackFunc         Pointer to user-defined callback function to be
called for each CRL. See the description section for callback function details.
\param userArg              Pointer to argument to provide to callback function:
$NULL$ or a context to provide to the callback function.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_enumCrl(ubyte* pCertificate, ubyte4 certificateLength,
                CA_MGMT_EnumItemCBFun callbackFunc, void* userArg)
{
    ASN1_ITEM*           pCert           = NULL;
    MemFile              certMemFile;
    CStream              cs;
    MSTATUS              status;
    CA_MGMT_enumItemInfo info;

    if ((NULL == pCertificate) || (NULL == callbackFunc))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    info.counter = 0;
    info.userArg = userArg;
    info.userCb = callbackFunc;

    if (OK > (status = CERT_enumerateCRL( pCert, cs, EnumItemCallbackFun, &info)))
        goto exit;

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Enumerate the subject/issuer alternative name in a certificate.
This function enumerates the subject/issuer alternative name in a
certificate.

\since 4.0
\version 4.0 and later

The $callbackFunc$ parameter's callback function must have the following method
signature:

$sbyte4 userFunc(const ubyte *pContent, ubyte4 contentLen, ubyte4 contentType, ubyte4 index, void *userArg);$

!!! Syntax/parameters
||Name||Type||Description
|$\<pContent\>$|$const ubyte *$|Pointer to alternative name buffer.
|$\<contentLen\>$|$ubyte4$|Number of bytes in buffer ($pContent$).
|$\<contentType\>$|$ubyte4$|ASN.1 tag associated with the alternative name, which indicates how to interpret the alternative name's contents. For details, refer to the description of the GeneralName type http://www.itu.int/ITU-T/asn1/database/itu-t/x/x509/1997/CertificateExtensions.html#CertificateExtensions.GeneralName.
|$\<index\>$|$ubyte4$|0-based index of this alternative name's location in the alternative name list.
|$\<userArg\>$|$void *$|Value of the $userArg$ parameter when the $%CA_MGMT_enumAltName$ function was called.
|&nbsp;|&nbsp;|&nbsp;\n
||Return value|$sbyte4$|Negative number to stop alternative name enumeration; otherwise a number >= 0 to continue alternative name enumeration.

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertificate         Pointer to DER-encoded certificate.
\param certificateLength    Number of bytes in the certificate ($pCertificate$).
\param isSubject            $1$ = subject, $0$ = issuer; alternative name to be returned.
\param callbackFunc         Pointer to user-defined callback function to be
called for each alternative name. See the description section for callback function details.
\param userArg              Pointer to argument to provide to callback function:
$NULL$ or a context to provide to the callback function.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_enumAltName(ubyte* pCertificate, ubyte4 certificateLength, sbyte4 isSubject,
                    CA_MGMT_EnumItemCBFun callbackFunc, void* userArg)
{
    ASN1_ITEM*           pCert           = NULL;
    MemFile              certMemFile;
    CStream              cs;
    MSTATUS              status;
    CA_MGMT_enumItemInfo info;

    if ((NULL == pCertificate) || (NULL == callbackFunc))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, certificateLength, pCertificate);
    CS_AttachMemFile( &cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pCert)))
        goto exit;

    info.counter = 0;
    info.userArg = userArg;
    info.userCb = callbackFunc;

    if (OK > (status = CERT_enumerateAltName( pCert, cs, isSubject, EnumItemCallbackFun, &info)))
        goto exit;

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

/* Allocate and initialize $pCertificateDesc$ structure.
This function allocates and initializaes (to zero) a $pCertificateDesc$ structure.

\since 1.41
\version 1.41 and later

! Flags
No flag definitions are required to use this function.

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param ppNewCertDistName    On return, structure referenced contains the allocated and initialized certificate.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_allocCertDistinguishedName(certDistinguishedName **ppNewCertDistName)
{
    certDistinguishedName *pInitCertDistName;
    MSTATUS status;

    if (NULL == ppNewCertDistName)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *ppNewCertDistName = NULL;

    if (NULL == (pInitCertDistName = MALLOC(sizeof(certDistinguishedName))))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    status = MOC_MEMSET((ubyte *)pInitCertDistName, 0x00, sizeof(certDistinguishedName));

    *ppNewCertDistName = pInitCertDistName;

exit:
    return status;
}


/*------------------------------------------------------------------*/

/* Free $certDistinguishedName$ structure's memory.
This function frees the memory in the given $certDistinguishedName$ structure,
as well as all memory pointed to by the structure's fields.

\since 1.41
\version 1.41 and later

! Flags
No flag definitions are required to use this function.

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param ppFreeCertDistName Pointer to the X.509 certificate's distinguished name structure to release.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This is a convenience function provided for your application's use; it
is not used by Mocana internal code.

*/
extern sbyte4
CA_MGMT_freeCertDistinguishedName(certDistinguishedName **ppFreeCertDistName)
{
    certDistinguishedName *pFreeCertDistName;
    MSTATUS status = OK;

    if ((NULL == ppFreeCertDistName) || (NULL == *ppFreeCertDistName))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pFreeCertDistName = *ppFreeCertDistName;

    if (NULL != pFreeCertDistName->pDistinguishedName)
    {
        relativeDN *pRDN;
        ubyte4 i = 0;

        for (pRDN = pFreeCertDistName->pDistinguishedName;
            i < pFreeCertDistName->dnCount; pRDN = pFreeCertDistName->pDistinguishedName + i)
        {
            ubyte4 j = 0;
            nameAttr *pNameComponent;
            for (pNameComponent = pRDN->pNameAttr;
                j < pRDN->nameAttrCount; pNameComponent = pRDN->pNameAttr + j)
            {
                if (pNameComponent->value && pNameComponent->valueLen > 0)
                {
                    FREE(pNameComponent->value);
                }
                j = j + 1;
            }
            FREE(pRDN->pNameAttr);
            i = i + 1;
        }
        FREE(pFreeCertDistName->pDistinguishedName);
    }
    if (NULL != pFreeCertDistName->pEndDate)
        FREE(pFreeCertDistName->pEndDate);

    if (NULL != pFreeCertDistName->pStartDate)
        FREE(pFreeCertDistName->pStartDate);

    FREE(pFreeCertDistName);
    *ppFreeCertDistName = NULL;

exit:
    return status;
}


/*------------------------------------------------------------------*/

#if (!defined(__DISABLE_MOCANA_KEY_GENERATION__))
/* Generate a naked key.
This function generates a }naked key}&mdash;a key blob that has no
associated certificate&mdash;. The naked key can be used as input to generate a
certificate or as signing data for authentication.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_KEY_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param keyType              Type of key to generate.\n
\n
The following enumerated values (defined in ca_mgmt.h) are supported:\n
\n
&bull; $akt_undefined$\n
&bull; $akt_rsa$\n
&bull; $akt_ecc$\n
&bull; $akt_dsa$
\param keySize              Number of bits the generated key must contain.
\param ppRetNewKeyBlob      On return, pointer to generated naked key.
\param pRetNewKeyBlobLength On return, pointer to number of bytes in the naked key ($ppRetNewKeyBlob$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern sbyte4
CA_MGMT_generateNakedKey(ubyte4 keyType, ubyte4 keySize,
                         ubyte **ppRetNewKeyBlob, ubyte4 *pRetNewKeyBlobLength)
{
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   key;
    vlong*          pVlongQueue = NULL;
    intBoolean      isHwAccelInit = FALSE;
    MSTATUS         status;

    if ((NULL == ppRetNewKeyBlob) || (NULL == pRetNewKeyBlobLength))
        return ERR_NULL_POINTER;

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
       return status;

    isHwAccelInit = TRUE;

    CRYPTO_initAsymmetricKey(&key);

    if (akt_ecc == keyType)
    {
#if (defined(__ENABLE_MOCANA_ECC__))
        PEllipticCurvePtr   pCurve = 0;

        switch (keySize)
        {
#ifdef __ENABLE_MOCANA_ECC_P192__
            case 192:
            {
                pCurve = EC_P192;
                break;
            }
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
            case 224:
            {
                pCurve = EC_P224;
                break;
            }
#endif
#ifndef __DISABLE_MOCANA_ECC_P256__
            case 256:
            {
                pCurve = EC_P256;
                break;
            }
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
            case 384:
            {
                pCurve = EC_P384;
                break;
            }
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
            case 521:
            {
                pCurve = EC_P521;
                break;
            }
#endif
            default:
            {
                status = ERR_EC_UNSUPPORTED_CURVE;
                goto exit;
            }
        } /* switch */

        if (pCurve)
        {
            ECCKey* pECCKey;

            if (OK > (status = CRYPTO_createECCKey(&key, pCurve, &pVlongQueue)))
                goto exit;

            pECCKey = key.key.pECC;

            if (OK > (status = EC_generateKeyPair(pCurve, RANDOM_rngFun, g_pRandomContext,
                                                  pECCKey->k, pECCKey->Qx, pECCKey->Qy)))
            {
                goto exit;
            }
            pECCKey->privateKey = TRUE;
        }
#else
        status = ERR_CRYPTO_ECC_DISABLED;
        goto exit;
#endif
    }
    else if (akt_rsa == keyType)
    {
        if (OK > (status = CRYPTO_createRSAKey(&key, &pVlongQueue)))
            goto exit;

        if (OK > (status = RSA_generateKey(MOC_RSA(hwAccelCtx) g_pRandomContext,
                                           key.key.pRSA, keySize, &pVlongQueue)))
        {
            goto exit;
        }
    }
    else if (akt_dsa == keyType)
    {
#if (defined(__ENABLE_MOCANA_DSA__))
        if (OK > (status = CRYPTO_createDSAKey(&key, &pVlongQueue)))
            goto exit;

        if (OK > (status = DSA_generateKey(MOC_DSA(hwAccelCtx) g_pRandomContext, key.key.pDSA, keySize, NULL, NULL, NULL, &pVlongQueue)))
            goto exit;
#else
        status = ERR_CRYPTO_DSA_DISABLED;
        goto exit;
#endif
    }
    else
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    status = CA_MGMT_makeKeyBlobEx(&key, ppRetNewKeyBlob, pRetNewKeyBlobLength);

exit:
    CRYPTO_uninitAsymmetricKey(&key, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);

    if (TRUE == isHwAccelInit)
        HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#if (!defined(__DISABLE_MOCANA_KEY_GENERATION__))
/* Free naked key blob's memory.
This function frees the memory used by a naked key blob.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_KEY_GENERATION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param ppFreeKeyBlob    Pointer naked key blob to release.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern sbyte4
CA_MGMT_freeNakedKey(ubyte **ppFreeKeyBlob)
{
    if ((NULL != ppFreeKeyBlob) && (NULL != *ppFreeKeyBlob))
    {
        FREE(*ppFreeKeyBlob);
        *ppFreeKeyBlob = NULL;
    }

    return OK;
}
#endif


/*------------------------------------------------------------------*/

#if ((!defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__)) && (!(defined(__DISABLE_MOCANA_KEY_GENERATION__)) || defined(__ENABLE_MOCANA_PEM_CONVERSION__) || defined(__ENABLE_MOCANA_DER_CONVERSION__)))
/* Convert unprotected RSA private key to a Mocana private RSA keyblob.
This function converts an unprotected RSA private key (extracted from a
PKCS&nbsp;\#8 DER-encoded buffer) to a Mocana private RSA keyblob.

\note After you are done with the returned keyblob, be sure to free its memory
by calling the CA_MGMT_freeKeyBlob function.

\since 5.1
\version 5.1 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_PARSING__$

Additionally, $__DISABLE_MOCANA_KEY_GENERATION__$ must #not# be defined, or one
of the following flags must be defined:
- $__ENABLE_MOCANA_PEM_CONVERSION__$
- $__ENABLE_MOCANA_DER_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pPKCS8DER            Pointer to PKCS&nbsp;\#8 DER-encoded key.
\param pkcs8DERLen          Number of bytes of DER-encoded key ($pPKCS8DER$).
\param ppRetKeyBlob         On return, pointer to converted keyblob.
\param pRetKeyBlobLength    On return, number of bytes of the converted keyblob ($ppRetKeyBlob$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
MOC_EXTERN sbyte4
CA_MGMT_convertPKCS8KeyToKeyBlob(const ubyte* pPKCS8DER, ubyte4 pkcs8DERLen,
                                 ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength)
{
    intBoolean      isHwAccelInit = FALSE;
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   rsaKey;
    MSTATUS         status;

    CRYPTO_initAsymmetricKey(&rsaKey);

    if (OK > (status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        goto exit;

    isHwAccelInit = TRUE;

    /* convert PKCS #8 encoded key into internal key structure */
    if (OK > (status = PKCS_getPKCS8Key(MOC_RSA(hwAccelCtx) pPKCS8DER, pkcs8DERLen, &rsaKey)))
        goto exit;

    /* convert internal structure into key blob */
    status = CA_MGMT_makeKeyBlobEx(&rsaKey, ppRetKeyBlob, pRetKeyBlobLength);

exit:
    CRYPTO_uninitAsymmetricKey(&rsaKey, NULL);

    if (TRUE == isHwAccelInit)
        HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return (sbyte4)status;

} /* CA_MGMT_convertPKCS8KeyToKeyBlob */

#endif


/*------------------------------------------------------------------*/

#if ((!defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__)) && (!(defined(__DISABLE_MOCANA_KEY_GENERATION__)) || defined(__ENABLE_MOCANA_PEM_CONVERSION__) || defined(__ENABLE_MOCANA_DER_CONVERSION__)))
/* Extract a protected RSA private key from a PKCS&nbsp;\#8 DER encoded buffer, converting into Mocana unprotected private RSA key blob.
This function extracts a protected RSA private key from a PKCS&nbsp;\#8 DER-
encoded buffer, and converts the key to a Mocana unproected private RSA
keyblob.

\note After you are done with the returned keyblob, be sure to free its memory
by calling the CA_MGMT_freeKeyBlob function.

\since 5.1
\version 5.1 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_PARSING__$

Additionally, $__DISABLE_MOCANA_KEY_GENERATION__$ must #not# be defined, or one
of the following flags must be defined:
- $__ENABLE_MOCANA_PEM_CONVERSION__$
- $__ENABLE_MOCANA_DER_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pPKCS8DER            Pointer to PKCS&nbsp;\#8 DER encoded key.
\param pkcs8DERLen          Length in bytes of $pPKCS8DER$.
\param pPassword            Pointer to password to protect the PKCS&nbsp;\#8 DER encoded key.
\param passwordLen          Length in bytes of $pPassword$.
\param ppRetKeyBlob         On return, pointer to converted key blob.
\param pRetKeyBlobLength    On return, length of extracted key blob $ppRetKeyBlob$.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/

MOC_EXTERN sbyte4
CA_MGMT_convertProtectedPKCS8KeyToKeyBlob(const ubyte* pPKCS8DER, ubyte4 pkcs8DERLen,
                                          ubyte *pPassword, ubyte4 passwordLen,
                                          ubyte **ppRetKeyBlob, ubyte4 *pRetKeyBlobLength)
{
    intBoolean      isHwAccelInit = FALSE;
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   rsaKey;
    MSTATUS         status;

    CRYPTO_initAsymmetricKey(&rsaKey);

    if (OK > (status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        goto exit;

    isHwAccelInit = TRUE;

    /* convert PKCS #8 encoded key into internal key structure */
    if (OK > (status = PKCS_getPKCS8KeyEx(MOC_RSA_SYM_HASH(hwAccelCtx) pPKCS8DER, pkcs8DERLen,
                                          pPassword, passwordLen, &rsaKey)))
    {
        goto exit;
    }

    /* convert internal structure into key blob */
    status = CA_MGMT_makeKeyBlobEx(&rsaKey, ppRetKeyBlob, pRetKeyBlobLength);

exit:
    CRYPTO_uninitAsymmetricKey(&rsaKey, NULL);

    if (TRUE == isHwAccelInit)
        HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return (sbyte4)status;

} /* CA_MGMT_convertProtectedPKCS8KeyToKeyBlob */

#endif


/*------------------------------------------------------------------*/

#if ((!defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__)) && defined( __ENABLE_MOCANA_DER_CONVERSION__))
/* Encapsulate a keyblob in a protected PKCS&nbsp;\#8 DER-encoded buffer.
This function encapsulates a keyblob in a protected PKCS&nbsp;\#8 DER-encoded buffer.

\note After you are done with the returned keyblob, be sure to free its memory
by calling the CA_MGMT_freeKeyBlob function.

\since 5.1
\version 5.1 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_PARSING__$

Additionally, the following flag must be defined:
- $__ENABLE_MOCANA_DER_CONVERSION__$

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pKeyBlob             Pointer to key blob to convert.
\param keyBlobLength        Number of bytes in the extracted keyblob ($pKeyBlob$).
\param encType              Type of encryption method to use; see pkcs_key.h.
\param pPassword            Pointer to password to use to protect the
PKCS&nbsp;\#8 DER-encoded key.
\param passwordLen          Number of bytes in the password ($pPassword$).
\param ppRetPKCS8DER        On return, pointer to PKCS&nbsp;\#8 DER-encoded key.
\param pRetPkcs8DERLen      On return, number of bytes in the DER-encoded key
($ppRetPKCS8DER$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/

MOC_EXTERN sbyte4
CA_MGMT_convertKeyBlobToPKCS8Key(const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                                 enum PKCS8EncryptionType encType,
                                 const ubyte *pPassword, ubyte4 passwordLen,
                                 ubyte **ppRetPKCS8DER, ubyte4 *pRetPkcs8DERLen)
{
    intBoolean      isHwAccelInit = FALSE;
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   rsaKey;
    MSTATUS         status;

    MOC_UNUSED(hwAccelCtx);

    CRYPTO_initAsymmetricKey(&rsaKey);

    if (OK > (status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        goto exit;

    isHwAccelInit = TRUE;

    /* convert key blob into internal key structure */
    if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &rsaKey)))
        goto exit;

    /* convert internal key structure into PKCS #8 encoded key */
    status = PKCS_setPKCS8Key(MOC_RSA_SYM_HASH(hwAccelCtx)
                              &rsaKey, g_pRandomContext, encType,
                              pPassword, passwordLen,
                              ppRetPKCS8DER, pRetPkcs8DERLen);

exit:
    CRYPTO_uninitAsymmetricKey(&rsaKey, NULL);

    if (TRUE == isHwAccelInit)
        HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return (sbyte4)status;

} /* CA_MGMT_convertProtectedPKCS8KeyToKeyBlob */

#endif /* ((!defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__)) && defined( __ENABLE_MOCANA_DER_CONVERSION__)) */


/*------------------------------------------------------------------*/

MOC_EXTERN sbyte4
CA_MGMT_cloneKeyBlob(const ubyte* pKeyBlob, ubyte4 keyBlobLen, ubyte** ppRetKeyBlob, ubyte4 *pRetKeyBlobLen)
{
    MSTATUS status = ERR_NULL_POINTER;
    ubyte* tmp;

    if ((NULL != pKeyBlob) && (NULL != ppRetKeyBlob))
    {
        *ppRetKeyBlob = NULL;
        tmp = MALLOC(keyBlobLen);

        if (NULL == tmp)
        {
            status = ERR_MEM_ALLOC_FAIL;
        }
        else
        {
            MOC_MEMCPY(tmp, pKeyBlob, keyBlobLen);
            if (pRetKeyBlobLen)
                *pRetKeyBlobLen = keyBlobLen;
            *ppRetKeyBlob = tmp;
            status = OK;
        }
    }

    return (sbyte4)status;
}

/*------------------------------------------------------------------*/

/* Free memory allocated for a Mocana keyblob.
This function frees keyblob memory allocated by a previous call to
CA_MGMT_convertPKCS8KeyToKeyBlob, CA_MGMT_convertProtectedPKCS8KeyToKeyBlob, or
CA_MGMT_convertKeyBlobToPKCS8Key.

\since 5.1
\version 5.1 and later

! Flags
No flag definitions are required to use this function.

#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param ppFreeThisKeyBlob    Pointer to the key to free.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\example
sbyte4 status = 0;

status = CA_MGMT_freeKeyBlob(&pFreeThisKeyBlob);
\endexample

*/
MOC_EXTERN sbyte4
CA_MGMT_freeKeyBlob(ubyte **ppFreeThisKeyBlob)
{
    MSTATUS status = ERR_NULL_POINTER;

    if ((NULL != ppFreeThisKeyBlob) && (NULL != *ppFreeThisKeyBlob))
    {
        FREE(*ppFreeThisKeyBlob);
        *ppFreeThisKeyBlob = NULL;

        status = OK;
    }

    return (sbyte4)status;
}

/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
/* This function reorder the un ordered cert chain

\since 5.5.1
\version 5.5.1 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_CERTIFICATE_PARSING__$

Additionally, the following flag must be defined:


#Include %file:#&nbsp;&nbsp;ca_mgmt.h

\param pCertChain           chain of certificate that have to reorder
\param numCertsInChain      number of cerificate in the chain

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/

MOC_EXTERN sbyte4
CA_MGMT_reorderChain(certDescriptor* pCertChain, sbyte4 numCertsInChain,sbyte* numReorderdCerts)
{
    MemFile         certMemFile;
    MemFile         prevCertMemFile;
    CStream         cs;
    ASN1_ITEM       *pCertificate;
    certBasket      *pCertBasket = NULL;
    certBasket      *pRoot = NULL;
    certBasket      *pNewRoot = NULL;
    certBasket      *pLast = NULL;
    certBasket      *pNewLast = NULL;
    certBasket      *pTempCB = NULL;
    certBasket      *pCurrent = NULL;
    CStream         currentCS;
    CStream         prevCS;
    sbyte4          status = OK;
    sbyte4          i = 0;
    sbyte4          isRoot = 0;
    sbyte4          invalidCertChain = 0;
    intBoolean      attachRoot = FALSE;
    intBoolean      attachLeaf = FALSE;

    if (NULL == pCertChain || NULL == numReorderdCerts)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }
    if ( 0 >= numCertsInChain)
    {
        status = ERR_INVALID_ARG;
        goto exit;
    }
    else if (1 == numCertsInChain)
    {
        status = OK;
        *numReorderdCerts = 1;
        goto exit;
    }

    /* avoid multiple asn1 parsing */
    for (i=0; i < numCertsInChain; i++)
    {
        if ((pCertChain[i].pCertificate == NULL) || (pCertChain[i].certLength <= 0))
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }

        if (NULL == (pCertBasket = MALLOC(sizeof(certBasket))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MF_attach(&certMemFile, pCertChain[i].certLength, pCertChain[i].pCertificate);
        CS_AttachMemFile(&cs, &certMemFile);

        if (OK > (status = ASN1_Parse(cs, &pCertificate)))
        {
            FREE (pCertBasket);
            goto exit;
        }

        pCertBasket->pCertificate = pCertChain[i].pCertificate;
        pCertBasket->certLength   = pCertChain[i].certLength;
        pCertBasket->pCert = pCertificate;

        pRoot = pCertBasket;

        pCertBasket->pPrev = pLast;
        pCertBasket->pNext = NULL;
        pLast = pCertBasket;
        pCertBasket = NULL;
    }

    /* New list preparation for the orderd list */
    pNewRoot        = pRoot;
    pNewLast        = pRoot;
    pRoot           = pRoot->pNext;
    if (pRoot != NULL)
        pRoot->pPrev    = NULL;

    pNewRoot->pNext = NULL;
    pNewRoot->pPrev = NULL;

    pTempCB         = pRoot;
    i = 1; // counter for total cerificate count that are arranged
    /* check first cert for root certificate */
    MF_attach(&certMemFile, pNewRoot->certLength, pNewRoot->pCertificate);
    CS_AttachMemFile(&cs, &certMemFile);

    if (OK <= (CERT_isRootCertificate(pNewRoot->pCert, cs)))
        isRoot++;

    /*    root end --- pRootCert->p1Cert->p2Cert->p3Cert->pLastCert --- Leaf end
      first check at the root end if we got the root of current root cert then add above
      the pRoot or if we got the leaf of the pLast then add next to the pLastCert */
    while (NULL != pTempCB)
    {
        /* verify issuer of prevCert == subject of currentCert   */
        MF_attach(&prevCertMemFile, pNewRoot->certLength, pNewRoot->pCertificate);
        CS_AttachMemFile(&currentCS, &prevCertMemFile);

        MF_attach(&certMemFile,pTempCB->certLength, pTempCB->pCertificate);
        CS_AttachMemFile(&prevCS, &certMemFile);

        /* check multiple root certificate */
        if ((isRoot > 0 ) && (OK <= (status = CERT_isRootCertificate(pTempCB->pCert, prevCS))))
        {
            if (pTempCB == pLast)
            {
                /* this is the case where some of the certificates are out of order */
                if (invalidCertChain == 0)
                    break;

                invalidCertChain = 0;
                pTempCB = pRoot;
            }
            else
                pTempCB = pTempCB->pNext;

            continue;
        }

        /* check for the root certificate */
        if ((isRoot == 0) && (OK == (status = CERT_checkCertificateIssuer(pNewRoot->pCert,currentCS, pTempCB->pCert, prevCS))))
        {
            /* check root certificate */
            if (OK <= (CERT_isRootCertificate(pTempCB->pCert, prevCS)))
                isRoot++;

            attachRoot = TRUE;
        }

        if (attachRoot == FALSE)
        {
            /* check for the leaf of last certificate */
            MF_attach(&prevCertMemFile, pNewLast->certLength, pNewLast->pCertificate);
            CS_AttachMemFile(&prevCS, &prevCertMemFile);

            MF_attach(&certMemFile,pTempCB->certLength, pTempCB->pCertificate);
            CS_AttachMemFile(&currentCS, &certMemFile);

            if (OK == (status = CERT_checkCertificateIssuer(pTempCB->pCert, currentCS, pNewLast->pCert, prevCS)))
                attachLeaf = TRUE;
        }

        if (attachRoot || attachLeaf)
        {
            /* Remove certificate from the un-ordered basket */
            if (pTempCB == pRoot)
            {
                pRoot         = pRoot->pNext;
                if (pRoot != NULL)
                    pRoot->pPrev  = NULL;
                pCurrent      = pRoot;
            }
            else if (pTempCB == pLast)
            {
                pLast        = pLast->pPrev;
                if (pLast != NULL)
                    pLast->pNext = NULL;
                pCurrent     = pRoot;
            }
            else
            {
                pTempCB->pNext->pPrev  = pTempCB->pPrev;
                pTempCB->pPrev->pNext  = pTempCB->pNext;
                pCurrent               = pTempCB->pNext;
            }

            if (attachRoot)
            {
                /* put certificate into the ordered basket */
                pNewRoot->pPrev = pTempCB;
                pTempCB->pNext  = pNewRoot;
                pTempCB->pPrev  = NULL;
                pNewRoot        = pTempCB;

                attachRoot = FALSE;
            }
            else if (attachLeaf)
            {
                /* put certificate into the ordered basket */
                pNewLast->pNext = pTempCB;
                pTempCB->pNext  = NULL;
                pTempCB->pPrev  = pNewLast;
                pNewLast        = pTempCB;

                attachLeaf = FALSE;
            }

            pTempCB         = pCurrent;
            invalidCertChain++;
            i++;
            continue;
        }

        if (pTempCB == pLast)
        {
            /* this is the case where some of the certificates are out of order */
            if (invalidCertChain == 0)
                break;

            invalidCertChain = 0;
            pTempCB = pRoot;
        }
        else
            pTempCB = pTempCB->pNext;
    }

    *numReorderdCerts = i;
    pTempCB = pNewLast;
    i = 0;

    /* make the orderd certificates chain */
    while (pTempCB != NULL)
    {
        pCertChain[i].pCertificate  = pTempCB->pCertificate;
        pCertChain[i].certLength    = pTempCB->certLength;
        pTempCB = pTempCB->pPrev;
        i++;
    }

    /* remaining unorderd cert chain */
    if (*numReorderdCerts < numCertsInChain)
    {
        if (NULL != pLast)
            pTempCB = pLast;

        while (pTempCB != NULL)
        {
            pCertChain[i].pCertificate = pTempCB->pCertificate;
            pCertChain[i].certLength   = pTempCB->certLength;
            pTempCB = pTempCB->pPrev;
            i++;
        }
    }

    /* lets check if we miss any cerificate */
    if (i != numCertsInChain)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    else
        status = OK;
exit:
    pTempCB = pNewRoot;
    while (pTempCB != NULL)
    {
        certBasket      *pCurrent = pTempCB;
        pTempCB = pTempCB->pNext;
        if (pCurrent->pCert)
            TREE_DeleteTreeItem( (TreeItem*) pCurrent->pCert);
        FREE (pCurrent);
    }

    pTempCB = pRoot;
    while (pTempCB != NULL)
    {
        certBasket      *pCurrent = pTempCB;
        pTempCB = pTempCB->pNext;
        if (pCurrent->pCert)
            TREE_DeleteTreeItem( (TreeItem*) pCurrent->pCert);
        FREE (pCurrent);
    }

    return status;
}

#endif

/*-----------------------------------------------------------------*/

#if !(defined(__DISABLE_MOCANA_KEY_GENERATION__)) && !(defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__))
MOC_EXTERN sbyte4
CA_MGMT_extractPublicKeyInfo(ubyte *pCertificate, ubyte4 certificateLen,
                             ubyte** ppRetKeyBlob, ubyte4 *pRetKeyBlobLen)
{
    MSTATUS         status = OK;
    MemFile         mf;
    CStream         cs;
    ASN1_ITEMPTR    pCertRoot   = NULL;
    hwAccelDescr    hwAccelCtx;
    AsymmetricKey   pubKey;

    MOC_UNUSED(hwAccelCtx);
    CRYPTO_initAsymmetricKey(&pubKey);

    if (OK > (status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    /* Input parameter check */
    if (NULL == pCertificate || NULL == ppRetKeyBlob || NULL == pRetKeyBlobLen)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLen)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    MF_attach(&mf, certificateLen, pCertificate);
    CS_AttachMemFile(&cs, &mf);

    if (OK > (status = ASN1_Parse(cs, &pCertRoot)))
        goto exit;

    if (NULL == pCertRoot)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(hwAccelCtx) pCertRoot, cs, &pubKey)))
    {
        goto exit;
    }

    if (OK > (status = CA_MGMT_makeKeyBlobEx(&pubKey, ppRetKeyBlob, pRetKeyBlobLen)))
        goto exit;

exit:
    CRYPTO_uninitAsymmetricKey(&pubKey, NULL);

    if (pCertRoot)
        TREE_DeleteTreeItem((TreeItem *) pCertRoot);

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;
}

/*-----------------------------------------------------------------*/

MOC_EXTERN sbyte4
CA_MGMT_extractSignature(ubyte* pCertificate, ubyte4 certificateLen,
                         ubyte** ppSignature, ubyte4* pSignatureLen)
{

    MSTATUS         status      = OK;
    MemFile         mf;
    CStream         cs;
    ASN1_ITEMPTR    pCertRoot   = NULL;
    const ubyte*    buffer      = NULL;
    ASN1_ITEMPTR    pSignature  = NULL;
    ubyte*          pTemp       = NULL;

    /* Input parameter check */
    if ((NULL == pCertificate) ||(NULL == ppSignature) || (NULL == pSignatureLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLen)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    MF_attach(&mf, certificateLen, pCertificate);
    CS_AttachMemFile(&cs, &mf);

    if (OK > (status = ASN1_Parse(cs, &pCertRoot)))
        goto exit;

    if (NULL == pCertRoot)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* get the signature */
    if (OK > ( status = CERT_getSignatureItem(pCertRoot, cs, &pSignature)))
        goto exit;

    /* access the buffer */
    buffer = CS_memaccess( cs, (/*FSL*/sbyte4)pSignature->dataOffset, (/*FSL*/sbyte4)pSignature->length);
    if (NULL == buffer)
    {
        status = ERR_MEM_;
        goto exit;
    }

    /* Copy Signature */
    if (NULL == (pTemp = MALLOC( pSignature->length)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pTemp, buffer, pSignature->length);

    *pSignatureLen = pSignature->length;
    *ppSignature   = pTemp;
    pTemp          = NULL;

exit:
    if (pCertRoot)
        TREE_DeleteTreeItem((TreeItem *) pCertRoot);

    return status;

}

/*-----------------------------------------------------------------*/

MOC_EXTERN sbyte4
CA_MGMT_extractBasicConstraint(ubyte* pCertificate, ubyte4 certificateLen,
                               intBoolean* pIsCritical, certExtensions* pCertExtensions)
{

    MSTATUS         status      = OK;
    MemFile         mf;
    CStream         cs;
    ASN1_ITEMPTR    pCertRoot   = NULL;
    ASN1_ITEM*      pExtensions = NULL;
    ASN1_ITEM*      pExtension  = NULL;
    ASN1_ITEM*      pExtPart    = NULL;

    /* Input parameter check */
    if ((NULL == pCertificate) || (NULL == pCertExtensions))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLen)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    /* Explicitely MEMSET pCertExtension to 0x00 */
    if (OK > (status = MOC_MEMSET((ubyte *)pCertExtensions, 0x00, sizeof(certExtensions))))
        goto exit;

    /* Initialize to negative as non-negative including 0 is a valid path Len */
    pCertExtensions->certPathLen = -1;

    MF_attach(&mf, certificateLen, pCertificate);
    CS_AttachMemFile(&cs, &mf);

    if (OK > (status = ASN1_Parse(cs, &pCertRoot)))
        goto exit;

    if (NULL == pCertRoot)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* extract extensions */
    if (OK > (status = CERT_getCertificateExtensions(pCertRoot, &pExtensions)))
        goto exit;

    if (NULL == pExtensions)
    {
        status = ERR_CERT_BASIC_CONSTRAINT_EXTENSION_NOT_FOUND;
        goto exit;
    }

    if (OK > (status = CERT_getCertExtension(pExtensions, cs, basicConstraints_OID,
                                             pIsCritical, &pExtension)))
    {
        status = ERR_CERT_BASIC_CONSTRAINT_EXTENSION_NOT_FOUND;
        goto exit;
    }

    if (NULL == pExtension)
    {
        status = ERR_CERT_BASIC_CONSTRAINT_EXTENSION_NOT_FOUND;
        goto exit;
    }

    /* API returns -1 for isCritical and hence the conversion */
    if (-1 == *pIsCritical)
        *pIsCritical = TRUE;

    pCertExtensions->hasBasicConstraints = TRUE;

    /* BasicConstraintsSyntax ::= SEQUENCE {
                            cA                 BOOLEAN DEFAULT FALSE,
                            pathLenConstraint  INTEGER(0..MAX) OPTIONAL }*/

    if ((pExtension->id & CLASS_MASK) != UNIVERSAL ||
            pExtension->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if (0 == pExtension->length)
    {
        /* Enter default values as payload is null */
        pCertExtensions->isCA = FALSE;
        pCertExtensions->certPathLen = -1;
        goto exit;
    }

    /* verify that it is for a CA */
    if (NULL == (pExtPart = ASN1_FIRST_CHILD( pExtension)))
    {
        status = ERR_CERT_INVALID_CERT_POLICY; /* cA  BOOLEAN DEFAULT FALSE */
        goto exit;
    }

    if ((pExtPart->id & CLASS_MASK) != UNIVERSAL ||
            pExtPart->tag != BOOLEAN)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if (pExtPart->data.m_boolVal)
    {
        pCertExtensions->isCA = TRUE;
    }
    else
        pCertExtensions->isCA = FALSE;

    /* verify the maximum chain length if there */
    pExtPart = ASN1_NEXT_SIBLING( pExtPart);
    if (pExtPart)
    {
        if ( (pExtPart->id & CLASS_MASK) != UNIVERSAL ||
                pExtPart->tag != INTEGER)
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }

        pCertExtensions->certPathLen = pExtPart->data.m_intVal;
    }

exit:
    if (pCertRoot)
        TREE_DeleteTreeItem((TreeItem *) pCertRoot);

    return status;
}

/*-----------------------------------------------------------------*/

MOC_EXTERN MSTATUS
CA_MGMT_getCertSignAlgoType(ubyte *pCertificate,
                            ubyte4 certificateLen, ubyte4* pHashType, ubyte4* pPubKeyType)
{
    MSTATUS         status      = OK;
    MemFile         mf;
    CStream         cs;
    ASN1_ITEMPTR    pCertRoot   = NULL;
    ASN1_ITEMPTR    pItem       = NULL;
    ASN1_ITEMPTR    pSeqAlgoId  = NULL;

    /* Input parameter check */
    if ((NULL == pCertificate) || (NULL == pHashType) || (NULL == pPubKeyType))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == certificateLen)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    MF_attach(&mf, certificateLen, pCertificate);
    CS_AttachMemFile(&cs, &mf);

    if (OK > (status = ASN1_Parse(cs, &pCertRoot)))
        goto exit;

    if (NULL == pCertRoot)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* get the algorithm identifier */
    if ( NULL == (pItem = ASN1_FIRST_CHILD(pCertRoot)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* algo id is the second child of signed */
    if (OK > (status = ASN1_GetNthChild(pItem, 2, &pSeqAlgoId)))
    {
        goto exit;
    }

    if ( OK > (status = CERT_getCertSignAlgoType(pSeqAlgoId, cs, pHashType, pPubKeyType)))
    {
        goto exit;
    }

exit:
    if (pCertRoot)
        TREE_DeleteTreeItem((TreeItem *) pCertRoot);

    return status;
}

/*-----------------------------------------------------------------*/

MOC_EXTERN sbyte4
CA_MGMT_verifySignature(const ubyte* pIssuerCertBlob,
                        ubyte4 issuerCertBlobLen, ubyte* pCertificate, ubyte4 certLen)
{
    MSTATUS status = OK;
    MemFile mf;
    CStream cs;
    ASN1_ITEMPTR    pCertRoot  = NULL;
    AsymmetricKey   derivedKey;
    hwAccelDescr    hwAccelCtx;

    MOC_UNUSED(hwAccelCtx);
    CRYPTO_initAsymmetricKey(&derivedKey);

    if (OK > (status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    MF_attach(&mf, certLen, pCertificate);
    CS_AttachMemFile(&cs, &mf);

    if (OK > (status = ASN1_Parse(cs, &pCertRoot)))
        goto exit;

    if (NULL == pCertRoot)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* Convert keyblob to Asymmetric key */
    if (OK > (status = CA_MGMT_extractKeyBlobEx(pIssuerCertBlob, issuerCertBlobLen, &derivedKey)))
        goto exit;

    if (OK > (status = CERT_verifySignature(MOC_RSA(hwAccelCtx) pCertRoot, cs, &derivedKey)))
        goto exit;

exit:
    if (pCertRoot)
        TREE_DeleteTreeItem((TreeItem *) pCertRoot);
         CRYPTO_uninitAsymmetricKey(&derivedKey, NULL);

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;
}
#endif 

/*-----------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_CA_MGMT_UTILS__
MOC_EXTERN sbyte4
CA_MGMT_certDistinguishedNameCompare(certDistinguishedName *pNameInfo1,
                                     certDistinguishedName *pNameInfo2,
                                     intBoolean *pResult)
{
    relativeDN* pRelativeDN1 = NULL;
    relativeDN* pRelativeDN2 = NULL;
    ubyte4      i;
    MSTATUS     status = OK;

    if ((NULL == pResult) || (NULL == pNameInfo1) || (NULL == pNameInfo2))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pResult = FALSE;

   if (pNameInfo1->dnCount != pNameInfo2->dnCount)
   {
       goto exit;
   }
   else
   {
       pRelativeDN1 = pNameInfo1->pDistinguishedName;
       pRelativeDN2 = pNameInfo2->pDistinguishedName;

       for (i = 0; i < pNameInfo1->dnCount; i++)
       {
            nameAttr *pAttr1;
            nameAttr *pAttr2;
            ubyte4 j;

            if ((pRelativeDN1+i)->nameAttrCount != (pRelativeDN2+i)->nameAttrCount)
                goto exit;

            pAttr1 = (pRelativeDN1+i)->pNameAttr;
            pAttr2 = (pRelativeDN2+i)->pNameAttr;

            for (j = 0; j < (pRelativeDN1+i)->nameAttrCount; j++)
            {
                sbyte4 result;
                /* Compare the length of the two attributes */
                if (OK > (status = MOC_MEMCMP(&(pAttr1+j)->oid[0], &(pAttr2+j)->oid[0], sizeof(ubyte), &result)))
                    goto exit;

                if (OK != result)
                    goto exit;

                /* Now compare the oid values */
                if (OK > (status = MOC_MEMCMP(&(pAttr1+j)->oid[1], &(pAttr2+j)->oid[1], (pAttr1+j)->oid[0], &result)))
                    goto exit;

                if (OK != result)
                      goto exit;

                if ((pAttr1+j)->type != (pAttr2+j)->type)
                    goto exit;

                if ((pAttr1+j)->valueLen != (pAttr2+j)->valueLen)
                    goto exit;

                if (OK > (status = MOC_MEMCMP((pAttr1+j)->value, (pAttr2+j)->value, (pAttr1+j)->valueLen, &result)))
                    goto exit;

                if (result != 0)
                    goto exit;
            }
       }

       *pResult = TRUE;
   }

exit:
    return status;

} /* CA_MGMT_certDistinguishedNameCompare */
#endif /* __ENABLE_MOCANA_CA_MGMT_UTILS__ */
