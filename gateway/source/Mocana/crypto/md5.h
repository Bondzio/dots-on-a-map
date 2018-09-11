/* Version: mss_v6_3 */
/*
 * md5.h
 *
 * MD5 Message Digest Algorithm
 *
 * Copyright Mocana Corp 2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __MD5_HEADER__
#define __MD5_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define MD5_DIGESTSIZE      (16)
#define MD5_RESULT_SIZE     (16)    /* synonym */
#define MD5_BLOCK_SIZE      (64)

/* MD5 context. */
#ifndef __CUSTOM_MD5_CONTEXT__

typedef struct MD5_CTX
{
    ubyte4  hashBlocks[4];
    ubyte8  mesgLength;

    sbyte4  hashBufferIndex;
    ubyte   hashBuffer[64];

#ifdef __ENABLE_MOCANA_MINIMUM_STACK__
    ubyte4  M[16];
#endif

} MD5_CTX, MD5_CTXHS;

#endif

MOC_EXTERN MSTATUS MD5Alloc_m        (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);
MOC_EXTERN MSTATUS MD5Free_m         (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);
MOC_EXTERN MSTATUS MD5Init_m         (MOC_HASH(hwAccelDescr hwAccelCtx) MD5_CTX *);
MOC_EXTERN MSTATUS MD5Update_m       (MOC_HASH(hwAccelDescr hwAccelCtx) MD5_CTX *, const ubyte *, ubyte4);
MOC_EXTERN MSTATUS MD5Final_m        (MOC_HASH(hwAccelDescr hwAccelCtx) MD5_CTX *, ubyte [MD5_DIGESTSIZE]);
MOC_EXTERN MSTATUS MD5_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pMdOutput);


#ifdef __MD5_HARDWARE_HASH__
MOC_EXTERN MSTATUS MD5init_HandShake  (MOC_HASH(hwAccelDescr hwAccelCtx) MD5_CTXHS*);
MOC_EXTERN MSTATUS MD5update_HandShake(MOC_HASH(hwAccelDescr hwAccelCtx) MD5_CTXHS*, const ubyte *, ubyte4);
MOC_EXTERN MSTATUS MD5final_HandShake (MOC_HASH(hwAccelDescr hwAccelCtx) MD5_CTXHS*, ubyte [MD5_DIGESTSIZE]);
#else

#define MD5init_HandShake   MD5Init_m
#define MD5update_HandShake MD5Update_m
#define MD5final_HandShake  MD5Final_m
#endif

#ifdef __cplusplus
}
#endif

#endif
