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
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

#ifndef _CUSTOM_STACK_OFFLOAD_H_
#define _CUSTOM_STACK_OFFLOAD_H_

#if ENABLE_STACK_OFFLOAD

#include <atheros_stack_offload.h>	 

#define IS_SOCKET_BLOCKED(index)  \
        isSocketBlocked((index))

/* Headroom definitions*/
#define UDP_HEADROOM 44 
#define TCP_HEADROOM 64
#define UDP6_HEADROOM 64
#define TCP6_HEADROOM 88
#define TCP6_HEADROOM_WITH_NO_OPTION 84

/***Custom socket context. This must be adjusted based on the underlying OS ***/
typedef struct socket_custom_context {
    KAL_SEM_HANDLE     sockRxWakeEvent;	//Event to unblock waiting RX socket
    KAL_SEM_HANDLE     sockTxWakeEvent;	//Event to unblock waiting TX socket
    A_NETBUF_QUEUE_T   rxqueue;     	//Queue to hold incoming packets	
    QOSAL_UINT8            blockFlag;
    QOSAL_BOOL             respAvailable;   //Flag to indicate a response from target is available
    QOSAL_BOOL             txUnblocked;
    QOSAL_BOOL             txBlock;
    QOSAL_BOOL             rxBlock;
} SOCKET_CUSTOM_CONTEXT, *SOCKET_CUSTOM_CONTEXT_PTR;


//#if ZERO_COPY
extern A_NETBUF_QUEUE_T zero_copy_free_queue;
#endif

QOSAL_UINT32 isSocketBlocked(QOSAL_VOID* ctxt);
A_STATUS custom_socket_context_init();
A_STATUS custom_socket_context_deinit();
QOSAL_UINT32 custom_receive_tcpip(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
void     txpkt_free(QOSAL_VOID* buffPtr);

QOSAL_VOID   custom_free(QOSAL_VOID* buf);
QOSAL_VOID*  custom_alloc(QOSAL_UINT32 size);
QOSAL_UINT32 get_total_pkts_buffered();
QOSAL_UINT32 custom_queue_empty(QOSAL_UINT32 index);

#endif //ENABLE_STACK_OFFLOAD
