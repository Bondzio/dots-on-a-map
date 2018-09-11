/* Version: mss_v6_3 */
/*
 * md4.h
 *
 * Message Digest 4 (MD4) Header
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


#ifndef __MD4_HEADER__
#define __MD4_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define MD4_DIGESTSIZE      16
#define MD4_RESULT_SIZE     16   /* synonym */

#define MD4_BLOCK_SIZE      (64)

/*------------------------------------------------------------------*/

/* MD4 context. */
typedef struct MD4_CTX {
  ubyte4 state[4];                                   /* state (ABCD) */
  ubyte4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  ubyte buffer[MD4_BLOCK_SIZE];              /* input buffer */
} MD4_CTX;


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_MD4__
MSTATUS MD4Alloc(MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);
MSTATUS MD4Free(MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);
MSTATUS MD4Init(MOC_HASH(hwAccelDescr hwAccelCtx) MD4_CTX* pCtx);
MSTATUS MD4Update(MOC_HASH(hwAccelDescr hwAccelCtx) MD4_CTX* pCtx, const ubyte* pData, ubyte4 dataLen);
MSTATUS MD4Final(MOC_HASH(hwAccelDescr hwAccelCtx) MD4_CTX* pCtx, ubyte output[MD4_DIGESTSIZE]);

/* all at once hash */
MOC_EXTERN MSTATUS MD4_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pOutput);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MD4_HEADER__ */
