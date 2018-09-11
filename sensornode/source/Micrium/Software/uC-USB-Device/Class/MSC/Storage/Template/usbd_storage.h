/*
*********************************************************************************************************
*                                            uC/USB-Device
*                                       The Embedded USB Stack
*
*                         (c) Copyright 2004-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/USB-Device is provided in source form to registered licensees ONLY.  It is
*                  illegal to distribute this source code to any third party unless you receive
*                  written permission by an authorized Micrium representative.  Knowledge of
*                  the source code may NOT be used to develop a similar product.
*
*                  Please help us continue to provide the Embedded community with the finest
*                  software available.  Your honesty is greatly appreciated.
*
*                  You can find our product's user manual, API reference, release notes and
*                  more information at: https://doc.micrium.com
*
*                  You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                 USB DEVICE MSC CLASS STORAGE DRIVER
*
*                                              TEMPLATE
*
* File           : usbf_storage.h
* Version        : V4.05.00
* Programmer(s)  :
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#ifndef  USBF_STORAGE_H
#define  USBF_STORAGE_H


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  <app_cfg.h>
#include  "../../../../Source/usbd_core.h"
#include  "../../usbd_scsi.h"


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 DEVICE TYPES
*********************************************************************************************************
*/

#define  USBD_DISK_DIRECT_ACCESS_DEVICE                    0x00
#define  USBD_DISK_SEQUENTIAL_ACCESS_DEVICE                0x01
#define  USBD_DISK_PRINTER_DEVICE                          0x02


/*
*********************************************************************************************************
*                                   DIRECT ACCESS MEDIUM TYPE
*********************************************************************************************************
*/

#define  USBD_DISK_MEMORY_MEDIA                            0x00


/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  USBD_StorageInit       (USBD_ERR          *p_err);

void  USBD_StorageCapacityGet(USBD_STORAGE_LUN  *p_storage_lun,
                              CPU_INT64U        *p_nbr_blks,
                              CPU_INT32U        *p_blk_size,
                              USBD_ERR          *p_err);

void  USBD_StorageRd         (USBD_STORAGE_LUN  *p_storage_lun,
                              CPU_INT64U         blk_addr,
                              CPU_INT32U         nbr_blks,
                              CPU_INT08U        *p_data_buf,
                              USBD_ERR          *p_err);

void  USBD_StorageWr         (USBD_STORAGE_LUN  *p_storage_lun,
                              CPU_INT64U         blk_addr,
                              CPU_INT32U         nbr_blks,
                              CPU_INT08U        *p_data_buf,
                              USBD_ERR          *p_err);

void  USBD_StorageStatusGet  (USBD_STORAGE_LUN  *p_storage_lun,
                              USBD_ERR          *p_err);

void  USBD_StorageLock       (USBD_STORAGE_LUN  *p_storage_lun,
                              CPU_INT32U         timeout_ms,
                              USBD_ERR          *p_err);

void  USBD_StorageUnlock     (USBD_STORAGE_LUN  *p_storage_lun,
                              USBD_ERR          *p_err);


/*
*********************************************************************************************************
*                                          CONFIGURATION ERRORS
*********************************************************************************************************
*/



#endif
