/*
*********************************************************************************************************
*                                              uC/TCP-IP
*                                      The Embedded TCP/IP Suite
*
*                          (c) Copyright 2003-2014; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/TCP-IP is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                        NETWORK DEVICE DRIVER
*
*                                               QCA400X
*
* Filename      :
* Version       : V1.00.00
* Programmer(s) : AL
*********************************************************************************************************
* Note(s)       : (1) The net_dev_ar4100 module is essentially a port of the Qualcomm Atheros driver.
*                     Please see the Qualcomm Atheros copyright below and on every other document
*                     provided with this WiFi driver.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      QUALCOMM/ATHEROS COPYRIGHT
*********************************************************************************************************
*/
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

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/


#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <netbuf.h>
#include <driver_cxt.h>
#include <wmi_api.h>
#include <custom_stack_offload.h>
#include <common_stack_offload.h>

extern A_NETBUF_QUEUE_T custom_alloc_queue;

#define AR6000_DATA_OFFSET    64

QOSAL_VOID a_netbuf_init (QOSAL_VOID *p_ctx,
                      QOSAL_VOID *p_buf, 
                      QOSAL_VOID *p_data_buf)
{
	A_NETBUF     *p_a_netbuf;
                                                                /* ---------- SET A_NETBUF PTR WITH DEV CFG ----------- */
    p_a_netbuf       = (A_NETBUF*) p_buf;
    p_a_netbuf->head = (QOSAL_UINT8 *) p_data_buf;
	p_a_netbuf->data = (QOSAL_UINT8 *)((QOSAL_UINT32)p_a_netbuf->head + AR6000_DATA_OFFSET);
	p_a_netbuf->tail =  p_a_netbuf->data;
    p_a_netbuf->end  =  p_a_netbuf->data;
}


QOSAL_VOID *a_netbuf_alloc(QOSAL_INT32 size)
{
    A_NETBUF        *p_a_netbuf;
    QCA_CTX         *p_qca;
    QOSAL_UINT8         *p_buf;
    QCA_BUF_POOL    *p_pool;
                                                                /* -------------------- GET NET IF -------------------- */
    p_qca = QCA_CtxPtr;
                                                                /* ----------- OBTAIN PTR TO NEW A_NETBUF ------------- */    
    p_pool = QCA_BufPoolGet(p_qca->BufPools,
                            p_qca->Dev_Cfg->NbBufPool, 
                            size);
    
    A_ASSERT(p_pool != DEF_NULL);
     
                                                                
    p_buf = (QOSAL_UINT8 *)QCA_BufBlkGet(p_pool, 
                                     size,
                                     DEF_YES);
    if (p_buf == DEF_NULL) {
        return p_buf;
    }
                                                                /* ------------ A_NET_BUF INITIALIZATION -------------- */
    p_a_netbuf = (A_NETBUF *)p_buf;
    p_buf     += sizeof(A_NETBUF);
    
    A_NETBUF_CONFIGURE(p_a_netbuf, p_buf,  AR6000_DATA_OFFSET, 0u,  p_pool->Pool.BlkSize);
    
    p_a_netbuf->pool_id  = A_TX_NET_POOL;
    p_a_netbuf->buf_pool = p_pool;  
    
    return ((QOSAL_VOID *)p_a_netbuf);
}

QOSAL_VOID *a_netbuf_alloc_raw(QOSAL_INT32 size)
{
    return (QOSAL_VOID *)DEF_NULL;
}

QOSAL_VOID a_netbuf_free(QOSAL_VOID* buffptr)
{
    A_NETBUF    *p_a_netbuf;
    QOSAL_VOID      *p_ctx;
    
    p_ctx      = QCA_CtxPtr; 
    p_a_netbuf = (A_NETBUF *) buffptr;
    
#if !NON_BLOCKING_TX    
    if ((p_a_netbuf->trans.epid == 2u           ) && 
        (p_a_netbuf->pool_id    == A_TX_NET_POOL)) {
         UNBLOCK(p_a_netbuf->context, TX_DIRECTION);
    } else { 
        QCA_BufBlkFree(p_a_netbuf->buf_pool, p_a_netbuf); 
        Driver_ReportRxBuffStatus(p_ctx, TRUE);
    }
#else 
    QCA_BufBlkFree(p_a_netbuf->buf_pool, p_a_netbuf); 
    Driver_ReportRxBuffStatus(p_ctx, TRUE);
#endif
    
}

QOSAL_UINT32 a_netbuf_to_len(QOSAL_VOID *bufPtr)
{
	A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
    QOSAL_UINT32 len = (QOSAL_UINT32)a_netbuf_ptr->tail - (QOSAL_UINT32)a_netbuf_ptr->data;    
    
    return len;
}

/* returns a buffer fragment of a packet.  If the packet is not 
 * fragmented only index == 0 will return a buffer which will be 
 * the whole packet. pLen will hold the length of the buffer in 
 * bytes.
 */
QOSAL_VOID*
a_netbuf_get_fragment(QOSAL_VOID  *bufPtr, 
                      QOSAL_UINT8  index, 
                      QOSAL_INT32 *pLen)
{
	void* pBuf = NULL;
	A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
	
	if (0==index){
		pBuf = a_netbuf_to_data(bufPtr);
		*pLen = (QOSAL_INT32)((QOSAL_UINT32)a_netbuf_ptr->tail - (QOSAL_UINT32)a_netbuf_ptr->data);
	} else {	
	}
	
	return pBuf;	
}

QOSAL_VOID a_netbuf_configure(QOSAL_VOID   *buffptr,
                          QOSAL_VOID   *buffer, 
                          QOSAL_UINT16  headroom, 
                          QOSAL_UINT16  length, 
                          QOSAL_UINT16  size)
{
	A_NETBUF* a_netbuf_ptr = (A_NETBUF*)buffptr;
	
	A_MEMZERO(a_netbuf_ptr, sizeof(A_NETBUF));
	
	if(buffer != NULL){
	    a_netbuf_ptr->head = buffer;
        a_netbuf_ptr->data = &(((QOSAL_UINT8*)buffer)[headroom]);
	    a_netbuf_ptr->tail = &(((QOSAL_UINT8*)buffer)[headroom+length]);
        a_netbuf_ptr->end = &(((QOSAL_UINT8*)buffer)[size]);
    }
}

QOSAL_VOID *a_netbuf_to_data(QOSAL_VOID *bufPtr)
{
    return (((A_NETBUF*)bufPtr)->data);
}

/*
 * Add len # of bytes to the beginning of the network buffer
 * pointed to by bufPtr
 */
A_STATUS a_netbuf_push(QOSAL_VOID  *bufPtr,
                       QOSAL_INT32  len)
{
    A_NETBUF *a_netbuf_ptr = (A_NETBUF*)bufPtr;
    
    if((QOSAL_UINT32)a_netbuf_ptr->data - (QOSAL_UINT32)a_netbuf_ptr->head < len) 
    {
    	A_ASSERT(0);
    }
    
    a_netbuf_ptr->data = (pointer)(((QOSAL_UINT32)a_netbuf_ptr->data) - len);    

    return A_OK;
}
/*
 * Add len # of bytes to the beginning of the network buffer
 * pointed to by bufPtr and also fill with data
 */
A_STATUS a_netbuf_push_data(QOSAL_VOID  *bufPtr, 
                            QOSAL_UINT8 *srcPtr, 
                            QOSAL_INT32  len)
{
    a_netbuf_push(bufPtr, len);
    A_MEMCPY(((A_NETBUF*)bufPtr)->data, srcPtr, (QOSAL_UINT32)len);

    return A_OK;
}
/*
 * Add len # of bytes to the end of the network buffer
 * pointed to by bufPtr
 */
A_STATUS a_netbuf_put(QOSAL_VOID  *bufPtr, 
                      QOSAL_INT32  len)
{
    A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
    
    if((QOSAL_UINT32)a_netbuf_ptr->end - (QOSAL_UINT32)a_netbuf_ptr->tail < len) {
    	A_ASSERT(0);
    }
    
    a_netbuf_ptr->tail = (pointer)(((QOSAL_UINT32)a_netbuf_ptr->tail) + len);
    
    return A_OK;
}

/*
 * Add len # of bytes to the end of the network buffer
 * pointed to by bufPtr and also fill with data
 */
A_STATUS a_netbuf_put_data(QOSAL_VOID  *bufPtr, 
                           QOSAL_UINT8 *srcPtr, 
                           QOSAL_INT32  len)
{
    A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
    void *start = a_netbuf_ptr->tail;
    
    a_netbuf_put(bufPtr, len);
    A_MEMCPY(start, srcPtr, (QOSAL_UINT32)len);

    return A_OK;
}

/*
 * Returns the number of bytes available to a a_netbuf_push()
 */
QOSAL_INT32 a_netbuf_headroom(QOSAL_VOID *bufPtr)
{
    A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
    
    return (QOSAL_INT32)((QOSAL_UINT32)a_netbuf_ptr->data - (QOSAL_UINT32)a_netbuf_ptr->head);
}

QOSAL_INT32 a_netbuf_tailroom(QOSAL_VOID *bufPtr)
{
	A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
    
    return (QOSAL_INT32)((QOSAL_UINT32)a_netbuf_ptr->end - (QOSAL_UINT32)a_netbuf_ptr->tail);	
}

/*
 * Removes specified number of bytes from the beginning of the buffer
 */
A_STATUS a_netbuf_pull(QOSAL_VOID  *bufPtr, 
                       QOSAL_INT32  len)
{
    A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
    
    if((QOSAL_UINT32)a_netbuf_ptr->tail - (QOSAL_UINT32)a_netbuf_ptr->data < len) {
    	A_ASSERT(0);
    }

    a_netbuf_ptr->data = (pointer)((QOSAL_UINT32)a_netbuf_ptr->data + len);

    return A_OK;
}

/*
 * Removes specified number of bytes from the end of the buffer
 */
A_STATUS a_netbuf_trim(QOSAL_VOID  *bufPtr,
                       QOSAL_INT32  len)
{
    A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
    
    if((QOSAL_UINT32)a_netbuf_ptr->tail - (QOSAL_UINT32)a_netbuf_ptr->data < len) {
    	A_ASSERT(0);
    }

    a_netbuf_ptr->tail = (pointer)((QOSAL_UINT32)a_netbuf_ptr->tail - len);

    return A_OK;
}
#if 0
QOSAL_VOID* a_malloc(QOSAL_INT32 size, 
                 QOSAL_UINT8 id)
{
    QOSAL_UINT8      *p_buf;
    A_NETBUF     *p_a_netbuf;
    
    p_a_netbuf = (A_NETBUF *)A_NETBUF_ALLOC(size);
    if (p_a_netbuf == NULL) {
        return DEF_NULL; 
    }
    
    p_buf = A_NETBUF_DATA(p_a_netbuf);
    
    A_NETBUF_ENQUEUE(&custom_alloc_queue, p_a_netbuf);
                                                                /* --------------- FIND SOCKET CONTEXT ---------------- */
    p_a_netbuf->pool_id = A_TX_NET_POOL;
	UNUSED_ARGUMENT(id);
	return (QOSAL_VOID*)p_buf;
}

QOSAL_VOID a_free(QOSAL_VOID  *addr, 
              QOSAL_UINT8  id)
{
  	A_NETBUF* a_netbuf_ptr = NULL;
	
	/*find buffer in zero copy queue*/
	a_netbuf_ptr = A_NETBUF_DEQUEUE_ADV(&custom_alloc_queue, addr);
	
	if (a_netbuf_ptr != NULL) {
        QCA_BufBlkFree(a_netbuf_ptr->buf_pool, a_netbuf_ptr); 
        Driver_ReportRxBuffStatus(QCA_CtxPtr, TRUE);
    }

	UNUSED_ARGUMENT(id);

}
#endif
QOSAL_VOID* a_netbuf_dequeue_adv(A_NETBUF_QUEUE_T *q, QOSAL_VOID *pkt)
{        
    A_NETBUF* curr, *prev;
    
    if(q->head == NULL) return (QOSAL_VOID*)NULL;
    
    curr = (A_NETBUF*)q->head;
    
    while(curr != NULL)
    {
    	if((A_NETBUF*)curr->data == pkt)
    	{
    		/*Match found*/
    		if(curr == (A_NETBUF*)q->head)
    		{
    			/*Match found at head*/
    			q->head = curr->queueLink;
    			break;
    		}
    		else
    		{
    			/*Match found at intermediate node*/
    			prev->queueLink = curr->queueLink;
    			if(q->tail == curr)
    			{
    				/*Last node*/
    				q->tail = prev;
    			}
    			break;   			
    		}
    	}    	
    	prev = curr;
    	curr = curr->queueLink;     	
    }

	if(curr != NULL)
	{
		q->count--;
    	curr->queueLink = NULL;    	
	}
	return (QOSAL_VOID*)curr;    
}

QOSAL_VOID a_netbuf_purge_queue(QOSAL_UINT32 index)
{
	SOCKET_CONTEXT_PTR pcustctxt;
	A_NETBUF_QUEUE_T *q;
	A_NETBUF* temp;
	
	pcustctxt = GET_SOCKET_CONTEXT(ath_sock_context[index]);
	
	q = &pcustctxt->rxqueue;
	
	while(q->count)
	{
		temp = q->head;
		q->head = A_GET_QUEUE_LINK(temp); 
		A_CLEAR_QUEUE_LINK(temp);
		A_NETBUF_FREE(temp);
		q->count--;	
	}
	q->head = q->tail = NULL;
}

