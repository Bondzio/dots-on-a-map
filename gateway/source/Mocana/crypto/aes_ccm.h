/* Version: mss_v6_3 */
/*
 * aes_ccm.h
 *
 * AES-CCM Implementation
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */




/*------------------------------------------------------------------*/

#ifndef __AES_CCM_HEADER__
#define __AES_CCM_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------*/

/*  Function prototypes  */
/* AES Counter with CBC-MAC (CCM) -- cf RFC 3610 for explanations of parameters. encryption is in place */
/* the U buffer must be M bytes long */

/* AES Counter with CBC-MAC (CCM) encrypts and protects a data buffer. */
MOC_EXTERN MSTATUS  AESCCM_encrypt(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte M, ubyte L, ubyte* keyMaterial, sbyte4 keyLength,
                                    const ubyte* nonce, ubyte* encData, ubyte4 eDataLength,
                                    const ubyte* authData, ubyte4 aDataLength, ubyte U[/*M*/]);

/* AES Counter with CBC-MAC (CCM) decrypt and authenticates a data buffer. */
MOC_EXTERN MSTATUS  AESCCM_decrypt(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte M, ubyte L, ubyte* keyMaterial, sbyte4 keyLength,
                                    const ubyte* nonce, ubyte* encData, ubyte4 eDataLength,
                                    const ubyte* authData, ubyte4 aDataLength, const ubyte U[/*M*/]);

MOC_EXTERN BulkCtx AESCCM_createCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, sbyte4 keylen, sbyte4 encrypt);

MOC_EXTERN MSTATUS AESCCM_deleteCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);

MOC_EXTERN MSTATUS AESCCM_cipher(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* nonce, ubyte4 nlen, ubyte* aData, ubyte4 aDataLength, ubyte* data, ubyte4 dataLength, ubyte4 verifyLen, sbyte4 encrypt);

#ifdef __cplusplus
}
#endif

#endif /* __AES_CCM_HEADER__ */

