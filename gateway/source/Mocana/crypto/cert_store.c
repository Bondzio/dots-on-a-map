/* Version: mss_v6_3 */
/*
 * cert_store.c
 *
 * Certificate Store
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*! \file cert_store.c Mocana certificate store factory.
This file contains certificate store functions.

\since 2.02
\version 2.02 and later

! Flags
No flag definitions are required to use this file's functions.

! External Functions
This file contains the following public ($extern$) functions:
- CERT_STORE_addIdentityNakedKey
- CERT_STORE_addTrustPoint
- CERT_STORE_createStore
- CERT_STORE_releaseStore

*/
#include "../common/moptions.h"

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/hash_value.h"
#include "../common/hash_table.h"
#include "../common/random.h"
#include "../common/vlong.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/sizedbuffer.h"
#if (defined(__ENABLE_MOCANA_ECC__))
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#endif
#include "../crypto/pubcrypto.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/rsa.h"
#include "../crypto/crypto.h"
#include "../crypto/ca_mgmt.h"
#include "../crypto/cert_store.h"
#include "../harness/harness.h"
#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
#include "../asn1/parseasn1.h"
#include "../asn1/parsecert.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../asn1/oiddefs.h"
#endif


/*------------------------------------------------------------------*/

#define MOCANA_CERT_STORE_INIT_HASH_VALUE   (0x07d50624)

enum trustPointTypes
{
    CERT_STORE_TRUSTPOINT_CA                        = 0,
    CERT_STORE_TRUSTPOINT_RA                        = 1,
    CERT_STORE_TRUSTPOINT_TP                        = 2
};


/*------------------------------------------------------------------*/

/*!
\exclude
*/
typedef struct identityPair
{
    AsymmetricKey               identityKey;
    sbyte4                      numCertificate;
    SizedBuffer*                certificates;
    ubyte4                      certAlgoFlags; /* see flag definitions in the .h file */

    struct identityPair*        pNextIdentityKeyPair;

} identityPair;

/*!
\exclude
*/
typedef struct identityPskTuple
{
    ubyte*                      pPskIdentity;               /* i.e. PSK identity */
    ubyte4                      pskIdentityLength;
    ubyte*                      pPskHint;                   /* i.e. PSK hint */
    ubyte4                      pskHintLength;
    ubyte*                      pPskSecret;                 /* i.e. PSK secret */
    ubyte4                      pskSecretLength;

    struct identityPskTuple*    pNextIdentityPskTuple;

} identityPskTuple;


/*!
\exclude
*/
typedef struct certStore
{
    identityPair*               pIdentityMatrixList[CERT_STORE_AUTH_TYPE_ARRAY_SIZE][CERT_STORE_IDENTITY_TYPE_ARRAY_SIZE];
    identityPskTuple*           pIdentityPskList;
    hashTableOfPtrs*            pTrustHashTable;            /* a hash table of "trustPoint"s */

    intBoolean                  isCertStoreLocked;          /*!!!! TODO */

} certStore;


/*------------------------------------------------------------------*/

/*!
\exclude
*/
typedef struct trustPoint
{
    enum trustPointTypes        trustType;                  /* CA, RA, trustpoint */

    ubyte*                      pDerCert;
    ubyte4                      derCertLength;

    ubyte4                      subjectOffset;              /* for fast comparisons */
    ubyte4                      subjectLength;

} trustPoint;

/*!
\exclude
*/
typedef struct subjectDescr
{
    ubyte*                      pSubject;
    ubyte4                      subjectLength;

} subjectDescr;


/*------------------------------------------------------------------*/

static MSTATUS
CERT_STORE_convertPubKeyTypeToCertStoreKeyType(ubyte4 pubKeyType, ubyte4 *pRetCertStoreKeyType)
{
    MSTATUS status = OK;

    switch (pubKeyType)
    {
        case akt_rsa:
        {
            *pRetCertStoreKeyType = CERT_STORE_AUTH_TYPE_RSA;
            break;
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            *pRetCertStoreKeyType = CERT_STORE_AUTH_TYPE_ECDSA;
            break;
        }
#endif
#if (defined(__ENABLE_MOCANA_DSA__))
        case akt_dsa:
        {
            *pRetCertStoreKeyType = CERT_STORE_AUTH_TYPE_DSA;
            break;
        }
#endif
        default:
        {
            status = ERR_CERT_STORE_UNKNOWN_KEY_TYPE;
            break;
        }
    }

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_STORE_allocHashPtrElement(void *pHashCookie, hashTablePtrElement **ppRetNewHashElement)
{
    /* we could use a memory pool here to reduce probability of fragmentation */
    /* certificates stores should be fairly small, so a pool is probably not necessary */
    MSTATUS status = OK;

    if (NULL == (*ppRetNewHashElement = MALLOC(sizeof(hashTablePtrElement))))
        status = ERR_MEM_ALLOC_FAIL;

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_STORE_freeHashPtrElement(void *pHashCookie, hashTablePtrElement *pFreeHashElement)
{
    trustPoint*     pTrustPointDescr = (trustPoint *)pFreeHashElement->pAppData;

    /* Clear App data added to Hash Table */
    if(NULL != pTrustPointDescr)
    {
        if(NULL != pTrustPointDescr->pDerCert)
            FREE(pTrustPointDescr->pDerCert);

        FREE(pTrustPointDescr);
    }

    FREE(pFreeHashElement);
    return OK;
}


/*------------------------------------------------------------------*/

/*! Create and initialize a certificate store.
This function creates and initializes a certificate store container instance.
(Multiple instances are allowed.)

\since 2.02
\version 2.02 and later

! Flags
No flag definitions are required to use this callback.

#Include %file:#&nbsp;&nbsp;cert_store.h

\param ppNewStore   Pointer to certStorePtr, which on return, contains the newly
allocated and initialized certificate store container.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern MSTATUS
CERT_STORE_createStore(certStorePtr *ppNewStore)
{
    hashTableOfPtrs*    pTrustHashTable = NULL;
    certStore*          pNewStore = NULL;
    MSTATUS             status;

    if (NULL == ppNewStore)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL == (pNewStore = MALLOC(sizeof(certStore))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET((ubyte *)(pNewStore), 0x00, sizeof(certStore));

    if (OK > (status = HASH_TABLE_createPtrsTable(&pTrustHashTable, MAX_SIZE_CERT_STORE_TRUST_HASH_TABLE, NULL, CERT_STORE_allocHashPtrElement, CERT_STORE_freeHashPtrElement)))
        goto exit;

    /* add/clean trust hash table to new store */
    pNewStore->pTrustHashTable = pTrustHashTable;
    pTrustHashTable = NULL;

    /* return/clean new store */
    *ppNewStore = pNewStore;
    pNewStore = NULL;

exit:
    if (NULL != pNewStore)
        FREE(pNewStore);

    return status;
}


/*------------------------------------------------------------------*/

/*! Release (free) memory used by a certificate store.
This function releases (frees) memory used by a certificate store, including all
its component structures.

\since 2.02
\version 2.02 and later

! Flags
No flag definitions are required to use this callback.

#Include %file:#&nbsp;&nbsp;cert_store.h

\param ppReleaseStore   Pointer to certificate store to release (free).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern MSTATUS
CERT_STORE_releaseStore(certStorePtr *ppReleaseStore)
{
    ubyte4  i, j;
    MSTATUS status = OK;

    if ((NULL == ppReleaseStore) || (NULL == *ppReleaseStore))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* Remove identity descriptors */
    for (i = 0; i < CERT_STORE_AUTH_TYPE_ARRAY_SIZE; i++)
    {
        identityPair*   pIdentityPair;
        identityPair*   pNextIdentityPair;

        for (j = 0; j < CERT_STORE_IDENTITY_TYPE_ARRAY_SIZE; j++)
        {
            if (NULL != (pIdentityPair = (*ppReleaseStore)->pIdentityMatrixList[i][j]))
            {
                /* clear head */
                (*ppReleaseStore)->pIdentityMatrixList[i][j] = NULL;

                while (pIdentityPair)
                {
                    /* get next pair ptr */
                    pNextIdentityPair = (identityPair *)pIdentityPair->pNextIdentityKeyPair;

                    /* free identity record */
                    CRYPTO_uninitAsymmetricKey(&pIdentityPair->identityKey, NULL);

                    if (NULL != pIdentityPair->certificates)
                    {
                        sbyte4 i;
                        for (i = 0; i < pIdentityPair->numCertificate; i++)
                        {
                            SB_Release(&pIdentityPair->certificates[i]);
                        }
                        FREE(pIdentityPair->certificates);
                    }

                    FREE(pIdentityPair);

                    /* move to next identity in list */
                    pIdentityPair = pNextIdentityPair;
                }
            }
        }
    }

    /* Remove PSK identity descriptors */
    if (NULL != (*ppReleaseStore)->pIdentityPskList)
    {
        identityPskTuple*   pIdentityPskTuple = (*ppReleaseStore)->pIdentityPskList;
        identityPskTuple*   pNextIdentityPskTuple;

        /* clear head */
        (*ppReleaseStore)->pIdentityPskList = NULL;

        while (pIdentityPskTuple)
        {
            /* get next tuple */
            pNextIdentityPskTuple = pIdentityPskTuple->pNextIdentityPskTuple;

            /* clear out current tuple */
            if (pIdentityPskTuple->pPskIdentity)
                FREE(pIdentityPskTuple->pPskIdentity);

            if (pIdentityPskTuple->pPskHint)
                FREE(pIdentityPskTuple->pPskHint);

            if (pIdentityPskTuple->pPskSecret)
                FREE(pIdentityPskTuple->pPskSecret);

            /* free whole record */
            FREE(pIdentityPskTuple);

            /* move to next tuple */
            pIdentityPskTuple = pNextIdentityPskTuple;
        }
    }

    /* remove hash table of trust points, CAs, etc */
    HASH_TABLE_removePtrsTable((*ppReleaseStore)->pTrustHashTable, NULL);

    FREE(*ppReleaseStore);
    *ppReleaseStore = NULL;

exit:
    return status;
}


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
static MSTATUS
CERT_STORE_verifyCertWithAsymmetricKey(identityPair *pIdentity, ubyte4 *pRetCertPubKeyType, intBoolean *pIsGood)
{
    hwAccelDescr    hwAccelCtx;
    ubyte*          pDerCert = NULL;
    ubyte4          derCertLength = 0;
    AsymmetricKey*  pKey;
    ASN1_ITEM*      pCert = NULL;
    ASN1_ITEM*      pSignAlgoId = NULL;
    MemFile         certMemFile;
    CStream         cs;
    AsymmetricKey   certKey;
    vlong*          pN = NULL;
    ubyte4          hashType = 0;
    ubyte4          pubKeyType = 0;
    MSTATUS         status;

    static WalkerStep signatureAlgoWalkInstructions[] =
    {
        { GoFirstChild, 0, 0},
        { GoNthChild, 2, 0},
        { VerifyType, SEQUENCE, 0 },
        { Complete, 0, 0}
    };

    if (pIdentity->numCertificate > 0)
    {
        pDerCert = pIdentity->certificates[0].data;
        derCertLength = pIdentity->certificates[0].length;
    }

    pKey = &pIdentity->identityKey;

    if ((NULL == pDerCert) || (NULL == pIsGood))
        return ERR_NULL_POINTER;

    if (OK > (status = CRYPTO_initAsymmetricKey(&certKey)))
        goto exit;

    *pIsGood = FALSE;

    /* extract the public key of the certificate */
    if (0 == derCertLength)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    MF_attach(&certMemFile, derCertLength, (ubyte *)pDerCert);
    CS_AttachMemFile(&cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse( cs, &pCert)))
        goto exit;

    if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        goto exit;

    status = CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(hwAccelCtx) pCert, cs, &certKey);

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    if (OK > status)
        goto exit;

    if ((akt_undefined != pKey->type) && (certKey.type != pKey->type))
    {
        status = ERR_CERT_AUTH_MISMATCH_PUBLIC_KEYS;
        goto exit;
    }

    *pRetCertPubKeyType = certKey.type;

    if ( OK > ASN1_WalkTree( pCert, cs, signatureAlgoWalkInstructions, &pSignAlgoId))
    {
        return ERR_CERT_INVALID_STRUCT;
    }

    status = CERT_getCertSignAlgoType( pSignAlgoId, cs, &hashType, &pubKeyType);
    if (OK > status)
    {
        status = ERR_CERT_STORE_UNSUPPORTED_SIGNALGO;
        goto exit;
    }

    switch (hashType)
    {
    case ht_md5:
        hashType = CERT_STORE_ALGO_FLAG_MD5;
        break;

    case ht_sha1:
        hashType = CERT_STORE_ALGO_FLAG_SHA1;
        break;

#ifndef __DISABLE_MOCANA_SHA224__
    case ht_sha224:
        hashType = CERT_STORE_ALGO_FLAG_SHA224;
        break;
#endif

#ifndef __DISABLE_MOCANA_SHA256__
    case ht_sha256:
        hashType = CERT_STORE_ALGO_FLAG_SHA256;
        break;
#endif

#ifndef __DISABLE_MOCANA_SHA384__
    case ht_sha384:
        hashType = CERT_STORE_ALGO_FLAG_SHA384;
        break;
#endif

#ifndef __DISABLE_MOCANA_SHA512__
    case ht_sha512:
        hashType = CERT_STORE_ALGO_FLAG_SHA512;
        break;
#endif
    default:
        status = ERR_CERT_STORE_UNSUPPORTED_SIGNALGO;
        goto exit;
    }

    switch (pubKeyType)
    {
    case akt_rsa:
        pubKeyType = CERT_STORE_ALGO_FLAG_RSA;
        break;

#if (defined(__ENABLE_MOCANA_ECC__))
    case akt_ecc:
        pubKeyType = CERT_STORE_ALGO_FLAG_ECDSA;
        break;
#endif

    default:
        status = ERR_CERT_STORE_UNSUPPORTED_SIGNALGO;
        goto exit;
    }

    pIdentity->certAlgoFlags = hashType | pubKeyType;

    switch (certKey.type)
    {
        case akt_rsa:
        {
            if (akt_undefined != pKey->type)
            {
                /* verify key blob and certificate public keys match */
                if ((0 != VLONG_compareSignedVlongs(RSA_N(certKey.key.pRSA), RSA_N(pKey->key.pRSA))) ||
                    (0 != VLONG_compareSignedVlongs(RSA_E(certKey.key.pRSA), RSA_E(pKey->key.pRSA))) )
                {
                    status = ERR_CERT_AUTH_MISMATCH_PUBLIC_KEYS;
                    goto exit;
                }

                /* verify key blob private key matches public key */
                if (OK > (status = VLONG_allocVlong(&pN, NULL)))
                    goto exit;

                if (OK > (status = VLONG_vlongSignedMultiply(pN, RSA_P(pKey->key.pRSA), RSA_Q(pKey->key.pRSA))))
                    goto exit;

                if (0 != VLONG_compareSignedVlongs(pN, RSA_N(pKey->key.pRSA)))
                {
                    status = ERR_CERT_AUTH_KEY_BLOB_CORRUPT;
                    goto exit;
                }
            }

            pIdentity->certAlgoFlags &= ~(CERT_STORE_ALGO_FLAG_ECCURVES);

            *pIsGood = TRUE;

            break;
        }
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            if (akt_undefined != pKey->type)
            {
                if (OK > (status = EC_verifyKeyPair(certKey.key.pECC->pCurve, pKey->key.pECC->k,
                                                    certKey.key.pECC->Qx, certKey.key.pECC->Qy)))
                    goto exit;
            }

            /* record the ec Curve of the cert */
#ifdef __ENABLE_MOCANA_ECC_P192__
            if (EC_P192 == certKey.key.pECC->pCurve)
            {
                pIdentity->certAlgoFlags |= CERT_STORE_ALGO_FLAG_EC192;
            } else
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
                if (EC_P224 == certKey.key.pECC->pCurve)
            {
                pIdentity->certAlgoFlags |= CERT_STORE_ALGO_FLAG_EC224;
            } else
#endif
#ifndef __DISABLE_MOCANA_ECC_P256__
                if (EC_P256== certKey.key.pECC->pCurve)
            {
                pIdentity->certAlgoFlags |= CERT_STORE_ALGO_FLAG_EC256;
            } else
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
                if (EC_P384 == certKey.key.pECC->pCurve)
            {
                pIdentity->certAlgoFlags |= CERT_STORE_ALGO_FLAG_EC384;
            } else
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
                if (EC_P521 == certKey.key.pECC->pCurve)
            {
                pIdentity->certAlgoFlags |= CERT_STORE_ALGO_FLAG_EC521;
            } else
#endif
            {
                status = ERR_CERT_STORE_UNSUPPORTED_ECCURVE;
                goto exit;
            }
            *pIsGood = TRUE;
            break;
        }
#endif
        default:
        {
            status = ERR_CERT_AUTH_MISMATCH_PUBLIC_KEYS;
            goto exit;
        }

    } /* switch */

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    VLONG_freeVlong(&pN, NULL);
    CRYPTO_uninitAsymmetricKey(&certKey, NULL);

    return status;
}
#endif


/*------------------------------------------------------------------*/

static MSTATUS
CERT_STORE_checkStore(const certStorePtr pCertStore)
{
    MSTATUS status = OK;

    if (pCertStore->isCertStoreLocked)
        status = ERR_CERT_STORE_LOCKED_STORE;

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_STORE_addGenericIdentity(certStorePtr pCertStore,
                              const ubyte *pKeyBlob, ubyte4 keyBlobLength,
                              enum identityTypes identityType, SizedBuffer *certificates, ubyte4 numCertificate)
{
    hwAccelDescr    hwAccelCtx;
    ubyte4          keyType = 0;
    identityPair*   pNewIdentity = NULL;
    MSTATUS         status;

    /* allocate store for new identity */
    if (NULL == (pNewIdentity = MALLOC(sizeof(identityPair))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET((ubyte *)pNewIdentity, 0x00, sizeof(identityPair));

    if (numCertificate > 0)
    {
        ubyte4 i;

        /* duplicate the certificates */
        if (NULL == (pNewIdentity->certificates = MALLOC(sizeof(SizedBuffer)*numCertificate)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        for (i = 0; i < numCertificate; i++)
        {
            SB_Allocate(&pNewIdentity->certificates[i], certificates[i].length);
            MOC_MEMCPY(pNewIdentity->certificates[i].data, certificates[i].data, certificates[i].length);
        }
        pNewIdentity->numCertificate = numCertificate;
    }

    if (NULL != pKeyBlob)
    {
        /* extract key from blob */
        if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
            goto exit;

        /* store extracted key */
        status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &(pNewIdentity->identityKey));

        HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

        if (OK > status)
            goto exit;

        keyType = pNewIdentity->identityKey.type;
    }

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
    /* verify certificate and key blob match */
    if (numCertificate > 0)
    {
        intBoolean isGood = FALSE;

        if (OK > (status = CERT_STORE_verifyCertWithAsymmetricKey(pNewIdentity, &keyType, &isGood)))
            goto exit;

        if (FALSE == isGood)
        {
            status = ERR_CERT_STORE_CERT_KEY_MISMATCH;
            goto exit;
        }
    }
#endif

    if (OK > (status = CERT_STORE_convertPubKeyTypeToCertStoreKeyType(keyType, &keyType)))
        goto exit;

    if (NULL == pCertStore->pIdentityMatrixList[keyType][identityType])
    {
        /* add to head of list */
        pCertStore->pIdentityMatrixList[keyType][identityType] = pNewIdentity;
    }
    else
    {
        /* add to end of list */
        identityPair *pIdentityTravseList = pCertStore->pIdentityMatrixList[keyType][identityType];

        /* traverse to end of list */
        while (NULL != pIdentityTravseList->pNextIdentityKeyPair)
            pIdentityTravseList = (identityPair *)pIdentityTravseList->pNextIdentityKeyPair;

        /* tack new identity to the end of the list */
        pIdentityTravseList->pNextIdentityKeyPair = pNewIdentity;
    }

    pNewIdentity = NULL;

exit:
    if (pNewIdentity)
    {
        if (pNewIdentity->numCertificate > 0)
        {
            sbyte4 i;
            for (i = 0; i < pNewIdentity->numCertificate; i++)
            {
                SB_Release(&pNewIdentity->certificates[i]);
            }

            FREE(pNewIdentity->certificates);
        }
        FREE(pNewIdentity);
    }

    return status;

} /* CERT_STORE_addGenericIdentity */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_addIdentity(certStorePtr pCertStore, const ubyte *pDerCert, ubyte4 derCertLength, const ubyte *pKeyBlob, ubyte4 keyBlobLength)
{
    MSTATUS status;
    SizedBuffer certificates[1];
    ubyte4 numCertificate = 1;

    if (pDerCert && derCertLength > 0)
    {
        certificates[0].data   = (ubyte*)pDerCert;
        certificates[0].length = (ubyte2)derCertLength;
    } else
    {
        numCertificate = 0;
    }

    status = CERT_STORE_addIdentityWithCertificateChain(pCertStore, certificates, numCertificate, pKeyBlob, keyBlobLength);

    return status;
}

extern MSTATUS
CERT_STORE_addIdentityWithCertificateChain(certStorePtr pCertStore, SizedBuffer *certificates, ubyte4 numCertificate, const ubyte *pKeyBlob, ubyte4 keyBlobLength)
{
    MSTATUS status;

    if ((NULL == pCertStore) || (NULL == certificates) || (0 == numCertificate))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_STORE_checkStore(pCertStore)))
        goto exit;

    status = CERT_STORE_addGenericIdentity(pCertStore,
                                           pKeyBlob, keyBlobLength,
                                           CERT_STORE_IDENTITY_TYPE_CERT_X509_V3, certificates, numCertificate);

exit:
    return status;
}
#endif


/*------------------------------------------------------------------*/

/*! Add a naked key to a certificate store.
This function adds a }naked key}&mdash;a key blob that has no
associated certificate&mdash; to a certificate store.

\since 2.02
\version 2.02 and later

! Flags
No flag definitions are required to use this callback.

#Include %file:#&nbsp;&nbsp;cert_store.h

\param pCertStore       Pointer to the certificate store to add the naked key to.
\param pKeyBlob         Pointer to the naked key to add.
\param keyBlobLength    Number of bytes in the naked key ($pKeyBlob$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern MSTATUS
CERT_STORE_addIdentityNakedKey(certStorePtr pCertStore, const ubyte *pKeyBlob, ubyte4 keyBlobLength)
{
    MSTATUS status;

    if ((NULL == pCertStore) || (NULL == pKeyBlob))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_STORE_checkStore(pCertStore)))
        goto exit;

    status = CERT_STORE_addGenericIdentity(pCertStore,
                                           pKeyBlob, keyBlobLength,
                                           CERT_STORE_IDENTITY_TYPE_NAKED, NULL, 0);

exit:
    return status;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_addIdentityPSK(certStorePtr pCertStore,
                          const ubyte *pPskIdentity, ubyte4 pskIdentityLength,
                          const ubyte *pPskHint, ubyte4 pskHintLength,
                          const ubyte *pPskSecret, ubyte4 pskSecretLength)
{
    identityPskTuple*   pNewPskIdentity = NULL;
    MSTATUS             status = OK;

    if ((NULL == pCertStore) || (NULL == pPskIdentity) || (NULL == pPskSecret))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_STORE_checkStore(pCertStore)))
        goto exit;

    /* allocate store for new identity */
    if (NULL == (pNewPskIdentity = MALLOC(sizeof(identityPskTuple))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET((ubyte *)pNewPskIdentity, 0x00, sizeof(identityPskTuple));

    /* duplicate the psk identity */
    if (NULL == (pNewPskIdentity->pPskIdentity = MALLOC(pskIdentityLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pNewPskIdentity->pPskIdentity, pPskIdentity, pskIdentityLength);
    pNewPskIdentity->pskIdentityLength = pskIdentityLength;

    /* optionally, duplicate the psk hint */
    if (NULL != pPskHint)
    {
        if (NULL == (pNewPskIdentity->pPskHint = MALLOC(pskHintLength)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMCPY(pNewPskIdentity->pPskHint, pPskHint, pskHintLength);
        pNewPskIdentity->pskIdentityLength = pskHintLength;
    }

    /* duplicate the psk secret */
    if (NULL == (pNewPskIdentity->pPskSecret = MALLOC(pskSecretLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pNewPskIdentity->pPskSecret, pPskSecret, pskSecretLength);
    pNewPskIdentity->pskSecretLength = pskSecretLength;

    /* insert new identity to head of list */
    pNewPskIdentity->pNextIdentityPskTuple = pCertStore->pIdentityPskList;
    pCertStore->pIdentityPskList = pNewPskIdentity;

    pNewPskIdentity = NULL;

exit:
    if (pNewPskIdentity)
    {
        if(pNewPskIdentity->pPskIdentity)
            FREE(pNewPskIdentity->pPskIdentity);

        if(pNewPskIdentity->pPskHint)
            FREE(pNewPskIdentity->pPskHint);

        if(pNewPskIdentity->pPskSecret)
            FREE(pNewPskIdentity->pPskSecret);

        FREE(pNewPskIdentity);
    }
    return status;

} /* CERT_STORE_addIdentityPSK */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
static MSTATUS
CERT_STORE_addToTrustHashTable(certStorePtr pCertStore, const ubyte *pDerCert, ubyte4 derCertLength, enum trustPointTypes trustType)
{
    trustPoint*     pTrustPointDescr = NULL;
    ASN1_ITEMPTR    pAsn1CertTree = NULL;
    ASN1_ITEMPTR    pSubject;
    MemFile         certMemFile;
    CStream         cs;
    ubyte*          pCertClone = NULL;
    ubyte4          hashValue;
    MSTATUS         status = OK;

    /* clone certificate */
    if (NULL == (pCertClone = MALLOC(derCertLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pCertClone, pDerCert, derCertLength);

    /* allocate/init trust point structure */
    if (NULL == (pTrustPointDescr = MALLOC(sizeof(trustPoint))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET((ubyte *)pTrustPointDescr, 0x00, sizeof(trustPoint));

    pTrustPointDescr->trustType      = trustType;
    pTrustPointDescr->pDerCert       = pCertClone;
    pTrustPointDescr->derCertLength  = derCertLength;

    pCertClone = NULL;  /* to prevent double-free */

    /* extract certificate's subject */
    MF_attach(&certMemFile, derCertLength, (ubyte*)pTrustPointDescr->pDerCert);
    CS_AttachMemFile(&cs, &certMemFile);

    /* parse the certificate */
    if (OK > (status = ASN1_Parse(cs, &pAsn1CertTree)))
        goto exit;

    /* fetch the data we want to grab */
    if (OK > (status = CERT_getCertificateSubject(pAsn1CertTree, &pSubject)))
        goto exit;

    pTrustPointDescr->subjectOffset = pSubject->dataOffset;
    pTrustPointDescr->subjectLength = pSubject->length;

    /* calculate hash for subject */
    HASH_VALUE_hashGen(pTrustPointDescr->subjectOffset + pDerCert, pTrustPointDescr->subjectLength, MOCANA_CERT_STORE_INIT_HASH_VALUE, &hashValue);

    /* store results */
    if (OK > (status = HASH_TABLE_addPtr(pCertStore->pTrustHashTable, hashValue, pTrustPointDescr)))
        goto exit;

    pTrustPointDescr = NULL;

exit:
    if (pAsn1CertTree)
        TREE_DeleteTreeItem((TreeItem*)pAsn1CertTree);

    if (NULL != pTrustPointDescr)
    {
        if (pTrustPointDescr->pDerCert)
            FREE(pTrustPointDescr->pDerCert);

        FREE(pTrustPointDescr);
    }

    if (pCertClone)
        FREE(pCertClone);

    return status;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_addCertAuthority(certStorePtr pCertStore, const ubyte *pDerCert, ubyte4 derCertLength)
{
    MSTATUS status;

    if ((NULL == pCertStore) || (NULL == pDerCert))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_STORE_checkStore(pCertStore)))
        goto exit;

    status = CERT_STORE_addToTrustHashTable(pCertStore, pDerCert, derCertLength, CERT_STORE_TRUSTPOINT_CA);

exit:
    return status;
}


/*------------------------------------------------------------------*/

/*! Add a trust point to a certificate store.
This function adds a trust point to a certificate store.

\since 2.02
\version 2.02 and later

! Flags
No flag definitions are required to use this callback.

#Include %file:#&nbsp;&nbsp;cert_store.h

\param pCertStore           Pointer to the certificate store to add the trust point to.
\param pDerTrustPoint       Pointer to the trust point to add.
\param derTrustPointLength  Number of bytes in the trust point ($pDerTrustPoint$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern MSTATUS
CERT_STORE_addTrustPoint(certStorePtr pCertStore, const ubyte *pDerTrustPoint, ubyte4 derTrustPointLength)
{
    MSTATUS status = OK;

    if ((NULL == pCertStore) || (NULL == pDerTrustPoint))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_STORE_checkStore(pCertStore)))
        goto exit;

    status = CERT_STORE_addToTrustHashTable(pCertStore, pDerTrustPoint, derTrustPointLength, CERT_STORE_TRUSTPOINT_TP);

exit:
    return status;
}
#endif /* #ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__ */


/*------------------------------------------------------------------*/

static MSTATUS
CERT_STORE_testSubject(void* pAppData, void* pTestData, intBoolean *pRetIsMatch)
{
    trustPoint*     pTrustPointDescr = pAppData;
    subjectDescr*   pSubjectDescr    = pTestData;
    sbyte4          compareResult;
    MSTATUS         status = OK;

    if ((NULL == pTrustPointDescr) || (NULL == pSubjectDescr))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pRetIsMatch = FALSE;

    if (pTrustPointDescr->subjectLength == pSubjectDescr->subjectLength)
    {
        status = MOC_MEMCMP(pTrustPointDescr->subjectOffset + pTrustPointDescr->pDerCert, pSubjectDescr->pSubject, pSubjectDescr->subjectLength, &compareResult);

        if (0 == compareResult)
            *pRetIsMatch = TRUE;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_findCertBySubject(const certStorePtr pCertStore,
                             ubyte *pSubject, ubyte4 subjectLength,
                             ubyte **ppRetDerCert, ubyte4 *pRetDerCertLength)
{
    trustPoint*     pTrustPointDescr = NULL;
    subjectDescr    subjectData;
    ubyte4          hashValue;
    intBoolean      foundHashValue;
    MSTATUS         status = OK;

    if ((NULL == pCertStore) || (NULL == pSubject) || (NULL == ppRetDerCert) || (NULL == pRetDerCertLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* calculate hash for subject */
    HASH_VALUE_hashGen(pSubject, subjectLength, MOCANA_CERT_STORE_INIT_HASH_VALUE, &hashValue);

    subjectData.pSubject      = pSubject;
    subjectData.subjectLength = subjectLength;

    /* look up subject in certificate store */
    if (OK > (status = HASH_TABLE_findPtr(pCertStore->pTrustHashTable, hashValue, (void *)&subjectData, CERT_STORE_testSubject, (void **)&pTrustPointDescr, &foundHashValue)))
        goto exit;

    if ((TRUE != foundHashValue) || (NULL == pTrustPointDescr))
    {
        *ppRetDerCert = NULL;
        *pRetDerCertLength = 0;
    }
    else
    {
        /* found it, and pTrustPointDescr pointer is good */
        *ppRetDerCert = pTrustPointDescr->pDerCert;
        *pRetDerCertLength = pTrustPointDescr->derCertLength;
    }

exit:
    return status;
}

/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_findIdentityByTypeFirst(const certStorePtr pCertStore,
                                   enum authTypes authType, enum identityTypes identityType,
                                   AsymmetricKey** ppRetIdentityKey,
                                   ubyte **ppRetDerCert, ubyte4 *pRetDerCertLength,
                                   void** ppRetHint)
{
    identityPair*   pIdentityPair;
    MSTATUS         status = OK;

    if (NULL == pCertStore)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL != (pIdentityPair = pCertStore->pIdentityMatrixList[authType][identityType]))
    {
        if (ppRetIdentityKey)
            *ppRetIdentityKey  = &(pIdentityPair->identityKey);

        if (ppRetDerCert)
            *ppRetDerCert      = pIdentityPair->numCertificate > 0? pIdentityPair->certificates[0].data : NULL;

        if (pRetDerCertLength)
            *pRetDerCertLength = pIdentityPair->numCertificate > 0? pIdentityPair->certificates[0].length : 0;
    }
    else
    {
        if (ppRetIdentityKey)
            *ppRetIdentityKey  = NULL;

        if (ppRetDerCert)
            *ppRetDerCert      = NULL;

        if (pRetDerCertLength)
            *pRetDerCertLength = 0;
    }

    if (ppRetHint)
        *ppRetHint = pIdentityPair;

exit:
    return status;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_findIdentityByTypeNext(const certStorePtr pCertStore,
                                  enum authTypes authType, enum identityTypes identityType,
                                  AsymmetricKey** ppRetIdentityKey,
                                  ubyte **ppRetDerCert, ubyte4 *pRetDerCertLength,
                                  void** ppRetHint)
{
    identityPair*   pIdentityPair;
    MSTATUS         status = OK;

    if ((NULL == pCertStore) || (NULL == ppRetHint))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (ppRetIdentityKey)
        *ppRetIdentityKey  = NULL;

    if (ppRetDerCert)
        *ppRetDerCert      = NULL;

    if (pRetDerCertLength)
        *pRetDerCertLength = 0;

    if (NULL == *ppRetHint)
        goto exit;          /* nothing to continue from */

    pIdentityPair = *ppRetHint;
    pIdentityPair = (identityPair *)pIdentityPair->pNextIdentityKeyPair;

    if (NULL != pIdentityPair)
    {
        if (ppRetIdentityKey)
            *ppRetIdentityKey  = &(pIdentityPair->identityKey);

        if (ppRetDerCert)
            *ppRetDerCert      = pIdentityPair->numCertificate > 0? pIdentityPair->certificates[0].data : NULL;

        if (pRetDerCertLength)
            *pRetDerCertLength = pIdentityPair->numCertificate > 0? pIdentityPair->certificates[0].length : 0;
    }

    *ppRetHint = pIdentityPair;

exit:
    return status;
}

/*------------------------------------------------------------------*/

/* find an identity certificate by type:the following is only applicable to identityTypes == IDENTITY_TYPE_CERT_X509_V3  */
extern MSTATUS
CERT_STORE_findIdentityCertChainFirst(const certStorePtr pCertStore, ubyte4 pubKeyType, ubyte4 supportedAlgoFlags,
                                                   struct AsymmetricKey** ppRetIdentityKey, SizedBuffer** ppRetCertificates,
                                                   ubyte4 *pRetNumberCertificate, void** ppRetHint)
{
    identityPair*   pIdentityPair;
    ubyte4          authType;
    MSTATUS         status = OK;

    if (NULL == pCertStore || NULL == ppRetIdentityKey ||
        NULL == ppRetCertificates || NULL == pRetNumberCertificate)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* initialize */
    *ppRetIdentityKey  = NULL;
    *ppRetCertificates      = NULL;
    *pRetNumberCertificate = 0;
    if (ppRetHint)
        *ppRetHint = NULL;

    if (OK > (status = CERT_STORE_convertPubKeyTypeToCertStoreKeyType(pubKeyType, &authType)))
        goto exit;

    pIdentityPair = pCertStore->pIdentityMatrixList[authType][CERT_STORE_IDENTITY_TYPE_CERT_X509_V3];
    while (pIdentityPair)
    {
        /* check whether the cert algos flags falls into the supported algo flags */
        if ((pIdentityPair->certAlgoFlags & supportedAlgoFlags & CERT_STORE_ALGO_FLAG_SIGNKEYTYPE) &&
            (pIdentityPair->certAlgoFlags & supportedAlgoFlags & CERT_STORE_ALGO_FLAG_HASHALGO) &&
            ( (akt_ecc != pIdentityPair->identityKey.type) ||
                (pIdentityPair->certAlgoFlags & supportedAlgoFlags & CERT_STORE_ALGO_FLAG_ECCURVES)) )
        {
            if (ppRetIdentityKey)
                *ppRetIdentityKey  = &  (pIdentityPair->identityKey);

            if (ppRetCertificates)
                *ppRetCertificates      = pIdentityPair->certificates;

            if (pRetNumberCertificate)
                *pRetNumberCertificate = pIdentityPair->numCertificate;
            break;
        }
        pIdentityPair = pIdentityPair->pNextIdentityKeyPair;
    }

    if (ppRetHint)
        *ppRetHint = pIdentityPair;

exit:
    return status;
}

extern MSTATUS
CERT_STORE_findIdentityCertChainNext(const certStorePtr pCertStore, ubyte4 pubKeyType, ubyte4 supportedAlgoFlags,
                                                  struct AsymmetricKey** ppRetIdentityKey, SizedBuffer** ppRetCertificates,
                                                  ubyte4 *pRetNumberCertificate, void** ppRetHint)
{
    identityPair*   pIdentityPair;
    MSTATUS         status = OK;

    if ((NULL == pCertStore) || (NULL == ppRetHint) ||
        (NULL == ppRetIdentityKey) || (NULL == ppRetCertificates) || (NULL == pRetNumberCertificate))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (ppRetIdentityKey)
        *ppRetIdentityKey  = NULL;

    if (ppRetCertificates)
        *ppRetCertificates      = NULL;

    if (pRetNumberCertificate)
        *pRetNumberCertificate = 0;

    if (NULL == *ppRetHint)
        goto exit;          /* nothing to continue from */

    pIdentityPair = *ppRetHint;
    pIdentityPair = (identityPair *)pIdentityPair->pNextIdentityKeyPair;

    while (NULL != pIdentityPair)
    {
        /* check whether the cert algos flags falls into the supported algo flags */
        if ((pIdentityPair->certAlgoFlags & supportedAlgoFlags & CERT_STORE_ALGO_FLAG_ECCURVES) &&
            (pIdentityPair->certAlgoFlags & supportedAlgoFlags & CERT_STORE_ALGO_FLAG_SIGNKEYTYPE) &&
            (pIdentityPair->certAlgoFlags & supportedAlgoFlags & CERT_STORE_ALGO_FLAG_HASHALGO))
        {

            *ppRetIdentityKey  = &(pIdentityPair->identityKey);

            *ppRetCertificates      = pIdentityPair->certificates;

            *pRetNumberCertificate = pIdentityPair->numCertificate;
            break;
        }
        pIdentityPair = pIdentityPair->pNextIdentityKeyPair;
    }
    *ppRetHint = pIdentityPair;

exit:
    return status;
}

/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_traversePskListHead(const certStorePtr pCertStore,
                               ubyte **ppRetPskIdentity, ubyte4 *pRetPskIdentityLength,
                               ubyte **ppRetPskHint, ubyte4 *pRetPskHintLength,
                               ubyte **ppRetPskSecret, ubyte4 *pRetPskSecretLength,
                               void** ppRetHint)
{
    identityPskTuple*   pIdentityPskTuple;
    MSTATUS             status = OK;

    if (NULL == pCertStore)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL != (pIdentityPskTuple = pCertStore->pIdentityPskList))
    {
        if (ppRetPskIdentity)
            *ppRetPskIdentity      = pIdentityPskTuple->pPskIdentity;

        if (pRetPskIdentityLength)
            *pRetPskIdentityLength = pIdentityPskTuple->pskIdentityLength;

        if (ppRetPskHint)
            *ppRetPskHint = pIdentityPskTuple->pPskHint;

        if (pRetPskHintLength)
            *pRetPskHintLength = pIdentityPskTuple->pskHintLength;

        if (ppRetPskSecret)
            *ppRetPskSecret = pIdentityPskTuple->pPskSecret;

        if (pRetPskSecretLength)
            *pRetPskSecretLength = pIdentityPskTuple->pskSecretLength;
    }
    else
    {
        if (ppRetPskIdentity)
            *ppRetPskIdentity      = NULL;

        if (pRetPskIdentityLength)
            *pRetPskIdentityLength = 0;

        if (ppRetPskHint)
            *ppRetPskHint = NULL;

        if (pRetPskHintLength)
            *pRetPskHintLength = 0;

        if (ppRetPskSecret)
            *ppRetPskSecret = NULL;

        if (pRetPskSecretLength)
            *pRetPskSecretLength = 0;
    }

    if (ppRetHint)
        *ppRetHint = pIdentityPskTuple;

exit:
    return status;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_traversePskListNext(const certStorePtr pCertStore,
                               ubyte **ppRetPskIdentity, ubyte4 *pRetPskIdentityLength,
                               ubyte **ppRetPskHint, ubyte4 *pRetPskHintLength,
                               ubyte **ppRetPskSecret, ubyte4 *pRetPskSecretLength,
                               void** ppRetHint)
{
    identityPskTuple*   pIdentityPskTuple;
    MSTATUS             status = OK;

    if ((NULL == pCertStore) || (NULL == ppRetHint))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (ppRetPskIdentity)
        *ppRetPskIdentity      = NULL;

    if (pRetPskIdentityLength)
        *pRetPskIdentityLength = 0;

    if (ppRetPskHint)
        *ppRetPskHint = NULL;

    if (pRetPskHintLength)
        *pRetPskHintLength = 0;

    if (ppRetPskSecret)
        *ppRetPskSecret = NULL;

    if (pRetPskSecretLength)
        *pRetPskSecretLength = 0;

    if (NULL == *ppRetHint)
        goto exit;          /* nothing to continue from */

    pIdentityPskTuple = *ppRetHint;
    pIdentityPskTuple = pIdentityPskTuple->pNextIdentityPskTuple;

    if (NULL != pIdentityPskTuple)
    {
        if (ppRetPskIdentity)
            *ppRetPskIdentity      = pIdentityPskTuple->pPskIdentity;

        if (pRetPskIdentityLength)
            *pRetPskIdentityLength = pIdentityPskTuple->pskIdentityLength;

        if (ppRetPskHint)
            *ppRetPskHint = pIdentityPskTuple->pPskHint;

        if (pRetPskHintLength)
            *pRetPskHintLength = pIdentityPskTuple->pskHintLength;

        if (ppRetPskSecret)
            *ppRetPskSecret = pIdentityPskTuple->pPskSecret;

        if (pRetPskSecretLength)
            *pRetPskSecretLength = pIdentityPskTuple->pskSecretLength;
    }

    *ppRetHint = pIdentityPskTuple;

exit:
    return status;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for future use, and should not be included in the
current API documentation.
*/
extern MSTATUS
CERT_STORE_findPskByIdentity(const certStorePtr pCertStore,
                             ubyte *pPskIdentity, ubyte4 pskIdentityLength,
                             ubyte **ppRetPskSecret, ubyte4 *pRetPskSecretLength)
{
    sbyte4              compareResults;
    identityPskTuple*   pIdentityPskTuple;
    MSTATUS             status = OK;

    if ((NULL == pCertStore) || (NULL == pPskIdentity) || (NULL == ppRetPskSecret) || (NULL == pRetPskSecretLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *ppRetPskSecret      = NULL;
    *pRetPskSecretLength = 0;

    pIdentityPskTuple = pCertStore->pIdentityPskList;

    while (NULL != pIdentityPskTuple)
    {
        if ((pIdentityPskTuple->pskIdentityLength == pskIdentityLength) &&
            (OK <= MOC_MEMCMP(pIdentityPskTuple->pPskIdentity, pPskIdentity, pskIdentityLength, &compareResults)) &&
            (0 == compareResults) )
        {
            break;
        }

        pIdentityPskTuple = pIdentityPskTuple->pNextIdentityPskTuple;
    }

    if (NULL != pIdentityPskTuple)
    {
        *ppRetPskSecret      = pIdentityPskTuple->pPskSecret;
        *pRetPskSecretLength = pIdentityPskTuple->pskSecretLength;
    }

exit:
    return status;
}

