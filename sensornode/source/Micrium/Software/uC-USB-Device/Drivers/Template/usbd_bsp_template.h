/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at: https://doc.micrium.com
*
*               You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                             USB DEVICE DRIVER BOARD-SPECIFIC FUNCTIONS
*
*                                              TEMPLATE
*
* File          : usbd_bsp_template.h
* Version       : V4.05.00.00
* Programmer(s) : FGK
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This USB device driver board-specific function header file is protected from multiple
*               pre-processor inclusion through use of the USB device configuration module present pre-
*               processor macro definition.
*********************************************************************************************************
*/

#ifndef  USBD_BSP_TEMPLATE_MODULE_PRESENT                       /* See Note #1.                                         */
#define  USBD_BSP_TEMPLATE_MODULE_PRESENT


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  "usbd_drv_template.h"


/*
*********************************************************************************************************
*                                          USB DEVICE DRIVER
*********************************************************************************************************
*/

extern  USBD_DRV_BSP_API  USBD_DrvBSP_Template;
extern  USBD_DRV_EP_INFO  USBD_DrvEP_InfoTbl_Template[];


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif
