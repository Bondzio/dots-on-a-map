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
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../Source/usbd_core.h"
#include  "usbd_bsp_template.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*
* Note(s) : $$$$ You may define any register outside the USB device controller registers interface needed
*           to configure the clock, interrupt and I/O.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                USB DEVICE ENDPOINT INFORMATION TABLE
*********************************************************************************************************
*/

USBD_DRV_EP_INFO  USBD_DrvEP_InfoTbl_Template[] = {
    {USBD_EP_INFO_TYPE_CTRL                                                   | USBD_EP_INFO_DIR_OUT, 0u,   64u},
    {USBD_EP_INFO_TYPE_CTRL                                                   | USBD_EP_INFO_DIR_IN,  0u,   64u},
    {USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 1u, 1024u},
    {USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  1u, 1024u},
    {USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 2u, 1024u},
    {USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  2u, 1024u},
    {USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 3u, 1024u},
    {USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  3u, 1024u},
    {DEF_BIT_NONE                                                                                 ,   0u,    0u}
};


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  USBD_DRV  *USBD_BSP_<controller>_DrvPtr;


/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  USBD_BSP_Template_Init           (USBD_DRV  *p_drv);
static  void  USBD_BSP_Template_Conn           (void            );
static  void  USBD_BSP_Template_Disconn        (void            );
static  void  USBD_BSP_<controller>_IntHandler (void      *p_arg);


/*
*********************************************************************************************************
*                                   USB DEVICE DRIVER BSP INTERFACE
*********************************************************************************************************
*/

USBD_DRV_BSP_API  USBD_DrvBSP_Template = {
    USBD_BSP_Template_Init,
    USBD_BSP_Template_Conn,
    USBD_BSP_Template_Disconn
};


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       USBD_BSP_Template_Init()
*
* Description : USB device controller board-specific initialization.
*
* Argument(s) : p_drv       Pointer to device driver structure.
*
* Return(s)   : none.
*
* Caller(s)   : Device controller driver init function via 'p_drv_api->Init()'
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBD_BSP_Template_Init (USBD_DRV  *p_drv)
{
    (void)&p_drv;

    /* $$$$ If ISR does NOT support argument passing, save reference to USBD_DRV in a local global variable. */
    USBD_BSP_<controller>_DrvPtr = p_drv;

    /* $$$$ This function perform all operations that the device controller cannot do. Typical operations are: */

    /* $$$$ Enable device control registers and bus clock [mandatory]. */
    /* $$$$ Configure main USB device interrupt in interrupt controller (e.g. registering BSP ISR) [mandatory]. */
    /* $$$$ Disable device transceiver clock [optional]. */
    /* $$$$ Configure I/O pins [optional]. */
}


/*
*********************************************************************************************************
*                                       USBD_BSP_Template_Conn()
*
* Description : Connect pull-up on DP.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Device controller driver start function via 'p_drv_api->Conn()'
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBD_BSP_Template_Conn (void)
{
    /* $$$$ Enable device transceiver clock [optional]. */
    /* $$$$ Enable pull-up resistor (this operation may be done in the driver) [mandatory]. */
    /* $$$$ Enable main USB device interrupt [mandatory]. */
}


/*
*********************************************************************************************************
*                                     USBD_BSP_Template_Disconn()
*
* Description : Disconnect pull-up on DP.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Device controller driver stop function via 'p_drv_api->Disconn()'
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBD_BSP_Template_Disconn (void)
{
    /* $$$$ Disable device transceiver clock [optional]. */
    /* $$$$ Disable pull-up resistor (this operation may be done in the driver) [mandatory]. */
    /* $$$$ Disable main USB device interrupt [mandatory]. */
}


/*
*********************************************************************************************************
*                                     USBD_BSP_<controller>_IntHandler()
*
* Description : USB device interrupt handler.
*
* Argument(s) : p_arg   Interrupt handler argument.
*
* Return(s)   : none.
*
* Caller(s)   : This is a ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBD_BSP_<controller>_IntHandler (void  *p_arg)
{
    USBD_DRV      *p_drv;
    USBD_DRV_API  *p_drv_api;


                                                                /* Get a reference to USBD_DRV.                         */
    /* $$$$ If ISR does NOT support argument passing, get USBD_DRV from a global variable initialized in USBD_BSP_Template_Init(). */
    p_drv = USBD_BSP_<controller>_DrvPtr;
    /* $$$$ Otherwise if ISR supports argument passing, get USBD_DRV from the argument. Do not forget to pass USBD_DRV structure when registering the BSP ISR in USBD_BSP_Template_Init(). */
    p_drv = (USBD_DRV *)p_arg;

    p_drv_api = p_drv->API_Ptr;                                 /* Get a reference to USBD_DRV_API.                     */
    p_drv_api->ISR_Handler(p_drv);                              /* Call the USB Device driver ISR.                      */
}

