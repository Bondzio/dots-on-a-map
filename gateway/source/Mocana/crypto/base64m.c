/* Version: mss_v6_3 */
/*
 * base64.c
 *
 * Base64 Encoder & Decoder
 *
 * Copyright Mocana Corp 2003-2013. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../crypto/base64.h"


/*------------------------------------------------------------------*/

static const ubyte m_encBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* static ubyte*      mp_decBase64; */

static const ubyte m_decBase64[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x40,
    0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c,
    0x3d, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
    0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*------------------------------------------------------------------*/

/* deprecated */
extern MSTATUS
BASE64_initializeContext(void)
{
/*
    ubyte4  index;
    MSTATUS status = OK;

    if (NULL == (mp_decBase64 = MALLOC(256)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET(mp_decBase64, 0x00, 256);

    for (index = 0; index < sizeof(m_encBase64) - 1; index++)
        mp_decBase64[m_encBase64[index]] = (ubyte)(index + 1);

exit:
    return status;
*/
    return OK;
}


/*------------------------------------------------------------------*/

/* deprecated */
extern MSTATUS
BASE64_freeContext(void)
{
/*
    MSTATUS status = OK;

    if (NULL == mp_decBase64)
        status = ERR_NULL_POINTER;
    else
    {
        FREE(mp_decBase64);
        mp_decBase64 = NULL;
    }

    return status;
*/
    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
BASE64_encodeMessage(const ubyte *pOrigMesg, ubyte4 origLen,
                     ubyte **ppRetMesg, ubyte4 *pRetMesgLen)
{
    ubyte4  chunk;
    ubyte4  numChunks;
    ubyte4  remChunk;
    ubyte4  index1, index2;
    ubyte4  value;
    MSTATUS status = OK;

    if ((NULL == pOrigMesg) || (NULL == ppRetMesg) || (NULL == pRetMesgLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    numChunks    = origLen / 3;
    remChunk     = origLen % 3;

    *pRetMesgLen = (numChunks + (remChunk ? 1 : 0)) * 4;

    /* allocate one extra byte in case we wish to null terminate the string later */
    if (NULL == (*ppRetMesg = MALLOC(1 + *pRetMesgLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    for (index1 = index2 = chunk = 0; chunk < numChunks; chunk++)
    {
        /* process in chunks of three */
        value  = (ubyte4)(pOrigMesg[index1]    ); value <<= 8;
        value |= (ubyte4)(pOrigMesg[index1 + 1]); value <<= 8;
        value |= (ubyte4)(pOrigMesg[index1 + 2]);

        /* output in pieces of four */
        (*ppRetMesg)[index2 + 3] = m_encBase64[value & 0x3f]; value >>= 6;
        (*ppRetMesg)[index2 + 2] = m_encBase64[value & 0x3f]; value >>= 6;
        (*ppRetMesg)[index2 + 1] = m_encBase64[value & 0x3f]; value >>= 6;
        (*ppRetMesg)[index2]     = m_encBase64[value & 0x3f];

        /* adjust indices */
        index1 += 3;
        index2 += 4;
    }

    if (1 == remChunk)
    {
        value  = (pOrigMesg[index1]    ); value <<= 16;

        (*ppRetMesg)[index2 + 3] = '=';                       value >>= 6;
        (*ppRetMesg)[index2 + 2] = '=';                       value >>= 6;
        (*ppRetMesg)[index2 + 1] = m_encBase64[value & 0x3f]; value >>= 6;
        (*ppRetMesg)[index2]     = m_encBase64[value & 0x3f];
    }
    else if (remChunk)
    {
        value  = (pOrigMesg[index1]    ); value <<= 8;
        value |= (pOrigMesg[index1 + 1]); value <<= 8;

        /* output in pieces of four */
        (*ppRetMesg)[index2 + 3] = '=';                       value >>= 6;
        (*ppRetMesg)[index2 + 2] = m_encBase64[value & 0x3f]; value >>= 6;
        (*ppRetMesg)[index2 + 1] = m_encBase64[value & 0x3f]; value >>= 6;
        (*ppRetMesg)[index2]     = m_encBase64[value & 0x3f];
    }

exit:
    return status;

} /* BASE64_encodeMessage */


/*------------------------------------------------------------------*/

extern MSTATUS
BASE64_decodeMessage(const ubyte *pOrigMesg, ubyte4 origLen,
                     ubyte **ppRetMesg, ubyte4 *pRetMesgLen)
{
    ubyte4  chunk;
    ubyte4  numChunks;
    ubyte4  remChunk;
    ubyte4  index1, index2;
    ubyte4  value = 0;
    ubyte4  tmpValue;
    ubyte4  phase;
    MSTATUS status = ERR_BASE64_BAD_INPUT;

    if (NULL != ppRetMesg) *ppRetMesg = NULL;

    if ((NULL == pOrigMesg) || (NULL == ppRetMesg) || (NULL == pRetMesgLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* messages should be multiples of 4 */
    numChunks    = origLen >> 2;
    remChunk     = origLen &  0x03;

    if ((0 == numChunks) || (0 != remChunk))
        goto exit;

    *pRetMesgLen = numChunks * 3;

    if (NULL == (*ppRetMesg = MALLOC(*pRetMesgLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    for (index1 = index2 = chunk = 0; chunk < numChunks; chunk++)
    {
        /* process in chunks of four */
        if (0 == (tmpValue = m_decBase64[pOrigMesg[index1]]))
        {
            phase = 0;
            goto endMesg;
        }
        value  = tmpValue-1; value <<= 6;

        if (0 == (tmpValue = m_decBase64[pOrigMesg[index1 + 1]]))
        {
            phase = 1;
            goto endMesg;
        }
        value |= tmpValue-1; value <<= 6;

        if (0 == (tmpValue = m_decBase64[pOrigMesg[index1 + 2]]))
        {
            phase = 2;
            goto endMesg;
        }
        value |= tmpValue-1; value <<= 6;

        if (0 == (tmpValue = m_decBase64[pOrigMesg[index1 + 3]]))
        {
            phase = 3;
            goto endMesg;
        }
        value |= tmpValue-1;

        /* output in pieces of three */
        (*ppRetMesg)[index2 + 2] = (ubyte)value; value >>= 8;
        (*ppRetMesg)[index2 + 1] = (ubyte)value; value >>= 8;
        (*ppRetMesg)[index2]     = (ubyte)value;

        /* adjust indices */
        index1 += 4;
        index2 += 3;
    }

    phase = 4;

endMesg:
    /* deal with remaining data */
    if (4 > phase)
    {
        if ((2 > phase) || ('=' != pOrigMesg[index1 + phase]) || (chunk < (numChunks-1)))
            goto exit;

        /* adjust for trailing equal signs */
        *pRetMesgLen -= (4 - phase);

        /* write out remnant bytes */
        if (3 == phase)
            (*ppRetMesg)[index2 + 1] = (ubyte)(value >> 8);
        else
            value <<= 6;

        (*ppRetMesg)[index2] = (ubyte)(value >> 16);
    }

    status = OK;

exit:
    if ((OK > status) && (NULL != ppRetMesg) && (NULL != *ppRetMesg))
    {
        FREE(*ppRetMesg);
        *ppRetMesg = NULL;
    }
    return status;
} /* BASE64_decodeMessage */


/*------------------------------------------------------------------*/

extern MSTATUS
BASE64_freeMessage(ubyte **ppMessage)
{
    MSTATUS status = OK;

    if ((NULL == ppMessage) || (NULL == *ppMessage))
        status = ERR_NULL_POINTER;
    else
    {
        FREE(*ppMessage);
        *ppMessage = NULL;
    }

    return status;
}
