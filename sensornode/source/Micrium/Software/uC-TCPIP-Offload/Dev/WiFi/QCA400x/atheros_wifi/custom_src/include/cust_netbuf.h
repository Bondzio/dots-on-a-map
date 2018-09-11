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
#ifndef _CUST_NETBUF_H_
#define _CUST_NETBUF_H_


#include <a_types.h>
#include <a_config.h>
#include "../../include/athdefs.h"
#include "../../common_src/include/netbuf.h"
#include <qca.h>
typedef  enum net_dev_pool_type {
    A_RX_NET_POOL = 1u,
    A_TX_NET_POOL = 2u,
}NET_DEV_POOL_TYPE;

#define MAX_BUF_FRAGS 2 /* must match definition of PCB2 */
/*Fragment structure*/
typedef struct db_frag 
{
	QOSAL_UINT32 length;		//Length in bytes
	QOSAL_UINT8* payload;    //Pointer to data
} DB_FRAG, *DB_FRAG_PTR;

typedef struct drv_buffer 
{
	QOSAL_VOID* context;	//Generic placeholder, can be used to store context
	DB_FRAG bufFragment[1];		//data fragment
}DRV_BUFFER, *DRV_BUFFER_PTR ;

typedef struct min_db
{
    QOSAL_VOID*      context;	
    DB_FRAG      bufFragment[MAX_BUF_FRAGS];
} MIN_DB, *MIN_DB_PTR;

typedef struct tx_packet
{
   DRV_BUFFER db;
   QOSAL_UINT8    hdr[88];
   QOSAL_VOID*    a_netbuf_ptr;
} TX_PACKET, *TX_PACKET_PTR;

/* A_NATIVE_NETBUF and A_NATIVE_ORIG are used to point to upper-layer 
 *	netbuf structures. They are OS/TCP stack dependant. */
typedef MIN_DB A_NATIVE_NETBUF;
typedef DRV_BUFFER A_NATIVE_ORIG;


/* A_NETBUF represents the data structure for a pReq object (packet)
 * 	throughout the driver. It is part of the custom implementation 
 *	because there may be more optimal ways to achieve the same results
 *	in other OS/systems. It is expected that any of the elements except
 *	for the A_TRANSPORT_OBJ can be replaced by some OS specific construct
 * 	thereby making more efficient use of resources. */


/* A_NETBUF_SET_TX_POOL - Set pool ID to indicate buffer is part of tx pool  */
#define A_NETBUF_SET_TX_POOL(bufPtr) \
    a_netbuf_set_tx_pool(bufPtr) 

/* A_NETBUF_SET_RX_POOL - Set pool ID to indicate buffer is part of RX pool  */
#define A_NETBUF_SET_RX_POOL(bufPtr) \
    a_netbuf_set_rx_pool(bufPtr)   

#define A_NETBUF_DEQUEUE_ADV(q, pkt) \
   a_netbuf_dequeue_adv((q), (pkt))

#define CUSTOM_PURGE_QUEUE(index) \
   a_netbuf_purge_queue((index)) 
 	         


QOSAL_VOID default_native_free_fn(QOSAL_VOID* pcb_ptr);
QOSAL_VOID a_netbuf_set_tx_pool(QOSAL_VOID* buffptr);
QOSAL_VOID a_netbuf_set_rx_pool(QOSAL_VOID* buffptr);
QOSAL_VOID* a_netbuf_dequeue_adv(A_NETBUF_QUEUE_T *q, QOSAL_VOID *pkt);
QOSAL_VOID a_netbuf_purge_queue(QOSAL_UINT32 index);
QOSAL_VOID* a_netbuf_reinit(QOSAL_VOID* netbuf_ptr, QOSAL_INT32 size);


/* A_NETBUF represents the data structure for a pReq object (packet)
 * 	throughout the driver. It is part of the custom implementation
 *	because there may be more optimal ways to achieve the same results
 *	in other OS/systems. It is expected that any of the elements except
 *	for the A_TRANSPORT_OBJ can be replaced by some OS specific construct
 * 	thereby making more efficient use of resources. */
typedef struct _ath_netbuf {
    NET_DEV_POOL_TYPE  pool_id;
    QCA_BUF_POOL      *buf_pool;
    QOSAL_UINT8     	  *head; // marks the start of head space
    QOSAL_UINT8     	  *data; // marks the start of data space
    QOSAL_UINT8     	  *tail; // marks the start of tail space
    QOSAL_UINT8     	  *end;  // marks the end of packet.
    QOSAL_VOID			  *queueLink; // used to enqueue packets
    QOSAL_VOID*            context;
    A_TRANSPORT_OBJ    trans;       /* container object for netbuf elements */
    
} A_NETBUF, *A_NETBUF_PTR;


/* the A_REQ_... definitions are used in calls to A_NETBUF_GET_ELEM and
 *	A_NETBUF_SET_ELEM.  They identify what element is required. The 
 *	developer can either adopt this implementation or create their
 *	own more suitable definitions as appropriate. */
#define A_REQ_ADDRESS trans.address
#define A_REQ_EPID trans.epid
#define A_REQ_STATUS trans.status
#define A_REQ_CREDITS trans.credits
#define A_REQ_CALLBACK trans.cb
#define A_REQ_COMMAND trans.cmd
#define A_REQ_LOOKAHEAD trans.lookahead
#define A_REQ_TRANSFER_LEN trans.transferLength

#define A_NETBUF_GET_ELEM(_obj, _e) ((A_NETBUF*)(_obj))->_e
#define A_NETBUF_SET_ELEM(_obj, _e, _v) ((A_NETBUF*)(_obj))->_e = (_v)

/* A_CLEAR_QUEUE_LINK - used by netbuf queue code to clear the link. */
#define A_CLEAR_QUEUE_LINK(_pReq) ((A_NETBUF*)(_pReq))->queueLink = NULL
/* A_ASSIGN_QUEUE_LINK - used by netbuf queue code to populate/connect the link. */
#define A_ASSIGN_QUEUE_LINK(_pReq, _pN) ((A_NETBUF*)(_pReq))->queueLink = (_pN)
/* A_GET_QUEUE_LINK - used by netbuf queue code to get the link. */
#define A_GET_QUEUE_LINK(_pReq) ((A_NETBUF*)(_pReq))->queueLink
/*
 * Network buffer support
 */

/* A_NETBUF_DECLARE - declare a NETBUF object on the stack */
#define A_NETBUF_DECLARE A_NETBUF
/* A_NETBUF_CONFIGURE - (re-)configure a NETBUF object */
#define A_NETBUF_CONFIGURE(bufPtr, buffer, headroom, length, size) \
	a_netbuf_configure((bufPtr), (buffer), (headroom), (length), (size))
/* A_NETBUF_ALLOC - allocate a NETBUF object from the "heap". */
#define A_NETBUF_ALLOC(size) \
    a_netbuf_alloc(size)
/* A_NETBUF_ALLOC_RAW - allocate a NETBUF object from the "heap". */
#define A_NETBUF_ALLOC_RAW(size) \
    a_netbuf_alloc_raw(size)
/* A_NETBUF_INIT - (re-)initialize a NETBUF object. */    
#define A_NETBUF_INIT(bufPtr, freefn, priv) \
	a_netbuf_init((bufPtr), (freefn), (priv))
/* A_NETBUF_FREE - release a NETBUF object back to the "heap". */	    
#define A_NETBUF_FREE(bufPtr) \
    a_netbuf_free(bufPtr)
/* A_NETBUF_DATA - Get a pointer to the front of valid/populated NETBUF memory. */
#define A_NETBUF_DATA(bufPtr) \
    a_netbuf_to_data(bufPtr)
/* A_NETBUF_LEN - Get the length of valid/populated NETBUF memory (total for all fragments). */
#define A_NETBUF_LEN(bufPtr) \
    a_netbuf_to_len(bufPtr)
/* A_NETBUF_PUSH - Adds #len bytes to the beginning of valid/populated NETBUF memory */
#define A_NETBUF_PUSH(bufPtr, len) \
    a_netbuf_push(bufPtr, len)
 /* A_NETBUF_PUSH - Adds #len bytes to the beginning of valid/populated NETBUF memory */
#define A_NETBUF_PUSH_DATA(bufPtr,src_buf, len) \
    a_netbuf_push_data(bufPtr, src_buf, len)
/* A_NETBUF_PUT - Adds #len bytes to the end of valid/populated NETBUF memory */
#define A_NETBUF_PUT(bufPtr, len) \
    a_netbuf_put(bufPtr, len)
/* A_NETBUF_TRIM - Removes #len bytes from the end of the valid/populated NETBUF memory */
#define A_NETBUF_TRIM(bufPtr,len) \
    a_netbuf_trim(bufPtr, len)
/* A_NETBUF_PULL - Removes #len bytes from the beginning valid/populated NETBUF memory */
#define A_NETBUF_PULL(bufPtr, len) \
    a_netbuf_pull(bufPtr, len)
/* A_NETBUF_HEADROOM - return the amount of space remaining to prepend content/headers */
#define A_NETBUF_HEADROOM(bufPtr)\
    a_netbuf_headroom(bufPtr)
/* A_NETBUF_TAILROOM - return the amount of space remaining to append content/trailers */
#define A_NETBUF_TAILROOM(bufPtr)\
	a_netbuf_tailroom(bufPtr)    
/* A_NETBUF_GET_FRAGMENT - Get the indexed fragment from the netbuf */
#define A_NETBUF_GET_FRAGMENT(bufPtr, index, pLen) \
	a_netbuf_get_fragment((bufPtr), (index), (pLen))

/* A_NETBUF_APPEND_FRAGMENT - Add a fragment to the end of the netbuf fragment list */
#define A_NETBUF_APPEND_FRAGMENT(bufPtr, frag, len) \
    a_netbuf_append_fragment((bufPtr), (frag), (len))

/* A_NETBUF_PUT_DATA - Add data to end of a buffer  */
#define A_NETBUF_PUT_DATA(bufPtr, srcPtr,  len) \
    a_netbuf_put_data(bufPtr, srcPtr, len) 

/*
 * OS specific network buffer access routines
 */
 
QOSAL_VOID a_netbuf_configure(QOSAL_VOID* buffptr, QOSAL_VOID *buffer, QOSAL_UINT16 headroom, QOSAL_UINT16 length, QOSAL_UINT16 size);
QOSAL_VOID a_netbuf_init(QOSAL_VOID* buffptr, QOSAL_VOID* freefn, QOSAL_VOID* priv);
QOSAL_VOID *a_netbuf_alloc(QOSAL_INT32 size);
QOSAL_VOID *a_netbuf_alloc_raw(QOSAL_INT32 size);
QOSAL_VOID a_netbuf_free(QOSAL_VOID *bufPtr);
QOSAL_VOID *a_netbuf_to_data(QOSAL_VOID *bufPtr);
QOSAL_UINT32 a_netbuf_to_len(QOSAL_VOID *bufPtr);
A_STATUS a_netbuf_push(QOSAL_VOID *bufPtr, QOSAL_INT32 len);
A_STATUS a_netbuf_push_data(QOSAL_VOID *bufPtr, QOSAL_UINT8 *srcPtr, QOSAL_INT32 len);
A_STATUS a_netbuf_put(QOSAL_VOID *bufPtr, QOSAL_INT32 len);
A_STATUS a_netbuf_put_data(QOSAL_VOID *bufPtr, QOSAL_UINT8 *srcPtr, QOSAL_INT32 len);
A_STATUS a_netbuf_pull(QOSAL_VOID *bufPtr, QOSAL_INT32 len);
QOSAL_VOID* a_netbuf_get_fragment(QOSAL_VOID *bufPtr, QOSAL_UINT8 index, QOSAL_INT32 *pLen);
QOSAL_VOID a_netbuf_append_fragment(QOSAL_VOID *bufPtr, QOSAL_UINT8 *frag, QOSAL_INT32 len);
A_STATUS a_netbuf_trim(QOSAL_VOID *bufPtr, QOSAL_INT32 len);
QOSAL_INT32 a_netbuf_headroom(QOSAL_VOID *bufPtr);
QOSAL_INT32 a_netbuf_tailroom(QOSAL_VOID *bufPtr);

#endif /* _CUST_NETBUF_H_ */
