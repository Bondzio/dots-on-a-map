/* Version: mss_v6_3 */
/*
 * sslsock.h
 *
 * SSL implementation
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __SSLSOCK_H__
#define __SSLSOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__)
#define IS_SSL_ASYNC(X)         ((X)->internalFlags & SSL_INT_FLAG_ASYNC_MODE)
#define IS_SSL_SYNC(X)          ((X)->internalFlags & SSL_INT_FLAG_SYNC_MODE)

#elif defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
#define IS_SSL_ASYNC(X)         (1)
#define IS_SSL_SYNC(X)          (0)

#else
#define IS_SSL_ASYNC(X)         (0)
#define IS_SSL_SYNC(X)          (1)
#endif

#define CONNECT_DISABLED        0
#define CONNECT_CLOSED          1
#define CONNECT_NEGOTIATE       2
#define CONNECT_OPEN            3

#if defined(__ENABLE_TLSEXT_RFC6066__) && defined(__ENABLE_MOCANA_OCSP_CLIENT__)
#define MAX_OCSP_TRUSTED_RESPONDERS 5
#endif

/*------------------------------------------------------------------*/

enum sslAsyncStates
{
    SSL_ASYNC_RECEIVE_RECORD_1,
    SSL_ASYNC_RECEIVE_RECORD_2,
    SSL_ASYNC_RECEIVE_RECORD_COMPLETED
};

enum sslHandshakeStates
{
    kSslReceiveHelloInitState0, /* used by DTLS for sending server HelloVerifyRequest */
    kSslReceiveHelloInitState,
    kSslReceiveHelloState,
    kSslReceiveHelloState1,
    kSslReceiveHelloState2,
    kSslReceiveUntil,
    kSslReceiveUntil1,
    kSslReceiveUntil2,
    kSslReceiveUntil3,
    kSslReceiveUntilResume,
    kSslReceiveUntilResume1,
    kSslReceiveUntilResume2,
    kSslReceiveUntilResume3,
    kSslOpenState
};

enum sslOpenStates
{
    kSslSecureSessionNotEstablished = 0,
    kSslSecureSessionJustEstablished,
    kSslSecureSessionEstablished
};

enum sslSyncRecordStates
{
    kRecordStateReceiveFrameWait = 0,
    kRecordStateReceiveFrameComplete
};

#define SSL_RANDOMSIZE                  (32)
#define SSL_RSAPRESECRETSIZE            (48)

#define SSL_MAXMACSECRETSIZE            (SHA384_RESULT_SIZE)
#define SSL_MAXKEYSIZE                  (32)
#define SSL_MAXIVSIZE                   (20)   /* FORTEZZA */

#ifdef __ENABLE_MOCANA_EAP_FAST__
#define PACKEY_SIZE                     (32)
#define SKS_SIZE                        (40)
#define FAST_MSCHAP_CHAL_SIZE           (32)
#define SSL_MAXMATERIALS                (2 * (SSL_MAXMACSECRETSIZE + SSL_MAXKEYSIZE) + SKS_SIZE + FAST_MSCHAP_CHAL_SIZE)
#else
#define SSL_MAXMATERIALS                (2 * (SSL_MAXMACSECRETSIZE + SSL_MAXKEYSIZE + SSL_MAXIVSIZE))
#endif

#ifdef __ENABLE_MOCANA_INNER_APP__
#define SSL_INNER_SECRET_SIZE           (48)
#endif

//#ifdef __ENABLE_MOCANA_SSL_EXTERNAL_CERT_CHAIN_VERIFY__
#define SSL_MAX_NUM_CERTS_IN_CHAIN      (10)
//#endif

#define SSL_MAXROUNDS                   (1 + (SSL_MAXMATERIALS / MD5_DIGESTSIZE ))
/*    key material size  this is the maximum space occupied for the
    encryption material, it includes 2 write MAC secrets  */

#define SSL_SHA1_PADDINGSIZE            (40)
#define SSL_MD5_PADDINGSIZE             (48)
#define SSL_MAX_PADDINGSIZE             (SSL_MD5_PADDINGSIZE)
#define SSL_FINISHEDSIZE                (SHA_HASH_RESULT_SIZE + MD5_DIGESTSIZE)
#define SSL_MAXDIGESTSIZE               (SHA384_RESULT_SIZE)

/*
 *  o  "client_verify_data":  the verify_data from the Finished message
 *     sent by the client on the immediately previous handshake.  For
 *     currently defined TLS versions and cipher suites, this will be a
 *     12-byte value; for SSLv3, this will be a 36-byte value.
 *
 *  o  "server_verify_data":  the verify_data from the Finished message
 *     sent by the server on the immediately previous handshake.
 */
#define SSL_VERIFY_DATA                 (SSL_FINISHEDSIZE)

#define SSL_RX_RECORD_STATE(X)          (X)->asyncState
#define SSL_RX_RECORD_BUFFER(X)         (X)->pAsyncBuffer
#define SSL_RX_RECORD_BYTES_READ(X)     (X)->asyncBytesRead
#define SSL_RX_RECORD_BYTES_REQUIRED(X) (X)->asyncBytesRequired
#define SSL_RX_RECORD_STATE_INIT(X)     (X)->asyncStateInit

#define SSL_SYNC_RECORD_STATE(X)        (X)->recordState

#define SSL_HANDSHAKE_STATE(X)          (X)->sslHandshakeState
#define SSL_REMOTE_HANDSHAKE_STATE(X)   (X)->theirHandshakeState

#define SSL_OPEN_STATE(X)               (X)->openState

#define SSL_TIMER_START_TIME(X)         (X)->timerStartTime
#define SSL_TIMER_MS_EXPIRE(X)          (X)->timerMsExpire

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#define DTLS_TIMER_STATE(X)             (X)->dtlsTimerState
#define DTLS_PEEREPOCH(X)               ((X)->peerSeqnumHigh >> 16)
#endif

#define SSL_KEYEX_RSA_BIT               (0x001)
#define SSL_KEYEX_DH_BIT                (0x002)
#define SSL_KEYEX_ECDH_BIT              (0x004)
#define SSL_KEYEX_ECDHE_BIT             (0x008)

#define SSL_AUTH_RSA_BIT                (0x010)
#define SSL_AUTH_ANON_BIT               (0x020)
#define SSL_AUTH_ECDSA_BIT              (0x040)

#define SSL_PSK_BIT                     (0x100)
#define SSL_NO_MUTUAL_AUTH_BIT          (0x400)

#define SSL_RSA                         (SSL_KEYEX_RSA_BIT  | SSL_AUTH_RSA_BIT )
#define SSL_ECDH_RSA                    (SSL_KEYEX_ECDH_BIT | SSL_AUTH_RSA_BIT)
#define SSL_ECDH_ECDSA                  (SSL_KEYEX_ECDH_BIT | SSL_AUTH_ECDSA_BIT)
#define SSL_ECDHE_RSA                   (SSL_KEYEX_ECDHE_BIT| SSL_AUTH_RSA_BIT)
#define SSL_ECDHE_ECDSA                 (SSL_KEYEX_ECDHE_BIT| SSL_AUTH_ECDSA_BIT)
#define SSL_ECDH_ANON                   (SSL_KEYEX_ECDHE_BIT| SSL_AUTH_ANON_BIT | SSL_NO_MUTUAL_AUTH_BIT)
#define SSL_ECDH_PSK                    (SSL_KEYEX_ECDHE_BIT| SSL_AUTH_ANON_BIT | SSL_PSK_BIT | SSL_NO_MUTUAL_AUTH_BIT)
#define SSL_RSA_PSK                     (SSL_RSA            | SSL_PSK_BIT       | SSL_NO_MUTUAL_AUTH_BIT)
#define SSL_DH_RSA                      (SSL_KEYEX_DH_BIT   | SSL_AUTH_RSA_BIT)
#define SSL_DH_ANON                     (SSL_KEYEX_DH_BIT   | SSL_AUTH_ANON_BIT | SSL_NO_MUTUAL_AUTH_BIT)
#define SSL_DH_PSK                      (SSL_KEYEX_DH_BIT   | SSL_AUTH_ANON_BIT | SSL_PSK_BIT        | SSL_NO_MUTUAL_AUTH_BIT)
#define SSL_PSK                         (SSL_PSK_BIT        | SSL_AUTH_ANON_BIT | SSL_NO_MUTUAL_AUTH_BIT)

/* Mocana SSL Internal Flags */
#define SSL_INT_FLAG_SYNC_MODE          (0x00000001)
#define SSL_INT_FLAG_ASYNC_MODE         (0x00000002)

typedef ubyte4 SESSIONID;

typedef enum
{
    E_NoSessionResume = 0,
    E_SessionIDResume = 1,
    E_SessionTicketResume = 2,
    E_SessionEAPFASTResume = 3
} E_SessionResumeType;

/* forward declare */
struct CipherSuiteInfo;
struct diffieHellmanContext;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
enum dtlsTimerStates
{
    kDtlsPreparing,
    kDtlsSending,
    kDtlsWaiting,
    kDtlsFinished,
    kDtlsUnknown
};

typedef struct msgBufferDescr
{
    ubyte *ptr;
    ubyte4 recordSize;
    ubyte2 firstHoleOffset;
} msgBufferDescr;

typedef struct retransBufferDescr
{
    ubyte   recordType;
    ubyte*  pData;
    ubyte4  length;
} retransBufferDescr;

#define MAX_HANDSHAKE_MESG_IN_FLIGHT  (8)
#define DTLS_HANDSHAKE_HEADER_SISE    (12)
#define DTLS_MAX_REPLAY_WINDOW_SIZE   (64)


#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#if defined(__ENABLE_MOCANA_DTLS_SRTP__) && defined(__ENABLE_MOCANA_SRTP_PROFILES_SELECT__)
#define SRTP_MAX_NUM_PROFILES (16)
#endif
#endif

#endif

/* arrange the structure so that number of hash operations is limited */
typedef struct SSLSocket
{

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    peerDescr                       peerDescr;
    ubyte*                          dtlsHandshakeTimer;
    ubyte4                          dtlsHandshakeTimeout; /* in millisecond */
    ubyte4                          dtlsPMTU;
#endif
    TCP_SOCKET                      tcpSock;
    /* timer support */
    moctime_t                       timerStartTime;
    ubyte4                          timerMsExpire;
    /* configuration struct for Enable Disable certificate parameter check */
    certParamCheckConf              certParamCheck;

    /* used for receiving data asynchronously */
    enum sslOpenStates              openState;
    enum sslAsyncStates             asyncState;
    enum sslSyncRecordStates        recordState;
    ubyte*                          pAsyncBuffer;
    ubyte4                          asyncBytesRead;
    ubyte4                          asyncBytesRequired;
    intBoolean                      asyncStateInit;
    ubyte*                          pSharedInBuffer;

    /* send buffered data out */
    ubyte*                          pOutputBufferBase;      /* malloc'd base */
    ubyte*                          pOutputBuffer;          /* current position */
    ubyte4                          outputBufferSize;       /* size of output buffer */
    ubyte4                          numBytesToSend;         /* number of bytes pending inside of buffer */

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    intBoolean                      isRetransmit;
    retransBufferDescr              retransBuffers[MAX_HANDSHAKE_MESG_IN_FLIGHT];   /* remember for retransmission */
#endif

    /* used for handshake only */
    ubyte4                          handshakeCount;
    RNGFun                          rngFun;
    void*                           rngFunArg;
    enum sslHandshakeStates         sslHandshakeState;      /* my handshake state */
    sbyte4                          theirHandshakeState;    /* peer's handshake state */
#ifdef __ENABLE_MOCANA_SSL_REHANDSHAKE__
    /* Timer count for rehandshake */
    moctime_t    sslRehandshakeTimerCount;

    /* Number of bytes send count for rehandshake */
    sbyte4       sslRehandshakeByteSendCount;
#endif 
#if defined(__ENABLE_SSL_DYNAMIC_CERTIFICATE__) || (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
    certStorePtr                    pCertStore;
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    enum dtlsTimerStates            dtlsTimerState;
    ubyte2                          nextSendSeq; /* next sending handshake message sequence counter */
    ubyte2                          nextRecvSeq; /* next receive handshake message sequence counter */
    msgBufferDescr                  msgBufferDescrs[MAX_HANDSHAKE_MESG_IN_FLIGHT]; /* max outstanding messages in a flight */
    ubyte4                          msgBase;     /* base sequence number of handshake message flight */
    ubyte                           HSHBytes[DTLS_HANDSHAKE_HEADER_SISE]; /* sizeof(SSLHandShakeHeader) */
    ubyte                           helloCookieLen;
    ubyte                           helloCookie[255];
    byteBoolean                     shouldChangeCipherSpec; /* true if epoch increases by 1 */
    ubyte2                          currentPeerEpoch;       /* the current peer epoch in effect */
    intBoolean                      receivedFinished;
#endif

    SizedBuffer                     buffers[10];
    sbyte4                          bufIndex;
    sbyte4                          numBuffers;

    SHA1_CTX*                       pShaCtx;
    MD5_CTX*                        pMd5Ctx;
    BulkCtx                         pHashCtx;
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    BulkCtx*                        pHashCtxList;
#endif

    intBoolean                      isDTLS; /* whether we are doing dtls */

    ubyte                           advertisedMinorVersion;  /* set by ioctl */
    ubyte                           clientHelloMinorVersion; /* sent/receive in client hello */
    ubyte                           minFallbackMinorVersion; /* set by ioctl */

    ubyte                           sslMinorVersion;         /* negotiated, current */


    /* count for active send/recv calls */
    byteBoolean                     blockConnection; /* syncronise between threads */
    sbyte4                          threadCount;

    poolHeaderDescr                 shaPool;
    poolHeaderDescr                 md5Pool;
    poolHeaderDescr                 hashPool; /* used for TLS1.2 and up */
    poolHeaderDescr                 smallPool;

#if (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__))
    ubyte4                          serverNameListLength;
    ubyte*                          serverNameList;

    /* certificate status request */
    /* Client: send certificate status request */
    /* Server: certificate status request received */
    intBoolean                      certStatusReqExt;
#ifdef __ENABLE_MOCANA_OCSP_CLIENT__
    /* Client: Input parameter to be used while create certificate status request */
    /* Server: Extensions to be attached while generating OCSP Request */
    extensions*                     pExts;
    ubyte4                          numOfExtension;
    void*                           pOcspContext; /* typecast of ocspContext */
#endif
//#ifndef __ENABLE_TLSEXT_RFC6066__
    ubyte4                          signatureAlgoListLength;
    ubyte*                          signatureAlgoList;
//#endif
#endif
    ubyte2                          signatureAlgo;

#ifdef __ENABLE_MOCANA_EAP_FAST__
    ubyte                           pacKey[PACKEY_SIZE];
#endif

#ifdef __ENABLE_MOCANA_INNER_APP__
    intBoolean                      receivedInnerApp;
    ubyte2                          receivedInnerAppValue;
    ubyte                           innerSecret[SSL_INNER_SECRET_SIZE];
#endif

    /* session resumption */
    E_SessionResumeType             sessionResume;

    /** encryption ***********/
    const struct CipherSuiteInfo*   pActiveOwnCipherSuite;  /* cipher suite used for encrypting data */
    const struct CipherSuiteInfo*   pActivePeerCipherSuite; /* cipher suite used for decrypting data */
    const struct CipherSuiteInfo*   pHandshakeCipherSuite;  /* cipher suite used connection handshake */

    /* bulk encryption contexts */
    BulkCtx                         clientBulkCtx;
    BulkCtx                         serverBulkCtx;

    /* MAC secrets -> points to materials below */
    ubyte*                          clientMACSecret;
    ubyte*                          serverMACSecret;

    /* IV for block encryption -> points to materials below */
    ubyte*                          clientIV;
    ubyte*                          serverIV;

#ifdef __ENABLE_MOCANA_EAP_FAST__
    /* session key seed -> points to materials below */
    ubyte*                          sessionKeySeed;
    ubyte*                          fastChapChallenge;
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    /* handshake message retransmission */
    struct retransCipherInfo {
        intBoolean                      deleteOldBulkCtx;    /* TRUE if we should call deleteCtxFunc */
        BulkCtx                         oldBulkCtx;
        ubyte                           oldMACSecret[SSL_MAXMACSECRETSIZE];
        const struct CipherSuiteInfo*   pOldCipherSuite;     /* cipher suite used for encrypting retransmitted data */
    } retransCipherInfo;
#endif

    /* sequence numbers */
    ubyte4                          ownSeqnum;
    ubyte4                          ownSeqnumHigh;  /* if DTLS, first two octets are epoch */
    ubyte4                          peerSeqnum;
    ubyte4                          peerSeqnumHigh;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    ubyte4                          oldSeqnum;      /* remember for retransmission */
    ubyte4                          oldSeqnumHigh;  /* remember for retransmission */

    /* anti-replay */
    ubyte                           replayWindow[DTLS_MAX_REPLAY_WINDOW_SIZE/8];
    ubyte4                          windowStartSeqnum;
    ubyte4                          windowStartSeqnumHigh;
#endif

    /* SSL 3.0 this will contain the master secret
     followed by the client random and server random
        TLS 1.0 this will contain "master secret" followed by the client
     random and the server random. "master secret" is at position
     SSL_RSAPRESECRETSIZE - TLS_MASTERSECRETLEN
    */
    ubyte*                          pSecretAndRand;
    ubyte*                          pClientRandHello;
    ubyte*                          pServerRandHello;
    /* contains the key materials */
    ubyte*                          pMaterials;
    ubyte*                          pActiveMaterials;   /* active materials is a clone of pMaterials --- allows for key reuse */

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
    /* ability to chose at run-time cipher suites to support */
    byteBoolean                     isCipherTableInit;
    byteBoolean                     isCipherEnabled[SSL_MAX_NUM_CIPHERS];
    ubyte4                          eccCurves;
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__)) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
    byteBoolean                     useSrtp; /* is use_srtp enabled? */
    ubyte*                          srtpMki; /* opaque srtp_mki<0..255> */
    const struct SrtpProfileInfo*   pHandshakeSrtpProfile;  /* protection profile used connection handshake */
    ubyte*                          pSrtpMaterials; /* contains the key materials */
#ifdef __ENABLE_MOCANA_SRTP_PROFILES_SELECT__
    /* ability to chose at run-time srtp protection profiles to support */
    byteBoolean                     isSrtpProfileTableInit;
    byteBoolean                     isSrtpProfileEnabled[SRTP_MAX_NUM_PROFILES]; /* TODO: should change to a smaller value */
#endif
#endif

    /* enough space to receive any SSL record */
    sbyte*                          pReceiveBuffer;
    sbyte*                          pReceiveBufferBase;
    sbyte4                          receiveBufferSize;
    sbyte4                          offset;                 /* points inside receive buffer ( used by SSL_Receive) */
    sbyte4                          recordSize;             /* size of record in receiveBuffer */
    ubyte4                          protocol;               /* current SSL message type (i.e. application, alert, inner, etc) */

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
    struct diffieHellmanContext*    pDHcontext;
#endif

#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined( __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__))
    AsymmetricKey                   ecdheKey;
#endif

    /* these two are used by both client and server */
    intBoolean                      isMutualAuthNegotiated;
    intBoolean                      generateEmptyCert;

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    AsymmetricKey                   mutualAuthKey;
#endif

    hwAccelDescr                    hwAccelCookie;          /* hardware accelerator cookie */

    ubyte4                          runtimeFlags;
    ubyte4                          internalFlags;

#if (defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__))
    intBoolean                      isRehandshakeExtPresent;
    intBoolean                      isRehandshakeAllowed;
    ubyte                           client_verify_data[SSL_VERIFY_DATA];
    ubyte                           server_verify_data[SSL_VERIFY_DATA];
#endif

    /* engineer defined cookie */
    sbyte8                          cookie;

    sbyte4                          server;                 /* are we a server or a client */
    union
    {
#ifdef __ENABLE_MOCANA_SSL_SERVER__
        struct ServerInfo
        {
            AsymmetricKey           privateKey;             /* private key: set by the server init routine */
            sbyte4                  numCertificates;
            SizedBuffer*            certificates;
            SESSIONID               sessionId;              /* our own session ID is 4 bytes long */
            intBoolean              isDynamicAlloc;         /* callback clone or static? */
            ubyte4                  certECCurves;           /* bit is set if curve is used in certchain */
            ubyte4                  clientECCurves;         /* bit is set if curve can be used by client */
#ifdef __ENABLE_RFC3546__
            byteBoolean             sendSessionTicket;
#endif
#if (defined(__ENABLE_RFC3546__) && defined(__ENABLE_MOCANA_INNER_APP__))
            intBoolean              innerApp;
            ubyte2                  innerAppValue;
#endif
#if (defined(__ENABLE_TLSEXT_RFC6066__) && defined(__ENABLE_MOCANA_OCSP_CLIENT__))
            ubyte*                  pResponderUrl;
#endif
        } server;
#endif

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
        struct ClientInfo
        {
            AsymmetricKey           publicKey;              /* public key: constructed from received Certificates */

            /* this part is set by the client specific extra initialization routine */
            ubyte                   sessionIdLen;
            ubyte                   sessionId[SSL_MAXSESSIONIDSIZE];
                                                            /* this contains a copy of the buffer passed
                                                               in the client specific extra initialization
                                                               routine originally and then the session id
                                                               sent by the server */
            ubyte*                  pMasterSecret;          /* points to some external buffer */
            const sbyte*            pDNSName;               /* points to some external buffer */
#if defined(__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__)
            const CNMatchInfo*      pCNMatchInfos;          /* points to some external buffer */
#endif

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
            ubyte2                  mutualAuthSignAlgo;     /* TLS 1.2 signature Algo */
#endif

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
            ubyte                   psk[SSL_PSK_MAX_LENGTH];
            ubyte4                  pskLength;
            ubyte                   pskIdentity[SSL_PSK_SERVER_IDENTITY_LENGTH];
            ubyte4                  pskIdentityLength;
#endif
            sbyte4                          numMutualAuthCert;
            SizedBuffer*                    sslMutualAuthCerts;
#ifdef __ENABLE_MOCANA_EAP_FAST__
            ubyte4                  ticketLength;
            ubyte*                  ticket;
#endif
#ifdef __ENABLE_MOCANA_INNER_APP__
            intBoolean              innerApp;
            ubyte2                  innerAppValue;
#endif
            ubyte*                  helloBuffer; /* for saving the client hello for hash calculation */
            ubyte4                  helloBufferLen; /* for saving the client hello for hash calculation */

#if defined(__ENABLE_TLSEXT_RFC6066__) && defined(__ENABLE_MOCANA_OCSP_CLIENT__)
            /* We will extract responder Ids from this; and use it for parsing
                          the received response */
            ubyte4                  trustedResponderCount;
            certDescriptor*         pOcspTrustedResponderCerts;

            /* certificate status request extension data */
            ubyte4                  certStatusReqExtLen;
            ubyte*                  certStatusReqExtData;
            intBoolean              didRecvCertStatusExtInServHello;
#endif
        } client;
#endif

    } roleSpecificInfo;

} SSLSocket;

/* Initialize SSL server engine */
MOC_EXTERN MSTATUS    SSL_SOCK_initServerEngine(RNGFun rngFun, void* rngFunArg);

/* Initialization /Deinitialization */
MOC_EXTERN MSTATUS    SSL_SOCK_init(SSLSocket* pSSLSock, intBoolean isDTLS, TCP_SOCKET tcpSock, peerDescr *pPeerDescr, RNGFun rngFun, void* rngFunArg);
MOC_EXTERN MSTATUS SSL_SOCK_initHashPool(SSLSocket *pSSLSock );

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
MOC_EXTERN MSTATUS    SSL_SOCK_initSocketExtraClient(SSLSocket* pSSLSock, ubyte sessionIdLen, ubyte* sessionId, ubyte* masterSecret, const sbyte* dnsName);
#endif

#if (defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
MOC_EXTERN MSTATUS    SSL_SOCK_setEAPFASTParams(SSLSocket* pSSLSock, ubyte* pPacOpaque, ubyte4 pacOpaqueLen, ubyte pacKey[PACKEY_SIZE]);
#endif

#ifdef __ENABLE_MOCANA_EAP_FAST__
MOC_EXTERN MSTATUS    SSL_SOCK_generateEAPFASTIntermediateCompoundKey(SSLSocket *pSSLSock, ubyte *s_imk, ubyte *msk, ubyte mskLen, ubyte *imk);
MOC_EXTERN MSTATUS    SSL_SOCK_generateEAPFASTSessionKeys(SSLSocket *pSSLSock, ubyte* S_IMCK, sbyte4 s_imckLen, ubyte* MSK, sbyte4 mskLen, ubyte* EMSK, sbyte4 emskLen/*64 Len */);
#endif

#if (defined(__ENABLE_MOCANA_EAP_PEER__) || defined(__ENABLE_MOCANA_EAP_AUTH__))
MOC_EXTERN MSTATUS    SSL_SOCK_generatePEAPIntermediateKeys(SSLSocket *pSSLSock, ubyte* IPMK, sbyte4 ipmkLen, ubyte* ISK, sbyte4 iskLen, ubyte* result, sbyte4 resultLen/*32 Len */);
MOC_EXTERN MSTATUS    SSL_SOCK_generatePEAPServerCompoundMacKeys(SSLSocket *pSSLSock, ubyte* IPMK , sbyte4 ipmkLen, ubyte* S_NONCE, sbyte4 s_nonceLen, ubyte* result, sbyte4 resultLen/*20 bytes*/);
MOC_EXTERN MSTATUS    SSL_SOCK_generatePEAPClientCompoundMacKeys(SSLSocket *pSSLSock, ubyte* IPMK , sbyte4 ipmkLen, ubyte* S_NONCE, sbyte4 s_nonceLen, ubyte* C_NONCE, sbyte4 c_nonceLen, ubyte* result, sbyte4 resultLen/*20 bytes*/);
MOC_EXTERN MSTATUS    SSL_SOCK_generatePEAPCompoundSessionKey(SSLSocket *pSSLSock, ubyte* IPMK , sbyte4 ipmkLen, ubyte* S_NONCE, sbyte4 s_nonceLen, ubyte* C_NONCE, sbyte4 c_nonceLen, ubyte* result, sbyte4 resultLen);
#endif

#ifdef __ENABLE_MOCANA_SSL_SERVER__
MOC_EXTERN MSTATUS    SSL_SOCK_initSocketExtraServer(SSLSocket* pSSLSock);
MOC_EXTERN MSTATUS    SSL_SOCK_setServerCert(SSLSocket* pSSLSock, sbyte4 numCertificates, SizedBuffer* certificates, AsymmetricKey* privateKey, ubyte4 certECCurves);
#endif

#if (defined( __ENABLE_RFC3546__) && defined(__ENABLE_MOCANA_SSL_SERVER__))
MOC_EXTERN MSTATUS    SSL_SOCK_setCipherById(SSLSocket* pSSLSock, ubyte2 cipherId);
#endif /* __ENABLE_RFC3546__ */

MOC_EXTERN void       SSL_SOCK_uninit(SSLSocket* pSSLSock);

/* Handshake */
#ifdef __ENABLE_MOCANA_SSL_SERVER__
MOC_EXTERN MSTATUS    SSL_SOCK_serverHandshake(SSLSocket* pSSLSock, intBoolean isWriter);
#endif

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
MOC_EXTERN MSTATUS    SSL_SOCK_clientHandshake(SSLSocket* pSSLSock, intBoolean isWriter);
#endif
#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
MOC_EXTERN MSTATUS    SSL_SOCK_validateCert(SSLSocket *pSSLSock, certDescriptor* pCertChain, sbyte4 numCertsInChain, certParamCheckConf *pCertParamCheckConf, certDescriptor *caCertificate, errorArray* pStatusArray);
#endif
/* Send/Receive */
MOC_EXTERN MSTATUS    SSL_SOCK_send(SSLSocket* pSSLSock, const sbyte* data, sbyte4 dataSize);
MOC_EXTERN MSTATUS    SSL_SOCK_sendPendingBytes(SSLSocket* pSSLSock);
MOC_EXTERN MSTATUS    SSL_SOCK_receive(SSLSocket* pSSLSock, sbyte* buffer, sbyte4 bufferSize, ubyte **ppPacketPayload, ubyte4 *pPacketLength, sbyte4 *pRetNumBytesReceived);
MOC_EXTERN MSTATUS    SSLSOCK_sendEncryptedHandshakeBuffer(SSLSocket* pSSLSock);

/* SSL Alerts */
MOC_EXTERN intBoolean SSLSOCK_lookupAlert(SSLSocket* pSSLSock, sbyte4 lookupError, sbyte4 *pRetAlertId, sbyte4 *pAlertClass);
MOC_EXTERN MSTATUS    SSLSOCK_sendAlert(SSLSocket* pSSLSock, intBoolean encryptBool, sbyte4 alertId, sbyte4 alertClass);
MOC_EXTERN MSTATUS    SSLSOCK_clearServerSessionCache(SSLSocket* pSSLSock);

/* cipher manipulation/information */
#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
MOC_EXTERN sbyte4     SSL_SOCK_numCiphersAvailable(void);
MOC_EXTERN ubyte4     SSL_SOCK_getECCCurves(void);
#endif

#if defined ( __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__) || defined( __ENABLE_RFC3546__)
MOC_EXTERN sbyte4     SSL_SOCK_getCipherTableIndex(SSLSocket* pSSLSock, ubyte2 cipherId);
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#if (defined(__ENABLE_MOCANA_DTLS_SRTP__) && defined(__ENABLE_MOCANA_SRTP_PROFILES_SELECT__))
MOC_EXTERN sbyte4     SSL_SOCK_numSrtpProfilesAvailable(void);
MOC_EXTERN sbyte4     SSL_SOCK_getSrtpProfileIndex(SSLSocket* pSSLSock, ubyte2 profileId);
#endif
#endif

#if defined(__ENABLE_MOCANA_INNER_APP__)
MOC_EXTERN MSTATUS    SSLSOCK_sendInnerApp(SSLSocket* pSSLSock, InnerAppType innerApp, ubyte* pMsg, ubyte4 msgLen, ubyte4 *retMsgLen, sbyte4 isClient);
MOC_EXTERN MSTATUS    SSLSOCK_updateInnerAppSecret(SSLSocket* pSSLSock, ubyte* session_key, ubyte4 sessionKeyLen);
MOC_EXTERN MSTATUS    SSLSOCK_verifyInnerAppVerifyData(SSLSocket *pSSLSock, ubyte *data, InnerAppType innerAppType, sbyte4 isClient);
#endif

/* EAP-TTLS */
#ifdef __ENABLE_MOCANA_SSL_KEY_EXPANSION__
MOC_EXTERN MSTATUS    SSL_SOCK_generateKeyExpansionMaterial(SSLSocket *pSSLSock,ubyte *pKey, ubyte2 keySize, ubyte *keyPhrase, ubyte2 keyPhraseLen);
MOC_EXTERN MSTATUS    SSL_SOCK_generateTLSKeyExpansionMaterial(SSLSocket *pSSLSock,ubyte *pKey, ubyte2 keySize, ubyte *keyPhrase, ubyte2 keyPhraseLen);
#endif

MOC_EXTERN MSTATUS    SSL_SOCK_getCipherId(SSLSocket* pSSLSock, ubyte2* pCipherId);
MOC_EXTERN MSTATUS    SSL_SOCK_sendServerHelloRequest(SSLSocket* pSSLSock);

#ifdef __cplusplus
}
#endif


#endif
