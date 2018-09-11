/* Version: mss_v6_3 */
/*
 * sizedbuffer.c
 *
 * Simple utility to keep track of allocated memory and its size.
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mdefs.h"
#include "../common/sizedbuffer.h"


/*------------------------------------------------------------------*/

extern MSTATUS
SB_Allocate(SizedBuffer* pBuff, ubyte2 len)
{
    MSTATUS status = OK;

    if (NULL == pBuff)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pBuff->pHeader = pBuff->data = MALLOC(len);
    if ( pBuff->data )
    {
        pBuff->length = len;
    }
    else
    {
        pBuff->length = 0;
        status = ERR_MEM_ALLOC_FAIL;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern void
SB_Release(SizedBuffer* pBuff)
{
    if ((pBuff) && (pBuff->pHeader))
    {
        FREE(pBuff->pHeader);
        pBuff->pHeader = 0;
    }
}

