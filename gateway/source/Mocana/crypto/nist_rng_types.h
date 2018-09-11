/* Version: mss_v6_3 */
/*
 * nist_rng_types.h
 *
 * Definitions of the RNG data structures used for FIPS-186 and NIST 800-90
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*! \file nist_rng_types.h NIST RNG TYPES developer API header.
This header file contains definitions, enumerations, and structures used
for FIPS-186 and NIST RNG constructions as described in NIST 800-90.

\since 3.0.6
\version 5.0.5 and later

! Flags
No flag definitions are required to use this file.

! External Functions
*/


/*------------------------------------------------------------------*/

#ifndef __NIST_RNG_TYPES_HEADER__
#define __NIST_RNG_TYPES_HEADER__


#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*
 * FIPS-186 structure
 */
#define MOCANA_RNG_MAX_KEY_SIZE                     (64)

#ifndef MOCANA_RNG_DEFAULT_KEY_SIZE
#define MOCANA_RNG_DEFAULT_KEY_SIZE                 MOCANA_RNG_MAX_KEY_SIZE
#endif

#if ((20 > MOCANA_RNG_DEFAULT_KEY_SIZE) || (64 < MOCANA_RNG_DEFAULT_KEY_SIZE))
#error MOCANA_RNG_DEFAULT_KEY_SIZE out of range.
#endif


typedef struct rngFIPS186Ctx
{
    const ubyte*    pSeed;                              /* can be null */
    sbyte4          seedLen;
    ubyte           result[2 * SHA1_RESULT_SIZE];       /* result */
    ubyte           rngHistory[SHA1_RESULT_SIZE];       /* previously generated value--kept for FIPS test */
    sbyte           numBytesAvail;
    ubyte           b;                                  /* 20 <= b <= 64 */
    ubyte           scratch[MOCANA_RNG_MAX_KEY_SIZE];   /* messsage sent to SHA-1 */
    ubyte           key[MOCANA_RNG_MAX_KEY_SIZE];       /* real size = b */
    ubyte4          bitPos;
    RTOS_MUTEX      rngMutex;

} rngFIPS186Ctx;

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 * NIST SP-800-90 types and constants
 */


#ifdef __ENABLE_MOCANA_ECC__


#endif /* __ENABLE_MOCANA_ECC__ */

typedef MSTATUS (*BlockEncryptFunc)(MOC_SYM(hwAccelDescr hwAccelCtx)
                                    void* context, const ubyte* in,
                                    ubyte* out);

enum {
    e_NIST_RNG_use_df = 0x01
};

typedef struct NIST_CTR_DRBG_Ctx
{
    ubyte8 reseedCounter;
    ubyte4 outLenBytes;
    ubyte4 keyLenBytes;
    BlockEncryptFunc bef;
    union
    {
#if !defined(__DISABLE_3DES_CIPHERS__)
        ctx3des des;
#endif
        aesCipherContext aes;
    } ctx;
    ubyte flags;
    RTOS_MUTEX fipsMutex;
    ubyte* history;
    ubyte byteBuff[1];  /* V & KEY are added to this field */
} NIST_CTR_DRBG_Ctx;

#define V(Ctx)  ((Ctx)->byteBuff)
#define KEY(Ctx) (((Ctx)->byteBuff) + (Ctx)->outLenBytes)

#if !defined(__DISABLE_3DES_CIPHERS__)
#define IS_TDES(Ctx) (THREE_DES_BLOCK_SIZE == (Ctx)->outLenBytes)
#else
#define IS_TDES(Ctx) (0)
#endif

#define IS_AES(Ctx) (AES_BLOCK_SIZE == (Ctx)->outLenBytes)

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 * Common Random context data structure
 */
typedef enum {
    NIST_FIPS186=0, NIST_CTR_DRBG=2
} randomContextType;

typedef struct RandomCtxWrapper
{
    randomContextType WrappedCtxType;
    ubyte4 reseedBitCounter;
    union
    {
        NIST_CTR_DRBG_Ctx  CTR_Ctx;
        rngFIPS186Ctx FIPS186_Ctx;
    } WrappedCtx;
    /* Don't add any fields here, because CTR adds bytes on the end of byteBuff */

} RandomCtxWrapper;


#define IS_CTR_DRBG_CTX(wrap) \
    ((wrap->WrappedCtxType == NIST_CTR_DRBG))

#define GET_CTR_DRBG_CTX(wrap) \
    ((wrap->WrappedCtxType == NIST_CTR_DRBG) ? (&(wrap->WrappedCtx.CTR_Ctx)):(NULL))

#define IS_FIPS186_CTX(wrap) \
    ((wrap->WrappedCtxType == NIST_FIPS186))

#define GET_FIPS186_CTX(wrap) \
    ((wrap->WrappedCtxType == NIST_FIPS186) ? (&(wrap->WrappedCtx.FIPS186_Ctx)):(NULL))


/*---------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __NIST_RNG_TYPES_HEADER__ */

