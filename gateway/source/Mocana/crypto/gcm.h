/* Version: mss_v6_3 */
/*
 * gcm.h
 *
 * GCM Implementation
 *
 * Copyright Mocana Corp 2008-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 * Code derived from public domain code on www.zork.org
 */

/* \file gcm.h GCM developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for GCM encryption and decryption.

\since 3.0.6
\version 5.0.5 and later

! Flags
To enable the functions in this header file, at least one of the following flags
must be defined in moptions.h:
- $__ENABLE_MOCANA_GCM__$
- $__ENABLE_MOCANA_GCM_64K__$
- $__ENABLE_MOCANA_GCM_4K__$
- $__ENABLE_MOCANA_GCM_256B__$

! External Functions
This file contains the following public ($extern$) function declarations:
- GCM_createCtx_256b
- GCM_deleteCtx_256b
- GCM_init_256b
- GCM_update_encrypt_256b
- GCM_update_decrypt_256b
- GCM_final_256b
- GCM_cipher_256b

*/


/*------------------------------------------------------------------*/

#ifndef __GCM_HEADER__
#define __GCM_HEADER__

#if defined(__ENABLE_MOCANA_GCM__)
#if !defined(__ENABLE_MOCANA_GCM_64K__) && !defined(__ENABLE_MOCANA_GCM_4K__) && !defined(__ENABLE_MOCANA_GCM_256B__)
#define __ENABLE_MOCANA_GCM_256B__   /*default implementation*/
#endif
#endif

#if defined(__ENABLE_MOCANA_GCM_64K__) || defined(__ENABLE_MOCANA_GCM_4K__)  || defined(__ENABLE_MOCANA_GCM_256B__)

#ifndef __ENABLE_MOCANA_GCM__
#define __ENABLE_MOCANA_GCM__
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ENABLE_MOCANA_GCM_64K__

/* 64K -> fastest with big memory usage */

#define GCM_I_LIMIT      (16)
#define GCM_J_LIMIT      (0x100)

typedef struct gcm_ctx_64k {
    ubyte4            table[GCM_I_LIMIT][GCM_J_LIMIT][4];
    ubyte4            tag4[4];
    ubyte4            s[4];
    sbyte4            hashBufferIndex;
    ubyte             hashBuffer[AES_BLOCK_SIZE];
    ubyte4            alen;
    ubyte4            dlen;
    AES_CTR_Ctx       ctx;
} gcm_ctx_64k;

MOC_EXTERN BulkCtx GCM_createCtx_64k(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, sbyte4 keylen, sbyte4 encrypt);

MOC_EXTERN MSTATUS GCM_deleteCtx_64k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);

MOC_EXTERN MSTATUS GCM_init_64k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx,
                            ubyte* nonce, ubyte4 nlen,
                            ubyte* adata, ubyte4 alen);

MOC_EXTERN MSTATUS GCM_update_encrypt_64k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte *data, ubyte4 dlen);

MOC_EXTERN MSTATUS GCM_update_decrypt_64k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte *ct, ubyte4 ctlen);

MOC_EXTERN MSTATUS GCM_final_64k( BulkCtx ctx, ubyte tag[/*AES_BLOCK_SIZE*/]);

MOC_EXTERN MSTATUS GCM_cipher_64k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx,
                            ubyte* nonce, ubyte4 nlen,
                            ubyte* adata, ubyte4 alen,
                            ubyte* data, ubyte4 dlen, ubyte4 verifyLen, sbyte4 encrypt);

#endif /* __ENABLE_MOCANA_GCM_64K__ */

#ifdef __ENABLE_MOCANA_GCM_4K__

/* 4K -> intermediate */
typedef struct gcm_ctx_4k {
    ubyte4            table[256][4];
    ubyte4            tag4[4];
    ubyte4            s[4];
    sbyte4            hashBufferIndex;
    ubyte             hashBuffer[AES_BLOCK_SIZE];
    ubyte4            alen;
    ubyte4            dlen;
    AES_CTR_Ctx       ctx;
} gcm_ctx_4k;

MOC_EXTERN BulkCtx GCM_createCtx_4k(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, sbyte4 keylen, sbyte4 encrypt);

MOC_EXTERN MSTATUS GCM_deleteCtx_4k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);

MOC_EXTERN MSTATUS GCM_init_4k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx,
                            ubyte* nonce, ubyte4 nlen,
                            ubyte* adata, ubyte4 alen);

MOC_EXTERN MSTATUS GCM_update_encrypt_4k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte *data, ubyte4 dlen);

MOC_EXTERN MSTATUS GCM_update_decrypt_4k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte *ct, ubyte4 ctlen);

MOC_EXTERN MSTATUS GCM_final_4k( BulkCtx ctx, ubyte tag[/*AES_BLOCK_SIZE*/]);

MOC_EXTERN MSTATUS GCM_cipher_4k(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx,
                            ubyte* nonce, ubyte4 nlen,
                            ubyte* adata, ubyte4 alen,
                            ubyte* data, ubyte4 dlen, ubyte4 verifyLen, sbyte4 encrypt);

#endif  /* __ENABLE_MOCANA_GCM_4K__ */

#ifdef __ENABLE_MOCANA_GCM_256B__

/* 256b -> slowest, less memory usage */

typedef struct gcm_ctx_256b {
    ubyte4            table[16][4];
    ubyte4            tag4[4];
    ubyte4            s[4];
    sbyte4            hashBufferIndex;
    ubyte             hashBuffer[AES_BLOCK_SIZE];
    ubyte4            alen;
    ubyte4            dlen;
    AES_CTR_Ctx       ctx;
} gcm_ctx_256b;


/* AES Galois/Counter Mode (GCM) provide both authentication and privacy. Defined in NIST Special Publication 800-38D. */
MOC_EXTERN BulkCtx GCM_createCtx_256b(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, sbyte4 keylen, sbyte4 encrypt);

/* AES Galois/Counter Mode (GCM) provide both authentication and privacy. Defined in NIST Special Publication 800-38D.  */
MOC_EXTERN MSTATUS GCM_deleteCtx_256b(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);

/* AES Galois/Counter Mode (GCM) provide both authentication and privacy.*/
MOC_EXTERN MSTATUS GCM_init_256b(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx,
                            ubyte* nonce, ubyte4 nlen,
                            ubyte* adata, ubyte4 alen);

/* AES Galois/Counter Mode (GCM) provide both authentication and privacy.*/
MOC_EXTERN MSTATUS GCM_update_encrypt_256b(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte *data, ubyte4 dlen);

/* AES Galois/Counter Mode (GCM) provide both authentication and privacy. */
MOC_EXTERN MSTATUS GCM_update_decrypt_256b(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte *ct, ubyte4 ctlen);

/* AES Galois/Counter Mode (GCM) provide both authentication and privacy. */
MOC_EXTERN MSTATUS GCM_final_256b( BulkCtx ctx, ubyte tag[/*AES_BLOCK_SIZE*/]);

/* AES Galois/Counter Mode (GCM) provide both authentication and privacy. */
MOC_EXTERN MSTATUS GCM_cipher_256b(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx,
                            ubyte* nonce, ubyte4 nlen,
                            ubyte* adata, ubyte4 alen,
                            ubyte* data, ubyte4 dlen, ubyte4 verifyLen, sbyte4 encrypt);

#endif /* __ENABLE_MOCANA_GCM_256B__ */

#ifdef __cplusplus
}
#endif

#endif /* defined(__ENABLE_MOCANA_GCM_64K__) || defined(__ENABLE_MOCANA_GCM_4K__)  || defined(__ENABLE_MOCANA_GCM_256B__) */

#endif /* __GCM_HEADER__ */

