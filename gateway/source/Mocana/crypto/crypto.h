/* Version: mss_v6_3 */
/*
 * crypto.h
 *
 * General Crypto Definitions & Types Header
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __CRYPTO_HEADER__
#define __CRYPTO_HEADER__


#define CERT_MAXDIGESTSIZE         (64)      /*(SHA512_RESULT_SIZE)*/
#define MAX_IV_LENGTH              (16)      /* AES */
#define MAX_ENC_KEY_LENGTH         (32)      /* AES-256 */

/*------------------------------------------------------------------*/


/* bulk encryption algorithms descriptions */
typedef BulkCtx (*CreateBulkCtxFunc)(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);
typedef MSTATUS (*DeleteBulkCtxFunc)(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);
typedef MSTATUS (*CipherFunc)       (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv);

typedef struct BulkEncryptionAlgo
{
    ubyte4                  blockSize;
    CreateBulkCtxFunc       createFunc;
    DeleteBulkCtxFunc       deleteFunc;
    CipherFunc              cipherFunc;

} BulkEncryptionAlgo;

/* predefined BulkEncryptionAlgos */
#ifndef __DISABLE_3DES_CIPHERS__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_TripleDESSuite;
#endif

#ifndef __DISABLE_3DES_CIPHERS__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_TwoKeyTripleDESSuite;
#endif

#ifdef __ENABLE_DES_CIPHER__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_DESSuite;
#endif

#ifndef __DISABLE_ARC4_CIPHERS__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_RC4Suite;
#endif

#ifdef __ENABLE_ARC2_CIPHERS__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_RC2Suite;
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_RC2EffectiveBitsSuite;
#endif

#ifdef __ENABLE_BLOWFISH_CIPHERS__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_BlowfishSuite;
#endif

#ifndef __DISABLE_AES_CIPHERS__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_AESSuite;
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_AESCtrSuite;
#endif

#ifdef __ENABLE_NIL_CIPHER__
MOC_EXTERN const BulkEncryptionAlgo CRYPTO_NilSuite;
#endif


/* utility function to */
MOC_EXTERN MSTATUS CRYPTO_Process( MOC_SYM(hwAccelDescr hwAccelCtx) const BulkEncryptionAlgo* pAlgo,
                                            ubyte* keyMaterial, sbyte4 keyLength,
                                            ubyte* iv, ubyte* data, sbyte4 dataLength, sbyte4 encrypt);

/* bulk hash algorithms descriptions */
typedef MSTATUS (*BulkCtxAllocFunc) (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *ctx);
typedef MSTATUS (*BulkCtxFreeFunc)  (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *ctx);
typedef MSTATUS (*BulkCtxInitFunc)  (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx ctx);
typedef MSTATUS (*BulkCtxUpdateFunc)(MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx ctx, const ubyte *data, ubyte4 datalength);
typedef MSTATUS (*BulkCtxFinalFunc) (MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte *result);

typedef struct BulkHashAlgo
{
    ubyte4                  digestSize;
    ubyte4                  blockSize; /* used for HMAC */
    BulkCtxAllocFunc        allocFunc;
    BulkCtxFreeFunc         freeFunc;
    BulkCtxInitFunc         initFunc;
    BulkCtxUpdateFunc       updateFunc;
    BulkCtxFinalFunc        finalFunc;
} BulkHashAlgo;


/* suffixes of pkcs1_OID */
enum {
    rsaEncryption = 1,
    md2withRSAEncryption = 2,
    md4withRSAEncryption = 3,
    md5withRSAEncryption = 4,
    sha1withRSAEncryption = 5,
    sha256withRSAEncryption = 11,
    sha384withRSAEncryption = 12,
    sha512withRSAEncryption = 13,
    sha224withRSAEncryption = 14,
    /* duplicate definition = hash_type */
    ht_md2 = 2,
    ht_md4 = 3,
    ht_md5 = 4,
    ht_sha1 = 5,
    ht_sha256 = 11,
    ht_sha384 = 12,
    ht_sha512 = 13,
    ht_sha224 = 14
};

MOC_EXTERN MSTATUS CRYPTO_getRSAHashAlgo( ubyte rsaAlgoId, const BulkHashAlgo** ppBulkHashAlgo);
MOC_EXTERN MSTATUS CRYPTO_getRSAHashAlgoOID( ubyte rsaAlgoId, ubyte rsaAlgoOID[/* 1 + MAX_SIG_OID_LEN */]);
MOC_EXTERN MSTATUS CRYPTO_getDSAHashAlgoOID( ubyte rsaAlgoId, ubyte dsaAlgoOID[/* 1 + MAX_SIG_OID_LEN */]);
MOC_EXTERN MSTATUS CRYPTO_getECDSAHashAlgoOID( ubyte rsaAlgoId, ubyte ecdsaAlgoOID[/* 1 + MAX_SIG_OID_LEN */]);
MOC_EXTERN MSTATUS CRYPTO_getHashAlgoOID( ubyte rsaAlgoId, const ubyte** pHashAlgoOID);

/* bulk encryption algorithms descriptions */
typedef BulkCtx (*CreateAeadCtxFunc)(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);
typedef MSTATUS (*DeleteAeadCtxFunc)(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);
typedef MSTATUS (*AeadCipherFunc)  (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx,  ubyte* nonce, ubyte4 nlen, ubyte* adata, ubyte4 alen, ubyte* data, ubyte4 dataLength, ubyte4 verifyLen, sbyte4 encrypt);

typedef struct AeadAlgo
{
    ubyte4                  implicitNonceSize;
    ubyte4                  explicitNonceSize;
    ubyte4                  tagSize;
    CreateAeadCtxFunc       createFunc;
    DeleteAeadCtxFunc       deleteFunc;
    AeadCipherFunc          cipherFunc;

} AeadAlgo;

#endif /* __CRYPTO_HEADER__ */
