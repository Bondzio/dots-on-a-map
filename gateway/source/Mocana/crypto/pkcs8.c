/* Version: mss_v6_3 */
/*
 * pkcs8.c
 *
 * PKCS #8
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#if (defined(__ENABLE_MOCANA_PKCS8__) && (defined(__ENABLE_MOCANA_PEM_CONVERSION__) || defined(__ENABLE_MOCANA_DER_CONVERSION__)))

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
#include "../crypto/rsa.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../asn1/derencoder.h"
#include "../crypto/ca_mgmt.h"
#include "../crypto/base64.h"
#include "../crypto/pkcs_key.h"
#include "../crypto/pkcs8.h"


/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_PEM_CONVERSION__))
static MSTATUS
fetchLine(const ubyte *pSrc,  ubyte4 *pSrcIndex, const ubyte4 srcLength,
          ubyte *pDest, ubyte4 *pDestIndex)
{
    /* this is here for now... we will want to use the version in crypto/ca_mgmt.c */
    MSTATUS status = OK;

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

    return status;

} /* fetchLine */
#endif /* (defined(__ENABLE_MOCANA_PEM_CONVERSION__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_PEM_CONVERSION__))
extern MSTATUS
PKCS8_decodePrivateKeyPEM(const ubyte* pFilePemPkcs8, ubyte4 fileSizePemPkcs8,
                          ubyte** ppRsaKeyBlob, ubyte4 *pRsaKeyBlobLength)
{
    /* decode a PKCS #8 private key file */
    AsymmetricKey key;
    ubyte*  pBase64Mesg = NULL;
    ubyte4  srcIndex    = 0;
    ubyte4  destIndex   = 0;
    ubyte*  pDecodeFile = NULL;
    ubyte4  decodedLength;
    MSTATUS status;
    hwAccelDescr hwAccelCtx;

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_SSL, &hwAccelCtx)))
        return status;

    if ( OK >( status = CRYPTO_initAsymmetricKey( &key)))
        return status;

    /* alloc temp memory for base64 decode buffer */
    if (NULL == (pBase64Mesg = MALLOC(fileSizePemPkcs8)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* strip our line feeds and comments lines from base64 text  */
    while (fileSizePemPkcs8 > srcIndex)
    {
        if (OK > (status = fetchLine(pFilePemPkcs8, &srcIndex, fileSizePemPkcs8, pBase64Mesg, &destIndex)))
            goto exit;
    }

    /* decode a contiguous base64 block of text */
    if (OK > (status = BASE64_decodeMessage((ubyte *)pBase64Mesg, destIndex, &pDecodeFile, &decodedLength)))
        goto exit;

    /* extract RSA key from DER private key PKCS8 file */
    if (OK > (status = PKCS_getPKCS8Key(MOC_RSA(hwAccelCtx) pDecodeFile, decodedLength, &key)))
        goto exit;

    /* return a Mocana RSA key blob */
    status = CA_MGMT_makeKeyBlobEx(&key, ppRsaKeyBlob, pRsaKeyBlobLength);

exit:
    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_SSL, &hwAccelCtx);

    if (NULL != pBase64Mesg)
        FREE(pBase64Mesg);

    if (NULL != pDecodeFile)
        FREE(pDecodeFile);

    CRYPTO_uninitAsymmetricKey(&key, NULL);

    return status;

} /* PKCS8_decodePrivateKeyPEM */
#endif /* (defined(__ENABLE_MOCANA_PEM_CONVERSION__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DER_CONVERSION__))
extern MSTATUS
PKCS8_decodePrivateKeyDER(const ubyte* pFileDerPkcs8, ubyte4 fileSizeDerPkcs8,
                          ubyte** ppRsaKeyBlob, ubyte4 *pRsaKeyBlobLength)
{
    /* decode a PKCS #8 private key file */
    AsymmetricKey key;
    MSTATUS status;
    hwAccelDescr hwAccelCtx;

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_SSL, &hwAccelCtx)))
        return status;

    if (OK > (status = CRYPTO_initAsymmetricKey(&key)))
        return status;

    /* extract RSA key from DER private key PKCS8 file */
    if (OK > (status = PKCS_getPKCS8Key(MOC_RSA(hwAccelCtx) pFileDerPkcs8, fileSizeDerPkcs8, &key)))
        goto exit;

    /* return a Mocana RSA key blob */
    status = CA_MGMT_makeKeyBlobEx(&key, ppRsaKeyBlob, pRsaKeyBlobLength);

exit:
    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_SSL, &hwAccelCtx);
    CRYPTO_uninitAsymmetricKey(&key, NULL);

    return status;

} /* PKCS8_decodePrivateKeyDER */
#endif /* (defined(__ENABLE_MOCANA_DER_CONVERSION__)) */

#endif /* __ENABLE_MOCANA_PKCS8__*/

