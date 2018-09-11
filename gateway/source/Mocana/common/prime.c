/* Version: mss_v6_3 */
/*
 * prime.c
 *
 * Prime Factory
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
#include "../common/random.h"
#include "../common/vlong.h"
#include "../common/lucas.h"
#include "../common/prime.h"
#include "../common/memory_debug.h"


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_PRIME_TEST__

static ubyte m_primeTable[] =
{   3,   5,   7,  11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,
   61,  67,  71,  73,  79,  83,  89,  97, 101, 103, 107, 109, 113, 127, 131, 137,
  139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
  229, 233, 239, 241, 251 };

#define SIZEOF_PRIME_TABLE  (sizeof(m_primeTable) / sizeof(ubyte))

#endif /* __DISABLE_MOCANA_PRIME_TEST__ */

/* based on the size of the prime, we determine the number of rounds necessary */
#ifndef NUM_PRIME_ATTEMPTS_GREATER_512
#define NUM_PRIME_ATTEMPTS_GREATER_512      8
#endif

#ifndef NUM_PRIME_ATTEMPTS_LESS_512
#define NUM_PRIME_ATTEMPTS_LESS_512         28
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_PRIME_TEST__

extern MSTATUS
PRIME_simplePrimeTest(MOC_MOD(hwAccelDescr hwAccelCtx) vlong *pPrime, ubyte4 startingIndex, ubyte4 endingIndex, intBoolean *pRetIsPrime, vlong **ppVlongQueue)
{
    ubyte4  index      = startingIndex;
    vlong*  pDivisor   = NULL;
    vlong*  pRemainder = NULL;
    MSTATUS status;

    *pRetIsPrime = FALSE;

    if (OK > (status = VLONG_makeVlongFromUnsignedValue((ubyte4)m_primeTable[0], &pDivisor, ppVlongQueue)))
        goto exit;

    while (index < endingIndex)
    {
        /* quickly, place prime number in divisor */
        pDivisor->pUnits[0] = (ubyte4)m_primeTable[index];

        VLONG_freeVlong(&pRemainder, ppVlongQueue);
        if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) pPrime, pDivisor, &pRemainder, ppVlongQueue)))
            goto exit;

        /* not prime, if remainder is zero */
        if (TRUE == VLONG_isVlongZero(pRemainder))
            goto exit;

        index++;
    }

    if (index == endingIndex)
        *pRetIsPrime = TRUE;

exit:
    VLONG_freeVlong(&pRemainder, ppVlongQueue);
    VLONG_freeVlong(&pDivisor, ppVlongQueue);

    return status;

} /* simplePrimeTest */

#endif /* __DISABLE_MOCANA_PRIME_TEST__ */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_PRIME_SIEVE__))

extern MSTATUS
PRIME_simpleDualPrimeTest(MOC_MOD(hwAccelDescr hwAccelCtx) vlong *pPrimeA, vlong *pPrimeB, ubyte4 startingIndex, ubyte4 endingIndex, intBoolean *pRetIsPrime, vlong **ppVlongQueue)
{
    ubyte4  index      = startingIndex;
    vlong*  pDivisor   = NULL;
    vlong*  pRemainder = NULL;
    MSTATUS status;

    *pRetIsPrime = FALSE;

    if (OK > (status = VLONG_makeVlongFromUnsignedValue((ubyte4)m_primeTable[0], &pDivisor, ppVlongQueue)))
        goto exit;

    while (index < endingIndex)
    {
        /* quickly, place prime number in divisor */
        pDivisor->pUnits[0] = (ubyte4)m_primeTable[index];

        VLONG_freeVlong(&pRemainder, ppVlongQueue);
        if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) pPrimeA, pDivisor, &pRemainder, ppVlongQueue)))
            goto exit;

        /* not prime, if remainder is zero */
        if (TRUE == VLONG_isVlongZero(pRemainder))
            goto exit;

        VLONG_freeVlong(&pRemainder, ppVlongQueue);
        if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) pPrimeB, pDivisor, &pRemainder, ppVlongQueue)))
            goto exit;

        /* not prime, if remainder is zero */
        if (TRUE == VLONG_isVlongZero(pRemainder))
            goto exit;

        index++;
    }

    if (index == endingIndex)
        *pRetIsPrime = TRUE;

exit:
    VLONG_freeVlong(&pRemainder, ppVlongQueue);
    VLONG_freeVlong(&pDivisor, ppVlongQueue);

    return status;

} /* simpleDualPrimeTest */

#endif /* (defined(__ENABLE_MOCANA_PRIME_SIEVE__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_PRIME_SIEVE__) || !defined(__DISABLE_MOCANA_PRIME_TEST__))
static MSTATUS
performRabinMillerTest(MOC_MOD(hwAccelDescr hwAccelCtx)
        ModExpHelper MEH, vlong *p, vlong *a, intBoolean *isGoodPrime,
        vlong **ppVlongQueue)
{
    vlong*  m   = NULL;
    vlong*  p_1 = NULL;  /* p-1 */
    vlong*  z   = NULL;
    vlong*  tmp = NULL;
    ubyte4  j, b;
    MSTATUS status;

    j = 0;
    b = 0;
    *isGoodPrime = FALSE;

    if (OK > (status = VLONG_allocVlong(&tmp, ppVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(tmp);

    /* p_1 = (p-1) */
    if (OK > (status = VLONG_makeVlongFromVlong(p, &p_1, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_decrement(p_1, ppVlongQueue)))
        goto exit;

    /* m = (p-1) */
    if (OK > (status = VLONG_makeVlongFromVlong(p_1, &m, ppVlongQueue)))
        goto exit;

    /* compute m, such that p = 1 + 2^b * m */
    while (0 == (m->pUnits[0] & 1))
    {
        /* divide by 2 until m is odd */
        VLONG_shrVlong(m);
        b++;
    }

    /* z = a^m mod p */
    if (OK > (status = VLONG_modExp(MOC_MOD(hwAccelCtx) MEH, a, m, &z, ppVlongQueue)))
        goto exit;

    /* if ((z=1) or (z=(p-1))), p is probably prime */
    if ((VLONG_compareUnsigned(z,1) == 0) || (VLONG_compareSignedVlongs(z, p_1) == 0))
    {
        *isGoodPrime = TRUE;
        goto exit;
    }

    for (j++; j < b; j++)
    {
        /* tmp = z^2 */
        if (OK > (status = VLONG_vlongSignedSquare(tmp, z)))
            goto exit;

        VLONG_freeVlong(&z, ppVlongQueue);

        /* z = z^2 mod p */
        if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) tmp, p, &z, ppVlongQueue)))
            goto exit;

        /* if z = p-1, p is probably prime */
        if (0 == VLONG_compareSignedVlongs(z, p_1))
        {
            *isGoodPrime = TRUE;
            goto exit;
        }

        /* if j > 0 and z = 1, p is not prime */
        if (VLONG_compareUnsigned(z,1) == 0)
            goto exit;
    }

exit:
    VLONG_freeVlong(&m, ppVlongQueue);
    VLONG_freeVlong(&p_1, ppVlongQueue);
    VLONG_freeVlong(&z, ppVlongQueue);
    VLONG_freeVlong(&tmp, ppVlongQueue);

    return status;

} /* performRabinMillerTest */

/*------------------------------------------------------------------*/

static ubyte4
PRIME_numRoundsRequired(ubyte4 bitLength, PrimeTestType type)
{
    ubyte4 numRounds = 50;          /* less than 100 bits, or no Lucas test */

    switch (type)
    {
        case prime_Legacy:
        {
            switch (bitLength >> 8)
            {
                case 0:
                {
                    if (100 <= bitLength)
                        numRounds = 27;     /* 100 - 255 bits */

                    break;
                }

                case 1:
                {
                    numRounds = 15;         /* 256 - 511 bits */
                    break;
                }

                case 2:
                {
                    numRounds = 8;          /* 512 - 767 bits*/
                    break;
                }

                case 3:
                {
                    numRounds = 4;          /* 768 - 1023 bits*/
                    break;
                }

                default:
                {
                    numRounds = 2;          /* 1024 bits or more */
                    break;
                }
            }
            break;
        }
        case prime_DSA:
        {
            switch (bitLength)
            {
                case 160:
                {
                    numRounds = 19;
                    break;
                }
                case 224:
                {
                    numRounds = 24;
                    break;
                }
                case 256:
                {
                    numRounds = 27;
                    break;
                }
                case 1024:
                {
                    numRounds = 3;
                    break;
                }
                case 2048:
                {
                    numRounds = 3;
                    break;
                }
                case 3048:
                {
                    numRounds = 2;
                    break;
                }
            }
            break;
        }
        case prime_RSA:
        {
            if(bitLength > 170)
            {
                numRounds = 41;
            }
            else if(bitLength > 140)
            {
                numRounds = 38;
            }
            else if(bitLength > 100)
            {
                numRounds = 28;
            }
            else if(bitLength == 512)
            {
                numRounds = 5;
            }
            else if(bitLength == 1024)
            {
                numRounds = 5;
            }
            else if(bitLength == 1536)
            {
                numRounds = 4;
            }
            break;
        }
    }

    return numRounds;

} /* PRIME_numRoundsRequired */

#endif /* (defined(__ENABLE_MOCANA_PRIME_SIEVE__) || !defined(__DISABLE_MOCANA_PRIME_TEST__)) */

/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_PRIME_TEST__

extern MSTATUS
PRIME_doPrimeTestsEx(MOC_MOD(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                   vlong *pPrime, PrimeTestType type, intBoolean *pIsPrime, vlong **ppVlongQueue)
{
    ModExpHelper    MEH = NULL;
    vlong*          a = NULL;
    sbyte4          index;
    ubyte4          numRounds;
    MSTATUS         status;

    if ((NULL == pPrime) || (NULL == pIsPrime))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == (pPrime->pUnits[0] & 1))
    {
        status = ERR_EVEN_NUMBER;
        goto exit;
    }

    *pIsPrime = FALSE;

    /* do a fast simple check */
    if ((OK > (status = PRIME_simplePrimeTest(MOC_MOD(hwAccelCtx) pPrime, 0, SIZEOF_PRIME_TABLE, pIsPrime, ppVlongQueue))) || (FALSE == *pIsPrime))
        goto exit;

    /* ANSI X9.80-2005: num of rounds required is dependent on size of the prime */
    numRounds = PRIME_numRoundsRequired(VLONG_bitLength(pPrime), type);

    if (OK > (status = VLONG_makeVlongFromUnsignedValue(1, &a, ppVlongQueue)))
        goto exit;

    /* set up montgomery exp helper based on p */
    if (OK > (status = VLONG_newModExpHelper(MOC_MOD(hwAccelCtx)
                                             &MEH, pPrime, ppVlongQueue)))
        goto exit;

    /* try random primes */
    for (index = numRounds; index > 0; index--)
    {
        VLONG_freeVlong(&a, ppVlongQueue);

        /* a must be between 2..(p-2) */
        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, &a, VLONG_bitLength(pPrime) - 1, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_addImmediate(a, 2, ppVlongQueue)))
            goto exit;

        if ((OK > (status = performRabinMillerTest(MOC_MOD(hwAccelCtx) MEH, pPrime, a, pIsPrime, ppVlongQueue))) ||
            (FALSE == *pIsPrime))
        {
            goto exit;
        }
    }
    if(type == prime_DSA || type == prime_Legacy)
    {
        status = LUCAS_primeTest(MOC_MOD(hwAccelCtx) pPrime, pIsPrime, ppVlongQueue);
    }

exit:
    VLONG_deleteModExpHelper(&MEH, ppVlongQueue);
    VLONG_freeVlong(&a, ppVlongQueue);

    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
PRIME_doPrimeTests(MOC_MOD(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                   vlong *pPrime, intBoolean *pIsPrime, vlong **ppVlongQueue)
{
    return PRIME_doPrimeTestsEx(MOC_MOD(hwAccelCtx) pRandomContext, pPrime, prime_Legacy, pIsPrime, ppVlongQueue);
}

#endif /* __DISABLE_MOCANA_PRIME_TEST__ */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_PRIME_SIEVE__))
extern MSTATUS
PRIME_doDualPrimeTests(MOC_MOD(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                       sbyte4 startingIndex, vlong *pPrimeA, vlong *pPrimeB, intBoolean *pIsBothPrime, vlong **ppVlongQueue)
{
    ModExpHelper    MEHa = NULL;
    ModExpHelper    MEHb = NULL;
    vlong*          a = NULL;
    sbyte4          index;
    ubyte4          numRounds;
    MSTATUS         status;

    if ((NULL == pPrimeA) || (NULL == pPrimeB) || (NULL == pIsBothPrime))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if ((0 == (pPrimeA->pUnits[0] & 1)) || (0 == (pPrimeB->pUnits[0] & 1)))
    {
        status = ERR_EVEN_NUMBER;
        goto exit;
    }

    *pIsBothPrime = FALSE;

    /* do a fast simple check */
    if ((OK > (status = PRIME_simpleDualPrimeTest(MOC_MOD(hwAccelCtx) pPrimeA, pPrimeB, startingIndex, SIZEOF_PRIME_TABLE, pIsBothPrime, ppVlongQueue))) || (FALSE == *pIsBothPrime))
        goto exit;

    /* ANSI X9.80-2005: num of rounds required is dependent on size of the prime */
    numRounds = PRIME_numRoundsRequired(VLONG_bitLength(pPrimeB));

    if (OK > (status = VLONG_makeVlongFromUnsignedValue(1, &a, ppVlongQueue)))
        goto exit;

    /* set up montgomery exp helper based on p */
    if (OK > (status = VLONG_newModExpHelper(MOC_MOD(hwAccelCtx)
                                             &MEHa, pPrimeA, ppVlongQueue)))
        goto exit;

    /* set up montgomery exp helper based on p */
    if (OK > (status = VLONG_newModExpHelper(MOC_MOD(hwAccelCtx)
                                             &MEHb, pPrimeB, ppVlongQueue)))
        goto exit;

    /* try random primes */
    for (index = numRounds; index > 0; index--)
    {
        VLONG_freeVlong(&a, ppVlongQueue);

        /* a must be between 2..(p-2) */
        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, &a, VLONG_bitLength(pPrimeA) - 1, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_addImmediate(a, 2, ppVlongQueue)))
            goto exit;

        if ((OK > (status = performRabinMillerTest(MOC_MOD(hwAccelCtx) MEHa, pPrimeA, a, pIsBothPrime, ppVlongQueue))) ||
            (FALSE == *pIsBothPrime))
        {
            goto exit;
        }

        VLONG_freeVlong(&a, ppVlongQueue);

        /* a must be between 2..(p-2) */
        if (OK > (status = VLONG_makeRandomVlong(pRandomContext, &a, VLONG_bitLength(pPrimeB) - 1, ppVlongQueue)))
            goto exit;

        if (OK > (status = VLONG_addImmediate(a, 2, ppVlongQueue)))
            goto exit;

        if ((OK > (status = performRabinMillerTest(MOC_MOD(hwAccelCtx) MEHb, pPrimeB, a, pIsBothPrime, ppVlongQueue))) ||
            (FALSE == *pIsBothPrime))
        {
            goto exit;
        }
    }

    if ((OK > (status = LUCAS_primeTest(MOC_MOD(hwAccelCtx) pPrimeA, pIsBothPrime, ppVlongQueue))) || (FALSE == *pIsBothPrime))
        goto exit;

    status = LUCAS_primeTest(MOC_MOD(hwAccelCtx) pPrimeB, pIsBothPrime, ppVlongQueue);

exit:
    VLONG_deleteModExpHelper(&MEHb, ppVlongQueue);
    VLONG_deleteModExpHelper(&MEHa, ppVlongQueue);
    VLONG_freeVlong(&a, ppVlongQueue);

    return status;

} /* PRIME_doDualPrimeTests */

#endif /* (defined(__ENABLE_MOCANA_PRIME_SIEVE__)) */


/*------------------------------------------------------------------*/

#if (!defined(__PRIME_GEN_HARDWARE__) && (!defined(__DISABLE_MOCANA_PRIME_TEST__)))

extern MSTATUS
PRIME_generateSizedPrime(MOC_PRIME(hwAccelDescr hwAccelCtx) randomContext *pRandomContext,
                         vlong **ppRetPrime, ubyte4 numBitsLong, vlong **ppVlongQueue)
{
    vlong*      pPrime     = NULL;
    vlong*      pDivisor   = NULL;
    vlong*      pRemainder = NULL;
    intBoolean  isPrime    = FALSE;
    MSTATUS     status;

    *ppRetPrime = NULL;

    if (OK > (status = VLONG_makeRandomVlong(pRandomContext, &pPrime, numBitsLong, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_makeVlongFromUnsignedValue(3, &pDivisor, ppVlongQueue)))
        goto exit;

    if (OK > (status = VLONG_operatorModSignedVlongs(MOC_MOD(hwAccelCtx) pPrime, pDivisor, &pRemainder, ppVlongQueue)))
        goto exit;

    /* prime is now a multiple of 3 */
    /* 3, 6, 9, 12, 15, 18 */
    if (OK > (status = VLONG_subtractSignedVlongs(pPrime, pRemainder, ppVlongQueue)))
        goto exit;

    /* move to next odd value */
    /* 5, 11, 17, 23 */
    if (FALSE == VLONG_isVlongBitSet(pPrime, 0))
    {
        if (OK > (status = VLONG_addImmediate(pPrime, 3, ppVlongQueue)))
            goto exit;
    }

    /* move to 5 */
    if (OK > (status = VLONG_addImmediate(pPrime, 2, ppVlongQueue)))
        goto exit;

    /* 3, 5, 7, 9, 11, 13, 15 */
    while (1)
    {
        /* test first value */
        /* test 5 */
        if (OK > (status = PRIME_doPrimeTests(MOC_MOD(hwAccelCtx) pRandomContext, pPrime, &isPrime, ppVlongQueue)))
            goto exit;

        if (TRUE == isPrime)
            break;

        /* move to next odd number */
        /* move to 7 */
        if (OK > (status = VLONG_addImmediate(pPrime, 2, ppVlongQueue)))
            goto exit;

        /* do next prime test */
        /* test 7 */
        if (OK > (status = PRIME_doPrimeTests(MOC_MOD(hwAccelCtx) pRandomContext, pPrime, &isPrime, ppVlongQueue)))
            goto exit;

        if (TRUE == isPrime)
            break;

        /* skip next odd number */
        /* skip 9, move to 11 */
        if (OK > (status = VLONG_addImmediate(pPrime, 4, ppVlongQueue)))
            goto exit;
    }

    *ppRetPrime = pPrime; pPrime = NULL;

exit:
    VLONG_freeVlong(&pRemainder, ppVlongQueue);
    VLONG_freeVlong(&pDivisor, ppVlongQueue);
    VLONG_freeVlong(&pPrime, ppVlongQueue);
    VLONG_freeVlongQueue(ppVlongQueue);

    return status;
}

#endif /* __PRIME_GEN_HARDWARE__ */

