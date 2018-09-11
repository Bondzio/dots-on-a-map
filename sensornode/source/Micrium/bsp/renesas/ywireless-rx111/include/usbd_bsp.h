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
*                                            RENESAS RX111
*                                               on the
*                                    YWireless-RX111 Wireless Demo Kit
*
* File          : usbd_bsp.h
* Version       : V4.03.00
* Programmer(s) : DC
*                 JPC
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

#ifndef  USBD_BSP_RSKRX111_MODULE_PRESENT                      /* See Note #1.                                         */
#define  USBD_BSP_RSKRX111_MODULE_PRESENT

/*
*********************************************************************************************************
*                                    USBD ISR FUNCTION PROTOTYPE
*********************************************************************************************************
*/

CPU_ISR  USBD_BSP_RX111_IntHandler(void);


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif
