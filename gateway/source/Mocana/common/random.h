/* Version: mss_v6_3 */
/*
 * random.h
 *
 * Random Number FIPS-186 Factory
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */
/* \file random.h Random Number Generator (RNG) API header.
This header file contains definitions, enumerations, structures, and function
declarations used by the Random Number Generator Module.

\since 1.41
\version 3.06 and later

! Flags
Whether the following flag is defined determines which functions
are declared:
$__DISABLE_MOCANA_ADD_ENTROPY__$

! External Functions
This file contains the following public ($extern$) function declarations:
- RANDOM_acquireContext
- RANDOM_addEntropyBit
- RANDOM_newFIPS186Context
- RANDOM_numberGenerator
- RANDOM_releaseContext
- RANDOM_rngFun
*/


/*------------------------------------------------------------------*/

#ifndef __RANDOM_HEADER__
#define __RANDOM_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define RANDOM_CONTEXT(X)       (X)->pRandomContext


/*------------------------------------------------------------------*/

typedef void            randomContext;
/*!
\exclude
*/
typedef sbyte4          (*RNGFun)(void* rngFunArg, ubyte4 length, ubyte *buffer);
MOC_EXTERN randomContext*  g_pRandomContext;

/*------------------------------------------------------------------*/
/*  RNG Entropy source */
#define ENTROPY_SRC_INTERNAL 0         /* Internal entropy threads will be used */
#define ENTROPY_SRC_EXTERNAL 1         /* External entropy will be added to Random contexts */

#ifndef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
#define ENTROPY_DEFAULT_SRC  ENTROPY_SRC_INTERNAL
#else
#define ENTROPY_DEFAULT_SRC  ENTROPY_SRC_EXTERNAL
#endif

/*------------------------------------------------------------------*/
/*  RNG Algorithm Defines algoId */
#define MODE_RNG_ANY         0         /* Any random number generator will do */
#define MODE_RNG_FIPS186     1         /* Use FIPS186 RNG */
#define MODE_DRBG_CTR        3         /* Use DRBG CTR Mode  */

#define RANDOM_DEFAULT_ALGO  MODE_DRBG_CTR  /* Must be one of the above (FIPS186 or CTR) */

/*------------------------------------------------------------------*/
MOC_EXTERN MSTATUS RANDOM_acquireGlobalContext(randomContext **pp_randomContext);
MOC_EXTERN MSTATUS RANDOM_releaseGlobalContext(randomContext **pp_randomContext);

MOC_EXTERN MSTATUS RANDOM_acquireContext(randomContext **pp_randomContext);

MOC_EXTERN MSTATUS RANDOM_acquireContextEx(randomContext **pp_randomContext, ubyte algoId);

MOC_EXTERN MSTATUS RANDOM_releaseContext (randomContext **pp_randomContext);

#ifndef __DISABLE_MOCANA_ADD_ENTROPY__
MOC_EXTERN MSTATUS RANDOM_addEntropyBit(randomContext *pRandomContext, ubyte entropyBit);
#endif /*__DISABLE_MOCANA_ADD_ENTROPY__*/

MOC_EXTERN MSTATUS RANDOM_numberGenerator(randomContext *pRandomContext, ubyte *pBuffer, sbyte4 bufSize);

MOC_EXTERN sbyte4 RANDOM_rngFun(void* rngFunArg, ubyte4 length, ubyte *buffer);

MOC_EXTERN MSTATUS RANDOM_setEntropySource(ubyte EntropySrc);
MOC_EXTERN ubyte RANDOM_getEntropySource(void);

/* FIPS 186 specific functions */

MOC_EXTERN MSTATUS RANDOM_KSrcGenerator(randomContext *pRandomContext, ubyte buffer[40]);
MOC_EXTERN MSTATUS RANDOM_newFIPS186Context( randomContext **ppRandomContext, ubyte b, const ubyte pXKey[/*b*/], sbyte4 seedLen, const ubyte pXSeed[/*seedLen*/]);
MOC_EXTERN MSTATUS RANDOM_deleteFIPS186Context( randomContext **ppRandomContext);
MOC_EXTERN MSTATUS RANDOM_numberGeneratorFIPS186(randomContext *pRandomContext, ubyte *pRetRandomBytes, sbyte4 numRandomBytes);

#ifdef __FIPS_OPS_TEST__
MOC_EXTERN void triggerRNGFail(void);
MOC_EXTERN void resetRNGFail(void);
#endif


#ifdef __cplusplus
}
#endif

#endif /* __RANDOM_HEADER__ */
