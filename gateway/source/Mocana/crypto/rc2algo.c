/* Version: mss_v6_3 */
/*
 * rc2algo.c
 *
 * RC2 Algorithm
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#if (defined(__ENABLE_ARC2_CIPHERS__) && !defined(__ARC2_HARDWARE_CIPHER__))

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../crypto/rc2algo.h"
#include "../crypto/arc2.h"


/*------------------------------------------------------------------*/

extern BulkCtx
CreateRC2Ctx2(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 effectiveBits)
{
    ubyte2* xkey = (ubyte2*) MALLOC(64 * sizeof(ubyte2));

    if (xkey)
    {
        rc2_keyschedule( xkey, keyMaterial, (ubyte4) keyLength, effectiveBits);
    }

    return xkey;
}


/*------------------------------------------------------------------*/

extern BulkCtx
CreateRC2Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt)
{
    MOC_UNUSED(encrypt);
    return CreateRC2Ctx2( MOC_SYM( hwAccelCtx) keyMaterial, keyLength, keyLength * 8);
}

/*------------------------------------------------------------------*/

extern MSTATUS
DeleteRC2Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx)
{
    if (*ctx)
    {
        FREE(*ctx);
        *ctx = NULL;
    }

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
DoRC2(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength,
              sbyte4 encrypt, ubyte* iv)
{
    MSTATUS status = OK;

    if (ctx)
    {
        ubyte2 *xkey = (ubyte2*) ctx;

        if (dataLength % RC2_BLOCK_SIZE)
        {
            status = ERR_RC2_BAD_LENGTH;
            goto exit;
        }

        if (encrypt)
        {
            while (dataLength > 0)
            {
                /* XOR block with iv */
                sbyte4 i;

                for (i = 0; i < RC2_BLOCK_SIZE; ++i)
                {
                    data[i] ^= iv[i];
                }

                /* encrypt */
                rc2_encrypt( xkey, data, data);

                /* save into iv */
                MOC_MEMCPY(iv, data, RC2_BLOCK_SIZE);

                /* advance */
                dataLength -= RC2_BLOCK_SIZE;
                data += RC2_BLOCK_SIZE;
            }
        }
        else
        {
            while ( dataLength > 0)
            {
                sbyte4 i;
                ubyte nextIV[ RC2_BLOCK_SIZE];

                /* save block in next IV */
                MOC_MEMCPY( nextIV, data, RC2_BLOCK_SIZE);

                /* decrypt */
                rc2_decrypt(xkey, data, data);

                /* XOR with iv */
                for (i = 0; i < RC2_BLOCK_SIZE; ++i)
                {
                    data[i] ^= iv[i];
                }

                /* put nextIV into iv */
                MOC_MEMCPY(iv, nextIV, RC2_BLOCK_SIZE);

                /* advance */
                dataLength -= RC2_BLOCK_SIZE;
                data += RC2_BLOCK_SIZE;
            }
        }
    }

exit:
    return status;
}

#endif
