//------------------------------------------------------------------------------
// Copyright (c) Qualcomm Atheros, Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
// the limitations in the disclaimer below) provided that the following conditions are met:
//
// · Redistributions of source code must retain the above copyright notice, this list of conditions and the
//   following disclaimer.
// · Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//   following disclaimer in the documentation and/or other materials provided with the distribution.
// · Neither the name of nor the names of its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//------------------------------------------------------------------------------
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <driver_cxt.h>
#include <common_api.h>
#include <custom_wlan_api.h>
#include <wmi_api.h>
#include <htc.h>
#include <qca.h>
#include "atheros_wifi.h"
#include "atheros_wifi_api.h"
#include "atheros_wifi_internal.h"
#include "cust_netbuf.h"

#if ENABLE_STACK_OFFLOAD
#include "atheros_stack_offload.h"
#include "common_stack_offload.h"
#include "custom_stack_offload.h"


extern QCA_CTX *QCA_CtxPtr;

#if ZERO_COPY
//A_NETBUF_QUEUE_T zero_copy_free_queue;
#endif
A_NETBUF_QUEUE_T zero_copy_free_queue;
A_NETBUF_QUEUE_T custom_alloc_queue;
#if ENABLE_SSL

QOSAL_INT32 find_socket_context_from_ssl(SSL *ssl);
#endif
QOSAL_UINT32 (*dhcps_success_callback)(QOSAL_UINT8 *mac, QOSAL_UINT32 ip) = NULL;
QOSAL_UINT32 (*dhcpc_success_callback)(QOSAL_UINT32 ip,QOSAL_UINT32 mask,QOSAL_UINT32 gw) = NULL;

QOSAL_UINT32 dns_block_time =(DHCP_WAIT_TIME *2);

QOSAL_UINT32 custom_queue_empty(QOSAL_UINT32 index);



/*****************************************************************************/
/*  custom_socket_context_init - Initializes the custom section of socket context.
 * RETURNS: A_OK on success or error otherwise. 
 *****************************************************************************/
A_STATUS custom_socket_context_init()
{
    SOCKET_CUSTOM_CONTEXT_PTR pcustctxt = NULL;
    QOSAL_UINT32 index;
    KAL_ERR  err_kal;
    
    for (index = 0; index < MAX_SOCKETS_SUPPORTED + 1; index++)	
    {	      
        pcustctxt = &(((SOCKET_CUSTOM_CONTEXT_PTR)QCA_CtxPtr->SocketCustomCxtPtr)[index]);
        
        A_MEMZERO(pcustctxt, sizeof(SOCKET_CUSTOM_CONTEXT));
      
        pcustctxt->sockRxWakeEvent = KAL_SemCreate((const  CPU_CHAR *) "NetSockRxEvent",
                                                                             DEF_NULL,
                                                                            &err_kal);
         if (err_kal != KAL_ERR_NONE) {
             return A_ERROR;
         }
         pcustctxt->sockTxWakeEvent = KAL_SemCreate((const  CPU_CHAR *) "NetSockTxEvent",
                                                                             DEF_NULL,
                                                                            &err_kal);
         if (err_kal != KAL_ERR_NONE) {
             return A_ERROR;
         }
 		
        A_NETBUF_QUEUE_INIT(&(pcustctxt->rxqueue));
        pcustctxt->blockFlag = 0;
        pcustctxt->respAvailable = QOSAL_FALSE;
        ath_sock_context[index]->sock_context = pcustctxt;
        ath_sock_context[index]->remaining_bytes = 0;
        ath_sock_context[index]->old_netbuf = NULL;
        
    }
    A_NETBUF_QUEUE_INIT(&custom_alloc_queue);            
    #if ZERO_COPY	
    /*Initilize common queue used to store Rx packets for zero copy option*/
    A_NETBUF_QUEUE_INIT(&zero_copy_free_queue);
    #endif
    
    
    return A_OK;
}

/*****************************************************************************/
/*  custom_socket_context_deinit - De-initializes the custom section of socket context.
 * RETURNS: A_OK on success or error otherwise. 
 *****************************************************************************/
A_STATUS custom_socket_context_deinit()
{
	SOCKET_CONTEXT_PTR pcustctxt = NULL;
	QOSAL_UINT32 index;
    RTOS_ERR err;
	
	for (index = 0; index < MAX_SOCKETS_SUPPORTED + 1; index++)	
	{	
           pcustctxt = GET_SOCKET_CONTEXT(ath_sock_context[index]);        		
           KAL_SemDel(pcustctxt->sockRxWakeEvent,&err);
           KAL_SemDel(pcustctxt->sockTxWakeEvent,&err);
	}			

	return A_OK;
}

/*****************************************************************************/
/*  zero_copy_http_free - ZERO_COPY free API.It will find and remove rx buffer 
 *                  for HTTP client.   
 * RETURNS: Nothing
 *****************************************************************************/
QOSAL_VOID zero_copy_http_free(QOSAL_VOID* buffer)
{
    QOSAL_UINT32 *length;
    
    length = (QOSAL_UINT32 *) buffer;
    buffer = (QOSAL_VOID *) (length - 3);
    //zero_copy_free(buffer);
}


/*****************************************************************************/
/*  t_sendto - Custom version of socket send API. Used for datagram sockets.                 
 * 		Sends provided buffer to target. Creates space for headroom at
 *              the begining of the packet. The headroom is used by the stack to 
 *              fill in the TCP-IP header.   
 * RETURNS: Number of bytes sent to target 
 *****************************************************************************/

QOSAL_INT32 Api_sendto(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, QOSAL_UINT8* buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, SOCKADDR_T* name, QOSAL_UINT32 socklength)
{
    QOSAL_UINT8     custom_hdr[TCP6_HEADROOM];
    QOSAL_INT32     index = 0;
    QOSAL_UINT16    hdr_length = sizeof(ATH_MAC_HDR);
    QOSAL_UINT8     hdr[TCP6_HEADROOM];
	QOSAL_INT32     result=0;
	A_STATUS    status = A_OK;
    QOSAL_UINT16    rem_len = length;
    A_NETBUF   *p_a_netbuf;
	

                                                                /* --------------- FIND SOCKET CONTEXT ---------------- */

    p_a_netbuf = A_NETBUF_DEQUEUE_ADV(&custom_alloc_queue, buffer);
    A_NETBUF_PUT(p_a_netbuf,length);
#if !NON_BLOCKING_TX    
    A_NETBUF_ENQUEUE(&custom_alloc_queue,  p_a_netbuf) ;
#endif    
	if((index = find_socket_context(handle,TRUE)) == SOCKET_NOT_FOUND)
	{
#if NON_BLOCKING_TX
        A_NETBUF_FREE(p_a_netbuf);
#endif
		last_driver_error = A_SOCKCXT_NOT_FOUND;
		return A_SOCK_INVALID;
	}

#if !NON_BLOCKING_TX
    p_a_netbuf->context = ath_sock_context[index];
#endif

    while (rem_len != 0) {
                                                                /* Give the headroom for the header.                    */
        length = rem_len; 
        A_MEMZERO(custom_hdr, TCP6_HEADROOM);

        if (ath_sock_context[index]->domain == ATH_AF_INET) {
            if (ath_sock_context[index]->type == SOCK_STREAM_TYPE)
                hdr_length = TCP_HEADROOM;
            else
                hdr_length = UDP_HEADROOM;
        } else {
            if(ath_sock_context[index]->type == SOCK_STREAM_TYPE)
                hdr_length = TCP6_HEADROOM_WITH_NO_OPTION;
            else
                hdr_length = UDP6_HEADROOM;
        }
                                                                /* Calculate fragmentation threshold. We cannot send... */
                                                                /* ...packets greater that HTC credit size over HTC,... */
                                                                /* ...Bigger packets need to be fragmented.             */

                                                                /* Thershold are different for IP v4 vs v6.             */
        if (ath_sock_context[index]->domain == ATH_AF_INET6) {
            if (name != NULL) {
                ((SOCK_SEND6_T*)custom_hdr)->name6.sin6_port     = A_CPU2LE16(((SOCKADDR_6_T*)name)->sin6_port);
                ((SOCK_SEND6_T*)custom_hdr)->name6.sin6_family   = A_CPU2LE16(((SOCKADDR_6_T*)name)->sin6_family);
                ((SOCK_SEND6_T*)custom_hdr)->name6.sin6_flowinfo = A_CPU2LE32(((SOCKADDR_6_T*)name)->sin6_flowinfo);
                A_MEMCPY((QOSAL_UINT8*)&(((SOCK_SEND6_T*)custom_hdr)->name6.sin6_addr),(QOSAL_UINT8*)&((SOCKADDR_6_T*)name)->sin6_addr,sizeof(IP6_ADDR_T));
                ((SOCK_SEND6_T*)custom_hdr)->socklength          = A_CPU2LE32(socklength);
            } else {
                A_MEMZERO((&((SOCK_SEND6_T*)custom_hdr)->name6),sizeof(name));
            }
        } else {
            if (name != NULL) {
                 ((SOCK_SEND_T*)custom_hdr)->name.sin_port     = A_CPU2LE16(((SOCKADDR_T*)name)->sin_port);
                 ((SOCK_SEND_T*)custom_hdr)->name.sin_family   = A_CPU2LE16(((SOCKADDR_T*)name)->sin_family);
                 ((SOCK_SEND_T*)custom_hdr)->name.sin_addr     = A_CPU2LE32(((SOCKADDR_T*)name)->sin_addr);
                 ((SOCK_SEND_T*)custom_hdr)->socklength        = A_CPU2LE32(socklength);
            }else{
                A_MEMZERO((QOSAL_UINT8*)(&((SOCK_SEND_T*)custom_hdr)->name),sizeof(name));
            }
        }
                                                                /* Populate common fields of custom header, these are the same for IP v4/v6*/
        ((SOCK_SEND_T*)custom_hdr)->handle = A_CPU2LE32(handle);
        ((SOCK_SEND_T*)custom_hdr)->flags  = A_CPU2LE32(flags);
  
        A_MEMZERO(hdr, hdr_length);		
	
        if((length == 0) || (length > IPV4_FRAGMENTATION_THRESHOLD)) {
          /*Host fragmentation is not allowed, application must send payload
           below IPV4_FRAGMENTATION_THRESHOLD size*/
            A_ASSERT(0);
        }
        
        ((SOCK_SEND_T*)custom_hdr)->length = A_CPU2LE32(length);
        
        rem_len -= length;
        result += length;

        A_NETBUF_PUSH_DATA(p_a_netbuf,(QOSAL_UINT8*)custom_hdr, hdr_length);
        //ensure there is enough headroom to complete the tx operation
        if (A_NETBUF_HEADROOM(p_a_netbuf) < sizeof(ATH_MAC_HDR) + sizeof(ATH_LLC_SNAP_HDR) +
                    sizeof(WMI_DATA_HDR) + HTC_HDR_LENGTH + WMI_MAX_TX_META_SZ) {
            goto END_TX;

        }
        
        status = Api_DataTxStart(pCxt, (QOSAL_VOID*)p_a_netbuf);
        if (status != A_OK) {  
           goto END_TX;
        }

#if !NON_BLOCKING_TX	
        /*Wait till packet is sent to target*/		
        if (BLOCK(pCxt,ath_sock_context[index], TRANSMIT_BLOCK_TIMEOUT, TX_DIRECTION) != A_OK)
        {
           result = A_ERROR;
           goto END_TX;	
        }                
#endif		
    }
END_TX: 
  
    if (status != A_OK) {   
#if NON_BLOCKING_TX
        A_NETBUF_FREE(p_a_netbuf);
#endif  
        result = A_ERROR;
    }
#if !NON_BLOCKING_TX	
     p_a_netbuf->data = p_a_netbuf->head + AR6000_DATA_OFFSET + TCP6_HEADROOM;
     p_a_netbuf->tail = p_a_netbuf->data;
#endif
	return result;	
}
#if ZERO_COPY

/*****************************************************************************/
/*  zero_copy_free - ZERO_COPY free API.It will find and remove rx buffer from
 *                  a common queue.   
 * RETURNS: Number of bytes received or A_ERROR in case of failure
 *****************************************************************************/
QOSAL_VOID zero_copy_free(QOSAL_VOID* buffer)
{
	A_NETBUF* a_netbuf_ptr = NULL;
	
	/*find buffer in zero copy queue*/
	a_netbuf_ptr = A_NETBUF_DEQUEUE_ADV(&zero_copy_free_queue, buffer);
	
	if(a_netbuf_ptr != NULL)
		A_NETBUF_FREE(a_netbuf_ptr);
}

#endif

/*****************************************************************************/
/*  Api_recvfrom - ZERO_COPY version of receive API.It will check socket's
 *				receive quese for pending packets. If a packet is available,
 *				it will be passed to the application without a memcopy. The 
 *				application must call a zero_copy_free API to free this buffer.
 * RETURNS: Number of bytes received or A_ERROR in case of failure
 *****************************************************************************/


/*****************************************************************************/
/*  Api_recvfrom - Non ZERO_COPY version of receive API.It will check socket's
 *				receive quese for pending packets. If a packet is available,
 *				it will be copied into user provided buffer.
 * RETURNS: Number of bytes received or A_ERROR in case of failure
 *****************************************************************************/
#if ZERO_COPY
QOSAL_INT32 Api_recvfrom(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, void** buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, QOSAL_VOID* name, QOSAL_UINT32* socklength)
#else
QOSAL_INT32 Api_recvfrom(QOSAL_VOID *pCxt, QOSAL_UINT32 handle, void* buffer, QOSAL_UINT32 length, QOSAL_UINT32 flags, QOSAL_VOID* name, QOSAL_UINT32* socklength)
#endif
{
	QOSAL_INT32                    index;
	QOSAL_UINT32                   len = 0;
	QOSAL_UINT32                   hdrlen=0;
	A_NETBUF                  *a_netbuf_ptr = NULL;
	QOSAL_UINT8*                   data = NULL;
	SOCKET_CONTEXT_PTR  pcustctxt;
	
	/*Find context*/
	if((index = find_socket_context(handle,TRUE)) == SOCKET_NOT_FOUND)
	{
		return A_SOCK_INVALID;
	}
	pcustctxt = GET_SOCKET_CONTEXT(ath_sock_context[index]);
	
    if((ath_sock_context[index]->type            == SOCK_STREAM_TYPE) &&
       (ath_sock_context[index]->remaining_bytes != 0               ) &&
       (ath_sock_context[index]->old_netbuf      != NULL            )) {

        a_netbuf_ptr = ath_sock_context[index]->old_netbuf;
        data         = (QOSAL_UINT8*)A_NETBUF_DATA(a_netbuf_ptr);
    } else {
        while((a_netbuf_ptr = A_NETBUF_DEQUEUE(&(pcustctxt->rxqueue))) == NULL) {
#if NON_BLOCKING_RX
                                                                /* No packet is available, return -1                    */
            return A_ERROR;
#else          
            if (ath_sock_context[index]->TCPCtrFlag == TCP_FIN) {
                clear_socket_context(index);
                return A_SOCK_INVALID;

            }
                                                                /* No packet available, block                           */
            if(BLOCK(pCxt, ath_sock_context[index], 0, RX_DIRECTION) != A_OK)
            {
                                                                /* Check if Peer closed socket while we were waiting    */
                if(ath_sock_context[index]->handle == 0) {
                    return A_SOCK_INVALID;
                }
                if (ath_sock_context[index]->TCPCtrFlag == TCP_FIN) {
                    clear_socket_context(index);
                    return A_SOCK_INVALID;
                }
                return A_ERROR;
            }
#endif
        }
    }

    /*Extract custom header*/
    data = (QOSAL_UINT8*)A_NETBUF_DATA(a_netbuf_ptr);
    if(ath_sock_context[index]->domain == ATH_AF_INET)
    {
        hdrlen = sizeof(SOCK_RECV_T);
        /*If we are receiving data for a UDP socket, extract sockaddr info*/
        if(ath_sock_context[index]->type == SOCK_DGRAM_TYPE)
        {
            A_MEMCPY(name, &((SOCK_RECV_T*)data)->name, sizeof(SOCKADDR_T));
            ((SOCKADDR_T*)name)->sin_port = A_CPU2LE16(((SOCKADDR_T*)name)->sin_port);
            ((SOCKADDR_T*)name)->sin_family = A_CPU2LE16(((SOCKADDR_T*)name)->sin_family);
            ((SOCKADDR_T*)name)->sin_addr = A_CPU2LE32(((SOCKADDR_T*)name)->sin_addr);
            *socklength = sizeof(SOCKADDR_T);
        }
    }
    else if(ath_sock_context[index]->domain == ATH_AF_INET6)
    {
        hdrlen = sizeof(SOCK_RECV6_T);
        /*If we are receiving data for a UDP socket, extract sockaddr info*/
        if(ath_sock_context[index]->type == SOCK_DGRAM_TYPE)
        {
            A_MEMCPY(name, &((SOCK_RECV6_T*)data)->name6, sizeof(SOCKADDR_6_T));
            *socklength = sizeof(SOCKADDR_6_T);
        }
    }
    else
    {
        A_ASSERT(0);
    }

    if(!((ath_sock_context[index]->type == SOCK_STREAM_TYPE) &&
         (ath_sock_context[index]->remaining_bytes != 0) &&
         (ath_sock_context[index]->old_netbuf != NULL)))
    {
        /*Remove custom header from payload*/
        A_NETBUF_PULL(a_netbuf_ptr, hdrlen);
    }

	len = A_NETBUF_LEN(a_netbuf_ptr);

    if(ath_sock_context[index]->type == SOCK_STREAM_TYPE)
    {
        if(len > length)
        {
            ath_sock_context[index]->remaining_bytes = (len - length);
            len = length;
        }
        else
        {
            ath_sock_context[index]->remaining_bytes = 0;
            ath_sock_context[index]->old_netbuf = NULL;
        }
    }
    else
    {
        if(len > length)
        {
            len = length;  //Discard excess bytes
        }
    }

#if ZERO_COPY
   *buffer = (QOSAL_UINT8*)A_NETBUF_DATA(a_netbuf_ptr);
#else
    A_MEMCPY((QOSAL_UINT8*)buffer,(QOSAL_UINT8*)A_NETBUF_DATA(a_netbuf_ptr), len);
#endif
    
    if(ath_sock_context[index]->type == SOCK_STREAM_TYPE)
    {
        if(ath_sock_context[index]->remaining_bytes == 0)
        {
#if ZERO_COPY          
            A_NETBUF_ENQUEUE(&zero_copy_free_queue, a_netbuf_ptr);
#else 
            A_NETBUF_FREE(a_netbuf_ptr);
#endif
        }
        else
        {
            A_NETBUF_PULL(a_netbuf_ptr, len);
            ath_sock_context[index]->old_netbuf = a_netbuf_ptr;
        }
    }
    else
    {
#if ZERO_COPY         
        A_NETBUF_ENQUEUE(&zero_copy_free_queue, a_netbuf_ptr);
#else 
        A_NETBUF_FREE(a_netbuf_ptr);
#endif
    }

	return len;
}

/*****************************************************************************/
/*  custom_alloc - API for application to allocate a TX buffer. Here we ensure that 
              enough resources are available for a packet TX. All allocations 
             for a TX operation are made here. This includes Headroom, DB and netbuf.
             If any allocation fails, this API returns NULL, and the host must 
             wait for some time to allow memory to be freed. 
      Note 1-  Allocation may fail if too many packets are queued up for Transmission,
               e.g. in the the non-blocking TX case.
      Note 2- This API should ONLY be used for TX packets.

 * RETURNS: pointer to payload buffer for success and NULL for failure 
 *****************************************************************************/


QOSAL_VOID* custom_alloc(QOSAL_UINT32 size)
{
    QOSAL_UINT8      *p_buf;
    A_NETBUF     *p_a_netbuf;
    
    p_a_netbuf = (A_NETBUF *)A_NETBUF_ALLOC(size + TCP6_HEADROOM);
    if (p_a_netbuf == NULL) {
        return DEF_NULL; 
    }
    A_NETBUF_PUT(p_a_netbuf, TCP6_HEADROOM);
    A_NETBUF_PULL(p_a_netbuf, TCP6_HEADROOM);
    p_buf = A_NETBUF_DATA(p_a_netbuf);
    
    A_NETBUF_ENQUEUE(&custom_alloc_queue, p_a_netbuf);
                                                                /* --------------- FIND SOCKET CONTEXT ---------------- */
    p_a_netbuf->pool_id = A_TX_NET_POOL;
    
    return p_buf;
}


/*****************************************************************************/
/*  custom_free - API for application to free TX buffer. It should ONLY be called
 *              by the app if Blocking TX mode is enabled. It will free all allocations
 *              made during the custom_alloc 
 * RETURNS: none 
 *****************************************************************************/
QOSAL_VOID custom_free(QOSAL_VOID* buf)
{
	A_NETBUF* a_netbuf_ptr = NULL;
	
	/*find buffer in zero copy queue*/
	a_netbuf_ptr = A_NETBUF_DEQUEUE_ADV(&custom_alloc_queue, buf);
	
	if (a_netbuf_ptr != NULL) {
        QCA_BufBlkFree(a_netbuf_ptr->buf_pool, a_netbuf_ptr); 
        Driver_ReportRxBuffStatus(QCA_CtxPtr, TRUE);
    }

}

/*****************************************************************************/
/*  get_total_pkts_buffered - Returns number of packets buffered across all
 *          socket queues.           
 * RETURNS: number of packets 
 *****************************************************************************/
QOSAL_UINT32 get_total_pkts_buffered()
{
	QOSAL_UINT32 index;
	QOSAL_UINT32 totalPkts = 0;
	
	SOCKET_CONTEXT_PTR pcustctxt;
	
	for(index = 0; index < MAX_SOCKETS_SUPPORTED; index++)
	{
		pcustctxt = GET_SOCKET_CONTEXT(ath_sock_context[index]);
		totalPkts += A_NETBUF_QUEUE_SIZE(&(pcustctxt->rxqueue));
	}
	return totalPkts;
}

/*****************************************************************************/
/*  custom_queue_empty - Checkes whether a socket queue is empty
 * RETURNS: 1 - empty or 0 - not empty 
 *****************************************************************************/
QOSAL_UINT32 custom_queue_empty(QOSAL_UINT32 index)
{
	SOCKET_CONTEXT_PTR pcustctxt;
	pcustctxt = GET_SOCKET_CONTEXT(ath_sock_context[index]);
		
	return A_NETBUF_QUEUE_EMPTY(&(pcustctxt->rxqueue));
}

#if ENABLE_SSL
/*****************************************************************************/
/* SSL_ctx_new - Create new SSL context. This function must be called before
 *               using any other SSL functions. It needs to be called as either
 *               server or client
 * Sslrole role - 1 = server, 2 = client
 * QOSAL_INT32 inbufSize - initial inBuf size: Can grow
 * QOSAL_INT32 outbufSize - outBuf size: Fixed
 * QOSAL_INT32 reserved - currently not used (must be zero)
 * Returns - SSL context handle on success or NULL on error (out of memory)
 *****************************************************************************/
SSL_CTX* SSL_ctx_new(SSL_ROLE_T role, QOSAL_INT32 inbufSize, QOSAL_INT32 outbufSize, QOSAL_INT32 reserved)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

    /*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return NULL;
    }
	return (Api_SSL_ctx_new(pCxt,role,inbufSize,outbufSize,reserved));
}

/*****************************************************************************/
/* SSL_ctx_free - Free the SSL context
 * SSL_CTX *ctx - sslContext
 *****************************************************************************/
QOSAL_INT32 SSL_ctx_free(SSL_CTX *ctx)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

    /*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
	return (Api_SSL_ctx_free(pCxt,ctx));
}

/*****************************************************************************/
/* SSL_new - Create SSL connection object. When SSL transaction is done, close
 *           it with ssl_shutdown().
 *           Note that the socket should subsequently also be closed.
 * SSL_CTX *ctx - sslContext
 * Return - SSL object handle on success or NULL on error (out of memory)
 *****************************************************************************/
SSL* SSL_new(SSL_CTX *ctx)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

    
	/*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return NULL;
    }
	return (Api_SSL_new(pCxt,ctx));
}

/*****************************************************************************/
/* SSL_set_fd - Attach given socket descriptor to the SSL connection
 * SSL *ssl - SSL connection
 * QOSAL_UINT32 fd - Socket descriptor
 * Return - 1 on success or negative error code on error (see SslErrors)
 *****************************************************************************/
QOSAL_INT32 SSL_set_fd(SSL *ssl, QOSAL_UINT32 fd)
{
   QOSAL_VOID *pCxt = QCA_CtxPtr;
   

	/*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
	return (Api_SSL_set_fd(pCxt,ssl,fd));
}

/*****************************************************************************/
/* SSL_accept - Initiate SSL handshake.
 * SSL *ssl - SSL connection
 * Returns - 1 on success, ESSL_HSDONE if handshake has already been performed.
 *           Negative error code otherwise.
 *****************************************************************************/
QOSAL_INT32 SSL_accept(SSL *ssl)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

	/*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
	return (Api_SSL_accept(pCxt,ssl));
}

/*****************************************************************************/
/* SSL_connect - Initiate SSL handshake.
 * SSL *ssl - SSL connection
 * Returns - 1 on success, ESSL_HSDONE if handshake has already been performed.
 *           Negative error code otherwise.
 *****************************************************************************/
QOSAL_INT32 SSL_connect(SSL *ssl)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    
    
	/*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
	return (Api_SSL_connect(pCxt,ssl));
}

/*****************************************************************************/
/* SSL_shutdown - Close SSL connection.
 *                The socket must be closed by other means.
 * SSL *ssl - SSL connection
 * Returns - 1 on success or negative error code on error (see SslErrors)
 *****************************************************************************/
QOSAL_INT32 SSL_shutdown(SSL *ssl)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


	/*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
	return (Api_SSL_shutdown(pCxt,ssl));
}

/*****************************************************************************/
/* SSL_configure - Configure a SSL connection.
 * SSL *ssl - SSL connection
 * SSL_CONFIG *cfg - pointer to struct holding the SSL configuration.
 * Returns - 1 on success or negative error code on error (see SslErrors)
 *****************************************************************************/
QOSAL_INT32 SSL_configure(SSL *ssl, SSL_CONFIG *cfg)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;  
    

    /*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return (Api_SSL_configure(pCxt, ssl, cfg));
}

/*****************************************************************************/
/* SSL_setCaList - Set a Certificate Authority (CA) list so SSL can perform
 *                 certificate validation on the peer's certificate.
 *                 You can only set one CA list, thus the CA list must include
 *                 all root certificates required for the session.
 * SSL *ssl - SSL connection
 * SslCert cert -address of array of binary data
 * QOSAL_UINT32 size - size of array
 * Returns - 1 on success or negative error code on error (see SslErrors)
 *****************************************************************************/
QOSAL_INT32 SSL_setCaList(SSL_CTX *ctx, SslCAList caList, QOSAL_UINT32 size)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

    /*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return (Api_SSL_addCert(pCxt, ctx, SSL_CA_LIST, (QOSAL_UINT8*)caList, size));
}

/*****************************************************************************/
/* SSL_addCert - Add a certificate to the SharkSsl object. A SharkSsl object
 *               in server mode is required to have at least one certificate.
 * SSL *ssl - SSL connection
 * SslCert cert -address of array of binary data
 * QOSAL_UINT32 size - size of array
 * Returns - 1 on success or negative error code on error (see SslErrors)
 *****************************************************************************/
QOSAL_INT32 SSL_addCert(SSL_CTX *ctx, SslCert cert, QOSAL_UINT32 size)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

	/*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
	return (Api_SSL_addCert(pCxt, ctx, SSL_CERTIFICATE, (QOSAL_UINT8*)cert, size));
}

/*****************************************************************************/
/* SSL_storeCert - Store a certificate or CA list in FLASH.
 * A_CHAR *name - the name
 * SslCert cert -address of array of binary data
 * QOSAL_UINT32 size - size of array
 * Returns - 1 on success or negative error code on error (see SslErrors)
 *****************************************************************************/
QOSAL_INT32 SSL_storeCert(A_CHAR *name, SslCert cert, QOSAL_UINT32 size)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

    /*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return (Api_SSL_storeCert(pCxt, name, (QOSAL_UINT8*)cert, size));
}

/*****************************************************************************/
/* SSL_loadCert - Load a certificate or CA list from FLASH and store it in the
 *                sharkSs object.
 * SSL_CERT_TYPE_T type - certificate or CA list
 * A_CHAR *name - the name
 * Returns - 1 on success or negative error code on error (see SslErrors)
 *****************************************************************************/
QOSAL_INT32 SSL_loadCert(SSL_CTX *ctx, SSL_CERT_TYPE_T type, char *name)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

    /*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return (Api_SSL_loadCert(pCxt, ctx, type, name));
}

QOSAL_INT32 SSL_listCert(SSL_FILE_NAME_LIST *fileNames)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    

    /*Wait for chip to be up*/
    if(A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return (Api_SSL_listCert(pCxt, fileNames));
}

#if ZERO_COPY
/*****************************************************************************/
/*  SSL_read - ZERO_COPY version of SSL read function. It uses the ssl pointer
 *             to find the socket to read from.It will check socket's receive
 *             queues for pending packets.  If a packet is available, it will be
 *             passed to the application without a memcopy. The application must
 *             call a zero_copy_free API to free this buffer.
 * SSL *ssl - pointer to SSL connection object.
 * void **buf - pointer to pointer holding the address of the receive buffer.
 * QOSAL_INT32 num - The max number of bytes to read.
 * Returns - Number of bytes received or A_ERROR in case of failure
 *****************************************************************************/
QOSAL_INT32 SSL_read(SSL *ssl, void **buf, QOSAL_INT32 num)
#else
QOSAL_INT32 SSL_read(SSL *ssl, void *buf, QOSAL_INT32 num)
#endif
{
    /* Find the socket used with this SSL connection */
    QOSAL_INT32 index = find_socket_context_from_ssl(ssl);
    if (index == SOCKET_NOT_FOUND)
    {
        return A_ERROR;
    }

    /* Read data from the socket */
    return Api_recvfrom(QCA_CtxPtr, ath_sock_context[index]->handle, buf, num, 0, NULL, NULL);
}

/*****************************************************************************/
/* SSL_write - Encrypt and send data in buf and send them on the associated
 *             socket. The ssl pointer is sued to find the socket to send on.
 * SSL *ssl - pointer to SSL connection object.
 * void *buf - pointer to buffer holding the data to send.
 * QOSAL_INT32 num  - The number of bytes to send.
 * Returns - Number of bytes sent on success or negative error code on error
 *****************************************************************************/
QOSAL_INT32 SSL_write(SSL *ssl, const void *buf, QOSAL_INT32 num)
{
    /* Find the socket used with this SSL connection */
    QOSAL_INT32 index = find_socket_context_from_ssl(ssl);
    if (index == SOCKET_NOT_FOUND)
    {
        return A_ERROR;
    }

    /* Send the data */
    return Api_sendto(QCA_CtxPtr, ath_sock_context[index]->handle, (QOSAL_UINT8 *)buf, num, 0, NULL, NULL);
}

#endif // ENABLE_SSL


QOSAL_INT32
Custom_Api_Dhcps_Success_Callback_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap)
{
    SOCK_DHCPS_SUCCESS_CALLBACK_T *dhcps_cb_info=(SOCK_DHCPS_SUCCESS_CALLBACK_T *)datap;    
    if(dhcps_success_callback){
        dhcps_success_callback(dhcps_cb_info->mac,dhcps_cb_info->ip);
    }
	 return QOSAL_OK;
}

QOSAL_INT32
Custom_Api_Dhcpc_Success_Callback_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap)
{
    SOCK_DHCPC_SUCCESS_CALLBACK_T *dhcpc_cb_info=(SOCK_DHCPC_SUCCESS_CALLBACK_T *)datap;    
    if(dhcpc_success_callback){
        dhcpc_success_callback(dhcpc_cb_info->ip,dhcpc_cb_info->mask,dhcpc_cb_info->gw);
    }
	 return QOSAL_OK;
}


QOSAL_VOID
Custom_Api_Http_Post_Event(QOSAL_VOID *pCxt, QOSAL_UINT8 *datap)
{
    WMI_HTTP_POST_EVENT_CB cb = NULL;
        
    DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
    
    cb = (WMI_HTTP_POST_EVENT_CB)(GET_DRIVER_CXT(pCxt)->httpPostCb);
    
    DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);
    
    if(cb){
        cb(GET_DRIVER_CXT(pCxt)->httpPostCbCxt, (QOSAL_VOID*)datap);
    }
}

QOSAL_INT32 custom_http_set_post_cb(QOSAL_VOID *handle, QOSAL_VOID* cxt, QOSAL_VOID* callback)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    
    GET_DRIVER_CXT(pCxt)->httpPostCb = callback;
    GET_DRIVER_CXT(pCxt)->httpPostCbCxt = cxt;
    return QOSAL_OK;
}


QOSAL_INT32 custom_ota_set_response_cb(QOSAL_VOID *handle, QOSAL_VOID* callback)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    
    GET_DRIVER_CXT(pCxt)->otaCB = callback;
    return A_OK;  
}

QOSAL_VOID Custom_Api_Ota_Resp_Result(QOSAL_VOID *pCxt, QOSAL_UINT32 cmd, QOSAL_UINT32 resp_code, QOSAL_UINT32 result)
{
    ATH_OTA_CB cb = NULL;
    DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
    cb = (ATH_OTA_CB)(GET_DRIVER_CXT(pCxt)->otaCB);
    DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);
    
    if(cb){
        cb(cmd, resp_code, result);
    }
}
#endif
