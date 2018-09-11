/* Version: mss_v6_3 */
/*
 * nist_rng.h
 *
 * Implementation of the RNGs described in NIST 800-90
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*! \file nist_rng.h NIST RNG developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for NIST RNG constructions as described in NIST 800-90.

\since 3.0.6
\version 5.0.5 and later

! Flags
No flag definitions are required to use this file.

! External Functions
*/


/*------------------------------------------------------------------*/

#ifndef __NIST_RNG_HEADER__
#define __NIST_RNG_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------*/

/*  Function prototypes  */
    /* CTR_DRBG */

MOC_EXTERN MSTATUS NIST_CTRDRBG_newContext(MOC_SYM(hwAccelDescr hwAccelCtx)
                            randomContext **ppNewContext,
                            const ubyte* entropyInput,
                            ubyte4 keyLenBytes, ubyte4 outLenBytes,
                            const ubyte* personalization,
                            ubyte4 personalizationLen);

MOC_EXTERN MSTATUS NIST_CTRDRBG_newDFContext(MOC_SYM(hwAccelDescr hwAccelCtx)
                            randomContext **ppNewContext,
                            ubyte4 keyLenBytes, ubyte4 outLenBytes,
                            const ubyte* entropyInput, ubyte4 entropyInputLen,
                            const ubyte* nonce, ubyte4 nonceLen,
                            const ubyte* personalization,
                            ubyte4 personalizationLen);

MOC_EXTERN MSTATUS NIST_CTRDRBG_deleteContext( MOC_SYM(hwAccelDescr hwAccelCtx)
                                               randomContext **ppNewContext);

MOC_EXTERN MSTATUS NIST_CTRDRBG_reseed(MOC_SYM(hwAccelDescr hwAccelCtx)
                            randomContext *pContext,
                            const ubyte* entropyInput,
                            ubyte4 entropyInputLen,
                            const ubyte* additionalInput,
                            ubyte4 additionalInputLen);

MOC_EXTERN MSTATUS NIST_CTRDRBG_generate(MOC_SYM(hwAccelDescr hwAccelCtx)
                                        randomContext* pContext,
                                        const ubyte* additionalInput, ubyte4 additionalInputLen,
                                        ubyte* output, ubyte4 outputLenBits);

/* canonical interface to random number generator */
MOC_EXTERN MSTATUS NIST_CTRDRBG_numberGenerator(MOC_SYM(hwAccelDescr hwAccelCtx)
                                        randomContext *pRandomContext,
                                        ubyte *pBuffer, sbyte4 bufSize);

MOC_EXTERN sbyte4 NIST_CTRDRBG_rngFun(MOC_SYM(hwAccelDescr hwAccelCtx)
                                        void* rngFunArg,
                                        ubyte4 length, ubyte *buffer);


#ifdef __FIPS_OPS_TEST__

MOC_EXTERN void triggerDRBGFail(void);
MOC_EXTERN void resetDRBGFail(void);

#endif


#ifdef __cplusplus
}
#endif

#endif /* __NIST_RNG_HEADER__ */

