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
#include <wmi.h>
#include <atheros_wifi_api.h>

#include "dset_api.h"

HOST_DSET_HANDLE   dset_handles[MAX_HOST_DSET_SIZE];

A_UINT32 remote_dset_op(DSET_OP op, HOST_DSET_HANDLE *pDsetHandle);

HOST_DSET_HANDLE *dset_find_handle(A_UINT32 dset_id)
{
	A_UINT16	i;
	HOST_DSET_HANDLE  *pDsetHandle = dset_handles;

    for (i=0; i<MAX_HOST_DSET_SIZE; i++)
    {
		if (pDsetHandle->dset_id == dset_id)
			return pDsetHandle; 
    }
 	return NULL;
}

HOST_DSET_HANDLE *dset_insert_handle(A_UINT32 dset_id, A_UINT32 flags, dset_callback_fn_t cb, void *cb_args)
{
	A_UINT16	i;
	HOST_DSET_HANDLE  *pDsetHandle = dset_handles;

    for (i=0; i<MAX_HOST_DSET_SIZE; i++)
    {
		if (pDsetHandle->dset_id == INVALID_DSET_ID)
		{
			pDsetHandle->dset_id = dset_id;
			pDsetHandle->flags = flags;
			pDsetHandle->cb = cb;
            pDsetHandle->cb_args = cb_args;

			return pDsetHandle;
		}; 
    }
 	return NULL;
}

A_UINT32    api_host_dset_create(HOST_DSET_HANDLE **pDsetHandle, A_UINT32 dset_id, A_UINT32 media_id, A_UINT32 length, A_UINT32 flags, dset_callback_fn_t create_cb, void *callback_arg)
{
	A_UINT32		status;

	*pDsetHandle = dset_find_handle(dset_id);
    if (*pDsetHandle != NULL)
		return A_FALSE;

	*pDsetHandle = dset_insert_handle(dset_id, flags, create_cb, callback_arg);
	if (*pDsetHandle == NULL)
		return A_FALSE;

	(*pDsetHandle)->length = length;
	(*pDsetHandle)->media_id = media_id;

	status = remote_dset_op(DSET_OP_CREATE, *pDsetHandle);

	return  status;
}

A_UINT32    api_host_dset_open(HOST_DSET_HANDLE **ppDsetHandle, A_UINT32 dset_id, A_UINT32 flags, dset_callback_fn_t open_cb, void *callback_arg)
{
	A_UINT32		status;

	*ppDsetHandle = dset_find_handle(dset_id);
    if (*ppDsetHandle != NULL)
		return A_OK;

	*ppDsetHandle = dset_insert_handle(dset_id, flags, open_cb, callback_arg);
	if (*ppDsetHandle == NULL)
		return A_FALSE;

	status = remote_dset_op(DSET_OP_OPEN, *ppDsetHandle);

	return  status;
}

A_UINT32    api_host_dset_write(HOST_DSET_HANDLE *pDsetHandle, A_UINT8 *buffer, A_UINT32 length, A_UINT32 offset, A_UINT32 flags, dset_callback_fn_t write_cb, void *callback_arg)
{
	A_UINT32		status;

    pDsetHandle->offset = offset;
    pDsetHandle->length = length;
    pDsetHandle->flags = flags;
    pDsetHandle->cb = write_cb;
    pDsetHandle->cb_args = callback_arg;
	pDsetHandle->data_ptr = buffer;

	status = remote_dset_op(DSET_OP_WRITE, pDsetHandle);
	return status;
}

A_UINT32    api_host_dset_read(HOST_DSET_HANDLE *pDsetHandle, A_UINT8 *buffer, A_UINT32 length, A_UINT32 offset, dset_callback_fn_t  read_cb, void *callback_arg)
{
	A_UINT32		status;

    pDsetHandle->offset = offset;
    pDsetHandle->length = length;

    pDsetHandle->cb = read_cb;
    pDsetHandle->cb_args = callback_arg;
	pDsetHandle->data_ptr = buffer;

	status = remote_dset_op(DSET_OP_READ, pDsetHandle);
	return status;
}

A_UINT32    api_host_dset_commit(HOST_DSET_HANDLE *pDsetHandle, dset_callback_fn_t commit_cb, void *callback_arg)
{
	A_UINT32		status;

    pDsetHandle->cb = commit_cb;
    pDsetHandle->cb_args = callback_arg;
	status = remote_dset_op(DSET_OP_COMMIT, pDsetHandle);
	return status;
}

A_UINT32    api_host_dset_close(HOST_DSET_HANDLE *pDsetHandle, dset_callback_fn_t close_cb, void *callback_arg)
{
	A_UINT32		status;

    pDsetHandle->cb = close_cb;
    pDsetHandle->cb_args = callback_arg;

	status = remote_dset_op(DSET_OP_CLOSE, pDsetHandle);
	return status;
}

A_UINT32    api_host_dset_size(HOST_DSET_HANDLE *pDsetHandle, dset_callback_fn_t  size_cb, void *callback_arg)
{
	A_UINT32		status;

    pDsetHandle->cb = size_cb;
    pDsetHandle->cb_args = callback_arg;

	status = remote_dset_op(DSET_OP_SIZE, pDsetHandle);
	return status;
}

A_UINT32    api_host_dset_delete(A_UINT32 dset_id, A_UINT32 flags, dset_callback_fn_t  delete_cb, void *callback_arg)
{
	A_UINT32		status;
    HOST_DSET_HANDLE *pDsetHandle;

	pDsetHandle = dset_find_handle(dset_id);
    if (pDsetHandle == NULL)
	{
		pDsetHandle = dset_insert_handle(dset_id, flags, delete_cb, callback_arg);
		if (pDsetHandle == NULL)
			return A_FALSE;
	}
	else
	{	
		pDsetHandle->cb = delete_cb;
		pDsetHandle->cb_args = callback_arg;
		pDsetHandle->dset_id = dset_id;
		pDsetHandle->flags = flags;
	}

	status = remote_dset_op(DSET_OP_DELETE, pDsetHandle);
	return status;
}


