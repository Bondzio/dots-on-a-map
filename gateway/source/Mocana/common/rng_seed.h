/* Version: mss_v6_3 */
/*
 * rng_seed.h
 *
 * A random number generator seed header
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __RNG_SEED_HEADER__
#define __RNG_SEED_HEADER__

extern MSTATUS RNG_SEED_extractInitialDepotBits(ubyte *pDstCloneEntropyBytes, ubyte4 numEntropyBytes);

extern MSTATUS RNG_SEED_extractDepotBits(ubyte *pDstCloneEntropyBytes, ubyte4 numEntropyBytes);

extern MSTATUS RNG_SEED_addEntropyBit(ubyte entropyBit);

/* If you must kill and then later restart entropy threads, do this: */
/* 1. Get the thread ids */
/* 2. Kill the threads */
/* 3. Wait and verify the threads are indeed dead --- may take a little while for them to get the message */
/* 4. Release mutexes, reset state */
/* Now you can safely spin up threads at a future point. */

extern MSTATUS RNG_SEED_entropyThreadIds(RTOS_THREAD **ppRetTid1, RTOS_THREAD **ppRetTid2, RTOS_THREAD **ppRetTid3);
extern MSTATUS RNG_SEED_killEntropyThreads(void);
extern MSTATUS RNG_SEED_freeMutexes(void);
extern MSTATUS RNG_SEED_resetState(void);

#ifdef __FIPS_OPS_TEST__
MOC_EXTERN void triggerSeedFail(void);
MOC_EXTERN void resetSeedFail(void);
#endif


#endif /* __RNG_SEED_HEADER__ */

