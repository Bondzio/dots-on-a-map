#ifndef __dset_h__
#define __dset_h__

/**HEADER********************************************************************
* 
*
* Copyright (c) 2014    Qualcomm Atheros Inc.;
* All Rights Reserved
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

#include <a_types.h>
/*--------------------------------------------------------------------------*/
/*                        
**                            CONSTANT DEFINITIONS
*/

#define			MAX_DSET_SIZE				16

/*--------------------------------------------------------------------------*/
/*                        
**                            TYPE DEFINITIONS
*/

typedef struct host_dset_struct {
   A_UINT32     dset_id;         /* dset id    */
   A_UINT32     length;          /* dset length            */
   A_UINT8     *data_ptr;        /* dset buffer address */
} HOST_DSET;

typedef struct host_dset_item {
   A_UINT32     dset_id;         /* dset id    */
   A_UINT32     length;          /* dset length            */
} HOST_DSET_ITEM;


/*--------------------------------------------------------------------------*/
/*                        
**                            PROTOTYPES AND GLOBAL EXTERNS
*/

HOST_DSET  *dset_find(A_UINT32 dset_id);
HOST_DSET  *dset_get(A_UINT32 dset_id, A_UINT32 length);
A_UINT32    dset_write(HOST_DSET *pDset, A_UINT8 *pData, A_UINT32 offset, A_UINT32 length);
A_UINT32    dset_read(HOST_DSET *pDset, A_UINT8 *pData, A_UINT32 offset, A_UINT32 length);
A_UINT32    dset_fill(A_UINT8 *pData, A_UINT32 max_dset_count);

HOST_DSET  *dset_get_first();
HOST_DSET  *dset_get_next();




#endif
