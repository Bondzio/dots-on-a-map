/* Version: mss_v6_3 */
/*
 * pkcs12.c
 *
 * PKCS12 Utilities
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef DEBUG_PKCS12
#include <stdio.h>
#endif

#ifdef __ENABLE_MOCANA_PKCS12__

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/debug_console.h"
#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../crypto/crypto.h"
#include "../crypto/sha1.h"
#include "../crypto/md5.h"
#include "../common/vlong.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../asn1/derencoder.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../common/random.h"
#include "../crypto/rsa.h"
#if (defined(__ENABLE_MOCANA_ECC__))
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#endif
#include "../crypto/pubcrypto.h"
#include "../crypto/pkcs_common.h"
#include "../crypto/pkcs7.h"
#include "../crypto/pkcs_key.h"
#include "../crypto/hmac.h"
#include "../crypto/des.h"
#include "../crypto/three_des.h"
#include "../crypto/arc4.h"
#include "../crypto/rc4algo.h"
#include "../crypto/arc2.h"
#include "../crypto/rc2algo.h"
#include "../crypto/pkcs12.h"
#include "../crypto/ca_mgmt.h"
#include "../asn1/parsecert.h"

#if _DEBUG
#include <stdio.h>
#endif

#ifdef DEBUG_PKCS12
char gDebFileName[128]="\0";
#endif

/*------------------------------------------------------------------*/

/* OID */
/* note that there is another OID for the same thing in previous versions of PKCS#12
 see the DumpASn1.Cfg that comes with the DumpASN1 tool */
const ubyte pkcs12_bagtypes_root_OID[] =
    { 10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x0A, 0x01}; /* 1.2.840.113549.1.12.10.1 */


const ubyte pkcs12_Pbe_root_OID[] = /* 1.2.840.113549.1.12.1 */
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01 };

const ubyte pkcs9_pkcs12_certtypes_root_OID[] = /* 1.2.840.113549.1.9.22 */
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x16 };

const ubyte pkcs9_pkcs12_certtypes_X509_OID[] = { 10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x16, 0x01 };
const ubyte pkcs9_pkcs12_certtypes_sdsi_OID[] = { 10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x16, 0x02 };

/* bag types OID */
const ubyte pkcs12_bagtypes_keyBag[] = {11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x0A, 0x01, 0x01}; /* 1.2.840.113549.1.12.10.1.1 */
const ubyte pkcs12_bagtypes_pkcs8ShroudedKeyBag[] = {11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x0A, 0x01, 0x02}; /* 1.2.840.113549.1.12.10.1.2 */
const ubyte pkcs12_bagtypes_certBag[] = {11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x0A, 0x01, 0x03}; /* 1.2.840.113549.1.12.10.1.3 */
const ubyte pkcs12_bagtypes_crlBag[] = {11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x0A, 0x01, 0x04}; /* 1.2.840.113549.1.12.10.1.4 */
const ubyte pkcs12_bagtypes_secretBag[] = {11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x0A, 0x01, 0x05}; /* 1.2.840.113549.1.12.10.1.5 */
const ubyte pkcs12_bagtypes_safeContentsBag[] = {11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x0A, 0x01, 0x06}; /* 1.2.840.113549.1.12.10.1.6 */

/* PBE-ID OID */
#ifndef __DISABLE_ARC4_CIPHERS__
const ubyte pkcs12_pbe_128rc4_OID[]  = {10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01, 0x01};
const ubyte pkcs12_pbe_40rc4_OID[]   = {10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01, 0x02};
#endif
#ifndef __DISABLE_3DES_CIPHERS__
const ubyte pkcs12_pbe_3DES_OID[]    = {10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01, 0x03};
const ubyte pkcs12_pbe_2DES_OID[]    = {10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01, 0x04};
#endif
#ifdef __ENABLE_ARC2_CIPHERS__
const ubyte pkcs12_pbe_128rc2_OID[]  = {10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01, 0x05};
const ubyte pkcs12_pbe_40rc2_OID[]   = {10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01, 0x06};
#endif

typedef enum 
{
    PKCS12SafeBagType_keyBag = 1,
    PKCS12SafeBagType_pkcs8ShroudedKeyBag,
    PKCS12SafeBagType_certBag,
    PKCS12SafeBagType_crlBag,
    PKCS12SafeBagType_secretBag,
    PKCS12SafeBagType_safeContentsBag
} PKCS12SafeBagType;

typedef struct PKCS12CipherSuite
{
    sbyte4                      cipherSuiteId;  /* identifer for cipher suite */
    sbyte4                      keySize;        /* size of key */
    const BulkEncryptionAlgo*   pBEAlgo;        /* the encryption functions */
    const ubyte*                pOID;           /* OID */
} PCKS12CipherSuite;


/* array of PKCS12CipherSuite - it's ordered by the Pbe id */
static PCKS12CipherSuite mPKCS12CipherSuites[] =
{
#ifndef __DISABLE_ARC4_CIPHERS__
    { 1, 16, &CRYPTO_RC4Suite, pkcs12_pbe_128rc4_OID},                /* 1: SHA 128 RC4 */
    { 2, 5,  &CRYPTO_RC4Suite, pkcs12_pbe_40rc4_OID },                /* 2: SHA 40 RC4 */
#endif
#ifndef __DISABLE_3DES_CIPHERS__
    { 3, 24, &CRYPTO_TripleDESSuite, pkcs12_pbe_3DES_OID},            /* 3: SHA 3key 3DES CBC */
#ifdef __ENABLE_MOCANA_2KEY_3DES__
    { 4, 16, &CRYPTO_TwoKeyTripleDESSuite, pkcs12_pbe_2DES_OID},      /* 4: SHA 2key 3DES CBC */
#endif
#endif
#ifdef __ENABLE_ARC2_CIPHERS__
    { 5, 16, &CRYPTO_RC2Suite, pkcs12_pbe_128rc2_OID},                /* 5: SHA 128 RC2 CBC */
    { 6, 5,  &CRYPTO_RC2Suite, pkcs12_pbe_40rc2_OID },                /* 6: SHA 40 RC2 CBC */
#endif
    { -1, 0, NULL }                                                   /* -1: NOTHING */
};

/* these 2 constants allow us to allocate buffer on the stack instead of MALLOC */
#define PKCS12_PBE_MAX_KEY_LENGTH   (24) /* 3 Key Triple DES */

/*
 * Note: these constants are declared as byte arrays on the stack, an then
 * passed to a RNG where they are filled with a number of bytes corresponding
 * to pCipherSuite->pBEAlgo->blockSize. (note: that is the block size, not
 * the hash output size.)
 *
 * Per RFC 4828 section 2.1:
 *   Block size:  the size of the data block the underlying hash algorithm
 *     operates upon.  For SHA-256, this is 512 bits, for SHA-384 and
 *     SHA-512, this is 1024 bits.
 */
#if defined( __DISABLE_MOCANA_SHA512__) && defined(__DISABLE_MOCANA_SHA384__)
/* Maximum Hash Block Size = MD5_BLOCK_SIZE = SHA1_BLOCK_SIZE  = SHA256_BLOCK_SIZE */
#define PKCS12_PBE_MAX_IV_LENGTH            (64)
#else
/* Maximum Hash Block Size = max(SHA3_BLOCK_SIZE, SHA512_BLOCK_SIZE) */
/* (The SHA-3 value is used for futureproofing.) */
#define PKCS12_PBE_MAX_IV_LENGTH            (144)
#endif

static WalkerStep gotoPKCS12FromSafeBagToBagValue[] =
{
    { GoNthChild, 2, 0},
    { VerifyTag, 0, 0 },
    { GoFirstChild, 0, 0},
    { Complete, 0, 0}
};

typedef struct {
    int type;
    void* context; /* for type = 1 */
    union 
    {
        PKCS12_contentHandler handler; /* type = 0 */
        PKCS12_contentHandlerEx handlerEx; /* type = 1 */
    } func;   
} PKCS12CallbackInfo;

/* function prototypes */
static MSTATUS PKCS12_SHA1_GenerateRandom( MOC_HASH(hwAccelDescr hwAccelCtx)
                                           ubyte ID, sbyte4 r, /* numIters */
                                           const ubyte* salt, sbyte4 s, /* saltLen */
                                           const ubyte* uniPass, sbyte4 p, /* uniPassLen */
                                           ubyte* random, sbyte4 randomLen);

static MSTATUS PKCS12_GenerateKey( MOC_HASH(hwAccelDescr hwAccelCtx)
                                   ubyte id, ASN1_ITEMPTR pMacData,
                                   CStream s, const ubyte* uniPassword,
                                   sbyte4 uniPassLen, ubyte* key, sbyte4 keyLen);

static MSTATUS PKCS12_GenerateMac(MOC_HASH(hwAccelDescr hwAccelCtx)
                                  ASN1_ITEM* pT, CStream s,
                                  ubyte* hmacKey, sbyte4 hmacKeyLen,
                                  ubyte hmacRes[SHA_HASH_RESULT_SIZE]);

static MSTATUS PKCS12_VerifyMac(MOC_HASH(hwAccelDescr hwAccelCtx)
                                ASN1_ITEM* pContentType, CStream s,
                                ubyte* hmacKey, ubyte* digest);

static MSTATUS PKCS12_ProcessKeyBag( ASN1_ITEMPTR pSafeBag, CStream s, 
                                     const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessPKCS8ShroudedKeyBag( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                                  ASN1_ITEMPTR pSafeBag,
                                                  CStream s, const ubyte* uniPassword,
                                                  sbyte4 uniPassLen, ubyte** privateKeyInfo,
                                                  sbyte4* privateKeyInfoLen,
                                                  const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessCertBag( MOC_HASH(hwAccelDescr hwAccelCtx)
                                      ASN1_ITEMPTR pSafeBag,
                                      CStream s, const ubyte* uniPassword,
                                      sbyte4 uniPassLen,
                                      const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessCrlBag( MOC_HASH(hwAccelDescr hwAccelCtx)
                                     ASN1_ITEMPTR pSafeBag,
                                     CStream s, const ubyte* uniPassword,
                                     sbyte4 uniPassLen,
                                     const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessSecretBag( MOC_HASH(hwAccelDescr hwAccelCtx)
                                        ASN1_ITEMPTR pSafeBag,
                                        CStream s, const ubyte* uniPassword,
                                        sbyte4 uniPassLen,
                                        const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessSafeContentsBag( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                              ASN1_ITEMPTR pSafeBag,
                                              CStream s, const ubyte* uniPassword,
                                              sbyte4 uniPassLen,
                                              const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessSafeBags( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                       ASN1_ITEMPTR pSafeBag,
                                       CStream s, const ubyte* uniPassword,
                                       sbyte4 uniPassLen,
                                       const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessSafeContents(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) 
                                          ASN1_ITEM* pSafeContents,
                                          CStream s, const ubyte* uniPassword, sbyte4 uniPassLen, 
                                          const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessDataContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pContentInfo,
                                             CStream s, const ubyte* uniPassword, sbyte4 uniPassLen, 
                                             const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessEncryptedDataContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pContentInfo,
                                                      CStream s, const ubyte* uniPassword, sbyte4 uniPassLen, 
                                                      const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_ProcessEnvelopedDataContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                                      ASN1_ITEMPTR pContentInfo,
                                                      CStream s,
                                                      const ubyte* uniPassword, ubyte4 uniPassLen,
                                                      PKCS7_Callbacks* pkcs7CBs,
                                                      const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PCKS12_ProcessContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pContentInfo,
                                         ASN1_ITEM* pMacData, CStream s,
                                         const ubyte* uniPassword, sbyte4 uniPassLen,
                                         PKCS7_Callbacks* pkcs7CBs,
                                         const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_PasswordIntegrityMode( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                             ASN1_ITEM* pRootItem, ASN1_ITEM* pContentType,
                                             CStream s, const ubyte* uniPassword, sbyte4 uniPassLen,
                                             PKCS7_Callbacks* pkcs7CBs,
                                             const PKCS12CallbackInfo* handlerInfo);

static MSTATUS PKCS12_PublicKeyIntegrityMode(MOC_RSA(hwAccelDescr hwAccelCtx) 
                                             ASN1_ITEM* pContentType, CStream s,
                                             const ubyte* uniPassword, sbyte4 uniPassLen,
                                             PKCS7_Callbacks* pkcs7CBs, const PKCS12CallbackInfo* handlerInfo);

static MSTATUS 
PKCS12_AddEncryptedContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                               DER_ITEMPTR pParent,
                               randomContext* pRandomContext,
                               enum PKCS8EncryptionType encType,
                               const ubyte* password, ubyte passwordLen,
                               ubyte* pContentPayload, ubyte4 payLoadLen);
static MSTATUS
PKCS12_AddContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) 
                      DER_ITEMPTR pParent,
                      randomContext* pRandomContext,
                      enum PKCS8EncryptionType encType,
                      const ubyte* pContentType,
                      const ubyte* password, ubyte4 passwordLen,
                      byteBoolean  contentInfo,
                      ubyte* pContentPayload, ubyte4 payLoadLen);

static PCKS12CipherSuite *
PKCS12_getCipherSuite(const sbyte4 pbeSubType)
{
    sbyte4 index;

    if (-1 != pbeSubType)
    {
        for (index = 0; index < COUNTOF(mPKCS12CipherSuites); index++)
            if (pbeSubType == mPKCS12CipherSuites[index].cipherSuiteId)
                return &mPKCS12CipherSuites[index];
    }

    return NULL;
}

/*-------------------------------------------------------------------------------*/

extern const BulkEncryptionAlgo*
PKCS12_GetEncryptionAlgo( ubyte pbeSubType)
{
    PCKS12CipherSuite* pCipherSuite = PKCS12_getCipherSuite(pbeSubType);
    return ( pCipherSuite) ? pCipherSuite->pBEAlgo : NULL;
}

/*-------------------------------------------------------------------------------*/

/* The PKCS#12 random generator -- uniPass is the password in Unicode
 format (big endian) cf test code
*/
static MSTATUS
PKCS12_SHA1_GenerateRandom( MOC_HASH(hwAccelDescr hwAccelCtx)
                                ubyte ID, sbyte4 r, /* numIters */
                                const ubyte* salt, sbyte4 s, /* saltLen */
                                const ubyte* uniPass, sbyte4 p, /* uniPassLen */
                                ubyte* random, sbyte4 randomLen)
{
    MSTATUS status = OK;
    vlong *vlongB = 0;
    vlong *vlongI = 0;
    /* here we follow the naming used in the PKCS#12 ref document */
    ubyte D[SHA_HASH_BLOCK_SIZE]; /* diversifier */
    ubyte* I;   /* S + P */
    ubyte* tmp;   /* tmp */
    sbyte4 lenS, lenP, lenI;    /* lengths of S and P, lenI = lenS + lenP */
    sbyte4 c;           /* number of SHA result blocks to fill random */
    sbyte4 i;           /* loop counter */
    ubyte A[SHA_HASH_RESULT_SIZE];

    c = SHA_HASH_RESULT_SIZE * ((randomLen + SHA_HASH_RESULT_SIZE -1) / SHA_HASH_RESULT_SIZE);
    lenS = SHA_HASH_BLOCK_SIZE * (( s + SHA_HASH_BLOCK_SIZE - 1) / SHA_HASH_BLOCK_SIZE);
    lenP = (0 == p) ? 0 : SHA_HASH_BLOCK_SIZE * (( p + SHA_HASH_BLOCK_SIZE - 1) / SHA_HASH_BLOCK_SIZE);
    lenI = lenS + lenP;
    /* verify here that lenI is a multiple pof SHA_HASH_BLOCK_SIZE */
    /* we need assertions in the mocana library */
    I = (ubyte*) MALLOC( lenI); /* only allocation in this module except vlong */
    if ( NULL == I)
    {
        return ERR_MEM_ALLOC_FAIL;
    }

    /* fill the buffers */
    for ( i = 0; i < SHA_HASH_BLOCK_SIZE; ++i)
    {
        D[i] = ID;
    }
    tmp = I;
    for ( i = 0; i < lenS; ++i)
    {
        *tmp++ = salt[i % s];
    }
    for ( i = 0; i < lenP; ++i)
    {
        *tmp++ = uniPass[i % p];
    }
    /* generate the hash */
    for ( i = 0; i < c; ++i)
    {
        ubyte B[SHA_HASH_BLOCK_SIZE];
        int j, numCopied;

        /* a) set A = H(H(H(...(H(D+I)))))) */
        /* first round */
        shaDescr ctx;
        SHA1_initDigest( MOC_HASH(hwAccelCtx) &ctx);
        SHA1_updateDigest(MOC_HASH(hwAccelCtx) &ctx, D, SHA_HASH_BLOCK_SIZE);
        SHA1_updateDigest(MOC_HASH(hwAccelCtx) &ctx, I, lenI);
        SHA1_finalDigest(MOC_HASH(hwAccelCtx) &ctx, A);
        /* round 2 -> r of hashes */
        for (j = 1; j < r; ++j)
        {
            SHA1_initDigest( MOC_HASH( hwAccelCtx) &ctx);
            SHA1_updateDigest(MOC_HASH( hwAccelCtx) &ctx, A, SHA_HASH_RESULT_SIZE);
            SHA1_finalDigest(MOC_HASH( hwAccelCtx) &ctx, A);
        }
        /* copy A into random output */
        numCopied = ( SHA_HASH_RESULT_SIZE  < randomLen) ? SHA_HASH_RESULT_SIZE : randomLen;
        MOC_MEMCPY( random, A, numCopied);
        randomLen -= numCopied;
        random += numCopied;

        /* no need to do the rest if last round */
        if ( 0 == randomLen )
        {
            break;
        }
        else if ( 0 > randomLen)
        {
            /* we are in trouble */
	    DEBUG_PRINT( DEBUG_CRYPTO, (sbyte*)"Problem in pkcs12"); /* we need an assert */
            break;
        }

        /* b) Concatenate A into B */
        for (j = 0; j < SHA_HASH_BLOCK_SIZE; ++j)
        {
            B[j] = A[j % SHA_HASH_RESULT_SIZE];
        }

        /* c) Treating I as a concatenation of SHA_HASH_BLOCK_SIZE blocks, modify each block
         by adding B+1 to each block */
        /* compute B+1 into vlongB */
        status = VLONG_vlongFromByteString( B, SHA_HASH_BLOCK_SIZE, &vlongB, 0);
        if ( status < OK) goto exit;
        status = VLONG_increment( vlongB, 0); /* B = B+1 */
        if ( status < OK) goto exit;
        for (j = 0; j < lenI; j += SHA_HASH_BLOCK_SIZE)
        {
            /* allocate a vlong based on content of I lenI is a multiple of SHA_HASH_BLOCK_SIZE */
            status = VLONG_vlongFromByteString( I + j, SHA_HASH_BLOCK_SIZE, &vlongI, 0);
            if ( status < OK) goto exit;
            /* add B + 1 to it */
            status = VLONG_addSignedVlongs( vlongI, vlongB, 0);
            if ( status < OK) goto exit;
            /* put it back into I, this routine does everything pads with zeros and truncate */
            status = VLONG_fixedByteStringFromVlong( vlongI, I+j, SHA_HASH_BLOCK_SIZE);
            if ( status < OK) goto exit;
            /* free vlongI here for next loop */
            VLONG_freeVlong( &vlongI,0);
        }

        VLONG_freeVlong( &vlongB, 0);

    }

exit:

    /* clean up */
    VLONG_freeVlong( &vlongB, 0);
    VLONG_freeVlong( &vlongI, 0);

    if (I)
    {
        FREE(I);
    }

    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_GenerateKey( MOC_HASH(hwAccelDescr hwAccelCtx)
                        ubyte id,
                        ASN1_ITEMPTR pMacData,
                        CStream s,
                        const ubyte* uniPassword,
                        sbyte4 uniPassLen,
                        ubyte* key,
                        sbyte4 keyLen)
{
    MSTATUS status;
    ASN1_ITEMPTR pSalt, pIterations;
    sbyte4 iterations = 1; /* DEFAULT (deprecated) */
    ubyte* salt;

    /* salt is the 2nd child of macData */
    status = ASN1_GetNthChild( pMacData, 2, &pSalt);
    if ( status < OK) return status;

    pIterations = ASN1_NEXT_SIBLING( pSalt);
    if ( pIterations)
    {
        if ( OK != ASN1_VerifyType( pIterations, INTEGER))
        {
            return ERR_PKCS12_INVALID_STRUCT;
        }
        else
        {
            iterations = pIterations->data.m_intVal;
        }
    }

    salt = (ubyte*) CS_memaccess( s, pSalt->dataOffset, pSalt->length);
    if ( 0 == salt)
    {
        return ERR_MEM_ALLOC_FAIL;
    }
    status = PKCS12_SHA1_GenerateRandom( MOC_HASH(hwAccelCtx)
                                id, iterations, salt, pSalt->length,
                                uniPassword, uniPassLen,
                                key, keyLen);
    CS_stopaccess(s, salt);
    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_decryptAux(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
               ASN1_ITEMPTR pEncryptedData,
               ASN1_ITEMPTR pAlgoIdentifier,
               CStream s, const ubyte* uniPassword,
               sbyte4 uniPassLen,
               ubyte** decryptedInfo,
               sbyte4* decryptedInfoLen)
{
    MSTATUS             status;
    BulkCtx             bulkCtx = 0;
    PCKS12CipherSuite*  pCipherSuite;
    ubyte               encKey[PKCS12_PBE_MAX_KEY_LENGTH];
    ubyte               iv[PKCS12_PBE_MAX_IV_LENGTH];
    ubyte               pbeSubType;
    ubyte*              salt;
    sbyte4              iterations;
    ASN1_ITEMPTR        pAlgoIdentifierOID,
                        pPKCS12PbeParams,
                        pSalt,
                        pIterations;


    /* AlgoIdentifierOID is first child */
    pAlgoIdentifierOID = ASN1_FIRST_CHILD( pAlgoIdentifier);
    if (!pAlgoIdentifierOID) return ERR_PKCS12_INVALID_STRUCT;

    status = ASN1_VerifyOIDRoot( pAlgoIdentifierOID, s, pkcs12_Pbe_root_OID, &pbeSubType);
    if (status < OK)
        return status;

    if (NULL == (pCipherSuite = PKCS12_getCipherSuite((sbyte4)pbeSubType)))
        return ERR_PKCS12_UNSUPPORTED_ALGO;

    /* get salt and iteration count */
    pPKCS12PbeParams = ASN1_NEXT_SIBLING( pAlgoIdentifierOID);
    if (NULL == pPKCS12PbeParams) return ERR_PKCS12_INVALID_STRUCT;
    status = ASN1_VerifyType( pPKCS12PbeParams, SEQUENCE);
    if ( status < OK) return status;

    /* salt */
    pSalt = ASN1_FIRST_CHILD( pPKCS12PbeParams);
    if (NULL == pSalt) return ERR_PKCS12_INVALID_STRUCT;
    status = ASN1_VerifyType( pSalt, OCTETSTRING);
    if ( status < OK) return status;

    pIterations = ASN1_NEXT_SIBLING( pSalt);
    /* Iterations must be present for pkcs-12PbeParams no default */
    if ( NULL == pIterations || OK != ASN1_VerifyType( pIterations, INTEGER))
    {
        return ERR_PKCS12_INVALID_STRUCT;
    }
    iterations = pIterations->data.m_intVal;

    salt = (ubyte*) CS_memaccess( s, pSalt->dataOffset, pSalt->length);
    if ( 0 == salt)
    {
        return ERR_MEM_ALLOC_FAIL;
    }

    /* derive the keys now */
    /* for encryption/decryption, id = 1 */
    status = PKCS12_SHA1_GenerateRandom( MOC_HASH(hwAccelCtx)
                            1, iterations, salt, pSalt->length,
                            uniPassword, uniPassLen,
                            encKey, pCipherSuite->keySize);

    /* for IV, id = 2 */
    if (OK == status && pCipherSuite->pBEAlgo->blockSize > 0)
    {
        status = PKCS12_SHA1_GenerateRandom( MOC_HASH(hwAccelCtx)
                            2, iterations, salt, pSalt->length,
                            uniPassword, uniPassLen,
                            iv, pCipherSuite->pBEAlgo->blockSize);
    }
    CS_stopaccess(s, salt);
    if ( status < OK) return status;

    /* decrypt */
    /* REVIEW MOC_SYM */
    bulkCtx = (pCipherSuite->pBEAlgo->createFunc)(MOC_SYM(hwAccelCtx)
                                                encKey, pCipherSuite->keySize, 0);
    if ( 0 == bulkCtx) { return ERR_MEM_ALLOC_FAIL; }

    /* call the shared routine */
    status = PKCS_BulkDecrypt(MOC_SYM(hwAccelCtx) pEncryptedData, s, bulkCtx, pCipherSuite->pBEAlgo, iv,
                        decryptedInfo, decryptedInfoLen);

    (pCipherSuite->pBEAlgo->deleteFunc)(MOC_SYM(hwAccelCtx) &bulkCtx);

    return status;
}


/*---------------------------------------------------------------------*/

MSTATUS
PKCS12_decrypt(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
               ASN1_ITEMPTR pEncryptedData,
               ASN1_ITEMPTR pAlgoIdentifier,
               CStream s, const ubyte* password,
               sbyte4 passwordLen,
               ubyte** decryptedInfo,
               sbyte4* decryptedInfoLen)
{
    MSTATUS    status;
    ubyte*     uniPassword = 0; /* allocated unicode password if necessary */

    if (!pEncryptedData || !pAlgoIdentifier || !password ||
        !decryptedInfo || !decryptedInfoLen)
    {
        return ERR_NULL_POINTER;
    }

    /* is the password unicode ? use a simple heuristic */
    if (0 != *password)
    {
        sbyte4 i;

        uniPassword = MALLOC( 2 * passwordLen + 2);
        if (!uniPassword)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMSET( uniPassword, 0, 2 * passwordLen + 2);

        for (i = 0; i < passwordLen; ++i)
        {
            uniPassword[i*2+1] = password[i];
        }

        password = uniPassword;
        passwordLen = 2 * passwordLen + 2;
    }

    status = PKCS12_decryptAux( MOC_SYM_HASH(hwAccelCtx)
               pEncryptedData, pAlgoIdentifier, s,
               password, passwordLen,
               decryptedInfo, decryptedInfoLen);

exit:

    if (uniPassword)
    {
        FREE(uniPassword);
    }

    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_encryptAux(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                  ubyte pbeSubType,
                  const ubyte* uniPassword, sbyte4 uniPassLen,
                  const ubyte* salt, ubyte4 saltLen, ubyte4 iterCount,
                  ubyte* plainText, sbyte4 plainTextLen,
                  byteBoolean mode, 
                  ubyte** ppHashOutput, ubyte4 *pHashOutputLen)
{
    MSTATUS             status;
    PCKS12CipherSuite*  pCipherSuite;
    ubyte               encKey[PKCS12_PBE_MAX_KEY_LENGTH];
    ubyte               iv[PKCS12_PBE_MAX_IV_LENGTH];
    HMAC_CTX*           pHmacCtx = NULL;
    ubyte*              pHashOutput = NULL;

    if (NULL == (pCipherSuite = PKCS12_getCipherSuite((sbyte4)pbeSubType)))
        return ERR_PKCS12_UNSUPPORTED_ALGO;

    /* derive the keys now */
    /* for encryption/decryption, id = 1 */
    if (OK > (status = PKCS12_SHA1_GenerateRandom( MOC_HASH(hwAccelCtx)
                                                   ( mode ? 1 : 3), iterCount, salt, saltLen,
                                                   uniPassword, uniPassLen,
                                                   encKey, (mode ? pCipherSuite->keySize : SHA_HASH_RESULT_SIZE))))
    {
        goto exit;
    }

    if (mode)
    {
        /* for IV, id = 2 */
        if (pCipherSuite->pBEAlgo->blockSize > 0)
        {
            if (OK > (status = PKCS12_SHA1_GenerateRandom( MOC_HASH(hwAccelCtx)
                                                           2, iterCount, salt, saltLen,
                                                           uniPassword, uniPassLen,
                                                           iv, pCipherSuite->pBEAlgo->blockSize)))
            {
                goto exit;
            }
        }

        /* encrypt */
        if (OK > ( status = CRYPTO_Process( MOC_SYM(hwAccelCtx)
                                            pCipherSuite->pBEAlgo,
                                            encKey, pCipherSuite->keySize,
                                            iv, plainText, plainTextLen, 1)))
        {
            goto exit;
        }
    }
    else
    {
        /* hash code */
        const BulkHashAlgo*    pBHA;

        if (!ppHashOutput || !pHashOutputLen)
        {
            status = ERR_NULL_POINTER;
            goto exit;
        }

        if (OK > (status = CRYPTO_getRSAHashAlgo(ht_sha1, &pBHA)))
            goto exit;

        if (OK > (status = HmacCreate(MOC_HASH(hwAccelCtx) &pHmacCtx, pBHA)))
            goto exit;

        if (OK > (status = HmacKey(MOC_HASH(hwAccelCtx) pHmacCtx, encKey, SHA_HASH_RESULT_SIZE/*pCipherSuite->keySize*/)))
            goto exit;

        if (OK > (status = HmacUpdate(MOC_HASH(hwAccelCtx) pHmacCtx, plainText, plainTextLen)))
            goto exit;

        if (NULL == (pHashOutput = MALLOC(pBHA->digestSize)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        if (OK > (status = HmacFinal(MOC_HASH(hwAccelCtx) pHmacCtx, pHashOutput)))
            goto exit;

        *ppHashOutput = pHashOutput;
        *pHashOutputLen = pBHA->digestSize;
    }

exit:

    if (pHmacCtx)
    {
        HmacDelete(MOC_HASH(hwAccelCtx) &pHmacCtx);
        if (OK > status)
        {
            if (pHashOutput)
                FREE(pHashOutput);
        }
    }

    return status;

}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_encryptEx(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                 ubyte pbeSubType,
                 const ubyte* password, sbyte4 passwordLen,
                 const ubyte* salt, sbyte4 saltLen, ubyte4 iterCount,
                 ubyte* plainText, sbyte4 plainTextLen,
                 byteBoolean mode,
                 ubyte** ppHashOutput, ubyte4 *pHashOutput)
{

    MSTATUS    status;
    ubyte*     uniPassword = 0; /* allocated unicode password if necessary */

    if (!password || !salt || !plainText)
    {
        return ERR_NULL_POINTER;
    }

    /* is the password unicode ? use a simple heuristic */
    if (0 != *password)
    {
        sbyte4 i;

        uniPassword = MALLOC( 2 * passwordLen + 2);
        if (!uniPassword)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMSET( uniPassword, 0, 2 * passwordLen + 2);

        for (i = 0; i < passwordLen; ++i)
        {
            uniPassword[i*2+1] = password[i];
        }

        password = uniPassword;
        passwordLen = 2 * passwordLen + 2;
    }

    status = PKCS12_encryptAux( MOC_SYM_HASH(hwAccelCtx)
                                pbeSubType,
                                password, passwordLen,
                                salt, saltLen, iterCount,
                                plainText, plainTextLen,
                                mode,
                                ppHashOutput, pHashOutput);

exit:

    if (uniPassword)
    {
        FREE(uniPassword);
    }

    return status;
}

/*---------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
PKCS12_encrypt(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
               ubyte pbeSubType,
               const ubyte* password, sbyte4 passwordLen,
               const ubyte* salt, sbyte4 saltLen, ubyte4 iterCount,
               ubyte* plainText, sbyte4 plainTextLen)
{
    return PKCS12_encryptEx(MOC_SYM_HASH(hwAccelCtx)
                            pbeSubType,
                            password, passwordLen,
                            salt, saltLen, iterCount,
                            plainText, plainTextLen,
                            TRUE,
                            NULL, NULL);
}

/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_GenerateMac(MOC_HASH(hwAccelDescr hwAccelCtx)
                        ASN1_ITEM* pT,
                        CStream s,
                        ubyte* hmacKey,
                        sbyte4 hmacKeyLen,
                        ubyte hmacRes[SHA_HASH_RESULT_SIZE])
{

    MSTATUS status;
    HMAC_CTX* pHmacCtx = 0;
    ubyte* hmacInput = 0;
    const BulkHashAlgo* pBHA;

    if (OK > ( status = CRYPTO_getRSAHashAlgo( ht_sha1, &pBHA)))
    {
        goto exit;
    }

    if (OK > ( status = HmacCreate(MOC_HASH(hwAccelCtx) &pHmacCtx, pBHA)))
    {
        goto exit;
    }

    if (OK > ( status = HmacKey( MOC_HASH(hwAccelCtx) pHmacCtx,
                                hmacKey, (ubyte4) hmacKeyLen)))
    {
        goto exit;
    }

    while (OK <= ASN1_VerifyType( pT, OCTETSTRING))
    {
        /* compute HMAC of pT -- take into account pT can be BER encoded .... */
        hmacInput = (ubyte*) CS_memaccess(s, pT->dataOffset, pT->length);
        if ( 0 == hmacInput)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
        if (OK > ( status  = HmacUpdate( MOC_HASH(hwAccelCtx) pHmacCtx,
                                            hmacInput, pT->length)))
        {
            goto exit;
        }
        CS_stopaccess(s, hmacInput);
        hmacInput = 0;
        pT = ASN1_NEXT_SIBLING( pT);
    }

    if (OK > ( status = HmacFinal( MOC_HASH(hwAccelCtx) pHmacCtx, hmacRes)))
    {
        goto exit;
    }

exit:
    if (hmacInput)
    {
        CS_stopaccess(s, hmacInput);
    }

    if (pHmacCtx)
    {
        HmacDelete(MOC_HASH(hwAccelCtx) &pHmacCtx);
    }

    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_VerifyMac(MOC_HASH(hwAccelDescr hwAccelCtx)
                    ASN1_ITEM* pContentType,
                    CStream s,
                    ubyte* hmacKey,
                    ubyte* digest)
{
    MSTATUS status;
    ubyte hmacRes[SHA_HASH_RESULT_SIZE];
    sbyte4 memCmpResult;

    status = PKCS12_GenerateMac(MOC_HASH(hwAccelCtx) pContentType,
                        s, hmacKey, SHA_HASH_RESULT_SIZE, hmacRes);

    if ( status < OK) return status;

    /* compare result with expected digest */
    status = MOC_MEMCMP( digest, hmacRes, SHA_HASH_RESULT_SIZE, &memCmpResult);
    if ( status < OK) return status;

    if ( memCmpResult)
    {
        return ERR_PKCS12_INTEGRITY_CHECK_FAILED;
    }

    return OK;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_CallHandler( const PKCS12CallbackInfo* handlerInfo, contentTypes type, 
                   ubyte4 extraInfo, const ubyte* content, ubyte4 contentLen)
{
    MSTATUS status = OK;
    switch (handlerInfo->type)
    {
        case 0:
            if (handlerInfo->func.handler)
            {
                status = handlerInfo->func.handler( type, extraInfo, 
                                             content, contentLen);
            }
            break;
            
        case 1:
            if (handlerInfo->func.handlerEx)
            {
                status = handlerInfo->func.handlerEx( handlerInfo->context, type,
                                               extraInfo, content, contentLen);
            }
            break;
            
        default: /* ??? */
            status = ERR_INTERNAL_ERROR;
            break;        
    }
    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessKeyBag( ASN1_ITEMPTR pSafeBag, CStream s,
                     const PKCS12CallbackInfo* handlerInfo)
{
    MSTATUS status = OK;
    const ubyte* privateKeyInfo = NULL;
    ASN1_ITEMPTR  pPrivateKeyInfoItem;

    if (pSafeBag)
    {
        if (OK > (status = ASN1_WalkTree(pSafeBag, s, gotoPKCS12FromSafeBagToBagValue, &pPrivateKeyInfoItem)))
            goto exit;

        //privateKeyInfo = CS_memaccess(s, pSafeBag->dataOffset - pSafeBag->headerSize, 
        //                              pSafeBag->length + pSafeBag->headerSize);
        if (NULL == (privateKeyInfo = CS_memaccess(s,
                                                  pPrivateKeyInfoItem->dataOffset - pPrivateKeyInfoItem->headerSize,
                                                  pPrivateKeyInfoItem->length + pPrivateKeyInfoItem->headerSize)))
        {
            status = ERR_NULL_POINTER;
            goto exit;
        }

        if (OK > (status = PKCS12_CallHandler(handlerInfo, KEYINFO, 0, 
                                              privateKeyInfo,
                                              pPrivateKeyInfoItem->length + pPrivateKeyInfoItem->headerSize)))
        {
            goto exit;
        }
    }

exit:
    if (privateKeyInfo)
    {
        CS_stopaccess(s, privateKeyInfo);
    }
    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessPKCS8ShroudedKeyBag( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                            ASN1_ITEMPTR pSafeBag,
                            CStream s, const ubyte* uniPassword,
                            sbyte4 uniPassLen, ubyte** privateKeyInfo,
                            sbyte4* privateKeyInfoLen,
                            const PKCS12CallbackInfo* handlerInfo)
{
    ASN1_ITEMPTR        pEncryptedKeyInfo;
    MSTATUS             status;

    if ( NULL == privateKeyInfo || NULL == privateKeyInfoLen)
    {
        return ERR_NULL_POINTER;
    }

    *privateKeyInfo = 0;
    *privateKeyInfoLen = 0;

    /* go to bag value -- in this case, this is a PKCS8 EncryptedPrivateKeyInfo */
    if (OK > (status = ASN1_WalkTree( pSafeBag, s,
                            gotoPKCS12FromSafeBagToBagValue,
                            &pEncryptedKeyInfo)))
    {
        return status;
    }

    if (OK > (status = PKCS_DecryptPKCS8Key( MOC_SYM_HASH(hwAccelCtx)
                            pEncryptedKeyInfo, s, uniPassword, uniPassLen,
                            privateKeyInfo, privateKeyInfoLen)))
    {
        return status;
    }

    return PKCS12_CallHandler( handlerInfo, KEYINFO, 0, 
                              *privateKeyInfo, *privateKeyInfoLen);
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessCertBag( MOC_HASH(hwAccelDescr hwAccelCtx)
                                ASN1_ITEMPTR pSafeBag,
                                CStream s, const ubyte* uniPassword,
                                sbyte4 uniPassLen,
                                const PKCS12CallbackInfo* handlerInfo)
{
    MSTATUS         status;
    ASN1_ITEMPTR    pCertBag;
    ubyte           certType;
    ASN1_ITEMPTR    pCertId;
    ASN1_ITEMPTR    pCertificate;

    static WalkerStep gotoPKCS12FromCertBagToX509Cert[] =
    {
        { GoNthChild, 2, 0},
        { VerifyTag, 0, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, OCTETSTRING, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0},  /* X.509 certificate */
        { Complete, 0, 0}
    };

    static WalkerStep gotoPKCS12FromCertBagToSDSICert[] =
    {
        { GoNthChild, 2, 0},
        { VerifyTag, 0, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, IA5STRING, 0 }, /* SDSI certificate (Base 64 encoded) */
        { Complete, 0, 0}
    };

    MOC_UNUSED(uniPassword);
    MOC_UNUSED(uniPassLen);

    /* go to bag value -- in this case, this is a CertBag */
    status = ASN1_WalkTree( pSafeBag, s,
                            gotoPKCS12FromSafeBagToBagValue,
                            &pCertBag);
    if ( status < OK ) return status;

    /* verify Type */
    if ( OK != ASN1_VerifyType( pCertBag, SEQUENCE))
    {
        return ERR_PKCS12_INVALID_STRUCT;
    }
    /* look at the certificate type */
    pCertId = ASN1_FIRST_CHILD( pCertBag);

    if (!pCertId)
    {
        return ERR_PKCS12_INVALID_STRUCT;
    }

    status = ASN1_VerifyOIDRoot( pCertId, s, pkcs9_pkcs12_certtypes_root_OID, &certType);
    if ( OK != status)
    {
        return  ERR_PKCS12_NOT_EXPECTED_OID;
    }

    switch ( certType)
    {
        case 1: /* DER encoded X509 certificate */
            status = ASN1_WalkTree( pCertBag, s,
                            gotoPKCS12FromCertBagToX509Cert,
                            &pCertificate);
            if ( status < OK ) return status;
            /* now what do we do with pCert ?*/
            break;

        case 2: /* Base 64 encoded SDSI certificate */
            status = ASN1_WalkTree( pCertBag, s,
                            gotoPKCS12FromCertBagToSDSICert,
                            &pCertificate);
            if ( status < OK ) return status;
            /* now what do we do with pCert ?*/
            break;

        default:
            return ERR_PKCS12;
            break;
    }

    if (pCertificate )
    {
        const ubyte* cert = CS_memaccess(s, pCertificate->dataOffset - pCertificate->headerSize, pCertificate->length + pCertificate->headerSize);
        status = PKCS12_CallHandler( handlerInfo, CERT, certType, cert, pCertificate->length + pCertificate->headerSize);
        CS_stopaccess(s, cert);
    }

    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessCrlBag( MOC_HASH(hwAccelDescr hwAccelCtx)
                            ASN1_ITEMPTR pSafeBag,
                            CStream s, const ubyte* uniPassword,
                            sbyte4 uniPassLen,
                            const PKCS12CallbackInfo* handlerInfo)
{
    MOC_UNUSED(pSafeBag);
    MOC_UNUSED(s);
    MOC_UNUSED(uniPassword);
    MOC_UNUSED(uniPassLen);

    return OK;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessSecretBag( MOC_HASH(hwAccelDescr hwAccelCtx)
                            ASN1_ITEMPTR pSafeBag,
                            CStream s, const ubyte* uniPassword,
                            sbyte4 uniPassLen,
                            const PKCS12CallbackInfo* handlerInfo)
{
    MOC_UNUSED(pSafeBag);
    MOC_UNUSED(s);
    MOC_UNUSED(uniPassword);
    MOC_UNUSED(uniPassLen);

    return OK;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessSafeContentsBag( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                            ASN1_ITEMPTR pSafeBag,
                            CStream s, const ubyte* uniPassword,
                            sbyte4 uniPassLen,
                            const PKCS12CallbackInfo* handlerInfo)
{
    MSTATUS status;
    ASN1_ITEMPTR pSafeContents;
    /* go to bag value -- in this case, this is a SafeContents */
    status = ASN1_WalkTree( pSafeBag, s,
                            gotoPKCS12FromSafeBagToBagValue,
                            &pSafeContents);
    if ( status < OK ) return status;

    /* verify Type */
    if ( OK != ASN1_VerifyType( pSafeContents, SEQUENCE))
    {
        return ERR_PKCS12_INVALID_STRUCT;
    }
    /* get to first SafeBag and recursive call */
    return PKCS12_ProcessSafeContents( MOC_SYM_HASH(hwAccelCtx)
                                            pSafeContents,
                                            s, uniPassword, uniPassLen,
                                           handlerInfo);
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessSafeBags(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                       ASN1_ITEMPTR pSafeBag,
                       CStream s, const ubyte* uniPassword,
                       sbyte4 uniPassLen,
                       const PKCS12CallbackInfo* handlerInfo)
{
    MSTATUS   status = OK;
    ubyte*    info = NULL;
    sbyte4    infoLen = 0;

    while ( pSafeBag)
    {
        ubyte subType;
        ASN1_ITEMPTR pBagId;

        /* verify Type */
        if ( OK != ASN1_VerifyType( pSafeBag, SEQUENCE))
        {
            status = ERR_PKCS12_INVALID_STRUCT;
            goto exit;
        }
        /* look at the bag type */
        pBagId = ASN1_FIRST_CHILD( pSafeBag);

        if (!pBagId)
        {
            status = ERR_PKCS12_INVALID_STRUCT;
            goto exit;
        }

        status = ASN1_VerifyOIDRoot( pBagId, s, pkcs12_bagtypes_root_OID, &subType);
        if ( OK != status)
        {
            status = ERR_PKCS12_NOT_EXPECTED_OID;
            goto exit;
        }

        switch ( subType)
        {
            case 1: /* keybag aka PKCS8 PrivateKeyInfo*/
                status = PKCS12_ProcessKeyBag( pSafeBag, s, handlerInfo);
                break;

            case 2: /* pkcs8ShroudedKetBag aka PKCS8 EncryptedKeyInfo */
                status = PKCS12_ProcessPKCS8ShroudedKeyBag( MOC_SYM_HASH(hwAccelCtx)
                                                            pSafeBag, s, uniPassword, uniPassLen,
                                                            &info, &infoLen, handlerInfo);
                if ( OK == status)
                {
#if _DEBUG
                    /* info contain the Encoding of a PrivateKeyInfo */
                    MemFile mf;
                    CStream mfs;
                    ASN1_ITEMPTR pPrivateKeyInfo;

                    MF_attach(&mf, infoLen, info);
                    CS_AttachMemFile( &mfs, &mf);

                    if (OK == ASN1_Parse( mfs, &pPrivateKeyInfo))
                    {
                        /* how do we report it */
                        FILE* f = fopen("privatekey.asn", "wb");
                        if ( f)
                        {
                            fwrite( info, 1, infoLen, f);
                            fclose(f);
                        }

                        TREE_DeleteTreeItem( (TreeItem*) pPrivateKeyInfo);
                    }
#endif

                    FREE( info);
                    info = NULL;
                }
                break;

            case 3: /* certBag */
                status = PKCS12_ProcessCertBag( MOC_HASH(hwAccelCtx)
                        pSafeBag, s, uniPassword, uniPassLen, handlerInfo);
                break;

            case 4: /* crlBag*/
                status = PKCS12_ProcessCrlBag( MOC_HASH(hwAccelCtx)
                        pSafeBag, s, uniPassword, uniPassLen, handlerInfo);
                break;

            case 5: /* secret Bag */
                status = PKCS12_ProcessSecretBag( MOC_HASH(hwAccelCtx)
                        pSafeBag, s, uniPassword, uniPassLen, handlerInfo);
                break;

            case 6: /* safeContentsBag */
                status = PKCS12_ProcessSafeContentsBag( MOC_SYM_HASH(hwAccelCtx)
                        pSafeBag, s, uniPassword, uniPassLen, handlerInfo);
                break;

            default: /* extensions that we don't know about -> ignore, no error */
                break;
        }
        if (OK > status)
          goto exit;

        pSafeBag = ASN1_NEXT_SIBLING( pSafeBag);
    }

exit:
    if (OK > status)
    {
        if (info)
            FREE(info);
    }

    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessSafeContents(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pSafeContents,
                          CStream s, const ubyte* uniPassword, sbyte4 uniPassLen,
                          const PKCS12CallbackInfo* handlerInfo)
{
    ASN1_ITEMPTR pFirstSafeBag = ASN1_FIRST_CHILD( pSafeContents);

    if (pFirstSafeBag)
    {
        return PKCS12_ProcessSafeBags( MOC_SYM_HASH( hwAccelCtx) pFirstSafeBag,
                                s, uniPassword, uniPassLen, handlerInfo);
    }
    return OK;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessBERCIData(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pBERCIData,
                              CStream s, const ubyte* uniPassword, sbyte4 uniPassLen,
                              const PKCS12CallbackInfo* handlerInfo)
{
    ASN1_ITEMPTR pSafeContents;
    MSTATUS status;
    MemFile mf;
    CStream newS;
    ASN1_ITEMPTR pNewRoot = 0;
    const void* berData = 0;
    ubyte* consolidatedData = 0;

    ASN1_ITEMPTR pTemp;
    ubyte* dest ;
    ubyte4 dataSize;

    /* access first OCTET STRING blob and store the length */
    if (NULL == (pTemp = ASN1_NEXT_SIBLING(pBERCIData)))
    {
        return ERR_PKCS12_INVALID_STRUCT;
    }
    dataSize = pBERCIData->length + pTemp->length;

    /* access succeeding OCTET STRING blobs and store the lengths */
    for (pTemp = ASN1_NEXT_SIBLING(pTemp);
        OK <= ASN1_VerifyType( pTemp, OCTETSTRING);
        pTemp = ASN1_NEXT_SIBLING(pTemp))
    {
        dataSize += pTemp->length;
    }

    /* allocate new mem to store the OCTET STRING blob */
    consolidatedData = MALLOC( dataSize);
    if (!consolidatedData)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    dest = consolidatedData;
    /* reset ASN1 pointer */
    pTemp = pBERCIData;
    while (OK <=  ASN1_VerifyType( pTemp, OCTETSTRING))
    {
        berData = CS_memaccess( s, pTemp->dataOffset, pTemp->length);
        if ( !berData)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
        MOC_MEMCPY( dest, berData, pTemp->length);
        dest += pTemp->length;
        CS_stopaccess( s, berData);
        berData = 0;
        pTemp = ASN1_NEXT_SIBLING(pTemp);
    }

    /* register new memory filestream to access this OCTET STRING blob */
    MF_attach( &mf, dataSize, (void*) consolidatedData);

#ifdef DEBUG_PKCS12
    {
        FILE *f;
        strcat(gDebFileName, ".insideCI.p12");
        f = fopen(gDebFileName, "wb");
        fwrite(consolidatedData, dataSize, 1, f);
        fclose(f);
    }
#endif

    /* Parse the blob we got */
    CS_AttachMemFile( &newS, &mf);
    status = ASN1_Parse(newS, &pNewRoot);
    if ( status < OK) goto exit;

    /* pT = pNewRoot; */
    s = newS;

    /* SEQUENCE - SafeContents */
    pSafeContents = ASN1_FIRST_CHILD( pNewRoot);
    if (!pSafeContents)
        return ERR_PKCS12_INVALID_STRUCT;
    
    status = ASN1_VerifyType( pSafeContents, SEQUENCE);
    if (status < OK) 
        goto exit;

    /* Ta da!  Now we have the ptr to SafeContents */
    status = PKCS12_ProcessSafeContents(MOC_SYM_HASH(hwAccelCtx) pSafeContents,
        s, uniPassword, uniPassLen, handlerInfo);


exit: 
    if (pNewRoot)
    {
        TREE_DeleteTreeItem( (TreeItem*) pNewRoot);
    }

    if (berData)
    {
        CS_stopaccess( s, berData);
    }

    if (consolidatedData)
        FREE(consolidatedData);


    return status;
}

/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessDataContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pContentInfo,
                              CStream s, const ubyte* uniPassword, sbyte4 uniPassLen,
                              const PKCS12CallbackInfo* handlerInfo)
{
    static WalkerStep gotoPKCS12FromContentInfoToData[] =
    {
        { GoNthChild, 2, 0},
        { VerifyTag, 0, 0 },
        { GoFirstChildBER, 0, 0},
        { VerifyType, OCTETSTRING, 0 },
        { GoFirstChild, 0, 0},
        { Complete, 0, 0}
    };

    /* data -> OCTET STRING encapsulating the SafeContents
    which is itself a sequence of SafeBag*/
    static WalkerStep gotoPKCS12FromContentInfoToFirstSafeBag[] =
    {
        { GoNthChild, 2, 0},
        { VerifyTag, 0, 0 },
        { GoFirstChildBER, 0, 0},
        { VerifyType, OCTETSTRING, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0},  /* SafeContents */
        { GoFirstChild, 0, 0 },      /* FirstSafeBag */
        { Complete, 0, 0}
    };

    /* ContentInfo data may contain another OCTET STRING chain */
    static WalkerStep gotoPKCS12BERFromContentInfoToCIData[] =
    {

        { GoNthChild, 2, 0},
        { VerifyTag, 0, 0},
        { GoFirstChildBER, 0, 0},
        { VerifyType, OCTETSTRING, 0 }, /* data = [0] OCTET STRING */
        { Complete, 0, 0 }
    };

    ASN1_ITEMPTR pFirstSafeBag, pBERCIData, pContentInfoRoot;
    MSTATUS status = OK;
    /* don't test status, might be OK to have no SafeBag at all */
    status = ASN1_WalkTree( pContentInfo, s,
                            gotoPKCS12FromContentInfoToFirstSafeBag,
                            &pFirstSafeBag);

    /* If the walk was successful, we've found the first safebag. */
    if (status == OK)
    {
        return PKCS12_ProcessSafeBags( MOC_SYM_HASH( hwAccelCtx) pFirstSafeBag,
                                s, uniPassword, uniPassLen, handlerInfo);
    }

    /* Otherwise, we check if it's another BER OCTET STRING */ 
    status = ASN1_WalkTree( pContentInfo, s,
                            gotoPKCS12FromContentInfoToData,
                            &pContentInfoRoot);
    if (NULL != pContentInfoRoot)
    {
        if (OK == (status = ASN1_VerifyType( pContentInfoRoot, SEQUENCE)))
        {
            if (0 == pContentInfoRoot->length) 
            {
                goto exit;
            }
        }
    }

    /* Otherwise, we check if it's another BER OCTET STRING */
    if (OK > (status = ASN1_WalkTree( pContentInfo, s,
                        gotoPKCS12BERFromContentInfoToCIData,
                        &pBERCIData)))
    {
        goto exit;
    }

    return PKCS12_ProcessBERCIData( MOC_SYM_HASH( hwAccelCtx) pBERCIData,
                            s, uniPassword, uniPassLen, handlerInfo);

exit:
    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ProcessEncryptedDataContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pContentInfo,
                          CStream s, const ubyte* uniPassword, sbyte4 uniPassLen,
                          const PKCS12CallbackInfo* handlerInfo)
{
    /* Encrypted Data (PKCS#7) */
    /* EncryptedData := SEQUENCE { version VERSION,
        encryptedContentInfo EncryptedContentInfo }
        EncryptedContentInfo := SEQUENCE { contentType ContentType,
            contentEncryptionAlgorithm ContentEncryptionAlgorithmIdentifier,
            encryptedContent [0] IMPLICIT EncryptedContent OPTIONAL }
        EncryptedContent := OCTET STRING
    */

    static WalkerStep gotoPKCS12FromContentInfoToEncryptedContentInfo[] =
    {
        { GoNthChild, 2, 0},
        { VerifyTag, 0, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, INTEGER, 0},   /* VERSION */
        { GoNextSibling, 0, 0 },     /* EncryptedContentInfo */
        { VerifyType, SEQUENCE, 0},
        { Complete, 0, 0}
    };

    ASN1_ITEMPTR        pEncryptedContentInfo;
    ASN1_ITEMPTR        pContentType;
    ASN1_ITEMPTR        pAlgoIdentifier;
    ASN1_ITEMPTR        pEncryptedContent;
    MSTATUS             status;
    ubyte*              decrypted = 0;
    sbyte4              decryptedLen;

    MemFile             mf;
    CStream             mfs;
    ASN1_ITEMPTR        pDecrypted = 0;
    ASN1_ITEMPTR        pSafeContents;

    /* don't test status, might be OK to have no SafeBag at all */
    status = ASN1_WalkTree( pContentInfo, s,
                            gotoPKCS12FromContentInfoToEncryptedContentInfo,
                            &pEncryptedContentInfo);
    if ( status < OK) return status;

    /* first child of content info is contentType */
    pContentType = ASN1_FIRST_CHILD(pEncryptedContentInfo);
    if ( !pContentType) return ERR_PKCS12_INVALID_STRUCT;

    /* second child is ContentEncryptionAlgorithmIdentifier */
    pAlgoIdentifier = ASN1_NEXT_SIBLING( pContentType);
    if (!pAlgoIdentifier) return ERR_PKCS12_INVALID_STRUCT;
    status = ASN1_VerifyType( pAlgoIdentifier, SEQUENCE);
    if (status < OK) return status;

    /* third child is the [0] encrypted content (OCTET STRING) */
    pEncryptedContent = ASN1_NEXT_SIBLING(pAlgoIdentifier);
    if (!pEncryptedContent) return OK; /* OPTIONAL */
    status = ASN1_VerifyTag( pEncryptedContent, 0);
    if ( status < OK) return ERR_PKCS12_INVALID_STRUCT;

    /* decrypt */
    status = PKCS12_decrypt(MOC_SYM_HASH(hwAccelCtx)
                        pEncryptedContent, pAlgoIdentifier,
                        s, uniPassword, uniPassLen,
                        &decrypted, &decryptedLen);
    if ( status < OK ) return status;

    /* decrypted is a SafeContents, that is a sequence of SafeBags */
    MF_attach(&mf, decryptedLen, decrypted);
    CS_AttachMemFile( &mfs, &mf);
    status = ASN1_Parse( mfs, &pDecrypted);
    if ( status < OK) goto exit;

#ifdef _DEBUG
    {
        /* how do we report it */
        FILE* f = fopen("c:\\ws\\src\\pkcs12_dev\\decrypted.asn", "wb");
    if ( f)
    {
        fwrite( decrypted, 1, decryptedLen, f);
        fclose(f);
    }
    }
#endif

    pSafeContents = ASN1_FIRST_CHILD( pDecrypted);
    if ( 0 == pSafeContents || ASN1_VerifyType( pSafeContents, SEQUENCE) < OK)
    {
        goto exit;
    }

    status = PKCS12_ProcessSafeContents( MOC_SYM_HASH( hwAccelCtx) pSafeContents,
                                        mfs, uniPassword, uniPassLen, handlerInfo);

exit:

    if (decrypted)
    {
        FREE( decrypted);
    }

    if (pDecrypted)
    {
        TREE_DeleteTreeItem( (TreeItem*) pDecrypted);
    }

    return status;
}

/*---------------------------------------------------------------------*/
static MSTATUS PKCS12_ProcessEnvelopedDataContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                                                      ASN1_ITEMPTR pContentInfo,
                                                      CStream s,
                                                      const ubyte* uniPassword, ubyte4 uniPassLen,
                                                      PKCS7_Callbacks* pkcs7CBs,
                                                      const PKCS12CallbackInfo* handlerInfo)
{
    MSTATUS        status = OK;
    ubyte*         pDecryptedContent = NULL;
    sbyte4         decryptedContentLen = 0;
    ASN1_ITEMPTR   pEnvelopedData = NULL,
                   pDecryptedData = NULL,
                   pSafeContents = NULL;
    MemFile        memFileSC;
    CStream        csSC;

    static WalkerStep contentInfoRootToContent[] =
    {
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},
        { VerifyOID, 0, (ubyte *)pkcs7_envelopedData_OID},
        { GoNextSibling, 0, 0 },
        { VerifyTag, 0, 0 },
        { GoFirstChild, 0, 0 },
        { Complete, 0, 0}
    };

    if (!pkcs7CBs->getPrivKeyFun)
    {
        status = ERR_PKCS12_DECRYPT_CALLBACK_NOT_SET;
        goto exit;
    }

    if (OK > (status = ASN1_WalkTree(pContentInfo, s, contentInfoRootToContent, &pEnvelopedData)))
        goto exit;

    if (OK > (status = PKCS7_DecryptEnvelopedData(MOC_RSA_SYM(hwAccelCtx)
                                                  pEnvelopedData,
                                                  s,
                                                  pkcs7CBs->getPrivKeyFun,
                                                  &pDecryptedContent,
                                                  &decryptedContentLen)))
        goto exit;

    /* decryptedContent contains SafeContents */
    MF_attach(&memFileSC, decryptedContentLen, pDecryptedContent);
    CS_AttachMemFile(&csSC, &memFileSC);

    if (OK > (status = ASN1_Parse(csSC, &pDecryptedData)))
        goto exit;

    if (NULL == (pSafeContents = ASN1_FIRST_CHILD(pDecryptedData)))
    {
        status = ERR_PKCS12_INVALID_STRUCT;
        goto exit;
    }

    if (OK > (status = ASN1_VerifyType(pSafeContents, SEQUENCE)))
        goto exit;

    status = PKCS12_ProcessSafeContents(MOC_SYM_HASH(hwAccelCtx)
                                        pSafeContents,
                                        csSC,
                                        uniPassword, uniPassLen,
                                        handlerInfo);

exit:

    if (pDecryptedContent)
        FREE(pDecryptedContent);
    if (pDecryptedData)
        TREE_DeleteTreeItem((TreeItem *)pDecryptedData);

    return status;
}
/*---------------------------------------------------------------------*/

static MSTATUS
PCKS12_ProcessContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pContentInfo,
                          ASN1_ITEM* pMacData, CStream s,
                          const ubyte* uniPassword, sbyte4 uniPassLen,
                          PKCS7_Callbacks* pkcs7CBs,
                          const PKCS12CallbackInfo* handlerInfo)
{
    MSTATUS status;
    ASN1_ITEMPTR pOID;
    ubyte subType;
    MOC_UNUSED(pMacData);


    pOID = ASN1_FIRST_CHILD( pContentInfo);
    if ( !pOID)
    {
        /* could be a EOC (BER encoding) */
        return (OK == ASN1_VerifyType( pContentInfo, EOC))? OK : ERR_PKCS12_INVALID_STRUCT;
    }

    if ( OK == ASN1_VerifyOIDRoot( pOID, s, pkcs7_root_OID, &subType))
    {
        switch (subType)
        {
            case 1: /* pkcs7_data_OID */
                status = PKCS12_ProcessDataContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                                       pContentInfo,
                                                       s,
                                                       uniPassword, uniPassLen,
                                                       handlerInfo);
                break;

            case 6:
            /* encryptedData -> */
                status = PKCS12_ProcessEncryptedDataContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                                                pContentInfo,
                                                                s, 
                                                                uniPassword, uniPassLen,
                                                                handlerInfo);
                break;

            case 3:
            /* envelopedData should not be supported in this MODE. Not sure though */
              status = PKCS12_ProcessEnvelopedDataContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                                              pContentInfo,
                                                              s,
                                                              uniPassword, uniPassLen,
                                                              pkcs7CBs,
                                                              handlerInfo);
                break;
            default:
                status = ERR_PKCS12_NOT_EXPECTED_OID;
                break;
        }
    }
    else
    {
        status = ERR_PKCS12_NOT_EXPECTED_OID;
    }
    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_PasswordIntegrityMode( MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                              ASN1_ITEM* pRootItem,
                              ASN1_ITEM* pContentType,
                              CStream s,
                              const ubyte* uniPassword,
                              sbyte4 uniPassLen,
                              PKCS7_Callbacks* pkcs7CBs,
                              const PKCS12CallbackInfo* handlerInfo)
{
    static WalkerStep gotoPKCS12MacData[] =
    {
        { GoFirstChild, 0, 0}, /* SEQUENCE */
        { GoNthChild, 3, 0},   /* MacData */
        { VerifyType, SEQUENCE, 0},
        { Complete, 0, 0 }
    };

    static WalkerStep gotoPKCS12FromMacDataToDigest[] =
    {
        { GoFirstChild, 0, 0}, /* DigestInfo */
        { VerifyType, SEQUENCE, 0 },
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0 },
        { GoFirstChild, 0, 0},          /* go down to look at the OID */
        { VerifyOID, 0, (ubyte*) sha1_OID},
        { GoParent, 0, 0},
        { GoNextSibling, 0, 0},
        { VerifyType, OCTETSTRING, 0}, /* the digest */
        { Complete, 0, 0}
    };

    /* the sibling of pContentType is an [0] OCTET STRING that encapsulates */
    /* the information to HMAC hash (this is actually PKCS7 Data )*/
    static WalkerStep gotoPKCS12FromContentTypeToT[] =
    {
        { GoNextSibling, 0, 0},
        { VerifyTag, 0, 0},
        { GoFirstChildBER, 0, 0},
        { VerifyType, OCTETSTRING, 0 }, /* data = [0] OCTET STRING */
        { Complete, 0, 0 }
    };

    static WalkerStep gotoPKCS12FromTToFirstContentInfo[] =
    {
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0}, /* Authenticated Safe */
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0 }, /* First ContentInfo */
        { Complete, 0, 0 }
    };

    ubyte hmacKey[SHA_HASH_RESULT_SIZE];
    ASN1_ITEMPTR pMacData, pDigest, pT, pContentInfo;
    ubyte* digest;
    MSTATUS status;
    ASN1_ITEMPTR pNewRoot = 0;
    const void* berData = 0;
    ubyte* consolidatedData = 0;
    MemFile mf;
    CStream newS;

    /* password integrity mode */
    /* generate key based on password */
    if (NULL == uniPassword)
    {
        return ERR_PKCS12_PASSWORD_NEEDED;
    }

    /* find the macData information */
    status = ASN1_WalkTree( pRootItem, s, gotoPKCS12MacData, &pMacData);
    if ( status < OK) goto exit;

    /* find the digest (and does some verifications on the way) */
    status = ASN1_WalkTree( pMacData, s, gotoPKCS12FromMacDataToDigest, &pDigest);
    if ( status < OK) goto exit;

    /* verify length of digest */
    if (SHA_HASH_RESULT_SIZE != pDigest->length)
    {
        status = ERR_PKCS12_INVALID_STRUCT;
        goto exit;
    }

    status = PKCS12_GenerateKey( MOC_HASH(hwAccelCtx)
                        3, pMacData, s, uniPassword,
                        uniPassLen, hmacKey, SHA_HASH_RESULT_SIZE);
    if ( status < OK) goto exit;


    /* get pointer to content to hash  */
    status = ASN1_WalkTree( pContentType, s, gotoPKCS12FromContentTypeToT, &pT);
    if (status < OK ) goto exit;

    /* compare the computed hash with the hash in the PKCS#12 */
    digest = (ubyte*) CS_memaccess(s, pDigest->dataOffset, pDigest->length);
    if ( 0 == digest)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    status = PKCS12_VerifyMac( MOC_HASH(hwAccelCtx) pT, s, hmacKey, digest);
    CS_stopaccess(s, digest);
    if ( status < OK) goto exit;

    if (!ASN1_FIRST_CHILD(pT)) /* BER encoded */
    {
        /* restart the parse but this may also be spread over
        several ASN1 OCTETSTRING !!! */
        ASN1_ITEMPTR pTemp;

        pTemp = ASN1_NEXT_SIBLING(pT);
        if ( OK <= ASN1_VerifyType( pTemp, OCTETSTRING)) /* spread over several OCTETSTRING */
        {
            ubyte* dest ;
            ubyte4 dataSize = pT->length + pTemp->length;

            for (pTemp = ASN1_NEXT_SIBLING(pTemp);
                OK <= ASN1_VerifyType( pTemp, OCTETSTRING);
                pTemp = ASN1_NEXT_SIBLING(pTemp))
            {
                dataSize += pTemp->length;
            }

            consolidatedData = MALLOC( dataSize);
            if (!consolidatedData)
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }

            dest = consolidatedData;
            pTemp = pT;
            while (OK <=  ASN1_VerifyType( pTemp, OCTETSTRING))
            {
                berData = CS_memaccess( s, pTemp->dataOffset, pTemp->length);
                if ( !berData)
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }
                MOC_MEMCPY( dest, berData, pTemp->length);
                dest += pTemp->length;
                CS_stopaccess( s, berData);
                berData = 0;
                pTemp = ASN1_NEXT_SIBLING(pTemp);
            }
            MF_attach( &mf, dataSize, (void*) consolidatedData);
#ifdef DEBUG_PKCS12
            {
                FILE *f;
                strcat(gDebFileName, ".contentinfo.p12");
                f = fopen(gDebFileName, "wb");
                fwrite(consolidatedData, dataSize, 1, f);
                fclose(f);
            }
#endif
        }
        else
        {
            berData = CS_memaccess( s, pT->dataOffset, pT->length);
            if (!berData)
            {
                 status = ERR_MEM_ALLOC_FAIL;
                 goto exit;
            }
            MF_attach( &mf, pT->length, (void*) berData);

#ifdef DEBUG_PKCS12
            {
                FILE *f;
                strcat(gDebFileName, ".contentinfo.p12");
                f = fopen(gDebFileName, "wb");
                fwrite(berData, pT->length, 1, f);
                fclose(f);
            }
#endif

        }

        CS_AttachMemFile( &newS, &mf);
        status = ASN1_Parse(newS, &pNewRoot);
        if ( status < OK) goto exit;

        pT = pNewRoot;
        s = newS;
    }

    status = ASN1_WalkTree( pT, s, gotoPKCS12FromTToFirstContentInfo, &pContentInfo);
    if ( status < OK) goto exit;
    while ( pContentInfo )
    {
        status = PCKS12_ProcessContentInfo(MOC_SYM_HASH(hwAccelCtx) pContentInfo, pMacData,
                                           s, uniPassword, uniPassLen, pkcs7CBs, handlerInfo);
        if ( status < OK) goto exit;
        pContentInfo = ASN1_NEXT_SIBLING( pContentInfo);
    }

exit:

    if (pNewRoot)
    {
        TREE_DeleteTreeItem( (TreeItem*) pNewRoot);
    }

    if (berData)
    {
        CS_stopaccess( s, berData);
    }

    if (consolidatedData)
    {
        FREE( consolidatedData);
    }

    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_PublicKeyIntegrityMode(MOC_RSA(hwAccelDescr hwAccelCtx) 
                              ASN1_ITEM* pContentType,
                              CStream s,
                              const ubyte* uniPassword,
                              sbyte4 uniPassLen,
                              PKCS7_Callbacks* pkcs7CBs,
                              const PKCS12CallbackInfo* handlerInfo)
 {
    /* the sibling of pContentType for PKCS#7 signed data is a */
    /* SIGNED DATA sequence */
    static WalkerStep gotoPKCS12FromContentTypeToT[] =
    {
        { GoNextSibling, 0, 0},
        { VerifyTag, 0, 0},
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0 },
        { Complete, 0, 0 }
    };

    static WalkerStep gotoPKCS12FromTToFirstContentInfo[] =
    {
        { GoNthChild, 3, 0 },
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0 },
        { VerifyOID, 0, (ubyte*)pkcs7_data_OID },
        { GoNextSibling, 0, 0 },
        { VerifyTag, 0, 0 },
        { GoFirstChildBER, 0, 0 },
        { VerifyType, OCTETSTRING, 0 },
        { GoFirstChild, 0, 0 },
        { VerifyType, SEQUENCE, 0 },
        { GoFirstChild, 0, 0 },
        { Complete, 0, 0 }
    };


    ASN1_ITEMPTR  pT = NULL,
                  pContentInfo = NULL;
    MSTATUS       status = OK;
    sbyte4        numSigners = 0;

    if (!pkcs7CBs || !pkcs7CBs->valCertFun)
    {
        status = ERR_PKCS12_DECRYPT_CALLBACK_NOT_SET;
        goto exit;
    }

    if (OK > (status = ASN1_WalkTree( pContentType, s, gotoPKCS12FromContentTypeToT, &pT)))
        goto exit;
    
    if (OK > (status = PKCS7_VerifySignedData(MOC_RSA(hwAccelCtx) pT, s, pkcs7CBs->getCertFun,
                                           pkcs7CBs->valCertFun, &numSigners)))
        goto exit;

    if ( 0 == numSigners)
    {
        status = ERR_PKCS12_NO_KNOWN_SIGNERS;
    }

    if (OK > (status = ASN1_WalkTree( pT, s, gotoPKCS12FromTToFirstContentInfo, &pContentInfo)))
      goto exit;

    while ( pContentInfo )
    {
        if (OK > (status = PCKS12_ProcessContentInfo(MOC_SYM_HASH(hwAccelCtx) pContentInfo, NULL,
                                                     s, uniPassword, uniPassLen, pkcs7CBs, handlerInfo)))
            goto exit;

        pContentInfo = ASN1_NEXT_SIBLING( pContentInfo);
    }

exit:
    return status;
}


/*---------------------------------------------------------------------*/

static MSTATUS
PKCS12_ExtractInfoAux(MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                    ASN1_ITEM* pRootItem,
                    CStream s,
                    const ubyte* uniPassword,
                    sbyte4 uniPassLen,
                    PKCS7_Callbacks* pkcs7CBs,
                    const PKCS12CallbackInfo* handlerInfo)
{
    /* PFX := SEQUENCE { version INTEGER (v3),
                        authSafe ContentInfo,
                        macData MacData OPTIONAL }

        MacData := SEQUENCE {
                mac DigestInfo,
                macSalt OCTET STRING,
                iterations INTEGER DEFAULT 1}

        DigestInfo ::= SEQUENCE {
                    digestAlgorithm DigestAlgorithmIdentifier,
                    digest          Digest }


        ContentInfo ::= SEQUENCE {
            contentType ContentType,
            content     [0] EXPLICIT CONTENTS.&Type({Contents}{@ContentType}) OPTIONAL
            }
            

    */
    static WalkerStep gotoPKCS12ContentType[] =
    {
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},
        { VerifyInteger, 3, 0 }, /* verify version */
        { GoNextSibling, 0, 0},  /* authSafe */
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},
        { VerifyType, OID, 0},
        { Complete, 0, 0 }
    };

    MSTATUS status;
    ASN1_ITEMPTR pContentType;
    ubyte subType;

    /* get pointer to OID verify Version on the way */
    status = ASN1_WalkTree( pRootItem, s, gotoPKCS12ContentType, &pContentType);

    if (status < OK ) return status;

    if (OK == ASN1_VerifyOIDRoot( pContentType, s, pkcs7_root_OID, &subType))
    {
        switch ( subType)
        {
            case 1: /* pkcs7_data_OID */
                status = PKCS12_PasswordIntegrityMode( MOC_SYM_HASH(hwAccelCtx)
                                                       pRootItem, 
                                                       pContentType,
                                                       s,
                                                       uniPassword, uniPassLen,
                                                       pkcs7CBs, handlerInfo);
                break;

            case 2: /* pkcs7_signedData_OID */
                /* public key integrity mode */
                status = PKCS12_PublicKeyIntegrityMode(MOC_RSA(hwAccelCtx)
                                                       pContentType,
                                                       s,
                                                       uniPassword, uniPassLen,
                                                       pkcs7CBs, handlerInfo);
                break;

            default: /* invalid value */
                status = ERR_PKCS12_NOT_EXPECTED_OID;
                break;
        }
    }
    else
    {
        status = ERR_PKCS12_NOT_EXPECTED_OID;
    }
    return status;
}



/*---------------------------------------------------------------------*/

MOC_EXTERN MSTATUS PKCS12_ExtractInfo(MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                                      ASN1_ITEM* pRootItem,
                                      CStream s,
                                      const ubyte* uniPassword,
                                      sbyte4 uniPassLen,
                                      PKCS7_Callbacks* pkcs7CBs,
                                      PKCS12_contentHandler handler)
{
    PKCS12CallbackInfo handlerInfo;
    
    handlerInfo.type = 0;
    handlerInfo.context = 0;
    handlerInfo.func.handler = handler;
    
    return PKCS12_ExtractInfoAux( MOC_RSA_SYM(hwAccelCtx)
                                 pRootItem, s, uniPassword, uniPassLen,
                                 pkcs7CBs, &handlerInfo);
    
}


/*---------------------------------------------------------------------*/

MOC_EXTERN MSTATUS PKCS12_ExtractInfoEx(MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                                        ASN1_ITEM* pRootItem,
                                        CStream s,
                                        const ubyte* uniPassword,
                                        sbyte4 uniPassLen,
                                        PKCS7_Callbacks* pkcs7CBs,
                                        PKCS12_contentHandlerEx handler,
                                        void* handlerContext)
{
    PKCS12CallbackInfo handlerInfo;
    
    handlerInfo.type = 1;
    handlerInfo.context = handlerContext;
    handlerInfo.func.handlerEx = handler;
    
    return PKCS12_ExtractInfoAux( MOC_RSA_SYM(hwAccelCtx)
                                 pRootItem, s, uniPassword, uniPassLen,
                                 pkcs7CBs, &handlerInfo);
    
}


/*
 * Code to get pkcs12 encrypted file out
*/

#define PFX_VERSION         3
#define VERSION             PFX_VERSION

static const ubyte*
PKCS12_getPbeOIDFrom(sbyte pbeSubType)
{
    PCKS12CipherSuite* pCipherSuite = PKCS12_getCipherSuite(pbeSubType);
    return ((pCipherSuite) ? pCipherSuite->pOID : NULL);
}

static MSTATUS
PKCS12_AddBag(DER_ITEMPTR pSafeBagValue, const ubyte* pBagOid, ubyte type, const ubyte* pValue, ubyte4 valueLen)
{
    MSTATUS      status = OK;
    DER_ITEMPTR  pBag = NULL;
    DER_ITEMPTR  pBagValue = NULL;

    if (OK > (status = DER_AddSequence(pSafeBagValue, &pBag)))
        goto exit;

    if (OK > (status = DER_AddOID(pBag, pBagOid, NULL)))
        goto exit;

    /*Bag Value : explicit tag [0]*/
    if (OK > (status = DER_AddTag(pBag, 0, &pBagValue)))
      goto exit;

    if (OK > (status = DER_AddItem(pBagValue, type, valueLen, pValue, NULL)))
        goto exit;

exit:
    return status;
}

static MSTATUS
PKCS12_AddPKCS12Attribute(DER_ITEMPTR pSafeBag, 
                          PKCS12AttributeUserValue** ppPKCS12AttrUserValue,
                          ubyte4 numPKCS12AttrUserValue)
{
    MSTATUS            status = OK;
    DER_ITEMPTR        pBagAttributes = NULL;
    sbyte4             iter;

    if (!ppPKCS12AttrUserValue) /* no pkcs12 attribute set */
        goto exit;

    if (!pSafeBag)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 >= numPKCS12AttrUserValue)
    {
        status = ERR_PKCS12_INVALID_ENCRYPT_ARG;
        goto exit;
    }

    if (OK > (status = DER_AddSet(pSafeBag, &pBagAttributes)))
        goto exit;

    for (iter = 0; iter < numPKCS12AttrUserValue; iter++)
    {
        const ubyte*                pOID = NULL;
        ubyte                       type = 0x00;
        DER_ITEMPTR                 pItemTmp = NULL;
        PKCS12AttributeUserValue*   pPKCSAttrUserVal = ppPKCS12AttrUserValue[iter];

        switch(pPKCSAttrUserVal->eAttrType)
        {
            case PKCS12_AttributeType_friendlyName:
            {
                if ((255 < pPKCSAttrUserVal->valueLen) || (1 > pPKCSAttrUserVal->valueLen))
                {
                    status = ERR_PKCS12_INVALID_ENCRYPT_ARG;
                    goto exit;
                }
                pOID = pkcs9_friendlyName_OID;
                type = BMPSTRING;
                break;
            }
            case PKCS12_AttributeType_localKeyId:
            {
                pOID = pkcs9_localKeyId_OID;
                type = OCTETSTRING;
                break;
            }
            default:
            {
                status = ERR_PKCS12_INVALID_ENCRYPT_ARG;
                goto exit;
            }
        }

        if (OK > (status = DER_AddSequence(pBagAttributes, &pItemTmp)))
            goto exit;

        if (OK > (status = DER_AddOID(pItemTmp, pOID, NULL)))
            goto exit;

        if (OK > (status = DER_AddSet(pItemTmp, &pItemTmp)))
            goto exit;

        if (OK > (status = DER_AddItem(pItemTmp, type, 
                                       pPKCSAttrUserVal->valueLen, pPKCSAttrUserVal->pValue,
                                       NULL)))
            goto exit;
    }

exit:
    return status;
}

static MSTATUS
PKCS12_AddSafeBag(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx) 
                  DER_ITEMPTR pSafeContents,
                  randomContext* pRandomContext,
                  PKCS12SafeBagType ePkcsSafeBagType,
                  sbyte4 encType,
                  AsymmetricKey* pKey,
                  const ubyte* pSafeBagData, ubyte4 safeBagDataLen,
                  PKCS12AttributeUserValue** ppPKCS12AttrUserValue, 
                  ubyte4 numPKCS12AttrUserValue)
{
    MSTATUS        status = OK;
    DER_ITEMPTR    pSafeBag = NULL,
                   pSafeBagVal = NULL;
    ubyte*         pRetKeyDer = NULL;
    ubyte4         retKeyDerLength = 0;
    const ubyte*   pPkcs12BagTypesOid = NULL;
    const ubyte*   pBagOid = NULL;
    ubyte          type = 0x00;
    ubyte*         pUniCodePassword = NULL;

    if (OK > (status = DER_AddSequence(pSafeContents, &pSafeBag)))
        goto exit;

    switch(ePkcsSafeBagType)
    {
        case PKCS12SafeBagType_keyBag:
        {
            pPkcs12BagTypesOid = pkcs12_bagtypes_keyBag;
            if (OK > (status = PKCS_setPKCS8Key(MOC_RSA_SYM_HASH(hwAccelCtx)
                                                pKey,
                                                pRandomContext, 
                                                encType,
                                                pSafeBagData, safeBagDataLen,
                                                &pRetKeyDer, &retKeyDerLength)))
                goto exit;

            break;
        }
        case PKCS12SafeBagType_pkcs8ShroudedKeyBag:
        {
            ubyte4 iter;

            for (iter = 0; iter < safeBagDataLen; iter++)
                if (!((31 < (pSafeBagData[iter])) &&
                      (127 > (pSafeBagData[iter]))))
                {
                    status = ERR_PKCS12_NOT_PRINTABLE_PASSWORD;
                    goto exit;
                }

            if (0 != *pSafeBagData)
            {
                sbyte4 i;

                if (NULL == (pUniCodePassword = MALLOC((2 * safeBagDataLen) + 2)))
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }
                MOC_MEMSET(pUniCodePassword, 0, ((2 * safeBagDataLen) + 2));

                for (i = 0; i < safeBagDataLen; ++i)
                {
                    pUniCodePassword[i*2 + 1] = pSafeBagData[i];
                }

                pSafeBagData = pUniCodePassword;
                safeBagDataLen = 2 * safeBagDataLen + 2;
            }

            pPkcs12BagTypesOid = pkcs12_bagtypes_pkcs8ShroudedKeyBag;
            if (OK > (status = PKCS_setPKCS8Key(MOC_RSA_SYM_HASH(hwAccelCtx)
                                                pKey,
                                                pRandomContext, 
                                                encType,
                                                pSafeBagData,safeBagDataLen,
                                                &pRetKeyDer, &retKeyDerLength)))
                goto exit;

            break;
        }
        case PKCS12SafeBagType_certBag:
        {
            pPkcs12BagTypesOid = pkcs12_bagtypes_certBag;
            if (X509 == encType)
              type = OCTETSTRING;
            else
              type = IA5STRING;
            pBagOid = pkcs9_pkcs12_certtypes_X509_OID;
            break;
        }
        case PKCS12SafeBagType_crlBag:
        {
            pPkcs12BagTypesOid = pkcs12_bagtypes_crlBag;
            pBagOid = pkcs9_pkcs12_certtypes_X509_OID;
            type = OCTETSTRING;
            break;
        }
        case PKCS12SafeBagType_secretBag:
        case PKCS12SafeBagType_safeContentsBag:
        default:
            status = ERR_PKCS12_UNKNOWN_BAGTYPE;
            goto exit;
    }

    /* Create Safe Bag : id */
    if (OK > (status = DER_AddOID(pSafeBag, pPkcs12BagTypesOid, NULL)))
        goto exit;

    /* Create Safe Bag : bagValue, explicit tag [0] */
    if (OK > (status = DER_AddTag(pSafeBag, 0, &pSafeBagVal)))
        goto exit;

    if (pRetKeyDer)
    {
        /* store keys within safe bag val */
        if (OK > (status = DER_AddDERBufferOwn(pSafeBagVal, retKeyDerLength, (const ubyte**)&pRetKeyDer, NULL)))
            goto exit;
        pRetKeyDer = NULL;
    }
    else if (NULL != pBagOid)
    {
        /*Add certificate and crl within safe bag*/
        if (OK > (status = PKCS12_AddBag(pSafeBagVal, (const ubyte*)pBagOid,
                                       type, pSafeBagData, safeBagDataLen)))
            goto exit;
    }
    else
    {
        /* TODO: find more appropriate error */
        status = ERR_PKCS12;
        goto exit;
    }

    /* Add pkcs12 attribute, if present */
    status = PKCS12_AddPKCS12Attribute(pSafeBag, ppPKCS12AttrUserValue, numPKCS12AttrUserValue);

exit:
    if (OK > status)
    {
        if (pRetKeyDer)
            FREE(pRetKeyDer);
    }

    if (pUniCodePassword)
        FREE(pUniCodePassword);

    return status;
}

static MSTATUS
PKCS12_AddSafeContent(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                      randomContext* pRandomContext,
                      PCKS12CipherSuite* pCipherSuite,
                      PKCS12DataObject *pPKCS12DataObject,
                      ubyte** ppSafeContent, ubyte4 *pSafeContentLen)
{
    MSTATUS              status = OK;
    DER_ITEMPTR          pSafeContent = NULL;
    sbyte4               padSize = 0;
    sbyte2               i = 0;
    ubyte*               pPayLoad = NULL;
    ubyte4               payLoadLen = 0;

    if (!ppSafeContent || !pSafeContentLen || !pPKCS12DataObject)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = DER_AddSequence(pSafeContent, &pSafeContent)))
        goto exit;

    if (NULL != pPKCS12DataObject->pPrivateKey)
    {
        if (NULL != pPKCS12DataObject->pKeyPassword)
        {
            if (0 >= pPKCS12DataObject->keyPasswordLen)
            {
                status = ERR_PKCS12_INVALID_ENCRYPT_ARG;
                goto exit;
            }

        }

        if (OK > (status = PKCS12_AddSafeBag(MOC_RSA_SYM_HASH(hwAccelCtx)
                                             pSafeContent,
                                             pRandomContext,
                                             (pPKCS12DataObject->pKeyPassword ? 
                                              PKCS12SafeBagType_pkcs8ShroudedKeyBag : PKCS12SafeBagType_keyBag),
                                             (pPKCS12DataObject->pKeyPassword ?
                                              pPKCS12DataObject->encKeyType : PCKS8_EncryptionType_undefined),
                                             pPKCS12DataObject->pPrivateKey,
                                             pPKCS12DataObject->pKeyPassword,
                                             pPKCS12DataObject->keyPasswordLen,
                                             pPKCS12DataObject->ppPKCS12AttrValue,
                                             pPKCS12DataObject->numPKCS12AttrValue)))
          goto exit;
    }

    if (NULL != pPKCS12DataObject->pCertificate)
    {
        if (0 >= (pPKCS12DataObject->certificateLen))
        {
            status = ERR_PKCS12_INVALID_ENCRYPT_ARG;
            goto exit;
        }

        if (OK > (status = PKCS12_AddSafeBag(MOC_RSA_SYM_HASH(hwAccelCtx)
                                             pSafeContent,
                                             pRandomContext,
                                             PKCS12SafeBagType_certBag,
                                             pPKCS12DataObject->eCertType,
                                             NULL,
                                             pPKCS12DataObject->pCertificate,
                                             pPKCS12DataObject->certificateLen,
                                             pPKCS12DataObject->ppPKCS12AttrValue,
                                             pPKCS12DataObject->numPKCS12AttrValue)))
            goto exit;
    }

    if (NULL != pPKCS12DataObject->pCrl)
    {
        if (0 >= (pPKCS12DataObject->crlLen))
        {
            status = ERR_PKCS12_INVALID_ENCRYPT_ARG;
            goto exit;
        }

        if (OK > (status = PKCS12_AddSafeBag(MOC_RSA_SYM_HASH(hwAccelCtx)
                                             pSafeContent,
                                             pRandomContext,
                                             PKCS12SafeBagType_crlBag,
                                             PCKS8_EncryptionType_undefined,
                                             NULL,
                                             pPKCS12DataObject->pCrl,
                                             pPKCS12DataObject->crlLen,
                                             pPKCS12DataObject->ppPKCS12AttrValue,
                                             pPKCS12DataObject->numPKCS12AttrValue)))
            goto exit;
    }

    if (OK > (status = DER_GetLength(pSafeContent, &payLoadLen)))
        goto exit;

    if (pCipherSuite)
    {
        /* pad size is useful in password encryption mode */
        if (0 < pCipherSuite->pBEAlgo->blockSize)
            padSize = pCipherSuite->pBEAlgo->blockSize - (payLoadLen % pCipherSuite->pBEAlgo->blockSize);
        if (0 == padSize)
            padSize = pCipherSuite->pBEAlgo->blockSize;
    }

    payLoadLen = payLoadLen + padSize;
    if (NULL == (pPayLoad = MALLOC(payLoadLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = DER_SerializeInto(pSafeContent, pPayLoad, &payLoadLen)))
        goto exit;

    /* Add padding */
    for (i = 0; i < padSize; i++)
        pPayLoad[payLoadLen + i] = (ubyte)padSize;

    *ppSafeContent = pPayLoad;
    *pSafeContentLen = payLoadLen + padSize;

exit:
    if (OK > status)
    {
        if (pPayLoad)
            FREE(pPayLoad);
    }
    if (pSafeContent)
        TREE_DeleteTreeItem((TreeItem *)pSafeContent);

    return status;
}

static MSTATUS
PKCS12_AddEncryptedSafeContentWithPubKey(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                         DER_ITEMPTR pParent,
                                         randomContext* pRandomContext,
                                         CStream** ppCSDestPubKeyStreams,
                                         ubyte4 numCStreams,
                                         const ubyte* pEncryptionAlgoOID,
                                         PKCS12DataObject *pPKCS12DataObject)
{
    MSTATUS          status = OK;
    ASN1_ITEMPTR     *ppCertificates = NULL;
    CStream*         pCStreams = NULL;
    ubyte*           pSafeContent = NULL;
    ubyte4           safeContentLen = 0;
    ubyte*           pEnvelopedDataBuf = NULL;
    ubyte4           envelopedDataLen = 0;
    DER_ITEMPTR      pContentInfo = NULL;
    DER_ITEMPTR      pEnvelopedData = NULL;
    ubyte4           iter;

    if (!pParent || !ppCSDestPubKeyStreams || !pEncryptionAlgoOID || (0 >= numCStreams))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL == (ppCertificates = MALLOC(numCStreams * sizeof(ASN1_ITEMPTR))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    MOC_MEMSET((ubyte *)ppCertificates, 0x00, (numCStreams * sizeof(ASN1_ITEMPTR)));

    if (NULL == (pCStreams = MALLOC(numCStreams * sizeof(CStream))))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }
    MOC_MEMSET((ubyte*)pCStreams, 0x00, (numCStreams * sizeof(CStream)));

    for (iter = 0; iter < numCStreams; iter++)
    {
        ASN1_ITEMPTR    pTPDestEncK = NULL;

        if (OK > (status = ASN1_Parse(*ppCSDestPubKeyStreams[iter], &pTPDestEncK)))
            goto exit;

        pCStreams[iter] = *ppCSDestPubKeyStreams[iter];
        ppCertificates[iter] = pTPDestEncK;
    }

    if (OK > (status = PKCS12_AddSafeContent(MOC_RSA_SYM_HASH(hwAccelCtx)
                                             pRandomContext,
                                             NULL,
                                             pPKCS12DataObject,
                                             &pSafeContent, &safeContentLen)))
        goto exit;

    /* create Enveloped content info */
    if (OK > (status = DER_AddSequence(NULL, &pContentInfo)))
        goto exit;

    if (OK > (status = DER_AddOID(pContentInfo, pkcs7_envelopedData_OID, NULL)))
        goto exit;

    if (OK > (status = DER_AddTag(pContentInfo, 0, &pEnvelopedData)))
        goto exit;

    if (OK > (status = PKCS7_EnvelopData(MOC_RSA_SYM(hwAccelCtx)
                                         pContentInfo,
                                         pEnvelopedData,
                                         ppCertificates,
                                         pCStreams,//csDestPubKeyStreams,
                                         numCStreams,
                                         pEncryptionAlgoOID,
                                         RANDOM_rngFun,
                                         pRandomContext,
                                         pSafeContent,
                                         safeContentLen,
                                         &pEnvelopedDataBuf,
                                         &envelopedDataLen)))
        goto exit;

    /* Add BER Enveloped data to Authenticated Safe */
    status = DER_AddDERBufferOwn(pParent, envelopedDataLen, (const ubyte**)&pEnvelopedDataBuf, NULL);
exit:
    if (OK > status)
    {
        if (pEnvelopedDataBuf)
            FREE(pEnvelopedDataBuf);
    }

    if (pSafeContent)
        FREE(pSafeContent);

    if (pContentInfo)
        TREE_DeleteTreeItem((TreeItem *)pContentInfo);

    if (ppCertificates)
    {
        for (iter = 0; iter < numCStreams; iter++)
        {
            if (ppCertificates[iter])
                TREE_DeleteTreeItem((TreeItem *)ppCertificates[iter]);
        }
        FREE(ppCertificates);
    }

    if (pCStreams)
        FREE(pCStreams);

    return status;
}

static MSTATUS
PKCS12_AddUnEncryptedSafeContent(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                 DER_ITEMPTR pParent,
                                 randomContext* pRandomContext,
                                 PKCS12DataObject *pPKCS12DataObject)
{
    MSTATUS       status = OK;
    ubyte*        pSafeContent = NULL;
    ubyte4        safeContentLen = 0;

    if (!pParent)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = PKCS12_AddSafeContent(MOC_RSA_SYM_HASH(hwAccelCtx)
                                             pRandomContext,
                                             NULL,
                                             pPKCS12DataObject,
                                             &pSafeContent, &safeContentLen)))
        goto exit;

    if (OK > (status = PKCS12_AddContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                             pParent,
                                             pRandomContext,
                                             0,
                                             pkcs7_data_OID,
                                             NULL, 0,
                                             TRUE,
                                             pSafeContent, safeContentLen)))
        goto exit;

exit:

    if (OK > status)
    {
        if (pSafeContent)
            FREE(pSafeContent);
    }

    return status;
}

static MSTATUS
PKCS12_AddEncryptedSafeContentWithPassword(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                           DER_ITEMPTR pParent,
                                           randomContext* pRandomContext,
                                           ubyte4 encPkcs12SubType,
                                           const ubyte* uniPassword, ubyte4 uniPasswordLen,
                                           PKCS12DataObject *pPKCS12DataObject)
{
    MSTATUS               status = OK;
    ubyte*                pSafeContent = NULL;
    ubyte4                safeContentLen = 0;
    PCKS12CipherSuite*    pCipherSuite = PKCS12_getCipherSuite(encPkcs12SubType);


    if ((!pParent) || (!pCipherSuite))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = PKCS12_AddSafeContent(MOC_RSA_SYM_HASH(hwAccelCtx)
                                             pRandomContext,
                                             pCipherSuite,
                                             pPKCS12DataObject,
                                             &pSafeContent, &safeContentLen)))
        goto exit;

    if (OK > (status = PKCS12_AddEncryptedContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                                      pParent,
                                                      pRandomContext,
                                                      encPkcs12SubType,
                                                      uniPassword, uniPasswordLen,
                                                      pSafeContent, safeContentLen)))
        goto exit;

exit:
    if (OK > status)
    {
        if (pSafeContent)
            FREE(pSafeContent);
    }

    return status;
}

static MSTATUS
PKCS12_AddMacData(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                 DER_ITEMPTR pParent,
                 randomContext *pRandomContext, 
                 enum PKCS8EncryptionType encType,
                 const ubyte* password, ubyte4 passwordLen,
                 ubyte* pContentInfo, ubyte4 contentInfoLen)
{
    MSTATUS                      status = OK;
    DER_ITEMPTR                  pMacData, pDigestInfo, pDigestAlgoSeq;
    ubyte*                       salt = NULL;
    ubyte*                       pHashOutput = NULL;
    ubyte4                       hashOutputLen = 0;

    if (!pParent || !pRandomContext || !password)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = DER_AddSequence(pParent, &pMacData)))
        goto exit;

    if (OK > (status = DER_AddSequence(pMacData, &pDigestInfo)))
        goto exit;

    if (OK > (status = DER_AddSequence(pDigestInfo, &pDigestAlgoSeq)))
        goto exit;

    /* Digest Algorithm: should this be SHA1 id ?? */
    if (OK > (status = DER_AddOID(pDigestAlgoSeq, sha1_OID, NULL)))
        goto exit;

    /* generate salt */
    if (NULL == (salt = MALLOC(8)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = RANDOM_numberGenerator(pRandomContext, salt, 8)))
        goto exit;

    /* Calculate hash */
    if (OK > (status = PKCS12_encryptEx(MOC_SYM_HASH(hwAccelCtx)
                                        encType, password, passwordLen,
                                        salt, 8, 2048, 
                                        pContentInfo, contentInfoLen,
                                        FALSE,
                                        &pHashOutput, &hashOutputLen)))
        goto exit;

    if (OK > (status = DER_AddItemOwnData(pMacData, OCTETSTRING, 8, &salt, NULL)))
        goto exit;

    /* iteration = 2048 */
    if (OK > (status = DER_AddIntegerEx(pMacData, 2048, NULL)))
        goto exit;

    if (OK > (status = DER_AddItemOwnData(pDigestInfo, OCTETSTRING, hashOutputLen, &pHashOutput, NULL)))
        goto exit;

exit:

    if (OK > status)
    {
        if (pHashOutput)
            FREE(pHashOutput);
        if (salt)
            FREE(salt);
    }
 
    return status;
}

static MSTATUS
PKCS12_AddDigitalSignature(MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                           DER_ITEMPTR pParent,
                           randomContext* pRandomContext,
                           AsymmetricKey* pPrivKey,
                           CStream csCert[],
                           ubyte4 numSignerCerts,
                           const ubyte* pDigestAlgoOID,
                           ubyte* pAuthenticatedSafe, ubyte4 authenticatedSafeLen)
{
    MSTATUS         status = OK;
    DER_ITEMPTR     pContentInfo = NULL;
    DER_ITEMPTR     pSignedData = NULL;
    signerInfoPtr*  ppSignerInfos = NULL;
    ASN1_ITEMPTR    *ppCertificates = NULL;
    ubyte*          pSignedDataBuf = NULL;
    ubyte4          signedDataLen = 0;
    ubyte4          iter;

    if (!pParent || !pDigestAlgoOID || (0 >= numSignerCerts))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL == (ppCertificates = MALLOC(numSignerCerts * sizeof(ASN1_ITEMPTR))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    MOC_MEMSET((ubyte *)ppCertificates, 0x00, (numSignerCerts * sizeof(ASN1_ITEMPTR)));

    if (NULL == (ppSignerInfos = MALLOC(numSignerCerts * sizeof(signerInfoPtr))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    MOC_MEMSET((ubyte *)ppSignerInfos, 0x00, (numSignerCerts * sizeof(signerInfoPtr)));

    for (iter = 0; iter < numSignerCerts; iter++)
    {
        ASN1_ITEMPTR    pRootItem = NULL,
                        pIssuer = NULL,
                        pSerialNumber = NULL;
        signerInfoPtr   pSingleSignerInfo = NULL;

        if (OK > (status = ASN1_Parse(csCert[iter], &pRootItem)))
            goto exit;

        ppCertificates[iter] = pRootItem;

        if (NULL == (pSingleSignerInfo = MALLOC(sizeof(signerInfo))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMSET((ubyte *)pSingleSignerInfo, 0x00, sizeof(signerInfo));
        ppSignerInfos[iter] = pSingleSignerInfo;

        if (OK > (status = CERT_getCertificateIssuerSerialNumber(pRootItem, &pIssuer, &pSerialNumber)))
            goto exit;

        /* fill signer info */
        pSingleSignerInfo->pIssuer = pIssuer;
        pSingleSignerInfo->pSerialNumber = pSerialNumber;
        pSingleSignerInfo->cs = csCert[iter];
        pSingleSignerInfo->digestAlgoOID = pDigestAlgoOID;
        pSingleSignerInfo->pKey = pPrivKey;
        pSingleSignerInfo->pUnauthAttrs = NULL;
        pSingleSignerInfo->unauthAttrsLen = 0;
    }

    /* Create signedData content info */
    if (OK > (status = DER_AddSequence(NULL, &pContentInfo)))
        goto exit;

    if (OK > (status = DER_AddOID(pContentInfo, pkcs7_signedData_OID, NULL)))
        goto exit;

    if (OK > (status = DER_AddTag(pContentInfo, 0, &pSignedData)))
        goto exit;

    if (OK > (status = PKCS7_SignData(MOC_RSA_HASH(hwAccelCtx)
                                      pContentInfo, pSignedData,
                                      ppCertificates, 
                                      csCert, numSignerCerts,
                                      NULL, NULL, 0, /* no crls */
                                      ppSignerInfos, numSignerCerts,
                                      pkcs7_data_OID,
                                      pAuthenticatedSafe, authenticatedSafeLen,
                                      RANDOM_rngFun, pRandomContext,
                                      &pSignedDataBuf, &signedDataLen
                                      )))
        goto exit;

    /* Add signed data to PFX as authSafe */
    status = DER_AddDERBufferOwn(pParent, signedDataLen, (const ubyte**)&pSignedDataBuf, NULL);

exit:

    if (OK > status)
    {
        if (pSignedDataBuf)
            FREE(pSignedDataBuf);
    }

    if (pContentInfo)
        TREE_DeleteTreeItem((TreeItem *)pContentInfo);

    if (ppCertificates)
    {
        for (iter = 0; iter < numSignerCerts; iter++)
        {
            if (ppCertificates[iter])
                TREE_DeleteTreeItem((TreeItem *)ppCertificates[iter]);
        }
        FREE(ppCertificates);
    }

    if (ppSignerInfos)
    {
        for(iter = 0; iter < numSignerCerts; iter++)
        {
            if (ppSignerInfos[iter])
                FREE(ppSignerInfos[iter]);
        }
        FREE(ppSignerInfos);
    }

    return status;
}

static MSTATUS
PKCS12_AddContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx) 
                      DER_ITEMPTR pParent,
                      randomContext* pRandomContext,
                      enum PKCS8EncryptionType encType,
                      const ubyte* pContentType,
                      const ubyte* password, ubyte4 passwordLen,
                      byteBoolean  contentInfo,
                      ubyte* pContentPayload, ubyte4 payLoadLen)
{
    MSTATUS       status = OK;
    DER_ITEMPTR   pContentInfo = NULL;
    DER_ITEMPTR   pEncContentInfo = NULL;
    intBoolean    cmpResult = 0;
    ubyte*        salt = NULL;

    if (!pParent || !pContentType || !pContentPayload)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    cmpResult = EqualOID(pkcs7_data_OID, pContentType);

    if (OK > (status = DER_AddSequence(pParent, &pContentInfo)))
        goto exit;

    if (!contentInfo && cmpResult)
    {
        ubyte          copyData[4];
        DER_ITEMPTR    pAlgoSequence = NULL;
        const ubyte*   pPbeOid = PKCS12_getPbeOIDFrom(encType);

        if (!pPbeOid)
        {
            status = ERR_PKCS12_UNSUPPORTED_ALGO;
            goto exit;
        }

        copyData[0] = 0;
        if (OK > ( status = DER_AddItemCopyData(pContentInfo, INTEGER, 1, copyData, NULL)))
            goto exit;

        if (OK > (status = DER_AddSequence(pContentInfo, &pEncContentInfo)))
            goto exit;

        if (OK > (status = DER_AddOID(pEncContentInfo, pContentType, NULL)))
            goto exit;

        if (OK > (status = DER_AddSequence(pEncContentInfo, &pAlgoSequence)))
            goto exit;

        if (OK > (status = DER_AddOID(pAlgoSequence, pPbeOid, NULL)))
            goto exit;

        if (OK > (status = DER_AddSequence(pAlgoSequence, &pAlgoSequence)))
            goto exit;

        /* salt */
        if (NULL == (salt = MALLOC(8)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        if (OK > (status = RANDOM_numberGenerator(pRandomContext, salt, 8)))
            goto exit;

        /* Encrypt */
        if (OK > (status = PKCS12_encrypt(MOC_SYM_HASH(hwAccelCtx)
                                          encType, password, passwordLen,
                                          salt, 8, 2048,
                                          pContentPayload, payLoadLen)))
            goto exit;

        if (OK > (status = DER_AddItemOwnData(pAlgoSequence, OCTETSTRING, 8, &salt, NULL)))
            goto exit;

        /* iterations */
        if (OK > (status = DER_AddIntegerEx(pAlgoSequence, 2048, NULL)))
            goto exit;

        //if (OK > (status = DER_AddTag(pEncContentInfo, 0, &pEncContentInfo)))
        //    goto exit;

        if (OK > (status = DER_AddItemOwnData(pEncContentInfo, CONTEXT | 0, payLoadLen, &pContentPayload, NULL)))
            goto exit; 
    }
    else
    {
        if (OK > (status = DER_AddOID(pContentInfo, pContentType, NULL)))
            goto exit;

        /* Content */
        if (OK > (status = DER_AddTag(pContentInfo, 0, &pEncContentInfo)))
            goto exit;

        if (contentInfo && cmpResult)
        {
            if (OK > (status = DER_AddItemOwnData(pEncContentInfo, PRIMITIVE|OCTETSTRING, payLoadLen, &pContentPayload, NULL)))
                goto exit;
        }
        else if (!cmpResult)
        {
            if (OK > (status = PKCS12_AddContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                                  pEncContentInfo,
                                                  pRandomContext,
                                                  encType,
                                                  pkcs7_data_OID,
                                                  password, passwordLen,
                                                  FALSE,
                                                  pContentPayload, payLoadLen)))
                goto exit;
        }
        else
        {
            status = ERR_INTERNAL_ERROR;
            goto exit;
        }
    } 
exit:
    if (OK > status)
    {
        if (salt)
           FREE(salt);
    }
    return status;
}

static MSTATUS
PKCS12_AddEncryptedContentInfo(MOC_SYM_HASH(hwAccelDescr hwAccelCtx)
                               DER_ITEMPTR pParent,
                               randomContext* pRandomContext,
                               enum PKCS8EncryptionType encType,
                               const ubyte* password, ubyte passwordLen,
                               ubyte* pContentPayload, ubyte4 payLoadLen)
{
    return PKCS12_AddContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                 pParent,
                                 pRandomContext,
                                 encType,
                                 pkcs7_encryptedData_OID,
                                 password, passwordLen,
                                 FALSE,
                                 pContentPayload,
                                 payLoadLen);
}

MOC_EXTERN MSTATUS
PKCS12_EncryptPFXPdu(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                     randomContext* pRandomContext,
                     ubyte4 integrityMode,
                     /* Password Integrity Mode */
                     const ubyte* pIntegrityPswd,
                     ubyte4 integrityPswdLen,
                     /* Pub Key Integrity Mode */
                     AsymmetricKey* pVsrcSigK,
                     const ubyte* pDigestAlgoOID,
                     CStream csSignerCertificate[],
                     ubyte4  numSignerCerts,
                     /* PKCS Privacy Mode Configuration */
                     const PKCS12PrivacyModeConfig *pPkcs12PrivacyModeConfig,
                     /* Data to be encrypted */
                     PKCS12DataObject pkcs12DataObject[/*numPKCS12DataObj*/],
                     ubyte4 numPKCS12DataObj,
                     /* return PKCS#12 certificate */
                     ubyte** ppRetPkcs12CertDer, ubyte4* pRetPkcs12CertDerLen)
{
    MSTATUS            status = OK;
    DER_ITEMPTR        pPfxSequence = NULL;
    ubyte*             pPKCS12CertDER = NULL;
    ubyte4             pkcs12CertDERLen = 0;
    ubyte*             pAuthSafeContent = NULL;
    ubyte4             authSafeContentLen = 0;
    ubyte              copyData[MAX_DER_STORAGE];
    DER_ITEMPTR        pAuthenticatedSafe = NULL;
    sbyte4             encPkcs12SubType = 0;
    const ubyte*       pPassword = NULL;
    ubyte4             passwordLen = 0;
    byteBoolean        passwordIntegrityMode = FALSE;
    ubyte4             iter = 0;

    if (!pRandomContext || !ppRetPkcs12CertDer || !pRetPkcs12CertDerLen ||
        !pPkcs12PrivacyModeConfig || (0 >= numPKCS12DataObj))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* Test for parameters depending on integrity mode */
    if (((PKCS12Mode_Integrity_password > integrityMode) || (PKCS12Mode_Integrity_pubKey < integrityMode))
        ||
        ((PKCS12Mode_Integrity_password == integrityMode) && (!(pIntegrityPswd) || (0 >= integrityPswdLen)))
        ||
        ((PKCS12Mode_Integrity_pubKey == integrityMode) && (!pVsrcSigK || !pDigestAlgoOID || (0 >= numSignerCerts))))
    {
        status = ERR_PKCS12_INVALID_INTEGRITY_MODE;
        goto exit;
    }

    /* Set default values for password */
    if (PKCS12Mode_Integrity_password == integrityMode)
    {
        ubyte4       iter = 0;
        ubyte*       pwdIterations[3] = {NULL, NULL, NULL};
        ubyte4       pwdSizeIterations[3] = {0, 0, 0};
        sbyte4       i = 0;

        passwordIntegrityMode = TRUE;
        pwdIterations[0] = (ubyte *)pIntegrityPswd;
        pwdSizeIterations[0] = integrityPswdLen;
        if (!pPkcs12PrivacyModeConfig->pPrivacyPassword)
        {
            pPassword = pIntegrityPswd;
            passwordLen = integrityPswdLen;
        }
        else
        {
            pPassword = pPkcs12PrivacyModeConfig->pPrivacyPassword;
            passwordLen = pPkcs12PrivacyModeConfig->privacyPasswordLen;
            pwdIterations[1] = (ubyte *)pPassword;
            pwdSizeIterations[1] = passwordLen;
        }

        /* check for range */
        while(pwdIterations[i])
        {
            for (iter = 0; iter < pwdSizeIterations[i]; iter++)
              if (!((31 < ((pwdIterations[i])[iter])) &&
                   (127 > ((pwdIterations[i])[iter]))))
                {
                    status = ERR_PKCS12_NOT_PRINTABLE_PASSWORD;
                    goto exit;
                }

            i++;
        }
    }
    else
    {
        if (pPkcs12PrivacyModeConfig->pPrivacyPassword && 
            (0 >= pPkcs12PrivacyModeConfig->privacyPasswordLen))
        {
            ubyte4 iter = 0;
            for (; iter < pPkcs12PrivacyModeConfig->privacyPasswordLen; iter ++)
                if (!((31 < (pPkcs12PrivacyModeConfig->pPrivacyPassword[iter])) &&
                      (127 > (pPkcs12PrivacyModeConfig->pPrivacyPassword[iter]))))
                {
                    status = ERR_PKCS12_NOT_PRINTABLE_PASSWORD;
                    goto exit;
                }
        }
    }

    encPkcs12SubType = pPkcs12PrivacyModeConfig->pkcs12EncryptionType;
    if (PKCS8_EncryptionType_pkcs12 >= encPkcs12SubType)
    {
        /* default to rc2_40, if enabled */
#ifdef __ENABLE_ARC2_CIPHERS__
        encPkcs12SubType = PCKS8_EncryptionType_pkcs12_sha_rc2_40;
#else
        status = ERR_PKCS12_INVALID_ENCRYPT_ARG;
        goto exit;
#endif
    }
    encPkcs12SubType = encPkcs12SubType - PKCS8_EncryptionType_pkcs12;

    /* Create a PFX sequence */
    if (OK > (status = DER_AddSequence(NULL, &pPfxSequence)))
        goto exit;

    /* Add version indicator */
    copyData[0] = VERSION;
    if (OK > (status = DER_AddItem(pPfxSequence, INTEGER, 1, copyData, NULL)))
        goto exit;

    if (OK > (status = DER_AddSequence(NULL, &pAuthenticatedSafe)))
        goto exit;

    for (iter = 0; iter < numPKCS12DataObj; iter++)
    {
        PKCS12DataObject* pPKCS12DataObject = &pkcs12DataObject[iter];

        if (!pPKCS12DataObject)
        {
            status = ERR_NULL_POINTER;
            goto exit;
        }

        if ((PKCS12Mode_Privacy_password == pPKCS12DataObject->privacyMode) ||
            (PKCS12Mode_Privacy_none == pPKCS12DataObject->privacyMode))
        {
            if (OK > (status = PKCS12_AddEncryptedSafeContentWithPassword(MOC_RSA_SYM_HASH(hwAccelCtx)
                                                                          pAuthenticatedSafe,
                                                                          pRandomContext,
                                                                          encPkcs12SubType,
                                                                          ((pPkcs12PrivacyModeConfig->pPrivacyPassword) ? 
                                                                           pPkcs12PrivacyModeConfig->pPrivacyPassword : pPassword),
                                                                          ((pPkcs12PrivacyModeConfig->pPrivacyPassword) ? 
                                                                           pPkcs12PrivacyModeConfig->privacyPasswordLen : passwordLen),
                                                                          pPKCS12DataObject)))
                goto exit;
        }
        else if (PKCS12Mode_Privacy_pubKey == pPKCS12DataObject->privacyMode)
        {
            if (OK > (status = PKCS12_AddEncryptedSafeContentWithPubKey(MOC_RSA_SYM_HASH(hwAccelCtx)
                                                                        pAuthenticatedSafe,
                                                                        pRandomContext,
                                                                        pPkcs12PrivacyModeConfig->ppCSDestPubKeyStream,
                                                                        pPkcs12PrivacyModeConfig->numPubKeyStream,
                                                                        pPkcs12PrivacyModeConfig->pEncryptionAlgoOID,
                                                                        pPKCS12DataObject)))
                goto exit;
        }
        else if (PKCS12Mode_Privacy_data == pPKCS12DataObject->privacyMode)
        {
            /* MODE : DATA*/
            if (OK > (status = PKCS12_AddUnEncryptedSafeContent(MOC_RSA_SYM_HASH(hwAccelCtx)
                                                                pAuthenticatedSafe,
                                                                pRandomContext,
                                                                pPKCS12DataObject)))
                goto exit;
        }
        else
        {
            status = ERR_PKCS12_INVALID_PRIVACY_MODE;
            goto exit;
        }
    }

    if (OK > (status = DER_Serialize(pAuthenticatedSafe, &pAuthSafeContent, &authSafeContentLen)))
        goto exit;

    if (passwordIntegrityMode)
    {
        ubyte*   pTemp = NULL;

        /* Add  auth safe */
        if (OK > (status = PKCS12_AddContentInfo(MOC_SYM_HASH(hwAccelCtx)
                                                 pPfxSequence,
                                                 pRandomContext,
                                                 0,
                                                 pkcs7_data_OID,
                                                 NULL, 0,
                                                 TRUE,
                                                 pAuthSafeContent, authSafeContentLen)))
            goto exit;

        pTemp = pAuthSafeContent;
        pAuthSafeContent = NULL;

        /* HMAC computed on contents of data in T (excluding OCTET STRING tag and length bytes )*/
        if (OK > (status = PKCS12_AddMacData(MOC_SYM_HASH(hwAccelCtx)
                                             pPfxSequence,
                                             pRandomContext,
                                             encPkcs12SubType,
                                             pIntegrityPswd, integrityPswdLen,
                                             pTemp, authSafeContentLen)))
            goto exit;
    }
    else
    {
        /* Integrity mode is public key*/
        /* Add auth Safe within SignedData Content Info */
        if (OK > (status = PKCS12_AddDigitalSignature(MOC_RSA_SYM(hwAccelDescr hwAccelCtx)
                                                      pPfxSequence,
                                                      pRandomContext,
                                                      pVsrcSigK,
                                                      csSignerCertificate,
                                                      numSignerCerts,
                                                      pDigestAlgoOID,
                                                      pAuthSafeContent, authSafeContentLen)))
            goto exit;
    }

    if (OK > (status = DER_Serialize(pPfxSequence, &pPKCS12CertDER, &pkcs12CertDERLen)))
        goto exit;

    *ppRetPkcs12CertDer = pPKCS12CertDER;
    *pRetPkcs12CertDerLen = pkcs12CertDERLen;

exit:
    if (pAuthSafeContent)
        FREE(pAuthSafeContent);
    if (pAuthenticatedSafe)
        TREE_DeleteTreeItem((TreeItem *)pAuthenticatedSafe);
    if (pPfxSequence)
        TREE_DeleteTreeItem((TreeItem *)pPfxSequence);

    return status;
}
/*------------------------------------------------------------------*/

/**************************************************************
TEST CODE
***************************************************************/
#ifdef __ENABLE_ALL_TESTS__

typedef struct KeyGenTestSpec
{
    ubyte   ID;             /* 1, 2, or 3 */
    sbyte4  iter;           /* num iterations */
    sbyte4  uniPassLen;     /* length password */
    ubyte   *uniPass;       /* unicode password */
    sbyte4  saltLen;        /* salt len */
    ubyte   *salt;          /* salt */
    sbyte4  keyLen;         /* desired key len*/
    ubyte   *expectedKey;   /* expected key */
} KeyGenTestSpec;

KeyGenTestSpec gTestSpecs[] =
{
    { 1, 1, 10, "\x00\x73\x00\x6D\x00\x65\x00\x67\x00\x00",
        8, "\x0A\x58\xCF\x64\x53\x0D\x82\x3F",
        24, "\x8A\xAA\xE6\x29\x7B\x6C\xB0\x46\x42\xAB\x5B\x07\x78\x51\x28\x4E\xB7\x12\x8F\x1A\x2A\x7F\xBC\xA3"
    },
    { 2, 1, 10, "\x00\x73\x00\x6D\x00\x65\x00\x67\x00\x00",
        8, "\x0A\x58\xCF\x64\x53\x0D\x82\x3F",
        8, "\x79\x99\x3D\xFE\x04\x8D\x3B\x76",
    },
    { 1, 1, 10, "\x00\x73\x00\x6D\x00\x65\x00\x67\x00\x00",
        8, "\x64\x2B\x99\xAB\x44\xFB\x4B\x1F",
        24, "\xF3\xA9\x5F\xEC\x48\xD7\x71\x1E\x98\x5C\xFE\x67\x90\x8C\x5A\xB7\x9F\xA3\xD7\xC5\xCA\xA5\xD9\x66"
    },
    { 2, 1, 10, "\x00\x73\x00\x6D\x00\x65\x00\x67\x00\x00",
        8, "\x64\x2B\x99\xAB\x44\xFB\x4B\x1F",
        8, "\xC0\xA3\x8D\x64\xA7\x9B\xEA\x1D"
    },
    { 3, 1, 10, "\x00\x73\x00\x6D\x00\x65\x00\x67\x00\x00",
        8, "\x3D\x83\xC0\xE4\x54\x6A\xC1\x40",
        20, "\x8D\x96\x7D\x88\xF6\xCA\xA9\xD7\x14\x80\x0A\xB3\xD4\x80\x51\xD6\x3F\x73\xA3\x12"
    },
    /* Test Vectors set 2. */
    { 1, 1000, 12, "\x00\x71\x00\x75\x00\x65\x00\x65\x00\x67\x00\x00",
        8, "\x05\xDE\xC9\x59\xAC\xFF\x72\xF7",
        24, "\xED\x20\x34\xE3\x63\x28\x83\x0F\xF0\x9D\xF1\xE1\xA0\x7D\xD3\x57\x18\x5D\xAC\x0D\x4F\x9E\xB3\xD4"
    },
    { 2, 1000, 12, "\x00\x71\x00\x75\x00\x65\x00\x65\x00\x67\x00\x00",
        8, "\x05\xDE\xC9\x59\xAC\xFF\x72\xF7",
        8, "\x11\xDE\xDA\xD7\x75\x8D\x48\x60"
    },
    { 1, 1000, 12, "\x00\x71\x00\x75\x00\x65\x00\x65\x00\x67\x00\x00",
        8, "\x16\x82\xC0\xFC\x5B\x3F\x7E\xC5",
        24, "\x48\x3D\xD6\xE9\x19\xD7\xDE\x2E\x8E\x64\x8B\xA8\xF8\x62\xF3\xFB\xFB\xDC\x2B\xCB\x2C\x02\x95\x7F"
    },
    { 2, 1000, 12, "\x00\x71\x00\x75\x00\x65\x00\x65\x00\x67\x00\x00",
        8, "\x16\x82\xC0\xFC\x5B\x3F\x7E\xC5",
        8, "\x9D\x46\x1D\x1B\x00\x35\x5C\x50"
    },
    { 3, 1000, 12, "\x00\x71\x00\x75\x00\x65\x00\x65\x00\x67\x00\x00",
        8, "\x26\x32\x16\xFC\xC2\xFA\xB3\x1C",
        20, "\x5E\xC4\xC7\xA8\x0D\xF6\x52\x29\x4C\x39\x25\xB6\x48\x9A\x7A\xB8\x57\xC8\x34\x76"
    }
};


#if 0
ID 1, ITER 1
Password (length 10):
0073006D006500670000
Salt (length 8):
0A58CF64530D823F
ID 1, ITER 1
Output KEY (length 24)
8AAAE6297B6CB04642AB5B077851284EB7128F1A2A7FBCA3

KEYGEN DEBUG
ID 2, ITER 1
Password (length 10):
0073006D006500670000
Salt (length 8):
0A58CF64530D823F
ID 2, ITER 1
Output KEY (length 8)
79993DFE048D3B76

KEYGEN DEBUG
ID 1, ITER 1
Password (length 10):
0073006D006500670000
Salt (length 8):
642B99AB44FB4B1F
ID 1, ITER 1
Output KEY (length 24)
F3A95FEC48D7711E985CFE67908C5AB79FA3D7C5CAA5D966

KEYGEN DEBUG
ID 2, ITER 1
Password (length 10):
0073006D006500670000
Salt (length 8):
642B99AB44FB4B1F
ID 2, ITER 1
Output KEY (length 8)
C0A38D64A79BEA1D

KEYGEN DEBUG
ID 3, ITER 1
Password (length 10):
0073006D006500670000
Salt (length 8):
3D83C0E4546AC140
ID 3, ITER 1
Output KEY (length 20)
8D967D88F6CAA9D714800AB3D48051D63F73A312

Test Vectors set 2.
Password: queeg

KEYGEN DEBUG
ID 1, ITER 1000
Password (length 12):
007100750065006500670000
Salt (length 8):
05DEC959ACFF72F7
ID 1, ITER 1000
Output KEY (length 24)
ED2034E36328830FF09DF1E1A07DD357185DAC0D4F9EB3D4

KEYGEN DEBUG
ID 2, ITER 1000
Password (length 12):
007100750065006500670000
Salt (length 8):
05DEC959ACFF72F7
ID 2, ITER 1000
Output KEY (length 8)
11DEDAD7758D4860

KEYGEN DEBUG
ID 1, ITER 1000
Password (length 12):
007100750065006500670000
Salt (length 8):
1682C0FC5B3F7EC5
ID 1, ITER 1000
Output KEY (length 24)
483DD6E919D7DE2E8E648BA8F862F3FBFBDC2BCB2C02957F

KEYGEN DEBUG
ID 2, ITER 1000
Password (length 12):
007100750065006500670000
Salt (length 8):
1682C0FC5B3F7EC5
ID 2, ITER 1000
Output KEY (length 8)
9D461D1B00355C50

KEYGEN DEBUG
ID 3, ITER 1000
Password (length 12):
007100750065006500670000
Salt (length 8):
263216FCC2FAB31C
ID 3, ITER 1000
Output KEY (length 20)
5EC4C7A80DF652294C3925B6489A7AB857C83476

#endif

typedef struct PKVCS12Test
{
    char* fileName;
    char* uniPass;
    sbyte4 uniPassLen;
} PKCS12Test;

/* TEST CODE BELOW */
int RunKeyGenTestSpec(KeyGenTestSpec* pKeyGenTestSpec)
{
    sbyte4 cmpRes = 1;
    MSTATUS status;
    ubyte* result = (ubyte*) MALLOC( pKeyGenTestSpec->keyLen);
    if ( 0 == result)
    {
        return 1; /* error */
    }

    status = PKCS12_SHA1_GenerateRandom( pKeyGenTestSpec->ID,
                        pKeyGenTestSpec->iter,
                        pKeyGenTestSpec->salt,
                        pKeyGenTestSpec->saltLen,
                        pKeyGenTestSpec->uniPass,
                        pKeyGenTestSpec->uniPassLen,
                        result,
                        pKeyGenTestSpec->keyLen);
    if (OK > status) goto exit;

    /* compare results */
    MOC_MEMCMP( result, pKeyGenTestSpec->expectedKey, pKeyGenTestSpec->keyLen, &cmpRes);

    if ( cmpRes != 0) cmpRes = 1; /* so that return is 0 or 1 (error) */
exit:

    FREE( result);
    return cmpRes;
}

#include <stdio.h>


MSTATUS testContentHandler(contentTypes type, ubyte4 extraInfo, const ubyte* content, ubyte4 contentLen){
    MSTATUS status = OK;
    ASN1_ITEMPTR pPrivateKeyInfoRoot = NULL;
    
    ubyte* keyBlob = NULL;
    ubyte4 keyBlobLen;
    
    switch (type)
    {
    case KEYINFO:
        if (OK > (status = PKCS8_decodePrivateKeyDER((ubyte*)content, contentLen, &keyBlob, &keyBlobLen)))            goto exit;
        if (OK > (status = MOCANA_writeFile("pkcs12keyBlob.dat", keyBlob, keyBlobLen)))                goto exit;
        break;
    case CERT:
        if (OK > (status = MOCANA_writeFile("pkcs12cert.der", (ubyte*)content, contentLen)))            goto exit;
        break;
    default:
        break;
    }   
    
exit:
    if (pPrivateKeyInfoRoot)
    {
        TREE_DeleteTreeItem((TreeItem*)pPrivateKeyInfoRoot);
    }   
    
    if (keyBlob)
    {
        FREE(keyBlob);
    }
    return status;
}


int TestSampleFile(sbyte* fileName, sbyte* uniPass, sbyte4 passLen)
{
    MSTATUS status = OK;
    int retVal = 0;
    FILE* f;
    ASN1_ITEMPTR pRootItem;
    CStream cs;

#ifdef DEBUG_PKCS12
    strcpy(gDebFileName, fileName);
    gDebFileName[MOC_STRLEN(gDebFileName)-4] = 0x00;
#endif

    f = fopen(fileName, "rb");
    if ( !f)
    {
        printf("TestSampleFile() file not found. file=%s", 
                fileName);
        return 1;
    }

    cs.pFuncs = &gStdCFileAbsStreamFuncs;
    cs.pStream = f;

    status = ASN1_Parse( cs, &pRootItem );

    if ( OK == status)
    {
        /* pkcs 12 */
        status = PKCS12_ExtractInfo(MOC_RSA_SYM(hwAccelCtx)
                                        pRootItem,
                                        cs,
                                        uniPass, passLen,
                                        NULL,&testContentHandler);

    }

    if (pRootItem)
    {
        TREE_DeleteTreeItem( (TreeItem*) pRootItem);
    }

    fclose(f);
    return (status < OK) ? 1 : 0;
}


int PKCS12_Test()
{
    int i, retVal = 0;
    PKCS12Test pkcs12Test[] =
    {
        {
            "hornet.p12",
            "\x00\x73\x00\x65\x00\x63"
            "\x00\x72\x00\x65\x00\x74"
            "\x00\x00", /* secret */
            14
        },

#if 0
        {
            "ThawteMocana.pfx", /* file name */
            "\x00\x4d\x00\x6f\x00\x63"  /* password = Mocana7est */
            "\x00\x61\x00\x6e\x00\x61"
            "\x00\x37\x00\x65\x00\x73"
            "\x00\x74\x00\x00",
            22
        },
        {
            "lakeport.pfx",
            "\x00\x31\x00\x32\x00\x33" /* password = 1234 */
            "\x00\x34\x00\x00",
            10
        }
#endif
    };

    printf("PKCS12_Test()\n");
    for (i = 0; i < COUNTOF(gTestSpecs); ++i)
    {
        retVal += RunKeyGenTestSpec( gTestSpecs+i);

        if (retVal) 
            printf("RunKeyGenTestSpec error: %d\n", retVal);
    }


    for (i =0; i < COUNTOF(pkcs12Test); ++i)
    {
        retVal += TestSampleFile(pkcs12Test[i].fileName, pkcs12Test[i].uniPass, pkcs12Test[i].uniPassLen);
    }

    return retVal;
}

#endif

#endif /* __ENABLE_MOCANA_PKCS12__ */

