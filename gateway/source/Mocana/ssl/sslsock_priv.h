/* Version: mss_v6_3 */
/*
 * sslsock_priv.h
 *
 * SSL implementation internal definitions
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __SSLSOCK_PRIV_H__
#define __SSLSOCK_PRIV_H__

#ifdef __cplusplus
extern "C" {
#endif

/* one can override these values in the make/project/moptions.h file */
#ifndef __MOCANA_PARENT_CERT_CHECK_OPTIONS__
#define __MOCANA_PARENT_CERT_CHECK_OPTIONS__    (0xFFFF) /* everything turned on by default */
#endif

#ifndef __MOCANA_SELFSIGNED_CERT_CHECK_OPTIONS__
#define __MOCANA_SELFSIGNED_CERT_CHECK_OPTIONS__    (0xFFFF) /* everything turned on by default */
#endif

/* FOR NOW, we force RSA to be enabled. */
#ifndef __ENABLE_MOCANA_SSL_RSA_SUPPORT__
#define __ENABLE_MOCANA_SSL_RSA_SUPPORT__
#endif


/*------------------------------------------------------------------*/

MOC_EXTERN sbyte4 SSL_findConnectionInstance(SSLSocket *pSSLSock);

#ifndef SSL_PACKED
#define SSL_PACKED
#endif

#ifndef SSL_CERT_VERIFY_ARG
#define SSL_CERT_VERIFY_ARG                 (0)
#endif

#ifndef TLS_EAP_PAD
#define TLS_EAP_PAD                         (52)
#endif

#ifndef SSL_PACKED_POST
#ifdef __GNUC__
#define SSL_PACKED_POST    __attribute__ ((__packed__))
#else
#define SSL_PACKED_POST
#endif
#endif

/* size of the weird medium SSL type */
#define SSL_MEDIUMSIZE                      (3)

/* SSL Record layer: max record size */
#ifndef SSL_RECORDSIZE
#define SSL_RECORDSIZE                      (16384)
#endif

#define SSL_MAX_RECORDSIZE                  (SSL_RECORDSIZE + 2048)

#ifndef SSL_DEFAULT_SMALL_BUFFER
#define SSL_DEFAULT_SMALL_BUFFER            (4096)
#endif

/* SSL Record Header type */
#define SSL_CHANGE_CIPHER_SPEC              (20)
#define SSL_ALERT                           (21)
#define SSL_HANDSHAKE                       (22)
#define SSL_APPLICATION_DATA                (23)
#define SSL_INNER_APPLICATION               (24)

#define SSLV2_HELLO_CLIENT                  (128)

/* SSL Handshake Record type also used as a Handshake state identifier */
#define SSL_BEGIN                           (-1)
#define SSL_HELLO_REQUEST                   (0)
#define SSL_CLIENT_HELLO                    (1)
#define SSL_SERVER_HELLO                    (2)
#define SSL_SERVER_HELLO_VERIFY_REQUEST     (3)
#define SSL_CERTIFICATE                     (11)
#define SSL_SERVER_KEY_EXCHANGE             (12)
#define SSL_CERTIFICATE_REQUEST             (13)
#define SSL_SERVER_HELLO_DONE               (14)
#define SSL_CLIENT_CERTIFICATE_VERIFY       (15)
#define SSL_CLIENT_KEY_EXCHANGE             (16)
#define SSL_EXPECTING_FINISHED              (19) /* not a valid record type -> only a state */
#define SSL_FINISHED                        (20)
#ifdef __ENABLE_TLSEXT_RFC6066__
#define SSL_CERTIFCATE_STATUS               (22)
#endif

#define TLS_MASTERSECRETSIZE                (13) /* length of "master secret" */
#define TLS_KEYEXPANSIONSIZE                (13) /* length of "key expansion" */
#define TLS_FINISHEDLABELSIZE               (15)  /* length of "server finished"
                                                    and "client finished" */
#define TLS_VERIFYDATASIZE                  (12)

#define START_RANDOM(pS)                    (pS->pSecretAndRand + SSL_MASTERSECRETSIZE)

#ifdef __ENABLE_MOCANA_EAP_FAST__
#define EAPFAST_PAC_MASTERSECRET_HASH       (32) /* length of "PAC to master secret label hash\0" including null*/
#define EAPFAST_IM_COMPOUND_KEY_SIZE        (27) /* length of "Inner Methods Compound Keys" */
#define EAPFAST_IM_MSK_SIZE                 (32)
#endif

#ifdef __ENABLE_MOCANA_INNER_APP__
#define  SSL_INNER_APP_CLIENT_PHRASE        "client phase finished"
#define  SSL_INNER_APP_CLIENT_PHRASE_LEN    (21)
#define  SSL_INNER_APP_SERVER_PHRASE        "server phase finished"
#define  SSL_INNER_APP_SERVER_PHRASE_LEN    (21)
#define  SSL_INNER_APP_SECRET_PHRASE        "inner secret permutation"
#define  SSL_INNER_APP_SECRET_PHRASE_LEN    (24)
#endif

enum hashTypes
{
    hashTypeSSLv3CertificateVerify = 0,     /* matches SSLv3 minor value */
    hashTypeSSLv3Finished          = 0xFFFFFFFF
};

/* optimization for hardware accelerators */
#ifndef SSL_MALLOC_BLOCK_SIZE
#define SSL_MALLOC_BLOCK_SIZE               (16)
#elif (13 > SSL_MALLOC_BLOCK_SIZE)
#error SSL_MALLOC_BLOCK_SIZE cannot be less than 13!
#endif

#define SSL_SMALL_TEMP_BUF_SIZE             (64)
#define SSL_BIGGER_TEMP_BUF_SIZE            (96)
#define SSL_HASH_SEQUENCE_SIZE              (8)

/* careful with semicolons following the invocation of this macro */
#define SSL_RX_RECORD_STATE_CHANGE(X,Y)     { SSL_RX_RECORD_STATE(X) = Y; SSL_RX_RECORD_STATE_INIT(pSSLSock) = FALSE; }


/*------------------------------------------------------------------*/

/* CodeWarrior some flavors need this...  #pragma pack(push,1) */

typedef SSL_PACKED struct SSLClientHelloV2Fixed {
    ubyte recordType;
    ubyte recordLen;
    ubyte msgType;
    ubyte majorVersion;
    ubyte minorVersion;
    ubyte zero1;
    ubyte cipherSuiteLen;
    ubyte zero2;
    ubyte sessionLen;
    ubyte zero3;
    ubyte challengeLen;
} SSL_PACKED_POST SSLClientHelloV2Fixed;

typedef SSL_PACKED union SSLClientHelloV2 {
    SSLClientHelloV2Fixed    record;
    ubyte            array[258];
} SSL_PACKED_POST SSLClientHelloV2;

#define SSLRecordHeaderPart     ubyte    protocol; ubyte    majorVersion;ubyte    minorVersion; ubyte    recordLength[2]
#define SSL_SET_RECORD_HEADER(PTR,PROTO,MINVER,LEN)   { PTR[0] = PROTO; PTR[1] = SSL3_MAJORVERSION; PTR[2] = MINVER; PTR[3] = (ubyte)((LEN) >> 8); PTR[4] = (ubyte)(LEN); }
#define SSLHandshakeHeaderPart     ubyte    handshakeType; ubyte    handshakeSize[SSL_MEDIUMSIZE]

typedef SSL_PACKED struct SSLRecordHeader
{
    SSLRecordHeaderPart;
} SSL_PACKED_POST SSLRecordHeader;

typedef SSL_PACKED struct SSLChangeCipherSpecRecord {
    SSLRecordHeaderPart;
    ubyte changeCipherSpec;
} SSL_PACKED_POST SSLChangeCipherSpecRecord;

typedef SSL_PACKED struct SSLAlertRecord {
    SSLRecordHeaderPart;
    ubyte level;
    ubyte description;
} SSL_PACKED_POST SSLAlertRecord;

typedef SSL_PACKED struct SSLHandshakeRecord {
    SSLRecordHeaderPart;
    SSLHandshakeHeaderPart;
} SSL_PACKED_POST SSLHandshakeRecord;

typedef SSL_PACKED struct SSLHandshakeHeader {
    SSLHandshakeHeaderPart;
} SSL_PACKED_POST SSLHandshakeHeader;

typedef SSL_PACKED union SSLSharedInBuffer {
    SSLRecordHeader  srh;
    SSLClientHelloV2 clientHello;
} SSL_PACKED_POST SSLSharedInBuffer;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#define DTLSRecordHeaderPart     ubyte    protocol; ubyte    majorVersion;ubyte    minorVersion; ubyte epoch[2]; ubyte seqNo[6]; ubyte    recordLength[2]
#define DTLS_SET_RECORD_HEADER(PTR,PROTO,MINVER,SEQNOHIGH,SEQNO,LEN)   { PTR[0] = PROTO; PTR[1] = DTLS1_MAJORVERSION; PTR[2] = MINVER; PTR[3] = (ubyte)((SEQNOHIGH & 0xff000000) >> 24); PTR[4] = (ubyte)((SEQNOHIGH & 0x00ff0000) >> 16); PTR[5] = (ubyte)((SEQNOHIGH & 0x0000ff00) >> 8); PTR[6] = (ubyte)(SEQNOHIGH & 0x000000ff); PTR[7] = (ubyte)((SEQNO) >> 24); PTR[8] = (ubyte)((SEQNO & 0x00ff0000) >> 16); PTR[9] = (ubyte)((SEQNO & 0x0000ff00) >> 8); PTR[10] = (ubyte)(SEQNO & 0x000000ff); PTR[11] = (ubyte)((LEN) >> 8); PTR[12] = (ubyte)(LEN); }
#define DTLSHandshakeHeaderPart     ubyte    handshakeType; ubyte    handshakeSize[SSL_MEDIUMSIZE]; ubyte msgSeq[2]; ubyte fragOffset[3]; ubyte fragLength[3]

#define DTLS_SET_RECORD_HEADER_EXT(PTR,SSLSOCK,PROTO,LEN)        {DTLS_SET_RECORD_HEADER(PTR,PROTO,SSLSOCK->sslMinorVersion, SSLSOCK->ownSeqnumHigh, SSLSOCK->ownSeqnum, LEN); if (0 == (++SSLSOCK->ownSeqnum)) {SSLSOCK->ownSeqnumHigh = (SSLSOCK->ownSeqnumHigh & 0xffff0000) | ((SSLSOCK->ownSeqnumHigh + 1) & 0xffff);}}

#define DTLS_SET_HANDSHAKE_HEADER_EXTRA(SHSH, OFFSET, LEN) {setShortValue(SHSH->msgSeq, OFFSET); setMediumValue(SHSH->fragOffset, 0); setMediumValue(SHSH->fragLength, LEN);}

#define DTLS_RECORD_SIZE(X) ((((ubyte2)X[11]) << 8) | ((ubyte2)X[12]))

typedef SSL_PACKED struct DTLSRecordHeader
{
    DTLSRecordHeaderPart;
} SSL_PACKED_POST DTLSRecordHeader;


typedef SSL_PACKED struct DTLSChangeCipherSpecRecord {
    DTLSRecordHeaderPart;
    ubyte changeCipherSpec;
} SSL_PACKED_POST DTLSChangeCipherSpecRecord;

typedef SSL_PACKED struct DTLSAlertRecord {
    DTLSRecordHeaderPart;
    ubyte level;
    ubyte description;
} SSL_PACKED_POST DTLSAlertRecord;

typedef SSL_PACKED struct DTLSHandshakeRecord {
    DTLSRecordHeaderPart;
    DTLSHandshakeHeaderPart;
} SSL_PACKED_POST DTLSHandshakeRecord;

typedef SSL_PACKED struct DTLSHandshakeHeader {
    DTLSHandshakeHeaderPart;
} SSL_PACKED_POST DTLSHandshakeHeader;

typedef SSL_PACKED union DTLSSharedInBuffer {
    DTLSRecordHeader  srh;
    SSLClientHelloV2 clientHello;
} SSL_PACKED_POST DTLSSharedInBuffer;

#endif

/* CodeWarrior some flavors need this...  #pragma pack(pop) */

/*------------------------------------------------------------------*/

typedef const struct KeyExAuthSuiteInfo
{
    ubyte4                  flags;          /* e.g. SSL_RSA, SSL_DH_RSA, SSL_DH_ANON, SSL_DH_PSK */

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    MSTATUS(*FillServerKEX)(SSLSocket* pSSLSock, ubyte *pHSH, ubyte *pPskHint, ubyte4 pskHintLength);
#endif

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
    MSTATUS(*ProcessServerKEX)(SSLSocket *pSSLSock, ubyte* pMessage, ubyte2 recLen);
#endif

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
    MSTATUS(*FillClientKEX)(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue);
#endif

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    MSTATUS(*ProcessClientKEX)(SSLSocket* pSSLSock, ubyte* pMessage, ubyte2 recLen, vlong **ppVlongQueue);
#endif

} KeyExAuthSuiteInfo;

typedef enum {Block_Size=1, Hash_Size=2, FixedIV_Size=3, RecordIV_Size=4, TagLen=5} CipherField;

/* generic algorithm definition for both combination algo, ie. encryption+hash, and AEAD algos */
typedef BulkCtx (*SSLCipherAlgoCreateCtx)(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* key, ubyte4 keySize, sbyte4 encrypt);
typedef MSTATUS (*SSLCipherAlgoDeleteCtx)(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);
typedef MSTATUS (*SSLCipherAlgoEncryptRecordFunc)(SSLSocket *pSSLSock, ubyte* pData, ubyte2 dataLength, sbyte padLength);
typedef MSTATUS (*SSLCipherAlgoDecryptVerifyRecordFunc)(SSLSocket *pSSLSock, ubyte protocol);
typedef MSTATUS (*SSLCipherAlgoGetFieldFunc)(CipherField type);

typedef struct SSLCipherAlgo
{
    ubyte4 blockOrFixedIVSize; /* blockSize for non-Aead ciphers; implicitNonceSize for Aead ciphers */
    ubyte4 hashOrRecordIVSize; /* hashSize for non-Aead ciphers; explictNonceSize for Aead ciphers */
    ubyte4 tagLen; /* increase in size for Aead ciphers */

    SSLCipherAlgoCreateCtx createCtxFunc;
    SSLCipherAlgoDeleteCtx deleteCtxFunc;
    SSLCipherAlgoEncryptRecordFunc encryptRecordFunc;
    SSLCipherAlgoDecryptVerifyRecordFunc decryptVerifyRecordFunc;
    SSLCipherAlgoGetFieldFunc getFieldFunc;
} SSLCipherAlgo;

typedef const struct CipherSuiteInfo
{
    ubyte2                     cipherSuiteId;              /* cipher identification */
    ubyte                      supported;                  /* support by this implementation */
    ubyte                      minimumSSLVersion;          /* minimum SSL version for this cipher (0=SSLv3, 1=TLS1.0, 2=TLS1.1, 3=TLS1.2 */
    sbyte4                     keySize;                    /* size of key */
    const SSLCipherAlgo       *pCipherAlgo;
    KeyExAuthSuiteInfo        *pKeyExAuthAlgo;
    const BulkHashAlgo        *pPRFHashAlgo;

#if defined(__SSL_SINGLE_PASS_SUPPORT__)
    typeForSinglePass          sslSinglePassInCookie;      /* for in-bound single pass hw offload */
    typeForSinglePass          sslSinglePassOutCookie;     /* for out-bound single pass hw offload */
#endif

} CipherSuiteInfo;

#define IMPLICIT_IV_SIZE(VER, CIPHER) ((VER < TLS11_MINORVERSION)?  CIPHER->pCipherAlgo->getFieldFunc(1): CIPHER->pCipherAlgo->getFieldFunc(3))
#define EXPLICIT_IV_SIZE(VER, CIPHER) ((VER >= TLS11_MINORVERSION)? (CIPHER->pCipherAlgo->getFieldFunc(1) == 0? CIPHER->pCipherAlgo->getFieldFunc(4) : CIPHER->pCipherAlgo->getFieldFunc(1)) : 0)

/*---------------------------------------------------------------------------*/

/* MOC_EXTERN functions: implementation internal */
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
MOC_EXTERN MSTATUS getNumBytesSent(ubyte *pOutputBuffer, ubyte4 maxLen, ubyte4 *pNumBytesSent);
MOC_EXTERN MSTATUS cleanupOutputBuffer(SSLSocket *pSSLSock);
#endif

MOC_EXTERN MSTATUS SSL_rngFun( ubyte4 len, ubyte buff[/*len*/]);

#ifdef __cplusplus
}
#endif


#endif
