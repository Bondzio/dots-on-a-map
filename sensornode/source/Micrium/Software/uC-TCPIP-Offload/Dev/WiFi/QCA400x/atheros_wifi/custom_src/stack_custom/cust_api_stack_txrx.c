//------------------------------------------------------------------------------
// Copyright (c) 2011 Qualcomm Atheros, Inc.
// All Rights Reserved.
// Qualcomm Atheros Confidential and Proprietary.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is
// hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
// USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <common_api.h>
#include <custom_wlan_api.h>
#include <htc.h>
#include <qca.h>
#include <atheros_wifi_api.h>
#include "cust_netbuf.h"

#if ENABLE_STACK_OFFLOAD
#include "atheros_stack_offload.h"
#include "common_stack_offload.h"
#include "custom_stack_offload.h"
#include "atheros_wifi.h"


QOSAL_UINT32 custom_sock_is_pending(ATH_SOCKET_CONTEXT *sock_context)
{
    QOSAL_UINT32 i, rtn;
    for(rtn = 0, i = 0; i <(SOCK_CMD_LAST/32+1); i++)
    { 
        rtn |= sock_context->sock_st_mask[i];
    }
    
    return rtn;
}

/*****************************************************************************/
/*  custom_receive_tcpip - Called by Custom_DeliverFrameToNetworkStack to 
 *   deliver a data frame to the application thread. It unblocks a waiting 
 *   application thread and queues the packet in the queue of the corresponding 
 *   socket. If socket is not valid, the packet is dropped.
 *      QOSAL_VOID *pCxt - the driver context. 
 *      QOSAL_VOID *pReq - the packet.
 *****************************************************************************/
QOSAL_UINT32 custom_receive_tcpip(QOSAL_VOID *pCxt, QOSAL_VOID *pReq)
{
	A_NETBUF* a_netbuf_ptr = (A_NETBUF*)pReq;	
	QOSAL_INT32 index = 0, handle=0;
	QOSAL_UINT8* data;
	
	SOCKET_CONTEXT_PTR pcustctxt = NULL;
	
    if(a_netbuf_ptr) {        
  	
		data = (QOSAL_UINT8*)A_NETBUF_DATA(a_netbuf_ptr);
	
    	/*Extract socket handle from custom header*/
    	handle = A_CPU2LE32(*((QOSAL_UINT32*)data));
    	
    	/*Get index*/
    	index = find_socket_context(handle, TRUE);
		if(index < 0 || index > MAX_SOCKETS_SUPPORTED)
		{
			last_driver_error = A_SOCKCXT_NOT_FOUND;
			A_NETBUF_FREE(pReq);
			return A_ERROR;
		}

        if(custom_sock_is_pending(ath_sock_context[index]) != 0){
            /* socket is pending a socket event so it cannot 
             * receive data packets */
            last_driver_error = A_SOCK_UNAVAILABLE;
            A_NETBUF_FREE(pReq);
            return A_ERROR;
        }
						
		UNBLOCK_SELECT(pCxt);
		pcustctxt = GET_SOCKET_CONTEXT(ath_sock_context[index]);

        A_NETBUF_ENQUEUE(&(pcustctxt->rxqueue), a_netbuf_ptr);
        UNBLOCK(ath_sock_context[index], RX_DIRECTION);  
    }
    return A_OK;
}

/*****************************************************************************/
/*  Custom_DeliverFrameToNetworkStack - Called by API_RxComplete to 
 *   deliver a data frame to the network stack. This code will perform 
 *   platform specific operations.
 *      QOSAL_VOID *pCxt - the driver context. 
 *      QOSAL_VOID *pReq - the packet.
 *****************************************************************************/
QOSAL_VOID Custom_DeliverFrameToNetworkStack(QOSAL_VOID *pCxt, QOSAL_VOID *pReq)
{
	ATH_PROMISCUOUS_CB prom_cb = (ATH_PROMISCUOUS_CB)(GET_DRIVER_CXT(pCxt)->promiscuous_cb);
    		
    if(GET_DRIVER_COMMON(pCxt)->promiscuous_mode){
        prom_cb(pReq);
    }else{
    	custom_receive_tcpip(pCxt,pReq);
	}     
}

void Custom_APITxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq) 
{ 
#if !NON_BLOCKING
    A_NETBUF* p_a_netbuf = (A_NETBUF*)pReq;
    if (p_a_netbuf->trans.epid == 2) {
        UNBLOCK(p_a_netbuf->context, TX_DIRECTION);
    }
#endif
}
#endif