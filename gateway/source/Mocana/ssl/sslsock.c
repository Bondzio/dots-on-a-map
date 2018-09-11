/* Version: mss_v6_3 */
/*
 * sslsock.c
 *
 * SSL implementation
 *
 * Copyright Mocana Corp 2003-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#if (defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_CLIENT__))

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/mtcp.h"
#include "../common/random.h"
#include "../common/vlong.h"
#include "../common/prime.h"
#include "../common/debug_console.h"
#include "../common/sizedbuffer.h"
#include "../common/mem_pool.h"
#include "../common/memory_debug.h"
#include "../crypto/crypto.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/rsa.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../crypto/des.h"
#include "../crypto/three_des.h"
#include "../crypto/aes.h"
#include "../crypto/nil.h"
#if defined(__ENABLE_MOCANA_GCM__)
#include "../crypto/aes_ctr.h"
#include "../crypto/gcm.h"
#endif
#include "../crypto/aes_ccm.h"
#include "../crypto/hmac.h"
#include "../crypto/dh.h"
#include "../crypto/ca_mgmt.h"
#include "../harness/harness.h"
/* certificate business */
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/tree.h"
#include "../asn1/parseasn1.h"
#include "../asn1/parsecert.h"
#if defined(__ENABLE_SSL_DYNAMIC_CERTIFICATE__) || (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
#include "../crypto/cert_store.h"
#endif
#include "../ssl/ssl.h"
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#include "../dtls/dtls.h"
#include "../common/timer.h"
#if (defined(__ENABLE_MOCANA_DTLS_SRTP__))
#include "../dtls/dtls_srtp.h"
#endif
#endif
#include "../ssl/sslsock.h"
#include "../ssl/sslsock_priv.h"
/* the ECDHE ECDSA signature is a DER encoded ASN1. sequence ... */
#if defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__)
#include "../asn1/derencoder.h"
#endif
#include "../asn1/ASN1TreeWalker.h"
#include "../asn1/oiddefs.h"
#include "../common/moc_net.h"

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#define DTLS_RECORD_SIZE(X)          ((((ubyte2)X[11]) << 8) | ((ubyte2)X[12]))
#endif

/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
static MSTATUS handshakeTimerCallbackFunc(void *s, ubyte* type);
#endif

/*---------------------------------------------------------------------------*/

enum tlsClientCertType
{
    tlsClientCertType_rsa_sign = 1,
    tlsClientCertType_ecdsa_sign = 64,
    tlsClientCertType_rsa_fixed_ecdh = 65,
    tlsClientCertType_ecdsa_fixed_ecdh = 66,
};

#ifdef __ENABLE_MOCANA_ECC__
#define NUM_CLIENT_CERT_TYPES (4)
#else
#define NUM_CLIENT_CERT_TYPES (1)
#endif

enum tlsExtECPointFormat
{
     tlsExtECPointFormat_uncompressed = 0,
     tlsExtECPointFormat_ansiX962_compressed_prime = 1,
     tlsExtECPointFormat_ansiX962_compressed_char2 = 2
};

enum tlsECCurveType
{
    tlsECCurveType_explicit_prime = 1,
    tlsECCurveType_explicit_char2 = 2,
    tlsECCurveType_named_curve = 3
};

/*------------------------------------------------------------------*/
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION

/* static variables */
static const ubyte gHashPad36[SSL_MAX_PADDINGSIZE] =
{
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36
};

static const ubyte gHashPad5C[SSL_MAX_PADDINGSIZE] =
{
    0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
    0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
    0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
    0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
    0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
    0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C
};

#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__))
static MSTATUS fillClientRsaKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
#endif
#if (defined(__ENABLE_MOCANA_SSL_ECDH_SUPPORT__))
static MSTATUS fillClientEcdhKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
#endif
#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__))
static MSTATUS fillClientEcdheKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
static MSTATUS processServerEcdheKeyExchange( SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen);
#endif
#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS fillClientDiffieHellmanKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
static MSTATUS processServerKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen);
#endif
#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS fillClientDiffieHellmanPskKeyExchange(SSLSocket* pSSLSockArg, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
static MSTATUS processServerDiffieHellmanPskKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen);
#endif
#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__))
static MSTATUS fillClientEcdhePskKeyExchange(SSLSocket* pSSLSockArg, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
static MSTATUS processServerEcdhePskKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen);
#endif
#if defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)
static MSTATUS fillClientPskKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
static MSTATUS fillClientRsaPskKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
static MSTATUS processServerPskKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen);
#endif
#ifdef __ENABLE_TLSEXT_RFC6066__
extern MSTATUS SSL_OCSP_addCertificates(void * pContext,certDescriptor * pCertChain,ubyte4 certNum);
#endif
#endif /* __ENABLE_MOCANA_SSL_CLIENT__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_SERVER__
static MSTATUS processClientRsaKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage, ubyte2 recLen, vlong **ppVlongQueue);
#if (defined(__ENABLE_MOCANA_SSL_ECDH_SUPPORT__))
static MSTATUS processClientEcdhKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
#endif
#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
static MSTATUS processClientEcdheKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
static MSTATUS fillServerEcdheKeyExchange(SSLSocket* pSSLSock, ubyte *pHSH, ubyte *pHint, ubyte4 hintLength);
#endif
#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS processClientDiffieHellmanKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage, ubyte2 recLen, vlong **ppVlongQueue);
static MSTATUS fillServerKeyExchange(SSLSocket* pSSLSock, ubyte *pHSH, ubyte *pPskHint, ubyte4 pskHintLength);
#endif
#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
#ifdef __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__
static MSTATUS processClientEcdhePskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage, ubyte2 recLen, vlong **ppVlongQueue);
#endif
#ifdef __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__
static MSTATUS processClientDiffieHellmanPskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage, ubyte2 recLen, vlong **ppVlongQueue);
#endif
#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
static MSTATUS processClientRsaPskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage, ubyte2 recLen, vlong **ppVlongQueue);
#endif
static MSTATUS processClientPskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage, ubyte2 recLen, vlong **ppVlongQueue);
static MSTATUS fillServerPskKeyExchange(SSLSocket* pSSLSockTemp, ubyte *pHSH, ubyte *pPskHint, ubyte4 pskHintLength);
#endif
#endif /* __ENABLE_MOCANA_SSL_SERVER__ */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
#define KEYEX_DESCR(CIPHER_ID,FILL_SERVER_KEX,PROCESS_SERVER_KEX,FILL_CLIENT_KEX,PROCESS_CLIENT_KEX) { CIPHER_ID,FILL_SERVER_KEX,PROCESS_SERVER_KEX,FILL_CLIENT_KEX,PROCESS_CLIENT_KEX }
#elif defined(__ENABLE_MOCANA_SSL_CLIENT__)
#define KEYEX_DESCR(CIPHER_ID,FILL_SERVER_KEX,PROCESS_SERVER_KEX,FILL_CLIENT_KEX,PROCESS_CLIENT_KEX) { CIPHER_ID,PROCESS_SERVER_KEX,FILL_CLIENT_KEX }
#else
#define KEYEX_DESCR(CIPHER_ID,FILL_SERVER_KEX,PROCESS_SERVER_KEX,FILL_CLIENT_KEX,PROCESS_CLIENT_KEX) { CIPHER_ID,FILL_SERVER_KEX,PROCESS_CLIENT_KEX }
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
static KeyExAuthSuiteInfo RsaSuite =                KEYEX_DESCR( SSL_RSA,                                /* cipher flags */
                                                                NULL,                                   /* server-side:FillServerKEX */
                                                                NULL,                                   /* client-side:ProcessServerKEX */
                                                                fillClientRsaKeyExchange,               /* client-side:FillClientKEX */
                                                                processClientRsaKeyExchange );          /* server-side:ProcessClientKEX */
#endif
#ifdef __ENABLE_MOCANA_SSL_ECDH_SUPPORT__
#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
static KeyExAuthSuiteInfo EcdhRsaSuite =            KEYEX_DESCR( SSL_ECDH_RSA,
                                                                NULL,
                                                                NULL,
                                                                fillClientEcdhKeyExchange,
                                                                processClientEcdhKeyExchange);
#endif
#endif
#ifdef __ENABLE_MOCANA_SSL_ECDH_SUPPORT__
static KeyExAuthSuiteInfo EcdhEcdsaSuite =          KEYEX_DESCR( SSL_ECDH_ECDSA,
                                                                NULL,
                                                                NULL,
                                                                fillClientEcdhKeyExchange,
                                                                processClientEcdhKeyExchange);
#endif
#ifdef __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
static KeyExAuthSuiteInfo EcdheRsaSuite =           KEYEX_DESCR( SSL_ECDHE_RSA,
                                                                fillServerEcdheKeyExchange,
                                                                processServerEcdheKeyExchange,
                                                                fillClientEcdheKeyExchange,
                                                                processClientEcdheKeyExchange);
#endif
#endif
#ifdef __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
static KeyExAuthSuiteInfo EcdheEcdsaSuite =         KEYEX_DESCR( SSL_ECDHE_ECDSA,
                                                                fillServerEcdheKeyExchange,
                                                                processServerEcdheKeyExchange,
                                                                fillClientEcdheKeyExchange,
                                                                processClientEcdheKeyExchange);
#endif
#ifdef __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__
static KeyExAuthSuiteInfo EcdheAnonSuite =          KEYEX_DESCR( SSL_ECDH_ANON,
                                                                fillServerEcdheKeyExchange,
                                                                processServerEcdheKeyExchange,
                                                                fillClientEcdheKeyExchange,
                                                                processClientEcdheKeyExchange);
#endif
#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__))
static KeyExAuthSuiteInfo DiffieHellmanRsaSuite =   KEYEX_DESCR( SSL_DH_RSA,
                                                                fillServerKeyExchange,
                                                                processServerKeyExchange,
                                                                fillClientDiffieHellmanKeyExchange,
                                                                processClientDiffieHellmanKeyExchange );
#endif

#ifdef __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__
static KeyExAuthSuiteInfo DiffieHellmanAnonSuite =  KEYEX_DESCR( SSL_DH_ANON,
                                                                fillServerKeyExchange,
                                                                processServerKeyExchange,
                                                                fillClientDiffieHellmanKeyExchange,
                                                                processClientDiffieHellmanKeyExchange );
#endif

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
static KeyExAuthSuiteInfo PskSuite =                KEYEX_DESCR( SSL_PSK,
                                                                fillServerPskKeyExchange,
                                                                processServerPskKeyExchange,
                                                                fillClientPskKeyExchange,
                                                                processClientPskKeyExchange );

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
static KeyExAuthSuiteInfo RsaPskSuite =             KEYEX_DESCR( SSL_RSA_PSK,
                                                                fillServerPskKeyExchange,
                                                                processServerPskKeyExchange,
                                                                fillClientRsaPskKeyExchange,
                                                                processClientRsaPskKeyExchange );
#endif /* __ENABLE_MOCANA_SSL_RSA_SUPPORT__ */

#ifdef __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__
static KeyExAuthSuiteInfo DiffieHellmanPskSuite =   KEYEX_DESCR( SSL_DH_PSK,
                                                                fillServerKeyExchange,
                                                                processServerDiffieHellmanPskKeyExchange,
                                                                fillClientDiffieHellmanPskKeyExchange,
                                                                processClientDiffieHellmanPskKeyExchange );
#endif /* __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__ */

#ifdef __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__
static KeyExAuthSuiteInfo EcdhePskSuite =           KEYEX_DESCR( SSL_ECDH_PSK,
                                                                fillServerEcdheKeyExchange,
                                                                processServerEcdhePskKeyExchange,
                                                                fillClientEcdhePskKeyExchange,
                                                                processClientEcdhePskKeyExchange );
#endif /* __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__ */

#endif /* __ENABLE_MOCANA_SSL_PSK_SUPPORT__ */

/* Bulk Hash Algorithms */
static const BulkHashAlgo MD5Suite =
    { MD5_RESULT_SIZE, MD5_BLOCK_SIZE, MD5Alloc_m, MD5Free_m, (BulkCtxInitFunc)MD5Init_m, (BulkCtxUpdateFunc)MD5Update_m, (BulkCtxFinalFunc)MD5Final_m };

static const BulkHashAlgo SHA1Suite =
    { SHA1_RESULT_SIZE, SHA1_BLOCK_SIZE, SHA1_allocDigest, SHA1_freeDigest, (BulkCtxInitFunc)SHA1_initDigest, (BulkCtxUpdateFunc)SHA1_updateDigest, (BulkCtxFinalFunc)SHA1_finalDigest };
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

#if defined(__SSL_SINGLE_PASS_SUPPORT__)
#define SSL_CIPHER_DEF(CIPHER_ID,CIPHER_ENABLE,MINSSLVER,KEY_SIZE,SYM_CIPHER,KEYEX,PRFHASH,HW_IN,HW_OUT) { CIPHER_ID, CIPHER_ENABLE, MINSSLVER, KEY_SIZE, SYM_CIPHER, KEYEX, PRFHASH, HW_IN, HW_OUT }
#else
#define SSL_CIPHER_DEF(CIPHER_ID,CIPHER_ENABLE,MINSSLVER,KEY_SIZE,SYM_CIPHER,KEYEX,PRFHASH, HW_IN,HW_OUT) { CIPHER_ID, CIPHER_ENABLE, MINSSLVER, KEY_SIZE, SYM_CIPHER, KEYEX, PRFHASH}
#endif


/*------------------------------------------------------------------*/

/* used for combo algo only, i.e. encryption algo + hash algo */
typedef struct ComboAlgo
{
    const BulkEncryptionAlgo *pBEAlgo;
    const BulkHashAlgo *pBHAlgo;
} ComboAlgo;

/* Encryption Layer */
static BulkCtx SSLComboCipher_createCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, ubyte4 keySize, sbyte4 encrypt, const ComboAlgo *pComboAlgo);
static MSTATUS SSLComboCipher_deleteCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx, const ComboAlgo *pComboAlgo);
static MSTATUS SSLComboCipher_decryptVerifyRecord(SSLSocket* pSSLSock, ubyte protocol, const ComboAlgo *pComboAlgo);
static MSTATUS SSLComboCipher_formEncryptedRecord(SSLSocket* pSSLSock, ubyte* data, ubyte2 dataSize, sbyte padLen, const ComboAlgo *pComboAlgo);

#if defined(__ENABLE_MOCANA_AEAD_CIPHER__)
static BulkCtx SSLAeadCipher_createCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, ubyte4 keySize, sbyte4 encrypt, const AeadAlgo *pAeadAlgo);
static MSTATUS SSLAeadCipher_deleteCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx, const AeadAlgo *pAeadAlgo);
static MSTATUS SSLAeadCipher_decryptVerifyRecord(SSLSocket* pSSLSock, ubyte protocol, const AeadAlgo *pAeadAlgo);
static MSTATUS SSLAeadCipher_formEncryptedRecord(SSLSocket* pSSLSock, ubyte* data, ubyte2 dataSize, sbyte padLen, const AeadAlgo *pAeadAlgo);
#endif
static MSTATUS sendData(SSLSocket* pSSLSock, ubyte protocol, const sbyte* data, sbyte4 dataSize, intBoolean skipEmptyMesg);

/* Crypto Helpers */
static void    addToHandshakeHash(SSLSocket* pSSLSock, ubyte* data, sbyte4 size);
static sbyte4  computePadLength(sbyte4 msgSize, sbyte4 blockSize);
#if defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
static MSTATUS calculateSSLTLSHashes(SSLSocket *pSSLSock, sbyte4 client, ubyte* result, enum hashTypes hashType);
#endif
static MSTATUS calculateTLSFinishedVerify(SSLSocket *pSSLSock, sbyte4 client, ubyte result[TLS_VERIFYDATASIZE]);
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
static MSTATUS SHAMD5Rounds(SSLSocket *pSSLSock, const ubyte* pPresecret, ubyte4 presecretLength, const ubyte data[2 * SSL_RANDOMSIZE], sbyte4 numRounds, ubyte* dest);
extern MSTATUS SSL_SOCK_computeSSLMAC(SSLSocket *pSSLSock, ubyte* secret, sbyte4 macSize,
                                      ubyte *pSequence, ubyte2 mesgLen,
                                      ubyte result[SSL_MAXDIGESTSIZE]);
#endif
extern MSTATUS SSL_SOCK_computeTLSMAC(MOC_HASH(SSLSocket *pSSLSock) ubyte* secret,
                                      ubyte *pMesg, ubyte2 mesgLen,
                                      ubyte *pMesgOpt, ubyte2 mesgOptLen,
                                      ubyte result[SSL_MAXDIGESTSIZE], const BulkHashAlgo *pBHAlgo);
extern MSTATUS SSL_SOCK_generateKeyMaterial(SSLSocket* pSSLSock, ubyte* preMasterSecret, ubyte4 preMasterSecretLength);
extern MSTATUS SSL_SOCK_setClientKeyMaterial(SSLSocket *pSSLSock);
extern MSTATUS SSL_SOCK_setServerKeyMaterial(SSLSocket *pSSLSock);
static void    resetCipher(SSLSocket* pSSLSock, intBoolean clientSide, intBoolean serverSide);
/* HMAC implementations for TLS */
static void    P_hash(SSLSocket *pSSLSock, const ubyte* secret, sbyte4 secretLen,
                      const ubyte* seed, sbyte4 seedLen,
                      ubyte* result, sbyte4 resultLen, const BulkHashAlgo *pBHAlgo);
static MSTATUS PRF(SSLSocket *pSSLSock, const ubyte* secret, sbyte4 secretLen,
                   const ubyte* labelSeed, sbyte4 labelSeedLen,
                   ubyte* result, sbyte4 resultLen);

/*------------------------------------------------------------------*/
/* MACROs for creating encryption + hash combination ciphers: bSize = blockSize, hSize = hashSize */
#define MAKE_COMBO_CIPHER(e,h, bSize, hSize)   \
    static const ComboAlgo k##e##h##_comboAlgo = {&CRYPTO_##e##Suite, &h##Suite}; \
    static BulkCtx  e##h##_createCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, ubyte4 keySize, sbyte4 encrypt) \
    { return SSLComboCipher_createCtx(MOC_SYM(hwAccelCtx)  key, keySize, encrypt, &k##e##h##_comboAlgo); } \
    \
    static MSTATUS e##h##_deleteCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx* pCtx) \
    { return SSLComboCipher_deleteCtx(MOC_SYM(hwAccelCtx) pCtx, &k##e##h##_comboAlgo); } \
    \
    static MSTATUS e##h##_encrypt(SSLSocket *pSSLSock, ubyte* pData, ubyte2 dataLength, sbyte padLength) \
    { return SSLComboCipher_formEncryptedRecord(pSSLSock, pData, dataLength, padLength, &k##e##h##_comboAlgo); } \
    \
    static MSTATUS e##h##_decrypt(SSLSocket *pSSLSock, ubyte protocol) \
    { return SSLComboCipher_decryptVerifyRecord(pSSLSock, protocol, &k##e##h##_comboAlgo); } \
    \
    static MSTATUS e##h##_getField(CipherField type) \
    { return (1 == type? bSize : (2 == type? hSize : 0)); } \
    \
    static SSLCipherAlgo e##h = {bSize, hSize, 0, e##h##_createCtx, e##h##_deleteCtx, e##h##_encrypt, e##h##_decrypt, e##h##_getField };

#ifndef __DISABLE_AES_CIPHERS__
MAKE_COMBO_CIPHER( AES, SHA1,  AES_BLOCK_SIZE, SHA1_RESULT_SIZE )
#ifndef __DISABLE_MOCANA_SHA256__
MAKE_COMBO_CIPHER( AES, SHA256,  AES_BLOCK_SIZE, SHA256_RESULT_SIZE )
#endif
#if ((defined __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || (defined __ENABLE_MOCANA_SSL_PSK_SUPPORT__))
#ifndef __DISABLE_MOCANA_SHA384__
MAKE_COMBO_CIPHER( AES, SHA384,  AES_BLOCK_SIZE, SHA384_RESULT_SIZE )
#endif
#endif
#endif

#ifndef __DISABLE_3DES_CIPHERS__
MAKE_COMBO_CIPHER( TripleDES, SHA1, THREE_DES_BLOCK_SIZE, SHA1_RESULT_SIZE )
#endif

#if MIN_SSL_MINORVERSION==SSL3_MINORVERSION
#ifndef __DISABLE_ARC4_CIPHERS__
/* no support for RC4 MD5 */
MAKE_COMBO_CIPHER( RC4, SHA1, 0, SHA1_RESULT_SIZE )
#endif
#endif

#ifdef __ENABLE_DES_CIPHER__
MAKE_COMBO_CIPHER( DES, SHA1, DES_BLOCK_SIZE, SHA1_RESULT_SIZE )
#endif

#ifdef __ENABLE_NIL_CIPHER__
#ifndef __DISABLE_NULL_MD5_CIPHER__
MAKE_COMBO_CIPHER( Nil, MD5, 0, MD5_RESULT_SIZE )
#endif
MAKE_COMBO_CIPHER( Nil, SHA1, 0, SHA1_RESULT_SIZE )
#ifndef __DISABLE_MOCANA_SHA256__
MAKE_COMBO_CIPHER( Nil, SHA256, 0, SHA256_RESULT_SIZE )
#endif
/* this is used only for PSK ciphers as of 06/22/2015 */
#if !defined(__DISABLE_MOCANA_SHA384__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)
MAKE_COMBO_CIPHER( Nil, SHA384, 0, SHA384_RESULT_SIZE )
#endif
#endif

#if defined(__ENABLE_MOCANA_AEAD_CIPHER__)
/* MACROs for creating AEAD ciphers:  fIVSize = fixedIVSize, rIVSize = recordIVSize, tSize = tagSize */
#define MAKE_AEAD_CIPHER(aead, fIVSize, rIVSize, tSize)   \
    static BulkCtx  aead##_createCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, ubyte4 keySize, sbyte4 encrypt) \
    { return SSLAeadCipher_createCtx(MOC_SYM(hwAccelCtx) key, keySize, encrypt, &aead##Suite); } \
    \
    static MSTATUS aead##_deleteCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx* pCtx) \
    { return SSLAeadCipher_deleteCtx(MOC_SYM(hwAccelCtx) pCtx, &aead##Suite); } \
    \
    static MSTATUS aead##_encrypt(SSLSocket *pSSLSock, ubyte* pData, ubyte2 dataLength, sbyte padLength) \
    { return SSLAeadCipher_formEncryptedRecord(pSSLSock, pData, dataLength, padLength, &aead##Suite); } \
    \
    static MSTATUS aead##_decrypt(SSLSocket *pSSLSock, ubyte protocol) \
    { return SSLAeadCipher_decryptVerifyRecord(pSSLSock, protocol, &aead##Suite); } \
    \
    static MSTATUS aead##_getField(CipherField type) \
    { return (3 == type?  fIVSize : (4 == type? rIVSize : (5 == type?  tSize : 0))); } \
    \
    static SSLCipherAlgo aead = {fIVSize, rIVSize, tSize, aead##_createCtx, aead##_deleteCtx, aead##_encrypt, aead##_decrypt, aead##_getField };

#if defined(__ENABLE_MOCANA_GCM__)
#define GCM_FIXED_IV_LENGTH 4
#define GCM_RECORD_IV_LENGTH 8
#ifdef __ENABLE_MOCANA_GCM_64K__
static AeadAlgo GCMSuite =
    { GCM_FIXED_IV_LENGTH, GCM_RECORD_IV_LENGTH, AES_BLOCK_SIZE, GCM_createCtx_64k, GCM_deleteCtx_64k, GCM_cipher_64k };
#endif
#ifdef __ENABLE_MOCANA_GCM_4K__
static AeadAlgo GCMSuite =
    { GCM_FIXED_IV_LENGTH, GCM_RECORD_IV_LENGTH, AES_BLOCK_SIZE, GCM_createCtx_4k, GCM_deleteCtx_4k, GCM_cipher_4k };
#endif
#ifdef __ENABLE_MOCANA_GCM_256B__
static AeadAlgo GCMSuite =
    { GCM_FIXED_IV_LENGTH, GCM_RECORD_IV_LENGTH, AES_BLOCK_SIZE, GCM_createCtx_256b, GCM_deleteCtx_256b, GCM_cipher_256b };
#endif
MAKE_AEAD_CIPHER( GCM, GCM_FIXED_IV_LENGTH, GCM_RECORD_IV_LENGTH, AES_BLOCK_SIZE )
#endif

#if defined(__ENABLE_MOCANA_CCM__) || defined(__ENABLE_MOCANA_CCM_8__)
#define AESCCM_FIXED_IV_LENGTH   4
#define AESCCM_RECORD_IV_LENGTH  8

#ifdef __ENABLE_MOCANA_CCM_8__
#define AESCCM_8_TAGSIZE         8

static AeadAlgo AESCCM8Suite =
    { AESCCM_FIXED_IV_LENGTH, AESCCM_RECORD_IV_LENGTH, AESCCM_8_TAGSIZE, AESCCM_createCtx, AESCCM_deleteCtx, AESCCM_cipher };

MAKE_AEAD_CIPHER( AESCCM8, AESCCM_FIXED_IV_LENGTH, AESCCM_RECORD_IV_LENGTH, AESCCM_8_TAGSIZE )

#endif /* __ENABLE_MOCANA_CCM_8__ */

#ifdef __ENABLE_MOCANA_CCM__
#define AESCCM_TAGSIZE         16

static AeadAlgo AESCCM16Suite =
{ AESCCM_FIXED_IV_LENGTH, AESCCM_RECORD_IV_LENGTH, AESCCM_TAGSIZE, AESCCM_createCtx, AESCCM_deleteCtx, AESCCM_cipher };

MAKE_AEAD_CIPHER( AESCCM16, AESCCM_FIXED_IV_LENGTH, AESCCM_RECORD_IV_LENGTH, AESCCM_TAGSIZE )

#endif /* __ENABLE_MOCANA_CCM__ */

#endif /* __ENABLE_MOCANA_CCM__ || __ENABLE_MOCANA_CCM_8__  */

#endif /* __ENABLE_MOCANA_AEAD_CIPHER__ */

/* table with infos about the ssl cipher suites
 *   THIS IS IN ORDER OF PREFERENCE
 */
static CipherSuiteInfo gCipherSuites[] = {     /*cipherIndex supported keysize ivsize cipherFun hashtype*/

/***** Ephemeral EC diffie-hellman */
#ifdef __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
#ifndef __DISABLE_AES_CIPHERS__

#ifdef __ENABLE_MOCANA_GCM__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0xC02C TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384*/ SSL_CIPHER_DEF( 0xC02C, 1, 3, 32, &GCM, &EcdheEcdsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC030 TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384*/   SSL_CIPHER_DEF( 0xC030, 1, 3, 32, &GCM, &EcdheRsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0xC02B TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256*/ SSL_CIPHER_DEF( 0xC02B, 1, 3, 16, &GCM, &EcdheEcdsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC02F TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256*/   SSL_CIPHER_DEF( 0xC02F, 1, 3, 16, &GCM, &EcdheRsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#endif

#endif /* __ENABLE_MOCANA_GCM__ */

#ifdef __ENABLE_MOCANA_CCM__

#ifndef __DISABLE_AES_256_CIPHER__
    /* 0xC0AD TLS_ECDHE_ECDSA_WITH_AES_256_CCM */ SSL_CIPHER_DEF( 0xC0AD, 1, 3, 32, &AESCCM16, &EcdheEcdsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES_128_CIPHER__
    /* 0xC0AE TLS_ECDHE_ECDSA_WITH_AES_128_CCM */ SSL_CIPHER_DEF( 0xC0AC, 1, 3, 16, &AESCCM16, &EcdheEcdsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif /* __ENABLE_MOCANA_CCM__ */

#ifdef __ENABLE_MOCANA_CCM_8__

#ifndef __DISABLE_AES_256_CIPHER__
    /* 0xC0AE TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8 */ SSL_CIPHER_DEF( 0xC0AF, 1, 3, 32, &AESCCM8, &EcdheEcdsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES_128_CIPHER__
    /* 0xC0AE TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 */ SSL_CIPHER_DEF( 0xC0AE, 1, 3, 16, &AESCCM8, &EcdheEcdsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif /* __ENABLE_MOCANA_CCM_8__ */

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0xC024 TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384*/ SSL_CIPHER_DEF( 0xC024, 1, 3, 32, &AESSHA384, &EcdheEcdsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC028 TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384*/   SSL_CIPHER_DEF( 0xC028, 1, 3, 32, &AESSHA384, &EcdheRsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
    /* 0xC00A TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA*/    SSL_CIPHER_DEF( 0xC00A, 1, 1, 32, &AESSHA1, &EcdheEcdsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC014 TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA*/      SSL_CIPHER_DEF( 0xC014, 1, 1, 32, &AESSHA1, &EcdheRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif


#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0xC023 TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256*/ SSL_CIPHER_DEF( 0xC023, 1, 3, 16, &AESSHA256, &EcdheEcdsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC027 TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256*/   SSL_CIPHER_DEF( 0xC027, 1, 3, 16, &AESSHA256, &EcdheRsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
    /* 0xC009 TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA*/    SSL_CIPHER_DEF( 0xC009, 1, 1, 16, &AESSHA1, &EcdheEcdsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC013 TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA*/      SSL_CIPHER_DEF( 0xC013, 1, 1, 16, &AESSHA1, &EcdheRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif /* __DISABLE_AES_CIPHERS__ */

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0xC008 SSL_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA*/   SSL_CIPHER_DEF( 0xC008, 1, 1, 24, &TripleDESSHA1, &EcdheEcdsaSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
    /* 0xC012 SSL_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA*/     SSL_CIPHER_DEF( 0xC012, 1, 1, 24, &TripleDESSHA1, &EcdheRsaSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#endif /* __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__ */

/***** diffie-hellman ephemeral rsa */
#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__))
#ifndef __DISABLE_AES_CIPHERS__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x006B TLS_DHE_RSA_WITH_AES_256_CBC_SHA256*/ SSL_CIPHER_DEF( 0x006B, 1, 3, 32, &AESSHA256, &DiffieHellmanRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
    /*  0x0039 TLS_DHE_RSA_WITH_AES_256_CBC_SHA*/   SSL_CIPHER_DEF( 0x0039, 1, 0, 32, &AESSHA1, &DiffieHellmanRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x0067 TLS_DHE_RSA_WITH_AES_128_CBC_SHA256*/ SSL_CIPHER_DEF( 0x0067, 1, 3, 16, &AESSHA256, &DiffieHellmanRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
   /*  0x0033 TLS_DHE_RSA_WITH_AES_128_CBC_SHA*/   SSL_CIPHER_DEF( 0x0033, 1, 0, 16, &AESSHA1, &DiffieHellmanRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif /* __DISABLE_AES_CIPHERS__ */

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0x0016 SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA*/   SSL_CIPHER_DEF( 0x0016, 1, 0, 24, &TripleDESSHA1, &DiffieHellmanRsaSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#endif

/***** EC diffie-hellman */
#ifdef __ENABLE_MOCANA_SSL_ECDH_SUPPORT__
#ifndef __DISABLE_AES_CIPHERS__

#ifdef __ENABLE_MOCANA_GCM__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0xC02E TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384*/ SSL_CIPHER_DEF( 0xC02E, 1, 3, 32, &GCM, &EcdhEcdsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC032 TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384*/   SSL_CIPHER_DEF( 0xC032, 1, 3, 32, &GCM, &EcdhRsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0xC02D TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256*/  SSL_CIPHER_DEF( 0xC02D, 1, 3, 16, &GCM, &EcdhEcdsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC031 TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256*/    SSL_CIPHER_DEF( 0xC031, 1, 3, 16, &GCM, &EcdhRsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#endif

#endif /* __ENABLE_MOCANA_GCM__ */

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0xC026 TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384*/ SSL_CIPHER_DEF( 0xC026, 1, 3, 32, &AESSHA384, &EcdhEcdsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC02A TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384*/   SSL_CIPHER_DEF( 0xC02A, 1, 3, 32, &AESSHA384, &EcdhRsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
    /* 0xC005 TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA*/  SSL_CIPHER_DEF( 0xC005, 1, 1, 32, &AESSHA1, &EcdhEcdsaSuite,  NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC00F TLS_ECDH_RSA_WITH_AES_256_CBC_SHA*/    SSL_CIPHER_DEF( 0xC00F, 1, 1, 32, &AESSHA1, &EcdhRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0xC025 TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256*/  SSL_CIPHER_DEF( 0xC025, 1, 3, 16, &AESSHA256, &EcdhEcdsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC029 TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256*/    SSL_CIPHER_DEF( 0xC029, 1, 3, 16, &AESSHA256, &EcdhRsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
    /* 0xC004 TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA*/     SSL_CIPHER_DEF( 0xC004, 1, 1, 16, &AESSHA1, &EcdhEcdsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
    /* 0xC00E TLS_ECDH_RSA_WITH_AES_128_CBC_SHA*/       SSL_CIPHER_DEF( 0xC00E, 1, 1, 16, &AESSHA1, &EcdhRsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif /* __DISABLE_AES_CIPHERS__ */

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0xC003 SSL_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA*/    SSL_CIPHER_DEF( 0xC003, 1, 1, 24, &TripleDESSHA1, &EcdhEcdsaSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
    /* 0xC00D SSL_ECDH_RSA_WITH_3DES_EDE_CBC_SHA*/      SSL_CIPHER_DEF( 0xC00D, 1, 1, 24, &TripleDESSHA1, &EcdhRsaSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#endif /* __ENABLE_MOCANA_SSL_ECDH_SUPPORT__ */


/***** RSA ciphers */
#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
#ifndef __DISABLE_AES_CIPHERS__

#ifdef __ENABLE_MOCANA_GCM__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x009D TLS_RSA_WITH_AES_256_GCM_SHA384*/ SSL_CIPHER_DEF( 0x009D, 1, 3, 32, &GCM, &RsaSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x009C TLS_RSA_WITH_AES_128_GCM_SHA256*/ SSL_CIPHER_DEF( 0x009C, 1, 3, 16, &GCM, &RsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#endif

#endif /* __ENABLE_MOCANA_GCM__ */

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x003D TLS_RSA_WITH_AES_256_CBC_SHA256*/ SSL_CIPHER_DEF( 0x003D, 1, 3, 32, &AESSHA256, &RsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
   /* 0x0035 TLS_RSA_WITH_AES_256_CBC_SHA*/    SSL_CIPHER_DEF( 0x0035, 1, 0, 32, &AESSHA1, &RsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x003C TLS_RSA_WITH_AES_128_CBC_SHA256*/ SSL_CIPHER_DEF( 0x003C, 1, 3, 16, &AESSHA256, &RsaSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
    /* 0x002F TLS_RSA_WITH_AES_128_CBC_SHA*/   SSL_CIPHER_DEF( 0x002F, 1, 0, 16, &AESSHA1, &RsaSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif  /* __DISABLE_AES_CIPHERS__ */

    /* pick ARC4 over 3DES CBC if SSL3 is enabled, there's no real good choice here anyway */
#if MIN_SSL_MINORVERSION==SSL3_MINORVERSION
#ifndef __DISABLE_ARC4_CIPHERS__
    /*  5 SSL_RSA_WITH_RC4_128_SHA*/       SSL_CIPHER_DEF( 0x0005, 1, 0, 16, &RC4SHA1, &RsaSuite, NULL, SINGLE_PASS_RC4_SHA1_IN, SINGLE_PASS_RC4_SHA1_OUT ),
#endif
#endif

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0x000A SSL_RSA_WITH_3DES_EDE_CBC_SHA*/  SSL_CIPHER_DEF( 0x000A, 1, 0, 24, &TripleDESSHA1, &RsaSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#endif

/***** Ephemeral EC diffie-hellman anonymous*/
#ifdef __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__
#ifndef __DISABLE_AES_CIPHERS__
#ifndef __DISABLE_AES256_CIPHER__
    /* 0xC019 TLS_ECDH_anon_WITH_AES_256_CBC_SHA  */ SSL_CIPHER_DEF( 0xC019, 1, 1, 32, &AESSHA1, &EcdheAnonSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#ifndef __DISABLE_AES128_CIPHER__
    /* 0xC018 TLS_ECDH_anon_WITH_AES_128_CBC_SHA*/   SSL_CIPHER_DEF( 0xC018, 1, 1, 16, &AESSHA1, &EcdheAnonSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
#endif /* __DISABLE_AES_CIPHERS__ */
#ifndef __DISABLE_3DES_CIPHERS__
    /* 0xC017 SSL_ECDH_anon_WITH_3DES_EDE_CBC_SHA*/  SSL_CIPHER_DEF( 0xC017, 1, 1, 24, &TripleDESSHA1, &EcdheAnonSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif
#endif /* __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__ */


/***** diffie-hellman ephemeral anonymous */
#ifdef __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__

#ifndef __DISABLE_AES_CIPHERS__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x006D TLS_DH_ANON_WITH_AES_256_CBC_SHA256*/ SSL_CIPHER_DEF( 0x006D, 1, 3, 32, &AESSHA256, &DiffieHellmanAnonSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
    /* 0x003A TLS_DH_ANON_WITH_AES_256_CBC_SHA*/    SSL_CIPHER_DEF( 0x003A, 1, 0, 32, &AESSHA1, &DiffieHellmanAnonSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x006C TLS_DH_ANON_WITH_AES_128_CBC_SHA256*/ SSL_CIPHER_DEF( 0x006C, 1, 3, 16, &AESSHA256, &DiffieHellmanAnonSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif
   /* 0x0034 TLS_DH_ANON_WITH_AES_128_CBC_SHA*/    SSL_CIPHER_DEF( 0x0034, 1, 0, 16, &AESSHA1, &DiffieHellmanAnonSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif   /* __DISABLE_AES_CIPHERS__ */

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0x001B SSL_DH_ANON_WITH_3DES_EDE_CBC_SHA*/  SSL_CIPHER_DEF( 0x001B, 1, 0, 24, &TripleDESSHA1, &DiffieHellmanAnonSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#endif /* __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__ */

/*********************** preshared key ************************/

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__

/***** RFC5487 supported ciphers */
#ifndef __DISABLE_AES_CIPHERS__

#ifdef __ENABLE_MOCANA_GCM__

#ifdef __ENABLE_MOCANA_SSL_DHE_SUPPORT__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00AB TLS_DHE_PSK_WITH_AES_256_GCM_SHA384 */  SSL_CIPHER_DEF( 0x00AB, 1, 3, 32, &GCM, &DiffieHellmanPskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00AA TLS_DHE_PSK_WITH_AES_128_GCM_SHA256 */  SSL_CIPHER_DEF( 0x00AA, 1, 3, 16, &GCM, &DiffieHellmanPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#endif /* __ENABLE_MOCANA_SSL_DHE_SUPPORT__ */

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00A9 TLS_PSK_WITH_AES_256_GCM_SHA384 */  SSL_CIPHER_DEF( 0x00A9, 1, 3, 32, &GCM, &PskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00A8 TLS_PSK_WITH_AES_128_GCM_SHA256 */  SSL_CIPHER_DEF( 0x00A8, 1, 3, 16, &GCM, &PskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00AD TLS_RSA_PSK_WITH_AES_256_GCM_SHA384 */  SSL_CIPHER_DEF( 0x00AD, 1, 3, 32, &GCM, &RsaPskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00AC TLS_RSA_PSK_WITH_AES_128_GCM_SHA256 */  SSL_CIPHER_DEF( 0x00AC, 1, 3, 16, &GCM, &RsaPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#endif /* __ENABLE_MOCANA_SSL_RSA_SUPPORT__ */

#endif /* __ENABLE_MOCANA_GCM__ */

#ifdef __ENABLE_MOCANA_CCM__
#ifdef __ENABLE_MOCANA_SSL_DHE_SUPPORT__

#ifndef __DISABLE_AES256_CIPHER__
    /* 0xC0A7 TLS_DHE_PSK_WITH_AES_256_CCM */  SSL_CIPHER_DEF( 0xC0A7, 1, 3, 32, &AESCCM16, &DiffieHellmanPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0xC0A6 TLS_DHE_PSK_WITH_AES_128_CCM */  SSL_CIPHER_DEF( 0xC0A6, 1, 3, 16, &AESCCM16, &DiffieHellmanPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#endif /* __ENABLE_MOCANA_SSL_DHE_SUPPORT__ */

#ifndef __DISABLE_AES256_CIPHER__
    /* 0xC0A5 TLS_PSK_WITH_AES_256_CCM */  SSL_CIPHER_DEF( 0xC0A5, 1, 3, 32, &AESCCM16, &PskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0xC0A4 TLS_PSK_WITH_AES_128_CCM */  SSL_CIPHER_DEF( 0xC0A4, 1, 3, 16, &AESCCM16, &PskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#endif /* __ENABLE_MOCANA_CCM__ */


#ifdef __ENABLE_MOCANA_CCM_8__
#ifdef __ENABLE_MOCANA_SSL_DHE_SUPPORT__

#ifndef __DISABLE_AES256_CIPHER__
    /* 0xC0AB TLS_DHE_PSK_WITH_AES_256_CCM_8 */  SSL_CIPHER_DEF( 0xC0AB, 1, 3, 32, &AESCCM8, &DiffieHellmanPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0xC0AA TLS_DHE_PSK_WITH_AES_128_CCM_8 */  SSL_CIPHER_DEF( 0xC0AA, 1, 3, 16, &AESCCM8, &DiffieHellmanPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#endif /* __ENABLE_MOCANA_SSL_DHE_SUPPORT__ */

#ifndef __DISABLE_AES256_CIPHER__
    /* 0xC0A9 TLS_PSK_WITH_AES_256_CCM_8 */  SSL_CIPHER_DEF( 0xC0A9, 1, 3, 32, &AESCCM8, &PskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0xC0A8 TLS_PSK_WITH_AES_128_CCM_8 */  SSL_CIPHER_DEF( 0xC0A8, 1, 3, 16, &AESCCM8, &PskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#endif /* __ENABLE_MOCANA_CCM_8__ */

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00AF TLS_PSK_WITH_AES_256_CBC_SHA384 */  SSL_CIPHER_DEF( 0x00AF, 1, 1, 32, &AESSHA384, &PskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00AE TLS_PSK_WITH_AES_128_CBC_SHA256 */  SSL_CIPHER_DEF( 0x00AE, 1, 1, 16, &AESSHA256, &PskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES256_CIPHER__
    /* 0x008D TLS_PSK_WITH_AES_256_CBC_SHA*/   SSL_CIPHER_DEF( 0x008D, 1, 0, 32, &AESSHA1, &PskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0x008C TLS_PSK_WITH_AES_128_CBC_SHA*/   SSL_CIPHER_DEF( 0x008C, 1, 0, 16, &AESSHA1, &PskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif /* __DISABLE_AES_CIPHERS__   */

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0x008B TLS_PSK_WITH_3DES_EDE_CBC_SHA*/  SSL_CIPHER_DEF( 0x008B, 1, 0, 24, &TripleDESSHA1, &PskSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#ifdef __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__
#ifndef __DISABLE_AES_CIPHERS__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0xC038 TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384*/ SSL_CIPHER_DEF( 0xC038, 1, 1, 32, &AESSHA384, &EcdhePskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0xC037 TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256*/ SSL_CIPHER_DEF(0xC037, 1, 1, 16, &AESSHA256, &EcdhePskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES256_CIPHER__
    /* 0xC036 TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA*/ SSL_CIPHER_DEF(0xC036, 1, 1, 32, &AESSHA1, &EcdhePskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0xC035 TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA*/ SSL_CIPHER_DEF(0xC035, 1, 1, 16, &AESSHA1, &EcdhePskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#endif /* __DISABLE_AES_CIPHERS__ */

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0xC034 TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA*/ SSL_CIPHER_DEF(0xC034, 1, 1, 24, &TripleDESSHA1, &EcdhePskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif /* __DISABLE_3DES_CIPHERS__ */

#ifdef __ENABLE_NIL_CIPHER__

#ifndef __DISABLE_MOCANA_SHA384__
    /* 0xC03B TLS_ECDHE_PSK_WITH_NULL_SHA384*/ SSL_CIPHER_DEF(0xC03B, 1, 1, 0, &NilSHA384, &EcdhePskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

#ifndef __DISABLE_MOCANA_SHA256__
    /* 0xC03A TLS_ECDHE_PSK_WITH_NULL_SHA256*/ SSL_CIPHER_DEF(0xC03A, 1, 1, 0, &NilSHA256, &EcdhePskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif

    /* 0xC039 TLS_ECDHE_PSK_WITH_NULL_SHA*/ SSL_CIPHER_DEF(0xC039, 1, 1, 0, &NilSHA1, &EcdhePskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),

#endif /* __ENABLE_NIL_CIPHER__ */

#endif /* __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__ */


#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__

#ifndef __DISABLE_AES_CIPHERS__
#ifndef __DISABLE_AES256_CIPHER__
    /* 0x0095 TLS_RSA_PSK_WITH_AES_256_CBC_SHA*/  SSL_CIPHER_DEF( 0x0095, 1, 0, 32, &AESSHA1, &RsaPskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0x0094 TLS_RSA_PSK_WITH_AES_128_CBC_SHA*/  SSL_CIPHER_DEF( 0x0094, 1, 0, 16, &AESSHA1, &RsaPskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif /* __DISABLE_AES_CIPHERS__ */

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0x0093 TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA*/ SSL_CIPHER_DEF( 0x0093, 1, 0, 24, &TripleDESSHA1, &RsaPskSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#endif /* __ENABLE_MOCANA_SSL_RSA_SUPPORT__  */

#ifdef __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__

#ifndef __DISABLE_AES_CIPHERS__

#ifndef __DISABLE_AES256_CIPHER__
    /* 0x0091 TLS_DHE_PSK_WITH_AES_256_CBC_SHA*/  SSL_CIPHER_DEF( 0x0091, 1, 0, 32, &AESSHA1, &DiffieHellmanPskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#ifndef __DISABLE_AES128_CIPHER__
    /* 0x0090 TLS_DHE_PSK_WITH_AES_128_CBC_SHA*/  SSL_CIPHER_DEF( 0x0090, 1, 0, 16, &AESSHA1, &DiffieHellmanPskSuite, NULL, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT ),
#endif

#endif

#ifndef __DISABLE_3DES_CIPHERS__
    /* 0x008F SSL_DHE_PSK_WITH_3DES_EDE_CBC_SHA*/ SSL_CIPHER_DEF( 0x008F, 1, 0, 24, &TripleDESSHA1, &DiffieHellmanPskSuite, NULL, SINGLE_PASS_3DES_SHA1_IN, SINGLE_PASS_3DES_SHA1_OUT ),
#endif

#endif /* __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__ */


#ifdef __ENABLE_NIL_CIPHER__

#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00B1 TLS_PSK_WITH_NULL_SHA384 */  SSL_CIPHER_DEF( 0x00B1, 1, 0, 0, &NilSHA384, &PskSuite, &SHA384Suite, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT),
#endif
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00B0 TLS_PSK_WITH_NULL_SHA256 */  SSL_CIPHER_DEF( 0x00B0, 1, 0, 0, &NilSHA256, &PskSuite, &SHA256Suite, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT),
#endif

#endif  /* __ENABLE_NIL_CIPHER__ */

#ifdef __ENABLE_MOCANA_SSL_DHE_SUPPORT__

#ifndef __DISABLE_AES_CIPHERS__

#ifndef __DISABLE_MOCANA_SHA384__
#ifndef __DISABLE_AES256_CIPHER__
    /* 0x00B3 TLS_DHE_PSK_WITH_AES_256_CBC_SHA384 */ SSL_CIPHER_DEF( 0x00B3, 1, 0, 32, &AESSHA384, &DiffieHellmanPskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00B2 TLS_DHE_PSK_WITH_AES_128_CBC_SHA256 */ SSL_CIPHER_DEF( 0x00B2, 1, 0, 16, &AESSHA256, &DiffieHellmanPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#endif /* __DISABLE_AES_CIPHERS__ */

#ifdef __ENABLE_NIL_CIPHER__

#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00B5 TLS_DHE_PSK_WITH_NULL_SHA384 */  SSL_CIPHER_DEF( 0x00B5, 1, 0, 0, &NilSHA384, &DiffieHellmanPskSuite, &SHA384Suite, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT),
#endif
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00B4 TLS_DHE_PSK_WITH_NULL_SHA256 */  SSL_CIPHER_DEF( 0x00B4, 1, 0, 0, &NilSHA256, &DiffieHellmanPskSuite, &SHA256Suite, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT),
#endif

#endif /* __ENABLE_NIL_CIPHER__ */

#endif /* __ENABLE_MOCANA_SSL_DHE_SUPPORT__ */

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__

#ifndef __DISABLE_AES_CIPHERS__

#ifndef __DISABLE_AES256_CIPHER__
#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00B7 TLS_RSA_PSK_WITH_AES_256_CBC_SHA384 */ SSL_CIPHER_DEF( 0x00B7, 1, 0, 32, &AESSHA384, &RsaPskSuite, &SHA384Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#ifndef __DISABLE_AES128_CIPHER__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00B6 TLS_RSA_PSK_WITH_AES_128_CBC_SHA256 */  SSL_CIPHER_DEF( 0x00B6, 1, 0, 16, &AESSHA256, &RsaPskSuite, &SHA256Suite, SINGLE_PASS_AES_SHA1_IN, SINGLE_PASS_AES_SHA1_OUT),
#endif
#endif

#endif  /* __DISABLE_AES_CIPHERS__ */

#ifdef __ENABLE_NIL_CIPHER__

#ifndef __DISABLE_MOCANA_SHA384__
    /* 0x00B9 TLS_RSA_PSK_WITH_NULL_SHA384 */ SSL_CIPHER_DEF( 0x00B9, 1, 0, 0, &NilSHA384, &RsaPskSuite, &SHA384Suite, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT),
#endif
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x00B8 TLS_RSA_PSK_WITH_NULL_SHA256 */ SSL_CIPHER_DEF( 0x00B8, 1, 0, 0, &NilSHA256, &RsaPskSuite, &SHA256Suite, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT),
#endif

#endif /* __ENABLE_NIL_CIPHER__ */

#endif /* __ENABLE_MOCANA_SSL_RSA_SUPPORT__ */

#endif /* __ENABLE_MOCANA_SSL_PSK_SUPPORT__ */

/***** DES cipher */
#ifdef __ENABLE_DES_CIPHER__

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__))
    /* 0x0015 SSL_DHE_RSA_WITH_DES_CBC_SHA*/   SSL_CIPHER_DEF( 0x0015, 1, 0, 8, &DESSHA1, &DiffieHellmanRsaSuite, NULL, SINGLE_PASS_DES_SHA1_IN, SINGLE_PASS_DES_SHA1_OUT ),
#endif

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
    /*  0x0009 SSL_RSA_WITH_DES_CBC_SHA*/      SSL_CIPHER_DEF( 0x0009, 1, 0, 8, &DESSHA1, &RsaSuite, NULL, SINGLE_PASS_DES_SHA1_IN, SINGLE_PASS_DES_SHA1_OUT ),
#endif

#ifdef __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__
    /* 0x001A SSL_DH_ANON_WITH_DES_CBC_SHA*/   SSL_CIPHER_DEF( 0x001A, 1, 0, 8, &DESSHA1, &DiffieHellmanAnonSuite, NULL, SINGLE_PASS_DES_SHA1_IN, SINGLE_PASS_DES_SHA1_OUT ),
#endif

#endif


/***** no symmetric crypto */
#ifdef __ENABLE_NIL_CIPHER__

#ifdef __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
    /* 0xC006 TLS_ECDHE_ECDSA_WITH_NULL_SHA */  SSL_CIPHER_DEF( 0xC006, 1, 0, 0, &NilSHA1, &EcdheEcdsaSuite, NULL, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT ),
#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
    /* 0xC010 TLS_ECDHE_RSA_WITH_NULL_SHA */    SSL_CIPHER_DEF( 0xC010, 1, 0, 0, &NilSHA1, &EcdheRsaSuite, NULL, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT ),
#endif
#endif

#ifdef __ENABLE_MOCANA_SSL_ECDH_SUPPORT__
    /* 0xC001 TLS_ECDH_ECDSA_WITH_NULL_SHA */  SSL_CIPHER_DEF( 0xC001, 1, 1, 0, &NilSHA1, &EcdhEcdsaSuite, NULL, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT ),
#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
    /* 0xC00B TLS_ECDH_RSA_WITH_NULL_SHA */    SSL_CIPHER_DEF( 0xC00B, 1, 1, 0, &NilSHA1, &EcdhRsaSuite, NULL, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT ),
#endif
#endif

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
#ifndef __DISABLE_MOCANA_SHA256__
    /* 0x003B SSL_RSA_WITH_NULL_SHA256 */      SSL_CIPHER_DEF( 0x003B, 1, 3, 0, &NilSHA256, &RsaSuite, &SHA256Suite, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT ),
#endif
    /* 0x0002 SSL_RSA_WITH_NULL_SHA*/          SSL_CIPHER_DEF( 0x0002, 1, 0, 0, &NilSHA1, &RsaSuite, NULL, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT ),
#endif /* __ENABLE_MOCANA_SSL_RSA_SUPPORT__ */


#ifdef __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__
    /* 0xC015 TLS_ECDH_anon_WITH_NULL_SHA */    SSL_CIPHER_DEF( 0xC015, 1, 1, 0, &NilSHA1, &EcdheAnonSuite, NULL, SINGLE_PASS_NULL_SHA1_IN, SINGLE_PASS_NULL_SHA1_OUT ),
#endif

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
#ifndef __DISABLE_NULL_MD5_CIPHER__
    /*  0x0001 SSL_RSA_WITH_NULL_MD5*/          SSL_CIPHER_DEF( 0x0001, 1, 0, 0, &NilMD5, &RsaSuite, NULL, SINGLE_PASS_NULL_MD5_IN, SINGLE_PASS_NULL_MD5_OUT ),
#endif
#endif

#endif /* __ENABLE_NIL_CIPHER___ */
};

#define NUM_CIPHER_SUITES    (COUNTOF(gCipherSuites))

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
static const ubyte2 gCipherIdDTLSExclusion[] =
{
    0xC002, 0xC00C, 0xC007, 0xC011, 0x0004, 0x0005, 0x0018, 0xC016,
    0x008a, 0x0092, 0x008e
};

#define NUM_CIPHERID_DTLS_EXCLUSION (COUNTOF(gCipherIdDTLSExclusion))
#endif

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
/* table with infos about the supported Elliptic curves
*   THIS IS IN ORDER OF PREFERENCE
*/
static ubyte2 gSupportedEllipticCurves[] = {
#ifndef __DISABLE_MOCANA_ECC_P256__
    tlsExtNamedCurves_secp256r1,
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
    tlsExtNamedCurves_secp384r1,
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
    tlsExtNamedCurves_secp521r1,
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
    tlsExtNamedCurves_secp224r1,
#endif
#ifdef __ENABLE_MOCANA_ECC_P192__
    tlsExtNamedCurves_secp192r1,
#endif
};

#define NUM_SSL_SUPPORTED_ELLIPTIC_CURVES (COUNTOF( gSupportedEllipticCurves))

#ifndef __ENABLE_MOCANA_ECC_P192__
#define EC_P192_FLAG    0
#else
#define EC_P192_FLAG    (1 << tlsExtNamedCurves_secp192r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P224__
#define EC_P224_FLAG    0
#else
#define EC_P224_FLAG    (1 << tlsExtNamedCurves_secp224r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P256__
#define EC_P256_FLAG    0
#else
#define EC_P256_FLAG    (1 << tlsExtNamedCurves_secp256r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P384__
#define EC_P384_FLAG    0
#else
#define EC_P384_FLAG    (1 << tlsExtNamedCurves_secp384r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P521__
#define EC_P521_FLAG    0
#else
#define EC_P521_FLAG    (1 << tlsExtNamedCurves_secp521r1)
#endif

#ifndef SUPPORTED_CURVES_FLAGS
#define SUPPORTED_CURVES_FLAGS (EC_P192_FLAG | EC_P224_FLAG | EC_P256_FLAG | EC_P384_FLAG | EC_P521_FLAG)
#endif

#else
#define SUPPORTED_CURVES_FLAGS 0

#endif  /* defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) ||
            defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)||
            defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)*/


/* New for TLS1.2: signature algorithms definitions */
enum TLS_HashAlgorithm
{
    TLS_NONE        = 0,
    TLS_MD5         = 1,
    TLS_SHA1        = 2,
    TLS_SHA224      = 3,
    TLS_SHA256      = 4,
    TLS_SHA384      = 5,
    TLS_SHA512      = 6,
    TLS_HASH_MAX    = 255
};

enum TLS_SignatureAlgorithm
{
    TLS_ANONYMOUS       = 0,
    TLS_RSA             = 1,
    TLS_DSA             = 2,
    TLS_ECDSA           = 3,
    TLS_SIGNATURE_MAX   = 255
};

static ubyte2 gSupportedSignatureAlgorithms[] =
{
#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__))

#ifndef __DISABLE_MOCANA_SHA512__
    TLS_SHA512 << 8 |TLS_ECDSA,
#endif
#ifndef __DISABLE_MOCANA_SHA384__
    TLS_SHA384 << 8 |TLS_ECDSA,
#endif
#ifndef __DISABLE_MOCANA_SHA256__
    TLS_SHA256 << 8 |TLS_ECDSA,
#endif
#ifndef __DISABLE_MOCANA_SHA224__
    TLS_SHA224 << 8 |TLS_ECDSA,
#endif
    TLS_SHA1   << 8 |TLS_ECDSA,

#endif

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
#ifndef __DISABLE_MOCANA_SHA512__
    TLS_SHA512 << 8 |TLS_RSA,
#endif
#ifndef __DISABLE_MOCANA_SHA384__
    TLS_SHA384 << 8 |TLS_RSA,
#endif
#ifndef __DISABLE_MOCANA_SHA256__
    TLS_SHA256 << 8 |TLS_RSA,
#endif
#ifndef __DISABLE_MOCANA_SHA224__
    TLS_SHA224 << 8 |TLS_RSA,
#endif
    TLS_SHA1   << 8 |TLS_RSA,
    TLS_MD5    << 8 |TLS_RSA,
#endif

/* no support for DSA yet
#ifdef __ENABLE_MOCANA_DSA__
#define SHA1_DSA_FLAG      (1 << (SHA1 + 8) | DSA)
#else
#define SHA1_DSA_FLAG      0
#endif
*/
};

#define NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS (COUNTOF( gSupportedSignatureAlgorithms))

typedef struct hashSuite
{
    ubyte hashType;
    const ubyte* oid;
    ubyte4 ctxSize;
    const BulkHashAlgo *algo;
} hashSuite;


static hashSuite gSupportedHashAlgorithms[] =
{
#ifndef __DISABLE_MOCANA_SHA512__
    {TLS_SHA512, sha512_OID, sizeof(SHA512_CTX), &SHA512Suite},
#endif
#ifndef __DISABLE_MOCANA_SHA384__
    {TLS_SHA384, sha384_OID, sizeof(SHA384_CTX), &SHA384Suite},
#endif
#ifndef __DISABLE_MOCANA_SHA256__
    {TLS_SHA256, sha256_OID, sizeof(SHA256_CTX), &SHA256Suite},
#endif
#ifndef __DISABLE_MOCANA_SHA224__
    {TLS_SHA224, sha224_OID, sizeof(SHA224_CTX), &SHA224Suite},
#endif
    {TLS_SHA1, sha1_OID, sizeof(SHA1_CTX), &SHA1Suite},
    {TLS_MD5, md5_OID, sizeof(MD5_CTX), &MD5Suite}
};

#define NUM_SSL_SUPPORTED_HASH_ALGORITHMS (COUNTOF( gSupportedHashAlgorithms))

#ifdef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
#ifdef __DISABLE_MOCANA_SHA512__
#define TLS_SHA521_RSA_FLAG    0
#else
#define TLS_SHA521_RSA_FLAG    (1 << (TLS_SHA512 + 8) | TLS_RSA)
#endif
#ifdef __DISABLE_MOCANA_SHA384__
#define TLS_SHA384_RSA_FLAG    0
#else
#define TLS_SHA384_RSA_FLAG    (1 << (TLS_SHA384 + 8) | TLS_RSA)
#endif
#ifdef __DISABLE_MOCANA_SHA256__
#define TLS_SHA256_RSA_FLAG    0
#else
#define TLS_SHA256_RSA_FLAG    (1 << (TLS_SHA256 + 8) | TLS_RSA)
#endif
#ifdef __DISABLE_MOCANA_SHA224__
#define TLS_SHA224_RSA_FLAG    0
#else
#define TLS_SHA224_RSA_FLAG    (1 << (TLS_SHA224 + 8) | TLS_RSA)
#endif
#define TLS_SHA1_RSA_FLAG      (1 << (TLS_SHA1 + 8) | TLS_RSA)
#define TLS_MD5_RSA_FLAG       (1 << (TLS_MD5 + 8) | TLS_RSA)
#endif

/* no support for DSA yet
#ifdef __ENABLE_MOCANA_DSA__
#define SHA1_DSA_FLAG      (1 << (SHA1 + 8) | DSA)
#else
#define SHA1_DSA_FLAG      0
#endif
*/

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__))
#define TLS_SHA1_ECDSA_FLAG    (1 << (TLS_SHA1 + 8) |TLS_ECDSA) << (TLS_ECDSA-1)*8
#else
#define TLS_SHA1_ECDSA_FLAG    0 << (TLS_ECDSA-1)*8
#endif

#ifndef SUPPORTED_SIGNATURE_ALGORITHM_FLAGS
#define SUPPORTED_SIGNATURE_ALGORITHM_FLAGS (TLS_SHA521_RSA_FLAG | TLS_SHA384_RSA_FLAG | TLS_SHA256_RSA_FLAG | TLS_SHA224_RSA_FLAG | TLS_SHA1_RSA_FLAG | TLS_MD5_RSA_FLAG | TLS_SHA1_ECDSA_FLAG)
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#define DTLS_RECORD_SIZE(X)          ((((ubyte2)X[11]) << 8) | ((ubyte2)X[12]))
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ALERTS__
/* SSL Alerts */

typedef struct
{
    sbyte4  sslAlertId;         /* the alert */
    sbyte4  sslAlertClass;      /* warning or error */
    sbyte4  sslProtocol;        /* SSL, TLS, combos, etc */
    MSTATUS mocErrorCode;       /* map error codes to alerts */

} sslAlertsInfo;

#define ALERT_SSL           (0x01)
#define ALERT_TLS           (0x02)
#define ALERT_SSL_TLS       (ALERT_SSL | ALERT_TLS)

static sslAlertsInfo mAlertsSSL[] = {
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL,     ERR_SSL_INVALID_PRESECRET },
    { SSL_ALERT_NO_CERTIFICATE,          SSLALERTLEVEL_FATAL,   ALERT_SSL,     ERR_SSL_PROTOCOL_PROCESS_CERTIFICATE },
    { SSL_ALERT_RECORD_OVERFLOW,         SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_BAD_RECORD_SIZE },
    { SSL_ALERT_UNKNOWN_CA,              SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_UNKNOWN_CERTIFICATE_AUTHORITY },
    { SSL_ALERT_ACCESS_DENIED,           SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_MUTUAL_AUTHENTICATION_REQUEST_IGNORED },
    { SSL_ALERT_ACCESS_DENIED,           SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_MUTUAL_AUTHENTICATION_FAILED },
    { SSL_ALERT_DECODE_ERROR,            SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_PROTOCOL },
    { SSL_ALERT_PROTOCOL_VERSION,        SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_PROTOCOL_VERSION },
    { SSL_ALERT_INTERNAL_ERROR,          SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_MEM_ALLOC_FAIL },
    { SSL_ALERT_INTERNAL_ERROR,          SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_CONFIG },
    { SSL_ALERT_INAPPROPRIATE_FALLBACK,  SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_SERVER_INAPPROPRIATE_FALLBACK_SCSV },
    { SSL_ALERT_UNSUPPORTED_EXTENSION,   SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_EXTENSION_UNSOLICITED_OFFER },
    { SSL_ALERT_UNRECOGNIZED_NAME,       SSLALERTLEVEL_FATAL,   ALERT_TLS,     ERR_SSL_EXTENSION_UNRECOGNIZED_NAME },
    { SSL_ALERT_BAD_CERTIFICATE_STATUS_RESPONSE,
                                         SSLALERTLEVEL_FATAL, ALERT_TLS,       ERR_SSL_EXTENSION_CERTIFICATE_STATUS_RESPONSE },
    { SSL_ALERT_NO_RENEGOTIATION,        SSLALERTLEVEL_WARNING, ALERT_TLS,     ERR_SSL_SERVER_RENEGOTIATE_NOT_ALLOWED },
    { SSL_ALERT_NO_RENEGOTIATION,        SSLALERTLEVEL_WARNING, ALERT_TLS,     ERR_SSL_CLIENT_RENEGOTIATE_NOT_ALLOWED },
    { SSL_ALERT_CLOSE_NOTIFY,            SSLALERTLEVEL_WARNING, ALERT_SSL_TLS, OK },
    { SSL_ALERT_BAD_RECORD_MAC,          SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_CRYPT_BLOCK_SIZE },
    { SSL_ALERT_BAD_RECORD_MAC,          SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_INVALID_PADDING },
    { SSL_ALERT_BAD_RECORD_MAC,          SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_CRYPT_BLOCK_SIZE },
    { SSL_ALERT_UNEXPECTED_MESSAGE,      SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_PROTOCOL_BAD_STATE },
    { SSL_ALERT_BAD_RECORD_MAC,          SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_INVALID_MAC },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_NO_CIPHER_MATCH },
    { SSL_ALERT_BAD_CERTIFICATE,         SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_CERT_VALIDATION_FAILED },
    { SSL_ALERT_UNSUPPORTED_CERTIFICATE, SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_NO_SELF_SIGNED_CERTIFICATES },
    { SSL_ALERT_CERTIFICATE_REVOKED,     SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_CERT_REVOKED },
    { SSL_ALERT_CERTIFICATE_EXPIRED,     SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_CERT_EXPIRED },
    { SSL_ALERT_CERTIFICATE_UNKNOWN,     SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_CERT_INVALID_STRUCT },
    { SSL_ALERT_ILLEGAL_PARAMETER,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_PROTOCOL },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_SERVER_RENEGOTIATE_LENGTH },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_SERVER_RENEGOTIATE_CLIENT_VERIFY },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_SERVER_RENEGOTIATE_ILLEGAL_SCSV },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_SERVER_RENEGOTIATE_ILLEGAL_EXTENSION },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_CLIENT_RENEGOTIATE_LENGTH },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_CLIENT_RENEGOTIATE_CLIENT_VERIFY },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_CLIENT_RENEGOTIATE_SERVER_VERIFY },
    { SSL_ALERT_HANDSHAKE_FAILURE,       SSLALERTLEVEL_FATAL,   ALERT_SSL_TLS, ERR_SSL_CLIENT_RENEGOTIATE_ILLEGAL_EXTENSION }
};

#define NUM_ALERTS  ((sizeof(mAlertsSSL)/sizeof(mAlertsSSL[0])) - 1)

/* End SSL Alerts */
#endif

/* DTLS_SRTP_BUSINESS */
#if defined (__ENABLE_MOCANA_DTLS_SRTP__)
#if defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__)

#define DTLS_SRTP_EXTRACTOR         "EXTRACTOR-dtls_srtp"
#define DTLS_SRTP_EXTRACTOR_SIZE    (19)

#define SRTP_PROFILE_DEF(CIPHER_ID, PROFILE_ENABLE, KEY_SIZE, SALT_SIZE) { CIPHER_ID, PROFILE_ENABLE, KEY_SIZE, SALT_SIZE }

/* table with infos about the SRTP profiles
 *   THIS IS IN ORDER OF PREFERENCE
 */

/* __ENABLE_MOCANA_ITERATE_DTLS_SRTP_PROFILES__ is used to expose globals for unit testing */
#ifndef __ENABLE_MOCANA_ITERATE_DTLS_SRTP_PROFILES__
static
#endif
const SrtpProfileInfo gSrtpProfiles[] = {

    /*   ################# remember to update dtls_srtp.h ################
    #define SRTP_MAX_KEY_SIZE           (32)
    #define SRTP_MAX_SALT_SIZE          (14)
    if necessary ########################################################## */

#ifdef __ENABLE_MOCANA_GCM__

#ifndef __DISABLE_AES128_CIPHER__
    SRTP_PROFILE_DEF( E_SRTP_AES_128_GCM_8,  1,  16, 12 ),
    SRTP_PROFILE_DEF( E_SRTP_AES_128_GCM_12, 1,  16, 12 ),
#endif
#ifndef __DISABLE_AES256_CIPHER__
    SRTP_PROFILE_DEF( E_SRTP_AES_256_GCM_8,  1,  32, 12 ),
    SRTP_PROFILE_DEF( E_SRTP_AES_256_GCM_12, 1,  32, 12 ),
#endif

#endif

#ifndef __DISABLE_AES_CIPHERS__
#ifndef __DISABLE_AES128_CIPHER__
    SRTP_PROFILE_DEF( E_SRTP_AES_128_CM_SHA1_80, 1,  16, 14 ),
    SRTP_PROFILE_DEF( E_SRTP_AES_128_CM_SHA1_32, 1,  16, 14 ),
#endif
#endif /* __DISABLE_AES_CIPHERS__ */
    SRTP_PROFILE_DEF( E_SRTP_NULL_SHA1_80, 1,  0, 0 ),
    SRTP_PROFILE_DEF( E_SRTP_NULL_SHA1_32, 1,  0, 0 )
};

#define NUM_SRTP_PROFILES    (COUNTOF(gSrtpProfiles))

#ifdef __ENABLE_MOCANA_ITERATE_DTLS_SRTP_PROFILES__
const ubyte4 gNumSrtpProfiles = NUM_SRTP_PROFILES;
#endif



#endif /* defined (__ENABLE_MOCANA_DTLS_SRTP__) */
#endif /* defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__) */

/*------------------------------------------------------------------*/

/* convert ecCurveFlags, and signatureAlgoList into CertStore algorithm flag */
#if defined(__ENABLE_SSL_DYNAMIC_CERTIFICATE__) || (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
static MSTATUS
convertToCertStoreFlags(SSLSocket *pSSLSock, ubyte4 ecCurveFlags,
                        ubyte* signatureAlgoList, ubyte4 signatureAlgoListLength,
                        ubyte4 *pCertStoreAlgoFlags)
{
    ubyte4 certStoreAlgoFlags = 0;
    ubyte4 i;

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
    if (ecCurveFlags)
    {
        if (ecCurveFlags & EC_P192_FLAG)
        {
            certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_EC192;
        }

        if (ecCurveFlags & EC_P224_FLAG)
        {
            certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_EC224;
        }

        if (ecCurveFlags & EC_P256_FLAG)
        {
            certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_EC256;
        }

        if (ecCurveFlags & EC_P384_FLAG)
        {
            certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_EC384;
        }

        if (ecCurveFlags & EC_P521_FLAG)
        {
            certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_EC521;
        }
    } else
    {
        certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_ECCURVES;
    }
#else
    {
        certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_ECCURVES;
    }
#endif

    if (0 < signatureAlgoListLength)
    {
        for (i = 0; i < signatureAlgoListLength; i+=2 )
        {
            ubyte2 sigAlgo = signatureAlgoList[i] << 8 | signatureAlgoList[i+1];
            /* hash algo */
            switch (sigAlgo >>8)
            {
            case TLS_MD5:
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_MD5;
                break;
            case TLS_SHA1:
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_SHA1;
                break;
            case TLS_SHA224:
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_SHA224;
                break;
            case TLS_SHA256:
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_SHA256;
                break;
            case TLS_SHA384:
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_SHA384;
                break;
            case TLS_SHA512:
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_SHA512;
                break;
            default:
                /* unknown algo, ignore */
                break;
            }
            /* sign algo */
            if (pSSLSock->server &&
                (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDHE_BIT))
            {
                if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ECDSA_BIT)
                {
                    certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_ECDSA;
                } else
                {
                    certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_RSA;
                }
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_HASHALGO;

            } else
            {
                switch (sigAlgo & 0xff)
                {
                case TLS_RSA:
                    certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_RSA;
                    break;
                case TLS_ECDSA:
                    certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_ECDSA;
                    break;
                case TLS_DSA:
                    certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_DSA;
                    break;
                default:
                    /* unknown algo, ignore */
                    break;
                }
            }
        }
    }
    else
    {
        if (pSSLSock->server &&
            (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDHE_BIT))
        {
            if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ECDSA_BIT)
            {
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_ECDSA;
            }
            else
            {
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_RSA;
            }
            certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_HASHALGO;

        }
        else
        {
            if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
                (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION) )
            {
                /* change in tls1.2 only default are allowed */
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_DSA | CERT_STORE_ALGO_FLAG_RSA | CERT_STORE_ALGO_FLAG_ECDSA;
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_SHA1 | CERT_STORE_ALGO_FLAG_MD5;
            }
            else
            {
                /* all hash and sign algos are good if they are supported */
                /* Backward compatibility:
                * when <= TLS11MINORVERSION, if ECDH_RSA, cert has to be signed by RSA;
                * if ECDH_ECDSA, cert has to be signed by ECDSA */
                if (pSSLSock->server &&
                    (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDH_BIT))
                {
                    if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ECDSA_BIT)
                    {
                        certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_ECDSA;
                    }
                    else
                    {
                        certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_RSA;
                    }
                }
                else
                {
                    certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_SIGNKEYTYPE;
                }
                certStoreAlgoFlags |= CERT_STORE_ALGO_FLAG_HASHALGO;
            }
        }
    }

    *pCertStoreAlgoFlags = certStoreAlgoFlags;
    return OK;
}
#endif /* (__ENABLE_SSL_DYNAMIC_CERTIFICATE__) || ((__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && (__ENABLE_MOCANA_SSL_CLIENT__)) */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
static ubyte4
SSL_SOCK_ECCKeyToECFlag( const ECCKey* pKey)
{
#ifdef __ENABLE_MOCANA_ECC_P192__
    if (EC_P192 == pKey->pCurve)
    {
        return (1 << tlsExtNamedCurves_secp192r1);
    }
    else
#endif

#ifndef __DISABLE_MOCANA_ECC_P224__
    if (EC_P224 == pKey->pCurve)
    {
        return (1 << tlsExtNamedCurves_secp224r1);
    } else
#endif

#ifndef __DISABLE_MOCANA_ECC_P256__
    if (EC_P256 == pKey->pCurve)
    {
        return (1 << tlsExtNamedCurves_secp256r1);
    } else
#endif

#ifndef __DISABLE_MOCANA_ECC_P384__
    if (EC_P384 == pKey->pCurve)
    {
       return (1 << tlsExtNamedCurves_secp384r1);
    } else
#endif

#ifndef __DISABLE_MOCANA_ECC_P521__
    if (EC_P521== pKey->pCurve)
    {
       return (1 << tlsExtNamedCurves_secp521r1);
    } else
#endif
    {
        return 0; /* no match */
    }
}

#endif /*   __ENABLE_MOCANA_SSL_ECDH_SUPPORT__
            __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
            __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT_
       */
#endif /* __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__ */
#endif /* __ENABLE_MOCANA_SSL_CLIENT__ */

/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
extern MSTATUS
getCertSigAlgo(ubyte* pCertificate, ubyte4 certLen, ubyte4* pRetCertSigAlgo)
{
    MSTATUS         status;
    ASN1_ITEM*      pCert = NULL;
    ASN1_ITEM*      pSignAlgoId = NULL;
    MemFile         certMemFile;
    CStream         cs;
    ubyte4          hashType = 0;
    ubyte4          pubKeyType = 0;

    static WalkerStep signatureAlgoWalkInstructions[] =
    {
        { GoFirstChild, 0, 0},
        { GoNthChild, 2, 0},
        { VerifyType, SEQUENCE, 0 },
        { Complete, 0, 0}
    };

    if (NULL == pCertificate)
        return ERR_NULL_POINTER;

    if (0 == certLen)
    {
        status = ERR_CERT_AUTH_BAD_CERT_LENGTH;
        goto exit;
    }

    *pRetCertSigAlgo = 0;

    MF_attach(&certMemFile, certLen, (ubyte *)pCertificate);
    CS_AttachMemFile(&cs, &certMemFile);

    if (OK > (status = ASN1_Parse( cs, &pCert)))
        goto exit;

    if ( OK > ASN1_WalkTree( pCert, cs, signatureAlgoWalkInstructions, &pSignAlgoId))
    {
        return ERR_CERT_INVALID_STRUCT;
    }

   status = CERT_getCertSignAlgoType( pSignAlgoId, cs, &hashType, &pubKeyType);
    if (OK > status)
    {
        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
        goto exit;
    }

    switch (hashType)
    {
    case ht_md5:
        hashType = TLS_MD5;
        break;

    case ht_sha1:
        hashType = TLS_SHA1;
        break;

#ifndef __DISABLE_MOCANA_SHA224__
    case ht_sha224:
        hashType = TLS_SHA224;
        break;
#endif

#ifndef __DISABLE_MOCANA_SHA256__
    case ht_sha256:
        hashType = TLS_SHA256;
        break;
#endif

#ifndef __DISABLE_MOCANA_SHA384__
    case ht_sha384:
        hashType = TLS_SHA384;
        break;
#endif

#ifndef __DISABLE_MOCANA_SHA512__
    case ht_sha512:
        hashType = TLS_SHA512;
        break;
#endif
    default:
        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
        goto exit;
    }

    switch (pubKeyType)
    {
    case akt_rsa:
        pubKeyType = TLS_RSA;
        break;

#if (defined(__ENABLE_MOCANA_ECC__))
    case akt_ecc:
        pubKeyType = TLS_ECDSA;
        break;
#endif

    default:
        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
        goto exit;
    }

    *pRetCertSigAlgo = hashType << 8 | pubKeyType;

exit:
    if (pCert)
        TREE_DeleteTreeItem((TreeItem*)pCert);

    return status;
}
#endif /* !__DISABLE_MOCANA_CERTIFICATE_PARSING__ */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)))
static MSTATUS
calculateTLS12KeyExchangeRSASignature(SSLSocket* pSSLSock,
                                    ubyte* pData, ubyte4 dataLen,
                                    ubyte2 signatureAlgo, ubyte* pHashResult, ubyte4 *pHashLen)
{
    MSTATUS     status = OK;
    BulkCtx     pHashCtx = NULL;
    hashSuite  *pHashSuite = NULL;
    ubyte4      offset;
    ubyte4      hashLen;
    ubyte4      hashResultLen;
    ubyte4      i;

    /* verify we are indeed using the 1.2 versions */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        status = ERR_SSL;
        goto exit;
    }

    /* for tls1.2 and above, the struct to sign is
    DigestInfo ::= SEQUENCE {
    digestAlgorithm AlgorithmIdentifier,
    digest OCTET STRING
    }
    0x30 XX (length=YY+1 (tag) +1 (len) + digestLen+1 (tag) +1(len)) 0x30 YY (len=OID_len+2+2 (NULL)) 06 (OID tag) OID 05 00 (NULL) 04 ZZ (digestLen) digest
    */
    for (i = 0; i < NUM_SSL_SUPPORTED_HASH_ALGORITHMS; i++)
    {
        if (gSupportedHashAlgorithms[i].hashType == ((signatureAlgo >> 8) & 0xff))
        {
            pHashSuite = &gSupportedHashAlgorithms[i];
            break;
        }
    }

    if (!pHashSuite)
    {
        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
        goto exit;
    }

    offset = 0;
    hashResultLen = 2+2+2+pHashSuite->oid[0]+2+2+pHashSuite->algo->digestSize;
    MOC_MEMSET(pHashResult, 0x00, hashResultLen);
    pHashResult[offset++] = 0x30; /* SEQUENCE */
    pHashResult[offset++] = (ubyte) (hashResultLen - 2);
    pHashResult[offset++] = 0x30; /* SEQUENCE */
    pHashResult[offset++] = 2 + pHashSuite->oid[0] + 2;
    pHashResult[offset++] = 0x06; /* OID */
    pHashResult[offset++] = pHashSuite->oid[0];
    MOC_MEMCPY(pHashResult+offset, pHashSuite->oid+1, pHashSuite->oid[0]); /* oid */
    offset += pHashSuite->oid[0];
    pHashResult[offset++] = 0x05; /* NULL */
    pHashResult[offset++] = 0x00; /* NULL */
    pHashResult[offset++] = 0x04; /* OCTETSTRING */
    pHashResult[offset++] = (ubyte) (pHashSuite->algo->digestSize);

    hashLen = pHashSuite->algo->digestSize;

    if (OK > (status = pHashSuite->algo->allocFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx )))
        goto exit;

    /* compute the hash of the data */
    if (OK > (status = pHashSuite->algo->initFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx)))
        goto exit;

    if (OK > (status = pHashSuite->algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
        goto exit;

    if (OK > (status = pHashSuite->algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
        goto exit;

    if (OK > (status = pHashSuite->algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pData, dataLen)))
        goto exit;

    if (OK > (status = pHashSuite->algo->finalFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pHashResult+offset)))
        goto exit;
    *pHashLen = offset + hashLen;
exit:
    if (pHashCtx)
        status = pHashSuite->algo->freeFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx);
    return status;
}
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
static MSTATUS
calculateTLS12CertificateVerifyHash(ubyte4 signAlgo, SSLSocket *pSSLSock,
                                    ubyte* pHashResult, ubyte4 *pLen,
                                    const ubyte** ppHashOID)
{
    MSTATUS status = OK;
    ubyte4 i;
    const hashSuite *pHashSuite = NULL;
    BulkCtx              pHashCtx  = NULL;
    const BulkHashAlgo  *pHashAlgo = NULL;

    for (i = 0; i < NUM_SSL_SUPPORTED_HASH_ALGORITHMS; i++)
    {
        if (gSupportedHashAlgorithms[i].hashType == ((signAlgo >> 8) & 0xff))
        {
            pHashSuite = &gSupportedHashAlgorithms[i];
            break;
        }
    }

    if (!pHashSuite)
    {
        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
        goto exit;
    }


    /* find the signatureAlgo in the supported list */
    for (i = 0; i < NUM_SSL_SUPPORTED_HASH_ALGORITHMS; i++)
    {
        if (gSupportedHashAlgorithms[i].hashType == ((signAlgo >> 8) & 0xff))
        {
            pHashCtx = pSSLSock->pHashCtxList[i];
            pHashAlgo = gSupportedHashAlgorithms[i].algo;
            break;
        }
    }

    /* put the signature into digestData area */
    if (signAlgo && pHashAlgo && pHashCtx)
    {
        pHashAlgo->finalFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pHashResult);
        *pLen = pHashSuite->algo->digestSize;
        *ppHashOID = pHashSuite->oid;
    }

    /* done: clean the list now */
    for (i = 0; i < NUM_SSL_SUPPORTED_HASH_ALGORITHMS; i++)
    {
        BulkCtx               pHashCtx  = pSSLSock->pHashCtxList[i];
        const BulkHashAlgo   *pHashAlgo = gSupportedHashAlgorithms[i].algo;

        if (pHashCtx)
            pHashAlgo->freeFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx);
    }

    FREE(pSSLSock->pHashCtxList);
    pSSLSock->pHashCtxList = NULL;

exit:
    return status;
}
#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */

/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
static intBoolean
isCipherIdExcludedForDTLS(ubyte2 cipherId)
{
    ubyte4 count;

    for (count = 0; count < NUM_CIPHERID_DTLS_EXCLUSION; count++)
    {
        if (cipherId == gCipherIdDTLSExclusion[count])
            return TRUE;
    }

    return FALSE;
}

#endif

/*------------------------------------------------------------------*/

#if defined(__ENABLE_ALL_DEBUGGING__)
static void
PrintBytes( ubyte* buffer, sbyte4 len)
{
    sbyte4 i;

    for ( i = 0; i < len; ++i)
    {
        DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, buffer[i]);
        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)" ");

        if ( i % 16 == 15)
        {
            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
        }
    }

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
}
#endif


/*------------------------------------------------------------------*/

/* static functions */
static void    setMediumValue(ubyte medium[SSL_MEDIUMSIZE], ubyte2 val);
static ubyte2  getMediumValue(const ubyte* medium);
static void    setShortValue(ubyte shortBuff[2], ubyte2 val);
static ubyte2  getShortValue(const ubyte* medium);

/* TCP helpers */
static MSTATUS recvAll(SSLSocket* pSSLSock, sbyte* pBuffer, sbyte4 toReceive, const enum sslAsyncStates curAsyncState, const enum sslAsyncStates nextAsyncState, ubyte **ppPacketPayload, ubyte4 *pPacketLength);


#if defined(__ENABLE_MOCANA_EAP_FAST__)
static MSTATUS T_PRF(MOC_HASH(SSLSocket *pSSLSock) ubyte* secret, sbyte4 secretLen, ubyte* labelSeed, sbyte4 labelSeedLen,
                    ubyte* result, sbyte4 resultLen);
static MSTATUS SSL_SOCK_generateEAPFASTMasterSecret(SSLSocket *pSSLSock);
#endif


/* Handshake */
extern MSTATUS SSL_INTERNAL_setConnectionState(sbyte4 connectionInstance, sbyte4 connectionState);
static MSTATUS SSLSOCK_doOpenUpcalls(SSLSocket* pSSLSock);

/* server specific */
#ifdef    __ENABLE_MOCANA_SSL_SERVER__
static MSTATUS processClientHello2(SSLSocket* pSSLSock);
static MSTATUS processClientHello3(SSLSocket* pSSLSock);

static ubyte*  fillServerHello(SSLSocket* pSSLSock, ubyte* pHSRec);
static ubyte*  fillCertificate(SSLSocket* pSSLSock, ubyte* pHSRec);

static MSTATUS SSL_SERVER_sendServerHello(SSLSocket* pSSLSock);

static MSTATUS handleServerHandshakeMessages(SSLSocket* pSSLSock);
#endif /* __ENABLE_MOCANA_SSL_SERVER__ */

/* client specific */
#ifdef    __ENABLE_MOCANA_SSL_CLIENT__
static MSTATUS SSL_CLIENT_sendClientHello(SSLSocket* pSSLSock);
static MSTATUS processServerHello(SSLSocket* pSSLSock, ubyte* pSHSH, ubyte2 recLen);
static MSTATUS handleClientHandshakeMessages(SSLSocket* pSSLSock);
#endif /* __ENABLE_MOCANA_SSL_CLIENT__ */

#if (defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)) || (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined (__ENABLE_MOCANA_ECC__))
static MSTATUS SSL_SOCK_getECCSignatureLength( ECCKey* pECCKey, sbyte4* signatureLen);
#endif

/* shared */
static MSTATUS sendChangeCipherSpec(SSLSocket* pSSLSock);
static MSTATUS sendFinished(SSLSocket* pSSLSock);

static MSTATUS processFinished(SSLSocket* pSSLSock, ubyte* pSHSH, ubyte2 recLen);
static MSTATUS handleAlertMessage(SSLSocket* pSSLSock);
static MSTATUS handleInnerAppMessage(SSLSocket* pSSLSock);
/* static MSTATUS SSL_SOCK_receiveRecord(SSLSocket* pSSLSock, SSLRecordHeader* pSRH, ubyte **ppPacketPayload, ubyte4 *pPacketLength); */
static MSTATUS SSL_SOCK_receiveV23Record(SSLSocket* pSSLSock, ubyte* pSRH, ubyte **ppPacketPayload, ubyte4 *pPacketLength);
static MSTATUS checkBuffer(SSLSocket* pSSLSock, sbyte4 requestedSize);

#if defined(__ENABLE_RFC3546__)
static MSTATUS processHelloExtensions(SSLSocket* pSSLSock, ubyte *pExtensions,
                                      sbyte4 extensionsLen);
#if defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_MOCANA_SSL_CLIENT__)
static MSTATUS resetTicket(SSLSocket* pSSLSock);
#endif /* defined(__ENABLE_MOCANA_SSL_CLIENT__) */

#endif /* defined(__ENABLE_RFC3546__) */

#if (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_CLIENT__))
static MSTATUS processCertificate(SSLSocket* pSSLSock, ubyte* pSHSH, ubyte2 recLen, intBoolean isCertRequired);
static MSTATUS validateFirstCertificate(ASN1_ITEM* rootItem,
                                        CStream s,
                                        SSLSocket* pSSLSock,
                                        intBoolean chkCommonName,
                                        errorArray* pStatusArray);
#endif


/*------------------------------------------------------------------*/

#ifdef    __ENABLE_MOCANA_SSL_CLIENT__
#include "../ssl/client/ssl_client.inc"
#endif /* __ENABLE_MOCANA_SSL_CLIENT__ */

#ifdef    __ENABLE_MOCANA_SSL_SERVER__
#include "../ssl/server/ssl_server.inc"
#endif /* __ENABLE_MOCANA_SSL_SERVER__ */

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#include "../dtls/dtlssock.inc"
#endif

/*------------------------------------------------------------------*/

static MSTATUS
checkBuffer(SSLSocket* pSSLSock, sbyte4 requestedSize)
{
    ubyte   temp[20]; /* enough for either SSL or DTLS record header */
    ubyte4  sizeofRecordHeader;
    MSTATUS status = OK;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
    } else
#endif
    {
        sizeofRecordHeader = sizeof(SSLRecordHeader);
    }

    if ((sbyte4)(SSL_MALLOC_BLOCK_SIZE + requestedSize + sizeofRecordHeader + SSL_MAXMACSECRETSIZE) <= pSSLSock->receiveBufferSize)
        goto exit;

    if (NULL != pSSLSock->pReceiveBufferBase)
    {
        /* copy out SSL record header, if we realloc */
        if (pSSLSock->pReceiveBuffer)
            MOC_MEMCPY(temp, pSSLSock->pReceiveBuffer - sizeofRecordHeader, sizeofRecordHeader);

        CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pSSLSock->pReceiveBufferBase);
    }

    pSSLSock->receiveBufferSize = requestedSize;

    status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, SSL_MALLOC_BLOCK_SIZE + requestedSize + sizeofRecordHeader + SSL_MAXMACSECRETSIZE, TRUE, (void **)&pSSLSock->pReceiveBufferBase);

    if (pSSLSock->pReceiveBufferBase)
    {
        pSSLSock->pReceiveBuffer = pSSLSock->pReceiveBufferBase + SSL_MALLOC_BLOCK_SIZE;
        pSSLSock->pSharedInBuffer = (ubyte *)(pSSLSock->pReceiveBuffer - sizeofRecordHeader);

        MOC_MEMCPY(pSSLSock->pReceiveBuffer - sizeofRecordHeader, temp, sizeofRecordHeader);
    }
    else
    {
        pSSLSock->receiveBufferSize = 0;
        pSSLSock->pReceiveBuffer = NULL;
        pSSLSock->pSharedInBuffer = NULL;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

static void
setMediumValue(ubyte medium[SSL_MEDIUMSIZE], ubyte2 val)
{
    medium[2] = (ubyte)(val & 0xFF);
    medium[1] = (ubyte)((val >> 8) & 0xFF);
    medium[0] = 0;
}


/*------------------------------------------------------------------*/

static ubyte2
getMediumValue(const ubyte* med)
{
    return  (ubyte2)(((ubyte2)med[1] << 8) | (med[2]));
}


/*------------------------------------------------------------------*/

static void
setShortValue(ubyte shortBuff[2], ubyte2 val)
{
    shortBuff[1] = (ubyte)(val & 0xFF);
    shortBuff[0] = (ubyte)((val >> 8) & 0xFF);
}


/*------------------------------------------------------------------*/

static ubyte2
getShortValue(const ubyte* med)
{
    return  (ubyte2)(((ubyte2)med[0] << 8) | (med[1]));
}

/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
extern MSTATUS
getNumBytesSent(ubyte *pOutputBuffer, ubyte4 maxLen, ubyte4 *pNumBytesSent)
{
    ubyte *ptr = pOutputBuffer;
    ubyte2 recordSize = 0;

    *pNumBytesSent = 0;

    while (*pNumBytesSent < maxLen)
    {
        recordSize = (ubyte2)DTLS_RECORD_SIZE(ptr);

        if (*pNumBytesSent + recordSize > maxLen)
            break;

        *pNumBytesSent += recordSize + 13; /* sizeof(SSLRecordHeader) == 13 */
        ptr = ptr + 13 + recordSize;
    }

    if (0 == *pNumBytesSent)
    {
        /*
         * Possible error conditions:
         * 1. application send buffer is too small to hold a record
         * 2. recordSize is invalid (this is less likely to happen)
         */
        return ERR_DTLS_SEND_BUFFER;
    }

    return OK;
}

extern MSTATUS cleanupOutputBuffer(SSLSocket *pSSLSock)
{
    if (NULL != pSSLSock->pOutputBufferBase)
        FREE(pSSLSock->pOutputBufferBase);

    pSSLSock->pOutputBufferBase = NULL;
    pSSLSock->pOutputBuffer     = NULL;

    return OK;
}
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ALERTS__
extern intBoolean
SSLSOCK_lookupAlert(SSLSocket* pSSLSock, sbyte4 lookupError, sbyte4 *pRetAlertId, sbyte4 *pAlertClass)
{
    sbyte4      index;
    intBoolean  isFound = FALSE;

    if ((NULL == pSSLSock) || (NULL == pRetAlertId) || (NULL == pAlertClass))
        goto exit;

    for (index = 0; NUM_ALERTS > index; index++)
    {
        if (lookupError == mAlertsSSL[index].mocErrorCode)
        {
            if (ALERT_SSL_TLS == mAlertsSSL[index].sslProtocol)
                break;

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
            if ((ALERT_SSL == mAlertsSSL[index].sslProtocol) && (SSL3_MINORVERSION == pSSLSock->sslMinorVersion))
                break;

            if ((ALERT_TLS == mAlertsSSL[index].sslProtocol) && (SSL3_MINORVERSION < pSSLSock->sslMinorVersion))
                break;
#else
            if ((ALERT_TLS == mAlertsSSL[index].sslProtocol))
                break;
#endif
        }
    }

    if (NUM_ALERTS > index)
    {
        *pRetAlertId = mAlertsSSL[index].sslAlertId;
        *pAlertClass = mAlertsSSL[index].sslAlertClass;
        isFound = TRUE;
    }

exit:
    return isFound;
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ALERTS__
extern MSTATUS
SSLSOCK_sendAlert(SSLSocket* pSSLSock, intBoolean encryptBool, sbyte4 alertId, sbyte4 alertClass)
{
    MSTATUS status = OK;

    if (NULL == pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        /* for DTLS, encrypt alert once changeCipherSpec is sent (pActiveOwnCipherSuite is set) */
        /* since epoch is incremented, records sent with the new epoch should be encrypted */
        encryptBool = TRUE;
    }
#endif

    if ((NULL != pSSLSock->pActiveOwnCipherSuite) && (encryptBool))
    {
        sbyte   alertMesg[2];

        alertMesg[0] = (sbyte)alertClass;
        alertMesg[1] = (sbyte)alertId;

        status = sendData(pSSLSock, SSL_ALERT, alertMesg, 2, TRUE);
    }
    else
    {
        sbyte   alertMesg[20]; /* adequate for both SSL and DTLS alertMesg */
        ubyte4  sizeofAlertMesg;
        ubyte4  sizeofRecordHeader;
        ubyte4  numBytesSent = 0;

        /* fill buffer */
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            sizeofRecordHeader = sizeof(DTLSRecordHeader);
            DTLS_SET_RECORD_HEADER_EXT(alertMesg,pSSLSock,SSL_ALERT,2);
        } else
#endif
        {
            sizeofRecordHeader = sizeof(SSLRecordHeader);
            SSL_SET_RECORD_HEADER(alertMesg, SSL_ALERT, pSSLSock->sslMinorVersion, 2);
        }

        sizeofAlertMesg = sizeofRecordHeader + 2;

        alertMesg[0 + sizeofRecordHeader] = (sbyte)alertClass;
        alertMesg[1 + sizeofRecordHeader] = (sbyte)alertId;

        if (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)
        {
            if (NULL != pSSLSock->pOutputBufferBase)
                FREE(pSSLSock->pOutputBufferBase);

            if (NULL == (pSSLSock->pOutputBufferBase = MALLOC(sizeofAlertMesg)))
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }

            pSSLSock->pOutputBuffer     = pSSLSock->pOutputBufferBase;
            MOC_MEMCPY(pSSLSock->pOutputBuffer, (ubyte *)alertMesg, sizeofAlertMesg);
            pSSLSock->outputBufferSize  = sizeofAlertMesg;
            pSSLSock->numBytesToSend    = sizeofAlertMesg;
            status = pSSLSock->numBytesToSend;

            goto exit;
        }
        else
        {
#ifndef __MOCANA_IPSTACK__
            status = TCP_WRITE(pSSLSock->tcpSock, alertMesg, sizeofAlertMesg, &numBytesSent);
#else
            status = MOC_TCP_WRITE(pSSLSock->tcpSock, alertMesg, sizeofAlertMesg, &numBytesSent);
#endif
        }
    }

exit:
    return status;
}
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_INNER_APP__)
extern MSTATUS
SSLSOCK_sendInnerApp(SSLSocket* pSSLSock, InnerAppType innerApp, ubyte* pMsg, ubyte4 msgLen, ubyte4 * retMsgLen, sbyte4 isClient)
{
    ubyte   *pAppMsg = NULL;
    ubyte4   appMsgLen;
    MSTATUS status = ERR_SSL_NO_CIPHERSUITE;

    if (NULL == pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (SSL_INNER_APPLICATION_DATA == innerApp)
    {
        if (NULL ==  pMsg || 0 == msgLen)
        {
            goto exit;
        }

        pAppMsg = MALLOC(msgLen + 4);

        if (NULL == pAppMsg)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        *pAppMsg = innerApp;

        pAppMsg[1] = (msgLen >> 16) & 0xFF ;
        pAppMsg[2] = (msgLen >> 8) & 0xFF ;
        pAppMsg[3] = (msgLen) & 0xFF ;
        MOC_MEMCPY(pAppMsg + 4,pMsg,msgLen);

        appMsgLen = msgLen + 4;
    }
    else if ((SSL_INNER_INTER_FINISHED == innerApp) ||
             (SSL_INNER_FINAL_FINISHED == innerApp))
    {
        pAppMsg = MALLOC(16);

        if (NULL == pAppMsg)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        *pAppMsg = innerApp;
        pAppMsg[1] = 0;
        pAppMsg[2] = 0;
        pAppMsg[3] = 12 ;

        /* Calculate 12 Bytes Verify Data  and Copy it to the pAppMsg*/
        if (  0 < isClient )
            status = PRF(pSSLSock, pSSLSock->innerSecret, SSL_MASTERSECRETSIZE,
                         (const ubyte*)SSL_INNER_APP_CLIENT_PHRASE,SSL_INNER_APP_CLIENT_PHRASE_LEN,
                         pAppMsg+4, 12);
        else
            status = PRF(pSSLSock, pSSLSock->innerSecret, SSL_MASTERSECRETSIZE,
                         (const ubyte*)SSL_INNER_APP_SERVER_PHRASE,SSL_INNER_APP_SERVER_PHRASE_LEN,
                         pAppMsg+4, 12);

        if (OK > status)
            goto exit;

            appMsgLen = 16;
    }
    else
    {
        status = ERR_SSL_INVALID_INNER_TYPE;
        goto exit;
    }

    if ((NULL != pSSLSock->pActiveOwnCipherSuite))
        status = sendData(pSSLSock, SSL_INNER_APPLICATION, (const sbyte*)pAppMsg, appMsgLen, TRUE);
    else
        status = ERR_SSL_NO_CIPHERSUITE;



exit:
    if (pAppMsg)
    {
        FREE(pAppMsg);
        pAppMsg = NULL;
    }
    if (0 < status) /* Return the Length of the Data to be Sent */
    {
        *retMsgLen = status;
        status = OK;
    }
    return status;
}
#endif /* defined(__ENABLE_MOCANA_INNER_APP__) */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_INNER_APP__)
extern MSTATUS
SSLSOCK_updateInnerAppSecret(SSLSocket* pSSLSock, ubyte* session_key, ubyte4 sessionKeyLen)
{
    ubyte               innerSecret[SSL_MASTERSECRETSIZE];
    ubyte               masterSecret[SSL_MASTERSECRETSIZE];
    sbyte4              i;
    ubyte*              random1;
    ubyte*              random2;
    MSTATUS status = OK;

    if (NULL == pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    MOC_MEMCPY(masterSecret, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);

    random1 = START_RANDOM( pSSLSock);
    random2 = random1 + SSL_RANDOMSIZE;
    for (i = 0; i < SSL_RANDOMSIZE; ++i)
    {
        ubyte swap = *random1;
        *random1++ = *random2;
        *random2++ = swap;
    }

    /* copy label "inner app permutation" in its special place */
    MOC_MEMCPY( START_RANDOM( pSSLSock) - SSL_INNER_APP_SECRET_PHRASE_LEN,
            SSL_INNER_APP_SECRET_PHRASE,
            SSL_INNER_APP_SECRET_PHRASE_LEN);

    /* generate innersecret with PRF */
    status = PRF(pSSLSock, pSSLSock->innerSecret, SSL_MASTERSECRETSIZE,
            START_RANDOM( pSSLSock) - SSL_INNER_APP_SECRET_PHRASE_LEN,
            SSL_INNER_APP_SECRET_PHRASE_LEN + 2 * SSL_RANDOMSIZE,
            innerSecret, SSL_MASTERSECRETSIZE);

    /* store master secret in its place after that */
    MOC_MEMCPY(pSSLSock->pSecretAndRand, masterSecret, SSL_MASTERSECRETSIZE);

    /* update inner secret  */
    if (OK == status)
        MOC_MEMCPY(pSSLSock->innerSecret, innerSecret, SSL_MASTERSECRETSIZE);

exit:
    return status;
}
#endif /* defined(__ENABLE_MOCANA_INNER_APP__) */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_INNER_APP__)
extern MSTATUS
SSLSOCK_verifyInnerAppVerifyData(SSLSocket *pSSLSock, ubyte *data, InnerAppType innerAppType, sbyte4 isClient)
{
    ubyte   verifyData[12];
    sbyte4  cmp;
    MSTATUS status = OK;

    if (NULL == pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 < isClient)
    {
        status = PRF(pSSLSock, pSSLSock->innerSecret, SSL_MASTERSECRETSIZE,
                     (const ubyte*)SSL_INNER_APP_CLIENT_PHRASE,SSL_INNER_APP_CLIENT_PHRASE_LEN,
                     verifyData, 12);
    }
    else
    {
        status = PRF(pSSLSock, pSSLSock->innerSecret, SSL_MASTERSECRETSIZE,
                     (const ubyte*)SSL_INNER_APP_SERVER_PHRASE,SSL_INNER_APP_SERVER_PHRASE_LEN,
                     verifyData, 12);
    }

    if (OK > status)
        goto exit;

    MOC_MEMCMP(verifyData,data,12,&cmp);
    if (cmp)
    {
        status = ERR_SSL_INNER_APP_VERIFY_DATA;
        goto exit;
    }

exit:
    return status;

}
#endif /* defined(__ENABLE_MOCANA_INNER_APP__) */


/*------------------------------------------------------------------*/

static MSTATUS
recvAll(SSLSocket* pSSLSock, sbyte* pBuffer, sbyte4 toReceive,
        const enum sslAsyncStates curAsyncState, const enum sslAsyncStates nextAsyncState,
        ubyte **ppPacketPayload, ubyte4 *pPacketLength)
{
    intBoolean  boolComplete = FALSE;
    ubyte4      numBytesRead;
    MSTATUS     status = OK;

    if ((NULL == pSSLSock) || (NULL == pBuffer) || (NULL == ppPacketPayload) ||
        (NULL == *ppPacketPayload) || (NULL == pPacketLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 == toReceive)
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    if ((curAsyncState != SSL_RX_RECORD_STATE(pSSLSock)) || (FALSE == SSL_RX_RECORD_STATE_INIT(pSSLSock)))
    {
        /* new state */
        SSL_RX_RECORD_STATE(pSSLSock) = curAsyncState;
        SSL_RX_RECORD_BUFFER(pSSLSock) = (ubyte *)pBuffer;
        SSL_RX_RECORD_BYTES_READ(pSSLSock) = 0;
        SSL_RX_RECORD_BYTES_REQUIRED(pSSLSock) = toReceive;
        SSL_RX_RECORD_STATE_INIT(pSSLSock) = TRUE;
    }

    if (0 == (SSL_RX_RECORD_BYTES_REQUIRED(pSSLSock) - SSL_RX_RECORD_BYTES_READ(pSSLSock)))
    {
        status = ERR_MISSING_STATE_CHANGE;
        goto exit;
    }

    if (SSL_RX_RECORD_BYTES_READ(pSSLSock) > SSL_RX_RECORD_BYTES_REQUIRED(pSSLSock))
    {
        /*!-!-! should never happen */
        status = ERR_BUFFER_OVERFLOW;
        goto exit;
    }

    if ((SSL_RX_RECORD_BYTES_REQUIRED(pSSLSock) - SSL_RX_RECORD_BYTES_READ(pSSLSock)) <= *pPacketLength)
    {
        /* enough bytes available for a complete message */
        numBytesRead = (SSL_RX_RECORD_BYTES_REQUIRED(pSSLSock) - SSL_RX_RECORD_BYTES_READ(pSSLSock));
        boolComplete = TRUE;
    }
    else
        numBytesRead = *pPacketLength;

    if (0 != numBytesRead)
    {
        MOC_MEMCPY(SSL_RX_RECORD_BYTES_READ(pSSLSock) + SSL_RX_RECORD_BUFFER(pSSLSock), *ppPacketPayload, numBytesRead);

        SSL_RX_RECORD_BYTES_READ(pSSLSock) += numBytesRead;

        /* digest bytes from packet */
        *ppPacketPayload += numBytesRead;
        *pPacketLength   -= numBytesRead;
    }

    if (TRUE == boolComplete)
        SSL_RX_RECORD_STATE_CHANGE(pSSLSock, nextAsyncState)

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"recvAll() returns status = ", status);
#endif

    return status;

} /* recvAll */


/*------------------------------------------------------------------*/

/*******************************************************************************
* addToHandshakeHash
* This routine is used to had all handshakes records to the hashes. These
* hashes are used in the Finished handshake records.
*/
static void
addToHandshakeHash(SSLSocket* pSSLSock, ubyte* data, sbyte4 size)
{
    MSTATUS status = OK;

#ifdef __ENABLE_ALL_DEBUGGING__
    SSLHandshakeHeader* pSHSH = (SSLHandshakeHeader*) data;
    DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"Handshake length = ", size);

    RTOS_sleepMS(200);

    if (pSSLSock->server)
        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)" (SERVER)");
    else
        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)" (CLIENT)");

    switch ( pSHSH->handshakeType)
    {
    case SSL_CLIENT_HELLO:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Client Hello");
        break;

    case SSL_SERVER_HELLO:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Server hello");
        break;

    case SSL_SERVER_HELLO_VERIFY_REQUEST:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Server HelloVerifyRequest");
        break;

    case SSL_CERTIFICATE:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Certificate");
        break;

    case SSL_CERTIFICATE_REQUEST:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Certificate Request");
        break;

    case SSL_SERVER_HELLO_DONE:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Server Hello done");
        break;

    case SSL_CLIENT_CERTIFICATE_VERIFY:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Client Certificate Verify");
        break;

    case SSL_SERVER_KEY_EXCHANGE:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Server Key Exchange");
        break;

    case SSL_CLIENT_KEY_EXCHANGE:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Client Key Exchange");
        break;

    case SSL_FINISHED:
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" Finished");
        break;
    }
    PrintBytes( data, size);
#endif
    /* Fix some compiler warnings by referencing these vars */
    (void)gSupportedSignatureAlgorithms;
    (void)gSupportedHashAlgorithms;

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        MD5update_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pMd5Ctx, data, size);
        SHA1_updateDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pShaCtx, data, size);
    }
    else
    {
        const BulkHashAlgo *pHashAlgo = NULL;

        if (!pSSLSock->pHandshakeCipherSuite)
            return;

        /* TLS1.2 changes */
        /* the hash is used for finished verify. the hash algo is PRFHashingAlgo */
        pHashAlgo = pSSLSock->pHandshakeCipherSuite->pPRFHashAlgo;

#ifndef __DISABLE_MOCANA_SHA256__
        if (!pHashAlgo)
        {
            pHashAlgo = &SHA256Suite;
        }
#endif

        if (OK > (status = pHashAlgo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pHashCtx, data, size)))
            goto exit;

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
        /* change in TLS12 calculate for all supported hash algos */
        if ((SSL_NO_MUTUAL_AUTH_BIT != (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_NO_MUTUAL_AUTH_BIT)) &&
            (SSL_FLAG_NO_MUTUAL_AUTH_REPLY != (pSSLSock->runtimeFlags & SSL_FLAG_NO_MUTUAL_AUTH_REPLY)))
        {
            /* allocate memory for BulkCtx if not already done */
                ubyte4 i;
                /* initialize hashCtx */
                if (!pSSLSock->pHashCtxList)
                {
                    pSSLSock->pHashCtxList = MALLOC(sizeof(BulkCtx)*NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS);

                    if (NULL == pSSLSock->pHashCtxList)
                    {
                        status = ERR_MEM_ALLOC_FAIL;
                        goto exit;
                    }

                    MOC_MEMSET((ubyte*)pSSLSock->pHashCtxList, 0x00, sizeof(BulkCtx)*NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS);
                }

                for (i = 0; i < NUM_SSL_SUPPORTED_HASH_ALGORITHMS; i++)
                {
                    if (!pSSLSock->pHashCtxList[i])
                    {
                        if (OK > (status = gSupportedHashAlgorithms[i].algo->allocFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pSSLSock->pHashCtxList[i])))
                            goto exit;
                        if (OK > (status = gSupportedHashAlgorithms[i].algo->initFunc(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pHashCtxList[i])))
                            goto exit;
                    }
                    if (OK > (status = gSupportedHashAlgorithms[i].algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pHashCtxList[i], data, size)))
                        goto exit;
                }
        }
#endif /* #ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */
    }

exit:
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"addToHandshakeHash return error: ", status);
}


/*------------------------------------------------------------------*/

/*******************************************************************************
* computePadLength
* This routine compute the minimum pad length in case of block encryption
*/
static sbyte4
computePadLength(sbyte4 msgSize, sbyte4 blockSize)
{
    if (blockSize)
    {
        sbyte4 retVal = blockSize - (msgSize % blockSize);
        return (0 == retVal) ? blockSize : retVal;
    }
    return 0;
}


/*------------------------------------------------------------------*/

/*******************************************************************************
* calculateTLSFinishedVerify
* this function is called only for the TLS implementation
*/
static MSTATUS
calculateTLSFinishedVerify(SSLSocket *pSSLSock, sbyte4 client, ubyte result[TLS_VERIFYDATASIZE])
{
    /* pSSLSock's pMd5Ctx and pShaCtx contain hash of all the handshake
        messages (see addToHandshakeHash)*/
    /* There are 2 finished messages one sent by server, the other by the client
       one is to be sent, the other is received and we want to verify it */
    ubyte*      pBuffer = NULL;     /* [ TLS_FINISHEDLABELSIZE + MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE] */
    MSTATUS     status;
    ubyte4      hashSize = 0;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)&(pBuffer))))
        goto exit;

    /* put the correct label in the beginning of buffer */
    MOC_MEMCPY(pBuffer, (ubyte *)(client ? "client finished" : "server finished"), TLS_FINISHEDLABELSIZE);

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        MD5_CTXHS*  pMd5Copy = NULL;
        shaDescrHS* pShaCopy = NULL;
        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Copy))))
            goto exit1;

        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pShaCopy))))
            goto exit1;

        /* copy the contexts */
#ifndef __ENABLE_HARDWARE_ACCEL_CRYPTO__
        MOC_MEMCPY(pMd5Copy, pSSLSock->pMd5Ctx, sizeof(MD5_CTXHS));
        MOC_MEMCPY(pShaCopy, pSSLSock->pShaCtx, sizeof(shaDescrHS));
#else
        if (OK > (status = MD5CopyCtx_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, &pSSLSock->pMd5Ctx)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit1;
        }

        if (OK > (status = SHA1_CopyCtxHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, &pSSLSock->pShaCtx)))
        {
            MD5FreeCtx_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy);
            status = ERR_MEM_ALLOC_FAIL;
            goto exit1;
        }
#endif

        /* save results in the buffer */
        MD5final_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, pBuffer + TLS_FINISHEDLABELSIZE);
        SHA1_finalDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, pBuffer + TLS_FINISHEDLABELSIZE + MD5_DIGESTSIZE);

        hashSize = MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE;
exit1:
        MEM_POOL_putPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Copy));
        MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pShaCopy));

    } else
    {
        BulkCtx pHashCtxCopy = NULL;

        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->hashPool, (void **)(&pHashCtxCopy))))
            goto exit2;

        /* TLS1.2 and up */
        /* copy the hashcontext because it needs to be used again for verifying the client's finished message */
#ifndef __ENABLE_HARDWARE_ACCEL_CRYPTO__
        MOC_MEMCPY(pHashCtxCopy, pSSLSock->pHashCtx, pSSLSock->hashPool.poolObjectSize);
#else
        /* don't know what to do for hw_accel */
#endif
        if (pSSLSock->pHandshakeCipherSuite->pPRFHashAlgo)
        {
            pSSLSock->pHandshakeCipherSuite->pPRFHashAlgo->finalFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtxCopy, pBuffer + TLS_FINISHEDLABELSIZE);
            hashSize = pSSLSock->pHandshakeCipherSuite->pPRFHashAlgo->digestSize;
        }
#ifndef __DISABLE_MOCANA_SHA256__
        else
        {
            SHA256Suite.finalFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtxCopy, pBuffer + TLS_FINISHEDLABELSIZE);
            hashSize = SHA256Suite.digestSize;
        }
#endif

exit2:
        MEM_POOL_putPoolObject(&pSSLSock->hashPool, (void **)(&pHashCtxCopy));

    }

    /* call prf with master secret */
    status = PRF(pSSLSock, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE,
        pBuffer, TLS_FINISHEDLABELSIZE + hashSize,
        result, TLS_VERIFYDATASIZE);

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (client)
        MOC_MEMCPY(pSSLSock->client_verify_data, result, TLS_VERIFYDATASIZE);
    else
        MOC_MEMCPY(pSSLSock->server_verify_data, result, TLS_VERIFYDATASIZE);
#endif

exit:
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pBuffer));

    return status;
}


/*--------------------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
/*******************************************************************************
* calculateFinishedHashes
* see fig. 4-28 of SSL and TLS Essentials, page 91
* This function is used for calculating
* 1. SSL 3.0 FinishedVerify;
* and 2. CertificateVerify for versions prior to TLS1.2 (or DTLS1.2)
*/
static MSTATUS
calculateSSLTLSHashes(SSLSocket *pSSLSock, sbyte4 client,
                      ubyte* result,
                      enum hashTypes hashType)
{
    MSTATUS     status = OK;

    /* pSSLSock's pMd5Ctx and pShaCtx contain hash of all the handshake
        messages (see addToHandshakeHash)*/
    /* There are 2 finished messages one sent by server, the other by the client
       one is to be sent, the other is received and we want to verify it */
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    ubyte* senderRole = (ubyte*)
    ((client) ? "CLNT" : "SRVR");
#endif

    MD5_CTXHS*  pMd5Copy = NULL;
    shaDescrHS* pShaCopy = NULL;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Copy))))
        goto exit1;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pShaCopy))))
        goto exit1;

#ifndef __ENABLE_HARDWARE_ACCEL_CRYPTO__
    MOC_MEMCPY(pMd5Copy, pSSLSock->pMd5Ctx, sizeof(MD5_CTXHS));
    MOC_MEMCPY(pShaCopy, pSSLSock->pShaCtx, sizeof(shaDescrHS));
#else
    if ( MD5CopyCtx_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, &pSSLSock->pMd5Ctx) != OK)
    {
        goto exit1;
    }
    if (SHA1_CopyCtxHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, &pSSLSock->pShaCtx) != OK)
    {
        MD5FreeCtx_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy);
        goto exit1;
    }
#endif

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    switch (hashType)
    {
        case hashTypeSSLv3Finished:

            /* add sender Role*/
            MD5update_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, senderRole, 4);
            SHA1_updateDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, senderRole, 4);
            /* flow-through */

        case hashTypeSSLv3CertificateVerify:
            /* add master secret */
            MD5update_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);
            SHA1_updateDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);

            /* add first padding (0x36) */
            MD5update_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, gHashPad36, SSL_MD5_PADDINGSIZE);
            SHA1_updateDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, gHashPad36, SSL_SHA1_PADDINGSIZE);

            /* save results in the buffer provided as argument */
            MD5final_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, result);
            SHA1_finalDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, result + MD5_DIGESTSIZE);

            /* initialize the hashes */
#ifndef __ENABLE_HARDWARE_ACCEL_CRYPTO__
            MD5init_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy);
            SHA1_initDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy);
#else
            if (OK > MD5init_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy))
                goto exit1;

            if (OK > SHA1_initDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy))
            {
                MD5FreeCtx_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy);
                goto exit1;
            }
#endif

            /* add master secret */
            MD5update_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);
            SHA1_updateDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);

            /* add second padding  (0x5C) */
            MD5update_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, gHashPad5C, SSL_MD5_PADDINGSIZE);
            SHA1_updateDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, gHashPad5C, SSL_SHA1_PADDINGSIZE);

            /* add first hash */
            MD5update_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, result, MD5_DIGESTSIZE);
            SHA1_updateDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, result + MD5_DIGESTSIZE, SHA_HASH_RESULT_SIZE);
            /* flow-through */

        default:
            /* save results in the buffer provided as argument */
            /* for TLS CertificateVerify, the hash is only calculated on the handshake messages */
            MD5final_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, result);
            SHA1_finalDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, result + MD5_DIGESTSIZE);
    }

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (hashTypeSSLv3Finished == hashType)
    {
        if (client)
            MOC_MEMCPY(pSSLSock->client_verify_data, result, SSL_VERIFY_DATA);
        else
            MOC_MEMCPY(pSSLSock->server_verify_data, result, SSL_VERIFY_DATA);
    }
#endif

#else  /* MIN_SSL_MINORVERSION <= SSL3_MINORVERSION */

    MD5final_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Copy, result);
    SHA1_finalDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pShaCopy, result + MD5_DIGESTSIZE);

#endif

exit1:
    MEM_POOL_putPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Copy));
    MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pShaCopy));

    return status;
}

#endif /* defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || MIN_SSL_MINORVERSION <= SSL3_MINORVERSION */

/*--------------------------------------------------------------------------------*/

/* this is the pseudo random output function for TLS
    output is of length resultLen and is stored in result
    (which should be at least resultLen bytes long!)
*/

static void
P_hash(SSLSocket *pSSLSock, const ubyte* secret, sbyte4 secretLen,
       const ubyte* seed, sbyte4 seedLen,
       ubyte* result, sbyte4 resultLen, const BulkHashAlgo *pBHAlgo)
{
    ubyte*  pA = NULL;
    ubyte*  pB = NULL;
    sbyte4  produced;
    BulkCtx context = NULL;

    if (OK > MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)&(pA)))
        goto exit;

    if (OK > MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)&(pB)))
        goto exit;


    if (OK > pBHAlgo->allocFunc(MOC_HASH(hwAccelCtx) &context))
	{
		goto exit;
	}

    /* A(0) */
    HmacQuickerInline(MOC_HASH(pSSLSock->hwAccelCookie) secret, secretLen, seed, seedLen, pA, pBHAlgo,context);
    for (produced = 0; produced < resultLen; produced += pBHAlgo->digestSize)
    {
        sbyte4 numToCopy;

        HmacQuickerInlineEx(MOC_HASH(pSSLSock->hwAccelCookie) secret, secretLen, pA, pBHAlgo->digestSize, seed, seedLen, pB, pBHAlgo,context);
        /* put in result buffer */
        numToCopy =  resultLen - produced;
        if ( numToCopy > pBHAlgo->digestSize)
        {
            numToCopy = pBHAlgo->digestSize;
        }
        MOC_MEMCPY( result + produced, pB, numToCopy);

        /* A(i) */
        HmacQuickerInline(MOC_HASH(pSSLSock->hwAccelCookie) secret, secretLen, pA, pBHAlgo->digestSize, pA, pBHAlgo,context);
    }

exit:
	if ( context != NULL )
	{
		pBHAlgo->freeFunc(MOC_HASH(hwAccelCtx) &context);
	}

    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pB));
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pA));
}

/*--------------------------------------------------------------------------------*/

static MSTATUS
PRF(SSLSocket *pSSLSock, const ubyte* secret, sbyte4 secretLen,
    const ubyte* labelSeed, sbyte4 labelSeedLen,
    ubyte* result, sbyte4 resultLen)
{
    MSTATUS status = OK;

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        ubyte*  temp = NULL;   /* temp buffer allocated locally */
        const ubyte*  s2;            /* half secrets */
        sbyte4     i;
        sbyte4     halfSecretLen;  /* length of half secrets */

        /* split the secret in two */
        if ( secretLen & 1) /* odd */
        {
            halfSecretLen = (secretLen + 1) / 2;
        }
        else
        {
            halfSecretLen = secretLen / 2;
        }

        /* start of halfsecret */
        s2 = secret + secretLen - halfSecretLen;

        /* compute both XOR inputs */
        if (NULL == (temp = (ubyte*) MALLOC(resultLen)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
        P_hash(pSSLSock, secret, halfSecretLen, labelSeed, labelSeedLen, result, resultLen, &MD5Suite);
        P_hash(pSSLSock, s2, halfSecretLen, labelSeed, labelSeedLen, temp, resultLen, &SHA1Suite);

        for ( i=0; i < resultLen; ++i)
        {
            result[i] ^= temp[i];
        }

exit:
        if (NULL != temp)
            FREE(temp);
    }
    else
    {
        /* use SHA256 for MD5 and SHA1 ciphers; use cipher specified for new ciphers */
        /* tiny sslcli build disables SHA256 by default, what to do?  not supporting tls1.2? */
        CipherSuiteInfo *pCS = pSSLSock->pHandshakeCipherSuite;
        if (!pCS->pPRFHashAlgo)
        {
#if defined(__DISABLE_MOCANA_SHA256__)
            status = ERR_SSL_UNSUPPORTED_ALGORITHM;
#else
            P_hash(pSSLSock, secret, secretLen, labelSeed, labelSeedLen, result, resultLen, &SHA256Suite);
#endif
        }
        else
        {
            P_hash(pSSLSock, secret, secretLen, labelSeed, labelSeedLen, result, resultLen, pCS->pPRFHashAlgo);
        }
    }
    return status;
}


/*--------------------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_EAP_FAST__)
static MSTATUS
T_PRF(MOC_HASH(SSLSocket *pSSLSock) ubyte* secret, sbyte4 secretLen,
      ubyte* labelSeed, sbyte4 labelSeedLen,
      ubyte* result, sbyte4 resultLen)
{
    ubyte*  texts[3];               /* argument to HMAC_SHA1Ex */
    sbyte4  textLens[3];            /* argument to HMAC_SHA1Ex */
    ubyte   suffix[3];              /* output length + counter */
    sbyte4  numTexts;
    MSTATUS status;

    /* initialize variables for first round */
    suffix[0] = (ubyte) (resultLen >> 8);
    suffix[1] = (ubyte) (resultLen);
    suffix[2] = 1;

    texts[0]= labelSeed;
    textLens[0] = labelSeedLen;
    texts[1] = texts[2] = suffix;
    textLens[1] = textLens[2] = 3;
    numTexts = 2;

    while ( resultLen > SHA1_RESULT_SIZE)
    {
        if ( OK > (status = HMAC_SHA1Ex(MOC_HASH(pSSLSock) secret, secretLen,
                                            texts, textLens,
                                            numTexts, result)))
        {
            goto exit;
        }

        /* prepare next round */
        texts[0] = result;
        textLens[0] = SHA1_RESULT_SIZE;
        texts[1] = labelSeed;
        textLens[1] = labelSeedLen;
        numTexts = 3; /* for all subsequent rounds */

        /* increment counters and pointers */
        ++suffix[2];
        resultLen -= SHA1_RESULT_SIZE;
        result += SHA1_RESULT_SIZE;
    }

    if ( resultLen > 0)
    {
        ubyte   temp[SHA1_RESULT_SIZE]; /* last result */
        if ( OK > (status = HMAC_SHA1Ex(MOC_HASH(pSSLSock) secret, secretLen,
                                            texts, textLens,
                                            numTexts, temp)))
        {
            goto exit;
        }

        MOC_MEMCPY( result, temp, resultLen);
    }

    status = OK;

exit:
    return status;
}
#endif /* __ENABLE_MOCANA_EAP_FAST__ */


/*--------------------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_EAP_FAST__)
static MSTATUS
SSL_SOCK_generateEAPFASTMasterSecret(SSLSocket *pSSLSock)
{
    /*    Master_secret = T-PRF(PAC-Key,
                     "PAC to master secret label hash",
                          server_random + Client_random,
                          48)
    */
    /* use a stack buffer so that the pSSLSock pSecretAndRand is not
    perturbed: we would need to switch client and server randoms twice
    otherwise */
    ubyte   labelSeed[EAPFAST_PAC_MASTERSECRET_HASH + 2 * SSL_RANDOMSIZE];

    MOC_MEMCPY( labelSeed, (ubyte *)"PAC to master secret label hash",
                EAPFAST_PAC_MASTERSECRET_HASH);
    MOC_MEMCPY( labelSeed + EAPFAST_PAC_MASTERSECRET_HASH,
                pSSLSock->pServerRandHello,
                SSL_RANDOMSIZE);
    MOC_MEMCPY( labelSeed + EAPFAST_PAC_MASTERSECRET_HASH + SSL_RANDOMSIZE,
                pSSLSock->pClientRandHello,
                SSL_RANDOMSIZE);

   return T_PRF(MOC_HASH(pSSLSock) pSSLSock->pacKey, PACKEY_SIZE,
                    labelSeed, EAPFAST_PAC_MASTERSECRET_HASH + 2 * SSL_RANDOMSIZE,
                    pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);
}
#endif /* __ENABLE_MOCANA_EAP_FAST__ */


/*--------------------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_EAP_FAST__)
extern MSTATUS
SSL_SOCK_generateEAPFASTIntermediateCompoundKey(SSLSocket *pSSLSock,
                                    ubyte *s_imk,
                                    ubyte *msk,
                                    ubyte mskLen, ubyte *imk)
{
    ubyte   labelSeed[EAPFAST_IM_COMPOUND_KEY_SIZE + EAPFAST_IM_MSK_SIZE + 1];
    sbyte4  labelSeedLen = EAPFAST_IM_COMPOUND_KEY_SIZE+EAPFAST_IM_MSK_SIZE+1;
    MSTATUS status = OK;
    MOC_MEMSET(labelSeed, 0, labelSeedLen);
    MOC_MEMCPY(labelSeed, (const ubyte *)"Inner Methods Compound Keys", EAPFAST_IM_COMPOUND_KEY_SIZE);
    if (msk && mskLen != 0)
    {
        if (mskLen > EAPFAST_IM_MSK_SIZE)
        {
            MOC_MEMCPY(labelSeed + EAPFAST_IM_COMPOUND_KEY_SIZE + 1,
                       msk, EAPFAST_IM_MSK_SIZE);
        }
        else
        {
            MOC_MEMCPY(labelSeed + EAPFAST_IM_COMPOUND_KEY_SIZE + 1,
            msk, mskLen);
        }
    }
    if (NULL == s_imk)
    {
        status = T_PRF(MOC_HASH(pSSLSock)pSSLSock->sessionKeySeed,
                           40, labelSeed, labelSeedLen, imk, 60);
    }
    else
    {
        status = T_PRF(MOC_HASH(pSSLSock)s_imk,
                           40, labelSeed, labelSeedLen, imk, 60);
    }
    if (OK > status)
        goto exit;
exit:
    return status;
}
#endif /* __ENABLE_MOCANA_EAP_FAST__ */


/*--------------------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_EAP_FAST__)
extern MSTATUS
SSL_SOCK_generateEAPFASTSessionKeys(SSLSocket *pSSLSock, ubyte* S_IMCK, sbyte4 s_imckLen,
                                    ubyte* MSK, sbyte4 mskLen, ubyte* EMSK, sbyte4 emskLen/*64 Len */)

{
    ubyte*  temp = NULL;   /* temp buffer allocated locally */
    ubyte   labelSeed[] = "Session Key Generating Function";
    ubyte4  labelSeedLen;
    ubyte   emsklabelSeed[] = "Extended Session Key Generating Function";
    ubyte4  emsklabelSeedLen;
    MSTATUS status = OK;

    labelSeedLen = MOC_STRLEN((const sbyte *)labelSeed) + 1;
    emsklabelSeedLen = MOC_STRLEN((const sbyte *)emsklabelSeed) + 1;

    T_PRF(MOC_HASH(pSSLSock) S_IMCK, s_imckLen, labelSeed, labelSeedLen, MSK, mskLen/*64 Bytes */);

    T_PRF(MOC_HASH(pSSLSock) S_IMCK, s_imckLen, emsklabelSeed, emsklabelSeedLen, EMSK, emskLen/*64 Bytes */);
#ifdef __ENABLE_ALL_DEBUGGING__
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"FAST S IMCK ");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    PrintBytes(S_IMCK, s_imckLen);
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"MSK ");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    PrintBytes(MSK, mskLen);
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"EMSK ");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    PrintBytes(EMSK, emskLen);
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
#endif
    if (NULL != temp)
        FREE(temp);

    return status;
}
#endif /* __ENABLE_MOCANA_EAP_FAST__ */


/*--------------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__))
extern MSTATUS
SSL_SOCK_generatePEAPIntermediateKeys(SSLSocket *pSSLSock, ubyte* IPMK, sbyte4 ipmkLen,
                                      ubyte* ISK, sbyte4 iskLen, ubyte* result, sbyte4 resultLen/*32 Len */)
{
    ubyte*  temp = NULL;   /* temp buffer allocated locally */
    ubyte   labelSeed[] = "Intermediate PEAP MAC key";
    ubyte4  labelSeedLen;
    MSTATUS status = OK;

    labelSeedLen = MOC_STRLEN((const sbyte*)labelSeed);

    if (NULL == (temp = (ubyte*) MALLOC(iskLen+labelSeedLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(temp,labelSeed,labelSeedLen);
    MOC_MEMCPY(temp+labelSeedLen,ISK,iskLen);

    P_hash(pSSLSock, IPMK, ipmkLen, temp, iskLen+labelSeedLen, result, resultLen, &SHA1Suite);

exit:
    if (NULL != temp)
        FREE(temp);

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__)) */


/*--------------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__))
extern MSTATUS
SSL_SOCK_generatePEAPServerCompoundMacKeys(SSLSocket *pSSLSock, ubyte* IPMK,
                                           sbyte4 ipmkLen, ubyte* S_NONCE, sbyte4 s_nonceLen,
                                           ubyte* result, sbyte4 resultLen/*20 bytes*/)
{
    ubyte*  temp = NULL;   /* temp buffer allocated locally */
    ubyte   labelSeed[] = "PEAP Server B1 MAC key";
    ubyte4  labelSeedLen;
    MSTATUS status = OK;

    labelSeedLen = MOC_STRLEN((const sbyte*)labelSeed);

    if (NULL == (temp = (ubyte*) MALLOC(s_nonceLen+labelSeedLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(temp,labelSeed,labelSeedLen);
    MOC_MEMCPY(temp+labelSeedLen,S_NONCE,s_nonceLen);

    P_hash(pSSLSock, IPMK, ipmkLen, temp, s_nonceLen+labelSeedLen, result, resultLen, &SHA1Suite);

exit:
    if (NULL != temp)
        FREE(temp);

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__)) */


/*--------------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__))
extern MSTATUS
SSL_SOCK_generatePEAPClientCompoundMacKeys(SSLSocket *pSSLSock, ubyte* IPMK,
                                           sbyte4 ipmkLen, ubyte* S_NONCE, sbyte4 s_nonceLen,
                                           ubyte* C_NONCE, sbyte4 c_nonceLen, ubyte* result,
                                           sbyte4 resultLen/*20 bytes*/)
{
    ubyte*  temp = NULL;   /* temp buffer allocated locally */
    ubyte   labelSeed[] = "PEAP Client B2 MAC key";
    ubyte4  labelSeedLen;
    MSTATUS status = OK;

    labelSeedLen = MOC_STRLEN((const sbyte*)labelSeed);

    if (NULL == (temp = (ubyte*) MALLOC(s_nonceLen+c_nonceLen+labelSeedLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(temp,labelSeed,labelSeedLen);
    MOC_MEMCPY(temp+labelSeedLen,S_NONCE,s_nonceLen);
    MOC_MEMCPY(temp+labelSeedLen+s_nonceLen,C_NONCE,c_nonceLen);

    P_hash(pSSLSock, IPMK, ipmkLen, temp, s_nonceLen+c_nonceLen+labelSeedLen, result, resultLen, &SHA1Suite);

exit:
    if (NULL != temp)
        FREE(temp);

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__)) */


/*--------------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__))
extern MSTATUS
SSL_SOCK_generatePEAPCompoundSessionKey(SSLSocket *pSSLSock, ubyte* IPMK , sbyte4 ipmkLen,
                                        ubyte* S_NONCE, sbyte4 s_nonceLen, ubyte* C_NONCE,
                                        sbyte4 c_nonceLen, ubyte* result, sbyte4 resultLen)
{
    ubyte*  temp = NULL;   /* temp buffer allocated locally */
    ubyte   labelSeed[] = "PEAP compound session key";
    ubyte4  labelSeedLen;
    MSTATUS status = OK;

    labelSeedLen = MOC_STRLEN((const sbyte*)labelSeed);

    if (NULL == (temp = (ubyte*) MALLOC(s_nonceLen+c_nonceLen+labelSeedLen+4)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(temp,labelSeed,labelSeedLen);
    MOC_MEMCPY(temp+labelSeedLen,S_NONCE,s_nonceLen);
    MOC_MEMCPY(temp+labelSeedLen+s_nonceLen,C_NONCE,c_nonceLen);
    MOC_MEMCPY(temp+labelSeedLen+s_nonceLen+c_nonceLen,(ubyte *)&resultLen,4);

    P_hash(pSSLSock, IPMK, ipmkLen, temp, s_nonceLen+c_nonceLen+labelSeedLen+4, result, resultLen, &SHA1Suite);

exit:
    if (NULL != temp)
        FREE(temp);

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__)) */


#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
/*******************************************************************************
* SHAMD5Rounds
* implement the core routine used to generate the master secret and the key
* pMaterials for SSL 3.0
*  ASSUMPTIONS:
    dest receives the results, it should be numRounds * MD5_DIGESTSIZE long
    dest should not share space with data (can add a test for this)
*/
static MSTATUS
SHAMD5Rounds(SSLSocket *pSSLSock, const ubyte* pPresecret, ubyte4 presecretLength,
             const ubyte data[2 * SSL_RANDOMSIZE],
             sbyte4 numRounds,
             ubyte* dest)
{
    ubyte prefix = (ubyte)'A';
    sbyte4 i;
    MD5_CTX*    pMd5Hash  = NULL;
    shaDescr*   pSha1Hash = NULL;
    ubyte*      pSha1Result = NULL;
    MSTATUS     status;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pSha1Hash))))
        goto exit;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Hash))))
        goto exit;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pSha1Result))))
        goto exit;

    for (i = 1; i <= numRounds; ++i, ++prefix)
    {
        sbyte4 j;

        MD5Init_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Hash);
        SHA1_initDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Hash);

        /* hash 'A' , 'BB', 'CCC' depending on the round */
        for (j = 0; j < i; ++j)
        {
            SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Hash, &prefix, 1);
        }

        /* hash the rest, this is either presecret,clientrandom, serverrandom for
         the master secret generation or mastersecret,clientrandom, serverrandom for
         the key material generation */
        SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Hash, pPresecret, presecretLength);
        SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Hash, data, 2 * SSL_RANDOMSIZE);
        SHA1_finalDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Hash, pSha1Result);

        /*SHA done*/

        /* hash only the first presecretLength(RSA == 48) bytes of data with MD5 */
        MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Hash, (ubyte *)pPresecret, presecretLength);
        /* followed by the just computed SHA1 hash */
        MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Hash, pSha1Result, SHA_HASH_RESULT_SIZE);
        /* store the result in dest */
        MD5Final_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Hash, dest);

        /* increment dest pointer */
        dest += MD5_DIGESTSIZE;
    }

exit:
    MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pSha1Hash));
    MEM_POOL_putPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Hash));
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pSha1Result));

    return status;
}
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_KEY_EXPANSION__
extern MSTATUS
SSL_SOCK_generateTLSKeyExpansionMaterial(SSLSocket *pSSLSock,
                                         ubyte *pKey, ubyte2 keySize,
                                         ubyte *keyPhrase, ubyte2 keyPhraseLen)
{
    ubyte               masterSecret[SSL_MASTERSECRETSIZE];
    sbyte4              i;
    ubyte4              keyLen = keySize;
    ubyte*              random1;
    ubyte*              random2;
    MSTATUS             status = OK;

    MOC_MEMCPY( masterSecret, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);

    random1 = START_RANDOM( pSSLSock);
    random2 = random1 + SSL_RANDOMSIZE;
    for (i = 0; i < SSL_RANDOMSIZE; ++i)
    {
        ubyte swap = *random1;
        *random1++ = *random2;
        *random2++ = swap;
    }

    /* copy label "key expansion" in its special place */
    MOC_MEMCPY( START_RANDOM( pSSLSock) - keyPhraseLen,
            keyPhrase,
            keyPhraseLen);
    /* generate keys with PRF */
    if (128 >= keySize)
        keyLen = keySize;
    else
        keyLen = 128;

    status = PRF(pSSLSock, masterSecret, SSL_MASTERSECRETSIZE,
            START_RANDOM( pSSLSock) - keyPhraseLen,
            keyPhraseLen + 2 * SSL_RANDOMSIZE,
            pKey, keyLen);

    if (OK > status)
        goto exit;


    if (128 <  keySize)
    {
        status = PRF(pSSLSock, (const ubyte*)"", SSL_MASTERSECRETSIZE,
                START_RANDOM( pSSLSock) - keyPhraseLen,
                keyPhraseLen + 2 * SSL_RANDOMSIZE,
                pKey + 128, keySize - 128);
    }

exit:
        /* store master secret in its place after that */
    MOC_MEMCPY(pSSLSock->pSecretAndRand, masterSecret, SSL_MASTERSECRETSIZE);
    random1 = START_RANDOM( pSSLSock);
    random2 = random1 + SSL_RANDOMSIZE;
    for (i = 0; i < SSL_RANDOMSIZE; ++i)
    {
        ubyte swap = *random1;
        *random1++ = *random2;
        *random2++ = swap;
    }

    return status;
}
#endif /* __ENABLE_MOCANA_SSL_KEY_EXPANSION__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_KEY_EXPANSION__
extern MSTATUS
SSL_SOCK_generateKeyExpansionMaterial(SSLSocket *pSSLSock,
                                      ubyte *pKey, ubyte2 keySize,
                                      ubyte *keyPhrase, ubyte2 keyPhraseLen)
{
    ubyte               masterSecret[SSL_MASTERSECRETSIZE];
    sbyte4              i;
    ubyte*              random1;
    ubyte*              random2;
    MSTATUS             status = OK;

    MOC_MEMCPY( masterSecret, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);

    random1 = START_RANDOM( pSSLSock);
    random2 = random1 + SSL_RANDOMSIZE;
    for (i = 0; i < SSL_RANDOMSIZE; ++i)
    {
        ubyte swap = *random1;
        *random1++ = *random2;
        *random2++ = swap;
    }

    /* copy label "key expansion" in its special place */
    MOC_MEMCPY( START_RANDOM( pSSLSock) - keyPhraseLen,
            keyPhrase,
            keyPhraseLen);
    /* generate keys with PRF */
    status = PRF(pSSLSock, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE,
            START_RANDOM( pSSLSock) - keyPhraseLen,
            keyPhraseLen + 2 * SSL_RANDOMSIZE,
            pKey, keySize);

        /* store master secret in its place after that */
    MOC_MEMCPY(pSSLSock->pSecretAndRand, masterSecret, SSL_MASTERSECRETSIZE);

    return status;
}
#endif /* __ENABLE_MOCANA_SSL_KEY_EXPANSION__ */


/*------------------------------------------------------------------*/

/*******************************************************************************
* SSL_SOCK_generateKeyMaterial
* see pages 97-104 of SSL and TLS Essentials
* the algorithm used varies between SSL and TLS
*
* preMasterSecret & preMasterSecretLength is used only when the session is not
* resumed
*/
extern MSTATUS
SSL_SOCK_generateKeyMaterial(SSLSocket *pSSLSock,
                             ubyte* preMasterSecret, ubyte4 preMasterSecretLength)
{
    ubyte*              pMasterSecret = NULL; /* [SSL_MASTERSECRETSIZE] */
    sbyte4              totalMaterialSize=0, i;
    ubyte*              random1;
    ubyte*              random2;
    CipherSuiteInfo*    pCS = pSSLSock->pHandshakeCipherSuite;
    MSTATUS             status = OK;
    ubyte*              preMasterSecretCopy = NULL;

    if (preMasterSecretLength > 0)
    {
        if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, preMasterSecretLength, TRUE, (void **)&preMasterSecretCopy)))
            goto exit;
        MOC_MEMCPY(preMasterSecretCopy, preMasterSecret, preMasterSecretLength);
    }

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pMasterSecret))))
        goto exit;

    /* compute the total size of pMaterials needed */
    /* for TLS1.1 and up, don't need clientIV or serverIVs, R is generated per record, so don't generate them */
    totalMaterialSize = 2 * (pCS->keySize +
                             IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS) +
                             pCS->pCipherAlgo->getFieldFunc(Hash_Size));

#if defined(__ENABLE_MOCANA_EAP_FAST__)
    totalMaterialSize += SKS_SIZE;
    totalMaterialSize += FAST_MSCHAP_CHAL_SIZE;
#endif

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        if (E_NoSessionResume == pSSLSock->sessionResume)
        {
            /************************ generate the master secret */
#ifdef __ENABLE_ALL_DEBUGGING__

            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" premaster secret");
            PrintBytes(preMasterSecretCopy, preMasterSecretLength);

            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" client random");
            PrintBytes(START_RANDOM(pSSLSock), SSL_RANDOMSIZE);

            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" server random");
            PrintBytes(START_RANDOM(pSSLSock) + SSL_RANDOMSIZE, SSL_RANDOMSIZE);
#endif
            /* generate master secret with 3 rounds */
            if (OK > (status = SHAMD5Rounds(pSSLSock, preMasterSecretCopy, preMasterSecretLength, START_RANDOM(pSSLSock), 3, pMasterSecret)))
                goto exit;

            /* place master secret */
            MOC_MEMCPY(pSSLSock->pSecretAndRand, pMasterSecret, SSL_MASTERSECRETSIZE);
        }

        /* *************************** generate keys */
        /* swap server random and client random in pSecretAndRand */
        random1 = START_RANDOM( pSSLSock);
        random2 = random1 + SSL_RANDOMSIZE;
        for (i = 0; i < SSL_RANDOMSIZE; ++i)
        {
            ubyte swap = *random1;
            *random1++ = *random2;
            *random2++ = swap;
        }

        /* generate key pMaterials with the appropriate number of rounds */
        if (OK > (status = SHAMD5Rounds(pSSLSock, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE,
                                        START_RANDOM(pSSLSock),
                                        (totalMaterialSize + MD5_RESULT_SIZE - 1 )/ MD5_RESULT_SIZE,
                                        pSSLSock->pMaterials)))
        {
            goto exit;
        }
    }
    else
#endif
    /* TLS */
    {
        if (E_NoSessionResume == pSSLSock->sessionResume)
        {
            /************************ generate the master secret */
            /* copy label "master secret" in its special place */
            MOC_MEMCPY( START_RANDOM( pSSLSock) - TLS_MASTERSECRETSIZE, (ubyte *)"master secret", TLS_MASTERSECRETSIZE);

#ifdef __ENABLE_ALL_DEBUGGING__

            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" premaster secret");
            PrintBytes(preMasterSecretCopy, preMasterSecretLength);

            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" client random");
            PrintBytes(START_RANDOM(pSSLSock), SSL_RANDOMSIZE);

            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)" server random");
            PrintBytes(START_RANDOM(pSSLSock) + SSL_RANDOMSIZE, SSL_RANDOMSIZE);
#endif
            /* generate master secret with PRF */
            status = PRF(pSSLSock, preMasterSecretCopy, preMasterSecretLength,
                         START_RANDOM( pSSLSock) - TLS_MASTERSECRETSIZE,
                         TLS_MASTERSECRETSIZE + 2 * SSL_RANDOMSIZE,
                         pMasterSecret, SSL_MASTERSECRETSIZE);

            if (OK > status)
                goto exit;
        }
        else /* TLS: copy the master secret to the buffer because we are going to use
                its space for "key expansion" */
        {
            MOC_MEMCPY(pMasterSecret, pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);
        }

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__)) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
        if (pSSLSock->isDTLS && pSSLSock->useSrtp)
        {
            ubyte4 srtpMaterialSize;

            if (!pSSLSock->pHandshakeSrtpProfile)
            {
                status = ERR_DTLS_SRTP_NO_PROFILE_MATCH;
                goto exit;
            }

            /* assign the maximum material size, in case keySize and saltSize are 0 */
            /* srtpMaterialSize = 2 * (pSSLSock->pHandshakeSrtpProfile->keySize + pSSLSock->pHandshakeSrtpProfile->saltSize); */
            srtpMaterialSize = 2 * (SRTP_MAX_KEY_SIZE + SRTP_MAX_SALT_SIZE);

            /* copy label "key expansion" in its special place */
            MOC_MEMCPY( START_RANDOM( pSSLSock) - DTLS_SRTP_EXTRACTOR_SIZE,
                (ubyte *)DTLS_SRTP_EXTRACTOR,
                DTLS_SRTP_EXTRACTOR_SIZE);

            /* generate extractor for dtls_srtp with PRF */
            status = PRF(pSSLSock, pMasterSecret, SSL_MASTERSECRETSIZE,
                START_RANDOM( pSSLSock) - DTLS_SRTP_EXTRACTOR_SIZE,
                DTLS_SRTP_EXTRACTOR_SIZE + 2 * SSL_RANDOMSIZE,
                pSSLSock->pSrtpMaterials, srtpMaterialSize);

            if (OK > status)
                goto exit;
        }
#endif

        /* *************************** generate keys */
        /* NOTE: the method below is valid only for non export
         algorithm. See a text on SSL for the method to use in this case */
        /* swap server random and client random in pSecretAndRand */
        random1 = START_RANDOM(pSSLSock);
        random2 = random1 + SSL_RANDOMSIZE;

        for (i = 0; i < SSL_RANDOMSIZE; ++i)
        {
            ubyte swap = *random1;
            *random1++ = *random2;
            *random2++ = swap;
        }

        /* copy label "key expansion" in its special place */
        MOC_MEMCPY( START_RANDOM( pSSLSock) - TLS_KEYEXPANSIONSIZE,
                (ubyte *)"key expansion",
                TLS_KEYEXPANSIONSIZE);

        /* generate keys with PRF */
        status = PRF(pSSLSock, pMasterSecret, SSL_MASTERSECRETSIZE,
                     START_RANDOM( pSSLSock) - TLS_KEYEXPANSIONSIZE,
                     TLS_KEYEXPANSIONSIZE + 2 * SSL_RANDOMSIZE,
                     pSSLSock->pMaterials, totalMaterialSize);

        if (OK > status)
            goto exit;

        /* store master secret in its place after that */
        MOC_MEMCPY(pSSLSock->pSecretAndRand, pMasterSecret, SSL_MASTERSECRETSIZE);

    }

#if defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_DTLS_SERVER__)
    /* finally store the master secret for session resumption */
    if (pSSLSock->server && E_SessionIDResume != pSSLSock->sessionResume)
    {
        sbyte4 cacheIndex;

        if (OK > (status = RTOS_mutexWait(gSslSessionCacheMutex)))
        {
            MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"sslsock.c: RTOS_mutexWait() failed.");
            goto exit;
        }

        cacheIndex = pSSLSock->roleSpecificInfo.server.sessionId % SESSION_CACHE_SIZE;
        gSessionCache[cacheIndex].m_pCipherSuite = pSSLSock->pHandshakeCipherSuite;
        gSessionCache[cacheIndex].m_sessionId = pSSLSock->roleSpecificInfo.server.sessionId;
        MOC_MEMCPY(gSessionCache[cacheIndex].m_masterSecret,
                pSSLSock->pSecretAndRand,
                SSL_MASTERSECRETSIZE);
#if defined(__ENABLE_MOCANA_DTLS_SERVER__) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
        if (pSSLSock->isDTLS && pSSLSock->useSrtp)
        {
            gSessionCache[cacheIndex].m_pSrtpProfile = pSSLSock->pHandshakeSrtpProfile;
        }
#endif
		if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
            (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
        {
            gSessionCache[cacheIndex].m_signatureAlgo = pSSLSock->signatureAlgo;
        }

		gSessionCache[cacheIndex].m_clientECCurves = pSSLSock->roleSpecificInfo.server.clientECCurves;

        RTOS_deltaMS(NULL, &gSessionCache[cacheIndex].startTime);
        gSessionCache[cacheIndex].m_minorVersion = pSSLSock->sslMinorVersion;

        status = RTOS_mutexRelease(gSslSessionCacheMutex);
    }
#endif

exit:
#ifdef __ENABLE_ALL_DEBUGGING__
    RTOS_sleepMS(200);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");

    if (pSSLSock->server)
        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)" (SERVER) ");
    else
        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)" (CLIENT) ");

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"master secret");
    PrintBytes(pSSLSock->pSecretAndRand, SSL_MASTERSECRETSIZE);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"key material");
    PrintBytes(pSSLSock->pMaterials, totalMaterialSize);
#endif
    if (preMasterSecretCopy)
        CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, &preMasterSecretCopy);

    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pMasterSecret));

    return status;

} /* SSL_SOCK_generateKeyMaterial */


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_setClientKeyMaterial(SSLSocket *pSSLSock)
{
    CipherSuiteInfo*    pCS = pSSLSock->pHandshakeCipherSuite;
    ubyte*              keyStart;
#ifdef __ENABLE_MOCANA_EAP_FAST__
    ubyte*              keyStartFast;
#endif
    sbyte4              offset;
    MSTATUS             status = OK;

    /* dup client key material */
    MOC_MEMCPY(pSSLSock->pActiveMaterials, pSSLSock->pMaterials, pCS->pCipherAlgo->getFieldFunc(Hash_Size));

    offset = (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
    MOC_MEMCPY(offset + pSSLSock->pActiveMaterials, offset + pSSLSock->pMaterials, pCS->keySize);

    if (0 < IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS))
    {
        offset = offset + (2 * pCS->keySize);
        MOC_MEMCPY(offset + pSSLSock->pActiveMaterials, offset + pSSLSock->pMaterials, IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
    }

    /* initialize all the parameters for the cipher */
    resetCipher(pSSLSock, TRUE, FALSE);

#ifdef __ENABLE_HARDWARE_ACCEL_CRYPTO__
    if (NULL == (pSSLSock->clientMACSecret = MALLOC_ALIGN(pCS->pCipherAlgo->getFieldFunc(Hash_Size), hwCryptoMac)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pSSLSock->clientMACSecret, pSSLSock->pActiveMaterials, pCS->pCipherAlgo->getFieldFunc(Hash_Size));

    keyStart = pSSLSock->pActiveMaterials + (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
#ifdef __ENABLE_MOCANA_EAP_FAST__
    keyStartFast = pSSLSock->pMaterials + (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
#endif
    pSSLSock->clientBulkCtx = pCS->pCipherAlgo->createCtxFunc(MOC_SYM(pSSLSock->hwAccelCookie) keyStart, pCS->keySize, pSSLSock->server ? FALSE : TRUE);

    /* initialize pointers to IV if used */
    if (IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS) > 0)
    {
        if (NULL == (pSSLSock->clientIV = MALLOC_ALIGN(IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS), hwCryptoIV)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMCPY(pSSLSock->clientIV, keyStart + (2 * pCS->keySize),IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize) + (2 * IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
    else
    {
        pSSLSock->clientIV = 0;
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize);
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
#else

    /* initialize pointers to MAC secrets */
    pSSLSock->clientMACSecret = pSSLSock->pActiveMaterials;

    /* initialize bulkCtx */
    keyStart = pSSLSock->clientMACSecret + (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
#ifdef __ENABLE_MOCANA_EAP_FAST__
    keyStartFast = pSSLSock->pMaterials + (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
#endif
    pSSLSock->clientBulkCtx = pCS->pCipherAlgo->createCtxFunc(MOC_SYM(pSSLSock->hwAccelCookie) keyStart, pCS->keySize, pSSLSock->server ? FALSE : TRUE);

    if (NULL == pSSLSock->clientBulkCtx)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* initialize pointers to IV if used */
    if (IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS))
    {
        pSSLSock->clientIV = keyStart + (2 * pCS->keySize);
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize) + (2 * IMPLICIT_IV_SIZE(pSSLSock->sslMinorVer, pCS));
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
    else
    {
        pSSLSock->clientIV = 0;
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize);
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
#endif

#ifdef __ENABLE_ALL_DEBUGGING__
    RTOS_sleepMS(200);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"client MAC secret");
    PrintBytes( pSSLSock->clientMACSecret, pCS->pCipherAlgo->getFieldFunc(Hash_Size));
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"client key");
    PrintBytes( keyStart, pCS->keySize);

    if ( pSSLSock->clientIV)
    {
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"client IV");
        PrintBytes( pSSLSock->clientIV, IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
    }
#ifdef __ENABLE_MOCANA_EAP_FAST__
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"Session Key Seed");
    PrintBytes( pSSLSock->sessionKeySeed, SKS_SIZE);
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"CHAP CHALLENGES");
    PrintBytes( pSSLSock->fastChapChallenge, FAST_MSCHAP_CHAL_SIZE);
#endif
#endif

#ifdef __ENABLE_MOCANA_INNER_APP__
    /* Initialize innerSecret to Master Secret */
    MOC_MEMCPY(pSSLSock->innerSecret, pSSLSock->pSecretAndRand, SSL_INNER_SECRET_SIZE);
#endif

    /* on the client, the master secret is always at pSSLSock->pSecretAndRand */

exit:
    return status;

} /* SSL_SOCK_setClientKeyMaterial */


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_setServerKeyMaterial(SSLSocket *pSSLSock)
{
    CipherSuiteInfo*    pCS = pSSLSock->pHandshakeCipherSuite;
    ubyte*              keyStart;
#ifdef __ENABLE_MOCANA_EAP_FAST__
    ubyte*              keyStartFast;
#endif
    sbyte4              offset;
    MSTATUS             status = OK;

    /* dup client key material */
    MOC_MEMCPY(pCS->pCipherAlgo->getFieldFunc(Hash_Size) + pSSLSock->pActiveMaterials, pCS->pCipherAlgo->getFieldFunc(Hash_Size) + pSSLSock->pMaterials, pCS->pCipherAlgo->getFieldFunc(Hash_Size));

    offset = (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
    MOC_MEMCPY(pCS->keySize + offset + pSSLSock->pActiveMaterials, pCS->keySize + offset + pSSLSock->pMaterials, pCS->keySize);

    if (0 < IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS))
    {
        offset = offset + (2 * pCS->keySize) + IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS);
        MOC_MEMCPY(offset + pSSLSock->pActiveMaterials, offset + pSSLSock->pMaterials, IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
    }

    /* initialize all the parameters for the cipher */
    resetCipher(pSSLSock, FALSE, TRUE);

#ifdef __ENABLE_HARDWARE_ACCEL_CRYPTO__
    if (NULL == (pSSLSock->serverMACSecret = MALLOC_ALIGN(pCS->pCipherAlgo->getFieldFunc(Hash_Size), hwCryptoMac)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pSSLSock->serverMACSecret, pSSLSock->pActiveMaterials + pCS->pCipherAlgo->getFieldFunc(Hash_Size), pCS->pCipherAlgo->getFieldFunc(Hash_Size));

    keyStart = pSSLSock->pActiveMaterials + (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
#ifdef __ENABLE_MOCANA_EAP_FAST__
    keyStartFast = pSSLSock->pMaterials + (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
#endif

    pSSLSock->serverBulkCtx = pCS->pCipherAlgo->createCtxFunc(MOC_SYM(pSSLSock->hwAccelCookie) keyStart + pCS->keySize, pCS->keySize, pSSLSock->server ? TRUE : FALSE);

    /* initialize pointers to IV if used */
    if (IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS) > 0)
    {
        if (NULL == (pSSLSock->serverIV= MALLOC_ALIGN(IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS), hwCryptoIV)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMCPY(pSSLSock->serverIV, keyStart + (2 * pCS->keySize) + IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS),
                   IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize) + (2 * IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
    else
    {
        pSSLSock->serverIV = 0;
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize);
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
#else

    /* initialize pointers to MAC secrets */
    pSSLSock->serverMACSecret = pSSLSock->pActiveMaterials + pCS->pCipherAlgo->getFieldFunc(Hash_Size);

    /* initialize bulkCtx */
    keyStart = pSSLSock->serverMACSecret + pCS->pCipherAlgo->getFieldFunc(Hash_Size);
#ifdef __ENABLE_MOCANA_EAP_FAST__
    keyStartFast = pSSLSock->pMaterials + (2 * pCS->pCipherAlgo->getFieldFunc(Hash_Size));
#endif

    pSSLSock->serverBulkCtx = pCS->pCipherAlgo->createCtxFunc(MOC_SYM(pSSLSock->hwAccelCookie) keyStart + pCS->keySize, pCS->keySize, pSSLSock->server ? TRUE : FALSE);

    if (NULL == pSSLSock->serverBulkCtx)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* initialize pointers to IV if used */
    if (0 < IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS))
    {
        pSSLSock->serverIV = keyStart + (2 * pCS->keySize) + IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS);
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize) + (2 * IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
    else
    {
        pSSLSock->serverIV = 0;
#ifdef __ENABLE_MOCANA_EAP_FAST__
        pSSLSock->sessionKeySeed = keyStartFast + (2 * pCS->keySize);
        pSSLSock->fastChapChallenge = pSSLSock->sessionKeySeed + SKS_SIZE;
#endif
    }
#endif

#ifdef __ENABLE_ALL_DEBUGGING__
    RTOS_sleepMS(200);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"server MAC secret");
    PrintBytes( pSSLSock->serverMACSecret, pCS->pCipherAlgo->getFieldFunc(Hash_Size));
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"server key");
    PrintBytes( keyStart + pCS->keySize, pCS->keySize);

    if ( pSSLSock->serverIV)
    {
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"server IV");
        PrintBytes( pSSLSock->serverIV, IMPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS));
    }
#ifdef __ENABLE_MOCANA_EAP_FAST__
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"Session Key Seed");
    PrintBytes( pSSLSock->sessionKeySeed, 40);
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"CHAP CHALLENGES");
    PrintBytes( pSSLSock->fastChapChallenge, FAST_MSCHAP_CHAL_SIZE);
#endif
#endif

#ifdef __ENABLE_MOCANA_INNER_APP__
    /* Initialize innerSecret to Master Secret */
    MOC_MEMCPY(pSSLSock->innerSecret, pSSLSock->pSecretAndRand, SSL_INNER_SECRET_SIZE);
#endif

exit:
    return status;

} /* SSL_SOCK_setServerKeyMaterial */


/*------------------------------------------------------------------*/
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION

extern MSTATUS
SSL_SOCK_computeSSLMAC(SSLSocket *pSSLSock, ubyte* secret, sbyte4 macSize,
                       ubyte *pSequence, ubyte2 mesgLen,
                       ubyte result[SSL_MAXDIGESTSIZE])
{
    /* Complete Message Format: <sequence counter : 8 bytes><SSL frame : 5 bytes><pMessage -> message to encrypt : (mesgLen bytes)><hmac to insert><Pad><PadLen byte> */
    MD5_CTX*    pMd5Ctx  = NULL;
    shaDescr*   pSha1Ctx = NULL;
    MSTATUS     status;

    if (MD5_DIGESTSIZE == macSize) /* MD5 */
    {
        if (OK <= (status = MEM_POOL_getPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Ctx))))
        {
            if (OK > (status = MD5Init_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx)))
                goto exit;

            if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, secret, macSize)))
                goto exit;

            if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, gHashPad36, SSL_MD5_PADDINGSIZE)))
                goto exit;

            if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, pSequence, mesgLen)))
                goto exit;

            if (OK > (status = MD5Final_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, result)))
                goto exit;

            MD5Init_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx);

            MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, secret, macSize);

            MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, gHashPad5C, SSL_MD5_PADDINGSIZE);

            MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, result, macSize);

            status = MD5Final_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Ctx, result);
        }
    }
    else /* SHA1 */
    {
        if (OK <= (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pSha1Ctx))))
        {
            if (OK > (status = SHA1_initDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx)))
                goto exit;

            if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, secret, macSize)))
                goto exit;

            if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, gHashPad36, SSL_SHA1_PADDINGSIZE)))
                goto exit;

            if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, pSequence, mesgLen)))
                goto exit;

            if (OK > (status = SHA1_finalDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, result)))
                goto exit;

            if (OK > (status = SHA1_initDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx)))
                goto exit;

            if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, secret, macSize)))
                goto exit;

            if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, gHashPad5C, SSL_SHA1_PADDINGSIZE)))
                goto exit;

            if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, result, macSize)))
                goto exit;

            status = SHA1_finalDigest(MOC_HASH(pSSLSock->hwAccelCookie) pSha1Ctx, result);
        }
    }

#ifdef __ENABLE_ALL_DEBUGGING__
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"secret=");
    PrintBytes(secret, macSize);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"complete message=");
    PrintBytes(pSequence, mesgLen);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL Mac=");
    PrintBytes( result, macSize);
#endif

exit:
    MEM_POOL_putPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Ctx));
    MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pSha1Ctx));

    return status;
}
#endif

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_computeTLSMAC(MOC_HASH(SSLSocket *pSSLSock) ubyte* secret,
                       ubyte *pMesg, ubyte2 mesgLen,
                       ubyte *pMesgOpt, ubyte2 mesgOptLen,
                       ubyte result[SSL_MAXDIGESTSIZE], const BulkHashAlgo *pBHAlgo)
{
    MSTATUS status;

    /* Complete Message Format: <sequence counter : 8 bytes><SSL frame : 5 bytes><message to encrypt><hmac to insert><Pad><PadLen byte> */

    status = HmacQuickEx(MOC_HASH(pSSLSock->hwAccelCookie) secret, pBHAlgo->digestSize, pMesg, mesgLen, pMesgOpt, mesgOptLen, result, pBHAlgo);

#ifdef __ENABLE_ALL_DEBUGGING__
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"secret=");
    PrintBytes(secret, pBHAlgo->digestSize);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"complete message=");
    PrintBytes(pMesg, mesgLen);
    PrintBytes(pMesgOpt, mesgOptLen);

    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"TLS Mac=");
    PrintBytes( result, pBHAlgo->digestSize);
#endif

    return status;
}


/*------------------------------------------------------------------*/

static void
resetCipher(SSLSocket* pSSLSock, intBoolean clientSide, intBoolean serverSide)
{
    const struct CipherSuiteInfo* pClientCryptoSuite;
    const struct CipherSuiteInfo* pServerCryptoSuite;
#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
    intBoolean   isSetKey = (clientSide && serverSide) ? FALSE : TRUE;
#endif

    if (0 == pSSLSock->server)
    {
        pClientCryptoSuite = pSSLSock->pActiveOwnCipherSuite;
        pServerCryptoSuite = pSSLSock->pActivePeerCipherSuite;
    }
    else
    {
        pClientCryptoSuite = pSSLSock->pActivePeerCipherSuite;
        pServerCryptoSuite = pSSLSock->pActiveOwnCipherSuite;
    }

    if (NULL == pClientCryptoSuite)
        clientSide = FALSE;

    if (NULL == pServerCryptoSuite)
        serverSide = FALSE;

    if ((clientSide) && (pSSLSock->clientBulkCtx))
    {
#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
        if (isSetKey && pSSLSock->isDTLS && !pSSLSock->server)
        {
            /* make sure we delete the ctx when closing connection */
            pSSLSock->retransCipherInfo.deleteOldBulkCtx = TRUE;
        } else
#endif
        {
            pClientCryptoSuite->pCipherAlgo->deleteCtxFunc(MOC_SYM(pSSLSock->hwAccelCookie) &pSSLSock->clientBulkCtx);
        }

        pSSLSock->clientBulkCtx = 0;
    }

    if ((serverSide) && (pSSLSock->serverBulkCtx))
    {
#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
        if (isSetKey && pSSLSock->isDTLS && pSSLSock->server)
        {
            /* make sure we delete the ctx when closing connection */
            pSSLSock->retransCipherInfo.deleteOldBulkCtx = TRUE;
        } else
#endif
        {
            pServerCryptoSuite->pCipherAlgo->deleteCtxFunc(MOC_SYM(pSSLSock->hwAccelCookie) &pSSLSock->serverBulkCtx);
        }

        pSSLSock->serverBulkCtx = 0;
    }

#ifdef __ENABLE_HARDWARE_ACCEL_CRYPTO__
    if ((clientSide) && (pSSLSock->clientMACSecret))
    {
        FREE_ALIGN(pSSLSock->clientMACSecret, hwCryptoMac);
        pSSLSock->clientMACSecret = 0;
    }

    if ((serverSide) && (pSSLSock->serverMACSecret))
    {
        FREE_ALIGN(pSSLSock->serverMACSecret, hwCryptoMac);
        pSSLSock->serverMACSecret = 0;
    }

    if ((clientSide) && (pSSLSock->clientIV))
    {
        FREE_ALIGN(pSSLSock->clientIV, hwCryptoIV);
        pSSLSock->clientIV = 0;
    }

    if ((serverSide) && (pSSLSock->serverIV))
    {
        FREE_ALIGN(pSSLSock->serverIV, hwCryptoIV);
        pSSLSock->serverIV = 0;
    }
#endif
}


/*------------------------------------------------------------------*/

#if 0 /* original --- don't remove. we might include again... jab */
static MSTATUS
SSL_SOCK_receiveRecord(SSLSocket* pSSLSock, SSLRecordHeader* pSRH,
                       ubyte **ppPacketPayload, ubyte4 *pPacketLength)
{
    MSTATUS status;

    if (SSL_ASYNC_RECEIVE_RECORD_2 == SSL_RX_RECORD_STATE(pSSLSock))
        goto nextState;

    status = recvAll(pSSLSock, (sbyte *)pSRH, sizeof(SSLRecordHeader),
                     SSL_ASYNC_RECEIVE_RECORD_1, SSL_ASYNC_RECEIVE_RECORD_2,
                     ppPacketPayload, pPacketLength);

    /* check for errors and no state change */
    if ((OK > status) || (SSL_ASYNC_RECEIVE_RECORD_1 == SSL_RX_RECORD_STATE(pSSLSock)))
        goto exit;

    status = ERR_SSL_PROTOCOL_RECEIVE_RECORD;

#if 0
    /* verify versions */
    if ((pSRH->majorVersion != SSL3_MAJORVERSION) ||
        (pSRH->minorVersion != pSSLSock->sslMinorVersion))
        goto exit;
#endif

    /* get the size */
    pSSLSock->recordSize = getShortValue(pSRH->recordLength);
    if ((SSL_MAX_RECORDSIZE < pSSLSock->recordSize) || (0 > pSSLSock->recordSize))
    {
#ifdef SSL_MESG_TOO_LONG_COUNTER
        if (SSL_MAX_RECORDSIZE < pSSLSock->recordSize)
        {
            SSL_MESG_TOO_LONG_COUNTER(1);        /* increment counter by 1 */
        }
#endif

        /* buffer overrun (attack?) */
        goto exit;
    }

    /* grow buffer support here */
    if (OK > (status = checkBuffer(pSSLSock, pSSLSock->recordSize)))
        goto exit;

nextState:
    status = recvAll(pSSLSock, pSSLSock->pReceiveBuffer, pSSLSock->recordSize,
                     SSL_ASYNC_RECEIVE_RECORD_2, SSL_ASYNC_RECEIVE_RECORD_COMPLETED,
                     ppPacketPayload, pPacketLength);

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL_SOCK_receiveRecord() returns status = ", status);
#endif

    return status;
}
#endif


/*------------------------------------------------------------------*/

static MSTATUS
SSL_SOCK_receiveV23Record(SSLSocket* pSSLSock, ubyte* pSRH,
                          ubyte **ppPacketPayload, ubyte4 *pPacketLength)
{
    MSTATUS status;
    ubyte4 sizeofRecordHeader;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    ubyte pSeqNum[8];
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
    } else
#endif
    {
        sizeofRecordHeader = sizeof(SSLRecordHeader);
    }

    if (SSL_ASYNC_RECEIVE_RECORD_2 == SSL_RX_RECORD_STATE(pSSLSock))
        goto nextState;

    status = recvAll(pSSLSock, (sbyte *)pSRH, sizeofRecordHeader,
                     SSL_ASYNC_RECEIVE_RECORD_1, SSL_ASYNC_RECEIVE_RECORD_2,
                     ppPacketPayload, pPacketLength);

    /* check for errors and no state change */
    if ((OK > status) || (SSL_ASYNC_RECEIVE_RECORD_1 == SSL_RX_RECORD_STATE(pSSLSock)))
        goto exit;

    status = ERR_SSL_PROTOCOL_RECEIVE_RECORD;

    if (128 == (((SSLClientHelloV2 *)pSRH)->record.recordType))
    {
        /* handle SSLv2 clientHello record */
        pSSLSock->recordSize = ((SSLClientHelloV2 *)pSRH)->record.recordLen;

        if (3 > pSSLSock->recordSize)
            goto exit;

        pSSLSock->recordSize = pSSLSock->recordSize - 3;
    }
    else
    {
        /* get the size */
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            pSSLSock->recordSize = getShortValue(((DTLSRecordHeader*)pSRH)->recordLength);
        } else
#endif
        {
            pSSLSock->recordSize = getShortValue(((SSLRecordHeader*)pSRH)->recordLength);
        }

        if ((SSL_MAX_RECORDSIZE < pSSLSock->recordSize) || (0 > pSSLSock->recordSize))
        {
#ifdef SSL_MESG_TOO_LONG_COUNTER
            if (SSL_MAX_RECORDSIZE < pSSLSock->recordSize)
               {
                   SSL_MESG_TOO_LONG_COUNTER(1);        /* increment counter by 1 */
               }
#endif

            /* buffer overrun (attack?) */
            goto exit;
        }
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            /* in DTLS, the record carries peerSeqnum explicitly */
            MOC_MEMCPY(pSeqNum, (ubyte*)((DTLSRecordHeader*)pSRH)->epoch, 8);
            if ((ubyte2)(DTLS_PEEREPOCH(pSSLSock) + 1) == (((DTLSRecordHeader*)pSRH)->epoch[0] << 8 | ((DTLSRecordHeader*)pSRH)->epoch[1]))
            {
                pSSLSock->shouldChangeCipherSpec = TRUE;
            }
            pSSLSock->peerSeqnumHigh = (ubyte4) ((((ubyte4)(pSeqNum[0])) << 24) | (((ubyte4)(pSeqNum[1])) << 16) | (((ubyte4)(pSeqNum[2])) << 8) | ((ubyte4)(pSeqNum[3])));
            pSSLSock->peerSeqnum = (ubyte4)((((ubyte4)(pSeqNum[4])) << 24) | (((ubyte4)(pSeqNum[5])) << 16) | (((ubyte4)(pSeqNum[6])) << 8) | ((ubyte4)(pSeqNum[7])));
        }
#endif
    }
    /* grow buffer support here */
    if (OK > (status = checkBuffer(pSSLSock, pSSLSock->recordSize)))
        goto exit;

nextState:
    status = recvAll(pSSLSock, pSSLSock->pReceiveBuffer, pSSLSock->recordSize,
                     SSL_ASYNC_RECEIVE_RECORD_2, SSL_ASYNC_RECEIVE_RECORD_COMPLETED,
                     ppPacketPayload, pPacketLength);

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL_SOCK_receiveV23Record() returns status = ", status);
#endif

    return status;

} /* SSL_SOCK_receiveV23Record */

/*------------------------------------------------------------------*/
/*
    Generic SSL encryption+hash algorithm implementation
*/

static BulkCtx
SSLComboCipher_createCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, ubyte4 keySize, sbyte4 encrypt, const ComboAlgo *pComboAlgo)
{
    return pComboAlgo->pBEAlgo->createFunc(MOC_SYM(hwAccelCtx) key, keySize, encrypt);
}

static MSTATUS
SSLComboCipher_deleteCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx, const ComboAlgo *pComboAlgo)
{
    return pComboAlgo->pBEAlgo->deleteFunc(MOC_SYM(hwAccelCtx) ctx);
}

static MSTATUS
SSLComboCipher_decryptVerifyRecord(SSLSocket* pSSLSock, ubyte protocol, const ComboAlgo *pComboAlgo)
{
    ubyte*              header;
    ubyte4              headerLength;
    ubyte*              pData;
    ubyte4              dataLength;
    ubyte*              macSecret = pSSLSock->server ? pSSLSock->clientMACSecret : pSSLSock->serverMACSecret;
    BulkCtx             ctx = pSSLSock->server ? pSSLSock->clientBulkCtx : pSSLSock->serverBulkCtx;
    ubyte*              pIV = pSSLSock->server ?     pSSLSock->clientIV : pSSLSock->serverIV ;
    ubyte*              pMacOut = NULL;
    ubyte4              paddingLength;
    ubyte               padChar;
    ubyte4              padLoop;
    ubyte*              pSeqNum=0;
    ubyte4              seqNumHigh;
    ubyte4              seqNum;
    MSTATUS             status = OK;
    byteBoolean         isTLS11Compatible;
    ubyte4              explicitIVLen;
    ubyte               nilIV[64];
    ubyte4              sizeofRecordHeader;
#if defined(__SSL_SINGLE_PASS_SUPPORT__)
    sbyte4              result = -1;
    CipherSuiteInfo*    pCipherSuite = pSSLSock->pActivePeerCipherSuite;
#endif

    pData = (ubyte *)(pSSLSock->pReceiveBuffer);
    dataLength = pSSLSock->recordSize;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        isTLS11Compatible = TRUE;
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
    } else
#endif
    {
        isTLS11Compatible = (pSSLSock->sslMinorVersion >= TLS11_MINORVERSION);
        sizeofRecordHeader = sizeof(SSLRecordHeader);
    }

    explicitIVLen = (isTLS11Compatible? pComboAlgo->pBEAlgo->blockSize : 0);

    pMacOut = (ubyte *)(pData + dataLength);  /* buffer has padding for a second hash */

    if (isTLS11Compatible)
    {
        MOC_MEMSET(nilIV, 0x00, 64);
        pIV = nilIV;
    }
    if ((dataLength < (pComboAlgo->pBHAlgo->digestSize + 1)) ||
        ((pComboAlgo->pBEAlgo->blockSize) && (0 != (dataLength % pComboAlgo->pBEAlgo->blockSize))) )
    {
        /* someone mucked with the clear text record size field, */
        /* not revealing anything by breaking out early */
        status = ERR_SSL_CRYPT_BLOCK_SIZE;
        goto exit;
    }

    if (!pSSLSock->isDTLS)
    {
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        /* buffer has leading pad */
        if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
        {
            pSeqNum    = (ubyte *)(pData - (3 /* (proto: byte) + (length: 2 byte) */ + SSL_HASH_SEQUENCE_SIZE));
            pSeqNum[SSL_HASH_SEQUENCE_SIZE] = protocol;
            header = pSeqNum;
            headerLength = 3 /* (proto: byte) + (length: 2 byte) */ + SSL_HASH_SEQUENCE_SIZE;
        }
        else
#endif
        {
            pSeqNum    = (ubyte *)(pData - (sizeofRecordHeader + SSL_HASH_SEQUENCE_SIZE));
            header = pSeqNum;
            headerLength = sizeofRecordHeader + SSL_HASH_SEQUENCE_SIZE;
        }

        seqNumHigh = pSSLSock->peerSeqnumHigh;
        seqNum     = pSSLSock->peerSeqnum;

        pSeqNum[0]  = (ubyte)(seqNumHigh >> 24);
        pSeqNum[1]  = (ubyte)(seqNumHigh >> 16);
        pSeqNum[2]  = (ubyte)(seqNumHigh >> 8);
        pSeqNum[3]  = (ubyte)(seqNumHigh);
        pSeqNum[4]  = (ubyte)(seqNum >> 24);
        pSeqNum[5]  = (ubyte)(seqNum >> 16);
        pSeqNum[6]  = (ubyte)(seqNum >> 8);
        pSeqNum[7]  = (ubyte)(seqNum);

        if (0 == (++pSSLSock->peerSeqnum))
            pSSLSock->peerSeqnumHigh++;
    } else
    {
        header = pData - sizeofRecordHeader;
        headerLength = sizeofRecordHeader;
    }

/* TODO: can't do single pass with tls1.1 unless chip supports tls1.1: due to data structure change */
#if defined(__SSL_SINGLE_PASS_SUPPORT__)
    if (NO_SINGLE_PASS != pCipherSuite->sslSinglePassInCookie)
    {
        ubyte4 verified = 0;

#if defined(__SSL_SINGLE_PASS_DECRYPT_ADJUST_SSL_RECORD_SIZE_SUPPORT__)
        /* some chips have a bug in which they do not process the SSL record size correctly for hmac calculations */
        /* we could have hidden this code in hw abstraction layer, but that would have been problematic for SSL-CC. JAB */
        if (pComboAlgo->pBEAlgo->blockSize)
        {
            ubyte*  pDecryptBlock = pSSLSock->pReceiveBuffer + pSSLSock->recordSize - pComboAlgo->pBEAlgo->blockSize;
            ubyte2  adjustSum;

            /* this is only a problem for block ciphers */
            if (OK > (status = (MSTATUS)HWOFFLOAD_doQuickBlockDecrypt(MOC_SYM_HASH(pSSLSock->hwAccelCookie) pCipherSuite->sslSinglePassInCookie,
                                                                      pSSLSock->server ? pSSLSock->clientBulkCtx : pSSLSock->serverBulkCtx,  /* key */
                                                                      pDecryptBlock - pComboAlgo->pBEAlgo->blockSize,                      /* incoming iv */
                                                                      pComboAlgo->pBEAlgo->blockSize,                                      /* iv length */
                                                                      pDecryptBlock,                                                         /* decrypt last block */
                                                                      pMacOut)))                                                             /* outgoing iv / original last block of data */
            {
                goto exit;
            }

            /* we can now access the pad length */
            paddingLength = pDecryptBlock[pComboAlgo->pBEAlgo->blockSize - 1];
            adjustSum     = (paddingLength + pComboAlgo->pBHAlgo->digestSize + 1);

            if (adjustSum <= pSSLSock->recordSize)
            {
                /* paddingLength looks acceptable --- we want to adjust the record length in the buffer */
                setShortValue((((SSLRecordHeader *)pSSLSock->pReceiveBuffer) - 1)->recordLength, (ubyte2)(pSSLSock->recordSize - adjustSum));
            }

            /* put the encrypted block back over the last block */
            MOC_MEMCPY(pDecryptBlock, pMacOut, pComboAlgo->pBEAlgo->blockSize);
        }
#endif /* __SSL_SINGLE_PASS_DECRYPT_ADJUST_SSL_RECORD_SIZE_SUPPORT__ */

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        /* do single pass decryption */
        if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
        {
            status = HWOFFLOAD_doSinglePassDecryption(MOC_SYM_HASH(pSSLSock->hwAccelCookie) MOCANA_SSL,
                                                      pCipherSuite->sslSinglePassInCookie, SSL3_MINORVERSION,
                                                      pSSLSock->server ? pSSLSock->clientBulkCtx : pSSLSock->serverBulkCtx,
                                                      pSSLSock->server ? pSSLSock->clientMACSecret : pSSLSock->serverMACSecret,
                                                      pComboAlgo->pBHAlgo->digestSize,
                                                      pSeqNum, pSSLSock->recordSize + (3 + SSL_HASH_SEQUENCE_SIZE),
                                                      pSSLSock->pReceiveBuffer, pSSLSock->recordSize,
                                                      NULL,
                                                      pSSLSock->server ? pSSLSock->clientIV : pSSLSock->serverIV,
                                                      pComboAlgo->pBEAlgo->blockSize,
                                                      pMacOut, pComboAlgo->pBHAlgo->digestSize,
                                                      &verified);
        }
        else
#endif
        {
            status = HWOFFLOAD_doSinglePassDecryption(MOC_SYM_HASH(pSSLSock->hwAccelCookie) MOCANA_SSL,
                                                      pCipherSuite->sslSinglePassInCookie, pSSLSock->sslMinorVersion,
                                                      pSSLSock->server ? pSSLSock->clientBulkCtx : pSSLSock->serverBulkCtx,
                                                      pSSLSock->server ? pSSLSock->clientMACSecret : pSSLSock->serverMACSecret,
                                                      pComboAlgo->pBHAlgo->digestSize,
                                                      pSeqNum, SSL_HASH_SEQUENCE_SIZE + sizeof(SSLRecordHeader) + pSSLSock->recordSize,
                                                      pSSLSock->pReceiveBuffer, pSSLSock->recordSize,
                                                      NULL,
                                                      pSSLSock->server ? pSSLSock->clientIV : pSSLSock->serverIV,
                                                      pComboAlgo->pBEAlgo->blockSize,
                                                      pMacOut, pComboAlgo->pBHAlgo->digestSize,
                                                      &verified);
        }

        if (0 < pComboAlgo->pBEAlgo->blockSize)
        {
            /* to prevent timing attacks go through all verifications
                even if there is an error in some of them */
            paddingLength = ((ubyte4)(pSSLSock->pReceiveBuffer[pSSLSock->recordSize - 1])) & 0xff;

            if (!(HW_OFFLOAD_PAD_VERIFIED & verified))
            {
                /* padding is (padding[paddingLength] + paddingLength) */
                if (((paddingLength + 1) > (pSSLSock->recordSize - pComboAlgo->pBHAlgo->digestSize)) ||
                    ((SSL3_MINORVERSION == pSSLSock->sslMinorVersion) && (paddingLength >= (sbyte)(pComboAlgo->pBEAlgo->blockSize))) )
                {
                    /* only SSLv3 pad length must be shorter than the block/iv size */
                    if (OK <= status)
                        status = ERR_SSL_INVALID_PADDING;

                    paddingLength = 0;
                }

                padChar = (ubyte)paddingLength;

                if ((TLS10_MINORVERSION <= pSSLSock->sslMinorVersion) && (0 < paddingLength))
                {
                    /* verify padding is filled w/ padChar */
                    for (padLoop = 1; padLoop < (paddingLength + 1); padLoop++)
                        if (padChar != (ubyte)pSSLSock->pReceiveBuffer[pSSLSock->recordSize - padLoop])
                            if (OK <= status)
                                status = ERR_SSL_INVALID_PADDING;
                }
            }

            if (OK <= status)
                pSSLSock->recordSize -= (1 + paddingLength);
        }

        /* remove MAC size from size of message */
        if (pSSLSock->recordSize >= pComboAlgo->pBHAlgo->digestSize)
        {
            pSSLSock->recordSize -= pComboAlgo->pBHAlgo->digestSize;
        }
        else
        {
            /* runt message */
            status = ERR_SSL_INVALID_MAC;
        }

        if (!(HW_OFFLOAD_MAC_VERIFIED & verified))
        {
            if ((OK > MOC_MEMCMP(pMacOut, (ubyte *)(pSSLSock->pReceiveBuffer + pSSLSock->recordSize),
                                 pComboAlgo->pBHAlgo->digestSize, &result)) ||
                (0 != result))
            {
                if (OK <= status)
                    status = ERR_SSL_INVALID_MAC;
            }
        }

        goto exit;
    }
#endif /* __SSL_SINGLE_PASS_SUPPORT__ */

    /* decrypt the received record in place */
    status = pComboAlgo->pBEAlgo->cipherFunc(MOC_SYM(pSSLSock->hwAccelCookie)
                    ctx,
                    (ubyte *)pData,
                    dataLength,
                    0, /* decrypt */
                    pIV);

    if (OK > status)
    {
        /* someone mucked with the clear text record size field, or hardware failure */
        /* not revealing anything by breaking out early */
        goto exit;
    }

    if (0 < pComboAlgo->pBEAlgo->blockSize)
    {
        /* to prevent timing attacks go through all verifications
            even if there is an error in some of them */
        paddingLength = ((ubyte4)(pData[dataLength - 1])) & 0xff;

        /* padding is (padding[paddingLength] + paddingLength) */
        if (((paddingLength + 1) > (dataLength - pComboAlgo->pBHAlgo->digestSize)) ||
            ((SSL3_MINORVERSION == pSSLSock->sslMinorVersion) && (paddingLength >= (sbyte)(pComboAlgo->pBEAlgo->blockSize))) )
        {
            /* only SSLv3 pad length must be shorter than the block/iv size */
            if (OK <= status)
                status = ERR_SSL_INVALID_PADDING;

            paddingLength = 0;
        }

        padChar = (ubyte)paddingLength;

        if ((TLS10_MINORVERSION <= pSSLSock->sslMinorVersion) && (0 < paddingLength))
        {
            /* verify padding is filled w/ padChar */
            for (padLoop = 1; padLoop < (paddingLength + 1); padLoop++)
                if (padChar != (ubyte)pData[dataLength - padLoop])
                    if (OK <= status)
                        status = ERR_SSL_INVALID_PADDING;
        }

	/* Lucky13 protection -- loop over the rest of potential padding */
	{
	    ubyte* tmpData = MALLOC(256);
	    ubyte4 tmpDataLength = dataLength;
	    MSTATUS tempStatus = status;

        if (NULL == tmpData)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

	    dataLength = 255;
	    for (padLoop = paddingLength; padLoop < 256; padLoop++)
	        if (padChar != tmpData[dataLength - padLoop])
	            if (OK <= tempStatus)
		        tempStatus = ERR_SSL_INVALID_PADDING;

	    dataLength = tmpDataLength;
	    FREE(tmpData);

	    /* Make sure we perform the computation regardless */
	    if (OK <= status)
	        dataLength -= (1 + paddingLength);
	    else
	        tmpDataLength -= (1 + paddingLength);
	}
    }

    /* remove MAC size and explicitIVLen from size of message */
    if (dataLength >= (pComboAlgo->pBHAlgo->digestSize + explicitIVLen))
    {
        dataLength -= (pComboAlgo->pBHAlgo->digestSize + explicitIVLen);
    }
    else
    {
        /* runt message */
        status = ERR_SSL_INVALID_MAC;
    }

    /* overwrite with adjusted value, prior to mac calculation */
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        setShortValue((((DTLSRecordHeader *)pData) - 1)->recordLength, (ubyte2)dataLength);
    } else
#endif
    {
        setShortValue((((SSLRecordHeader *)pData) - 1)->recordLength, (ubyte2)dataLength);
    }

    /* verify MAC of message (p. 94) */
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        SSL_SOCK_computeSSLMAC(pSSLSock,
                               macSecret,
                               pComboAlgo->pBHAlgo->digestSize,
                               header,
                               (ubyte2)(dataLength + headerLength),
                               pMacOut);
    }
    else
#endif
    {
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            DTLSRecordHeader tmpRecordHeader;

            /*
             * since seq_num comes first when generating TLS MAC, we need to reorder
             * DTLS record header so that epoch + sequence number comes before protocol type
             */
            reorderDTLSRecordHeader((ubyte*)header,
                                    (ubyte*)&tmpRecordHeader);

            SSL_SOCK_computeTLSMAC(MOC_HASH(pSSLSock)
                                   macSecret,
                                   (ubyte*)&tmpRecordHeader,
                                   (ubyte2)headerLength,
                                   (ubyte*)pData + explicitIVLen,
                                   (ubyte2)dataLength,
                                   pMacOut,
                                   pComboAlgo->pBHAlgo);
        } else
#endif
        {
            SSL_SOCK_computeTLSMAC(MOC_HASH(pSSLSock)
                                   macSecret,
                                   header,
                                   (ubyte2) headerLength,
                                   (ubyte*)pData + explicitIVLen,
                                   (ubyte2)dataLength,
                                   pMacOut, pComboAlgo->pBHAlgo);
        }
    }
    {
        /* Lucky13 protection --
	 * make sure that we run HMAC data on all our data */
	sbyte4 L1 = 13 + pSSLSock->recordSize - pComboAlgo->pBHAlgo->digestSize;
	sbyte4 L2 = dataLength + headerLength;   /* L1 - paddingLength - 1; */
	sbyte4 bs = pComboAlgo->pBHAlgo->blockSize;
	sbyte4 compressions = ((L1-(bs-9))+(bs-1))/bs - ((L2-(bs-9))+(bs-1))/bs;
	sbyte4 luckyLen = compressions * bs;
	BulkCtx hashCtxt = NULL;

	if (luckyLen < 0)
	    luckyLen = 0;

	if (OK <= pComboAlgo->pBHAlgo->allocFunc(MOC_HASH(pSSLSock->hwAccelCookie) &hashCtxt) &&
	    OK <= pComboAlgo->pBHAlgo->initFunc(MOC_HASH(pSSLSock->hwAccelCookie) hashCtxt))
	{
	    if (luckyLen > 0)
	        pComboAlgo->pBHAlgo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) hashCtxt, pData, luckyLen);
	    pComboAlgo->pBHAlgo->freeFunc(MOC_HASH(pSSLSock->hwAccelCookie) &hashCtxt);
    }
    }

    /* For Lucky13, make this a constant-time compare */
    {
        ubyte* p1 = pMacOut;
        ubyte* p2 = (ubyte *)(pData + dataLength + explicitIVLen);
        for (padLoop = 0; padLoop < pComboAlgo->pBHAlgo->digestSize; padLoop++)
            if (*(p1++) != *(p2++))
                if (OK <= status)
                    status = ERR_SSL_INVALID_MAC;
    }

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS && OK >= status)
    {
        /* MAC verification succeeded, clear anti-replay window */
        MOC_MEMSET(pSSLSock->replayWindow, 0x0, sizeof(pSSLSock->replayWindow));

        /* update starting sequence number */
        pSSLSock->windowStartSeqnum     = pSSLSock->peerSeqnum;
        pSSLSock->windowStartSeqnumHigh = pSSLSock->peerSeqnumHigh;

        if (0 == (++pSSLSock->windowStartSeqnum))
            pSSLSock->windowStartSeqnumHigh = (pSSLSock->windowStartSeqnumHigh & 0xffff0000) | ((pSSLSock->windowStartSeqnumHigh + 1) & 0xffff);
    }
#endif

    if (isTLS11Compatible)
    {
        /* for TLS1.1 and up, get rid of the explicit IV after the SRH */
        ubyte* pTransfer;
		pTransfer = (ubyte*)pData;
        /* use memmove to improve performance more */
		MOC_MEMMOVE(pTransfer, (pData+explicitIVLen), dataLength);
    }
    pSSLSock->recordSize = dataLength;

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if ((!pSSLSock->isDTLS) && (SSL3_MINORVERSION == pSSLSock->sslMinorVersion))
    {
        /* for SSL 3.0 only */
        /* version information is checked in processClientHello3,
           so we should restore record header; otherwise, rehandshake fails */
        ubyte* pRH = pData - sizeofRecordHeader;

        pRH[0] = protocol;
        pRH[1] = SSL3_MAJORVERSION;
        pRH[2] = SSL3_MINORVERSION;
    }
#endif

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSLComboCipher_decryptVerifyRecord() returns status = ", status);
#endif

    return status;

} /* decryptVerifyRecord */


/*------------------------------------------------------------------*/

static MSTATUS
SSLComboCipher_formEncryptedRecord(SSLSocket* pSSLSock, ubyte* pData, ubyte2 dataLength, sbyte padLength, const ComboAlgo *pComboAlgo)
{
    /* Complete Message Format: <HMAC sequence : 8 bytes><SSL frame : 5 bytes><message to encrypt><hash><Pad><PadLen byte> */
    /* pData points to message to be encrypted.  An empty 16 byte block is before message for sequence number and SSL frame data */
    BulkCtx             ctx = pSSLSock->server ? pSSLSock->serverBulkCtx : pSSLSock->clientBulkCtx;
    ubyte*              pIV = pSSLSock->server ? pSSLSock->serverIV      : pSSLSock->clientIV;
    ubyte*              macSecret = pSSLSock->server ? pSSLSock->serverMACSecret : pSSLSock->clientMACSecret;
    sbyte4              len;
    MSTATUS             status;
    byteBoolean         isTLS11Compatible;
    ubyte4              explicitIVLen;
    ubyte               nilIV[64];
    ubyte4              sizeofRecordHeader;
    ubyte               *header;
    ubyte4              headerLength;

#if defined(__SSL_SINGLE_PASS_SUPPORT__)
    CipherSuiteInfo*    pCS = pSSLSock->pActiveOwnCipherSuite;
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        isTLS11Compatible = TRUE;
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
    } else
#endif
    {
        isTLS11Compatible = (pSSLSock->sslMinorVersion >= TLS11_MINORVERSION);
        sizeofRecordHeader = sizeof(SSLRecordHeader);
    }

    if (NULL == pData)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (isTLS11Compatible)
    {
        MOC_MEMSET(nilIV, 0x00, 64);
        pIV = nilIV;
        explicitIVLen = pComboAlgo->pBEAlgo->blockSize;

        /* for TLS1.1 and up, write the explicit IV at the beginning of pData in the buffer */
        pSSLSock->rngFun(pSSLSock->rngFunArg, explicitIVLen, pData - explicitIVLen);
    } else
    {
        explicitIVLen = 0;
    }

    /* compute mac before encryption */
    /* write mac directly to buffer to avoid an unnecessary copy */
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        header = pData - (SSL_HASH_SEQUENCE_SIZE + 3);
        headerLength = (SSL_HASH_SEQUENCE_SIZE + 3) + dataLength;
    }
    else
#endif
    {
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            header = pData - sizeofRecordHeader - explicitIVLen;
            headerLength = sizeofRecordHeader;
        } else
#endif
        {
            header = pData - (SSL_HASH_SEQUENCE_SIZE + sizeofRecordHeader + explicitIVLen);
            headerLength = (SSL_HASH_SEQUENCE_SIZE + sizeofRecordHeader);
        }
    }

    /* pad if block algo */
    len = dataLength + pComboAlgo->pBHAlgo->digestSize;

    if (pComboAlgo->pBEAlgo->blockSize)
    {
        sbyte padChar = (sbyte)(padLength-1);
        sbyte padLoop;

        for (padLoop = 0; padLoop < padLength; padLoop++)
            pData[len++] = padChar;

    }
    len += explicitIVLen;

    /* TODO: can't do single pass with tls1.1 unless chip supports tls1.1: due to data structure change */
#if defined(__SSL_SINGLE_PASS_SUPPORT__)
    if (NO_SINGLE_PASS != pCS->sslSinglePassOutCookie)
    {
        ubyte4 sslFrameSize = sizeof(SSLRecordHeader);

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
            sslFrameSize = 3;
#endif

        /* do single pass encryption */
        status = HWOFFLOAD_doSinglePassEncryption(MOC_SYM_HASH(pSSLSock->hwAccelCookie) MOCANA_SSL,
                                                  pCS->sslSinglePassOutCookie, pSSLSock->sslMinorVersion,
                                                  ctx,
                                                  pSSLSock->server ? pSSLSock->serverMACSecret : pSSLSock->clientMACSecret,
                                                  pCS->pCipherAlgo->getFieldFunc(Hash_Size),
                                                  pData - (SSL_HASH_SEQUENCE_SIZE + sslFrameSize),
                                                  (SSL_HASH_SEQUENCE_SIZE + sslFrameSize) + dataLength,
                                                  pData, dataLength + pCS->pCipherAlgo->getFieldFunc(Hash_Size) + padLength,
                                                  NULL,
                                                  pIV, pCS->pCipherAlgo->getFieldFunc(Block_Size),
                                                  pCS->pCipherAlgo->getFieldFunc(Hash_Size), padLength);

        goto exit;
    }
#endif

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    /* compute mac before encryption */
    /* write mac directly to buffer to avoid an unnecessary copy */
    if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        status = SSL_SOCK_computeSSLMAC(pSSLSock,
                                        macSecret,
                                        pComboAlgo->pBHAlgo->digestSize,
                                        header,
                                        (ubyte2) headerLength,
                                        pData + dataLength);
    }
    else
#endif
    {
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            DTLSRecordHeader tmpRecordHeader;

            /*
             * since seq_num comes first when generating TLS MAC, we need to reorder
             * DTLS record header so that epoch + sequence number comes before protocol type
             */
            reorderDTLSRecordHeader((ubyte*)header,
                                    (ubyte*)&tmpRecordHeader);

            status = SSL_SOCK_computeTLSMAC(MOC_HASH(pSSLSock)
                                            macSecret,
                                            (ubyte*)&tmpRecordHeader,
                                            headerLength,
                                            pData,
                                            dataLength,
                                            pData + dataLength, /* mac */
                                            pComboAlgo->pBHAlgo);
        } else
#endif
        {
            status = SSL_SOCK_computeTLSMAC(MOC_HASH(pSSLSock)
                                            macSecret,
                                            header,
                                            (ubyte2) headerLength,
                                            pData,
                                            dataLength,
                                            pData + dataLength, /* mac */
                                            pComboAlgo->pBHAlgo);
        }
    }

    if (OK > status)
        goto exit;

    status = pComboAlgo->pBEAlgo->cipherFunc(MOC_SYM(pSSLSock->hwAccelCookie) ctx, pData - explicitIVLen, len, TRUE, pIV);
exit:
    return status;

} /* formEncryptedRecord */

/*------------------------------------------------------------------*/
#if defined(__ENABLE_MOCANA_AEAD_CIPHER__)
/* Generic AEAD cipher implementations */

static BulkCtx

SSLAeadCipher_createCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, ubyte4 keySize, sbyte4 encrypt, const AeadAlgo *pAeadAlgo)
{
    return pAeadAlgo->createFunc(MOC_SYM(hwAccelCtx) key, keySize, encrypt);
}

static MSTATUS
SSLAeadCipher_deleteCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx, const AeadAlgo *pAeadAlgo)
{
    return pAeadAlgo->deleteFunc(MOC_SYM(hwAccelCtx) ctx);
}

static MSTATUS
SSLAeadCipher_decryptVerifyRecord(SSLSocket* pSSLSock, ubyte protocol, const AeadAlgo *pAeadAlgo)
{
    ubyte*              header;
    ubyte*              pData;
    ubyte4              dataLength;
    BulkCtx             ctx = pSSLSock->server ? pSSLSock->clientBulkCtx : pSSLSock->serverBulkCtx;
    ubyte*              pImplicitNonce = pSSLSock->server ?     pSSLSock->clientIV : pSSLSock->serverIV ;
    ubyte*              pSeqNum=0;
    ubyte4              seqNumHigh;
    ubyte4              seqNum;
    MSTATUS             status = OK;
    ubyte*              explicitNonce;
    ubyte4              explicitNonceLen;
    ubyte4              sizeofRecordHeader;
    ubyte               nonce[12]; /* hard code for now */
    ubyte*              aData = NULL;
    ubyte4              aDataLen = 0;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    ubyte               aDataBuf[16]; /* only going to use 11 bytes */
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    DTLSRecordHeader    *pHeader;
    if (pSSLSock->isDTLS)
    {
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
    } else
#endif
    {
        sizeofRecordHeader = sizeof(SSLRecordHeader);
    }

    pData = (ubyte *)(pSSLSock->pReceiveBuffer);
    dataLength = pSSLSock->recordSize;

    explicitNonceLen = pAeadAlgo->explicitNonceSize;
    explicitNonce = pData;

    /* now pData starts from the real data content */
    pData = pData + explicitNonceLen;
    dataLength -= (explicitNonceLen + pAeadAlgo->tagSize);

    if (!pSSLSock->isDTLS)
    {
        ubyte2 tmpRecordSize = getShortValue(((SSLRecordHeader *)(pData - sizeofRecordHeader - explicitNonceLen))->recordLength);
        /* buffer has leading pad */
        pSeqNum    = (ubyte *)(pData - (sizeofRecordHeader + SSL_HASH_SEQUENCE_SIZE + explicitNonceLen));
        /* adjust the record size before decryption: recordSize -= tagSize + explicitIVLen */
        setShortValue(((SSLRecordHeader *)(pData - sizeofRecordHeader - explicitNonceLen))->recordLength, (ubyte2)(tmpRecordSize - explicitNonceLen - pAeadAlgo->tagSize));

        header = pSeqNum;

        aData = header;
        aDataLen = SSL_HASH_SEQUENCE_SIZE + 5;

        seqNumHigh = pSSLSock->peerSeqnumHigh;
        seqNum     = pSSLSock->peerSeqnum;

        pSeqNum[0]  = (ubyte)(seqNumHigh >> 24);
        pSeqNum[1]  = (ubyte)(seqNumHigh >> 16);
        pSeqNum[2]  = (ubyte)(seqNumHigh >> 8);
        pSeqNum[3]  = (ubyte)(seqNumHigh);
        pSeqNum[4]  = (ubyte)(seqNum >> 24);
        pSeqNum[5]  = (ubyte)(seqNum >> 16);
        pSeqNum[6]  = (ubyte)(seqNum >> 8);
        pSeqNum[7]  = (ubyte)(seqNum);

        if (0 == (++pSSLSock->peerSeqnum))
            pSSLSock->peerSeqnumHigh++;
    }
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    else
    {
        ubyte2 tmpRecordSize;
        header = pData - sizeofRecordHeader - explicitNonceLen;

        pHeader = (DTLSRecordHeader*)header;
        /* copy out the a data from the record header */
        MOC_MEMCPY(aDataBuf, pHeader->seqNo, sizeof(pHeader->seqNo));
        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo), &pHeader->protocol, 1);
        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo)+1, &pHeader->majorVersion, 1);
        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo)+1 +1, &pHeader->minorVersion, 1);
        /* adjust the record size before decryption: recordSize -= tagSize + explicitIVLen */
        tmpRecordSize = getShortValue(pHeader->recordLength);

        setShortValue(pHeader->recordLength, (ubyte2)(tmpRecordSize - explicitNonceLen - pAeadAlgo->tagSize));

        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo)+1 +1+ 1, pHeader->recordLength, 2);

        aData = aDataBuf;
        aDataLen = sizeof(pHeader->seqNo) +1 +1 +1 +2;
    }
#endif

    /* initializ nonce= implicitNonce + explicitNonce */
    MOC_MEMCPY(nonce, pImplicitNonce, 4);
    /* the explicitNonce is using the sequence number */
    /*MOC_MEMCPY(explicitNonce, header, SSL_HASH_SEQUENCE_SIZE);*/
    MOC_MEMCPY(nonce+4, explicitNonce, SSL_HASH_SEQUENCE_SIZE);

    /* decrypt the received record in place */
    status = pAeadAlgo->cipherFunc(MOC_SYM(pSSLSock->hwAccelCookie) ctx, nonce, 12, aData, aDataLen, pData, dataLength, pAeadAlgo->tagSize, FALSE);

    if (OK > status)
    {
        /* someone mucked with the clear text record size field, or hardware failure */
        /* not revealing anything by breaking out early */
        goto exit;
    }

    {
        /* for TLS1.1 and up, get rid of the explicit IV after the SRH */
        ubyte* pTransfer;
        sbyte4 i;

        for (pTransfer = (ubyte*)pData-explicitNonceLen, i = 0; i < dataLength; i++, pTransfer++)
        {
            *pTransfer = pData[i];
        }
    }
    pSSLSock->recordSize = dataLength;

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSLComboCipher_decryptVerifyRecord() returns status = ", status);
#endif

    return status;

} /* decryptVerifyRecord */


/*------------------------------------------------------------------*/

static MSTATUS
SSLAeadCipher_formEncryptedRecord(SSLSocket* pSSLSock, ubyte* pData, ubyte2 dataLength, sbyte padLength, const AeadAlgo *pAeadAlgo)
{
    /* Complete Message Format: <HMAC sequence : 8 bytes><SSL frame : 5 bytes><message to encrypt>*/
    /* pData points to message to be encrypted.  An empty 16 byte block is before message for sequence number and SSL frame data */
    BulkCtx             ctx = pSSLSock->server ? pSSLSock->serverBulkCtx : pSSLSock->clientBulkCtx;
    ubyte*              pImplicitNonce = pSSLSock->server ? pSSLSock->serverIV      : pSSLSock->clientIV;
    MSTATUS             status;
    ubyte4              sizeofRecordHeader;
    ubyte*              header;
    ubyte*              explicitNonce;
    ubyte4              explicitNonceLen;
    ubyte               nonce[12];
    ubyte*              aData = NULL;
    ubyte4              aDataLen = 0;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    ubyte               aDataBuf[16]; /* only going to use 11 bytes */
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    DTLSRecordHeader    *pHeader;
    if (pSSLSock->isDTLS)
    {
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
    } else
#endif
    {
        sizeofRecordHeader = sizeof(SSLRecordHeader);
    }

    explicitNonceLen = pAeadAlgo->explicitNonceSize;

    if (NULL == pData)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    explicitNonce = pData - explicitNonceLen;

    /* adata = seq_num + message_type + version + length */
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        header = pData - sizeofRecordHeader - explicitNonceLen;
        pHeader = (DTLSRecordHeader*)header;
        /* copy out the a data from the record header */
        MOC_MEMCPY(aDataBuf, pHeader->seqNo, sizeof(pHeader->seqNo));
        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo), &pHeader->protocol, 1);
        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo)+1, &pHeader->majorVersion, 1);
        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo)+1 +1, &pHeader->minorVersion, 1);
        MOC_MEMCPY(aDataBuf+sizeof(pHeader->seqNo)+1 + 1+ 1, &pHeader->recordLength, 2);

        aData = aDataBuf;
        aDataLen = sizeof(pHeader->seqNo)+1 + 1+ 1 + 2;

        /* the explicitNonce is using the sequence number */
        MOC_MEMCPY(explicitNonce, ((DTLSRecordHeader*) header)->epoch, SSL_HASH_SEQUENCE_SIZE);
    } else
#endif
    {
        header = pData - (SSL_HASH_SEQUENCE_SIZE + sizeofRecordHeader + explicitNonceLen);
        aData = header;
        aDataLen = SSL_HASH_SEQUENCE_SIZE+5;

        /* the explicitNonce is using the sequence number */
        MOC_MEMCPY(explicitNonce, header, SSL_HASH_SEQUENCE_SIZE);
    }

    /* initializ nonce= implicitNonce + explicitNonce */
    MOC_MEMCPY(nonce, pImplicitNonce, 4);
    MOC_MEMCPY(nonce+4, explicitNonce, SSL_HASH_SEQUENCE_SIZE);

    status = pAeadAlgo->cipherFunc(MOC_SYM(pSSLSock->hwAccelCookie) ctx, nonce, 12, aData, aDataLen, pData, dataLength, pAeadAlgo->tagSize, TRUE);

exit:
    return status;

} /* formEncryptedRecord */

#endif /* #if defined(__ENABLE_MOCANA_AEAD_CIPHER__) */

/*------------------------------------------------------------------*/

static MSTATUS
sendDataSSL(SSLSocket* pSSLSock, ubyte protocol, const sbyte* data, sbyte4 dataSize, intBoolean skipEmptyMesg)
{
    ubyte*           pFreeBuffer = NULL;
    ubyte*           pSendBuffer;
    sbyte4           emptyRecordLen;
    sbyte4           chunkLen;
    sbyte4           padLenEmpty = 0;
    sbyte4           padLenMessage;
    ubyte4           sendBufferLen;
    SSLRecordHeader* pRecordHeaderEmpty;
    SSLRecordHeader* pRecordHeaderMessage;
    CipherSuiteInfo* pCS = pSSLSock->pActiveOwnCipherSuite;
    ubyte4           numBytesSent = 0;
    ubyte4           numBytesSocketSent = 0;
    ubyte4           seqNumHigh;
    ubyte4           seqNum;
    ubyte*           pSeqNumEmpty = NULL;
    ubyte*           pSeqNumMessage;
    MSTATUS          status = OK;
    byteBoolean      isTLS11Compatible;
    ubyte4           explicitIVLen;
    ubyte4           hashOrTagLen;

    isTLS11Compatible = (pSSLSock->sslMinorVersion >= TLS11_MINORVERSION);
    explicitIVLen = EXPLICIT_IV_SIZE(pSSLSock->sslMinorVersion, pCS);
    hashOrTagLen = pCS->pCipherAlgo->getFieldFunc(TagLen) == 0? pCS->pCipherAlgo->getFieldFunc(Hash_Size) : pCS->pCipherAlgo->getFieldFunc(TagLen);

    /* compute the maximum data that can be sent in one record */
    chunkLen = SSL_RECORDSIZE - (hashOrTagLen + pCS->pCipherAlgo->getFieldFunc(Block_Size) + explicitIVLen);

    if (chunkLen > dataSize)
    {
        /* no reason to allocate more than required */
        chunkLen = dataSize;
    }

    /* CBCATTACK counter measures is only on if
     * 1. using block cipher;
     * and 2. not TLS1.1 or up
     * and 3. not single record msgs such as changeCipherSuite or Finished
     * and 4. flag is turned on
     */
    if ((!isTLS11Compatible) && pCS->pCipherAlgo->getFieldFunc(Block_Size) && (TRUE != skipEmptyMesg) && (0 != (pSSLSock->runtimeFlags & SSL_FLAG_ENABLE_SEND_EMPTY_FRAME)))
    {
        padLenEmpty    = computePadLength(pCS->pCipherAlgo->getFieldFunc(Hash_Size), pCS->pCipherAlgo->getFieldFunc(Block_Size));
        emptyRecordLen = SSL_MALLOC_BLOCK_SIZE + pCS->pCipherAlgo->getFieldFunc(Hash_Size) + padLenEmpty;       /* this result will be a multiple of the IV */
        sendBufferLen  = pCS->pCipherAlgo->getFieldFunc(Hash_Size) + padLenEmpty + sizeof(SSLRecordHeader);
    }
    else
    {
        emptyRecordLen = 0;
        sendBufferLen  = 0;
    }

    /* enough space for everything */
    if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie,
                                    emptyRecordLen + SSL_MALLOC_BLOCK_SIZE + explicitIVLen + chunkLen +
                                    SSL_MAXDIGESTSIZE + SSL_MAXIVSIZE + SSL_MALLOC_BLOCK_SIZE,
                                    TRUE, (void **)&pFreeBuffer)))
    {
        goto exit;
    }

    /* these fields never change */
    pRecordHeaderEmpty   = (SSLRecordHeader *)((pFreeBuffer + SSL_MALLOC_BLOCK_SIZE) - sizeof(SSLRecordHeader));
    pRecordHeaderMessage = (SSLRecordHeader *)((pFreeBuffer + emptyRecordLen + SSL_MALLOC_BLOCK_SIZE) - sizeof(SSLRecordHeader));

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        pSeqNumMessage = pFreeBuffer + emptyRecordLen + SSL_MALLOC_BLOCK_SIZE - (SSL_HASH_SEQUENCE_SIZE + 3);
    }
    else
#endif
    {
        pSeqNumMessage = ((ubyte *)(pRecordHeaderMessage)) - SSL_HASH_SEQUENCE_SIZE;

        pRecordHeaderMessage->protocol     = protocol;
        pRecordHeaderMessage->majorVersion = SSL3_MAJORVERSION;
        pRecordHeaderMessage->minorVersion = pSSLSock->sslMinorVersion;
    }

    pSendBuffer = (ubyte *)pRecordHeaderMessage;

    if (emptyRecordLen)
    {
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
        {
            pSeqNumEmpty = pFreeBuffer + SSL_MALLOC_BLOCK_SIZE - (SSL_HASH_SEQUENCE_SIZE + 3);
        }
        else
#endif
        {
            pSeqNumEmpty = ((ubyte *)(pRecordHeaderEmpty)) - SSL_HASH_SEQUENCE_SIZE;

            pRecordHeaderEmpty->protocol     = protocol;
            pRecordHeaderEmpty->majorVersion = SSL3_MAJORVERSION;
            pRecordHeaderEmpty->minorVersion = pSSLSock->sslMinorVersion;
        }

        pSendBuffer = ((ubyte *)pRecordHeaderMessage) - (hashOrTagLen + padLenEmpty + sizeof(SSLRecordHeader));
    }

    /* compute pad length for encrypted message */
    padLenMessage = computePadLength(chunkLen + pCS->pCipherAlgo->getFieldFunc(Hash_Size), pCS->pCipherAlgo->getFieldFunc(Block_Size));

    /* set the length of the first n chunk records */
    sendBufferLen += (ubyte4)(sizeof(SSLRecordHeader) + explicitIVLen + chunkLen + hashOrTagLen + padLenMessage);

    while ((status >= OK) && (dataSize > 0))
    {
        if (dataSize < chunkLen)
        {
            if (emptyRecordLen)
                sendBufferLen = hashOrTagLen + padLenEmpty + sizeof(SSLRecordHeader);
            else
                sendBufferLen = 0;

            /* compute pad length for encrypted message */
            padLenMessage = computePadLength(dataSize + pCS->pCipherAlgo->getFieldFunc(Hash_Size), pCS->pCipherAlgo->getFieldFunc(Block_Size));

            sendBufferLen += (ubyte4)(sizeof(SSLRecordHeader) + explicitIVLen + dataSize + hashOrTagLen + padLenMessage);

            chunkLen = dataSize;
        }

        if (emptyRecordLen)
        {
            seqNumHigh = pSSLSock->ownSeqnumHigh;
            seqNum     = pSSLSock->ownSeqnum;

            pSeqNumEmpty[0]  = (ubyte)(seqNumHigh >> 24);
            pSeqNumEmpty[1]  = (ubyte)(seqNumHigh >> 16);
            pSeqNumEmpty[2]  = (ubyte)(seqNumHigh >> 8);
            pSeqNumEmpty[3]  = (ubyte)(seqNumHigh);
            pSeqNumEmpty[4]  = (ubyte)(seqNum >> 24);
            pSeqNumEmpty[5]  = (ubyte)(seqNum >> 16);
            pSeqNumEmpty[6]  = (ubyte)(seqNum >> 8);
            pSeqNumEmpty[7]  = (ubyte)(seqNum);

            pSeqNumEmpty[8]  = (ubyte)(protocol);

            if (0 == (++pSSLSock->ownSeqnum))
                pSSLSock->ownSeqnumHigh++;

            setShortValue(pRecordHeaderEmpty->recordLength, (ubyte2)0);

            /* create an empty record */
            if (OK > (status = pCS->pCipherAlgo->encryptRecordFunc(pSSLSock, ((ubyte *)(pRecordHeaderEmpty + 1)), 0, (sbyte)padLenEmpty)))
                goto exit;

            setShortValue(pRecordHeaderEmpty->recordLength, (ubyte2)(hashOrTagLen + padLenEmpty));
        }

        seqNumHigh = pSSLSock->ownSeqnumHigh;
        seqNum     = pSSLSock->ownSeqnum;

        pSeqNumMessage[0]  = (ubyte)(seqNumHigh >> 24);
        pSeqNumMessage[1]  = (ubyte)(seqNumHigh >> 16);
        pSeqNumMessage[2]  = (ubyte)(seqNumHigh >> 8);
        pSeqNumMessage[3]  = (ubyte)(seqNumHigh);
        pSeqNumMessage[4]  = (ubyte)(seqNum >> 24);
        pSeqNumMessage[5]  = (ubyte)(seqNum >> 16);
        pSeqNumMessage[6]  = (ubyte)(seqNum >> 8);
        pSeqNumMessage[7]  = (ubyte)(seqNum);

        pSeqNumMessage[8]  = (ubyte)(protocol);

        if (0 == (++pSSLSock->ownSeqnum))
            pSSLSock->ownSeqnumHigh++;

        /* duplicate message data */
        if (OK > (status = MOC_MEMCPY(((ubyte *)(pRecordHeaderMessage + 1)) + explicitIVLen, data, chunkLen)))
            goto exit;

        /* set the length to chunkLen prior to mac calc */
        setShortValue(pRecordHeaderMessage->recordLength, (ubyte2)chunkLen);

        /* create real message record */
        if (OK > (status = pCS->pCipherAlgo->encryptRecordFunc(pSSLSock, ((ubyte *)(pRecordHeaderMessage + 1)) + explicitIVLen, (ubyte2)chunkLen, (sbyte)padLenMessage)))
            goto exit;

        setShortValue(pRecordHeaderMessage->recordLength, (ubyte2)(explicitIVLen + chunkLen + hashOrTagLen + padLenMessage));

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
        {
            pRecordHeaderMessage->protocol     = protocol;
            pRecordHeaderMessage->majorVersion = SSL3_MAJORVERSION;
            pRecordHeaderMessage->minorVersion = SSL3_MINORVERSION;
        }
#endif
        if (emptyRecordLen)
        {
            ubyte* pTransfer = ((ubyte *)pRecordHeaderMessage);
            sbyte4 i;

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
            if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
            {
                pRecordHeaderEmpty->protocol     = protocol;
                pRecordHeaderEmpty->majorVersion = SSL3_MAJORVERSION;
                pRecordHeaderEmpty->minorVersion = SSL3_MINORVERSION;
            }
#endif
            /* move empty record byte adjacent to real message */
            for (pTransfer--, i = (sizeof(SSLRecordHeader) + hashOrTagLen + padLenEmpty); 0 < i; i--, pTransfer--)
            {
                *pTransfer = ((ubyte *)pRecordHeaderEmpty)[i - 1];
            }
        }

        data += chunkLen;
        dataSize -= chunkLen;
        numBytesSent += chunkLen;

        /* send real message record */
        if (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)
        {
            if (NULL == pSSLSock->pOutputBufferBase)
            {
                pSSLSock->pOutputBufferBase = pFreeBuffer;
                pSSLSock->pOutputBuffer     = pSendBuffer;
                pSSLSock->outputBufferSize  = emptyRecordLen + SSL_MALLOC_BLOCK_SIZE + explicitIVLen + chunkLen + SSL_MAXDIGESTSIZE + SSL_MAXIVSIZE;
                pSSLSock->numBytesToSend    = sendBufferLen;
                status = pSSLSock->numBytesToSend;

                pFreeBuffer                 = NULL;
                goto exit;
            }
            else
            {
                if (pSSLSock->outputBufferSize <
                             pSSLSock->numBytesToSend + sendBufferLen)
                {
                    /* Need to Realloc this buffer */
                    ubyte * output = MALLOC(pSSLSock->numBytesToSend + sendBufferLen);
                    if (NULL == output)
                    {
                        status = ERR_MEM_ALLOC_FAIL;
                        goto exit;
                    }
                    MOC_MEMCPY(output, pSSLSock->pOutputBuffer, pSSLSock->numBytesToSend);
                    pSSLSock->outputBufferSize  = pSSLSock->numBytesToSend + sendBufferLen;
                    FREE(pSSLSock->pOutputBufferBase);
                    pSSLSock->pOutputBufferBase = output;

                }
                else
                {
                    if (pSSLSock->pOutputBufferBase != pSSLSock->pOutputBuffer)
                        MOC_MEMMOVE(pSSLSock->pOutputBufferBase, pSSLSock->pOutputBuffer, pSSLSock->numBytesToSend);
                }

                pSSLSock->pOutputBuffer      = pSSLSock->pOutputBufferBase + pSSLSock->numBytesToSend;
                MOC_MEMCPY(pSSLSock->pOutputBuffer, pSendBuffer, sendBufferLen);
                pSSLSock->pOutputBuffer      = pSSLSock->pOutputBufferBase;
                pSSLSock->numBytesToSend    += sendBufferLen;
                status = (MSTATUS)pSSLSock->numBytesToSend;
                CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pFreeBuffer);
                pFreeBuffer                 = NULL;
                goto exit;
            }
        }
        else
        {
#ifndef __MOCANA_IPSTACK__
            if (OK > (status = TCP_WRITE(pSSLSock->tcpSock, (sbyte *)pSendBuffer, sendBufferLen, &numBytesSocketSent)))
#else
            if (OK > (status = MOC_TCP_WRITE(pSSLSock->tcpSock, (sbyte *)pSendBuffer, sendBufferLen, &numBytesSocketSent)))
#endif
                goto exit;
        }

        if (numBytesSocketSent != sendBufferLen)
        {
            pSSLSock->pOutputBufferBase = pFreeBuffer;
            pSSLSock->pOutputBuffer     = numBytesSocketSent + pSendBuffer;
            pSSLSock->outputBufferSize  = emptyRecordLen + SSL_MALLOC_BLOCK_SIZE + explicitIVLen + chunkLen + SSL_MAXDIGESTSIZE + SSL_MAXIVSIZE;
            pSSLSock->numBytesToSend    = sendBufferLen - numBytesSocketSent;

            pFreeBuffer                 = NULL;
            goto exit;
        }
    }

exit:
    CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pFreeBuffer);

    /* if successful, return the total number of input bytes sent */
    if (OK == status)
        status = (MSTATUS)numBytesSent;

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"sendData() returns status = ", status);
#endif

    return status;
}

/*------------------------------------------------------------------*/

static MSTATUS
sendData(SSLSocket* pSSLSock, ubyte protocol, const sbyte* data, sbyte4 dataSize, intBoolean skipEmptyMesg)
{
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        return sendDataDTLS(pSSLSock, protocol, data, dataSize, skipEmptyMesg);
    } else
#endif
    {
        return sendDataSSL(pSSLSock, protocol, data, dataSize, skipEmptyMesg);
    }
}

/*------------------------------------------------------------------*/

/*******************************************************************************
*      sendChangeCipherSpec
* see page 72 of SSL and TLS essentials
*/
static MSTATUS
sendChangeCipherSpec(SSLSocket* pSSLSock)
{
    ubyte   ccs[20]; /* this would be adequate for both SSL and DTLS ccs */
    ubyte4  numBytesSent = 0;
    ubyte4  sizeofRecordHeader;
    ubyte4  sizeofCcs;
    MSTATUS status;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        sizeofRecordHeader = sizeof(DTLSRecordHeader);

        /* sendData will set record header if encrypted */
        if (NULL == pSSLSock->pActiveOwnCipherSuite)
        {
            DTLS_SET_RECORD_HEADER_EXT(ccs,pSSLSock,SSL_CHANGE_CIPHER_SPEC,1);
        }
    } else
#endif
    {
        sizeofRecordHeader = sizeof(SSLRecordHeader);
        SSL_SET_RECORD_HEADER(ccs,SSL_CHANGE_CIPHER_SPEC,pSSLSock->sslMinorVersion, 1);
    }

    sizeofCcs = sizeofRecordHeader + 1;

    /* */
    ccs[sizeofRecordHeader] = 0x01;

    /* THIS NEEDS TO BE REFACTORED AS A SINGLE FUNCTION */
    if (NULL == pSSLSock->pActiveOwnCipherSuite)
    {
        if (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)
        {
            numBytesSent = 0;

            if (NULL == pSSLSock->pOutputBufferBase)
            {
                if (NULL == (pSSLSock->pOutputBufferBase = MALLOC(sizeofCcs + TLS_EAP_PAD)))
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }

                pSSLSock->pOutputBuffer      = pSSLSock->pOutputBufferBase;
                pSSLSock->outputBufferSize   = sizeofCcs + TLS_EAP_PAD;
                pSSLSock->numBytesToSend     = 0;
            }

            if ((sizeofCcs + pSSLSock->numBytesToSend) > pSSLSock->outputBufferSize)
            {
                /* Need to Realloc this buffer */
                ubyte * output = MALLOC(pSSLSock->numBytesToSend + sizeofCcs);

                if (NULL == output)
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }

                MOC_MEMCPY(output, pSSLSock->pOutputBufferBase, pSSLSock->numBytesToSend);
                pSSLSock->outputBufferSize  += sizeofCcs;

                FREE(pSSLSock->pOutputBufferBase);
                pSSLSock->pOutputBufferBase = output;

            }
            else
            {
                if (pSSLSock->pOutputBufferBase != pSSLSock->pOutputBuffer)
                    MOC_MEMMOVE(pSSLSock->pOutputBufferBase, pSSLSock->pOutputBuffer, pSSLSock->numBytesToSend);
            }

            pSSLSock->pOutputBuffer   = pSSLSock->pOutputBufferBase + pSSLSock->numBytesToSend;
            MOC_MEMCPY(pSSLSock->pOutputBuffer, ccs, sizeofCcs);

            pSSLSock->numBytesToSend += sizeofCcs;
            pSSLSock->pOutputBuffer   = pSSLSock->pOutputBufferBase;
            status = (MSTATUS)pSSLSock->numBytesToSend;

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
            if (pSSLSock->isDTLS)
            {
                if (OK > (status = addDataToRetransmissionBuffer(pSSLSock, SSL_CHANGE_CIPHER_SPEC,(const sbyte*) ccs, sizeofCcs)))
                    goto exit;
            }
#endif

            goto exit;
        }
        else
        {
#ifndef __MOCANA_IPSTACK__
            if (OK > (status = TCP_WRITE(pSSLSock->tcpSock, (sbyte *)ccs, sizeofCcs, &numBytesSent)))
#else
            if (OK > (status = MOC_TCP_WRITE(pSSLSock->tcpSock, (sbyte *)ccs, sizeof(ccs), &numBytesSent)))
#endif
                goto exit;

            if (sizeofCcs != numBytesSent)
            {
                if (NULL == (pSSLSock->pOutputBufferBase = MALLOC(sizeofCcs)))
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }

                MOC_MEMCPY(pSSLSock->pOutputBufferBase, ccs, sizeofCcs);

                pSSLSock->pOutputBuffer     = numBytesSent + pSSLSock->pOutputBufferBase;
                pSSLSock->outputBufferSize  = sizeofCcs;
                pSSLSock->numBytesToSend    = sizeofCcs - numBytesSent;
            }
        }
    }
    else
    {
        sbyte one[] = { 0x01 };
        sbyte4 sizeOne = sizeof(one);

        status = sendData(pSSLSock, SSL_CHANGE_CIPHER_SPEC, one, sizeOne, TRUE);
        goto exit;
    }

    /* END REFACTOR */

exit:
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        /* save for retransmission */
        pSSLSock->oldSeqnum     = pSSLSock->ownSeqnum;
        pSSLSock->oldSeqnumHigh = pSSLSock->ownSeqnumHigh;

        /* epoch++, seqNo = 0 */
        pSSLSock->ownSeqnum     = 0;
        pSSLSock->ownSeqnumHigh = (pSSLSock->ownSeqnumHigh & 0xffff0000) + 0x00010000;
    } else
#endif
    {
        pSSLSock->ownSeqnum = 0;
        pSSLSock->ownSeqnumHigh = 0;
    }

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
sendFinished(SSLSocket* pSSLSock)
{
    ubyte*              pFinished = NULL;
    ubyte*              pSHSH;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status = OK;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
    }

    /* common initialization for SSL or TLS */
    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pFinished))))
        goto exit;

    pSHSH = pFinished;

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        /* ubyte finished[sizeof(SSLHandshakeHeader) + MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE]; */

        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_FINISHED;
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE);
        calculateSSLTLSHashes(pSSLSock, pSSLSock->server ? 0 : 1, (ubyte *)(pSHSH+sizeofHandshakeHeader), hashTypeSSLv3Finished);
        addToHandshakeHash(pSSLSock, pFinished, sizeofHandshakeHeader + MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE);

        status = sendData(pSSLSock, SSL_HANDSHAKE, (sbyte *)pFinished, sizeofHandshakeHeader + MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE, TRUE);
    }
    else
#endif
    /* TLS */
    {

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_FINISHED;
            setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, TLS_VERIFYDATASIZE);

            DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader*)pSHSH), pSSLSock->nextSendSeq++, TLS_VERIFYDATASIZE);
        } else
#endif
        {
            ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_FINISHED;
            setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, TLS_VERIFYDATASIZE);
        }

        if (OK > (status = calculateTLSFinishedVerify( pSSLSock,  pSSLSock->server ? 0 : 1, (ubyte *)(pSHSH+sizeofHandshakeHeader))))
            goto exit;

        addToHandshakeHash(pSSLSock, pFinished, sizeofHandshakeHeader + TLS_VERIFYDATASIZE);
        status = sendData(pSSLSock, SSL_HANDSHAKE, (sbyte *)pFinished, sizeofHandshakeHeader + TLS_VERIFYDATASIZE, TRUE);
    }

exit:
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pFinished));

    return status;

} /* sendFinished */


/*------------------------------------------------------------------*/

static MSTATUS
SSLSOCK_doOpenUpcalls(SSLSocket* pSSLSock)
{
    MSTATUS             status = OK;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__)) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
    if (pSSLSock->isDTLS)
    {
        if (pSSLSock->useSrtp)
        {
            status = ERR_DTLS_SRTP_CALLBACK_MISSING;
            if(NULL != SSL_sslSettings()->funcPtrSrtpInitCallback)
            {
                if (OK > (status = SSL_sslSettings()->funcPtrSrtpInitCallback(
                                                SSL_findConnectionInstance(pSSLSock),
                                                &pSSLSock->peerDescr, pSSLSock->pHandshakeSrtpProfile,
                                                pSSLSock->pSrtpMaterials, pSSLSock->srtpMki)))
                {
                    goto exit;
                }
            }
        }
    }
#endif

    pSSLSock->handshakeCount++;

    if (0 == pSSLSock->handshakeCount)
    {
        status = ERR_SSL_TOO_MANY_REHANDSHAKES;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__
    if ((IS_SSL_ASYNC(pSSLSock)) && (0 == pSSLSock->server) && (NULL != SSL_sslSettings()->funcPtrClientOpenStateUpcall))
        status = SSL_sslSettings()->funcPtrClientOpenStateUpcall(SSL_findConnectionInstance(pSSLSock), (sbyte4)((1 < pSSLSock->handshakeCount) ? TRUE : FALSE));
#endif

#ifdef __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__
    if ((IS_SSL_ASYNC(pSSLSock)) && (pSSLSock->server) && (NULL != SSL_sslSettings()->funcPtrOpenStateUpcall))
        status = SSL_sslSettings()->funcPtrOpenStateUpcall(SSL_findConnectionInstance(pSSLSock), (sbyte4)((1 < pSSLSock->handshakeCount) ? TRUE : FALSE));
#endif

exit:
    return status;
}


/*------------------------------------------------------------------*/

/*******************************************************************************
*      processFinished
*    see page 91 of SSL and TLS esssentials
*/
static MSTATUS
processFinished(SSLSocket* pSSLSock, ubyte* pSHSH, ubyte2 recLen)
{
    sbyte4  result = -1;
    ubyte4  sizeofHandshakeHeader;
    MSTATUS status;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
    }

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if ( SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        ubyte* pFinishedHashes = NULL;      /* [ MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE] */

        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pFinishedHashes))))
            goto exit;

        calculateSSLTLSHashes(pSSLSock, pSSLSock->server ? 1 : 0, pFinishedHashes, hashTypeSSLv3Finished);

        if ((recLen != MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE) ||
            (OK > MOC_MEMCMP(pFinishedHashes, (ubyte *)(pSHSH+sizeofHandshakeHeader), MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE, &result)) ||
            (0 != result))
        {
            status = ERR_SSL_PROTOCOL_PROCESS_FINISHED;
        }

        MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pFinishedHashes));
    }
    else
#endif
    /* TLS */
    {
        ubyte* pVerifyData = NULL;      /* [TLS_VERIFYDATASIZE] */

        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pVerifyData))))
            goto exit;

        if (OK <= (status = calculateTLSFinishedVerify(pSSLSock, pSSLSock->server ? 1 : 0, pVerifyData)))
        {
            if ((recLen != TLS_VERIFYDATASIZE) ||
                (OK > MOC_MEMCMP(pVerifyData, (ubyte *)(pSHSH+sizeofHandshakeHeader), TLS_VERIFYDATASIZE, &result)) ||
                (0 != result))
            {
                status = ERR_SSL_PROTOCOL_PROCESS_FINISHED;
            }
        }

        MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pVerifyData));
    }

exit:
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (OK <= status)
        pSSLSock->receivedFinished = TRUE;
#endif

    return status;
}


/*------------------------------------------------------------------*/
#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
#if (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_CLIENT__))
static MSTATUS
addErrorStatus(errorArray* pStatusArray, MSTATUS inStatus, MSTATUS defStatus)
{
    MSTATUS status = -1;
    ubyte   count  = 0;

    if (pStatusArray)
    {
        switch (inStatus)
        {
            case ERR_CERT_INVALID_STRUCT:
            case ERR_CERT_BAD_COMMON_NAME:
            case ERR_CERT_KEYUSAGE_MISSING:
            case ERR_CERT_INVALID_KEYUSAGE:
			case ERR_MEM_ALLOC_FAIL:
            case ERR_CERT_EXPIRED:
            case ERR_CERT_START_TIME_VALID_IN_FUTURE:
            case ERR_CERT_INVALID_PARENT_CERTIFICATE:
            case ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO:
            case ERR_CERT_UNSUPPORTED_DIGEST:
            case ERR_SSL_UNKNOWN_CERTIFICATE_AUTHORITY:
                defStatus = inStatus;
                break;

            default:
                break;
        }

        /* Now check if the status is already present in the array */
        for (count = 0; count < pStatusArray->numErrors; count++)
        {
            if (defStatus == pStatusArray->errors[count])
            {
                status = OK;
                goto exit;
            }
        }

        pStatusArray->errors[pStatusArray->numErrors] = defStatus;
        pStatusArray->numErrors ++;

        status = OK;

        if (MAX_CERT_ERRORS <= pStatusArray->numErrors)
        {
            /* Maximum number of errors reached */
            status = -1;
        }
    }
exit:
    return status;
}
/*------------------------------------------------------------------*/

static MSTATUS
validateFirstCertificate(ASN1_ITEM* rootItem,
                         CStream s,
                         SSLSocket* pSSLSock,
                         intBoolean chkCommonName,
                         errorArray* pStatusArray)
{
    MSTATUS status = OK;

#if (defined(__ENABLE_MOCANA_SSL_CLIENT__) && !defined(__DISABLE_MOCANA_SSL_COMMON_NAME_CHECK__))
    if ((!pSSLSock->server) && (FALSE != chkCommonName))
    {
        CNMatchInfo cn[2];
#if defined(__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__)

        if ( pSSLSock->roleSpecificInfo.client.pCNMatchInfos)
        {
            status = CERT_CompSubjectCommonNameEx( rootItem, s,
                        pSSLSock->roleSpecificInfo.client.pCNMatchInfos);
            if ( ERR_CERT_BAD_COMMON_NAME == status)
            {
                status = CERT_CompSubjectAltNamesEx( rootItem, s,
                    (const CNMatchInfo*)pSSLSock->roleSpecificInfo.client.pCNMatchInfos,
                    (1 << 2)); /* 2 = DNS name tag */
            }
        }
        else
        {
#ifdef __ENABLE_MOCANA_RFC_2818_COMMON_NAME_CHECK__
            status = CERT_CompSubjectAltNamesExact(rootItem, s,
                                               pSSLSock->roleSpecificInfo.client.pDNSName,
                                               (1 << 2)); /* 2 = DNS name tag */
            if ((OK > status) && (ERR_CERT_BAD_COMMON_NAME != status))
            {
                status = CERT_CompSubjectCommonName( rootItem, s,
                                                     pSSLSock->roleSpecificInfo.client.pDNSName);
            }
#else
            status = CERT_CompSubjectCommonName(rootItem, s,
                                                pSSLSock->roleSpecificInfo.client.pDNSName);

            if ( ERR_CERT_BAD_COMMON_NAME == status)
            {
                cn[0].name = pSSLSock->roleSpecificInfo.client.pDNSName;
                cn[0].flags = 0;
                MOC_MEMSET((ubyte *)&cn[1], 0, sizeof(CNMatchInfo));

                status = CERT_CompSubjectAltNamesEx( rootItem, s, (const CNMatchInfo*)cn, (1 << 2)); /* 2 = DNS name tag */

            }
#endif

        }
#else
        /* verify common name */
#ifdef __ENABLE_MOCANA_RFC_2818_COMMON_NAME_CHECK__
        status = CERT_CompSubjectAltNamesExact(rootItem, s,
                                           pSSLSock->roleSpecificInfo.client.pDNSName,
                                           (1 << 2)); /* 2 = DNS name tag */
        if ((OK > status) && (ERR_CERT_BAD_COMMON_NAME != status))
        {
            status = CERT_CompSubjectCommonName( rootItem, s,
                                                 pSSLSock->roleSpecificInfo.client.pDNSName);
        }
#else
        status = CERT_CompSubjectCommonName(rootItem, s,
                                            pSSLSock->roleSpecificInfo.client.pDNSName);

        if ( ERR_CERT_BAD_COMMON_NAME == status)
        {
            cn[0].name = pSSLSock->roleSpecificInfo.client.pDNSName;
            cn[0].flags = 0;
            MOC_MEMSET((ubyte*)&cn[1], 0, sizeof(CNMatchInfo));

            status = CERT_CompSubjectAltNamesEx( rootItem, s, (const CNMatchInfo*)cn, (1 << 2)); /* 2 = DNS name tag */

        }
#endif /* end of __ENABLE_MOCANA_RFC_2818_COMMON_NAME_CHECK__ */
#endif /* end of __ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__ */
        if (OK > status)
        {
            if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_BAD_COMMON_NAME))
               goto exit;
        }
    }
#endif /* defined(__ENABLE_MOCANA_SSL_CLIENT__) && !defined(__DISABLE_MOCANA_SSL_COMMON_NAME_CHECK__) */

    /* first certificate -> get the key */
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    if (pSSLSock->server)
    {
#ifdef __ENABLE_MOCANA_SSL_SERVER__
        /* check the client certificate for the keyUsage digital signature */
        /* everything is optional, i.e. if there is no key usage we still accept it */
        /* change the value of the third argument if this is not what's desired */
        ASN1_ITEM* pKeyUsageExtension = 0;
        byteBoolean bitVal;

        if (OK > (status = CERT_getCertificateKeyUsage( rootItem, s, SSL_CERT_VERIFY_ARG,
                                                        &pKeyUsageExtension)))
        {
            if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_KEYUSAGE_MISSING))
               goto exit;
        }

        /* if there is a pKeyUsageExtension then check its value */
        if (pKeyUsageExtension)
        {
            ASN1_getBitStringBit( pKeyUsageExtension, s, digitalSignature, &bitVal);
            if (!bitVal)
            {
                status = ERR_CERT_INVALID_KEYUSAGE;
                if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_KEYUSAGE))
                    goto exit;
            }
        }
#endif

        status = CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(pSSLSock->hwAccelCookie)
                                                     rootItem, s, &pSSLSock->mutualAuthKey);
        if (OK > status)
        {
            if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_STRUCT))
                goto exit;
        }
        goto exit;
    }
#endif

#ifdef __ENABLE_MOCANA_SSL_CLIENT__

    if (!(pSSLSock->server))
    {
        ASN1_ITEM* pKeyUsageExtension = 0;
#ifdef __ENABLE_MOCANA_CCM_8__
        ASN1_ITEM* pItem = NULL;
        ASN1_ITEM* pSeqAlgoId = NULL;
        ubyte4 hashType;
        ubyte4 pubKeyType;
#endif
        byteBoolean bitVal;

        if (OK > (status = CERT_getCertificateKeyUsage( rootItem, s, SSL_CERT_VERIFY_ARG,
                                                        &pKeyUsageExtension)))
        {
            if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_KEYUSAGE_MISSING))
                    goto exit;
        }

        /* if there is a pKeyUsageExtension then check its value */
        if (pKeyUsageExtension)
        {
            ASN1_getBitStringBit( pKeyUsageExtension, s, keyEncipherment, &bitVal);
            if (!bitVal)
            {
                ASN1_getBitStringBit( pKeyUsageExtension, s, keyAgreement, &bitVal);
            }
            if (!bitVal)
            {
                status = ERR_CERT_INVALID_KEYUSAGE;
                if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_KEYUSAGE))
                    goto exit;
            }
        }
        if (OK > ( status =
            CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(pSSLSock->hwAccelCookie) rootItem,
                                                s, &pSSLSock->roleSpecificInfo.client.publicKey)))
        {
            if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_STRUCT))
                goto exit;
        }


        /* if RSA, verify the public key is of sufficient size (FREAK) */
        if (akt_rsa == pSSLSock->roleSpecificInfo.client.publicKey.type)
        {
            vlong* modulus  = RSA_N(pSSLSock->roleSpecificInfo.client.publicKey.key.pRSA);

            if ((MIN_SSL_RSA_SIZE - 1) > VLONG_bitLength(modulus))
            {
                status = ERR_SSL_RSA_KEY_SIZE;
                if (OK > addErrorStatus(pStatusArray, status, status))
                    goto exit;
            }
        }

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )

        /* verify that if the key is ECC it matches one of the authorized curves */
        if ( akt_ecc == pSSLSock->roleSpecificInfo.client.publicKey.type)
        {
            if (0 == (pSSLSock->eccCurves &
                        SSL_SOCK_ECCKeyToECFlag( pSSLSock->roleSpecificInfo.client.publicKey.key.pECC)))
            {
                status = ERR_SSL_UNSUPPORTED_CURVE;
                if (OK > addErrorStatus(pStatusArray, status, ERR_SSL_UNSUPPORTED_CURVE))
                    goto exit;
            }
        }
#endif
#endif

#ifdef __ENABLE_MOCANA_CCM_8__
        if (0xC0AE == pSSLSock->pHandshakeCipherSuite->cipherSuiteId)
        {
            /* Additional check for issuer's public key to be ECDSA */
            pItem = ASN1_FIRST_CHILD( rootItem);
            if ( NULL == pItem)
            {
                status = ERR_CERT_INVALID_STRUCT;
                if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_STRUCT))
                    goto exit;

                goto exit;
            }

            /* algo id is the second child of signed */
            status = ASN1_GetNthChild( pItem, 2, &pSeqAlgoId);
            if (OK > status)
            {
                if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_STRUCT))
                    goto exit;

                goto exit;
            }

            status = CERT_getCertSignAlgoType( pSeqAlgoId, s, &hashType, &pubKeyType);
            if ( OK > status)
            {
                if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO))
                    goto exit;

                goto exit;
            }

            /* Add an additional check for either SHA-256, SHA-384, SHA-512 */
            if (((ubyte4) akt_ecc != pubKeyType) ||
                ((hashType != (ubyte4) ht_sha256) &&
                 (hashType != (ubyte4) ht_sha384) &&
                 (hashType != (ubyte4) ht_sha512)))
            {
                /* return an error */
                status = ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO;

                if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO))
                    goto exit;

                goto exit;
            }
        }
#endif
    }
#endif

exit:

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_CLIENT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_CLIENT__))
static MSTATUS
verifyKnownCa(SSLSocket *pSSLSock,
              ubyte *pTempCertificate, ubyte2 certLen,
              certDescriptor  *pCaCert, sbyte4 chainLen,
              certParamCheckConf *pCertParamCheckConf,
              errorArray* pStatusArray)
{
    certDescriptor  caCertificate;
    ASN1_ITEM*      pCertificate     = NULL;
    MemFile         certMemFile;
    CStream         cs;
    ASN1_ITEM*      pLastCertificate = NULL;
    MemFile         lastCertMemFile;
    CStream         lastCs;
    intBoolean      isSelfSigned     = FALSE;
    MSTATUS         status           = ERR_SSL_UNKNOWN_CERTIFICATE_AUTHORITY;



    MOC_MEMSET((ubyte *)&caCertificate, 0x00, sizeof(certDescriptor));

    if ((!pSSLSock->server) && (SSL_FLAG_DISABLE_CLIENT_CHAIN_CHK & pSSLSock->runtimeFlags))
    {
    	status = OK;
    	goto exit;
    }


    MF_attach(&certMemFile, certLen, pTempCertificate);
    CS_AttachMemFile(&cs, &certMemFile );

    if (OK > (status = ASN1_Parse(cs, &pCertificate)))
        goto exit;


    if (NULL != SSL_sslSettings()->funcPtrCertificateStoreLookup)
    {
#ifdef __ENABLE_MOCANA_EXTRACT_CERT_BLOB__
        ubyte*  pDistinguishedName = NULL;
        ubyte4  distinguishedNameLen;
#else
        certDistinguishedName *pIssuer = NULL;

        if (OK > (status = (MSTATUS)CA_MGMT_allocCertDistinguishedName(&pIssuer)))
            goto exit;
#endif

#if (!defined(__ENABLE_MOCANA_EXTRACT_CERT_BLOB__))
        status = CERT_extractDistinguishedNames(pCertificate, cs, 0, pIssuer);

        if (OK > status)
        {
            CA_MGMT_freeCertDistinguishedName(&pIssuer);
            goto exit;
        }

        status = SSL_sslSettings()->funcPtrCertificateStoreLookup(SSL_findConnectionInstance(pSSLSock),
                                                                  pIssuer, &caCertificate);

        CA_MGMT_freeCertDistinguishedName(&pIssuer);
#else
        if (OK > (status = CERT_extractDistinguishedNamesBlob(pCertificate, cs, FALSE, &pDistinguishedName, &distinguishedNameLen)))
            goto exit;

        status = SSL_sslSettings()->funcPtrCertificateStoreLookup(SSL_findConnectionInstance(pSSLSock),
                                                                                          pDistinguishedName, distinguishedNameLen, &caCertificate);

       FREE(pDistinguishedName);
#endif


        if (OK > status)
            goto exit;

        if (NULL == caCertificate.pCertificate)
        {
            status = ERR_SSL_UNKNOWN_CERTIFICATE_AUTHORITY;
            goto exit;
        }

        MF_attach(&lastCertMemFile, caCertificate.certLength, caCertificate.pCertificate);
        CS_AttachMemFile( &lastCs, &lastCertMemFile);

        /* parse the certificate */
        if (OK > (status = ASN1_Parse(lastCs, &pLastCertificate)))
        {
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() returns status = ", (sbyte4)status);
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() @ line # ", (sbyte4)__LINE__);

            goto exit;
        }

        status = CERT_validateCertificateWithConf(MOC_RSA_HASH(pSSLSock->hwAccelCookie)
                                                  pCertificate,
                                                  cs,
                                                  pLastCertificate,
                                                  lastCs,
                                                  __MOCANA_PARENT_CERT_CHECK_OPTIONS__, /* all checks enabled by default */
                                                  chainLen,
                                                  TRUE,
                                                  pCertParamCheckConf->chkValidityBefore,
                                                  pCertParamCheckConf->chkValidityAfter);
    }
    else
        status = ERR_NULL_POINTER;

    if (OK > status)
    {
        status = ERR_SSL_UNKNOWN_CERTIFICATE_AUTHORITY;
        addErrorStatus(pStatusArray, ERR_CERT_INVALID_PARENT_CERTIFICATE, ERR_CERT_INVALID_PARENT_CERTIFICATE);
        goto exit;
    }

    if (NULL != pCaCert)
    {
        pCaCert->pCertificate = caCertificate.pCertificate;
        pCaCert->certLength   = caCertificate.certLength;
    }

    /* last certificate -> is this root certificate in our store? */
    if ((NULL != pLastCertificate) && (NULL != SSL_sslSettings()->funcPtrCertificateStoreVerify))
    {
        status = CERT_isRootCertificate(pLastCertificate, lastCs);

        if (OK <= status)
        {
            isSelfSigned = TRUE;
            status = SSL_sslSettings()->funcPtrCertificateStoreVerify(SSL_findConnectionInstance(pSSLSock),
                                                                                          (ubyte*)caCertificate.pCertificate, caCertificate.certLength, isSelfSigned);
        }
        else if (ERR_FALSE != status)
        {
            goto exit;
        }
        else
            status = OK;
    }

exit:

    if(pCertificate)
        TREE_DeleteTreeItem((TreeItem *)pCertificate);
    if(pLastCertificate)
        TREE_DeleteTreeItem((TreeItem *)pLastCertificate);

    if ((NULL != SSL_sslSettings()->funcPtrCertificateStoreRelease) && (NULL != caCertificate.pCertificate))
        SSL_sslSettings()->funcPtrCertificateStoreRelease(SSL_findConnectionInstance(pSSLSock), &caCertificate);


    return status;

}/*verifyKnownCa*/

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_validateCert(SSLSocket *pSSLSock,
                      certDescriptor* pCertChain, sbyte4 numCertsInChain,
                      certParamCheckConf *pCertParamCheckConf,
                      certDescriptor *caCertificate, errorArray* pStatusArray)
{
    ASN1_ITEM*      pCertificate     = NULL;
    ASN1_ITEM*      pPrevCertificate    = NULL;
    CStream         prevCS;
    CStream         cs;
    MemFile         certMemFile;
    MemFile         prevCertMemFile;
    MSTATUS         status           = OK;
    sbyte4          i = 0;
    ubyte           *pTempCertificate = NULL;
    ubyte2          certLen = 0;
    intBoolean      isRoot = FALSE;

    if (pSSLSock == NULL || numCertsInChain <= 0 || pCertChain == NULL)
    {
        status = ERR_NULL_POINTER;
        addErrorStatus(pStatusArray, status, status);
        goto exit;
    }

    if (NULL == pSSLSock->pHandshakeCipherSuite)
    {
        status = ERR_SSL_NO_CIPHERSUITE;
        addErrorStatus(pStatusArray, status, status);
        goto exit;
    }

    if (caCertificate != NULL)
        MOC_MEMSET((ubyte *)caCertificate, 0x00, sizeof(certDescriptor));

    if (NULL == pCertParamCheckConf)
        pCertParamCheckConf = &pSSLSock->certParamCheck;

    for (i = 0; i < numCertsInChain; i++)
    {
        if (NULL == pCertChain[i].pCertificate)
        {
            status = ERR_NULL_POINTER;
            addErrorStatus(pStatusArray, status, status);
            goto exit;
        }

        pTempCertificate = pCertChain[i].pCertificate;
        certLen = pCertChain[i].certLength;

        MF_attach(&certMemFile, certLen, pTempCertificate);
        CS_AttachMemFile(&cs, &certMemFile );

        if (OK > (status = ASN1_Parse( cs, &pCertificate)))
        {
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() returns status = ", (sbyte4)status);
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() @ line # ", (sbyte4)__LINE__);
            addErrorStatus(pStatusArray, status, status);
            goto exit;
        }

        /* last certificate --  check if it's the root certificate
        This returns OK, ERR_FALSE or some error*/
        if (OK <= (status = CERT_isRootCertificate(pCertificate, cs)))
        {
            if (OK == CERT_validateCertificate(MOC_RSA_HASH(pSSLSock->hwAccelCookie)
                                               pCertificate, cs,
                                               pCertificate, cs,
                                               0,
                                               i, TRUE))
                isRoot = TRUE;
        }
        else if (ERR_FALSE != status)
        {
           if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_PARENT_CERTIFICATE))
               goto exit;
        }

        if (NULL == pPrevCertificate)  /* first certificate */
        {
            if (OK > (status = validateFirstCertificate(pCertificate, cs, pSSLSock, pCertParamCheckConf->chkCommonName, pStatusArray)))
            {
                if (pStatusArray)
                {
                    if (MAX_CERT_ERRORS <= pStatusArray->numErrors)
                        goto exit;

                } else {
                    goto exit;
                }
            }

            /* self-signed certificate */
            if (isRoot)
            {
                status = CERT_validateCertificateWithConf(MOC_RSA_HASH(pSSLSock->hwAccelCookie)
                                                          pCertificate,cs,
                                                          pCertificate, cs,
                                                          __MOCANA_SELFSIGNED_CERT_CHECK_OPTIONS__,
                                                          1, isRoot,
                                                          pCertParamCheckConf->chkValidityBefore,
                                                          pCertParamCheckConf->chkValidityAfter);
#ifdef __NO_SELF_SIGNED_CERTIFICATES__
                if (OK > status)
                {
                    if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_SIGNATURE))
                            goto exit;
                }

                status = ERR_SSL_NO_SELF_SIGNED_CERTIFICATES;
#endif

                if (OK > status)
                {
                    if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_SIGNATURE))
                        goto exit;
                }
            }
            else
            {   /* check for the ValidityTime (CERT_validateCertificate does
                it in the other cases) */
                if ((FALSE != pCertParamCheckConf->chkValidityBefore) || (FALSE != pCertParamCheckConf->chkValidityAfter))
                {
                    if (OK > (status = CERT_VerifyValidityTimeWithConf(pCertificate, cs, pCertParamCheckConf->chkValidityBefore, pCertParamCheckConf->chkValidityAfter)))
                    {
                      if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_EXPIRED))
                            goto exit;
                    }
                }
            }
        }
        else
        {
            /* validate prev certificate with current -> verify signature */
            status = CERT_validateCertificateWithConf(MOC_RSA_HASH(pSSLSock->hwAccelCookie)
                                                      pPrevCertificate, prevCS,
                                                      pCertificate, cs,
                                                      __MOCANA_PARENT_CERT_CHECK_OPTIONS__,
                                                      1, isRoot,
                                                      pCertParamCheckConf->chkValidityBefore,
                                                      pCertParamCheckConf->chkValidityAfter);

            if (OK > status)
            {
                /* Add ERR_CERT_INVALID_PARENT_CERTIFICATE to mark the beginning of errors
                               in parent certificates up the chain. */
                if (OK > addErrorStatus(pStatusArray, ERR_CERT_INVALID_PARENT_CERTIFICATE, ERR_CERT_INVALID_PARENT_CERTIFICATE))
                    goto exit;

                /* Now add the actual error code */
                if (OK > addErrorStatus(pStatusArray, status, ERR_CERT_INVALID_PARENT_CERTIFICATE))
                    goto exit;
            }
        }

        if (FALSE != pCertParamCheckConf->chkKnownCA)
        {
            status = verifyKnownCa(pSSLSock, pCertChain[i].pCertificate, pCertChain[i].certLength, caCertificate, i /*numCertsInChain*/, pCertParamCheckConf, pStatusArray);
            if (OK == status)
            {
                break;
            }
            else if ((OK > status) && (i == (numCertsInChain - 1)))
            {
                if (OK > addErrorStatus(pStatusArray, status, ERR_SSL_UNKNOWN_CERTIFICATE_AUTHORITY))
                    goto exit;
            }
            else
                status = OK;
        }

        if (pPrevCertificate)
            TREE_DeleteTreeItem((TreeItem *)pPrevCertificate);

        pPrevCertificate = pCertificate;
        pCertificate     = NULL;
        MF_attach( &prevCertMemFile, certMemFile.size, certMemFile.buff);
        CS_AttachMemFile(&prevCS, &prevCertMemFile);
    }

exit:
    if (pCertificate)
        TREE_DeleteTreeItem((TreeItem *)pCertificate);

    if (pPrevCertificate)
        TREE_DeleteTreeItem((TreeItem *)pPrevCertificate);

    return status;

}/* validateCert */

/*------------------------------------------------------------------*/

static MSTATUS
processCertificate(SSLSocket* pSSLSock,
                   ubyte* pSHSH, ubyte2 recLen,
                   intBoolean isCertRequired)
{
     ubyte*          pTempCertificate;
    ubyte2          certChainLen;
    ubyte2          certLen          = 0;
    ASN1_ITEM*      pCertificate     = NULL;
    ASN1_ITEM*      pPrevCertificate = NULL;
    MemFile         certMemFile;
    sbyte4          certNum          = 0;
    ubyte4          sizeofHandshakeHeader;
    MSTATUS         status           = OK;
#if !(defined (__ENABLE_MOCANA_SSL_EXTERNAL_CERT_CHAIN_VERIFY__) || defined (__DISABLE_MOCANA_CERTIFICATE_PARSING__ ))
    sbyte4          connectionInstance = -1;
#endif
    certDescriptor  certChain[SSL_MAX_NUM_CERTS_IN_CHAIN];

    MOC_MEMSET((ubyte *)certChain, 0x00, sizeof(certChain));

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
    }

    /* first check is the recLen */
    pTempCertificate = (ubyte*) (pSHSH+sizeofHandshakeHeader);
    certChainLen = getMediumValue(pTempCertificate);

    if (certChainLen != (recLen - SSL_MEDIUMSIZE))
    {
        status = ERR_SSL_PROTOCOL_PROCESS_CERTIFICATE;
        goto exit;
    }

    pTempCertificate += SSL_MEDIUMSIZE;

    while (certChainLen >= SSL_MEDIUMSIZE)
    {
        CStream cs;

        certLen = 0;

        /* parse the certificate using a memfile */
        certLen = getMediumValue(pTempCertificate);
        pTempCertificate += SSL_MEDIUMSIZE;
        certChainLen -= SSL_MEDIUMSIZE;
        if ( certLen > certChainLen)
            return ERR_SSL_PROTOCOL_PROCESS_CERTIFICATE;

        MF_attach(&certMemFile, certLen, pTempCertificate);
        CS_AttachMemFile(&cs, &certMemFile );

        if (OK > (status = ASN1_Parse( cs, &pCertificate)))
        {
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() returns status = ", (sbyte4)status);
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() @ line # ", (sbyte4)__LINE__);
            goto exit;
        }

        if (0 == pPrevCertificate)  /* first certificate */
        {
            if (SSL_sslSettings()->funcPtrCertificateLeafTest)
            {
                status = SSL_sslSettings()->funcPtrCertificateLeafTest(SSL_findConnectionInstance(pSSLSock),
                                                                                               pTempCertificate, (ubyte4)certLen);

                if (OK > status)
                    goto exit;
            }

        }
        else
        {
            if (SSL_sslSettings()->funcPtrCertificateChainTest)
            {
                status = SSL_sslSettings()->funcPtrCertificateChainTest(SSL_findConnectionInstance(pSSLSock),
                                                                                                pTempCertificate, (ubyte4)certLen);
                if (OK > status)
                    goto exit;
            }
        }

#ifdef __ENABLE_MOCANA_SSL_EXTERNAL_CERT_CHAIN_VERIFY__
        if (SSL_MAX_NUM_CERTS_IN_CHAIN <= certNum)
        {
            status = ERR_SSL_PROTOCOL_PROCESS_CERTIFICATE;
            goto exit;
        }

#endif /* __ENABLE_MOCANA_SSL_EXTERNAL_CERT_CHAIN_VERIFY__ */

        certChain[certNum].pCertificate = pTempCertificate;
        certChain[certNum].certLength = certLen;

        /* advance */
        /* if more than two certificates are present */
        if (pPrevCertificate)
        {
            TREE_DeleteTreeItem( (TreeItem*) pPrevCertificate);
        }
        pTempCertificate   += certLen;
        certChainLen        = (ubyte2)(certChainLen - certLen);
        pPrevCertificate    = pCertificate;
        pCertificate        = NULL;

        ++certNum;
    }

    if (certChainLen > 0)
    {
        status = ERR_SSL_PROTOCOL_PROCESS_CERTIFICATE;
        goto exit;
    }
#if !(defined (__ENABLE_MOCANA_SSL_EXTERNAL_CERT_CHAIN_VERIFY__) || defined (__DISABLE_MOCANA_CERTIFICATE_PARSING__ ))
    if (OK > (status = connectionInstance = SSL_findConnectionInstance(pSSLSock)))
        goto exit;

    if (OK > (status = SSL_validateCertParam(connectionInstance, certChain, certNum,
                                             &pSSLSock->certParamCheck,
                                             &certChain[certNum], NULL)))
    {
        goto exit;
    }

    if (NULL != certChain[certNum].pCertificate)
        certNum++;

     if ((!pSSLSock->server) && (SSL_FLAG_DISABLE_CLIENT_CHAIN_CHK & pSSLSock->runtimeFlags))
        goto exit;

#else
    if (isCertRequired)
        status = ERR_SSL_PROTOCOL_PROCESS_CERTIFICATE;

    if (0 < certNum)
    {
        CStream         cs;
        MemFile         certMemFile;

        MF_attach(&certMemFile, certChain[0].certLength, certChain[0].pCertificate);
        CS_AttachMemFile(&cs, &certMemFile );

        if (OK > (status = ASN1_Parse( cs, &pCertificate)))
        {
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() returns status = ", (sbyte4)status);
            DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() @ line # ", (sbyte4)__LINE__);
            goto exit;
        }

        if (pSSLSock->server)
        {
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
            /* For Mutual Auth we need to fill the mutualAuthKey */
            if (OK > (status = CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(pSSLSock->hwAccelCookie) pCertificate,
                                                               cs, &pSSLSock->mutualAuthKey)))
            {
                TREE_DeleteTreeItem( (TreeItem*) pCertificate);
                goto exit;
            }
#endif
        }
#ifdef __ENABLE_MOCANA_SSL_CLIENT__
        else
        {
            if (OK > (status = CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(pSSLSock->hwAccelCookie) pCertificate,
                                                              cs, &pSSLSock->roleSpecificInfo.client.publicKey)))
            {
                TREE_DeleteTreeItem( (TreeItem*) pCertificate);
                goto exit;
            }
            /* if RSA, verify the public key is of sufficient size (FREAK) */
            if (akt_rsa == pSSLSock->roleSpecificInfo.client.publicKey.type)
            {
                vlong* modulus  = RSA_N(pSSLSock->roleSpecificInfo.client.publicKey.key.pRSA);

                if ((MIN_SSL_RSA_SIZE - 1) > VLONG_bitLength(modulus))
                {
                    status = ERR_SSL_RSA_KEY_SIZE;
                    TREE_DeleteTreeItem( (TreeItem*) pCertificate);
                    goto exit;
                }
            }

        }
#endif
        TREE_DeleteTreeItem( (TreeItem*) pCertificate);

        if (NULL != SSL_sslSettings()->funcPtrExternalCertificateChainVerify)
        {
            status = SSL_sslSettings()->funcPtrExternalCertificateChainVerify(SSL_findConnectionInstance(pSSLSock),
                                                                              certChain, certNum);
        }
    }

/* exit for cleanup */
    goto exit;
#endif

#if (defined(__ENABLE_TLSEXT_RFC6066__) && defined(__ENABLE_MOCANA_OCSP_CLIENT__))
    /* Need server certificate and its issuer here for certificate status request extension */
    if (pSSLSock->certStatusReqExt &&
        pSSLSock->roleSpecificInfo.client.didRecvCertStatusExtInServHello)
    {
        if (OK > (status = SSL_OCSP_addCertificates(pSSLSock->pOcspContext, certChain, certNum)))
            goto exit;
    }
#endif


exit:
    if (pPrevCertificate)
    {
        TREE_DeleteTreeItem( (TreeItem*) pPrevCertificate);
    }

    return status;

} /* processCertificate */
#endif /* (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_CLIENT__)) */
#endif /* __DISABLE_MOCANA_CERTIFICATE_PARSING__ */
/*------------------------------------------------------------------*/

#ifndef __ENABLE_MOCANA_SSL_ALERTS__
static MSTATUS
handleAlertMessage(SSLSocket* pSSLSock)
{
    MOC_UNUSED(pSSLSock);

    return OK;
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_SERVER__
extern MSTATUS
SSLSOCK_clearServerSessionCache(SSLSocket* pSSLSock)
{
    MSTATUS status = OK;

    if (NULL == pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* on alert, we want to scramble session cache's master secret */
    if (pSSLSock->server)
    {
        sbyte4 cacheIndex;

        /* cacheIndex is session modulo SESSION_CACHE_SIZE */
        cacheIndex = (pSSLSock->roleSpecificInfo.server.sessionId) % SESSION_CACHE_SIZE;

        if (OK > (status = RTOS_mutexWait(gSslSessionCacheMutex)))
        {
            MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"sslsock.c: RTOS_mutexWait() #1 failed.");
            goto exit;
        }

        /* scramble previous session secret */
        if (pSSLSock->roleSpecificInfo.server.sessionId == gSessionCache[cacheIndex].m_sessionId)
        {
            sbyte4 index;

            /* make sure it will not be reused -- 0 is never a valid session id*/
            gSessionCache[cacheIndex].m_sessionId = 0;

            RANDOM_numberGenerator(g_pRandomContext, gSessionCache[cacheIndex].m_masterSecret, SSL_MASTERSECRETSIZE);

            for (index = 0; index < SSL_MASTERSECRETSIZE; index++)
            {
                gSessionCache[cacheIndex].m_masterSecret[index] ^= 0x5c;
                gSessionCache[cacheIndex].m_masterSecret[index] += 0x36;
            }
        }

        if (OK > (status = RTOS_mutexRelease(gSslSessionCacheMutex)))
            goto exit;
    }

exit:
    return status;
}

#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ALERTS__
static MSTATUS
handleAlertMessage(SSLSocket* pSSLSock)
{
    sbyte*  pMsg        = pSSLSock->pReceiveBuffer;
    sbyte4  alertClass;
    sbyte4  alertId;
    ubyte2  recordLen;
    MSTATUS status      = OK;

    recordLen = (ubyte2)(pSSLSock->recordSize);

    if (2 != recordLen)
    {
        status = ERR_SSL_PROTOCOL_BAD_LENGTH;
        goto exit;
    }

    alertId    = pMsg[1];
    alertClass = pMsg[0];

    if (SSLALERTLEVEL_WARNING != alertClass)
    {
        /* default to fatal */
        alertClass = SSLALERTLEVEL_FATAL;
        status     = ERR_SSL_FATAL_ALERT;
    }

    if (NULL != (SSL_sslSettings()->funcPtrAlertCallback))
    {
        status = (MSTATUS)SSL_sslSettings()->funcPtrAlertCallback(SSL_findConnectionInstance(pSSLSock), alertId, alertClass);
    }

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    /* on fatal alert, we want to scramble session cache's master secret */
    if (SSLALERTLEVEL_FATAL == alertClass)
    {
        MSTATUS status1 = SSLSOCK_clearServerSessionCache(pSSLSock);

        if ((OK <= status) && (OK > status1))
            status = status1;   /* don't overwrite a previous error code */
    }
#endif

exit:
    return status;

} /* handleAlertMessage */
#endif /* __ENABLE_MOCANA_SSL_ALERTS__ */


/*------------------------------------------------------------------*/

static MSTATUS
handleInnerAppMessage(SSLSocket* pSSLSock)
{
    ubyte2  recordLen;
    MSTATUS status      = OK;

    recordLen = (ubyte2)(pSSLSock->recordSize);

    if (6 > recordLen)
    {
        status = ERR_SSL_PROTOCOL_BAD_LENGTH;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_INNER_APP__
    if (NULL != (SSL_sslSettings()->funcPtrInnerAppCallback))
    {
        /* TODO: And We Negotiated Inner App */
        status = (MSTATUS)SSL_sslSettings()->funcPtrInnerAppCallback(SSL_findConnectionInstance(pSSLSock), (ubyte*)pSSLSock->pReceiveBuffer, recordLen);
    }
#endif

exit:
    return status;

} /* handleInnerAppMessage */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_RFC3546__))
static MSTATUS
resetTicket(SSLSocket* pSSLSock)
{
    pSSLSock->roleSpecificInfo.client.ticketLength = 0;

    if (pSSLSock->roleSpecificInfo.client.ticket)
    {
        FREE(pSSLSock->roleSpecificInfo.client.ticket);
        pSSLSock->roleSpecificInfo.client.ticket = 0;
    }

    return OK;
}
#endif /* (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_RFC3546__)) */


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_init(SSLSocket* pSSLSock, intBoolean isDTLS, TCP_SOCKET tcpSock, peerDescr *pPeerDescr, RNGFun rngFun, void* rngFunArg)
{
    MSTATUS status;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    ubyte4  i;
#endif

    pSSLSock->isDTLS = isDTLS;
    pSSLSock->pReceiveBuffer = NULL;
    pSSLSock->receiveBufferSize = 0;
    pSSLSock->pHandshakeCipherSuite = NULL;
    pSSLSock->pActiveOwnCipherSuite = NULL;
    pSSLSock->pActivePeerCipherSuite = NULL;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    pSSLSock->retransCipherInfo.pOldCipherSuite = NULL;
#endif
    pSSLSock->clientBulkCtx = pSSLSock->serverBulkCtx = NULL;
    pSSLSock->sessionResume = E_NoSessionResume;
    pSSLSock->server = 0; /* client by default */

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
    pSSLSock->eccCurves = SUPPORTED_CURVES_FLAGS; /* simplify processing later on */
#endif

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    if (OK > (status = CRYPTO_initAsymmetricKey( &pSSLSock->mutualAuthKey)))
        goto exit;
#endif

#if defined( __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)
    if (OK > (status = CRYPTO_initAsymmetricKey( &pSSLSock->ecdheKey)))
        goto exit;
#endif

    if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, SSL_MASTERSECRETSIZE + (4 * SSL_RANDOMSIZE) + (2 * (SSL_MAXROUNDS * MD5_DIGESTSIZE)), TRUE, (void **)&(pSSLSock->pSecretAndRand))))
        goto exit;

    pSSLSock->pClientRandHello = SSL_MASTERSECRETSIZE + 2 * SSL_RANDOMSIZE + pSSLSock->pSecretAndRand;
    pSSLSock->pServerRandHello = SSL_RANDOMSIZE + pSSLSock->pClientRandHello;
    pSSLSock->pMaterials       = SSL_RANDOMSIZE + pSSLSock->pServerRandHello;
    pSSLSock->pActiveMaterials = (SSL_MAXROUNDS * MD5_DIGESTSIZE) + pSSLSock->pMaterials;

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__)) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
    if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, (2 * (SRTP_MAX_KEY_SIZE + SRTP_MAX_SALT_SIZE)), TRUE, (void **)&(pSSLSock->pSrtpMaterials))))
        goto exit;
#endif
#ifdef __ENABLE_MOCANA_SSL_REHANDSHAKE__
    MOC_MEMSET((ubyte*)&pSSLSock->sslRehandshakeTimerCount, 0x00, sizeof(moctime_t));
    pSSLSock->sslRehandshakeByteSendCount = 0;
#endif
    /* allocate receive buffer */
    if (OK > (status = checkBuffer(pSSLSock, SSL_DEFAULT_SMALL_BUFFER)))
        goto exit;

#if (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__))
    pSSLSock->serverNameList = 0;
    pSSLSock->serverNameListLength = 0;
#ifdef __ENABLE_TLSEXT_RFC6066__
    pSSLSock->certStatusReqExt = FALSE;
    pSSLSock->pExts = NULL;
    pSSLSock->numOfExtension = 0;
#endif
#endif /* (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__)) */

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
    if (pSSLSock->isDTLS)
    {
        MOC_IP_ADDRESS srcAddrRef = REF_MOC_IPADDR(pPeerDescr->srcAddr);
        MOC_IP_ADDRESS peerAddrRef = REF_MOC_IPADDR(pPeerDescr->peerAddr);
        pSSLSock->peerDescr.pUdpDescr = pPeerDescr->pUdpDescr;
        pSSLSock->peerDescr.srcPort = pPeerDescr->srcPort;
        COPY_MOC_IPADDR(pSSLSock->peerDescr.srcAddr, srcAddrRef);
        pSSLSock->peerDescr.peerPort = pPeerDescr->peerPort;
        COPY_MOC_IPADDR(pSSLSock->peerDescr.peerAddr, peerAddrRef);
        /* create handshake timer */
        if (OK > (status = TIMER_createTimer((void*)handshakeTimerCallbackFunc, (ubyte**)&pSSLSock->dtlsHandshakeTimer)))
            goto exit;
        pSSLSock->dtlsHandshakeTimeout = 1000; /*default 1 second */
        pSSLSock->dtlsPMTU = 1500; /* default PMTU 1500 bytes */

        /* initialize handshake message defragment buffer */
        for (i = 0; i < MAX_HANDSHAKE_MESG_IN_FLIGHT; i++)
        {
            MOC_MEMSET((ubyte*)&pSSLSock->msgBufferDescrs[i], 0x00, sizeof(msgBufferDescr));
        }
        pSSLSock->msgBase = 0;
        pSSLSock->shouldChangeCipherSpec = FALSE;
        pSSLSock->currentPeerEpoch = 0;
        pSSLSock->receivedFinished = FALSE;
        pSSLSock->retransCipherInfo.deleteOldBulkCtx = FALSE;
    } else
# endif
    {
        pSSLSock->tcpSock = tcpSock;
    }

    pSSLSock->rngFun = rngFun;
    pSSLSock->rngFunArg = rngFunArg;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    MOC_MEMSET((ubyte *)pSSLSock->retransBuffers, 0x0, sizeof(retransBufferDescr) * MAX_HANDSHAKE_MESG_IN_FLIGHT);
    pSSLSock->isRetransmit = FALSE;
#endif
exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL_SOCK_init() returns status = ", status);
#endif

    return status;
}

extern MSTATUS
SSL_SOCK_initHashPool(SSLSocket *pSSLSock )
{
    void*       pTempMemBuffer = NULL;
    intBoolean  isRehandshake = (pSSLSock->pActiveOwnCipherSuite) ? TRUE : FALSE;
    MSTATUS     status;

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        if (!isRehandshake)
        {
            if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, SSL_SMALL_TEMP_BUF_SIZE * 5, TRUE, &pTempMemBuffer)))
                goto exit;

            DEBUG_RELABEL_MEMORY(pTempMemBuffer);

            if (OK > (status = MEM_POOL_initPool(&pSSLSock->smallPool, pTempMemBuffer, SSL_SMALL_TEMP_BUF_SIZE * 5, SSL_SMALL_TEMP_BUF_SIZE)))
                goto exit;
        }
        else
        {
            if (OK > (status = MEM_POOL_recyclePoolMemory(&pSSLSock->smallPool, SSL_SMALL_TEMP_BUF_SIZE)))
                goto exit;
        }

        if (!pSSLSock->pMd5Ctx && !pSSLSock->pShaCtx)
        {
            if (!isRehandshake)
            {
                if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, sizeof(shaDescrHS) * 5, TRUE, &pTempMemBuffer)))
                    goto exit;

                DEBUG_RELABEL_MEMORY(pTempMemBuffer);

                if (OK > (status = MEM_POOL_initPool(&pSSLSock->shaPool, pTempMemBuffer, sizeof(shaDescrHS) * 5, sizeof(shaDescrHS))))
                    goto exit;

                if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, sizeof(MD5_CTXHS) * 5, TRUE, &pTempMemBuffer)))
                    goto exit;

                DEBUG_RELABEL_MEMORY(pTempMemBuffer);

                if (OK > (status = MEM_POOL_initPool(&pSSLSock->md5Pool, pTempMemBuffer, sizeof(MD5_CTXHS) * 5, sizeof(MD5_CTXHS))))
                    goto exit;
            }
            else
            {
                if (OK > (status = MEM_POOL_recyclePoolMemory(&pSSLSock->shaPool, sizeof(shaDescrHS))))
                    goto exit;

                if (OK > (status = MEM_POOL_recyclePoolMemory(&pSSLSock->md5Pool, sizeof(MD5_CTXHS))))
                    goto exit;
            }

            if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)&(pSSLSock->pShaCtx))))
                goto exit;

            if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->md5Pool, (void **)&(pSSLSock->pMd5Ctx))))
                goto exit;
        }

        MD5init_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pMd5Ctx);
        SHA1_initDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pShaCtx);
    }
    else
    {
        CipherSuiteInfo *pCipher = pSSLSock->pHandshakeCipherSuite;
        const BulkHashAlgo *pHashAlgo = pCipher->pPRFHashAlgo;
        ubyte4 hashDescrSize = 0;

        if (!isRehandshake)
        {
            if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, SSL_BIGGER_TEMP_BUF_SIZE * 5, TRUE, &pTempMemBuffer)))
                goto exit;

            DEBUG_RELABEL_MEMORY(pTempMemBuffer);

            if (OK > (status = MEM_POOL_initPool(&pSSLSock->smallPool, pTempMemBuffer, SSL_BIGGER_TEMP_BUF_SIZE * 5, SSL_BIGGER_TEMP_BUF_SIZE)))
                goto exit;
        }
        else
        {
            if (OK > (status = MEM_POOL_recyclePoolMemory(&pSSLSock->smallPool, SSL_BIGGER_TEMP_BUF_SIZE)))
                goto exit;
        }

#ifndef __DISABLE_MOCANA_SHA256__
        if (!pHashAlgo)
            pHashAlgo = &SHA256Suite; /* default is SHA256 */
#endif

        if (&MD5Suite == pHashAlgo)
        {
            hashDescrSize = sizeof(MD5_CTXHS);
        } else if (&SHA1Suite == pHashAlgo)
        {
            hashDescrSize = sizeof(shaDescrHS);
#ifndef __DISABLE_MOCANA_SHA224__
        } else if (&SHA224Suite == pHashAlgo)
        {
            hashDescrSize = sizeof(SHA224_CTX);
#endif
#ifndef __DISABLE_MOCANA_SHA256__
        } else if (&SHA256Suite == pHashAlgo)
        {
            hashDescrSize = sizeof(SHA256_CTX);
#endif
#ifndef __DISABLE_MOCANA_SHA384__
        } else if (&SHA384Suite == pHashAlgo)
        {
            hashDescrSize = sizeof(SHA384_CTX);
#endif
#ifndef __DISABLE_MOCANA_SHA512__
        } else if (&SHA512Suite == pHashAlgo)
        {
            hashDescrSize = sizeof(SHA512_CTX);
#endif
        } else
        {
            status = ERR_SSL_UNSUPPORTED_ALGORITHM;
            goto exit;
        }

        if (!pSSLSock->pHashCtx)
        {
            if (!isRehandshake)
            {
                if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, hashDescrSize * 5, TRUE, &pTempMemBuffer)))
                    goto exit;

                DEBUG_RELABEL_MEMORY(pTempMemBuffer);

                if (OK > (status = MEM_POOL_initPool(&pSSLSock->hashPool, pTempMemBuffer, hashDescrSize * 5, hashDescrSize)))
                    goto exit;
            }
            else
            {
                if (OK > (status = MEM_POOL_recyclePoolMemory(&pSSLSock->hashPool, hashDescrSize)))
                    goto exit;
            }

            if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->hashPool, (void **)&(pSSLSock->pHashCtx))))
                goto exit;
         }

        if (OK > (status = pHashAlgo->initFunc(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pHashCtx)))
            goto exit;
    }
exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL_SOCK_initHashPool() returns status = ", status);
#endif

    return status;
}


/*------------------------------------------------------------------*/

extern void
SSL_SOCK_uninit(SSLSocket* pSSLSock)
{
    void *pTemp = NULL;

#if (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || \
     defined(__ENABLE_MOCANA_DTLS_CLIENT__) || \
     defined(__ENABLE_MOCANA_DTLS_SERVER__))
     ubyte4 i;
#endif
    resetCipher(pSSLSock, TRUE, TRUE);

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
    DH_freeDhContext(&pSSLSock->pDHcontext, NULL);
#endif

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    CRYPTO_uninitAsymmetricKey(&pSSLSock->mutualAuthKey, NULL);
#endif

#if defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)
    CRYPTO_uninitAsymmetricKey( &pSSLSock->ecdheKey, NULL);
#endif

    if (NULL != pSSLSock->pOutputBufferBase)
    {
        FREE(pSSLSock->pOutputBufferBase);

        pSSLSock->pOutputBufferBase = NULL;
        pSSLSock->pOutputBuffer     = NULL;
        pSSLSock->numBytesToSend    = 0;
    }

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    releaseRetransmissionBuffer(pSSLSock);
    clearRetransmissionSessionInfo(pSSLSock);
#endif

    if (NULL != pSSLSock->buffers[0].pHeader)
    {
        FREE(pSSLSock->buffers[0].pHeader);
        pSSLSock->buffers[0].pHeader = NULL;
        pSSLSock->numBuffers = 0;
        pSSLSock->bufIndex = 0;
    }

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    if (pSSLSock->server)
    {
        /* server specific clean up here */
        if (pSSLSock->roleSpecificInfo.server.isDynamicAlloc)
        {
            CRYPTO_uninitAsymmetricKey(&pSSLSock->roleSpecificInfo.server.privateKey, NULL);
        }
    }
#endif

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
    if (!(pSSLSock->server))
    {
        /* clean up for client */
        /* ignore return value */
        CRYPTO_uninitAsymmetricKey(&pSSLSock->roleSpecificInfo.client.publicKey, NULL);

        if (pSSLSock->roleSpecificInfo.client.helloBuffer)
        {
            FREE(pSSLSock->roleSpecificInfo.client.helloBuffer);
            pSSLSock->roleSpecificInfo.client.helloBuffer    = NULL;
            pSSLSock->roleSpecificInfo.client.helloBufferLen = 0;
        }
    }
#endif

#if (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__))
    pSSLSock->serverNameListLength = 0;
    if (pSSLSock->serverNameList)
    {
        FREE(pSSLSock->serverNameList);
        pSSLSock->serverNameList = NULL;
    }
#endif  /* (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__)) */

#if (defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__))
    pSSLSock->isRehandshakeExtPresent = FALSE;
    pSSLSock->isRehandshakeAllowed = FALSE;
    MOC_MEMSET(pSSLSock->client_verify_data, 0x0, SSL_VERIFY_DATA);
    MOC_MEMSET(pSSLSock->server_verify_data, 0x0, SSL_VERIFY_DATA);
#endif

#if (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_RFC3546__))
    if (0 == pSSLSock->server)
        resetTicket(pSSLSock);
#endif /* (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_RFC3546__)) */

    CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pSSLSock->pReceiveBufferBase);
    CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pSSLSock->pSecretAndRand);

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__)) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
    CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&(pSSLSock->pSrtpMaterials));
#endif

    if (OK <= MEM_POOL_uninitPool(&pSSLSock->smallPool, &pTemp))
        CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pTemp);

    if (OK <= MEM_POOL_uninitPool(&pSSLSock->shaPool, &pTemp))
    {
        if (pTemp)
            CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pTemp);
    }

    if (OK <= MEM_POOL_uninitPool(&pSSLSock->md5Pool, &pTemp))
    {
        if (pTemp)
            CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pTemp);
    }

    /* release ctx pool */
    if (OK <= MEM_POOL_uninitPool(&pSSLSock->hashPool, &pTemp))
    {
        if (pTemp)
            CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&pTemp);
    }
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    /* release the buffer */
    if (pSSLSock->pHashCtxList)
    {
        for (i = 0; i < NUM_SSL_SUPPORTED_HASH_ALGORITHMS; i++)
        {
            BulkCtx              pHashCtx  = pSSLSock->pHashCtxList[i];
            const BulkHashAlgo  *pHashAlgo = gSupportedHashAlgorithms[i].algo;

            if (pHashCtx)
                pHashAlgo->freeFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx);
        }

        FREE(pSSLSock->pHashCtxList);
    }
#endif

    pSSLSock->pReceiveBuffer = NULL;
    pSSLSock->pSharedInBuffer = NULL;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    /* destroy dtls handshake timer */
    if (pSSLSock->isDTLS)
    {
        TIMER_unTimer(pSSLSock, pSSLSock->dtlsHandshakeTimer);
        TIMER_destroyTimer(pSSLSock->dtlsHandshakeTimer);

        /* release handshake message defragment buffer */
        for (i = 0; i < MAX_HANDSHAKE_MESG_IN_FLIGHT; i++)
        {
            if (pSSLSock->msgBufferDescrs[i].ptr)
            {
                FREE(pSSLSock->msgBufferDescrs[i].ptr);
            }
        }
        pSSLSock->isDTLS = FALSE;
#if defined(__ENABLE_MOCANA_DTLS_SRTP__)
        if (pSSLSock->useSrtp && pSSLSock->srtpMki != NULL)
        {
            FREE(pSSLSock->srtpMki);
            pSSLSock->srtpMki = NULL;
            pSSLSock->useSrtp = FALSE;
        }
#endif
    }
#endif
}

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_send(SSLSocket* pSSLSock, const sbyte* data, sbyte4 dataSize)
{
    /* block sends between ChangeCipherSpec and Finished */
    if ((( pSSLSock->server) && ((kSslReceiveHelloState1 == SSL_HANDSHAKE_STATE(pSSLSock)) || (kSslReceiveUntil1       == SSL_HANDSHAKE_STATE(pSSLSock)))) ||
        ((!pSSLSock->server) && ((kSslReceiveHelloState1 == SSL_HANDSHAKE_STATE(pSSLSock)) || (kSslReceiveUntilResume1 == SSL_HANDSHAKE_STATE(pSSLSock)))))
    {
        return (MSTATUS)0;
    }

    return sendData(pSSLSock, SSL_APPLICATION_DATA, data, dataSize, FALSE);
}


/*------------------------------------------------------------------*/

static MSTATUS
processChangeCipherSpec(SSLSocket* pSSLSock)
{
    MSTATUS status;

    if (pSSLSock->server)
    {
        status = ERR_SSL_PROTOCOL_SERVER;

        /* is server */
        if (((SSL_CLIENT_KEY_EXCHANGE       == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) && (!pSSLSock->isMutualAuthNegotiated)) ||
            ((SSL_CLIENT_CERTIFICATE_VERIFY == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) &&  (pSSLSock->isMutualAuthNegotiated)) )
        {
            if (kSslSecureSessionEstablished != SSL_OPEN_STATE(pSSLSock))
                SSL_OPEN_STATE(pSSLSock) = kSslSecureSessionJustEstablished;

            if (OK > (status = SSL_SOCK_setClientKeyMaterial(pSSLSock)))
                goto exit;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
            if (pSSLSock->isDTLS)
            {
                pSSLSock->shouldChangeCipherSpec = FALSE;
                pSSLSock->currentPeerEpoch++;
            }
#endif

            SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_EXPECTING_FINISHED;
        }
    }
    else
    {
        status = ERR_SSL_PROTOCOL;

        /* is client */
        if (SSL_SERVER_HELLO_DONE == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock))
        {
            if (OK > (status = SSL_SOCK_setServerKeyMaterial(pSSLSock)))
                goto exit;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
            if (pSSLSock->isDTLS)
            {
                pSSLSock->shouldChangeCipherSpec = FALSE;
                pSSLSock->currentPeerEpoch++;
            }
#endif

            SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_EXPECTING_FINISHED;
        }
    }
    /* NOTE: this is needed because in DTLS, due to packet misorder, we need to
     * aggressively try this, but the state may not be ready */
    if (status < OK)
        goto exit;

    pSSLSock->pActivePeerCipherSuite = pSSLSock->pHandshakeCipherSuite;
    pSSLSock->peerSeqnum = 0;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        pSSLSock->peerSeqnumHigh = pSSLSock->peerSeqnumHigh & 0xffff0000;
        pSSLSock->receivedFinished = FALSE;
    } else
#endif
    {
        pSSLSock->peerSeqnumHigh = 0;
    }

exit:
    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_receive(SSLSocket* pSSLSock, sbyte* buffer, sbyte4 bufferSize,
                 ubyte **ppPacketPayload, ubyte4 *pPacketLength, sbyte4 *pRetNumBytesReceived)
{
    sbyte4  available;
    MSTATUS status = OK;
#if (!defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) && (defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)))
    MOC_UNUSED(buffer);
    MOC_UNUSED(bufferSize);
#endif

    *pRetNumBytesReceived = 0;

    /* if empty buffer, retrieve one record */
    /* receive a full record */
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__)
    if (((pSSLSock->internalFlags & SSL_INT_FLAG_SYNC_MODE) && (kRecordStateReceiveFrameWait == SSL_SYNC_RECORD_STATE(pSSLSock))) ||
        (pSSLSock->internalFlags & SSL_INT_FLAG_ASYNC_MODE) )
#elif (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))
    if (kRecordStateReceiveFrameWait == SSL_SYNC_RECORD_STATE(pSSLSock))
#endif /* !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__) */
    {
        ubyte               majorVersion, minorVersion;
        ubyte*              pSrh;

        if ((NULL == ppPacketPayload) || (NULL == *ppPacketPayload) ||
            (NULL == pPacketLength) || (0 == *pPacketLength))
        {
            goto exit;
        }

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            ubyte protocol = **ppPacketPayload;
            if (protocol < SSL_CHANGE_CIPHER_SPEC || protocol > SSL_INNER_APPLICATION)
            {
                SSL_RX_RECORD_STATE(pSSLSock) = SSL_ASYNC_RECEIVE_RECORD_COMPLETED;
                *ppPacketPayload += *pPacketLength;
                *pPacketLength = 0;
                status = ERR_SSL_PROTOCOL_RECEIVE_RECORD;
                goto exit;
            }
        }
#endif

        if (OK > (status = SSL_SOCK_receiveV23Record(pSSLSock, pSSLSock->pSharedInBuffer, ppPacketPayload, pPacketLength)))
            goto exit;

        pSrh = pSSLSock->pSharedInBuffer;

        if (SSL_ASYNC_RECEIVE_RECORD_COMPLETED != SSL_RX_RECORD_STATE(pSSLSock))
        {
            status = OK;
            goto exit;
        }

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (pSSLSock->isDTLS)
        {
            pSSLSock->protocol = ((DTLSRecordHeader*)pSrh)->protocol;
            minorVersion = ((DTLSRecordHeader*)pSrh)->minorVersion;
            majorVersion = ((DTLSRecordHeader*)pSrh)->majorVersion;

            status = antiReplay(pSSLSock);

            if (ERR_DTLS_DROP_REPLAY_RECORD == status)
            {
                /* silently drop replayed record */
                if (pSSLSock->pReceiveBuffer && pSSLSock->recordSize)
                {
                    pSSLSock->recordSize = pSSLSock->offset = 0;
                }

                status = OK;
                goto exit;
            }
        } else
#endif
        {
            pSSLSock->protocol = ((SSLRecordHeader*)pSrh)->protocol;
            minorVersion = ((SSLRecordHeader*)pSrh)->minorVersion;
            majorVersion = ((SSLRecordHeader*)pSrh)->majorVersion;
        }
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        /* in DTLS, since packet can be misordered and lost,
         * but ChangeCipherSpec is reflected by an increment in epoch,
         * and we simulate tcp for handshake messages (i.e. the peer will be at the right state),
         * we ignore the ChangeCipherSpec packet and use increment in epoch to
         * indicate the presence of ChangeCipherSpec */
        if (pSSLSock->isDTLS && pSSLSock->shouldChangeCipherSpec)
        {
            processChangeCipherSpec(pSSLSock); /* aggressive try: ignore the return status */
        }
#endif

        if (NULL != pSSLSock->pActivePeerCipherSuite)
        {
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
            if (pSSLSock->isDTLS && pSSLSock->currentPeerEpoch != DTLS_PEEREPOCH(pSSLSock))
            {
                /* this packet can't be decrypted successfully
                 * ignore this packet. udp can reorder packet */
                if (pSSLSock->pReceiveBuffer && pSSLSock->recordSize)
                {
                    pSSLSock->recordSize = pSSLSock->offset = 0;
                }
                status = OK;
                goto exit;
            }
#endif
            if (OK > (status = pSSLSock->pActivePeerCipherSuite->pCipherAlgo->decryptVerifyRecordFunc(pSSLSock, (ubyte)pSSLSock->protocol)))
                goto exit;
        }

        pSSLSock->offset = 0;

        if (pSSLSock->protocol != SSL_CHANGE_CIPHER_SPEC &&
            pSSLSock->protocol != SSL_ALERT &&
            pSSLSock->protocol != SSLV2_HELLO_CLIENT &&
            pSSLSock->protocol != SSL_HANDSHAKE)
        {
            /* we only deliver application data if the connection state is already opened */
            if (SSL_OPEN_STATE(pSSLSock) != kSslSecureSessionEstablished)
            {
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
                if (pSSLSock->isDTLS)
                {
                    /* silently drop data. it might be OK, since packets can be reordered */
                    if (pSSLSock->pReceiveBuffer && pSSLSock->recordSize)
                    {
                        pSSLSock->recordSize = pSSLSock->offset = 0;
                    }
                    status = OK;
                } else
#endif
                {
                    status = ERR_SSL_NOT_OPEN;
                }
                goto exit;
            }

        }

        switch (pSSLSock->protocol)
        {
            case SSL_CHANGE_CIPHER_SPEC:
            {
                /* verify versions */
                if ((pSSLSock->isDTLS && ((majorVersion != DTLS1_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion))) ||
                    ((!pSSLSock->isDTLS && ((majorVersion != SSL3_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion)))))

                {
                    status = ERR_SSL_BAD_HEADER_VERSION;
                    goto exit;
                }
                /* dtls will processChangeCipherSpec when Finished is encountered */
                if (!pSSLSock->isDTLS)
                {
                    status = processChangeCipherSpec(pSSLSock);
                }
                break;
            }

            case SSL_ALERT:
            {
                status = ERR_SSL_BAD_HEADER_VERSION;

                if ((NULL != pSSLSock->pHandshakeCipherSuite) && (kSslReceiveHelloInitState != SSL_HANDSHAKE_STATE(pSSLSock)))
                {
                /* verify versions */
                    if ((pSSLSock->isDTLS && ((majorVersion != DTLS1_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion))) ||
                        ((!pSSLSock->isDTLS && ((majorVersion != SSL3_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion)))))
                    {
                        goto exit;
                    }
                }
                else
                {
                    /* To make us future proof for TLS 1.9+ */
                    /* do not check SSL minor version here */
                    /* see processClientHello3 */
                    if ((pSSLSock->isDTLS && (majorVersion != DTLS1_MAJORVERSION)) ||
                        ((!pSSLSock->isDTLS && (majorVersion < SSL3_MAJORVERSION))))
                    {
                        goto exit;
                    }
                }

                handleAlertMessage(pSSLSock);

                break;
            }

            case SSLV2_HELLO_CLIENT:
            {
                if ((NULL != pSSLSock->pHandshakeCipherSuite) || (kSslReceiveHelloInitState != SSL_HANDSHAKE_STATE(pSSLSock)))
                    break;  /* ignore SSLv2 rehandshake hello */

                if (0 == pSSLSock->server)
                {
#ifdef __ENABLE_MOCANA_SSL_CLIENT__
                    status = SSL_SOCK_clientHandshake(pSSLSock, FALSE);
#endif
                }
                else
                {
#ifdef __ENABLE_MOCANA_SSL_SERVER__
                    status = SSL_SOCK_serverHandshake(pSSLSock, FALSE);
#endif
                }

                break;
            }

            case SSL_HANDSHAKE:
            {
                status = ERR_SSL_BAD_HEADER_VERSION;

                /* verify versions */
                if ((NULL != pSSLSock->pHandshakeCipherSuite) && (kSslReceiveHelloInitState != SSL_HANDSHAKE_STATE(pSSLSock)))
                {
                    if ((pSSLSock->isDTLS && ((majorVersion != DTLS1_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion))) ||
                        ((!pSSLSock->isDTLS && ((majorVersion != SSL3_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion)))))
                    {
                        goto exit;
                    }
                }
                else
                {
                    /* To make us future proof for TLS 1.9+ */
                    /* do not check SSL minor version here */
                    /* see processClientHello3 */
                    if ((pSSLSock->isDTLS && (majorVersion != DTLS1_MAJORVERSION)) ||
                        ((!pSSLSock->isDTLS && (majorVersion < SSL3_MAJORVERSION))))
                    {
                        goto exit;
                    }
                }

                if (0 == pSSLSock->server)
                {
#ifdef __ENABLE_MOCANA_SSL_CLIENT__
                    status = SSL_SOCK_clientHandshake(pSSLSock, FALSE);
#endif
                }
                else
                {
#ifdef __ENABLE_MOCANA_SSL_SERVER__
                    status = SSL_SOCK_serverHandshake(pSSLSock, FALSE);
#endif
                }

                break;
            }

            case SSL_INNER_APPLICATION:
            {
                /* verify versions */
                if ((pSSLSock->isDTLS && ((majorVersion != DTLS1_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion))) ||
                    ((!pSSLSock->isDTLS && ((majorVersion != SSL3_MAJORVERSION) || (minorVersion != pSSLSock->sslMinorVersion)))))
                {
                    status = ERR_SSL_BAD_HEADER_VERSION;
                    goto exit;
                }

                if (OK > (status = handleInnerAppMessage(pSSLSock)))
                    goto exit;

                break;
            }

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
            case SSL_APPLICATION_DATA:
            {
                /* TODO: should do this only after encounter of the first application data
                 * it's not efficient to check this every time afterwards
                 */
                /* by this time, handshake would have successfully finished, release retransmission buffer */
                if (pSSLSock->isDTLS)
                {
                    releaseRetransmissionBuffer(pSSLSock);
                }

                break;
            }
#endif

            default:
            {
                break;
            }
        } /* switch */

        if (SSL_APPLICATION_DATA != pSSLSock->protocol)
        {
#if (defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))
            if (SSL_FLAG_ENABLE_RECV_BUFFER & pSSLSock->runtimeFlags)
            {
                if ((SSL_INNER_APPLICATION == pSSLSock->protocol) ||
                    (SSL_ALERT             == pSSLSock->protocol))
                {
                    /* let the app harvest this data (ALERT/INNER_APP) out */
                    if (pSSLSock->recordSize > pSSLSock->offset)
                        *pRetNumBytesReceived = (ubyte4)(pSSLSock->recordSize - pSSLSock->offset);  /* yes, x - 0 == x but logical */

                    goto exit;
                }
            }
#endif
            pSSLSock->recordSize = 0;
        }

#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__)
        if ((0 < pSSLSock->recordSize) && (pSSLSock->internalFlags & SSL_INT_FLAG_SYNC_MODE))
            SSL_SYNC_RECORD_STATE(pSSLSock) = kRecordStateReceiveFrameComplete;
#elif (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))
        if (0 < pSSLSock->recordSize)
            SSL_SYNC_RECORD_STATE(pSSLSock) = kRecordStateReceiveFrameComplete;
#endif /* !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__) */
    }

    /* return as much of the record as possible */
    if (pSSLSock->recordSize > pSSLSock->offset)
    {
        available = pSSLSock->recordSize - pSSLSock->offset;  /* yes, x - 0 == x but logical */

#if ((!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)) || defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__))
#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__))
        if (pSSLSock->internalFlags & SSL_INT_FLAG_SYNC_MODE)
#endif
        {
            sbyte4 toCopy = (available < bufferSize) ? available : bufferSize;

            MOC_MEMCPY((ubyte *)buffer, (ubyte *)(pSSLSock->pReceiveBuffer + pSSLSock->offset), toCopy);
            pSSLSock->offset += toCopy;

            if (0 >= ((pSSLSock->recordSize) - (pSSLSock->offset)))
            {
                SSL_SYNC_RECORD_STATE(pSSLSock) = kRecordStateReceiveFrameWait;
            }

            *pRetNumBytesReceived = toCopy;

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__))
            goto exit;
#endif
        }
#endif /* ((!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)) || defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__)) */

#if (defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))
        if (SSL_FLAG_ENABLE_RECV_BUFFER & pSSLSock->runtimeFlags)
        {
            /* let the app harvest this data out */
            *pRetNumBytesReceived = available;
            goto exit;
        }
#endif
#if (defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__))
        if (1 == pSSLSock->server)
        {
            if (NULL != SSL_sslSettings()->funcPtrReceiveUpcall)
                SSL_sslSettings()->funcPtrReceiveUpcall(SSL_findConnectionInstance(pSSLSock), (ubyte*)pSSLSock->pReceiveBuffer, available);
        }
#endif
#if (defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))
        if (0 == pSSLSock->server)
        {
            if (NULL != SSL_sslSettings()->funcPtrClientReceiveUpcall)
                SSL_sslSettings()->funcPtrClientReceiveUpcall(SSL_findConnectionInstance(pSSLSock), (ubyte*)pSSLSock->pReceiveBuffer, available);
        }
#endif
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL_SOCK_receive() returns status = ", status);
#endif

    if (*pRetNumBytesReceived )
        status = (MSTATUS)(*pRetNumBytesReceived);

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_getCipherId( SSLSocket* pSSLSock, ubyte2* pCipherId)
{
    if (!pSSLSock)
        return ERR_NULL_POINTER;

    *pCipherId = (pSSLSock->pHandshakeCipherSuite) ?
                    pSSLSock->pHandshakeCipherSuite->cipherSuiteId :
                    0;
    return OK;
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
extern sbyte4
SSL_SOCK_numCiphersAvailable(void)
{
    return NUM_CIPHER_SUITES;
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
extern ubyte4
SSL_SOCK_getECCCurves(void)
{
    return SUPPORTED_CURVES_FLAGS;
}
#endif


/*------------------------------------------------------------------*/

#if defined ( __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__) || defined( __ENABLE_RFC3546__)
extern sbyte4
SSL_SOCK_getCipherTableIndex(SSLSocket* pSSLSock, ubyte2 cipherId)
{
    sbyte4  retIndex = -1;
    sbyte4  index;

    for (index = 0; index < NUM_CIPHER_SUITES; index++)
    {
        if ((cipherId == gCipherSuites[index].cipherSuiteId) &&
            (0 != gCipherSuites[index].supported))
        {
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
            if (pSSLSock->isDTLS && isCipherIdExcludedForDTLS(cipherId))
            {
                /* DTLS doesnot support stream ciphers */
                break;
            } else
#endif
            {
                retIndex = index;
                break;
            }
        }
    }

    return retIndex;
}
#endif /* ( __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__) || defined( __ENABLE_RFC3546__) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#if (defined(__ENABLE_MOCANA_DTLS_SRTP__) && defined(__ENABLE_MOCANA_SRTP_PROFILES_SELECT__))
extern sbyte4
SSL_SOCK_numSrtpProfilesAvailable(void)
{
    return NUM_SRTP_PROFILES;
}


/*------------------------------------------------------------------*/

extern sbyte4
SSL_SOCK_getSrtpProfileIndex(SSLSocket* pSSLSock, ubyte2 profileId)
{
    sbyte4  retIndex = -1;
    sbyte4  index;

    for (index = 0; index < NUM_SRTP_PROFILES; index++)
    {
        if ((profileId == gSrtpProfiles[index].profileId) &&
            (0 != gSrtpProfiles[index].supported))
        {
            retIndex = index;
            break;
        }
    }

    return retIndex;
}
#endif /* (__ENABLE_MOCANA_DTLS_SRTP__) && (__ENABLE_MOCANA_SRTP_PROFILES_SELECT__) */
#endif /* (__ENABLE_MOCANA_DTLS_CLIENT__) || (__ENABLE_MOCANA_DTLS_SERVER__) */




/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)) || (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined (__ENABLE_MOCANA_ECC__))

static MSTATUS
SSL_SOCK_getECCSignatureLength( ECCKey* pECCKey, sbyte4* signatureLen)
{
    /* computing the ECDSA signature length is complex -- it's DER encoded
    since the largest signature is with P521 -- R and S are at most 66 bytes
    long -- so TLV is 1 + 1 + 66 (68) for each -- (no leading zero for this
    curve but other curves might need one) and TLV for sequence is
    1 + 2 + 2 * 68 (2 bytes for length since 68*2 > 127 ) -- code is more generic
    This is actually the maximum length needed because the presence of leading
    zeros cannot be predicted before the signature is computed */
    MSTATUS         status;
    PrimeFieldPtr   pPF;
    sbyte4          elementLen;

    pPF = EC_getUnderlyingField( pECCKey->pCurve);

    if (OK > ( status = PRIMEFIELD_getElementByteStringLen( pPF, &elementLen)))
        return status;

    elementLen += 3; /* leading zero + type + length */
    *signatureLen = 2 * elementLen + 3; /* 2 INTEGER + type + length */
    return OK;
}
#endif /* defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined (__ENABLE_MOCANA_ECC__)) */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_RFC3546__
static MSTATUS
processHelloExtensions(SSLSocket* pSSLSock, ubyte *pExtensions,
                       sbyte4 extensionsLen)
{
    /* this function parses the extensions doing some checks before
    calling a role specific function for each extension */
    ubyte2  extensionType;
    ubyte2  extensionSize;
    ubyte4  extensionMask[2] = { 0, 0}; /* 64 bits available */
    MSTATUS status = OK;

    while ((OK <= status) && (0 < extensionsLen))
    {
        if (4 > extensionsLen)
        {
            status = ERR_SSL_PROTOCOL_PROCESS_CLIENT_HELLO;
            goto exit;
        }

        extensionType = getShortValue(pExtensions);
        pExtensions += 2;    extensionsLen -= 2;

        extensionSize = getShortValue(pExtensions);
        pExtensions += 2;    extensionsLen -= 2;

        /* RFC-3546: There MUST NOT be more than one extension of the same type. */
        if (32 > extensionType)
        {
            /* prevent duplicate extensions */
            if (extensionMask[0] & (1 << extensionType))
            {
                /* RFC-3546: There MUST NOT be more than one extension of the same type. */
                status = ERR_SSL_EXTENSION_DUPLICATE;
                goto exit;
            }

            extensionMask[0] |= (1 << extensionType);
        }
        else if ( 64 > extensionType)
        {
            /* prevent duplicate extensions */
            if (extensionMask[1] & (1 << (extensionType-32)))
            {
                /* RFC-3546: There MUST NOT be more than one extension of the same type. */
                status = ERR_SSL_EXTENSION_DUPLICATE;
                goto exit;
            }

            extensionMask[1] |= (1 << (extensionType-32));
        }

        /* check the size */
        if ( extensionsLen < (sbyte4) extensionSize )
        {
            /* buffer overrun attack? */
            status = ERR_SSL_EXTENSION_LENGTH;
            goto exit;
        }

        if (0 == pSSLSock->clientHelloMinorVersion)
        {
            /* renegotiation is the only extension supported for SSL 3.0 */
            if (tlsExt_renegotiated_connection != extensionType)
            {
                status = ERR_SSL_INVALID_MSG_SIZE;
                goto exit;
            }
        }

#if defined(__ENABLE_MOCANA_SSL_SERVER__)
        if (1 == pSSLSock->server)
        {
            status = processClientHelloExtension(pSSLSock, extensionType, extensionSize,
                                                 pExtensions);
        }
#endif

#if defined( __ENABLE_MOCANA_SSL_CLIENT__)
        if (0 == pSSLSock->server)
        {
            status = processServerHelloExtensions(pSSLSock, extensionType, extensionSize,
                                                 pExtensions);
        }
#endif

        /* move to the next extension */
        pExtensions   += extensionSize;
        extensionsLen -= extensionSize;

    } /* while */

exit:
    return status;
}
#endif /* __ENABLE_RFC3546__ */


/*------------------------------------------------------------------*/

extern MSTATUS
SSLSOCK_sendEncryptedHandshakeBuffer(SSLSocket* pSSLSock)
{
    MSTATUS status = OK;

    if ((NULL != pSSLSock) && (NULL != pSSLSock->buffers[0].pHeader))
    {
        while (pSSLSock->bufIndex < pSSLSock->numBuffers)
        {
            sbyte4  numBytesSent = 0;
            sbyte4  dataSize;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
            if (pSSLSock->isDTLS)
            {
                dataSize = pSSLSock->buffers[pSSLSock->bufIndex].length - sizeof(DTLSRecordHeader);
                /* sendData() will redo the record header in sendData() */
                if (0xffffffff == (--pSSLSock->ownSeqnum))
                    pSSLSock->ownSeqnumHigh = (pSSLSock->ownSeqnumHigh & 0xffff0000) | ((pSSLSock->ownSeqnumHigh - 1) & 0xffff);
            } else
#endif
            {
                dataSize = pSSLSock->buffers[pSSLSock->bufIndex].length - sizeof(SSLRecordHeader);
            }

            while ((0 <= status) && (numBytesSent < dataSize))
            {
                if (OK > (status = sendData(pSSLSock, SSL_HANDSHAKE,
                    (sbyte *)pSSLSock->buffers[pSSLSock->bufIndex].data + numBytesSent,
                    (dataSize - numBytesSent), TRUE)))
                {
                    goto exit;
                }
                numBytesSent += status;
            }
            pSSLSock->bufIndex++;

            /* send is flow controlling, we need to stop pushing data out for now */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                goto exit;
        }

        CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&(pSSLSock->buffers[0].pHeader));
        pSSLSock->bufIndex = 0;
        pSSLSock->numBuffers = 0;

        status = OK;
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSLSOCK_sendEncryptedHandshakeBuffer() returns status = ", status);
#endif

    return status;

} /* SSLSOCK_sendEncryptedHandshakeBuffer */

#endif /* (defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_CLIENT__)) */
