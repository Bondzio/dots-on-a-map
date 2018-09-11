/* Version: mss_v6_3 */
/*
 * rc4algo.c
 *
 * RC4 Algorithm
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#if (!defined(__DISABLE_ARC4_CIPHERS__) && !defined(__ARC4_HARDWARE_CIPHER__))

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../crypto/rc4algo.h"
#include "../crypto/arc4.h"


/*------------------------------------------------------------------*/

BulkCtx CreateRC4Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt)
{
     rc4_key* ctx = (rc4_key*)MALLOC(sizeof(rc4_key));
    MOC_UNUSED(encrypt);

     if (ctx)
     {
         prepare_key( keyMaterial, keyLength, ctx);
     }

     return ctx;
}


/*------------------------------------------------------------------*/

MSTATUS DeleteRC4Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx)
{
    if (*ctx)
    {
        FREE(*ctx);
        *ctx = NULL;
    }

    return OK;
}


/*------------------------------------------------------------------*/

MSTATUS DoRC4(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength,
              sbyte4 encrypt, ubyte* iv)
{
    MOC_UNUSED(encrypt);
    MOC_UNUSED(iv);

    if (ctx)
    {
        rc4( data, dataLength, ( rc4_key*) ctx);
    }

    return OK;
}

#endif
