/* Version: mss_v6_3 */
/*
 * random.c
 *
 * Random Number FIPS-186 Factory
 *
 * Copyright Mocana Corp 2007. All Rights Reserved.
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
#include "../crypto/sha1.h"

#include "../common/random.h"

#include "../crypto/crypto.h"

#ifdef __ENABLE_MOCANA_ECC__
#undef __ENABLE_MOCANA_VLONG_ECC_CONVERSION__
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/primefld_priv.h"
#include "../crypto/primeec_priv.h"
#endif /* __ENABLE_MOCANA_ECC__ */
#include "../harness/harness.h"
#include "../crypto/aesalgo.h"
#include "../crypto/aes.h"
#if !defined(__DISABLE_3DES_CIPHERS__)
#include "../crypto/des.h"
#include "../crypto/three_des.h"
#endif /* ! __DISABLE_3DES_CIPHERS__ */

#include "../crypto/nist_rng.h"

#include "../common/rng_seed.h"

#include "../crypto/nist_rng_types.h"  /* This is to get the RandomContext data structures */

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif

#if 0
#include <stdio.h>
#define __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
#define __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
#define __ENABLE_MOCANA_RAND_ENTROPY_THREADS_PERFMON__
#endif

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_MORE_TIME__
#define MOCANA_RAND_ENTROPY3_LOOP_COUNTER 5  /* N times thru the data. */
#define MOCANA_RAND_ENTROPY2_MAX_TIME  7100
#define MOCANA_RAND_ENTROPY1_MAX_TIME  7000
#else
#define MOCANA_RAND_ENTROPY3_LOOP_COUNTER 3  /* N times thru the data. */
#define MOCANA_RAND_ENTROPY2_MAX_TIME  5100
#define MOCANA_RAND_ENTROPY1_MAX_TIME  5000
#endif

/* personalization string used by some DRBG. Can be NULL (default)
or set up to be a function */
#ifndef MOCANA_RNG_GET_PERSONALIZATION_STRING
#define MOCANA_RNG_GET_PERSONALIZATION_STRING  GetNullPersonalizationString
#endif

#ifndef __DISABLE_MOCANA_RNG__

#ifdef __KERNEL__
#include <linux/kernel.h>       /* for printk */
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/string.h>
#define PRINTDBG printk
#else
#define PRINTDBG printf
#endif
#endif /*__DISABLE_MOCANA_RNG__*/

randomContext*   g_pRandomContext = NULL;
static volatile intBoolean          mIsGlobSeedInit  = FALSE;
static volatile intBoolean          mIsGlobSeedDone  = FALSE;
static RTOS_MUTEX          mGlobInitMutex   = NULL;
static int mEntropySource = ENTROPY_DEFAULT_SRC;

static volatile intBoolean  ethread04running = FALSE;

#ifndef __DISABLE_MOCANA_RNG__
/****************************************************/
/* Entropy Performance monitoring / tracing support */
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_PERFMON__
typedef struct threadstamp
{
	ubyte tid; /* 0..3 */
    ubyte4     sleepTime;
    moctime_t  outTime;
    moctime_t  inTime;
} threadstamp;

#define MAX_TSS_SAVED 10000     /* Overkill. */
#define MAX_TSS_THREADIDS 10    /* Overkill. */
#define MAX_VERBOSE_LOG_MSGS 0  /* 0 means just save to memory. > 0 is a per-thread counter (see below) */
#define MAX_LOG_DUMP_TID_LINES 20 /* Overkill. */

static int ts_ndx_entrycount;
static int ts_ndx;
static threadstamp tss[MAX_TSS_SAVED];
static int ethreadrunCounter[MAX_TSS_THREADIDS]; /* We only need four. */

static int verbose_log_curr[MAX_TSS_THREADIDS];
static int verbose_log_max[MAX_TSS_THREADIDS];

void PERFMON_loginit()
{   int iii;
	for (iii = 0; iii < MAX_TSS_THREADIDS; iii++)
	{
		ethreadrunCounter[iii] = 0;
		verbose_log_curr[iii] = 0;
		verbose_log_max[iii] = MAX_VERBOSE_LOG_MSGS;
	}
	ts_ndx = 0;
	ts_ndx_entrycount = 0;
}

void PERFMON_logentry(int id, ubyte4 sleepTime, moctime_t *pOutTime)
{
	ethreadrunCounter[id]++;
	tss[ts_ndx].tid = id;
	tss[ts_ndx].sleepTime = sleepTime;
	tss[ts_ndx].outTime = *pOutTime;
	RTOS_deltaMS(pOutTime, &tss[ts_ndx].inTime); /* Just to get inTime. */

	ts_ndx = (ts_ndx + 1) % MAX_TSS_SAVED;
	ts_ndx_entrycount++; /* Doesn't wrap... How many entries have been logged. */

	if (verbose_log_curr[id] < verbose_log_max[id])
	{
	    PRINTDBG("E-PERF: etid[%d] : sleepTime=%d \n", id, sleepTime);
	    verbose_log_curr[id] += 1;
	}
}
#define PERFMON_PRINTDBG() PRINTDBG()

void PERFMON_dump_thread_counters(void)
{
    PRINTDBG("E-PERF: entropy threads run counters = %d : %d : %d : %d \n",ethreadrunCounter[0],ethreadrunCounter[1],ethreadrunCounter[2],ethreadrunCounter[3]);
}

void PERFMON_dump_log_tids(void)
{
	/* NOTE: If it wraps, this will only print from [0] to the curr [ndx] */
	/*       if it matters, then this can be re-done to print them all..  */
    int currlinenum = 0;
    int maxtidperline = 80;
    static char tidline[120];
    int ii = 0;
    int iiline = 0;

    MOC_MEMSET(tidline,0,sizeof(tidline)); /* empty it. */

    while (ii <= ts_ndx)
    {
    	if (iiline > maxtidperline)
    	{   tidline[iiline] = '\0'; /* NULL terminate it. */
    		PRINTDBG("E-PERF: e-tids[]:%s\n",tidline);
    		iiline = 0;
    		MOC_MEMSET(tidline,0,sizeof(tidline)); /* empty it. */
    		if (currlinenum++ > MAX_LOG_DUMP_TID_LINES)
    		{
        		PRINTDBG("E-PERF: e-tids[]: Data truncated...\n");
    			break;
    		}
    	}
    	tidline[iiline++] = tss[ii].tid + '0';
    	ii++;
    }
}

void PERFMON_dump_log_full(void)
{
	 PERFMON_dump_log_tids(); /* Not needed for now. */
}

#else
#define PERFMON_loginit()
#define PERFMON_logentry(i,s,o)
#define PERFMON_dump_thread_counters()
#define PERFMON_dump_log_tids()
#define PERFMON_dump_log_full()
#endif
/****************************************************/

/*------------------------------------------------------------------*/

typedef struct entropyBundle
{
    rngFIPS186Ctx*  pCtx;

    ubyte           ethread01running;
    ubyte           ethread02running;
    ubyte           ethread03running;

} entropyBundle;

#ifdef __FIPS_OPS_TEST__
static int rng_fail = 0;
#endif

/*------------------------------------------------------------------*/

/* prototypes */
static void RNG_add(ubyte* a, sbyte4 aLen, const ubyte* b, sbyte4 bLen, ubyte carry) ;

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void RNG_scramble(rngFIPS186Ctx *pRngFipsCtx);
#endif

#ifdef __FIPS_OPS_TEST__
MOC_EXTERN void triggerRNGFail(void)
{
    rng_fail = 1;
}
MOC_EXTERN void resetRNGFail(void)
{
    rng_fail = 0;
}
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
static MSTATUS RNG_fipsConditionalTest(rngFIPS186Ctx* pCtx, ubyte* pNewRng)
{
    MSTATUS status = OK;
    sbyte4 cmp = 0;

#ifdef __FIPS_OPS_TEST__

    if ( 1 == rng_fail )
    {
        MOC_MEMCPY(pNewRng,pCtx->rngHistory,SHA1_RESULT_SIZE);
    }
#endif

    /* New Random Number must not be the same compare to the previous one -- FIPS */
    status =  MOC_MEMCMP(pNewRng, pCtx->rngHistory, SHA1_RESULT_SIZE,&cmp);

    if ( ( OK > status ) ||
         ( 0 == cmp )  )
    {
        status = ERR_FIPS_RNG_FAIL;
    }
    else
    {
        /* Copy the current RNG output to history for future comparision */
        MOC_MEMCPY(pCtx->rngHistory,pNewRng,SHA1_RESULT_SIZE);
    }

exit:
    return status;
}
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void
void_entropyThread1(void* context)
{
    entropyBundle *pEb = (entropyBundle *)context;
    moctime_t startTime;
    sbyte4 i;

    moctime_t outTime;
    ubyte4 sleepTime;

    RTOS_deltaMS(NULL, &startTime);

    for (i = 0; i < pEb->pCtx->b; i++)
    {
        ubyte4 newval;
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
        ubyte4 oldval;
        oldval = pEb->pCtx->key[i];
#endif

        newval = pEb->pCtx->key[i];
        newval ^= 0x10;
        pEb->pCtx->key[i] = (ubyte)newval;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
        PRINTDBG("ethread01[%03d] %8x %8x %8x\n", i, oldval, newval, pEb->pCtx->key[i]);
#endif

		sleepTime = (((RTOS_deltaMS(&startTime, &outTime) >> 1) & 0x3) + 13);
        RTOS_sleepMS(sleepTime);
        PERFMON_logentry(1, sleepTime, &outTime);
#if (defined(__KERNEL__))
		if (kthread_should_stop())
		{
			#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
				PRINTDBG("ethread01:(1) kthread_should_stop = TRUE\n");
			#endif
		    pEb->ethread01running = FALSE;
			return;
		}
#endif
    }

    pEb->ethread01running = FALSE;

    RTOS_deltaMS(NULL, &startTime);

    /* keep running until thread 3 is done (or up to 2 secs) */
    while ((FALSE != pEb->ethread03running) && (RTOS_deltaMS(&startTime, NULL) < MOCANA_RAND_ENTROPY1_MAX_TIME))
    {
        for (i = 0; i < ((pEb->pCtx->b) && (FALSE != pEb->ethread03running)); i++)
        {
            ubyte4 newval;
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
            ubyte4 oldval;
            oldval = pEb->pCtx->key[i];
#endif

            newval = pEb->pCtx->key[i];
            newval ^= 0x10;
            pEb->pCtx->key[i] = (ubyte)newval;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
            PRINTDBG("ethread01[%03d] %8x %8x %8x\n", i, oldval, newval, pEb->pCtx->key[i]);
#endif

            sleepTime = (((RTOS_deltaMS(&startTime, &outTime) >> 1) & 0x3) + 13);
            RTOS_sleepMS(sleepTime);
            PERFMON_logentry(1, sleepTime, &outTime);
#if (defined(__KERNEL__))
            if (kthread_should_stop())
            {
				#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
					PRINTDBG("ethread01:(2) kthread_should_stop = TRUE\n");
				#endif
					return;
            }
#endif
        }
    }

#ifdef __FREERTOS_RTOS__
    RTOS_taskSuspend(NULL);
#endif
    return;
}
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void
void_entropyThread2(void* context)
{
    entropyBundle*  pEb = (entropyBundle *)context;
    moctime_t       startTime;
    sbyte4          i;

    moctime_t       outTime;
    ubyte4          sleepTime;

    RTOS_deltaMS(NULL, &startTime);

    for (i = pEb->pCtx->b-1; i >= 0; i--)
    {
        ubyte4 newval;
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
        ubyte4 oldval;
        oldval = pEb->pCtx->key[i];
#endif

        newval = pEb->pCtx->key[i];
        newval = (newval ^ (newval * 13) ^ (newval * 37) ^ (newval * 57)) & 0xff;
        pEb->pCtx->key[i] = (ubyte)newval;
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
        PRINTDBG("ethread02[%03d] %8x %8x %8x\n", i, oldval, newval, pEb->pCtx->key[i]);
#endif
        sleepTime = (((RTOS_deltaMS(&startTime, &outTime) >> 1) & 0x3) + 7);
        RTOS_sleepMS(sleepTime);
        PERFMON_logentry(2, sleepTime, &outTime);
#if (defined(__KERNEL__))
		if (kthread_should_stop())
		{
			#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
				PRINTDBG("ethread02:(1) kthread_should_stop = TRUE\n");
			#endif
		    pEb->ethread02running = FALSE;
			return;
		}
#endif
    }

    pEb->ethread02running = FALSE;

    RTOS_deltaMS(NULL, &startTime);

    /* keep running until thread 3 is done (or up to 2.1 secs) */
    while ((FALSE != pEb->ethread03running) && (RTOS_deltaMS(&startTime, NULL) < MOCANA_RAND_ENTROPY2_MAX_TIME))
    {
        for (i = pEb->pCtx->b-1; ((i >= 0) && (FALSE != pEb->ethread03running)); i--)
        {
            ubyte4 newval;
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
            ubyte4 oldval;
            oldval = pEb->pCtx->key[i];
#endif

            newval = pEb->pCtx->key[i];
            newval = (newval ^ (newval * 13) ^ (newval * 37) ^ (newval * 57)) & 0xff;
            pEb->pCtx->key[i] = (ubyte)newval;
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
            PRINTDBG("ethread02[%03d] %8x %8x %8x\n", i, oldval, newval, pEb->pCtx->key[i]);
#endif
            sleepTime = (((RTOS_deltaMS(&startTime, &outTime) >> 1) & 0x3) + 7);
            RTOS_sleepMS(sleepTime);
            PERFMON_logentry(2, sleepTime, &outTime);
#if (defined(__KERNEL__))
            if (kthread_should_stop())
    		{
    			#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    				PRINTDBG("ethread02:(2) kthread_should_stop = TRUE\n");
    			#endif
    			return;
    		}
#endif
        }
    }

#ifdef __FREERTOS_RTOS__
    RTOS_taskSuspend(NULL);
#endif
    return;
}
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void
void_entropyThread3(void* context)
{
    entropyBundle*  pEb = (entropyBundle *)context;
    rngFIPS186Ctx*  pRngFipsCtx;
    ubyte4          i;
    moctime_t       startTime;

    moctime_t outTime;
    ubyte4 sleepTime;

    pRngFipsCtx = pEb->pCtx;

    for (i = 0; i < pRngFipsCtx->b; i += 7)
    {
        RTOS_deltaMS(NULL, &startTime);

        while (RTOS_deltaMS(&startTime, NULL) < ((i + 1) * 19))
            RNG_scramble(pRngFipsCtx);

        sleepTime = (((RTOS_deltaMS(&startTime, &outTime) >> 1) & 0x3) + 3);
        RTOS_sleepMS(sleepTime);
    	PERFMON_logentry(3, sleepTime, &outTime);
#if (defined(__KERNEL__))
    	if (kthread_should_stop())
    	{
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    		PRINTDBG("ethread03:kthread_should_stop = TRUE\n");
#endif
    		pEb->ethread03running = FALSE;
    		return;
    	}
#endif
    }

    pEb->ethread03running = FALSE;

#ifdef __FREERTOS_RTOS__
    RTOS_taskSuspend(NULL);
#endif
    return;
}
#endif

/*------------------------------------------------------------------*/
#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
#define TMP_SEED_BUFFER_SIZE        (48)

static void
void_entropyThread4(void* context)
{
    MSTATUS      status = OK;
    moctime_t startTime;
    intBoolean   isGlobMutexSet = FALSE;
    ubyte bitbuf[TMP_SEED_BUFFER_SIZE];
    randomContext*   tempRandomContext = NULL;
    ubyte4 currEntropySource;
    int byte_ndx;
    int bit_ndx;
    intBoolean* pEthread04Running = (intBoolean *)(context);


    RTOS_deltaMS(NULL, &startTime);

#ifndef __DISABLE_MOCANA_ADD_ENTROPY__

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("Time: %d : void_entropyThread4() STARTED..................\n",RTOS_getUpTimeInMS());
#endif

    /* only allow one consumer at a time... */
    if (OK > (status = MRTOS_mutexWait(mGlobInitMutex, &isGlobMutexSet)))
        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("Time: %d : void_entropyThread4() Extracting..................\n",RTOS_getUpTimeInMS());
#endif

    currEntropySource = RANDOM_getEntropySource();
    RANDOM_setEntropySource(ENTROPY_SRC_INTERNAL);
    if (OK > (status = RANDOM_acquireContext(&tempRandomContext)))
		goto exit;
    RANDOM_setEntropySource(currEntropySource);
    if (OK > (status = RANDOM_numberGenerator(tempRandomContext, &bitbuf[0], TMP_SEED_BUFFER_SIZE)))
		goto exit;
    if (OK > (status = RANDOM_releaseContext(&tempRandomContext)))
		goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("Time: %d : void_entropyThread4() Calling AddEntropyBit calls.............. \n",RTOS_getUpTimeInMS());
#endif

    for (byte_ndx = 0; byte_ndx < TMP_SEED_BUFFER_SIZE; byte_ndx++)
    {
        for (bit_ndx = 0; bit_ndx < 8; bit_ndx++)
        {
            if (g_pRandomContext != NULL)
                RANDOM_addEntropyBit(g_pRandomContext, (bitbuf[byte_ndx] >> bit_ndx));
        }
    }

    mIsGlobSeedDone = TRUE;
    MRTOS_mutexRelease(mGlobInitMutex, &isGlobMutexSet);

#endif /*__DISABLE_MOCANA_ADD_ENTROPY__*/

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("Time: %d : void_entropyThread4() DONE. \n",RTOS_getUpTimeInMS());
#endif

exit:
    RTOS_mutexFree(&mGlobInitMutex);
    *pEthread04Running = FALSE;
#ifdef __FREERTOS_RTOS__
    RTOS_taskSuspend(NULL);
#endif
    return;
}
#endif

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
#if (defined(__KERNEL__))
static int int_entropyThread1(void* context)
{
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread01 starting.\n");
#endif
    set_current_state(TASK_INTERRUPTIBLE);
    void_entropyThread1(context);
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread01 done.\n");
#endif
    return 0;
}

static int int_entropyThread2(void* context)
{
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread02 starting.\n");
#endif
    set_current_state(TASK_INTERRUPTIBLE);
    void_entropyThread2(context);
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread02 done.\n");
#endif
    return 0;
}

static int int_entropyThread3(void* context)
{
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread03 starting.\n");
#endif
    set_current_state(TASK_INTERRUPTIBLE);
    void_entropyThread3(context);
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread03 done.\n");
#endif
    return 0;
}

static int int_entropyThread4(void* context)
{
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread04 starting.\n");
#endif
    set_current_state(TASK_INTERRUPTIBLE);
    void_entropyThread4(context);
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("ethread04 done.\n");
#endif
    return 0;
}

#define entropyThread1 int_entropyThread1
#define entropyThread2 int_entropyThread2
#define entropyThread3 int_entropyThread3
#define entropyThread4 int_entropyThread4
#else
#define entropyThread1 void_entropyThread1
#define entropyThread2 void_entropyThread2
#define entropyThread3 void_entropyThread3
#define entropyThread4 void_entropyThread4
#endif  /* KERNEL */
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
static void
RNG_scramble(rngFIPS186Ctx *pRngFipsCtx)
{
    sbyte4  i, j;

    for (i = 0; i < 2; ++i)
    {
        ubyte* w = pRngFipsCtx->result + i * SHA1_RESULT_SIZE;

        MOC_MEMCPY(pRngFipsCtx->scratch, pRngFipsCtx->key, pRngFipsCtx->b);

        /* add the seed to the key in the scratch area */
        if (pRngFipsCtx->pSeed && pRngFipsCtx->seedLen>0)
        {
            RNG_add(pRngFipsCtx->scratch, pRngFipsCtx->b, pRngFipsCtx->pSeed, pRngFipsCtx->seedLen, 0);
            pRngFipsCtx->seedLen -= pRngFipsCtx->b;

            if (pRngFipsCtx->seedLen > 0 )
                pRngFipsCtx->pSeed += pRngFipsCtx->b;
        }

        /* pad with 0 to 512 bits */
        for (j = pRngFipsCtx->b; j < 64; ++j)
            pRngFipsCtx->scratch[j] = 0;

        SHA1_G( pRngFipsCtx->scratch, w);
        RNG_add( pRngFipsCtx->key, pRngFipsCtx->b, w, SHA1_RESULT_SIZE, 1);
    }
}
#endif /* __DISABLE_MOCANA_RAND_ENTROPY_THREADS__ */


/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_newFIPS186Context(randomContext **ppRandomContext,
                         ubyte b, const ubyte pXKey[/*b*/],
                         sbyte4 seedLen, const ubyte pXSeed[/*seedLen*/])
{
    RandomCtxWrapper* pWrapper = NULL;
    rngFIPS186Ctx*  pRngFipsCtx = NULL;
    MSTATUS         status;

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
    sbyte4          cmp = 0xA5A5;

   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186);

    if ( b == (ubyte)seedLen )
    {
        MOC_MEMCMP( pXKey, pXSeed, b, &cmp );
        if ( 0 == cmp )
        {
            status = ERR_CRYPTO;
            goto exit;
        }
    }
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    if (!ppRandomContext || !pXKey)
    {
        return ERR_NULL_POINTER;
    }
    *ppRandomContext = NULL;

    if ( b < 20 || b > 64)
    {
        status = ERR_INVALID_ARG;
        goto exit;
    }

    if (OK > (status = MOC_alloc(sizeof(RandomCtxWrapper), (void *)&pWrapper)))
    {
        return ERR_NULL_POINTER;
    }
    pWrapper->WrappedCtxType = NIST_FIPS186;
    pRngFipsCtx = GET_FIPS186_CTX(pWrapper);
    if (pRngFipsCtx == NULL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pRngFipsCtx->rngMutex = 0;

    pRngFipsCtx->b = b;
    MOC_MEMCPY(pRngFipsCtx->key, pXKey, b);

    pRngFipsCtx->numBytesAvail = 0;
    pRngFipsCtx->pSeed = pXSeed;
    pRngFipsCtx->seedLen = seedLen;

    /* Create mutex used to guard shared context */
    if ( OK > ( status = RTOS_mutexCreate( &pRngFipsCtx->rngMutex, 0, 0 ) ) )
        goto exit;

    /* setup for return */
    *ppRandomContext = pWrapper;
    pWrapper = NULL;

exit:

    RANDOM_deleteFIPS186Context((randomContext**) &pWrapper);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_deleteFIPS186Context( randomContext** pp_randomContext)
{
    RandomCtxWrapper* pWrapper = NULL;
    rngFIPS186Ctx* pRngFipsCtx = NULL;
#ifdef __ZEROIZE_TEST__
    int counter = 0;
#endif

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (!pp_randomContext || !*pp_randomContext)
    {
        return ERR_NULL_POINTER;
    }

    pWrapper = (RandomCtxWrapper*)(*pp_randomContext);

    pRngFipsCtx = GET_FIPS186_CTX(pWrapper);
    if (pRngFipsCtx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTOS_mutexFree(&pRngFipsCtx->rngMutex);

#ifdef __ZEROIZE_TEST__
    FIPS_PRINT("\nRNG - Before Zeroization\n");
    for( counter = 0; counter < sizeof(RandomCtxWrapper); counter++)
    {
        FIPS_PRINT("%02x",*((ubyte*)*pp_randomContext+counter));
    }
	FIPS_PRINT("\n");
#endif

    /* clear out data */
    MOC_MEMSET(*pp_randomContext, 0x00, sizeof(RandomCtxWrapper));

#ifdef __ZEROIZE_TEST__
    FIPS_PRINT("\nRNG - After Zeroization\n");
    for( counter = 0; counter < sizeof(RandomCtxWrapper); counter++)
    {
        FIPS_PRINT("%02x",*((ubyte*)*pp_randomContext+counter));
    }
	FIPS_PRINT("\n");
#endif

    return MOC_free(pp_randomContext);
}


/*------------------------------------------------------------------*/

extern sbyte4
RANDOM_rngFun(void* rngFunArg, ubyte4 length, ubyte *buffer)
{
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    return RANDOM_numberGenerator((randomContext *) rngFunArg,
                                    buffer, (sbyte4) length);
}


/*------------------------------------------------------------------*/

static MSTATUS
RANDOM_acquireFIPS186Context( randomContext** ppCtx, ubyte4 keySize)
{
#if ((!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__)) && (!defined(__DISABLE_MOCANA_RAND_SEED__)))
    RTOS_THREAD             ethread01;
    RTOS_THREAD             ethread02;
    RTOS_THREAD             ethread03;
    static entropyBundle    eb;

    moctime_t outTime;
    ubyte4 sleepTime;

#endif
    RandomCtxWrapper*       pWrapper = NULL;
    rngFIPS186Ctx*          pRngFipsCtx = NULL;
    ubyte                   key[MOCANA_RNG_MAX_KEY_SIZE];
#if (!  (defined(__DISABLE_MOCANA_RAND__) || defined(__DISABLE_MOCANA_RAND_SEED__)))
    moctime_t               startTime;
    ubyte4                  upTime, temp, i;
#endif
    MSTATUS                 status = OK;

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

#if (defined(__DISABLE_MOCANA_RAND__))
    PRINTDBG("RANDOM_acquireContext: WARNING: __DISABLE_MOCANA_RAND__ !!!\n");

#elif (defined(__DISABLE_MOCANA_RAND_SEED__))
    /* only useful for benchmarking / optimizing key generation */
    MOC_MEMSET(key, 0x00, 20);

    if (OK > (status = RANDOM_newFIPS186Context((randomContext **) &pWrapper,
                                                20, key, 0, NULL)))
    {
        goto exit;
    }
    else
    {
        pRngFipsCtx = GET_FIPS186_CTX(pWrapper);
        if (pRngFipsCtx == NULL)
        {
            status = ERR_NULL_POINTER;
            goto exit;
        }
    }
#else

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("*** Start basic seeding... \n");
#endif

    /* basic seeding of the key */
    RNG_SEED_extractInitialDepotBits(key, keySize);

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_PERFMON__
    ubyte4               perfStartTime;
    ubyte4               perfStartThreadTime;
    ubyte4               perfEndTime;
    perfStartTime = RTOS_getUpTimeInMS();
    PRINTDBG("E-PERF: RANDOM_acquireContext: Starting... time=%d \n",perfStartTime);
#endif

    if (OK > (status = RANDOM_newFIPS186Context((randomContext **)&pWrapper,
                                                (ubyte) keySize, key, 0, NULL)))
    {
        goto exit;
    }
    else
    {
        pRngFipsCtx = GET_FIPS186_CTX(pWrapper);
        if (pRngFipsCtx == NULL)
        {
            status = ERR_NULL_POINTER;
            goto exit;
        }
    }

    RTOS_deltaMS(NULL, &startTime);
    /* ...continue basic seeding */
    for (i = 0; i < pRngFipsCtx->b; i++)
    {
        upTime = RTOS_deltaMS(&startTime, NULL);

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
/*Verbose:        PRINTDBG("*** pass %d through random array \n", i); */
#endif

        if (((upTime & 0x3) & (pRngFipsCtx->key[i] & 0x3)) == 0x3)
        {
            temp = (pRngFipsCtx->key[upTime % pRngFipsCtx->b] + upTime);
            pRngFipsCtx->key[upTime % pRngFipsCtx->b] = (ubyte)(temp & 0xff);
        }

        temp = pRngFipsCtx->key[i] ^ upTime;
        pRngFipsCtx->key[i] = (ubyte)(temp & 0xff);
        if (OK > (status = RANDOM_numberGeneratorFIPS186((randomContext *)pWrapper, pRngFipsCtx->key, sizeof(pRngFipsCtx->key))))
		{
            goto exit;
 		}
    }
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("*** Finished basic seeding.\n");
#endif

    /* leverages preemptive RTOS */
#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
    if (mEntropySource == ENTROPY_SRC_INTERNAL)
    {

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    	PRINTDBG("*** Creating entropy threads \n", i);
#endif

    eb.pCtx = pRngFipsCtx;
    eb.ethread01running = eb.ethread02running = eb.ethread03running = TRUE;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_PERFMON__
    perfStartThreadTime = RTOS_getUpTimeInMS();
    PRINTDBG("E-PERF: RANDOM_acquireContext: Starting Entropy threads... time=%d \n",perfStartThreadTime);
#endif

    PERFMON_loginit();

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** creating entropy thread 01\n", i);
#endif

    if (OK > (status = RTOS_createThread(entropyThread1, &eb, (sbyte4)ENTROPY_THREAD, &ethread01)))
        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** creating entropy thread 02\n", i);
#endif

    if (OK > (status = RTOS_createThread(entropyThread2, &eb, (sbyte4)ENTROPY_THREAD, &ethread02)))
        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** creating entropy thread 03\n", i);
#endif

    if (OK > (status = RTOS_createThread(entropyThread3, &eb, (sbyte4)ENTROPY_THREAD, &ethread03)))
        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** entropy threads created \n", i);
#endif

    while (eb.ethread01running || eb.ethread02running || eb.ethread03running)
    {
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
        PRINTDBG("*** ethreads running\n");
#endif
        RNG_scramble(pRngFipsCtx);
		sleepTime = (((RTOS_deltaMS(&startTime, &outTime) >> 1) & 0xFF) + 1);
        RTOS_sleepMS(sleepTime);
    }

    RNG_scramble(pRngFipsCtx);
#if (defined(__KERNEL__))
	#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
		PRINTDBG("*** entropy threads already done. Not destoying them.\n");
	#endif
#else
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
	PRINTDBG("*** Calling destroyThread on entropy threads.\n");
#endif
    RTOS_destroyThread(ethread01);
    RTOS_destroyThread(ethread02);
    RTOS_destroyThread(ethread03);
#endif
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_DEBUGGING__
    PRINTDBG("*** entropy threads done\n");
#endif

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_PERFMON__
    perfEndTime = RTOS_getUpTimeInMS();
    PRINTDBG("E-PERF: RANDOM_acquireContext: Entropy threads ran for %d milliseconds\n",(perfEndTime-perfStartThreadTime));
#endif


    	PERFMON_dump_thread_counters();
    	PERFMON_dump_log_tids();
    } /* if (mEntropySource == ENTROPY_SRC_INTERNAL) */

#endif /* ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__ */

    if (OK > (status = RANDOM_numberGeneratorFIPS186((randomContext *)pWrapper, pRngFipsCtx->key, sizeof(pRngFipsCtx->key))))
        goto exit;


#endif

    pWrapper->reseedBitCounter = 0;
    *ppCtx = (randomContext *)pWrapper;
    pWrapper = NULL;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_THREADS_PERFMON__
    perfEndTime = RTOS_getUpTimeInMS();
    PRINTDBG("E-PERF: RANDOM_acquireContext: Done. Total process took %d milliseconds\n",(perfEndTime-perfStartTime));
#endif

exit:
    RANDOM_deleteFIPS186Context((randomContext**) &pWrapper);
    return status;
}


/*--------------------------------------------------------------------------*/

static void RNG_add( ubyte* a, sbyte4 aLen, const ubyte* b, sbyte4 bLen,
                    ubyte carry)
{
    sbyte4 i, j;

    for (i = aLen-1, j = bLen-1; i >= 0; --i, --j)
    {
        a[i] += carry;
        carry = (a[i] < carry) ? 1 : 0;

        if (j >= 0)
        {
            a[i] += b[j];
            carry += (a[i] < b[j]) ? 1 : 0;
        }
    }
}


/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_KSrcGenerator(randomContext *pRandomContext, ubyte buffer[40])
{
    RandomCtxWrapper* pWrapper = NULL;
    rngFIPS186Ctx*  pRngFipsCtx = NULL;
    sbyte4          i,j;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ( !pRandomContext || !buffer)
    {
        return ERR_NULL_POINTER;
    }

    pWrapper = (RandomCtxWrapper*)pRandomContext;
    pRngFipsCtx = GET_FIPS186_CTX(pWrapper);
    if (pRngFipsCtx == NULL)
    {
        return ERR_NULL_POINTER;
    }

    for (i = 0; i < 2; ++i)
    {
        ubyte* w = buffer + i * SHA1_RESULT_SIZE;

        MOC_MEMCPY( pRngFipsCtx->scratch, pRngFipsCtx->key, pRngFipsCtx->b);

        /* pad with 0 to 512 bits */
        for (j = pRngFipsCtx->b; j < 64; ++j)
        {
            pRngFipsCtx->scratch[j] = 0;
        }

        SHA1_GK( pRngFipsCtx->scratch, w);

        RNG_add( pRngFipsCtx->key, pRngFipsCtx->b, w, SHA1_RESULT_SIZE, 1);
    }

    return OK;
}


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
RANDOM_numberGeneratorFIPS186(randomContext *pRandomContext, ubyte *pRetRandomBytes,
                       sbyte4 numRandomBytes)
{
    RandomCtxWrapper* pWrapper = NULL;
    rngFIPS186Ctx*  pRngFipsCtx = NULL;
    sbyte4          bytesToCopy;
    MSTATUS         status = OK;
    sbyte4          i,j;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_FIPS186);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if ( !numRandomBytes)
    {
        return OK;
    }

    if ( !pRandomContext || !pRetRandomBytes)
    {
        return ERR_NULL_POINTER;
    }

    pWrapper = (RandomCtxWrapper*)pRandomContext;
    pRngFipsCtx = GET_FIPS186_CTX(pWrapper);
    if (pRngFipsCtx == NULL)
    {
        return ERR_NULL_POINTER;
    }

   /* Acquire the mutex before accessing context members shared among threads */
   if ( OK > ( status = RTOS_mutexWait(pRngFipsCtx->rngMutex) ) )
       goto exit;

    while (numRandomBytes)
    {
        if (numRandomBytes <= (sbyte4)pRngFipsCtx->numBytesAvail)
            bytesToCopy = numRandomBytes;
        else
            bytesToCopy = (sbyte4)pRngFipsCtx->numBytesAvail;

        if (0 < bytesToCopy)
        {
            MOC_MEMCPY(pRetRandomBytes,
                       pRngFipsCtx->result +
                       (2 * SHA1_RESULT_SIZE - pRngFipsCtx->numBytesAvail),
                       bytesToCopy);

            pRngFipsCtx->numBytesAvail = pRngFipsCtx->numBytesAvail - bytesToCopy;
            pRetRandomBytes = pRetRandomBytes + bytesToCopy;
            numRandomBytes  = numRandomBytes  - bytesToCopy;
        }

        if (0 >= pRngFipsCtx->numBytesAvail)
        {
            for (i = 0; i < 2; ++i)
            {
                ubyte* w = pRngFipsCtx->result + i * SHA1_RESULT_SIZE;

                MOC_MEMCPY( pRngFipsCtx->scratch, pRngFipsCtx->key, pRngFipsCtx->b);

                /* add the seed to the key in the scratch area */
                if (pRngFipsCtx->pSeed && pRngFipsCtx->seedLen>0)
                {
                    RNG_add( pRngFipsCtx->scratch, pRngFipsCtx->b,
                                    pRngFipsCtx->pSeed, pRngFipsCtx->seedLen, 0);
                    pRngFipsCtx->seedLen -= pRngFipsCtx->b;
                    if (pRngFipsCtx->seedLen > 0 )
                    {
                        pRngFipsCtx->pSeed += pRngFipsCtx->b;
                    }
                }

                /* pad with 0 to 512 bits */
                for (j = pRngFipsCtx->b; j < 64; ++j)
                {
                    pRngFipsCtx->scratch[j] = 0;
                }

                SHA1_G( pRngFipsCtx->scratch, w);
                RNG_add( pRngFipsCtx->key, pRngFipsCtx->b, w, SHA1_RESULT_SIZE, 1);

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
                if ( OK > ( status = RNG_fipsConditionalTest(pRngFipsCtx,w) ) )
                {
                	setFIPS_Status(FIPS_ALGO_RNG, status);
                    goto exit;
                }
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */
            }

            pRngFipsCtx->numBytesAvail = 2 * SHA1_RESULT_SIZE;
        }
    }

	RTOS_mutexRelease(pRngFipsCtx->rngMutex);

exit:
    return status;
}


/*------------------------------------------------------------------*/

static ubyte*
GetNullPersonalizationString(ubyte4* pLen)
{
    *pLen = 0;
    return ((ubyte*) 0);
}

/*------------------------------------------------------------------*/

static MSTATUS
RANDOM_acquireDRBGCTRContext(randomContext **ppRandomContext)
{
    MSTATUS status;
    randomContext* newCTRContext = 0;
    RandomCtxWrapper* pWrapper = NULL;
    ubyte4 persoStrLen;
    ubyte* persoStr;
    /* we use the vetted Entropy approach used for FIPS 186 and
    use the resulting FIPS 186 context 512 bits key as the initial
    entropy and nonce for the DRBG context.
    if no derivation function used:
    SeedLen = entropy length:(SP800-90)
    AES128 256 bits
    AES192 320 bits
    AES256 384 bits
    */
    ubyte entropyBytes[48];
    hwAccelDescr hwAccelCtx;

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG_CTR);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
   if (mEntropySource == ENTROPY_SRC_INTERNAL)
   {
	   if (OK > ( status = RNG_SEED_extractDepotBits(entropyBytes, 48)))
	   {
		   goto exit;
	   }
   }
   else
   {
	   if (OK > ( status = RNG_SEED_extractInitialDepotBits(entropyBytes, 48)))
	   {
		   goto exit;
	   }
   }
#else /* Only external is allowed anyway*/
   if (OK > ( status = RNG_SEED_extractDepotBits(entropyBytes, 48)))
   {
	   goto exit;
   }

#endif

    persoStr = MOCANA_RNG_GET_PERSONALIZATION_STRING( &persoStrLen);
    /* we always use the biggest entropy to provide for the maximum security
        strength. Example: generating AES 256 keys */
    if (OK > ( status = NIST_CTRDRBG_newContext( MOC_HASH(hwAccelCtx) &newCTRContext, entropyBytes,
                                                32, 16, persoStr, persoStrLen)))
    {
        goto exit;
    }
    pWrapper = (RandomCtxWrapper*)newCTRContext;
    pWrapper->reseedBitCounter = 0;

    *ppRandomContext = newCTRContext;
    newCTRContext = 0;

exit:

    NIST_CTRDRBG_deleteContext( MOC_HASH(hwAccelCtx) &newCTRContext);
    MOC_MEMSET( entropyBytes, 0, 48);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_acquireContext(randomContext **pp_randomContext)
{
    /* This function uses the Default algorithm defined in the header. The caller doesn't care, so we choose for him. */
    return RANDOM_acquireContextEx(pp_randomContext, RANDOM_DEFAULT_ALGO);
}

extern MSTATUS
RANDOM_acquireContextEx(randomContext **pp_randomContext, ubyte algoId)
{
    MSTATUS         status = OK;

    if (!pp_randomContext)
    {
        return ERR_NULL_POINTER;
    }

    if (algoId == MODE_RNG_ANY)
    {
        algoId = RANDOM_DEFAULT_ALGO; /* The caller doesn't care, so we choose for him. */
    }
    switch (algoId)
    {
    case MODE_RNG_FIPS186:
        status = RANDOM_acquireFIPS186Context( pp_randomContext, MOCANA_RNG_DEFAULT_KEY_SIZE);
        break;
    case MODE_DRBG_CTR:
        status = RANDOM_acquireDRBGCTRContext(pp_randomContext);
        break;
    default:
        status = ERR_INVALID_ARG;
        break;
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_releaseContext(randomContext **pp_randomContext)
{
    /* This function handles all types. */
    RandomCtxWrapper* pWrapper = NULL;
    hwAccelDescr hwAccelCtx;
    if (!pp_randomContext || !*pp_randomContext)
    {
        return ERR_NULL_POINTER;
    }
    pWrapper = (RandomCtxWrapper*)(*pp_randomContext);

    if (IS_FIPS186_CTX(pWrapper))
        return RANDOM_deleteFIPS186Context( pp_randomContext);
    else if (IS_CTR_DRBG_CTX(pWrapper))
        return NIST_CTRDRBG_deleteContext( MOC_HASH(hwAccelCtx) pp_randomContext);
    else
        return ERR_NULL_POINTER;
}


/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_numberGenerator(randomContext *pRandomContext, ubyte *pRetRandomBytes,
                       sbyte4 numRandomBytes)
{
    /* This function handles all types. */
    RandomCtxWrapper* pWrapper = NULL;
    hwAccelDescr hwAccelCtx;
    if ( !pRandomContext || !pRetRandomBytes)
    {
        return ERR_NULL_POINTER;
    }

    pWrapper = (RandomCtxWrapper*)pRandomContext;

    if (IS_FIPS186_CTX(pWrapper))
        return RANDOM_numberGeneratorFIPS186( pRandomContext, pRetRandomBytes, numRandomBytes);
    else if (IS_CTR_DRBG_CTX(pWrapper))
        return NIST_CTRDRBG_numberGenerator( MOC_HASH(hwAccelCtx) pRandomContext, pRetRandomBytes, numRandomBytes);
    else
        return ERR_NULL_POINTER;
}

/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_ADD_ENTROPY__

static MSTATUS
RANDOM_addEntropyBitFIPS186(randomContext *pRandomContext, ubyte entropyBit)
{
    RandomCtxWrapper* pWrapper = NULL;
    rngFIPS186Ctx*  pRngFipsCtx = NULL;
    ubyte4          modVal;
    ubyte4          bitPos;
    MSTATUS         status = OK;

    if (NULL == pRandomContext)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pWrapper = (RandomCtxWrapper*)pRandomContext;

    pRngFipsCtx = GET_FIPS186_CTX(pWrapper);
    if (pRngFipsCtx == NULL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }
    pWrapper->reseedBitCounter++;

    modVal = (8 * pRngFipsCtx->b);

    bitPos = pRngFipsCtx->bitPos = ((pRngFipsCtx->bitPos + 1) % modVal);

    if (entropyBit & 1)
    {
        ubyte4  index       = ((bitPos >> 3) % pRngFipsCtx->b);
        ubyte4  bitIndex    = (bitPos & 7);
        ubyte   byteXorMask = (1 << bitIndex);

        pRngFipsCtx->key[index] = pRngFipsCtx->key[index] ^ byteXorMask;
    }
exit:
    return status;
}


static MSTATUS
RANDOM_addEntropyBitDRBGCTR(randomContext *pRandomContext, ubyte entropyBit)
{
    RandomCtxWrapper* pWrapper = NULL;
    NIST_CTR_DRBG_Ctx* pCtx = NULL;

    MSTATUS         status = OK;

    ubyte4 MinBitsNeeded;
    hwAccelDescr hwAccelCtx;
    ubyte entropyBytes[48];
    /* we are using the same vetted Entropy approach used as the one used above
     * when creating the DBRG context which we'll be reseeding.
     * See comments above for length discussion
     * */

    if (NULL == pRandomContext)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pWrapper = (RandomCtxWrapper*)pRandomContext;

    pCtx = GET_CTR_DRBG_CTX(pWrapper);
    if (pCtx == NULL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* First add the bit into our EntropyDepot */
    if (OK > ( status = RNG_SEED_addEntropyBit(entropyBit)))
    {
        goto exit;
    }
    pWrapper->reseedBitCounter++;

    /* If we have "enough" new entropy bits, then reseed our context */
    MinBitsNeeded = 48 * 8;

    if (pWrapper->reseedBitCounter < MinBitsNeeded)
    {
        goto exit;
    }

    if (OK > ( status = RNG_SEED_extractDepotBits(entropyBytes, 48)))
    {
        goto exit;
    }

    if (OK > ( status = NIST_CTRDRBG_reseed( MOC_HASH(hwAccelCtx) pRandomContext,
                                            entropyBytes, 48,
                                            NULL, 0)))
    {
        goto exit;
    }
    pWrapper->reseedBitCounter = 0;  /* Reset the counter */

exit:
    return status;
}


extern MSTATUS
RANDOM_addEntropyBit(randomContext *pRandomContext, ubyte entropyBit)
{
    RandomCtxWrapper* pWrapper = NULL;
    MSTATUS         status = OK;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_RNG))
        return getFIPS_powerupStatus(FIPS_ALGO_RNG);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    if (NULL == pRandomContext)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pWrapper = (RandomCtxWrapper*)pRandomContext;

    if (IS_FIPS186_CTX(pWrapper))
        status =  RANDOM_addEntropyBitFIPS186(pRandomContext, entropyBit);
    else if (IS_CTR_DRBG_CTX(pWrapper))
        status =  RANDOM_addEntropyBitDRBGCTR(pRandomContext, entropyBit);
    else
        status = ERR_NULL_POINTER;

exit:
    return status;
}

#endif

#endif

/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS RANDOM_setEntropySource(ubyte EntropySrc)
{
    MSTATUS         status = OK;

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
    if ( (EntropySrc != ENTROPY_SRC_INTERNAL) && (EntropySrc != ENTROPY_SRC_EXTERNAL) )
    {
    	status = ERR_INVALID_ARG;
        goto exit;
    }
#else /* Only external is allowed */
    if ( (EntropySrc != ENTROPY_SRC_EXTERNAL) )
    {
    	status = ERR_INVALID_ARG;
        goto exit;
    }
#endif

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("RANDOM_setEntropySource() Setting mEntropySource == %d (was %d) \n",EntropySrc, mEntropySource);
#endif
	mEntropySource = EntropySrc;

exit:
	    return status;
}

MOC_EXTERN ubyte RANDOM_getEntropySource(void)
{
    if ( (mEntropySource != ENTROPY_SRC_INTERNAL) && (mEntropySource != ENTROPY_SRC_EXTERNAL) )
    {
    	mEntropySource = ENTROPY_DEFAULT_SRC;
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("RANDOM_getEntropySource() (0) Returning mEntropySource == %d \n", mEntropySource);
#endif
    	return ENTROPY_DEFAULT_SRC;
    }
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    PRINTDBG("RANDOM_getEntropySource() (1) Returning mEntropySource == %d \n", mEntropySource);
#endif
    return mEntropySource;
}

/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_acquireGlobalContext(randomContext **pp_randomContext)
{
	MSTATUS            status = OK;
    intBoolean         isGlobMutexSet = FALSE;
    ubyte              currEntropySource;
#if (!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__) && !defined(__DISABLE_MOCANA_RNG__))
    RTOS_THREAD        ethread04;
#endif

	if (TRUE == mIsGlobSeedDone)
	{
		goto exit;
	}

	if (FALSE == mIsGlobSeedInit)
	{
		if (NULL == mGlobInitMutex)
		{
			if (OK > (status = RTOS_mutexCreate(&mGlobInitMutex, 0, 0)))
				goto exit;
		}
		/* only allow one consumer at a time... */
		if (OK > (status = MRTOS_mutexWait(mGlobInitMutex, &isGlobMutexSet)))
		        goto exit;

#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    	PRINTDBG("Time: %d : RANDOM_acquireGlobalContext() STARTED ..................\n",RTOS_getUpTimeInMS());
#endif
		currEntropySource = RANDOM_getEntropySource();
	    RANDOM_setEntropySource(ENTROPY_SRC_EXTERNAL);
	    status = RANDOM_acquireContext(&g_pRandomContext);
	    RANDOM_setEntropySource(currEntropySource);
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    	PRINTDBG("Time: %d : RANDOM_acquireGlobalContext() CREATED ..................\n",RTOS_getUpTimeInMS());
#endif

	    if (OK > status)
	    {
	    	MRTOS_mutexRelease(mGlobInitMutex, &isGlobMutexSet);
	    	RTOS_mutexFree(&mGlobInitMutex);
	        goto exit;
	    }

#if (!defined(__DISABLE_MOCANA_RAND_ENTROPY_THREADS__) && !defined(__DISABLE_MOCANA_RNG__))
		if (ENTROPY_SRC_INTERNAL == currEntropySource)
		{
		    /* Need to start up our entropy thread. */
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
	    	PRINTDBG("Time: %d : RANDOM_acquireGlobalContext() Creating Entropy Thread4 ..................\n",RTOS_getUpTimeInMS());
#endif
	    	status = RTOS_createThread(entropyThread4, (void *) &ethread04running, (sbyte4)ENTROPY_THREAD, &ethread04);
			ethread04running = TRUE;
	        mIsGlobSeedInit = TRUE;
	    	MRTOS_mutexRelease(mGlobInitMutex, &isGlobMutexSet);
		}
		else
		{
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
			PRINTDBG("Time: %d : RANDOM_acquireGlobalContext() No Entropy thread created. H/W Entropy selected. \n",RTOS_getUpTimeInMS());
#endif
	        status = OK;
	        mIsGlobSeedInit = TRUE;
	    	mIsGlobSeedDone = TRUE;
	    	MRTOS_mutexRelease(mGlobInitMutex, &isGlobMutexSet);
	    	RTOS_mutexFree(&mGlobInitMutex);
		}
#else
    	/* No need for an entropy thread. */
#ifdef __ENABLE_MOCANA_RAND_ENTROPY_DEBUGGING__
    	PRINTDBG("Time: %d : RANDOM_acquireGlobalContext() No Entropy created. H/W Entropy. \n",RTOS_getUpTimeInMS());
#endif
        status = OK;
        mIsGlobSeedInit = TRUE;
    	mIsGlobSeedDone = TRUE;
    	MRTOS_mutexRelease(mGlobInitMutex, &isGlobMutexSet);
    	RTOS_mutexFree(&mGlobInitMutex);
#endif
	}
	else /* mIsGlobSeedInit == TRUE */
	{
		status = OK;
	}

	exit:
		if (OK == status)
		{
			if (pp_randomContext != NULL)
				*pp_randomContext = g_pRandomContext;
		}
		return status;

}

/*------------------------------------------------------------------*/

extern MSTATUS
RANDOM_releaseGlobalContext(randomContext **pp_randomContext)
{
    /* thread4 may still be running if application is ending */
	/* abnormally quickly; wait for it to finish */
	while(ethread04running)
		RTOS_sleepMS(100);

	mIsGlobSeedDone = FALSE;
	mIsGlobSeedInit = FALSE;
#ifndef __DISABLE_MOCANA_RNG__
    RNG_SEED_resetState();
    RNG_SEED_freeMutexes();
#endif
	return RANDOM_releaseContext(pp_randomContext);
}

/*------------------------------------------------------------------*/
