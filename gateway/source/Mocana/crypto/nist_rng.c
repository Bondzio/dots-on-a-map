/* Version: mss_v6_3 */
/*
 * nist_rng.c
 *
 * RNG described in NIST SP800 90
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
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
#include "../crypto/crypto.h"
#include "../common/random.h"
#include "../common/int64.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif

#include "../crypto/sha1.h"

#ifdef __ENABLE_MOCANA_ECC__
#undef __ENABLE_MOCANA_VLONG_ECC_CONVERSION__
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/primefld_priv.h"
#include "../crypto/primeec_priv.h"
#endif /* __ENABLE_MOCANA_ECC__ */
#include "../harness/harness.h"
#include "../crypto/aesalgo.h"
#include "../crypto/aes.h"
#if !defined(__DISABLE_3DES_CIPHERS__)
#include "../crypto/des.h"
#include "../crypto/three_des.h"
#endif
#include "../crypto/nist_rng.h"

#include "../crypto/nist_rng_types.h"  /* This is to get the RandomContext data structures */

#ifdef __FIPS_OPS_TEST__
static int drbg_fail = 0;
#endif

/*-------------------------------------------------------------------------*/
#ifdef __ENABLE_MOCANA_ECC__

#if !defined( __DISABLE_MOCANA_ECC_P521__)
#define MAX_EC_SEED_LEN    (521+7/8)
#elif !defined(__DISABLE_MOCANA_ECC_P384__)
#define MAX_EC_SEED_LEN    (384/8)
#else
#define MAX_EC_SEED_LEN    (256/8)
#endif


/*-------------------------------------------------------------------------*/

#endif /* __ENABLE_MOCANA_ECC__ */

/*-------------------------------------------------------------------------*/
#define MAX_CTR_OUT_LEN  (16)   /* AES_BLOCK_LENGTH */
#define MAX_CTR_KEY_LEN  (32)
#define MAX_CTR_SEED_LEN (MAX_CTR_OUT_LEN + MAX_CTR_KEY_LEN)
static const ubyte mKey[MAX_CTR_KEY_LEN] =
{
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f
};

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
#ifdef __FIPS_OPS_TEST__
MOC_EXTERN void triggerDRBGFail()
{
    drbg_fail = 1;
}
MOC_EXTERN void resetDRBGFail()
{
    drbg_fail = 0;
}
#endif

/*-------------------------------------------------------------------------*/
static void
NIST_RNG_TDESMakeKey(const ubyte* key7, ubyte* key8)
{
    sbyte4 i;

    for (i = 0; i < 3; ++i)
    {
        key8[0] = (key7[0] >> 1);
        key8[1] = ((key7[0] << 6) | (key7[1] >> 2));
        key8[2] = ((key7[1] << 5) | (key7[2] >> 3));
        key8[3] = ((key7[2] << 4) | (key7[3] >> 4));
        key8[4] = ((key7[3] << 3) | (key7[4] >> 5));
        key8[5] = ((key7[4] << 2) | (key7[5] >> 6));
        key8[6] = ((key7[5] << 1) | (key7[6] >> 7));
        key8[7] = (key7[6]);
        key8 += 8;
        key7 += 7;
    }
}

/*-------------------------------------------------------------------*/

static MSTATUS
NIST_RNG_Init_Crypto_Ctx(MOC_SYM(hwAccelDescr hwAccelCtx)
                          NIST_CTR_DRBG_Ctx* pCtx)
{
#if !defined(__DISABLE_3DES_CIPHERS__)
    if (IS_TDES(pCtx))
    {
        ubyte key8[THREE_DES_KEY_LENGTH];

        NIST_RNG_TDESMakeKey(KEY(pCtx), key8);
        return THREE_DES_initKey(&(pCtx->ctx.des), key8,
                                 THREE_DES_KEY_LENGTH);
    }
    else
#endif
#ifndef __DISABLE_AES_CIPHERS__
    if (IS_AES(pCtx))
    {
        return AESALGO_makeAesKey(&(pCtx->ctx.aes), pCtx->keyLenBytes * 8,
                                    KEY(pCtx), 1, MODE_ECB);
    }
#endif
    return ERR_NIST_RNG_CTR_INVALID_KEY_LENGTH;
}


#if !defined(__DISABLE_3DES_CIPHERS__)
/*-----------------------------------------------------------------------*/

static MSTATUS TDESBlockEncrypt(MOC_SYM(hwAccelDescr hwAccelCtx)
                                void* ctx, const ubyte* inBlock, ubyte* outBlock)
{
   return THREE_DES_encipher( (ctx3des*) ctx, (ubyte*) inBlock, outBlock,
                                  THREE_DES_BLOCK_SIZE);
}
#endif

/*-----------------------------------------------------------------------*/
#ifndef __DISABLE_AES_CIPHERS__
static MSTATUS AESBlockEncrypt(MOC_SYM(hwAccelDescr hwAccelCtx)
                               void* ctx, const ubyte* inBlock, ubyte* outBlock)
{
    sbyte4 dummy;
    return AESALGO_blockEncrypt( (aesCipherContext*) ctx, NULL,
                                    (ubyte*) inBlock, AES_BLOCK_SIZE*8, outBlock,
                                    &dummy);
}
#endif

#if !defined(__DISABLE_3DES_CIPHERS__)
/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_RNG_BCC_TDES( MOC_SYM(hwAccelDescr hwAccelCtx)
                    ctx3des *desCtx,
                    ubyte* data,
                    ubyte4 dataLength,
                    ubyte* output,
                    ubyte4 outputLength)
{
    MSTATUS status = OK;
    ubyte4 i, j, k, n;
    ubyte out[THREE_DES_BLOCK_SIZE];

    n = dataLength/THREE_DES_BLOCK_SIZE; /* dataLength is a multiple of outLength */

    if (OK > (status = THREE_DES_initKey(desCtx, mKey,
                                    THREE_DES_KEY_LENGTH)))
    {
        goto exit;
    }

    i = 0;
    while (outputLength)
    {
        ubyte* t;

        BIGEND32( data, i);

        /* BCC */
        MOC_MEMSET( out, 0, THREE_DES_BLOCK_SIZE);
        t = data;

        for (k = 0; k < n; ++k)
        {
            for (j = 0; j < THREE_DES_BLOCK_SIZE; ++j)
            {
                out[j] ^= *t++;
            }
            if (OK > ( status = THREE_DES_encipher( desCtx, out, out,
                                                    THREE_DES_BLOCK_SIZE)))
            {
                goto exit;
            }
        }

        if (outputLength >= THREE_DES_BLOCK_SIZE)
        {
            MOC_MEMCPY( output, out, THREE_DES_BLOCK_SIZE);
            outputLength -= THREE_DES_BLOCK_SIZE;
            output += THREE_DES_BLOCK_SIZE;
        }
        else
        {
            MOC_MEMCPY( output, out, outputLength);
            outputLength = 0;
        }
        ++i;
    }

exit:
    return status;
}


/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_RNG_Block_Cipher_TDES( MOC_SYM(hwAccelDescr hwAccelCtx)
                 ubyte4 keyLen,
                 ubyte* data,
                 ubyte4 dataLength,
                 ubyte* output,
                 ubyte4 outputLength)
{
    MSTATUS status = OK;
    ctx3des ctx;
    ubyte key8[THREE_DES_KEY_LENGTH];
    ubyte temp[MAX_CTR_SEED_LEN];

    if (OK > ( status = NIST_RNG_BCC_TDES( MOC_SYM(hwAccelCtx) &ctx,
                                            data, dataLength,
                                            temp, keyLen + THREE_DES_BLOCK_SIZE)))
    {
        goto exit;
    }

    NIST_RNG_TDESMakeKey(temp, key8);
    if (OK > ( status  = THREE_DES_initKey(&ctx, key8, THREE_DES_KEY_LENGTH)))
    {
        goto exit;
    }

    while (outputLength)
    {
        if (OK > ( status = TDESBlockEncrypt( MOC_SYM(hwAccelCtx) &ctx,
                                                temp + keyLen, temp + keyLen)))
        {
            goto exit;
        }

        if (outputLength >= THREE_DES_BLOCK_SIZE)
        {
            MOC_MEMCPY( output, temp + keyLen, THREE_DES_BLOCK_SIZE);
            output += THREE_DES_BLOCK_SIZE;
            outputLength -= THREE_DES_BLOCK_SIZE;
        }
        else
        {
            MOC_MEMCPY( output, temp + keyLen, outputLength);
            outputLength = 0;
        }
    }

exit:

    return status;
}
#endif


/*-----------------------------------------------------------------------*/
#ifndef __DISABLE_AES_CIPHERS__
static MSTATUS
NIST_RNG_BCC_AES( MOC_SYM(hwAccelDescr hwAccelCtx)
                aesCipherContext* aesCtx,
                 ubyte4 keyLength,
                 const ubyte* data,
                 ubyte4 dataLength,
                 ubyte* output,
                 ubyte4 outputLength)
{
    MSTATUS status = OK;
    ubyte4 i, j, k, n;
    sbyte4 dummy;
    ubyte out[AES_BLOCK_SIZE];

    n = dataLength/AES_BLOCK_SIZE; /* dataLength is a multiple of outLength */

    /* we should fix the AES API...it's quite different from the others */
    if (OK > (status = AESALGO_makeAesKey(aesCtx, keyLength * 8, mKey, 1,
                                            MODE_ECB)))
    {
        goto exit;
    }

    i = 0;
    while (outputLength)
    {
        const ubyte* t;

        BIGEND32( data, i);
        /* BCC */
        MOC_MEMSET( out, 0, AES_BLOCK_SIZE);
        t = data;
        for (k = 0; k < n; ++k)
        {
            for (j = 0; j < AES_BLOCK_SIZE; ++j)
            {
                out[j] ^= *t++;
            }
            /* we should fix the AES API...it's quite different from the others */
            if (OK > ( status = AESALGO_blockEncrypt( aesCtx, NULL, out,
                                                        AES_BLOCK_SIZE*8, out,
                                                        &dummy)))
            {
                goto exit;
            }
        }
        if (outputLength >= AES_BLOCK_SIZE)
        {
            MOC_MEMCPY( output, out, AES_BLOCK_SIZE);
            outputLength -= AES_BLOCK_SIZE;
            output += AES_BLOCK_SIZE;
        }
        else
        {
            MOC_MEMCPY( output, out, outputLength);
            outputLength = 0;
        }
        ++i;
    }

exit:
    return status;
}


/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_RNG_Block_Cipher_AES( MOC_SYM(hwAccelDescr hwAccelCtx)
                 ubyte4 keyLen,
                 ubyte* data,
                 ubyte4 dataLength,
                 ubyte* output,
                 ubyte4 outputLength)
{
    MSTATUS status = OK;
    aesCipherContext ctx;
    ubyte temp[MAX_CTR_SEED_LEN];

    if (OK > ( status = NIST_RNG_BCC_AES( MOC_SYM(hwAccelCtx) &ctx,
                                            keyLen, data, dataLength,
                                            temp, keyLen + AES_BLOCK_SIZE)))
    {
        goto exit;
    }

    /* we should fix the AES API...it's quite different from the others */
    if (OK > (status = AESALGO_makeAesKey(&ctx, keyLen * 8, temp, 1,
                                            MODE_ECB)))
    {
        goto exit;
    }

    while (outputLength)
    {
        if (OK > ( status = AESBlockEncrypt( MOC_SYM(hwAccelCtx) &ctx,
                                            temp + keyLen, temp + keyLen)))
        {
            goto exit;
        }

        if (outputLength >= AES_BLOCK_SIZE)
        {
            MOC_MEMCPY( output, temp + keyLen, AES_BLOCK_SIZE);
            output += AES_BLOCK_SIZE;
            outputLength -= AES_BLOCK_SIZE;
        }
        else
        {
            MOC_MEMCPY( output, temp + keyLen, outputLength);
            outputLength = 0;
        }
    }

exit:

    return status;
}
#endif

/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_RNG_Block_Cipher_df( MOC_SYM(hwAccelDescr hwAccelCtx)
                         ubyte4 outLen, /* bytes */
                         ubyte4 keyLen, /* bytes */
                         const ubyte* inputs[/*numInputs*/],
                         ubyte4 inputLens[/*numInputs*/],
                         ubyte4 numInputs,
                         ubyte* output, ubyte4 outputLenBytes)
{
    MSTATUS status = OK;
    ubyte* cryptoBuff = 0;
    ubyte* resultBuff;
    ubyte4 i, totalInputLen;
    ubyte4 pad;

    if ( outputLenBytes > (512/8))
    {
        return ERR_NIST_RNG_BLOCK_CIPHER_DF_BAD_OUTPUT_LEN;
    }

    totalInputLen = 0;
    for (i = 0; i < numInputs; ++i)
    {
       totalInputLen += inputLens[i];
    }
    /* pad to a multiple of outLen */
    pad = outLen - (( totalInputLen + 9) % outLen);
    if (pad == outLen)
    {
        pad = 0;
    }

    if (OK > ( status = CRYPTO_ALLOC( hwAccelCtx,
                                        outLen + 9 + totalInputLen + pad,
                                        TRUE, &cryptoBuff)))
    {
        goto exit;
    }

    resultBuff = cryptoBuff;
    MOC_MEMSET( cryptoBuff, 0, outLen); /* set IV with 0 */
    resultBuff += outLen;

    BIGEND32( resultBuff, totalInputLen); /* L */
    resultBuff += 4;
    BIGEND32( resultBuff, outputLenBytes);  /* N */
    resultBuff += 4;
    for (i = 0; i < numInputs; ++i)  /* Input String */
    {
        MOC_MEMCPY( resultBuff, inputs[i], inputLens[i]);
        resultBuff += inputLens[i];
    }
    *resultBuff++ = 0x80;             /* 0x80 */
    MOC_MEMSET( resultBuff, 0, pad);  /* pad */

#if !defined(__DISABLE_3DES_CIPHERS__)
    if (THREE_DES_BLOCK_SIZE == outLen)
    {
        if (OK > ( status = NIST_RNG_Block_Cipher_TDES( MOC_SYM(hwAccelCtx)
                                                keyLen,
                                                cryptoBuff,
                                                outLen + 9 + totalInputLen + pad,
                                                output, outputLenBytes)))
        {
            goto exit;
        }
    }
    else
#endif
#ifndef __DISABLE_AES_CIPHERS__
    if (AES_BLOCK_SIZE == outLen)
    {
        if (OK > ( status = NIST_RNG_Block_Cipher_AES( MOC_SYM(hwAccelCtx)
                                                keyLen,
                                                cryptoBuff,
                                                outLen + 9 + totalInputLen + pad,
                                                output, outputLenBytes)))
        {
            goto exit;
        }
    }
#endif

exit:
    CRYPTO_FREE( hwAccelCtx, TRUE, &cryptoBuff);

    return status;
}


/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_CTRDRBG_update_aux(MOC_SYM(hwAccelDescr hwAccelCtx)
                    const ubyte providedData[/* seedlen = outlen + keyLen */],
                    BlockEncryptFunc bef, void* ctx,
                    ubyte key[/* keyLen*/], ubyte4 keyLen,
                    ubyte V[/* outLen*/], ubyte4 outLen)
{
    MSTATUS status = OK;
    ubyte temp[MAX_CTR_SEED_LEN];
    ubyte* t;
    ubyte out[MAX_CTR_OUT_LEN];
    ubyte4 needed;
    sbyte4 i;

    status = ERR_NIST_RNG;
    t = temp;
    needed = outLen + keyLen;
    while (needed )
    {
        for ( i = ((sbyte4)outLen) - 1; i >= 0; --i)
        {
            if ( ++(V[i]) )
                break;
            /* it overflowed to 0 so carry over to prev byte */
        }
        if (OK > ( status = bef(MOC_SYM(hwAccelCtx) ctx, V, out)))
        {
            goto exit;
        }
        if (needed >= outLen)
        {
            MOC_MEMCPY( t, out, outLen);
            t += outLen;
            needed -= outLen;
        }
        else
        {
            MOC_MEMCPY( t, out, needed);
            needed = 0;
        }
    }
    /* steps 3-7 */
    for (i = 0; i < (sbyte4) keyLen; ++i)
    {
        key[i] = temp[i] ^ providedData[i];
    }
    for (; i < (sbyte4) (keyLen + outLen); ++i)
    {
        *V++ = temp[i] ^ providedData[i];
    }

exit:

    return status;
}



/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_CTRDRBG_update(MOC_SYM(hwAccelDescr hwAccelCtx)
                    const ubyte providedData[/* seedlen = outlen + keyLen */],
                    NIST_CTR_DRBG_Ctx* pCtx)
{
    MSTATUS status = OK;

    if (OK > ( status = NIST_CTRDRBG_update_aux( MOC_SYM(hwAccelCtx) providedData,
                                        pCtx->bef, &pCtx->ctx,
                                        KEY(pCtx), pCtx->keyLenBytes,
                                        V(pCtx), pCtx->outLenBytes)))
    {
        goto exit;
    }

    if (OK > ( status = NIST_RNG_Init_Crypto_Ctx( MOC_SYM(hwAccelCtx) pCtx)))
    {
        goto exit;
    }

exit:

    return status;
}


/*-----------------------------------------------------------------------*/

MSTATUS
NIST_CTRDRBG_newContext( MOC_SYM(hwAccelDescr hwAccelCtx)
                            randomContext **ppNewContext,
                            const ubyte* entropyInput,
                            ubyte4 keyLenBytes, ubyte4 outLenBytes,
                            const ubyte* personalization,
                            ubyte4 personalizationLen)
{
    MSTATUS status = OK;

    RandomCtxWrapper* pWrapper = NULL;
    NIST_CTR_DRBG_Ctx* pNewCtx = NULL;
    BlockEncryptFunc bef;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!ppNewContext || !entropyInput)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* personalization string length should be less than seedLen
    that is sum of outLenBytes + keyLenBytes */
    if (personalizationLen > outLenBytes + keyLenBytes)
    {
        status = ERR_NIST_RNG_CTR_BAD_PERSO_STR_LEN;
        goto exit;
    }

    switch (outLenBytes)
    {
#if !defined(__DISABLE_3DES_CIPHERS__)
    case THREE_DES_BLOCK_SIZE: /* TDES */
        if (keyLenBytes != 21)
        {
            status = ERR_NIST_RNG_CTR_INVALID_KEY_LENGTH;
            goto exit;
        }
        bef = TDESBlockEncrypt;
        break;
#endif
#ifndef __DISABLE_AES_CIPHERS__
    case AES_BLOCK_SIZE: /* AES */
        if (keyLenBytes != 16 && keyLenBytes != 24 && keyLenBytes != 32)
        {
            status = ERR_NIST_RNG_CTR_INVALID_KEY_LENGTH;
            goto exit;
        }
        bef = AESBlockEncrypt;
        break;
#endif
    default:
        status = ERR_NIST_RNG_CTR_INVALID_OUTPUT_LENGTH;
        goto exit;
        break; /* */
    }

    if (OK > ( status = CRYPTO_ALLOC(hwAccelCtx,
                                sizeof(RandomCtxWrapper) + outLenBytes + keyLenBytes,
                                TRUE, &pWrapper)) || (NULL == pWrapper))
    {
        goto exit;
    }

    MOC_MEMSET( (ubyte*) pWrapper, 0, sizeof(RandomCtxWrapper) + outLenBytes + keyLenBytes);
    pWrapper->WrappedCtxType = NIST_CTR_DRBG;
    pNewCtx = GET_CTR_DRBG_CTX(pWrapper);
    if (pNewCtx == NULL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pNewCtx->keyLenBytes = keyLenBytes;
    pNewCtx->outLenBytes = outLenBytes;
    pNewCtx->bef = bef;

    pNewCtx->history = MALLOC( pNewCtx->outLenBytes);
    if (!pNewCtx->history)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > ( status = NIST_RNG_Init_Crypto_Ctx( MOC_SYM(hwAccelCtx) pNewCtx)))
    {
        goto exit;
    }

    if (OK > ( status = NIST_CTRDRBG_reseed(MOC_SYM(hwAccelCtx) pWrapper,
                                            entropyInput, keyLenBytes + outLenBytes,
                                            personalization,
                                            personalizationLen)))
    {
        goto exit;
    }

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   /* Create mutex used for FIPS conditional test */
    if ( OK > ( status = RTOS_mutexCreate( &pNewCtx->fipsMutex, 0, 0 ) ) )
        goto exit;
#endif

    *ppNewContext = pWrapper;
    pWrapper = 0;

exit:

    if (pWrapper != NULL)
    {
        NIST_CTRDRBG_deleteContext(MOC_SYM(hwAccelCtx) (randomContext **)&pWrapper);
    }

    return status;
}


/*-----------------------------------------------------------------------*/

MSTATUS
NIST_CTRDRBG_newDFContext( MOC_SYM(hwAccelDescr hwAccelCtx)
                            randomContext **ppNewContext,
                            ubyte4 keyLenBytes, ubyte4 outLenBytes,
                            const ubyte* entropyInput,
                            ubyte4 entropyInputLen,
                            const ubyte* nonce,
                            ubyte4 nonceLen,
                            const ubyte* personalization,
                            ubyte4 personalizationLen)
{
    MSTATUS status = OK;
    RandomCtxWrapper* pWrapper = NULL;
    NIST_CTR_DRBG_Ctx* pNewCtx = NULL;
    ubyte seed[MAX_CTR_SEED_LEN] = { 0 };
    BlockEncryptFunc bef;
    ubyte* inputs[3];
    ubyte4 inputLens[3];


#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */


    if (!ppNewContext || !entropyInput)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    switch (outLenBytes)
    {
#if !defined(__DISABLE_3DES_CIPHERS__)
    case THREE_DES_BLOCK_SIZE: /* TDES */
        if (keyLenBytes != 21)
        {
            status = ERR_NIST_RNG_CTR_INVALID_KEY_LENGTH;
            goto exit;
        }
        bef = TDESBlockEncrypt;
        break;
#endif
#ifndef __DISABLE_AES_CIPHERS__
    case AES_BLOCK_SIZE: /* AES */
        if (keyLenBytes != 16 && keyLenBytes != 24 && keyLenBytes != 32)
        {
            status = ERR_NIST_RNG_CTR_INVALID_KEY_LENGTH;
            goto exit;
        }
        bef = AESBlockEncrypt;
        break;
#endif
    default:
        status = ERR_NIST_RNG_CTR_INVALID_OUTPUT_LENGTH;
        goto exit;
        break; /* */
    }

    if (OK > ( status = CRYPTO_ALLOC(hwAccelCtx,
                                sizeof(RandomCtxWrapper) + outLenBytes + keyLenBytes,
                                TRUE, &pWrapper)) || (NULL == pWrapper))
    {
        goto exit;
    }

    MOC_MEMSET( (ubyte*) pWrapper, 0, sizeof(RandomCtxWrapper) + outLenBytes + keyLenBytes);
    pWrapper->WrappedCtxType = NIST_CTR_DRBG;
    pNewCtx = GET_CTR_DRBG_CTX(pWrapper);
    if (pNewCtx == NULL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pNewCtx->keyLenBytes = keyLenBytes;
    pNewCtx->outLenBytes = outLenBytes;
    pNewCtx->bef = bef;

    pNewCtx->history = MALLOC( pNewCtx->outLenBytes);
    if (!pNewCtx->history)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* initialize context with 0 filled buffers */
    if (OK > ( status = NIST_RNG_Init_Crypto_Ctx( MOC_SYM(hwAccelCtx) pNewCtx)))
    {
        goto exit;
    }

    /* generate the seed */
    inputs[0] = (ubyte *) entropyInput;
    inputLens[0] = entropyInputLen;
    inputs[1] = (ubyte *) nonce;
    inputLens[1] = nonceLen;
    inputs[2] = (ubyte *) personalization;
    inputLens[2] = personalizationLen;

    /* the document says don't use the nonce if there is no personalization, but it
    turns out that's not true based on the published test vectors */
    if (OK > ( status = NIST_RNG_Block_Cipher_df( MOC_SYM(hwAccelCtx)
                                        outLenBytes, keyLenBytes,
                                        (const ubyte**)inputs, inputLens,
                                        (personalization && personalizationLen)? 3 : 2,
                                        seed, outLenBytes + keyLenBytes)))
    {
        goto exit;
    }

    /* initial key values */
    if (OK > ( status = NIST_CTRDRBG_update(MOC_SYM(hwAccelCtx) seed, pNewCtx)))
    {
        goto exit;
    }

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   /* Create mutex used for FIPS conditional test */
    if ( OK > ( status = RTOS_mutexCreate( &pNewCtx->fipsMutex, 0, 0 ) ) )
        goto exit;
#endif

    pNewCtx->flags = e_NIST_RNG_use_df;
    *ppNewContext = pWrapper;
    pWrapper = 0;

exit:

    if (pWrapper != NULL)
    {
        NIST_CTRDRBG_deleteContext(MOC_SYM(hwAccelCtx) (randomContext**)&pWrapper);
    }

    return status;
}


/*-----------------------------------------------------------------------*/

MOC_EXTERN MSTATUS NIST_CTRDRBG_deleteContext( MOC_SYM(hwAccelDescr hwAccelCtx)
                                              randomContext **ppNewContext)
{
    RandomCtxWrapper* pWrapper = NULL;
    NIST_CTR_DRBG_Ctx* pCtx = NULL;
#ifdef __ZEROIZE_TEST__
    int counter = 0;
#endif

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!ppNewContext || !*ppNewContext)
    {
        return ERR_NULL_POINTER;
    }

    pWrapper = (RandomCtxWrapper*)(*ppNewContext);

    pCtx = GET_CTR_DRBG_CTX(pWrapper);
    if (pCtx == NULL)
    {
        return ERR_NULL_POINTER;
    }

#ifdef __ZEROIZE_TEST__
    FIPS_PRINT("\nCTRDRBG history - Before Zeroization\n");
    for( counter = 0; counter < pCtx->outLenBytes; counter++)
    {
        FIPS_PRINT("%02x",*((pCtx->history)+counter));
    }
	FIPS_PRINT("\n");
#endif

    MOC_MEMSET(pCtx->history, 0, pCtx->outLenBytes);

#ifdef __ZEROIZE_TEST__
    FIPS_PRINT("\nCTRDRBG history - After Zeroization\n");
    for( counter = 0; counter < pCtx->outLenBytes; counter++)
    {
        FIPS_PRINT("%02x",*((pCtx->history)+counter));
    }
	FIPS_PRINT("\n");
#endif

    FREE(pCtx->history);

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    RTOS_mutexFree(&pCtx->fipsMutex);
#endif

#ifdef __ZEROIZE_TEST__
    FIPS_PRINT("\nCTRDRBG - Before Zeroization\n");
    for( counter = 0; counter <
                         sizeof(RandomCtxWrapper) + pCtx->outLenBytes + pCtx->keyLenBytes; counter++)
    {
        FIPS_PRINT("%02x",*((ubyte*)pWrapper+counter));
    }
	FIPS_PRINT("\n");
#endif

    MOC_MEMSET( (ubyte*) pWrapper, 0,
                sizeof( RandomCtxWrapper) + pCtx->outLenBytes + pCtx->keyLenBytes);

#ifdef __ZEROIZE_TEST__
    FIPS_PRINT("\nCTRDRBG - After Zeroization\n");
    for( counter = 0; counter <
                         sizeof(RandomCtxWrapper) + pCtx->outLenBytes + pCtx->keyLenBytes; counter++)
    {
        FIPS_PRINT("%02x",*((ubyte*)pWrapper+counter));
    }
	FIPS_PRINT("\n");
#endif

    CRYPTO_FREE(hwAccelCtx, TRUE, ppNewContext);

    *ppNewContext = 0;

    return OK;
}


/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_CTRDRBG_reseedNoDf(MOC_SYM(hwAccelDescr hwAccelCtx)
                            NIST_CTR_DRBG_Ctx* pCtx,
                            const ubyte* entropyInput,
                            ubyte4 entropyInputLen,
                            const ubyte* additionalInput,
                            ubyte4 additionalInputLen)
{
    MSTATUS status = OK;
    ubyte seed[MAX_CTR_SEED_LEN] = { 0 };
    ubyte4 i;
    ubyte4 seedLen;

    seedLen = pCtx->keyLenBytes + pCtx->outLenBytes;

    if (entropyInputLen != seedLen)
    {
        status = ERR_NIST_RNG_CTR_BAD_ENTROPY_INPUT_LEN;
        goto exit;
    }

    if (additionalInput && additionalInputLen)
    {
        if (additionalInputLen > seedLen)
        {
            MOC_MEMCPY( seed, additionalInput, seedLen);
        }
        else
        {
            MOC_MEMCPY( seed, additionalInput, additionalInputLen);
        }
    }

    for(i = 0; i < seedLen; ++i)
    {
        seed[i] ^= entropyInput[i];
    }

    if (OK > ( status = NIST_CTRDRBG_update( MOC_SYM(hwAccelCtx) seed, pCtx)))
    {
        goto exit;
    }

    U8INIT(pCtx->reseedCounter, 0, 1);

exit:

    return status;
}


/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_CTRDRBG_reseedDf(MOC_SYM(hwAccelDescr hwAccelCtx)
                            NIST_CTR_DRBG_Ctx* pCtx,
                            const ubyte* entropyInput,
                            ubyte4 entropyInputLen,
                            const ubyte* additionalInput,
                            ubyte4 additionalInputLen)
{
    MSTATUS status = OK;
    ubyte seed[MAX_CTR_SEED_LEN] = { 0 };
    ubyte* inputs[2];
    ubyte4 inputLens[2];
    ubyte4 seedLen;

    seedLen = pCtx->keyLenBytes + pCtx->outLenBytes;

    /* generate the seed */
    inputs[0] = (ubyte *) entropyInput;
    inputLens[0] = entropyInputLen;
    inputs[1] = (ubyte *) additionalInput;
    inputLens[1] = additionalInputLen;

    if (OK > ( status = NIST_RNG_Block_Cipher_df( MOC_SYM(hwAccelCtx)
                                        pCtx->outLenBytes, pCtx->keyLenBytes,
                                        (const ubyte**)inputs, inputLens,
                                        (additionalInput && additionalInputLen)? 2 : 1,
                                        seed, seedLen)))
    {
        goto exit;
    }

    if (OK > ( status = NIST_CTRDRBG_update( MOC_SYM(hwAccelCtx) seed, pCtx)))
    {
        goto exit;
    }

    U8INIT(pCtx->reseedCounter, 0, 1);

exit:

    return status;
}


/*-----------------------------------------------------------------------*/

MSTATUS
NIST_CTRDRBG_reseed(MOC_SYM(hwAccelDescr hwAccelCtx)
                            randomContext *pContext,
                            const ubyte* entropyInput,
                            ubyte4 entropyInputLen,
                            const ubyte* additionalInput,
                            ubyte4 additionalInputLen)
{
    RandomCtxWrapper* pWrapper = NULL;
    NIST_CTR_DRBG_Ctx* pCtx = NULL;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!pContext || !entropyInput)
    {
        return ERR_NULL_POINTER;
    }

    pWrapper = (RandomCtxWrapper *)pContext;
    pCtx = GET_CTR_DRBG_CTX(pWrapper);
    if (pCtx == NULL)
    {
        return ERR_NULL_POINTER;
    }


    return (pCtx->flags & e_NIST_RNG_use_df) ?
        NIST_CTRDRBG_reseedDf( MOC_SYM(hwAccelCtx)
                                    pCtx, entropyInput,
                                    entropyInputLen,
                                    additionalInput,
                                    additionalInputLen):
        NIST_CTRDRBG_reseedNoDf( MOC_SYM(hwAccelCtx)
                                    pCtx, entropyInput,
                                    entropyInputLen,
                                    additionalInput,
                                    additionalInputLen);
}



/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
static
MSTATUS NIST_CTRDRBG_fipsConditionalTest(NIST_CTR_DRBG_Ctx* pCtx,
                                         const ubyte* generated)
{
    MSTATUS status = OK;
    sbyte4 cmp = 0;

    /* Acquire the mutex as history is a shared resource among threads */
    if ( OK > ( status = RTOS_mutexWait(pCtx->fipsMutex) ) )
        goto exit;

#ifdef __FIPS_OPS_TEST__

    if ( 1 == drbg_fail )
    {
        MOC_MEMCPY(pCtx->history, generated, pCtx->outLenBytes);
    }
#endif

    /* New Random Number must not be the same compare to the previous one -- FIPS */
    status =  MOC_MEMCMP(pCtx->history, generated, pCtx->outLenBytes, &cmp);

    if ( ( OK > status ) || ( 0 == cmp )  )
    {
        status = ERR_FIPS_CTRDRBG_FAIL;
    }
    else
    {
        /* Copy the current RNG output to history for future comparision */
        MOC_MEMCPY(pCtx->history, generated, pCtx->outLenBytes);
    }

    RTOS_mutexRelease(pCtx->fipsMutex);

exit:
    return status;
}
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */


/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_CTRDRBG_generateNoDf(MOC_SYM(hwAccelDescr hwAccelCtx)
                          NIST_CTR_DRBG_Ctx* pCtx,
                         const ubyte* additionalInput, ubyte4 additionalInputLen,
                         ubyte* output, ubyte4 outputLenBits)
{
    MSTATUS status = OK;
    ubyte temp[MAX_CTR_SEED_LEN] = { 0 };
    ubyte out[MAX_CTR_OUT_LEN];
    sbyte4 i;
    ubyte4 seedLen;

    seedLen = pCtx->keyLenBytes + pCtx->outLenBytes;
    if (additionalInput && additionalInputLen)
    {
        if (additionalInputLen > seedLen)
        {
            MOC_MEMCPY( temp, additionalInput, seedLen);
        }
        else
        {
            MOC_MEMCPY( temp, additionalInput, additionalInputLen);
        }

        if (OK > ( status = NIST_CTRDRBG_update( MOC_SYM(hwAccelCtx) temp, pCtx)))
        {
            goto exit;
        }
    }

    while (outputLenBits)
    {
        for ( i = ((sbyte4)pCtx->outLenBytes) - 1; i >= 0; --i)
        {
            if ( ++(V(pCtx)[i]) )
                break;
            /* it overflowed to 0 so carry over to prev byte */
        }
        if (OK > ( status = pCtx->bef(MOC_SYM(hwAccelCtx) &pCtx->ctx, V(pCtx), out)))
        {
            goto exit;
        }

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
        if ( OK > ( status = NIST_CTRDRBG_fipsConditionalTest(pCtx, out) ) )
        {
            setFIPS_Status(FIPS_ALGO_RNG_CTR,status);
            goto exit;
        }
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

        if (outputLenBits > (pCtx->outLenBytes * 8))
        {
            MOC_MEMCPY( output, out, pCtx->outLenBytes);
            output += pCtx->outLenBytes;
            outputLenBits -= (pCtx->outLenBytes * 8);
        }
        else
        {
            MOC_MEMCPY( output, out, (outputLenBits + 7) / 8);
            outputLenBits = 0;
        }
    }

    if (OK > ( status = NIST_CTRDRBG_update( MOC_SYM(hwAccelCtx) temp, pCtx)))
    {
        goto exit;
    }

    INCR_U8(pCtx->reseedCounter);

exit:

    return status;
}


/*-----------------------------------------------------------------------*/

static MSTATUS
NIST_CTRDRBG_generateDf(MOC_SYM(hwAccelDescr hwAccelCtx)
                      NIST_CTR_DRBG_Ctx* pCtx,
                      const ubyte* additionalInput, ubyte4 additionalInputLen,
                      ubyte* output, ubyte4 outputLenBits)
{
    MSTATUS status = OK;
    ubyte temp[MAX_CTR_SEED_LEN] = { 0 };
    ubyte out[MAX_CTR_OUT_LEN];
    sbyte4 i;
    ubyte4 seedLen;

    status = ERR_NIST_RNG;
    seedLen = pCtx->keyLenBytes + pCtx->outLenBytes;
    if (additionalInput && additionalInputLen)
    {
        if (OK > ( NIST_RNG_Block_Cipher_df(MOC_SYM(hwAccelCtx) pCtx->outLenBytes, pCtx->keyLenBytes,
                                                &additionalInput, &additionalInputLen, 1, temp, seedLen)))
        {
            goto exit;
        }

        if (OK > ( status = NIST_CTRDRBG_update( MOC_SYM(hwAccelCtx) temp, pCtx)))
        {
            goto exit;
        }
    }

    while (outputLenBits)
    {
        for ( i = ((sbyte4)pCtx->outLenBytes) - 1; i >= 0; --i)
        {
            if ( ++(V(pCtx)[i]) )
                break;
            /* it overflowed to 0 so carry over to prev byte */
        }
        if (OK > ( status = pCtx->bef(MOC_SYM(hwAccelCtx) &pCtx->ctx, V(pCtx), out)))
        {
            goto exit;
        }

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
        if ( OK > ( status = NIST_CTRDRBG_fipsConditionalTest(pCtx, out) ) )
        {
            setFIPS_Status(FIPS_ALGO_RNG_CTR,status);
            goto exit;
        }
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */


        if (outputLenBits > (pCtx->outLenBytes * 8))
        {
            MOC_MEMCPY( output, out, pCtx->outLenBytes);
            output += pCtx->outLenBytes;
            outputLenBits -= (pCtx->outLenBytes * 8);
        }
        else
        {
            MOC_MEMCPY( output, out, (outputLenBits + 7) / 8);
            outputLenBits = 0;
        }
    }

    if (OK > ( status = NIST_CTRDRBG_update( MOC_SYM(hwAccelCtx) temp, pCtx)))
    {
        goto exit;
    }

    INCR_U8(pCtx->reseedCounter);

exit:

    return status;
}


/*-----------------------------------------------------------------------*/

MSTATUS
NIST_CTRDRBG_generate( MOC_SYM(hwAccelDescr hwAccelCtx) randomContext* pContext,
                       const ubyte* additionalInput, ubyte4 additionalInputLen,
                       ubyte* output, ubyte4 outputLenBits)
{
    MSTATUS status = OK;
    RandomCtxWrapper* pWrapper = NULL;
    NIST_CTR_DRBG_Ctx* pCtx = NULL;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!pContext || !output)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pWrapper = (RandomCtxWrapper *)pContext;
    pCtx = GET_CTR_DRBG_CTX(pWrapper);
    if (pCtx == NULL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* check the reseed counter */
    if ( (HI_U8(pCtx->reseedCounter) > 0x0000FFFF) ||
        (IS_TDES(pCtx) && HI_U8(pCtx->reseedCounter)) )

    {
        status = ERR_NIST_RNG_DBRG_RESEED_NEEDED;
        goto exit;
    }

    /* check the request length */
    if ( (outputLenBits >> 19) ||
        (IS_TDES(pCtx) && (outputLenBits >> 13)) )
    {
        status = ERR_NIST_RNG_DRBG_TOO_MANY_BITS;
        goto exit;
    }

    status = (pCtx->flags & e_NIST_RNG_use_df) ?
            NIST_CTRDRBG_generateDf(MOC_SYM(hwAccelCtx)
                         pCtx, additionalInput, additionalInputLen,
                         output, outputLenBits) :
            NIST_CTRDRBG_generateNoDf(MOC_SYM(hwAccelCtx)
                         pCtx, additionalInput, additionalInputLen,
                         output, outputLenBits);
exit:
    return status;
}


/*-----------------------------------------------------------------------*/

extern MSTATUS
NIST_CTRDRBG_numberGenerator(MOC_SYM(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, ubyte *pBuffer, sbyte4 bufSize)
{
    return NIST_CTRDRBG_generate(MOC_SYM(hwAccelCtx) pRandomContext, NULL, 0,
                                pBuffer, bufSize * 8);
}


/*-----------------------------------------------------------------------*/

extern sbyte4
NIST_CTRDRBG_rngFun(MOC_SYM(hwAccelDescr hwAccelCtx) void* rngFunArg, ubyte4 length, ubyte *buffer)
{
    return NIST_CTRDRBG_generate(MOC_SYM(hwAccelCtx) rngFunArg, NULL, 0,
                                buffer, length * 8);
}
