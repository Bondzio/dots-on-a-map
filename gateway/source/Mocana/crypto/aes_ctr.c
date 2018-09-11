/* Version: mss_v6_3 */
/*
 * aes_ctr.c
 *
 * AES-CTR Implementation (RFC 3686)
 *
 * Copyright Mocana Corp 2006-2009. All Rights Reserved.
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
#include "../crypto/fips.h"
#include "../crypto/aes.h"
#include "../crypto/aes_ctr.h"

#if (!defined(__DISABLE_AES_CIPHERS__))

#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/debug_console.h"
#include "../crypto/aesalgo.h"


/*------------------------------------------------------------------*/

extern MSTATUS
AESCTRInit( MOC_SYM(hwAccelDescr hwAccelCtx) AES_CTR_Ctx* ctx,
            const ubyte* keyMaterial, sbyte4 keyLength,
            const ubyte initCounter[AES_BLOCK_SIZE])
{
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_AES_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    MOC_MEMSET((ubyte *)ctx, 0x00, sizeof(aesCTRCipherContext));

    /* install the block */
    MOC_MEMCPY( ctx->u.counterBlock, initCounter, AES_BLOCK_SIZE);
    return AESALGO_makeAesKey(&ctx->ctx, 8 * keyLength, keyMaterial, 1, MODE_ECB);
}


/*------------------------------------------------------------------*/

extern BulkCtx
#ifdef __UCOS_RTOS__
CreateAESCTRCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial,
                sbyte4 keyLength, sbyte4 encrypt)
#else
CreateAESCTRCtx(MOC_SYM(hwAccelDescr hwAccelCtx) const ubyte* keyMaterial,
                sbyte4 keyLength, sbyte4 encrypt)
#endif /* __UCOS_RTOS__ */
{
    aesCTRCipherContext* ctx = MALLOC(sizeof(aesCTRCipherContext));
    MOC_UNUSED(encrypt);

    if (NULL != ctx)
    {
        keyLength -= AES_BLOCK_SIZE;
        if (OK > AESCTRInit(MOC_SYM(hwAccelCtx) ctx, keyMaterial,
                            keyLength, keyMaterial + keyLength))
        {
            FREE(ctx);  ctx = NULL;
        }
    }
    return ctx;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DeleteAESCTRCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx* ctx)
{
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_AES_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (*ctx)
    {
#ifdef __ZEROIZE_TEST__
        int counter = 0;
        FIPS_PRINT("\nAES - Before Zeroization\n");
        for( counter = 0; counter < sizeof(aesCTRCipherContext); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)*ctx+counter));
        }
        FIPS_PRINT("\n");
#endif
        /* Zeroize the sensitive information before deleting the memory */
        MOC_MEMSET((ubyte*)*ctx, 0x00, sizeof(aesCTRCipherContext));

#ifdef __ZEROIZE_TEST__
        FIPS_PRINT("\nAES - After Zeroization\n");
        for( counter = 0; counter < sizeof(aesCTRCipherContext); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)*ctx+counter));
        }
        FIPS_PRINT("\n");
#endif

        FREE(*ctx);
        *ctx = NULL;
    }

    return OK;
}


/*------------------------------------------------------------------*/

static void
AESCTR_GetNewBlock( aesCTRCipherContext* pCtx, sbyte4 limit)
{
    sbyte4 i;
    limit = AES_BLOCK_SIZE - limit;
    /* encrypt the current block */
    aesEncrypt(pCtx->ctx.rk, pCtx->ctx.Nr, pCtx->u.counterBlock, pCtx->encBlock);
    /* increment the block for next call */
    for ( i = AES_BLOCK_SIZE - 1; i >= limit; --i)
    {
        if ( ++(pCtx->u.counterBlock[i]))
            break;
        /* it overflowed to 0 so carry over to prev byte */
    }
}


/*------------------------------------------------------------------*/

extern MSTATUS
DoAESCTREx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength,
         sbyte4 encrypt, ubyte* iv, sbyte4 limit)
{
    aesCTRCipherContext*    pCtx = (aesCTRCipherContext *)ctx;
    sbyte4                  i;

    MOC_UNUSED(encrypt);

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_AES_CTR);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ( AES_BLOCK_SIZE < limit || limit < 0)
    {
        return ERR_INVALID_ARG;
    }

    if (iv)
    {
        /* reset to new settings, if IV is null */
        MOC_MEMCPY(pCtx->u.counterBlock, iv, AES_BLOCK_SIZE);
        pCtx->offset = 0;
    }

    /* was there some bytes remaining from last call? */
    if ( pCtx->offset && dataLength > 0)
    {
        while (dataLength > 0 && pCtx->offset > 0)
        {
            *data++ ^= pCtx->encBlock[pCtx->offset];
            dataLength--;
            pCtx->offset++;
            if (AES_BLOCK_SIZE == pCtx->offset)
            {
                pCtx->offset = 0;
            }
        }
    }

    while ( dataLength >= AES_BLOCK_SIZE)
    {
        AESCTR_GetNewBlock( pCtx, limit);
        /* XOR it with the data */
        for ( i = 0; i < AES_BLOCK_SIZE; ++i)
        {
            *data++ ^= pCtx->encBlock[i];
        }
        dataLength -= AES_BLOCK_SIZE;
    }

    if ( dataLength > 0)
    {
        AESCTR_GetNewBlock( pCtx, limit);
        /* XOR it with the data */
        for ( i = 0; (i < dataLength) && (i < AES_BLOCK_SIZE); ++i)
        {
            *data++ ^= pCtx->encBlock[i];
        }
        pCtx->offset = (ubyte)i;
    }

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DoAESCTR(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv)
{
    return DoAESCTREx(MOC_SYM(hwAccelCtx) ctx, data, dataLength, encrypt, iv, AES_BLOCK_SIZE);
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_IPSEC_SERVICE__

extern BulkCtx
CreateAesCtrCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial,
                sbyte4 keyLength, sbyte4 encrypt)
{
    aesCTRCipherContext* ctx = MALLOC(sizeof(aesCTRCipherContext));
    MOC_UNUSED(encrypt);

    if (NULL != ctx)
    {
        MOC_MEMSET((ubyte *)ctx, 0x00, sizeof(aesCTRCipherContext));

        /* install the nonce/block counter in the counter block */
        keyLength -= 4;
        MOC_MEMCPY(ctx->u.counterBlock, keyMaterial + keyLength, 4);
        ctx->u.counterBlock[AES_BLOCK_SIZE - 1] = 1;

        if (OK > AESALGO_makeAesKey(&ctx->ctx, 8 * keyLength, keyMaterial, 1, MODE_ECB))
        {
            FREE(ctx);  ctx = NULL;
        }
    }
    return ctx;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DoAesCtr(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data,
                         sbyte4 dataLength, sbyte4 encrypt, ubyte* iv)
{
    aesCTRCipherContext *pCtx = (aesCTRCipherContext *)ctx;

    /* install IV in the counter block */
    MOC_MEMCPY(&(pCtx->u.counterBlock[4]), iv, 8);

    return DoAESCTREx(MOC_SYM(hwAccelCtx) ctx, data, dataLength, encrypt, NULL, AES_BLOCK_SIZE);
}

#endif /* __ENABLE_MOCANA_IPSEC_SERVICE__ */


#endif /* (!defined(__DISABLE_AES_CIPHERS__) && !defined(__AES_HARDWARE_CIPHER__)) */

