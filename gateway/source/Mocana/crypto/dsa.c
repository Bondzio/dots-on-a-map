/* Version: mss_v6_3 */
/*
 * dsa.c
 *
 * DSA Factory
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#if (defined(__ENABLE_MOCANA_DSA__))
#define __MOCANA_DSA_C_FILE__

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/debug_console.h"
#include "../common/memory_debug.h"
#include "../common/prime.h"
#include "../crypto/dsa.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif
#include "../harness/harness.h"

/*------------------------------------------------------------------*/

#define SIZEOF_DSA_Q            (SHA_HASH_RESULT_SIZE * 8)
#define MAX_DSA_ITERATIONS      4096


#ifdef __FIPS_OPS_TEST__
static int dsa_fail = 0;
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
/* prototype */
extern MSTATUS
DSA_generateKey_FIPS_consistancy_test(MOC_DSA(sbyte4 hwAccelCtx) randomContext* pFipsRngCtx, DSAKey* p_dsaDescr);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

/*------------------------------------------------------------------*/

#ifdef __FIPS_OPS_TEST__
extern void triggerDSAFail()
{
    dsa_fail = 1;
}
#endif


extern MSTATUS
DSA_createKey(DSAKey **pp_retDsaDescr)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == pp_retDsaDescr)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL == (*pp_retDsaDescr = MALLOC(sizeof(DSAKey))))
        status = ERR_MEM_ALLOC_FAIL;
    else
        MOC_MEMSET((ubyte *)(*pp_retDsaDescr), 0x00, sizeof(DSAKey));

exit:
    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
DSA_cloneKey(DSAKey** ppNew, const DSAKey* pSrc)
{
    DSAKey* pNew = NULL;
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == ppNew) || (NULL == pSrc))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > ( status = DSA_createKey(&pNew)))
        goto exit;

    if (OK > ( status = VLONG_makeVlongFromVlong(DSA_P(pSrc), &DSA_P(pNew), NULL)))
        goto exit;

    if (OK > ( status = VLONG_makeVlongFromVlong(DSA_Q(pSrc), &DSA_Q(pNew), NULL)))
        goto exit;

    if (OK > ( status = VLONG_makeVlongFromVlong(DSA_G(pSrc), &DSA_G(pNew), NULL)))
        goto exit;

    if (OK > ( status = VLONG_makeVlongFromVlong(DSA_Y(pSrc), &DSA_Y(pNew), NULL)))
        goto exit;

    if (DSA_X(pSrc))
        if ( OK > ( status = VLONG_makeVlongFromVlong(DSA_X(pSrc), &DSA_X(pNew), NULL)))
            goto exit;

    /* OK */
    *ppNew = pNew;
    pNew = NULL;

exit:
    if (NULL != pNew)
        DSA_freeKey(&pNew, NULL);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_freeKey(DSAKey **ppFreeDSAKey, vlong **ppVlongQueue)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == ppFreeDSAKey) || (NULL == *ppFreeDSAKey))
    {
        status = ERR_NULL_POINTER;
    }
    else
    {
        sbyte4 i;

        for (i = 0; i < NUM_DSA_VLONG; i++)
            VLONG_freeVlong(&((*ppFreeDSAKey)->dsaVlong[i]), ppVlongQueue);

        FREE(*ppFreeDSAKey);
        *ppFreeDSAKey = NULL;
    }

    return status;
}


/*------------------------------------------------------------------*/

static void
DSA_clearKey(DSAKey *pDSAKey, vlong **ppVlongQueue)
{
    sbyte4 i;

    for (i = 0; i < NUM_DSA_VLONG; i++)
        VLONG_freeVlong(&(pDSAKey->dsaVlong[i]), ppVlongQueue);
}


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_equalKey(const DSAKey *pKey1, const DSAKey *pKey2, byteBoolean* pResult)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == pKey1) || (NULL == pKey2) || (NULL == pResult))
        status = ERR_NULL_POINTER;
    else
    {
        /* only compare the public part */
        *pResult = FALSE;

        if ((0 == VLONG_compareSignedVlongs(DSA_P(pKey1), DSA_P(pKey2))) &&
            (0 == VLONG_compareSignedVlongs(DSA_Q(pKey1), DSA_Q(pKey2))) &&
            (0 == VLONG_compareSignedVlongs(DSA_G(pKey1), DSA_G(pKey2))) &&
            (0 == VLONG_compareSignedVlongs(DSA_Y(pKey1), DSA_Y(pKey2))) )
        {
            *pResult = TRUE;
        }
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_verifySignature(MOC_DSA(hwAccelDescr hwAccelCtx) const DSAKey *p_dsaDescr,
                    vlong *m, vlong *pR, vlong *pS,
                    intBoolean *isGoodSignature, vlong **ppVlongQueue)
{
    vlong*  w  = NULL;
    vlong*  u1 = NULL;
    vlong*  u2 = NULL;
    vlong*  v  = NULL;
    vlong*  v1 = NULL;
    vlong*  v2 = NULL;
    vlong*  v3 = NULL;
    vlong*  t  = NULL;
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    *isGoodSignature = FALSE;

    /* From FIPS-186-2: To verify the signature, the verifier first checks to see
                        that 0 < r < q and 0 < s < q; if either condition is
                        violated the signature shall be rejected. */

    /* verify r and s are greater than zero */
    if ((pR->negative) || (pS->negative) ||
        (VLONG_isVlongZero(pR)) || (VLONG_isVlongZero(pS)) )
    {
        status = ERR_CRYPTO_DSA_SIGN_VERIFY_RS_TEST;
        goto exit;
    }

    /* r and s must be less than q */
    if ((VLONG_compareSignedVlongs(DSA_Q(p_dsaDescr), pR) <= 0) ||
        (VLONG_compareSignedVlongs(DSA_Q(p_dsaDescr), pS) <= 0) )
    {
        status = ERR_CRYPTO_DSA_SIGN_VERIFY_RS_TEST;
        goto exit;
    }

    if (OK > (status = VLONG_allocVlong(&t, ppVlongQueue)))
        goto exit;

    /* w = s^-1 mod q */
    if (OK > (status = VLONG_modularInverse(MOC_MOD(hwAccelCtx) pS, DSA_Q(p_dsaDescr), &w, ppVlongQueue)))
        goto exit;

    /* t = m * w */
    if (OK > (status = VLONG_vlongSignedMultiply(t, m, w)))
        goto exit;

    /* u1 = (m * w) mod q */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) t, DSA_Q(p_dsaDescr), &u1, ppVlongQueue)))
        goto exit;

    /* t = r * w */
    if (OK > (status = VLONG_vlongSignedMultiply(t, pR, w)))
        goto exit;

    /* u2 = (r * w) mod q */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) t, DSA_Q(p_dsaDescr), &u2, ppVlongQueue)))
        goto exit;

    /* v1 = g^u1 mod p */
    if (OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) DSA_G(p_dsaDescr), u1, DSA_P(p_dsaDescr), &v1, ppVlongQueue)))
        goto exit;

    /* v2 = y^u2 mod p */
    if (OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) DSA_Y(p_dsaDescr), u2, DSA_P(p_dsaDescr), &v2, ppVlongQueue)))
        goto exit;

    /* t = (g^u1 mod p) * (y^u2 mod p) */
    if (OK > (status = VLONG_vlongSignedMultiply(t, v1, v2)))
        goto exit;

    /* v3 = (g^u1 * y^u2) mod p */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) t, DSA_P(p_dsaDescr), &v3, ppVlongQueue)))
        goto exit;

    /* v = ((g^u1 * y^u2) mod p) mod q */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) v3, DSA_Q(p_dsaDescr), &v, ppVlongQueue)))
        goto exit;

    if (0 == VLONG_compareSignedVlongs(v, pR))
        *isGoodSignature = TRUE;

exit:
    VLONG_freeVlong(&w, ppVlongQueue);
    VLONG_freeVlong(&u1, ppVlongQueue);
    VLONG_freeVlong(&u2, ppVlongQueue);
    VLONG_freeVlong(&v, ppVlongQueue);
    VLONG_freeVlong(&v1, ppVlongQueue);
    VLONG_freeVlong(&v2, ppVlongQueue);
    VLONG_freeVlong(&v3, ppVlongQueue);
    VLONG_freeVlong(&t, ppVlongQueue);

    return status;

} /* DSA_verifySignature */


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_computeSignature(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx,
                     const DSAKey *p_dsaDescr, vlong* m, intBoolean *pVerifySignature,
                     vlong **ppR, vlong **ppS, vlong **ppVlongQueue)
{
    /* p, q, g, private, public are all provided by key file */
    /* k is random */
    /* x = private key */
    /* y = public key */
    /* m = digested data */
    /* transmit p,q,g,y(public) */
    ubyte4      privateKeySize = VLONG_bitLength(DSA_X(p_dsaDescr)) / 8;
    ubyte       kBuf[2*privateKeySize];
    vlong*      ksrc      = NULL;
    vlong*      k         = NULL;
    vlong*      kinv      = NULL;
    vlong*      x         = NULL;
    vlong*      tmp       = NULL;
    vlong*      tmp1      = NULL;
    MSTATUS     status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    /* compute a random k, less than q using FIPS 186-2 */
    if (OK > (status = RANDOM_numberGenerator(pFipsRngCtx, kBuf, 2 * privateKeySize)))
        goto exit;

    if (OK > (status = VLONG_vlongFromByteString(kBuf, 2*privateKeySize,
                                                 &ksrc, ppVlongQueue)))
    {
        goto exit;
    }
    if ( OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx)  ksrc, DSA_Q(p_dsaDescr),
                                                       &k, ppVlongQueue)))
    {
        goto exit;
    }

    /* Compute r = (g^k mod p) mod q */
    if (OK > (status = VLONG_modexp(MOC_DSA(hwAccelCtx) DSA_G(p_dsaDescr), k,
                                    DSA_P(p_dsaDescr), &tmp, ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp,
                                                     DSA_Q(p_dsaDescr), ppR, ppVlongQueue)))
    {
        goto exit;
    }

    /* Compute s = inv(k) (m + xr) mod q */
    /* tmp = xr */
    if (OK > (status = VLONG_vlongSignedMultiply(tmp, DSA_X(p_dsaDescr), *ppR)))
        goto exit;

    /* tmp1 = xr mod q */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp, DSA_Q(p_dsaDescr), &tmp1, ppVlongQueue)))
        goto exit;

    /* tmp1 = (m + xr) mod q */
    if (OK > (status = VLONG_addSignedVlongs(tmp1, m, ppVlongQueue)))
        goto exit;

    while (VLONG_compareSignedVlongs(tmp1, DSA_Q(p_dsaDescr)) > 0)
        if (OK > (status = VLONG_subtractSignedVlongs(tmp1, DSA_Q(p_dsaDescr), ppVlongQueue)))
            goto exit;

    /* kinv = inv(k) mod q */
    if (OK > (status = VLONG_modularInverse(MOC_DSA(hwAccelCtx) k, DSA_Q(p_dsaDescr), &kinv, ppVlongQueue)))
        goto exit;

    /* tmp = ((m + xr) mod q) * (inv(k) mod q) */
    if (OK > (status = VLONG_vlongSignedMultiply(tmp, tmp1, kinv)))
        goto exit;

    /* s = inv(k) (m + xr) mod q */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp, DSA_Q(p_dsaDescr), ppS, ppVlongQueue)))
        goto exit;

    if (NULL != pVerifySignature)
    {
        DSA_verifySignature(MOC_DSA(hwAccelCtx) p_dsaDescr, m, *ppR, *ppS, pVerifySignature, ppVlongQueue);
    }

exit:
    VLONG_freeVlong(&ksrc, ppVlongQueue);
    VLONG_freeVlong(&k, ppVlongQueue);
    VLONG_freeVlong(&kinv, ppVlongQueue);
    VLONG_freeVlong(&x, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);
    VLONG_freeVlong(&tmp1, ppVlongQueue);

    return status;

} /* DSA_computeSignature */



/*------------------------------------------------------------------*/

extern MSTATUS
DSA_computeSignatureEx(MOC_DSA(hwAccelDescr hwAccelCtx)
                       RNGFun rngfun, void* rngarg,
                       const DSAKey *p_dsaDescr, vlong* m,
                       intBoolean *pVerifySignature,
                       vlong **ppR, vlong **ppS, vlong **ppVlongQueue)
{
    /* p, q, g, private, public are all provided by key file */
    /* k is random */
    /* x = private key */
    /* y = public key */
    /* m = digested data */
    /* transmit p,q,g,y(public) */
    ubyte4      privateKeySize = VLONG_bitLength(DSA_X(p_dsaDescr)) / 8;
    ubyte       kBuf[2*privateKeySize];
    vlong*      ksrc      = NULL;
    vlong*      k         = NULL;
    vlong*      kinv      = NULL;
    vlong*      x         = NULL;
    vlong*      tmp       = NULL;
    vlong*      tmp1      = NULL;
    MSTATUS     status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    /* compute a random k, less than q using FIPS 186-2 */
    if (OK > (status = rngfun(rngarg, 2 * privateKeySize, kBuf)))
        goto exit;

    if (OK > (status = VLONG_vlongFromByteString(kBuf, 2*privateKeySize,
                                                 &ksrc, ppVlongQueue)))
    {
        goto exit;
    }
    if ( OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx)  ksrc, DSA_Q(p_dsaDescr),
                                                       &k, ppVlongQueue)))
    {
        goto exit;
    }

    /* Compute r = (g^k mod p) mod q */
    if (OK > (status = VLONG_modexp(MOC_DSA(hwAccelCtx) DSA_G(p_dsaDescr), k,
                                    DSA_P(p_dsaDescr), &tmp, ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp,
                                                     DSA_Q(p_dsaDescr), ppR, ppVlongQueue)))
    {
        goto exit;
    }

    /* Compute s = inv(k) (m + xr) mod q */
    /* tmp = xr */
    if (OK > (status = VLONG_vlongSignedMultiply(tmp, DSA_X(p_dsaDescr), *ppR)))
        goto exit;

    /* tmp1 = xr mod q */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp, DSA_Q(p_dsaDescr), &tmp1, ppVlongQueue)))
        goto exit;

    /* tmp1 = (m + xr) mod q */
    if (OK > (status = VLONG_addSignedVlongs(tmp1, m, ppVlongQueue)))
        goto exit;

    while (VLONG_compareSignedVlongs(tmp1, DSA_Q(p_dsaDescr)) > 0)
        if (OK > (status = VLONG_subtractSignedVlongs(tmp1, DSA_Q(p_dsaDescr), ppVlongQueue)))
            goto exit;

    /* kinv = inv(k) mod q */
    if (OK > (status = VLONG_modularInverse(MOC_DSA(hwAccelCtx) k, DSA_Q(p_dsaDescr), &kinv, ppVlongQueue)))
        goto exit;

    /* tmp = ((m + xr) mod q) * (inv(k) mod q) */
    if (OK > (status = VLONG_vlongSignedMultiply(tmp, tmp1, kinv)))
        goto exit;

    /* s = inv(k) (m + xr) mod q */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp, DSA_Q(p_dsaDescr), ppS, ppVlongQueue)))
        goto exit;

    if (NULL != pVerifySignature)
    {
        DSA_verifySignature(MOC_DSA(hwAccelCtx) p_dsaDescr, m, *ppR, *ppS, pVerifySignature, ppVlongQueue);
    }

exit:
    VLONG_freeVlong(&ksrc, ppVlongQueue);
    VLONG_freeVlong(&k, ppVlongQueue);
    VLONG_freeVlong(&kinv, ppVlongQueue);
    VLONG_freeVlong(&x, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);
    VLONG_freeVlong(&tmp1, ppVlongQueue);

    return status;

} /* DSA_computeSignatureEx */


/*------------------------------------------------------------------*/

static MSTATUS getHashValue(DSAHashType hashType, ubyte *src, ubyte4 length, ubyte *hashVal)
{
    MSTATUS status = OK;

    switch (hashType)
    {
        case DSA_sha1:
        {
            status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) src, length, hashVal);
            break;
        }
#ifndef __DISABLE_MOCANA_SHA224__
        case DSA_sha224:
        {
            status = SHA224_completeDigest(MOC_HASH(hwAccelCtx) src, length, hashVal);
            break;
        }
#endif

#ifndef __DISABLE_MOCANA_SHA256__
        case DSA_sha256:
        {
            status = SHA256_completeDigest(MOC_HASH(hwAccelCtx) src, length, hashVal);
            break;
        }
#endif

#ifndef __DISABLE_MOCANA_SHA384__
        case DSA_sha384:
        {
            status = SHA384_completeDigest(MOC_HASH(hwAccelCtx) src, length, hashVal);
            break;
        }
#endif

#ifndef __DISABLE_MOCANA_SHA512__
        case DSA_sha512:
        {
            SHA512_completeDigest(MOC_HASH(hwAccelCtx) src, length, hashVal);
            break;
        }
#endif
        default:
        {
            /* The needed SHA-2 algorithm was not built in */
            status = ERR_CRYPTO_SHA_ALGORITHM_DISABLED;
        }
    }

    return status;
}

/*------------------------------------------------------------------*/

static MSTATUS getHashLen(DSAHashType hashType, ubyte4 *hashLen)
{
    MSTATUS status = OK;

    switch(hashType)
    {
        case DSA_sha1:
        {
            *hashLen = 160;
            break;
        }
#ifndef __DISABLE_MOCANA_SHA224__
        case DSA_sha224:
        {
            *hashLen = 224;
            break;
        }
#endif
#ifndef __DISABLE_MOCANA_SHA256__
        case DSA_sha256:
        {
            *hashLen = 256;
            break;
        }
#endif
#ifndef __DISABLE_MOCANA_SHA384__
        case DSA_sha384:
        {
            *hashLen = 384;
            break;
        }
#endif
#ifndef __DISABLE_MOCANA_SHA512__
        case DSA_sha512:
        {
            *hashLen = 512;
            break;
        }
#endif
        default:
        {
            /* The needed SHA-2 algorithm was not built in */
            status = ERR_CRYPTO_SHA_ALGORITHM_DISABLED;
        }
    }

    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
generatePQ(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, DSAKey *p_dsaDescr, ubyte4 L, ubyte4 Nin, DSAHashType hashType, ubyte4 *pRetC, ubyte *pRetSeed, vlong **ppVlongQueue)
{
    /* this is the FIPS version! */
    ubyte*          S     = NULL;
    ubyte*          U     = NULL;
    ubyte*          U_tmp = NULL;
    ubyte*          U_hash  = NULL;
    vlong*          U_vlong = NULL;
    vlong*          U_vlongtmp = NULL;
    vlong*          bBit    = NULL;
    vlong*          q     = NULL;
    vlong*          W     = NULL;
    vlong*          Vk    = NULL;
    vlong*          X     = NULL;
    vlong*          p     = NULL;
    ubyte4          n, k, C, N, carry, b;
    sbyte4          index;
    intBoolean      isPrimeQ, isPrimeP;
    MSTATUS         status = OK;
    ubyte4          shaSize;
    ubyte4          shaBytes;
    ubyte4          iterations = (4 * L) - 1;
    ubyte4          seedSize = Nin/8;
#ifdef __ENABLE_MOCANA_64_BIT__
    ubyte4          unitSize = 64;
#else
    ubyte4          unitSize = 32;
#endif

    isPrimeP = FALSE;

    if( OK > (status = getHashLen(hashType, &shaSize)))
        goto exit;

    shaBytes = shaSize / 8;

    /* FIPS 186-4 only allows the following (L,N) pairs: (1024, 160), (2048, 224), (2048, 256), or (3072, 256) */
    if(!(
#ifdef __ENABLE_MOCANA_DSA_ALL_KEYSIZE__
    ((L == 1024) && (Nin == 160)) ||
#endif
    ((L == 2048) && (Nin == 224)) || ((L == 2048) && (Nin == 256)) || ((L == 3072) && (Nin == 256))))
    {
        status = ERR_DSA_INVALID_KEYLENGTH;
        goto exit;
    }

    /* The shaSize must be greater than N.  The only allowed SHA algorithms are SHA-1, SHA-224, SHA-256, SHA-384, SHA-512 */
    if(shaSize < Nin)
    {
        status = ERR_DSA_HASH_TOO_SMALL;
        goto exit;
    }

    if ( OK > (status = CRYPTO_ALLOC ( hwAccelCtx, seedSize, TRUE, &S ) ) )
        goto exit;

    /*if (NULL == (U = MALLOC(SHA_HASH_RESULT_SIZE)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }*/
    if ( OK > (status = CRYPTO_ALLOC ( hwAccelCtx, shaBytes , TRUE, &U ) ) )
        goto exit;

    if ( OK > (status = CRYPTO_ALLOC ( hwAccelCtx, seedSize , TRUE, &U_tmp ) ) )
        goto exit;

    if ( OK > (status = CRYPTO_ALLOC ( hwAccelCtx, shaBytes , TRUE, &U_hash ) ) )
        goto exit;

    if (OK > (status = VLONG_allocVlong(&W, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_allocVlong(&X, ppVlongQueue)))
        goto exit;

    n = (L-1) / shaSize;
    b = (L-1) - (n*shaSize);

    do
    {
        isPrimeQ = FALSE;

        do
        {
            /* make random number S */
            if (OK > (status = RANDOM_numberGenerator(pFipsRngCtx, S, seedSize)))
                goto exit;

            /* U = HASH(S) mod 2^(N-1) */
            if (OK > (status = getHashValue(hashType, S, seedSize, U)))
                goto exit;

            VLONG_freeVlong(&U_vlong, ppVlongQueue);
            if (OK > (status = VLONG_allocVlong(&U_vlong, ppVlongQueue)))
                goto exit;

            if (OK > (status = VLONG_vlongFromByteString(U, shaBytes, &U_vlong, ppVlongQueue)))
                goto exit;

            VLONG_freeVlong(&U_vlongtmp, ppVlongQueue);
            if (OK > (status = VLONG_allocVlong(&U_vlongtmp, ppVlongQueue)))
                goto exit;

            if (OK > (status = VLONG_setVlongBit(U_vlongtmp, Nin - 1)))
                goto exit;

            if(OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) U_vlong, U_vlongtmp, &U_vlong, ppVlongQueue)))
                goto exit;

            /* q = 2^(N-1) + U + 1 - (U mod 2) */
            VLONG_freeVlong(&q, ppVlongQueue);
            if (OK > (status = VLONG_allocVlong(&q, ppVlongQueue)))
                goto exit;
    
            if (OK > (status = VLONG_setVlongBit(q, Nin - 1)))
                goto exit; 

            /* if U is odd then U + 1 - (U mod 2) = U  */
            /* if U is even then U + 1 - (U mod 2) = U + 1 */
            if(!(U[shaBytes-1] & 0x01))
            {
                VLONG_increment(U_vlong, ppVlongQueue);
            }

            if (OK > (status = VLONG_addSignedVlongs(q, U_vlong, ppVlongQueue)))
                goto exit;

            if (OK > (status = PRIME_doPrimeTestsEx(MOC_MOD(hwAccelCtx) pFipsRngCtx, q, prime_DSA, &isPrimeQ, ppVlongQueue)))
                goto exit;
        }
        while (FALSE == isPrimeQ);

        /* Let C = 0 and N = 1 */
        C = 0;
        N = 1;

        do
        {
            /* Vk = HASH((S + N + k) mod 2^g) */
            VLONG_clearVlong(W);    /* W = 0 */
            for (k = 0; k <= n; k++)
            {
                MOC_MEMCPY(U_tmp, S, seedSize);

                carry = N + k;

                for (index = seedSize-1; 0 <= index; index--)
                {
                    carry += U_tmp[index];

                    U_tmp[index] = (ubyte)(carry & 0xff);

                    carry >>= 8;

                    if (0 == carry)
                        break;
                }

                /* Vk = HASH[(Seed + offset + k)] */
                if (OK > (status = getHashValue(hashType, U_tmp, seedSize, U_hash)))
                    goto exit;

                /* W += Vk; W = V0 + (V1 << shaSize) + ... + (Vk << (k * shaSize) */
                VLONG_freeVlong(&Vk, ppVlongQueue);

                if (OK > (status = VLONG_vlongFromByteString(U_hash, shaBytes, &Vk, ppVlongQueue)))
                    goto exit;

                if (k == n)
                {
                    ubyte4 i = (b + 1) / unitSize;

                    for (i = shaSize/unitSize; i > (b / unitSize); i--)
                        if (OK > (status = VLONG_setVlongUnit(Vk, i, 0)))
                            goto exit;

                    /* clear highest bit */
                    VLONG_freeVlong(&bBit, ppVlongQueue);
                    if (OK > (status = VLONG_allocVlong(&bBit, ppVlongQueue)))
                        goto exit;

                    if (OK > (status = VLONG_setVlongBit(bBit, b)))
                        goto exit;

                    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) Vk, bBit, &Vk, ppVlongQueue)))
                        goto exit;
                }

                if (OK > (status = VLONG_shlXvlong(Vk, k * shaSize)))
                    goto exit;

                if (OK > (status = VLONG_addSignedVlongs(W, Vk, ppVlongQueue)))
                    goto exit;
            }

            /* X = W + (2^(L-1)) */
            VLONG_clearVlong(X);

            if (OK > (status = VLONG_setVlongBit(X, L - 1)))
                goto exit;

            if (OK > (status = VLONG_addSignedVlongs(X, W, ppVlongQueue)))
                goto exit;

            /* p = X - ((X mod 2q) - 1) */
            if (OK > (status = VLONG_shlVlong(q)))
                goto exit;

            VLONG_freeVlong(&p, ppVlongQueue);
            if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) X, q, &p, ppVlongQueue)))
                goto exit;

            if (OK > (status = VLONG_shrVlong(q)))
                goto exit;

            if (OK > (status = VLONG_decrement(p, ppVlongQueue)))
                goto exit;

            /* negate it */
            p->negative ^= TRUE;

            if (OK > (status = VLONG_addSignedVlongs(p, X, ppVlongQueue)))
                goto exit;

            /* if p < (2^(L-1)), we need to try p again */
            if ((0 != (p->pUnits[0] & 1)) && (!p->negative) && (VLONG_bitLength(p) >= L))
            {
                if (OK > (status = PRIME_doPrimeTestsEx(MOC_MOD(hwAccelCtx) pFipsRngCtx, p, prime_DSA, &isPrimeP, ppVlongQueue)))
                    goto exit;

                /* if p is prime, we got our prime pair */
                if (isPrimeP)
                    break;
            }

            /* C = C + 1 and N = N + n + 1 */
            C++;
            N += n + 1;
        }
        while ((iterations > C) && (FALSE == isPrimeP));
    }
    while (FALSE == isPrimeP);

    /* copy values to DSA structure */
    DSA_P(p_dsaDescr) = p; p = NULL;
    DSA_Q(p_dsaDescr) = q; q = NULL;

    if (NULL != pRetC)
        *pRetC = C;

    if (NULL != pRetSeed)
        MOC_MEMCPY(pRetSeed, S, seedSize);

exit:
    /*FREE(S);
    FREE(U);
    FREE(U_tmp);*/

    CRYPTO_FREE ( hwAccelCtx, TRUE, &S );
    CRYPTO_FREE ( hwAccelCtx, TRUE, &U );
    CRYPTO_FREE ( hwAccelCtx, TRUE, &U_tmp );
    CRYPTO_FREE ( hwAccelCtx, TRUE, &U_hash);

    VLONG_freeVlong(&U_vlong, ppVlongQueue);
    VLONG_freeVlong(&U_vlongtmp, ppVlongQueue);
    VLONG_freeVlong(&bBit, ppVlongQueue);
    VLONG_freeVlong(&q, ppVlongQueue);
    VLONG_freeVlong(&W, ppVlongQueue);
    VLONG_freeVlong(&Vk, ppVlongQueue);
    VLONG_freeVlong(&X, ppVlongQueue);
    VLONG_freeVlong(&p, ppVlongQueue);

    return status;
} /* generatePQ */

/*------------------------------------------------------------------*/

extern MSTATUS
generateG(MOC_DSA(hwAccelDescr hwAccelCtx) DSAKey *p_dsaDescr, vlong **ppRetH, vlong **ppVlongQueue)
{
    vlong*  h         = NULL;
    vlong*  p_1       = NULL;
    vlong*  p_1_div_q = NULL;
    MSTATUS status = OK;

    /* p_1 = p-1 */
    if (OK > (status = VLONG_makeVlongFromVlong(DSA_P(p_dsaDescr), &p_1, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_decrement(p_1, ppVlongQueue)))
        goto exit;

    /* h must be less than (p-1) */
    if (OK > (status = VLONG_makeVlongFromVlong(p_1, &h, ppVlongQueue)))
        goto exit;

    /* compute (p-1)/q) */
    if (OK > (status = VLONG_operatorDivideSignedVlongs(p_1, DSA_Q(p_dsaDescr), &p_1_div_q, ppVlongQueue)))
        goto exit;

    /* g = (h^((p-1)/q)) mod p */
    do
    {
        VLONG_freeVlong(&DSA_G(p_dsaDescr), ppVlongQueue);

        if (OK > (status = VLONG_decrement(h, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) h, p_1_div_q, DSA_P(p_dsaDescr), &DSA_G(p_dsaDescr), ppVlongQueue)))
            goto exit;
    }
    while (1 >= VLONG_bitLength(DSA_G(p_dsaDescr)));

    if (NULL != ppRetH)
    {
        *ppRetH = h;
        h = NULL;
    }

exit:
    VLONG_freeVlong(&h, ppVlongQueue);
    VLONG_freeVlong(&p_1, ppVlongQueue);
    VLONG_freeVlong(&p_1_div_q, ppVlongQueue);

    return status;

} /* generateG */


static MSTATUS
verifyG(MOC_DSA(hwAccelDescr hwAccelCtx) vlong *pP, vlong *pQ, vlong *pG, intBoolean *isValid, vlong **ppVlongQueue)
{
    vlong*  p_1       = NULL;
    vlong*  tmp       = NULL;
    MSTATUS status = OK;

    /* verify g as per Appendix A.2.2 of NIST FIPS186-4 */
    if (OK > (status = VLONG_makeVlongFromVlong(pP, &p_1, ppVlongQueue)))
        goto exit;

    /* p_1 = p-1 */
    if (OK > (status = VLONG_decrement(p_1, ppVlongQueue)))
        goto exit;

    /* g must be less than (p-1) */
    if (VLONG_compareSignedVlongs(p_1, pG) <= 0)
        goto exit;

    /* g must be greater than 1 */
    if (1 >= VLONG_bitLength(pG))
        goto exit;

    /* if g^q = 1 mod p, then return PARTIALLY VALID */
    if(OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) pG, pQ, pP, &tmp, ppVlongQueue)))
        goto exit;

    if( (TRUE == VLONG_isVlongBitSet(tmp, 0)) && (1 == VLONG_bitLength(tmp)))
        *isValid = TRUE;
    else
        *isValid = FALSE;

exit:
    VLONG_freeVlong(&p_1, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);

    return status;

}

extern MSTATUS
DSA_verifyG(MOC_DSA(hwAccelDescr hwAccelCtx) vlong *pP, vlong *pQ, vlong *pG, intBoolean *isValid, vlong **ppVlongQueue)
{
    return verifyG(MOC_DSA(hwAccelDescr hwAccelCtx) pP, pQ, pG, isValid, ppVlongQueue);
}

/*------------------------------------------------------------------*/

extern MSTATUS
DSA_computeKeyPair(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, DSAKey *p_dsaDescr, vlong **ppVlongQueue)
{
    ubyte       privKeySrc[2*PRIVATE_KEY_BYTE_SIZE];
    MSTATUS     status    = OK;
    vlong*      pKeySrc   = 0;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == pFipsRngCtx) || (NULL == p_dsaDescr))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* For FIPS "KeyPair" testing */
    VLONG_freeVlong(&DSA_X(p_dsaDescr), ppVlongQueue);
    VLONG_freeVlong(&DSA_Y(p_dsaDescr), ppVlongQueue);

    /* compute a random private key(x), less than q -- using the FIPS algorithm */
    if (OK > (status = RANDOM_numberGenerator(pFipsRngCtx, privKeySrc, 2*PRIVATE_KEY_BYTE_SIZE)))
        goto exit;

    if (OK > ( status = VLONG_vlongFromByteString( privKeySrc, 2 * PRIVATE_KEY_BYTE_SIZE,
                                                   &pKeySrc, ppVlongQueue)))
    {
        goto exit;
    }

    if ( OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx)  pKeySrc, DSA_Q(p_dsaDescr),
                                                       &DSA_X(p_dsaDescr), ppVlongQueue)))
    {
        goto exit;
    }


    /* compute a public key(y): y = ((g^x) mod p) */
    if (OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) DSA_G(p_dsaDescr), DSA_X(p_dsaDescr),
                                    DSA_P(p_dsaDescr), &DSA_Y(p_dsaDescr), ppVlongQueue)))
    {
        goto exit;
    }

exit:
    if (OK > status)
    {
        VLONG_freeVlong(&DSA_Y(p_dsaDescr), ppVlongQueue);
        VLONG_freeVlong(&DSA_X(p_dsaDescr), ppVlongQueue);
    }
    VLONG_freeVlong(&pKeySrc, ppVlongQueue);

    return status;

} /* DSA_computeKeyPair */


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_generateKeyEx(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, DSAKey *p_dsaDescr, ubyte4 keySize, ubyte4 qSize, DSAHashType hashType, ubyte4 *pRetC, ubyte *pRetSeed, vlong **ppRetH, vlong **ppVlongQueue)
{
#ifdef __VERIFY_DSA_KEY_GENERATION__
    intBoolean dsaKeyGood;
    vlong*     pM = NULL;
    vlong*     pR = NULL;
    vlong*     pS = NULL;
    ubyte4 privatekeySize = qSize / 8;
#endif
    MSTATUS status;
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL != ppRetH)
        *ppRetH = NULL;

#ifdef __VERIFY_DSA_KEY_GENERATION__
    do
    {
        ubyte   buf[privatekeySize];

        dsaKeyGood = TRUE;

        if (NULL != ppRetH)
            VLONG_freeVlong(ppRetH, ppVlongQueue);
#endif

        /* compute the two big primes p & q */
        if (OK > (status = generatePQ(MOC_DSA(hwAccelCtx) pFipsRngCtx, p_dsaDescr, keySize, qSize, hashType, pRetC, pRetSeed, ppVlongQueue)))
            goto exit;

        /* compute g based on p & q */
        if (OK > (status = generateG(MOC_DSA(hwAccelCtx) p_dsaDescr, ppRetH, ppVlongQueue)))
            goto exit;

        /* compute public and private keys */
        if (OK > (status = DSA_computeKeyPair(MOC_DSA(hwAccelCtx) pFipsRngCtx, p_dsaDescr, ppVlongQueue)))
            goto exit;

#ifdef __VERIFY_DSA_KEY_GENERATION__
        if (OK > (status = RANDOM_numberGenerator(pFipsRngCtx, buf, privatekeySize)))
            goto exit;

        if (OK > (status = VLONG_vlongFromByteString(buf, privatekeySize, &pM, ppVlongQueue)))
            goto exit;

        if (OK > (status = DSA_computeSignature(MOC_DSA(hwAccelCtx) pFipsRngCtx, p_dsaDescr, pM, &dsaKeyGood, &pR, &pS, ppVlongQueue)))
            goto exit;

        VLONG_freeVlong(&pM, ppVlongQueue);
        VLONG_freeVlong(&pR, ppVlongQueue);
        VLONG_freeVlong(&pS, ppVlongQueue);

        if (FALSE == dsaKeyGood)
        {
            DEBUG_PRINTNL(DEBUG_CRYPTO, "DSA_generateKey: key verification failed.");
            DSA_clearKey(p_dsaDescr, ppVlongQueue);
        }
    }
    while (FALSE == dsaKeyGood);
#endif

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK > (status = DSA_generateKey_FIPS_consistancy_test(MOC_DSA(sbyte4 hwAccelCtx) pFipsRngCtx, p_dsaDescr)))
        goto exit;
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    goto nocleanup;

exit:
#ifdef __VERIFY_DSA_KEY_GENERATION__
    VLONG_freeVlong(&pM, ppVlongQueue);
    VLONG_freeVlong(&pR, ppVlongQueue);
    VLONG_freeVlong(&pS, ppVlongQueue);
#endif

    if (OK > status)
        DSA_clearKey(p_dsaDescr, ppVlongQueue);

nocleanup:
    return status;

}

/*------------------------------------------------------------------*/

extern MSTATUS
DSA_generateKey(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, DSAKey *p_dsaDescr, ubyte4 keySize, ubyte4 *pRetC, ubyte *pRetSeed, vlong **ppRetH, vlong **ppVlongQueue)
{
    ubyte4 nInput = 0;
    DSAHashType hashType;

    switch(keySize)
    {
#ifdef  __ENABLE_MOCANA_DSA_ALL_KEYSIZE__
        case 1024:
        {
            nInput = 160;
            hashType = DSA_sha1;
            break;
        }
#endif
        case 2048:
        {
            nInput = 256;
            hashType = DSA_sha256;
            break;
        }
        case 3072:
        {
            nInput = 256;
            hashType = DSA_sha256;
            break;
        }
        default:
        {
            return ERR_DSA_INVALID_KEYLENGTH;
        }
    }
    return DSA_generateKeyEx(MOC_DSA(hwAccelDescr hwAccelCtx) pFipsRngCtx, p_dsaDescr, keySize, nInput, hashType, pRetC, pRetSeed, ppRetH, ppVlongQueue);

} /* DSA_generateKey */

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
extern MSTATUS
DSA_generateKey_FIPS_consistancy_test(MOC_DSA(sbyte4 hwAccelCtx) randomContext* pFipsRngCtx, DSAKey* p_dsaDescr)
{
    MSTATUS status = OK;

    sbyte4 msgLen = 15;
    ubyte msg[] = {
        'C', 'L', 'E', 'A', 'R', '_', 'T', 'E', 'X', 'T', '_', 'L', 'I', 'N', 'E'
    };
    intBoolean verifySignature = FALSE;
    intBoolean isGoodSignature = FALSE;
    vlong* msgVL = NULL;
    vlong* pR = NULL;
    vlong* pS = NULL;


    if (OK > (status = VLONG_vlongFromByteString( msg, msgLen, &msgVL, NULL )))
        goto exit;

    if (OK > (status = DSA_computeSignature( MOC_DSA(sbyte4 hwAccelCtx) pFipsRngCtx, p_dsaDescr, msgVL, &verifySignature, &pR, &pS, NULL )))
        goto exit;

#ifdef __FIPS_OPS_TEST__
    if ( 1 == dsa_fail )
    {
        *(pR->pUnits) = 0xA6D3;
    }
#endif

    if (OK > (status = DSA_verifySignature( MOC_DSA(sbyte4 hwAccelCtx) p_dsaDescr, msgVL, pR, pS, &isGoodSignature, NULL )))
        goto exit;

    if (!isGoodSignature)
    {
        status = ERR_FIPS_DSA_SIGN_VERIFY_FAIL;
        setFIPS_Status(FIPS_ALGO_DSA,status);
        goto exit;
    }

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "DSA_generateKey_FIPS_consistancy_test: GOOD Signature Verify!");
#endif

exit:
    VLONG_freeVlong(&msgVL, NULL);
    VLONG_freeVlong(&pR, NULL);
    VLONG_freeVlong(&pS, NULL);

    return status;
}
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

/*------------------------------------------------------------------*/

extern MSTATUS
DSA_makeKeyBlob(const DSAKey *p_dsaDescr, ubyte *pKeyBlob, ubyte4 *pRetKeyBlobLength)
{
    ubyte*  pMpintStringP   = NULL;
    ubyte*  pMpintStringQ   = NULL;
    ubyte*  pMpintStringG   = NULL;
    ubyte*  pMpintStringX   = NULL;
    ubyte*  pMpintStringY   = NULL;
    sbyte4  mpintByteSizeP  = 0;
    sbyte4  mpintByteSizeQ  = 0;
    sbyte4  mpintByteSizeG  = 0;
    sbyte4  mpintByteSizeX  = 0;
    sbyte4  mpintByteSizeY  = 0;
    ubyte4  keySize;
    ubyte4  index;
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == p_dsaDescr)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* p */
    if (OK > (status = VLONG_mpintByteStringFromVlong(DSA_P(p_dsaDescr), &pMpintStringP, &mpintByteSizeP)))
        goto exit;

    /* q */
    if (OK > (status = VLONG_mpintByteStringFromVlong(DSA_Q(p_dsaDescr), &pMpintStringQ, &mpintByteSizeQ)))
        goto exit;

    /* g */
    if (OK > (status = VLONG_mpintByteStringFromVlong(DSA_G(p_dsaDescr), &pMpintStringG, &mpintByteSizeG)))
        goto exit;

    /* y */
    if (OK > (status = VLONG_mpintByteStringFromVlong(DSA_Y(p_dsaDescr), &pMpintStringY, &mpintByteSizeY)))
        goto exit;

    /* x */
    if (DSA_X(p_dsaDescr))
        if (OK > (status = VLONG_mpintByteStringFromVlong(DSA_X(p_dsaDescr), &pMpintStringX, &mpintByteSizeX)))
            goto exit;

    keySize = mpintByteSizeP + mpintByteSizeQ + mpintByteSizeG + mpintByteSizeY + mpintByteSizeX;

    if (NULL != pKeyBlob)
    {
        /* p */
        if (OK > (status = MOC_MEMCPY(pKeyBlob, pMpintStringP, mpintByteSizeP)))
            goto exit;
        index = mpintByteSizeP;

        /* q */
        if (OK > (status = MOC_MEMCPY(index + pKeyBlob, pMpintStringQ, mpintByteSizeQ)))
            goto exit;
        index += mpintByteSizeQ;

        /* g */
        if (OK > (status = MOC_MEMCPY(index + pKeyBlob, pMpintStringG, mpintByteSizeG)))
            goto exit;
        index += mpintByteSizeG;

        /* y */
        if (OK > (status = MOC_MEMCPY(index + pKeyBlob, pMpintStringY, mpintByteSizeY)))
            goto exit;
        index += mpintByteSizeY;

        /* x */
        if (DSA_X(p_dsaDescr))
            if (OK > (status = MOC_MEMCPY(index + pKeyBlob, pMpintStringX, mpintByteSizeX)))
                goto exit;
    }

    *pRetKeyBlobLength = keySize;

exit:
    if (NULL != pMpintStringP)
        FREE(pMpintStringP);

    if (NULL != pMpintStringQ)
        FREE(pMpintStringQ);

    if (NULL != pMpintStringG)
        FREE(pMpintStringG);

    if (NULL != pMpintStringX)
        FREE(pMpintStringX);

    if (NULL != pMpintStringY)
        FREE(pMpintStringY);

    return status;

} /* DSA_makeKeyBlob */


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_extractKeyBlob(DSAKey **pp_RetNewDsaDescr, const ubyte *pKeyBlob, ubyte4 keyBlobLength)
{
    ubyte4  index;
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (0 == keyBlobLength)
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    if (OK > (status = DSA_createKey(pp_RetNewDsaDescr)))
        goto exit;

    /* p */
    index = 0;
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &(DSA_P(*pp_RetNewDsaDescr)), &index)))
        goto exit;

    pKeyBlob += index;
    if (0 >= (sbyte4)(keyBlobLength = (keyBlobLength - index)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* q */
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &(DSA_Q(*pp_RetNewDsaDescr)), &index)))
        goto exit;

    pKeyBlob += index;
    if (0 >= (sbyte4)(keyBlobLength = (keyBlobLength - index)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* g */
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &(DSA_G(*pp_RetNewDsaDescr)), &index)))
        goto exit;

    pKeyBlob += index;
    if (0 >= (sbyte4)(keyBlobLength = (keyBlobLength - index)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* y */
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &(DSA_Y(*pp_RetNewDsaDescr)), &index)))
        goto exit;

    /* !!! no private key, just exit */
    if (0 == (keyBlobLength - index))
        goto exit;

    pKeyBlob += index;
    if (0 >= (sbyte4)(keyBlobLength = (keyBlobLength - index)))
    {
        status = ERR_BAD_KEY_BLOB;
        goto exit;
    }

    /* x */
    if (OK > (status = VLONG_newFromMpintBytes(pKeyBlob, keyBlobLength, &(DSA_X(*pp_RetNewDsaDescr)), &index)))
        goto exit;

    if (0 != keyBlobLength - index)
        status = ERR_BAD_KEY_BLOB;

exit:
    return status;

} /* DSA_extractKeyBlob */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_FIPS_DSA_PQGVER_TEST__

static MSTATUS
verifyPQ1864(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx,
         DSAKey *p_dsaDescr, ubyte4 L, ubyte4 Nin, DSAHashType hashType, ubyte4 *pRetC,
         ubyte *pSeed, ubyte4 seedSize, intBoolean *pIsPrimePQ, vlong **ppVlongQueue)
{
    ubyte*          S     = NULL;
    ubyte*          U     = NULL;
    ubyte*          U_tmp = NULL;
    ubyte*          U_hash  = NULL;
    vlong*          U_vlong = NULL;
    vlong*          U_vlongtmp = NULL;
    vlong*          bBit  = NULL;
    vlong*          q     = NULL;
    vlong*          W     = NULL;
    vlong*          Vk    = NULL;
    vlong*          X     = NULL;
    vlong*          p     = NULL;
    ubyte4          n, k, C, N, carry, b;
    sbyte4          index;
    intBoolean      isPrimeQ, isPrimeP;
    MSTATUS         status = OK;
    ubyte4          shaSize;
    ubyte4          shaBytes;
    ubyte4          iterations = (4 * L) - 1;
#ifdef __ENABLE_MOCANA_64_BIT__
    ubyte4          unitSize = 64;
#else
    ubyte4          unitSize = 32;
#endif

    isPrimeP = FALSE;
    *pIsPrimePQ = FALSE;

    if( OK > (status = getHashLen(hashType, &shaSize)))
        goto exit;

    shaBytes = shaSize / 8;

    if(!(((L == 1024) && (Nin == 160)) || ((L == 2048) && (Nin == 224)) || ((L == 2048) && (Nin == 256)) || ((L == 3072) && (Nin == 256))))
    {
        status = ERR_DSA_INVALID_KEYLENGTH;
        goto exit;
    }

    /* The shaSize must be greater than N.  The only allowed SHA algorithms are SHA-1, SHA-224, SHA-256, SHA-384, SHA-512 */
    if(shaSize < Nin)
    {
        status = ERR_DSA_HASH_TOO_SMALL;
        goto exit;
    }

    /* allocate buffers */
    if (NULL == (S = MALLOC(seedSize)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (NULL == (U = MALLOC(shaBytes)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (NULL == (U_tmp = MALLOC(seedSize)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (NULL == (U_hash = MALLOC(shaBytes)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = VLONG_allocVlong(&W, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_allocVlong(&X, ppVlongQueue)))
        goto exit;

    n = (L-1) / shaSize;
    b = (L-1) - (n*shaSize);

    isPrimeQ = FALSE;

    /* use the provided seed as S */
    if (OK > (status = MOC_MEMCPY(S, pSeed, seedSize)))
        goto exit;

    /* U = HASH(S) mod 2^(N-1) */
    if (OK > (status = getHashValue(hashType, S, seedSize, U)))
        goto exit;

    VLONG_freeVlong(&U_vlong, ppVlongQueue);
    if (OK > (status = VLONG_allocVlong(&U_vlong, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_vlongFromByteString(U, shaBytes, &U_vlong, ppVlongQueue)))
        goto exit;

    VLONG_freeVlong(&U_vlongtmp, ppVlongQueue);
    if (OK > (status = VLONG_allocVlong(&U_vlongtmp, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_setVlongBit(U_vlongtmp, Nin - 1)))
        goto exit;

    if(OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) U_vlong, U_vlongtmp, &U_vlong, ppVlongQueue)))
        goto exit;

    /* q = 2^(N-1) + U + 1 - (U mod 2) */
    VLONG_freeVlong(&q, ppVlongQueue);
    if (OK > (status = VLONG_allocVlong(&q, ppVlongQueue)))
        goto exit;
    if (OK > (status = VLONG_setVlongBit(q, Nin - 1)))
        goto exit; 

    /* if U is odd then U + 1 - (U mod 2) = U  */
    /* if U is even then U + 1 - (U mod 2) = U + 1 */
    if(!(U[shaBytes-1] & 0x01))
    {
        VLONG_increment(U_vlong, ppVlongQueue);
    }

    if (OK > (status = VLONG_addSignedVlongs(q, U_vlong, ppVlongQueue)))
        goto exit;

    if (OK > (status = PRIME_doPrimeTestsEx(MOC_MOD(hwAccelCtx) pFipsRngCtx, q, prime_DSA, &isPrimeQ, ppVlongQueue)))
        goto exit;

    /* Q should have been prime with the provided seed */
    if (FALSE == isPrimeQ)
        goto exit;

    /* Let C = 0 and N = 1 */
    C = 0;
    N = 1;

    do
    {
        /* Vk = HASH((S + N + k) mod 2^g) */
        VLONG_clearVlong(W);    /* W = 0 */

        for (k = 0; k <= n; k++)
        {
            
            MOC_MEMCPY(U_tmp, S, seedSize);

            carry = N + k;

            for (index = seedSize-1; 0 <= index; index--)
            {
                carry += U_tmp[index];

                U_tmp[index] = (ubyte)(carry & 0xff);

                carry >>= 8;

                if (0 == carry)
                    break;
            }

            /* Vk = HASH[(Seed + offset + k)] */
            if (OK > (status = getHashValue(hashType, U_tmp, seedSize, U_hash)))
                goto exit;

            /* W += Vk; W = V0 + (V1 << shaSize) + ... + (Vk << (k * shaSize) */
            VLONG_freeVlong(&Vk, ppVlongQueue);

            if (OK > (status = VLONG_vlongFromByteString(U_hash, shaBytes, &Vk, ppVlongQueue)))
                goto exit;

            if (k == n)
            {
                ubyte4 i = (b + 1) / unitSize;

                for (i = shaSize/unitSize; i > (b / unitSize); i--)
                    if (OK > (status = VLONG_setVlongUnit(Vk, i, 0)))
                        goto exit;

                /* clear highest bit */
                VLONG_freeVlong(&bBit, ppVlongQueue);
                if (OK > (status = VLONG_allocVlong(&bBit, ppVlongQueue)))
                    goto exit;

                if (OK > (status = VLONG_setVlongBit(bBit, b)))
                    goto exit;

                if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) Vk, bBit, &Vk, ppVlongQueue)))
                    goto exit;
            }

            if (OK > (status = VLONG_shlXvlong(Vk, k * shaSize)))
                goto exit;

            if (OK > (status = VLONG_addSignedVlongs(W, Vk, ppVlongQueue)))
                goto exit;
        }

        /* X = W + (2^(L-1)) */
        VLONG_clearVlong(X);

        if (OK > (status = VLONG_setVlongBit(X, L - 1)))
            goto exit;

        if (OK > (status = VLONG_addSignedVlongs(X, W, ppVlongQueue)))
            goto exit;

        /* p = X - ((X mod 2q) - 1) */
        if (OK > (status = VLONG_shlVlong(q)))
            goto exit;

        VLONG_freeVlong(&p, ppVlongQueue);
        if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) X, q, &p, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_shrVlong(q)))
            goto exit;

        if (OK > (status = VLONG_decrement(p, ppVlongQueue)))
            goto exit;

        /* negate it */
        p->negative ^= TRUE;

        if (OK > (status = VLONG_addSignedVlongs(p, X, ppVlongQueue)))
            goto exit;

        /* if p < (2^(L-1)), we need to try p again */
        if ((0 != (p->pUnits[0] & 1)) && (!p->negative) && (VLONG_bitLength(p) >= L))
        {
            if (OK > (status = PRIME_doPrimeTestsEx(MOC_MOD(hwAccelCtx) pFipsRngCtx, p, prime_DSA, &isPrimeP, ppVlongQueue)))
                goto exit;

            /* if p is prime, we got our prime pair */
            if (isPrimeP)
                break;
        }

        /* C = C + 1 and N = N + n + 1 */
        C++;
        N += n + 1;
    }
    while ((iterations > C) && (FALSE == isPrimeP));

    /* p should have been prime from q, based on the seed */
    if (FALSE == isPrimeP)
        goto exit;

    /* PQ are both prime */
    *pIsPrimePQ = TRUE;

    /* copy values to DSA structure */
    DSA_P(p_dsaDescr) = p; p = NULL;
    DSA_Q(p_dsaDescr) = q; q = NULL;

    if (NULL != pRetC)
        *pRetC = C;

exit:
    FREE(S);
    FREE(U);
    FREE(U_tmp);
    FREE(U_hash);
    VLONG_freeVlong(&q, ppVlongQueue);
    VLONG_freeVlong(&W, ppVlongQueue);
    VLONG_freeVlong(&Vk, ppVlongQueue);
    VLONG_freeVlong(&X, ppVlongQueue);
    VLONG_freeVlong(&p, ppVlongQueue);
    VLONG_freeVlong(&bBit, ppVlongQueue);
    VLONG_freeVlong(&U_vlong, ppVlongQueue);
    VLONG_freeVlong(&U_vlongtmp, ppVlongQueue);

    return status;

} /* verifyPQ */

/*------------------------------------------------------------------*/

static MSTATUS
verifyPQ1862(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx,
         DSAKey *p_dsaDescr, ubyte4 L, ubyte4 Nin, ubyte4 *pRetC,
         ubyte *pSeed, intBoolean *pIsPrimePQ, vlong **ppVlongQueue)
{
    ubyte*          S     = NULL;
    ubyte*          U     = NULL;
    ubyte*          U_tmp = NULL;
    vlong*          q     = NULL;
    vlong*          W     = NULL;
    vlong*          Vk    = NULL;
    vlong*          X     = NULL;
    vlong*          p     = NULL;
    vlong*          bBit  = NULL;
    ubyte4          n, k, C, N, carry, b;
    sbyte4          index;
    intBoolean      isPrimeQ, isPrimeP;
    MSTATUS         status = OK;
#ifdef __ENABLE_MOCANA_64_BIT__
    ubyte4          unitSize = 64;
#else
    ubyte4          unitSize = 32;
#endif

    isPrimeP = FALSE;
    *pIsPrimePQ = FALSE;
    /* An implementation of Appendix A.1.1.1 of NIST FIPS 186-4 */
    /* FIPS 186-4 only allows for keys of the (L,N) pair (1024, 160)  (Appendix A.1.1.1) */
    if ((L != 1024) | (Nin != 160))
    {
        status = ERR_DSA_INVALID_KEYLENGTH;
        goto exit;
    }

    /* allocate buffers */
    if (NULL == (S = MALLOC(SHA_HASH_RESULT_SIZE)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (NULL == (U = MALLOC(SHA_HASH_RESULT_SIZE)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (NULL == (U_tmp = MALLOC(SHA_HASH_RESULT_SIZE)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = VLONG_allocVlong(&W, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_allocVlong(&X, ppVlongQueue)))
        goto exit;

    n = (L-1) / 160;
    b = (L-1) - (n*160);

    isPrimeQ = FALSE;

    /* use the provided seed as S */
    if (OK > (status = MOC_MEMCPY(S, pSeed, SHA_HASH_RESULT_SIZE)))
        goto exit;

    /* U = SHA(S) */
    if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) S, SHA_HASH_RESULT_SIZE, U)))
        goto exit;

    /* make temp copy of S */
    MOC_MEMCPY(U_tmp, S, SHA_HASH_RESULT_SIZE);

    /* compute S+1, we don't need to do anything to compute ((S+1) mod 2^g) */
    for (index = SHA_HASH_RESULT_SIZE - 1; 0 <= index; index--)
        if (++U_tmp[index])
            break;

    /* hash U_tmp on top of itself */
    if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) U_tmp, SHA_HASH_RESULT_SIZE, U_tmp)))
        goto exit;

    for (index = SHA_HASH_RESULT_SIZE - 1; 0 <= index; index--)
        U[index] = (ubyte)(U[index] ^ U_tmp[index]);

    /* set the most and least significant bits */
    U[0] |= 0x80;
    U[SHA_HASH_RESULT_SIZE - 1] |= 0x01;

    /* q = U */
    VLONG_freeVlong(&q, ppVlongQueue);
    if (OK > (status = VLONG_vlongFromByteString(U, SHA_HASH_RESULT_SIZE, &q, ppVlongQueue)))
        goto exit;

    if (OK > (status = PRIME_doPrimeTests(MOC_MOD(hwAccelCtx) pFipsRngCtx, q, &isPrimeQ, ppVlongQueue)))
        goto exit;

    /* Q should have been prime with the provided seed */
    if (FALSE == isPrimeQ)
        goto exit;

    /* Let C = 0 and N = 2 */
    C = 0;
    N = 2;

    do
    {
        /* Vk = SHA((S + N + k) mod 2^g) */
        VLONG_clearVlong(W);    /* W = 0 */

        for (k = 0; k <= n; k++)
        {
            MOC_MEMCPY(U_tmp, S, SHA_HASH_RESULT_SIZE);

            carry = N + k;

            for (index = SHA_HASH_RESULT_SIZE-1; 0 <= index; index--)
            {
                carry += U_tmp[index];

                U_tmp[index] = (ubyte)(carry & 0xff);

                carry >>= 8;

                if (0 == carry)
                    break;
            }

            /* Vk = SHA-1[(Seed + offset + k)] */
            if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) U_tmp, SHA_HASH_RESULT_SIZE, U_tmp)))
                goto exit;

            /* W += Vk; W = V0 + (V1 << 160) + ... + (Vk << (k * 160) */
            VLONG_freeVlong(&Vk, ppVlongQueue);

            if (OK > (status = VLONG_vlongFromByteString(U_tmp, SHA_HASH_RESULT_SIZE, &Vk, ppVlongQueue)))
                goto exit;

            if (k == n)
            {
                ubyte4 i = (b + 1) / 32;

                for (i = 5; i > (b / unitSize); i--)
                    if (OK > (status = VLONG_setVlongUnit(Vk, i, 0)))
                        goto exit;

                /* clear highest bit */
                VLONG_freeVlong(&bBit, ppVlongQueue);
                if (OK > (status = VLONG_allocVlong(&bBit, ppVlongQueue)))
                    goto exit;

                if (OK > (status = VLONG_setVlongBit(bBit, b)))
                    goto exit;

                if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) Vk, bBit, &Vk, ppVlongQueue)))
                    goto exit;
            }

            if (OK > (status = VLONG_shlXvlong(Vk, k * SIZEOF_DSA_Q)))
                goto exit;

            if (OK > (status = VLONG_addSignedVlongs(W, Vk, ppVlongQueue)))
                goto exit;
        }

        /* X = W + (2^(L-1)) */
        VLONG_clearVlong(X);

        if (OK > (status = VLONG_setVlongBit(X, L - 1)))
            goto exit;

        if (OK > (status = VLONG_addSignedVlongs(X, W, ppVlongQueue)))
            goto exit;

        /* p = X - ((X mod 2q) - 1) */
        if (OK > (status = VLONG_shlVlong(q)))
            goto exit;

        VLONG_freeVlong(&p, ppVlongQueue);
        if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) X, q, &p, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_shrVlong(q)))
            goto exit;

        if (OK > (status = VLONG_decrement(p, ppVlongQueue)))
            goto exit;

        /* negate it */
        p->negative ^= TRUE;

        if (OK > (status = VLONG_addSignedVlongs(p, X, ppVlongQueue)))
            goto exit;

        /* if p < (2^(L-1)), we need to try p again */
        if ((0 != (p->pUnits[0] & 1)) && (!p->negative) && (VLONG_bitLength(p) >= L))
        {
            if (OK > (status = PRIME_doPrimeTests(MOC_MOD(hwAccelCtx) pFipsRngCtx, p, &isPrimeP, ppVlongQueue)))
                goto exit;

            /* if p is prime, we got our prime pair */
            if (isPrimeP)
                break;
        }

        /* C = C + 1 and N = N + n + 1 */
        C++;
        N += n + 1;
    }
    while ((MAX_DSA_ITERATIONS > C) && (FALSE == isPrimeP));

    /* p should have been prime from q, based on the seed */
    if (FALSE == isPrimeP)
        goto exit;

    /* PQ are both prime */
    *pIsPrimePQ = TRUE;

    /* copy values to DSA structure */
    DSA_P(p_dsaDescr) = p; p = NULL;
    DSA_Q(p_dsaDescr) = q; q = NULL;

    if (NULL != pRetC)
        *pRetC = C;

exit:
    FREE(S);
    FREE(U);
    FREE(U_tmp);
    VLONG_freeVlong(&q, ppVlongQueue);
    VLONG_freeVlong(&W, ppVlongQueue);
    VLONG_freeVlong(&Vk, ppVlongQueue);
    VLONG_freeVlong(&X, ppVlongQueue);
    VLONG_freeVlong(&p, ppVlongQueue);
    VLONG_freeVlong(&bBit, ppVlongQueue);

    return status;

} /* verifyPQ */

/*------------------------------------------------------------------*/

static MSTATUS
verifyPQ(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx,
         DSAKey *p_dsaDescr, ubyte4 L, ubyte4 Nin, DSAHashType hashType, DSAKeyType keyType, ubyte4 *pRetC,
         ubyte *pSeed, ubyte4 seedSize, intBoolean *pIsPrimePQ, vlong **ppVlongQueue)
{
    MSTATUS status = OK;
    
    if(keyType == DSA_186_4)
    {
        status = verifyPQ1864(MOC_DSA(hwAccelDescr hwAccelCtx) pFipsRngCtx,
                            p_dsaDescr, L, Nin, hashType,  pRetC,
                            pSeed, seedSize, pIsPrimePQ, ppVlongQueue);
    }
    else
    {
        status = verifyPQ1862(MOC_DSA(hwAccelDescr hwAccelCtx) pFipsRngCtx,
                             p_dsaDescr, L, Nin, pRetC,
                             pSeed, pIsPrimePQ, ppVlongQueue);
    }

    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
DSA_verifyPQ(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx,
            DSAKey *p_dsaDescr, ubyte4 L, ubyte4 Nin, DSAHashType hashType, DSAKeyType keyType, ubyte4 C,
            ubyte *pSeed, ubyte4 seedSize, intBoolean *pIsPrimePQ, vlong **ppVlongQueue)
{
    ubyte4          newC = 0;
    DSAKey*         pDsa = NULL;
    intBoolean      isNewPrimePQ = FALSE;
    MSTATUS         status = OK;

    *pIsPrimePQ = FALSE;

    if (OK > (status = DSA_createKey(&pDsa)))
        goto exit;

    if(OK > (status =  verifyPQ(pFipsRngCtx, pDsa, L, Nin, hashType, keyType, &newC, pSeed, seedSize, &isNewPrimePQ, ppVlongQueue)))
        goto exit;

    if ((FALSE == isNewPrimePQ) || (newC != C) || (0 != VLONG_compareSignedVlongs(DSA_P(p_dsaDescr), DSA_P(pDsa)))  || (0 != VLONG_compareSignedVlongs(DSA_Q(p_dsaDescr), DSA_Q(pDsa))))
        goto exit;

    *pIsPrimePQ = TRUE;
exit:
    DSA_freeKey(&pDsa, ppVlongQueue);
    return status;
}

#endif /* __DISABLE_MOCANA_FIPS_DSA_PQGVER_TEST__ */

/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_FIPS_DSA_PQGVER_TEST__
extern MSTATUS
DSA_verifyKeysEx(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, ubyte *pSeed, ubyte4 seedSize, const DSAKey *p_dsaDescr, DSAHashType hashType, DSAKeyType keyType, ubyte4 C, vlong *pH, intBoolean *isGoodKeys, vlong **ppVlongQueue)
{
    ubyte4          newC = 0;
    vlong*          p_1       = NULL;
    vlong*          p_1_div_q = NULL;
    DSAKey*         pDsa = NULL;
    intBoolean      isPrimePQ = FALSE;
    intBoolean      isValidG = FALSE;
    MSTATUS         status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == pSeed) || (NULL == p_dsaDescr) || (NULL == pH) || (NULL == isGoodKeys) || (NULL == DSA_P(p_dsaDescr)) || (NULL == DSA_Q(p_dsaDescr)))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *isGoodKeys = FALSE;

    /* we first verify p&q are good */
    if (OK > (status = DSA_createKey(&pDsa)))
        goto exit;

    if (OK > (status = verifyPQ(MOC_DSA(hwAccelCtx) pFipsRngCtx, pDsa, VLONG_bitLength(DSA_P(p_dsaDescr)), VLONG_bitLength(DSA_Q(p_dsaDescr)), hashType, keyType, &newC, pSeed, seedSize, &isPrimePQ, ppVlongQueue)))
        goto exit;

    if ((FALSE == isPrimePQ) || (newC != C) || (0 != VLONG_compareSignedVlongs(DSA_P(p_dsaDescr), DSA_P(pDsa)))  || (0 != VLONG_compareSignedVlongs(DSA_Q(p_dsaDescr), DSA_Q(pDsa))))
        goto exit;
    
    if(OK > (status =  verifyG(MOC_DSA(hwAccelDescr) DSA_P(p_dsaDescr), DSA_Q(p_dsaDescr), DSA_G(p_dsaDescr), &isValidG, ppVlongQueue)))
        goto exit;
    
    if(isValidG)
        *isGoodKeys = TRUE;

exit:
    VLONG_freeVlong(&p_1_div_q, ppVlongQueue);
    VLONG_freeVlong(&p_1, ppVlongQueue);
    DSA_freeKey(&pDsa, ppVlongQueue);

    return status;

}

/*------------------------------------------------------------------*/

extern MSTATUS
DSA_verifyKeys(MOC_DSA(hwAccelDescr hwAccelCtx) randomContext* pFipsRngCtx, ubyte *pSeed, const DSAKey *p_dsaDescr, ubyte4 C, vlong *pH, intBoolean *isGoodKeys, vlong **ppVlongQueue)
{
    /* Default to (SHA-1) */
    return DSA_verifyKeysEx(MOC_DSA(hwAccelDescr hwAccelCtx) pFipsRngCtx, pSeed, 20, p_dsaDescr, DSA_sha1, DSA_186_4, C, pH, isGoodKeys, ppVlongQueue);
} /* DSA_verifyKeys */
#endif /* __DISABLE_MOCANA_FIPS_DSA_PQGVER_TEST__ */


/*------------------------------------------------------------------*/

static MSTATUS
DSA_setKeyParameters(DSAKey *pKey,
                        const ubyte* p, ubyte4 pLen,
                        const ubyte* q, ubyte4 qLen,
                        const ubyte* g, ubyte4 gLen,
                        vlong **ppVlongQueue)
{
    MSTATUS status;

    DSA_clearKey( pKey, ppVlongQueue);

    if (OK > (status = VLONG_vlongFromByteString(p, pLen, &DSA_P(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }
    DEBUG_RELABEL_MEMORY(DSA_P(pKey));

    if (OK > (status = VLONG_vlongFromByteString(q, qLen, &DSA_Q(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }
    DEBUG_RELABEL_MEMORY(DSA_Q(pKey));

    if (OK > (status = VLONG_vlongFromByteString(g, gLen, &DSA_G(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }
    DEBUG_RELABEL_MEMORY(DSA_G(pKey));

exit:

    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
DSA_setAllKeyParameters(MOC_DSA(hwAccelDescr hwAccelCtx) DSAKey *pKey,
                        const ubyte* p, ubyte4 pLen,
                        const ubyte* q, ubyte4 qLen,
                        const ubyte* g, ubyte4 gLen,
                        const ubyte* x, ubyte4 xLen,
                        vlong **ppVlongQueue)
{
    /* use this to set the parameters of a private key */
    MSTATUS status;

    if (!pKey || !p || !q || !g  || !x)   /* y is not used */
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    if ( OK > (status = DSA_setKeyParameters(pKey, p, pLen, q, qLen,
                                                g, gLen, ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_vlongFromByteString(x, xLen,
                                                 &DSA_X(pKey), ppVlongQueue)))
    {
        goto exit;
    }

    DEBUG_RELABEL_MEMORY(DSA_X(pKey));

    /* recompute Y */
    if (OK > (status = VLONG_modexp(MOC_MOD(hwAccelCtx) DSA_G(pKey), DSA_X(pKey),
                                    DSA_P(pKey), &DSA_Y(pKey), ppVlongQueue)))
    {
        goto exit;
    }

exit:

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_setPublicKeyParameters(DSAKey *pKey, const ubyte* p, ubyte4 pLen,
                            const ubyte* q, ubyte4 qLen,
                            const ubyte* g, ubyte4 gLen,
                            const ubyte* y, ubyte4 yLen,
                            vlong **ppVlongQueue)
{
    /* use this to set the parameters of a public key */
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_DSA))
        return getFIPS_powerupStatus(FIPS_ALGO_DSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */


    if (!pKey || !p || !q || !g  || !y)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    DSA_clearKey( pKey, ppVlongQueue);

    if (OK > (status = VLONG_vlongFromByteString(p, pLen, &DSA_P(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }
    DEBUG_RELABEL_MEMORY(DSA_P(pKey));

    if (OK > (status = VLONG_vlongFromByteString(q, qLen, &DSA_Q(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }
    DEBUG_RELABEL_MEMORY(DSA_Q(pKey));

    if (OK > (status = VLONG_vlongFromByteString(g, gLen, &DSA_G(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }
    DEBUG_RELABEL_MEMORY(DSA_G(pKey));

    if (OK > (status = VLONG_vlongFromByteString(y, yLen, &DSA_Y(pKey),
                                                 ppVlongQueue)))
    {
        goto exit;
    }
    DEBUG_RELABEL_MEMORY(DSA_Y(pKey));

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DSA_getCipherTextLength(const DSAKey *pKey, sbyte4* cipherTextLen)
{
    if ((NULL == pKey) || (NULL == cipherTextLen) || (NULL == DSA_P(pKey)))
    {
        return ERR_NULL_POINTER;
    }

    return VLONG_byteStringFromVlong( DSA_P(pKey), NULL, cipherTextLen);
}


#endif /* (defined(__ENABLE_MOCANA_DSA__)) */
