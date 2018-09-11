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
*                                              TEMPLATE
*
* Filename      : fs_dev_####.h
* Version       : v4.07.00.00
* Programmer(s) : BAN
*********************************************************************************************************
* Note(s)       : (a) Replace #### with the driver identifier (in the correct case).
*                 (b) Replace $$$$ with code/definitions/etc.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#ifndef  FS_DEV_TEMPLATE_PRESENT
#define  FS_DEV_TEMPLATE_PRESENT


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   FS_DEV_TEMPLATE_MODULE
#define  FS_DEV_TEMPLATE_EXT
#else
#define  FS_DEV_TEMPLATE_EXT  extern
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


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

extern const         FS_DEV_API  FSDev_Template;
FS_DEV_TEMPLATE_EXT  FS_QTY      FSDev_Template_UnitCtr;


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


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*                                 DEFINED IN BSP'S 'fs_dev_####_bsp.c'
*********************************************************************************************************
*/

void  FSDev_Template_BSP_Open (FS_QTY  unit_nbr);               /* Open (initialize).                                   */

void  FSDev_Template_BSP_Close(FS_QTY  unit_nbr);               /* Close (uninitialize).                                */

    /* $$$$ OTHER BSP FUNCTIONS */


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

#endif                                                          /* End of #### module include.                          */
