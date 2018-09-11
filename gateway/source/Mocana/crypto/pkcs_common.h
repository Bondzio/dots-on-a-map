/* Version: mss_v6_3 */
/*
 * pkcs_common.h
 *
 * Routines shared by PKCS implementations
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PKCS_COMMON_HEADER__
#define __PKCS_COMMON_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NORMAL=0, SCEP=1
} encryptedContentType;

/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS PKCS_BulkDecrypt(MOC_SYM(hwAccelDescr hwAccelCtx)
                                ASN1_ITEM* pEncryptedContent,
                                CStream s,
                                BulkCtx bulkCtx,
                                const BulkEncryptionAlgo* pBulkAlgo,
                                ubyte* iv,
                                ubyte** decryptedInfo,
                                sbyte4* decryptedInfoLen);

MOC_EXTERN MSTATUS PKCS_BulkDecryptEx(MOC_SYM(hwAccelDescr hwAccelCtx)
                                encryptedContentType type,
                                ASN1_ITEM* pEncryptedContent,
                                CStream s,
                                BulkCtx bulkCtx,
                                const BulkEncryptionAlgo* pBulkAlgo,
                                ubyte* iv,
                                ubyte** decryptedInfo,
                                sbyte4* decryptedInfoLen);

#if defined(__ENABLE_MOCANA_PKCS5__) || defined(__ENABLE_MOCANA_PKCS12__)
MOC_EXTERN MSTATUS PKCS_DecryptPKCS8Key( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                            ASN1_ITEMPTR pEncryptedKeyInfo,
                            CStream s, const ubyte* uniPassword,
                            sbyte4 uniPassLen, ubyte** privateKeyInfo,
                            sbyte4* privateKeyInfoLen);
#endif


MOC_EXTERN MSTATUS PKCS_GetCBCParams( ASN1_ITEM* pAlgoOID, CStream s,
                                     ubyte blockSize, ubyte iv[16]);

#ifdef __ENABLE_ARC2_CIPHERS__
MOC_EXTERN MSTATUS PKCS_GetRC2CBCParams( ASN1_ITEM* pAlgoOID,
                               CStream s,
                               sbyte4* pEffectiveKeyBits,
                               ubyte iv[/*RC2_BLOCK_SIZE*/]);

#endif

#ifdef __cplusplus
}
#endif

#endif  /*#ifndef __PKCS_COMMON_HEADER__ */

