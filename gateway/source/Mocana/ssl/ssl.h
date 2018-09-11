/* Version: mss_v6_3 */
/*
 * ssl.h
 *
 * SSL Developer API
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file ssl.h SSL and DTLS developer API header.
This header file contains definitions, enumerations, and function declarations
used by NanoSSL and NanoDTLS servers and clients.

\since 1.41
\version 4.2 and later

! Flags
Whether the following flags are defined determines which function declarations
and callbacks are enabled:
- $__ENABLE_MOCANA_EAP_FAST__$
- $__ENABLE_MOCANA_EXTRACT_CERT_BLOB__$
- $__ENABLE_MOCANA_INNER_APP__$
- $__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__$
- $__ENABLE_MOCANA_SSL_ALERTS__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_DUAL_MODE_API__$
- $__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__$
- $__ENABLE_MOCANA_SSL_ECDH_SUPPORT__$
- $__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__$
- $__ENABLE_MOCANA_SSL_INTERNAL_STRUCT_ACCESS__$
- $__ENABLE_MOCANA_SSL_KEY_EXPANSION__$
- $__ENABLE_MOCANA_SSL_MUTUAL_AUTH_CERT_SINGLE_ONLY__$
- $__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__$
- $__ENABLE_MOCANA_SSL_NEW_HANDSHAKE__$
- $__ENABLE_MOCANA_SSL_PSK_SUPPORT__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_RFC3546__$
- $__ENABLE_MOCANA_SSL_CUSTOM_RNG__$
- $__ENABLE_TLSEXT_RFC6066__$
*/


/*------------------------------------------------------------------*/

#ifndef __SSL_HEADER__
#define __SSL_HEADER__

#include "../crypto/hw_accel.h"
#include "../common/moc_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: copyed over from ike_utils.h */
#ifndef __ENABLE_MOCANA_IPV6__

#define ZERO_MOC_IPADDR(a)      a = 0
#define ISZERO_MOC_IPADDR(a)    (0 == a)
#define SAME_MOC_IPADDR(a, b)   (a == b)
#define COPY_MOC_IPADDR(d, s)   d = s
#define REF_MOC_IPADDR(a)       a
#define GET_MOC_IPADDR4(a)      a
#define SET_MOC_IPADDR4(a, v)   a = v;
#define LT_MOC_IPADDR4(a, b)    (a < b)
#define LT_MOC_IPADDR           LT_MOC_IPADDR4
#define TEST_MOC_IPADDR6(a, _c)

#else

#ifndef AF_INET
#define AF_INET     2   /* Internet IP Protocol */
#endif

#ifndef AF_INET6        /* IP version 6 */
#if defined(__LINUX_RTOS__)
#define AF_INET6    10
#elif defined (__WIN32_RTOS__)
#define AF_INET6    23
#else
#error Must define AF_INET6
#endif
#endif

#define ZERO_MOC_IPADDR(s)      (s).family = 0;\
                                (s).uin.addr6[0] = (s).uin.addr6[1] =\
                                (s).uin.addr6[2] = (s).uin.addr6[3] = 0
#define ISZERO_MOC_IPADDR(s)    (0 == (s).family)
#define SAME_MOC_IPADDR(a, s)   ((a) && ((a)->family == (s).family) &&\
                                 (((AF_INET == (a)->family) &&\
                                   ((a)->uin.addr == (s).uin.addr))\
                                  ||\
                                  ((AF_INET6 == (a)->family) &&\
                                   ((a)->uin.addr6[0] == (s).uin.addr6[0]) &&\
                                   ((a)->uin.addr6[1] == (s).uin.addr6[1]) &&\
                                   ((a)->uin.addr6[2] == (s).uin.addr6[2]) &&\
                                   ((a)->uin.addr6[3] == (s).uin.addr6[3]))\
                                  ))
#define COPY_MOC_IPADDR(s, a)   s = *(a)
#define REF_MOC_IPADDR(s)       &(s)
#define GET_MOC_IPADDR4(a)      (a)->uin.addr
#define SET_MOC_IPADDR4(s, v)   (s).family = AF_INET; (s).uin.addr = v
#define LT_MOC_IPADDR4(x, y)    ((x).uin.addr < (y).uin.addr)

#define TEST_MOC_IPADDR6(a, _c) if (AF_INET6 == (a)->family) _c else

#define GET_MOC_IPADDR6(a)      (ubyte *) (a)->uin.addr6
#define SET_MOC_IPADDR6(s, v)   (s).family = AF_INET6;\
                                MOC_MEMCPY((ubyte *) (s).uin.addr6, (ubyte *)(v), 16)
#define LT_MOC_IPADDR6(x, y)    ((GET_NTOHL((x).uin.addr6[0]) < GET_NTOHL((y).uin.addr6[0])) ||\
                                 (((x).uin.addr6[0] == (y).uin.addr6[0]) &&\
                                  ((GET_NTOHL((x).uin.addr6[1]) < GET_NTOHL((y).uin.addr6[1])) ||\
                                   (((x).uin.addr6[1] == (y).uin.addr6[1]) &&\
                                    ((GET_NTOHL((x).uin.addr6[2]) < GET_NTOHL((y).uin.addr6[2])) ||\
                                     (((x).uin.addr6[2] == (y).uin.addr6[2]) &&\
                                      (GET_NTOHL((x).uin.addr6[3]) < GET_NTOHL((y).uin.addr6[3]))))))))
#define LT_MOC_IPADDR(p, q)     (((p).family != (q).family) ||\
                                 ((AF_INET == (p).family) ? LT_MOC_IPADDR4(p, q) : LT_MOC_IPADDR6(p, q)))

#endif /* __ENABLE_MOCANA_IPV6__ */

#if !defined( __ENABLE_MOCANA_SSL_CLIENT__ ) && defined( __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__ )
#define __ENABLE_MOCANA_SSL_CLIENT__
#endif

#if !defined( __ENABLE_MOCANA_SSL_SERVER__ ) && defined( __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__ )
#define __ENABLE_MOCANA_SSL_SERVER__
#endif

/* check for possible build configuration errors */
#ifndef __ENABLE_MOCANA_SSL_DUAL_MODE_API__
#if defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__) && defined(__ENABLE_MOCANA_SSL_SERVER__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
#error SSL build configuration error.  Mixing async client w/ sync server prohibited.
#endif

#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && defined(__ENABLE_MOCANA_SSL_CLIENT__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
#error SSL build configuration error.  Mixing async server w/ sync client prohibited.
#endif
#endif /* __ENABLE_MOCANA_SSL_DUAL_MODE_API__ */

#if defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_CLIENT__)

#ifdef _DEBUG
#define TIMEOUT_SSL_RECV                    (0)
#define TIMEOUT_SSL_HELLO                   (0)
#else
/* timeouts in milliseconds  0 means for ever */
#define TIMEOUT_SSL_RECV                    (15000)
#define TIMEOUT_SSL_HELLO                   (15000)
#endif

#define TIMEOUT_DTLS_CONNECT_TIMED_WAIT     (2*60*1000)

#ifndef SSL_WRITE_FAIL_RETRY_TIME
#define SSL_WRITE_FAIL_RETRY_TIME           (5)
#endif

/* for reference */
#define SSL_DEFAULT_TCPIP_PORT              (443)

/* sizes */
#define SSL_SHA_FINGER_PRINT_SIZE           (20)
#define SSL_MD5_FINGER_PRINT_SIZE           (16)
#ifdef __UCOS_RTOS__
#define SSL_SYNC_BUFFER_SIZE                (512)
#else
#define SSL_SYNC_BUFFER_SIZE                (2048)
#endif /* __UCOS_RTOS__ */
#define SSL_MAXSESSIONIDSIZE                (32)
#define SSL_MASTERSECRETSIZE                (48)

#define SSL_PSK_SERVER_IDENTITY_LENGTH      (128)
#define SSL_PSK_MAX_LENGTH                  (64)

#define SSL_MAX_NUM_CIPHERS                 (140)

/* SSL runtime flags */
#define SSL_FLAG_REQUIRE_MUTUAL_AUTH         (0x00000001L)
#define SSL_FLAG_NO_MUTUAL_AUTH_REQUEST      (0x00000002L)       /* for server */
#define SSL_FLAG_NO_MUTUAL_AUTH_REPLY        (0x00000002L)       /* for client */
#define SSL_FLAG_ENABLE_SEND_EMPTY_FRAME     (0x00000004L)
#define SSL_FLAG_ENABLE_SEND_BUFFER          (0x00000008L)
#define SSL_FLAG_ENABLE_RECV_BUFFER          (0x00000010L)
#define SSL_FLAG_ACCEPT_SERVER_NAME_LIST     (0x00000020L)
#define SSL_FLAG_DISABLE_CLIENT_CHAIN_CHK    (0x00000040L)
#define SSL_FLAG_ALLOW_INSECURE_REHANDSHAKE  (0x00000080L)       /* permit legacy renegotiation */

/* DTLS runtime flags */
#define DTLS_FLAG_ENABLE_SRTP_DATA_SEND     (0x00001000L)

/* SSL runtime flags: upper 2 octect for internal use only */
#define SSL_FLAG_INTERNAL_USE                   (0xFF000000L)
#define SSL_FLAG_VERSION_SET                    (0x80000000L)
#define SSL_FLAG_MINIMUM_FALLBACK_VERSION_SET   (0x40000000L)
#define SSL_FLAG_SCSV_FALLBACK_VERSION_SET      (0x20000000L)

/*TLS 1.2 SUITE B minimum level of security */
#define SSL_TLS12_MINLOS_128                 (1)
#define SSL_TLS12_MINLOS_192                 (2)

/* SSL ioctl settings */
#define SSL_SET_VERSION                         (1)
#define SSL_SET_MINIMUM_VERSION                 (2)
#define SSL_SET_SCSV_VERSION                    (3)

/* DTLS ioctl settings */
#define DTLS_SET_HANDSHAKE_RETRANSMISSION_TIMER (10)
#define DTLS_SET_PMTU                           (11)
#define DTLS_USE_SRTP                           (12)
#define DTLS_SET_HELLO_VERIFIED                 (13)

/* SSL Alert level */
#define SSLALERTLEVEL_WARNING               (1)
#define SSLALERTLEVEL_FATAL                 (2)

/* SSL Alert description */
#define SSL_ALERT_CLOSE_NOTIFY                      (0)
#define SSL_ALERT_UNEXPECTED_MESSAGE                (10)
#define SSL_ALERT_BAD_RECORD_MAC                    (20)
#define SSL_ALERT_DECRYPTION_FAILED                 (21)
#define SSL_ALERT_RECORD_OVERFLOW                   (22)
#define SSL_ALERT_DECOMPRESSION_FAILURE             (30)
#define SSL_ALERT_HANDSHAKE_FAILURE                 (40)
#define SSL_ALERT_NO_CERTIFICATE                    (41)
#define SSL_ALERT_BAD_CERTIFICATE                   (42)
#define SSL_ALERT_UNSUPPORTED_CERTIFICATE           (43)
#define SSL_ALERT_CERTIFICATE_REVOKED               (44)
#define SSL_ALERT_CERTIFICATE_EXPIRED               (45)
#define SSL_ALERT_CERTIFICATE_UNKNOWN               (46)
#define SSL_ALERT_ILLEGAL_PARAMETER                 (47)
#define SSL_ALERT_UNKNOWN_CA                        (48)
#define SSL_ALERT_ACCESS_DENIED                     (49)
#define SSL_ALERT_DECODE_ERROR                      (50)
#define SSL_ALERT_DECRYPT_ERROR                     (51)
#define SSL_ALERT_EXPORT_RESTRICTION                (60)
#define SSL_ALERT_PROTOCOL_VERSION                  (70)
#define SSL_ALERT_INSUFFICIENT_SECURITY             (71)
#define SSL_ALERT_INTERNAL_ERROR                    (80)
#define SSL_ALERT_INAPPROPRIATE_FALLBACK            (86)
#define SSL_ALERT_USER_CANCELED                     (90)
#define SSL_ALERT_NO_RENEGOTIATION                  (100)
#define SSL_ALERT_UNSUPPORTED_EXTENSION             (110)
#define SSL_ALERT_CERTIFICATE_UNOBTAINABLE          (111)
#define SSL_ALERT_UNRECOGNIZED_NAME                 (112)
#define SSL_ALERT_BAD_CERTIFICATE_STATUS_RESPONSE   (113)
#define SSL_ALERT_BAD_CERTIFICATE_HASH_VALUE        (114)
#define SSL_ALERT_INNER_APPLICATION_FAILURE         (208)
#define SSL_ALERT_INNER_APPLICATION_VERIFICATION    (209)

#define SSL_CONNECTION_OPEN                 (3)
#define SSL_CONNECTION_NEGOTIATE            (2)

#ifndef MIN_SSL_RSA_SIZE
#define MIN_SSL_RSA_SIZE                (2048)
#endif

#ifndef MIN_SSL_DH_SIZE
#define MIN_SSL_DH_SIZE                 (1024)
#endif

    /* default DH group size is 2048: the values allowed are defined in
     crypto/dh.h */
#ifndef SSL_DEFAULT_DH_GROUP
#define SSL_DEFAULT_DH_GROUP            DH_GROUP_14
#endif

#define SSL3_MAJORVERSION               (3)
#define SSL3_MINORVERSION               (0)
#define TLS10_MINORVERSION              (1)
#define TLS11_MINORVERSION              (2)
#define TLS12_MINORVERSION              (3)
    /* define max and min version if not specified
     disable SSLv3 by default */
#ifndef MIN_SSL_MINORVERSION
#define MIN_SSL_MINORVERSION  (TLS10_MINORVERSION)
#endif

#ifndef MAX_SSL_MINORVERSION
#define MAX_SSL_MINORVERSION (TLS12_MINORVERSION)
#endif

#define VALID_SSL_VERSION( major, minor) (( SSL3_MAJORVERSION == major) && (MIN_SSL_MINORVERSION <= minor) && (minor <= MAX_SSL_MINORVERSION))

    /* DTLS: we should use a signed quantity for minor version and use negative numbers
     that would prevent minimum being bigger than maximum ! */
#define DTLS1_MAJORVERSION              (254)
#define DTLS10_MINORVERSION             (255)
#define DTLS12_MINORVERSION             (253)

#ifndef MIN_DTLS_MINORVERSION
#define MIN_DTLS_MINORVERSION  (DTLS10_MINORVERSION)
#endif

#ifndef MAX_DTLS_MINORVERSION
#define MAX_DTLS_MINORVERSION (DTLS12_MINORVERSION)
#endif

    /* careful here since MAX_DTLS_MINORVERSION <= MIN_DTLS_MINORVERSION */
#define VALID_DTLS_VERSION( major, minor) (( DTLS1_MAJORVERSION == major) && (MIN_DTLS_MINORVERSION >= minor) && (minor >= MAX_DTLS_MINORVERSION))

struct AsymmetricKey;

enum tlsExtensionTypes
{
    tlsExt_server_name = 0,
    tlsExt_max_fragment_length = 1,
    tlsExt_client_certificate_url = 2,
    tlsExt_trusted_ca_keys = 3,
    tlsExt_truncated_hmac = 4,
    tlsExt_status_request = 5,
    tlsExt_supportedEllipticCurves = 10,
    tlsExt_ECPointFormat = 11,
    tlsExt_supportedSignatureAlgorithms = 13,
    dtlsExt_use_srtp = 14, /* RFC 5764 */
    tlsExt_ticket = 35,
    tlsExt_innerApplication = 37703,
    tlsExt_renegotiated_connection = 0xff01
};

enum tlsExtNamedCurves
{
    /* only the ones we support */
    tlsExtNamedCurves_secp192r1 = 19,
    tlsExtNamedCurves_secp224r1 = 21,
    tlsExtNamedCurves_secp256r1 = 23,
    tlsExtNamedCurves_secp384r1 = 24,
    tlsExtNamedCurves_secp521r1 = 25
};

#ifdef __ENABLE_MOCANA_INNER_APP__
typedef enum innerAppType
{
    SSL_INNER_APPLICATION_DATA =0,
    SSL_INNER_INTER_FINISHED   =1,
    SSL_INNER_FINAL_FINISHED   =2,
} InnerAppType;
#endif

#ifdef __ENABLE_TLSEXT_RFC6066__

typedef enum nameTypeSNI
{
    nameTypeHostName = 0
    /* currently only one is supported */

} NameTypeSNI;

typedef enum certificateStatusType
{
    certStatusType_ocsp = 1
    /* currently only one is supported */
}CertificateStatusType;
#endif

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__)   || \
        defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)|| \
        defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
ubyte2 SSL_getNamedCurveOfCurveId( ubyte4 curveId);
ubyte4 SSL_getCurveIdOfNamedCurve( ubyte2 namedCurve);
#endif

/* definition needed by DTLS. however the common interfaces need this definition */
typedef struct peerDescr
{
    void *pUdpDescr;
    ubyte2 srcPort;
    MOC_IP_ADDRESS_S srcAddr;
    ubyte2 peerPort;
    MOC_IP_ADDRESS_S peerAddr;
} peerDescr;

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__)) && defined(__ENABLE_MOCANA_DTLS_SRTP__)

typedef struct SrtpProfileInfo
{
    ubyte2                  profileId;                  /* profile identification */
    ubyte                   supported;                  /* support by this implementation */
    sbyte                   keySize;                    /* size of key */
    sbyte                   saltSize;                   /* size of salt */
} SrtpProfileInfo;

#endif

#ifdef __ENABLE_TLSEXT_RFC6066__
/* structured needs for extensions defined in RFC 6066 */

typedef struct ServerName
{
    NameTypeSNI name_type;
    void*       pName;
    ubyte       nameLen;
} ServerName;

#ifdef __ENABLE_MOCANA_OCSP_CLIENT__
typedef struct responderID
{
    ubyte responderIDlen;
    void* pResponderID;
} ResponderID;

typedef struct ocspStatusRequest
{
    void*  pResponderIdList;
    ubyte2 responderIdListLen;

    /* Need to put extensions here */
    void*  pExtensionsList;
    ubyte2 extensionListLen;

} OCSPStatusRequest;

/*
struct {
          CertificateStatusType status_type;
          select (status_type) {
              case ocsp: OCSPStatusRequest;
          } request;
      } CertificateStatusRequest;
*/

typedef struct certificateStatusRequest
{
    CertificateStatusType status_type;
    OCSPStatusRequest     ocspReq;
} CertificateStatusRequest;
#endif /* __ENABLE_MOCANA_OCSP_CLIENT__ */
#endif /* __ENABLE_TLSEXT_RFC6066__ */

/*------------------------------------------------------------------*/

#define MAX_CERT_ERRORS                             11

typedef struct errorArray_t
{
    sbyte4 errors[MAX_CERT_ERRORS];
    ubyte4 numErrors;
} errorArray;

typedef struct certParamCheckConf
{
    intBoolean chkCommonName;           /* Common name Check */
    intBoolean chkValidityBefore;       /* Validity Check of stop time */
    intBoolean chkValidityAfter;        /* Validity Check of start time */
    intBoolean chkKnownCA;              /* Known CA Check */
} certParamCheckConf;

struct certDistinguishedName;
struct certDescriptor;

/* Configuration settings and callback function pointers for SSL/TLS and DTLS clients.
This structure is used for SSL/TLS and DTLS Client configuration. Which products and
features you've included (by defining the appropriate flags in moptions.h)
determine which data fields and callback functions are present in this structure.
Each included callback function should be customized for your application and
then registered by assigning it to the appropriate structure function pointer(s).

\since 1.41
\version 3.2 and later

! Flags
To use this structure, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

*/
typedef struct sslSettings
{
#ifdef __ENABLE_MOCANA_SSL_SERVER__
    /* Port number for the connection context.
    Port number for the connection context.
    ! Flags
    This field is defined only if the $__ENABLE_MOCANA_SSL_SERVER__$ flag is defined in moptions.h.
    */
    ubyte4    sslListenPort;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    /* Internal use only.
    Internal use only.
    */
    ubyte                   helloCookieSecret[2][SSL_SHA_FINGER_PRINT_SIZE];
    /* Internal use only.
    Internal use only.
    */
    ubyte4                  helloCookieSecretLen[2];
    /* Internal use only.
    Internal use only.
    */
    ubyte4                  helloCookieSecretLastGenTime;
    /* Internal use only.
    Internal use only.
    */
    /* indicate the helloCookieSecret version currently in use.
     * it alternates between 0 and 1 */
    ubyte                   helloCookieVersion;
    /* Internal use only.
    Internal use only.
    */
    hwAccelDescr            hwAccelCookie;          /* hardware accelerator cookie */
    /* Number of seconds to wait for connection timeout.
    Number of seconds to wait for connection timeout.
    ! Flags
    This field is defined only if the $__ENABLE_MOCANA_SSL_SERVER__$ and
    $__ENABLE_MOCANA_DTLS_SERVER__$ flags are defined in moptions.h.
    */
    ubyte4                  sslTimeOutConnectTimedWait;
#endif

#ifdef __ENABLE_TLSEXT_RFC6066__
    ubyte*                  pCachedOcspResponse;
    ubyte4                  cachedOcspResponseLen;
    TimeDate                nextUpdate;
#endif
#endif

/* Number of seconds to wait for a $Receive$ message.
Number of seconds to wait for a $Receive$ message.
*/
    ubyte4    sslTimeOutReceive;
/* Number of seconds to wait for a $Hello$ message.
Number of seconds to wait for a $Hello$ message.
*/
    ubyte4    sslTimeOutHello;
#ifdef __ENABLE_MOCANA_SSL_REHANDSHAKE__
/* Max timer count for rehandshake */
    sbyte4    maxTimerCountForRehandShake;

/* Max number of bytes send count for rehandshake */
    sbyte4    maxByteCountForRehandShake;
/* callback fuction for rehandshake
This callback indicates that timer or bytes sent count has rollover the max
set count and it is a indication for SSL-rehandshake

\since 5.8
\version 5.8 and later

\param connectionInstance   Connection instance returned from SSL_ASYNC_acceptConnection.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.
*/
    sbyte4(*funcPtrClientRehandshakeRequest)(sbyte4 connectionInstance);
#endif

#ifdef __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__
/* Indicate successful asynchronous session establishment.
This callback indicates that a secure asynchronous session has been
(re)established between peers (%client and %server). This function is called
when an SSL-rehandshake has completed.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Connection instance returned from SSL_ASYNC_acceptConnection.
\param isRehandshake        True (1) indicates a rehandshake notice.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

*/
    sbyte4(*funcPtrOpenStateUpcall)(sbyte4 connectionInstance, sbyte4 isRehandshake);

/* Decrypt and return data received through a connection context.
This callback decrypts and returns data received through a connection context.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Connection instance returned from SSL_ASYNC_acceptConnection.
\param pMesg                Pointer to decrypted message.
\param mesgLen              Number of bytes in received message ($pMesg$).

\return None.

*/
    void(*funcPtrReceiveUpcall)  (sbyte4 connectionInstance,
                                  ubyte *pMesg,
                                  ubyte4 mesgLen);

/* Start a timer to use for timeout notifications.
This callback starts a timer to use for timeout notifications.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Connection instance returned from SSL_ASYNC_acceptConnection.
\param msTimerExpire        Number of milliseconds until timer expires.
\param future               (Reserved for future use.)

\return None.

*/
    void(*funcPtrStartTimer)     (sbyte4 connectionInstance,
                                  ubyte4 msTimerExpire,
                                  sbyte4 future);
#endif /* __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__ */

#ifdef __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__
/* Indicate that a secure asynchronous session has been established between peers.
This callback indicates that a secure asynchronous session has been (re)established
between peers (%client and %server). This function is called when an SSL-rehandshake has
completed.

\note Your application should not attempt to send data through the given
connection context until the successful session establishment indicated by this
upcall is achieved.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.
\param isRehandshake        True(One) indicates a rehandshake notice.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

*/
    sbyte4(*funcPtrClientOpenStateUpcall)(sbyte4 connectionInstance, sbyte4 isRehandshake);

/* Retrieve data received from a server.
This callback retrieves data received from a server through the given connection
context. The data is returned through the $pMesg$ parameter as decrypted text.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.
\param pMesg                Pointer to the received decrypted message.
\param mesgLen              Number of bytes in the received message ($pMesg$).

\return None.

*/
    void(*funcPtrClientReceiveUpcall)  (sbyte4 connectionInstance,
                                        ubyte *pMesg,
                                        ubyte4 mesgLen);

/* Start a timer for timeout notifications.
This callback starts a timer for timeout notifications.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.
\param msTimerExpire        Number of milliseconds until timer expires.
\param future               (Reserved for future use.)

\return None.

*/
    void(*funcPtrClientStartTimer)     (sbyte4 connectionInstance,
                                        ubyte4 msTimerExpire,
                                        sbyte4 future);
#endif /* __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__ */

/* Verify that the leaf (first certificate in a chain) is an acceptable, known certificate.
This callback function is used by NanoSSL %client to verify that the
leaf (first certificate in a chain) is an acceptable, known certificate.

\since 1.41
\version 1.41 and later

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\note If you accept certificates from only a single trusted source, just return
that source. Mocana Security Suite will verify the returned certificate.
\note To avoid memory leaks, be sure to call Mocana free certificate functions
where appropriate.

! Flags
To enable this callback, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pCertificate         Pointer to the first X.509 certificate in the certificate chain of interest.
\param cetificateLen        Number of bytes in X.509 certificate ($pCertificate$).

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured to use digital certificates for authentication.

*/
    sbyte4 (*funcPtrCertificateLeafTest)    (sbyte4 connectionInstance, ubyte *pCertificate, ubyte4 certificateLen);

/* Verify every certificate in a chain.
This callback function is used by NanoSSL %client to verify every
certificate in a chain (instead of just the chain's root (last) certificate, as
is done by sslSettings::funcPtrCertificateStoreVerify).

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\note If you accept certificates from only a single trusted source, just return
that source. Mocana Security Suite will verify the returned certificate.
\note To avoid memory leaks, be sure to call Mocana free certificate functions
where appropriate.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pCertificate         Pointer to the X.509 certificate in the certificate chain of interest.
\param certificateLen       Number of bytes in X.509 certificate ($pCertificate$).

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured to use digital certificates for authentication.

*/
    sbyte4 (*funcPtrCertificateChainTest)   (sbyte4 connectionInstance, ubyte *pCertificate, ubyte4 certificateLen);

#if (!defined(__ENABLE_MOCANA_EXTRACT_CERT_BLOB__))
/* Get a certificate from the trusted certificate store based on a dintinguished name.
This callback function retrieves a certificate from the trusted certificate store
based on the provided distinguished name. It is also used by NanoSSL
%client when a certificate chain is incomplete.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\note If you accept certificates from only a single trusted source, just return
that source. Mocana Security Suite will verify the returned certificate.
\note To avoid memory leaks, be sure to call Mocana free certificate functions
where appropriate.

\since 1.41
\version 2.02 and later

! Flags
To enable this callback, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

The method signature differs based on whether the following flag is defined:
- $__ENABLE_MOCANA_EXTRACT_CERT_BLOB__$

\param connectionInstance       Pointer to the SSL/TLS Client instance.
\param pLookupCertDN            (This argument is present
only if the $__ENABLE_MOCANA_EXTRACT_CERT_BLOB__$ flag is #not# defined.)
Pointer to a distinguished name structure to be used to lookup a certificate in
the trusted certificate store.
\param pDistinguishedName       (This argument is present
only if the $__ENABLE_MOCANA_EXTRACT_CERT_BLOB__$ flag }is} defined.) Pointer
to distinguished name string.
\param distinguishedNameLen     (This argument is present
only if the $__ENABLE_MOCANA_EXTRACT_CERT_BLOB__$ flag }is} defined.) Number of
bytes in distinguished name ($pDistinguishedName$).
\param pReturnCert              Pointer to a structure in which to
store the resulting certificate information. Only the certificate and
certificate length fields are required; the public and private key fields are
not relevant.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured to use digital certificates for authentication.

\todo Documentation for variants.
*/
    sbyte4 (*funcPtrCertificateStoreLookup) (sbyte4 connectionInstance, struct certDistinguishedName *pLookupCertDN, struct certDescriptor* pReturnCert);
#else
    sbyte4 (*funcPtrCertificateStoreLookup) (sbyte4 connectionInstance, ubyte* pDistinguishedName, ubyte4 distinguishedNameLen, struct certDescriptor* pReturnCert);
#endif

/* Release memory associated with a previous call to sslSettings::funcPtrCertificateStoreLookup.
This callback function releases memory associated with a previous call to
sslSettings::funcPtrCertificateStoreLookup.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pFreeCert            Pointer to the certificate to free.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured to use digital certificates for authentication.

*/
    sbyte4 (*funcPtrCertificateStoreRelease)(sbyte4 connectionInstance, struct certDescriptor* pFreeCert);

/* Verify that the last certificate or self-signed certificate is trusted.
This callback function verifies that the last certificate or self-signed
certificate is trusted. Your code may simply $memcmp$ certificates you trust with
the provided certificate, or implement a more efficient and rigorous algorithm,
such as including a call to the Mocana certificate management function,
CA_MGMT_extractCertDistinguishedName.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pCertificate         Pointer to the X.509 certificate of interest.
\param certificateLength    Number of bytes in X.509 certificate of interest ($pCertificate$).
\param isSelfSigned         TRUE (1) to specify that the certificate does not have a certificate chain; or FALSE (0) to specify that the certificate needs to be verified as a trusted certificate.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured to use digital certificates for authentication.

*/
    sbyte4 (*funcPtrCertificateStoreVerify) (sbyte4 connectionInstance, ubyte *pCertificate, ubyte4 certificateLength, sbyte4 isSelfSigned);

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
#ifdef __ENABLE_MOCANA_SSL_CLIENT__
/* Compute the signature for a certificate verify message sent for %client authentication.
This callback function is used by an %ssl %client when it needs to compute the
content of a certificate verify message for mutual authentication.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 3.2
\version 3.2 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

\param connectionInstance   Pointer to the SSL/TLS %client instance.
\param hash                 Pointer to hash byte string.
\param hashLen              Number of bytes in the hash byte string ($hash$).
\param result               Pointer to the signature.
\param resultLength         Number of bytes in the signature buffer ($result$).

\return 0 or a positive number if successful; for ECDSA signatures, the return value
is the size of the signature ( a DER encoded SEQUENCE);
for RSA signatures, the return value has no additional significance.
Otherwise a negative number error code definition from merrors.h.
To retrieve a string containing an English text error identifier corresponding to
the function's returned error status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured to use mutual authentication and the private
key used for mutual authentication is not accessible (that is, it's provided by
external hardware such as a Smart Card). Your implementation of this function
must place the signature of the hash (of length $hashLength$) into this result
buffer.

*/
    sbyte4 (*funcPtrMutualAuthCertificateVerify) (sbyte4 connectionInstance, const ubyte* hash,
                                    ubyte4 hashLen, ubyte* result, ubyte4 resultLength);

/* Gets a certificate (assign a certStore) to enable a %client to authenticate itself.
This callback function gets a certificate (that is, assign a certStore using
SSL_assignCertificateStore) to enable a %client to authenticate itself. After the
calling routine has the certificate, it can send it to the %server.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 5.3.1
\version 5.3.1 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pDistNames           Pointer to array of distribution names
(domain names that hint at the expected certificates).
\param distNameLen          Number of bytes in distribution names array ($pDistNames$).
\param pSigAlgos            Pointer to array of signature and hash algorithms.
\param sigAlgosLen          Number of bytes in signature and hash algorithms
array ($pDistNames$).

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if mutual authentication needs to be configured upon server's request.

*/
    sbyte4 (*funcPtrMutualAuthGetCertificate) (sbyte4 connectionInstance, ubyte *pDistNames, ubyte4 distNameLen, ubyte *pSigAlgos, ubyte4 sigAlgosLen);
#endif
#endif

#if (defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
/* Retrieve a server's preferred PSK.
This callback function returns a hint through the $hintPSK$ parameter indicating
the server's preferred PSK. To abort the session, the function should return an
error code (a negative value) instead of $OK$ ($0$).

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\note If this function isn't defined, no hint can be returned to the %client.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_PSK_SUPPORT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param hintPSK              On return, the server's preferred PSK.
\param pRetHintLength       On return, pointer to number of bytes (excluding any terminating $NULL$) in $hintPSK$.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured for PSK support.

*/
    sbyte4 (*funcPtrGetHintPSK)(sbyte4 connectionInstance, ubyte hintPSK[SSL_PSK_SERVER_IDENTITY_LENGTH], ubyte4 *pRetHintLength);
/* Retrieve a (based on the provided PSK's name/identity) the preferred PSK.
This callback function looks up the specified identity (the PSK's name) and
returns its preferred PSK&mdash;the secret used to encrypt data&mdash; through
the $retPSK$ parameter. To abort the session, the function should return an
error code (a negative value) instead of $OK$ (0).

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_PSK_SUPPORT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pIdentityPSK         Pointer to buffer containing the PSK identity to look up.
\param identityLengthPSK    Number of bytes in PSK identity ($pIdentityPSK$).
\param retPSK               On return, buffer containing the identity's PSK.
\param pRetLengthPSK        On return, pointer to number of bytes in identity's PSK ($retPSK$).

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured for PSK support.

*/
    sbyte4 (*funcPtrLookupPSK)(sbyte4 connectionInstance, ubyte *pIdentityPSK, ubyte4 identityLengthPSK, ubyte retPSK[SSL_PSK_MAX_LENGTH], ubyte4 *pRetLengthPSK);
#endif

#if (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
/* Retrieve (based on the provided hint) the chosen PSK, its identifying name, and their lengths.
This callback function retrieves (based on the provided hint) the chosen PSK and its identifying name, as well
as their lengths. A negative return status indicates that the session
should be aborted.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\note If this function isn't defined, no hint can be returned to the %client.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_PSK_SUPPORT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pHintPSK             Pointer to buffer containing the PSK hint&mdash;a previously agreed on identifier which %client and %server use to look up the PSK.
\param hintLength           Number of bytes (excluding any terminating $NULL$) in $pHintPSK$.
\param retPskIdentity       On return, buffer containing the chosen PSK.
\param pRetPskIdentity      On return, pointer to number of bytes in chosen PSK ($retPskIdentity$).
\param retPSK               On return, buffer containing the chosen PSK's name.
\param pRetLengthPSK        On return, pointer to number of bytes in chosen PSK's name ($retPSK$).

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured for PSK support.

*/
    sbyte4 (*funcPtrChoosePSK)(sbyte4 connectionInstance, ubyte *pHintPSK, ubyte4 hintLength, ubyte retPskIdentity[SSL_PSK_SERVER_IDENTITY_LENGTH], ubyte4 *pRetPskIdentity, ubyte retPSK[SSL_PSK_MAX_LENGTH], ubyte4 *pRetLengthPSK);
#endif

#if defined(__ENABLE_MOCANA_SSL_ALERTS__)
/* Do application-specific work required when the alert is received.
This callback function does any application-specific work required when the
alert is received.

For example, a typical response upon receiving an $SSL_ALERT_ACCESS_DENIED$ error
would be to notify the %client application that this error has occurred.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 1.41
\version 1.41 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ALERTS__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param alertId              SSL alert code (see "SSL Alert Codes").
\param alertClass           Alert class ($SSLALERTLEVEL_WARNING$ or $SSLALERTLEVEL_FATAL$).

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function for your
application if SSL is configured to use alerts.

*/
    sbyte4 (*funcPtrAlertCallback)(sbyte4 connectionInstance, sbyte4 alertId, sbyte4 alertClass);
#endif

#if defined(__ENABLE_MOCANA_SSL_NEW_HANDSHAKE__)
/* Determine whether to grant or ignore a %client or server rehandshake request.
This callback function determines whether to grant or ignore a %client or server
rehandshake rehandshake request. For example, this callback could count the number
of rehandshake requests received, and choose to ignore the request after an
excessive number of attempts (which could indicate a DoS attack).

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 2.45
\version 2.45 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_NEW_HANDSHAKE__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

\param connectionInstance       Pointer to the SSL/TLS Client instance.
\param pRetDoRehandshake        On return, pointer to $TRUE$ if request should
be granted; otherwise, pointer to $FALSE$.
\param pRetDoSessionResumption  On return, pointer to $TRUE$ if request should
be granted; otherwise, pointer to $FALSE$.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark You should define and customize this hookup function if the server
should respond in some fashion to a client's rehandshake request.

*/
    sbyte4 (*funcPtrNewHandshakeCallback)(sbyte4 connectionInstance, sbyte4 *pRetDoRehandshake, sbyte4 *pRetDoSessionResumption);
#endif

#if (defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_RFC3546__))
/* Internal use only.
Doc Note: This function is for Mocana internal code (EAP stack) use only.
*/
    sbyte4 (*funcPtrExtensionRequestCallback)(sbyte4 connectionInstance, ubyte4 extensionType, ubyte *pExtension, ubyte4 extensionLength);
#if defined(__ENABLE_MOCANA_EAP_FAST__)
/* Internal use only.
Doc Note: This function is for Mocana internal code (EAP stack) use only.
*/
    sbyte4 (*funcPtrPACOpaqueCallback)(sbyte4 connectionInstance, ubyte* pPACOpaque, ubyte4 pacOpaqueLen, ubyte pacKey[/*PACKEY_SIZE*/]);
#endif
#endif

#if defined(__ENABLE_MOCANA_INNER_APP__)
/* Internal use only.
Doc Note: This function is for Mocana internal code (EAP stack) use only.
*/
    sbyte4 (*funcPtrInnerAppCallback)(sbyte4 connectionInstance, ubyte* data, ubyte4 dataLen);
#endif


#if (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_RFC3546__))
/* Internal use only.
Doc Note: This function is for Mocana internal code (EAP stack) use only.
*/
    sbyte4 (*funcPtrExtensionApprovedCallback)(sbyte4 connectionInstance, ubyte4 extensionType, ubyte *pApproveExt, ubyte4 approveExtLength);
#endif

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__)) && defined(__ENABLE_MOCANA_DTLS_SRTP__)

/* Initialize SRTP cryptographic context.
This callback function is called at the end of a DTLS handshake to initialize
SRTP cryptographic context for given connectionInstance.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 4.2
\version 4.2 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_DTLS_SRTP__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_DTLS_CLIENT__$
- $__ENABLE_MOCANA_DTLS_SERVER__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pChannelDescr        Pointer peer descriptor.
\param profileId            Profile Id.
\param keyMaterials         Opaque value that contains the SRTP key materials.
Its components can be retrieved from the key material by using the following
macro functions (defined in $dtls_srtp.h$):\n
\n
&bull; $clientMasterKey$\n
&bull; $serverMasterKey$\n
&bull; $clientMasterSalt$\n
&bull; $serverMasterSalt$\n
\param mki                  Pointer to MKI (master key identifier) value, a variable
length value defined with TLS syntax.

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

*/
    sbyte4 (*funcPtrSrtpInitCallback)(sbyte4 connectionInstance, peerDescr *pChannelDescr, const SrtpProfileInfo* pProfile, void* keyMaterials, ubyte* mki);

/* Apply SRTP profile to an RTP packet.
This callback function is called by the DTLS stack at data send to apply
the SRTP profile to an RTP packet. Using this callback is optional; the SRTP
stack can take over the data communication using the DTLS-SRTP negotiated
security profile.

Callback registration happens at session creation and initialization by
assigning your custom callback function (which can have any name) to this
callback pointer.

\since 4.2
\version 4.2 and later

! Flags
To enable this callback, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_DTLS_SRTP__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_DTLS_CLIENT__$
- $__ENABLE_MOCANA_DTLS_SERVER__$

\param connectionInstance   Pointer to the SSL/TLS Client instance.
\param pChannelDescr        Pointer peer descriptor.
\param pData                At call, the RTP packet. On return, pointer to enlarged SRTP packet.
\param pDataLength          At call, number of bytes in RTP packet ($pData$).
On return, pointer to number of bytes in enlarged SRTP packet ($pData$).

\return $OK$ (0) if successful; otherwise a negative number
error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
status, use the $DISPLAY_ERROR$ macro.

\remark Be sure that the data pData buffer is large enough to accomodate the
added bytes of the SRTP profile.

*/
    sbyte4 (*funcPtrSrtpEncodeCallback)(sbyte4 connectionInstance, peerDescr *pChannelDescr, ubyte* pData, ubyte4* pDataLength);
#endif

#ifdef __ENABLE_MOCANA_SSL_EXTERNAL_CERT_CHAIN_VERIFY__
/* Internal use only.
Doc Note: This function is for Mocana internal code use only.
*/
    sbyte4 (*funcPtrExternalCertificateChainVerify)(sbyte4 connectionInstance, struct certDescriptor* pCertChain, ubyte4 numCertsInChain);
#endif


#if defined(__ENABLE_MOCANA_SSL_SERVER__) &&  (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
/* Specify the curve to use for this ECDHE or ECDH_ANON cipher suite. Using
 this callback is optional. If it's not specified, the curve will be
 selected according to the global array gSupportedEllipticCurves.

 Callback registration happens at session creation and initialization by
 assigning your custom callback function (which can have any name) to this
 callback pointer.

 \since 5.5
 \version 5.5 and later

 ! Flags
 To enable this callback, the following flag must be defined in moptions.h:
 - $__ENABLE_MOCANA_SSL_SERVER__$

 Additionally, at least one of the following flags must be defined in moptions.h:
 - $__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__$
 - $__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__$

 \param connectionInstance      Pointer to the SSL/TLS Client instance.
 \param cipherSuiteID           Identifier for the cipher suite.\n
 Values are as specified per RFC 4346 for the TLS
 Cipher Suite Registry; refer to the following Web page: http://www.iana.org/assignments/tls-parameters .
 \param pECCCurvesList          List of ECC curves that can be possibly selected.\n
 This is the intersection of curves supported by both the client and the server.
 \param4 eccCurvesListLength    Number of entries in PSK identity ($pIdentityPSK$).
 \param selectedCurveIndex      On return, the curve to use for the key exchange.\n

 \return $OK$ (0) if successful; otherwise a negative number
 error code definition from merrors.h. To retrieve a string containing an
 English text error identifier corresponding to the function's returned error
 status, use the $DISPLAY_ERROR$ macro.

 \remark Your implementation can return an error if the pECCCurvesList does not
 contain any appropriate curve. This could occur if the client does not support
 the minimum level of security you require for that cipher suite.
 */
    sbyte4 (*funcPtrChooseECCCurve)(sbyte4 connectionInstance, ubyte2 cipherSuiteID,
                                    const enum tlsExtNamedCurves* pECCCurvesList,
                                    ubyte4 eccCurvesListLength,
                                    enum tlsExtNamedCurves* selectedCurve);
#endif

#if (defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_TLSEXT_RFC6066__))
    sbyte4 (*funcPtrCertStatusCallback)(sbyte4 connectionInstance, intBoolean certStatus);
#endif

} sslSettings;


/*------------------------------------------------------------------*/

/* common */
MOC_EXTERN sbyte4  SSL_shutdown(void);
MOC_EXTERN sbyte4  SSL_releaseTables(void);
MOC_EXTERN sbyte4  SSL_getInstanceFromSocket(sbyte4 socket);
MOC_EXTERN sbyte4  SSL_getCookie(sbyte4 connectionInstance, sbyte8 *pCookie);
MOC_EXTERN sbyte4  SSL_setCookie(sbyte4 connectionInstance, sbyte8 cookie);
MOC_EXTERN sslSettings* SSL_sslSettings(void);
#if (!defined(__ENABLE_MOCANA_DTLS_SERVER__) && !defined(__ENABLE_MOCANA_DTLS_CLIENT__))
MOC_EXTERN sbyte4  SSL_getSocketId(sbyte4 connectionInstance, sbyte4 *pRetSocket);
#else
MOC_EXTERN sbyte4  SSL_getPeerDescr(sbyte4 connectionInstance, const peerDescr **ppRetPeerDescr);
#endif
MOC_EXTERN sbyte4  SSL_isSessionSSL(sbyte4 connectionInstance);
MOC_EXTERN sbyte4  SSL_isSessionDTLS(sbyte4 connectionInstance);
MOC_EXTERN sbyte4  SSL_getSessionFlags(sbyte4 connectionInstance, ubyte4 *pRetFlagsSSL);
MOC_EXTERN sbyte4  SSL_getSessionStatus(sbyte4 connectionInstance, ubyte4 *pRetStatusSSL);
MOC_EXTERN sbyte4  SSL_setSessionFlags(sbyte4 connectionInstance, ubyte4 flagsSSL);
MOC_EXTERN sbyte4  SSL_ioctl(sbyte4 connectionInstance, ubyte4 setting, void *value);
MOC_EXTERN sbyte4  SSL_lookupAlert(sbyte4 connectionInstance, sbyte4 lookupError, sbyte4 *pRetAlertId, sbyte4 *pAlertClass);
MOC_EXTERN sbyte4  SSL_sendAlert(sbyte4 connectionInstance, sbyte4 alertId, sbyte4 alertClass);
MOC_EXTERN sbyte4  SSL_enableCiphers(sbyte4 connectionInstance, ubyte2 *pCipherSuiteList, ubyte4 listLength);
#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__)   || \
        defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)|| \
        defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
MOC_EXTERN sbyte4  SSL_enableECCCurves(sbyte4 connectionInstance,
                                       enum tlsExtNamedCurves* pECCCurvesList,
                                       ubyte4 listLength);
#endif
#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__))
MOC_EXTERN sbyte4  SSL_enableCiphersTLS12WithMinLOS(sbyte4 connectionInstance, ubyte2 *pCipherSuiteList, ubyte4 listLength, ubyte2 minLOS);
#endif
MOC_EXTERN sbyte4  SSL_getCipherInfo( sbyte4 connectionInstance, ubyte2* pCipherId, ubyte4* pPeerEcCurves);
MOC_EXTERN sbyte4  SSL_getSSLTLSVersion( sbyte4 connectionInstance, ubyte4* pVersion);
#ifdef __ENABLE_MOCANA_SSL_REHANDSHAKE__
MOC_EXTERN sbyte4  SSL_initiateRehandshake(sbyte4 connectionInstance);
MOC_EXTERN sbyte4  SSL_checkRehandshakeTimer(sbyte4 connectionInstance);
#endif
MOC_EXTERN sbyte4  SSL_getSessionInfo(sbyte4 connectionInstance, ubyte* sessionIdLen, ubyte sessionId[SSL_MAXSESSIONIDSIZE], ubyte masterSecret[SSL_MASTERSECRETSIZE]);

#if defined(__ENABLE_SSL_DYNAMIC_CERTIFICATE__) || (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
MOC_EXTERN sbyte4  SSL_assignCertificateStore(sbyte4 connectionInstance, certStorePtr pCertStore);
#endif

#ifdef __ENABLE_MOCANA_SSL_KEY_EXPANSION__
MOC_EXTERN sbyte4  SSL_generateExpansionKey(sbyte4 connectionInstance, ubyte *pKey,ubyte2 keyLen, ubyte *keyPhrase, ubyte2 keyPhraseLen);
MOC_EXTERN sbyte4  SSL_generateTLSExpansionKey(sbyte4 connectionInstance, ubyte *pKey,ubyte2 keyLen, ubyte *keyPhrase, ubyte2 keyPhraseLen);
#endif

#ifdef __ENABLE_MOCANA_SSL_INTERNAL_STRUCT_ACCESS__
MOC_EXTERN void*   SSL_returnPtrToSSLSocket(sbyte4 connectionInstance);
#endif

/* common client */
#if defined(__ENABLE_MOCANA_SSL_CLIENT__)
MOC_EXTERN sbyte4  SSL_getClientSessionInfo(sbyte4 connectionInstance, ubyte* sessionIdLen, ubyte sessionId[SSL_MAXSESSIONIDSIZE], ubyte masterSecret[SSL_MASTERSECRETSIZE]);
MOC_EXTERN sbyte4  SSL_setServerNameList(sbyte4 connectionInstance, ubyte *pServerNameList, ubyte4 serverNameListLen);  /* TLS 1.0(+) only sessions */
#if defined(__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__)
MOC_EXTERN sbyte4  SSL_setDNSNames( sbyte4 connectionInstance, const CNMatchInfo* cnMatchInfo);
#endif
#if defined(__ENABLE_TLSEXT_RFC6066__)
MOC_EXTERN sbyte4 SSL_setSNI(sbyte4 connectionInstance, ServerName *pServerName, ubyte4 numOfServers);
MOC_EXTERN sbyte4 SSL_setCertifcateStatusRequestExtensions(sbyte4 connectionInstance, sbyte** ppTrustedResponderCertPath,
                        ubyte4 trustedResponderCertCount, intBoolean shouldAddNonceExtension, extensions* pExts, ubyte4 extCount);

#endif
#endif

#if defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_MOCANA_SSL_CLIENT__)
MOC_EXTERN sbyte4  SSL_setEAPFASTParams(sbyte4 connectionInstance, ubyte* pPacOpaque, ubyte4 pacOpaqueLen, ubyte pPacKey[/*PACKEY_SIZE*/]);
#endif
#if defined(__ENABLE_MOCANA_EAP_FAST__)
MOC_EXTERN sbyte4 SSL_getEAPFAST_CHAPChallenge(sbyte4 connectionInstance, ubyte *challenge , ubyte4 challengeLen);
MOC_EXTERN sbyte4  SSL_getEAPFAST_IntermediateCompoundKey(sbyte4 connectionInstance, ubyte *s_imk, ubyte *msk, ubyte mskLen, ubyte *imk);
MOC_EXTERN sbyte4  SSL_generateEAPFASTSessionKeys(sbyte4 connectionInstance, ubyte* S_IMCK, sbyte4 s_imckLen, ubyte* MSK, sbyte4 mskLen, ubyte* EMSK, sbyte4 emskLen);
#endif

#if (defined(__ENABLE_MOCANA_INNER_APP__) && defined(__ENABLE_RFC3546__))
MOC_EXTERN sbyte4 SSL_setInnerApplicationExt(sbyte4 connectionInstance, ubyte4 innerAppValue);
MOC_EXTERN sbyte4 SSL_sendInnerApp(sbyte4 connectionInstance, InnerAppType innerApp, ubyte* pMsg, ubyte4 msgLen,ubyte4 *retMsgLen);
MOC_EXTERN sbyte4 SSL_updateInnerAppSecret(sbyte4 connectionInstance, ubyte* session_key, ubyte4 sessionKeyLen);
MOC_EXTERN sbyte4 SSL_verifyInnerAppVerifyData(sbyte4 connectionInstance,ubyte *data,InnerAppType appType);
#endif

/* common server */
#if defined(__ENABLE_MOCANA_SSL_SERVER__)
MOC_EXTERN sbyte4  SSL_initServerCert(struct certDescriptor *pCertificateDescr, intBoolean isChain, ubyte4 ecCurves);
MOC_EXTERN sbyte4  SSL_initServerCertEx(struct certDescriptor *pCertificateDescr, sbyte4 numCerts, ubyte4 ecCurves);
MOC_EXTERN sbyte4  SSL_retrieveServerNameList(sbyte4 connectionInstance, ubyte *pBuffer, ubyte4 bufferSize, ubyte4 *pRetServerNameLength);  /* TLS 1.0(+) only sessions */

#if defined(__ENABLE_TLSEXT_RFC6066__)
MOC_EXTERN sbyte4 SSL_setOcspResponderUrl(sbyte4 connectionInstance,void * pUrl);
#endif
#endif

/* common synchronous client/server */
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || ((defined(__ENABLE_MOCANA_SSL_SERVER__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__))) || ((defined(__ENABLE_MOCANA_SSL_CLIENT__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)))
MOC_EXTERN sbyte4  SSL_init(sbyte4 numServerConnections, sbyte4 numClientConnections);
#if defined(__ENABLE_MOCANA_SSL_CUSTOM_RNG__)
MOC_EXTERN sbyte4  SSL_initEx(sbyte4 numServerConnections, sbyte4 numClientConnections, RNGFun rngFun, void* rngArg);
#endif
MOC_EXTERN sbyte4  SSL_initWithAsymmetricKey(sbyte4 numServerConnections, sbyte4 numClientConnections, struct AsymmetricKey* pKey);
MOC_EXTERN sbyte4  SSL_negotiateConnection(sbyte4 connectionInstance);
MOC_EXTERN sbyte4  SSL_send(sbyte4 connectionInstance, sbyte *pBuffer, sbyte4 bufferSize);
MOC_EXTERN sbyte4  SSL_recv(sbyte4 connectionInstance, sbyte *pRetBuffer, sbyte4 bufferSize, sbyte4 *pNumBytesReceived, ubyte4 timeout);
MOC_EXTERN sbyte4  SSL_sendPending(sbyte4 connectionInstance, sbyte4 *pNumBytesPending);
MOC_EXTERN sbyte4  SSL_recvPending(sbyte4 connectionInstance, sbyte4 *pRetBooleanIsPending);
MOC_EXTERN sbyte4  SSL_closeConnection(sbyte4 connectionInstance);
#endif
#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
MOC_EXTERN sbyte4
SSL_validateCertParam(sbyte4 connectionInstance,
                      certDescriptor* pCertChain, sbyte4 numCertsInChain,
                      certParamCheckConf *pCertParamCheckConf,
                      certDescriptor* pCACert, errorArray *pStatusArray);
#endif

/* common asynchronous client/server */
#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
MOC_EXTERN sbyte4 SSL_ASYNC_init(sbyte4 numServerConnections, sbyte4 numClientConnections);
#if defined(__ENABLE_MOCANA_SSL_CUSTOM_RNG__)
MOC_EXTERN sbyte4 SSL_ASYNC_initEx(sbyte4 numServerConnections, sbyte4 numClientConnections, RNGFun rngFun, void* rngArg);
#endif
MOC_EXTERN sbyte4 SSL_ASYNC_initServer(struct certDescriptor *pCertificateDescr, sbyte4 pIsChain, ubyte4 ecCurves);
MOC_EXTERN sbyte4 SSL_ASYNC_recvMessage(sbyte4 connectionInstance, ubyte *pBytesReceived, ubyte4 numBytesReceived);
MOC_EXTERN sbyte4 SSL_ASYNC_recvMessage2(sbyte4 connectionInstance, ubyte *pBytesReceived, ubyte4 numBytesReceived, ubyte **ppRetBytesReceived, ubyte4 *pRetNumRxBytesRemaining);
MOC_EXTERN sbyte4 SSL_ASYNC_sendMessage(sbyte4 connectionInstance, sbyte *pBuffer, sbyte4 bufferSize, sbyte4 *pBytesSent);
MOC_EXTERN sbyte4 SSL_ASYNC_sendMessagePending(sbyte4 connectionInstance);
MOC_EXTERN sbyte4 SSL_ASYNC_closeConnection(sbyte4 connectionInstance);
MOC_EXTERN sbyte4 SSL_ASYNC_getSendBuffer(sbyte4 connectionInstance, ubyte *data, ubyte4 *len);
MOC_EXTERN sbyte4 SSL_ASYNC_getRecvBuffer(sbyte4 connectionInstance, ubyte **data, ubyte4 *len, ubyte4 *pRetProtocol);
#endif

#if ((defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) && defined(__ENABLE_MOCANA_SSL_CLIENT__)) || \
     (defined(__ENABLE_MOCANA_SSL_CLIENT__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)))
MOC_EXTERN sbyte4
SSL_connectWithCfgParam(sbyte4 tempSocket,
                        ubyte sessionIdLen, ubyte* sessionId,
                        ubyte* masterSecret, const sbyte* dnsName, certParamCheckConf *pCertParamCheck);
#endif

/* sync server */
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || ((defined(__ENABLE_MOCANA_SSL_SERVER__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)))
MOC_EXTERN sbyte4  SSL_acceptConnection(sbyte4 tempSocket);
#endif

/* sync client */
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || ((defined(__ENABLE_MOCANA_SSL_CLIENT__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)))
MOC_EXTERN sbyte4  SSL_connect(sbyte4 tempSocket, ubyte sessionIdLen, ubyte * sessionId,  ubyte * masterSecret, const sbyte* dnsName);
#endif

/* async server */
#ifdef __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__
MOC_EXTERN sbyte4  SSL_ASYNC_initServerCert(struct certDescriptor *pCertificateDescr, intBoolean pIsChain, ubyte4 ecCurves);
MOC_EXTERN sbyte4  SSL_ASYNC_acceptConnection(sbyte4 tempSocket);
#endif

/* async client */
#ifdef __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__
MOC_EXTERN sbyte4  SSL_ASYNC_connect(sbyte4 tempSocket, ubyte sessionIdLen, ubyte * sessionId, ubyte * masterSecret, const sbyte* dnsName);
MOC_EXTERN sbyte4  SSL_ASYNC_start(sbyte4 connectionInstance);
#endif

#endif /* (defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_CLIENT__)) */

#ifdef __cplusplus
}
#endif

#endif /* __SSL_HEADER__ */
