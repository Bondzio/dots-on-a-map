/* Version: mss_v6_3 */
/*
 * hmac.c
 *
 * Hash Message Authentication Code
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/crypto.h"
#include "../crypto/hmac.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif
#include "../harness/harness.h"

/* hmac specific definitions */
#define IPAD                    0x36
#define OPAD                    0x5c





/*------------------------------------------------------------------*/

#ifndef __HMAC_MD5_HARDWARE_HASH__

extern MSTATUS
HMAC_MD5(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen,
         const ubyte* text, sbyte4 textLen,
         const ubyte* textOpt, sbyte4 textOptLen,
         ubyte result[MD5_DIGESTSIZE])
{
    MD5_CTX     context;
    ubyte       kpad[MD5_BLOCK_SIZE];
    ubyte       tk[MD5_DIGESTSIZE];
    sbyte4      i;
    MSTATUS     status;

#ifndef __ENABLE_MOCANA_HMAC_ALL_KEYSIZE__
    if(14 > keyLen)
    {
        status = ERR_CRYPTO_HMAC_INVALID_KEY_LENGTH;
        goto exit;
    }
#endif

    /* if key is longer than MD5_BLOCK_SIZE bytes reset it to key=MD5(key) */
    if (keyLen > MD5_BLOCK_SIZE)
    {
        if (OK > (status = MD5_completeDigest(MOC_HASH(hwAccelCtx) key, keyLen, tk)))
            goto exit;

        key = tk;
        keyLen = MD5_DIGESTSIZE;
    }

    /*
     * HMAC_MD5 transform:
     * MD5(K XOR opad, MD5(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated MD5_BLOCK_SIZE times
     * opad is the byte 0x5c repeated MD5_BLOCK_SIZE times
     * and text is the data being protected
     */

    /* XOR key padded with 0 to HMAC_BUFFER_SIZE with 0x36 */
    for (i=0; i < keyLen; ++i)
        kpad[i] = (ubyte)(key[i] ^ IPAD);
    for (; i < MD5_BLOCK_SIZE; i++)
        kpad[i] = 0 ^ IPAD;

    /*  perform inner MD5 */
    if (OK > (status = MD5Init_m(MOC_HASH(hwAccelCtx) &context)))
        goto exit;
    if (OK > (status = MD5Update_m(MOC_HASH(hwAccelCtx) &context, kpad, MD5_BLOCK_SIZE)))
        goto exit;
    if (OK > (status = MD5Update_m(MOC_HASH(hwAccelCtx) &context, text, textLen)))
        goto exit;
    if ((NULL != textOpt) && (0 < textOptLen))
        if (OK > (status = MD5Update_m(MOC_HASH(hwAccelCtx) &context, textOpt, textOptLen)))
            goto exit;
    if (OK > (status = MD5Final_m(MOC_HASH(hwAccelCtx) &context, result)))
        goto exit;

    /* XOR key padded with 0 to MD5_BLOCK_SIZE with 0x5C*/
    for (i=0; i < keyLen; i++)
        kpad[i] = (ubyte)(key[i] ^ OPAD);
    for (; i < MD5_BLOCK_SIZE; i++)
        kpad[i] = 0 ^ OPAD;

    /* perform outer MD5 */
    if (OK > (status = MD5Init_m(MOC_HASH(hwAccelCtx) &context)))
        goto exit;
    if (OK > (status = MD5Update_m(MOC_HASH(hwAccelCtx) &context, kpad, MD5_BLOCK_SIZE)))
        goto exit;
    if (OK > (status = MD5Update_m(MOC_HASH(hwAccelCtx) &context, result, MD5_DIGESTSIZE)))
        goto exit;
    status = MD5Final_m(MOC_HASH(hwAccelCtx) &context, result);

exit:
    return status;
}

#endif /* __HMAC_MD5_HARDWARE_HASH__ */


/*------------------------------------------------------------------*/

#ifndef __HMAC_MD5_HARDWARE_HASH__
MOC_EXTERN MSTATUS
HMAC_MD5_quick(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
               const ubyte* pText, sbyte4 textLen,
               ubyte* pResult /* MD5_DIGESTSIZE */)
{
    /* try to use the quick version; for hw acceleration */
    return HMAC_MD5(MOC_HASH(hwAccelCtx) pKey, keyLen, pText, textLen, NULL, 0, pResult);
}
#endif


/*------------------------------------------------------------------*/

#ifndef __HMAC_SHA1_HARDWARE_HASH__

/* compute the HMAC output using SHA1 the textOpt can be null */
extern MSTATUS
HMAC_SHA1(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen,
          const ubyte* text, sbyte4 textLen,
          const ubyte* textOpt, sbyte4 textOptLen,
          ubyte result[SHA_HASH_RESULT_SIZE])
{
    shaDescr    context;
    ubyte       kpad[SHA1_BLOCK_SIZE];
    ubyte       tk[SHA1_RESULT_SIZE];
    sbyte4      i;
    MSTATUS     status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

#ifndef __ENABLE_MOCANA_HMAC_ALL_KEYSIZE__
    /* as of 2014, NIST no longer allows HMAC generation with a key size less than 112 bits (14 bytes) */
    if(14 > keyLen)
    {
        status = ERR_CRYPTO_HMAC_INVALID_KEY_LENGTH;
        goto exit;
    }
#endif

    /* if key is longer than SHA1_BLOCK_SIZE bytes reset it to key = SHA1(key) */
    if (keyLen > SHA1_BLOCK_SIZE)
    {
        if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) key, keyLen, tk)))
            goto exit;

        key = tk;
        keyLen = SHA1_RESULT_SIZE;
    }

    /*
     * HMAC_SHA1 transform:
     * SHA1(K XOR opad, SHA1(K XOR ipad, (text | textOpt)))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated SHA1_BLOCK_SIZE times
     * opad is the byte 0x5c repeated SHA1_BLOCK_SIZE times
     * and text is the data being protected
     */

    /* XOR key padded with 0 to SHA1_BLOCK_SIZE with 0x36 */
    for (i = 0; i < keyLen; i++)
        kpad[i] = (ubyte)(key[i] ^ IPAD);
    for (; i < SHA1_BLOCK_SIZE; i++)
        kpad[i] = 0 ^ IPAD;

    /*  perform inner SHA1 */
    if (OK > (status = SHA1_initDigest(MOC_HASH(hwAccelCtx) &context)))
        goto exit;
    if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, kpad, SHA1_BLOCK_SIZE)))
        goto exit;
    if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, text, textLen)))
        goto exit;
    if ((0 != textOpt) && (0 != textOptLen))
        if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, textOpt, textOptLen)))
            goto exit;
    if (OK > (status = SHA1_finalDigest(MOC_HASH(hwAccelCtx) &context, result)))
        goto exit;

    /* XOR key padded with 0 to HMAC_BUFFER_SIZE with 0x5C*/
    for (i = 0; i < keyLen; i++)
        kpad[i] = (ubyte)(key[i] ^ OPAD);
    for (; i < SHA1_BLOCK_SIZE; i++)
        kpad[i] = 0 ^ OPAD;

    /* perform outer SHA1 */
    if (OK > (status = SHA1_initDigest(MOC_HASH(hwAccelCtx) &context)))
        goto exit;
    if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, kpad, SHA1_BLOCK_SIZE)))
        goto exit;
    if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, result, SHA1_RESULT_SIZE)))
        goto exit;
    status = SHA1_finalDigest(MOC_HASH(hwAccelCtx) &context, result);

exit:
    return status;

} /* HMAC_SHA1 */

#endif /* __HMAC_SHA1_HARDWARE_HASH__ */


/*------------------------------------------------------------------*/

#ifndef __HMAC_SHA1_HARDWARE_HASH__
MOC_EXTERN MSTATUS
HMAC_SHA1_quick(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
                const ubyte* pText, sbyte4 textLen,
                ubyte* pResult /* SHA_HASH_RESULT_SIZE */)
{
    /* try to use the quick version; for hw acceleration */
    return HMAC_SHA1(MOC_HASH(hwAccelCtx) pKey, keyLen, pText, textLen, NULL, 0, pResult);

}
#endif /* __HMAC_SHA1_HARDWARE_HASH__ */


/*------------------------------------------------------------------*/

#ifndef __HMAC_SHA1_HARDWARE_HASH__

/* compute the HMAC output using SHA1 */
extern MSTATUS
HMAC_SHA1Ex(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen,
                        const ubyte* texts[], sbyte4 textLens[],
                        sbyte4 numTexts, ubyte result[SHA_HASH_RESULT_SIZE])
{
    shaDescr    context;
    ubyte       kpad[SHA1_BLOCK_SIZE];
    ubyte       tk[SHA1_RESULT_SIZE];
    sbyte4      i;
    MSTATUS     status;

#ifndef __ENABLE_MOCANA_HMAC_ALL_KEYSIZE__
    /* as of 2014, NIST no longer allows HMAC generation with a key size less than 112 bits (14 bytes) */
    if(14 > keyLen)
    {
        status = ERR_CRYPTO_HMAC_INVALID_KEY_LENGTH;
        goto exit;
    }
#endif

    /* if key is longer than HMAC_BUFFER_SIZE bytes reset it to key = SHA1(key) */
    if (keyLen > SHA1_BLOCK_SIZE)
    {
        if (OK > (status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) key, keyLen, tk)))
            goto exit;

        key = tk;
        keyLen = SHA1_RESULT_SIZE;
    }

    /*
     * HMAC_SHA1 transform:
     * SHA1(K XOR opad, SHA1(K XOR ipad, (text | textOpt)))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    /* XOR key padded with 0 to HMAC_BUFFER_SIZE with 0x36 */
    for (i = 0; i < keyLen; i++)
        kpad[i] = (ubyte)(key[i] ^ IPAD);
    for (; i < SHA1_BLOCK_SIZE; i++)
        kpad[i] = 0 ^ IPAD;

    /*  perform inner SHA1 */
    if (OK > (status = SHA1_initDigest(MOC_HASH(hwAccelCtx) &context)))
        goto exit;
    if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, kpad, SHA1_BLOCK_SIZE)))
        goto exit;
    for (i = 0; i < numTexts; ++i)
    {
        if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, texts[i], textLens[i])))
            goto exit;
    }
    if (OK > (status = SHA1_finalDigest(MOC_HASH(hwAccelCtx) &context, result)))
        goto exit;

    /* XOR key padded with 0 to HMAC_BUFFER_SIZE with 0x5C*/
    for (i = 0; i < keyLen; i++)
        kpad[i] = (ubyte)(key[i] ^ OPAD);
    for (; i < SHA1_BLOCK_SIZE; i++)
        kpad[i] = 0 ^ OPAD;

    /* perform outer SHA1 */
    if (OK > (status = SHA1_initDigest(MOC_HASH(hwAccelCtx) &context)))
        goto exit;
    if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, kpad, SHA1_BLOCK_SIZE)))
        goto exit;
    if (OK > (status = SHA1_updateDigest(MOC_HASH(hwAccelCtx) &context, result, SHA1_RESULT_SIZE)))
        goto exit;
    status = SHA1_finalDigest(MOC_HASH(hwAccelCtx) &context, result);

exit:
    return status;

} /* HMAC_SHA1Ex */

#endif /* __HMAC_SHA1_HARDWARE_HASH__ */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacCreate(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX **pctx,
            const BulkHashAlgo *pBHAlgo)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, sizeof(HMAC_CTX), TRUE, (void **)pctx)))
        goto exit;

    (*pctx)->pBHAlgo = pBHAlgo;
    (*pctx)->hashCtxt = NULL;

exit:
    return status;
} /* HmacCreate */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacDelete(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX **pctx)
{
    MSTATUS status = OK;

    HMAC_CTX *ctx = *pctx;
#ifdef __ZEROIZE_TEST__
        int counter = 0;
#endif

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == ctx) goto exit;

    if (OK > (status = ctx->pBHAlgo->freeFunc(MOC_HASH(hwAccelCtx) &(ctx->hashCtxt))))
        goto exit;

#ifdef __ZEROIZE_TEST__
        FIPS_PRINT("\nHMAC - Before Zeroization\n");
        for( counter = 0; counter < sizeof(HMAC_CTX); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)ctx+counter));
        }
        FIPS_PRINT("\n");
#endif

    /* Zeroize the sensitive information before deleting the memory */
    MOC_MEMSET((unsigned char *)ctx,0x00,sizeof(HMAC_CTX));

#ifdef __ZEROIZE_TEST__
        FIPS_PRINT("\nHMAC - After Zeroization\n");
        for( counter = 0; counter < sizeof(HMAC_CTX); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)ctx+counter));
        }
        FIPS_PRINT("\n");
#endif

    status = CRYPTO_FREE(hwAccelCtx, TRUE, (void **)pctx);

exit:
    return status;
} /* HmacDelete */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacKey(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx, const ubyte *key, ubyte4 keyLen)
{
    MSTATUS status;
    const BulkHashAlgo* pBHAlgo = NULL;

#ifndef __ENABLE_MOCANA_HMAC_ALL_KEYSIZE__
    /* as of 2014, NIST no longer allows HMAC generation with a key size less than 112 bits (14 bytes) */
    if(14 > keyLen)
    {
        status = ERR_CRYPTO_HMAC_INVALID_KEY_LENGTH;
        goto exit;
    }
#endif

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */
    pBHAlgo = ctx->pBHAlgo;
    /* if key is longer than the hash algo block size reset it to key=hash(key) */
    if (keyLen > pBHAlgo->blockSize)
    {
        BulkCtx             hash_ctx = NULL;

        if ((NULL == ctx->hashCtxt) &&
            (OK > (status = pBHAlgo->allocFunc(MOC_HASH(hwAccelCtx) &ctx->hashCtxt))))
        {
            goto exit;
        }

        hash_ctx = ctx->hashCtxt;

        if (OK > (status = pBHAlgo->initFunc(MOC_HASH(hwAccelCtx) hash_ctx)))
            goto exit;
        if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) hash_ctx, key, keyLen)))
            goto exit;
        if (OK > (status = pBHAlgo->finalFunc(MOC_HASH(hwAccelCtx) hash_ctx, ctx->key)))
            goto exit;

        ctx->keyLen = pBHAlgo->digestSize;
    }
    else
    {
        MOC_MEMCPY(ctx->key, key, keyLen);
        ctx->keyLen = keyLen;
    }

    status = HmacReset(MOC_HASH(hwAccelCtx) ctx);

exit:
    return status;
} /* HmacKey */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacReset(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx)
{
    MSTATUS status;
    const BulkHashAlgo* pBHAlgo = ctx->pBHAlgo;
    BulkCtx             hash_ctx = NULL;
    ubyte4              keyLen = ctx->keyLen;
    ubyte*              kpad = ctx->kpad;
    ubyte*              key = ctx->key;
    ubyte4              i;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == ctx->hashCtxt) &&
        (OK > (status = pBHAlgo->allocFunc(MOC_HASH(hwAccelCtx) &ctx->hashCtxt))))
    {
        goto exit;
    }

    hash_ctx = ctx->hashCtxt;

    /* XOR key padded with 0 to HMAC_BUFFER_SIZE with 0x36 */
    for (i=0; i < keyLen; ++i)
        kpad[i] = (ubyte)(key[i] ^ IPAD);
    for (; i < pBHAlgo->blockSize; i++)
        kpad[i] = 0 ^ IPAD;

    /*  perform inner hash */
    if (OK > (status = pBHAlgo->initFunc(MOC_HASH(hwAccelCtx) hash_ctx)))
        goto exit;
    status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) hash_ctx, kpad, pBHAlgo->blockSize);

exit:
    return status;
} /* HmacReset */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacUpdate(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx, const ubyte *text, ubyte4 textLen)
{

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    return ctx->pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) ctx->hashCtxt, text, textLen);
} /* HmacUpdate */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacFinal(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx, ubyte *result)
{
    MSTATUS status;

    const BulkHashAlgo *pBHAlgo = ctx->pBHAlgo;
    BulkCtx hash_ctx = ctx->hashCtxt;
    ubyte4 keyLen = ctx->keyLen;
    ubyte *kpad = ctx->kpad;
    ubyte *key = ctx->key;
    ubyte4 i;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (OK > (status = pBHAlgo->finalFunc(MOC_HASH(hwAccelCtx) hash_ctx, result)))
        goto exit;

    /* XOR key padded with 0 to HMAC_BUFFER_SIZE with 0x5C*/
    for (i=0; i < keyLen; i++)
        kpad[i] = (ubyte)(key[i] ^ OPAD);
    for (; i < pBHAlgo->blockSize; i++)
        kpad[i] = 0 ^ OPAD;

    /* perform outer hash */
    if (OK > (status = pBHAlgo->initFunc(MOC_HASH(hwAccelCtx) hash_ctx)))
        goto exit;
    if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) hash_ctx, kpad, pBHAlgo->blockSize)))
        goto exit;
    if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) hash_ctx, result, pBHAlgo->digestSize)))
        goto exit;
    status = pBHAlgo->finalFunc(MOC_HASH(hwAccelCtx) hash_ctx, result);

exit:
    return status;
} /* HmacFinal */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacQuick(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
          const ubyte* pText, sbyte4 textLen, ubyte* pResult,
          const BulkHashAlgo *pBHAlgo)
{
    return HmacQuickEx(MOC_HASH( hwAccelCtx) pKey, keyLen,
          pText, textLen, NULL, 0, pResult, pBHAlgo);
} /* HmacQuick */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacQuicker(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
            const ubyte* pText, sbyte4 textLen, ubyte* pResult,
            const BulkHashAlgo *pBHAlgo,
            HMAC_CTX *ctx)
{
    return HmacQuickerEx(MOC_HASH( hwAccelCtx) pKey, keyLen,
                         pText, textLen, NULL, 0, pResult, pBHAlgo, ctx);
} /* HmacQuick */


/*------------------------------------------------------------------*/

extern MSTATUS
HmacQuickEx(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
          const ubyte* pText, sbyte4 textLen,
          const ubyte* pOptText, ubyte4 optTextLen,
          ubyte* pResult, const BulkHashAlgo *pBHAlgo)
{
    MSTATUS status;
    HMAC_CTX *ctx = NULL;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (OK > (status = HmacCreate(MOC_HASH(hwAccelCtx) &ctx, pBHAlgo)))
        goto exit;
    if (OK > (status = HmacKey(MOC_HASH(hwAccelCtx) ctx, pKey, keyLen)))
        goto exit;
    if (OK > (status = HmacUpdate(MOC_HASH(hwAccelCtx) ctx, pText, textLen)))
        goto exit;
    if (pOptText && optTextLen)
    {
        if (OK > (status = HmacUpdate(MOC_HASH(hwAccelCtx) ctx, pOptText, optTextLen)))
            goto exit;
    }
    status = HmacFinal(MOC_HASH(hwAccelCtx) ctx, pResult);

exit:
    if (ctx) HmacDelete(MOC_HASH(hwAccelCtx) &ctx);
    return status;
} /* HmacQuickEx */



/*------------------------------------------------------------------*/

extern MSTATUS
HmacQuickerEx(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
              const ubyte* pText, sbyte4 textLen,
              const ubyte* pOptText, ubyte4 optTextLen,
              ubyte* pResult, 
              const BulkHashAlgo *pBHAlgo,
              HMAC_CTX *ctx)
{
    MSTATUS status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (OK > (status = HmacKey(MOC_HASH(hwAccelCtx) ctx, pKey, keyLen)))
        goto exit;
    if (OK > (status = HmacUpdate(MOC_HASH(hwAccelCtx) ctx, pText, textLen)))
        goto exit;
    if (pOptText && optTextLen)
    {
        if (OK > (status = HmacUpdate(MOC_HASH(hwAccelCtx) ctx, pOptText, optTextLen)))
            goto exit;
    }
    status = HmacFinal(MOC_HASH(hwAccelCtx) ctx, pResult);

exit:
    return status;
} /* HmacQuickerEx */

extern MSTATUS
HmacQuickerInline(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
          const ubyte* pText, sbyte4 textLen, ubyte* pResult,
          const BulkHashAlgo *pBHAlgo,BulkCtx* context)
{
    return HmacQuickerInlineEx(MOC_HASH( hwAccelCtx) pKey, keyLen,
          pText, textLen, NULL, 0, pResult, pBHAlgo,context);
} /* HmacQuickerInline */

/*------------------------------------------------------------------*/
extern MSTATUS
HmacQuickerInlineEx(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen,
         const ubyte* text, sbyte4 textLen,
         const ubyte* textOpt, sbyte4 textOptLen,
         ubyte* pResult,const BulkHashAlgo *pBHAlgo,BulkCtx* context)
{
	ubyte4 		blockSize = pBHAlgo->blockSize;
	ubyte4 		digestSize = pBHAlgo->digestSize;
    ubyte       kpad[HMAC_BLOCK_SIZE];
    ubyte       tk[HMAC_BLOCK_SIZE];
    sbyte4      i;
    MSTATUS     status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_HMAC))
        return getFIPS_powerupStatus(FIPS_ALGO_HMAC);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    /* if key is longer than blockSize bytes reset it to key=MD5(key) */
    if (keyLen > blockSize)
    {
    	if (OK > (status = pBHAlgo->initFunc(MOC_HASH(hwAccelCtx) context)))
			goto exit;
		if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) context, key, keyLen)))
			goto exit;
		if (OK > (status = pBHAlgo->finalFunc(MOC_HASH(hwAccelCtx) context, tk)))
			goto exit;
        key = tk;
        keyLen = digestSize;
    }

    /* XOR key padded with 0 to HMAC_BUFFER_SIZE with 0x36 */
    for (i=0; i < keyLen; ++i)
        kpad[i] = (ubyte)(key[i] ^ IPAD);
    for (; i < blockSize; i++)
        kpad[i] = 0 ^ IPAD;

    /*  perform inner hash */
    if (OK > (status = pBHAlgo->initFunc(MOC_HASH(hwAccelCtx) context)))
        goto exit;
    if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) context, kpad, blockSize)))
        goto exit;
    if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) context, text, textLen)))
        goto exit;
    if ((NULL != textOpt) && (0 < textOptLen))
        if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) context, textOpt, textOptLen)))
            goto exit;
    if (OK > (status = pBHAlgo->finalFunc(MOC_HASH(hwAccelCtx) context, pResult)))
        goto exit;

    /* XOR key padded with 0 to blockSize with 0x5C*/
    for (i=0; i < keyLen; i++)
        kpad[i] = (ubyte)(key[i] ^ OPAD);
    for (; i < blockSize; i++)
        kpad[i] = 0 ^ OPAD;

    /* perform outer hash */
    if (OK > (status = pBHAlgo->initFunc(MOC_HASH(hwAccelCtx) context)))
        goto exit;
    if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) context, kpad, blockSize)))
        goto exit;
    if (OK > (status = pBHAlgo->updateFunc(MOC_HASH(hwAccelCtx) context, pResult, digestSize)))
        goto exit;
    status = pBHAlgo->finalFunc(MOC_HASH(hwAccelCtx) context, pResult);

exit:
    return status;
}/* HmacQuickerInlineEx */
