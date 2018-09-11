/* Version: mss_v6_3 */
/*
 * md2.h
 *
 * Message Digest 2 (MD2) Header
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


#ifndef __MD2_HEADER__
#define __MD2_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define MD2_DIGESTSIZE      16
#define MD2_RESULT_SIZE     16   /* synonym */

#define MD2_BLOCK_SIZE      (16)

/*------------------------------------------------------------------*/

typedef struct MD2_CTX {
  ubyte     state[16];
  ubyte     checksum[16];
  ubyte4    count;
  ubyte     buffer[MD2_BLOCK_SIZE];
} MD2_CTX;


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_MD2__
MOC_EXTERN MSTATUS MD2Alloc(MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);
MOC_EXTERN MSTATUS MD2Free(MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context);
MOC_EXTERN MSTATUS MD2Init(MOC_HASH(hwAccelDescr hwAccelCtx) MD2_CTX *);
MOC_EXTERN MSTATUS MD2Update(MOC_HASH(hwAccelDescr hwAccelCtx) MD2_CTX *, const ubyte *, ubyte4);
MOC_EXTERN MSTATUS MD2Final(MOC_HASH(hwAccelDescr hwAccelCtx) MD2_CTX *, ubyte [MD2_DIGESTSIZE]);

/* all at once hash */
MOC_EXTERN MSTATUS MD2_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pOutput);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MD2_HEADER__ */
