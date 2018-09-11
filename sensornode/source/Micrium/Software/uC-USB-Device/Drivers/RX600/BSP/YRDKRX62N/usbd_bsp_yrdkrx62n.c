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
*                              USB DEVICE DRIVER BOARD-SPECIFIC FUNCTIONS
*
*                                           Renesas YRDKRX62N
*
* File          : usbd_bsp_yrdkrx62n.h
* Version       : V4.05.00.00
* Programmer(s) : JPB
*                 OD
*                 JM
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <Source/usbd_core.h>
#include  "usbd_bsp_yrdkrx62n.h"
#include  <bsp_int.h>
#include  <bsp_int_vect_tbl.h>
#include  <bsp_cfg.h>

#if       BSP_CFG_OS3_EN   > 0u
#include  <os.h>
#endif

#if       BSP_CFG_OS2_EN   > 0u
#include  <ucos_ii.h>
#endif


/*
*********************************************************************************************************
*                                          HARDWARE REGISTERS
*
* Note(s): (1) The RX600 series has several functions for reducing power consumption, including switching
*              of clock signals, stopping modules, etc.
*********************************************************************************************************
*/

                                                                /* Input Buffer Control Register.                       */
#define  RX62N_REG_PORT1_BASE_ADDR               0x0008C000u
#define  RX62N_REG_PORT1_ICR                    (*(volatile  CPU_INT08U *)(RX62N_REG_PORT1_BASE_ADDR + 0x061u))
                                                                /* Module Stop Control Register Address (see note 1).   */
#define  RX62N_REG_USB_STANDBY_BASE_ADDR         0x00080014u
#define  RX62N_REG_USB_STANDBY                  (*(volatile  CPU_INT32U *)(RX62N_REG_USB_STANDBY_BASE_ADDR + 0x000u))
                                                                /* Port Function Control Register K.                    */
#define  RX62N_REG_IOPORT_BASE_ADDR              0x0008C100u
#define  RX62N_REG_IOPORT_PFKUSB                (*(volatile  CPU_INT08U *)(RX62N_REG_IOPORT_BASE_ADDR + 0x014u))

                                                                /* USB Interrupt Request Register.                      */
#define  RX62N_REG_INT_REQ_BASE_ADDR             0x00087010u
#define  RX62N_REG_USB_INT_REQ_1                (*(volatile  CPU_INT08U *)(RX62N_REG_INT_REQ_BASE_ADDR + 0x014u))
#define  RX62N_REG_USB_INT_REQ_2                (*(volatile  CPU_INT08U *)(RX62N_REG_INT_REQ_BASE_ADDR + 0x015u))
#define  RX62N_REG_USB_INT_REQ_3                (*(volatile  CPU_INT08U *)(RX62N_REG_INT_REQ_BASE_ADDR + 0x016u))
#define  RX62N_REG_USB_INT_REQ_4                (*(volatile  CPU_INT08U *)(RX62N_REG_INT_REQ_BASE_ADDR + 0x04Au))

                                                                /* USB Interrupt Enable Register.                       */
#define  RX62N_REG_INT_EN_BASE_ADDR              0x00087202u
#define  RX62N_REG_USB_INT_EN_1                 (*(volatile  CPU_INT08U *)(RX62N_REG_INT_EN_BASE_ADDR + 0x002u))
#define  RX62N_REG_USB_INT_EN_2                 (*(volatile  CPU_INT08U *)(RX62N_REG_INT_EN_BASE_ADDR + 0x009u))

                                                                /* USB Interrupt Priority Register.                     */
#define  RX62N_REG_INT_PRIO_BASE_ADDR            0x00087300u
#define  RX62N_REG_USB_INT_PRIO_1               (*(volatile  CPU_INT08U *)(RX62N_REG_INT_PRIO_BASE_ADDR + 0x00Cu))
#define  RX62N_REG_USB_INT_PRIO_2               (*(volatile  CPU_INT08U *)(RX62N_REG_INT_PRIO_BASE_ADDR + 0x00Du))
#define  RX62N_REG_USB_INT_PRIO_3               (*(volatile  CPU_INT08U *)(RX62N_REG_INT_PRIO_BASE_ADDR + 0x00Eu))
#define  RX62N_REG_USB_INT_PRIO_4               (*(volatile  CPU_INT08U *)(RX62N_REG_INT_PRIO_BASE_ADDR + 0x03Au))


/*
*********************************************************************************************************
*                                  HARDWARE REGISTER BIT FIELD DEFINES
*********************************************************************************************************
*/

#define  USB_D0FIFO0_INT_EN                 DEF_BIT_04
#define  USB_D1FIFO0_INT_EN                 DEF_BIT_05
#define  USB_USBI0_INT_EN                   DEF_BIT_06
#define  USB_USBR0_INT_EN                   DEF_BIT_02
                                                                /* ------------------- PORT1 ICR REG ------------------ */
#define  PORT1_ICR_B6                       DEF_BIT_06
                                                                /* ---- USB MODULES STOP CONTROL REGISTER: STANDBY ---- */
#define  USB_STANDBY_USB0                   DEF_BIT_19
#define  USB_STANDBY_USB1                   DEF_BIT_20
                                                                /* ---------- PORT FUNCTION CONTROL REG K ------------- */
#define  IOPORT_PFKUSB_USBMD               (DEF_BIT_00 | DEF_BIT_01)
#define  IOPORT_PFKUSB_USBE                 DEF_BIT_04

#define  USB_VECT_USB0_D0FIFO0              36
#define  USB_VECT_USB0_D1FIFO0              37
#define  USB_VECT_USB0_USBI0                38
#define  USB_VECT_USB_resume_USBR0          90


/*
*********************************************************************************************************
*                                 USB DEVICE ENDPOINT INFORMATION TABLE
*
* According to the RX62N Reference Manual, the USB Device Controller on the RX62N supports up
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
*********************************************************************************************************
*/

USBD_DRV_EP_INFO  USBD_DrvEP_InfoTbl_RX62N[] = {
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
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  USBD_DRV  *USBD_BSP_RX62N_DrvPtr;


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  USBD_BSP_YRDKRX62N_Init   (USBD_DRV  *p_drv);


/*
*********************************************************************************************************
*                                    USB DEVICE DRIVER BSP INTERFACE
*********************************************************************************************************
*/

USBD_DRV_BSP_API  USBD_DrvBSP_YRDKRX62N = {
    USBD_BSP_YRDKRX62N_Init,                                    /* Init board-specific USB ctrlr dependencies.          */
    (void *) 0u,                                                /* En  USB ctrlr conn dependencies.                     */
    (void *) 0u                                                 /* Dis USB ctrlr conn dependencies.                     */
};


/*
*********************************************************************************************************
*                                       USBD_BSP_YRDKRX62N_Init()
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

static  void  USBD_BSP_YRDKRX62N_Init (USBD_DRV  *p_drv)
{
    USBD_BSP_RX62N_DrvPtr = p_drv;

    BSP_IntVectSet(               USB_VECT_USB0_D0FIFO0,
                   (CPU_FNCT_VOID)USBD_BSP_RX62N_IntHandler);

    BSP_IntVectSet(               USB_VECT_USB0_D1FIFO0,
                   (CPU_FNCT_VOID)USBD_BSP_RX62N_IntHandler);

    BSP_IntVectSet(               USB_VECT_USB0_USBI0,
                   (CPU_FNCT_VOID)USBD_BSP_RX62N_IntHandler);

    BSP_IntVectSet(               USB_VECT_USB_resume_USBR0,
                   (CPU_FNCT_VOID)USBD_BSP_RX62N_IntHandler);
                                                                /* En int src.                                          */
    RX62N_REG_USB_INT_EN_1  |= (USB_D0FIFO0_INT_EN |
                                USB_D1FIFO0_INT_EN |
                                USB_USBI0_INT_EN);
    RX62N_REG_USB_INT_EN_2  |=  USB_USBR0_INT_EN;
                                                                /* Clr any pending ISR.                                 */
    RX62N_REG_USB_INT_REQ_1  =  DEF_CLR;
    RX62N_REG_USB_INT_REQ_2  =  DEF_CLR;
    RX62N_REG_USB_INT_REQ_3  =  DEF_CLR;
    RX62N_REG_USB_INT_REQ_4  =  DEF_CLR;
                                                                /* Set int prio.                                        */
    RX62N_REG_USB_INT_PRIO_1 =  3;                              /* USB0 D0FIFO0                                         */
    RX62N_REG_USB_INT_PRIO_2 =  3;                              /* USB0 D1FIFO0                                         */
    RX62N_REG_USB_INT_PRIO_3 =  3;                              /* USB0 USBI0                                           */
    RX62N_REG_USB_INT_PRIO_4 =  3;                              /* USB0 USBR0                                           */

    DEF_BIT_SET(RX62N_REG_PORT1_ICR,     PORT1_ICR_B6);         /* En in buf for USB0_VBUS pin.                         */
    DEF_BIT_CLR(RX62N_REG_IOPORT_PFKUSB, IOPORT_PFKUSB_USBMD);  /* Sel fnct mode for USB0 pins.                         */
    DEF_BIT_SET(RX62N_REG_IOPORT_PFKUSB, IOPORT_PFKUSB_USBE);   /* En all pin fncts for USB0.                           */

    DEF_BIT_CLR(RX62N_REG_USB_STANDBY, USB_STANDBY_USB0);       /* Transition the USB0 module from the stop state.      */
}


/*
*********************************************************************************************************
*                                      USBD_BSP_RX62N_IntHandler()
*
* Description : USB device interrupt handler.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is a ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#if      __RENESAS__
#pragma  interrupt  USBD_BSP_RX62N_IntHandler
#endif

CPU_ISR  USBD_BSP_RX62N_IntHandler (void)
{
    USBD_DRV      *p_drv;
    USBD_DRV_API  *p_drv_api;
    CPU_SR_ALLOC();

    OSIntNestingCtr++;                                          /* Notify uC/OS-III of ISR entry                        */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */
    CPU_CRITICAL_ENTER();

    p_drv     = USBD_BSP_RX62N_DrvPtr;
    p_drv_api = p_drv->API_Ptr;

    p_drv_api->ISR_Handler(p_drv);

    CPU_CRITICAL_EXIT();
    OSIntExit();                                                /* Notify uC/OS-III of ISR exit                         */
}
