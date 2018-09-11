/* Version: mss_v6_3 */
/*
 * three_des.c
 *
 * 3DES Encipher & Decipher
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#if (!defined(__DISABLE_3DES_CIPHERS__))

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../crypto/des.h"
#include "../crypto/three_des.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif


/*------------------------------------------------------------------*/


extern MSTATUS
THREE_DES_initKey(ctx3des *p_3desContext, const ubyte *pKey, sbyte4 keyLen)
{
    MSTATUS status;

    if ((NULL == p_3desContext) || (NULL == pKey))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (THREE_DES_KEY_LENGTH != keyLen)
    {
        status = ERR_3DES_BAD_KEY_LENGTH;
        goto exit;
    }

    if (OK > (status = DES_initKey(&p_3desContext->firstKey,  pKey, DES_KEY_LENGTH)))
        goto exit;

    if (OK > (status = DES_initKey(&p_3desContext->secondKey, pKey+DES_KEY_LENGTH, DES_KEY_LENGTH)))
        goto exit;

    status = DES_initKey(&p_3desContext->thirdKey,  pKey+(DES_KEY_LENGTH*2), DES_KEY_LENGTH);

exit:
    return status;

} /* THREE_DES_initKey */


/*------------------------------------------------------------------*/

extern MSTATUS
THREE_DES_encipher(ctx3des *p_3desContext, ubyte *pSrc, ubyte *pDest, ubyte4 numBytes)
{
    MSTATUS status;

    if ((NULL == p_3desContext) || (NULL == pSrc) || (NULL == pDest))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    status = DES_encipher(&p_3desContext->firstKey,  pSrc, pDest, numBytes);
    if (OK > status)
        goto exit;

    status = DES_decipher(&p_3desContext->secondKey, pDest, pDest, numBytes);
    if (OK > status)
        goto exit;

    status = DES_encipher(&p_3desContext->thirdKey,  pDest, pDest, numBytes);

exit:
    return status;

} /* THREE_DES_encipher */


/*------------------------------------------------------------------*/

extern MSTATUS
THREE_DES_decipher(ctx3des *p_3desContext, ubyte *pSrc, ubyte *pDest, ubyte4 numBytes)
{
    MSTATUS status;

    if ((NULL == p_3desContext) || (NULL == pSrc) || (NULL == pDest))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    status = DES_decipher(&p_3desContext->thirdKey,  pSrc, pDest, numBytes);
    if (OK > status)
        goto exit;

    status = DES_encipher(&p_3desContext->secondKey, pDest, pDest, numBytes);
    if (OK > status)
        goto exit;

    status = DES_decipher(&p_3desContext->firstKey,  pDest, pDest, numBytes);

exit:
    return status;

} /* THREE_DES_decipher */

#if (!defined(__3DES_HARDWARE_CIPHER__))
/*------------------------------------------------------------------*/

extern BulkCtx
Create3DESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt)
{
    DES3Ctx* ctx = NULL;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_3DES))
        return NULL;
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    ctx = MALLOC(sizeof(DES3Ctx));

    if (NULL != ctx)
    {
        if (encrypt)
        {
            if (OK > THREE_DES_initKey(&(ctx->encryptKey), keyMaterial, keyLength))
            {
                FREE(ctx);  ctx = NULL;
            }
        }
        else
        {
            if (OK > THREE_DES_initKey(&(ctx->decryptKey), keyMaterial, keyLength))
            {
                FREE(ctx);  ctx = NULL;
            }
        }
    }

    return ctx;
}


/*------------------------------------------------------------------*/

extern MSTATUS
Delete3DESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx)
{
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_3DES))
        return getFIPS_powerupStatus(FIPS_ALGO_3DES);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */
    if (*ctx)
    {
#ifdef __ZEROIZE_TEST__
        int counter = 0;
        FIPS_PRINT("\nTES - Before Zeroization\n");
        for( counter = 0; counter < sizeof(DES3Ctx); counter++)
        {
            FIPS_PRINT("%x",*((ubyte*)*ctx+counter));
        }
        FIPS_PRINT("\n");
#endif
        /* Zeroize the sensitive information before deleting the memory */
        MOC_MEMSET(*ctx,0x00,sizeof(DES3Ctx));
#ifdef __ZEROIZE_TEST__
        FIPS_PRINT("\nTES - After Zeroization\n");
        for( counter = 0; counter < sizeof(DES3Ctx); counter++)
        {
            FIPS_PRINT("%x",*((ubyte*)*ctx+counter));
        }
        FIPS_PRINT("\n");
#endif
        FREE(*ctx);
        *ctx = NULL;
    }

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
Do3DES(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength,
       sbyte4 encrypt, ubyte* iv)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_3DES))
        return getFIPS_powerupStatus(FIPS_ALGO_3DES);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (ctx)
    {
        DES3Ctx *p_3desContext = (DES3Ctx *)ctx;

        if (dataLength % THREE_DES_BLOCK_SIZE)
        {
            status = ERR_3DES_BAD_LENGTH;
            goto exit;
        }

        if (encrypt)
        {
            while (dataLength > 0)
            {
                /* XOR block with iv */
                sbyte4 i;

                for (i = 0; i < DES_BLOCK_SIZE; ++i)
                {
                    data[i] ^= iv[i];
                }

                /* encrypt */
                THREE_DES_encipher(&(p_3desContext->encryptKey), data, data, THREE_DES_BLOCK_SIZE);

                /* save into iv */
                MOC_MEMCPY(iv, data, DES_BLOCK_SIZE);

                /* advance */
                dataLength -= DES_BLOCK_SIZE;
                data += DES_BLOCK_SIZE;
            }
        }
        else
        {
            while ( dataLength > 0)
            {
                sbyte4 i;
                ubyte nextIV[ DES_BLOCK_SIZE];

                /* save block in next IV */
                MOC_MEMCPY( nextIV, data, DES_BLOCK_SIZE);

                /* decrypt */
                THREE_DES_decipher(&(p_3desContext->decryptKey), data, data, THREE_DES_BLOCK_SIZE);

                /* XOR with iv */
                for (i = 0; i < DES_BLOCK_SIZE; ++i)
                {
                    data[i] ^= iv[i];
                }

                /* put nextIV into iv */
                MOC_MEMCPY(iv, nextIV, DES_BLOCK_SIZE);

                /* advance */
                dataLength -= DES_BLOCK_SIZE;
                data += DES_BLOCK_SIZE;
            }
        }
    }

exit:
    return status;
}

#endif /* (!defined(__3DES_HARDWARE_CIPHER__)) */


/*------------------------------------------------------------------*/

/* support for 2keyTripleDes */
extern BulkCtx
Create2Key3DESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial,
                  sbyte4 keyLength, sbyte4 encrypt)
{
    ubyte scratch[THREE_DES_KEY_LENGTH];

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_3DES))
        return NULL;
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ((NULL == keyMaterial) || (keyLength != 2 * DES_KEY_LENGTH))
    {
        return NULL;
    }

    MOC_MEMCPY( scratch, keyMaterial, 2 * DES_KEY_LENGTH);
    MOC_MEMCPY( scratch + 2 * DES_KEY_LENGTH, keyMaterial, DES_KEY_LENGTH);

    return Create3DESCtx( MOC_SYM(hwAccelCtx) scratch, THREE_DES_KEY_LENGTH, encrypt);
}

#endif /* (!defined(__DISABLE_3DES_CIPHERS__)) */
