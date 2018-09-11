/* Version: mss_v6_3 */
/*
 * rsa.c
 *
 * RSA public key encryption
 *
 * Copyright Mocana Corp 2003-2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#ifndef __RSA_HARDWARE_ACCELERATOR__

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/prime.h"
#include "../common/debug_console.h"
#include "../common/memory_debug.h"
#include "../crypto/rsa.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif
#ifdef __ENABLE_MOCANA_PKCS11_CRYPTO__
#include "../crypto/pkcs11.h"
#include "../crypto/hw_offload/pkcs11_rsa.h"
#endif

/*------------------------------------------------------------------*/

#define PREDEFINED_E        (65537)

#ifndef MOCANA_MAX_MODULUS_SIZE
#define MOCANA_MAX_MODULUS_SIZE     (512)
#endif

#ifndef MOCANA_MAX_BLIND_FACTOR_REUSE
#define MOCANA_MAX_BLIND_FACTOR_REUSE (32)
#endif

#define RSA_BLOB_VERSION    (2)


/*------------------------------------------------------------------*/
/* prototype */

#if (defined(__RSAINT_HARDWARE__) && defined(__ENABLE_MOCANA_PKCS11_CRYPTO__))
extern MSTATUS
RSAINT_decrypt(CK_SESSION_HANDLE hSession,
               CK_MECHANISM_PTR pMechanism,
               CK_OBJECT_HANDLE hKey,
               CK_BYTE_PTR pEngcryptedData,
               CK_ULONG ulEncryptedDataLen,
               CK_BYTE_PTR pData,
               CK_ULONG_PTR pulDataLen);

extern MSTATUS
RSA_signMessage(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pKey,
                const ubyte* plainText, ubyte4 plainTextLen,
                ubyte* cipherText, vlong **ppVlongQueue);

#elif (defined(__RSAINT_HARDWARE__))
extern MSTATUS
RSAINT_decrypt(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKeyInt,
               const vlong *pCipher, RNGFun rngFun, void* rngFunArg,
               vlong **ppRetDecrypt, vlong **ppVlongQueue);

#endif   /* __RSAINT_HARDWARE__ */

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
/* prototype */
extern MSTATUS
RSA_generateKey_FIPS_consistancy_test(MOC_RSA(sbyte4 hwAccelCtx) RSAKey *p_rsaKey);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */


/*------------------------------------------------------------------*/

static MSTATUS
RSAINT_encrypt(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKeyInt,
               const vlong *pPlain, vlong **ppRetEncrypt, vlong **ppVlongQueue)
{
    return VLONG_modexp(MOC_MOD(hwAccelCtx) pPlain, RSA_E(pRSAKeyInt),
                        RSA_N(pRSAKeyInt), ppRetEncrypt, ppVlongQueue);
}


/*------------------------------------------------------------------*/

#if !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
static MSTATUS
RSAINT_decryptAux(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKey,
                  const vlong *c, vlong **ppRetDecrypt, vlong **ppVlongQueue)
{
    vlong*  tmp = NULL;
    vlong*  m1  = NULL;
    vlong*  m2  = NULL;
    vlong*  h   = NULL;
    MSTATUS status;

    if (!RSA_DP(pRSAKey) || !RSA_DQ(pRSAKey) || !RSA_QINV(pRSAKey) ||
        !RSA_MODEXP_P(pRSAKey) || !RSA_MODEXP_Q(pRSAKey) )
    {
        /* must call the correct routines to set up the key */
        return ERR_RSA_KEY_NOT_READY;
    }

    if (OK > ( status = VLONG_modExp(MOC_MOD(hwAccelCtx) RSA_MODEXP_P(pRSAKey), c,
                                     RSA_DP(pRSAKey), &m1, ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > ( status = VLONG_modExp(MOC_MOD(hwAccelCtx) RSA_MODEXP_Q(pRSAKey), c,
                                     RSA_DQ(pRSAKey), &m2, ppVlongQueue)))
    {
        goto exit;
    }

    /* h = qInv * (m1 - m2) mod p */
    if ( VLONG_compareSignedVlongs( m1, m2) <  0 )
    {
        if ( OK > ( status = VLONG_addSignedVlongs(m1, RSA_P(pRSAKey),
                                                   ppVlongQueue)))
        {
            goto exit;
        }
    }

    /* m1 -= m2 */
    if (OK > ( status = VLONG_subtractSignedVlongs(m1, m2, ppVlongQueue)))
        goto exit;

    /* temporary = qInv * m1 */
    if (OK > ( status = VLONG_allocVlong(&tmp, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(tmp);

    if (OK > ( status = VLONG_unsignedMultiply(tmp, m1, RSA_QINV(pRSAKey))))
        goto exit;

    /* h = pm mod p */
    if (OK > ( status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp,
                                                      RSA_P(pRSAKey), &h,
                                                      ppVlongQueue)))
    {
        goto exit;
    }

    /* m = m2 + hq */
    if (OK > ( status = VLONG_unsignedMultiply( tmp, h, RSA_Q(pRSAKey))))
        goto exit;

    /* m2 += m1 */
    if (OK > ( status = VLONG_addSignedVlongs(m2, tmp, ppVlongQueue)))
        goto exit;

    *ppRetDecrypt = m2;
    m2 = 0;

exit:
    VLONG_freeVlong(&m1, ppVlongQueue);
    VLONG_freeVlong(&m2, ppVlongQueue);
    VLONG_freeVlong(&h, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);

    return status;

} /* RSAINT_decryptAux */

#endif


#ifdef __ENABLE_MOCANA_VERIFY_RSA_SIGNATURE__

/*------------------------------------------------------------------*/

static MSTATUS
RSAINT_decryptLong(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKey,
               const vlong *c, vlong **ppRetDecrypt, vlong **ppVlongQueue)
{
    vlong*  pm      = NULL;
    vlong*  qm      = NULL;
    vlong*  d       = NULL;
    vlong*  tmp     = NULL;
    MSTATUS status;

    if (OK > (status = VLONG_allocVlong(&tmp, ppVlongQueue)))
        goto exit;

    /* pm = p - 1 */
    if (OK > (status = VLONG_makeVlongFromVlong(RSA_P(pRSAKey), &pm,
                                                ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_decrement(pm, ppVlongQueue)))
        goto exit;

    /* qm = q - 1 */
    if (OK > (status = VLONG_makeVlongFromVlong(RSA_Q(pRSAKey), &qm,
                                                ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_decrement(qm, ppVlongQueue)))
        goto exit;

    /* d = e^-1 mod ((p-1)*(q-1)) */
    if (OK > (status = VLONG_vlongSignedMultiply(tmp, pm, qm)))
        goto exit;

    if (OK > (status = VLONG_modularInverse(MOC_MOD(hwAccelCtx) RSA_E(pRSAKey),
                                            tmp, &d, ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) c, d, RSA_N(pRSAKey),
                                    ppRetDecrypt, ppVlongQueue)))
   {
       goto exit;
   }

exit:
    VLONG_freeVlong(&pm, ppVlongQueue);
    VLONG_freeVlong(&qm, ppVlongQueue);
    VLONG_freeVlong(&d, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);

    return status;

} /* RSAINT_decryptLong */
#endif  /* __ENABLE_MOCANA_VERIFY_RSA_SIGNATURE__ */


/*--------------------------------------------------------------------------*/

#if !defined( __RSAINT_HARDWARE__) && !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
static MSTATUS
RSAINT_initBlindingFactors(MOC_MOD(hwAccelDescr hwAccelCtx) const RSAKey* pRSAKey,
                           vlong** ppRE, vlong** ppR1,
                           RNGFun rngFun, void* rngFunArg,
                           vlong **ppVlongQueue)
{
    MSTATUS status;
    vlong*  pR = 0;
    ubyte4 rSize = RSA_N(pRSAKey)->numUnitsUsed-1;

    /* generate a random number < RSA_N(pRSAKey)  */
    if (OK > (status = VLONG_allocVlong( &pR, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pR);

    if (OK > (status = VLONG_reallocVlong( pR, rSize)))
    {
        goto exit;
    }

#ifdef __MOCANA_BLIND_FACTOR_SIZE__
    if ( __MOCANA_BLIND_FACTOR_SIZE__ &&
         __MOCANA_BLIND_FACTOR_SIZE__ <=  rSize)
    {
        rSize = __MOCANA_BLIND_FACTOR_SIZE__;
    }
#endif

    pR->numUnitsUsed = rSize;
    rngFun( rngFunArg,  rSize * sizeof(vlong_unit), (ubyte*) pR->pUnits);

    /* RE modular E exponent of R */
    if (OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) pR, RSA_E(pRSAKey),
                                    RSA_N(pRSAKey), ppRE, ppVlongQueue)))
    {
        goto exit;
    }

    /* R1 = modular inverse of r */
    if (OK > (status = VLONG_modularInverse(MOC_MOD(hwAccelCtx) pR,
                                            RSA_N(pRSAKey), ppR1,
                                            ppVlongQueue)))
    {
        goto exit;
    }

exit:

    VLONG_freeVlong( &pR, ppVlongQueue);
    return status;
}
#endif /* __RSAINT_HARDWARE__ */


/*--------------------------------------------------------------------------*/

#if !defined( __RSAINT_HARDWARE__) && !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
static MSTATUS
RSAINT_decrypt(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKeyInt,
               const vlong *pCipher, RNGFun rngFun, void* rngFunArg,
               vlong **ppRetDecrypt, vlong **ppVlongQueue)
{
    vlong*  product = NULL;
    vlong*  blinded = NULL;
    vlong*  savedR1 = NULL;
#ifndef __PSOS_RTOS__
    BlindingHelper* pBH;
#endif
    MSTATUS status;

    if ( 0 == rngFun ) /* no blinding */
    {
        return RSAINT_decryptAux( MOC_RSA(hwAccelCtx) pRSAKeyInt,
               pCipher, ppRetDecrypt, ppVlongQueue);
    }

    /* support for custom blinding implementation */
#if defined( CUSTOM_RSA_BLIND_FUNC)
    return CUSTOM_RSA_BLIND_FUNC( MOC_RSA(hwAccelCtx) pRSAKeyInt,
                                pCipher, rngFun, rngFunArg,
                                RSAINT_decryptAux, ppRetDecrypt,
                                ppVlongQueue);

#else

#if !defined(__PSOS_RTOS__)

    /* to defeat constness warnings */
    pBH = (BlindingHelper*) &pRSAKeyInt->blinding;

    /* acquire the lock on the blinding factors */
    if (OK > (status = RTOS_mutexWait( pBH->blindingMutex)))
        goto exit;

    if ( pBH->counter >= MOCANA_MAX_BLIND_FACTOR_REUSE)
    {
        VLONG_freeVlong( &pBH->pR1, ppVlongQueue);
        VLONG_freeVlong( &pBH->pRE, ppVlongQueue);
    }

    if ( !pBH->pR1 || !pBH->pRE)
    {
        if (OK > ( status = RSAINT_initBlindingFactors( MOC_MOD(hwAccelCtx)
                                                    pRSAKeyInt,
                                                    &pBH->pRE, &pBH->pR1,
                                                    rngFun, rngFunArg,
                                                    ppVlongQueue)))
        {
            goto release_mutex;
        }
        /* reset the counter */
        pBH->counter = 0;
    }
    else
    {
        ++(pBH->counter); /* increment the counter */
    }

    if (OK > (status = VLONG_allocVlong(&product, ppVlongQueue)))
        goto release_mutex;

    DEBUG_RELABEL_MEMORY(product);

    if (OK > (status = VLONG_vlongSignedMultiply( product, pBH->pRE, pCipher)))
        goto release_mutex;

    /* blinded is the blinded cipher text */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx)
                                                     product,
                                                     RSA_N(pRSAKeyInt),
                                                     &blinded,
                                                     ppVlongQueue)))
    {
        goto release_mutex;
    }
    /* savedR1 is a copy of the blinding inverse os we can release the mutex early */
    if (OK > (status = VLONG_makeVlongFromVlong( pBH->pR1, &savedR1, ppVlongQueue)))
        goto release_mutex;

    /* square both blinding factors -- note that if it fails in the middle, the blinding
    factors will be out of sync and all decryption will fail after that !!! */
    if (OK > ( VLONG_vlongSignedSquare( product, pBH->pRE)))
    {
        goto release_mutex;
    }

    VLONG_freeVlong(&pBH->pRE, ppVlongQueue);
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) product,
                                                     RSA_N(pRSAKeyInt),
                                                     &pBH->pRE,
                                                     ppVlongQueue)))
    {
        goto release_mutex;
    }

    if (OK > ( VLONG_vlongSignedSquare( product, pBH->pR1)))
    {
        goto release_mutex;
    }

    VLONG_freeVlong(&pBH->pR1, ppVlongQueue);
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx)
                                                     product,
                                                     RSA_N(pRSAKeyInt),
                                                     &pBH->pR1,
                                                     ppVlongQueue)))
    {
        goto release_mutex;
    }

release_mutex:
    RTOS_mutexRelease( pBH->blindingMutex);
    if (OK > status) /* there was an error i.e. we jumped to release_mutex */
    {
        goto exit;
    }

#else  /* __PSOS_RTOS__ -> no mutex */

    if (OK > ( status = RSAINT_initBlindingFactors( MOC_MOD(hwAccelCtx)
                                                pRSAKeyInt,
                                                &blinded, &savedR1,
                                                rngFun, rngFunArg,
                                                ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_allocVlong(&product, ppVlongQueue)))
        goto exit;

    if (OK > ( status = VLONG_vlongSignedMultiply( product, blinded, pCipher)))
        goto exit;

    VLONG_freeVlong(&blinded, ppVlongQueue);
    /* blinded is now the blinded cipher text */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx)
                                                     product,
                                                     RSA_N(pRSAKeyInt),
                                                     &blinded,
                                                     ppVlongQueue)))
    {
        goto exit;
    }

    /* product -> allocated, can be disposed of
        blinded is blinded cipher text
        savedR1 is inverse blinding factor */

#endif

    VLONG_freeVlong( &product, ppVlongQueue);
    /* call the normal routine */
    if (OK > (status = RSAINT_decryptAux( MOC_RSA(hwAccelCtx) pRSAKeyInt,
                                          blinded, &product, ppVlongQueue)))
    {
        goto exit;
    }

    /* unblind with savedR1 */
    if (OK > ( status = VLONG_vlongSignedMultiply( blinded, product, savedR1)))
        goto exit;

    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx)
                                                     blinded,
                                                     RSA_N(pRSAKeyInt),
                                                     ppRetDecrypt,
                                                     ppVlongQueue)))
    {
        goto exit;
    }

exit:
    VLONG_freeVlong(&product, ppVlongQueue);
    VLONG_freeVlong(&blinded, ppVlongQueue);
    VLONG_freeVlong(&savedR1, ppVlongQueue);

    return status;
#endif  /* __CUSTOM_RSA_BLINDING__ */
} /* RSAINT_decrypt */
#endif /* __RSAINT_HARDWARE__ */


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_createKey(RSAKey **pp_RetRSAKey)
{
    MSTATUS status = OK;
    RSAKey* pNewKey = 0;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */


    if (NULL == pp_RetRSAKey)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL == (pNewKey = MALLOC(sizeof(RSAKey))))
        status = ERR_MEM_ALLOC_FAIL;
    else
        status = MOC_MEMSET((ubyte *)(pNewKey), 0x00, sizeof(RSAKey));

#if !defined( __PSOS_RTOS__) && !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
    if (OK > status)
    {
        goto exit;
    }

    if (OK > ( status = RTOS_mutexCreate( &(pNewKey->blinding.blindingMutex),
                                          0,0)))
    {
        goto exit;
    }
#endif

    *pp_RetRSAKey = pNewKey;
    pNewKey = 0;

exit:

    if ( pNewKey)
    {
        FREE( pNewKey);
    }

    return status;
}


/*------------------------------------------------------------------*/

static void
RSA_clearKey( RSAKey* pRSAKey, vlong **ppVlongQueue)
{
    sbyte4 i;

    for (i = 0; i < NUM_RSA_VLONG; ++i)
    {
        VLONG_freeVlong(&(pRSAKey->v[i]), ppVlongQueue);
    }
    for (i = 0; i < NUM_RSA_MODEXP; ++i)
    {
        VLONG_deleteModExpHelper( &pRSAKey->modExp[i], ppVlongQueue);
    }

#if !defined( __PSOS_RTOS__) && !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
    /* free the blinding factors */
    VLONG_freeVlong( &pRSAKey->blinding.pR1, ppVlongQueue);
    VLONG_freeVlong( &pRSAKey->blinding.pRE, ppVlongQueue);
#endif

}


/*--------------------------------------------------------------------------*/

extern MSTATUS
RSA_cloneKey( RSAKey** ppNew, const RSAKey* pSrc, vlong **ppVlongQueue)
{
    MSTATUS status;
    RSAKey* pNew = 0;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!ppNew || !pSrc)
    {
        return ERR_NULL_POINTER;
    }

    if (OK > ( status = RSA_createKey( &pNew)))
    {
        return status;
    }

    pNew->keyType = pSrc->keyType;
    if ( OK > ( status = VLONG_makeVlongFromVlong( RSA_N(pSrc), &RSA_N(pNew), ppVlongQueue)))
    {
        goto exit;
    }

    DEBUG_RELABEL_MEMORY(RSA_N(pNew));

    if ( OK > ( status = VLONG_makeVlongFromVlong( RSA_E(pSrc), &RSA_E(pNew), ppVlongQueue)))
    {
        goto exit;
    }

    DEBUG_RELABEL_MEMORY(RSA_E(pNew));

    if (RsaPrivateKey == pSrc->keyType)
    {
        sbyte4 i;

        if (!RSA_DP(pSrc) || !RSA_DQ(pSrc) || !RSA_QINV(pSrc) ||
            !RSA_MODEXP_P(pSrc) || !RSA_MODEXP_Q(pSrc))
        {
            status = ERR_RSA_KEY_NOT_READY;
            goto exit;
        }

        for (i = 2; i < NUM_RSA_VLONG; ++i)
        {
            if ( OK > ( status = VLONG_makeVlongFromVlong( pSrc->v[i], &pNew->v[i], ppVlongQueue)))
            {
                goto exit;
            }

            DEBUG_RELABEL_MEMORY(pNew->v[i]);
        }

        for (i = 0; i < NUM_RSA_MODEXP; ++i)
        {
            if ( OK > ( status = VLONG_makeModExpHelperFromModExpHelper(
                        pSrc->modExp[i], &pNew->modExp[i], ppVlongQueue)))
            {
                goto exit;
            }
        }
    }

#ifdef __ENABLE_MOCANA_HW_SECURITY_MODULE__
    if(RsaProtectedKey == pSrc->keyType)
    {
        pNew->signCallback = pSrc->signCallback;
        pNew->decryptCallback = pSrc->decryptCallback;
        pNew->validateCallback = pSrc->validateCallback;
        pNew->copyCookieCallback = pSrc->copyCookieCallback;
        pNew->freeCookieCallback = pSrc->freeCookieCallback;
        pNew->freeDataCallback = pSrc->freeDataCallback;
        
        if(NULL != pNew->copyCookieCallback)
        {
            pNew->copyCookieCallback(&pNew->pParams, pSrc->pParams);
        }
        else
        {
            /* assume static cookie */
            pNew->pParams = pSrc->pParams;
        }
    }
#endif

    /* OK */
    *ppNew = pNew;
    pNew = 0;

exit:

    if (pNew)
    {
        RSA_freeKey( &pNew, NULL);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_freeKey(RSAKey **ppFreeRSAKey, vlong **ppVlongQueue)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == ppFreeRSAKey) || (NULL == *ppFreeRSAKey))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    RSA_clearKey( *ppFreeRSAKey, ppVlongQueue);

#ifdef __ENABLE_MOCANA_HW_SECURITY_MODULE__
    if((*ppFreeRSAKey)->freeCookieCallback)
    {
        (*ppFreeRSAKey)->freeCookieCallback((*ppFreeRSAKey)->pParams);
    }
#endif

#if !defined( __PSOS_RTOS__) && !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
    RTOS_mutexFree(&((**ppFreeRSAKey).blinding.blindingMutex));
#endif

    FREE(*ppFreeRSAKey);
    *ppFreeRSAKey = NULL;

exit:
    return status;
}


/*------------------------------------------------------------------*/

#ifndef __DISABLE_RSA_KEY_EQUALITY_TEST__

extern MSTATUS
RSA_equalKey(const RSAKey *pKey1, const RSAKey *pKey2, byteBoolean* pResult)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == pKey1) || (NULL == pKey2) || (NULL == pResult))
        status = ERR_NULL_POINTER;
    else
    {
        /* only compare the public part */
        *pResult = FALSE;

        if ((0 == VLONG_compareSignedVlongs(RSA_E(pKey1), RSA_E(pKey2))) &&
            (0 == VLONG_compareSignedVlongs(RSA_N(pKey1), RSA_N(pKey2))))
        {
            *pResult = TRUE;
        }
    }

    return status;
}

#endif /* __DISABLE_RSA_KEY_EQUALITY_TEST__ */


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_setPublicKeyParameters(RSAKey *pKey, ubyte4 exponent,
                           const ubyte* modulus, ubyte4 modulusLen,
                           vlong **ppVlongQueue)
{
    /* use this to set the parameters of a public key */
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */


    if (NULL == pKey)
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }

    if (2 > exponent)
    {
        status = ERR_BAD_EXPONENT;
        goto exit;
    }

    RSA_clearKey( pKey, ppVlongQueue);

    if (OK > (status = VLONG_makeVlongFromUnsignedValue(exponent, &RSA_E(pKey),
                                                        ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_vlongFromByteString(modulus, modulusLen, &RSA_N(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }

    DEBUG_RELABEL_MEMORY(RSA_N(pKey));

    pKey->keyType = RsaPublicKey;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_prepareKey(MOC_RSA(hwAccelDescr hwAccelCtx)
               RSAKey *pRSAKey, vlong** ppVlongQueue)
{
    /* This precomputes some values used for the decrypt operation */
    MSTATUS status = OK;
    vlong* pm = 0;
    vlong* qm = 0;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!pRSAKey)
        return ERR_NULL_POINTER;

    if (!pRSAKey->keyType)
        return OK;

    /* make sure  p > q */
    if ( VLONG_compareSignedVlongs( RSA_P(pRSAKey), RSA_Q(pRSAKey) ) < 0)
    {
        vlong* tmp;
        tmp = RSA_P(pRSAKey);
        RSA_P(pRSAKey) = RSA_Q(pRSAKey);
        RSA_Q(pRSAKey) = tmp;
        VLONG_freeVlong(&RSA_DP(pRSAKey), ppVlongQueue);
        VLONG_freeVlong(&RSA_DQ(pRSAKey), ppVlongQueue);
        VLONG_freeVlong(&RSA_QINV(pRSAKey), ppVlongQueue);
        VLONG_deleteModExpHelper( &RSA_MODEXP_P(pRSAKey), ppVlongQueue);
        VLONG_deleteModExpHelper( &RSA_MODEXP_Q(pRSAKey), ppVlongQueue);
    }

    if ( !RSA_DP(pRSAKey))
    {
        /* pm = p - 1; */
        if (OK > (status = VLONG_makeVlongFromVlong(RSA_P(pRSAKey), &pm,
                                                    ppVlongQueue)))
        {
            goto exit;
        }

        DEBUG_RELABEL_MEMORY(pm);

        if (OK > (status = VLONG_decrement(pm, ppVlongQueue)))
            goto exit;

        /* dP = e^-1 mod pm */
        if (OK > ( status = VLONG_modularInverse( MOC_MOD( hwAccelCtx)
                                                  RSA_E(pRSAKey), pm,
                                                  &RSA_DP(pRSAKey), ppVlongQueue)))
        {
            goto exit;
        }
    }

    if (!RSA_DQ(pRSAKey))
    {
        /* qm = q - vlong(1); */
        if (OK > (status = VLONG_makeVlongFromVlong(RSA_Q(pRSAKey), &qm,
                                                    ppVlongQueue)))
        {
            goto exit;
        }

        DEBUG_RELABEL_MEMORY(qm);

        if (OK > (status = VLONG_decrement(qm, ppVlongQueue)))
            goto exit;

        /* dQ = e^-1 mod qm */
        if (OK > ( status = VLONG_modularInverse( MOC_MOD(hwAccelCtx)
                                                  RSA_E(pRSAKey), qm,
                                                  &RSA_DQ(pRSAKey), ppVlongQueue)))
        {
            goto exit;
        }
    }

    if (!RSA_QINV(pRSAKey))
    {
        /* qInv = q^ -1 mod p */
        if ( OK > ( status = VLONG_modularInverse( MOC_MOD( hwAccelCtx)
                                                   RSA_Q(pRSAKey), RSA_P(pRSAKey),
                                                   &RSA_QINV(pRSAKey),
                                                   ppVlongQueue)))
        {
            goto exit;
        }
    }

    if (!RSA_MODEXP_P(pRSAKey) )
    {
        if (OK > ( status = VLONG_newModExpHelper(MOC_MOD(hwAccelCtx) &RSA_MODEXP_P(pRSAKey),
                                                   RSA_P(pRSAKey), ppVlongQueue)))
        {
            goto exit;
        }
    }

    if (!RSA_MODEXP_Q(pRSAKey) )
    {
        if (OK > ( status = VLONG_newModExpHelper(MOC_MOD(hwAccelCtx)
                                        &RSA_MODEXP_Q(pRSAKey),
                                        RSA_Q(pRSAKey), ppVlongQueue)))
        {
            goto exit;
        }
    }

exit:
    VLONG_freeVlong(&pm, ppVlongQueue);
    VLONG_freeVlong(&qm, ppVlongQueue);

    return status;
}


/*------------------------------------------------------------------*/

#if defined( __ENABLE_ALL_TESTS__) || !defined(__DISABLE_PKCS1_KEY_READ__)

MSTATUS RSA_setAllKeyParameters(MOC_RSA(hwAccelDescr hwAccelCtx)
                                RSAKey *pKey,
                                ubyte4 exponent,
                                const ubyte* modulus,
                                ubyte4 modulusLen,
                                const ubyte* prime1,
                                ubyte4 prime1Len,
                                const ubyte* prime2,
                                ubyte4 prime2Len,
                                vlong **ppVlongQueue)
{
    /* use this to set the parameters of a private key */
    MSTATUS status;

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    status = RSA_setPublicKeyParameters(pKey, exponent, modulus, modulusLen,
                                        ppVlongQueue);
    if (OK == status)
    {
        if (OK > (status = VLONG_vlongFromByteString(prime1, prime1Len,
                                                     &RSA_P(pKey), ppVlongQueue)))
        {
            goto exit;
        }

        DEBUG_RELABEL_MEMORY(RSA_P(pKey));

        if (OK > (status = VLONG_vlongFromByteString(prime2, prime2Len,
                                                     &RSA_Q(pKey), ppVlongQueue)))
        {
            goto exit;
        }

        DEBUG_RELABEL_MEMORY(RSA_Q(pKey));

        pKey->keyType = RsaPrivateKey;

        if (OK > ( status = RSA_prepareKey(MOC_MOD(hwAccelCtx)
                                           pKey, ppVlongQueue)))
            goto exit;
    }

exit:
    return status;
}

#endif /* __ENABLE_ALL_TESTS__ || !__DISABLE_PKCS1_KEY_READ__ */


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_getCipherTextLength(const RSAKey *pKey, sbyte4* cipherTextLen)
{

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == pKey) || (NULL == cipherTextLen) || (NULL == RSA_N(pKey)))
    {
        return ERR_NULL_POINTER;
    }

    return VLONG_byteStringFromVlong( RSA_N(pKey), NULL, cipherTextLen);
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_ALL_TESTS__) || (!defined(__DISABLE_MOCANA_RSA_CLIENT_CODE__)))

extern MSTATUS
RSA_encrypt(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pKey,
            const ubyte* plainText, ubyte4 plainTextLen, ubyte* cipherText,
            RNGFun rngFun, void* rngFunArg, vlong **ppVlongQueue)
{
    /* encrypt the plainText using PKCS#1 scheme */
    sbyte4  keyLen;
    sbyte4  i;
    vlong*  pPkcs1       = NULL;
    vlong*  pEncrypted   = NULL;
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == pKey)
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }

    if ( 0 == plainText || 0 == cipherText || 0 == rngFun)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = VLONG_byteStringFromVlong(RSA_N(pKey), NULL, &keyLen)))
        goto exit;

    if (keyLen < (sbyte4) (plainTextLen+3+8)) /* padding must be at least 8 chars long */
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }

    cipherText[0] = 0;
    cipherText[1] = 2;

    (*rngFun)(rngFunArg, (keyLen - 3) - plainTextLen, cipherText + 2);

    for(i = 2; i < (sbyte4) (keyLen - plainTextLen) - 1; ++i)
    {
        if (0 == cipherText[i])
        {
            cipherText[i] = 1;
        }
    }

    cipherText[(keyLen - plainTextLen) - 1] = 0;

    MOC_MEMCPY((cipherText + keyLen) - plainTextLen, plainText, plainTextLen);

    if (OK > (status = VLONG_vlongFromByteString(cipherText, keyLen, &pPkcs1,
                                                 ppVlongQueue)))
    {
        goto exit;
    }

    DEBUG_RELABEL_MEMORY(pPkcs1);

    if (OK > (status = RSAINT_encrypt(MOC_RSA(hwAccelCtx) pKey, pPkcs1,
                                      &pEncrypted, ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_byteStringFromVlong(pEncrypted, cipherText,
                                                 &keyLen)))
    {
        goto exit;
    }

exit:
    VLONG_freeVlong(&pEncrypted, ppVlongQueue);
    VLONG_freeVlong(&pPkcs1, ppVlongQueue);

    return status;

} /* RSA_encrypt */

#endif /* (defined(__ENABLE_ALL_TESTS__) || (!defined(__DISABLE_MOCANA_RSA_CLIENT_CODE__))) */


/*------------------------------------------------------------------*/
#if !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
extern MSTATUS
RSA_decrypt(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pKey,
            const ubyte* cipherText, ubyte* plainText, ubyte4* plainTextLen,
            RNGFun rngFun, void* rngFunArg, vlong **ppVlongQueue)
{
    sbyte4  keyLen = 0, decryptedLen = 0;
    sbyte4  i;
    vlong*  pPkcs1     = NULL;
    vlong*  pEncrypted = NULL;
    sbyte4  realLen    = 0;
    MSTATUS status;
#if defined(__ENABLE_MOCANA_PKCS11_CRYPTO__) || defined(__ENABLE_MOCANA_HW_SECURITY_MODULE__)
    sbyte4  cipherTextLen = 0;
#endif

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == pKey)
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }

    if ((NULL == plainText) || (NULL == cipherText) || (NULL == plainTextLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (FALSE == pKey->keyType)
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }
#if defined(__ENABLE_MOCANA_PKCS11_CRYPTO__) || defined(__ENABLE_MOCANA_HW_SECURITY_MODULE__)
    if(RsaProtectedKey != pKey->keyType)
    {
        if (OK > (status = RSA_getCipherTextLength(pKey, &cipherTextLen)))
        {
            goto exit;
        }
    }
#endif

#ifdef __ENABLE_MOCANA_PKCS11_CRYPTO__
    /* C_Decrypt is for single part decryption, meaning no loop is required
     * pMechanism is fixed to NULL upon BAI's request
     *
     */
    if (OK != (status = RSAINT_decrypt((CK_SESSION_HANDLE)hwAccelCtx,
                       NULL,
                                       (CK_OBJECT_HANDLE)pKey,
                       (CK_BYTE_PTR)cipherText,
                                       (CK_ULONG)cipherTextLen,
                                       (CK_BYTE_PTR)plainText,
                       (CK_ULONG_PTR)&decryptedLen)))
    {
        goto exit;
    }

#else    /* regular case */

#ifdef __ENABLE_MOCANA_HW_SECURITY_MODULE__
    if(pKey->keyType == RsaProtectedKey)
    {
        ubyte* pDecrypted = NULL;
        ubyte4 decryptedLen;

        if (NULL == pKey->decryptCallback)
        {
            status = ERR_RSA_DECRYPT_CALLBACK_MISSING;
            goto exit;
        }

        if (OK > (status = pKey->decryptCallback(pKey->pParams, cipherText, cipherTextLen, &pDecrypted, &decryptedLen)))
        {
            goto exit;
        }

        if (NULL != pDecrypted)
        {
            MOC_MEMCPY(plainText, pDecrypted, decryptedLen);
            *plainTextLen = decryptedLen;
            if(NULL != pKey->freeDataCallback)
            {
                pKey->freeDataCallback(pDecrypted);
            }
        }
        else
        {
            status = ERR_RSA_DECRYPT_CALLBACK_FAIL;
        }

        /* note: signedLen should equal keyLen, or something went wrong */

        goto exit;
    }
#endif

    if (OK > (status = VLONG_byteStringFromVlong(RSA_N(pKey), NULL, &keyLen)))
        goto exit;

    if (OK > (status = VLONG_vlongFromByteString(cipherText, keyLen, &pEncrypted, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pEncrypted);

    if (OK > (status = RSAINT_decrypt(MOC_RSA(hwAccelCtx) pKey, pEncrypted, rngFun, rngFunArg,
                                        &pPkcs1, ppVlongQueue)))
    {
        goto exit;
    }

    decryptedLen = keyLen;
    if (OK > (status = VLONG_byteStringFromVlong(pPkcs1, plainText, &decryptedLen)))
        goto exit;



    /* plaintext contains actually the whole pkcs1 */
    /* some verifications */
    i = 2;

    if ((plainText[0] != 0) || (plainText[1] != 2) || decryptedLen != keyLen)
    {
        /* The leading 0 might have been trimmed */
        if (2 == plainText[0] && decryptedLen == keyLen - 1 )
        {
            i = 1;
        }
        else
        {
            status = ERR_RSA_DECRYPTION;
            goto exit;
        }
    }

    /* scan until the first 0 byte */
    for (; i < decryptedLen; ++i)
    {
        if (0 == plainText[i])
        {
            break;
        }
    }

    /*    0    1    2    3    4    5    6    7    8    9    10 */
    /*    0    2    1    2    3    4    5    6    7    8    0 */
    if ( i < 10) /* padding must be at least 8 bytes */
    {
        status = ERR_RSA_DECRYPTION;
        goto exit;
    }

    for ( ++i; i < decryptedLen; ++i, ++realLen)
    {
        plainText[realLen] = plainText[i];
    }

    *plainTextLen = realLen;

#endif    /* __ENABLE_MOCANA_PKCS11_CRYPTO__ */

exit:
#ifndef __ENABLE_MOCANA_PKCS11_CRYPTO__
    VLONG_freeVlong(&pPkcs1, ppVlongQueue);
    VLONG_freeVlong(&pEncrypted, ppVlongQueue);
#endif
    return status;

} /* RSA_decrypt */
#endif /* !__DISABLE_MOCANA_RSA_DECRYPTION__ */

/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RSA_VERIFY__

extern MSTATUS
RSA_verifySignature(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pKey, const ubyte* cipherText,
                    ubyte* plainText, ubyte4* plainTextLen, vlong **ppVlongQueue)
{
    /* decrypt the cipherText using PKCS#1 scheme */
    sbyte4  keyLen;
    sbyte4  i;
    vlong*  pPkcs1     = NULL;
    vlong*  pEncrypted = NULL;
    sbyte4  realLen     = 0;
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == pKey)
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }

    if ((NULL == plainText) || (NULL == cipherText) || (NULL == plainTextLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = VLONG_byteStringFromVlong(RSA_N(pKey), NULL, &keyLen)))
        goto exit;

    if (MOCANA_MAX_MODULUS_SIZE < keyLen)
    {
        status = ERR_RSA_INVALID_MODULUS;
        goto exit;
    }

    if (!RSA_E(pKey) || (1 < RSA_E(pKey)->numUnitsUsed))
    {
        status = ERR_RSA_INVALID_EXPONENT;
        goto exit;
    }

    if (OK > (status = VLONG_vlongFromByteString(cipherText, keyLen, &pEncrypted, ppVlongQueue)))
        goto exit;

    if (OK > (status = RSAINT_encrypt(MOC_RSA(hwAccelCtx) pKey, pEncrypted, &pPkcs1, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_byteStringFromVlong(pPkcs1, plainText, &keyLen)))
        goto exit;

    /* plaintext contains actually the whole pkcs1 */

    /* some verifications */
    if (((plainText[0] != 0) || (plainText[1] != 1)) && (plainText[0] != 1))
    {
        status = ERR_RSA_DECRYPTION;
        goto exit;
    }

    /* scan until the first 0 byte */
    for (i = 2; i < keyLen; ++i)
    {
        if (0 == plainText[i])
        {
            break;
        }
        else if (0xff != plainText[i])
        {
            status = ERR_RSA_DECRYPTION;
            goto exit;
        }
    }
    /*    0    1    2    3    4    5    6    7    8    9    10 */
    /*    0    2    1    2    3    4    5    6    7    8    0 */
    if (10 > i) /* padding must be at least 8 bytes */
    {
        status = ERR_RSA_DECRYPTION;
        goto exit;
    }

    for (++i; i < keyLen; ++i, ++realLen)
    {
        plainText[realLen] = plainText[i];
    }

    *plainTextLen = realLen;

exit:
    VLONG_freeVlong(&pPkcs1, ppVlongQueue);
    VLONG_freeVlong(&pEncrypted, ppVlongQueue);

    return status;

} /* RSA_verifySignature */

#endif /* __DISABLE_MOCANA_RSA_VERIFY_CERTIFICATE__ */


/*------------------------------------------------------------------*/
/* added __RSAINT_HARDWARE__ */
#if !defined( __DISABLE_MOCANA_RSA_SIGN__) && !defined(__DISABLE_MOCANA_RSA_DECRYPTION__) && !defined(__ENABLE_MOCANA_PKCS11_CRYPTO__)

extern MSTATUS
RSA_signMessage(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pKey,
                const ubyte* plainText, ubyte4 plainTextLen,
                ubyte* cipherText, vlong **ppVlongQueue)
{
    /* encrypt the plainText using PKCS#1 scheme */
    vlong*  pPkcs1      = NULL;
    vlong*  pEncrypted  = NULL;
    vlong*  pVerify     = NULL;
    sbyte4  keyLen;
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == pKey) || (NULL == plainText) || (NULL == cipherText))
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }

    /* If the RSA key is generated by a security module, we will find the key length in
       the RSA key's signCallback */
    if(RsaProtectedKey != pKey->keyType)
    {
        /* If the RSA key is a Mocana generated key, then find the key length as usual */
        if (OK > (status = VLONG_byteStringFromVlong(RSA_N(pKey), NULL, &keyLen)))
            goto exit;
    }
    
#ifndef __ENABLE_MOCANA_RSA_ALL_KEYSIZE__
    /* if the key is not 2048 or 3072 bits long, fail */
    if((256 != keyLen) && (384 != keyLen))
    {
        status = ERR_RSA_UNSUPPORTED_KEY_LENGTH;
        goto exit;
    }
#endif

    if (keyLen < (sbyte4) (plainTextLen+3+8)) /* padding must be at least 8 chars long */
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }

    cipherText[0] = 0;
    cipherText[1] = 1;

    MOC_MEMSET(cipherText + 2, 0xff, (keyLen - 3) - plainTextLen);

    cipherText[(keyLen - plainTextLen) - 1] = 0;

    MOC_MEMCPY((cipherText + keyLen) - plainTextLen, plainText, plainTextLen);

#ifdef __ENABLE_MOCANA_HW_SECURITY_MODULE__
    if(pKey->keyType == RsaProtectedKey)
    {
        ubyte* pSignature = NULL;
        ubyte4 signedLen;

        if(NULL == pKey->signCallback)
        {
            status = ERR_RSA_SIGN_CALLBACK_MISSING;
            goto exit;
        }

        if(OK > (status = pKey->signCallback(pKey->pParams, plainText, plainTextLen, &pSignature, &signedLen)))
        {
            goto exit;
        }

        if(NULL != pSignature)
        {
            MOC_MEMCPY(cipherText, pSignature, signedLen);
            if(NULL != pKey->freeDataCallback)
            {
                pKey->freeDataCallback(pSignature);
            }
        }
        else
        {
            status = ERR_RSA_SIGN_CALLBACK_FAIL;
        }

        /* note: signedLen should equal keyLen, or something went wrong */

        goto exit;
    }
    else
#endif
    {

        if (OK > (status = VLONG_vlongFromByteString(cipherText, keyLen, &pPkcs1, ppVlongQueue)))
            goto exit;

        if (OK > (status = RSAINT_decrypt(MOC_RSA(hwAccelCtx) pKey, pPkcs1, NULL, NULL, &pEncrypted, ppVlongQueue)))
            goto exit;

#ifdef __ENABLE_MOCANA_VERIFY_RSA_SIGNATURE__
        /* verify the signature -- note that this will significantly decrease the performance (about 25 %) */
        if ( OK > ( status = RSAINT_encrypt( MOC_RSA(hwAccelCtx) pKey, pEncrypted, &pVerify, ppVlongQueue)))
            goto exit;

        if ( VLONG_compareSignedVlongs( pPkcs1, pVerify) )
        {
            VLONG_freeVlong(&pEncrypted, ppVlongQueue);
            /* RSA-CRT failed because of a random or hardware error --- don't send back wrong result since it
                allows to recover the private key cf. http://theory.stanford.edu/~dabo/papers/faults.ps.gz */
            if ( OK > ( status = RSAINT_decryptLong( MOC_MOD(hwAccelCtx) pKey, pPkcs1, &pEncrypted, ppVlongQueue)))
            goto exit;
        }
#endif
        if (OK > (status = VLONG_byteStringFromVlong(pEncrypted, cipherText, &keyLen)))
            goto exit;
    }

exit:

    VLONG_freeVlong(&pEncrypted, ppVlongQueue);
    VLONG_freeVlong(&pPkcs1, ppVlongQueue);
    VLONG_freeVlong(&pVerify, ppVlongQueue);

    return status;

} /* RSA_signMessage */

#endif /* __DISABLE_MOCANA_RSA_SIGN__  __DISABLE_MOCANA_RSA_DECRYPTION__ */


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_keyFromByteString(MOC_RSA(hwAccelDescr hwAccelCtx)
                      RSAKey **ppKey, const ubyte* byteString, ubyte4 len,
                      vlong** ppVlongQueue)
{
    /* format is version + {length (4 bytes big-endian) - bytes} for each
      e, n, p, q, dP, dQ, qInv */
    MSTATUS status = ERR_BAD_KEY_BLOB;
    sbyte4 numVlong;
    ubyte4 partLen;
    sbyte4 i;
    RSAKey* pNewKey = 0;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!byteString || !ppKey)
        return ERR_NULL_POINTER;

    if (len < 2 )
        return ERR_BAD_KEY_BLOB;

    /* support version 1 (with MontgomeryCtx) and 2 (without MontgomeryCtx) */
    if (*byteString++ > RSA_BLOB_VERSION)
        return ERR_BAD_KEY_BLOB_VERSION;
    len--;

    if (OK > (status = RSA_createKey( &pNewKey)))
        return status;

    pNewKey->keyType = (*byteString++) ? RsaPrivateKey : RsaPublicKey;
    len--;

    if (pNewKey->keyType)
    {
        numVlong = NUM_RSA_VLONG;
    }
    else
    {
        numVlong = 2;
    }

    /* read each v */
    for ( i = 0; i < numVlong; ++i)
    {
        if (len < sizeof(ubyte4))
        {
            status = ERR_BAD_KEY_BLOB;
            goto exit;
        }

        partLen = ((ubyte4)byteString[0] << 24) +
            ((ubyte4)byteString[1] << 16) +
            ((ubyte4)byteString[2] << 8) +
            ((ubyte4)byteString[3]);
        byteString += 4;
        len -= 4;
        if (len < partLen)
        {
            status = ERR_BAD_KEY_BLOB;
            goto exit;
        }

        if (OK > ( status = VLONG_vlongFromByteString( byteString, partLen,
                                    pNewKey->v+i, ppVlongQueue)))
        {
            goto exit;
        }
        byteString += partLen;
        len -= partLen;
    }

    if (pNewKey->keyType)
    {
        if (OK > (status = RSA_prepareKey(MOC_RSA(hwAccelCtx)
               pNewKey, ppVlongQueue)))
        {
            goto exit;
        }
    }

    *ppKey = pNewKey;
    pNewKey = 0;
    status = OK;

exit:

    if (pNewKey)
    {
        RSA_freeKey( &pNewKey, ppVlongQueue);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_byteStringFromKey(MOC_RSA(hwAccelDescr hwAccelCtx)
                      const RSAKey *pKey, ubyte* pBuffer, ubyte4 *pRetLen)
{
    MSTATUS status;
    sbyte4  i, numVlong;
    ubyte4  vlongLen[NUM_RSA_VLONG];
    ubyte4  requiredLen;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!pKey || !pRetLen)
    {
        return ERR_NULL_POINTER;
    }

    if (OK > (status = RSA_prepareKey(MOC_MOD(hwAccelCtx) (RSAKey*) pKey, 0)))
        return status;

    if (RsaPrivateKey == pKey->keyType)
    {
        numVlong = NUM_RSA_VLONG;
    }
    else
    {
        numVlong = 2;
    }

    requiredLen = 2 + (numVlong ) * sizeof(ubyte4);

    for ( i = 0; i < numVlong; ++i)
    {
        if (OK > ( status = VLONG_byteStringFromVlong(pKey->v[i], NULL, (sbyte4 *)vlongLen+i)))
            return status;
        requiredLen += vlongLen[i];
    }

    if (pBuffer)
    {
        if ( *pRetLen >= requiredLen)
        {
            *pBuffer++ = RSA_BLOB_VERSION;
            *pBuffer++ = (pKey->keyType) ? 1 : 0;

            for ( i = 0; i < numVlong; ++i)
            {
                BIGEND32( pBuffer, vlongLen[i]);
                pBuffer += sizeof(ubyte4);

                if (OK > ( status = VLONG_byteStringFromVlong(pKey->v[i], pBuffer, (sbyte4 *)vlongLen + i)))
                    return status;
                pBuffer += vlongLen[i];
            }
        }
        else
        {
            status = ERR_BUFFER_OVERFLOW;
        }
    }
    *pRetLen = requiredLen;

    return status;
}


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_KEY_GENERATION__
static MSTATUS
incrementalPrimeFind(MOC_PRIME(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                     vlong *pBase, vlong **ppPrime, vlong **ppVlongQueue)
{
    vlong*      pPrime = NULL;
    intBoolean  isPrime = FALSE;
    MSTATUS     status;

    /* find the first prime after the previous number */
    if (OK > (status = VLONG_makeVlongFromVlong(pBase, &pPrime, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pPrime);

    if (FALSE == VLONG_isVlongBitSet(pPrime, 0))
    {
        /* if even number, decrement by one in order to move to next odd number */
        VLONG_decrement(pPrime, ppVlongQueue);
    }

    while (1)
    {
        /* DO THIS FIRST: move to next odd number per spec */
        if (OK > (status = VLONG_addImmediate(pPrime, 2, ppVlongQueue)))
            goto exit;

        if (OK > (status = PRIME_doPrimeTestsEx(MOC_MOD(hwAccelCtx) pRandomContext, pPrime, prime_RSA, &isPrime, ppVlongQueue)))
            goto exit;

        if (TRUE == isPrime)
            break;
    }

    *ppPrime = pPrime;
    pPrime = NULL;

exit:
    VLONG_freeVlong(&pPrime, ppVlongQueue);

    return status;
}
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_KEY_GENERATION__
static MSTATUS
applyCRT(MOC_RSA(hwAccelDescr hwAccelCtx) vlong *v1, vlong *v2, vlong *Xv, vlong **ppY,
         vlong **ppScalar, vlong **ppVlongQueue)
{
    vlong*  pT1     = NULL;     /* (p2^-1 mod p1), (p1^-1 mod p2), Xp mod p1p2 */
    vlong*  R       = NULL;     /* R1 = (p2^-1 mod p1)p2 - (p1^-1 mod p2)p1 */
    vlong*  Y       = NULL;
    vlong*  Ry      = NULL;     /* (p1^-1 mod p2)p1 */
    vlong*  v1v2    = NULL;     /* p1p2 */
    MSTATUS status;

    /* make temp variables */
    if (OK > (status = VLONG_allocVlong(&Ry, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(Ry);

    if (OK > (status = VLONG_allocVlong(&R, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(R);

    if (OK > (status = VLONG_allocVlong(&v1v2, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(v1v2);

    /* Compute: R1 = (p2^-1 mod p1)p2 - (p1^-1 mod p2)p1 */
    /* p2^-1 mod p1 */
    if (OK > (status = VLONG_modularInverse(MOC_MOD(hwAccelCtx) v2, v1, &pT1, ppVlongQueue)))
        goto exit;

    /* R = (p2^-1 mod p1)p2 */
    if (OK > (status = VLONG_vlongSignedMultiply(R, pT1, v2)))
        goto exit;

    /* free temp */
    VLONG_freeVlong(&pT1, ppVlongQueue);

    if (OK > (status = VLONG_modularInverse(MOC_MOD(hwAccelCtx) v1, v2, &pT1, ppVlongQueue)))
        goto exit;

    /* Ry = (p1^-1 mod p2)p1 */
    if (OK > (status = VLONG_vlongSignedMultiply(Ry, pT1, v1)))
        goto exit;

    /* free temp */
    VLONG_freeVlong(&pT1, ppVlongQueue);

    /* Final result: R1 = (p2^-1 mod p1)p2 - (p1^-1 mod p2)p1 */
    if (OK > (status = VLONG_subtractSignedVlongs(R, Ry, ppVlongQueue)))
        goto exit;

    /* Compute: Yp0 = Xp + (R1 - Xp mod p1p2) */
    /* p1p2 */
    if (OK > (status = VLONG_vlongSignedMultiply(v1v2, v1, v2)))
        goto exit;

    /* Xp mod p1p2 */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) Xv, v1v2, &pT1, ppVlongQueue)))
        goto exit;

    /* (R1 - (Xp mod p1p2)) */
    if (OK > (status = VLONG_subtractSignedVlongs(R, pT1, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_makeVlongFromVlong(Xv, &Y, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(Y);

    /* Compute: Yp0 = Xp + (R1 - Xp mod p1p2) */
    if (OK > (status = VLONG_addSignedVlongs(Y, R, ppVlongQueue)))
        goto exit;

    /* return Yp0 / Yq0 */
    *ppY = Y;
    Y = NULL;
    *ppScalar = v1v2;
    v1v2 = NULL;

exit:
    VLONG_freeVlong(&Y, ppVlongQueue);
    VLONG_freeVlong(&pT1, ppVlongQueue);
    VLONG_freeVlong(&v1v2, ppVlongQueue);
    VLONG_freeVlong(&R, ppVlongQueue);
    VLONG_freeVlong(&Ry, ppVlongQueue);

    return status;

} /* applyCRT */

#endif /* __DISABLE_MOCANA_KEY_GENERATION__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_KEY_GENERATION__
static MSTATUS
findCandidate(MOC_PRIME(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
              vlong *Y, vlong *pScalar, vlong *e,
              vlong **ppPrime, vlong **ppVlongQueue)
{
    intBoolean  isPrime = FALSE;
    vlong*      pPrime = NULL;
    vlong*      gcd = NULL;
    MSTATUS     status;

    if (OK > (status = VLONG_makeVlongFromVlong(Y, &pPrime, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pPrime);

    if (FALSE == VLONG_isVlongBitSet(e, 0))
    {
        /* for even e, (8p1p2) */
        if (OK > (status = VLONG_shlXvlong(pScalar, 3)))
            goto exit;
    }

    /* if prime candidate is even, add scalar to make it odd */
    if (FALSE == VLONG_isVlongBitSet(pPrime, 0))
        if (OK > (status = VLONG_addSignedVlongs(pPrime, pScalar, ppVlongQueue)))
            goto exit;

    while (1)
    {
        VLONG_freeVlong(&gcd, ppVlongQueue);

        if (TRUE == VLONG_isVlongBitSet(e, 0))
        {
            /* e is odd */
            if (OK > (status = VLONG_decrement(pPrime, ppVlongQueue)))
                goto exit;

            if (OK > (status = VLONG_greatestCommonDenominator(MOC_MOD(hwAccelCtx) pPrime, e, &gcd, ppVlongQueue)))
                goto exit;

            if (OK > (status = VLONG_increment(pPrime, ppVlongQueue)))
                goto exit;
        }
        else
        {
            /* e is even */
            vlong_unit  saveUnit;       /* faster than cloning */

            saveUnit = VLONG_getVlongUnit(pPrime, 0);

            if (OK > (status = VLONG_decrement(pPrime, ppVlongQueue)))
                goto exit;

            if (OK > (status = VLONG_shrVlong(pPrime)))
                goto exit;

            if (OK > (status = VLONG_greatestCommonDenominator(MOC_MOD(hwAccelCtx) pPrime, e, &gcd, ppVlongQueue)))
                goto exit;

            if (OK > (status = VLONG_shlVlong(pPrime)))
                goto exit;

            if (OK > (status = VLONG_setVlongUnit(pPrime, 0, saveUnit)))
                goto exit;
        }

        if (0 == VLONG_compareUnsigned(gcd, 1))
        {
            /* only do prime test, if gcd == 1 */
            if (OK > (status = PRIME_doPrimeTestsEx(MOC_MOD(hwAccelCtx) pRandomContext, pPrime, prime_RSA, &isPrime, ppVlongQueue)))
                goto exit;

            /* did we find a prime?  */
            if (TRUE == isPrime)
                break;      /* YES! */
        }

        do
        {
            if (OK > (status = VLONG_addSignedVlongs(pPrime, pScalar, ppVlongQueue)))
                goto exit;

            /* loop to make sure our next prime candidate is odd */
        }
        while (FALSE == VLONG_isVlongBitSet(pPrime, 0));
    }

    *ppPrime = pPrime;
    pPrime = NULL;

exit:
    VLONG_freeVlong(&gcd, ppVlongQueue);
    VLONG_freeVlong(&pPrime, ppVlongQueue);

    return status;

} /* findCandidate */

#endif /* __DISABLE_MOCANA_KEY_GENERATION__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_KEY_GENERATION__
extern MSTATUS
RSA_generateKeyFipsSteps(MOC_RSA(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                         vlong *e, vlong *Xp, vlong *Xp1, vlong *Xp2,
                         intBoolean *doContinue, vlong **p, vlong **ppVlongQueue)
{
    /* FIPS only cares about steps 2b-4 */
    vlong*  p1 = NULL;
    vlong*  p2 = NULL;
    vlong*  Yp0 = NULL;
    vlong*  pScalarP = NULL;
    MSTATUS status;

    *doContinue = FALSE;

    /* search for prime numbers using sequential search (+2): p1, p2, q1, q2 */
    if (OK > (status = incrementalPrimeFind(MOC_PRIME(hwAccelCtx) pRandomContext, Xp1, &p1, ppVlongQueue)))
        goto exit;

    if (OK > (status = incrementalPrimeFind(MOC_PRIME(hwAccelCtx) pRandomContext, Xp2, &p2, ppVlongQueue)))
        goto exit;

    /* step 3: apply Chinese Remainder Theorem (twice) */
    if (OK > (status = applyCRT(MOC_RSA(hwAccelCtx) p1, p2, Xp, &Yp0, &pScalarP, ppVlongQueue)))
        goto exit;

    /* check Yp0 and pScalarP, (even + even) nevers equals prime candidate */
    if ((FALSE == VLONG_isVlongBitSet(Yp0, 0)) && (FALSE == VLONG_isVlongBitSet(pScalarP, 0)))
    {
        /* unfortunately, we have to start over */
        /* good news, this has never happen during testing, but jic keep it */
        *doContinue = TRUE;
    }

    /* step 4: check sequence of p and q candidates */
    if (OK > (status = findCandidate(MOC_PRIME(hwAccelCtx) pRandomContext, Yp0, pScalarP, e, p, ppVlongQueue)))
        goto exit;

exit:
    VLONG_freeVlong(&Yp0, ppVlongQueue);
    VLONG_freeVlong(&pScalarP, ppVlongQueue);
    VLONG_freeVlong(&p2, ppVlongQueue);
    VLONG_freeVlong(&p1, ppVlongQueue);

    return status;

} /* RSA_generateKeyFipsSteps */

#endif /* __DISABLE_MOCANA_KEY_GENERATION__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_KEY_GENERATION__
static MSTATUS
RSA_generateKeyEx(MOC_RSA(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                  RSAKey *p_rsaKey, ubyte4 keySize,
                  vlong **Xp, vlong **Xp1, vlong **Xp2,
                  vlong **Xq, vlong **Xq1, vlong **Xq2, vlong **ppVlongQueue)
{
    intBoolean  doContinue;
    vlong*      n   = NULL;
    vlong*      e   = NULL;
    vlong*      delta = NULL;
    vlong*      p   = NULL;
    vlong*      q   = NULL;
    ubyte4      pBits, qBits, s, auxBitLen;
    MSTATUS     status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == p_rsaKey)
    {
        status = ERR_RSA_INVALID_KEY;
        goto exit;
    }
#ifdef __ENABLE_MOCANA_RSA_ALL_KEYSIZE__

    keySize += 127;
    keySize &= 0xffffff80L;

    if (1024 > keySize)
    {
        status = ERR_RSA_KEY_LENGTH_TOO_SMALL;
        goto exit;
    }

#else
    /* key size must be 2048, or 3072 */
    if ((2048 != keySize) && (3072 != keySize))
    {
        status = ERR_RSA_UNSUPPORTED_KEY_LENGTH;
        goto exit;
    }
#endif

    if(2048 > keySize)
    {
        auxBitLen = 101;
    }
    else if(3072 > keySize)
    {
        auxBitLen = 141;
    }
    else
    {
        auxBitLen = 171;
    }

    /* bit length of p and q */
    pBits = keySize >> 1;
    qBits = keySize - pBits;

    /* s = number of 128-bits above 512 bits */
    s = ((pBits - 512) >> 7);

    /* e is predefined and meets the constraints */
    if (OK > (status = VLONG_makeVlongFromUnsignedValue(PREDEFINED_E, &e, ppVlongQueue)))
        goto exit;

    while (1)
    {
        /* to prevent memory leaks... */
        VLONG_freeVlong(&q, ppVlongQueue);
        VLONG_freeVlong(&p, ppVlongQueue);

        VLONG_freeVlong(Xq2, ppVlongQueue);
        VLONG_freeVlong(Xq1, ppVlongQueue);
        VLONG_freeVlong(Xp2, ppVlongQueue);
        VLONG_freeVlong(Xp1, ppVlongQueue);

        VLONG_freeVlong(&delta, ppVlongQueue);
        VLONG_freeVlong(Xq, ppVlongQueue);
        VLONG_freeVlong(Xp, ppVlongQueue);

        /* step 1a: generate Xp & Xq, a random number of pBits&qBits range { (sqrt(2) * 2^(bits-1)) .. (2^bits)-1 }  */
        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, Xp, pBits, ppVlongQueue)))
            goto exit;

        /* approximate sqrt(2) */
        if (FALSE == VLONG_isVlongBitSet(*Xp, pBits-2))  /* we adjust pBits from a length to an index */
        {
            /* set some bits to make sure we meet minimal threshold */
            if (OK > (status = VLONG_setVlongBit(*Xp, pBits-3)))     /* ditto (index) */
                goto exit;

            if (OK > (status = VLONG_setVlongBit(*Xp, pBits-4)))     /* ditto (index) */
                goto exit;
        }

        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, Xq, qBits, ppVlongQueue)))
            goto exit;

        /* approximate sqrt(2) */
        if (FALSE == VLONG_isVlongBitSet(*Xq, qBits-2))              /* we adjust qBits from a length to an index */
        {
            /* set some bits to make sure we meet minimal threshold */
            if (OK > (status = VLONG_setVlongBit(*Xq, qBits-3)))     /* ditto (index) */
                goto exit;

            if (OK > (status = VLONG_setVlongBit(*Xq, qBits-4)))     /* ditto (index) */
                goto exit;
        }

        /* step 1b: to prevent Fermat style factoring attacks, ensure |Xp - Xq| exceeds (2^(412+128s)) */
        if (OK > (status = VLONG_makeVlongFromVlong(*Xp, &delta, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_subtractSignedVlongs(delta, *Xq, ppVlongQueue)))
            goto exit;

        /* do we exceed (412+128s) bits?  */
        if (!((412 + (s * 128)) < VLONG_bitLength(delta)))
        {
            /* need to try again, delta to small to prevent Fermat style factoring attacks */
            continue;
        }

        /* Yes it's wasteful to free at the top of loop and intermix in the loop, */
        /* but this is critical code, which rarely runs.  We want to make sure */
        /* the code is safe from leaks.  Also, note the "continues" which could alter */
        /* the flow to create a memory leak.  Let's be safe!  */
        VLONG_freeVlong(&delta, ppVlongQueue);

        /* step 2: generate four 101-bit random odd numbers: Xp1, Xp2, Xq1, Xq2 */
        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, Xp1, auxBitLen, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, Xp2, auxBitLen, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, Xq1, auxBitLen, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, Xq2, auxBitLen, ppVlongQueue)))
            goto exit;

        /* do p */
        if (OK > (status = RSA_generateKeyFipsSteps(MOC_RSA(hwAccelCtx) pRandomContext,
                                                    e, *Xp, *Xp1, *Xp2, &doContinue,
                                                    &p, ppVlongQueue)))
        {
            goto exit;
        }

        if (TRUE == doContinue)
        {
            /* RSA_generateKeyFipsSteps() failed due to bad values, try again... */
            continue;
        }

        /* do q */
        if (OK > (status = RSA_generateKeyFipsSteps(MOC_RSA(hwAccelCtx) pRandomContext,
                                                    e, *Xq, *Xq1, *Xq2, &doContinue,
                                                    &q, ppVlongQueue)))
        {
            goto exit;
        }

        if (TRUE == doContinue)
        {
            /* RSA_generateKeyFipsSteps() failed due to bad values, try again... */
            continue;
        }

        /* we successfully generated our private key! */
        break;
    }

    /* small optimization later on when using private keys */
    if (VLONG_compareSignedVlongs(p, q) < 0)
    {
        vlong *swap = p;
        p = q;
        q = swap;
    }

    /* compute n */
    if (OK > (status = VLONG_allocVlong(&n, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(n);

    if (OK > (status = VLONG_vlongSignedMultiply(n, p, q)))
        goto exit;

    /* store results */
    p_rsaKey->keyType  = RsaPrivateKey;
    RSA_N(p_rsaKey) = n;    n = NULL;
    RSA_E(p_rsaKey) = e;    e = NULL;
    RSA_P(p_rsaKey) = p;    p = NULL;
    RSA_Q(p_rsaKey) = q;    q = NULL;

    status = RSA_prepareKey(MOC_MOD(hwAccelCtx) p_rsaKey, ppVlongQueue);

exit:
    VLONG_freeVlong(&n, ppVlongQueue);
    VLONG_freeVlong(&q, ppVlongQueue);
    VLONG_freeVlong(&p, ppVlongQueue);
    VLONG_freeVlong(&delta, ppVlongQueue);
    VLONG_freeVlong(&e, ppVlongQueue);

    return status;

} /* RSA_generateKeyEx */


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_generateKeyFIPS(MOC_RSA(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                RSAKey *p_rsaKey, ubyte4 keySize, vlong **Xp, vlong **Xp1, vlong **Xp2,
                vlong **Xq, vlong **Xq1, vlong **Xq2, vlong **ppVlongQueue)
{
    intBoolean  isFirstTime = TRUE;
    MSTATUS     status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    /* to ensure we produce keys of the correct bit length */
    do
    {
        if (FALSE == isFirstTime)
            RSA_clearKey(p_rsaKey, ppVlongQueue);

        if (OK > (status = RSA_generateKeyEx(MOC_RSA(hwAccelCtx) pRandomContext,
                                             p_rsaKey, keySize,
                                             Xp, Xp1, Xp2, Xq, Xq1, Xq2, ppVlongQueue)))
        {
            goto exit;
        }

        isFirstTime = FALSE;
    }
    while (keySize != VLONG_bitLength(RSA_N(p_rsaKey)));

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK > (status = RSA_generateKey_FIPS_consistancy_test(MOC_RSA(hwAccelCtx) p_rsaKey)))
        goto exit;
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

exit:
    return status;

}

/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_KEY_GENERATION__
extern MSTATUS
RSA_generateKey(MOC_RSA(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                RSAKey *p_rsaKey, ubyte4 keySize, vlong **ppVlongQueue)
{
    vlong*      Xp = NULL;
    vlong*      Xp1 = NULL;
    vlong*      Xp2 = NULL;
    vlong*      Xq = NULL;
    vlong*      Xq1 = NULL;
    vlong*      Xq2 = NULL;
    MSTATUS     status;

    status = RSA_generateKeyFIPS(MOC_RSA(hwAccelCtx) pRandomContext,
                p_rsaKey, keySize, &Xp, &Xp1, &Xp2, &Xq, &Xq1, &Xq2, ppVlongQueue);

    VLONG_freeVlong(&Xq2, ppVlongQueue);
    VLONG_freeVlong(&Xq1, ppVlongQueue);
    VLONG_freeVlong(&Xp2, ppVlongQueue);
    VLONG_freeVlong(&Xp1, ppVlongQueue);
    VLONG_freeVlong(&Xq, ppVlongQueue);
    VLONG_freeVlong(&Xp, ppVlongQueue);
    return status;

} /* RSA_generateKey */
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
extern MSTATUS
RSA_generateKey_FIPS_consistancy_test(MOC_RSA(sbyte4 hwAccelCtx) RSAKey* p_rsaKey)
{
    sbyte4  msgLen = 15;
    ubyte   msg[] = {
        'C', 'L', 'E', 'A', 'R', '_', 'T', 'E', 'X', 'T', '_', 'L', 'I', 'N', 'E'
    };
    ubyte*  pPlainText = NULL;
    ubyte*  pCipherText = NULL;
    sbyte4  cipherTextLen = 0;
    sbyte4  msgLenRet = 0;
    sbyte4  cmpRes = 0;
    MSTATUS status = OK;

    /* Get the cipher text length */
    if (OK > (status = RSA_getCipherTextLength( p_rsaKey, &cipherTextLen )))
        goto exit;

    /* Allocate memory for Cipher Text */
    pCipherText = MALLOC(cipherTextLen);

    if (NULL == pCipherText)
        goto exit;

    /* Allocate memory for plaintext */
    pPlainText = MALLOC(cipherTextLen);
    if (NULL == pPlainText)
        goto exit;

    if (OK > (status = RSA_signMessage(MOC_RSA(hwAccelCtx) p_rsaKey, msg, msgLen, pCipherText, NULL)))
        goto exit;

    if (OK > (status = RSA_verifySignature(MOC_RSA(hwAccelCtx) p_rsaKey, pCipherText, pPlainText, &msgLenRet, NULL)))
        goto exit;

    if (msgLen != msgLenRet)
    {
        status = ERR_FIPS_RSA_SIGN_VERIFY_FAIL;
        goto exit;
    }

    if (OK != MOC_MEMCMP(msg, pPlainText, msgLen, &cmpRes))
    {
        status = ERR_FIPS_RSA_SIGN_VERIFY_FAIL;
        goto exit;
    }

    if (0 != cmpRes)
    {
        status = ERR_FIPS_RSA_SIGN_VERIFY_FAIL;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL( DEBUG_CRYPTO, "RSA_generateKey_FIPS_consistancy_test: GOOD Signature Verify!" );
#endif

exit:
    if (NULL != pPlainText)
        FREE( pPlainText);

    if (NULL != pCipherText)
        FREE( pCipherText);

    return status;

} /* RSA_generateKey_FIPS_consistancy_test */

#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

#endif /* __DISABLE_MOCANA_KEY_GENERATION__ */


/*------------------------------------------------------------------*/

#if (!defined(__DISABLE_MOCANA_RSA_DECRYPTION__) && !defined(__RSAINT_HARDWARE__) && !defined(__ENABLE_MOCANA_PKCS11_CRYPTO__))
extern MSTATUS
RSA_RSASP1(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKey,
           const vlong *pMessage, RNGFun rngFun, void* rngFunArg, vlong **ppRetSignature, vlong **ppVlongQueue)
{
    return RSAINT_decrypt(MOC_RSA(hwAccelCtx) pRSAKey, pMessage, rngFun, rngFunArg, ppRetSignature, ppVlongQueue);
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_RSAVP1(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pPublicRSAKey,
           const vlong *pSignature, vlong **ppRetMessage, vlong **ppVlongQueue)
{
    return RSAINT_encrypt(MOC_RSA(hwAccelCtx) pPublicRSAKey, pSignature, ppRetMessage, ppVlongQueue);
}


/*------------------------------------------------------------------*/

#if !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
extern MSTATUS
RSA_RSADP(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pRSAKey,
          const vlong *pCipherText, vlong **ppMessage, vlong **ppVlongQueue)
{
    return RSAINT_decryptAux(MOC_RSA(hwAccelCtx) pRSAKey, pCipherText, ppMessage, ppVlongQueue);
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
RSA_RSAEP(MOC_RSA(hwAccelDescr hwAccelCtx) const RSAKey *pPublicRSAKey,
          const vlong *pMessage, vlong **ppRetCipherText, vlong **ppVlongQueue)
{
    return RSAINT_encrypt(MOC_RSA(hwAccelCtx) pPublicRSAKey, pMessage, ppRetCipherText, ppVlongQueue);
}

#endif /* __RSA_HARDWARE_ACCELERATOR__ */

