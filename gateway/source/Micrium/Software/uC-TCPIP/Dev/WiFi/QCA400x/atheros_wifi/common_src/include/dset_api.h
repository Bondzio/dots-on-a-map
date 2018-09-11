#ifndef __dset_api_h__
#define __dset_api_h__

/**HEADER********************************************************************
* 
*
* Copyright (c) 2014    Qualcomm Atheros Inc.;
* All Rights Reserved
*
*
*
* $FileName: dset.h$
* $Version : 3.8.40.0$
* $Date    : May-28-2014$
*
* Comments:
*
*   This file contains the defines, externs and data
*   structure definitions required by application
*   programs in order to use the Ethernet packet driver.
*
*END************************************************************************/


/*--------------------------------------------------------------------------*/
/*                        
**                            CONSTANT DEFINITIONS
*/

#define			MAX_HOST_DSET_SIZE				8

#define			INVALID_DSET_ID					0

typedef  enum  __DSET_OP  {
		DSET_OP_CREATE  = 1,
		DSET_OP_OPEN  = 2,
		DSET_OP_READ  = 3,
		DSET_OP_WRITE  = 4,
		DSET_OP_COMMIT = 5,
		DSET_OP_CLOSE  = 6,
		DSET_OP_SIZE   = 7,
		DSET_OP_DELETE  = 8,
} DSET_OP;

/*--------------------------------------------------------------------------*/
/*                        
**                            TYPE DEFINITIONS
*/

typedef void  (*dset_callback_fn_t)(A_UINT32 status, void *pv);

typedef struct host_api_dset_struct {
   A_UINT32     dset_id;         /* dset id    */
   A_UINT32     media_id;
   A_UINT32     flags;
   A_UINT32     offset;          /* dset offset            */
   A_UINT32     length;          /* dset length            */
   A_UINT32     left_len;        /* for large read/write   */
   dset_callback_fn_t  cb;
   void		   *cb_args;
   A_UINT8      done_flag;
   A_UINT8      *data_ptr;        /* dset buffer address */
} HOST_DSET_HANDLE;

//typedef struct host_dset_item {
//   A_UINT32     dset_id;         /* dset id    */
//   A_UINT32     length;          /* dset length            */
//} HOST_DSET_ITEM;

/*--------------------------------------------------------------------------*/
/*                        
**                            PROTOTYPES AND GLOBAL EXTERNS
*/

extern HOST_DSET_HANDLE   dset_handles[];

HOST_DSET_HANDLE *dset_find_handle(A_UINT32 dset_id);

A_UINT32    api_host_dset_create(HOST_DSET_HANDLE **pDsetHandle, A_UINT32 dset_id, A_UINT32 media_id, A_UINT32 length, A_UINT32 flags, dset_callback_fn_t create_cb, void *callback_arg);
A_UINT32    api_host_dset_open(HOST_DSET_HANDLE **pDsetHandle, A_UINT32 dset_id, A_UINT32 flags, dset_callback_fn_t open_cb, void *callback_arg);
A_UINT32    api_host_dset_write(HOST_DSET_HANDLE *pDsetHandle, A_UINT8 *buffer, A_UINT32 length, A_UINT32 offset, A_UINT32 flags, dset_callback_fn_t write_cb, void *callback_arg);
A_UINT32    api_host_dset_read(HOST_DSET_HANDLE *pDsetHandle, A_UINT8 *buffer, A_UINT32 length, A_UINT32 offset, dset_callback_fn_t  read_cb, void *callback_arg);
A_UINT32    api_host_dset_commit(HOST_DSET_HANDLE *pDsetHandle, dset_callback_fn_t  read_cb, void *callback_arg);
A_UINT32    api_host_dset_close(HOST_DSET_HANDLE *pDsetHandle, dset_callback_fn_t  read_cb, void *callback_arg);
A_UINT32    api_host_dset_size(HOST_DSET_HANDLE *pDsetHandle, dset_callback_fn_t  read_cb, void *callback_arg);
A_UINT32    api_host_dset_delete(A_UINT32 dset_id, A_UINT32 flags, dset_callback_fn_t  read_cb, void *callback_arg);

#endif
