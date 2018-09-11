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
*                                           Renesas YLCDRX63N
*
* File          : usbd_bsp_ylcdrx63n.h
* Version       : V4.05.00.00
* Programmer(s) : OD
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../../../Source/usbd_core.h"
#include  "usbd_bsp_ylcdrx63n.h"
#include  <bsp.h>

#include  <os.h>


/*
*********************************************************************************************************
*                                          HARDWARE REGISTERS
*
* Note(s): (1) The RX600 series has several functions for reducing power consumption, including switching
*              of clock signals, stopping modules, etc.
*********************************************************************************************************
*/

                                                                /* Input Buffer Control Register                        */
/*
#define  RX62N_REG_PORT1_BASE_ADDR               0x0008C000
#define  RX62N_REG_PORT1_ICR                    (*(volatile  CPU_INT08U *)(RX62N_REG_PORT1_BASE_ADDR + 0x061))
*/
                                                                /* Module Stop Control Register Address (see note 1).   */
#define  RX63N_REG_USB_STANDBY_BASE_ADDR         0x00080014
#define  RX63N_REG_USB_STANDBY                  (*(volatile  CPU_INT32U *)(RX63N_REG_USB_STANDBY_BASE_ADDR + 0x000))
                                                                /* Port Function Control Register K                     */
/*
#define  RX62N_REG_IOPORT_BASE_ADDR              0x0008C100
#define  RX62N_REG_IOPORT_PFKUSB                (*(volatile  CPU_INT08U *)(RX62N_REG_IOPORT_BASE_ADDR + 0x014))
*/

                                                                /* USB Interrupt Request Register                       */
#define  RX63N_REG_INT_REQ_BASE_ADDR             0x00087010
#define  RX63N_REG_USB_INT_REQ_1                (*(volatile  CPU_INT08U *)(RX63N_REG_INT_REQ_BASE_ADDR + 0x014/*0x24*/))
#define  RX63N_REG_USB_INT_REQ_2                (*(volatile  CPU_INT08U *)(RX63N_REG_INT_REQ_BASE_ADDR + 0x015/*0x25*/))
#define  RX63N_REG_USB_INT_REQ_3                (*(volatile  CPU_INT08U *)(RX63N_REG_INT_REQ_BASE_ADDR + 0x016/*0x26*/))
#define  RX63N_REG_USB_INT_REQ_4                (*(volatile  CPU_INT08U *)(RX63N_REG_INT_REQ_BASE_ADDR + 0x04B/*0x5B*/))

                                                                /* USB Interrupt Enable Register                        */
#define  RX63N_REG_INT_EN_BASE_ADDR              0x00087202
#define  RX63N_REG_USB_INT_EN_1                 (*(volatile  CPU_INT08U *)(RX63N_REG_INT_EN_BASE_ADDR + 0x02))
#define  RX63N_REG_USB_INT_EN_2                 (*(volatile  CPU_INT08U *)(RX63N_REG_INT_EN_BASE_ADDR + 0x09))

                                                                /* USB Interrupt Priority Register                      */
#define  RX63N_REG_INT_PRIO_BASE_ADDR            0x00087300
#define  RX63N_REG_USB_INT_PRIO_1               (*(volatile  CPU_INT08U *)(RX63N_REG_INT_PRIO_BASE_ADDR + 0x24))
#define  RX63N_REG_USB_INT_PRIO_2               (*(volatile  CPU_INT08U *)(RX63N_REG_INT_PRIO_BASE_ADDR + 0x25))
#define  RX63N_REG_USB_INT_PRIO_3               (*(volatile  CPU_INT08U *)(RX63N_REG_INT_PRIO_BASE_ADDR + 0x26))
#define  RX63N_REG_USB_INT_PRIO_4               (*(volatile  CPU_INT08U *)(RX63N_REG_INT_PRIO_BASE_ADDR + 0x5B))


/*
*********************************************************************************************************
*                                HARDWARE REGISTER BIT FIELD DEFINES
*********************************************************************************************************
*/

#define     USB_D0FIFO1_INT_EN                 DEF_BIT_04
#define     USB_D1FIFO1_INT_EN                 DEF_BIT_05
#define     USB_USBI1_INT_EN                   DEF_BIT_06
#define     USB_USBR1_INT_EN                   DEF_BIT_03

#define     USB_VECT_USB_D0FIFO1                       36u
#define     USB_VECT_USB_D1FIFO1                       37u
#define     USB_VECT_USB_USBI1                         38u
#define     USB_VECT_USB_USBR1                         91u

                                                                /* ---- USB MODULES STOP CONTROL REGISTER: STANDBY ---- */
#define     USB_STANDBY_USB1                   DEF_BIT_18


/*
*********************************************************************************************************
*                                USB DEVICE ENDPOINT INFORMATION TABLE
*
* According to the RX63N Reference Manual, the USB Device Controller on the RX63N supports up
* to 20 endpoints (10 pipes) and their settings are not fixed; instead, you can select the
* transfer conditions for each pipe as follows:
*
* PIPE0:           Transfer Type : Control transfer only (default control pipe: DPC)
*                  Buffer size   : 8, 16, 32, or 64 bytes (single buffer)
*
* PIPE1 and PIPE2: Transfer Type : Bulk or Isochronous transfer
*                  Buffer size   : 8, 16, 32, or 64 bytes for bulk transfer or 1 to 256 bytes for
*                                  isochronous transfer (double buffer can be specified)
*
* PIPE3 to PIPE5:  Transfer Type : Bulk transfer only
*                  Buffer size   : 8, 16, 32, or 64 bytes (double buffer can be specified)
*
* PIPE6 to PIPE9:  Transfer Type : Interrupt transfer only
*                  Buffer size   : 1 to 64 bytes (single buffer)
*
*********************************************************************************************************
*/

USBD_DRV_EP_INFO  USBD_DrvEP_InfoTbl_RX63N[] = {
    {USBD_EP_INFO_TYPE_CTRL                          | USBD_EP_INFO_DIR_OUT,                       0u,  64u},
    {USBD_EP_INFO_TYPE_CTRL                          | USBD_EP_INFO_DIR_IN,                        0u,  64u},
    {USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 1u, 256u},
    {USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 2u, 256u},
    {USBD_EP_INFO_TYPE_BULK                          | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 3u,  64u},
    {USBD_EP_INFO_TYPE_BULK                          | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 4u,  64u},
    {USBD_EP_INFO_TYPE_BULK                          | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 5u,  64u},
    {USBD_EP_INFO_TYPE_INTR                          | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 6u,  64u},
    {USBD_EP_INFO_TYPE_INTR                          | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 7u,  64u},
    {USBD_EP_INFO_TYPE_INTR                          | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 8u,  64u},
    {USBD_EP_INFO_TYPE_INTR                          | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 9u,  64u},
    {DEF_BIT_NONE                                                                                , 0u,   0u}
};


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  USBD_DRV  *USBD_BSP_RX63N_DrvPtr;


/*
*********************************************************************************************************
*                                     LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  USBD_BSP_YRDKRX63N_Init   (USBD_DRV  *p_drv);
static  void  USBD_BSP_YRDKRX63N_Conn   (void);
static  void  USBD_BSP_YRDKRX63N_Disconn(void);


/*
*********************************************************************************************************
*                                   USB DEVICE DRIVER BSP INTERFACE
*********************************************************************************************************
*/

USBD_DRV_BSP_API  USBD_DrvBSP_YLCDRX63N = {
    USBD_BSP_YRDKRX63N_Init,                                    /* Init board-specific USB controller dependencies.     */
    USBD_BSP_YRDKRX63N_Conn,                                    /* Enable  USB controller connection dependencies.      */
    USBD_BSP_YRDKRX63N_Disconn                                  /* Disable USB controller connection dependencies.      */
};


/*
*********************************************************************************************************
*                                       USBD_BSP_YRDKRX63N_Init()
*
* Description : USB device controller board-specific initialization.
*
* Argument(s) : p_drv  Pointer to device driver structure.
*
* Return(s)   : none.
*
* Caller(s)   : Device controller driver init function USBD_DrvInit() via 'p_bsp_api->Init()'.
*
* Note(s)     : (1) The PLL initialization is done in the board support package.
*********************************************************************************************************
*/

static  void  USBD_BSP_YRDKRX63N_Init (USBD_DRV  *p_drv)
{
    USBD_BSP_RX63N_DrvPtr = p_drv;
                                                                /* Enable interrupt source.                             */
    RX63N_REG_USB_INT_EN_1  |= (USB_D0FIFO1_INT_EN |
                                USB_D1FIFO1_INT_EN |
                                USB_USBI1_INT_EN);
    RX63N_REG_USB_INT_EN_2  |=  USB_USBR1_INT_EN;
                                                                /* Clear any pending ISR.                               */
    RX63N_REG_USB_INT_REQ_1  =  DEF_CLR;
    RX63N_REG_USB_INT_REQ_2  =  DEF_CLR;
    RX63N_REG_USB_INT_REQ_3  =  DEF_CLR;
    RX63N_REG_USB_INT_REQ_4  =  DEF_CLR;
                                                                /* Set interrupt priority.                              */
    RX63N_REG_USB_INT_PRIO_1 =  3;                              /* USB D0FIFO1                                          */
    RX63N_REG_USB_INT_PRIO_2 =  3;                              /* USB D1FIFO1                                          */
    RX63N_REG_USB_INT_PRIO_3 =  3;                              /* USB USBI1                                            */
    RX63N_REG_USB_INT_PRIO_4 =  3;                              /* USB USBR1                                            */

    DEF_BIT_CLR(RX63N_REG_USB_STANDBY, USB_STANDBY_USB1);       /* Transition the USB1 module from the stop state.      */
}


/*
*********************************************************************************************************
*                                       USBD_BSP_YRDKRX63N_Conn()
*
* Description : Connect pull-up on DP and enable int.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Device controller driver start function via 'p_bsp_api->Conn()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBD_BSP_YRDKRX63N_Conn (void)
{

}


/*
*********************************************************************************************************
*                                     USBD_BSP_YRDKRX63N_Disconn()
*
* Description : Disconnect pull-up on DP.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Device controller driver stop function via 'p_bsp_api->Disconn()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBD_BSP_YRDKRX63N_Disconn (void)
{

}


/*
*********************************************************************************************************
*                                      USBD_BSP_RX63N_IntHandler()
*
* Description : USB device interrupt handler.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#if      __RENESAS__
#pragma  interrupt  USBD_BSP_RX63N_IntHandler
#endif

CPU_ISR  USBD_BSP_RX63N_IntHandler (void)
{
    USBD_DRV      *p_drv;
    USBD_DRV_API  *p_drv_api;


    CPU_SR_ALLOC();

    OSIntEnter();
    CPU_CRITICAL_ENTER();

    p_drv     = USBD_BSP_RX63N_DrvPtr;
    p_drv_api = p_drv->API_Ptr;

    p_drv_api->ISR_Handler(p_drv);

    CPU_CRITICAL_EXIT();
    OSIntExit();
}
