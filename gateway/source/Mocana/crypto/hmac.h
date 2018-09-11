/* Version: mss_v6_3 */
/*
 * hmac.h
 *
 * Hash Message Authentication Code
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file hmac.h HMAC developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for HMAC (keyed-hash message authentication code) operations.

\since 1.41
\version 5.3 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- HMAC_MD5
- HMAC_MD5_quick
- HMAC_SHA1
- HMAC_SHA1_quick
- HmacCreate
- HmacDelete
- HmacFinal
- HmacKey
- HmacQuick
- HmacQuickEx
- HmacReset
- HmacUpdate

\note Not all functions are available in all versions of the Cryptographic
API. If a function is not available, it is listed but not hyperlinked to any
documentation.

*/


/*------------------------------------------------------------------*/

#ifndef __HMAC_H__
#define __HMAC_H__


#ifdef __cplusplus
extern "C" {
#endif

#if defined( __DISABLE_MOCANA_SHA512__) && defined(__DISABLE_MOCANA_SHA384__)
#define HMAC_BLOCK_SIZE            (64)  /* Maximum Hash Block Size = MD5_BLOCK_SIZE = SHA1_BLOCK_SIZE  = SHA256_BLOCK_SIZE */
#else
#define HMAC_BLOCK_SIZE            (128) /* Maximum Hash Block Size = SHA512_BLOCK_SIZE */
#endif

/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
HMAC_MD5(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen, const ubyte* text,
            sbyte4 textLen, const ubyte* textOpt, sbyte4 textOptLen, ubyte result[MD5_DIGESTSIZE]);

MOC_EXTERN MSTATUS
HMAC_SHA1(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen,
          const ubyte* text, sbyte4 textLen,
          const ubyte* textOpt, sbyte4 textOptLen, ubyte result[SHA_HASH_RESULT_SIZE]);

MOC_EXTERN MSTATUS
HMAC_SHA1Ex(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen,
                    const ubyte* texts[], sbyte4 textLens[],
                     sbyte4 numTexts, ubyte result[SHA_HASH_RESULT_SIZE]);

MOC_EXTERN MSTATUS HMAC_MD5_quick(MOC_HASH(hwAccelDescr hwAccelCtx)  const ubyte* pKey, sbyte4 keyLen,
                                  const ubyte* pText, sbyte4 textLen, ubyte* pResult /* MD5_DIGESTSIZE */);

MOC_EXTERN MSTATUS HMAC_SHA1_quick(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
                                   const ubyte* pText, sbyte4 textLen, ubyte* pResult /* SHA_HASH_RESULT_SIZE */);


/*------------------------------------------------------------------*/

/* HMAC context. */
struct HMAC_CTX
{
    const BulkHashAlgo* pBHAlgo;        /* external pointer, not a copy */
    BulkCtx             hashCtxt;

    ubyte4              keyLen;
    ubyte               key[HMAC_BLOCK_SIZE];
    ubyte               kpad[HMAC_BLOCK_SIZE];

};

typedef struct HMAC_CTX HMAC_CTX;
typedef struct HMAC_CTX _moc_HMAC_CTX;  /* Needed for openssl crypto engine */

/*------------------------------------------------------------------*/

/* single steps */

MOC_EXTERN MSTATUS HmacCreate(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX **pctx, const BulkHashAlgo *pBHAlgo);

MOC_EXTERN MSTATUS HmacKey(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx, const ubyte *key, ubyte4 keyLen);

MOC_EXTERN MSTATUS HmacReset(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx);

MOC_EXTERN MSTATUS HmacUpdate(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx, const ubyte *data, ubyte4 dataLen);

MOC_EXTERN MSTATUS HmacFinal(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX *ctx, ubyte *result);

MOC_EXTERN MSTATUS HmacDelete(MOC_HASH(hwAccelDescr hwAccelCtx) HMAC_CTX **pctx);

MOC_EXTERN MSTATUS HmacQuick(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
                             const ubyte* pText, sbyte4 textLen,
                             ubyte* pResult, const BulkHashAlgo *pBHAlgo);

MOC_EXTERN MSTATUS HmacQuicker(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
                               const ubyte* pText, sbyte4 textLen,
                               ubyte* pResult, const BulkHashAlgo *pBHAlgo, 
                               HMAC_CTX *ctx);

MOC_EXTERN MSTATUS HmacQuickEx(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
                               const ubyte* pText, sbyte4 textLen,
                               const ubyte* pOptText, ubyte4 optTextLen,
                               ubyte* pResult, const BulkHashAlgo *pBHAlgo);

MOC_EXTERN MSTATUS HmacQuickerEx(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
                                 const ubyte* pText, sbyte4 textLen,
                                 const ubyte* pOptText, ubyte4 optTextLen,
                                 ubyte* pResult, const BulkHashAlgo *pBHAlgo,
                                 HMAC_CTX *ctx);

MOC_EXTERN MSTATUS HmacQuickerInline(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pKey, sbyte4 keyLen,
          	  	  	  	  	  	  	 const ubyte* pText, sbyte4 textLen,
          	  	  	  	  	  	  	 ubyte* pResult, const BulkHashAlgo *pBHAlgo,
          	  	  	  	  	  	  	 BulkCtx* context);

MOC_EXTERN MSTATUS HmacQuickerInlineEx(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* key, sbyte4 keyLen,
         	 	 	 	 	 	 	   const ubyte* text, sbyte4 textLen,
         	 	 	 	 	 	 	   const ubyte* textOpt, sbyte4 textOptLen,
         	 	 	 	 	 	 	   ubyte* pResult, const BulkHashAlgo *pBHAlgo,
         	 	 	 	 	 	 	   BulkCtx* context);

#ifdef __cplusplus
}
#endif

#endif
