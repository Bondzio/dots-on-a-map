/*
*********************************************************************************************************
*                                             uC/FS V4
*                                     The Embedded File System
*
*                         (c) Copyright 2008-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/FS is provided in source form to registered licensees ONLY.  It is
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
*                                      FILE SYSTEM DEVICE DRIVER
*
*                                             DATA FLASH
*
* Filename      : fs_dev_dataflash.h
* Version       : v4.07.00.00
* Programmer(s) : JPC
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#ifndef  FS_DEV_DATAFLASH_PRESENT
#define  FS_DEV_DATAFLASH_PRESENT


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   FS_DEV_DATAFLASH_MODULE
#define  FS_DEV_DATAFLASH_EXT
#else
#define  FS_DEV_DATAFLASH_EXT  extern
#endif


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <fs_dev.h>


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/
///@todo this is not needed. DF data are obtained from the driver
typedef  struct  fs_dev_dataflash_cfg_struct {
    FS_SEC_SIZE   SecSize;
    FS_SEC_QTY    Size;
    void         *DiskPtr;
} FS_DEV_DATAFLASH_CFG;


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

extern const          FS_DEV_API  FSDev_DataFlash;
FS_DEV_DATAFLASH_EXT  FS_QTY      FSDev_DataFlash_UnitCtr;


/*
*********************************************************************************************************
*                                               MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void                 DF_AccessEn            (void);
void                 DF_AccessDis           (void);

CPU_INT08U          *DF_BaseAddrGet         (void);
FS_SEC_SIZE          DF_SecSizeGet          (void);
FS_SEC_QTY           DF_SecCntGet           (void);


int                  DF_ProgramEraseModeEnter  (void);
int                  DF_ProgramEraseModeExit   (void);

void                 DF_FClkFreqSet         (void);

int                  DF_Program             (CPU_INT08U  *p_dst,
                                             CPU_INT08U  *p_src,
                                             CPU_SIZE_T   cnt_octets);


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif                                                          /* End of DataFlash module include.                     */
