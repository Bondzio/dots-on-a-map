/* Version: mss_v6_3 */
/*
 * pkcs10.c
 *
 * PKCS #10
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file pkcs10.c Mocana certificate store factory.
This file contains PKCS10 functions.

\since 2.02
\version 2.02 and later

! Flags
To use this file's functions, the following flag must be defined:
- $__ENABLE_MOCANA_PKCS10__$

! External Functions
This file contains the following public ($extern$) functions:
- PKCS10_GenerateSelfSignedCert
- PKCS10_generateCSR
- PKCS10_generateCSREx
*/

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#ifdef __ENABLE_MOCANA_PKCS10__

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/random.h"
#include "../common/vlong.h"
#include "../common/utils.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../crypto/sha1.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../crypto/rsa.h"
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../asn1/derencoder.h"
#include "../crypto/ca_mgmt.h"
#include "../crypto/asn1cert.h"
#include "../crypto/md5.h"
#include "../crypto/base64.h"
#include "../crypto/crypto.h"
#include "../common/memfile.h"
#include "../crypto/asn1cert.h"
#include "../crypto/pkcs10.h"
#include "../harness/harness.h"


/*------------------------------------------------------------------*/

#ifndef CSR_LINE_LENGTH
#define CSR_LINE_LENGTH     65
#endif

#define BEGIN_CSR_BLOCK     "-----BEGIN CERTIFICATE REQUEST-----\x0d\x0a"
#define END_CSR_BLOCK       "-----END CERTIFICATE REQUEST-----\x0d\x0a"


/*------------------------------------------------------------------*/

static MSTATUS
PKCS10_AddRequestAttributes(DER_ITEMPTR pCertificationRequestInfo,
                     const requestAttributes* pReqAttrs)
{
    MSTATUS         status;
    DER_ITEMPTR     pReqAttrsItem, pTempItem;

    /* add tag and optionally attributes */
    if  (OK > ( status = DER_AddTag( pCertificationRequestInfo, 0, &pReqAttrsItem)))
        goto exit;

    /* NOTE: in v1.6, SetOf Attributes is implicit.
     * in v1.7 it seems to become explicit. we are using v1.6 here. */
    /* add OPTIONAL attributes */
    if (pReqAttrs)
    {
        /* add challengePassword if present */
        if (pReqAttrs->challengePwdLength >0)
        {
            if  (OK > ( status = DER_AddSequence( pReqAttrsItem, &pTempItem)))
            goto exit;

            if  (OK > ( status = DER_AddOID( pTempItem, pkcs9_challengePassword_OID, NULL)))
            goto exit;

            if  (OK > ( status = DER_AddSet( pTempItem, &pTempItem)))
            goto exit;

            if  (OK > ( status = DER_AddItem( pTempItem, PRINTABLESTRING, pReqAttrs->challengePwdLength, (ubyte *)pReqAttrs->pChallengePwd, NULL)))
            goto exit;

        }

        /* add extensions if present */
        if (pReqAttrs->pExtensions)
        {
            /*
            extensionRequest ATTRIBUTE ::= {
                WITH SYNTAX ExtensionRequest
                SINGLE VALUE TRUE
                ID pkcs-9-at-extensionRequest
            }

            ExtensionRequest ::= Extensions
            */
            if (OK > (status = DER_AddSequence(pReqAttrsItem, &pTempItem)))
                goto exit;
            if (OK > (status = DER_AddOID(pTempItem, pkcs9_extensionRequest_OID, NULL)))
                goto exit;
            if (OK > (status = DER_AddSet(pTempItem, &pTempItem)))
                goto exit;
            ASN1CERT_AddExtensions(pTempItem, pReqAttrs->pExtensions, NULL);
        }
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_GenerateCertReq(AsymmetricKey* pPubKey, certDistinguishedName *pCertInfo,
                       ubyte** ppCertReq, ubyte4* pCertReqLength)
{
    return PKCS10_GenerateCertReqEx2(pPubKey, pCertInfo, md5withRSAEncryption_OID,
                                    NULL, ppCertReq, pCertReqLength);
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_GenerateCertReqEx(AsymmetricKey* pPubKey, certDistinguishedName *pCertInfo,
                       requestAttributes *pReqAttrs, ubyte** ppCertReq, ubyte4* pCertReqLength)
{
    return PKCS10_GenerateCertReqEx2(pPubKey, pCertInfo, md5withRSAEncryption_OID,
                                        pReqAttrs, ppCertReq, pCertReqLength);
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_GenerateCertReqEx2(AsymmetricKey* pPubKey, certDistinguishedName *pCertInfo,
                        const ubyte* signAlgoOID, requestAttributes *pReqAttrs,
                        ubyte** ppCertReq, ubyte4* pCertReqLength)
{
    hwAccelDescr    hwAccelCtx;
    DER_ITEMPTR     pCertificationRequest = 0;
    DER_ITEMPTR     pCertificationRequestInfo;
    ubyte           copyData[MAX_DER_STORAGE];
    MSTATUS         status = OK;

    if (OK > ( status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        goto no_cleanup;

    if (OK > ( status = DER_AddSequence( NULL, &pCertificationRequest)))
        goto exit;

    if (OK > ( status = DER_AddSequence( pCertificationRequest, &pCertificationRequestInfo)))
        goto exit;

    copyData[0] = 0;
    if ( OK > ( status = DER_AddItemCopyData( pCertificationRequestInfo, INTEGER, 1, copyData, NULL)))
        goto exit;

    if ( OK > ( status = ASN1CERT_StoreDistinguishedName( pCertificationRequestInfo, pCertInfo)))
        goto exit;

    if ( OK > ( status = ASN1CERT_storePublicKeyInfo( pPubKey, pCertificationRequestInfo)))
        goto exit;

    if (OK > ( status = PKCS10_AddRequestAttributes( pCertificationRequestInfo, pReqAttrs)))
        goto exit;


    /* add signature now */
    if ( OK > ( status = ASN1CERT_sign(MOC_RSA_HASH(hwAccelCtx) pCertificationRequest, pPubKey,
                                        signAlgoOID, RANDOM_rngFun, g_pRandomContext,
                                        ppCertReq, pCertReqLength)))
    {
        goto exit;
    }

exit:
    if (pCertificationRequest)
    {
        TREE_DeleteTreeItem( (TreeItem*) pCertificationRequest);
    }

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

no_cleanup:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_GenerateCertReqFromASN1( AsymmetricKey* pPubKey, const ubyte* pASN1Name,
                        ubyte4 asn1NameLen, ubyte** ppCertReq, ubyte4* pCertReqLength)
{
    hwAccelDescr    hwAccelCtx;
    DER_ITEMPTR     pCertificationRequest = 0;
    DER_ITEMPTR     pCertificationRequestInfo;
    ubyte           copyData[MAX_DER_STORAGE];
    MSTATUS         status = OK;

    if (OK > ( status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        goto no_cleanup;

    if (OK > ( status = DER_AddSequence( NULL, &pCertificationRequest)))
        goto exit;

    if (OK > ( status = DER_AddSequence( pCertificationRequest, &pCertificationRequestInfo)))
        goto exit;

    copyData[0] = 0;
    if ( OK > ( status = DER_AddItemCopyData( pCertificationRequestInfo, INTEGER, 1, copyData, NULL)))
        goto exit;

    /* subject -- add the whole ASN1 sequence as is!*/
    if ( OK > ( status = DER_AddItem( pCertificationRequestInfo, (CONSTRUCTED|SEQUENCE), asn1NameLen,
                                        pASN1Name, NULL)))
    {
        goto exit;
    }

    if ( OK > ( status = ASN1CERT_storePublicKeyInfo( pPubKey, pCertificationRequestInfo)))
        goto exit;

    /* add tag but no attributes */
    if  (OK > ( status = DER_AddTag( pCertificationRequestInfo, 0, NULL)))
        goto exit;

    /* add signature now */
    if ( OK > ( status = ASN1CERT_sign(MOC_RSA_HASH(hwAccelCtx) pCertificationRequest, pPubKey,
                                        md5withRSAEncryption_OID, RANDOM_rngFun, g_pRandomContext,
                                        ppCertReq, pCertReqLength)))
    {
        goto exit;
    }

exit:

    if ( pCertificationRequest)
    {
        TREE_DeleteTreeItem( (TreeItem*) pCertificationRequest);
    }

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

no_cleanup:
    return status;
}

/*------------------------------------------------------------------*/

/* Generate a self-signed X509v3 certificate based on the specified parameters.
This function generates a self-signed X509v3 certificate based on the specified
key pair, certificate subject name, certificate extensions, and signing algorithm.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined:
- $__ENABLE_MOCANA_PKCS10__$

\param hwAccelCtx               For future use.
\param pKey                     Key to use when signing the certificate. The
key's public key is added to the certificate and associated with the certificate's
subject.
\param certDistinguishedName    Certificate distinguished name.
\param pSubjectInfo             Certificate's subject name.
\param pExtensions              Certificate's extensions.
\param signAlgo         Signature algorithm (containing hash and key) to use to sign the certificate.\n
\n
The following enumerated values (defined in crypto.h) are supported:\n
\n
&bull; $ht_md2$\n
&bull; $ht_md4$\n
&bull; $ht_md5$\n
&bull; $ht_sha1$\n
&bull; $ht_sha256$\n
&bull; $ht_sha384$\n
&bull; $ht_sha512$\n
&bull; $ht_sha224

\param rngFun                   Pointer to RNG function. (To ensure FIPS-certified
operations, this should point to the Mocana random number generator, RANDOM_rngFun.)
\param rngFunArg                Pointer to RNG function's arguments.
\param ppSelfSignedCert         On return, pointer to self signed certificate.
\param pSelfSignedCertLength    On return, pointer to number of bytes in resultant
self signed certificate ($ppSelfSignedCert$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.
*/
extern MSTATUS PKCS10_GenerateSelfSignedCert(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) AsymmetricKey* pKey,
                                             const certDistinguishedName *pSubjectInfo,
                                             const certExtensions* pExtensions,
                                             ubyte signAlgo, RNGFun rngFun, void* rngFunArg,
                                             ubyte** ppSelfSignedCert, ubyte4* pSelfSignedCertLength)

{

    return ASN1CERT_generateCertificate(MOC_RSA_HASH(hwAccelCtx) pKey, pSubjectInfo,
                                        pKey, pSubjectInfo, signAlgo, pExtensions,
                                        rngFun, rngFunArg,
                                        ppSelfSignedCert, pSelfSignedCertLength);
}

/*------------------------------------------------------------------*/

static MSTATUS
PKCS10_generateCSRAux(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength,
            ubyte **ppRetLineCsr, ubyte4 *pRetLineCsrLength,
            certDistinguishedName *pCertInfo, requestAttributes *reqAttrs)
{
    AsymmetricKey pubKey;
    ubyte*  pCertReq        = NULL;
    ubyte4  certReqLen;
    MSTATUS status = OK;

    /* create temporary key context */
    if (OK > ( status = CRYPTO_initAsymmetricKey(&pubKey)))
        return status;

    /* extract keys from blob */
    if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &pubKey)))  /* !!! move to common area */
    {
        goto exit;
    }

    /* build cert req */
    if ( OK > (status = PKCS10_GenerateCertReqEx(&pubKey, pCertInfo, reqAttrs, &pCertReq, &certReqLen)))
        goto exit;

    status = BASE64_encodeMessage(pCertReq, certReqLen, ppRetLineCsr, pRetLineCsrLength);

exit:
    if (pCertReq)
        FREE(pCertReq);

    CRYPTO_uninitAsymmetricKey(&pubKey, NULL);

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS10_generateCSRASN1Aux(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength,
            ubyte **ppRetLineCsr, ubyte4 *pRetLineCsrLength,
            const ubyte* pASN1Name, ubyte4 asn1NameLen)
{
    AsymmetricKey pubKey;
    ubyte*  pCertReq        = NULL;
    ubyte4  certReqLen;
    MSTATUS status = OK;

    if (OK > ( status = CRYPTO_initAsymmetricKey( &pubKey)))
        return status;

    /* extract keys from blob */
    if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &pubKey)))    /* !!! move to common area */
        goto exit;

    /* build cert req */
    if ( OK > (status = PKCS10_GenerateCertReqFromASN1(&pubKey, pASN1Name, asn1NameLen,
                                                        &pCertReq, &certReqLen)))
    {
        goto exit;
    }

    status = BASE64_encodeMessage(pCertReq, certReqLen, ppRetLineCsr, pRetLineCsrLength);

exit:
    if (pCertReq)
        FREE(pCertReq);

    CRYPTO_uninitAsymmetricKey(&pubKey, NULL);

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS10_breakIntoLines(ubyte* pLineCsr, ubyte4 lineCsrLength,
                        ubyte **ppRetCsr, ubyte4 *p_retCsrLength)
{
    ubyte*  pBlockCSR = NULL;
    ubyte*  pTempLineCsr;
    ubyte4  numLines;

    /* break the data up into (CSR_LINE_LENGTH) sized blocks */
    numLines     = ((lineCsrLength + (CSR_LINE_LENGTH - 1)) / CSR_LINE_LENGTH);
    pTempLineCsr = pLineCsr;

    /* calculate the new block length */
    *p_retCsrLength = (sizeof(BEGIN_CSR_BLOCK) - 1) + lineCsrLength + numLines + numLines + (sizeof(END_CSR_BLOCK) - 1);

    /* allocate the new csr block */
    if (NULL == (*ppRetCsr = pBlockCSR = MALLOC(*p_retCsrLength)))
    {
        return ERR_MEM_ALLOC_FAIL;
    }

    /* copy the start of block identifier */
    MOC_MEMCPY(pBlockCSR, (const ubyte *)BEGIN_CSR_BLOCK, (sizeof(BEGIN_CSR_BLOCK) - 1));
    pBlockCSR += (sizeof(BEGIN_CSR_BLOCK) - 1);

    /* copy contiguous blocks of data */
    while (1 < numLines)
    {
        MOC_MEMCPY(pBlockCSR, pTempLineCsr, CSR_LINE_LENGTH);
        pBlockCSR[CSR_LINE_LENGTH] = CR;
        pBlockCSR[CSR_LINE_LENGTH + 1] = LF;

        pBlockCSR += CSR_LINE_LENGTH + 2;
        pTempLineCsr += CSR_LINE_LENGTH;
        lineCsrLength -= CSR_LINE_LENGTH;

        numLines--;
    }

    /* copy any remaining bytes */
    if (lineCsrLength)
    {
        MOC_MEMCPY(pBlockCSR, pTempLineCsr, lineCsrLength);
        pBlockCSR += lineCsrLength;

        *pBlockCSR = CR; pBlockCSR++;
        *pBlockCSR = LF; pBlockCSR++;
    }

    /* copy the end of block identifier */
    MOC_MEMCPY(pBlockCSR, (const ubyte *)END_CSR_BLOCK, (sizeof(END_CSR_BLOCK) - 1));

    return OK;
}


/*------------------------------------------------------------------*/
/* Generate outbound CSR base 64-encoded certificate.
This function generates an outbound CSR (certificate signing request) base
64-encoded certificate based on the specified input parameters.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined:
- $__ENABLE_MOCANA_PKCS10__$

\param hwAccelCtx       For future use.
\param pKeyBlob         Mocana-formatted keyblob.
\param keyBlobLength    Number of bytes in Mocana-formatted keyblob ($pKeyBlob$).
\param ppRetCsr         On return, pointer to CSR in base 64-encoded format.
\param p_retCsrLength   On return, number of bytes in CSR ($ppRetCsr$).
\param pCertInfo        Pointer to certificate subject or entity for which a
certificate is requested.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.
*/
extern MSTATUS
PKCS10_generateCSR(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength,
                   ubyte **ppRetCsr, ubyte4 *p_retCsrLength,
                   certDistinguishedName *pCertInfo)
{
    return PKCS10_generateCSREx(MOC_RSA(hwAccelCtx) pKeyBlob, keyBlobLength, ppRetCsr, p_retCsrLength,
                                pCertInfo, NULL);
}

/* Generate outbound CSR base 64-encoded certificate with PKCS&nbsp;\#10 extensions.
This function generates an outbound CSR (certificate signing request) base
64-encoded certificate with PKCS&nbsp;\#10 extensions based on the specified input parameters.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined:
- $__ENABLE_MOCANA_PKCS10__$

\param hwAccelCtx       For future use.
\param pKeyBlob         Mocana-formatted keyblob.
\param keyBlobLength    Number of bytes in Mocana-formatted keyblob ($pKeyBlob$).
\param ppRetCsr         On return, pointer to CSR in base 64-encoded format.
\param p_retCsrLength   On return, number of bytes in CSR ($ppRetCsr$).
\param pCertInfo        Pointer to certificate subject or entity for which a
certificate is requested.
\param reqAttrs         Pointer to PKCS&nbsp;\#10 extensions (as defined in PKCS&nbsp;\#9); see pkcs10.h for structure definition.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.
*/
extern MSTATUS
PKCS10_generateCSREx(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength,
                   ubyte **ppRetCsr, ubyte4 *p_retCsrLength,
                   certDistinguishedName *pCertInfo, requestAttributes *reqAttrs)
{
    ubyte*  pLineCsr  = NULL;
    ubyte4  lineCsrLength;
    MSTATUS status;

    /* generate a CSR line of data */
    if (OK > (status = PKCS10_generateCSRAux(MOC_RSA(hwAccelCtx) pKeyBlob, keyBlobLength, &pLineCsr, &lineCsrLength, pCertInfo, reqAttrs)))
        goto exit;

    if (OK > ( status = PKCS10_breakIntoLines(pLineCsr, lineCsrLength, ppRetCsr, p_retCsrLength)))
        goto exit;

exit:
    if (NULL != pLineCsr)
        FREE(pLineCsr);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_generateCSRFromASN1(MOC_RSA(hwAccelDescr hwAccelCtx) ubyte *pKeyBlob, ubyte4 keyBlobLength,
                           ubyte **ppRetCsr, ubyte4 *p_retCsrLength,
                           const ubyte* pASN1Name, ubyte4 asn1NameLen)
{
    ubyte*  pLineCsr  = NULL;
    ubyte4  lineCsrLength;
    MSTATUS status;

    /* generate a CSR line of data */
    if (OK > (status = PKCS10_generateCSRASN1Aux(MOC_RSA(hwAccelCtx) pKeyBlob, keyBlobLength, &pLineCsr, &lineCsrLength,
                                                    pASN1Name, asn1NameLen)))
    {
        goto exit;
    }

    if (OK > ( status = PKCS10_breakIntoLines( pLineCsr, lineCsrLength, ppRetCsr, p_retCsrLength)))
        goto exit;

exit:
    if (NULL != pLineCsr)
        FREE(pLineCsr);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_freeCSR(ubyte **ppCsrToFree)
{
    return BASE64_freeMessage(ppCsrToFree);
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_decodeSignedCertificate(ubyte* pCertBase64,    ubyte4 certLengthBase64,
                               ubyte** ppCertificate, ubyte4 *pRetCertificateLen)
{
    return (MSTATUS)CA_MGMT_decodeCertificate(pCertBase64, certLengthBase64,
                                              ppCertificate, pRetCertificateLen);
}


/*------------------------------------------------------------------*/

extern MSTATUS
PKCS10_freeDecodedSignedCertificate(ubyte** ppFreeCertificate)
{
    if (NULL != ppFreeCertificate)
    {
        if (NULL != *ppFreeCertificate)
        {
            FREE(*ppFreeCertificate);
            *ppFreeCertificate = NULL;
        }
    }

    return OK;
}

#endif /* __ENABLE_MOCANA_PKCS10__*/

