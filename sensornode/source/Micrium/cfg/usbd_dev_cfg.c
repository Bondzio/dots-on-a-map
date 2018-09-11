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
*                                    USB DEVICE CONFIGURATION FILE
*
*                                              TEMPLATE
*
* File          : usbd_dev_cfg.c
* Version       : V4.04.01
* Programmer(s) : FGK
*                 OD
*                 JPC
*********************************************************************************************************
*/

#include  <Source/usbd_core.h>
#include  <usbd_dev_cfg.h>


/*
*********************************************************************************************************
*                                      USB DEVICE CONFIGURATION
*********************************************************************************************************
*/

USBD_DEV_CFG  USBD_DevCfg_RX111 = {
    0xFFFE,                                                     /* Vendor  ID.                                          */
    0x1232,                                                     /* MSC Product ID.                                      */
    0x0100,                                                     /* Device release number.                               */
   "Micrium",                                                   /* Manufacturer  string.                                */
   "YWireless RX111 Factory Demo",                              /* Product       string.                                */
   "1234567890ABCDEF",                                          /* Serial number string.                                */
    USBD_LANG_ID_ENGLISH_US                                     /* String language ID.                                  */
};


/*
*********************************************************************************************************
*                                   USB DEVICE DRIVER CONFIGURATION
*********************************************************************************************************
*/

extern  USBD_DRV_EP_INFO  const  USBD_DrvEP_InfoTbl_RX111[];

USBD_DRV_CFG  USBD_DrvCfg_RX111 = {
    0x000A0000,                                                 /* Base addr   of device controller hw registers.       */
    0x00000000,                                                 /* Base addr   of device controller dedicated mem.      */
             0u,                                                /* Size        of device controller dedicated mem.      */

    USBD_DEV_SPD_FULL,                                          /* Speed       of device controller.                    */

    USBD_DrvEP_InfoTbl_RX111                                    /* EP Info tbl of device controller.                    */
};
