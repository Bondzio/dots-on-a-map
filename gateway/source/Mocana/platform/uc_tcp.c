/*
 * ucos_tcp.c
 *
 * uC-OS TCP Abstraction Layer
 *
 * Copyright Mocana Corp 2009-2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef __UCOS_TCP__

#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mtcp.h"
#include "../common/moc_net.h"
#include "../common/mstdlib.h"
#include "../common/debug_console.h"


#include <KAL/kal.h>
#include <IP/IPv4/net_ipv4.h>
#include <Source/net_sock.h>
#include <Source/net_util.h>
#include <Source/net_ascii.h>
#include <Source/net_conn.h>

#include <Secure/net_secure.h>



#ifndef SOMAXCONN
#define SOMAXCONN       (10)
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_init(void)
{
    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_shutdown(void)
{
    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_listenSocket(TCP_SOCKET *listenSocket, ubyte2 portNumber)
{
#if defined(NET_TCP_MODULE_EN) && \
    defined(NET_IPv4_MODULE_EN)
    TCP_SOCKET            newSocket;
    NET_SOCK_ADDR_IPv4    saServer;
    NET_ERR               err;

    newSocket = NetSock_Open(NET_SOCK_PROTOCOL_FAMILY_IP_V4,
                             NET_SOCK_TYPE_STREAM,
                             NET_SOCK_PROTOCOL_TCP,
                             &err);

    if (NET_SOCK_ERR_NONE != err)
    {
        DEBUG_PRINTNL(DEBUG_PLATFORM, (sbyte *)"Could not create listen socket");
        return (ERR_TCP_LISTEN_SOCKET_ERROR);
    }

    Mem_Clr((void *)&saServer, sizeof(saServer));

    saServer.AddrFamily = NET_SOCK_ADDR_FAMILY_IP_V4;
    saServer.Addr       = NET_UTIL_HOST_TO_NET_32(NET_IPv4_ADDR_ANY);
    saServer.Port       = NET_UTIL_HOST_TO_NET_16(portNumber);

    NetSock_Bind((NET_SOCK_ID)newSocket, (NET_SOCK_ADDR *)&saServer, (NET_SOCK_ADDR_LEN)NET_SOCK_ADDR_SIZE, &err);

    if (NET_SOCK_ERR_NONE != err)
    {
        NetSock_Close(newSocket, &err);
        return ERR_TCP_LISTEN_ERROR;
    }

    NetSock_Listen(newSocket, SOMAXCONN, &err);

    if (NET_SOCK_ERR_NONE != err)
    {
        NetSock_Close(newSocket, &err);
        return ERR_TCP_LISTEN_ERROR;
    }

    *listenSocket = newSocket;

    return OK;
#else
    // This function is not available using the Micrium NetSecure layer
    return ERR_TCP_LISTEN_SOCKET_ERROR;
#endif
}

/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_acceptSocket(TCP_SOCKET *clientSocket, TCP_SOCKET listenSocket,
                      intBoolean *isBreakSignalRequest)
{
#if defined(NET_TCP_MODULE_EN) && \
    defined(NET_IPv4_MODULE_EN)
    NET_SOCK_ADDR_LEN   nLen;
    NET_SOCK_ADDR       sockAddr;
    TCP_SOCKET          newClientSocket;
    NET_ERR             err;
    MSTATUS             status = OK;

    nLen = sizeof(sockAddr);
    newClientSocket = NetSock_Accept((NET_SOCK_ID)listenSocket, &sockAddr, &nLen, &err);

    if (NET_SOCK_ERR_NONE != err)
    {
        status = ERR_TCP_ACCEPT_ERROR;
        goto exit;
    }

    *clientSocket = newClientSocket;

exit:
    return status;
#else
    // This function is not available using the Micrium NetSecure layer
    return ERR_TCP_ACCEPT_ERROR;
#endif
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_connectSocket(TCP_SOCKET *pConnectSocket, sbyte *pIpAddress, ubyte2 portNo)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_ADDR        serverIpAddress;
    NET_SOCK_ADDR_IPv4   serverAddr;
    NET_ERR         err;
    MSTATUS              status = OK;
    NET_SOCK_RTN_CODE   conn_rtn_code;

    *pConnectSocket = NetSock_Open(NET_SOCK_PROTOCOL_FAMILY_IP_V4, NET_SOCK_TYPE_STREAM, NET_SOCK_PROTOCOL_TCP, &err);

    if (NET_SOCK_ERR_NONE != err)
    {
        status = ERR_TCP_CONNECT_CREATE;
        goto exit;
    }

    serverIpAddress = NetASCII_Str_to_IPv4((CPU_CHAR *)pIpAddress, &err);

    if (NET_ASCII_ERR_NONE != err)
    {
        status = ERR_TCP_CONNECT_CREATE;
        NetSock_Close(*pConnectSocket, &err);
        goto exit;
    }

    Mem_Clr((void *)&serverAddr, (CPU_SIZE_T) NET_SOCK_ADDR_SIZE);
    serverAddr.AddrFamily = NET_SOCK_PROTOCOL_FAMILY_IP_V4;
    serverAddr.Addr       = NET_UTIL_HOST_TO_NET_32(serverIpAddress);
    serverAddr.Port       = NET_UTIL_HOST_TO_NET_16(portNo);

    conn_rtn_code = NetSock_Conn((NET_SOCK_ID)*pConnectSocket, (NET_SOCK_ADDR*)&serverAddr, sizeof(serverAddr), &err);

    if (NET_SOCK_BSD_ERR_NONE != conn_rtn_code)
    {
        status = ERR_TCP_CONNECT_ERROR;
        NetSock_Close(*pConnectSocket, &err);
        goto exit;
    }

exit:
    return status;
#else
    // This function is not available using the Micrium NetSecure layer
    return ERR_TCP_CONNECT_ERROR;
#endif
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_closeSocket(TCP_SOCKET socket)
{
#ifdef  NET_IPv4_MODULE_EN
    NET_ERR err;

    NetSock_Close(socket, &err);

    return OK;
#else
    // This function is not available using the Micrium NetSecure layer
    return ERR_TCP_SOCKET_CLOSED;
#endif
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_readSocketAvailable(TCP_SOCKET socket, sbyte *pBuffer, ubyte4 maxBytesToRead,
                              ubyte4 *pNumBytesRead, ubyte4 msTimeout)
{
#if defined(NET_TCP_MODULE_EN) && \
    defined(NET_IPv4_MODULE_EN)
    NET_SOCK_RTN_CODE    retValue;
    NET_ERR              err;
    MSTATUS              status;
    NET_SOCK            *p_sock;
    NET_SOCK_ID          sock_id;
    CPU_BOOLEAN          block;




    Net_GlobalLockAcquire((void *)&UCOS_TCP_readSocketAvailable, &err);

    if ((NULL == pBuffer) || (NULL == pNumBytesRead))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }


    sock_id = (NET_SOCK_ID)socket;
    p_sock  =  NetSock_GetObj(sock_id);
    if (p_sock == DEF_NULL) {
        status = ERR_TCP_NO_SUCH_SOCKET;
        goto exit;
    }

   *pNumBytesRead = 0;

    NetConn_IsConn(p_sock->ID_Conn, &err);
    if (err == NET_CONN_ERR_NOT_USED) {
        status = ERR_TCP_NO_SUCH_SOCKET;
        goto exit;
    }
    retValue =  NetSock_RxDataHandlerStream((NET_SOCK_ID        ) sock_id,
                                            (NET_SOCK          *) p_sock,
                               (void      *)pBuffer,
                               (CPU_INT16U )maxBytesToRead,
                                            (NET_SOCK_API_FLAGS ) p_sock->Flags,
                                            (NET_SOCK_ADDR     *) 0,
                                            (NET_SOCK_ADDR_LEN *) 0,
                                            (NET_ERR           *)&err);
    switch (err) {
        case NET_SOCK_ERR_NONE:

        case NET_ERR_RX:
             break;

        case NET_SOCK_ERR_RX_Q_CLOSED:
        case NET_SOCK_ERR_CLOSED:
             status = ERR_TCP_SOCKET_CLOSED;
             goto exit;

        case NET_SOCK_ERR_RX_Q_EMPTY:

             block = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);
             if (block == DEF_YES) {
                 status = ERR_TCP_READ_BLOCK_FAIL;
                 goto exit;
             }
             status = OK;
             goto exit_dly;

        case NET_SOCK_ERR_NOT_USED:
        case NET_SOCK_ERR_FAULT:
        case NET_SOCK_ERR_CONN_FAIL:
        case NET_SOCK_ERR_INVALID_FAMILY:
        case NET_SOCK_ERR_INVALID_PROTOCOL:
        case NET_SOCK_ERR_INVALID_STATE:
        case NET_SOCK_ERR_INVALID_OP:
        case NET_SOCK_ERR_INVALID_ADDR_LEN:
        case NET_CONN_ERR_INVALID_CONN:
        case NET_CONN_ERR_NOT_USED:
        case NET_ERR_FAULT_NULL_PTR:
        case NET_CONN_ERR_INVALID_ADDR_LEN:
        case NET_CONN_ERR_ADDR_NOT_USED:
        default:
             status = ERR_TCP_READ_ERROR;
             goto exit;
    }

   *pNumBytesRead = retValue;
    status = OK;

exit_dly:
    Net_GlobalLockRelease();
    KAL_Dly(1);
    goto exit_return;

exit:
    Net_GlobalLockRelease();

exit_return:
    return status;
#else
    return (ERR_TCP_NO_SUCH_SOCKET);
#endif
} /* UCOS_TCP_readSocketAvailable */


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_writeSocket(TCP_SOCKET socket, sbyte *pBuffer, ubyte4 numBytesToWrite,
                     ubyte4 *pNumBytesWritten)
{
#if defined(NET_TCP_MODULE_EN) && \
    defined(NET_IPv4_MODULE_EN)
    int         retValue;
    MSTATUS     status;
    NET_ERR     err;
    NET_SOCK    *p_sock;  // socket is a NET_SOCK pointer
    NET_SOCK_ID sock_id;
    CPU_BOOLEAN block;


    Net_GlobalLockAcquire((void *)&UCOS_TCP_writeSocket, &err);
    if ((NULL == pBuffer) || (NULL == pNumBytesWritten))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }


   *pNumBytesWritten = 0;


    sock_id = (NET_SOCK_ID)socket;
    p_sock  =  NetSock_GetObj(sock_id);
    if (p_sock == DEF_NULL) {
        status = ERR_TCP_NO_SUCH_SOCKET;
        goto exit;
    }

    NetConn_IsConn(p_sock->ID_Conn, &err);
    if (err == NET_CONN_ERR_NOT_USED) {
        status = ERR_TCP_NO_SUCH_SOCKET;
        goto exit;
    }

    retValue =  NetSock_TxDataHandlerStream((NET_SOCK_ID       ) sock_id,
                                            (NET_SOCK         *) p_sock,
                                            (void             *) pBuffer,
                                            (CPU_INT16U        ) numBytesToWrite,
                                            (NET_SOCK_API_FLAGS) p_sock->Flags,
                                            (NET_ERR          *)&err);

    switch (err) {
        case NET_SOCK_ERR_NONE:
             break;

        case NET_SOCK_ERR_CLOSED:
             status = ERR_TCP_SOCKET_CLOSED;
             goto exit;

        case NET_ERR_TX:
             block = DEF_BIT_IS_CLR(p_sock->Flags, NET_SOCK_FLAG_SOCK_NO_BLOCK);

             if(block == DEF_YES) {
                 status = ERR_TCP_WRITE_BLOCK_FAIL;
                 goto exit;
             }

             status = ERR_TCP_WRITE_ERROR;
             goto exit_dly;

        case NET_ERR_IF_LINK_DOWN:
        case NET_SOCK_ERR_NOT_USED:
        case NET_SOCK_ERR_FAULT:
        case NET_SOCK_ERR_CONN_FAIL:
        case NET_SOCK_ERR_INVALID_PROTOCOL:
        case NET_SOCK_ERR_INVALID_STATE:
        case NET_SOCK_ERR_INVALID_OP:
        case NET_SOCK_ERR_INVALID_DATA_SIZE :
        case NET_SOCK_ERR_TX_Q_CLOSED:
        case NET_CONN_ERR_INVALID_CONN:
        case NET_CONN_ERR_NOT_USED:
        default:
             status = ERR_TCP_WRITE_ERROR;
             goto exit;
    }

   *pNumBytesWritten = retValue;
    status = OK;
    goto exit;

exit_dly:
    Net_GlobalLockRelease();
    KAL_Dly(1);
    return status;

exit:
    Net_GlobalLockRelease();
    return status;
#else
    return (ERR_TCP_NO_SUCH_SOCKET);
#endif
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_TCP_getPeerName(TCP_SOCKET socket, ubyte2 *pRetPortNo, MOC_IP_ADDRESS_S *pRetAddr)
{
    /* doesn't look like APIs are available to get at the connection data... */
    return ERR_TCP_GETSOCKNAME;
}


#endif /* __UCOS_TCP__ */
