/* Version: mss_v6_3 */
/*
 * pubcrypto.c
 *
 * General Public Crypto Operations
 *
 * Copyright Mocana Corp 2006-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"
#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/memory_debug.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#if (defined(__ENABLE_MOCANA_DSA__))
#include "../crypto/dsa.h"
#endif
#include "../crypto/rsa.h"
#if (defined(__ENABLE_MOCANA_ECC__))
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../asn1/oiddefs.h"
#endif
#include "../crypto/pubcrypto.h"

#include "../crypto/ca_mgmt.h"

/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_initAsymmetricKey(AsymmetricKey* pKey)
{
    MSTATUS status = ERR_NULL_POINTER;

    if (NULL == pKey)
        goto exit;

    status = MOC_MEMSET((ubyte *)pKey, 0x00, sizeof(AsymmetricKey));
    pKey->type = akt_undefined;

exit:
    return status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
extern MSTATUS
CRYPTO_createDSAKey( AsymmetricKey* pKey, vlong** ppVlongQueue)
{
    MSTATUS status;

    if (!pKey)
    {
        return ERR_NULL_POINTER;
    }

    if (OK > ( status = CRYPTO_uninitAsymmetricKey(pKey, ppVlongQueue)))
        goto exit;

    if (OK > (status = DSA_createKey(&pKey->key.pDSA)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pKey->key.pDSA);

    pKey->type = akt_dsa;

exit:
    return status;
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_createRSAKey( AsymmetricKey* pKey, vlong** ppVlongQueue)
{
    MSTATUS status;

    if (!pKey)
    {
        return ERR_NULL_POINTER;
    }
    if (OK > ( status = CRYPTO_uninitAsymmetricKey( pKey, ppVlongQueue)))
        goto exit;

    if (OK > (status = RSA_createKey( &pKey->key.pRSA)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pKey->key.pRSA);

    pKey->type = akt_rsa;
exit:
    return status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
extern MSTATUS
CRYPTO_createECCKey( AsymmetricKey* pKey, PEllipticCurvePtr pEC,
                    vlong** ppVlongQueue)
{
    MSTATUS status;

    if (!pKey)
    {
        return ERR_NULL_POINTER;
    }

    if (OK > ( status = CRYPTO_uninitAsymmetricKey( pKey, ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = EC_newKey( pEC, &pKey->key.pECC)))
    {
        goto exit;
    }

    DEBUG_RELABEL_MEMORY(pKey->key.pECC);

    pKey->type = akt_ecc;
exit:

    return status;
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_copyAsymmetricKey(AsymmetricKey* pNew, const AsymmetricKey* pSrc)
{
    MSTATUS status;

    CRYPTO_uninitAsymmetricKey( pNew, 0);

    pNew->type = pSrc->type;

    switch (pSrc->type)
    {
        case akt_rsa:
        {
            status = RSA_cloneKey( &pNew->key.pRSA, pSrc->key.pRSA, 0);
            break;
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            status = EC_cloneKey( &pNew->key.pECC, pSrc->key.pECC);
            break;
        }
#endif
#if (defined(__ENABLE_MOCANA_DSA__))
        case akt_dsa:
        {
            status = DSA_cloneKey(&pNew->key.pDSA, pSrc->key.pDSA);
            break;
        }
#endif
        case akt_undefined:
        {
            status = OK;
            break;
        }
        default:
        {
            status = ERR_INTERNAL_ERROR;
            break;
        }
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_uninitAsymmetricKey(AsymmetricKey* pKey, vlong** ppVlongQueue)
{
    MSTATUS status;

    if (!pKey)
    {
        return ERR_NULL_POINTER;
    }
    switch (pKey->type)
    {
        case akt_rsa:
        {
            status = RSA_freeKey( &pKey->key.pRSA, ppVlongQueue);
            break;
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            status = EC_deleteKey( &pKey->key.pECC);
            break;
        }
#endif
#if (defined(__ENABLE_MOCANA_DSA__))
        case akt_dsa:
        {
            status = DSA_freeKey(&pKey->key.pDSA, ppVlongQueue);
            break;
        }
#endif
        case akt_undefined:
        {
            status = OK;
            break;
        }
        default:
        {
            status = ERR_INTERNAL_ERROR;
            break;
        }
    }

    if (OK > status)
        goto exit;

    pKey->type = akt_undefined;

exit:

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_matchPublicKey(const AsymmetricKey* pKey1, const AsymmetricKey* pKey2)
{
    byteBoolean res;
    MSTATUS status;

    /* see if the public key part of the keys match */
    if (!pKey1 || !pKey2)
        return ERR_NULL_POINTER;

    if (pKey1->type != pKey2->type)
        return ERR_FALSE;

    switch (pKey1->type)
    {
    case akt_rsa:
        if (OK > (status = RSA_equalKey( pKey1->key.pRSA, pKey2->key.pRSA, &res)))
            return status;
        return (res) ? OK : ERR_FALSE;

#if (defined(__ENABLE_MOCANA_ECC__))
    case akt_ecc:
        if (OK > (status = EC_equalKey( pKey1->key.pECC, pKey2->key.pECC, &res)))
            return status;
        return (res) ? OK : ERR_FALSE;

#endif

#if (defined(__ENABLE_MOCANA_DSA__))
    case akt_dsa:
        if (OK > (status = DSA_equalKey( pKey1->key.pDSA, pKey2->key.pDSA, &res)))
            return status;
        return (res) ? OK : ERR_FALSE;
#endif

      default:
          break;

    }

    return ERR_INVALID_ARG;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_setRSAParameters(MOC_RSA(hwAccelDescr hwAccelCtx)
                        AsymmetricKey* pKey, ubyte4 exponent,
                        const ubyte* modulus, ubyte4 modulusLen,
                        const ubyte* p, ubyte4 pLen,
                        const ubyte* q, ubyte4 qLen,
                        vlong **ppVlongQueue)
{
    MSTATUS status;

    if ( !pKey || !modulus)
        return ERR_NULL_POINTER;

    if (akt_rsa != pKey->type)
    {
        /* reset everything */
        if ( OK > (status = CRYPTO_createRSAKey( pKey, ppVlongQueue)))
             return status;
    }

    if ( p && pLen && q && qLen)
    {
        return RSA_setAllKeyParameters(MOC_RSA(hwAccelCtx)
                                       pKey->key.pRSA,
                                       exponent, modulus, modulusLen,
                                       p, pLen, q, qLen, ppVlongQueue);

    }

    return RSA_setPublicKeyParameters( pKey->key.pRSA,
                                        exponent, modulus, modulusLen, ppVlongQueue);
}


/*----------------------------------------------------------------------*/
#if (defined(__ENABLE_MOCANA_DSA__))
extern MSTATUS
CRYPTO_setDSAParameters( MOC_DSA(hwAccelDescr hwAccelCtx) AsymmetricKey* pKey,
                        const ubyte* p, ubyte4 pLen,
                        const ubyte* q, ubyte4 qLen,
                        const ubyte* g, ubyte4 gLen,
                        const ubyte* y, ubyte4 yLen,
                        const ubyte* x, ubyte4 xLen,
                        vlong **ppVlongQueue)
{
    MSTATUS status;

    if ( !pKey || !q || !q || !g )
        return ERR_NULL_POINTER;

    if (!pLen || !qLen || !gLen )
        return ERR_INVALID_ARG;

    if (akt_dsa != pKey->type)
    {
        /* reset everything */
        if ( OK > (status = CRYPTO_createDSAKey( pKey, ppVlongQueue)))
             return status;
    }

    if ( x && xLen)
    {
        return DSA_setAllKeyParameters(MOC_DSA(hwAccelCtx)
                                       pKey->key.pDSA,
                                       p, pLen, q, qLen,
                                       g, gLen, x, xLen, ppVlongQueue);

    }
    else if ( y && yLen)
    {
        return DSA_setPublicKeyParameters( pKey->key.pDSA,
                                       p, pLen, q, qLen,
                                       g, gLen, y, yLen,
                                       ppVlongQueue);
    }
    return ERR_INVALID_ARG;
}

#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static PEllipticCurvePtr
CRYPTO_getEllipticCurveFromCurveId( ubyte4 curveId)
{
    switch ( curveId) /* curveId is also the oid suffix */
    {
#ifdef __ENABLE_MOCANA_ECC_P192__
    case cid_EC_P192:
        return EC_P192;
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
    case cid_EC_P224:
        return EC_P224;
#endif
#ifndef __DISABLE_MOCANA_ECC_P256__
    case cid_EC_P256:
        return EC_P256;
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
    case cid_EC_P384:
        return EC_P384;
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
    case cid_EC_P521:
        return EC_P521;
#endif
    default:
        break;
    }

    return 0;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static ubyte4
CRYPTO_getOIDSuffixOfEllipticCurve( PEllipticCurvePtr pEC)
{
    /* pointer to pointer comparison is good */
#ifdef __ENABLE_MOCANA_ECC_P192__
    if ( EC_P192 == pEC)
    {
        return cid_EC_P192;
    }
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
    if (EC_P224 == pEC)
    {
        return cid_EC_P224;
    }
#endif
#ifndef __DISABLE_MOCANA_ECC_P256__
    if ( EC_P256 == pEC)
    {
        return cid_EC_P256;
    }
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
    if (EC_P384 == pEC)
    {
        return cid_EC_P384;
    }
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
    if (EC_P521 == pEC)
    {
        return cid_EC_P521;
    }
#endif
    return 0;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
extern ubyte4
CRYPTO_getECCurveId( const AsymmetricKey* pKey)
{
    if (!pKey)
        return 0;

    if ( akt_ecc != pKey->type)
        return 0;

    return CRYPTO_getOIDSuffixOfEllipticCurve( pKey->key.pECC->pCurve);
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
extern MSTATUS
CRYPTO_setECCParameters( AsymmetricKey* pKey, ubyte4 curveId,
                                const ubyte* point, ubyte4 pointLen,
                                const ubyte* scalar, ubyte4 scalarLen,
                                vlong** ppVlongQueue)
{
    MSTATUS status;
    PEllipticCurvePtr pCurve;

    if ( !pKey || !point)
        return ERR_NULL_POINTER;

    pCurve = CRYPTO_getEllipticCurveFromCurveId( curveId);
    if ( 0 == pCurve)
        return ERR_EC_UNSUPPORTED_CURVE;

    if (akt_ecc != pKey->type || pKey->key.pECC->pCurve != pCurve)
    {
        /* reset everything */
        if ( OK > (status = CRYPTO_createECCKey( pKey, pCurve, ppVlongQueue)))
             return status;
    }

    if (scalar && scalarLen)
    {
        pKey->key.pECC->privateKey = TRUE;
    }

    return EC_setKeyParameters( pKey->key.pECC, point, pointLen,
                                    scalar, scalarLen);
}

#endif



/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
MSTATUS CRYPTO_getECCurveOID( const ECCKey* pECCKey, const ubyte** pCurveOID)
{
    if (!pECCKey)
        return ERR_NULL_POINTER;

#ifdef __ENABLE_MOCANA_ECC_P192__
    if ( EC_P192 == pECCKey->pCurve)
    {
        *pCurveOID = secp192r1_OID;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
    if ( EC_P224 == pECCKey->pCurve)
    {
        *pCurveOID = secp224r1_OID;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P256__
    if ( EC_P256 == pECCKey->pCurve)
    {
        *pCurveOID = secp256r1_OID;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
    if ( EC_P384 == pECCKey->pCurve)
    {
        *pCurveOID = secp384r1_OID;
    }
    else
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
    if ( EC_P521 == pECCKey->pCurve)
    {
        *pCurveOID = secp521r1_OID;
    }
    else
#endif
    {
        *pCurveOID = 0;
        return ERR_EC_UNSUPPORTED_CURVE;
    }

    return OK;
}
#endif

#if (defined (__ENABLE_MOCANA_HW_SECURITY_MODULE__) && defined(__ENABLE_MOCANA_TPM__))
MOC_EXTERN MSTATUS  CRYPTO_createTPMRSAKey(secModDescr* secModContext, AsymmetricKey* pKey)
{
    MSTATUS status = OK;

    return status;
}
#endif

MOC_EXTERN MSTATUS  CRYPTO_serializeIn(ubyte* pKeyBlob, ubyte4 keyBlobLen, AsymmetricKey* pKey)
{
    MSTATUS status = OK;

    if(12 > keyBlobLen)
    {
        return ERR_BAD_KEY_BLOB;
    }

    switch(pKeyBlob[11])
    {
        case akt_rsa:
#ifdef __ENABLE_MOCANA_ECC__
        case akt_ecc:
#endif
#ifdef __ENABLE_MOCANA_DSA__
        case akt_dsa:
#endif
        {
            status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLen, pKey);
            break;
        }
#if (defined (__ENABLE_MOCANA_HW_SECURITY_MODULE__) && defined(__ENABLE_MOCANA_TPM__))
        case akt_rsa_tpm:
        {
            status = SECMOD_serializeIn(pKeyBlob, keyBlobLen, pKey);
            break;
        }
#endif
        default:
        {
            status = ERR_CRYPTO_BAD_KEY_TYPE;
            break;
        }
    }

    return status;
}

MOC_EXTERN MSTATUS  CRYPTO_serializeOut(AsymmetricKey* pKey, ubyte** ppKeyBlob, ubyte4* pKeyBlobLen)
{
    MSTATUS status = OK;

    ubyte4  bufferLength;
    ubyte*  buffer = NULL;

    switch(pKey->type)
    {
        case akt_rsa:
        {
            if (OK > ( status = RSA_byteStringFromKey(MOC_RSA(hwAccelCtx)
                                        pKey->key.pRSA, 0, &bufferLength)))
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
                                        pKey->key.pRSA, buffer+12, &bufferLength)))
                goto exit;
            break;
        }
#ifdef __ENABLE_MOCANA_ECC__
        case akt_ecc:
        {

            PrimeFieldPtr pPF;
            sbyte4 pointLen;
            sbyte4 scalarLen;
            sbyte4 index;

            /* get total length */
            pPF = EC_getUnderlyingField( pKey->key.pECC->pCurve);

            if ( OK > (status = EC_getPointByteStringLen( pKey->key.pECC->pCurve, &pointLen)))
                goto exit;
            if (pKey->key.pECC->privateKey)
            {
                if ( OK > (status = PRIMEFIELD_getElementByteStringLen(pPF, &scalarLen)))
                    goto exit;
            }
            else
            {
                scalarLen = 0;
            }

            bufferLength = 12 + 4 + 4 + pointLen + 4 + scalarLen;

            if (NULL == (buffer = MALLOC(bufferLength)))
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }

            /* headers */
            MOC_MEMSET( buffer, 0, 12);
            buffer[7] = 1; /* version */
            buffer[11] = akt_ecc; /* key type */

            index = 12;
            BIGEND32( buffer + index, CRYPTO_getECCurveId(pKey));
            index += 4;

            BIGEND32( buffer + index, pointLen);
            index += 4;
            if (OK > (status = EC_writePointToBuffer( pKey->key.pECC->pCurve, pKey->key.pECC->Qx, pKey->key.pECC->Qy,
                                                        buffer + index, pointLen)))
            {
                goto exit;
            }
            index += pointLen;

            if (pKey->key.pECC->privateKey)
            {
                BIGEND32( buffer + index, scalarLen);
                index += 4;

                if (OK > ( status = PRIMEFIELD_writeByteString( pPF, pKey->key.pECC->k,
                                                                buffer + index, scalarLen)))
                {
                    goto exit;
                }
            }
            break;
        }
#endif
#ifdef __ENABLE_MOCANA_DSA__
        case akt_dsa:
        {
            if (OK > (status = DSA_makeKeyBlob(pKey->key.pDSA, NULL, &bufferLength)))
                goto exit;

            /* Need 12 bytes to create the header for the keyblob */
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

            if (OK > (status = DSA_makeKeyBlob(pKey->key.pDSA, buffer+12, &bufferLength)))
                goto exit;
            break;
        }
#endif
#if (defined (__ENABLE_MOCANA_HW_SECURITY_MODULE__) && defined(__ENABLE_MOCANA_TPM__))
        case akt_rsa_tpm:
        {
            status = SECMOD_serializeOut(pKey, &buffer, &bufferLength);
            break;
        }
#endif
        default:
        {
            status = ERR_CRYPTO_BAD_KEY_TYPE;
        }
    }
    *ppKeyBlob = buffer;
    buffer = NULL;
    *pKeyBlobLen = bufferLength + 12;

exit:
    if((OK > status) && (NULL != buffer))
        FREE(buffer);
    return status;
}
