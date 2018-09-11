/* Version: mss_v6_3 */
/*
 * pkcs5.h
 *
 * PKCS #5 Factory Header
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PPKCS5_HEADER__
#define __PPKCS5_HEADER__

enum hashFunc
{
    md2Encryption = 2,
    md4Encryption = 3,
    md5Encryption = 4,
    sha1Encryption = 5,
    sha256Encryption = 11,
    sha384Encryption = 12,
    sha512Encryption = 13,
    sha224Encryption = 14
};

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_PKCS5__

MOC_EXTERN const ubyte pkcs5_root_OID[];    /* 1.2.840.113549.1.5 */
MOC_EXTERN const ubyte pkcs5_PBKDF2_OID[];  /* 1.2.840.113549.1.5.12 */
MOC_EXTERN const ubyte pkcs5_PBES2_OID[];   /* 1.2.840.113549.1.5.13 */

MOC_EXTERN MSTATUS PKCS5_CreateKey_PBKDF1(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pSalt, ubyte4 saltLen,
                                          ubyte4 iterationCount, enum hashFunc hashingFunction,
                                          const ubyte *pPassword, ubyte4 passwordLen,
                                          ubyte4 dkLen, ubyte *pRetDerivedKey);
MOC_EXTERN MSTATUS PKCS5_CreateKey_PBKDF2(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pSalt, ubyte4 saltLen,
                                          ubyte4 iterationCount, ubyte rsaAlgoId,
                                          const ubyte *pPassword, ubyte4 passwordLen,
                                          ubyte4 dkLen, ubyte *pRetDerivedKey);

MOC_EXTERN MSTATUS PKCS5_decrypt( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                    ubyte subType, CStream cs,
                                    ASN1_ITEMPTR pPBEParam, ASN1_ITEMPTR pEncrypted,
                                    const ubyte* password, sbyte4 passwordLen,
                                    ubyte** privateKeyInfo,
                                    sbyte4* privateKeyInfoLen);

MOC_EXTERN MSTATUS PKCS5_encryptV1( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                  ubyte pkcs5SubType,
                                  const ubyte* password, ubyte4 passwordLen,
                                  const ubyte* salt, ubyte4 saltLen,
                                  ubyte4 iterCount,
                                  ubyte* plainText, ubyte4 ptLen);

MOC_EXTERN MSTATUS PKCS5_encryptV2( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                  const BulkEncryptionAlgo* pAlgo,
                                  ubyte4 keyLength, ubyte4 effectiveKeyBits,
                                  const ubyte* password, ubyte4 passwordLen,
                                  const ubyte* salt, ubyte4 saltLen,
                                  ubyte4 iterCount, const ubyte* iv,
                                  ubyte* plainText, ubyte4 ptLen);

#endif

#endif /* __PPKCS5_HEADER__ */

