/* Version: mss_v6_3 */
/*
 * sha1.h
 *
 * SHA - Secure Hash Algorithm Header
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file sha1.h SHA1 developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for SHA1 operations.

\since 1.41
\version 3.06 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- SHA1_allocDigest
- SHA1_completeDigest
- SHA1_finalDigest
- SHA1_freeDigest
- SHA1_initDigest
- SHA1_updateDigest

*/

/*------------------------------------------------------------------*/

#ifndef __SHA1_HEADER__
#define __SHA1_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define SHA_HASH_RESULT_SIZE    (20)
#define SHA_HASH_BLOCK_SIZE     (64)
/* conventions */
#define SHA1_RESULT_SIZE        (20)
#define SHA1_BLOCK_SIZE         (64)


/*------------------------------------------------------------------*/

typedef struct SW_SHA1_CTX
{
    ubyte4  hashBlocks[5];

    ubyte8  mesgLength;

    sbyte4  hashBufferIndex;
    ubyte   hashBuffer[SHA1_BLOCK_SIZE];

#ifdef __ENABLE_MOCANA_MINIMUM_STACK__
    ubyte4 W[80];
#endif

} SW_SHA1_CTX;

#ifndef __CUSTOM_SHA1_CONTEXT__
typedef struct SW_SHA1_CTX      shaDescr, shaDescrHS, SHA1_CTX;
#endif


/*------------------------------------------------------------------*/

/* single steps */

MOC_EXTERN MSTATUS SHA1_allocDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);

MOC_EXTERN MSTATUS SHA1_freeDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);

MOC_EXTERN MSTATUS SHA1_initDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) shaDescr *p_shaContext);

MOC_EXTERN MSTATUS SHA1_updateDigest  (MOC_HASH(hwAccelDescr hwAccelCtx) shaDescr *p_shaContext, const ubyte *pData, ubyte4 dataLen);

MOC_EXTERN MSTATUS SHA1_finalDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) shaDescr *p_shaContext, ubyte *pShaOutput);

/* all at once hash */

MOC_EXTERN MSTATUS SHA1_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pShaOutput);


#if (!(defined(__DISABLE_MOCANA_RNG__)))
/* G function used for FIPS 186 RNG change notice section 1 */
MOC_EXTERN MSTATUS SHA1_G(ubyte *pData, ubyte *pShaOutput);
/* G function used for FIPS 186 RNG change notice section 2 */
MOC_EXTERN MSTATUS SHA1_GK(ubyte *pData, ubyte *pShaOutput);
#endif

#ifdef __SHA1_HARDWARE_HASH__
MOC_EXTERN MSTATUS SHA1_initDigestHandShake    (MOC_HASH(hwAccelDescr hwAccelCtx) shaDescrHS *p_shaContext);
MOC_EXTERN MSTATUS SHA1_updateDigestHandShake  (MOC_HASH(hwAccelDescr hwAccelCtx) shaDescrHS*p_shaContext, const ubyte *pData, ubyte4 dataLen);
MOC_EXTERN MSTATUS SHA1_finalDigestHandShake   (MOC_HASH(hwAccelDescr hwAccelCtx) shaDescrHS *p_shaContext, ubyte *pShaOutput);

#else
#define SHA1_initDigestHandShake    SHA1_initDigest
#define SHA1_updateDigestHandShake  SHA1_updateDigest
#define SHA1_finalDigestHandShake   SHA1_finalDigest
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SHA1_HEADER__ */
