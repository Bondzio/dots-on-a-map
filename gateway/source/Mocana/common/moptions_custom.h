/* Version: mss_v5_5_26511 */
/*
 * Use this file to add your own custom #defines to moptions.h without the
 * potential for merge conflicts.
 */

#ifndef __MOPTIONS_CUSTOM_HEADER__
#define __MOPTIONS_CUSTOM_HEADER__

#ifdef __cplusplus
extern "C" {
#endif
/*------------------------------------------------------------------*/
/*                Add your own custom #defines here                 */
/*------------------------------------------------------------------*/

// enable string lookup of error messages
#define __ENABLE_LOOKUP_TABLE__

// enable cert conversion from pem
#define __ENABLE_MOCANA_PKCS10__
#define __ENABLE_MOCANA_PEM_CONVERSION__


#define __ENABLE_MOCANA_SSL_CLIENT__
#define __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
#define __ENABLE_MOCANA_SSL_SERVER__
#define __ENABLE_SSL_DYNAMIC_CERTIFICATE__
//#define __ENABLE_RFC3546__
//#define __ENABLE_MOCANA_SSL_PSK_SUPPORT__
//#define __ENABLE_MOCANA_RSA_ALL_KEYSIZE__


#if 0
#define  __ENABLE_MOCANA_DTLS_CLIENT__
#define  __ENABLE_MOCANA_DTLS_SERVER__
#endif

//#define  __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
//#define  __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
//#define  __DISABLE_MOCANA_SHA384__

//#define  __ENABLE_MOCANA_SSL_NEW_HANDSHAKE__
//#define  __ENABLE_MOCANA_SSL_PSK_SUPPORT__
//#define  __ENABLE_RFC3576__
//#define  __ENABLE_TLSEXT_RFC6066__


//#define __ENABLE_MOCANA_SSL_CLIENT_EXAMPLE__
//#define __ENABLE_MOCANA_EXAMPLES__
#define __UCOS__


// prevents a problem calling createThread
#define __DISABLE_MOCANA_RAND_ENTROPY_THREADS__

// need to use if ucos system time is not properly set
#define __MOCANA_DISABLE_CERT_TIME_VERIFY__

// smaller code footprint options
#define __ENABLE_MOCANA_SMALL_CODE_FOOTPRINT__
#define SSL_DEFAULT_SMALL_BUFFER 256
#if 0
#define SSL_RECORDSIZE 1024
#endif

#define RTOS_malloc   UCOS_malloc
#define RTOS_free     UCOS_free

//#define __ENABLE_UCOS_FS__


// for console debug
//#define __ENABLE_MOCANA_DEBUG_CONSOLE__
//#define __MOCANA_DUMP_CONSOLE_TO_STDOUT__
//#define __ENABLE_ALL_DEBUGGING__


/*------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif /* __MOPTIONS_CUSTOM_HEADER__ */
