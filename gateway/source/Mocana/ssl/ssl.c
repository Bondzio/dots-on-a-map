/* Version: mss_v6_3 */
/*
 * ssl.c
 *
 * SSL Developer API
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file ssl.c SSL developer API.
This file contains functions used by NanoSSL servers and clients.

\since 1.41
\version 3.2 and later

! Flags
Whether the following flags are defined determines which additional header files are included:
- $__DEBUG_SSL_TIMER__$
- $__ENABLE_ALL_DEBUGGING__$
- $__ENABLE_MOCANA_DTLS_CLIENT__$
- $__ENABLE_MOCANA_DTLS_SERVER__$

Whether the following flags are defined determines which functions are enabled:
- $__ENABLE_MOCANA_EAP_FAST__$
- $__ENABLE_MOCANA_INNER_APP__$
- $__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__$
- $__ENABLE_MOCANA_SSL_ALERTS__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$
- $__ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__$
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_INTERNAL_STRUCT_ACCESS__$
- $__ENABLE_MOCANA_SSL_KEY_EXPANSION__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_RFC3546__$

! External Functions
This file contains the following public ($extern$) functions:
- SSL_acceptConnection
- SSL_assignCertificateStore
- SSL_ASYNC_acceptConnection
- SSL_ASYNC_closeConnection
- SSL_ASYNC_connect
- SSL_ASYNC_getRecvBuffer
- SSL_ASYNC_getSendBuffer
- SSL_ASYNC_init
- SSL_ASYNC_initServer
- SSL_ASYNC_recvMessage
- SSL_ASYNC_recvMessage2
- SSL_ASYNC_sendMessage
- SSL_ASYNC_sendMessagePending
- SSL_ASYNC_start
- SSL_closeConnection
- SSL_connect
- SSL_enableCiphers
- SSL_getCipherInfo
- SSL_getClientSessionInfo
- SSL_getCookie
- SSL_getInstanceFromSocket
- SSL_getSessionFlags
- SSL_getSessionStatus
- SSL_getSocketId
- SSL_init
- SSL_initiateRehandshake
- SSL_initServerCert
- SSL_ioctl
- SSL_isSessionSSL
- SSL_lookupAlert
- SSL_negotiateConnection
- SSL_recv
- SSL_recvPending
- SSL_releaseTables
- SSL_send
- SSL_sendAlert
- SSL_sendPending
- SSL_setCookie
- SSL_setDNSNames
- SSL_setServerNameList
- SSL_setSessionFlags
- SSL_shutdown
- SSL_sslSettings

*/

#include "../common/moptions.h"

#if (defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_CLIENT__))

#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/mtcp.h"
#include "../common/moc_net.h"
#include "../common/random.h"
#include "../common/vlong.h"
#include "../common/debug_console.h"
#include "../common/sizedbuffer.h"
#include "../common/mem_pool.h"
#include "../common/hash_value.h"
#include "../common/hash_table.h"
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
#include "../crypto/dh.h"
#include "../crypto/ca_mgmt.h"
#if defined(__ENABLE_SSL_DYNAMIC_CERTIFICATE__) || (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
#include "../crypto/cert_store.h"
#endif
#include "../harness/harness.h"
#include "../ssl/ssl.h"
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
#include "../common/circq.h"
#include "../dtls/dtls.h"
#if (defined(__ENABLE_MOCANA_DTLS_SRTP__))
#include "../dtls/dtls_srtp.h"
#endif
#include "../common/timer.h"
#endif
#include "../ssl/sslsock.h"
#if (defined(__ENABLE_TLSEXT_RFC6066__) && defined(__ENABLE_MOCANA_OCSP_CLIENT__))
#include "../ssl/ssl_ocsp.h"
#endif

#if (defined(__DEBUG_SSL_TIMER__) || defined(__ENABLE_ALL_DEBUGGING__))
#include <stdio.h>
#endif


/*------------------------------------------------------------------*/


/*!
\exclude
*/
typedef struct
{
    ubyte2      age;
    sbyte4      instance;
    SSLSocket*  pSSLSock;
    sbyte4      connectionState;
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    peerDescr   peerDescr;
#endif
    TCP_SOCKET  socket;
    intBoolean  isClient;

#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))
    /* non-blocking read data buffers */
    ubyte*      pReadBuffer;
    ubyte*      pReadBufferPosition;
    ubyte4      numBytesRead;
#endif

#ifdef __PSOS_RTOS__
    unsigned long tid;
#endif

} sslConnectDescr;



/* session related bits and masks */
#define MASK_SSL_SESSION_INDEX            (0x0000ffff)
#define NUM_BITS_SSL_SESSION_INDEX        (0)

#define MASK_SSL_SESSION_AGE              (0xffff0000)
#define NUM_BITS_SSL_SESSION_AGE          (16)
#define SESSION_AGE(X)                     ((X)->age)

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__))
/*!
\exclude
*/
typedef struct
{
    peerDescr   peerDescr;
    ubyte4      startTime;
} sslConnectTimedWaitDescr;

static c_queue_t *m_sslConnectTimedWaitQueue = NULL;

#endif



/*------------------------------------------------------------------*/

extern sbyte4 SSL_findConnectionInstance(SSLSocket *pSSLSock);

static sslSettings      m_sslSettings;
static sslConnectDescr* m_sslConnectTable = NULL;
static RTOS_MUTEX       m_sslConnectTableMutex;
static sbyte4           m_sslMaxConnections;
static hashTableOfPtrs* m_sslConnectHashTable = NULL;

/* random number generator */
static RNGFun mSSL_rngFun;
static void*  mSSL_rngArg;

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
static sbyte4           m_sslNumCiphersEnabled;
#endif

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)))
#define SSL_EXTERN      extern
#else
/* hide synchronous apis, if async enabled */
#define SSL_EXTERN      static
#endif

#if defined(__ENABLE_MOCANA_SSL_SERVER__)
/*!
\exclude
*/
RTOS_MUTEX gSslSessionCacheMutex;

static SizedBuffer*     m_certificates = NULL;
static sbyte4           m_numCerts = 0;
static AsymmetricKey    m_privateKey;   /* static initialized to type akt_undefined = 0 */
static AsymmetricKey*   m_pPrivateKey = &m_privateKey;
static ubyte4           m_certificateECCurves;

static int isDefaultKey()
{
    return (&m_privateKey == m_pPrivateKey);
}


#endif  /* __ENABLE_MOCANA_SSL_SERVER__ */

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
extern MSTATUS getNumBytesSent(ubyte *pOutputBuffer, ubyte4 maxLen, ubyte4 *pNumBytesSent);
extern MSTATUS cleanupOutputBuffer(SSLSocket *pSSLSock);
#endif

/*------------------------------------------------------------------*/
/* INIT_HASH_VALUE is a seed value to throw off attackers */
#define INIT_HASH_VALUE   (0xab341c12)

typedef struct
{
    intBoolean isDTLS;
    peerDescr *pPeerDescr;
    TCP_SOCKET socket;

} testData;


/*---------------------------------------------------------------------------*/

static MSTATUS
allocHashPtrElement(void *pHashCookie, hashTablePtrElement **ppRetNewHashElement)
{
    MSTATUS status = OK;

    if (NULL == (*ppRetNewHashElement = MALLOC(sizeof(hashTablePtrElement))))
        status = ERR_MEM_ALLOC_FAIL;

    return status;
}

/*---------------------------------------------------------------------------*/

static MSTATUS
freeHashPtrElement(void *pHashCookie, hashTablePtrElement *pFreeHashElement)
{
    if (NULL == pFreeHashElement)
        return ERR_NULL_POINTER;

    FREE(pFreeHashElement);

    return OK;
}


/*---------------------------------------------------------------------------*/

static MSTATUS
hashtable_hashGen(intBoolean isDTLS, TCP_SOCKET socket, peerDescr *pPeerDescr, ubyte4 *pHashValue)
{
    MSTATUS status = OK;
    ubyte4 addrLen = 0;
    MOC_IP_ADDRESS pMocAddr;

    if (isDTLS)
    {
        ubyte buf[2*sizeof(ubyte2) + 2*MOCADDRSIZE];
        MOC_MEMCPY(buf, &pPeerDescr->srcPort, sizeof(ubyte2));
        addrLen = MOCADDRSIZE;
        pMocAddr = MOC_AND(pPeerDescr->srcAddr);
        MOC_MEMCPY(buf+sizeof(ubyte2), MOCADDR( (MOC_STAR(pMocAddr)) ), addrLen);
        MOC_MEMCPY(buf+sizeof(ubyte2)+addrLen, &pPeerDescr->peerPort, sizeof(ubyte2));
        /*TEST_MOC_IPADDR6(&pPeerDescr->peerAddr, {addrLen = 16; MOC_MEMCPY(buf+sizeof(void*)+sizeof(ubyte2), GET_MOC_IPADDR6(&pPeerDescr->peerAddr), addrLen);})
        {addrLen = 4;  MOC_MEMCPY(buf+sizeof(void*)+sizeof(ubyte2), GET_MOC_IPADDR4(&pPeerDescr->peerAddr), addrLen); }
        */
        pMocAddr = MOC_AND(pPeerDescr->peerAddr);
        MOC_MEMCPY(buf+addrLen+2*sizeof(ubyte2), MOCADDR( (MOC_STAR(pMocAddr)) ), addrLen);


        /* pointer to udpDescr, ubyte2 port, and IP addr */
        HASH_VALUE_hashGen(buf, 2*sizeof(ubyte2) + 2*addrLen, INIT_HASH_VALUE, pHashValue);
    }
    else
    {
        HASH_VALUE_hashGen(&socket, sizeof(TCP_SOCKET), INIT_HASH_VALUE, pHashValue);
    }

    return status;
}

/*---------------------------------------------------------------------------*/

static MSTATUS
hashtable_insert(hashTableOfPtrs* pHashTable, intBoolean isDTLS, TCP_SOCKET socket, peerDescr *pPeerDescr, sslConnectDescr *pSslConnectDescr)
{
    ubyte4  hashValue;
    MSTATUS status;

    hashtable_hashGen(isDTLS, socket, pPeerDescr, &hashValue);    

    if (OK > (status = HASH_TABLE_addPtr(pHashTable, hashValue, pSslConnectDescr)))
        goto exit;

exit:
    return status;
}

/*---------------------------------------------------------------------------*/

static MSTATUS
hashtable_extraMatchTest(void *pAppData, void *pTestData, intBoolean *pRetIsMatch)
{
    sslConnectDescr *pSslConnectDescr = (sslConnectDescr*) pAppData;
    testData *pTestDataTemp = (testData*) pTestData;

    *pRetIsMatch = FALSE;

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
    if (pTestDataTemp->isDTLS)
    {
        MOC_IP_ADDRESS srcAddrRef = REF_MOC_IPADDR(pTestDataTemp->pPeerDescr->srcAddr);
        MOC_IP_ADDRESS peerAddrRef = REF_MOC_IPADDR(pTestDataTemp->pPeerDescr->peerAddr);

        if (pTestDataTemp->pPeerDescr->srcPort == pSslConnectDescr->peerDescr.srcPort &&
            SAME_MOC_IPADDR(srcAddrRef, pSslConnectDescr->peerDescr.srcAddr) &&
            pTestDataTemp->pPeerDescr->peerPort == pSslConnectDescr->peerDescr.peerPort &&
            SAME_MOC_IPADDR(peerAddrRef, pSslConnectDescr->peerDescr.peerAddr))
        {
            *pRetIsMatch = TRUE;
        }
    } else
#endif
    if (pTestDataTemp->socket == pSslConnectDescr->socket)
    {
        *pRetIsMatch = TRUE;
    }
    return OK;
}

/*---------------------------------------------------------------------------*/

static MSTATUS
hashtable_remove(hashTableOfPtrs* pHashTable, intBoolean isDTLS, TCP_SOCKET socket, peerDescr *pPeerDescr)
{
    ubyte4  hashValue;
    MSTATUS status;
    sslConnectDescr* pSslConnectDescrToDelete;
    intBoolean retFoundHashValue;
    testData testDataTemp;

    hashtable_hashGen(isDTLS, socket, pPeerDescr, &hashValue);

    testDataTemp.isDTLS = isDTLS;
    testDataTemp.socket = socket;
    testDataTemp.pPeerDescr = pPeerDescr;
    if (OK > (status = HASH_TABLE_deletePtr(pHashTable, hashValue, &testDataTemp, hashtable_extraMatchTest, (void**)&pSslConnectDescrToDelete, &retFoundHashValue)))
        goto exit;

exit:
    return status;
}

/*---------------------------------------------------------------------------*/

static MSTATUS
hashtable_find(hashTableOfPtrs* pHashTable, intBoolean isDTLS, TCP_SOCKET socket, peerDescr *pPeerDescr, sslConnectDescr** ppSslConnectDescr)
{
    intBoolean  foundHashValue;
    ubyte4  hashValue;
    MSTATUS status;
    testData testDataTemp;

    hashtable_hashGen(isDTLS, socket, pPeerDescr, &hashValue);

    testDataTemp.isDTLS = isDTLS;
    testDataTemp.socket = socket;
    testDataTemp.pPeerDescr = pPeerDescr;

    if (OK > (status = HASH_TABLE_findPtr(pHashTable, hashValue, &testDataTemp, hashtable_extraMatchTest, (void**)ppSslConnectDescr, &foundHashValue)))
        goto exit;

    if (FALSE == foundHashValue)
        status = ERR_SSL_BAD_ID;

exit:
    return status;
}

/*---------------------------------------------------------------------------*/

static sbyte4
getIndexFromConnectionInstance(sbyte4 connectionInstance)
{
    sbyte4 status;
    ubyte2 index = (ubyte2)((connectionInstance & MASK_SSL_SESSION_INDEX) >> NUM_BITS_SSL_SESSION_INDEX);

    if (index >= m_sslMaxConnections)
    {
        /* index wrong, a severe bug */
        status = ERR_SSL_BAD_ID;
        goto exit;
    }

    if (CONNECT_CLOSED  == m_sslConnectTable[index].connectionState)
    {
        /* good index in table, but the connection is closed. */
        status = ERR_SSL_BAD_ID;
        goto exit;
    }

    if (((connectionInstance & MASK_SSL_SESSION_AGE) >> NUM_BITS_SSL_SESSION_AGE) != (ubyte4)((SESSION_AGE(&(m_sslConnectTable[index])))))
    {
        /* good index in table, but the age is wrong. */
        status = ERR_SSL_BAD_ID;
        goto exit;
    }


    status = index;

exit:
    return status;
}

/*---------------------------------------------------------------------------*/

static MSTATUS
setMessageTimer(SSLSocket *pSSLSock, sbyte4 connectionInstance, ubyte4 msTimeToExpire)
{
    MSTATUS status = OK;
#if !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
    MOC_UNUSED(connectionInstance);
#endif

    if (NULL == pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    RTOS_deltaMS(NULL, &(SSL_TIMER_START_TIME(pSSLSock)));
    SSL_TIMER_MS_EXPIRE(pSSLSock)  = msTimeToExpire;  /* in milliseconds */

#ifdef __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__
    if (IS_SSL_ASYNC(pSSLSock))
    {
        if (1 == pSSLSock->server)
        {
            if (NULL != m_sslSettings.funcPtrStartTimer)
                m_sslSettings.funcPtrStartTimer(connectionInstance, msTimeToExpire, 0);
        }
    }
#endif

#ifdef __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__
    if (IS_SSL_ASYNC(pSSLSock))
    {
        if (0 == pSSLSock->server)
        {
            if (NULL != m_sslSettings.funcPtrClientStartTimer)
                m_sslSettings.funcPtrClientStartTimer(connectionInstance, msTimeToExpire, 0);
        }
    }
#endif

exit:
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
sendPendingBytes(SSLSocket* pSSLSock, sbyte4 index)
{
    ubyte4  numBytesSent = 0;
    MSTATUS status = OK;

    /* do we need to write any pending data? */
    if (!((NULL == pSSLSock->pOutputBuffer) || (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
    {
#ifndef __MOCANA_IPSTACK__
        if (OK <= (status = TCP_WRITE(pSSLSock->tcpSock, (sbyte *)pSSLSock->pOutputBuffer, pSSLSock->numBytesToSend, &numBytesSent)))
#else
        if (OK <= (status = MOC_TCP_WRITE(pSSLSock->tcpSock, (sbyte *)pSSLSock->pOutputBuffer, pSSLSock->numBytesToSend, &numBytesSent)))
#endif
        {
            if (numBytesSent > pSSLSock->numBytesToSend)
                pSSLSock->numBytesToSend = numBytesSent = 0;        /*!!!! should never happen */

            pSSLSock->pOutputBuffer  = numBytesSent + pSSLSock->pOutputBuffer;
            pSSLSock->numBytesToSend = pSSLSock->numBytesToSend - numBytesSent;
        }
    }

    if (0 == pSSLSock->numBytesToSend)
    {
        if (NULL != pSSLSock->pOutputBufferBase)
            FREE(pSSLSock->pOutputBufferBase);

        pSSLSock->pOutputBufferBase = NULL;
        pSSLSock->pOutputBuffer     = NULL;

    }

    /* data is still pending, bail out */
    if (!((NULL == pSSLSock->pOutputBuffer) || (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
        goto exit;

    if ((NULL == pSSLSock->pOutputBuffer) && (NULL != pSSLSock->buffers[0].pHeader))
    {
        if (OK > (status = SSLSOCK_sendEncryptedHandshakeBuffer(pSSLSock)))
            goto exit;
    }

#if 1
    /*!!!! need additional test here */
    if (kSslOpenState != SSL_HANDSHAKE_STATE(pSSLSock))
    {
        if (0 == pSSLSock->server)
        {
#ifdef __ENABLE_MOCANA_SSL_CLIENT__
            status = SSL_SOCK_clientHandshake(pSSLSock, TRUE);
#endif
        }
        else
        {
#ifdef __ENABLE_MOCANA_SSL_SERVER__
            status = SSL_SOCK_serverHandshake(pSSLSock, TRUE);
#endif
        }
    }
#endif

exit:
    return status;
}

/*------------------------------------------------------------------*/
#if defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_TLSEXT_RFC6066__)

static void
setShortValue(ubyte shortBuff[2], ubyte2 val)
{
    shortBuff[1] = (ubyte)(val & 0xFF);
    shortBuff[0] = (ubyte)((val >> 8) & 0xFF);
}

#endif
/*------------------------------------------------------------------*/

static sbyte4 
updateThreadCount(intBoolean val, SSLSocket *pSSLSock)
{
    sbyte4 status = OK;
    intBoolean isMutexTaken = FALSE;

    if (OK > (status = RTOS_mutexWait(m_sslConnectTableMutex)))
    {
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexWait( ) failed.");
        goto exit;
    }

    isMutexTaken = TRUE;

    if (TRUE == val)
    {
        if (FALSE == pSSLSock->blockConnection)
            pSSLSock->threadCount++;
        else
            status = ERR_SSL_CONNECTION_BUSY;

        goto exit;
    }

    if (FALSE == val && (0 < pSSLSock->threadCount))
        pSSLSock->threadCount--;
    else
        status = ERR_SSL_CONNECTION_BUSY;

exit:
    if (TRUE == isMutexTaken)
    {
        if (OK > (RTOS_mutexRelease(m_sslConnectTableMutex)))
        {
            MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexRelease( ) failed.");
        }
    }

    return status;
}



/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
/* Get a pointer to the connection's receive data buffer (the socket buffer itself).
This function returns a pointer (through the $data$ parameter) to the specified
connection's most recently received data buffer (the socket buffer itself).

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.
\param data                 On return, pointer to the address of the connection's receive buffer.
\param len                  On return pointer to number of bytes in $data$.
\param pRetProtocol         On return, the SSL protocol type for $data$ (usually 23 == SSL Application Data)

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous clients and servers.
*/
extern sbyte4
SSL_ASYNC_getRecvBuffer(sbyte4 connectionInstance, ubyte **data, ubyte4 *len, ubyte4 *pRetProtocol)
{
    sbyte4      index;
    SSLSocket*  pSSLSock = NULL;
    MSTATUS     status   = ERR_SSL_BAD_ID;

    if ((NULL == data) || (NULL == len))
        return ERR_NULL_POINTER;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    pSSLSock = m_sslConnectTable[index].pSSLSock;

    if (NULL == pSSLSock)
    {
        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_ASYNC_getRecvBuffer: connectionInstance not found.");
        goto exit;
    }

    if (IS_SSL_SYNC(pSSLSock))
        goto exit;

    status = OK;

    if (NULL == pSSLSock->pReceiveBuffer)
    {
        status = ERR_SSL_NO_DATA_TO_RECEIVE;
        goto exit;
    }

    *data = (ubyte *)pSSLSock->pReceiveBuffer;
    *len = pSSLSock->recordSize - pSSLSock->offset;

    if (pRetProtocol)
        *pRetProtocol = pSSLSock->protocol;

exit:
    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code use only, and
should not be included in the API documentation.
*/
extern MSTATUS
SSL_INTERNAL_setConnectionState(sbyte4 connectionInstance, sbyte4 connectionState)
{
    sbyte4      index;
    MSTATUS     status   = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    m_sslConnectTable[index].connectionState = connectionState;
    status = OK;
exit:
    return status;
}


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
/* Get a copy of the connection's send data buffer.
This function returns a copy (through the $data$ parameter) of the specified
connection's most recently sent data buffer.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.
\param data                 On return, pointer to the buffer containing the data in the connection's send buffer.
\param len                  On return pointer to number of bytes in $data$.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous clients and servers.
*/
extern sbyte4
SSL_ASYNC_getSendBuffer(sbyte4 connectionInstance, ubyte *data, ubyte4 *len)
{
    ubyte4      numBytesSent;
    SSLSocket*  pSSLSock = NULL;
    MSTATUS     status   = ERR_SSL_BAD_ID;

    if ((NULL == data) || (NULL == len))
        return ERR_NULL_POINTER;

    numBytesSent = *len;

    *len = 0;

    if (OK > (status = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    pSSLSock = m_sslConnectTable[status].pSSLSock;

    if (NULL == pSSLSock)
    {
        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_ASYNC_getSendBuffer: connectionInstance not found.");
        goto exit;
    }

    if (IS_SSL_SYNC(pSSLSock))
        goto exit;

    status   = OK;

    if (NULL == pSSLSock->pOutputBuffer)
    {
        status = ERR_SSL_NO_DATA_TO_SEND;
        goto exit;
    }

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    /* for DTLS, mindful of record boundaries and pmtu !!! */
    if (pSSLSock->isDTLS)
    {
        if (numBytesSent > pSSLSock->dtlsPMTU)
            numBytesSent = pSSLSock->dtlsPMTU;

        if (numBytesSent > pSSLSock->numBytesToSend)
        {
            numBytesSent = pSSLSock->numBytesToSend ;        /*!!!! should never happen */
        } else
        {
            if (OK > (status = getNumBytesSent(pSSLSock->pOutputBuffer, numBytesSent, &numBytesSent)))
                goto exit;
        }
    } else
#endif
    {
        if (numBytesSent > pSSLSock->numBytesToSend)
            numBytesSent = pSSLSock->numBytesToSend ;        /*!!!! should never happen */
    }

    MOC_MEMCPY(data,pSSLSock->pOutputBuffer,numBytesSent);
    pSSLSock->pOutputBuffer  = numBytesSent + pSSLSock->pOutputBuffer;
    pSSLSock->numBytesToSend = pSSLSock->numBytesToSend - numBytesSent;
    *len = numBytesSent;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        if (0 == pSSLSock->numBytesToSend)
        {
            if (OK > (status = cleanupOutputBuffer(pSSLSock)))
                goto exit;
        }
    } else
#endif
    {
        if (0 == pSSLSock->numBytesToSend)
        {
            if (NULL != pSSLSock->pOutputBufferBase)
                FREE(pSSLSock->pOutputBufferBase);

            pSSLSock->pOutputBufferBase = NULL;
            pSSLSock->pOutputBuffer     = NULL;
        }
    }
exit:
    return (sbyte4)status;

} /* SSL_ASYNC_getSendBuffer */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)))
static MSTATUS
doProtocol(SSLSocket *pSSLSock, sbyte4 index, intBoolean useTimeout,
           ubyte4 timeout, sbyte *pRetBuffer, sbyte4 bufferSize,
           sbyte4 *pRetNumBytesReceived)
{
    sbyte4  startState = m_sslConnectTable[index].connectionState;
    ubyte4  adjustedTimeout;
    MSTATUS status = OK;

    if (NULL == pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (TRUE == useTimeout)
    {
        RTOS_deltaMS(NULL, &(SSL_TIMER_START_TIME(pSSLSock)));
        SSL_TIMER_MS_EXPIRE(pSSLSock) = timeout;
    }

    do
    {
        /* don't spin loop, if writes fail, yield the processor briefly */
        if ((CONNECT_NEGOTIATE == m_sslConnectTable[index].connectionState) && (NULL != pSSLSock->pOutputBuffer))
            RTOS_sleepMS(SSL_WRITE_FAIL_RETRY_TIME);
#ifndef __ENABLE_NONBLOCKING_SOCKET_CONNECT__ 
        /* handle across events time outs */
        timeout   = SSL_TIMER_MS_EXPIRE(pSSLSock);

        if (TCP_NO_TIMEOUT != timeout)
        {
            adjustedTimeout = RTOS_deltaMS(&(SSL_TIMER_START_TIME(pSSLSock)), NULL);

            if (adjustedTimeout >= timeout)
            {
                status = ERR_TCP_READ_TIMEOUT;
                goto exit;
            }

            adjustedTimeout = timeout - adjustedTimeout;
        }
        else
        {
            adjustedTimeout = TCP_NO_TIMEOUT;  /* timeout */
        }
#endif
        if (OK > (status = sendPendingBytes(pSSLSock, index)))
            goto exit;

        if ((NULL != pSSLSock->pOutputBuffer) || (startState != m_sslConnectTable[index].connectionState))
        {
            status = OK;
            continue;
        }

        if ((CONNECT_OPEN == startState) &&
            (kRecordStateReceiveFrameComplete == SSL_SYNC_RECORD_STATE(pSSLSock)) &&
            (0 < (pSSLSock->recordSize - pSSLSock->offset)))
        {
            ubyte*  pDummy = NULL;
            ubyte4  dummy  = 0;

            status = (MSTATUS)SSL_SOCK_receive(pSSLSock, pRetBuffer,
                                               bufferSize, &pDummy, &dummy, pRetNumBytesReceived);
        }
        else
        {
            if (0 == m_sslConnectTable[index].numBytesRead)
            {
#ifndef __MOCANA_IPSTACK__
                if (OK <= (status = TCP_READ_AVL(pSSLSock->tcpSock,
                                                 (sbyte *)m_sslConnectTable[index].pReadBuffer,
                                                 SSL_SYNC_BUFFER_SIZE,
                                                 &m_sslConnectTable[index].numBytesRead,
                                                 adjustedTimeout)))
#else
                if (OK <= (status = MOC_TCP_READ_AVL(pSSLSock->tcpSock,
                                                 (sbyte *)m_sslConnectTable[index].pReadBuffer,
                                                 SSL_SYNC_BUFFER_SIZE,
                                                 &m_sslConnectTable[index].numBytesRead,
                                                 adjustedTimeout)))
#endif
                {
                    m_sslConnectTable[index].pReadBufferPosition = m_sslConnectTable[index].pReadBuffer;
                }
                else
                {
                    goto exit;
                }
            }

            status = (MSTATUS)SSL_SOCK_receive(m_sslConnectTable[index].pSSLSock,
                                                pRetBuffer, bufferSize,
                                                &m_sslConnectTable[index].pReadBufferPosition,
                                                &m_sslConnectTable[index].numBytesRead,
                                                pRetNumBytesReceived);
        }

        if (CONNECT_NEGOTIATE == startState)
        {
            *pRetNumBytesReceived = 0;
        }

        if (OK < status)
            status = OK;
    }
    while ((OK == status) && (0 == *pRetNumBytesReceived) && (startState == m_sslConnectTable[index].connectionState));

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte *)"SSL:doProtocol() returns status = ", (sbyte4)status);
#endif

    return status;

} /* doProtocol */
#endif


/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_findConnectionInstance(SSLSocket *pSSLSock)
{
    sslConnectDescr* pSslConnectDescr;

    if (NULL == pSSLSock)
        goto exit;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
    if (pSSLSock->isDTLS)
    {
        if (OK > (hashtable_find(m_sslConnectHashTable, pSSLSock->isDTLS, (TCP_SOCKET)-1, &pSSLSock->peerDescr, &pSslConnectDescr)))
            goto exit;
    } else
#endif
    {
        if (OK > (hashtable_find(m_sslConnectHashTable, FALSE, pSSLSock->tcpSock, NULL, &pSslConnectDescr)))
            goto exit;
    }

    if (CONNECT_CLOSED != pSslConnectDescr->connectionState)
    {
        return pSslConnectDescr->instance;
    }

exit:
    return (sbyte4)ERR_SSL_BAD_ID;
}

/*------------------------------------------------------------------*/
#ifdef __ENABLE_MOCANA_DTLS_SERVER__

static sbyte4
SSL_findConnectTimedWait(MOC_IP_ADDRESS srcAddr, ubyte2 srcPort, MOC_IP_ADDRESS peerAddr, ubyte2 peerPort)
{
    MSTATUS status = OK;
    ubyte4 head;
    ubyte4 tail;
    ubyte4 capacity;
    ubyte4 curTime;

    curTime = RTOS_getUpTimeInMS();

    head = m_sslConnectTimedWaitQueue->head;
    tail = m_sslConnectTimedWaitQueue->tail;
    capacity = m_sslConnectTimedWaitQueue->capacity;

    /* check time and purge all expired connections */
    for (; head != tail; head = (head + 1) % (capacity + 1))
    {
        sslConnectTimedWaitDescr *descr = (sslConnectTimedWaitDescr *)m_sslConnectTimedWaitQueue->ppQueue[head];
        if ((curTime - descr->startTime) > 2*60*1000) /* timeout after 2 minutes */
        {
            CIRCQ_deq(m_sslConnectTimedWaitQueue, (ubyte**)&descr);
            FREE(descr);
        }
        else
            break;
    }
    for (; head != tail; head = (head + 1) % (capacity + 1))
    {
        sslConnectTimedWaitDescr *descr = (sslConnectTimedWaitDescr *)m_sslConnectTimedWaitQueue->ppQueue[head];
        if (descr->peerDescr.srcPort == srcPort &&
            SAME_MOC_IPADDR(srcAddr, descr->peerDescr.srcAddr) &&
            SAME_MOC_IPADDR(peerAddr, descr->peerDescr.peerAddr) &&
            descr->peerDescr.peerPort == peerPort)
        {
            status = ERR_DTLS_CONNECT_TIMED_WAIT;
            goto exit;
        }
    }

exit:
    return status;
}

/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code (DTLS stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_getConnectionInstance(MOC_IP_ADDRESS srcAddr, ubyte2 srcPort, MOC_IP_ADDRESS peerAddr, ubyte2 peerPort)
{
    MSTATUS status;
    sslConnectDescr *pSslConnectDescr;
    peerDescr tempPeerDescr;

    if (OK > (status = SSL_findConnectTimedWait(srcAddr, srcPort, peerAddr, peerPort)))
        goto exit;

    tempPeerDescr.srcPort = srcPort;
    COPY_MOC_IPADDR(tempPeerDescr.srcAddr, srcAddr);
    tempPeerDescr.peerPort = peerPort;
    COPY_MOC_IPADDR(tempPeerDescr.peerAddr, peerAddr);

    if (OK > (status = hashtable_find(m_sslConnectHashTable, TRUE, (TCP_SOCKET)-1, &tempPeerDescr, &pSslConnectDescr)))
        goto exit;

    return pSslConnectDescr->instance;

exit:
    return status;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code (DTLS stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_removeConnectTimedWait(MOC_IP_ADDRESS srcAddr, ubyte2 srcPort, MOC_IP_ADDRESS peerAddr, ubyte2 peerPort)
{
    MSTATUS status = OK;
    ubyte4 head;
    ubyte4 tail;
    ubyte4 capacity;

    head = m_sslConnectTimedWaitQueue->head;
    tail = m_sslConnectTimedWaitQueue->tail;
    capacity = m_sslConnectTimedWaitQueue->capacity;

    /* check time and purge all expired connections */
    for (; head != tail; head = (head + 1) % (capacity + 1))
    {
        sslConnectTimedWaitDescr *descr = (sslConnectTimedWaitDescr *)m_sslConnectTimedWaitQueue->ppQueue[head];
        if (descr->peerDescr.srcPort == srcPort &&
            SAME_MOC_IPADDR(srcAddr, descr->peerDescr.srcAddr) &&
            SAME_MOC_IPADDR(peerAddr, descr->peerDescr.peerAddr) &&
            descr->peerDescr.peerPort == peerPort)
        {
            CIRCQ_deq(m_sslConnectTimedWaitQueue, (ubyte**)&descr);
            FREE(descr);
            break;
        }
    }

    return status;
}
#endif


/*------------------------------------------------------------------*/

/* Clean up memory and mutexes and shut down the SSL stack.
This function performs memory and mutex cleanup and shuts down the SSL stack. In
rare instances, for example changing the port number to which an embedded device
listens, you may need to completely stop the SSL/TLS Client/Server and all its
resources. However, in most circumstances this is unnecessary because the
NanoSSL %client/server is threadless.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

\example
sbyte4 status = 0;

status = SSL_shutdown();
\endexample

*/
extern sbyte4
SSL_shutdown(void)
{
    MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"SSL stack shutting down.");

    RTOS_mutexFree( &m_sslConnectTableMutex);

#if defined(__ENABLE_MOCANA_SSL_SERVER__)
    CRYPTO_uninitAsymmetricKey(m_pPrivateKey, NULL);
    while (m_numCerts > 0)
    {
        SB_Release(&m_certificates[--m_numCerts]);
    }
    if (m_certificates)
        FREE(m_certificates);
    m_certificates = NULL;
    RTOS_mutexFree(&gSslSessionCacheMutex);
#ifdef __ENABLE_TLSEXT_RFC6066__
    if (m_sslSettings.pCachedOcspResponse)
        FREE (m_sslSettings.pCachedOcspResponse);
#endif
#endif

#ifndef __DISABLE_MOCANA_INIT__
    gMocanaAppsRunning--;
#endif

    return (sbyte4)OK;
}


/*------------------------------------------------------------------*/

/* Release memory used by internal SSL/TLS memory tables.
This function releases the SSL/TLS Client's or Server's internal memory tables.
It should only be called after a call to SSL_shutdown. To resume
communication with a device after calling this function, you must create a new
connection and register encryption keys and an X.509 certificate.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

\example
sbyte4 status;

status = SSL_releaseTables();
\endexample
*/
extern sbyte4
SSL_releaseTables(void)
{
    void*               pRetHashCookie = NULL;

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__))
    sslConnectTimedWaitDescr *pDescr;
#endif

    if (NULL != m_sslConnectTable)
    {
        FREE(m_sslConnectTable);
        m_sslConnectTable = NULL;
    }

    if (NULL != m_sslConnectHashTable)
    {
        HASH_TABLE_removePtrsTable(m_sslConnectHashTable, &pRetHashCookie);
        m_sslConnectHashTable = NULL;
    }

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__))
    while (OK == CIRCQ_deq(m_sslConnectTimedWaitQueue, (ubyte**)&pDescr))
    {
        FREE(pDescr);
    }
    CIRCQ_deInit(m_sslConnectTimedWaitQueue);
#endif

    return (sbyte4)OK;
}


/*------------------------------------------------------------------*/

/* Get a socket's connection instance.
This function returns a connection instance for the specified socket identifier.
The connection instance can be used as a parameter in subsequent calls to
NanoSSL %client and server functions.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param socket   TCP/IP socket for which you want to retrieve a connection instance.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

\example
sbyte4 connectionInstance;
TCP_SOCKET socketClient;

connectionInstance = SSL_getInstanceFromSocket(socketClient);
\endexample
*/
extern sbyte4
SSL_getInstanceFromSocket(sbyte4 socket)
{
    sbyte4  status;
    sslConnectDescr *pSslConnectDescr;
#ifdef __PSOS_RTOS__
    unsigned long tid;

    t_ident((char *)0, 0, &tid);
#endif

    if (OK > (status = hashtable_find(m_sslConnectHashTable, FALSE, socket, NULL, &pSslConnectDescr)))
        goto exit;

    if (((TCP_SOCKET)socket == pSslConnectDescr->socket) &&
#ifdef __PSOS_RTOS__
        (tid == pSslConnectDescr->tid) &&
#endif
        (CONNECT_CLOSED < pSslConnectDescr->connectionState))
    {
        status = pSslConnectDescr->instance;
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if ((sbyte4)OK > status)
    {
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_getInstanceFromSocket() returns status = ", (sbyte4)status);
    }
#endif

exit:
    return status;
}

/*------------------------------------------------------------------*/

/* Get custom information for a connection instance.
This function retrieves custom information stored in the connection instance's
context. Your application should not call this function until after calls to
SSL_setCookie.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pCookie              On return, pointer to the cookie containing the context's custom information.

\remark This function is applicable to synchronous and asynchronous clients and servers.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\example
mySessionInfo *myCookie = NULL;

SSL_getCookie(connectionInstance, (int *)(&myCookie));
\endexample
*/
extern sbyte4
SSL_getCookie(sbyte4 connectionInstance, sbyte8 *pCookie)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (NULL == pCookie)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    *pCookie = m_sslConnectTable[index].pSSLSock->cookie;
    status = OK;

exit:
    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Store custom information for a connection instance.
This function stores information about the context connection. Your application
should not call this function until after calling SSL_connect.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param cookie               Custom information (cookie data) to store.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

\example
mySessionInfo *mySession = malloc(sizeof(mySessionInfo));

SSL_setCookie(connectionInstance, (int)(&mySession));
\endexample
*/
extern sbyte4
SSL_setCookie(sbyte4 connectionInstance, sbyte8 cookie)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    m_sslConnectTable[index].pSSLSock->cookie = cookie;
    status = OK;
exit:
    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Get a pointer to current context's configuration settings.
This function returns a pointer to NanoSSL %client/server settings that
can be dynamically adjusted during initialization or runtime.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\return Pointer to NanoSSL %client/server settings that can be
dynamically adjusted during initialization or runtime.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sslSettings *
SSL_sslSettings(void)
{
    return &m_sslSettings;
}


/*------------------------------------------------------------------*/
/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern sbyte4
SSL_getSessionInfo(sbyte4 connectionInstance, ubyte* sessionIdLen,
                         ubyte sessionId[SSL_MAXSESSIONIDSIZE],
                         ubyte masterSecret[SSL_MASTERSECRETSIZE])
{
    sbyte4      index;
    MSTATUS     status = ERR_SSL_BAD_ID;
    SSLSocket*  pSSLSock = NULL;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    pSSLSock = m_sslConnectTable[index].pSSLSock;

    if ( pSSLSock)
    {
        ubyte* pInstanceSessionId = NULL;

        *sessionIdLen = 0;
        if (pSSLSock->server)
        {
#ifdef __ENABLE_MOCANA_SSL_SERVER__
            *sessionIdLen = sizeof(SESSIONID);
            pInstanceSessionId = (ubyte*)&pSSLSock->roleSpecificInfo.server.sessionId;
#endif
        }
        else
        {
#ifdef __ENABLE_MOCANA_SSL_CLIENT__
            *sessionIdLen = pSSLSock->roleSpecificInfo.client.sessionIdLen;
            pInstanceSessionId = pSSLSock->roleSpecificInfo.client.sessionId;
#endif
        }

        MOC_MEMCPY( sessionId,
            pInstanceSessionId,
            *sessionIdLen);
        MOC_MEMCPY( masterSecret,
            pSSLSock->pSecretAndRand,
            SSL_MASTERSECRETSIZE);
        status = OK;
    }

exit:
    return status;

} /* SSL_getSessionInfo */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
/* Get connection instance's identifying information.
This function retrieves identifying information for the connection instance's
context. This information can be saved for SSL session reuse, allowing
subsequent connections to be made much more quickly than the initial connection.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect
\param sessionIdLen         Pointer to number of bytes in $sessionId$.
\param sessionId            Buffer for returned session ID.
\param masterSecret         Buffer for returned master secret.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients.
*/
extern sbyte4
SSL_getClientSessionInfo(sbyte4 connectionInstance, ubyte* sessionIdLen,
                         ubyte sessionId[SSL_MAXSESSIONIDSIZE],
                         ubyte masterSecret[SSL_MASTERSECRETSIZE])
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (m_sslConnectTable[index].isClient == TRUE)
    {
        SSLSocket* pSSLSock = m_sslConnectTable[index].pSSLSock;
        if ( pSSLSock)
        {
            *sessionIdLen = pSSLSock->roleSpecificInfo.client.sessionIdLen;
            MOC_MEMCPY( sessionId,
                pSSLSock->roleSpecificInfo.client.sessionId,
                *sessionIdLen);
            MOC_MEMCPY( masterSecret,
                pSSLSock->pSecretAndRand,
                SSL_MASTERSECRETSIZE);
            status = OK;
        }
    }

exit:
    return status;

} /* SSL_getClientSessionInfo */
#endif /* __ENABLE_MOCANA_SSL_CLIENT__ */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__) && \
    defined(__ENABLE_MOCANA_SSL_CLIENT__)
/* Specify a list of DNS names acceptable to the %client.
This function specifies a list of DNS names that when matched to the certificate
subject name will enable a connection.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param cnMatchInfos         Pointer to CNMatchInfo structure (defined in
ca_mgmt.h) containing acceptable DNS names. The $flags$ field is a bit
combination of $matchFlag$ enumerations (see ca_mgmt.h). The length of the array
is indicated by setting the $name$ field of the array's final element to $NULL$.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\example
MatchInfo myMatchInfo[] = {  { 0, "yael.AMT.com"}, {1, ".intel.com"}, {0, NULL} };
SSL_setDNSNames( myConnection, myMatchInfo);
\endexample

*/
extern sbyte4
SSL_setDNSNames( sbyte4 connectionInstance, const CNMatchInfo* cnMatchInfos)
{
    sbyte4  index;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (m_sslConnectTable[index].isClient == TRUE)
    {
        SSLSocket* pSSLSock = m_sslConnectTable[index].pSSLSock;
        if ( pSSLSock)
        {
            pSSLSock->roleSpecificInfo.client.pCNMatchInfos = cnMatchInfos;
            return OK;
        }
    }

exit:
    return ERR_SSL_BAD_ID;

} /* SSL_setDNSNames */

#endif  /* defined(__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__) && \
    defined(__ENABLE_MOCANA_SSL_CLIENT__) */


/*----------------------------------------------------------------------------*/

static sbyte4
SSL_initAux(sbyte4 numServerConnections, sbyte4 numClientConnections, AsymmetricKey* pKey)
{
    sbyte4  sslMaxConnections = 0;
    sbyte4  index;
    MSTATUS status = OK;

    if (0 > numServerConnections)
        numServerConnections = 0;

    if (0 > numClientConnections)
        numClientConnections = 0;

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    sslMaxConnections += numServerConnections;
#else
    numServerConnections = 0;
#endif

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
    sslMaxConnections += numClientConnections;
#else
    numClientConnections = 0;
#endif

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    MOC_MEMSET((ubyte *)&gSslSessionCacheMutex, 0x00, sizeof(RTOS_MUTEX));
    if(NULL == pKey)
    {
        MOC_MEMSET((ubyte *)m_pPrivateKey, 0x00, sizeof(AsymmetricKey));
    }
    else
    {
        m_pPrivateKey = pKey;
    }
#endif
    MOC_MEMSET((ubyte *)&m_sslConnectTableMutex, 0x00, sizeof(RTOS_MUTEX));

#ifndef __DISABLE_MOCANA_INIT__
    gMocanaAppsRunning++;
#endif

    if (0 == sslMaxConnections)
    {
        status = ERR_SSL_CONFIG;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
    if (SSL_MAX_NUM_CIPHERS < (m_sslNumCiphersEnabled = SSL_SOCK_numCiphersAvailable()))
    {
        status = ERR_SSL_CONFIG;
        goto exit;
    }
#endif

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    if (OK > (status = RTOS_mutexCreate(&gSslSessionCacheMutex, SSL_CACHE_MUTEX, 0)))
        goto exit;
#endif

#ifdef __ENABLE_MOCANA_SSL_SERVER__
    if (OK > (status = SSL_SOCK_initServerEngine(mSSL_rngFun, mSSL_rngArg)))
        goto exit;
#endif

    if (OK > (status = RTOS_mutexCreate(&m_sslConnectTableMutex, SSL_CACHE_MUTEX, 1)))
        goto exit;

    if (NULL == m_sslConnectTable)
    {
        ubyte4 remain;
        ubyte4 count = 0;
        m_sslMaxConnections = sslMaxConnections;

        if (NULL == (m_sslConnectTable = MALLOC(sslMaxConnections * sizeof(sslConnectDescr))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
        /* create hashtable for quick access from TCP_SOCKET or peerDescr to sslConnectDescr */
        /* find out the hashtable size */
        remain = sslMaxConnections;
        while (remain > 0)
        {
            remain = remain >> 1;
            count++;
        }
        if (OK > (status = HASH_TABLE_createPtrsTable(&m_sslConnectHashTable, (1 << count) - 1, NULL, allocHashPtrElement, freeHashPtrElement)))
            goto exit;
    }
    else
    {
        if (m_sslMaxConnections < sslMaxConnections)
        {
            status = ERR_SSL_CONFIG;
            goto exit;
        }
    }

    MOC_MEMSET((ubyte *)m_sslConnectTable, 0x00, (usize)(sslMaxConnections * sizeof(sslConnectDescr)));
    MOC_MEMSET((ubyte *)&m_sslSettings, 0x00, (usize)sizeof(sslSettings));

#if defined(__ENABLE_MOCANA_SSL_SERVER__)
    m_sslSettings.sslListenPort     = SSL_DEFAULT_TCPIP_PORT;
#ifdef __ENABLE_TLSEXT_RFC6066__
    m_sslSettings.pCachedOcspResponse   = NULL;
    m_sslSettings.cachedOcspResponseLen = 0;
#endif
#endif
    m_sslSettings.sslTimeOutReceive = TIMEOUT_SSL_RECV;
    m_sslSettings.sslTimeOutHello   = TIMEOUT_SSL_HELLO;

    for (index = 0; index < m_sslMaxConnections; index++)
        m_sslConnectTable[index].connectionState = CONNECT_DISABLED;

    for (index = 0; index < m_sslMaxConnections; index++)
    {
        if (index < numServerConnections)
        {
            m_sslConnectTable[index].connectionState = CONNECT_CLOSED;
        }
        else
        {
            m_sslConnectTable[index].connectionState = CONNECT_CLOSED;
            m_sslConnectTable[index].isClient        = TRUE;
        }

        m_sslConnectTable[index].pSSLSock        = NULL;
    }

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__))
    m_sslSettings.sslTimeOutConnectTimedWait = TIMEOUT_DTLS_CONNECT_TIMED_WAIT;

    /* if numServerConnections is small we allocate 200 entries for the timedWaitQueue, else twice as many as numServerConnections */
    if (OK > (status = CIRCQ_init(&m_sslConnectTimedWaitQueue, (numServerConnections*2 > 200? numServerConnections*2 : 200))))
        goto exit;
#endif

exit:
    if (OK <= status)
    {
        DEBUG_PRINT(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_init: completed after = (");
        DEBUG_UPTIME(DEBUG_SSL_MESSAGES);
        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, (sbyte*)") milliseconds.");
    }

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

/* Initialize NanoSSL %client or server internal structures.
This function initializes NanoSSL %client/server internal structures. Your
application should call this function before starting the HTTPS and application
servers.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param numServerConnections     Maximum number of SSL/TLS %server connections to
allow. (Each connection requires only a few bytes of memory.)
\param numClientConnections     Maximum number of SSL/TLS %client connections to
allow.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients and servers.
*/
SSL_EXTERN sbyte4
SSL_init(sbyte4 numServerConnections, sbyte4 numClientConnections)
{
    mSSL_rngFun = RANDOM_rngFun;
    mSSL_rngArg = g_pRandomContext;

    return SSL_initAux( numServerConnections, numClientConnections, NULL);
}


/*--------------------------------------------------------------------------*/
#if defined(__ENABLE_MOCANA_SSL_CUSTOM_RNG__)

SSL_EXTERN sbyte4
SSL_initEx(sbyte4 numServerConnections, sbyte4 numClientConnections,
           RNGFun rngFun, void* rngArg)
{
    if ( rngFun)
    {
        mSSL_rngFun = RANDOM_rngFun;
        mSSL_rngArg = g_pRandomContext;

        return SSL_initAux( numServerConnections, numClientConnections, NULL);
    }

    /* NULL rngFun -> default to simpler function */
    return SSL_init( numServerConnections, numClientConnections);
}
#endif

SSL_EXTERN sbyte4
SSL_initWithAsymmetricKey(sbyte4 numServerConnections, sbyte4 numClientConnections, AsymmetricKey* pKey)
{
    mSSL_rngFun = RANDOM_rngFun;
    mSSL_rngArg = g_pRandomContext;

    return SSL_initAux( numServerConnections, numClientConnections, pKey);
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || \
    ((defined(__ENABLE_MOCANA_SSL_SERVER__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__))) || \
    ((defined(__ENABLE_MOCANA_SSL_CLIENT__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))) )
/* Send data to a connected server/client.
This function sends data to a connected server/client. It should not be called
until a secure SSL connection is established between the %client and %server.
A negative return value indicates that an error has occurred. A return value
>= 0 indicates the number of bytes transmitted.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pBuffer              Pointer to buffer containing the data to send.
\param bufferSize           Number of bytes in $pBuffer$.

\return Value >= 0 is the number of bytes transmitted; otherwise a negative
number error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
%status, use the $DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients and servers.

\example
char reply[1024];
sbyte4 status;
status = SSL_send(connectionInstance, reply, strlen(reply));
\endexample
*/
extern sbyte4
SSL_send(sbyte4 connectionInstance, sbyte *pBuffer, sbyte4 bufferSize)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    intBoolean isThrCountUpdate = FALSE;


    if (NULL == pBuffer)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (OK > (status = updateThreadCount(TRUE, m_sslConnectTable[index].pSSLSock)))
        goto exit;
    isThrCountUpdate = TRUE;

    if (CONNECT_OPEN == m_sslConnectTable[index].connectionState)
    {
        if (IS_SSL_ASYNC(m_sslConnectTable[index].pSSLSock))
            goto exit;

        if (OK > (status = sendPendingBytes(m_sslConnectTable[index].pSSLSock, index)))
            goto exit;

        if (NULL != m_sslConnectTable[index].pSSLSock->pOutputBuffer)
        {
            /* if there is data still pending, we will send no new bytes */
            status = (MSTATUS)0;
            goto exit;
        }

        status = (MSTATUS)SSL_SOCK_send(m_sslConnectTable[index].pSSLSock, pBuffer, bufferSize);

#if (defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__))
        /* Rehandshake checks against bytes send */
        if ((OK <= status) && (m_sslSettings.maxByteCountForRehandShake > 0))
        {
            m_sslConnectTable[index].pSSLSock->sslRehandshakeByteSendCount += bufferSize;
            if ((m_sslConnectTable[index].pSSLSock->sslRehandshakeByteSendCount > m_sslSettings.maxByteCountForRehandShake) && (m_sslSettings.funcPtrClientRehandshakeRequest != NULL))
            {
                status = m_sslSettings.funcPtrClientRehandshakeRequest(connectionInstance);
                m_sslConnectTable[index].pSSLSock->sslRehandshakeByteSendCount = 0;
                RTOS_deltaMS(NULL, &m_sslConnectTable[index].pSSLSock->sslRehandshakeTimerCount);
            }
        }

#endif
    }

exit:
    if (TRUE == isThrCountUpdate)    
    {
        if (OK > updateThreadCount(FALSE, m_sslConnectTable[index].pSSLSock))
            status = ERR_SSL_CONNECTION_BUSY;
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte *)"SSL_send() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || \
    ((defined(__ENABLE_MOCANA_SSL_SERVER__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__))) || \
    ((defined(__ENABLE_MOCANA_SSL_CLIENT__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))) )
/* Get data from a connected server/client.
This function retrieves data from a connected server/client. It should not be
called until an SSL connection is established between the %client and %server.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pRetBuffer           Pointer to the buffer in which to write the received data.
\param bufferSize           Number of bytes in receive data buffer.
\param pNumBytesReceived    On return, pointer to the number of bytes received.
\param timeout              Number of milliseconds the client/server will wait
to receive the message. To specify no timeout (an infinite wait), set this
parameter to 0.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients and servers.

\example
static int GetSecurePageAux(int connectionInstance, const char* pageName)
{
    char            buffer[1025];
    unsigned int    bytesSent;
    int             result = 0;

    sprintf(buffer, "GET /%s HTTP/1.0\r\n\r\n", pageName);
    bytesSent = SSL_send(connectionInstance,
                         buffer, strlen(buffer));
    if (bytesSent == strlen(buffer)) {
        int bytesReceived;

        // how to receive
        while (0 <= result) {
            memset(buffer, 0x00, 1025);
            result = SSL_recv(connectionInstance,
                              buffer, 1024, &bytesReceived, 0);
            printf("%s", buffer);
        }
        return 0;
    }

    return -1;
}
\endexample
*/
extern sbyte4
SSL_recv(sbyte4 connectionInstance, sbyte *pRetBuffer, sbyte4 bufferSize, sbyte4 *pNumBytesReceived, ubyte4 timeout)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    intBoolean isThrCountUpdate = FALSE;

    if ((NULL == pRetBuffer) || (NULL == pNumBytesReceived))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (0 >= bufferSize)
    {
        status = ERR_INVALID_ARG;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (OK > (status = updateThreadCount(TRUE, m_sslConnectTable[index].pSSLSock)))
        goto exit;
    isThrCountUpdate = TRUE;

    if (CONNECT_OPEN == m_sslConnectTable[index].connectionState)
    {
        if (IS_SSL_ASYNC(m_sslConnectTable[index].pSSLSock))
            goto exit;

        status = doProtocol(m_sslConnectTable[index].pSSLSock, index, TRUE, timeout, pRetBuffer, bufferSize, pNumBytesReceived);

    }
#ifdef __ENABLE_ALL_DEBUGGING__
    else
    {
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte *)"SSL_recv: bad SSL connection state = ", m_sslConnectTable[index].connectionState);
    }
#endif

exit:
    if (TRUE == isThrCountUpdate)
    {
        if (OK > updateThreadCount(FALSE, m_sslConnectTable[index].pSSLSock))
            status = ERR_SSL_CONNECTION_BUSY;
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte *)"SSL_recv() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || \
    ((defined(__ENABLE_MOCANA_SSL_SERVER__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__))) || \
    ((defined(__ENABLE_MOCANA_SSL_CLIENT__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))) )
/* Determines whether there is data in a connection instance's SSL send buffer.
This function determines whether there is data in a connection instance's SSL
send buffer. If the send buffer is empty, zero (0) is returned through the
$pNumBytesPending$ parameter. If send data is pending, an attempt is made to
send the data, and the subsequent number of bytes remaining to be sent is
returned through the $pNumBytesPending$ parameter. (A function return value of
zero (0) indicates that the send was successful and that no data remains in the
send buffer.)

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pNumBytesPending     On return, the number of bytes remaining in the SSL
send buffer.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients and servers.

*/
extern sbyte4
SSL_sendPending(sbyte4 connectionInstance, sbyte4 *pNumBytesPending)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    intBoolean isThrCountUpdate = FALSE;

    if (NULL == pNumBytesPending)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pNumBytesPending = 0;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (OK > (status = updateThreadCount(TRUE, m_sslConnectTable[index].pSSLSock)))
        goto exit;
    isThrCountUpdate = TRUE;

    if (CONNECT_OPEN == m_sslConnectTable[index].connectionState)
    {
        SSLSocket* pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (IS_SSL_ASYNC(pSSLSock))
            goto exit;

        /* try to push out any pending bytes */
        if (OK > (status = sendPendingBytes(pSSLSock, index)))
            goto exit;

        if (NULL == pSSLSock->pOutputBuffer)
            *pNumBytesPending = 0;
        else
            *pNumBytesPending = pSSLSock->numBytesToSend;

        status = OK;

    }

exit:
    if (TRUE == isThrCountUpdate)
    {
        if (OK > updateThreadCount(FALSE, m_sslConnectTable[index].pSSLSock))
            status = ERR_SSL_CONNECTION_BUSY;
    }
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte *)"SSL_sendPending() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || \
    ((defined(__ENABLE_MOCANA_SSL_SERVER__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__))) || \
    ((defined(__ENABLE_MOCANA_SSL_CLIENT__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))) )
/* determines whether there is data in a connection instance's SSL receive buffer.
This function determines whether there is data in a connection instance's SSL
receive buffer, and returns either $TRUE$ or $FALSE$ accordingly.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pRetBooleanIsPending On return, contains $TRUE$ if there is data to be
received, or $FALSE$ if no data is pending receipt.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients and servers.

*/
extern sbyte4
SSL_recvPending(sbyte4 connectionInstance, sbyte4 *pRetBooleanIsPending)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    intBoolean isThrCountUpdate = FALSE;

    if (NULL == pRetBooleanIsPending)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pRetBooleanIsPending = FALSE;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (OK > (status = updateThreadCount(TRUE, m_sslConnectTable[index].pSSLSock)))
        goto exit;
    isThrCountUpdate = TRUE;

    if (CONNECT_OPEN == m_sslConnectTable[index].connectionState)
    {
        SSLSocket* pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (IS_SSL_ASYNC(pSSLSock))
            goto exit;

        if (kRecordStateReceiveFrameComplete == SSL_SYNC_RECORD_STATE(pSSLSock))
            *pRetBooleanIsPending = (0 == (pSSLSock->recordSize - pSSLSock->offset)) ? FALSE : TRUE;

        if (0 != m_sslConnectTable[index].numBytesRead)
            *pRetBooleanIsPending = TRUE;

        status = OK;

    }

exit:
    if (TRUE == isThrCountUpdate)
    {
        if (OK > updateThreadCount(FALSE, m_sslConnectTable[index].pSSLSock))
            status = ERR_SSL_CONNECTION_BUSY;
    }
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte *)"SSL_recvPending() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

/* Close an SSL session and release resources.
This function closes a synchronous SSL session and releases all the resources that are
managed by the NanoSSL %client/server.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients and servers.
*/
SSL_EXTERN sbyte4
SSL_closeConnection(sbyte4 connectionInstance)
{
    /* for multi-concurrent sessions, a thread should be spawned for this call */
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    intBoolean isMutexTaken = FALSE;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (OK > (status = RTOS_mutexWait(m_sslConnectTableMutex)))
    {
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexWait( ) failed.");
        goto exit;
    }

    if (m_sslConnectTable[index].pSSLSock->blockConnection == FALSE)
    {
        m_sslConnectTable[index].pSSLSock->blockConnection = TRUE;
        if (0 < m_sslConnectTable[index].pSSLSock->threadCount)
        {
            status = ERR_SSL_CONNECTION_BUSY;
        }
    }
    else if (0 < m_sslConnectTable[index].pSSLSock->threadCount)
    {
        status = ERR_SSL_CONNECTION_BUSY;
    }

    if (OK > RTOS_mutexRelease(m_sslConnectTableMutex))
    {
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexRelease( ) failed.");
        status = ERR_SSL_CONNECTION_BUSY;
        goto exit;
    }

    if (OK > status)
        goto exit;

    if (CONNECT_NEGOTIATE <= m_sslConnectTable[index].connectionState)
    {
        intBoolean isDTLS;
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))
        if (IS_SSL_SYNC(m_sslConnectTable[index].pSSLSock))
        {
            FREE(m_sslConnectTable[index].pReadBuffer);
            m_sslConnectTable[index].pReadBuffer = NULL;
        }
#endif
        isDTLS = m_sslConnectTable[index].pSSLSock->isDTLS;

        if (OK > (status = RTOS_mutexWait( m_sslConnectTableMutex)))
        {
            MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexWait() failed.");
            goto exit;
        }
        isMutexTaken = TRUE;

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
        if (isDTLS)
        {
            if (OK > (status = hashtable_remove(m_sslConnectHashTable, isDTLS, (TCP_SOCKET)-1, &(m_sslConnectTable[index].peerDescr))))
                goto exit;
        } else
#endif
        {
            if (OK > (status = hashtable_remove(m_sslConnectHashTable, isDTLS, m_sslConnectTable[index].socket, NULL)))
                goto exit;
        }

        if (OK > (status = RTOS_mutexRelease(m_sslConnectTableMutex)))
        {
            MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexRelease() failed.");
            goto exit;
        }
        isMutexTaken= FALSE;

        SSL_SOCK_uninit(m_sslConnectTable[index].pSSLSock);
        HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_SSL, &(m_sslConnectTable[index].pSSLSock->hwAccelCookie));
        FREE(m_sslConnectTable[index].pSSLSock);

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
        if (m_sslConnectTable[index].peerDescr.pUdpDescr)
        {
#if (defined(__ENABLE_MOCANA_DTLS_SERVER__))
            /* if connection is not yet established, put in timed-wait queue */
            if (isDTLS && m_sslConnectTable[index].connectionState < CONNECT_OPEN)
            {
                sslConnectTimedWaitDescr *pTimedWaitDescr;
                MOC_IP_ADDRESS srcAddrRef = REF_MOC_IPADDR(m_sslConnectTable[index].peerDescr.srcAddr);
                MOC_IP_ADDRESS peerAddrRef = REF_MOC_IPADDR(m_sslConnectTable[index].peerDescr.peerAddr);

                if (NULL == (pTimedWaitDescr = MALLOC(sizeof(sslConnectTimedWaitDescr))))
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }

                pTimedWaitDescr->peerDescr.pUdpDescr = m_sslConnectTable[index].peerDescr.pUdpDescr;
                pTimedWaitDescr->peerDescr.srcPort = m_sslConnectTable[index].peerDescr.srcPort;
                COPY_MOC_IPADDR(pTimedWaitDescr->peerDescr.srcAddr, srcAddrRef);
                pTimedWaitDescr->peerDescr.peerPort = m_sslConnectTable[index].peerDescr.peerPort;
                COPY_MOC_IPADDR(pTimedWaitDescr->peerDescr.peerAddr, peerAddrRef);
                pTimedWaitDescr->startTime = RTOS_getUpTimeInMS();

                status = CIRCQ_enq(m_sslConnectTimedWaitQueue, (ubyte*)pTimedWaitDescr);
                if (OK > status)
                {
                    /* if queue is full, remove the oldest entry in queue and try again */
                    sslConnectTimedWaitDescr *pTimedWaitDescrToDel;
                    CIRCQ_deq(m_sslConnectTimedWaitQueue, (ubyte**)&pTimedWaitDescrToDel);
                    if (pTimedWaitDescrToDel)
                    {
                        FREE(pTimedWaitDescrToDel);
                    }
                    status = CIRCQ_enq(m_sslConnectTimedWaitQueue, (ubyte*)pTimedWaitDescr);
                    /* give up */
                    if (OK > status)
                    {
                        FREE(pTimedWaitDescr);
                        status = OK;
                    }
                }
            } else
#endif
            {
                m_sslConnectTable[index].peerDescr.pUdpDescr = NULL;
            }
        }
#endif
        m_sslConnectTable[index].pSSLSock        = NULL;
        m_sslConnectTable[index].instance        = -1;
        m_sslConnectTable[index].connectionState = CONNECT_CLOSED;

        status = OK;
    }

exit:
    if (isMutexTaken)
    {
        if (OK > RTOS_mutexRelease(m_sslConnectTableMutex))
        {
            MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexRelease() failed.");
        }
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_closeConnection() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
/* Initialize NanoSSL %client or %server internal structures.
This function initializes NanoSSL %client/server internal structures. Your
application should call this function before starting the HTTPS and application
servers.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param numServerConnections     Maximum number of SSL/TLS %server connections to
allow. (Each connection requires only a few bytes of memory.) If operating in
dual mode, this is the sum of the synchronous and asynchronous %server
connections.
\param numClientConnections     Maximum number of SSL/TLS %client connections to
allow. If operating in dual mode, this is the sum of the synchronous and
asynchronous %client connections.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous clients and servers.

*/
extern sbyte4
SSL_ASYNC_init(sbyte4 numServerConnections, sbyte4 numClientConnections)
{
    return SSL_init(numServerConnections, numClientConnections);
}


/*--------------------------------------------------------------------------*/
#if defined(__ENABLE_MOCANA_SSL_CUSTOM_RNG__)

extern sbyte4
SSL_ASYNC_initEx(sbyte4 numServerConnections, sbyte4 numClientConnections,
                 RNGFun rngFun, void* rngArg)
{
    return SSL_initEx(numServerConnections, numClientConnections,
                      rngFun, rngArg);
}
#endif

#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
/* Get a copy of data received from a connected server/client.
This function retrieves data from a connected server/client and copies it into a
new buffer. It should be called from your TCP/IP receive upcall handler, or from
your application after reading a packet of data. The engine decrypts and
processes the packet, and then calls NanoSSL server's upcall function,
$funcPtrReceiveUpcall$, to hand off the decrypted data.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.
\param pBytesReceived       On return, pointer to the packet or message received from the TCP/IP stack.
\param numBytesReceived     On return, number of bytes in $pBytesReceived$.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\note This function is provided for backward compatibility with earlier Embedded
SSL/TLS implementations. New implementations should use SSL_ASYNC_recvMessage2.
The SSL_ASYNC_recvMessage2 function returns a pointer to the full data buffer,
eliminating the need to consider maximum buffer sizes and manage multiple read
calls.

\remark This function is applicable to asynchronous clients and servers.

\example
while ((OK == status) && (TRUE != mBreakServer))
{
    if (OK <= (status = TCP_READ_AVL(socketClient,
                                     pInBuffer,
                                     SSH_SYNC_BUFFER_SIZE,
                                     &numBytesRead,
                                     20000)))
    {
        if (0 != numBytesRead)
            status = SSL_ASYNC_recvMessage(connInstance,
                                           pInBuffer,
                                           numBytesRead);
    }

    if (ERR_TCP_READ_TIMEOUT == status)
        status = OK;
}
\endexample
*/
extern sbyte4
SSL_ASYNC_recvMessage(sbyte4 connectionInstance, ubyte *pBytesReceived, ubyte4 numBytesReceived)
{
    sbyte4  index;
    sbyte4  dummy;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (IS_SSL_SYNC(m_sslConnectTable[index].pSSLSock))
        goto exit;

    status = OK;

    if ((CONNECT_NEGOTIATE < m_sslConnectTable[index].connectionState) && (OK <= status))
        status = setMessageTimer(m_sslConnectTable[index].pSSLSock, connectionInstance, m_sslSettings.sslTimeOutReceive);

    while ((OK <= status) && (0 < numBytesReceived))
    {
        status = (MSTATUS)SSL_SOCK_receive(m_sslConnectTable[index].pSSLSock, NULL, 0,
                                            &pBytesReceived, &numBytesReceived, &dummy);
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_ASYNC_recvMessage() returns status = ", (sbyte4)status);
#endif

    return status;
}
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
#if defined(__ENABLE_MOCANA_SSL_ASYNC_API_EXTENSIONS__)
/* Get a pointer to the connection's most recently receiveed message.
This function returns a pointer (through the $pBytesReceived$ parameter) to the
specified connection's most recently received message. Typically, you'll call
this function and then, if the returned number of bytes of application data is
greater than 0, call SSL_ASYNC_getRecvBuffer to get the pointer to the
decrypted data.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_API_EXTENSIONS__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance       Connection instance returned from SSL_ASYNC_connect.
\param pBytesReceived           On return, pointer to the packet or message received from the TCP/IP stack.
\param numBytesReceived         On return, number of bytes in $pBytesReceived$.
\param ppRetBytesReceived       On return, pointer to buffer containing number of bytes remaining to be read.
\param pRetNumRxBytesRemaining  On return, pointer to number of bytes in $ppRetBytesReceived$.

\return Value >= 0 is the number of bytes of application data available when
the $SSL_FLAG_ENABLE_RECV_BUFFER$ is set; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error %status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous clients and servers.

*/
extern sbyte4
SSL_ASYNC_recvMessage2(sbyte4 connectionInstance, ubyte *pBytesReceived, ubyte4 numBytesReceived,
                       ubyte **ppRetBytesReceived, ubyte4 *pRetNumRxBytesRemaining)
{
    sbyte4  index;
    sbyte4  retNumBytesReceived = 0;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (IS_SSL_SYNC(m_sslConnectTable[index].pSSLSock))
        return (sbyte4)status;

    status = OK;

    if ((CONNECT_NEGOTIATE < m_sslConnectTable[index].connectionState) && (OK <= status))
        status = setMessageTimer(m_sslConnectTable[index].pSSLSock, connectionInstance, m_sslSettings.sslTimeOutReceive);

    if (OK > status)
        return (sbyte4)status;

    /* in most instances, all of the data will be consumed */
    *ppRetBytesReceived      = NULL;
    *pRetNumRxBytesRemaining = 0;

    while (0 < numBytesReceived)
    {
        status = (MSTATUS)SSL_SOCK_receive(m_sslConnectTable[index].pSSLSock, NULL, 0,
                                            &pBytesReceived, &numBytesReceived, &retNumBytesReceived);

        if (OK > status)
            return (sbyte4)status;

        if ((SSL_FLAG_ENABLE_RECV_BUFFER & m_sslConnectTable[index].pSSLSock->runtimeFlags) &&
            (0 < retNumBytesReceived))
        {
            /* this conditional prevents a second SSL frame from overwriting a first frame */
            *ppRetBytesReceived    = pBytesReceived;
            *pRetNumRxBytesRemaining = numBytesReceived;
            break;
        }
    }

exit:
    return retNumBytesReceived;

} /* SSL_ASYNC_recvMessage2 */
#endif
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
/* Send data to a connected server/client.
This function sends data to a connected server/client. It should not be called
until a secure SSL connection is established between the %client and %server.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.
\param pBuffer              Pointer to buffer containing the data to send.
\param bufferSize           Number of bytes in $pBuffer$.
\param pBytesSent           On return, pointer to number of bytes successfully sent.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\note This function should not be called until after a $funcPtrOpenStateUpcall$
upcall event.
\remark This function is applicable to asynchronous clients and servers.

\example
static void SSL_EXAMPLE_helloWorld(int connectionInstance)
{
    sbyte4 bytesSent = 0;
    sbyte4 status;

    status = SSL_ASYNC_sendMessage(connInstance,
                                   "hello world!", 12,
                                   &bytesSent);
}
\endexample
*/
extern sbyte4
SSL_ASYNC_sendMessage(sbyte4 connectionInstance, sbyte *pBuffer, sbyte4 bufferSize, sbyte4 *pBytesSent)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (NULL == pBuffer)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_OPEN == m_sslConnectTable[index].connectionState)
    {
        SSLSocket* pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (IS_SSL_SYNC(pSSLSock))
            goto exit;

        if (NULL != pBytesSent)
            *pBytesSent = 0;

        /* for DTLS, the send buffer should be cleared already */
        if (!pSSLSock->isDTLS)
        {
            /* send out any bytes that may be pending, before attempting to send "new" data */
            if (OK > (status = sendPendingBytes(pSSLSock, index)))
                goto exit;
        }

        if (NULL != pSSLSock->pOutputBuffer)
        {
            if (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)
            {
                /* Should Not Happen as in this case we shall be harvesting all */
                /* the data from the buffer */
                if (NULL != pBytesSent)
                {
                    *pBytesSent =  pSSLSock->numBytesToSend;
                }

                /* Should retry the buffer again */
                status = ERR_SSL_EAP_DATA_SEND;
                goto exit;
            }
        }

        if ((NULL == pSSLSock->pOutputBuffer) && (NULL != pSSLSock->buffers[0].pHeader))
        {
            if (OK > (status = SSLSOCK_sendEncryptedHandshakeBuffer(pSSLSock)))
                goto exit;
        }

        if (NULL == pSSLSock->pOutputBuffer)
        {
            status = (MSTATUS)SSL_SOCK_send(pSSLSock, pBuffer, bufferSize);

            if (OK > status)
                goto exit;

            if (NULL != pBytesSent)
                *pBytesSent = status;
        }

        /* return the number of bytes pending in the ssl send buffer */
        if (NULL == pSSLSock->pOutputBuffer)
            status = 0;
        else
            status = (MSTATUS)pSSLSock->numBytesToSend;

    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_ASYNC_sendMessage() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
/* Determines whether there is data in a connection instance's SSL send buffer.
This function determines whether there is data in a connection instance's SSL
send buffer. If the send buffer is empty, the function returns zero (0) as its
status. If send data is pending, an attempt is made to send the data, and the
subsequent number of bytes remaining to be sent is returned as the function
status. (A function return value of zero (0) indicates that the send was
successful and that no data remains in the send buffer.)

\since 1.41
\version 3.06 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.

\return $OK$ (0) if the send buffer is empty or if this function successfully
sent all remaining buffer data; otherwise the number of bytes remaining to be
sent.

\remark This function is applicable to asynchronous clients and servers.
*/
extern sbyte4
SSL_ASYNC_sendMessagePending(sbyte4 connectionInstance)
{
    /* on success, returns the number of bytes still pending */
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        if (IS_SSL_SYNC(m_sslConnectTable[index].pSSLSock))
            goto exit;

        status = sendPendingBytes(m_sslConnectTable[index].pSSLSock, index);

        if (OK > status)
            goto exit;

        status = (MSTATUS)m_sslConnectTable[index].pSSLSock->numBytesToSend;
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_ASYNC_sendMessage() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif

/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
/* Close an SSL session and release resources.
This function closes an asynchronous SSL session and releases all the resources that are
managed by the NanoSSL %client/server.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\note This function does not close sockets or TCBs (transmission control
blocks). Your integration code should explicitly close all TCP/IP sockets and
TCBs.

\remark This function is applicable to asynchronous clients and servers.

*/
extern sbyte4
SSL_ASYNC_closeConnection(sbyte4 connectionInstance)
{
    return SSL_closeConnection(connectionInstance);
}
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_SERVER__)
/* Register a certificate (or certificate chain) and key.
This function registers a certificate (or certificate chain) and key for the
server to use to identify itself to a %client. Your application should call this
function before starting the HTTPS and application servers.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param pCertificateDescr    Pointer to the X.509 certificate(s) and key blob(s) to register.
\param isChain              $TRUE$ if $pCertificateDescr$ points to a chain of
certificates; $FALSE$ if $pCertificateDescr$ points to a single certificate.
\param ecCurves             Bit field specifying which curves are supported by
the peer (%client). Supported curves are any of the $tlsExtNamedCurves$
enumerated values (see ssl.h). To build the bit field, shift left a single bit
(the value 1) by the value of the desired $tlsExtNamedCurves$ enumeration.
Multiple shift-left results can be OR-ed together.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern sbyte4
SSL_initServerCert(certDescriptor *pCertificateDescr, intBoolean isChain,
                    ubyte4 ecCurves)
{
    if (isChain)
        return ERR_SSL_OLD_CERT_CHAINS_NOT_SUPPORTED;

    return SSL_initServerCertEx(pCertificateDescr, 1, ecCurves);
}

extern sbyte4
SSL_initServerCertEx(certDescriptor *pCertificateDescr, sbyte4 numCerts,
                    ubyte4 ecCurves)
{
    MSTATUS status = ERR_NULL_POINTER;
    sbyte4 i;

    m_certificateECCurves = ecCurves;

    if ((numCerts <= 0) || (NULL == pCertificateDescr) || (NULL == pCertificateDescr->pCertificate))
        goto exit;

    if(pCertificateDescr->pKey)
    {
        m_pPrivateKey = pCertificateDescr->pKey;
        goto got_key;
    }

    if (NULL == pCertificateDescr->pKeyBlob)
        goto exit;

    if (0x4000 < pCertificateDescr->certLength)
    {
        status = ERR_SSL_INVALID_CERT_LENGTH;
        goto exit;
    }

    if (isDefaultKey())
    {
        if ( OK > (status = CA_MGMT_extractKeyBlobEx(
                                                pCertificateDescr->pKeyBlob,
                                                pCertificateDescr->keyBlobLength,
                                                m_pPrivateKey)))
        {
            goto exit;
        }
    }

got_key:

    /* XXX: Should we check to see if this has already been allocated? */
    m_certificates = MALLOC(numCerts * sizeof(SizedBuffer));
    if (NULL == m_certificates)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET((ubyte *)m_certificates, 0x00, numCerts * sizeof(SizedBuffer));

    for (i = 0; i < numCerts; i++)
    {
        if (0 > (status = SB_Allocate(&(m_certificates[i]), (ubyte2)pCertificateDescr[i].certLength)))
	    goto exit;

	MOC_MEMCPY(m_certificates[i].data, pCertificateDescr[i].pCertificate, pCertificateDescr[i].certLength);
    }

    m_numCerts = numCerts;

exit:
    return status;
}
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_SERVER__)
/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern MSTATUS
SSL_setServerCert(SSLSocket* pSSLSock)
{
    /* an internal API, not to be published */
    return SSL_SOCK_setServerCert(pSSLSock, m_numCerts, m_certificates,
				  m_pPrivateKey, m_certificateECCurves);
}
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_SSL_DYNAMIC_CERTIFICATE__) || (defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_CLIENT__))
/* Associate certificate/key pairs with a connection.
This function associates one or more certificate/key pairs with a specific
connection.

The function must not be called before a connection is established (see
SSL_connect for synchronous clients, SSL_ASYNC_connect for asynchronous clients,
SSL_acceptConnection for synchronous server, and SSL_ASYNC_acceptConnection for
asynchronous server), but must be called before SSL_negotiateConnection
(for synchronous clients or servers).

\since 1.41
\version 5.1 and later

! Flags
To enable this function, one of the following flags must be defined in moptions.h:
- $__ENABLE_SSL_DYNAMIC_CERTIFICATE__$
- $__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

For an example of how to call this function, refer to ssl_example.c in the
sample code (examples directory).

\param connectionInstance   Connection instance returned from SSL_acceptConnection or SSL_connect.
\param pCertStore           Pointer to certificates.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error %status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sbyte4
SSL_assignCertificateStore(sbyte4 connectionInstance, certStorePtr pCertStore)
{
    sbyte4      index;
    MSTATUS     status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (NULL != pSSLSock)
        {
            pSSLSock->pCertStore = pCertStore;
            status = OK;
        }
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_assignCertificateStore() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;

} /* SSL_assignCertificateStore */
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_SERVER__
/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern MSTATUS
SSL_acceptConnectionCommon(intBoolean isDTLS, sbyte4 tempSocket, peerDescr *pPeerDescr, ubyte4 initialInternalFlag)
{
    SSLSocket*  pSSLSock = NULL;
    TCP_SOCKET  socket   = tempSocket;
    sbyte4      instance = -1;
    sbyte4      index;
    intBoolean  isHwAccelInit = FALSE;
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
    ubyte*      pReadBuffer = NULL;
#endif
    intBoolean  isMutexTaken = FALSE;
    MSTATUS     status;

    if (OK > (status = RTOS_mutexWait( m_sslConnectTableMutex)))
    {
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexWait() failed.");
        goto exit;
    }
    isMutexTaken = TRUE;

    for (index = 0; index < m_sslMaxConnections; index++)
    {
        if ((CONNECT_CLOSED == m_sslConnectTable[index].connectionState) &&
            (FALSE == m_sslConnectTable[index].isClient))
        {
            m_sslConnectTable[index].connectionState = CONNECT_NEGOTIATE;
            m_sslConnectTable[index].age = ((m_sslConnectTable[index].age + 1) & 0x7fff);
            instance = ((sbyte4)(m_sslConnectTable[index].age << NUM_BITS_SSL_SESSION_AGE) | index);
            if (OK > (status = hashtable_insert(m_sslConnectHashTable, isDTLS, socket, pPeerDescr, &m_sslConnectTable[index])))
                goto exit;

            break;
        }
    }

    if (OK > (status = RTOS_mutexRelease(m_sslConnectTableMutex)))
    {
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexRelease() failed.");
        goto exit;
    }

    isMutexTaken = FALSE;

    if (!(index < m_sslMaxConnections))
    {
        status = ERR_SSL_TOO_MANY_CONNECTIONS;
    }
    else
    {
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
        if (initialInternalFlag & SSL_INT_FLAG_SYNC_MODE)
        {
            m_sslConnectTable[index].numBytesRead = 0;

            if (NULL == (pReadBuffer = MALLOC(SSL_SYNC_BUFFER_SIZE)))
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }
        }
#endif /* defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) */

        if (NULL == (pSSLSock = MALLOC(sizeof(SSLSocket))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMSET((ubyte *)pSSLSock, 0x00, sizeof(SSLSocket));

        HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_SSL, &(pSSLSock->hwAccelCookie));

        isHwAccelInit = TRUE;

        pSSLSock->internalFlags = initialInternalFlag;

        /* setMessageTimer() requires that pSSLSock->server be set to the right value
         * This is a bug fix.
         */
        pSSLSock->server = 1;

        setMessageTimer(pSSLSock, instance, m_sslSettings.sslTimeOutHello);

        if ((OK <= SSL_SOCK_init(pSSLSock, isDTLS, socket, pPeerDescr, mSSL_rngFun, g_pRandomContext)) &&
            (OK <= SSL_SOCK_initSocketExtraServer(pSSLSock)))
        {
            m_sslConnectTable[index].instance        = instance ;
            m_sslConnectTable[index].pSSLSock        = pSSLSock;
            m_sslConnectTable[index].pSSLSock->certParamCheck.chkCommonName = TRUE;
            m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityBefore = TRUE;
            m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityAfter = TRUE;
            m_sslConnectTable[index].pSSLSock->certParamCheck.chkKnownCA = TRUE;

#if defined(__ENABLE_MOCANA_DTLS_SERVER__)
            if (isDTLS)
            {
                MOC_IP_ADDRESS srcAddrRef = REF_MOC_IPADDR(pPeerDescr->srcAddr);
                MOC_IP_ADDRESS peerAddrRef = REF_MOC_IPADDR(pPeerDescr->peerAddr);

                m_sslConnectTable[index].peerDescr.pUdpDescr       = pPeerDescr->pUdpDescr;
                m_sslConnectTable[index].peerDescr.srcPort      = pPeerDescr->srcPort;
                COPY_MOC_IPADDR(m_sslConnectTable[index].peerDescr.srcAddr, srcAddrRef);
                m_sslConnectTable[index].peerDescr.peerPort      = pPeerDescr->peerPort;
                COPY_MOC_IPADDR(m_sslConnectTable[index].peerDescr.peerAddr, peerAddrRef);

                if (OK > DTLS_setSessionFlags(instance, SSL_FLAG_ENABLE_SEND_BUFFER | SSL_FLAG_ENABLE_RECV_BUFFER))
                    goto exit;
            } else
#endif
            {
                m_sslConnectTable[index].socket          = socket;
            }

            status                                   = (MSTATUS)instance;

            pSSLSock                                 = NULL;
#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
            m_sslConnectTable[index].pReadBuffer     = pReadBuffer;
            pReadBuffer                              = NULL;
#endif

#ifdef __PSOS_RTOS__
            t_ident((char *)0, 0, &m_sslConnectTable[index].tid);
#endif
        }
        else
        {
            SSL_SOCK_uninit(pSSLSock);
            m_sslConnectTable[index].connectionState = CONNECT_CLOSED;

            status = ERR_SSL_INIT_CONNECTION;
        }
    }

    if (OK <= status)
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"SSL server accept connection.");

exit:
    if (isMutexTaken)
    {
        if (OK > RTOS_mutexRelease(m_sslConnectTableMutex))
        {
            MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexRelease() failed.");
        }
    }

#if defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
    if ((initialInternalFlag & SSL_INT_FLAG_SYNC_MODE) && (NULL != pReadBuffer))
        FREE(pReadBuffer);
#endif /* defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) */

    if (pSSLSock)
    {
        if (TRUE == isHwAccelInit)
        {
            HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_SSL, &pSSLSock->hwAccelCookie);
        }

        FREE(pSSLSock);
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_acceptConnectionCommon() returns status = ", (sbyte4)status);
#endif

    return status;

} /* SSL_acceptConnectionCommon */
#endif


/*------------------------------------------------------------------*/

#if ((defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) && defined(__ENABLE_MOCANA_SSL_SERVER__)) || \
     (defined(__ENABLE_MOCANA_SSL_SERVER__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)))
/* Create a synchronous server connection context.
This function performs SSL handshaking, establishing a secure connection between a %server and %client.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param tempSocket       Socket or TCB identifier returned by a call to $accept()$.

\return Value >= 0 is the connection instance; otherwise a negative
number error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
%status, use the $DISPLAY_ERROR$ macro.

\note This function must be called from within the HTTPS daemon context. If you
are using multiple HTTPS daemons,  you must use a semaphore (mutex) around
this function call.
\note If your web %server and application %server run as separate tasks, you
should protect the call to SSL_acceptConnection with a semaphore to prevent
race conditions.

\remark This function is applicable to synchronous servers only.
*/
extern sbyte4
SSL_acceptConnection(sbyte4 tempSocket)
{
    return (sbyte4)SSL_acceptConnectionCommon(FALSE, tempSocket, NULL, SSL_INT_FLAG_SYNC_MODE);
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) || \
    ((defined(__ENABLE_MOCANA_SSL_SERVER__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__))) || \
    ((defined(__ENABLE_MOCANA_SSL_CLIENT__)) && (!defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__))) )
/* Establish a secure SSL client-server connection.
This function performs SSL handshaking, establishing a secure connection between
a %client and %server. Before calling this function, you must first create a
connection context (instance) by calling SSL_connect.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_SERVER__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients and servers.

\example
sbyte4 connectionInstance;
int mySocket;

// connect to server
connect(mySocket, (struct sockaddr *)&server, sizeof(server))

// register connect, get connectionInstance
connectionInstance = SSL_connect(mySocket, 0, NULL, NULL, "mocana.com");

// set a cookie
SSL_setCookie(connectionInstance, (int)&someFutureContext);

// negotiate SSL secure connection
if (0 > SSL_negotiateConnection(connectionInstance))
    goto error;
\endexample
*/
extern sbyte4
SSL_negotiateConnection(sbyte4 connectionInstance)
{
    /* a mutex is not necessary, this function should be called after accept */
    /* within the ssl connection daemon */
    sbyte4      index;
    sbyte4      dummy;
    MSTATUS     status = ERR_SSL_BAD_ID;
    intBoolean isThrCountUpdate = FALSE;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (OK > (status = updateThreadCount(TRUE, m_sslConnectTable[index].pSSLSock)))
        goto exit;
    isThrCountUpdate = TRUE;

    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        if (IS_SSL_ASYNC(m_sslConnectTable[index].pSSLSock))
            goto exit;

#ifdef __ENABLE_MOCANA_SSL_CLIENT__
        if (m_sslConnectTable[index].isClient)
        {
            if (OK > (status = SSL_SOCK_clientHandshake(m_sslConnectTable[index].pSSLSock, FALSE)))
                goto exit;
        }
#endif

        status = doProtocol(m_sslConnectTable[index].pSSLSock, index, FALSE, 0, NULL, 0, &dummy);
    }

#ifdef __ENABLE_HARDWARE_ACCEL_CRYPTO__
    if (index < m_sslMaxConnections)
    {
        MD5FreeCtx_HandShake(MOC_HASH(m_sslConnectTable[index].pSSLSock->hwAccelCookie) &m_sslConnectTable[index].pSSLSock->md5Ctx);
        SHA1_FreeCtxHandShake(MOC_HASH(m_sslConnectTable[index].pSSLSock->hwAccelCookie) &m_sslConnectTable[index].pSSLSock->shaCtx);
    }
#endif

    if (0 <= status)
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"SSL server negotiated connection.");

#if (defined( __ENABLE_MOCANA_SSL_REHANDSHAKE__))
    if (OK <= status)
         RTOS_deltaMS(NULL, &m_sslConnectTable[index].pSSLSock->sslRehandshakeTimerCount);
#endif

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte *)"SSL_negotiateConnection() returns status = ", (sbyte4)status);
#endif
    if (TRUE == isThrCountUpdate)
    {
        if (OK > updateThreadCount(FALSE, m_sslConnectTable[index].pSSLSock))
            status = ERR_SSL_CONNECTION_BUSY;
    }
    return (sbyte4)status;

} /* SSL_negotiateConnection */
#endif

/*------------------------------------------------------------------*/

#if ((defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_CLIENT__)) && !defined (__DISABLE_MOCANA_CERTIFICATE_PARSING__))
MOC_EXTERN sbyte4
SSL_validateCertParam(sbyte4 connectionInstance,
                      certDescriptor* pCertChain, sbyte4 numCertsInChain,
                      certParamCheckConf *pCertParamCheckConf,
                      certDescriptor* pCACert, errorArray* pStatusArray)

{
    sbyte4              status = ERR_SSL_BAD_ID;
    sbyte4              index = 0;

    if (pStatusArray)
        MOC_MEMSET((ubyte *)pStatusArray, 0x00, sizeof(errorArray));

    if (( 0 >= numCertsInChain) || !(pCertChain))
    {
        status = ERR_INVALID_ARG;

        if (pStatusArray)
        {
            pStatusArray->errors[pStatusArray->numErrors] = status;
            pStatusArray->numErrors ++;
        }

        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
    {
        if (pStatusArray)
        {
            pStatusArray->errors[pStatusArray->numErrors] = status;
            pStatusArray->numErrors ++;
        }

        goto exit;
    }

    if (OK > (status = SSL_SOCK_validateCert(m_sslConnectTable[index].pSSLSock,
                                             pCertChain, numCertsInChain,
                                             pCertParamCheckConf, pCACert, pStatusArray)))
         goto exit;

exit:
        if (pStatusArray && (pStatusArray->numErrors > 0))
            status = pStatusArray->errors[0];

        return status;
}
#endif

/*------------------------------------------------------------------*/

#if ((defined(__ENABLE_MOCANA_SSL_DUAL_MODE_API__) && defined(__ENABLE_MOCANA_SSL_CLIENT__)) || \
     (defined(__ENABLE_MOCANA_SSL_CLIENT__) && !defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)))
/* Create a synchronous %client connection context.
This function creates a connection context for a secure SSL/TLS synchronous
connection with a remote %server.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param tempSocket       Socket or TCB identifier returned by a call to $connect()$.
\param sessionIdLen     Number of bytes in $sessionId$, excluding the $NULL$ terminator.
\param sessionId        Pointer to session ID.
\param masterSecret     Pointer to master secret for the session.
\param dnsName          Pointer to expected DNS name of the server's certificate.

\return Value >= 0 is the connection instance; otherwise a negative
number error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
%status, use the $DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous clients only.
*/
static sbyte4
SSL_connectAux(sbyte4 tempSocket,
               ubyte sessionIdLen, ubyte* sessionId,
               ubyte* masterSecret, const sbyte* dnsName,
               certParamCheckConf *pCertParamCheck)
{
    /* a mutex is required around the call of this function, if multiple threads may invoke this API */
    sbyte4          index;
    hwAccelDescr    hwAccelCookie;
    intBoolean      isHwAccelCookieInit = FALSE;
    TCP_SOCKET      connectSocket       = (TCP_SOCKET)tempSocket;
    sbyte4          instance;
    MSTATUS         status, status1;

    if ( OK > (status = RTOS_mutexWait(m_sslConnectTableMutex)))
    {
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"RTOS_mutexWait() failed.");
        goto exitNoMutexRelease;
    }

    status = ERR_SSL_TOO_MANY_CONNECTIONS;

    for (index = 0; index < m_sslMaxConnections; index++)
    {
        if ((CONNECT_CLOSED == m_sslConnectTable[index].connectionState) &&
            (TRUE == m_sslConnectTable[index].isClient))
        {
            if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_SSL, &hwAccelCookie)))
                goto exit;

            isHwAccelCookieInit = TRUE;

            m_sslConnectTable[index].connectionState = CONNECT_NEGOTIATE;

            status = ERR_MEM_ALLOC_FAIL;

            m_sslConnectTable[index].numBytesRead = 0;

            if (NULL == (m_sslConnectTable[index].pReadBuffer = MALLOC(SSL_SYNC_BUFFER_SIZE)))
                goto exit;

            if (NULL == (m_sslConnectTable[index].pSSLSock = MALLOC(sizeof(SSLSocket))))
                goto exit;

            MOC_MEMSET((ubyte *)m_sslConnectTable[index].pSSLSock, 0x00, sizeof(SSLSocket));

            m_sslConnectTable[index].pSSLSock->hwAccelCookie = hwAccelCookie;
            m_sslConnectTable[index].pSSLSock->internalFlags = SSL_INT_FLAG_SYNC_MODE;

            m_sslConnectTable[index].age = ((m_sslConnectTable[index].age + 1) & 0x7fff);
            instance = ((sbyte4)(m_sslConnectTable[index].age << NUM_BITS_SSL_SESSION_AGE) | index);
            /* configurable certificate checks */
            if(NULL != pCertParamCheck)
            {
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkCommonName = pCertParamCheck->chkCommonName;
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityBefore = pCertParamCheck->chkValidityBefore;
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityAfter = pCertParamCheck->chkValidityAfter;
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkKnownCA = pCertParamCheck->chkKnownCA;
            }
            else
            {
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkCommonName = TRUE;
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityBefore = TRUE;
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityAfter = TRUE;
                m_sslConnectTable[index].pSSLSock->certParamCheck.chkKnownCA = TRUE;
            }
            if (OK > (status = setMessageTimer(m_sslConnectTable[index].pSSLSock, instance, m_sslSettings.sslTimeOutHello)))
                goto exit;

            if (OK > (status = SSL_SOCK_init(m_sslConnectTable[index].pSSLSock, FALSE, connectSocket, NULL,
                                                mSSL_rngFun, mSSL_rngArg)))
            {
                goto exit;
            }
            if (OK > (status = SSL_SOCK_initSocketExtraClient(m_sslConnectTable[index].pSSLSock, sessionIdLen, sessionId,
                                                              masterSecret, dnsName)))
            {
                SSL_SOCK_uninit(m_sslConnectTable[index].pSSLSock);
                goto exit;
            }

            m_sslConnectTable[index].instance        = instance ;

            if (OK > (status = hashtable_insert(m_sslConnectHashTable, FALSE, connectSocket, NULL, &m_sslConnectTable[index])))
                goto exit;

            m_sslConnectTable[index].socket = connectSocket;
            status = instance;
#ifdef __PSOS_RTOS__
            t_ident((char *)0, 0, &m_sslConnectTable[index].tid);
#endif

exit:
            if (OK > status)
            {
                m_sslConnectTable[index].connectionState = CONNECT_CLOSED;

                if (NULL != m_sslConnectTable[index].pSSLSock)
                {
                    FREE(m_sslConnectTable[index].pSSLSock);
                    m_sslConnectTable[index].pSSLSock = NULL;
                }

                if (NULL != m_sslConnectTable[index].pReadBuffer)
                {
                    FREE(m_sslConnectTable[index].pReadBuffer);
                    m_sslConnectTable[index].pReadBuffer = NULL;
                }

                if (TRUE == isHwAccelCookieInit)
                {
                    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_SSL, &hwAccelCookie);
                }
            }

            break;
        }
    }

    status1 = RTOS_mutexRelease(m_sslConnectTableMutex);

    if ((OK <= status) && (OK > status1))
        status = status1;

exitNoMutexRelease:

    if (0 <= status)
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"SSL client made connection.");

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_connect() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;           /* can be an instance or a status */

} /* SSL_connect */

MOC_EXTERN sbyte4
SSL_connect(sbyte4 tempSocket,
            ubyte sessionIdLen, ubyte* sessionId,
            ubyte* masterSecret, const sbyte* dnsName)
{
    MSTATUS    status = OK;

    if (0 > tempSocket)
    {
        status = ERR_INVALID_ARG;
        goto exit;
    }

    status = SSL_connectAux(tempSocket, sessionIdLen, sessionId, masterSecret, dnsName, NULL);

//    SSL_setSessionFlags(status, SSL_FLAG_ENABLE_SEND_BUFFER );

exit:
    return (sbyte4)status;
}

MOC_EXTERN sbyte4
SSL_connectWithCfgParam(sbyte4 tempSocket,
                        ubyte sessionIdLen, ubyte* sessionId,
                        ubyte* masterSecret, const sbyte* dnsName, certParamCheckConf *pCertParamCheck)
{
    MSTATUS    status = OK;

    if (0 > tempSocket)
    {
        status = ERR_INVALID_ARG;
        goto exit;
    }

    status = SSL_connectAux(tempSocket, sessionIdLen, sessionId, masterSecret, dnsName, pCertParamCheck);
    
//    SSL_setSessionFlags(status, SSL_FLAG_ENABLE_SEND_BUFFER);

exit:
    return (sbyte4)status;
}

#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__
/* Register the certificate and key for the %server to use to identify itself to a %client.
This function registers the certificate and key for the %server to use to identify itself
to a %client. Your application should call this function before starting the HTTPS
and application servers.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param pCertificateDescr    Pointer to the X.509 certificate and key blob to register.
\param pIsChain             $TRUE$ if $pCertificateDescr$ points to a chain of certificates; $FALSE$ if $pCertificateDescr$ points to a single certificate.
\param ecCurves             Bit field specifying which curves are supported by
the peer (%client). Supported curves are any of the $tlsExtNamedCurves$
enumerated values (see ssl.h). To build the bit field, shift left a single bit
(the value 1) by the value of the desired $tlsExtNamedCurves$ enumeration.
Multiple shift-left results can be OR-ed together.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous servers only.

*/
extern sbyte4
SSL_ASYNC_initServer(certDescriptor *pCertificateDescr, sbyte4 pIsChain, ubyte4 ecCurves)
{
    return SSL_initServerCert(pCertificateDescr, pIsChain, ecCurves);
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__
/* Register a secure asynchronous SSL/TLS connection.
This function registers a secure asynchronous SSL/TLS connection.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param tempSocket       Socket or TCB identifier returned by a call to $accept()$.

\return Value >= 0 is the connection instance; otherwise a negative
number error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
%status, use the $DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous servers only.

\note This function must be called from within the HTTPS daemon context. If you
are using multiple HTTPS daemons,  you must use a semaphore (mutex) around
this function call.
\note If your web %server and application %server run as separate tasks, you
should protect the call to SSL_ASYNC_acceptConnection with a semaphore to prevent
race conditions.

*/
extern sbyte4
SSL_ASYNC_acceptConnection(sbyte4 tempSocket)
{
    return SSL_acceptConnectionCommon(FALSE, tempSocket, NULL, SSL_INT_FLAG_ASYNC_MODE);
}
#endif /* __ENABLE_MOCANA_SSL_ASYNC_SERVER_API__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__
/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern MSTATUS
SSL_ASYNC_connectCommon(intBoolean isDTLS, sbyte4 tempSocket, peerDescr *pPeerDescr, ubyte sessionIdLen, ubyte * sessionId,
                  ubyte * masterSecret, const sbyte* dnsName)
{
    /* a mutex is required around the call of this function, if multiple threads may invoke this API */
    sbyte4          index;
    hwAccelDescr    hwAccelCookie;
    intBoolean      isHwAccelCookieInit = FALSE;
    TCP_SOCKET      connectSocket       = (TCP_SOCKET)tempSocket;
    sbyte4          instance;
    MSTATUS         status, status1;

    if ( OK > ( status = RTOS_mutexWait( m_sslConnectTableMutex)))
        goto exitNoMutexRelease;

    status = ERR_SSL_TOO_MANY_CONNECTIONS;

    for (index = 0; index < m_sslMaxConnections; index++)
    {
        if ((CONNECT_CLOSED == m_sslConnectTable[index].connectionState) &&
            (TRUE == m_sslConnectTable[index].isClient))
        {
            if (OK > (status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_SSL, &hwAccelCookie)))
                goto exit;

            isHwAccelCookieInit = TRUE;
            m_sslConnectTable[index].connectionState = CONNECT_NEGOTIATE;

            status = ERR_MEM_ALLOC_FAIL;

            if (NULL == (m_sslConnectTable[index].pSSLSock = MALLOC(sizeof(SSLSocket))))
                goto exit;

            MOC_MEMSET((ubyte *)m_sslConnectTable[index].pSSLSock, 0x00, sizeof(SSLSocket));

            m_sslConnectTable[index].pSSLSock->hwAccelCookie = hwAccelCookie;
            m_sslConnectTable[index].pSSLSock->internalFlags = SSL_INT_FLAG_ASYNC_MODE;

            m_sslConnectTable[index].age = ((m_sslConnectTable[index].age + 1) & 0x7fff);
            instance = ((sbyte4)(m_sslConnectTable[index].age << NUM_BITS_SSL_SESSION_AGE) | index);

            m_sslConnectTable[index].pSSLSock->certParamCheck.chkCommonName = TRUE;
            m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityBefore = TRUE;
            m_sslConnectTable[index].pSSLSock->certParamCheck.chkValidityAfter = TRUE;
            m_sslConnectTable[index].pSSLSock->certParamCheck.chkKnownCA = TRUE;

            if (OK > (status = hashtable_insert(m_sslConnectHashTable, isDTLS, connectSocket, pPeerDescr, &m_sslConnectTable[index])))
                goto exit;

            if (OK > (status = setMessageTimer(m_sslConnectTable[index].pSSLSock, instance, m_sslSettings.sslTimeOutHello)))
                goto exit;

            if (OK > (status = SSL_SOCK_init(m_sslConnectTable[index].pSSLSock, isDTLS, connectSocket, pPeerDescr, mSSL_rngFun, mSSL_rngArg)))
                goto exit;

            if (OK > (status = SSL_SOCK_initSocketExtraClient(m_sslConnectTable[index].pSSLSock, sessionIdLen, sessionId,
                                                              masterSecret, dnsName)))
            {
                SSL_SOCK_uninit(m_sslConnectTable[index].pSSLSock);
                goto exit;
            }

            m_sslConnectTable[index].instance        = instance ;

#if defined(__ENABLE_MOCANA_DTLS_CLIENT__)
            if (isDTLS)
            {
                MOC_IP_ADDRESS srcAddrRef = REF_MOC_IPADDR(pPeerDescr->srcAddr);
                MOC_IP_ADDRESS peerAddrRef = REF_MOC_IPADDR(pPeerDescr->peerAddr);

                m_sslConnectTable[index].peerDescr.pUdpDescr = pPeerDescr->pUdpDescr;
                m_sslConnectTable[index].peerDescr.srcPort = pPeerDescr->srcPort;
                COPY_MOC_IPADDR(m_sslConnectTable[index].peerDescr.srcAddr, srcAddrRef);
                m_sslConnectTable[index].peerDescr.peerPort = pPeerDescr->peerPort;
                COPY_MOC_IPADDR(m_sslConnectTable[index].peerDescr.peerAddr, peerAddrRef);

                if (OK > SSL_setSessionFlags(instance, SSL_FLAG_ENABLE_SEND_BUFFER | SSL_FLAG_ENABLE_RECV_BUFFER))
                    goto exit;
            } else
#endif
            {
                m_sslConnectTable[index].socket = connectSocket;
            }
            status = instance;
            break;
        }
    }

    if (0 <= status)
        MOCANA_log((sbyte4)MOCANA_SSL, (sbyte4)LS_INFO, (sbyte *)"SSL client made connection.");

    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_ASYNC_connect() returns status = ", (sbyte4)status);

exit:
    if ((OK > status) && (index < m_sslMaxConnections))
    {
        m_sslConnectTable[index].connectionState = CONNECT_CLOSED;

        if (TRUE == isHwAccelCookieInit)
        {
            HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_SSL, &hwAccelCookie);
        }

        if (NULL != m_sslConnectTable[index].pSSLSock)
        {
            FREE(m_sslConnectTable[index].pSSLSock);
            m_sslConnectTable[index].pSSLSock = NULL;
        }
    }

    status1 = RTOS_mutexRelease(m_sslConnectTableMutex);

    if ((OK <= status) && (OK > status1))
        status = status1;

exitNoMutexRelease:

    return (sbyte4)status;           /* can be an instance or a status */

} /* SSL_ASYNC_connectCommon */

/*------------------------------------------------------------------*/

/* Create an asynchronous %client connection context.
This function creates a connection context for a secure SSL/TLS asynchronous
connection with a remote %server.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param tempSocket       Socket or TCB identifier returned by a call to $connect()$.
\param sessionIdLen     Number of bytes in $sessionId$, excluding the $NULL$ terminator.
\param sessionId        Pointer to session ID.
\param masterSecret     Pointer to master secret for the session.
\param dnsName          Pointer to expected DNS name of the server's certificate.

\return Value >= 0 is the connection instance; otherwise a negative
number error code definition from merrors.h. To retrieve a string containing an
English text error identifier corresponding to the function's returned error
%status, use the $DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous clients only.
*/
extern sbyte4
SSL_ASYNC_connect(sbyte4 tempSocket, ubyte sessionIdLen, ubyte * sessionId,
                  ubyte * masterSecret, const sbyte* dnsName)
{
    return SSL_ASYNC_connectCommon((intBoolean)FALSE, tempSocket, NULL, sessionIdLen, sessionId,
                                    masterSecret, dnsName);
}

#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__
/* Start establishing a secure client-server connection.
This function begins the process of establishing a secure connection between a
%client and %server by sending an SSL $Hello$ message to a %server.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_ASYNC_connect.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to asynchronous clients only.
*/
extern sbyte4
SSL_ASYNC_start(sbyte4 connectionInstance)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if ((CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState) &&
        (TRUE               == m_sslConnectTable[index].isClient))
    {
        SSLSocket*  pSSLSock       = m_sslConnectTable[index].pSSLSock;

        if (IS_SSL_SYNC(pSSLSock))
            goto exit;

        if (kSslSecureSessionNotEstablished == SSL_OPEN_STATE(pSSLSock))
            status = SSL_SOCK_clientHandshake(pSSLSock, TRUE);
        else
            status = ERR_SSL_CLIENT_START;

    }

exit:
    return (sbyte4)status;

} /* SSL_ASYNC_start */
#endif


/*------------------------------------------------------------------*/

#ifndef __DISABLE_SSL_GET_SOCKET_API__
/* Get a connection's socket identifier.
This function returns the socket identifier for the specified connection
instance.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

Additionally, the following flag must #not# be defined:
- $__DISABLE_SSL_GET_SOCKET_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pRetSocket           On return, pointer to the socket corresponding to the connection instance.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sbyte4
SSL_getSocketId(sbyte4 connectionInstance, sbyte4 *pRetSocket)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (NULL == pRetSocket)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        *pRetSocket = m_sslConnectTable[index].socket;
        status = OK;
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_getSocketId() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}

/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
/* Doc Note: This function is for Mocana internal code use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_getPeerDescr(sbyte4 connectionInstance, const peerDescr **ppRetPeerDescr)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (NULL == ppRetPeerDescr)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        *ppRetPeerDescr = &m_sslConnectTable[index].peerDescr;
        status = OK;
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_getSocketId() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif
#endif /* __DISABLE_SSL_GET_SOCKET_API__ */

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
/* Doc Note: This function is for Mocana internal code use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_getNextConnectionInstance(ubyte4 *pCookie, sbyte4 *pConnectionInstance, const peerDescr **ppRetPeerDescr)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (NULL == pCookie || NULL == pConnectionInstance || NULL == ppRetPeerDescr)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    for (index = *pCookie; index < m_sslMaxConnections; index++)
    {
        if ((index >= 0 && index < m_sslMaxConnections) &&
            (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState) &&
            ( !m_sslConnectTable[index].isClient ))
        {
            *pCookie = index+1;
            *pConnectionInstance = m_sslConnectTable[index].instance;
            *ppRetPeerDescr = &m_sslConnectTable[index].peerDescr;
            status = OK;
            break;
        }
    }

exit:
    return (sbyte4)status;
}
#endif

/*------------------------------------------------------------------*/

#ifndef __DISABLE_SSL_IS_SESSION_API__
/* Determine whether a connection instance represents an SSL/TLS %server, an SSL/TLS %client, or an unrecognized connection (for example, SSH).
This function determines whether a given connection instance represents an
SSL/TLS %server, an SSL/TLS %client, or an unrecognized connection (for example,
SSH). The returned value will be one of the following:

- 0&mdash;Indicates an SSL/TLS %server connection
- 1&mdash;Indicates an SSL/TLS %client connection
- Negative number&mdash;Indicates an unknown connection type

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

Additionally, the following flag must #not# be defined:
- $__DISABLE_SSL_IS_SESSION_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.

\return 0 if the connection instance is an SSL/TLS %server; 1 if an SSL/TLS %client; negative number if an unrecognized connection.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sbyte4
SSL_isSessionSSL(sbyte4 connectionInstance)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        status = (MSTATUS) m_sslConnectTable[index].isClient;     /* 0 == server, 1 == client, negative == bad connection instance */
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_isSessionSSL() returns status = ", (sbyte4)status);
#endif

exit:
    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DTLS_SERVER__) || defined(__ENABLE_MOCANA_DTLS_CLIENT__))
extern sbyte4
SSL_isSessionDTLS(sbyte4 connectionInstance)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        status = (MSTATUS) m_sslConnectTable[index].pSSLSock->isDTLS;     /* 0 == SSL, 1 == DTLS, negative == bad connection instance */
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_isSessionDTLS() returns status = ", (sbyte4)status);
#endif

exit:
    return (sbyte4)status;
}
#endif

#endif /* __DISABLE_SSL_IS_SESSION_API__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_SSL_SESSION_FLAGS_API__
/* Get a connection's context (its flags).
This function returns a connection's context&mdash;its flags. Your application can
call this function anytime after it calls SSL_connect.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

Additionally, the following flag must #not# be defined:
- $__DISABLE_SSL_SESSION_FLAGS_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pRetFlagsSSL         Pointer to the connection's flags, which have been set by SSL_setSessionFlags.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sbyte4
SSL_getSessionFlags(sbyte4 connectionInstance, ubyte4 *pRetFlagsSSL)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (NULL == pRetFlagsSSL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        if (NULL != m_sslConnectTable[index].pSSLSock)
        {
            /* don't return the SSL_FLAG_INTERNAL_USE settings. */
            *pRetFlagsSSL = m_sslConnectTable[index].pSSLSock->runtimeFlags & (~SSL_FLAG_INTERNAL_USE);
            status = OK;
        }

    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_getSessionFlags() returns status = ", (sbyte4)status);
#endif

exit:
    return (sbyte4)status;
}
#endif /* __DISABLE_SSL_SESSION_FLAGS_API__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_SSL_SESSION_FLAGS_API__
/* Store a connection's context (its flags).
This function stores a connection's context&mdash;its flags. Your application can
call this function anytime after it calls SSL_connect.

The context flags are specified by OR-ing the desired bitmask flag definitions, defined in ssl.h:
- $SSL_FLAG_ACCEPT_SERVER_NAME_LIST$
- $SSL_FLAG_ENABLE_RECV_BUFFER$
- $SSL_FLAG_ENABLE_SEND_BUFFER$
- $SSL_FLAG_ENABLE_SEND_EMPTY_FRAME$
- $SSL_FLAG_NO_MUTUAL_AUTH_REQ$
- $SSL_FLAG_REQUIRE_MUTUAL_AUTH$

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

Additionally, the following flag must #not# be defined:
- $__DISABLE_SSL_SESSION_FLAGS_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param flagsSSL             Bitmask of flags to set for the given connection's
context. They can be retrieved by calling SSL_getSessionFlags.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\note To avoid clearing any flags that are already set, you should first call
SSL_getSessionFlags, then OR the returned value with the desired new flag, and
only then call %SSL_setSessionFlags.

\remark This function is applicable to synchronous and asynchronous clients and
servers.

*/
extern sbyte4
SSL_setSessionFlags(sbyte4 connectionInstance, ubyte4 flagsSSL)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        if (NULL != m_sslConnectTable[index].pSSLSock)
        {
            /* also add back the SSL_FLAG_INTERNAL_USE settings. */
            m_sslConnectTable[index].pSSLSock->runtimeFlags = flagsSSL | (m_sslConnectTable[index].pSSLSock->runtimeFlags & SSL_FLAG_INTERNAL_USE);
            status = OK;
        }

    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_setSessionFlags() returns status = ", (sbyte4)status);
#endif

exit:
    return (sbyte4)status;
}
#endif /* __DISABLE_SSL_SESSION_FLAGS_API__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_KEY_EXPANSION__
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_generateExpansionKey(sbyte4 connectionInstance, ubyte *pKey,ubyte2 keyLen, ubyte *keyPhrase, ubyte2 keyPhraseLen)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        if (NULL != m_sslConnectTable[index].pSSLSock)
        {
            status = SSL_SOCK_generateKeyExpansionMaterial(m_sslConnectTable[index].pSSLSock,
                pKey ,keyLen,keyPhrase,keyPhraseLen);
        }
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_generateExpansionKey() returns status = ", (sbyte4)status);
#endif

exit:
    return (sbyte4)status;
}
#endif /* __ENABLE_MOCANA_SSL_KEY_EXPANSION__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_KEY_EXPANSION__
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_generateTLSExpansionKey(sbyte4 connectionInstance, ubyte *pKey,ubyte2 keyLen, ubyte *keyPhrase, ubyte2 keyPhraseLen)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        if (NULL != m_sslConnectTable[index].pSSLSock)
        {
            status = SSL_SOCK_generateTLSKeyExpansionMaterial(m_sslConnectTable[index].pSSLSock,
                pKey ,keyLen,keyPhrase,keyPhraseLen);
        }
    }

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_generateTLSExpansionKey() returns status = ", (sbyte4)status);
#endif

exit:
    return (sbyte4)status;
}
#endif /* __ENABLE_MOCANA_SSL_KEY_EXPANSION__ */


/*------------------------------------------------------------------*/

/* Get a connection's status.
This function returns a connection's status: $SSL_CONNECTION_OPEN$ or
$SSL_CONNECTION_NEGOTIATE$.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h
\param connectionInstance   Connection instance returned from SSL_connect.
\param pRetStatusSSL        On successful return, session's current status: $SSL_CONNECTION_OPEN$ or
$SSL_CONNECTION_NEGOTIATE$.


\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.
*/
extern sbyte4
SSL_getSessionStatus(sbyte4 connectionInstance, ubyte4 *pRetStatusSSL)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (NULL == pRetStatusSSL)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        if (NULL != m_sslConnectTable[index].pSSLSock)
        {
            if (CONNECT_OPEN == m_sslConnectTable[index].connectionState)
                *pRetStatusSSL = SSL_CONNECTION_OPEN;
            else
                *pRetStatusSSL = SSL_CONNECTION_NEGOTIATE;

            status = OK;
        }
    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_getSessionStatus() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}


/*------------------------------------------------------------------*/


#ifndef __DISABLE_SSL_IOCTL_API__
/* Enable dynamic management of a connection's features.
This function enables dynamic management (enabling and disabling) of selected
features for a specific SSL session's connection instance. (The initial value
for these settings is defined in ssl.h.)

You can dynamically alter whether SSLv3, TLS 1.0, or TLS 1.1 is used by
calling this function for the $SSL_SET_VERSION$ feature flag setting with
any of the following values:
- 0&mdash;Use SSLv3
- 1&mdash;Use TLS 1.0
- 2&mdash;Use TLS 1.1

\since 1.41
\version 3.06 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

Additionally, the following flag must #not# be defined:
- $__DISABLE_SSL_IOCTL_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param setting              SSL feature flag to dynamically alter; see SSL
runtime flag definitions ($SSL_FLAG_*$) in ssl.h.
\param value                Value to assign to the $setting$ flag.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sbyte4
SSL_ioctl(sbyte4 connectionInstance, ubyte4 setting, void *value)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket *pSSLSock = m_sslConnectTable[index].pSSLSock;

        status = ERR_SSL_IOCTL_FAILED;

        if (NULL != pSSLSock)
        {
            switch (setting)
            {
            case SSL_SET_VERSION:
            {
                if (kSslReceiveHelloInitState == SSL_HANDSHAKE_STATE(pSSLSock))
                {
                    ubyte4 version = (ubyte4)(unsigned long)value;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
                    if (pSSLSock->isDTLS)
                    {
                        if ( version != DTLS10_MINORVERSION &&
                            version != DTLS12_MINORVERSION)
                        {
                            goto exit;
                        }
                    }
                    else
#endif
                    {
                        if ( version > MAX_SSL_MINORVERSION ||
                            version < MIN_SSL_MINORVERSION )
                        {
                            goto exit;
                        }
                    }
                    pSSLSock->advertisedMinorVersion = (ubyte)version;     /* SSLv3 and TLS 1.0 or TLS 1.1 */
                    pSSLSock->runtimeFlags |= SSL_FLAG_VERSION_SET;
                    status = OK;
                }

                break;
            }
            case SSL_SET_MINIMUM_VERSION:
            {
                if (kSslReceiveHelloInitState == SSL_HANDSHAKE_STATE(pSSLSock))
                {
                    ubyte4 version = (ubyte4)value;

                    if ( version > MAX_SSL_MINORVERSION ||
                        version < MIN_SSL_MINORVERSION )
                    {
                        goto exit;
                    }

                    pSSLSock->minFallbackMinorVersion = (ubyte)version;     /* SSLv3 and TLS 1.0 or TLS 1.1 */
                    pSSLSock->runtimeFlags |= SSL_FLAG_MINIMUM_FALLBACK_VERSION_SET;
                    status = OK;
                }

                break;
            }
            case SSL_SET_SCSV_VERSION:
			{
				if (kSslReceiveHelloInitState == SSL_HANDSHAKE_STATE(pSSLSock))
				{
                    ubyte4 version = (ubyte4)(unsigned long)value;

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
					if (pSSLSock->isDTLS)
					{
						if ( version != DTLS10_MINORVERSION && version != DTLS12_MINORVERSION)
							goto exit;
					} else
#endif
					{
                        if ( version > MAX_SSL_MINORVERSION ||
                            version < MIN_SSL_MINORVERSION )
                        {
                            goto exit;
                        }
					}
					pSSLSock->advertisedMinorVersion = (ubyte)version;     /* SSLv3 and TLS 1.0 or TLS 1.1 */
					pSSLSock->runtimeFlags |= SSL_FLAG_VERSION_SET;
					pSSLSock->runtimeFlags |= SSL_FLAG_SCSV_FALLBACK_VERSION_SET;
					status = OK;
				}
				break;
			}
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
            case DTLS_SET_HANDSHAKE_RETRANSMISSION_TIMER:
            {
                ubyte4 timeout = (ubyte4)value;

                if (pSSLSock->isDTLS && kSslReceiveHelloInitState == SSL_HANDSHAKE_STATE(pSSLSock))
                {
                    pSSLSock->dtlsHandshakeTimeout = timeout;
                    status = OK;
                }

                break;
            }
            case DTLS_SET_PMTU:
            {
                ubyte4 mtu = (ubyte4)value;

                if (pSSLSock->isDTLS && kSslReceiveHelloInitState == SSL_HANDSHAKE_STATE(pSSLSock))
                {
                    pSSLSock->dtlsPMTU = mtu;
                    status = OK;
                }

                break;
            }
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) && defined(__ENABLE_MOCANA_DTLS_SRTP__))
            case DTLS_USE_SRTP:
            {
                if (pSSLSock->isDTLS)
                {
                    ubyte* mki = (ubyte*)value;

                    if (NULL != pSSLSock->srtpMki)
                        FREE(pSSLSock->srtpMki);

                    pSSLSock->srtpMki = NULL;

                    if (mki && *mki > 0)
                    {
                        if (NULL == (pSSLSock->srtpMki = MALLOC(*mki + 1)))
                            return ERR_MEM_ALLOC_FAIL;

                        MOC_MEMCPY(pSSLSock->srtpMki, mki, (*mki+1));
                    }

                    pSSLSock->useSrtp = TRUE;
                    status = OK;
                }

                break;
            }
#endif

#if defined(__ENABLE_MOCANA_DTLS_SERVER__)
            case DTLS_SET_HELLO_VERIFIED:
            {
                intBoolean helloVerified = (intBoolean)value;

                if ( (1 == helloVerified) && pSSLSock->isDTLS &&
                    !(m_sslConnectTable[index].isClient) &&
                     (kSslReceiveHelloInitState == SSL_HANDSHAKE_STATE(pSSLSock)) )
                {
                    /* HelloVerifyRequest is sent, update sequence numbers */
                    pSSLSock->nextSendSeq = 1;
                    pSSLSock->nextRecvSeq = 1;
                    pSSLSock->ownSeqnum   = 1;
                    pSSLSock->msgBase     = pSSLSock->nextRecvSeq;

                    status = OK;
                }

                break;
            }
#endif

#endif /* (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__)) */

            default:
            {
                break;
            }
            }
        }

    }

exit:
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_ioctl() returns status = ", (sbyte4)status);
#endif

    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ALERTS__
/* Get the SSL alert code for a Mocana error.
This function returns the SSL alert code for the specified Mocana error (from
$merrors$.h), as well as the alert class ($SSLALERTLEVEL_WARNING$ or
$SSLALERTLEVEL_FATAL$). See "SSL Alert Codes" for the list of alert definitions.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ALERTS__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param lookupError          Mocana error value to look up.
\param pRetAlertId          On return, pointer to SSL alert code.
\param pAlertClass          On return, pointer to alert class definition value.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.
*/
extern sbyte4
SSL_lookupAlert(sbyte4 connectionInstance, sbyte4 lookupError, sbyte4 *pRetAlertId, sbyte4 *pAlertClass)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if ((NULL == pRetAlertId) || (NULL == pAlertClass))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket *pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (TRUE != SSLSOCK_lookupAlert(pSSLSock, lookupError, pRetAlertId, pAlertClass))
        {
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
            if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
            {
                *pRetAlertId = SSL_ALERT_CLOSE_NOTIFY;  /* there is no catch-all alert for SSLv3 */
                *pAlertClass = SSLALERTLEVEL_FATAL;     /* we set to fatal to indicate some other error */
            }
            else
#endif
            {
                *pRetAlertId = SSL_ALERT_INTERNAL_ERROR;
                *pAlertClass = SSLALERTLEVEL_FATAL;
            }
        }

        status = OK;

#ifdef __ENABLE_MOCANA_SSL_SERVER__
        /* on fatal alert, we want to scramble session cache's master secret */
        if (SSLALERTLEVEL_FATAL == *pAlertClass)
            status = SSLSOCK_clearServerSessionCache(pSSLSock);
#endif
    }


exit:
    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_ALERTS__
/* Send an SSL alert message to an SSL peer.
This function sends an SSL alert message to an SSL peer. Typical usage is to
look up an error code using SSL_lookupAlert, and then send the alert message
using the %SSL_sendAlert function.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_ALERTS__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param alertId              SSL alert code.
\param alertClass           SSL alert class definition value.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.
*/
extern sbyte4
SSL_sendAlert(sbyte4 connectionInstance, sbyte4 alertId, sbyte4 alertClass)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket* pSSLSock    = m_sslConnectTable[index].pSSLSock;
        intBoolean encryptBool = (CONNECT_NEGOTIATE == m_sslConnectTable[index].connectionState) ? FALSE : TRUE;

        status = SSLSOCK_sendAlert(pSSLSock, encryptBool, alertId, alertClass);
    }

exit:
    return (sbyte4)status;
}
#endif


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_INNER_APP__)
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_sendInnerApp(sbyte4 connectionInstance, InnerAppType innerApp,ubyte* pMsg, ubyte4 msgLen , ubyte4 * retMsgLen)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket* pSSLSock    = m_sslConnectTable[index].pSSLSock;

        status = SSLSOCK_sendInnerApp(pSSLSock, innerApp, pMsg, msgLen , retMsgLen, m_sslConnectTable[index].isClient);     /* 0 == server, 1 == client, negative == bad connection instance */
    }

exit:
    return (sbyte4)status;
}
#endif /* __ENABLE_MOCANA_INNER_APP__ */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_INNER_APP__)
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_updateInnerAppSecret(sbyte4 connectionInstance, ubyte* session_key, ubyte4 sessionKeyLen)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket* pSSLSock    = m_sslConnectTable[index].pSSLSock;

        status = SSLSOCK_updateInnerAppSecret(pSSLSock, session_key, sessionKeyLen);
    }

exit:
    return (sbyte4)status;
}
#endif /* __ENABLE_MOCANA_INNER_APP__ */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_INNER_APP__)
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_verifyInnerAppVerifyData(sbyte4 connectionInstance,ubyte *data,InnerAppType appType)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket* pSSLSock    = m_sslConnectTable[index].pSSLSock;

        status = SSLSOCK_verifyInnerAppVerifyData(pSSLSock, data, appType, m_sslConnectTable[index].isClient);     /* 0 == server, 1 == client, negative == bad connection instance */
    }

exit:
    return (sbyte4)status;
}
#endif /* __ENABLE_MOCANA_INNER_APP__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
static sbyte4
SSL_enableCiphersEx(sbyte4 connectionInstance, ubyte2 *pCipherSuiteList, ubyte4 listLength)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;
        sbyte4      cipherIndex;
        ubyte4      count;

        if (NULL == pSSLSock)
            goto exit;

        status = ERR_SSL_CONFIG;

        if (SSL_MAX_NUM_CIPHERS < SSL_SOCK_numCiphersAvailable())
        {
            /* bad news: we can't detect this problem at compile time */
            /* good news: the test monkeys should detect this problem */
            goto exit;
        }

        for (count = 0; count < listLength; count++)
        {
            /* ability to chose at run-time cipher suites to support */
            if (0 <= (cipherIndex = SSL_SOCK_getCipherTableIndex(pSSLSock, pCipherSuiteList[count])))
            {
                /* mark the cipher as active */
                pSSLSock->isCipherTableInit = TRUE;
                pSSLSock->isCipherEnabled[cipherIndex] = TRUE;

                /* we successfully enabled at least one cipher, so that is goodness */
                status = OK;
            }
        }
    }

exit:
    return (sbyte4)status;

} /* SSL_enableCiphersEx */

/* Enable specified ciphers.
This function dynamically enables just those ciphers that are specified in the
function call. If none of the specified ciphers match those supported by
NanoSSL %client/server and enabled in your implementation, an error is
returned.

The function must not be called before a connection is established (see
SSL_connect for synchronous clients, SSL_ASYNC_connect for asynchronous clients),
but must be called before SSL_negotiateConnection (for either synchronous or
asynchronous clients).

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pCipherSuiteList     Pointer to value (or array of values) representing
the desired cipher ID(s).\n
Values are as specified per RFC 4346 for the TLS
Cipher Suite Registry; refer to the following Web page: http://www.iana.org/assignments/tls-parameters .
\param listLength           Number of entries in $pCipherSuiteList$.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.
*/

extern sbyte4
SSL_enableCiphers(sbyte4 connectionInstance, ubyte2 *pCipherSuiteList, ubyte4 listLength)
{
    return SSL_enableCiphersEx(connectionInstance, pCipherSuiteList, listLength);

} /* SSL_enableCiphers */

#ifdef __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
/* Enable Ciphers for TLS12, along with any other ciphers */
MOC_EXTERN sbyte4
SSL_enableCiphersTLS12WithMinLOS(sbyte4 connectionInstance, ubyte2 *pCipherSuiteList, ubyte4 listLength, ubyte2 minLOS)
{
    MSTATUS     status = OK;
    ubyte2      cipherSuiteListForTLS12[2];
    ubyte4      numCipherSuites = 1;
    ubyte4      tlsVersion = 0;

    if (OK > (status = SSL_getSSLTLSVersion(connectionInstance, &tlsVersion)))
        goto exit;

    /* enabled only for TLS1.2 */
    if ((1 == (status = SSL_isSessionSSL(connectionInstance))) && 
        (TLS12_MINORVERSION != tlsVersion))
    {
        status = ERR_SSL_CONFIG;
        goto exit;
    }

    if (OK > status)
    {
        status = ERR_SSL_CONFIG;
        goto exit;
    }

    if (SSL_TLS12_MINLOS_128 == minLOS)
    {
        cipherSuiteListForTLS12[0] = 0xC02B;
        cipherSuiteListForTLS12[1] = 0xC02C;
        numCipherSuites = 2;
    }
    else if (SSL_TLS12_MINLOS_192 == minLOS)
    {
        cipherSuiteListForTLS12[0] = 0xC02C;
    }
    else
    {
        status = ERR_SSL_CONFIG;
        goto exit;
    }

    if (OK > (status = SSL_enableCiphersEx(connectionInstance, cipherSuiteListForTLS12, numCipherSuites)))
        goto exit;

    if (pCipherSuiteList && (0 < listLength))
    {
        status = SSL_enableCiphersEx(connectionInstance, pCipherSuiteList, listLength);
    }

exit:
    return (sbyte4)status;
}
#endif /*__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__*/

#endif /* __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__)   || \
        defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)|| \
        defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
extern sbyte4
SSL_enableECCCurves(sbyte4 connectionInstance,
                    enum tlsExtNamedCurves* pECCCurvesList,
                    ubyte4 listLength)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;
        ubyte4      count;

        if (NULL == pSSLSock)
            goto exit;

        status = ERR_SSL_CONFIG;

        pSSLSock->eccCurves = 0; /* clear all */

        for (count = 0; count < listLength; count++)
        {
            pSSLSock->eccCurves |= (1 << (pECCCurvesList[count]));
        }
        /* mask with the compile time flags */
        pSSLSock->eccCurves &= SSL_SOCK_getECCCurves();
        if ( pSSLSock->eccCurves) /* at least one curve supported */
        {
            status = OK;
        }
        else /* invalid arg: restore */
        {
            pSSLSock->eccCurves = SSL_SOCK_getECCCurves();
        }
    }

exit:
    return (sbyte4)status;

} /* SSL_enableECCCurves */
#endif
#endif /* __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__ */

/*------------------------------------------------------------------*/

/* Get a connection's ciphers and ecCurves.
This function retrieves the specified connection's cipher and ecCurves.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pCipherId            On return, pointer to the connection's cipher value.
\param pPeerEcCurves        On return, pointer to the connection's supported
ecCurves values (as a bit field built by OR-ing together shift-left combinations
of bits shifted by the value of $tlsExtNamedCurves$ enumerations).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sbyte4
SSL_getCipherInfo( sbyte4 connectionInstance, ubyte2* pCipherId,
                  ubyte4* pPeerEcCurves)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    /* default values */
    *pCipherId = 0;
    *pPeerEcCurves = 0;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API can be called prior or after SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (NULL == pSSLSock)
            goto exit;

#ifdef __ENABLE_MOCANA_SSL_SERVER__
        *pPeerEcCurves = pSSLSock->roleSpecificInfo.server.clientECCurves;
#endif
        status = SSL_SOCK_getCipherId( pSSLSock, pCipherId);
    }

exit:
    return (sbyte4)status;

} /* SSL_getCipherInfo */


/*------------------------------------------------------------------*/

/* Get a connection's SSL/TLS version
This function retrieves the specified connection's SSL/TLS version.

\since 2.02
\version 2.02 and later

! Flags
To enable this function, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param version              On return, pointer to the connection's SSL version.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\remark This function is applicable to synchronous and asynchronous clients and servers.

*/
extern sbyte4
SSL_getSSLTLSVersion( sbyte4 connectionInstance, ubyte4* pVersion)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    /* default values */
    *pVersion = 0xFFFFFFFF;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API can be called prior or after SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (NULL == pSSLSock)
            goto exit;

        *pVersion = pSSLSock->sslMinorVersion;

        status = OK;
    }

exit:
    return (sbyte4)status;

} /* SSL_getSSLTLSVersion */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_MOCANA_SSL_CLIENT__)
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_setEAPFASTParams(sbyte4 connectionInstance, ubyte* pPacOpaque,
                     ubyte4 pacOpaqueLen, ubyte pPacKey[/*PACKEY_SIZE*/])
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        status = SSL_SOCK_setEAPFASTParams(m_sslConnectTable[index].pSSLSock,
            pPacOpaque, pacOpaqueLen, pPacKey);
    }

exit:
    return (sbyte4)status;

} /* SSL_setEAPFASTParams */
#endif /* __ENABLE_MOCANA_EAP_FAST__ && __ENABLE_MOCANA_SSL_CLIENT__*/


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_EAP_FAST__
extern sbyte4
SSL_getEAPFAST_CHAPChallenge(sbyte4 connectionInstance, ubyte *challenge , ubyte4 challengeLen)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    SSLSocket*  pSSLSock = NULL;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    pSSLSock = m_sslConnectTable[index].pSSLSock;
    status   = OK;

    if ((NULL == pSSLSock) || (NULL == pSSLSock->fastChapChallenge))
    {
        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "SSL_findSocket: connectionInstance not found.");
        goto exit;
    }

    MOC_MEMCPY(challenge, pSSLSock->fastChapChallenge, (challengeLen > FAST_MSCHAP_CHAL_SIZE)? FAST_MSCHAP_CHAL_SIZE : challengeLen);


exit:
    return status;

}

/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_getEAPFAST_IntermediateCompoundKey(sbyte4 connectionInstance, ubyte *s_imk,
                                       ubyte *msk, ubyte mskLen,
                                       ubyte *imk)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    SSLSocket*  pSSLSock = NULL;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    pSSLSock = m_sslConnectTable[index].pSSLSock;
    status   = OK;

    if (NULL == pSSLSock)
    {
        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "SSL_findSocket: connectionInstance not found.");
        goto exit;
    }
    status = SSL_SOCK_generateEAPFASTIntermediateCompoundKey(pSSLSock,
                                                   s_imk, msk, mskLen, imk);

exit:
    return status;
}

/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_generateEAPFASTSessionKeys(sbyte4 connectionInstance, ubyte* S_IMCK, sbyte4 s_imckLen,
                                    ubyte* MSK, sbyte4 mskLen, ubyte* EMSK, sbyte4 emskLen/*64 Len */)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    SSLSocket*  pSSLSock = NULL;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    pSSLSock = m_sslConnectTable[index].pSSLSock;
    status   = OK;

    if (NULL == pSSLSock)
    {
        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "SSL_findSocket: connectionInstance not found.");
        goto exit;
    }
    status = SSL_SOCK_generateEAPFASTSessionKeys(pSSLSock, S_IMCK, s_imckLen, MSK, mskLen, EMSK, emskLen);

exit:
    return status;
}


#endif /* __ENABLE_MOCANA_EAP_FAST__  */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_RFC3546__)
/* Specify a list of server names acceptable to the %client.
This function specifies a list of server names acceptable to the %client. This
enables a %client to tell a server the server name the %client is attempting to
connect to. This may facilitate secure connections to servers that host multiple
virtual servers at a single underlying network address.

\since 2.02
\version 3.1.1 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_RFC3546__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.
\param pServerNameList      Pointer to string containing one or multiple
concatenated host names, each in the following format:\n
\n
0 + 2-byte length prefix + UTF8-encoded string (not $NULL$ terminated)\n
\n
For example, to specify two server names, }hello} and }world}, specify the
$%pServerNameList$ parameter value as $"\0\0\5hello\0\0\5world"$.
\param serverNameListLen    Number of bytes in the server name list
($pServerNameList$). For the above example, this value would be 14.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern sbyte4
SSL_setServerNameList(sbyte4 connectionInstance, ubyte *pServerNameList, ubyte4 serverNameListLen)
{
    /* the argument should be the following: pServerNameList is a succession of the following
    1 byte = 0 + 2 bytes length prefix + string UTF8 (not null terminated), serverNameListLen is the total length
    of the pServerNameList array */
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        status = ERR_SSL_CONFIG;

        if ( pSSLSock->serverNameList)
        {
            FREE( pSSLSock->serverNameList);
            pSSLSock->serverNameList = 0;
            pSSLSock->serverNameListLength = 0;
        }

        if ( serverNameListLen)
        {
            pSSLSock->serverNameList = MALLOC( serverNameListLen);
            if ( !pSSLSock->serverNameList)
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }

            MOC_MEMCPY(pSSLSock->serverNameList, pServerNameList, serverNameListLen);
            pSSLSock->serverNameListLength = serverNameListLen;
        }

        status = OK;
    }

exit:
    return (sbyte4)status;

} /* SSL_setServerNameList */
#endif /* __ENABLE_RFC3546__ */


/*------------------------------------------------------------------*/


#if defined(__ENABLE_MOCANA_SSL_CLIENT__) && defined(__ENABLE_TLSEXT_RFC6066__)

extern sbyte4
SSL_setSNI(sbyte4 connectionInstance, ServerName *pServerName, ubyte4 numOfServers)
{
    /* Argument should have an array of ServerName and num of servers */
    sbyte4  index,i, offset;
    MSTATUS status              = ERR_SSL_BAD_ID;
    ubyte4  serverNameListLen   = 0;

    if (NULL == pServerName)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        status = ERR_SSL_CONFIG;

        if ( pSSLSock->serverNameList)
        {
            FREE( pSSLSock->serverNameList);
            pSSLSock->serverNameList = 0;
            pSSLSock->serverNameListLength = 0;
        }

        if ((pServerName != NULL) && numOfServers)
        {
            if (1 < numOfServers)
            {
                /* Duplicate name_types are prohibited and current RFC
                              * only supports one name_type; essentially you cannot
                              * have more than one servername. We should exit.
                              */
                status = ERR_SSL_EXTENSION_DUPLICATE_NAMETYPE_SNI;
                goto exit;
            }

            for (i = 0; i < numOfServers; i++)
            {
                /* 3 = 1-byte 0 + 2-byte length + length of UTF-8 String*/
                serverNameListLen = serverNameListLen + 3 + pServerName[i].nameLen;
            }

            pSSLSock->serverNameListLength = serverNameListLen;

            if (NULL == (pSSLSock->serverNameList = MALLOC(serverNameListLen)))
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }

            offset = 0;
            for (i = 0; i < numOfServers; i++)
            {
                MOC_MEMCPY(pSSLSock->serverNameList + offset, "\0", 1);
                offset += 1;

                setShortValue(pSSLSock->serverNameList + offset, pServerName[i].nameLen);
                offset += 2;

                MOC_MEMCPY(pSSLSock->serverNameList + offset, pServerName[i].pName, pServerName[i].nameLen);
                offset += pServerName[i].nameLen;
            }
        }

        status = OK;
    }

exit:
    return (sbyte4)status;

} /* SSL_setSNI */

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_OCSP_CLIENT__

extern sbyte4
SSL_setCertifcateStatusRequestExtensions(sbyte4 connectionInstance,
    sbyte** ppTrustedResponderCertPath, ubyte4 trustedResponderCertCount,
    intBoolean shouldAddNonceExtension,
    extensions* pExts, ubyte4 extCount)
{
    MSTATUS     status = ERR_SSL_BAD_ID;
    sbyte4      index;
    CertificateStatusRequest request;
    ubyte4      offset = 0;

    if (((NULL == ppTrustedResponderCertPath) && trustedResponderCertCount) ||
        ((NULL == pExts) && extCount))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
           goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        pSSLSock->certStatusReqExt          = TRUE;
        pSSLSock->roleSpecificInfo.client.didRecvCertStatusExtInServHello = FALSE;

        /* Initialize ocsp Context */
        if (OK > (status = SSL_OCSP_initContext(&pSSLSock->pOcspContext)))
            goto exit;

        /* Request type is ocsp */
        request.status_type                 = certStatusType_ocsp;
        request.ocspReq.pExtensionsList     = NULL;
        request.ocspReq.pResponderIdList    = NULL;
        request.ocspReq.extensionListLen    = 0;
        request.ocspReq.responderIdListLen  = 0;

        /* Add trusted responder certificate to verify signed responses */
        if (0 < trustedResponderCertCount)
        {
            if (OK > (status = SSL_OCSP_createResponderIdList(pSSLSock->pOcspContext,
                            ppTrustedResponderCertPath, trustedResponderCertCount,
                            (ubyte **)&request.ocspReq.pResponderIdList, (ubyte4 *)&request.ocspReq.responderIdListLen)))
            {
                goto exit;
            }
        }

        if (shouldAddNonceExtension || (0 < extCount))
        {
            if (OK > (status = SSL_OCSP_createExtensionsList(pSSLSock->pOcspContext, pExts, extCount, shouldAddNonceExtension,
                            (ubyte **)&request.ocspReq.pExtensionsList, &request.ocspReq.extensionListLen)))
            {
                goto exit;
            }
        }

        /* Now fill the data */
        pSSLSock->roleSpecificInfo.client.certStatusReqExtLen  = 1 +
                        2 + request.ocspReq.responderIdListLen +
                        2 + request.ocspReq.extensionListLen;

        if (NULL == (pSSLSock->roleSpecificInfo.client.certStatusReqExtData
                        = MALLOC(pSSLSock->roleSpecificInfo.client.certStatusReqExtLen)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMSET(pSSLSock->roleSpecificInfo.client.certStatusReqExtData, 0x00,
                    pSSLSock->roleSpecificInfo.client.certStatusReqExtLen);

        /* status type */
        offset = 0;

        MOC_MEMCPY(pSSLSock->roleSpecificInfo.client.certStatusReqExtData + offset, &request.status_type, 1);
        offset += 1;

        /* Responder Id List Length */
        setShortValue(pSSLSock->roleSpecificInfo.client.certStatusReqExtData + offset,
            request.ocspReq.responderIdListLen);

        offset += 2;

        if (request.ocspReq.pResponderIdList)
        {
            MOC_MEMCPY(pSSLSock->roleSpecificInfo.client.certStatusReqExtData + offset,
                request.ocspReq.pResponderIdList, request.ocspReq.responderIdListLen);

            offset += request.ocspReq.responderIdListLen;
        }

        setShortValue(pSSLSock->roleSpecificInfo.client.certStatusReqExtData + offset,
            request.ocspReq.extensionListLen);

        offset += 2;

        if (request.ocspReq.pExtensionsList) {
            MOC_MEMCPY(pSSLSock->roleSpecificInfo.client.certStatusReqExtData + offset,
                request.ocspReq.pExtensionsList, request.ocspReq.extensionListLen);

            offset += request.ocspReq.pExtensionsList;
        }
    }

    status = OK;

exit:
    return (sbyte4)status;

}

#endif
#endif /* __ENABLE_TLSEXT_RFC6066__ */

/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_TLSEXT_RFC6066__)

extern sbyte4
SSL_setOcspResponderUrl(sbyte4 connectionInstance, void *pUrl)
{
    MSTATUS     status = ERR_SSL_BAD_ID;
    sbyte4      index;

    if(NULL == pUrl)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
           goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;
        pSSLSock->roleSpecificInfo.server.pResponderUrl = pUrl;
        status = OK;
    }

exit:
    return  status;

}

#endif /* defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_TLSEXT_RFC6066__) */

/*------------------------------------------------------------------*/

#if (defined(__ENABLE_RFC3546__) && defined(__ENABLE_MOCANA_INNER_APP__))
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_setInnerApplicationExt(sbyte4 connectionInstance, ubyte4 innerAppValue)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        status = ERR_SSL_CONFIG;

        if (TRUE == m_sslConnectTable[index].isClient)
        {
            pSSLSock->roleSpecificInfo.client.innerAppValue = innerAppValue;
            pSSLSock->roleSpecificInfo.client.innerApp      = TRUE;
        }
        else
        {
            pSSLSock->roleSpecificInfo.server.innerAppValue = innerAppValue;
            pSSLSock->roleSpecificInfo.server.innerApp      = TRUE;
        }
        status = OK;
    }

exit:
    return (sbyte4)status;

} /* SSL_setInnerApplicationExt */
#endif /* (defined(__ENABLE_RFC3546__) && defined(__ENABLE_MOCANA_INNER_APP__)) */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_RFC3546__
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_retrieveServerNameList(sbyte4 connectionInstance, ubyte *pBuffer, ubyte4 bufferSize, ubyte4 *pRetServerNameLength)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to SSL_negotiateConnection() */
    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        status = ERR_SSL_CONFIG;

        if (NULL != pSSLSock)
        {
            *pRetServerNameLength = (bufferSize >= pSSLSock->serverNameListLength) ? pSSLSock->serverNameListLength : bufferSize;

            status = MOC_MEMCPY(pBuffer, pSSLSock->serverNameList, *pRetServerNameLength);
        }

    }

exit:
    return (sbyte4)status;

} /* SSL_retrieveServerNameList */
#endif /* __ENABLE_RFC3546__ */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_INTERNAL_STRUCT_ACCESS__
/* Doc Note: This function is for Mocana internal code (EAP stack) use only, and
should not be included in the API documentation.
*/
extern void *
SSL_returnPtrToSSLSocket(sbyte4 connectionInstance)
{
    sbyte4      index;
    SSLSocket*  pSSLSock = NULL;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_NEGOTIATE  <= m_sslConnectTable[index].connectionState)
    {
        pSSLSock = m_sslConnectTable[index].pSSLSock;
    }

exit:
    return (void *)pSSLSock;
}
#endif /* __ENABLE_MOCANA_SSL_INTERNAL_STRUCT_ACCESS__ */


/*------------------------------------------------------------------*/

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) ||  \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
ubyte2 SSL_getNamedCurveOfCurveId( ubyte4 curveId)
{
    switch ( curveId)
    {
    case cid_EC_P192:
        return tlsExtNamedCurves_secp192r1;
        break;

    case cid_EC_P224:
        return tlsExtNamedCurves_secp224r1;
        break;

    case cid_EC_P256:
        return tlsExtNamedCurves_secp256r1;
        break;

    case cid_EC_P384:
        return tlsExtNamedCurves_secp384r1;
        break;

    case cid_EC_P521:
        return tlsExtNamedCurves_secp521r1;
        break;

    default:
        break;
    }
    return 0;
}
#endif


/*------------------------------------------------------------------*/

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
     defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
     defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__))
ubyte4 SSL_getCurveIdOfNamedCurve( ubyte2 namedCurve)
{
    switch ( namedCurve)
    {
        case tlsExtNamedCurves_secp192r1:
            return cid_EC_P192;

        case tlsExtNamedCurves_secp224r1:
            return cid_EC_P224;

        case tlsExtNamedCurves_secp256r1:
            return cid_EC_P256;

        case tlsExtNamedCurves_secp384r1:
            return cid_EC_P384;

        case tlsExtNamedCurves_secp521r1:
            return cid_EC_P521;

        default:
            break;
    }
    return 0;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__))
/* Renegotiate an SSL/TLS session.
This function renegotiates a %client or server SSL session. Renegoatiation
can be necessary in a variety of circumstances, including:
- Reducing attack vulnerability after a connection has been active for a long
time
- Enhancing security by using stronger encryption
- Performing mutual authentication

The peer can ignore the rehandshake request or send back an
$SSL_ALERT_NO_RENEGOTIATION$ alert.

\since 2.45
\version 2.45 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_REHANDSHAKE__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_CLIENT__$
- $__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__$
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/
extern sbyte4
SSL_initiateRehandshake(sbyte4 connectionInstance)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_OPEN <= m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;

        if (NULL == pSSLSock)
            goto exit;

        if (kSslOpenState != SSL_HANDSHAKE_STATE(pSSLSock))
        {
            /* we are already doing a (re)handshake */
            status = OK;
            goto exit;
        }

        if (pSSLSock->server)
        {
#if (defined(__ENABLE_MOCANA_SSL_SERVER__))
            status = SSL_SOCK_sendServerHelloRequest(pSSLSock);
#endif
        }
        else
        {
#if (defined(__ENABLE_MOCANA_SSL_CLIENT__))
            status = SSL_SOCK_clientHandshake(pSSLSock, TRUE);
#endif
        }

    }

exit:
    return (sbyte4)status;

} /* SSL_initiateRehandshake */


/* Timer check for rehandshaking
This function check for the timeout for rehandshaking for server SSL session.
If timeout occure then it will call the callback fuction to initiate the 
Rehandshake.

\since 5.8
\version 5.8 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_REHANDSHAKE__$

Additionally, at least one of the following flags must be defined in moptions.h:
- $__ENABLE_MOCANA_SSL_SERVER__$
- $__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__$

#Include %file:#&nbsp;&nbsp;ssl.h

\param connectionInstance   Connection instance returned from SSL_connect.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

*/

extern sbyte4
SSL_checkRehandshakeTimer(sbyte4 connectionInstance)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    if (CONNECT_OPEN == m_sslConnectTable[index].connectionState)
    {
        if (m_sslSettings.maxTimerCountForRehandShake > 0)
        {
            if ((RTOS_deltaMS(&m_sslConnectTable[index].pSSLSock->sslRehandshakeTimerCount, NULL) > m_sslSettings.maxTimerCountForRehandShake) && (m_sslSettings.funcPtrClientRehandshakeRequest != NULL))
            {
                status = m_sslSettings.funcPtrClientRehandshakeRequest(connectionInstance);
                m_sslConnectTable[index].pSSLSock->sslRehandshakeByteSendCount = 0;
                RTOS_deltaMS(NULL, &m_sslConnectTable[index].pSSLSock->sslRehandshakeTimerCount);
            }
        }
    }

exit:
    return status;

}

#endif /* (defined( __ENABLE_MOCANA_SSL_REHANDSHAKE__)) */
/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) || defined(__ENABLE_MOCANA_DTLS_SERVER__))
/* Doc Note: This function is for Mocana internal code use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_checkHandshakeTimer(sbyte4 connectionInstance)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;
    SSLSocket*  pSSLSock       = NULL;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    pSSLSock = m_sslConnectTable[index].pSSLSock;

    if (NULL == pSSLSock)
    {
        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, (sbyte*)"SSL_checkHandshakeTimer: connectionInstance not found.");
        goto exit;
    }

    if (IS_SSL_SYNC(pSSLSock))
        goto exit;

    /* check DTLS handshake timer. call timeout callback if timer expired */
    status = TIMER_checkTimer(pSSLSock->dtlsHandshakeTimer);

exit:
    return (sbyte4)status;
}


/*------------------------------------------------------------------*/

#if (defined (__ENABLE_MOCANA_DTLS_SRTP__) && defined (__ENABLE_MOCANA_SRTP_PROFILES_SELECT__))
/* Doc Note: This function is for Mocana internal code use only, and
should not be included in the API documentation.
*/
extern sbyte4
SSL_enableSrtpProfiles(sbyte4 connectionInstance, ubyte2 *pSrtpProfileList, ubyte4 listLength)
{
    sbyte4  index;
    MSTATUS status = ERR_SSL_BAD_ID;

    if (OK > (index = getIndexFromConnectionInstance(connectionInstance)))
        goto exit;

    /* this API must be called prior to start of handshake */
    if (CONNECT_NEGOTIATE  == m_sslConnectTable[index].connectionState)
    {
        SSLSocket*  pSSLSock = m_sslConnectTable[index].pSSLSock;
        sbyte4      profileIndex;
        ubyte4      count;

        if (NULL == pSSLSock)
            goto exit;

        status = ERR_SSL_CONFIG;

        if (SRTP_MAX_NUM_PROFILES < SSL_SOCK_numSrtpProfilesAvailable())
        {
            /* bad news: we can't detect this problem at compile time */
            /* good news: the test monkeys should detect this problem */
            goto exit;
        }

        for (count = 0; count < listLength; count++)
        {
            /* ability to chose at run-time srtp protection profiles to support */
            if (0 <= (profileIndex = SSL_SOCK_getSrtpProfileIndex(pSSLSock, pSrtpProfileList[count])))
            {
                /* mark the profile as active */
                pSSLSock->isSrtpProfileTableInit = TRUE;
                pSSLSock->isSrtpProfileEnabled[profileIndex] = TRUE;

                /* we successfully enabled at least one profile, so that is goodness */
                status = OK;
            }
        }

    }

exit:
    return (sbyte4)status;
}
#endif

#endif /* (__ENABLE_MOCANA_DTLS_CLIENT__) || (__ENABLE_MOCANA_DTLS_SERVER__) */


/*-------------------------------------------------------------------------*/

extern MSTATUS
SSL_rngFun( ubyte4 len, ubyte* buff)
{
    return mSSL_rngFun( mSSL_rngArg, len, buff);
}

#endif /* (defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_CLIENT__)) */

