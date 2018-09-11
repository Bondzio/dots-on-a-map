/* Version: mss_v6_3 */
/*
 * crypto.c
 *
 * General Crypto Operations
 *
 * Copyright Mocana Corp 2005-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../crypto/crypto.h"
#include "../crypto/md2.h"
#include "../crypto/md4.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#ifdef __ENABLE_ARC2_CIPHERS__
#include "../crypto/arc2.h"
#include "../crypto/rc2algo.h"
#endif
#ifndef __DISABLE_ARC4_CIPHERS__
#include "../crypto/arc4.h"
#include "../crypto/rc4algo.h"
#endif
#include "../crypto/des.h"
#ifndef __DISABLE_3DES_CIPHERS__
#include "../crypto/three_des.h"
#endif
#include "../crypto/aes.h"
#include "../crypto/aes_ctr.h"
#ifdef __ENABLE_BLOWFISH_CIPHERS__
#include "../crypto/blowfish.h"
#endif
#ifdef __ENABLE_NIL_CIPHER__
#include "../crypto/nil.h"
#endif
#include "../asn1/oiddefs.h"


/*------------------------------------------------------------------*/

/****************************** BulkEncryptionAlgo *************************************/

#ifndef __DISABLE_3DES_CIPHERS__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_TripleDESSuite =
    { THREE_DES_BLOCK_SIZE, Create3DESCtx, Delete3DESCtx, Do3DES };
#endif

#ifndef __DISABLE_3DES_CIPHERS__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_TwoKeyTripleDESSuite =
    { THREE_DES_BLOCK_SIZE, Create2Key3DESCtx, Delete3DESCtx, Do3DES };
#endif

#ifdef __ENABLE_DES_CIPHER__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_DESSuite =
    { DES_BLOCK_SIZE, CreateDESCtx, DeleteDESCtx, DoDES };
#endif

#ifndef __DISABLE_ARC4_CIPHERS__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_RC4Suite =
    { 0, CreateRC4Ctx, DeleteRC4Ctx, DoRC4 };
#endif

#ifdef __ENABLE_ARC2_CIPHERS__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_RC2Suite =
    { RC2_BLOCK_SIZE, CreateRC2Ctx, DeleteRC2Ctx, DoRC2 };
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_RC2EffectiveBitsSuite =
    { RC2_BLOCK_SIZE, CreateRC2Ctx2, DeleteRC2Ctx, DoRC2 };
#endif

#ifdef __ENABLE_BLOWFISH_CIPHERS__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_BlowfishSuite =
    { BLOWFISH_BLOCK_SIZE/*8*/, CreateBlowfishCtx, DeleteBlowfishCtx, DoBlowfish };
#endif

#ifndef __DISABLE_AES_CIPHERS__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_AESSuite =
    { AES_BLOCK_SIZE, CreateAESCtx, DeleteAESCtx, DoAES };
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_AESCtrSuite =
#ifdef __UCOS_RTOS__
    { 0, CreateAESCTRCtx, DeleteAESCTRCtx, DoAESCTR };
#else
    { 0, (CreateBulkCtxFunc)CreateAESCTRCtx, DeleteAESCTRCtx, DoAESCTR };
#endif /* __UCOS_RTOS__ */
#endif

#ifdef __ENABLE_NIL_CIPHER__
MOC_EXTERN_DATA_DEF const BulkEncryptionAlgo CRYPTO_NilSuite =
    { 0, CreateNilCtx, DeleteNilCtx, DoNil };
#endif


/****************************** BulkHashAlgo *******************************************/
#ifdef __ENABLE_MOCANA_MD2__
static const BulkHashAlgo MD2Suite =
    { MD2_RESULT_SIZE, MD2_BLOCK_SIZE, MD2Alloc, MD2Free,
        (BulkCtxInitFunc)MD2Init, (BulkCtxUpdateFunc)MD2Update, (BulkCtxFinalFunc)MD2Final };
#endif

#ifdef __ENABLE_MOCANA_MD4__
static const BulkHashAlgo MD4Suite =
    { MD4_RESULT_SIZE, MD4_BLOCK_SIZE, MD4Alloc, MD4Free,
        (BulkCtxInitFunc)MD4Init, (BulkCtxUpdateFunc)MD4Update, (BulkCtxFinalFunc)MD4Final };
#endif

static const BulkHashAlgo MD5Suite =
    { MD5_RESULT_SIZE, MD5_BLOCK_SIZE, MD5Alloc_m, MD5Free_m,
        (BulkCtxInitFunc)MD5Init_m, (BulkCtxUpdateFunc)MD5Update_m, (BulkCtxFinalFunc)MD5Final_m };

static const BulkHashAlgo SHA1Suite =
    { SHA1_RESULT_SIZE, SHA1_BLOCK_SIZE, SHA1_allocDigest, SHA1_freeDigest,
        (BulkCtxInitFunc)SHA1_initDigest, (BulkCtxUpdateFunc)SHA1_updateDigest, (BulkCtxFinalFunc)SHA1_finalDigest };

#ifndef __DISABLE_MOCANA_SHA224__
static const BulkHashAlgo SHA224Suite =
    { SHA224_RESULT_SIZE, SHA224_BLOCK_SIZE, SHA224_allocDigest, SHA224_freeDigest,
        (BulkCtxInitFunc)SHA224_initDigest, (BulkCtxUpdateFunc)SHA224_updateDigest, (BulkCtxFinalFunc)SHA224_finalDigest };
#endif

#ifndef __DISABLE_MOCANA_SHA256__
static const BulkHashAlgo SHA256Suite =
    { SHA256_RESULT_SIZE, SHA256_BLOCK_SIZE, SHA256_allocDigest, SHA256_freeDigest,
        (BulkCtxInitFunc)SHA256_initDigest, (BulkCtxUpdateFunc)SHA256_updateDigest, (BulkCtxFinalFunc)SHA256_finalDigest };
#endif

#ifndef __DISABLE_MOCANA_SHA384__
static const BulkHashAlgo SHA384Suite =
    { SHA384_RESULT_SIZE, SHA384_BLOCK_SIZE, SHA384_allocDigest, SHA384_freeDigest,
        (BulkCtxInitFunc)SHA384_initDigest, (BulkCtxUpdateFunc)SHA384_updateDigest, (BulkCtxFinalFunc)SHA384_finalDigest };
#endif

#ifndef __DISABLE_MOCANA_SHA512__
static const BulkHashAlgo SHA512Suite =
    { SHA512_RESULT_SIZE, SHA512_BLOCK_SIZE, SHA512_allocDigest, SHA512_freeDigest,
        (BulkCtxInitFunc)SHA512_initDigest, (BulkCtxUpdateFunc)SHA512_updateDigest, (BulkCtxFinalFunc)SHA512_finalDigest };
#endif


/*------------------------------------------------------------------*/

extern MSTATUS CRYPTO_getRSAHashAlgo( ubyte rsaAlgoId,
                                     const BulkHashAlgo** ppBulkHashAlgo)
{
    MSTATUS status = OK;

    switch (rsaAlgoId)
    {
#ifdef __ENABLE_MOCANA_MD2__
        case md2withRSAEncryption:
            *ppBulkHashAlgo = &MD2Suite;
            break;
#endif

#ifdef __ENABLE_MOCANA_MD4__
        case md4withRSAEncryption:
            *ppBulkHashAlgo = &MD4Suite;
            break;
#endif

        case md5withRSAEncryption:
            *ppBulkHashAlgo = &MD5Suite;
            break;

        case sha1withRSAEncryption:
            *ppBulkHashAlgo = &SHA1Suite;
            break;

#ifndef __DISABLE_MOCANA_SHA256__
        case sha256withRSAEncryption:
            *ppBulkHashAlgo = &SHA256Suite;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA384__
        case sha384withRSAEncryption:
            *ppBulkHashAlgo = &SHA384Suite;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA512__
        case sha512withRSAEncryption:
            *ppBulkHashAlgo = &SHA512Suite;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA224__
        case sha224withRSAEncryption:
            *ppBulkHashAlgo = &SHA224Suite;
            break;
#endif

        default:
            status = ERR_INVALID_ARG;
            break;
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS CRYPTO_getHashAlgoOID( ubyte rsaAlgoId,
                                     const ubyte** pHashAlgoOID)
{
    MSTATUS status = OK;

    switch (rsaAlgoId)
    {
        case md5withRSAEncryption:
            *pHashAlgoOID = md5_OID;
            break;

        case sha1withRSAEncryption:
            *pHashAlgoOID = sha1_OID;
            break;

        case sha256withRSAEncryption:
            *pHashAlgoOID = sha256_OID;
            break;

        case sha384withRSAEncryption:
            *pHashAlgoOID = sha384_OID;
            break;

        case sha512withRSAEncryption:
            *pHashAlgoOID = sha512_OID;
            break;

        case sha224withRSAEncryption:
            *pHashAlgoOID = sha224_OID;
            break;

        default:
            status = ERR_INVALID_ARG;
            break;
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_getRSAHashAlgoOID( ubyte rsaAlgoId, ubyte rsaAlgoOID[/* 1 + MAX_SIG_OID_LEN */])
{
    if (!rsaAlgoOID)
        return ERR_NULL_POINTER;

    /* build the PKCS1 OID */
    MOC_MEMCPY( rsaAlgoOID, pkcs1_OID, 1 + PKCS1_OID_LEN);
    /* add the suffix */
    ++rsaAlgoOID[0];
    rsaAlgoOID[1+PKCS1_OID_LEN] = rsaAlgoId;
    return OK;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
extern MSTATUS
CRYPTO_getDSAHashAlgoOID( ubyte rsaAlgoId, ubyte dsaAlgoOID[/* 1 + MAX_SIG_OID_LEN */])
{
    MSTATUS status = OK;
    ubyte subType = 0;
    if (!dsaAlgoOID)
        return ERR_NULL_POINTER;

    switch (rsaAlgoId)
    {
    case ht_sha1:
        MOC_MEMCPY( dsaAlgoOID, dsaWithSHA1_OID, 1 + dsaWithSHA1_OID[0]);
        goto exit;

    case ht_sha224:
        subType = 1;
        break;

    case ht_sha256:
        subType = 2;
        break;

    default:
        status = ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO;
        goto exit;
    }

    MOC_MEMCPY(dsaAlgoOID, dsaWithSHA2_OID, 1 + dsaWithSHA2_OID[0]);
    ++dsaAlgoOID[0];
    dsaAlgoOID[ dsaAlgoOID[0]] = subType;

exit:

    return status;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
extern MSTATUS
CRYPTO_getECDSAHashAlgoOID( ubyte rsaAlgoId, ubyte ecdsaAlgoOID[/* 1 + MAX_SIG_OID_LEN */])
{
    MSTATUS status = OK;
    ubyte subType = 0;
    if (!ecdsaAlgoOID)
        return ERR_NULL_POINTER;

    switch (rsaAlgoId)
    {
    case ht_sha1:
        MOC_MEMCPY( ecdsaAlgoOID, ecdsaWithSHA1_OID, 1 + ecdsaWithSHA1_OID[0]);
        goto exit;

    case ht_sha224:
        subType = 1;
        break;

    case ht_sha256:
        subType = 2;
        break;

    case ht_sha384:
        subType = 3;
        break;

    case ht_sha512:
        subType = 4;
        break;

    default:
        status = ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO;
        goto exit;
    }

    MOC_MEMCPY(ecdsaAlgoOID, ecdsaWithSHA2_OID, 1 + ecdsaWithSHA2_OID[0]);
    ++ecdsaAlgoOID[0];
    ecdsaAlgoOID[ ecdsaAlgoOID[0]] = subType;

exit:
    return status;
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
CRYPTO_Process( MOC_SYM(hwAccelDescr hwAccelCtx) const BulkEncryptionAlgo* pAlgo,
                    ubyte* keyMaterial, sbyte4 keyLength,
                    ubyte* iv, ubyte* data, sbyte4 dataLength, sbyte4 encrypt)
{
    MSTATUS status;
    BulkCtx ctx = 0;

    if ( !pAlgo || !keyMaterial || !data)
    {
        return ERR_NULL_POINTER;
    }

    ctx = pAlgo->createFunc( MOC_SYM( hwAccelCtx) keyMaterial, keyLength, encrypt);
    if (!ctx)
    {
        return ERR_MEM_ALLOC_FAIL;
    }

    status = pAlgo->cipherFunc( MOC_SYM( hwAccelCtx) ctx, data, dataLength, encrypt, iv);

    pAlgo->deleteFunc( MOC_SYM( hwAccelCtx) &ctx);

    return status;
}

