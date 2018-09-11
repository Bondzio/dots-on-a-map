/* Version: mss_v6_3 */
/*
 * sha256.h
 *
 * SHA - Secure Hash Algorithm Header
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file sha256.h SHA256 developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for SHA256 operations.

\since 1.41
\version 3.06 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- SHA224_allocDigest
- SHA224_completeDigest
- SHA224_finalDigest
- SHA224_freeDigest
- SHA224_initDigest
- SHA224_updateDigest
- SHA256_allocDigest
- SHA256_completeDigest
- SHA256_finalDigest
- SHA256_freeDigest
- SHA256_initDigest
- SHA256_updateDigest

*/


/*------------------------------------------------------------------*/

#ifndef __SHA256_HEADER__
#define __SHA256_HEADER__


#ifdef __cplusplus
extern "C" {
#endif


#define SHA256_RESULT_SIZE    (32)
#define SHA256_BLOCK_SIZE     (64)

#define SHA224_RESULT_SIZE    (28)
#define SHA224_BLOCK_SIZE     (64)

/*------------------------------------------------------------------*/

typedef struct SW_SHA256_CTX
{
    ubyte4  hashBlocks[8];

    ubyte8  mesgLength;

    sbyte4  hashBufferIndex;
    ubyte   hashBuffer[SHA256_BLOCK_SIZE];

#ifdef __ENABLE_MOCANA_MINIMUM_STACK__
    ubyte4  W[64];
#endif
} SW_SHA256_CTX, SHA224_CTX;

#ifndef __CUSTOM_SHA256_CONTEXT__
typedef struct SW_SHA256_CTX      sha256Descr, sha256DescrHS, SHA256_CTX;
#endif

/*------------------------------------------------------------------*/

/* single steps */
#ifndef __DISABLE_MOCANA_SHA256__

MOC_EXTERN MSTATUS SHA256_allocDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);

MOC_EXTERN MSTATUS SHA256_freeDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);

MOC_EXTERN MSTATUS SHA256_initDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) sha256Descr *pCtx);

MOC_EXTERN MSTATUS SHA256_updateDigest  (MOC_HASH(hwAccelDescr hwAccelCtx) sha256Descr *pCtx, const ubyte *pData, ubyte4 dataLen);

MOC_EXTERN MSTATUS SHA256_finalDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) sha256Descr *pCtx, ubyte *pOutput);

/* all at once hash */

MOC_EXTERN MSTATUS SHA256_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pShaOutput);
#endif /* __DISABLE_MOCANA_SHA256__ */

#ifdef __SHA256_HARDWARE_HASH__
MOC_EXTERN MSTATUS SHA256_initDigestHandShake   (MOC_HASH(hwAccelDescr hwAccelCtx) sha256DescrHS *p_shaContext);
MOC_EXTERN MSTATUS SHA256_updateDigestHandShake (MOC_HASH(hwAccelDescr hwAccelCtx) sha256DescrHS *p_shaContext, const ubyte *pData, ubyte4 dataLen);
MOC_EXTERN MSTATUS SHA256_finalDigestHandShake  (MOC_HASH(hwAccelDescr hwAccelCtx) sha256DescrHS *p_shaContext, ubyte *pShaOutput);
#else
#define SHA256_initDigestHandShake      SHA256_initDigest
#define SHA256_updateDigestHandShake    SHA256_updateDigest
#define SHA256_finalDigestHandShake     SHA256_finalDigest
#endif /* __SHA256_HARDWARE_HASH__ */

#ifndef __DISABLE_MOCANA_SHA224__
MOC_EXTERN MSTATUS SHA256_allocDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);
MOC_EXTERN MSTATUS SHA256_freeDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);
#define SHA224_allocDigest  SHA256_allocDigest
#define SHA224_freeDigest   SHA256_freeDigest
MOC_EXTERN MSTATUS SHA224_initDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) SHA224_CTX *pCtx);
MOC_EXTERN MSTATUS SHA256_updateDigest  (MOC_HASH(hwAccelDescr hwAccelCtx) SHA256_CTX *pCtx, const ubyte *pData, ubyte4 dataLen);
#define SHA224_updateDigest SHA256_updateDigest
MOC_EXTERN MSTATUS SHA224_finalDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) SHA224_CTX *pCtx, ubyte *pOutput);

MOC_EXTERN MSTATUS SHA224_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pShaOutput);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SHA256_HEADER__ */
