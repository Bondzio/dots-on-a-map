/* Version: mss_v6_3 */
/*
 * vlong.c
 *
 * Very Long Integer Library
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
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
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/memory_debug.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif

#include "../common/asm_math.h"

#ifndef VLONG_MAX_LENGTH
#define VLONG_MAX_LENGTH    (8192)
#endif

#ifdef __MOCANA_ENABLE_LONG_LONG__
#ifdef __MOCANA_ENABLE_64_BIT__
#error "Mocana: cannot define both __MOCANA_ENABLE_LONG_LONG__ and __MOCANA_ENABLE_64_BIT__"
#endif
#ifndef __WIN32_RTOS__
#define UBYTE8  unsigned long long
#else
#define UBYTE8  unsigned __int64
#endif
#endif

#ifdef __ALTIVEC__
#include <altivec.h>
#endif

#if defined(__ALTIVEC__) || defined(__SSE2__)

#if defined (__RTOS_OSX__)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif


/* for altivec, we make sure the units are allocated on 16 byte boundaries */
static vlong_unit*
VLONG_allocUnit( ubyte4 size)
{
#if ( defined(__RTOS_CYGWIN__) || defined(__RTOS_SOLARIS__) || defined(__RTOS_ANDROID__))
    return ( memalign( 16, size));
#elif defined(__RTOS_OSX__)
    /* mac OS X malloc always returns 16 byte aligned memory */
    return malloc( size);
#else
    vlong_unit* retVal;
    return ( posix_memalign( (void**)&retVal, 16, size)) ? 0: retVal;
#endif
}
#define UNITS_MALLOC(a)   VLONG_allocUnit(a)
#define UNITS_FREE(a)     free(a)
#else /* not __ALTIVEC__ */
#define UNITS_MALLOC(a)   MALLOC(a)
#define UNITS_FREE(a)     FREE(a)
#endif  /* __ALTIVEC__ */

#ifdef __ENABLE_MOCANA_SMALL_CODE_FOOTPRINT__
#ifndef __DISABLE_MOCANA_MODEXP_SLIDING_WINDOW__
#define __DISABLE_MOCANA_MODEXP_SLIDING_WINDOW__
#endif
#ifndef __DISABLE_MOCANA_BARRETT__
#define __DISABLE_MOCANA_BARRETT__
#endif
#ifndef __DISABLE_MOCANA_KARATSUBA__
#define __DISABLE_MOCANA_KARATSUBA__
#endif
#endif


#if !defined(__DISABLE_MOCANA_KARATSUBA__)
static MSTATUS fasterUnsignedMultiplyVlongs(vlong *pProduct,
                                            const vlong *pFactorA,
                                            const vlong *pFactorB,
                                            ubyte4 numUnits);
static MSTATUS fasterUnsignedSqrVlong(vlong *pProduct,
                                      const vlong *pFactorA,
                                      ubyte4 numUnits);
#endif

#if !defined( __ALTIVEC__) && !defined(__DISABLE_MOCANA_KARATSUBA__)
#define VLONG_FAST_MULT   fasterUnsignedMultiplyVlongs
#define VLONG_FAST_SQR     fasterUnsignedSqrVlong
#else
#define VLONG_FAST_MULT   fastUnsignedMultiplyVlongs
#define VLONG_FAST_SQR    fastUnsignedSqrVlong
#endif

/* Macros used to abstract operations */
#define BPU     (8 * sizeof(vlong_unit))        /* bits per unit */
#define HALF_MASK         (((vlong_unit)1) << (BPU-1))
#define LO_MASK           ((((vlong_unit)1)<<(BPU/2))-1)
#define LO_HUNIT(x)       ((x) & LO_MASK)             /* lower half */
#define HI_HUNIT(x)       ((x) >> (BPU/2))            /* upper half */
#define MAKE_HI_HUNIT(x)  (((vlong_unit)(x)) << (BPU/2))  /* make upper half */
#define MAKE_UNIT(h,l)    (MAKE_HI_HUNIT(h) | ((vlong_unit)(l)))
#define ZERO_UNIT          ((vlong_unit)0)
#define FULL_MASK         (~ZERO_UNIT)

#ifdef ASM_BIT_LENGTH
static ubyte4 BITLENGTH(vlong_unit w)
{
    vlong_unit bitlen;
    ASM_BIT_LENGTH(w, bitlen);
    return (ubyte4) bitlen;
}
#else
# ifdef __ENABLE_MOCANA_64_BIT__
static ubyte4 BITLENGTH(vlong_unit w)
{
    ubyte4 hi = (ubyte4) HI_HUNIT(w);
    return (hi) ? 32 + MOC_BITLENGTH(hi) :
        MOC_BITLENGTH( (ubyte4) LO_HUNIT(w));
}
# else
# define BITLENGTH(w) MOC_BITLENGTH(w)
# endif
#endif  /* ASM_BIT_LENGTH */

#ifndef __KERNEL__
#if 0
#include <stdio.h>

static void PrintVLong(const char* msg, const vlong* v)
{
#if 0
    ubyte* buffer;
    sbyte4 bufferLen;
    sbyte4 i;

    VLONG_byteStringFromVlong(v, NULL, &bufferLen);

    buffer = MALLOC( bufferLen+1);

    VLONG_byteStringFromVlong(v, buffer, &bufferLen);

    printf("%s\n", msg);
    for (i = 0; i < bufferLen; ++i)
    {
        printf("%02X", buffer[i]);
        if ( 23 == i % 24)
        {
            printf("\\\n");
        }
    }
    printf("\n");

    FREE(buffer);
#else
#endif
}
#endif

#endif

/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_allocVlong(vlong **ppRetVlongValue, vlong **ppVlongQueue)
{
    MSTATUS status = OK;

    if (NULL == ppRetVlongValue)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if ((NULL == ppVlongQueue) || (NULL == *ppVlongQueue))
    {
        if (NULL == (*ppRetVlongValue = MALLOC(sizeof(vlong))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
        else
        {
            /* status = MOC_MEMSET((ubyte *)(*ppRetVlongValue), 0x00, sizeof(vlong)); */
            (*ppRetVlongValue)->numUnitsAllocated = 0;
            (*ppRetVlongValue)->pUnits       = NULL;
        }
    }
    else
    {
        *ppRetVlongValue = *ppVlongQueue;               /* remove head of list */
        *ppVlongQueue    = (*ppVlongQueue)->pNextVlong; /* adjust head of list */
    }

    (*ppRetVlongValue)->negative     = 0;
    (*ppRetVlongValue)->numUnitsUsed = 0;
    (*ppRetVlongValue)->pNextVlong   = NULL;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_freeVlong(vlong **ppFreeVlong, vlong **ppVlongQueue)
{
    sbyte4  i;
    MSTATUS status = OK;

    if ((NULL == ppFreeVlong) || (NULL == *ppFreeVlong))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL == ppVlongQueue)
    {
        if (NULL != (*ppFreeVlong)->pUnits)
        {
#ifdef __ZEROIZE_TEST__
            FIPS_PRINT("\nVlong Unit - Before Zeroization\n");

            i = (*ppFreeVlong)->numUnitsAllocated;

            /* clear vlong memory */
            while (i)
            {
                i--;
                FIPS_PRINT(" %d",(*ppFreeVlong)->pUnits[i]);
            }

            FIPS_PRINT("\n");
#endif

            i = (*ppFreeVlong)->numUnitsAllocated;

            /* clear vlong memory */
            while (i)
            {
                i--;
                (*ppFreeVlong)->pUnits[i] = 0x00;
            }

#ifdef __ZEROIZE_TEST__
            FIPS_PRINT("\nVlong Unit - After Zeroization\n");

            i = (*ppFreeVlong)->numUnitsAllocated;

            /* clear vlong memory */
            while (i)
            {
                i--;
                FIPS_PRINT(" %d",(*ppFreeVlong)->pUnits[i]);
            }

            FIPS_PRINT("\n");
#endif

            UNITS_FREE((*ppFreeVlong)->pUnits);
        }

        FREE(*ppFreeVlong);
    }
    else
    {
        /* add released vlong to head of vlong queue */
        (*ppFreeVlong)->pNextVlong = *ppVlongQueue;
        *ppVlongQueue = *ppFreeVlong;
    }

    /* clear pointer */
    *ppFreeVlong = NULL;

exit:
    return status;

} /* VLONG_freeVlong */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_freeVlongQueue(vlong **ppVlongQueue)
{
    vlong* pVlong ;

    if (NULL != ppVlongQueue)
    {
        while (NULL != (pVlong = *ppVlongQueue))
        {
            *ppVlongQueue = pVlong->pNextVlong;
            VLONG_freeVlong(&pVlong, NULL);
        }
    }

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_reallocVlong(vlong *pThis, ubyte4 vlongNewLength)
{
    MSTATUS status = OK;

    if (VLONG_MAX_LENGTH < vlongNewLength)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    if (vlongNewLength > pThis->numUnitsAllocated)
    {
        vlong_unit* pNewArrayUnits;
        ubyte4  index;

        vlongNewLength += 3;
#if defined( __ALTIVEC__) || defined(__SSE2__)
        vlongNewLength = MOC_PAD( vlongNewLength, 4);
#elif defined(__ARM_NEON__) || defined(__ARM_V6__)
        vlongNewLength = MOC_PAD( vlongNewLength, 2);
#endif

        if (NULL == (pNewArrayUnits =
                     (vlong_unit*) UNITS_MALLOC(vlongNewLength * sizeof(vlong_unit))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        for (index = 0 ; index < pThis->numUnitsUsed; index++)
            pNewArrayUnits[index] = pThis->pUnits[index];

        if (NULL != pThis->pUnits)
            UNITS_FREE(pThis->pUnits);

        pThis->pUnits = pNewArrayUnits;
        pThis->numUnitsAllocated = vlongNewLength;
    }

exit:
    return status;

} /* VLONG_reallocVlong */


/*------------------------------------------------------------------*/

static MSTATUS
expandVlong(vlong *pThis, ubyte4 vlongNewLength)
{
    MSTATUS status = OK;

    if (VLONG_MAX_LENGTH < vlongNewLength)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    if (vlongNewLength > pThis->numUnitsAllocated)
    {
        vlong_unit* pNewArrayUnits;

        vlongNewLength += 1;
#if defined( __ALTIVEC__) || defined(__SSE2__)
        vlongNewLength = MOC_PAD( vlongNewLength, 4);
#elif defined(__ARM_NEON__) || defined(__ARM_V6__)
        vlongNewLength = MOC_PAD( vlongNewLength, 2);
#endif

        if (NULL == (pNewArrayUnits =
                    (vlong_unit*)  UNITS_MALLOC(vlongNewLength * sizeof(vlong_unit))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        if (NULL != pThis->pUnits)
            UNITS_FREE(pThis->pUnits);

        pThis->pUnits = pNewArrayUnits;
        pThis->numUnitsAllocated = vlongNewLength;
        pThis->numUnitsUsed = 0;
    }

exit:
    return status;

} /* expandVlong */


/*------------------------------------------------------------------*/

extern vlong_unit
VLONG_getVlongUnit(const vlong *pThis, ubyte4 index)
{
    vlong_unit result = 0;

    if (index < pThis->numUnitsUsed)
        if (NULL != pThis->pUnits)
            result = pThis->pUnits[index];

    return result;
}


/*------------------------------------------------------------------*/

static ubyte
getByte(const vlong *pThis, ubyte4 byteIndex)
{
    vlong_unit unit = VLONG_getVlongUnit(pThis, byteIndex / (sizeof(vlong_unit)));

    byteIndex %= sizeof(vlong_unit);

    unit >>= (byteIndex << 3);

    return ((ubyte)(unit & 0xff));
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_setVlongUnit(vlong *pThis, ubyte4 index, vlong_unit unitValue)
{
    MSTATUS status = OK;

    if (index < pThis->numUnitsUsed)
    {
        pThis->pUnits[index] = unitValue;

        /* remove leading zeros */
        if ( ZERO_UNIT == unitValue)
            while ((pThis->numUnitsUsed) && (ZERO_UNIT == pThis->pUnits[pThis->numUnitsUsed-1]))
                pThis->numUnitsUsed--;
    }
    else if (unitValue)
    {
        ubyte4 j;

        if (OK > (status = VLONG_reallocVlong(pThis, index+1)))
            goto exit;

        for (j=pThis->numUnitsUsed; j < index; j++)
            pThis->pUnits[j] = 0;

        pThis->pUnits[index] = unitValue;
        pThis->numUnitsUsed = index+1;
    }

exit:
    return status;

} /* VLONG_setVlongUnit */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_clearVlong(vlong *pThis)
{
    MSTATUS status = OK;

    if (NULL == pThis)
        status = ERR_NULL_POINTER;
    else
    {
        pThis->numUnitsUsed = 0;
        pThis->negative     = FALSE;
    }

    return status;
}


/*------------------------------------------------------------------*/

#define MUL_VLONG_UNIT(a0,a1,b0,b1) \
{                               \
    vlong_unit p0,p1,t0;        \
                                \
    p0   = (b0) * (a0);         \
    p1   = (b1) * (a0);         \
    t0   = (b0) * (a1);         \
    (a1) = (b1) * (a1);         \
    p1 += t0;                   \
    if (p1 < t0)                \
        a1+=MAKE_HI_HUNIT((vlong_unit)1); \
    a1 += HI_HUNIT(p1);         \
    t0  = MAKE_HI_HUNIT(p1);    \
    (a0)=(p0+t0);               \
    if ((a0) < t0)              \
        a1++;                   \
}


/*------------------------------------------------------------------*/

#ifndef MULT_ADDC
#ifndef __MOCANA_ENABLE_LONG_LONG__
#define MULT_ADDC(a,b,index0,index1,result0,result1,result2) \
{   vlong_unit a0, a1, b0, b1;                               \
    a0=LO_HUNIT(a[index0]); a1=HI_HUNIT(a[index0]);          \
    b0=LO_HUNIT(b[index1]); b1=HI_HUNIT(b[index1]);          \
    MUL_VLONG_UNIT(a0,a1,b0,b1);                             \
    result0 += a0; if (result0 < a0) a1++;                   \
    result1 += a1; if (result1 < a1) result2++;              \
}
#else
#define MULT_ADDC(a,b,index0,index1,result0,result1,result2) \
{ \
    UBYTE8 result; \
    ubyte4 temp_result; \
\
    result = ((UBYTE8)a[index0]) * ((UBYTE8)b[index1]); \
    temp_result = result0; \
    result0 += (ubyte4)(result); \
    if (result0 < temp_result) \
        if (0 == (++result1)) \
            result2++; \
    temp_result = result1; \
    result1 += (ubyte4)(result >> BPU); \
    if (result1 < temp_result) \
        result2++; \
}
#endif
#endif


/*------------------------------------------------------------------*/

#ifndef MULT_ADDC1
#ifndef __MOCANA_ENABLE_LONG_LONG__
#define MULT_ADDC1(a,b,index0,index1,result0,result1) \
    { vlong_unit a0,a1,b0,b1;                         \
    a0=LO_HUNIT(a[index0]); a1=HI_HUNIT(a[index0]);   \
    b0=LO_HUNIT(b[index1]); b1=HI_HUNIT(b[index1]);   \
    MUL_VLONG_UNIT(a0,a1,b0,b1);                      \
    result0 += a0; if (result0 < a0) a1++;            \
    result1 += a1;}
#else
#define MULT_ADDC1(a,b,index0,index1,result0,result1) \
{ \
    UBYTE8 result; \
    ubyte4 temp_result; \
\
    result = ((UBYTE8)a[index0]) * ((UBYTE8)b[index1]); \
    temp_result = result0; \
    result0 += (ubyte4)(result); \
    if (result0 < temp_result) \
        ++result1; \
    result1 += (ubyte4)(result >> 32); \
}
#endif
#endif

/*------------------------------------------------------------------*/

#ifndef ADD_DOUBLE
#define ADD_DOUBLE( result0, result1, result2, half0, half1, half2) \
{ vlong_unit carry;                                                     \
    half2 <<= 1;  half2  += (half1 & HALF_MASK) ? 1 : 0;         \
    half1 <<= 1;  half1  += (half0 & HALF_MASK) ? 1 : 0;         \
    half0 <<= 1;                                                    \
    result0 += half0;     carry  = (result0 < half0) ? 1 : 0;       \
    result1 += carry;     carry  = (result1 < carry) ? 1 : 0;       \
    result1 += half1;     carry += (result1 < half1) ? 1 : 0;       \
    result2 += (carry + half2); }
#endif

/*------------------------------------------------------------------*/

#ifndef MULT_ADDCX
#define MULT_ADDCX  MULT_ADDC
#endif

/*------------------------------------------------------------------*/

static MSTATUS
fastUnsignedMultiplyVlongs(vlong *pProduct,
                           const vlong *pFactorX, const vlong *pFactorY, ubyte4 x_limit)
{
#ifndef MACRO_MULTIPLICATION_LOOP
    vlong_unit  result0, result1, result2;
    ubyte4  i, j, x;
    ubyte4  j_upper;
#endif
    ubyte4  i_limit, j_limit;
    vlong_unit* pFactorA;
    vlong_unit* pFactorB;
    vlong_unit* pResult;
    MSTATUS status = OK;

    if ((NULL == pProduct) || (NULL == pFactorX) || (NULL == pFactorY))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

#if ((defined(__ASM_CAVIUM__)) && (defined(MACRO_MULTIPLICATION_LOOP)))
    if (pFactorX->numUnitsUsed < pFactorY->numUnitsUsed)
    {
        /* swap values */
        vlong *pTemp = pFactorX;

        pFactorX = pFactorY;
        pFactorY = pTemp;
    }
#endif

    pFactorA = pFactorX->pUnits;
    pFactorB = pFactorY->pUnits;

    if ((0 == pFactorX->numUnitsUsed) || (0 == pFactorY->numUnitsUsed))
    {
        status = VLONG_clearVlong(pProduct);
        goto exit;
    }

    if (pProduct->numUnitsAllocated < (ubyte4)x_limit)
        if (OK > (status = expandVlong(pProduct, x_limit)))
            goto exit;

#if ((defined(__ASM_CAVIUM__)) && (defined(MACRO_MULTIPLICATION_LOOP)))
    i_limit = pFactorX->numUnitsUsed;
    j_limit = pFactorY->numUnitsUsed;
#else
    i_limit = pFactorX->numUnitsUsed - 1;
    j_limit = pFactorY->numUnitsUsed - 1;
#endif
    pResult = pProduct->pUnits;

#ifndef MACRO_MULTIPLICATION_LOOP
    result0 = result1 = result2 = 0;

    for (x = 0; x < x_limit; x++)
    {
        i = (x <= i_limit) ? x : i_limit;
        j = x - i;

        j_upper = ((x <= j_limit) ? x : j_limit);

        while (j <= j_upper)
        {
            /* result2:result1:result0 += pFactorX->pUnits[i] * pFactorY->pUnits[j]; */
            MULT_ADDCX(pFactorA,pFactorB,i,j,result0,result1,result2);
            i--; j++;
        }

        *pResult++ = result0;

        result0  = result1;
        result1  = result2;
        result2  = 0;
    }
#else
    MACRO_MULTIPLICATION_LOOP(pResult,pFactorA,pFactorB,i_limit,j_limit,x_limit);
#endif

    /* calculate numUnitsUsed */
    while (x_limit && (ZERO_UNIT == pProduct->pUnits[x_limit-1]))
        x_limit--;

    pProduct->numUnitsUsed = x_limit;

exit:
    return status;

} /* fastUnsignedMultiplyVlongs */


/*------------------------------------------------------------------*/

static MSTATUS
fastUnsignedSqrVlong(vlong *pProduct, const vlong *pFactorSqrX,
                     ubyte4 x_limit)
{
#ifndef MACRO_SQR_LOOP
    vlong_unit  result0, result1, result2;
    vlong_unit  half0, half1, half2;
    ubyte4  i, j, x;
#endif
    ubyte4  i_limit;
    vlong_unit* pFactorA;
    vlong_unit* pResult;
    MSTATUS status = OK;

    if ((NULL == pProduct) || (NULL == pFactorSqrX))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pFactorA = pFactorSqrX->pUnits;

    if (0 == pFactorSqrX->numUnitsUsed)
    {
        status = VLONG_clearVlong(pProduct);
        goto exit;
    }

    if (pProduct->numUnitsAllocated < (ubyte4)x_limit)
        if (OK > (status = expandVlong(pProduct, x_limit)))
            goto exit;

    i_limit = (pFactorSqrX->numUnitsUsed - 1);
    pResult = pProduct->pUnits;

#ifndef MACRO_SQR_LOOP
    result0 = result1 = result2 = 0;

    for (x = 0; x < x_limit; x++)
    {
        half0 = half1 = half2 = 0;

        i = (x <= i_limit) ? x : i_limit;
        j = x - i;

        while (j < i)
        {
            /* result2:result1:result0 += pFactorSqrX->pUnits[i] * pFactorSqrX->pUnits[j]; */
            MULT_ADDCX(pFactorA,pFactorA,i,j,half0,half1,half2);
            i--; j++;
        }

        ADD_DOUBLE( result0, result1, result2, half0, half1, half2);

        /* add odd-even case */
        if (i == j)
        {
            MULT_ADDCX(pFactorA,pFactorA,i,j,result0,result1,result2);
        }

        *pResult++ = result0;

        result0  = result1;
        result1  = result2;
        result2  = 0;
    }
#else
    MACRO_SQR_LOOP(pResult,pFactorA,i_limit,x_limit);
#endif

    /* calculate numUnitsUsed */
    while (x_limit && (ZERO_UNIT == pProduct->pUnits[x_limit-1]))
        x_limit--;

    pProduct->numUnitsUsed = x_limit;

exit:
    return status;

} /* fastUnsignedSqrVlong */


/*------------------------------------------------------------------*/

extern intBoolean
VLONG_isVlongZero(const vlong *pThis)
{
    return (0 == pThis->numUnitsUsed) ? TRUE : FALSE;
}


/*------------------------------------------------------------------*/

extern intBoolean
VLONG_isVlongBitSet(const vlong *pThis, ubyte4 testBit)
{
    return (ZERO_UNIT !=
        (VLONG_getVlongUnit(pThis, testBit/BPU) &
            (((vlong_unit)1)<<(testBit%BPU)))) ? TRUE : FALSE;
}

/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_setVlongBit(vlong *pThis, ubyte4 setBit)
{
    vlong_unit unit = VLONG_getVlongUnit(pThis, setBit / BPU);

    unit |= (((vlong_unit)1) << (setBit % BPU));

    return VLONG_setVlongUnit(pThis, setBit / BPU, unit);
}


/*------------------------------------------------------------------*/

extern ubyte4
VLONG_bitLength(const vlong *pThis)
{
    ubyte4 numBits;

    if (0 != (numBits = pThis->numUnitsUsed))
    {
        numBits--;
        numBits *= BPU;
        numBits += BITLENGTH( pThis->pUnits[pThis->numUnitsUsed-1]);
    }

    return numBits;
} /* VLONG_bitLength */


/*------------------------------------------------------------------*/

static sbyte4
compareUnsignedVlongs(const vlong *pValueX, const vlong *pValueY)
{
    sbyte4 i;
    sbyte4 result = 0;

    if (pValueX->numUnitsUsed > pValueY->numUnitsUsed)
    {
        result = 1;
        goto exit;
    }

    if (pValueX->numUnitsUsed < pValueY->numUnitsUsed)
    {
        result = -1;
        goto exit;
    }

    i = pValueX->numUnitsUsed;

    while (i)
    {
        i--;

        if (pValueX->pUnits[i] > pValueY->pUnits[i])
        {
            result = 1;
            goto exit;
        }

        if (pValueX->pUnits[i] < pValueY->pUnits[i])
        {
            result = -1;
            goto exit;
        }
    }

exit:
    return result;

} /* compareUnsignedVlongs */


/*------------------------------------------------------------------*/

extern sbyte4
VLONG_compareUnsigned(const vlong* pTest, vlong_unit immValue)
{
    sbyte4 result;

    if (pTest->negative)
        result = -1;                    /* unsigned value is greater */
    else if (1 < pTest->numUnitsUsed)
        result =  1;                    /* vlong is greater */
    else if (0 == pTest->numUnitsUsed)
    {
        if (ZERO_UNIT == immValue)
            result = 0;                 /* both values are zero */
        else
            result = -1;                /* unsigned value is greater */
    }
    else
    {
        if (pTest->pUnits[0] == immValue)
            result = 0;                 /* both values are equal */
        else if (pTest->pUnits[0] > immValue)
            result = 1;                 /* vlong is greater */
        else
            result = -1;                /* unsigned value is greater */
    }

    return result;
}


/*------------------------------------------------------------------*/

extern sbyte4
VLONG_compareSignedVlongs(const vlong *pValueX, const vlong* pValueY)
{
    /* X == Y = 0; X > Y == +1; X < Y == -1*/
    sbyte4 neg = (pValueX->negative && !VLONG_isVlongZero(pValueX)) ? TRUE : FALSE;
    sbyte4 retValue;

    if (neg == ((pValueY->negative && !VLONG_isVlongZero(pValueY)) ? TRUE : FALSE))
    {
        retValue = compareUnsignedVlongs(pValueX, pValueY);
        if (neg)
        {
            retValue = -retValue;
        }
    }
    else if (neg)
    {
        retValue = -1;
    }
    else
    {
        retValue = 1;
    }
    return retValue;
}


/*------------------------------------------------------------------*/

#ifndef ASM_SHIFT_LEFT_DEFINED
extern MSTATUS
shlVlong(vlong *pThis)
{
    vlong_unit  carry = 0;
    ubyte4  N = pThis->numUnitsUsed; /* necessary, since numUnitsUsed can change */
    ubyte4  i = 0;
    MSTATUS status = OK;

#ifndef MACRO_SHIFT_LEFT
    while (i < N)
    {
        vlong_unit u;

        u = pThis->pUnits[i];
        pThis->pUnits[i] = (u<<1) | carry;

        carry = u >> (BPU-1);
        i++;
    }

#else
    carry = MACRO_SHIFT_LEFT(pThis->pUnits, N);

    i = N;
#endif

    if (ZERO_UNIT == carry)
    {
        while ((pThis->numUnitsUsed) && (ZERO_UNIT == pThis->pUnits[pThis->numUnitsUsed-1]))
            pThis->numUnitsUsed--;
    }
    else
    {
        status = VLONG_setVlongUnit(pThis, i, carry);
    }

    return status;
}
#endif

extern MSTATUS
VLONG_shlVlong(vlong *pThis)
{
    MSTATUS status = OK;

    if (NULL == pThis)
        status = ERR_NULL_POINTER;
    else
        status = shlVlong(pThis);

    return status;
}


/*------------------------------------------------------------------*/

#ifndef ASM_SHIFT_RIGHT_DEFINED
static void
shrVlong(vlong *pThis)
{
#ifndef MACRO_SHIFT_RIGHT
    vlong_unit carry = 0;
    sbyte4  i = pThis->numUnitsUsed;
    vlong_unit u;

    while (i)
    {
        i--;

        u = pThis->pUnits[i];
        pThis->pUnits[i] = ((u >> 1) | carry);

        carry = u << (BPU-1);
    }
#else
    MACRO_SHIFT_RIGHT(pThis->pUnits, pThis->numUnitsUsed);
#endif

    /* remove leading zeros */
    while ((pThis->numUnitsUsed) && (ZERO_UNIT == pThis->pUnits[pThis->numUnitsUsed-1]))
        pThis->numUnitsUsed--;
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_shrVlong(vlong *pThis)
{
    MSTATUS status = OK;

    if (NULL == pThis)
        status = ERR_NULL_POINTER;
    else
        shrVlong(pThis);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_shrXvlong(vlong *pThis, ubyte4 numBits)
{
    sbyte4  delta;
    sbyte4  i, i_limit;
    vlong_unit u;
    MSTATUS status = OK;

    if (0 == numBits)
        goto exit;

    delta = numBits/BPU;

    if ( delta >= (sbyte4)pThis->numUnitsUsed)
    {
        pThis->numUnitsUsed = 0;
        goto exit;
    }

    numBits %= BPU;

    i_limit = ((sbyte4) pThis->numUnitsUsed) - delta - 1;

    for (i = 0; i < i_limit; i++)
    {
        u = pThis->pUnits[i + delta];

        if (numBits)
        {
            u >>= numBits;
            u |= ((vlong_unit) pThis->pUnits[i + delta + 1]) << (BPU - numBits);
        }

        pThis->pUnits[i] = u;
    }

    /* handle highest unit */
    u = pThis->pUnits[i + delta];

    if (numBits)
        u >>= numBits;

    pThis->pUnits[i] = u;

    pThis->numUnitsUsed -= delta;

    /* adjust top */
    while ((pThis->numUnitsUsed) && (ZERO_UNIT == pThis->pUnits[pThis->numUnitsUsed - 1]))
        pThis->numUnitsUsed--;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_shlXvlong(vlong *pThis, ubyte4 numBits)
{
    ubyte4  delta;
    sbyte4  i;
    vlong_unit u, u1;
    MSTATUS status = OK;

    /* nothing to do */
    if ((0 == numBits) || (0 == pThis->numUnitsUsed))
        goto exit;

    delta    = numBits / BPU;
    numBits %= BPU;

    if ((pThis->numUnitsUsed + delta + 1) > pThis->numUnitsAllocated)
        if (OK > (status = VLONG_reallocVlong(pThis, pThis->numUnitsUsed + delta + 1)))
            goto exit;

    /* zero out new space */
    for (i = pThis->numUnitsUsed; i < (sbyte4) (pThis->numUnitsUsed + delta + 1); i++)
        pThis->pUnits[i] = 0x00;

    for (i = pThis->numUnitsUsed - 1; i >= 0; i--)
    {
        /* handle upper portion */
        u   = pThis->pUnits[i];
        u1  = pThis->pUnits[i + delta + ((numBits) ? 1 : 0)];

        u1 |= ((0 != numBits) ? u >> (BPU - numBits) : (u));
        pThis->pUnits[i + delta + ((numBits) ? 1 : 0)] = u1;

        /* clear it */
        pThis->pUnits[i] = 0;

        /* handle lower portion*/
        if (numBits)
        {
            u <<= numBits;
            pThis->pUnits[i + delta] = u;
        }
    }

    /* fix top */
    pThis->numUnitsUsed += delta + ((numBits) ? 1 : 0);

    while ((pThis->numUnitsUsed) && (ZERO_UNIT == pThis->pUnits[pThis->numUnitsUsed - 1]))
        pThis->numUnitsUsed--;

exit:
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
addUnsignedVlongs(vlong *pSumAndValue, const vlong *pValue)
{
    vlong_unit u, carry = 0;
    ubyte4 i;
    MSTATUS status = OK;

#ifdef ASM_ADD
    if (pSumAndValue->numUnitsUsed >= pValue->numUnitsUsed)
    {
        carry = ASM_ADD( pSumAndValue->pUnits,
                         pValue->pUnits,
                         pValue->numUnitsUsed);
        /* handle carry */
        i = pValue->numUnitsUsed;

        while (carry)
        {
            u = VLONG_getVlongUnit(pSumAndValue, i);
            u += carry; carry = (u < carry) ? 1 : 0;

            if (OK > (status = VLONG_setVlongUnit(pSumAndValue, i, u)))
                break;
            i++;
        }
    }
    else
    {
        ubyte4 max = pValue->numUnitsUsed;

        if (OK <= (status = VLONG_reallocVlong(pSumAndValue, max + 1)))
        {
            for ( i = pSumAndValue->numUnitsUsed;
                  i < pSumAndValue->numUnitsAllocated;
                  ++i)
            {
                pSumAndValue->pUnits[i] = 0;
            }

            pSumAndValue->numUnitsUsed = max;

            carry = ASM_ADD( pSumAndValue->pUnits,
                             pValue->pUnits,
                             pValue->numUnitsUsed);
            if ( carry)
            {
                if (OK > (status = VLONG_setVlongUnit( pSumAndValue,
                                                       max, carry)))
                {
                    return status;
                }
            }
        }
    }

#else
    if (pSumAndValue->numUnitsUsed >= pValue->numUnitsUsed)
    {
        vlong_unit ux;

        for (i = 0; i < pValue->numUnitsUsed; i++)
        {
            u = pSumAndValue->pUnits[i];
            u = u + carry; carry = (u < carry) ? 1 : 0;

            ux = pValue->pUnits[i];
            u = u + ux; carry += ((u < ux) ? 1 : 0);

            pSumAndValue->pUnits[i] = u;
        }

        /* handle carry */
        while (carry)
        {
            u = VLONG_getVlongUnit(pSumAndValue, i);
            u = u + carry; carry = (u < carry) ? 1 : 0;

            if (OK > (status = VLONG_setVlongUnit(pSumAndValue, i, u)))
                break;
            ++i;
        }
    }
    else
    {
        ubyte4 max = pValue->numUnitsUsed;
        vlong_unit ux;

        if (OK <= (status = VLONG_reallocVlong(pSumAndValue, max + 1)))
        {
            for (i = 0; i < max + 1; i++)
            {
                u = VLONG_getVlongUnit(pSumAndValue, i);
                u = u + carry; carry = (u < carry) ? 1 : 0;

                ux = VLONG_getVlongUnit(pValue, i);
                u = u + ux; carry += ((u < ux) ? 1 : 0);

                if (OK > (status = VLONG_setVlongUnit(pSumAndValue, i, u)))
                    break;
            }
        }
    }
#endif

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
subtractUnsignedVlongs(vlong *pResultAndValue, const vlong *pValue)
{
    MSTATUS status = OK;
#ifdef ASM_SUBTRACT
    /* NOTE: pResultAndValue must be greater than pValue */
    ASM_SUBTRACT( pResultAndValue->pUnits, pValue->pUnits,
                  pValue->numUnitsUsed);

#else
    /* NOTE: pResultAndValue must be greater than pValue */
    vlong_unit  carry = 0;
    ubyte4  N = pValue->numUnitsUsed;
    ubyte4  i;
    vlong_unit ux, u, nu;

    for (i = 0; i < N; i++)
    {
        ux = pValue->pUnits[i];
        ux += carry;

        if (ux >= carry)
        {
            u = pResultAndValue->pUnits[i];
            nu = u - ux;
            carry = nu > u;

            pResultAndValue->pUnits[i] = nu;
        }
    }

    while ((carry) && (i < pResultAndValue->numUnitsUsed))
    {
        u = pResultAndValue->pUnits[i];
        nu = u - carry;
        carry = nu > u;

        pResultAndValue->pUnits[i] = nu;

        i++;
    }
#endif

    while ((pResultAndValue->numUnitsUsed) && (ZERO_UNIT == pResultAndValue->pUnits[pResultAndValue->numUnitsUsed - 1]))
        pResultAndValue->numUnitsUsed--;

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
assignUnsignedToVlong(vlong *pThis, vlong_unit x)
{
    MSTATUS status;

    if (OK <= (status = VLONG_clearVlong(pThis)))
        status = VLONG_setVlongUnit(pThis,0,x);

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
copyUnsignedValue(vlong *pDest, const vlong *pSource)
{
    sbyte4  numUnits;
    MSTATUS status;

    if ((NULL == pDest) || (NULL == pSource))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    VLONG_clearVlong(pDest);
    numUnits = pSource->numUnitsUsed;

    if (OK > (status = VLONG_reallocVlong(pDest, numUnits)))
        goto exit;

    pDest->numUnitsUsed = numUnits;

    while (numUnits)
    {
        numUnits--;

        pDest->pUnits[numUnits] = pSource->pUnits[numUnits];
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_copySignedValue(vlong *pDest, const vlong *pSource)
{
    MSTATUS status;

    if (OK <= (status = copyUnsignedValue(pDest, pSource)))
        pDest->negative = pSource->negative;

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_makeVlongFromUnsignedValue(vlong_unit value, vlong **ppRetVlong, vlong **ppVlongQueue)
{
    vlong*  pTemp = NULL;
    MSTATUS status;

    if (NULL == ppRetVlong)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = VLONG_allocVlong(&pTemp, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(*ppRetVlong);

    if (OK > (status = assignUnsignedToVlong(pTemp, value)))
        goto exit;

    *ppRetVlong = pTemp;
    pTemp = NULL;

exit:
    VLONG_freeVlong(&pTemp, ppVlongQueue);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_makeVlongFromVlong(const vlong* pValue, vlong **ppRetVlong, vlong **ppVlongQueue)
{
    MSTATUS status;

    /* VLONG_allocVlong() doesn't allow ppRetVlong to be NULL */
    if (NULL == ppRetVlong)
        return ERR_NULL_POINTER;

    if (OK <= (status = VLONG_allocVlong(ppRetVlong, ppVlongQueue)))
    {
        DEBUG_RELABEL_MEMORY(*ppRetVlong);

        if (OK > (status = VLONG_copySignedValue(*ppRetVlong, pValue)))
            VLONG_freeVlong(ppRetVlong, ppVlongQueue);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_unsignedMultiply(vlong *pProduct, const vlong *pFactorX, const vlong *pFactorY)
{
    if (pFactorX == pFactorY)
    {
        return VLONG_FAST_SQR( pProduct, pFactorX, pFactorX->numUnitsUsed * 2);
    }
    return VLONG_FAST_MULT(pProduct, pFactorX, pFactorY, pFactorX->numUnitsUsed + pFactorY->numUnitsUsed);
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_vlongSignedMultiply(vlong *pProduct, const vlong *pFactorX, const vlong *pFactorY)
{
    MSTATUS status;

    if (pFactorX == pFactorY)
    {
        status = VLONG_FAST_SQR( pProduct, pFactorX, pFactorX->numUnitsUsed * 2);
        pProduct->negative = FALSE;
    }
    else
    {
        status = VLONG_FAST_MULT(pProduct, pFactorX, pFactorY, pFactorX->numUnitsUsed + pFactorY->numUnitsUsed);

        pProduct->negative = pFactorX->negative ^ pFactorY->negative;
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_vlongSignedSquare(vlong *pProduct, const vlong *pFactor)
{
    MSTATUS status;

    status = VLONG_FAST_SQR(pProduct, pFactor, 2*pFactor->numUnitsUsed);
    pProduct->negative = FALSE;

    return status;
}


/* 64 divided by 32 return only the least significant 32 bits of the quotient
which is all that's needed for VLONG_unsignedDivide because of normalization
This algorithm (HAC 14.20) is twice as fast as the algorithm based on shifts.
The implementation of this is as fast as the OpenSSL implementation and
contrarily to the latter, returns the correct result all of the time, not
only for normalized d (i.e. d >= 0x7FFFFFFF) */
#ifdef __ENABLE_MOCANA_64_BIT__
typedef ubyte4 hvlong_unit;
#else
typedef ubyte2 hvlong_unit;
#endif

static vlong_unit
VLONG_DoubleDiv( vlong_unit hi, vlong_unit lo, vlong_unit d)
{
    vlong_unit normShift;
    vlong_unit hihi = 0;    /* normalization overflow */
    ubyte4 bitLen;
    sbyte4 n, i;
    vlong_unit temp;
    hvlong_unit q[5] = {0};  /* quotient */
    hvlong_unit xx[8];       /* xx[0] = xx[1] = 0 used for negative indices */
    hvlong_unit* x = xx + 2;
    vlong_unit lod, hid;

    if (!hi)
    {
        return lo/d;
    }

    xx[0] = xx[1] = 0;

    bitLen = BITLENGTH(d);

    normShift = BPU - bitLen;
    if (normShift)
    {
        hihi = (hi >> (BPU-normShift));
        hi <<= normShift;
        hi |= (lo >> (BPU-normShift));
        lo <<= normShift;
        d <<= normShift;
    }

    if ( HI_HUNIT(hihi))
    {
        n = 5;
        while ( hihi >= d)
        {
            hihi -= d;
            q[4]++;
        }
    }
    else if (hihi)
    {
        n = 4;
        temp = MAKE_HI_HUNIT(hihi) + HI_HUNIT( hi);
        while ( temp >= d)
        {
            temp -= d;
            q[3]++;
        }
        hihi = HI_HUNIT(temp);
        hi = MAKE_UNIT(temp, LO_HUNIT(hi));
    }
    else
    {
        n = 3;
        while ( hi >= d)
        {
            hi -= d;
            q[2]++;
        }
    }

    x[5] = (hvlong_unit) HI_HUNIT( hihi);
    x[4] = (hvlong_unit) LO_HUNIT( hihi);
    x[3] = (hvlong_unit) HI_HUNIT( hi);
    x[2] = (hvlong_unit) LO_HUNIT( hi);
    x[1] = (hvlong_unit) HI_HUNIT( lo);
    x[0] = (hvlong_unit) LO_HUNIT( lo);

    lod = LO_HUNIT(d);
    hid = HI_HUNIT(d);

    for (i = n; i >= 2; --i)
    {
        vlong_unit t0,t1;
        vlong_unit borrow;

        if ( x[i] == hid)
        {
            q[i-2] = LO_MASK;
        }
        else
        {
            q[i-2] = (hvlong_unit) (MAKE_UNIT(x[i], x[i-1]) / hid);
        }

        for (;;)
        {
            /* multiply q[i-2] * d and compare with 3 digits of x */
            t0 = q[i-2] * lod;
            t1 = q[i-2] * hid;
            t1 += HI_HUNIT(t0);
            /* 3 digits: t1 has the first 2, t0 & 0xFFFF has the last one */
            temp = MAKE_UNIT(x[i], x[i-1]);
            if ( (t1 > temp) || ( t1 == temp && LO_HUNIT(t0) > x[i-2]) )
            {
                q[i-2]--;
            }
            else
            {
                break;
            }
        }
        /* now subtract d * q[i-2] from x[i-2]...
        we know because of above that d * q[i-2] <= x[i-2].... so
        we don't need to keep track of the borrow or whether this
        can end up being negative */
        borrow = 0;
        if ( x[i-2] < LO_HUNIT(t0) )
        {
            ++borrow;
        }
        x[i-2] -= (hvlong_unit) LO_HUNIT(t0);
#ifdef VERIFY_DIV_ALGO
        if ( x[i-1] > borrow)
        {
            x[i-1] -= borrow;
            borrow = 0;
        }
        if ( x[i-1] < LO_HUNIT(t1) )
        {
            ++borrow;
        }
        x[i-1] -= LO_HUNIT(t1);
        x[i] -= borrow + HI_HUNIT(t1);     /* and verify x[i] = 0 */
#else
        x[i-1] -= (hvlong_unit) (borrow + LO_HUNIT(t1));
#endif
    }
    return MAKE_UNIT(q[1],q[0]);
}

#define ELEM_0(a,i)   ((i >= 0)? a[i] : 0)

/*------------------------------------------------------------------*/

/* implementation of the HAC 14.20 algorithm */
extern MSTATUS
VLONG_unsignedDivide(vlong *pQuotient, const vlong *pDividend, const vlong *pDivisor,
                    vlong *pRemainder, vlong **ppVlongQueue)
{
    vlong*  pY = NULL;
    vlong*  pYBnt = NULL;
    ubyte4  normShift;
    sbyte4  n,t;
    sbyte4  i,j;
    vlong_unit *q, *x, *y;
    MSTATUS status;

    if (VLONG_isVlongZero(pDivisor))
    {
        status = ERR_DIVIDE_BY_ZERO;
        goto exit;
    }

    if (OK > ( status = copyUnsignedValue( pRemainder, pDividend)))
        goto exit;

    if ( compareUnsignedVlongs( pDividend, pDivisor) < 0)
    {
        status = assignUnsignedToVlong( pQuotient, 0);
        goto exit;
    }

    if (OK > (status = VLONG_makeVlongFromVlong(pDivisor, &pY, ppVlongQueue)))
        goto exit;

    /* normalize */
    normShift = BPU - BITLENGTH(pY->pUnits[pY->numUnitsUsed-1]);

    if (OK > ( status = VLONG_shlXvlong( pRemainder, normShift)) ||
        OK > ( status = VLONG_shlXvlong( pY, normShift)))
    {
        goto exit;
    }
    n = pRemainder->numUnitsUsed-1;
    t = pY->numUnitsUsed-1;

    if (OK > (status = expandVlong( pQuotient, n - t + 1)))
        goto exit;
    pQuotient->numUnitsUsed = n-t+1;
    q = pQuotient->pUnits;
    MOC_MEMSET( (ubyte*) q, 0, (n-t+1) * sizeof(vlong_unit));

    /* generate Y << n - t i.e. y(B^ (n-t)) Step 2*/
    if (OK > ( status = VLONG_allocVlong( &pYBnt, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pYBnt);

    j = n;
    for (i = t; i >= 0; --i)
    {
        if (OK > ( status = VLONG_setVlongUnit(pYBnt, j--, pY->pUnits[i])))
            goto exit;
    }

    /* Step 2. done only once if normalized */
    if (compareUnsignedVlongs( pRemainder, pYBnt) >= 0)
    {
        subtractUnsignedVlongs( pRemainder, pYBnt);
        q[n-t]++;
    }
    VLONG_freeVlong(&pYBnt, ppVlongQueue);

    x = pRemainder->pUnits;
    y = pY->pUnits;

    /* Step 3. */
    for (i = n; i > t; --i)
    {
        vlong_unit borrow;
#if __GNUC__ == 3 && defined(__OPTIMIZE__)
        volatile     /* prevent annoying GCC bug */
#endif
        vlong_unit r0,r1,r2,r3;
        sbyte4 index0;

        index0 = i-t-1; /* this is always >= 0 */
        /* 3.1 */
        if ( x[i] == y[t])
        {
            q[i-t-1] = FULL_MASK;
        }
        else
        {
            q[i-t-1] = VLONG_DoubleDiv(ELEM_0(x,i), ELEM_0(x, i-1), y[t]);
        }
        /* 3.2 */
        for (;;)
        {
            r3 = r2= r1= r0 = 0;
            if ( t > 0)
            {
                sbyte4 u = t-1;
                MULT_ADDCX(q, y, index0, u, r0, r1, r2);
            }

            MULT_ADDCX( q, y, index0, t, r1, r2, r3); /* r3 should be 0 */
            if ( r2 > ELEM_0(x, i) ||
                (r2 == ELEM_0(x, i) && r1 > ELEM_0(x, i-1)) ||
                (r2 == ELEM_0(x, i) && r1 == ELEM_0(x, i-1) && r0 > ELEM_0(x, i-2)))
            {
                q[index0]--;
            }
            else
            {
                break;
            }
        }
        /* 3.3 */
        r3 = r2= r1= r0 = 0;
        for ( j = 0; j <= t; ++j)
        {
            /* multiply the j digit of y by q[i-t-1] */
            MULT_ADDCX( q, y, index0, j, r0, r1, r2);
            /* subtract the low digit from x[i+j-t-1] */
            borrow = (x[index0+j] < r0) ? 1 : 0;
            x[index0+j] -= r0;
            /* add the other digits including the borrow */
            r0 = r1; r0 += borrow; borrow = (r0 >= r1) ? 0 : 1;
            r1 = r2; r1 += borrow; borrow = (r1 >= r2) ? 0 : 1;
            r2 = borrow;
        }
        /* Step 3.4 */
        if ( x[i] < r0 )
        {
            vlong_unit carry = 0;

            x[i] -= r0;
            for ( j = 0; j <= t; ++j)
            {
                x[index0+j] += carry;
                carry = (x[index0+j]  < carry) ? 1 : 0;

                x[index0+j] += y[j];
                carry += (x[index0+j]  < y[j]) ? 1 : 0;
            }
            x[i] += carry;
            q[index0]--;
        }
        else
        {
            x[i] -= r0;
        }
    }

    while ((pQuotient->numUnitsUsed) && (ZERO_UNIT == pQuotient->pUnits[pQuotient->numUnitsUsed - 1]))
        pQuotient->numUnitsUsed--;

    pRemainder->numUnitsUsed = t+1;
    while ((pRemainder->numUnitsUsed) && (ZERO_UNIT == pRemainder->pUnits[pRemainder->numUnitsUsed - 1]))
        pRemainder->numUnitsUsed--;

    if (OK > ( status = VLONG_shrXvlong( pRemainder, normShift)))
        goto exit;

exit:
    VLONG_freeVlong(&pY, ppVlongQueue);
    VLONG_freeVlong(&pYBnt, ppVlongQueue);

    return status;

} /* VLONG_unsignedDivide */

/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_addSignedVlongs(vlong *pSumAndValue, const vlong *pValue, vlong **ppVlongQueue)
{
    MSTATUS status;

    if (pSumAndValue->negative == pValue->negative)
    {
        status = addUnsignedVlongs(pSumAndValue, pValue);
    }
    else if (0 <= compareUnsignedVlongs(pSumAndValue, pValue))
    {
        status = subtractUnsignedVlongs(pSumAndValue, pValue);
    }
    else
    {
        vlong *pTmpValue = NULL;

        if (OK <= (status = VLONG_makeVlongFromVlong(pSumAndValue, &pTmpValue, ppVlongQueue)))
            if (OK <= (status = VLONG_copySignedValue(pSumAndValue, pValue)))
                status = subtractUnsignedVlongs(pSumAndValue, pTmpValue);

        VLONG_freeVlong(&pTmpValue, ppVlongQueue);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_subtractSignedVlongs(vlong *pSumAndValue, const vlong *pValue, vlong **ppVlongQueue)
{
    MSTATUS status;

    if (pSumAndValue->negative != pValue->negative)
    {
        status = addUnsignedVlongs(pSumAndValue, pValue);
    }
    else if (0 <= compareUnsignedVlongs(pSumAndValue, pValue))
    {
        status = subtractUnsignedVlongs(pSumAndValue, pValue);
    }
    else
    {
        vlong *pTmpValue = NULL;

        if (OK <= (status = VLONG_makeVlongFromVlong(pSumAndValue, &pTmpValue, ppVlongQueue)))
            if (OK <= (status = VLONG_copySignedValue(pSumAndValue, pValue)))
                status = subtractUnsignedVlongs(pSumAndValue, pTmpValue);

        pSumAndValue->negative = 1 - pSumAndValue->negative;
        VLONG_freeVlong(&pTmpValue, ppVlongQueue);
    }

    return status;
}


/*--------------------------------------------------------------------------*/
#ifdef __ALTIVEC__
static MSTATUS
operatorMinusSignedVlongs(vlong* pValueX, vlong* pValueY, vlong **ppSum, vlong **ppVlongQueue)
{
    MSTATUS status;

    if (OK <= (status = VLONG_makeVlongFromVlong(pValueX, ppSum, ppVlongQueue)))
        status = VLONG_subtractSignedVlongs(*ppSum, pValueY, ppVlongQueue);

    return status;
}
#endif

/*------------------------------------------------------------------*/

/* ff additional function to converts from bytes string */
/* byte string is completely in big endian format */
/* but the internal representation used array of unsigned so */
/* convert each x bytes to the correct unsigned vlong_unit */
extern MSTATUS
VLONG_vlongFromByteString(const ubyte* byteString, sbyte4 len,
                          vlong **ppRetVlong, vlong **ppVlongQueue)
{
    sbyte4  i, j, count;
    vlong_unit elem;
    MSTATUS status;

    if (NULL == ppRetVlong)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 > len)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    if (OK > (status = VLONG_allocVlong(ppRetVlong, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(*ppRetVlong);

    if (OK > (status = VLONG_reallocVlong(*ppRetVlong, 1 + (len / sizeof(vlong_unit)))))
        goto exit;

    /* now copy the contents of the byte string to the array of vlong_units */
    /* respecting the endianess of the architecture */

    count = 0;

    for (i = len - 1; i >= 0; ++count)
    {
        elem = 0;

        for (j = 0; j < (sbyte4)(sizeof(vlong_unit)) && i >= 0; ++j, --i)
        {
            elem |= (((vlong_unit) byteString[i]) << (j * 8));
        }

        if (OK > (status = VLONG_setVlongUnit(*ppRetVlong, count, elem)))
            goto exit;
    }

exit:
    return status;

} /* VLONG_vlongFromByteString */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_fixedByteStringFromVlong(vlong* pValue, ubyte* pDest, sbyte4 fixedLength)
{
    MSTATUS status = OK;

    if ((NULL == pValue) || (NULL == pDest))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    while (0 < fixedLength)
    {
        fixedLength--;
        *pDest = getByte(pValue, fixedLength);
        pDest++;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_byteStringFromVlong(const vlong* pValue, ubyte* pDest, sbyte4* pRetLen)
{
    sbyte4      requiredLen;
    sbyte4      index;
    vlong_unit  elem;
    MSTATUS status = OK;

    if (NULL == pValue)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* return a value precise to the byte */
    requiredLen = (sbyte4)((7 + VLONG_bitLength(pValue)) / 8);

    if (pDest)
    {
        if (*pRetLen >= requiredLen)
        {
            /* convert from array of vlong_unit to arrays of bytes */
            /* this just requires converting the unsigned to big endian format */
            ubyte4 mode = requiredLen & (sizeof(vlong_unit) -1);

            /* pad with 0 if necessary instead of setting *pRetLen to requiredLen
                This is because some higher levels API (rsa.h) do not provide
                a way to return the length of the result. They assume that
                length output = length input which is true in most cases */
            while (*pRetLen > requiredLen)
            {
                *pDest++ = 0;
                requiredLen++;
            }
            if ( pValue->numUnitsUsed > 0)
            {
                elem = VLONG_getVlongUnit(pValue, pValue->numUnitsUsed-1);
                if (0 == mode)
                {
                    mode = sizeof(vlong_unit);
                }
                switch (mode)
                {
/* next #ifdef is to prevent warnings (and reduce code size) -- numerically impossible */
#ifdef __ENABLE_MOCANA_64_BIT__
                case 8:
                    *pDest++ = (ubyte)((elem >> 56) & 0xff);
                case 7:
                    *pDest++ = (ubyte)((elem >> 48) & 0xff);
                case 6:
                    *pDest++ = (ubyte)((elem >> 40) & 0xff);
                case 5:
                    *pDest++ = (ubyte)((elem >> 32) & 0xff);
#endif
                case 4:
                    *pDest++ = (ubyte)((elem >> 24) & 0xff);
                case 3:
                    *pDest++ = (ubyte)((elem >> 16) & 0xff);
                case 2:
                    *pDest++ = (ubyte)((elem >>  8) & 0xff);
                case 1:
                    *pDest++ = (ubyte)elem;
               }
            }

            for (index = pValue->numUnitsUsed-2; 0 <= index; --index)
            {
                sbyte4 i;

                elem = VLONG_getVlongUnit(pValue, index);

                for (i = sizeof(vlong_unit) - 1; i >= 0; --i)
                {
                    *pDest++ = (ubyte) ((elem >> (i*8)) & 0xFF);
                }
            }
        }
        else
        {
            status = ERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        *pRetLen = requiredLen;
    }

exit:
    return status;
} /* VLONG_byteStringFromVlong */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_PEM_CONVERSION__) || !defined(__DISABLE_MOCANA_KEY_GENERATION__)

/* implement mpint encoding as described in RFC 4251, section 5 */
extern MSTATUS
VLONG_mpintByteStringFromVlong(const vlong* pValue, ubyte** ppDest, sbyte4* pRetLen)
{
    ubyte4  length;
    ubyte4  leadingByte = 0; /* 1 if we need a lead zero or 0xFF byte, 0 otherwise */
    ubyte*  pDest     = NULL;
    ubyte4  bitLen;
    MSTATUS status    = OK;

    if ((NULL == pValue) || (NULL == ppDest) || (NULL == pRetLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *ppDest  = NULL;
    *pRetLen = 0;

    if ( pValue->numUnitsUsed)
    {
        bitLen = BITLENGTH(pValue->pUnits[pValue->numUnitsUsed-1]);
        leadingByte = (0 == bitLen % 8) ? 1 : 0;
    }

    /* serialize now */
    if ( OK > (status = VLONG_byteStringFromVlong( pValue, NULL,
                                                   (sbyte4*) &length)))
    {
        goto exit;
    }

    pDest = MALLOC( length + 4 + leadingByte);

    if (!pDest)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* length */
    BIGEND32( pDest, length + leadingByte);
    /* leading byte */
    if (leadingByte)
    {
        pDest[4] = (pValue->negative) ? 0xFF : 0x00;
    }

    if (OK > (status =
              VLONG_byteStringFromVlong( pValue, pDest + 4 + leadingByte,
                                         (sbyte4*) &length)))
    {
        goto exit;
    }

    *pRetLen = length + 4 + leadingByte;
    /* convert to 2 complement if negative */
    if (pValue->negative)
    {
        ubyte4 i;
        /* flip all the bytes */
        for (i = 4 + leadingByte; i < (ubyte4) *pRetLen; ++i)
        {
            pDest[i] ^= 0xFF;
        }
        /* add 1 */
        for ( i = *pRetLen-1; i >= 4 + leadingByte; --i)
        {
            if (0xFF == pDest[i] )
            {
                pDest[i] = 0;
            }
            else
            {
                pDest[i]++;
                break;
            }
        }
    }

    *ppDest = pDest;
    pDest = 0;

exit:
    if (pDest)
    {
        FREE(pDest);
    }

    return status;

} /* VLONG_mpintByteStringFromVlong */

#endif /* __DISABLE_MOCANA_KEY_GENERATION__ */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_newFromMpintBytes(const ubyte *pArray, ubyte4 bytesAvailable,
                        vlong **ppNewVlong,  ubyte4 *pRetNumBytesUsed)
{
    intBoolean  isNegativeValue = FALSE;
    sbyte4      length, i;
    vlong*      pTmp = NULL;
    MSTATUS     status;

    if ((NULL == pArray) || (NULL == ppNewVlong) || (NULL == pRetNumBytesUsed))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *ppNewVlong = NULL;

    /* nothing to copy */
    if (4 > bytesAvailable)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    /* calc mpint length */
    length  = ((sbyte4)(pArray[3]));
    length |= ((sbyte4)(pArray[2])) <<  8;
    length |= ((sbyte4)(pArray[1])) << 16;
    length |= ((sbyte4)(pArray[0])) << 24;

    *pRetNumBytesUsed = length + 4;

    if (((((sbyte4)bytesAvailable) - 4) < length) || (0 > length))
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    pArray += 4;

    /* over the length */
    /* first byte indicates if the array is negative */
    if (length)
    {
        isNegativeValue = ((*pArray) & 0x80) ? TRUE : FALSE;
    }

    if ( OK > (status = VLONG_vlongFromByteString( pArray, length, &pTmp, NULL)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pTmp);

    if (isNegativeValue)
    {
        ubyte4 bitLen;

        /* decrement */
        if ( OK > (status = VLONG_decrement( pTmp, NULL)))
            goto exit;

        /* flip the bits */
        for (i = 0; i < ((sbyte4)pTmp->numUnitsUsed) - 1; ++i)
        {
            pTmp->pUnits[i] ^= FULL_MASK;
        }
        /* last unit is special */
        bitLen = BITLENGTH( pTmp->pUnits[pTmp->numUnitsUsed-1]);
        pTmp->pUnits[pTmp->numUnitsUsed-1] ^= (FULL_MASK >> (BPU - bitLen));

        pTmp->negative = TRUE;

        /* update pTmp->numUnitsUsed in case .... */
        while ((pTmp->numUnitsUsed) && (ZERO_UNIT == pTmp->pUnits[pTmp->numUnitsUsed - 1]))
            pTmp->numUnitsUsed--;

    }


    *ppNewVlong = pTmp;
    pTmp = 0;

exit:

    VLONG_freeVlong(&pTmp, NULL);
    return status;

} /* VLONG_newFromMpintBytes */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_vlongFromUByte4String(const ubyte4 *pU4Str, ubyte4 len, vlong **ppNewVlong)
{
    MSTATUS status;
    ubyte4  index;

    if ((NULL == pU4Str) || (NULL == ppNewVlong))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *ppNewVlong = NULL;

    if (OK > (status = VLONG_allocVlong(ppNewVlong, NULL)))
        goto exit;

    if(NULL != ppNewVlong)
        DEBUG_RELABEL_MEMORY(*ppNewVlong);

    if (0 == len)
        goto exit;

#ifdef __ENABLE_MOCANA_64_BIT__

    if (OK > (status = VLONG_reallocVlong(*ppNewVlong, ((len+1) >> 1) )))
        goto cleanup;

    index = 0;
    while ((1 < len) && (OK <= status))
    {
        vlong_unit u = pU4Str[len-2];
        u <<= 32;
        u |= pU4Str[len-1];

        status = VLONG_setVlongUnit(*ppNewVlong, index, u);
        index++;
        len -= 2;
    }
    if (OK <= status && 1 == len)
    {
        status = VLONG_setVlongUnit(*ppNewVlong, index, pU4Str[0]);
    }

#else
    if (OK > (status = VLONG_reallocVlong(*ppNewVlong, len)))
        goto cleanup;

    index = 0;
    while ((0 < len) && (OK <= status))
    {
        status = VLONG_setVlongUnit(*ppNewVlong, index, pU4Str[--len]);
        index++;
    }
#endif

cleanup:
    if (OK > status)
        VLONG_freeVlong(ppNewVlong, NULL);

exit:
    return status;

} /* VLONG_vlongFromUByte4String */


/*------------------------------------------------------------------*/

static MSTATUS
operatorMultiplySignedVlongs(const vlong* pFactorX, const vlong* pFactorY,
                             vlong **ppProduct, vlong **ppVlongQueue)
{
    MSTATUS status;

    if (NULL == ppProduct)
        return ERR_NULL_POINTER;

    if (OK <= (status = VLONG_allocVlong(ppProduct, ppVlongQueue)))
    {
        DEBUG_RELABEL_MEMORY(*ppProduct);

        status = VLONG_unsignedMultiply(*ppProduct, pFactorX, pFactorY);

        (*ppProduct)->negative = pFactorX->negative ^ pFactorY->negative;
    }

    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_operatorDivideSignedVlongs(const vlong* pDividend, const vlong* pDivisor,
                                 vlong **ppQuotient, vlong **ppVlongQueue)
{
    vlong*  pRemainder = NULL;
    MSTATUS status;

    if (NULL == ppQuotient)
        return ERR_NULL_POINTER;

    *ppQuotient = NULL;

    if (OK <= (status = VLONG_allocVlong(ppQuotient, ppVlongQueue)))
    {
        DEBUG_RELABEL_MEMORY(*ppQuotient);

        if (OK <= (status = VLONG_allocVlong(&pRemainder, ppVlongQueue)))
        {
            DEBUG_RELABEL_MEMORY(pRemainder);

            status = VLONG_unsignedDivide(*ppQuotient, pDividend, pDivisor, pRemainder, ppVlongQueue);
            (*ppQuotient)->negative = pDividend->negative ^ pDivisor->negative;
        }
    }

    if (OK > status)
        VLONG_freeVlong(ppQuotient, ppVlongQueue);

    VLONG_freeVlong(&pRemainder, ppVlongQueue);

    return status;
}


/*------------------------------------------------------------------*/

#ifndef __VLONG_MOD_OPERATOR_HARDWARE_ACCELERATOR__

extern MSTATUS
VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong* pDividend,
                              const vlong* pDivisor,
                              vlong **ppRemainder, vlong **ppVlongQueue)
{
    vlong*  pQuotient = NULL;
    MSTATUS status;

    if (NULL == ppRemainder)
        return ERR_NULL_POINTER;

    *ppRemainder = NULL;

    if (OK <= (status = VLONG_allocVlong(ppRemainder, ppVlongQueue)))   /* don't return something from queue */
    {
        DEBUG_RELABEL_MEMORY(*ppRemainder);

        if (OK <= (status = VLONG_allocVlong(&pQuotient, ppVlongQueue)))
        {
            DEBUG_RELABEL_MEMORY(pQuotient);

            status = VLONG_unsignedDivide(pQuotient, pDividend, pDivisor, *ppRemainder, ppVlongQueue);
            (*ppRemainder)->negative = pDividend->negative; /* this is correct */
        }
    }

    if (OK > status)
        VLONG_freeVlong(ppRemainder, ppVlongQueue); /* if alloc failed this will return an error */

    VLONG_freeVlong(&pQuotient, ppVlongQueue);

    return status;
}

#endif /* __VLONG_MOD_OPERATOR_HARDWARE_ACCELERATOR__ */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_greatestCommonDenominator(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *pValueX,
                                const vlong *pValueY, vlong **ppGcd, vlong **ppVlongQueue)
{
    vlong*  pTempX     = NULL;
    vlong*  pTempY     = NULL;
    vlong*  pRemainder = NULL;
    MSTATUS status;

    if (OK <= (status = VLONG_makeVlongFromVlong(pValueX, &pTempX, ppVlongQueue)))
        if (OK <= (status = VLONG_makeVlongFromVlong(pValueY, &pTempY, ppVlongQueue)))
        {
            while (1)
            {
                if (TRUE == VLONG_isVlongZero(pTempY))
                {
                    *ppGcd = pTempX;
                    pTempX = NULL;
                    break;
                }

                /* x = x % y; */
                VLONG_freeVlong(&pRemainder, ppVlongQueue);
                if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) pTempX, pTempY, &pRemainder, ppVlongQueue)))
                    break;

                VLONG_freeVlong(&pTempX, ppVlongQueue);
                pTempX     = pRemainder;
                pRemainder = NULL;


                if (TRUE == VLONG_isVlongZero(pTempX))
                {
                    *ppGcd = pTempY;
                    pTempY = NULL;
                    break;
                }

                /* y = y % x; */
                VLONG_freeVlong(&pRemainder, ppVlongQueue);
                if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) pTempY, pTempX, &pRemainder, ppVlongQueue)))
                    break;

                VLONG_freeVlong(&pTempY, ppVlongQueue);
                pTempY     = pRemainder;
                pRemainder = NULL;
            }
        }

    VLONG_freeVlong(&pTempX, ppVlongQueue);
    VLONG_freeVlong(&pTempY, ppVlongQueue);
    VLONG_freeVlong(&pRemainder, ppVlongQueue);

    return status;

} /* VLONG_greatestCommonDenominator */


/*------------------------------------------------------------------*/

#ifndef __VLONG_MODINV_OPERATOR_HARDWARE_ACCELERATOR__

extern MSTATUS
VLONG_modularInverse(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *pA,
                     const vlong *pModulus, vlong **ppRetModularInverse,
                     vlong **ppVlongQueue)
{
    vlong*  j   = NULL;
    vlong*  i   = NULL;
    vlong*  b   = NULL;
    vlong*  c   = NULL;
    vlong*  x   = NULL;
    vlong*  y   = NULL;
    vlong*  z   = NULL;
    MSTATUS status;

    if ((NULL == pA) || (NULL == pModulus) || (NULL == ppRetModularInverse))
    {
        return ERR_NULL_POINTER;
    }

    *ppRetModularInverse = NULL;

    if ((OK > (status = VLONG_makeVlongFromUnsignedValue(1, &j, ppVlongQueue)))   ||
        (OK > (status = VLONG_makeVlongFromUnsignedValue(0, &i, ppVlongQueue)))   ||
        (OK > (status = VLONG_makeVlongFromVlong(pModulus, &b, ppVlongQueue)))    ||
        (OK > (status = VLONG_makeVlongFromVlong(pA, &c, ppVlongQueue))) ||
        (OK > (status = VLONG_allocVlong(&y, ppVlongQueue)))             ||
        (OK > (status = VLONG_allocVlong(&x, ppVlongQueue))))
    {
        goto cleanup;
    }

    while (! VLONG_isVlongZero(c))
    {
        /* x = b / c  -- y = b % c */
        if (OK > (status = VLONG_unsignedDivide(x, b, c, y, ppVlongQueue)))
            goto cleanup;

        /* z = old b storage */
        z = b;

        b = c;
        c = y;
        y = j;

        /*** j = i - j*x  or (since y = j)   j = i - y*x ****/
        /* z = y * x */
        if (OK > (status = VLONG_vlongSignedMultiply(z, y, x)))
            goto cleanup;
        /* i -= z */
        if (OK > (status = VLONG_subtractSignedVlongs(i, z, ppVlongQueue)))
            goto cleanup;
        /* j = i */
        j = i;
        /* i = y */
        i = y;
        y = z; /* y use old b storage */
    }

    if ((i->negative) && (!VLONG_isVlongZero(i)))
        if (OK > (status = VLONG_addSignedVlongs(i, pModulus, ppVlongQueue)))
            goto cleanup;

    *ppRetModularInverse = i;
    i = NULL;

cleanup:

    VLONG_freeVlong(&j, ppVlongQueue);
    VLONG_freeVlong(&i, ppVlongQueue);
    VLONG_freeVlong(&b, ppVlongQueue);
    VLONG_freeVlong(&c, ppVlongQueue);
    VLONG_freeVlong(&x, ppVlongQueue);
    VLONG_freeVlong(&y, ppVlongQueue);

    return status;

} /* VLONG_modularInverse */


#endif /* __VLONG_MODINV_OPERATOR_HARDWARE_ACCELERATOR__ */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_decrement(vlong *pThis, vlong **ppVlongQueue)
{
    vlong*  pOne = NULL;
    MSTATUS status;

    if (OK > (status = VLONG_makeVlongFromUnsignedValue(1, &pOne, ppVlongQueue)))
        goto exit;

    status = VLONG_subtractSignedVlongs(pThis, pOne, ppVlongQueue);

exit:
    VLONG_freeVlong(&pOne, ppVlongQueue);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_increment(vlong *pThis, vlong **ppVlongQueue)
{
    vlong*  pOne = NULL;
    MSTATUS status;

    if (OK > (status = VLONG_makeVlongFromUnsignedValue(1, &pOne, ppVlongQueue)))
        goto exit;

    status = VLONG_addSignedVlongs(pThis, pOne, ppVlongQueue);

    VLONG_freeVlong(&pOne, ppVlongQueue);

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_addImmediate(vlong *pThis, ubyte4 immVal, vlong **ppVlongQueue)
{
    vlong*  pImmVal = NULL;
    MSTATUS status;

    if (OK > (status = VLONG_makeVlongFromUnsignedValue(immVal, &pImmVal, ppVlongQueue)))
        goto exit;

    status = VLONG_addSignedVlongs(pThis, pImmVal, ppVlongQueue);

    VLONG_freeVlong(&pImmVal, ppVlongQueue);

exit:
    return status;
}


/*------------------------------------------------------------------*/

#if !defined(__DISABLE_MOCANA_BARRETT__)

extern MSTATUS
VLONG_newBarrettMu( vlong** ppMu, const vlong* m, vlong** ppVlongQueue)
{
    /* pMu = floor( (2 ^ digit_size ) ^ (2 * m->numUnitsUsed) / m) */
    vlong* p = NULL;
    vlong* r = NULL;
    vlong* mu = NULL;
    MSTATUS status;

    if (OK > ( status = VLONG_allocVlong( &p, ppVlongQueue)) ||
        OK > ( status = VLONG_allocVlong( &r, ppVlongQueue)) ||
        OK > ( status = VLONG_allocVlong( &mu, ppVlongQueue)))
    {
        goto exit;
    }

    DEBUG_RELABEL_MEMORY(p);
    DEBUG_RELABEL_MEMORY(r);
    DEBUG_RELABEL_MEMORY(mu);

    /* p = 2 ^ (digit_size * 2 * m->numUnitsUsed) */
    if ( OK > ( status = VLONG_setVlongUnit( p, m->numUnitsUsed * 2, 1)))
        goto exit;


    if ( OK > ( status = VLONG_unsignedDivide( mu, p, m, r, ppVlongQueue)))
        goto exit;

    *ppMu = mu;
    mu = 0;

exit:

    VLONG_freeVlong(&p, ppVlongQueue);
    VLONG_freeVlong(&r, ppVlongQueue);
    VLONG_freeVlong(&mu, ppVlongQueue);

    return status;
}

/*------------------------------------------------------------------*/

static MSTATUS
VLONG_barrettReduction( vlong* pResult, const vlong* pX, const vlong* pM,
                        const vlong* pMu, vlong** ppVlongQueue)
{
     /*
        q1 = floor(x / b^(k-1))
        q2 = q1 * mu
        q3 = floor(q2 / b^(k+1))
        r2 = (q3 * m) mod b^(k+1)
        r1 = x mod b^(k+1)
        r1  -=  r2

        if(r1 < 0)
            r1 = r1 + b^(k+1)
        while(r1 >= m)
            r1 = r1 - m
        return r1
    */
    vlong* q1 = NULL;
    vlong* q2 = NULL;
    ubyte4 i, j;
    ubyte4 k;
    MSTATUS status;


    /* q1 = floor(x / b^(k-1)) <=>  X shifted right by k-1 units */
    k = pM->numUnitsUsed;

    if (compareUnsignedVlongs(pX, pM) < 0)
    {
        status = VLONG_copySignedValue( pResult, pX);
        goto exit;
    }

    if ( OK > ( status = VLONG_allocVlong( &q1, ppVlongQueue)) ||
         OK > ( status = VLONG_allocVlong( &q2, ppVlongQueue)) )
    {
        goto exit;
    }

    if (OK > ( status = expandVlong( q1, pX->numUnitsUsed - k + 1)))
        goto exit;

    j = 0;
    for (i = k - 1; i < pX->numUnitsUsed; ++i)
    {
        q1->pUnits[j++] = pX->pUnits[i];
    }
    q1->numUnitsUsed = j;

    /* q2 = q1 * mu */
    if (OK > ( status = VLONG_unsignedMultiply( q2, q1, pMu)))
        goto exit;

    /* q2 = floor(q2 / b^(k+1)) <=> q2 shifted right by k+1 units */
    j = 0;
    for ( i = k + 1; i < q2->numUnitsUsed; ++i)
    {
        q2->pUnits[j++] = q2->pUnits[i];
    }
    q2->numUnitsUsed = j;

    /* r2 = (q3 * m) mod b^(k+1) <=> multiply and keep only the k+1 least significant units */
    /* reuse q1. */
    if (OK > ( status = fastUnsignedMultiplyVlongs(q1, q2, pM, (k+1) )))
        goto exit;
    if (q1->numUnitsUsed > k + 1)
        q1->numUnitsUsed = k + 1;

    /* r1 = x mod b^(k+1) r1 = (k+1) least significant units */
    if  (OK > ( status = expandVlong( pResult, k + 1)))
        goto exit;
    for (i = 0; i < k + 1; ++i)
    {
        pResult->pUnits[i] = pX->pUnits[i];
    }
    pResult->numUnitsUsed = k + 1;

    /*  r1 -=  r2  ( remember that q1 = r2 in this code)
        if(r1 < 0)
            r1 = r1 + b^(k+1)
    */
    /* we can't call subtractUnsignedVlongs if pResult < q1 */
    if ( compareUnsignedVlongs( pResult, q1) < 0)
    {
        if ( OK > (status = VLONG_setVlongUnit( pResult, k+1, 1) ))
            goto exit;
    }
    if (OK > ( status = subtractUnsignedVlongs( pResult, q1)))
        goto exit;

    while ( compareUnsignedVlongs( pResult, pM) > 0)
    {
        if (OK > ( status = subtractUnsignedVlongs( pResult, pM)))
        {
            goto exit;
        }
    }

exit:

    VLONG_freeVlong(&q1, ppVlongQueue);
    VLONG_freeVlong(&q2, ppVlongQueue);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_barrettMultiply( vlong* pResult, const vlong* pX, const vlong* pY,
                        const vlong* pM, const vlong* pMu, vlong** ppVlongQueue)
{
    MSTATUS status;
    vlong* pProduct = 0;

    if ( OK > ( status = VLONG_allocVlong( &pProduct, ppVlongQueue)))
        goto exit;

    if ( OK > ( status = VLONG_unsignedMultiply( pProduct, pX, pY)))
        goto exit;

    if ( OK > ( status = VLONG_barrettReduction( pResult, pProduct, pM, pMu,
            ppVlongQueue)))
    {
        goto exit;
    }

exit:

    VLONG_freeVlong(&pProduct, ppVlongQueue);
    return status;
}

#endif /* __DEFINED_MOCANA_BARRETT__ */

/*------------------------------------------------------------------*/

/* multiplication is so fast with altivec we are better off
computing the whole modular inverse of N instead of only its first
word -- this way we can use Altivec Multiplication instead of
multiplying word by word */
#ifdef __ALTIVEC__
#define NUM_MONTY_VLONG (4)
#define NUM_MW_VLONG (2)
#else
#define NUM_MONTY_VLONG (3)
#define NUM_MW_VLONG (1)
#endif

/* once initialized, is const and can be shared among threads */
typedef struct MontgomeryCtx
{
#ifndef __ALTIVEC__
    vlong_unit rho;
#endif
    vlong*  v[NUM_MONTY_VLONG];
} MontgomeryCtx;

#define MONTY_R(m)          ((m)->v[0])
#define MONTY_R1(m)         ((m)->v[1])
#define MONTY_N(m)          ((m)->v[2])
#ifdef __ALTIVEC__
#define MONTY_N1(m)         ((m)->v[3])
#endif

/* one per thread */
typedef struct MontgomeryWork
{
    vlong* vw[NUM_MW_VLONG];
} MontgomeryWork;

#define MW_T(mw)            ((mw)->vw[0])
#ifdef __ALTIVEC__
#define MW_K(mw)            ((mw)->vw[1])
#endif


/*----------------------------------------------------------------------*/

#ifndef __ALTIVEC__
static vlong_unit
VLONG_rho( const vlong* modulus)
{
    vlong_unit b = modulus->pUnits[0];
    vlong_unit x;

    x = (((b+2) & 4) << 1) + b;
    x *= 2 - b * x;  /* 8 BIT */
    x *= 2 - b * x;  /* 16 BIT */
    x *= 2 - b * x;  /* 32 BIT */
#if defined(__ENABLE_MOCANA_64_BIT__)
    x *= 2 - b * x;  /* 64 BIT */
#endif
    return (ZERO_UNIT - x);
}
#endif


/*------------------------------------------------------------------*/

static void
VLONG_cleanMontgomeryWork( MontgomeryWork* pMW, vlong **ppVlongQueue)
{
    sbyte4 i;
    for (i = 0; i < NUM_MW_VLONG; ++i)
    {
        VLONG_freeVlong(&pMW->vw[i], ppVlongQueue);
    }
}


/*------------------------------------------------------------------*/

static MSTATUS
VLONG_initMontgomeryWork(MontgomeryWork *pMW, const MontgomeryCtx *pMonty,
                         vlong **ppVlongQueue)
{
    MSTATUS status;
    ubyte4 numUnits = 2 * MONTY_N(pMonty)->numUnitsUsed + 1;

    MOC_MEMSET((ubyte *)pMW, 0x00, sizeof(MontgomeryWork));

    if (OK > (status = VLONG_allocVlong(&MW_T(pMW), ppVlongQueue)))
        goto cleanup;

    if (OK > ( status = expandVlong( MW_T(pMW), numUnits)))
    {
        goto cleanup;
    }

#ifdef __ALTIVEC__
    if (OK > (status = VLONG_allocVlong(&MW_K(pMW), ppVlongQueue)))
        goto cleanup;

    if (OK > ( status = expandVlong( MW_K(pMW), numUnits)))
    {
        goto cleanup;
    }
#endif

    goto exit;

cleanup:
    VLONG_cleanMontgomeryWork(pMW, ppVlongQueue);

exit:

    return status;

} /* VLONG_initMontgomeryWork */


/*------------------------------------------------------------------*/

static void
VLONG_cleanMontgomeryCtx(MontgomeryCtx *pMonty, vlong **ppVlongQueue)
{
    sbyte4 i;
    for (i = 0; i < NUM_MONTY_VLONG; ++i)
    {
        VLONG_freeVlong(&pMonty->v[i], ppVlongQueue);
    }
}


/*------------------------------------------------------------------*/

static MSTATUS
VLONG_initMontgomeryCtx(MOC_MOD(hwAccelDescr hwAccelCtx) MontgomeryCtx *pMonty,
                        const vlong *pM, vlong **ppVlongQueue)
{
    MSTATUS status;
#ifdef __ALTIVEC__
    vlong* tmp = 0;
#endif

    MOC_MEMSET((ubyte *)pMonty, 0x00, sizeof(MontgomeryCtx));

#ifndef __ALTIVEC__
    pMonty->rho = VLONG_rho(pM);
#endif

    if (OK > (status = VLONG_allocVlong(&MONTY_R(pMonty), ppVlongQueue)))
        goto cleanup;

    DEBUG_RELABEL_MEMORY(MONTY_R(pMonty));

    if (OK > ( status = VLONG_setVlongUnit( MONTY_R(pMonty),
                                            pM->numUnitsUsed, 1)))
    {
        goto cleanup;
    }

    if (OK > (status = VLONG_makeVlongFromVlong(pM, &MONTY_N(pMonty),
                                                ppVlongQueue)))
    {
        goto cleanup;
    }
    /* MONTY_R1(pMonty) = VLONG_modularInverse(pMonty->R, MONTY_N(pMonty)) */
    if (OK > (status = VLONG_modularInverse(MOC_MOD(hwAccelCtx)
                                            MONTY_R(pMonty), pM,
                                            &MONTY_R1(pMonty), ppVlongQueue)))
    {
        goto cleanup;
    }

#ifdef __ALTIVEC__
    /* MONTY_N1(pMonty) = pMonty->R - VLONG_modularInverse(MONTY_M(pMonty), pMonty->R) */
    if (OK > (status = VLONG_modularInverse(MOC_MOD(hwAccelCtx)
                                            MONTY_N(pMonty), MONTY_R(pMonty),
                                            &tmp, ppVlongQueue)))
    {
        goto cleanup;
    }

    if (OK > (status = operatorMinusSignedVlongs(MONTY_R(pMonty), tmp,
                                                 &MONTY_N1(pMonty),
                                                 ppVlongQueue)))
    {
        goto cleanup;
    }
#endif

    goto exit;

cleanup:
    VLONG_cleanMontgomeryCtx(pMonty, ppVlongQueue);

exit:

#ifdef __ALTIVEC__
    VLONG_freeVlong(&tmp, ppVlongQueue);
#endif

    return status;

} /* VLONG_initMontgomeryCtx */


#ifdef __ALTIVEC__

/*------------------------------------------------------------------*/

static MSTATUS
VLONG_montyMultiply(const MontgomeryCtx *pMonty,
                    vlong* a, const vlong* b,
                    MontgomeryWork* pMW)
{
    MSTATUS status;
    ubyte4 j;
    vlong_unit borrow;
    const vlong* pModulus = MONTY_N(pMonty);
    const vlong_unit* pModulusUnits = pModulus->pUnits;
    vlong* T = MW_T(pMW);
    vlong* k = MW_K(pMW);
    vlong_unit* pTUnits;
    vlong_unit* pAUnits;
    ubyte4 numModUnits;

    numModUnits = pModulus->numUnitsUsed;

    /* T = x*y */
    status = VLONG_FAST_MULT(T, a, b, 2*numModUnits);
    if (OK > status)
        goto exit;

    /* k = ( T * n1 ) % R */
    status = VLONG_FAST_MULT(k, T, MONTY_N1(pMonty), numModUnits);
    if (OK > status)
        goto exit;

    /* x = ( T + k*n ) / R */
    status = VLONG_FAST_MULT(a, k, MONTY_N(pMonty), 2*numModUnits);
    if (OK > status)
        goto exit;

    /* x += pMonty->T */
    status = addUnsignedVlongs(a, T);
    if (OK > status)
        goto exit;

    pTUnits = T->pUnits;
    pAUnits = a->pUnits;

    /* always do the subtraction for protection against side channel attacks */
    borrow = 0;
    for (j = 0; j < numModUnits; ++j)
    {
        vlong_unit n_unit;
        vlong_unit a_unit = pAUnits[j+numModUnits];
        vlong_unit bbb = ( a_unit < borrow) ? 1 : 0;

        pTUnits[j] = a_unit - borrow;

        n_unit = pModulusUnits[j];
        bbb += ( pTUnits[j] < n_unit) ? 1 : 0;
        pTUnits[j] -= n_unit;
        borrow = bbb;
    }
    if ((a->numUnitsUsed <= 2*numModUnits && borrow ) ||
        pAUnits[2*numModUnits] < borrow)
    {
        pTUnits = pAUnits + numModUnits;
    }

    /* copy in place */
    for ( j = numModUnits-1; pTUnits[j] == ZERO_UNIT && j !=0; --j)
    {

    }
    a->numUnitsUsed = j+1;
    for (j = 0; j < a->numUnitsUsed; ++j)
    {
        pAUnits[j] = pTUnits[j];
    }

exit:
    return status;

} /* VLONG_montyMultiply */


/*------------------------------------------------------------------*/

static MSTATUS
VLONG_montySqr(const MontgomeryCtx *pMonty,
               vlong* a, MontgomeryWork * pMW)
{
    MSTATUS status;
    ubyte4 j;
    vlong_unit borrow;
    const vlong* pModulus = MONTY_N(pMonty);
    vlong* T = MW_T(pMW);
    vlong* k = MW_K(pMW);
    const vlong_unit* pModulusUnits = pModulus->pUnits;
    vlong_unit* pTUnits;
    vlong_unit* pAUnits;
    ubyte4 numModUnits;

    numModUnits = pModulus->numUnitsUsed;

    /* T = x*y */
    status = VLONG_FAST_SQR(T, a, 2*numModUnits);
    if (OK > status)
        goto exit;
    /* k = ( T * n1 ) % R */
    status = VLONG_FAST_MULT(k, T, MONTY_N1(pMonty), numModUnits);
    if (OK > status)
        goto exit;

    /* x = ( T + k*n ) / R */
    status = VLONG_FAST_MULT(a, k, MONTY_N(pMonty), 2*numModUnits);
    if (OK > status)
        goto exit;

    /* x += pMonty->T */
    status = addUnsignedVlongs(a, T);
    if (OK > status)
        goto exit;

    pTUnits = T->pUnits;
    pAUnits = a->pUnits;

    /* always do the subtraction for protection against side channel attacks */
    borrow = 0;
    for (j = 0; j < numModUnits; ++j)
    {
        vlong_unit n_unit;
        vlong_unit a_unit = pAUnits[j+numModUnits];
        vlong_unit bbb = ( a_unit < borrow) ? 1 : 0;

        pTUnits[j] = a_unit - borrow;

        n_unit = pModulusUnits[j];
        bbb += ( pTUnits[j] < n_unit) ? 1 : 0;
        pTUnits[j] -= n_unit;
        borrow = bbb;
    }
    if ((a->numUnitsUsed <= 2*numModUnits && borrow ) ||
        pAUnits[2*numModUnits] < borrow)
    {
        pTUnits = pAUnits + numModUnits;
    }

    /* copy in place */
    for ( j = numModUnits-1; pTUnits[j] == ZERO_UNIT && j !=0; --j)
    {

    }
    a->numUnitsUsed = j+1;
    for (j = 0; j < a->numUnitsUsed; ++j)
    {
        pAUnits[j] = pTUnits[j];
    }

exit:
    return status;

} /* VLONG_montySqr */


#else

/*------------------------------------------------------------------*/

static MSTATUS
VLONG_montyMultiply(const MontgomeryCtx *pMonty,
                    vlong* a, const vlong* b,
                    MontgomeryWork* pMW)
{
    MSTATUS status;
    vlong* T = MW_T(pMW);
    const vlong* pModulus = MONTY_N(pMonty);
    vlong_unit rho = pMonty->rho;
    const vlong_unit* pModulusUnits = pModulus->pUnits;
    vlong_unit* pTUnits;
    ubyte4 numModUnits;
#ifndef MONT_MULT_REDUCTION
    ubyte4 i, j;
    vlong_unit r0, r1, r2, m[1];
    vlong_unit borrow;
    vlong_unit* pAUnits;
#endif

    numModUnits = pModulus->numUnitsUsed;


#ifdef MONT_MULT_MULTIPLY
    if (numModUnits > 1 && a->numUnitsUsed == numModUnits &&
        b->numUnitsUsed == numModUnits)
    {
        MONT_MULT_MULTIPLY( a->pUnits, b->pUnits, pModulusUnits,
                            rho, numModUnits, T->pUnits);
        a->numUnitsUsed = numModUnits;
        return OK;
    }
#endif

    if (OK > ( status = expandVlong(a, numModUnits)))
    {
        goto exit;
    }

    T->pUnits[2*numModUnits] = ZERO_UNIT;

    /* T = x*y */
    status = VLONG_FAST_MULT(T, a, b, 2 * numModUnits);
    if (OK > status)
        goto exit;

    pTUnits = T->pUnits;

#ifdef MONT_MULT_REDUCTION
    a->numUnitsUsed = MONT_MULT_REDUCTION(pTUnits, pModulusUnits, rho,
                                          numModUnits, a->pUnits);
#else
    r2 = 0;
    for (i = 0; i < numModUnits; ++i)
    {
        r0 = 0;
        m[0] = pTUnits[i] * rho;
        for ( j = 0; j < numModUnits; ++j)
        {
#ifdef MULT_ADD_MONT
            MULT_ADD_MONT(pTUnits,i+j, pModulusUnits,j, m,0,r0,r1);
#else
            /* r0 = t[i+j] + r0; */
            r0 += pTUnits[i+j];
            /* carry ? */
            r1 = (r0 < pTUnits[i+j]) ? 1 : 0;
            /* r1:r0 +=  m * n[j] */
            MULT_ADDC1(pModulusUnits, m, j, 0, r0, r1);
#endif
            pTUnits[i+j] = r0;
            r0 = r1; r1 = 0;
        }
        r0 += r2;
        r2 = (r0 < r2) ? 1 : 0;
        pTUnits[i+j] += r0;
        if (pTUnits[i+j] < r0)
        {
            r2++;
        }
    }
    pTUnits[2*numModUnits] += r2;

    /* always do the subtraction for protection against side channel attacks */
    borrow = 0;
    for (j = 0; j < numModUnits; ++j)
    {
        vlong_unit nunit = pModulusUnits[j];
        vlong_unit tunit = pTUnits[j+numModUnits];
        vlong_unit bbb = (tunit < borrow) ? 1 : 0;

        pTUnits[j] = tunit - borrow;

        bbb += ( pTUnits[j] < nunit) ? 1 : 0;
        pTUnits[j] -= nunit;
        borrow = bbb;
    }
    if (pTUnits[2*numModUnits] < borrow)
    {
        pTUnits += numModUnits;
    }

    /* copy */
    for ( j = numModUnits-1; pTUnits[j] == ZERO_UNIT && j !=0; --j)
    {

    }
    a->numUnitsUsed = j+1;
    pAUnits = a->pUnits;
    for (j = 0; j < a->numUnitsUsed; ++j)
    {
        pAUnits[j] = pTUnits[j];
    }
#endif

exit:
    return status;

} /* VLONG_montyMultiply */


/*------------------------------------------------------------------*/

static MSTATUS
VLONG_montySqr(const MontgomeryCtx *pMonty,
               vlong* a, MontgomeryWork* pMW)
{
    MSTATUS status;
    vlong* T = MW_T(pMW);
    const vlong* pModulus = MONTY_N(pMonty);
    vlong_unit rho = pMonty->rho;
    const vlong_unit* pModulusUnits = pModulus->pUnits;
    vlong_unit* pTUnits;
    ubyte4 numModUnits;

#ifndef MONT_MULT_REDUCTION
    ubyte4 i, j;
    vlong_unit r0, r1, r2, m[1];
    vlong_unit borrow;
    vlong_unit* pAUnits;
#endif

    numModUnits = pModulus->numUnitsUsed;

#ifdef MONT_MULT_SQR
    if (numModUnits > 1 && a->numUnitsUsed == numModUnits)
    {
        MONT_MULT_SQR( a->pUnits, pModulusUnits,
                       rho, numModUnits, T->pUnits);
        a->numUnitsUsed = numModUnits;
        return OK;
    }
#endif

    if (OK > (status = expandVlong(a, numModUnits)))
    {
        goto exit;
    }

    T->pUnits[2*numModUnits] = ZERO_UNIT;

    /* T = x*x */
    status = VLONG_FAST_SQR(T, a, 2 * numModUnits);
    if (OK > status)
        goto exit;

    pTUnits = T->pUnits;

#ifdef MONT_MULT_REDUCTION
    a->numUnitsUsed = MONT_MULT_REDUCTION(pTUnits, pModulusUnits, rho,
                                          numModUnits, a->pUnits);
#else
    r2 = 0;
    for (i = 0; i < numModUnits; ++i)
    {
        r0 = 0;
        m[0] = pTUnits[i] * rho;
        for ( j = 0; j < numModUnits; ++j)
        {

#ifdef MULT_ADD_MONT
            MULT_ADD_MONT(pTUnits, i+j, pModulusUnits,j, m,0,r0,r1);
#else
            /* r0 = t[i+j] + r0; */
            r0 += pTUnits[i+j];
            /* carry ? */
            r1 = (r0 < pTUnits[i+j]) ? 1 : 0;
            /* r1:r0 +=  m * n[j] */
            MULT_ADDC1(pModulusUnits, m, j, 0, r0, r1);
#endif
            pTUnits[i+j] = r0;
            r0 = r1;
        }
        r0 += r2;
        r2 = (r0 < r2) ? 1 : 0;
        pTUnits[i+j] += r0;
        if (pTUnits[i+j] < r0)
        {
            r2++;
        }
    }
    pTUnits[2*numModUnits] += r2;

    /* always do the subtraction for protection against side channel attacks */
    borrow = 0;
    for (j = 0; j < numModUnits; ++j)
    {
        vlong_unit nunit =  pModulusUnits[j];
        vlong_unit tunit = pTUnits[j+numModUnits];
        vlong_unit bbb = (tunit < borrow) ? 1 : 0;

        pTUnits[j] = tunit - borrow;

        bbb += ( pTUnits[j] < nunit) ? 1 : 0;
        pTUnits[j] -= nunit;
        borrow = bbb;
    }
    if (pTUnits[2*numModUnits] < borrow)
    {
        pTUnits += numModUnits;
    }

    /* copy */
    for ( j = numModUnits-1; pTUnits[j] == ZERO_UNIT && j !=0; --j)
    {

    }
    a->numUnitsUsed = j+1;
    pAUnits = a->pUnits;
    for (j = 0; j < a->numUnitsUsed; ++j)
    {
        pAUnits[j] = pTUnits[j];
    }
#endif

exit:
    return status;

} /* VLONG_montySqr */

#endif

/*------------------------------------------------------------------*/

static MSTATUS
VLONG_montgomeryExpBin(MOC_MOD(hwAccelDescr hwAccelCtx) const MontgomeryCtx *pMonty,
                       const vlong *x, const vlong *e, vlong **ppRetMontyExp,
                       vlong **ppVlongQueue)
{
    vlong* result  = NULL;
    vlong* t       = NULL;
    vlong* tmp     = NULL;
    ubyte4 bits    = VLONG_bitLength(e);
    ubyte4 i       = 0;
    MontgomeryWork mw = {{0}};
    MSTATUS status;

    if ( OK > (status = VLONG_initMontgomeryWork( &mw, pMonty, ppVlongQueue)))
    {
        goto cleanup;
    }

    /* result = pMonty->R % MONTY_M(pMonty) */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) MONTY_R(pMonty),
                                                     MONTY_N(pMonty), &result,
                                                     ppVlongQueue)))
    {
        goto cleanup;
    }

    MOCANA_YIELD_PROCESSOR();

    /* t = (x * pMonty->R) % MONTY_M(pMonty) */
    if (OK > (status = operatorMultiplySignedVlongs(x, MONTY_R(pMonty),
                                                    &tmp, ppVlongQueue)))
    {
        goto cleanup;
    }
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp,
                                                     MONTY_N(pMonty), &t,
                                                     ppVlongQueue)))
    {
        goto cleanup;
    }

    VLONG_freeVlong(&tmp, ppVlongQueue);

    while (1)
    {
        /* if (e->test(i)) then montyMultiply(pMonty,result,t) */
        if (TRUE == VLONG_isVlongBitSet(e,i))
            if (OK > (status = VLONG_montyMultiply(pMonty,result,t,&mw)))
                goto cleanup;

        i++;

        if (i == bits)
            break;

        /* montyMultiply(pMonty,pMonty->t,pMonty->t) */
        if (OK > (status = VLONG_montySqr(pMonty, t, &mw)))
            goto cleanup;
    }

    /* *ppRetMontyExp = (result * MONTY_R1(pMonty)) % MONTY_M(pMonty) */
    if (OK > (status = operatorMultiplySignedVlongs(result, MONTY_R1(pMonty),
                                                    &tmp, ppVlongQueue)))
    {
        goto cleanup;
    }
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp,
                                                     MONTY_N(pMonty),
                                                     ppRetMontyExp,
                                                     ppVlongQueue)))
    {
        goto cleanup;
    }

cleanup:
    VLONG_cleanMontgomeryWork( &mw, ppVlongQueue);
    VLONG_freeVlong(&result, ppVlongQueue);
    VLONG_freeVlong(&t, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);

    MOCANA_YIELD_PROCESSOR();

    return status;

} /* montyExpBin */


#if !defined(__DISABLE_MOCANA_MODEXP_SLIDING_WINDOW__)
/*------------------------------------------------------------------*/

static MSTATUS
VLONG_montgomeryExp(MOC_MOD(hwAccelDescr hwAccelCtx) const MontgomeryCtx *pMonty,
                    const vlong *x, const vlong *e, vlong **ppRetMontyExp,
                    vlong **ppVlongQueue)
{
    MontgomeryWork mw = {{0}};
    vlong* result  = NULL;
    vlong* tmp     = NULL;
    ubyte4 bits    = VLONG_bitLength(e);
    sbyte4 i;
    MSTATUS status;
    vlong* g[32]  = { 0};
    sbyte4 winSize; /* windowSize */

    winSize = (bits > 671 ? 6 :
                bits > 239 ? 5 :
                bits >  79 ? 4 :
                bits >  23 ? 3 : 2);

    if (1 == winSize)
    {
        return VLONG_montgomeryExpBin( MOC_MOD(hwAccelCtx) pMonty, x, e,
                                       ppRetMontyExp, ppVlongQueue);
    }

    if ( OK > ( status = VLONG_initMontgomeryWork( &mw, pMonty, ppVlongQueue)))
    {
        goto cleanup;
    }

    /* result = pMonty->R % MONTY_M(pMonty)  = (1 * R) mod m */
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) MONTY_R(pMonty),
                                                     MONTY_N(pMonty), &result,
                                                     ppVlongQueue)))
    {
        goto cleanup;
    }

    /* g[0] = (x * R) % m */
    if (OK > (status = operatorMultiplySignedVlongs(x, MONTY_R(pMonty), &tmp,
                                                    ppVlongQueue)))
    {
        goto cleanup;
    }

    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp,
                                                     MONTY_N(pMonty), g,
                                                     ppVlongQueue)))
    {
        goto cleanup;
    }

    /* tmp = g[0] * g[0] */
    if (OK > (status = VLONG_copySignedValue( tmp, g[0])))
        goto cleanup;
    if (OK > ( status = VLONG_montySqr(pMonty, tmp, &mw)))
        goto cleanup;

    for (i = 1; i < (1 << (winSize - 1)); i++)
    {
        if (OK > ( status = VLONG_makeVlongFromVlong(g[i-1], g+i, ppVlongQueue)))
            goto cleanup;

        if (OK > ( status = VLONG_montyMultiply(pMonty, g[i], tmp, &mw)))
            goto cleanup;
    }
    VLONG_freeVlong(&tmp, ppVlongQueue);

    i = bits-1;
    while ( i >= 0)
    {
        if  (!VLONG_isVlongBitSet(e,i))
        {
            VLONG_montySqr(pMonty, result, &mw);
            --i;
        }
        else
        {
            /* find the longest bitstring of size <= window where last bit = 1 */
            sbyte4 j, L, index;

            index = 1; /* window value used as index for g[] */
            L = 1;     /* window length */

            if ( i > 0)
            {
                sbyte4 max = ( i + 1 < winSize) ? i : winSize;

                for (j = 1; j < max; ++j)
                {
                    index <<= 1;
                    if ( VLONG_isVlongBitSet( e, i - j))
                    {
                        L = j + 1;
                        index += 1;
                    }
                }
                index >>= (max - L);
            }

            /* assert( index & 1); index should be odd! */

            for (j = 0; j < L; ++j)
            {
                VLONG_montySqr(pMonty, result, &mw);
            }
            VLONG_montyMultiply( pMonty, result, g[index>>1], &mw);

            i -= L;
         }
    }

    /* convert from Monty residue to "real" number */
    /* *ppRetMontyExp = (result * MONTY_R1(pMonty)) % MONTY_M(pMonty) */
    if (OK > (status = operatorMultiplySignedVlongs(result, MONTY_R1(pMonty),
                                                    &tmp, ppVlongQueue)))
    {
        goto cleanup;
    }
    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp,
                                                     MONTY_N(pMonty),
                                                     ppRetMontyExp,
                                                     ppVlongQueue)))
    {
        goto cleanup;
    }
cleanup:
    VLONG_freeVlong(&result, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);
    VLONG_cleanMontgomeryWork( &mw, ppVlongQueue);

    for (i = 0; i < (1 << (winSize - 1)); ++i)
    {
        VLONG_freeVlong(g+i, ppVlongQueue);
    }

    return status;

} /* montyExp */

#endif

/*------------------------------------------------------------------*/

#ifndef __VLONG_MODEXP_OPERATOR_HARDWARE_ACCELERATOR__

static MSTATUS
VLONG_modexp_montgomery(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *x, const vlong *e, const vlong *n,
             vlong **ppRetModExp, vlong **ppVlongQueue)
{

    /* (x^e) mod m */
    MontgomeryCtx   me;
    MSTATUS status = ERR_FALSE;

    if (1 != (n->pUnits[0] & 1))
    {
        status = ERR_BAD_MODULO;
        goto exit;
    }

    MOCANA_YIELD_PROCESSOR();


    if (OK <= (status = VLONG_initMontgomeryCtx(MOC_MOD(hwAccelCtx) &me, n, ppVlongQueue)))
    {
#ifdef __DISABLE_MOCANA_MODEXP_SLIDING_WINDOW__
        status = VLONG_montgomeryExpBin(MOC_MOD(hwAccelCtx) &me, x, e,
                             ppRetModExp, ppVlongQueue);
#else
        status = VLONG_montgomeryExp(MOC_MOD(hwAccelCtx) &me, x, e,
                          ppRetModExp, ppVlongQueue);
#endif

    }

    VLONG_cleanMontgomeryCtx(&me, ppVlongQueue);

exit:

    return status;
}

/*------------------------------------------------------------------*/

#if !defined(__DISABLE_MOCANA_BARRETT__)

static MSTATUS
VLONG_modexp_barrett(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *x, const vlong *e,
                     const vlong *n, vlong **ppRet, vlong **ppVlongQueue)
{
    vlong* result  = NULL;
    vlong* tmp     = NULL;
    vlong* S       = NULL;
    vlong* mu      = NULL;
    ubyte4 i, bits;
    MSTATUS status;

    if (OK > ( status = VLONG_makeVlongFromUnsignedValue(1, &result, ppVlongQueue)))
        goto exit;

    if (OK > ( status = VLONG_makeVlongFromVlong(x, &S, ppVlongQueue)))
        goto exit;

    if (OK > ( status = VLONG_allocVlong( &tmp, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(tmp);

    if (OK > ( status = VLONG_newBarrettMu(&mu, n, ppVlongQueue)))
        goto exit;

    bits = VLONG_bitLength(e);

    i = 0;
    while (1)
    {
        vlong* swap;
        if (VLONG_isVlongBitSet(e,i))
        {
            /* tmp = (result * S) mod n */
            if ( OK > ( status = VLONG_barrettMultiply( tmp, result, S, n, mu, ppVlongQueue)))
                goto exit;

            swap = result;
            result = tmp;
            tmp = swap;
        }

        i++;

        if (i == bits)
            break;

        /* tmp = (S * S)  mod n*/
        if (OK > (status = VLONG_barrettMultiply(tmp, S, S, n, mu, ppVlongQueue)))
            goto exit;

        swap = S;
        S = tmp;
        tmp = swap;
    }

    *ppRet = result;
    result = 0;

exit:
    VLONG_freeVlong(&result, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);
    VLONG_freeVlong(&S, ppVlongQueue);
    VLONG_freeVlong(&mu, ppVlongQueue);

    MOCANA_YIELD_PROCESSOR();

    return status;
}

#endif

/*------------------------------------------------------------------*/
#if defined(__DISABLE_MOCANA_BARRETT__) || defined(__ENABLE_MOCANA_MODEXP_CLASSIC__)

static MSTATUS
VLONG_modexp_classic(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *x, const vlong *e,
                     const vlong *n, vlong **ppRet, vlong **ppVlongQueue)
{
    vlong* result  = NULL;
    vlong* t       = NULL;
    vlong* tmp     = NULL;
    vlong* S       = NULL;
    ubyte4 i, bits, N;
    MSTATUS status;

    if (OK > ( status = VLONG_makeVlongFromUnsignedValue(1, &result, ppVlongQueue)))
        goto exit;

    if (OK > ( status = VLONG_makeVlongFromVlong(x, &S, ppVlongQueue)))
        goto exit;

    if (OK > ( status = VLONG_allocVlong( &tmp, ppVlongQueue)))
        goto exit;

    if (OK > ( status = VLONG_allocVlong( &t, ppVlongQueue)))
        goto exit;

    bits = VLONG_bitLength(e);
    N = VLONG_bitLength(n);
    i = 0;
    while (1)
    {
        if (VLONG_isVlongBitSet(e,i))
        {
            /* tmp = result * S */
            if (OK > (status = VLONG_FAST_MULT(tmp, result, S, 2*N)))
                goto exit;

            /* result = tmp % n */
            if (OK > ( status = VLONG_unsignedDivide( t, tmp, n, result, ppVlongQueue)))
                goto exit;

        }

        i++;

        if (i == bits)
            break;

        /* tmp = S * S */
        if (OK > (status = VLONG_FAST_SQR(tmp, S, 2*N)))
            goto exit;

        /* S = tmp % n */
        if (OK > ( status = VLONG_unsignedDivide( t, tmp, n, S, ppVlongQueue)))
            goto exit;

    }

    *ppRet = result;
    result = 0;

exit:
    VLONG_freeVlong(&result, ppVlongQueue);
    VLONG_freeVlong(&t, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);
    VLONG_freeVlong(&S, ppVlongQueue);

    MOCANA_YIELD_PROCESSOR();

    return status;
}
#endif  /* __DISABLE_MOCANA_BARRETT__ || __ENABLE_MOCANA_MODEXP_CLASSIC__ */

/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_modexp(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *x, const vlong *e,
             const vlong *n, vlong **ppRet, vlong **ppVlongQueue)
{
/* value to switch to montgomery -- assuming exponent has half of bits sets */
#define USE_MONTY_LIMIT (2)

    if ((NULL == x) || (NULL == e) || (NULL == n) || (NULL == ppRet))
    {
        return ERR_NULL_POINTER;
    }

    if (VLONG_isVlongZero(n))
    {
        return ERR_BAD_MODULO;
    }

    if (VLONG_isVlongZero(x))
    {
        MSTATUS status = VLONG_allocVlong(ppRet, ppVlongQueue);

        if(NULL != ppRet)
            DEBUG_RELABEL_MEMORY(*ppRet);

        return status;
    }

    if (VLONG_isVlongZero(e))
    {
        return VLONG_makeVlongFromUnsignedValue(1, ppRet, ppVlongQueue);
    }
    if ((n->pUnits[0] & 1) && e->numUnitsUsed >= USE_MONTY_LIMIT)
    {
        return VLONG_modexp_montgomery( MOC_MOD(hwAccelCtx) x, e, n, ppRet, ppVlongQueue);
    }
    else
    {
#if defined(__DISABLE_MOCANA_BARRETT__)
        return VLONG_modexp_classic(MOC_MOD(hwAccelCtx) x, e, n, ppRet,
                                    ppVlongQueue);
#else
        return VLONG_modexp_barrett(MOC_MOD(hwAccelCtx) x, e, n, ppRet,
                                    ppVlongQueue);
#endif
    }
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_newModExpHelper(MOC_MOD(hwAccelDescr hwAccelCtx) ModExpHelper* pMEH, const vlong* m, vlong** ppVlongQueue)
{
    MSTATUS status;
    MontgomeryCtx* pNewMonty = 0;

    pNewMonty = MALLOC( sizeof(MontgomeryCtx));
    if (!pNewMonty) return ERR_MEM_ALLOC_FAIL;

    if ( OK > ( status = VLONG_initMontgomeryCtx(MOC_MOD(hwAccelCtx) pNewMonty, m, ppVlongQueue)))
        goto exit;

    *pMEH = pNewMonty;
    pNewMonty = 0;

exit:

    if ( pNewMonty)
    {
        FREE(pNewMonty);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_deleteModExpHelper( ModExpHelper* pMEH, vlong** ppVlongQueue)
{
    if (pMEH && *pMEH)
    {
        VLONG_cleanMontgomeryCtx(*pMEH, ppVlongQueue);
        FREE( *pMEH);
        *pMEH= NULL;
    }
    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_modExp(MOC_MOD(hwAccelDescr hwAccelCtx) CModExpHelper meh,  const vlong *x, const vlong *e,
                      vlong **ppRetModExp, vlong **ppVlongQueue)
{
    MSTATUS status;

    if (VLONG_isVlongZero(x))
    {
        status = VLONG_allocVlong(ppRetModExp, ppVlongQueue);
        if(NULL != ppRetModExp)
            DEBUG_RELABEL_MEMORY(*ppRetModExp);
        goto exit;
    }

    if (VLONG_isVlongZero(e))
    {
        status = VLONG_makeVlongFromUnsignedValue(1, ppRetModExp, ppVlongQueue);
        goto exit;
    }

#ifdef __DISABLE_MOCANA_MODEXP_SLIDING_WINDOW__
    status = VLONG_montgomeryExpBin(MOC_MOD(hwAccelCtx) meh, x, e,
                      ppRetModExp, ppVlongQueue);
#else
    status = VLONG_montgomeryExp(MOC_MOD(hwAccelCtx) meh, x, e,
                      ppRetModExp, ppVlongQueue);
#endif
exit:

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_makeModExpHelperFromModExpHelper( CModExpHelper meh,
                                       ModExpHelper* pMEH,
                                       vlong **ppVlongQueue)
{
    MontgomeryCtx*  pNewMonty = 0;
    MSTATUS status = OK;
    sbyte4 i;

    if ((NULL == meh) || (NULL == pMEH))
    {
        return ERR_NULL_POINTER;
    }

    pNewMonty = MALLOC( sizeof( MontgomeryCtx));
    if (!pNewMonty)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET((ubyte *)pNewMonty, 0x00, sizeof(MontgomeryCtx));

#ifndef __ALTIVEC__
    pNewMonty->rho = meh->rho;
#endif

    for (i = 0; i < NUM_MONTY_VLONG; ++i)
    {
        if (OK >( status = VLONG_makeVlongFromVlong( meh->v[i], &pNewMonty->v[i], ppVlongQueue)))
            goto exit;
    }

    *pMEH = pNewMonty;
    pNewMonty = 0;

exit:
    if (pNewMonty)
    {
        VLONG_cleanMontgomeryCtx(pNewMonty, ppVlongQueue);
        FREE(pNewMonty);
    }

    return status;
}



#if !defined(__DISABLE_MOCANA_KARATSUBA__)

/*------------------------------------------------------------------*/

static sbyte4
MATH_compareValues(const vlong_unit *a, const vlong_unit *b, sbyte4 len)
{
    while ((1 < len) && (a[len-1] == b[len-1]))
        len--;

    return (a[len-1] == b[len-1]) ? 0 : ((a[len-1] > b[len-1]) ? 1 : -1);
}


/*------------------------------------------------------------------*/

static sbyte4
MATH_subtractValues(vlong_unit *result,
                    const vlong_unit *a, const vlong_unit *b, sbyte4 length)
{

#ifdef ASM_SUB3
    return ASM_SUB3( result, a, b, length);
#else

    sbyte4 carry = 0;
    vlong_unit unitA, unitB;

    if (0 >= length)
        goto exit;

    while (4 <= length)
    {
        unitA = a[0]; unitB = b[0]; result[0] = (unitA - unitB - carry);
        if (unitA != unitB)
            carry = (unitA < unitB) ? 1 : 0;

        unitA = a[1]; unitB = b[1]; result[1] = (unitA - unitB - carry);
        if (unitA != unitB)
            carry = (unitA < unitB) ? 1 : 0;

        unitA = a[2]; unitB = b[2]; result[2] = (unitA - unitB - carry);
        if (unitA != unitB)
            carry = (unitA < unitB) ? 1 : 0;

        unitA = a[3]; unitB = b[3]; result[3] = (unitA - unitB - carry);
        if (unitA != unitB)
            carry = (unitA < unitB) ? 1 : 0;

        a += 4;
        b += 4;
        result += 4;
        length -= 4;
    }

    if (0 == length)
        goto exit;

    unitA = a[0]; unitB = b[0]; result[0] = (unitA - unitB - carry);
    if (unitA != unitB)
        carry = (unitA < unitB) ? 1 : 0;
    if (0 == --length)
        goto exit;

    unitA = a[1]; unitB = b[1]; result[1] = (unitA - unitB - carry);
    if (unitA != unitB)
        carry = (unitA < unitB) ? 1 : 0;
    if (0 == --length)
        goto exit;

    unitA = a[2]; unitB = b[2]; result[2] = (unitA - unitB - carry);
    if (unitA != unitB)
        carry = (unitA < unitB) ? 1 : 0;
    if (0 == --length)
        goto exit;

    unitA = a[3]; unitB = b[3]; result[3] = (unitA - unitB - carry);
    if (unitA != unitB)
        carry = (unitA < unitB) ? 1 : 0;

exit:
    return carry;
#endif

} /* MATH_subtractValues */


/*------------------------------------------------------------------*/

static sbyte4
MATH_sumValues(vlong_unit *result, const vlong_unit *a, const vlong_unit *b,
               sbyte4 length)
{
#ifdef ASM_ADD3
    return ASM_ADD3( result,a, b, length);
#else
    vlong_unit carry = 0;
    vlong_unit temp1;
    vlong_unit temp0;

    if (0 >= length)
        goto exit;

    while (4 <= length)
    {
        temp0 = a[0] + carry; carry  = (temp0 < carry) ? 1 : 0;
        temp1 = temp0 + b[0]; carry += (temp1 < temp0) ? 1 : 0;
        result[0] = temp1;

        temp0 = a[1] + carry; carry  = (temp0 < carry) ? 1 : 0;
        temp1 = temp0 + b[1]; carry += (temp1 < temp0) ? 1 : 0;
        result[1] = temp1;

        temp0 = a[2] + carry; carry  = (temp0 < carry) ? 1 : 0;
        temp1 = temp0 + b[2]; carry += (temp1 < temp0) ? 1 : 0;
        result[2] = temp1;

        temp0 = a[3] + carry; carry  = (temp0 < carry) ? 1 : 0;
        temp1 = temp0 + b[3]; carry += (temp1 < temp0) ? 1 : 0;
        result[3] = temp1;

        a += 4;
        b += 4;
        result += 4;
        length -= 4;
    }

    if (0 == length)
        goto exit;

    temp0 = a[0] + carry; carry  = (temp0 < carry) ? 1 : 0;
    temp1 = temp0 + b[0]; carry += (temp1 < temp0) ? 1 : 0;
    result[0] = temp1;
    if (0 == --length)
        goto exit;

    temp0 = a[1] + carry; carry  = (temp0 < carry) ? 1 : 0;
    temp1 = temp0 + b[1]; carry += (temp1 < temp0) ? 1 : 0;
    result[1] = temp1;
    if (0 == --length)
        goto exit;

    temp0 = a[2] + carry; carry  = (temp0 < carry) ? 1 : 0;
    temp1 = temp0 + b[2]; carry += (temp1 < temp0) ? 1 : 0;
    result[2] = temp1;
    if (0 == --length)
        goto exit;

    temp0 = a[3] + carry; carry  = (temp0 < carry) ? 1 : 0;
    temp1 = temp0 + b[3]; carry += (temp1 < temp0) ? 1 : 0;
    result[3] = temp1;

exit:
    return (sbyte4) carry;
#endif

} /* MATH_sumValues */


/*------------------------------------------------------------------*/

#ifndef ASM_MATH_8x8_DEFINED


static void
MATH_8x8(vlong_unit *pProduct, const vlong_unit *pFactorA,
         const vlong_unit *pFactorB)
{
    vlong_unit result0,result1,result2;

    result0 = result1 = result2 = 0;

    MULT_ADDC(pFactorA,pFactorB,0,0,result0,result1,result2);
    pProduct[0] = result0; result0 = 0;

    MULT_ADDC(pFactorA,pFactorB,0,1,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,1,0,result1,result2,result0);
    pProduct[1] = result1; result1 = 0;

    MULT_ADDC(pFactorA,pFactorB,2,0,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,1,1,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,0,2,result2,result0,result1);
    pProduct[2] = result2; result2 = 0;

    MULT_ADDC(pFactorA,pFactorB,0,3,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,1,2,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,2,1,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,3,0,result0,result1,result2);
    pProduct[3] = result0; result0 = 0;

    MULT_ADDC(pFactorA,pFactorB,4,0,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,3,1,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,2,2,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,1,3,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,0,4,result1,result2,result0);
    pProduct[4] = result1; result1 = 0;

    MULT_ADDC(pFactorA,pFactorB,0,5,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,1,4,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,2,3,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,3,2,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,4,1,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,5,0,result2,result0,result1);
    pProduct[5] = result2; result2 = 0;

    MULT_ADDC(pFactorA,pFactorB,0,6,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,1,5,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,2,4,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,3,3,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,4,2,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,5,1,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,6,0,result0,result1,result2);
    pProduct[6] = result0; result0 = 0;

    MULT_ADDC(pFactorA,pFactorB,0,7,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,1,6,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,2,5,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,3,4,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,4,3,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,5,2,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,6,1,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,7,0,result1,result2,result0);
    pProduct[7] = result1; result1 = 0;

    MULT_ADDC(pFactorA,pFactorB,7,1,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,6,2,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,5,3,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,4,4,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,3,5,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,2,6,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,1,7,result2,result0,result1);
    pProduct[8] = result2; result2 = 0;

    MULT_ADDC(pFactorA,pFactorB,2,7,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,3,6,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,4,5,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,5,4,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,6,3,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,7,2,result0,result1,result2);
    pProduct[9] = result0; result0 = 0;

    MULT_ADDC(pFactorA,pFactorB,7,3,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,6,4,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,5,5,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,4,6,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,3,7,result1,result2,result0);
    pProduct[10] = result1; result1 = 0;

    MULT_ADDC(pFactorA,pFactorB,4,7,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,5,6,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,6,5,result2,result0,result1);
    MULT_ADDC(pFactorA,pFactorB,7,4,result2,result0,result1);
    pProduct[11] = result2; result2 = 0;

    MULT_ADDC(pFactorA,pFactorB,7,5,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,6,6,result0,result1,result2);
    MULT_ADDC(pFactorA,pFactorB,5,7,result0,result1,result2);
    pProduct[12] = result0; result0 = 0;

    MULT_ADDC(pFactorA,pFactorB,6,7,result1,result2,result0);
    MULT_ADDC(pFactorA,pFactorB,7,6,result1,result2,result0);
    pProduct[13] = result1; result1 = 0;

    MULT_ADDC1(pFactorA,pFactorB,7,7,result2,result0);
    pProduct[14]=result2;
    pProduct[15]=result0;
}

static void
MATH_SQR8(vlong_unit *pProduct, const vlong_unit* pFactorA)
{
    vlong_unit result0,result1,result2;
    vlong_unit half0, half1, half2;

    result0 = result1 = result2 = 0;

    MULT_ADDC(pFactorA,pFactorA,0,0,result0,result1,result2);
    pProduct[0] = result0; result0 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,0,1,half0,half1,half2);
    ADD_DOUBLE(result1,result2,result0,half0,half1,half2);
    pProduct[1] = result1; result1 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,2,0,half0,half1,half2);
    ADD_DOUBLE(result2,result0,result1,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,1,1,result2,result0,result1);
    pProduct[2] = result2; result2 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,0,3,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,1,2,half0,half1,half2);
    ADD_DOUBLE(result0,result1,result2,half0,half1,half2);
    pProduct[3] = result0; result0 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,4,0, half0, half1,half2);
    MULT_ADDC(pFactorA,pFactorA,3,1, half0, half1,half2);
    ADD_DOUBLE(result1,result2,result0, half0, half1,half2);
    MULT_ADDC(pFactorA,pFactorA,2,2,result1,result2,result0);
    pProduct[4] = result1; result1 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,0,5, half0, half1,half2);
    MULT_ADDC(pFactorA,pFactorA,1,4, half0, half1,half2);
    MULT_ADDC(pFactorA,pFactorA,2,3, half0, half1,half2);
    ADD_DOUBLE(result2,result0,result1,half0,half1,half2);
    pProduct[5] = result2; result2 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,0,6,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,1,5,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,2,4,half0,half1,half2);
    ADD_DOUBLE(result0,result1,result2,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,3,3,result0,result1,result2);
    pProduct[6] = result0; result0 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,0,7,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,1,6,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,2,5,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,3,4,half0,half1,half2);
    ADD_DOUBLE(result1,result2,result0, half0, half1,half2);
    pProduct[7] = result1; result1 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,7,1,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,6,2,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,5,3,half0,half1,half2);
    ADD_DOUBLE(result2,result0,result1,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,4,4,result2,result0,result1);
    pProduct[8] = result2; result2 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,2,7,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,3,6,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,4,5,half0,half1,half2);
    ADD_DOUBLE(result0,result1,result2,half0,half1,half2);
    pProduct[9] = result0; result0 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,7,3,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,6,4,half0,half1,half2);
    ADD_DOUBLE( result1, result2,result0, half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,5,5,result1,result2,result0);
    pProduct[10] = result1; result1 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,4,7,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,5,6,half0,half1,half2);
    ADD_DOUBLE( result2, result0,result1, half0,half1,half2);
    pProduct[11] = result2; result2 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,7,5,half0, half1, half2);
    ADD_DOUBLE(result0,result1,result2,half0,half1,half2);
    MULT_ADDC(pFactorA,pFactorA,6,6,result0,result1,result2);
    pProduct[12] = result0; result0 = 0;

    half0 = half1 = half2 = 0;
    MULT_ADDC(pFactorA,pFactorA,6,7,half0, half1, half2);
    ADD_DOUBLE( result1, result2,result0, half0,half1,half2);
    pProduct[13] = result1; result1 = 0;

    MULT_ADDC1(pFactorA,pFactorA,7,7,result2,result0);
    pProduct[14]=result2;
    pProduct[15]=result0;
}

#endif /* ASM_MATH_8x8_DEFINED */

/*------------------------------------------------------------------*/

/* (A1*B1)n^2 + ((A1+A0) * (B1+B0) - (A1*B1) - (A0*B0))n + (A0*B0) */
/* (A1*B1)n^2 + ((A1*B1) + (A0*B0) - (A1-A0) * (B1-B0))n + (A0*B0) */

/* D0 = A0*B0 */
/* D1 = A1*B1 */
/* D2 = A1-A0 * B1-B0 */

static void
karatsubaMultiply(vlong_unit *pProduct, const vlong_unit *pFactorA,
                  const vlong_unit *pFactorB, vlong_unit *pWorkspace, sbyte4 n)
{
    sbyte4      half_n = n >> 1;
    intBoolean  negative;
    intBoolean  zeroFlag;
    sbyte4      compA;
    sbyte4      compB;

    if (8 == n)
    {
        MATH_8x8(pProduct, pFactorA, pFactorB);
        return;
    }

    /* recycle stack space during recursion */
    {
        compA = MATH_compareValues(&(pFactorA[half_n]), pFactorA, half_n);
        compB = MATH_compareValues(&(pFactorB[half_n]), pFactorB, half_n);

        zeroFlag = negative = FALSE;

        switch ((compA * 4) + compB)
        {
            case -5:    /* negative times negative */
                MATH_subtractValues(pWorkspace, pFactorA, &(pFactorA[half_n]), half_n);
                MATH_subtractValues(&(pWorkspace[half_n]), pFactorB, &(pFactorB[half_n]), half_n);
                break;
            case -3:    /* negative times positive */
                MATH_subtractValues(pWorkspace, pFactorA, &(pFactorA[half_n]), half_n);
                MATH_subtractValues(&(pWorkspace[half_n]), &(pFactorB[half_n]), pFactorB, half_n);
                negative = TRUE;
                break;
            case 3:     /* positive times negative */
                MATH_subtractValues(pWorkspace, &(pFactorA[half_n]), pFactorA, half_n);
                MATH_subtractValues(&(pWorkspace[half_n]), pFactorB, &(pFactorB[half_n]), half_n);
                negative = TRUE;
                break;
            case 5:     /* positive times positive */
                MATH_subtractValues(pWorkspace, &(pFactorA[half_n]), pFactorA, half_n);
                MATH_subtractValues(&(pWorkspace[half_n]), &(pFactorB[half_n]), pFactorB, half_n);
                break;
            default:    /* some combination of a zero times some x */
                zeroFlag = TRUE;
                break;
        }
    }
    /* A1 - A0 is stored in w[0...half_n-1] B1 - B0 is stored in w[half_n...2*half_n -1] */

    if (8 != half_n)
    {
        /* deal with the inner tree first to avoid an extra jump on leaf node case */
        /* multiply A1 - A0 by B1 - B0 in the w[2*half_n...4*half_n-1] or w[n..2*n-1] */
        /* the space w[2*n...] is used as "workspace" */
        if (!zeroFlag)
            karatsubaMultiply(&(pWorkspace[n]), pWorkspace, &(pWorkspace[half_n]), &(pWorkspace[n<<1]), half_n);
        else
        {
            register int i;
            for (i = n-1; i >= 0; i--)
                pWorkspace[n + i] = 0;
        }

        karatsubaMultiply(pProduct, pFactorA, pFactorB, &(pWorkspace[n<<1]), half_n);
        karatsubaMultiply(&(pProduct[n]), &(pFactorA[half_n]), &(pFactorB[half_n]), &(pWorkspace[n<<1]), half_n);
    }
    else
    {
        if (!zeroFlag)
            MATH_8x8(&(pWorkspace[n]), pWorkspace, &(pWorkspace[half_n]));  /* D2 = A1-A0 * B1-B0 */
        else
        {
            register int i;                                                 /* D2 = A1-A0 * B1-B0 */
            for (i = 15; i >= 0; i--)
                pWorkspace[n + i] = 0;
        }

        MATH_8x8(pProduct, pFactorA, pFactorB);                             /* D0 = A0 * B0 */
        MATH_8x8(&(pProduct[n]), &(pFactorA[half_n]), &(pFactorB[half_n])); /* D1 = A1 * B1 */
    }

    /* recycle stack space for recursion */
    {
        sbyte4  carryFlag;

        carryFlag = MATH_sumValues(pWorkspace, pProduct, &(pProduct[n]), n);    /* D0 + D1 */

        if (!negative)                                                          /* D0 + D1 - D2 */
            carryFlag -= MATH_subtractValues(&(pWorkspace[n]), pWorkspace, &(pWorkspace[n]), n);
        else
            carryFlag += MATH_sumValues(&(pWorkspace[n]), &(pWorkspace[n]), pWorkspace, n);

        carryFlag += MATH_sumValues(&(pProduct[half_n]), &(pProduct[half_n]), &(pWorkspace[n]), n);

        if (0 != carryFlag)
        {
            /* add carry for D2 term */
            vlong_unit* pTemp = &(pProduct[half_n+n]);

            *pTemp = (*pTemp + carryFlag);

            /* handle carryFlag */
            if (*pTemp < (vlong_unit)carryFlag)
            {
                do
                {
                    pTemp++;
                    (*pTemp)++;
                }
                while (ZERO_UNIT == *pTemp);
            }
        }
    }

} /* karatsubaMultiply */


/*------------------------------------------------------------------*/

static MSTATUS
fasterUnsignedMultiplyVlongs(vlong *pProduct,
                             const vlong *pFactorA, const vlong *pFactorB,
                             ubyte4 numUnits)
{
    vlong_unit* pWorkspace = NULL;
    sbyte4  lengthA;
    sbyte4  lengthB;
    sbyte4  limit;
    sbyte4  diff;
    sbyte4  sizeofWorkspace;
    sbyte4  twoPowerX;
    MSTATUS status = OK;

    pProduct->numUnitsUsed = 0;

    lengthA = pFactorA->numUnitsUsed;
    lengthB = pFactorB->numUnitsUsed;

    if ((0 == lengthA) || (0 == lengthB))
    {
        status = VLONG_clearVlong(pProduct);
        goto exit;
    }

    limit = lengthA + lengthB;

    if (limit == numUnits)
    {
        if ((0 == (diff = lengthA-lengthB)) && (8 == lengthA))
        {
            if (pProduct->numUnitsAllocated < 16)
                if (OK > (status = expandVlong(pProduct, 16)))
                    goto exit;

            MATH_8x8(pProduct->pUnits,pFactorA->pUnits,pFactorB->pUnits);
            while (limit && (ZERO_UNIT == pProduct->pUnits[limit-1]))
               limit--;

            pProduct->numUnitsUsed = limit;

            goto exit;
        }

        if ((0 == diff) && (lengthA >= 16))
        {
            twoPowerX = 1 << (BITLENGTH((ubyte4)lengthA) - 1);
            sizeofWorkspace = twoPowerX + twoPowerX;

            if (lengthA == twoPowerX) /* lengthA is equal to 2^x */
            {
                if (NULL == (pWorkspace = MALLOC(sizeofWorkspace * 2 * sizeof(vlong_unit))))
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }

                if (pProduct->numUnitsAllocated < (ubyte4)(sizeofWorkspace))
                    if (OK > (status = expandVlong(pProduct, sizeofWorkspace)))
                        goto exit;

                karatsubaMultiply(pProduct->pUnits,
                                  pFactorA->pUnits, pFactorB->pUnits,
                                  pWorkspace, lengthA);

                while (limit && (ZERO_UNIT == pProduct->pUnits[limit-1]))
                    limit--;

                pProduct->numUnitsUsed = limit;

                goto exit;
            }
        }
    }

    status = fastUnsignedMultiplyVlongs(pProduct, pFactorA, pFactorB, numUnits);

exit:
    if (NULL != pWorkspace)
        FREE(pWorkspace);

    return(status);

} /* fasterUnsignedMultiplyVlongs */

static void
karatsubaSqr(vlong_unit *pProduct, const vlong_unit *pFactorA, vlong_unit *pWorkspace,
             sbyte4 n)
{
    sbyte4      half_n = n >> 1;
    intBoolean  zeroFlag;
    sbyte4      compA;
    sbyte4      i;

    if (8 == n)
    {
        MATH_SQR8(pProduct, pFactorA);
        return;
    }


    compA = MATH_compareValues(&(pFactorA[half_n]), pFactorA, half_n);

    zeroFlag = FALSE;

    switch (compA)
    {
    case -1:    /* negative times negative */
        MATH_subtractValues(pWorkspace, pFactorA, &(pFactorA[half_n]), half_n);
        for ( i = 0; i < half_n; ++i)
        {
            pWorkspace[half_n+i] = pWorkspace[i];
        }
        /*MOC_MEMCPY( (ubyte*) ( pWorkspace+half_n),
          (ubyte*) pWorkspace, half_n * sizeof(ubyte4)); */
        break;
    case 1:     /* positive times positive */
        MATH_subtractValues(pWorkspace, &(pFactorA[half_n]), pFactorA, half_n);
        for ( i = 0; i < half_n; ++i)
        {
            pWorkspace[half_n+i] = pWorkspace[i];
        }

        /*MOC_MEMCPY( (ubyte*) (pWorkspace+half_n),
          (ubyte*) pWorkspace, half_n * sizeof(ubyte4));*/
        break;
    default:    /* some combination of a zero times some x */
        zeroFlag = TRUE;
        break;
    }


    if (8 != half_n)
    {
        /* deal with the inner tree first to avoid an extra jump on leaf node case */
        if (!zeroFlag)
            karatsubaSqr(&(pWorkspace[n]), pWorkspace, &(pWorkspace[n<<1]), half_n);
        else
        {
            register int i;
            for (i = n-1; i >= 0; i--)
                pWorkspace[n + i] = 0;
        }

        karatsubaSqr(pProduct, pFactorA, &(pWorkspace[n<<1]), half_n);
        karatsubaSqr(&(pProduct[n]), &(pFactorA[half_n]), &(pWorkspace[n<<1]), half_n);
    }
    else
    {
        if (!zeroFlag)
            MATH_SQR8(&(pWorkspace[n]), pWorkspace);  /* D2 = A1-A0 * B1-B0 */
        else
        {
            register int i;                                                 /* D2 = A1-A0 * B1-B0 */
            for (i = 15; i >= 0; i--)
                pWorkspace[n + i] = 0;
        }

        MATH_SQR8(pProduct, pFactorA);                             /* D0 = A0 * B0 */
        MATH_SQR8(&(pProduct[n]), &(pFactorA[half_n])); /* D1 = A1 * B1 */
    }

    {
        sbyte4  carryFlag;

        carryFlag = MATH_sumValues(pWorkspace, pProduct, &(pProduct[n]), n);    /* D0 + D1 */
        carryFlag -= MATH_subtractValues(&(pWorkspace[n]), pWorkspace, &(pWorkspace[n]), n);
        carryFlag += MATH_sumValues(&(pProduct[half_n]), &(pProduct[half_n]), &(pWorkspace[n]), n);

        if (0 != carryFlag)
        {
            /* add carry for D2 term */
            vlong_unit* pTemp = &(pProduct[half_n+n]);

            *pTemp = (*pTemp + carryFlag);

            /* handle carryFlag */
            if (*pTemp < (vlong_unit)carryFlag)
            {
                do
                {
                    pTemp++;
                    (*pTemp)++;
                }
                while (ZERO_UNIT == *pTemp);
            }
        }
    }

} /* karatsubaSqr */


/*------------------------------------------------------------------*/

static MSTATUS
fasterUnsignedSqrVlong(vlong *pProduct, const vlong *pFactorA,
                       ubyte4 numUnits)
{
    vlong_unit* pWorkspace = NULL;
    sbyte4  lengthA;
    sbyte4  limit;
    sbyte4  sizeofWorkspace;
    sbyte4  twoPowerX;
    MSTATUS status = OK;

    pProduct->numUnitsUsed = 0;

    lengthA = pFactorA->numUnitsUsed;

    if (0 == lengthA)
    {
        status = VLONG_clearVlong(pProduct);
        goto exit;
    }

    limit = 2 * lengthA;

    if (limit == numUnits)
    {
        if ( (8 == lengthA))
        {
            if (pProduct->numUnitsAllocated < 16)
                if (OK > (status = expandVlong(pProduct, 16)))
                    goto exit;

            MATH_SQR8(pProduct->pUnits,pFactorA->pUnits);
            while (limit && (ZERO_UNIT == pProduct->pUnits[limit-1]))
               limit--;

            pProduct->numUnitsUsed = limit;
            goto exit;
        }

        if ( (lengthA >= 16))
        {
            twoPowerX = 1 << (BITLENGTH((ubyte4)lengthA) - 1);
            sizeofWorkspace = twoPowerX + twoPowerX;

            if (lengthA == twoPowerX) /* lengthA is equal to 2^x */
            {
                if (NULL == (pWorkspace = MALLOC(sizeofWorkspace * 2 * sizeof(vlong_unit))))
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }

                if (pProduct->numUnitsAllocated < (ubyte4)(sizeofWorkspace))
                    if (OK > (status = expandVlong(pProduct, sizeofWorkspace)))
                        goto exit;

                karatsubaSqr(pProduct->pUnits,
                                  pFactorA->pUnits,
                                  pWorkspace, lengthA);

                while (limit && (ZERO_UNIT == pProduct->pUnits[limit-1]))
                    limit--;

                pProduct->numUnitsUsed = limit;

                goto exit;
            }
        }
    }

    status = fastUnsignedSqrVlong(pProduct, pFactorA, numUnits);

exit:
    if (NULL != pWorkspace)
        FREE(pWorkspace);

    return(status);

} /* fasterUnsignedSqrVlongs */

#endif /* __DISABLE_MOCANA_KARATSUBA__ */


/*------------------------------------------------------------------*/

extern MSTATUS
VLONG_makeRandomVlong(void *pRandomContext, vlong **ppRetPrime, ubyte4 numBitsLong, vlong **ppVlongQueue)
{
    vlong*      pPrime = NULL;
    ubyte*      pBuffer = NULL;
    ubyte4      bufLen;
    ubyte4      shiftBits;
    MSTATUS     status;

    *ppRetPrime = NULL;

    bufLen = ((7 + numBitsLong) >> 3);

    if (NULL == (pBuffer = MALLOC(bufLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* generate a random number of numBitsLong length */
    if (OK > (status = RANDOM_numberGenerator(pRandomContext, pBuffer, bufLen)))
        goto exit;

    if (OK > (status = VLONG_vlongFromByteString(pBuffer, bufLen, &pPrime, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pPrime);

    /* deal with primes that are not multiples of 8 */
    shiftBits = numBitsLong - (numBitsLong & 0xfffff8);

    if (0 < shiftBits)
    {
        /* shrink prime candidate to non byte multiple length */
        if (OK > (status = VLONG_shrXvlong(pPrime, shiftBits)))
            goto exit;
    }

    /* set highest and lowest bits */
    if (OK > (status = VLONG_setVlongBit(pPrime, 0)))
        goto exit;

    if (OK > (status = VLONG_setVlongBit(pPrime, numBitsLong-1)))
        goto exit;

    /* result */
    *ppRetPrime = pPrime;
    pPrime = NULL;

exit:
    if (NULL != pPrime)
        VLONG_freeVlong(&pPrime, ppVlongQueue);

    if (NULL != pBuffer)
        FREE(pBuffer);

    return status;

} /* VLONG_makeRandomVlong */


