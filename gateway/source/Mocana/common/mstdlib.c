/* Version: mss_v6_3 */
/*
 * mstdlib.c
 *
 * Mocana Standard Library
 *
 * Copyright Mocana Corp 2003-2011. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#ifdef __ENABLE_MOCANA_SUPPORT_FOR_NATIVE_STDLIB__
#if (defined(__KERNEL__) && (defined(__LINUX_RTOS__) || defined(__ANDROID_RTOS__)))
#include <linux/types.h>
#include <linux/string.h>
#else
#include <string.h>
#endif
#endif /*__ENABLE_MOCANA_SUPPORT_FOR_NATIVE_STDLIB__ */


/*------------------------------------------------------------------*/

#define MOC_MALLOC_MAX_BUF_SIZE     (131071L)


/*------------------------------------------------------------------*/

extern ubyte2 SWAPWORD(ubyte2 a)
{
    return (ubyte2)((a << 8) | (a >> 8));
}


/*------------------------------------------------------------------*/

extern ubyte4 SWAPDWORD(ubyte4 a)
{
    return ((a << 24) |
            ((a << 8) & 0x00ff0000) |
            ((a >> 8) & 0x0000ff00) |
            (a >> 24));
}


/*------------------------------------------------------------------*/

extern ubyte4
MOC_NTOHL(const ubyte *v)
{
    return (ubyte4)((((ubyte4)(v[0])) << 24) | (((ubyte4)(v[1])) << 16) | (((ubyte4)(v[2])) <<8 ) | ((ubyte4)(v[3])));
}


/*------------------------------------------------------------------*/

extern ubyte2
MOC_NTOHS(const ubyte *v)
{
    return (ubyte2)((((ubyte4)(v[0])) << 8) | v[1]);
}


/*------------------------------------------------------------------*/

extern void
MOC_HTONL(ubyte n[4], ubyte4 h)
{
    n[0] = (ubyte)((h >> 24) & 0xFF);
    n[1] = (ubyte)((h >> 16) & 0xFF);
    n[2] = (ubyte)((h >> 8)  & 0xFF);
    n[3] = (ubyte)( h        & 0xFF);
}


/*------------------------------------------------------------------*/

extern void
MOC_HTONS(ubyte n[2], ubyte2 h)
{
    n[0] = (ubyte)((h >> 8) & 0xFF);
    n[1] = (ubyte)( h       & 0xFF);
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_MEMMOVE(ubyte *pDest, const ubyte *pSrc, sbyte4 len)
{
    MSTATUS status = OK;

    if ((NULL == pDest) || (NULL == pSrc))
    {
        status = ERR_NULL_POINTER;
    }
    else
    {
#ifdef __ENABLE_MOCANA_SUPPORT_FOR_NATIVE_STDLIB__
        memmove(pDest, pSrc, len);
#else
        if ((pSrc > pDest) || (pDest >= pSrc + len))
        {
            while (0 < len)
            {
                *pDest = *pSrc;
                pDest++;
                pSrc++;
                len--;
            }
        }
        else
        {
            pSrc += len - 1;
            pDest += len - 1;

            while (0 < len)
            {
                *pDest = *pSrc;
                pDest--;
                pSrc--;
                len--;
            }
        }
#endif
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_MEMCPY(void *pDest1, const void *pSrc1, sbyte4 len)
{
    MSTATUS         status = OK;

    if ((NULL == pDest1) || (NULL == pSrc1))
        status = ERR_NULL_POINTER;
    else
    {
#ifdef __ENABLE_MOCANA_SUPPORT_FOR_NATIVE_STDLIB__
        memcpy(pDest1, pSrc1, len);
#else
        ubyte*          pDest  = (ubyte*)pDest1;
        const ubyte*    pSrc   = (const ubyte*)pSrc1;

        while (0 < len)
        {
            *pDest = *pSrc;
            pDest++;
            pSrc++;
            len--;
        }
#endif
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_MEMCMP(const ubyte *pSrc1, const ubyte *pSrc2, usize len, sbyte4 *pResult)
{
    MSTATUS status = OK;

    if ((NULL == pSrc1) || (NULL == pSrc2) || (NULL == pResult))
        status = ERR_NULL_POINTER;
    else
    {
#ifdef __ENABLE_MOCANA_SUPPORT_FOR_NATIVE_STDLIB__
        *pResult = memcmp(pSrc1, pSrc2, len);
#else
        *pResult = 0;

        while (((ubyte4)0 < len) && ((ubyte4)0 == (*pResult = (sbyte4)(*pSrc1 - *pSrc2))))
        {
            pSrc1++;
            pSrc2++;
            len--;
        }

        *pResult = ((sbyte4)0 < *pResult) ? (sbyte4) 1 : (((sbyte4) 0 == *pResult) ? (sbyte4) 0 :(sbyte4) -1);
#endif
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_SAFEMATCH(const ubyte *pTrustedSrc,   ubyte4 trustedSrcLen,
              const ubyte *pUntrustedSrc, ubyte4 untrustedLen,
              intBoolean *pResult)
{
    /* not to be confused with memcmp(). this code is resistant to a timing attack. */
    /* if bytes are identical match, *pResult will be TRUE */
    ubyte4  index;
    ubyte4  result;
    MSTATUS status = OK;

    if ((NULL == pTrustedSrc) || (NULL == pUntrustedSrc) || (NULL == pResult))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    result = trustedSrcLen ^ untrustedLen;

    for (index = 0; index < untrustedLen; index++)
        result = (result | (pTrustedSrc[index % trustedSrcLen] ^ pUntrustedSrc[index]));

    *pResult = (0 == result) ? TRUE : FALSE;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_MEMSET(ubyte *pDest, ubyte value, usize len)
{
    MSTATUS status = OK;

    if (NULL == pDest)
        status = ERR_NULL_POINTER;
    else
    {
#ifdef __ENABLE_MOCANA_SUPPORT_FOR_NATIVE_STDLIB__
        memset(pDest, value, len);
#else
        while (0 < len)
        {
            *pDest = value;
            pDest++;
            len--;
        }
#endif
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_XORCPY(void *pDst, const void *pSrc, ubyte4 numBytes)
{
    MSTATUS status = OK;

    if ((NULL == pDst) || (NULL == pSrc))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    while (numBytes)
    {
        *((ubyte *)pDst) ^= *((ubyte *)pSrc);
        pDst = (ubyte *) pDst + 1;
        pSrc = (ubyte *) pSrc + 1;
        numBytes--;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern ubyte
returnHexDigit(ubyte4 digit)
{
    digit &= 0x0f;

    if (0x0a > digit)
        return (ubyte)(digit + '0');

    return (ubyte)((digit - 10) + 'a');
}


/*------------------------------------------------------------------*/

extern sbyte
MTOLOWER(sbyte c)
{
    if (('A' <= c) && ('Z' >= c))
    {
        c += 'a' - 'A';
    }

    return c;
}


/*------------------------------------------------------------------*/

extern byteBoolean
MOC_ISSPACE(sbyte c)
{
    return ( (0x20 == c) || (( 0x09 <= c) && (c <= 0x0D)));
}

/*------------------------------------------------------------------*/
/* LWS  = [CRLF] 1*( SP | HT ) defined in RFC 2616
*/
extern byteBoolean
MOC_ISLWS( sbyte c)
{
    return ( (0x20 == c) || (0x09 == c) || (0x0A == c) || (0x0D == c));
}

/*------------------------------------------------------------------*/

extern byteBoolean
MOC_ISXDIGIT( sbyte c)
{
    return ( (c>= '0' && c <= '9') ||
             (c>= 'a' && c <= 'f') ||
             (c>= 'A' && c <= 'F') );
}


/*------------------------------------------------------------------*/

extern byteBoolean
MOC_ISASCII( sbyte c)
{
    return ( (c & ~0x7f) == 0 );
}


/*------------------------------------------------------------------*/

extern byteBoolean
MOC_ISDIGIT( sbyte c)
{
    return ( (c>= '0' && c <= '9') );
}


/*------------------------------------------------------------------*/

extern byteBoolean
MOC_ISLOWER( sbyte c)
{
    return ( (c>= 'a' && c <= 'z') );
}


/*------------------------------------------------------------------*/

extern ubyte4
MOC_STRLEN(const sbyte *s)
{
    const sbyte *t = s;

    while (0 != *t)
        t++;

    return (ubyte4)(t - s);
}


/*------------------------------------------------------------------*/

extern sbyte4
MOC_STRCMP(const sbyte *pString1, const sbyte *pString2)
{
    while (('\0' != *pString1) && (*pString1 == *pString2))
    {
        pString1++;
        pString2++;
    }

    return ((*pString1) - (*pString2));
}


/*------------------------------------------------------------------*/

extern sbyte4
MOC_STRNICMP(const sbyte *pString1, const sbyte *pString2, ubyte4 n)
{
    ubyte4 i;

    for (i = 0; i < n; ++i)
    {
        sbyte c1 = MTOLOWER(*pString1++);
        sbyte c2 = MTOLOWER(*pString2++);

        if ( 0 == c1 || 0 == c2 || c1!=c2)
            return c1-c2;
    }
    return 0;
}


/*------------------------------------------------------------------*/

extern ubyte4
MOC_STRCBCPY( sbyte* dest, ubyte4 destSize, const sbyte* src)
{
    ubyte4 i = 0;

    if (0 == dest || 0 == destSize || 0 == src)
    {
        return 0;
    }

    while ( i < destSize && (dest[i] = *src++) )
    {
        ++i;
    }

    if ( i == destSize)
    {
        /* destSize >= 1 */
        dest[--i] = 0; /* --len && NUL terminate */
    }

    return i;
}


/*------------------------------------------------------------------*/
extern ubyte4
MOC_STRCAT( sbyte* dest, const sbyte* addsrc)
{
	ubyte4 len = 0;

    if (NULL == dest || NULL == addsrc)
    {
        return 0;
    }

	for (; *dest; ++dest)
	{
		len++;
	}

	while ((*dest++ = *addsrc++) != 0)
	{
		len++;
	}

	return len;

}
/*------------------------------------------------------------------*/

extern sbyte*
MOC_STRCHR(sbyte *s, sbyte c, ubyte4 len)
{
    while((0 < len) && ('\0' != *s))
    {
        if (MTOLOWER(*s) == MTOLOWER(c))
            return (s);
        s++;
        len--;
    }
    return (NULL);
}

/*------------------------------------------------------------------*/

extern sbyte4
MOC_ATOL(const sbyte* s, const sbyte** stop)
{
    sbyte4 sign = 1;
    sbyte4 retVal = 0;

    /* white space */
    while ( (*s >= 0x9 && *s <= 0xD) || (0x20 == *s) )
    {
        ++s;
    }

    /* sign if any */
    if ('-' == *s)
    {
        sign = -1;
        ++s;
    }

    /* decimal part */
    while (*s >= '0' && *s <= '9')
    {
        retVal *= 10;
        retVal += *s - '0';
        ++s;
    }

    if ( stop)
    {
        *stop = (sbyte *)s;
    }

    return sign * retVal;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_MALLOC(void **ppPtr, ubyte4 bufSize)
{
    MSTATUS status;

    if (NULL == ppPtr)
    {
        status = ERR_MEM_ALLOC_PTR;
        goto exit;
    }

    if (MOC_MALLOC_MAX_BUF_SIZE < bufSize)
    {
        *ppPtr = NULL;
        status = ERR_MEM_ALLOC_SIZE;
        goto exit;
    }

    if (NULL == (*ppPtr = MALLOC(bufSize)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_FREE(void **ppPtr)
{
    MSTATUS status;

    if ((NULL == ppPtr) || (NULL == *ppPtr))
    {
        status = ERR_MEM_FREE_PTR;
        goto exit;
    }

    FREE(*ppPtr);
    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_UTOA(ubyte4 value, ubyte *pRetResult, ubyte4 *pRetNumDigitsLong)
{
    ubyte4     divisor = 1000000000UL;
    ubyte4     digit;
    intBoolean isLeadingZero = TRUE;

    *pRetResult = '0';
    *pRetNumDigitsLong = 0;

    while ((divisor > value) && (divisor))
        divisor = divisor / 10;

    while (0 < divisor)
    {
        digit = (value / divisor);

        if (0 != digit)
            isLeadingZero = FALSE;

        if ((digit) || (FALSE == isLeadingZero))
        {
            *pRetResult = (ubyte)(digit + '0');
            (*pRetNumDigitsLong)++;
            pRetResult++;
        }

        value = value - (digit * divisor);
        divisor = divisor / 10;
    }

    if (!(*pRetNumDigitsLong))
        *pRetNumDigitsLong = 1;

    return OK;
}


/*------------------------------------------------------------------*/

extern sbyte*
MOC_LTOA(sbyte4 value, sbyte *buff, ubyte4 bufSize)
{
    sbyte* p = buff;
    sbyte* retVal;

    if (!buff) return buff;


    do
    {
        if ( !bufSize)
        {
            return NULL;
        }

        *p++ = (sbyte)('0' + (value % 10));
        value /= 10;
        --bufSize;
    } while ( value);


    retVal = p;

    /* everything is in the wrong order -> reverse bytes */
    while ( --p > buff)
    {
        sbyte c = *p;
        *p = *buff;
        *buff++ = c;
    }

    return retVal;
}


/*---------------------------------------------------------------------------*/

extern sbyte4
MOC_DAYOFWEEK( sbyte4 d, sbyte4 m, sbyte4 y)
{
    /* C FAQ Tomohiko Sakamoto */
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}


/*------------------------------------------------------------------*/

extern ubyte4
MOC_BITLENGTH( ubyte4 w)
{
    ubyte4 numBits = 0;

    static const ubyte lookupBits[32] =
    {
        3,4,5,5,6,6,6,6,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
    };

    if (w & 0xff000000L)
    {
        numBits += 24;
        w >>= 24;
    }
    else if (w & 0x00ff0000L)
    {
        numBits += 16;
        w >>= 16;
    }
    else if (w & 0x0000ff00L)
    {
        numBits += 8;
        w >>= 8;
    }

    if (0 == (w & 0xf8))
    {
        numBits += lookupBits[w & 7] - 3;               /* value = 0..7 */
    }
    else
    {
        numBits += lookupBits[((w >> 3) & 0x1f)];       /* value = 8..255 */
    }
    return numBits;
}


/*------------------------------------------------------------------*/

extern ubyte4
MOC_BITCOUNT( ubyte4 v)
{
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_alloc(usize size, void **ppRetAllocBuf)
{
    MSTATUS status = OK;

    if (NULL == (*ppRetAllocBuf = MALLOC(size)))
        status = ERR_MEM_ALLOC_FAIL;

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MOC_free(void **ppFreeAllocBuf)
{
    MSTATUS status = ERR_NULL_POINTER;

    if ((NULL != ppFreeAllocBuf) && (NULL != *ppFreeAllocBuf))
    {
        FREE(*ppFreeAllocBuf);
        *ppFreeAllocBuf = NULL;
        status = OK;
    }

    return status;
}


/*------------------------------------------------------------------*/

extern ubyte4
MOC_floorPower2(ubyte4 value)
{
    value = (value | (value >> 1));
    value = (value | (value >> 2));
    value = (value | (value >> 4));
    value = (value | (value >> 8));
    value = (value | (value >> 16));

    return (value - (value >> 1));
}

