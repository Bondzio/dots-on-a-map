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
*                                           TK-850/JG4L
*                                        Evaluation Board
*
* File          : usbd_bsp_v850e2jg4l.c
* Version       : V4.05.00.00
* Programmer(s) : FF
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <Source/usbd_core.h>
#include  "usbd_bsp_v850e2jg4l.h"
#include  <bsp_int.h>
#include  <bsp.h>

/*
*********************************************************************************************************
*                                      REGISTER & BIT FIELD DEFINES
*********************************************************************************************************
*/

                                                                /* ----------- USB CLOCK GENERATOR REGISTERS ---------- */
#define  USBD_BSP_REG_BASE_ADDR                      (CPU_INT32U)0xFF420000
#define  USBD_BSP_REG_UPMC                        (*(CPU_REG08 *)(USBD_BSP_REG_BASE_ADDR + 0x80))
#define  USBD_BSP_REG_UPLLCTL                     (*(CPU_REG08 *)(USBD_BSP_REG_BASE_ADDR + 0x84))
#define  USBD_BSP_REG_UPLLCC                      (*(CPU_REG08 *)(USBD_BSP_REG_BASE_ADDR + 0x88))
#define  USBD_BSP_REG_UPLLCS                      (*(CPU_REG08 *)(USBD_BSP_REG_BASE_ADDR + 0x8C))
#define  USBD_BSP_REG_UPLLS                       (*(CPU_REG08 *)(USBD_BSP_REG_BASE_ADDR + 0x90))
#define  USBD_BSP_REG_ULOCKR                      (*(CPU_REG08 *)(USBD_BSP_REG_BASE_ADDR + 0x94))

                                                                /* --------- USB CLOCK GENERATOR BIT DEFINES ---------- */
#define  USBD_BSP_BIT_UPMC_UPMC2                       DEF_BIT_02
#define  USBD_BSP_BIT_UPLLCTL_UPLLON                   DEF_BIT_00
#define  USBD_BSP_BIT_UPLLCC_USELPLL                   DEF_BIT_00
#define  USBD_BSP_BIT_UPLLCS_UPLLCS0                   DEF_BIT_00
#define  USBD_BSP_BIT_ULOCKR_ULOCK                     DEF_BIT_00

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBD_BSP_VAL_MAX_TO                                     0xFFFF

/*
*********************************************************************************************************
*                                USB DEVICE ENDPOINT INFORMATION TABLE
*
* According to the V850E2/Jx4L Reference Manual, the USB Device Controller on the V850E2/JG4L supports up
* to 10 endpoints (5 pipes) and their settings are not fixed; instead, you can select the
* transfer conditions for each pipe as follows:
*
* PIPE0:           Transfer Type : Control transfer only (default control pipe: DPC)
*                  Buffer size   : 8, 16, 32, or 64 bytes (single buffer)
*
* PIPE1 and PIPE3: Not Available for the V850E2/Jx4L USB controller.
*
* PIPE4 to PIPE5:  Transfer Type : Bulk transfer only
*                  Buffer size   : 8, 16, 32, or 64 bytes (double buffer can be specified)
*
* PIPE6 to PIPE7:  Transfer Type : Interrupt transfer only
*                  Buffer size   : 1 to 64 bytes (single buffer)
*
*********************************************************************************************************
*/

USBD_DRV_EP_INFO  USBD_DrvEP_InfoTbl_V850E2Jx4L[] = {
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_DIR_OUT                      , 0u, 64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_DIR_IN                       , 0u, 64u},
    {USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 4u, 64u},
    {USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 5u, 64u},
    {USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 6u, 64u},
    {USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT | USBD_EP_INFO_DIR_IN, 7u, 64u},
    {DEF_BIT_NONE                                                       , 0u,  0u}
};

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  USBD_DRV  *USBD_BSP_V850E2Jx4L_DrvPtr;


/*
*********************************************************************************************************
*                                     LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  USBD_BSP_V850E2Jx4L_Init        (USBD_DRV  *p_drv);
static  void  USBD_BSP_V850E2Jx4L_Conn        (void);
static  void  USBD_BSP_V850E2Jx4L_Disconn     (void);

static  void  USBD_BSP_V850E2Jx4L_IntHandler1 (void);


/*
*********************************************************************************************************
*                                   USB DEVICE DRIVER BSP INTERFACE
*********************************************************************************************************
*/

USBD_DRV_BSP_API  USBD_DrvBSP_V850E2Jx4L = {
    USBD_BSP_V850E2Jx4L_Init,                                   /* Init board-specific USB controller dependencies.     */
    USBD_BSP_V850E2Jx4L_Conn,                                   /* Enable  USB controller connection dependencies.      */
    USBD_BSP_V850E2Jx4L_Disconn,                                /* Disable USB controller connection dependencies.      */
};


/*
*********************************************************************************************************
*                                      USBD_BSP_V850E2Jx4L_Init()
*
* Description : USB device controller board-specific initialization.
*
* Argument(s) : p_drv  Pointer to device driver structure.
*
* Return(s)   : none.
*
* Caller(s)   : Device controller driver init function via 'p_bsp_api->Init()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBD_BSP_V850E2Jx4L_Init (USBD_DRV  *p_drv)
{
    CPU_INT32U  reg_to;


    USBD_BSP_REG_UPLLCTL &= ~USBD_BSP_BIT_UPLLCTL_UPLLON;       /* Stop the USB PLL                                     */

                                                                /* Fupll = 6 x fmx(fmx = 8MHz) = 48 MHz                 */
                                                                /* Ensure that BIT0 in UPMC is set to "1"               */
    BSP_CLK_GEN_ProRegWr(&USBD_BSP_REG_UPMC, DEF_BIT_00);       /* UPMC2 bit clear to "0"                               */
    USBD_BSP_REG_UPLLS    =  0x3;                               /* USB PLL lockup period (2^13 / fmx)                   */
    USBD_BSP_REG_UPLLCTL |= USBD_BSP_BIT_UPLLCTL_UPLLON;        /* Stop the USB PLL                                     */

    reg_to = USBD_BSP_VAL_MAX_TO;                               /* Wait for USB PLL to lock                             */
    while (DEF_BIT_IS_SET(USBD_BSP_REG_ULOCKR, USBD_BSP_BIT_ULOCKR_ULOCK) &&
          (reg_to > 0)) {
        reg_to--;
    }

    USBD_BSP_REG_UPLLCC = USBD_BSP_BIT_UPLLCC_USELPLL;          /* USB clock input from USB PLL (Fupll)                 */

    USBD_BSP_V850E2Jx4L_DrvPtr = p_drv;

    BSP_IntDis(BSP_INT_ID_ICUSBI);

    BSP_IntVectReg(BSP_INT_ID_ICUSBI,                           /* Install USB-Device System Management Int. vector     */
                   BSP_INT_PRIO_LEVEL_07,
                   USBD_BSP_V850E2Jx4L_IntHandler1);

}

/*
*********************************************************************************************************
*                                      USBD_BSP_V850E2Jx4L_Conn()
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

static  void  USBD_BSP_V850E2Jx4L_Conn (void)
{
    BSP_IntClr(BSP_INT_ID_ICUSBI);                              /* Clear USB interrupt.                                 */
    BSP_IntEn(BSP_INT_ID_ICUSBI);                               /* Enable USB System and Power management interrupt.    */
}

/*
*********************************************************************************************************
*                                       USBD_BSP_V850E2Jx4L_Disconn()
*
* Description : Disconnect pull-up on DP and disable interrupts.
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

static  void  USBD_BSP_V850E2Jx4L_Disconn (void)
{
    BSP_IntDis(BSP_INT_ID_ICUSBI);                              /* Disable USB System and Power management interrupt.   */
    BSP_IntClr(BSP_INT_ID_ICUSBI);                              /* Clear USB interrupt.                                 */
}
/*
*********************************************************************************************************
*                                    USBD_BSP_V850E2Jx4L_IntHandler1()
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

static  void  USBD_BSP_V850E2Jx4L_IntHandler1 (void)
{
    USBD_DRV      *p_drv;
    USBD_DRV_API  *p_drv_api;


    p_drv     = USBD_BSP_V850E2Jx4L_DrvPtr;
    p_drv_api = p_drv->API_Ptr;
    p_drv_api->ISR_Handler(p_drv);
}
