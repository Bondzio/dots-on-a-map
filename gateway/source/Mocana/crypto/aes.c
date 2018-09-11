/* Version: mss_v6_3 */
/*
 * aes.c
 *
 * AES Implementation
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

#if (!defined(__DISABLE_AES_CIPHERS__) && !defined(__AES_HARDWARE_CIPHER__))

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/debug_console.h"
#include "../crypto/aesalgo.h"
#include "../crypto/aes.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif


/*------------------------------------------------------------------*/

extern BulkCtx
CreateAESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt)
{
    aesCipherContext* ctx = NULL;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES_CBC))
        return NULL;
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    ctx = MALLOC(sizeof(aesCipherContext));

    if (NULL != ctx)
    {
        MOC_MEMSET((ubyte *)ctx, 0x00, sizeof(aesCipherContext));

        if (OK > AESALGO_makeAesKey(ctx, 8 * keyLength, keyMaterial, encrypt, MODE_CBC))
        {
            FREE(ctx);  ctx = NULL;
        }
    }

    return ctx;
}

/*------------------------------------------------------------------*/

extern BulkCtx
CreateAESCFBCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt)
{
    aesCipherContext* ctx = NULL;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES_CFB))
        return NULL;
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    ctx = MALLOC(sizeof(aesCipherContext));

    if (NULL != ctx)
    {
        MOC_MEMSET((ubyte *)ctx, 0x00, sizeof(aesCipherContext));

        if (OK > AESALGO_makeAesKey(ctx, 8 * keyLength, keyMaterial, encrypt, MODE_CFB128))
        {
            FREE(ctx);  ctx = NULL;
        }
    }

    return ctx;
}

/*------------------------------------------------------------------*/

extern BulkCtx
CreateAESOFBCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt)
{
    aesCipherContext* ctx = NULL;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES_OFB))
        return NULL;
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    ctx = MALLOC(sizeof(aesCipherContext));

    if (NULL != ctx)
    {
        MOC_MEMSET((ubyte *)ctx, 0x00, sizeof(aesCipherContext));

        if (OK > AESALGO_makeAesKey(ctx, 8 * keyLength, keyMaterial, encrypt, MODE_OFB))
        {
            FREE(ctx);  ctx = NULL;
        }
    }

    return ctx;
}

/*------------------------------------------------------------------*/

extern MSTATUS
DeleteAESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx* ctx)
{
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES))
        return getFIPS_powerupStatus(FIPS_ALGO_AES);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */
    if (*ctx)
    {
#ifdef __ZEROIZE_TEST__
        int counter = 0;
        FIPS_PRINT("\nAES - Before Zeroization\n");
        for( counter = 0; counter < sizeof(aesCipherContext); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)*ctx+counter));
        }
        FIPS_PRINT("\n");
#endif
        /* Zeroize the sensitive information before deleting the memory */
        MOC_MEMSET((ubyte*)*ctx,0x00,sizeof(aesCipherContext));
#ifdef __ZEROIZE_TEST__
        FIPS_PRINT("\nAES - After Zeroization\n");
        for( counter = 0; counter < sizeof(aesCipherContext); counter++)
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

extern MSTATUS
DoAES(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv)
{
    sbyte4              retLength;
    aesCipherContext*   pAesContext = (aesCipherContext *)ctx;
    MSTATUS             status;

    if (NULL == pAesContext || (MODE_ECB != pAesContext->mode && NULL == iv))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_AES))
        return getFIPS_powerupStatus(FIPS_ALGO_AES);

   /* Need to check specific algo enabled too */
   switch (pAesContext->mode)
   {
       case MODE_ECB:
    	   status = getFIPS_powerupStatus(FIPS_ALGO_AES_ECB);
    	   break;
       case MODE_CBC:
    	   status = getFIPS_powerupStatus(FIPS_ALGO_AES_CBC);
    	   break;
#if 0
        case MODE_CFB1:
     	   status = getFIPS_powerupStatus(FIPS_ALGO_AES_CFB);
     	   break;
#endif
        case MODE_CFB128:
     	   status = getFIPS_powerupStatus(FIPS_ALGO_AES_CFB);
     	   break;
        case MODE_OFB:
     	   status = getFIPS_powerupStatus(FIPS_ALGO_AES_OFB);
     	   break;
   }

   if (OK != status)
        return status;

#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (0 != (dataLength % AES_BLOCK_SIZE))
    {
        status = ERR_AES_BAD_LENGTH;
        goto exit;
    }

    if (encrypt)
        status = AESALGO_blockEncrypt(pAesContext, iv, data, 8 * dataLength, data, &retLength);
    else
        status = AESALGO_blockDecrypt(pAesContext, iv, data, 8 * dataLength, data, &retLength);

#ifdef __ENABLE_ALL_DEBUGGING__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte *)"DoAES: cipher failed, error = ", status);
#endif

exit:
    return status;
}

#endif /* (!defined(__DISABLE_AES_CIPHERS__) && !defined(__AES_HARDWARE_CIPHER__)) */

