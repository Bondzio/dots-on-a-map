/* Version: mss_v6_3 */
/*
 * mtcp.h
 *
 * Mocana TCP Abstraction Layer
 *
 * Copyright Mocana Corp 2003-2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __MTCP_HEADER__
#define __MTCP_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define TCP_NO_TIMEOUT      (0)

#if defined __SOLARIS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            SOLARIS_TCP_init
#define TCP_SHUTDOWN        SOLARIS_TCP_shutdown
#define TCP_LISTEN_SOCKET   SOLARIS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   SOLARIS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    SOLARIS_TCP_closeSocket
#define TCP_READ_AVL        SOLARIS_TCP_readSocketAvailable
#define TCP_WRITE           SOLARIS_TCP_writeSocket
#define TCP_CONNECT         SOLARIS_TCP_connectSocket

#elif defined __LINUX_TCP__

#define TCP_SOCKET              int
#define TCP_INIT                LINUX_TCP_init
#define TCP_SHUTDOWN            LINUX_TCP_shutdown
#define TCP_LISTEN_SOCKET       LINUX_TCP_listenSocket
#define TCP_ACCEPT_SOCKET       LINUX_TCP_acceptSocket
#define TCP_CLOSE_SOCKET        LINUX_TCP_closeSocket
#define TCP_READ_AVL            LINUX_TCP_readSocketAvailable
#define TCP_WRITE               LINUX_TCP_writeSocket
#define TCP_CONNECT             LINUX_TCP_connectSocket
#define TCP_CONNECT_NONBLOCK    LINUX_TCP_connectSocketNonBlocking
#define TCP_getPeerName         LINUX_TCP_getPeerName


#elif defined __SYMBIAN_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            SYMBIAN_TCP_init
#define TCP_SHUTDOWN        SYMBIAN_TCP_shutdown
#define TCP_LISTEN_SOCKET   SYMBIAN_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   SYMBIAN_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    SYMBIAN_TCP_closeSocket
#define TCP_READ_AVL        SYMBIAN_TCP_readSocketAvailable
#define TCP_WRITE           SYMBIAN_TCP_writeSocket
#define TCP_CONNECT         SYMBIAN_TCP_connectSocket
#define TCP_getPeerName     SYMBIAN_TCP_getPeerName


#elif defined __WIN32_TCP__

#define TCP_SOCKET          unsigned int
#define TCP_INIT            WIN32_TCP_init
#define TCP_SHUTDOWN        WIN32_TCP_shutdown
#define TCP_LISTEN_SOCKET   WIN32_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   WIN32_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    WIN32_TCP_closeSocket
#define TCP_READ_AVL        WIN32_TCP_readSocketAvailable
#define TCP_WRITE           WIN32_TCP_writeSocket
#define TCP_CONNECT         WIN32_TCP_connectSocket
#define TCP_getPeerName     WIN32_TCP_getPeerName

#elif defined __VXWORKS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            VXWORKS_TCP_init
#define TCP_SHUTDOWN        VXWORKS_TCP_shutdown
#define TCP_LISTEN_SOCKET   VXWORKS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   VXWORKS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    VXWORKS_TCP_closeSocket
#define TCP_READ_AVL        VXWORKS_TCP_readSocketAvailable
#define TCP_WRITE           VXWORKS_TCP_writeSocket
#define TCP_CONNECT         VXWORKS_TCP_connectSocket
#define TCP_getPeerName     VXWORKS_TCP_getPeerName

#elif defined __NNOS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            NNOS_TCP_init
#define TCP_SHUTDOWN        NNOS_TCP_shutdown
#define TCP_LISTEN_SOCKET   NNOS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   NNOS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    NNOS_TCP_closeSocket
#define TCP_READ_AVL        NNOS_TCP_readSocketAvailable
#define TCP_WRITE           NNOS_TCP_writeSocket
#define TCP_CONNECT         NNOS_TCP_connectSocket

#elif defined __PSOS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            PSOS_TCP_init
#define TCP_SHUTDOWN        PSOS_TCP_shutdown
#define TCP_LISTEN_SOCKET   PSOS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   PSOS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    PSOS_TCP_closeSocket
#define TCP_READ_AVL        PSOS_TCP_readSocketAvailable
#define TCP_WRITE           PSOS_TCP_writeSocket
#define TCP_CONNECT         PSOS_TCP_connectSocket
#define TCP_SHARE_SOCKET    PSOS_TCP_shareSocket

#elif defined __NUCLEUS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            NUCLEUS_TCP_init
#define TCP_SHUTDOWN        NUCLEUS_TCP_shutdown
#define TCP_LISTEN_SOCKET   NUCLEUS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   NUCLEUS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    NUCLEUS_TCP_closeSocket
#define TCP_READ_AVL        NUCLEUS_TCP_readSocketAvailable
#define TCP_WRITE           NUCLEUS_TCP_writeSocket
#define TCP_CONNECT         NUCLEUS_TCP_connectSocket

#elif defined __CYGWIN_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            CYGWIN_TCP_init
#define TCP_SHUTDOWN        CYGWIN_TCP_shutdown
#define TCP_LISTEN_SOCKET   CYGWIN_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   CYGWIN_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    CYGWIN_TCP_closeSocket
#define TCP_READ_AVL        CYGWIN_TCP_readSocketAvailable
#define TCP_WRITE           CYGWIN_TCP_writeSocket
#define TCP_CONNECT         CYGWIN_TCP_connectSocket
#define TCP_getPeerName     CYGWIN_TCP_getPeerName

#elif defined __OSX_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            OSX_TCP_init
#define TCP_SHUTDOWN        OSX_TCP_shutdown
#define TCP_LISTEN_SOCKET   OSX_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   OSX_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    OSX_TCP_closeSocket
#define TCP_READ_AVL        OSX_TCP_readSocketAvailable
#define TCP_WRITE           OSX_TCP_writeSocket
#define TCP_CONNECT         OSX_TCP_connectSocket

#elif defined __THREADX_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            THREADX_TCP_init
#define TCP_SHUTDOWN        THREADX_TCP_shutdown
#define TCP_LISTEN_SOCKET   THREADX_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   THREADX_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    THREADX_TCP_closeSocket
#define TCP_READ_AVL        THREADX_TCP_readSocketAvailable
#define TCP_WRITE           THREADX_TCP_writeSocket
#define TCP_CONNECT         THREADX_TCP_connectSocket

#elif defined __POSNET_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            POSNET_TCP_init
#define TCP_SHUTDOWN        POSNET_TCP_shutdown
#define TCP_CLOSE_SOCKET    POSNET_TCP_closeSocket
#define TCP_READ_AVL        POSNET_TCP_readSocketAvailable
#define TCP_WRITE           POSNET_TCP_writeSocket
#define TCP_CONNECT         POSNET_TCP_connectSocket

#elif defined __OSE_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            OSE_TCP_init
#define TCP_SHUTDOWN        OSE_TCP_shutdown
#define TCP_LISTEN_SOCKET   OSE_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   OSE_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    OSE_TCP_closeSocket
#define TCP_READ_AVL        OSE_TCP_readSocketAvailable
#define TCP_WRITE           OSE_TCP_writeSocket
#define TCP_CONNECT         OSE_TCP_connectSocket

#elif defined __RTCS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            RTCS_TCP_init
#define TCP_SHUTDOWN        RTCS_TCP_shutdown
#define TCP_LISTEN_SOCKET   RTCS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   RTCS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    RTCS_TCP_closeSocket
#define TCP_READ_AVL        RTCS_TCP_readSocketAvailable
#define TCP_WRITE           RTCS_TCP_writeSocket
#define TCP_CONNECT         RTCS_TCP_connectSocket
#define TCP_SHARE_SOCKET    RTCS_TCP_shareSocket

#elif defined __TRECK_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            TRECK_TCP_init
#define TCP_SHUTDOWN        TRECK_TCP_shutdown
#define TCP_LISTEN_SOCKET   TRECK_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   TRECK_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    TRECK_TCP_closeSocket
#define TCP_READ_AVL        TRECK_TCP_readSocketAvailable
#define TCP_WRITE           TRECK_TCP_writeSocket
#define TCP_CONNECT         TRECK_TCP_connectSocket

#elif defined __NETBURNER_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            NETBURNER_TCP_init
#define TCP_SHUTDOWN        NETBURNER_TCP_shutdown
#define TCP_LISTEN_SOCKET   NETBURNER_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   NETBURNER_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    NETBURNER_TCP_closeSocket
#define TCP_READ_AVL        NETBURNER_TCP_readSocketAvailable
#define TCP_WRITE           NETBURNER_TCP_writeSocket
#define TCP_CONNECT         NETBURNER_TCP_connectSocket

#elif defined __OPENBSD_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            OPENBSD_TCP_init
#define TCP_SHUTDOWN        OPENBSD_TCP_shutdown
#define TCP_LISTEN_SOCKET   OPENBSD_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   OPENBSD_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    OPENBSD_TCP_closeSocket
#define TCP_READ_AVL        OPENBSD_TCP_readSocketAvailable
#define TCP_WRITE           OPENBSD_TCP_writeSocket
#define TCP_CONNECT         OPENBSD_TCP_connectSocket
#define TCP_getPeerName     OPENBSD_TCP_getPeerName

#elif defined __NUTOS_TCP__

#define TCP_SOCKET          void*
#define TCP_INIT            NUTOS_TCP_init
#define TCP_SHUTDOWN        NUTOS_TCP_shutdown
#define TCP_LISTEN_SOCKET   NUTOS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   NUTOS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    NUTOS_TCP_closeSocket
#define TCP_READ_AVL        NUTOS_TCP_readSocketAvailable
#define TCP_WRITE           NUTOS_TCP_writeSocket
#define TCP_CONNECT         NUTOS_TCP_connectSocket

#elif defined __INTEGRITY_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            INTEGRITY_TCP_init
#define TCP_SHUTDOWN        INTEGRITY_TCP_shutdown
#define TCP_LISTEN_SOCKET   INTEGRITY_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   INTEGRITY_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    INTEGRITY_TCP_closeSocket
#define TCP_READ_AVL        INTEGRITY_TCP_readSocketAvailable
#define TCP_WRITE           INTEGRITY_TCP_writeSocket
#define TCP_CONNECT         INTEGRITY_TCP_connectSocket
#define TCP_getPeerName     INTEGRITY_TCP_getPeerName

#elif defined __ANDROID_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            ANDROID_TCP_init
#define TCP_SHUTDOWN        ANDROID_TCP_shutdown
#define TCP_LISTEN_SOCKET   ANDROID_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   ANDROID_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    ANDROID_TCP_closeSocket
#define TCP_READ_AVL        ANDROID_TCP_readSocketAvailable
#define TCP_WRITE           ANDROID_TCP_writeSocket
#define TCP_CONNECT         ANDROID_TCP_connectSocket
#define TCP_getPeerName     ANDROID_TCP_getPeerName

#elif defined __FREEBSD_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            FREEBSD_TCP_init
#define TCP_SHUTDOWN        FREEBSD_TCP_shutdown
#define TCP_LISTEN_SOCKET   FREEBSD_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   FREEBSD_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    FREEBSD_TCP_closeSocket
#define TCP_READ_AVL        FREEBSD_TCP_readSocketAvailable
#define TCP_WRITE           FREEBSD_TCP_writeSocket
#define TCP_CONNECT         FREEBSD_TCP_connectSocket

#elif defined __IRIX_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            IRIX_TCP_init
#define TCP_SHUTDOWN        IRIX_TCP_shutdown
#define TCP_LISTEN_SOCKET   IRIX_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   IRIX_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    IRIX_TCP_closeSocket
#define TCP_READ_AVL        IRIX_TCP_readSocketAvailable
#define TCP_WRITE           IRIX_TCP_writeSocket
#define TCP_CONNECT         IRIX_TCP_connectSocket

#elif defined __QNX_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            QNX_TCP_init
#define TCP_SHUTDOWN        QNX_TCP_shutdown
#define TCP_LISTEN_SOCKET   QNX_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   QNX_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    QNX_TCP_closeSocket
#define TCP_READ_AVL        QNX_TCP_readSocketAvailable
#define TCP_WRITE           QNX_TCP_writeSocket
#define TCP_CONNECT         QNX_TCP_connectSocket
#define TCP_getPeerName     QNX_TCP_getPeerName

#elif defined __UITRON_TCP__

typedef struct
{
    intBoolean      isListenSocket;
    ubyte2          srcPortNumber;

    intBoolean      isConnected;
    MOC_IP_ADDRESS_S dstAddress;
    ubyte2          dstPortNumber;

    signed int      repId;
    signed int      cepId;

} uitronSocketDescr;

typedef uitronSocketDescr*  uitronSocketDescrPtr;

#define TCP_SOCKET          uitronSocketDescrPtr
#define TCP_INIT            UITRON_TCP_init
#define TCP_SHUTDOWN        UITRON_TCP_shutdown
#define TCP_LISTEN_SOCKET   UITRON_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   UITRON_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    UITRON_TCP_closeSocket
#define TCP_READ_AVL        UITRON_TCP_readSocketAvailable
#define TCP_WRITE           UITRON_TCP_writeSocket
#define TCP_CONNECT         UITRON_TCP_connectSocket
#define TCP_getPeerName     UITRON_TCP_getPeerName

#elif defined __WINCE_TCP__

#define TCP_SOCKET          unsigned int
#define TCP_INIT            WINCE_TCP_init
#define TCP_SHUTDOWN        WINCE_TCP_shutdown
#define TCP_LISTEN_SOCKET   WINCE_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   WINCE_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    WINCE_TCP_closeSocket
#define TCP_READ_AVL        WINCE_TCP_readSocketAvailable
#define TCP_WRITE           WINCE_TCP_writeSocket
#define TCP_CONNECT         WINCE_TCP_connectSocket
#define TCP_getPeerName     WINCE_TCP_getPeerName

#elif defined __WTOS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            WTOS_TCP_init
#define TCP_SHUTDOWN        WTOS_TCP_shutdown
#define TCP_LISTEN_SOCKET   WTOS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   WTOS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    WTOS_TCP_closeSocket
#define TCP_READ_AVL        WTOS_TCP_readSocketAvailable
#define TCP_WRITE           WTOS_TCP_writeSocket
#define TCP_CONNECT         WTOS_TCP_connectSocket

#elif defined __ECOS_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            ECOS_TCP_init
#define TCP_SHUTDOWN        ECOS_TCP_shutdown
#define TCP_LISTEN_SOCKET   ECOS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   ECOS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    ECOS_TCP_closeSocket
#define TCP_READ_AVL        ECOS_TCP_readSocketAvailable
#define TCP_WRITE           ECOS_TCP_writeSocket
#define TCP_CONNECT         ECOS_TCP_connectSocket

#elif defined __AIX_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            AIX_TCP_init
#define TCP_SHUTDOWN        AIX_TCP_shutdown
#define TCP_LISTEN_SOCKET   AIX_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   AIX_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    AIX_TCP_closeSocket
#define TCP_READ_AVL        AIX_TCP_readSocketAvailable
#define TCP_WRITE           AIX_TCP_writeSocket
#define TCP_CONNECT         AIX_TCP_connectSocket
#define TCP_getPeerName     AIX_TCP_getPeerName

#elif defined __HPUX_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            HPUX_TCP_init
#define TCP_SHUTDOWN        HPUX_TCP_shutdown
#define TCP_LISTEN_SOCKET   HPUX_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   HPUX_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    HPUX_TCP_closeSocket
#define TCP_READ_AVL        HPUX_TCP_readSocketAvailable
#define TCP_WRITE           HPUX_TCP_writeSocket
#define TCP_CONNECT         HPUX_TCP_connectSocket
#define TCP_getPeerName     HPUX_TCP_getPeerName

#elif defined __MICROCHIP_BSD_TCP__

#define TCP_SOCKET          char
#define TCP_INIT            MICROCHIP_TCP_init
#define TCP_SHUTDOWN        MICROCHIP_TCP_shutdown
#define TCP_LISTEN_SOCKET   MICROCHIP_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   MICROCHIP_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    MICROCHIP_TCP_closeSocket
#define TCP_READ_AVL        MICROCHIP_TCP_readSocketAvailable
#define TCP_WRITE           MICROCHIP_TCP_writeSocket
#define TCP_CONNECT         MICROCHIP_TCP_connectSocket

#elif defined __UCOS_TCP__

#define TCP_SOCKET          char
#define TCP_INIT            UCOS_TCP_init
#define TCP_SHUTDOWN        UCOS_TCP_shutdown
#define TCP_LISTEN_SOCKET   UCOS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   UCOS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    UCOS_TCP_closeSocket
#define TCP_READ_AVL        UCOS_TCP_readSocketAvailable
#define TCP_WRITE           UCOS_TCP_writeSocket
#define TCP_CONNECT         UCOS_TCP_connectSocket

#elif defined __UCOS_DIRECT_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            UCOS_TCP_init
#define TCP_SHUTDOWN        UCOS_TCP_shutdown
#define TCP_LISTEN_SOCKET   UCOS_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   UCOS_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    UCOS_TCP_closeSocket
#define TCP_READ_AVL        UCOS_TCP_readSocketAvailable
#define TCP_WRITE           UCOS_TCP_writeSocket
#define TCP_CONNECT         UCOS_TCP_connectSocket

#elif defined __LWIP_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            LWIP_TCP_init
#define TCP_SHUTDOWN        LWIP_TCP_shutdown
#define TCP_LISTEN_SOCKET   LWIP_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   LWIP_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    LWIP_TCP_closeSocket
#define TCP_READ_AVL        LWIP_TCP_readSocketAvailable
#define TCP_WRITE           LWIP_TCP_writeSocket
#define TCP_CONNECT         LWIP_TCP_connectSocket

#elif defined __DUMMY_TCP__

#define TCP_SOCKET          int
#define TCP_INIT            DUMMY_TCP_init
#define TCP_SHUTDOWN        DUMMY_TCP_shutdown
#define TCP_LISTEN_SOCKET   DUMMY_TCP_listenSocket
#define TCP_ACCEPT_SOCKET   DUMMY_TCP_acceptSocket
#define TCP_CLOSE_SOCKET    DUMMY_TCP_closeSocket
#define TCP_READ_AVL        DUMMY_TCP_readSocketAvailable
#define TCP_WRITE           DUMMY_TCP_writeSocket
#define TCP_CONNECT         DUMMY_TCP_connectSocket

#else

#error UNSUPPORTED PLATFORM

#endif

#if defined __MOCANA_TCP__

#define MOC_TCP_SOCKET          int
#define MOC_TCP_INIT            MO_TCP_init
#define MOC_TCP_SHUTDOWN        MO_TCP_shutdown
#define MOC_TCP_LISTEN_SOCKET   MO_TCP_listenSocket
#define MOC_TCP_ACCEPT_SOCKET   MO_TCP_acceptSocket
#define MOC_TCP_CLOSE_SOCKET    MO_TCP_closeSocket
#define MOC_TCP_READ_AVL        MO_TCP_readSocketAvailable
#define MOC_TCP_WRITE           MO_TCP_writeSocket
#define MOC_TCP_CONNECT         MO_TCP_connectSocket

#endif


MOC_EXTERN MSTATUS TCP_INIT         (void);
MOC_EXTERN MSTATUS TCP_SHUTDOWN     (void);
MOC_EXTERN MSTATUS TCP_LISTEN_SOCKET(TCP_SOCKET *socket, ubyte2 portNumber);
MOC_EXTERN MSTATUS TCP_ACCEPT_SOCKET(TCP_SOCKET *clientSocket, TCP_SOCKET listenSocket, intBoolean *isBreakSignalRequest);
#ifndef __THREADX_TCP__
MOC_EXTERN MSTATUS TCP_CONNECT      (TCP_SOCKET *pConnectSocket, sbyte *pIpAddress, ubyte2 portNo);
#else
MOC_EXTERN MSTATUS TCP_CONNECT      (TCP_SOCKET *pConnectSocket, MOC_IP_ADDRESS ipAddress, ubyte2 portNo);
#endif
MOC_EXTERN MSTATUS TCP_CLOSE_SOCKET (TCP_SOCKET socket);

MOC_EXTERN MSTATUS TCP_READ_AVL     (TCP_SOCKET socket, sbyte *pBuffer, ubyte4 maxBytesToRead,  ubyte4 *pNumBytesRead, ubyte4 msTimeout);
MOC_EXTERN MSTATUS TCP_WRITE        (TCP_SOCKET socket, sbyte *pBuffer, ubyte4 numBytesToWrite, ubyte4 *pNumBytesWritten);

MOC_EXTERN MSTATUS TCP_READ_ALL     (TCP_SOCKET socket, sbyte *pBuffer, ubyte4 maxBytesToRead,  ubyte4 *pNumBytesRead, ubyte4 msTimeout);
MOC_EXTERN MSTATUS TCP_WRITE_ALL    (TCP_SOCKET socket, sbyte *pBuffer, ubyte4 numBytesToWrite, ubyte4 *pNumBytesWritten);
MOC_EXTERN MSTATUS TCP_getPeerName  (TCP_SOCKET socket, ubyte2 *portNo, MOC_IP_ADDRESS_S *addr);


MOC_EXTERN MSTATUS MOC_TCP_INIT         (void);
MOC_EXTERN MSTATUS MOC_TCP_SHUTDOWN     (void);
MOC_EXTERN MSTATUS MOC_TCP_LISTEN_SOCKET(TCP_SOCKET *socket, ubyte2 portNumber);
MOC_EXTERN MSTATUS MOC_TCP_ACCEPT_SOCKET(TCP_SOCKET *clientSocket, TCP_SOCKET listenSocket, intBoolean *isBreakSignalRequest);
MOC_EXTERN MSTATUS MOC_TCP_CLOSE_SOCKET (TCP_SOCKET socket);
MOC_EXTERN MSTATUS MOC_TCP_READ_AVL     (TCP_SOCKET socket, sbyte *pBuffer, ubyte4 maxBytesToRead,  ubyte4 *pNumBytesRead, ubyte4 msTimeout);
MOC_EXTERN MSTATUS MOC_TCP_WRITE        (TCP_SOCKET socket, sbyte *pBuffer, ubyte4 numBytesToWrite, ubyte4 *pNumBytesWritten);
MOC_EXTERN MSTATUS MOC_TCP_CONNECT      (TCP_SOCKET *pConnectSocket, sbyte *pIpAddress, ubyte2 portNo);

#ifdef TCP_SHARE_SOCKET
MOC_EXTERN MSTATUS TCP_SHARE_SOCKET (TCP_SOCKET socket);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MTCP_HEADER__ */
