/* Version: mss_v6_3 */
/*
 * nil.c
 *
 * NIL Encipher & Decipher
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/debug_console.h"
#include "../crypto/nil.h"


/*------------------------------------------------------------------*/

#ifdef __ENABLE_NIL_CIPHER__

extern BulkCtx
CreateNilCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt)
{
    static sbyte4 dummy = 0;
    MOC_UNUSED(keyMaterial);
    MOC_UNUSED(keyLength);
    MOC_UNUSED(encrypt);

    return (BulkCtx)(&dummy);
}


/*------------------------------------------------------------------*/

extern MSTATUS
DeleteNilCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx* ctx)
{
    if (NULL == ctx)
        return ERR_NULL_POINTER;

    *ctx = NULL;

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DoNil(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv)
{
    MOC_UNUSED(ctx);
    MOC_UNUSED(data);
    MOC_UNUSED(dataLength);
    MOC_UNUSED(encrypt);
    MOC_UNUSED(iv);

    return OK;
}

#endif /* __ENABLE_NIL_CIPHER__ */
