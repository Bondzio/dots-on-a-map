/* Version: mss_v6_3 */
/*
 * sha512.h
 *
 * SHA - Secure Hash Algorithm Header
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file sha512.h SHA512 developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for SHA512 operations.

\since 1.41
\version 3.06 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- SHA384_allocDigest
- SHA384_completeDigest
- SHA384_finalDigest
- SHA384_freeDigest
- SHA384_initDigest
- SHA384_updateDigest
- SHA512_allocDigest
- SHA512_completeDigest
- SHA512_finalDigest
- SHA512_freeDigest
- SHA512_initDigest
- SHA512_updateDigest

*/


/*------------------------------------------------------------------*/

#ifndef __SHA512_HEADER__
#define __SHA512_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define SHA512_RESULT_SIZE    (64)
#define SHA512_BLOCK_SIZE     (128)

#define SHA384_RESULT_SIZE    (48)
#define SHA384_BLOCK_SIZE     (128)


/*------------------------------------------------------------------*/

typedef struct SHA512_CTX
{
    ubyte8  hashBlocks[8];

    ubyte16  msgLength;

    sbyte4  hashBufferIndex;
    ubyte   hashBuffer[SHA512_BLOCK_SIZE];

#ifdef __ENABLE_MOCANA_MINIMUM_STACK__
    ubyte8  W[80];
#endif

} SHA512_CTX, SHA384_CTX;


/*------------------------------------------------------------------*/

/* single steps */
#ifndef __DISABLE_MOCANA_SHA512__

MOC_EXTERN MSTATUS SHA512_allocDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);

MOC_EXTERN MSTATUS SHA512_freeDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);

MOC_EXTERN MSTATUS SHA512_initDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) SHA512_CTX *pCtx);

MOC_EXTERN MSTATUS SHA512_updateDigest  (MOC_HASH(hwAccelDescr hwAccelCtx) SHA512_CTX *pCtx, const ubyte *pData, ubyte4 dataLen);

MOC_EXTERN MSTATUS SHA512_finalDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) SHA512_CTX *pCtx, ubyte *pOutput);
/* all at once hash */

MOC_EXTERN MSTATUS SHA512_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pShaOutput);
#endif

#ifndef __DISABLE_MOCANA_SHA384__
MOC_EXTERN MSTATUS SHA512_allocDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);
MOC_EXTERN MSTATUS SHA512_freeDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_shaContext);
#define SHA384_allocDigest  SHA512_allocDigest
#define SHA384_freeDigest   SHA512_freeDigest
MOC_EXTERN MSTATUS SHA384_initDigest    (MOC_HASH(hwAccelDescr hwAccelCtx) SHA384_CTX *pCtx);
MOC_EXTERN MSTATUS SHA512_updateDigest  (MOC_HASH(hwAccelDescr hwAccelCtx) SHA512_CTX *pCtx, const ubyte *pData, ubyte4 dataLen);
#define SHA384_updateDigest SHA512_updateDigest
MOC_EXTERN MSTATUS SHA384_finalDigest   (MOC_HASH(hwAccelDescr hwAccelCtx) SHA384_CTX *pCtx, ubyte *pOutput);

MOC_EXTERN MSTATUS SHA384_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pShaOutput);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SHA512_HEADER__ */
