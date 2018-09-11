/* Version: mss_v6_3 */
/*
 * rng_seed.c
 *
 * A random number generator seed
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/rng_seed.h"
#include "../crypto/sha1.h"
#include "../harness/harness.h"

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif

#ifdef __ENABLE_MOCANA_VALGRIND_SUPPORT__
#include <stdio.h>
#endif


#ifndef __DISABLE_MOCANA_RNG__
/*------------------------------------------------------------------*/
#define MOCANA_RAND_ENTROPY3_KILLWAIT_TIME   700
#define MOCANA_RAND_ENTROPY2_KILLWAIT_TIME   300
#define MOCANA_RAND_ENTROPY1_KILLWAIT_TIME   100

#if (!defined(RNG_SEED_BUFFER_SIZE))
#define RNG_SEED_BUFFER_SIZE        (64)
#endif

#if (!defined(RNG_SEED_ROUNDS))
#define RNG_SEED_ROUNDS             (8)
#endif

#define RNG_SEED_NUM_SHA1_ROUNDS    ((RNG_SEED_BUFFER_SIZE + (SHA1_RESULT_SIZE - 1)) / SHA1_RESULT_SIZE)


#if defined(__ENABLE_MOCANA_RNG_SEED_STATE_DEBUG__) || defined(__ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__)

#ifdef __KERNEL__
#define PRINTDBG printk
#else
#include <stdio.h>
#define PRINTDBG printf
#endif

#endif

/*------------------------------------------------------------------*/

enum
{
    kEntropyInit = 0,
    kEntropyStart,
    kEntropyWorking,
    kEntropyDone,
    kEntropyIdle

} entropyThreadState;


/*------------------------------------------------------------------*/

#if (defined(_DEBUG))
/* when in debug mode we disable this to prevent normally helpful debugger warnings */
#define RNG_SEED_DEBUG_RESET(X)     X = 0
#else
#define RNG_SEED_DEBUG_RESET(X)     X = X
#endif

/* Our collection of entropy - must be volatile to prevent */
/* compiler optimizations that may reduce our entropy collecting */
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
static volatile ubyte               m_entropyByteDepotHistory[RNG_SEED_BUFFER_SIZE + SHA1_RESULT_SIZE];
#endif

static volatile ubyte               m_entropyByteDepot[RNG_SEED_BUFFER_SIZE + SHA1_RESULT_SIZE];
static volatile ubyte               m_entropyScratch[RNG_SEED_BUFFER_SIZE + SHA1_RESULT_SIZE];
static volatile ubyte4              m_indexEntropyByteDepot = 0;
static volatile ubyte4              m_indexEntropyBitIn = 0;

#define BITINMODVAL (8 * RNG_SEED_BUFFER_SIZE)

/* for entropy thread ipc mechanism */
static volatile sbyte4              mEntropyThreadState1;
static volatile sbyte4              mEntropyThreadState2;
static volatile sbyte4              mEntropyThreadState3;
static volatile sbyte4              mEntropyThreadsState;

static volatile intBoolean          mShouldEntropyThreadsDie  = FALSE;

static intBoolean                   mIsRngSeedInit            = FALSE;
static intBoolean                   mIsRngSeedThreadInit      = FALSE;
static RTOS_MUTEX                   mRngSeedMutex;
static RTOS_MUTEX                   mRngSeedThreadMutex;


/*------------------------------------------------------------------*/

static RTOS_THREAD ethread01;
static RTOS_THREAD ethread02;
static RTOS_THREAD ethread03;

static MSTATUS RNG_SEED_createInitialState(void);

/*------------------------------------------------------------------*/

static void
RNG_SEED_scramble(void)
{
    sbyte4  i, j, k, l;

    for (i = 0; i < RNG_SEED_NUM_SHA1_ROUNDS; ++i)
    {
        ubyte* w = (ubyte *)&(m_entropyScratch[i * SHA1_RESULT_SIZE]);

        SHA1_G((ubyte *)m_entropyScratch, w);

        k = RNG_SEED_NUM_SHA1_ROUNDS;
        j = (k - (i + 1));
        l = j * SHA1_RESULT_SIZE;
    }
    (void)l;   /* variable is set but not used */
}


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static MSTATUS
RNG_SEED_entropyThreadWaitForStart(ubyte4 threadCount,
                                   ubyte value)
{
    ubyte4      RNG_SEED_DEBUG_RESET(index);      /* don't initialize var */
    intBoolean  isMutexSet = FALSE;
    MSTATUS     status = OK;

    do
    {
        if (FALSE != mShouldEntropyThreadsDie)
        {
            status = ERR_RAND_TERMINATE_THREADS;
            goto exit;
        }

        /* while we are waiting for everyone to be ready, let's keep scrambling the scratch buffer */
        index = ((1 + index) % RNG_SEED_BUFFER_SIZE);
        m_entropyScratch[index] ^= value;
        RNG_SEED_scramble();

        RTOS_sleepMS(50);

        /* make sure everyone is sync'd up to ready state before moving to next state */
        MRTOS_mutexWait(mRngSeedThreadMutex, &isMutexSet);


#ifdef __ENABLE_MOCANA_RNG_SEED_STATE_DEBUG__
        PRINTDBG("RNG_SEED_entropyThreadWaitForStart [%d]: %d, %d, %d, %d\n", threadCount, mEntropyThreadState1, mEntropyThreadState2, mEntropyThreadState3, mEntropyThreadsState);
#endif

        if ((kEntropyWorking != mEntropyThreadsState) &&
            (kEntropyStart   == mEntropyThreadState1)  &&
            (kEntropyStart   == mEntropyThreadState2)  &&
            (kEntropyStart   == mEntropyThreadState3))
        {
            mEntropyThreadsState = kEntropyWorking;
        }

        MRTOS_mutexRelease(mRngSeedThreadMutex, &isMutexSet);
    }
    while (kEntropyWorking != mEntropyThreadsState);

exit:
    MRTOS_mutexRelease(mRngSeedThreadMutex, &isMutexSet);

    return status;
}
#endif /* __DISABLE_MOCANA_RAND_ENTROPY_THREADS__ */


/*------------------------------------------------------------------*/

static void RNG_SEED_entropyMoveScratchToDepot(void)
{
    /* xor copy out the newly generated scratch data into the depot */
    MOC_XORCPY((void *)m_entropyByteDepot, (void *)m_entropyScratch, RNG_SEED_BUFFER_SIZE);

    /* scramble previous seed to prevent eaves droppers */
    RNG_SEED_scramble();

    /* indicate number of bytes available in the depot */
    m_indexEntropyByteDepot = 0;

}

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static MSTATUS
RNG_SEED_entropyThreadWaitForDone(ubyte4 threadCount,
                                  ubyte value)
{
    ubyte4      RNG_SEED_DEBUG_RESET(index);      /* don't initialize var */
    intBoolean  isMutexSet = FALSE;
    MSTATUS     status = OK;

    do
    {
        if (FALSE != mShouldEntropyThreadsDie)
        {
            status = ERR_RAND_TERMINATE_THREADS;
            goto exit;
        }

        /* while we are waiting for everyone to be done, let's keep scrambling the scratch buffer */
        index = ((1 + index) % RNG_SEED_BUFFER_SIZE);
        m_entropyScratch[index] ^= value;
        RNG_SEED_scramble();

        RTOS_sleepMS(50);

        MRTOS_mutexWait(mRngSeedThreadMutex, &isMutexSet);

#ifdef __ENABLE_MOCANA_RNG_SEED_STATE_DEBUG__
        PRINTDBG("RNG_SEED_entropyThreadWaitForDone [%d]: %d, %d, %d, %d\n", threadCount, mEntropyThreadState1, mEntropyThreadState2, mEntropyThreadState3, mEntropyThreadsState);
#endif

        if ((kEntropyIdle != mEntropyThreadsState) &&
            (kEntropyDone == mEntropyThreadState1) &&
            (kEntropyDone == mEntropyThreadState2) &&
            (kEntropyDone == mEntropyThreadState3))
        {
            RNG_SEED_entropyMoveScratchToDepot();

            mEntropyThreadsState = kEntropyIdle;
        }

        MRTOS_mutexRelease(mRngSeedThreadMutex, &isMutexSet);
    }
    while (kEntropyIdle != mEntropyThreadsState);

exit:
    MRTOS_mutexRelease(mRngSeedThreadMutex, &isMutexSet);

    return status;
}
#endif /* __DISABLE_MOCANA_RAND_ENTROPY_THREADS__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void
RNG_SEED_entropyThread1(void* context)
{
    MSTATUS status;

    do
    {
        moctime_t       startTime;
        sbyte4          i;
        sbyte4          j;

        mEntropyThreadState1 = kEntropyStart;

        if (OK > (status = RNG_SEED_entropyThreadWaitForStart(1, 0x11)))
            goto exit;

        mEntropyThreadState1 = kEntropyWorking;

        for (i = 0; i < RNG_SEED_ROUNDS; i++)
        {
            RTOS_deltaMS(NULL, &startTime);

            for (j = 0; j < RNG_SEED_BUFFER_SIZE; j++)
            {
                if (FALSE != mShouldEntropyThreadsDie)
                    goto exit;

                m_entropyScratch[i] ^= 0x10;
                RTOS_sleepMS(((RTOS_deltaMS(&startTime, NULL) >> 1) & 0x3) + 13);
            }
        }

        mEntropyThreadState1 = kEntropyDone;

        RTOS_deltaMS(NULL, &startTime);

        /* keep running until thread 3 is done */
        while (kEntropyDone != mEntropyThreadState3)
        {
            if (FALSE != mShouldEntropyThreadsDie)
                goto exit;

            for (i = 0; ((RNG_SEED_BUFFER_SIZE > i) && (kEntropyDone != mEntropyThreadState3)); i++)
            {
                if (FALSE != mShouldEntropyThreadsDie)
                    goto exit;
                m_entropyScratch[i] ^= 0x10;
                RTOS_sleepMS(((RTOS_deltaMS(&startTime, NULL) >> 1) & 0x3) + 13);
            }
        }

        if (OK > (status = RNG_SEED_entropyThreadWaitForDone(1, 0x90)))
            goto exit;

        mEntropyThreadState1 = kEntropyIdle;

        while ((FALSE == mShouldEntropyThreadsDie) && (kEntropyIdle == mEntropyThreadState1))
            RTOS_sleepMS(MOCANA_RAND_ENTROPY1_KILLWAIT_TIME);
    }
    while (FALSE == mShouldEntropyThreadsDie);

exit:
    mShouldEntropyThreadsDie = TRUE;

    return;

} /* RNG_SEED_entropyThread1 */
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void
RNG_SEED_entropyThread2(void* context)
{
    MSTATUS status;

    do
    {
        moctime_t       startTime;
        sbyte4          i;
        sbyte4          j;

        mEntropyThreadState2 = kEntropyStart;

        if (OK > (status = RNG_SEED_entropyThreadWaitForStart(2, 0x22)))
            goto exit;

        mEntropyThreadState2 = kEntropyWorking;

        for (i = 0; i < RNG_SEED_ROUNDS; i++)
        {
            RTOS_deltaMS(NULL, &startTime);

            for (j = RNG_SEED_BUFFER_SIZE - 1; j >= 0; j--)
            {
                ubyte4 newval;

                if (FALSE != mShouldEntropyThreadsDie)
                    goto exit;

                newval = m_entropyScratch[j];
                newval = (newval ^ (newval >> 2) ^ (newval >> 5) ^ (newval * 13) ^ (newval * 37) ^ (newval * 57)) & 0xff;
                m_entropyScratch[j] = (ubyte)newval;

                RTOS_sleepMS(((RTOS_deltaMS(&startTime, NULL) >> 1) & 0x3) + 7);
            }
        }

        mEntropyThreadState2 = kEntropyDone;

        RTOS_deltaMS(NULL, &startTime);

        /* keep running until thread 3 is done */
        while (kEntropyDone != mEntropyThreadState3)
        {
            for (i = RNG_SEED_BUFFER_SIZE - 1; ((i >= 0) && (kEntropyDone != mEntropyThreadState3)); i--)
            {
                ubyte4 newval;

                if (FALSE != mShouldEntropyThreadsDie)
                    goto exit;

                newval = m_entropyScratch[i];
                newval = (newval ^ (newval >> 2) ^ (newval >> 5) ^ (newval * 13) ^ (newval * 37) ^ (newval * 57)) & 0xff;
                m_entropyScratch[i] = (ubyte)newval;

                RTOS_sleepMS(((RTOS_deltaMS(&startTime, NULL) >> 1) & 0x3) + 7);
            }
        }

        if (OK > (status = RNG_SEED_entropyThreadWaitForDone(2, 0xa2)))
            goto exit;

        mEntropyThreadState2 = kEntropyIdle;

        while ((FALSE == mShouldEntropyThreadsDie) && (kEntropyIdle == mEntropyThreadState2))
            RTOS_sleepMS(MOCANA_RAND_ENTROPY2_KILLWAIT_TIME);
    }
    while (FALSE == mShouldEntropyThreadsDie);

exit:
    mShouldEntropyThreadsDie = TRUE;

    return;
}
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void
RNG_SEED_entropyThread3(void* context)
{
    MSTATUS status;

    do
    {
        ubyte4          i;
        ubyte4          j;
        moctime_t       startTime;

        mEntropyThreadState3 = kEntropyStart;

        if (OK > (status = RNG_SEED_entropyThreadWaitForStart(3, 0x33)))
            goto exit;

        mEntropyThreadState3 = kEntropyWorking;

        for (i = 0; i < RNG_SEED_ROUNDS; i++)
        {
            for (j = 0; j < RNG_SEED_BUFFER_SIZE; j += 7)
            {
                if (FALSE != mShouldEntropyThreadsDie)
                    goto exit;

                RTOS_deltaMS(NULL, &startTime);

                while (RTOS_deltaMS(&startTime, NULL) < ((j + 1) * 7))
                    RNG_SEED_scramble();

                RTOS_sleepMS(((RTOS_deltaMS(&startTime, NULL) >> 1) & 0x3) + 3);
            }
        }

        mEntropyThreadState3 = kEntropyDone;

        if (OK > (status = RNG_SEED_entropyThreadWaitForDone(3, 0x3b)))
            goto exit;

        mEntropyThreadState3 = kEntropyIdle;

        while ((FALSE == mShouldEntropyThreadsDie) && (kEntropyIdle == mEntropyThreadState3))
            RTOS_sleepMS(MOCANA_RAND_ENTROPY3_KILLWAIT_TIME);
    }
    while (FALSE == mShouldEntropyThreadsDie);

exit:
    mShouldEntropyThreadsDie = TRUE;

    return;
}
#endif


/*------------------------------------------------------------------*/

static void
RNG_SEED_simpleSeedInit(void)
{
    ubyte4                  upTime, i;

#ifdef __ENABLE_MOCANA_VALGRIND_SUPPORT__
    /*
     * Note: valgrind complains when timeSeed isn't initialized.
     * (it must not be filled in completely from RTOS_timeGMT?)
     */
    TimeDate                timeSeed = { 0 };

    /*
     * If we're trying to use valgrind, use /dev/urandom instead of garbage.
     * Otherwise valgrind will report lots of uninitialized values originating
     * from this funciton.
     */
    ubyte                   garbage[2*RNG_SEED_BUFFER_SIZE];

    FILE* devRandom = fopen("/dev/urandom", "r");
    fread(garbage, sizeof(garbage), 1, devRandom);
    fclose(devRandom);
#else
    TimeDate                timeSeed;
    ubyte                   garbage[2*RNG_SEED_BUFFER_SIZE];
#endif

    upTime = RTOS_getUpTimeInMS();

    for (i = 0; i < sizeof(upTime); i++)
        m_entropyScratch[i] ^= ((ubyte *)(&upTime))[i];

    RTOS_timeGMT(&timeSeed);

    for (i = 1; i < (1 + sizeof(timeSeed)); i++)
        m_entropyScratch[RNG_SEED_BUFFER_SIZE - i] ^= ((ubyte *)(&timeSeed))[i - 1];

    for (i = 0; i < RNG_SEED_BUFFER_SIZE; i++)
        m_entropyScratch[(i + sizeof(upTime)) % RNG_SEED_BUFFER_SIZE] ^= (ubyte)(0x67 + i);

    for (i = 0; i < 2 * RNG_SEED_BUFFER_SIZE; i++)
        m_entropyScratch[i % RNG_SEED_BUFFER_SIZE] ^= garbage[i];

    RNG_SEED_scramble();
}

/*------------------------------------------------------------------*/

#if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__)))
static MSTATUS
RNG_SEED_entropyThreadLauncher(void)
{
    moctime_t   startTime;
    MSTATUS     status = OK;

    RTOS_deltaMS(NULL, &startTime);

    /* leverages preemptive RTOS */
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** creating entropy threads 00\n");
#endif

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** creating entropy thread 01\n");
#endif

    if (OK > (status = RTOS_createThread(RNG_SEED_entropyThread1, (void *)NULL, (sbyte4)ENTROPY_THREAD, &ethread01)))
        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** creating entropy thread 02\n");
#endif

    if (OK > (status = RTOS_createThread(RNG_SEED_entropyThread2, (void *)NULL, (sbyte4)ENTROPY_THREAD, &ethread02)))
        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** creating entropy thread 03\n");
#endif

    if (OK > (status = RTOS_createThread(RNG_SEED_entropyThread3, (void *)NULL, (sbyte4)ENTROPY_THREAD, &ethread03)))
        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** entropy threads created \n");
#endif

    while ((kEntropyDone <= mEntropyThreadState1) || (kEntropyDone <= mEntropyThreadState2) || (kEntropyDone <= mEntropyThreadState3))
    {
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
        PRINTDBG("*** ethreads running\n");
#endif
        RNG_SEED_scramble();
        RTOS_sleepMS(((RTOS_deltaMS(&startTime, NULL) >> 1) & 0xFF) + 1);
    }

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** entropy threads init done\n");
#endif

exit:
    return status;

} /* RNG_SEED_entropyThreadLauncher */

#endif /* #if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__))) */

/*------------------------------------------------------------------*/
#ifdef __FIPS_OPS_TEST__
static int seed_fail = 0;

MOC_EXTERN void triggerSeedFail(void)
{
    seed_fail = 1;
}
MOC_EXTERN void resetSeedFail(void)
{
    seed_fail = 0;
}
#endif


#ifdef __ENABLE_MOCANA_FIPS_MODULE__
static
MSTATUS RNG_SEED_fipsConditionalTest(ubyte *pGeneratedBytes, ubyte4 numEntropyBytes)
{
    MSTATUS status = OK;
    sbyte4 cmp = 0;

    if ( numEntropyBytes > sizeof(m_entropyByteDepotHistory) )
    {
        status = ERR_FIPS_RNG_FAIL;
        goto exit;
    }

#ifdef __FIPS_OPS_TEST__
    if ( 1 == seed_fail )
    {
        MOC_MEMCPY((void *)m_entropyByteDepotHistory, (void *)pGeneratedBytes, numEntropyBytes);
    }
#endif

    /* New Seed must not be the same compare to the previous one -- FIPS */
    status =  MOC_MEMCMP((const ubyte *)m_entropyByteDepotHistory, (const ubyte *)pGeneratedBytes, numEntropyBytes, &cmp);

    if ( ( OK > status ) || ( 0 == cmp )  )
    {
        status = ERR_FIPS_RNG_FAIL;
    }
    else
    {
        /* Copy the current Seed output to history for future comparision */
        MOC_MEMCPY((void *)m_entropyByteDepotHistory, (void *)pGeneratedBytes, numEntropyBytes);
    }

exit:
    return status;
}

#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

/*------------------------------------------------------------------*/

extern MSTATUS
RNG_SEED_extractInitialDepotBits(ubyte *pDstCloneEntropyBytes, ubyte4 numEntropyBytes)
{
    intBoolean      isExtractMutexSet = FALSE;
    ubyte4          numBytesToClone;
    ubyte4          index;
    moctime_t       startTime;
    MSTATUS         status;
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    ubyte4 			numEntropyBytesRequested = numEntropyBytes;
#endif
    ubyte 			*pTempDestCloneEntropyBytes = pDstCloneEntropyBytes;

    RTOS_deltaMS(NULL, &startTime);

    if (FALSE == mIsRngSeedInit)
    {
        if (OK > (status = RNG_SEED_createInitialState()))
            return status;
    }

    /* only allow one consumer at a time... */
    if (OK > (status = MRTOS_mutexWait(mRngSeedMutex, &isExtractMutexSet)))
        goto exit;

    RNG_SEED_simpleSeedInit();
    RNG_SEED_entropyMoveScratchToDepot();
    RNG_SEED_scramble();

    do
    {
        /* calculate number bytes available to clone */
        index = m_indexEntropyByteDepot;
        numBytesToClone = ((RNG_SEED_BUFFER_SIZE - index) > numEntropyBytes) ? numEntropyBytes : (RNG_SEED_BUFFER_SIZE - index);

        /* if no bytes are available for cloning... */
        if (0 == numBytesToClone)
        {
#ifndef __DISABLE_MOCANA_RAND_SEED__
            RNG_SEED_simpleSeedInit(); /* Do it again. */
            RNG_SEED_entropyMoveScratchToDepot();
#else /* ifndef __DISABLE_MOCANA_RAND_SEED__ */
            /* only useful for benchmarking / optimizing key generation */
            m_indexEntropyByteDepot = 0; /* It must be full again */
#endif /* ifndef __DISABLE_MOCANA_RAND_SEED__ */
            /* do over... */
            continue;
        }

        /* copy entropy bits out */
        MOC_MEMCPY(pTempDestCloneEntropyBytes, (void *)(index + m_entropyByteDepot), numBytesToClone);
        pTempDestCloneEntropyBytes += numBytesToClone;

        /* zeroize remove seed bytes */
        MOC_MEMSET((void *)(index + m_entropyByteDepot), 0x00, numBytesToClone);

        /* just in case we straddle buffers, etc */
        numEntropyBytes -= numBytesToClone;
        m_indexEntropyByteDepot += numBytesToClone;
    }
    while (0 != numEntropyBytes);

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if ( OK > ( status = RNG_SEED_fipsConditionalTest(pDstCloneEntropyBytes, numEntropyBytesRequested) ) )
    {
    	setFIPS_Status(FIPS_ALGO_RNG,status);
    	goto exit;
    }
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    status = OK;

exit:
	MRTOS_mutexRelease(mRngSeedMutex, &isExtractMutexSet);

	return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
RNG_SEED_extractDepotBits(ubyte *pDstCloneEntropyBytes, ubyte4 numEntropyBytes)
{
    intBoolean      isFirstTime = FALSE;
#if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__)))
    intBoolean      isExtractMutexSet = FALSE;
    ubyte4          numBytesToClone;
    ubyte4          index;
#endif
    moctime_t       startTime;
    MSTATUS         status;
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    ubyte4 			numEntropyBytesRequested = numEntropyBytes;
#endif
#if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__)))
    ubyte 			*pTempDestCloneEntropyBytes = pDstCloneEntropyBytes;
#endif

    RTOS_deltaMS(NULL, &startTime);

    if (TRUE == mShouldEntropyThreadsDie)
    {
        return ERR_RAND_TERMINATE_THREADS;
    }

    if (FALSE == mIsRngSeedInit)
    {
        if (OK > (status = RNG_SEED_createInitialState()))
            return status;
        else
            isFirstTime = TRUE; /* This really is the first time */
    }

    if (FALSE == mIsRngSeedThreadInit)
    {
        mEntropyThreadState1 = kEntropyInit;
        mEntropyThreadState2 = kEntropyInit;
        mEntropyThreadState3 = kEntropyInit;
        mEntropyThreadsState = kEntropyInit;

        isFirstTime = TRUE; /* Consider this the first time too */

    }

#if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__)))

    /* only allow one consumer at a time... */
    if (OK > (status = MRTOS_mutexWait(mRngSeedMutex, &isExtractMutexSet)))
        goto exit;

    /* the first time we are called, we need to spawn the entropy threads */
    if (TRUE == isFirstTime)
    {
        mEntropyThreadsState = kEntropyStart;

        if (OK > (status = RNG_SEED_entropyThreadLauncher()))
            goto exit;
        mIsRngSeedThreadInit = TRUE;

    }

    do
    {
        if (TRUE == mShouldEntropyThreadsDie)
        {
            status = ERR_RAND_TERMINATE_THREADS;
            goto exit;
        }

        /* calculate number bytes available to clone */
        index = m_indexEntropyByteDepot;
        numBytesToClone = ((RNG_SEED_BUFFER_SIZE - index) > numEntropyBytes) ? numEntropyBytes : (RNG_SEED_BUFFER_SIZE - index);

        /* if no bytes are available for cloning... */
        if (0 == numBytesToClone)
        {
            /* if the threads are not already working on new bits...  */
            if ((kEntropyIdle == mEntropyThreadsState) &&
                (kEntropyIdle == mEntropyThreadState1) &&
                (kEntropyIdle == mEntropyThreadState2) &&
                (kEntropyIdle == mEntropyThreadState3))
            {
                /* have them run! */
                mEntropyThreadsState = kEntropyStart;
                mEntropyThreadState1 = kEntropyStart;
                mEntropyThreadState2 = kEntropyStart;
                mEntropyThreadState3 = kEntropyStart;
            }
            else if (kEntropyIdle != mEntropyThreadsState)
            {
                /* otherwise, scramble while we wait until a new batch is available */
                RNG_SEED_scramble();
                RTOS_sleepMS(((RTOS_deltaMS(&startTime, NULL) >> 1) & 0xFF) + 1);
            }

            /* do over... */
            continue;
        }

        /* copy entropy bits out */
        MOC_MEMCPY(pTempDestCloneEntropyBytes, (void *)(index + m_entropyByteDepot), numBytesToClone);
        pTempDestCloneEntropyBytes += numBytesToClone;

        /* zeroize remove seed bytes */
        MOC_MEMSET((void *)(index + m_entropyByteDepot), 0x00, numBytesToClone);

        /* just in case we straddle buffers, etc */
        numEntropyBytes -= numBytesToClone;
        m_indexEntropyByteDepot += numBytesToClone;
    }
    while (0 != numEntropyBytes);

    /* kick off threads again, if they are idle */
    if ((kEntropyIdle == mEntropyThreadsState) &&
        (kEntropyIdle == mEntropyThreadState1) &&
        (kEntropyIdle == mEntropyThreadState2) &&
        (kEntropyIdle == mEntropyThreadState3))
    {
        /* have them run! */
        mEntropyThreadsState = kEntropyStart;
        mEntropyThreadState1 = kEntropyStart;
        mEntropyThreadState2 = kEntropyStart;
        mEntropyThreadState3 = kEntropyStart;
    }

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if ( OK > ( status = RNG_SEED_fipsConditionalTest(pDstCloneEntropyBytes, numEntropyBytesRequested) ) )
    {
    	setFIPS_Status(FIPS_ALGO_RNG,status);
    	goto exit;
    }
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    status = OK;

exit:
	MRTOS_mutexRelease(mRngSeedMutex, &isExtractMutexSet);

    return status;

#else /* #if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__))) */
    /* the first time we are called, we need to do a simple seed */
    if (TRUE == isFirstTime)
    {
        /* Needed so that next time we don't think it's still the first time */
        mIsRngSeedThreadInit = TRUE; /* Even though there really are no threads */
    }

    status = RNG_SEED_extractInitialDepotBits(pDstCloneEntropyBytes, numEntropyBytes);

	return status;

#endif /* #if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__))) */

}

/*------------------------------------------------------------------*/
#ifndef __DISABLE_MOCANA_ADD_ENTROPY__

#define BITMODVAL (8 * RNG_SEED_BUFFER_SIZE);

extern MSTATUS RNG_SEED_addEntropyBit(ubyte entropyBit)
{
    intBoolean      isExtractMutexSet = FALSE;
    moctime_t       startTime;
    ubyte4          bitPos;
    MSTATUS         status;

    RTOS_deltaMS(NULL, &startTime);

    if (TRUE == mShouldEntropyThreadsDie)
    {
        return ERR_RAND_TERMINATE_THREADS;
    }

    if (FALSE == mIsRngSeedInit)
    {
        if (OK > (status = RNG_SEED_createInitialState()))
            return status;
    }

    /* only allow one consumer at a time... */
    /* Note that this is same mutex used by ExtractDepotBits */
    if (OK > (status = MRTOS_mutexWait(mRngSeedMutex, &isExtractMutexSet)))
        goto exit;

    bitPos = m_indexEntropyBitIn = ((m_indexEntropyBitIn + 1) % BITINMODVAL);

    if (entropyBit & 1)
    {
        ubyte4  index       = ((bitPos >> 3) % RNG_SEED_BUFFER_SIZE);
        ubyte4  bitIndex    = (bitPos & 7);
        ubyte   byteXorMask = (1 << bitIndex);

        m_entropyScratch[index] = m_entropyScratch[index] ^ byteXorMask;
    }

    status = OK;

exit:
    MRTOS_mutexRelease(mRngSeedMutex, &isExtractMutexSet);

    return status;
}

#endif /* #ifndef __DISABLE_MOCANA_ADD_ENTROPY__ */


/*------------------------------------------------------------------*/

extern MSTATUS
RNG_SEED_entropyThreadIds(RTOS_THREAD **ppRetTid1, RTOS_THREAD **ppRetTid2, RTOS_THREAD **ppRetTid3)
{
    /* has the entropy threads been initialized? if not, error out. */
    if (FALSE == mIsRngSeedInit)
        return ERR_FALSE;

    /* return back reference */
    *ppRetTid1 = &ethread01;
    *ppRetTid2 = &ethread02;
    *ppRetTid3 = &ethread03;

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RNG_SEED_killEntropyThreads(void)
{
    mShouldEntropyThreadsDie = TRUE;

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RNG_SEED_freeMutexes(void)
{
    RTOS_mutexFree(&mRngSeedThreadMutex);
    RTOS_mutexFree(&mRngSeedMutex);

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RNG_SEED_resetState(void)
{
    /* clears local/module variables, if want to restart entropy threads */
    /* up to caller to make sure any previous threads are dead before calling this API */
    mShouldEntropyThreadsDie = FALSE;
    mIsRngSeedInit = FALSE;
    mIsRngSeedThreadInit = FALSE;

    return OK;
}


/*------------------------------------------------------------------*/

static MSTATUS
RNG_SEED_createInitialState(void)
{
    MSTATUS         status = OK;

    if (FALSE == mIsRngSeedInit)
    {
        /* we don't have a master mutex, so we need to */
        /* assume that there is no contention for the very first call */
        /* this should be thread safe, since we generally only have */
        /* have a single entropy context --- seed generation is too */
        /* expensive to have multiple contextes */
        if (OK > (status = RTOS_mutexCreate(&mRngSeedMutex, 0, 0)))
            goto exit;

        if (OK > (status = RTOS_mutexCreate(&mRngSeedThreadMutex, 0, 0)))
            goto exit;

        m_indexEntropyByteDepot = RNG_SEED_BUFFER_SIZE;     /* no bytes available */
        m_indexEntropyBitIn = 0;

        mIsRngSeedInit = TRUE;
    }
    else
    {
        status = ERR_FALSE;
    }

exit:
    return status;

}


/*------------------------------------------------------------------*/

#if 0
#include <stdio.h>

extern MSTATUS
RNG_SEED_TEST_it(void)
{
    ubyte   buffer[RNG_SEED_BUFFER_SIZE];
    sbyte4  i, j;
    MSTATUS status;

    MOC_MEMSET((ubyte *)m_entropyScratch, 0xff, sizeof(m_entropyScratch));

    for (i = 0; i < 2; i++)
    {
        MOC_MEMSET(buffer, 0x00, RNG_SEED_BUFFER_SIZE);

        if (OK > (status = RNG_SEED_extractDepotBits(buffer, RNG_SEED_BUFFER_SIZE)))
            goto exit;

        for (j = 0; (RNG_SEED_BUFFER_SIZE - 1) > j; j++)
            printf(" 0x%02x,", buffer[j]);

        printf(" 0x%02x\n", buffer[j]);
    }

exit:
    RNG_SEED_killEntropyThreads();

    return status;
}
#endif

#endif	/* #ifndef __DISABLE_MOCANA_RNG__ */


