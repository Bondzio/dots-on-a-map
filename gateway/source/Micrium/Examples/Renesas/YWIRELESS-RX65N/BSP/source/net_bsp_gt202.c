/*
*********************************************************************************************************
*                                             uC/TCP-IP V3
*                                      The Embedded TCP/IP Suite
*
*                          (c) Copyright 2003-2016; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/TCP-IP is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                            NETWORK BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
* Filename      : net_bsp.c
* Version       : V3.04.00
* Programmer(s) : ITJ
*                 FGK
*                 PC
*********************************************************************************************************
* Note(s)       : (1) Assumes the following versions (or more recent) of software modules are included in
*                     the project build :
*
*                     (a) uC/TCP-IP V2.10
*                     (b) uC/OS-III V3.00.0
*                     (c) uC/LIB    V1.27
*                     (d) uC/CPU    V1.20
*
*                 (2) To provide the required Board Support Package functionality, insert the appropriate
*                     board-specific code to perform the stated actions wherever '$$$$' comments are found.
*
*                     #### This note MAY be entirely removed for specific board support packages.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    NET_BSP_MODULE

#include  <Source/net.h>
#include  <net_bsp_gt202.h>
#include  <bsp.h>
#include  <bsp_int_vect_tbl.h>
#include  <bsp_cfg.h>
#include  <os.h>
#include  <iorx651.h>
#include  <bsp_os.h>
#include  <IF\net_if_wifi.h>
#include  <bsp_clk.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             LOCAL MACROS
*********************************************************************************************************
*/

#define  UNUSED_PARAM(p)                       (p) = (p)


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

CPU_BOOLEAN  SPI_LOCK;


/*
*********************************************************************************************************
*                                  NETWORK DEVICE INTERFACE NUMBERS
*
* Note(s) : (1) (a) Each network device maps to a unique network interface number.
*
*               (b) Instances of network devices' interface number SHOULD be named using the following
*                   convention :
*
*                       <Board><Device>[Number]_IF_Nbr
*
*                           where
*                                   <Board>         Development board name
*                                   <Device>        Network device name (or type)
*                                   [Number]        Network device number for each specific instance
*                                                       of device (optional if the development board
*                                                       does NOT support multiple instances of the
*                                                       specific device)
*
*                   For example, the network device interface number variable for the #2 MACB Ethernet
*                   controller on an Atmel AT91SAM92xx should be named 'AT91SAM92xx_MACB_2_IF_Nbr'.
*
*               (c) Network device interface number variables SHOULD be initialized to 'NET_IF_NBR_NONE'.
*********************************************************************************************************
*/

#ifdef  NET_IF_WIFI_MODULE_EN
static  NET_IF_NBR  WiFi_SPI_IF_Nbr  = NET_IF_NBR_NONE;
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                               NETWORK DEVICE BSP FUNCTION PROTOTYPES
*
* Note(s) : (1) Device driver BSP functions may be arbitrarily named.  However, it is recommended that
*               device BSP functions be named using the suggested names/conventions provided below.
*
*               (a) (1) Network device BSP functions SHOULD be named using the following convention :
*
*                           NetDev_[Device]<Function>[Number]()
*
*                               where
*                                   (A) [Device]        Network device name or type (optional if the
*                                                           development board does NOT support multiple
*                                                           devices)
*                                   (B) <Function>      Network device BSP function
*                                   (C) [Number]        Network device number for each specific instance
*                                                           of device (optional if the development board
*                                                           does NOT support multiple instances of the
*                                                           specific device)
*
*                       For example, the NetDev_CfgClk() function for the #2 MACB Ethernet controller
*                       on an Atmel AT91SAM92xx should be named NetDev_MACB_CfgClk2().
*
*                   (2) BSP-level device ISR handlers SHOULD be named using the following convention :
*
*                           NetDev_[Device]ISR_Handler[Type][Number]()
*
*                               where
*                                   (A) [Device]        Network device name or type (optional if the
*                                                           development board does NOT support multiple
*                                                           devices)
*                                   (B) [Type]          Network device interrupt type (optional if
*                                                           interrupt type is generic or unknown)
*                                   (C) [Number]        Network device number for each specific instance
*                                                           of device (optional if the development board
*                                                           does NOT support multiple instances of the
*                                                           specific device)
*
*               (b) All BSP function prototypes SHOULD be located within the development board's network
*                   BSP C source file ('net_bsp.c') & be declared as static functions to prevent name
*                   clashes with other network protocol suite BSP functions/files.
*********************************************************************************************************
*/

#ifdef  NET_IF_WIFI_MODULE_EN

static  void     NetDev_WiFi_GT202_Start                  (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_Stop                   (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_SPI_Lock               (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_SPI_Unlock             (NET_IF                          *p_if);

#if BSP_CFG_GT202_ON_BOARD > 0
static  void     NetDev_WiFi_GT202_OnBoard_CfgGPIO        (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_OnBoard_CfgIntCtrl     (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_OnBoard_IntCtrl        (NET_IF                          *p_if,
                                                           CPU_BOOLEAN                      en,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_OnBoard_SPI_Init       (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_OnBoard_SPI_WrRd       (NET_IF                          *p_if,
                                                           CPU_INT08U                      *p_buf_wr,
                                                           CPU_INT08U                      *p_buf_rd,
                                                           CPU_INT16U                       len,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_OnBoard_SPI_ChipSelEn  (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_OnBoard_SPI_ChipSelDis (NET_IF                          *p_if);

static  void     NetDev_WiFi_GT202_OnBoard_SPI_SetCfg     (NET_IF                          *p_if,
                                                           NET_DEV_CFG_SPI_CLK_FREQ         freq,
                                                           NET_DEV_CFG_SPI_CLK_POL          pol,
                                                           NET_DEV_CFG_SPI_CLK_PHASE        pha,
                                                           NET_DEV_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                                           NET_DEV_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir,
                                                           NET_ERR                         *p_err);

        CPU_ISR  NetDev_WiFiISR_Handler_OnBoard_GT202     (void);

#elif BSP_CFG_GT202_PMOD1 > 0
static  void     NetDev_WiFi_GT202_PMOD1_CfgGPIO          (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD1_CfgIntCtrl       (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD1_IntCtrl          (NET_IF                          *p_if,
                                                           CPU_BOOLEAN                      en,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD1_SPI_Init         (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD1_SPI_WrRd         (NET_IF                          *p_if,
                                                           CPU_INT08U                      *p_buf_wr,
                                                           CPU_INT08U                      *p_buf_rd,
                                                           CPU_INT16U                       len,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD1_SPI_ChipSelEn    (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD1_SPI_ChipSelDis   (NET_IF                          *p_if);

static  void     NetDev_WiFi_GT202_PMOD1_SPI_SetCfg       (NET_IF                          *p_if,
                                                           NET_DEV_CFG_SPI_CLK_FREQ         freq,
                                                           NET_DEV_CFG_SPI_CLK_POL          pol,
                                                           NET_DEV_CFG_SPI_CLK_PHASE        pha,
                                                           NET_DEV_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                                           NET_DEV_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir,
                                                           NET_ERR                         *p_err);

        CPU_ISR  NetDev_WiFiISR_Handler_PMOD1_GT202       (void);

#elif BSP_CFG_GT202_PMOD2 > 0
static  void     NetDev_WiFi_GT202_PMOD2_CfgGPIO          (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD2_CfgIntCtrl       (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD2_IntCtrl          (NET_IF                          *p_if,
                                                           CPU_BOOLEAN                      en,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD2_SPI_Init         (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD2_SPI_WrRd         (NET_IF                          *p_if,
                                                           CPU_INT08U                      *p_buf_wr,
                                                           CPU_INT08U                      *p_buf_rd,
                                                           CPU_INT16U                       len,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD2_SPI_ChipSelEn    (NET_IF                          *p_if,
                                                           NET_ERR                         *p_err);

static  void     NetDev_WiFi_GT202_PMOD2_SPI_ChipSelDis   (NET_IF                          *p_if);

static  void     NetDev_WiFi_GT202_PMOD2_SPI_SetCfg       (NET_IF                          *p_if,
                                                           NET_DEV_CFG_SPI_CLK_FREQ         freq,
                                                           NET_DEV_CFG_SPI_CLK_POL          pol,
                                                           NET_DEV_CFG_SPI_CLK_PHASE        pha,
                                                           NET_DEV_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                                           NET_DEV_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir,
                                                           NET_ERR                         *p_err);

        CPU_ISR  NetDev_WiFiISR_Handler_PMOD2_GT202       (void);
#endif

#endif


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                    NETWORK DEVICE BSP INTERFACE
*
* Note(s) : (1) Device board-support package (BSP) interface structures are used by the device driver to
*               call specific devices' BSP functions via function pointer instead of by name.  This enables
*               the network protocol suite to compile & operate with multiple instances of multiple devices
*               & drivers.
*
*           (2) In most cases, the BSP interface structure provided below SHOULD suffice for most devices'
*               BSP functions exactly as is with the exception that BSP interface structures' names MUST be
*               unique & SHOULD clearly identify the development board, device name, & possibly the specific
*               device number (if the development board supports multiple instances of any given device).
*
*               (a) BSP interface structures SHOULD be named using the following convention :
*
*                       NetDev_BSP_<Board><Device>[Number]{}
*
*                           where
*                               (1) <Board>         Development board name
*                               (2) <Device>        Network device name (or type)
*                               (3) [Number]        Network device number for each specific instance
*                                                       of device (optional if the development board
*                                                       does NOT support multiple instances of the
*                                                       specific device)
*
*                   For example, the BSP interface structure for the #2 MACB Ethernet controller on
*                   an Atmel AT91SAM92xx should be named NetDev_BSP_AT91SAM92xx_MACB_2{}.
*
*               (b) The BSP interface structure MUST also be externally declared in the development
*                   board's network BSP header file ('net_bsp.h') with the exact same name & type.
*********************************************************************************************************
*/

#ifdef  NET_IF_WIFI_MODULE_EN

#if BSP_CFG_GT202_ON_BOARD > 0
const  NET_DEV_BSP_WIFI_SPI  NetDev_BSP_GT202_OnBoard_SPI  =  {
                                                               &NetDev_WiFi_GT202_Start,
                                                               &NetDev_WiFi_GT202_Stop,
                                                               &NetDev_WiFi_GT202_OnBoard_CfgGPIO,
                                                               &NetDev_WiFi_GT202_OnBoard_CfgIntCtrl,
                                                               &NetDev_WiFi_GT202_OnBoard_IntCtrl,
                                                               &NetDev_WiFi_GT202_OnBoard_SPI_Init,
                                                               &NetDev_WiFi_GT202_SPI_Lock,
                                                               &NetDev_WiFi_GT202_SPI_Unlock,
                                                               &NetDev_WiFi_GT202_OnBoard_SPI_WrRd,
                                                               &NetDev_WiFi_GT202_OnBoard_SPI_ChipSelEn,
                                                               &NetDev_WiFi_GT202_OnBoard_SPI_ChipSelDis,
                                                               &NetDev_WiFi_GT202_OnBoard_SPI_SetCfg
                                                              };

#elif BSP_CFG_GT202_PMOD1 > 0
const  NET_DEV_BSP_WIFI_SPI  NetDev_BSP_GT202_PMOD1_SPI  =  {
                                                               &NetDev_WiFi_GT202_Start,
                                                               &NetDev_WiFi_GT202_Stop,
                                                               &NetDev_WiFi_GT202_PMOD1_CfgGPIO,
                                                               &NetDev_WiFi_GT202_PMOD1_CfgIntCtrl,
                                                               &NetDev_WiFi_GT202_PMOD1_IntCtrl,
                                                               &NetDev_WiFi_GT202_PMOD1_SPI_Init,
                                                               &NetDev_WiFi_GT202_SPI_Lock,
                                                               &NetDev_WiFi_GT202_SPI_Unlock,
                                                               &NetDev_WiFi_GT202_PMOD1_SPI_WrRd,
                                                               &NetDev_WiFi_GT202_PMOD1_SPI_ChipSelEn,
                                                               &NetDev_WiFi_GT202_PMOD1_SPI_ChipSelDis,
                                                               &NetDev_WiFi_GT202_PMOD1_SPI_SetCfg
                                                              };

#elif BSP_CFG_GT202_PMOD2 > 0
const  NET_DEV_BSP_WIFI_SPI  NetDev_BSP_GT202_PMOD2_SPI  =  {
                                                               &NetDev_WiFi_GT202_Start,
                                                               &NetDev_WiFi_GT202_Stop,
                                                               &NetDev_WiFi_GT202_PMOD2_CfgGPIO,
                                                               &NetDev_WiFi_GT202_PMOD2_CfgIntCtrl,
                                                               &NetDev_WiFi_GT202_PMOD2_IntCtrl,
                                                               &NetDev_WiFi_GT202_PMOD2_SPI_Init,
                                                               &NetDev_WiFi_GT202_SPI_Lock,
                                                               &NetDev_WiFi_GT202_SPI_Unlock,
                                                               &NetDev_WiFi_GT202_PMOD2_SPI_WrRd,
                                                               &NetDev_WiFi_GT202_PMOD2_SPI_ChipSelEn,
                                                               &NetDev_WiFi_GT202_PMOD2_SPI_ChipSelDis,
                                                               &NetDev_WiFi_GT202_PMOD2_SPI_SetCfg
                                                              };
#endif

#endif


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                WIRELESS SPI DEVICE DRIVER FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef  NET_IF_WIFI_MODULE_EN
/*
*********************************************************************************************************
*                                      NetDev_WiFi_GT202_Start()
*
* Description : Start wireless hardware.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Start()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetDev_WiFi_GT202_Start (NET_IF   *p_if,
                                       NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning.          */

    PORT4.PDR.BIT.B0 = 1;
    PORT4.PMR.BIT.B0 = 0;

    PORT4.PODR.BIT.B0 = 0;                                      /* BRES Signal input to CHIP_PWD                        */
    KAL_Dly(1000);

    PORT4.PODR.BIT.B0 = 1;                                      /* BRES Signal input to CHIP_PWD                        */
    KAL_Dly(1000);

   *p_err = NET_DEV_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      NetDev_WiFi_GT202_Stop()
*
* Description : Stop wireless hardware.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Stop()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  NetDev_WiFi_GT202_Stop (NET_IF   *p_if,
                                      NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    PORT4.PODR.BIT.B0 = 0;                                      /* BRES/PMODRST Signal input to CHIP_PWD                */

    *p_err = NET_DEV_ERR_NONE;
}


/*
*********************************************************************************************************
*                                 NetDev_WiFi_GT202_OnBoard_CfgGPIO()
*                                  NetDev_WiFi_GT202_PMOD1_CfgGPIO()
*                                  NetDev_WiFi_GT202_PMOD2_CfgGPIO()
*
* Description : Configure general-purpose I/O (GPIO) for the specified interface/device.
*
* Argument(s) : p_if        Pointer to network interface to configure.
*               ---         Argument validated in NetDev_Init().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_DEV_ERR_NONE                Device GPIO     successfully configured.
*                               NET_DEV_ERR_FAULT               Device GPIO NOT successfully configured.
*
* Return(s)   : none.
*
* Caller(s)   : NetDev_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
static  void  NetDev_WiFi_GT202_OnBoard_CfgGPIO (NET_IF   *p_if,
                                                 NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

                                                                /* Configure I/O pin directions. 0 = IN, 1 = OUT        */
    PORTE.PDR.BIT.B0 = 1;                                       /* PE[0] - SSLB1                                        */
    PORTE.PDR.BIT.B6 = 1;                                       /* PE[6] - MOSIB                                        */
    PORTE.PDR.BIT.B7 = 0;                                       /* PE[7] - MISOB                                        */
    PORTE.PDR.BIT.B5 = 1;                                       /* PE[5] - RSPCKB                                       */
    PORTA.PDR.BIT.B1 = 0;                                       /* PA[1] - IRQ11                                        */
    PORT4.PDR.BIT.B0 = 1;                                       /* P4[0] - BRES                                         */

    BSP_IO_Protect(WRITE_ENABLED);                              /* Enable writing to Multi-Function Pin Controller      */

#if 0
    MPC.PE0PFS.BIT.PSEL = 0xD;                                  /* Select SSLB1 function for PE[0]                      */
#endif
    MPC.PE6PFS.BIT.PSEL = 0xD;                                  /* Select MOSIB  function for PE[6]                     */
    MPC.PE7PFS.BIT.PSEL = 0xD;                                  /* Select MISOB  function for PE[7]                     */
    MPC.PE5PFS.BIT.PSEL = 0xD;                                  /* Select RSPCKB function for PE[5]                     */
    MPC.PA1PFS.BIT.ISEL = 1;                                    /* Enable IRQ11  on           PA[1]                     */

    BSP_IO_Protect(WRITE_DISABLED);                             /* Disable writing to MPC registers                     */

                                                                /* Configure pin modes. 0 = GPIO, 1 = Peripheral        */
    PORTE.PMR.BIT.B0 = 0;                                       /* PE[0] as GPIO for manual chip-select control.        */
    PORTE.PMR.BIT.B6 = 1;                                       /* PE[6] as SPI Peripheral                              */
    PORTE.PMR.BIT.B7 = 1;                                       /* PE[7] as SPI Peripheral                              */
    PORTE.PMR.BIT.B5 = 1;                                       /* PE[5] as SPI Peripheral                              */
                                                                /* Other pins are already 0 (reset value).              */

    PORTE.PODR.BIT.B0 = 1;                                      /* Set chip select to inactive.                         */

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD1 > 0
static  void  NetDev_WiFi_GT202_PMOD1_CfgGPIO (NET_IF   *p_if,
                                               NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

                                                                /* Configure I/O pin directions. 0 = IN, 1 = OUT        */
    PORTA.PDR.BIT.B4 = 1;                                       /* PA[4] - SSLA0                                        */
    PORTA.PDR.BIT.B6 = 1;                                       /* PA[6] - MOSIA                                        */
    PORTA.PDR.BIT.B7 = 0;                                       /* PA[7] - MISOA                                        */
    PORTA.PDR.BIT.B5 = 1;                                       /* PA[5] - RSPCKA                                       */
    PORTB.PDR.BIT.B0 = 0;                                       /* PB[0] - IRQ12                                        */
    PORT4.PDR.BIT.B0 = 1;                                       /* P4[0] - PMODRES                                      */
    PORT7.PDR.BIT.B5 = 1;                                       /* P7[5] - PMOD9                                        */
    PORT7.PDR.BIT.B4 = 1;                                       /* P7[4] - PMOD10                                       */

    BSP_IO_Protect(WRITE_ENABLED);                              /* Enable writing to Multi-Function Pin Controller      */

#if 0
    MPC.PA4PFS.BIT.PSEL = 0xD;                                  /* Select SSLA0 function for PA[4]                      */
#endif
    MPC.PA6PFS.BIT.PSEL = 0xD;                                  /* Select MOSIA  function for PA[6]                     */
    MPC.PA7PFS.BIT.PSEL = 0xD;                                  /* Select MISOA  function for PA[7]                     */
    MPC.PA5PFS.BIT.PSEL = 0xD;                                  /* Select RSPCKA function for PA[5]                     */
    MPC.PB0PFS.BIT.ISEL = 1;                                    /* Enable IRQ12  on           PB[0]                     */

    BSP_IO_Protect(WRITE_DISABLED);                             /* Disable writing to MPC registers                     */

                                                                /* Configure pin modes. 0 = GPIO, 1 = Peripheral        */
    PORTA.PMR.BIT.B4 = 0;                                       /* PA[4] as GPIO for manual chip-select control.        */
    PORTA.PMR.BIT.B6 = 1;                                       /* PA[6] as SPI Peripheral                              */
    PORTA.PMR.BIT.B7 = 1;                                       /* PA[7] as SPI Peripheral                              */
    PORTA.PMR.BIT.B5 = 1;                                       /* PA[5] as SPI Peripheral                              */
                                                                /* Other pins are already 0 (reset value).              */

    PORTA.PODR.BIT.B4 = 1;                                      /* Set chip select to inactive.                         */

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD2 > 0
static  void  NetDev_WiFi_GT202_PMOD2_CfgGPIO (NET_IF   *p_if,
                                               NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

                                                                /* Configure I/O pin directions. 0 = IN, 1 = OUT        */
    PORTE.PDR.BIT.B4 = 1;                                       /* PE[4] - SSLB0                                        */
    PORTE.PDR.BIT.B6 = 1;                                       /* PE[6] - MOSIB                                        */
    PORTE.PDR.BIT.B7 = 0;                                       /* PE[7] - MISOB                                        */
    PORTE.PDR.BIT.B5 = 1;                                       /* PE[5] - RSPCKB                                       */
    PORT6.PDR.BIT.B7 = 0;                                       /* P6[7] - IRQ15                                        */
    PORT4.PDR.BIT.B0 = 1;                                       /* P4[0] - PMODRST                                      */
    PORT7.PDR.BIT.B5 = 1;                                       /* P7[5] - PMOD9                                        */
    PORT7.PDR.BIT.B4 = 1;                                       /* P7[4] - PMOD10                                       */

                                                                /* Switch pins to high-drive output.                    */
    PORTE.DSCR.BIT.B5 = 1;
    PORTE.DSCR.BIT.B6 = 1;
    PORTE.DSCR.BIT.B4 = 1;

    BSP_IO_Protect(WRITE_ENABLED);                              /* Enable writing to Multi-Function Pin Controller      */

#if 0
    MPC.PE4PFS.BIT.PSEL = 0xD;                                  /* Select SSLB0 function for PE[4]                      */
#endif
    MPC.PE6PFS.BIT.PSEL = 0xD;                                  /* Select MOSIB  function for PE[6]                     */
    MPC.PE7PFS.BIT.PSEL = 0xD;                                  /* Select MISOB  function for PE[7]                     */
    MPC.PE5PFS.BIT.PSEL = 0xD;                                  /* Select RSPCKB function for PE[5]                     */
    MPC.P67PFS.BIT.ISEL = 1;                                    /* Enable IRQ15  on           P6[7]                     */

    BSP_IO_Protect(WRITE_DISABLED);                             /* Disable writing to MPC registers                     */

                                                                /* Configure pin modes. 0 = GPIO, 1 = Peripheral        */
    PORTE.PMR.BIT.B4 = 0;                                       /* PE[4] as GPIO for manual chip-select control.        */
    PORTE.PMR.BIT.B6 = 1;                                       /* PE[6] as SPI Peripheral                              */
    PORTE.PMR.BIT.B7 = 1;                                       /* PE[7] as SPI Peripheral                              */
    PORTE.PMR.BIT.B5 = 1;                                       /* PE[5] as SPI Peripheral                              */
                                                                /* Other pins are already 0 (reset value).              */

    PORTE.PODR.BIT.B4 = 1;                                      /* Set chip select to inactive.                         */

   *p_err = NET_DEV_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                               NetDev_WiFi_GT202_OnBoard_CfgIntCtrl()
*                                NetDev_WiFi_GT202_PMOD1_CfgIntCtrl()
*                                NetDev_WiFi_GT202_PMOD2_CfgIntCtrl()
*
* Description : Assign extern ISR to the ISR Handler.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->CfgExtIntCtrl()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
static  void  NetDev_WiFi_GT202_OnBoard_CfgIntCtrl (NET_IF   *p_if,
                                                    NET_ERR  *p_err)
{
    WiFi_SPI_IF_Nbr = p_if->Nbr;                                /* Configure this device's BSP instance with ...        */
                                                                /* ... specific interface number.                       */

    ICU.IRQCR[11].BIT.IRQMD = 0x1;                              /* Falling-edge trigger.                                */
    IPR(ICU, IRQ11)         = 0x1;                              /* Set IPL to lowest setting.                           */
    IR (ICU, IRQ11)         =   0;                              /* Clear any pending interrupt.                         */

    BSP_IntVectSet(75, (CPU_FNCT_VOID)NetDev_WiFiISR_Handler_OnBoard_GT202);

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD1 > 0
static  void  NetDev_WiFi_GT202_PMOD1_CfgIntCtrl (NET_IF   *p_if,
                                                  NET_ERR  *p_err)
{
    WiFi_SPI_IF_Nbr = p_if->Nbr;                                /* Configure this device's BSP instance with ...        */
                                                                /* ... specific interface number.                       */

    ICU.IRQCR[12].BIT.IRQMD = 0x1;                              /* Falling-edge trigger.                                */
    IPR(ICU, IRQ12)         = 0x1;                              /* Set IPL to lowest setting.                           */
    IR (ICU, IRQ12)         =   0;                              /* Clear any pending interrupt.                         */

    BSP_IntVectSet(76, (CPU_FNCT_VOID)NetDev_WiFiISR_Handler_PMOD1_GT202);

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD2 > 0
static  void  NetDev_WiFi_GT202_PMOD2_CfgIntCtrl (NET_IF   *p_if,
                                                  NET_ERR  *p_err)
{
    WiFi_SPI_IF_Nbr = p_if->Nbr;                                /* Configure this device's BSP instance with ...        */
                                                                /* ... specific interface number.                       */

    ICU.IRQCR[15].BIT.IRQMD = 0x1;                              /* Falling-edge trigger.                                */
    IPR(ICU, IRQ15)         = 0x1;                              /* Set IPL to lowest setting.                           */
    IR (ICU, IRQ15)         =   0;                              /* Clear any pending interrupt.                         */

    BSP_IntVectSet(79, (CPU_FNCT_VOID)NetDev_WiFiISR_Handler_PMOD2_GT202);

   *p_err = NET_DEV_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                 NetDev_WiFi_GT202_OnBoard_IntCtrl()
*                                  NetDev_WiFi_GT202_PMOD1_IntCtrl()
*                                  NetDev_WiFi_GT202_PMOD2_IntCtrl()
*
* Description : Enable or disable external interrupt.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               en      Enable or disable the interrupt.
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->ExtIntCtrl()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
static  void  NetDev_WiFi_GT202_OnBoard_IntCtrl (NET_IF       *p_if,
                                                 CPU_BOOLEAN   en,
                                                 NET_ERR      *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    IEN(ICU, IRQ11) = (en == DEF_YES ? 1u : 0u);

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD1 > 0
static  void  NetDev_WiFi_GT202_PMOD1_IntCtrl (NET_IF       *p_if,
                                               CPU_BOOLEAN   en,
                                               NET_ERR      *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    IEN(ICU, IRQ12) = (en == DEF_YES ? 1u : 0u);

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD2 > 0
static  void  NetDev_WiFi_GT202_PMOD2_IntCtrl (NET_IF       *p_if,
                                               CPU_BOOLEAN   en,
                                               NET_ERR      *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    IEN(ICU, IRQ15) = (en == DEF_YES ? 1u : 0u);

   *p_err = NET_DEV_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                NetDev_WiFi_GT202_OnBoard_SPI_Init()
*                                 NetDev_WiFi_GT202_PMOD1_SPI_Init()
*                                 NetDev_WiFi_GT202_PMOD2_SPI_Init()
*
* Description : Initialize SPI registers.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_Init()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) This function is called only when the wireless network interface is added.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
static  void  NetDev_WiFi_GT202_OnBoard_SPI_Init (NET_IF   *p_if,
                                                  NET_ERR  *p_err)
{
    CPU_SR_ALLOC();


    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    CPU_CRITICAL_ENTER();
    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_ENABLED);        /* Enable writing to Clock Generation registers.        */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_ENABLED);         /* Enable writing to Operation Mode registers.          */

    MSTP(RSPI1) = 0u;                                           /* Wake RSPI1 unit from standby mode                    */

    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_DISABLED);       /* Disable writing to Clock Generation registers.       */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_DISABLED);        /* Disable writing to Operation Mode registers.         */
    CPU_CRITICAL_EXIT();

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD1 > 0
static  void  NetDev_WiFi_GT202_PMOD1_SPI_Init (NET_IF   *p_if,
                                                NET_ERR  *p_err)
{
    CPU_SR_ALLOC();


    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    CPU_CRITICAL_ENTER();
    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_ENABLED);        /* Enable writing to Clock Generation registers.        */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_ENABLED);         /* Enable writing to Operation Mode registers.          */

    MSTP(RSPI0) = 0u;                                           /* Wake RSPI0 unit from standby mode                    */

    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_DISABLED);       /* Disable writing to Clock Generation registers.       */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_DISABLED);        /* Disable writing to Operation Mode registers.         */
    CPU_CRITICAL_EXIT();

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD2 > 0
static  void  NetDev_WiFi_GT202_PMOD2_SPI_Init (NET_IF   *p_if,
                                                NET_ERR  *p_err)
{
    CPU_SR_ALLOC();


    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    CPU_CRITICAL_ENTER();
    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_ENABLED);        /* Enable writing to Clock Generation registers.        */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_ENABLED);         /* Enable writing to Operation Mode registers.          */

    MSTP(RSPI1) = 0u;                                           /* Wake RSPI1 unit from standby mode                    */

    BSP_Register_Protect(PRC0_CLOCK_GEN, WRITE_DISABLED);       /* Disable writing to Clock Generation registers.       */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_DISABLED);        /* Disable writing to Operation Mode registers.         */
    CPU_CRITICAL_EXIT();

   *p_err = NET_DEV_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                    NetDev_WiFi_GT202_SPI_Lock()
*
* Description : Acquire SPI lock.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Lock()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) This function will be called before the device driver begins to access the SPI.  The
*                   application should NOT use the same bus to access another device until the matching
*                   call to 'NetDev_SPI_Unlock()' has been made.
*********************************************************************************************************
*/

static  void  NetDev_WiFi_GT202_SPI_Lock (NET_IF   *p_if,
                                          NET_ERR  *p_err)
{
    /* TODO: The SPI bus is shared on this device, so a mutex lock/unlock is needed here */
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

   *p_err = NET_DEV_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   NetDev_WiFi_GT202_SPI_Unlock()
*
* Description : Release SPI lock.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->Unlock()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_Lock()' will be called before the device driver begins to access the SPI.
*                   The application should NOT use the same bus to access another device until the
*                   matching call to this function has been made.
*********************************************************************************************************
*/

static  void  NetDev_WiFi_GT202_SPI_Unlock (NET_IF  *p_if)
{
    /* TODO: The SPI bus is shared on this device, so a mutex lock/unlock is needed here */
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */
}


/*
*********************************************************************************************************
*                                NetDev_WiFi_GT202_OnBoard_SPI_WrRd()
*                                 NetDev_WiFi_GT202_PMOD1_SPI_WrRd()
*                                 NetDev_WiFi_GT202_PMOD2_SPI_WrRd()
*
* Description : Write and read from SPI.
*
* Argument(s) : p_if        Pointer to interface to start the hardware.
*               ----        Argument validated in NetIF_Add().
*
*               p_buf_wr    Pointer to buffer to write.
*               --------    Argument validated in $$$$.
*
*               p_buf_rd    Pointer to buffer for data read.
*               --------    Argument validated in $$$$.
*
*               wr_rd_len   Number of octets to write and read.
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_WrRd()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
static  void  NetDev_WiFi_GT202_OnBoard_SPI_WrRd (NET_IF      *p_if,
                                                  CPU_INT08U  *p_buf_wr,
                                                  CPU_INT08U  *p_buf_rd,
                                                  CPU_INT16U   wr_rd_len,
                                                  NET_ERR     *p_err)
{
    CPU_INT32U              i;
    CPU_INT32U              data;
    CPU_INT08U              ptr_incr_wr;
    CPU_INT08U              ptr_incr_rd;
    CPU_INT32U              dummy_wr;
    CPU_INT32U              dummy_rd;
    CPU_INT32U              xfer_cnt;
    CPU_SR_ALLOC();


    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */
                                                                /* Determine buffer pointer incrementation size.        */
    ptr_incr_wr = ptr_incr_rd = 1u;

    xfer_cnt = wr_rd_len / ptr_incr_wr;                         /* Determine the number of transfer loops needed.       */

                                                                /* Setup read/write buffer data pointers.               */
    if (p_buf_wr == DEF_NULL) {
        dummy_wr    = 0xFFFFFFFF;                               /* Populate the write buffer with 1's.                  */
        p_buf_wr    = (CPU_INT08U *)&dummy_wr;
        ptr_incr_wr = 0u;
    } else if (p_buf_rd == DEF_NULL) {
        p_buf_rd    = (CPU_INT08U *)&dummy_rd;
        ptr_incr_rd = 0u;
    }

    RSPI1.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */
    RSPI1.SPCR.BIT.SPTIE  = 1;                                  /* Enable transmit interrupt request.                   */
    RSPI1.SPCR.BIT.SPRIE  = 1;                                  /* Enable receive  interrupt request.                   */
    RSPI1.SPCR.BIT.SPE    = 1;                                  /* Enable RSPI.                                         */

    CPU_CRITICAL_ENTER();
    for (i = 0; i < xfer_cnt; ++i) {
        IR(RSPI1, SPTI1) = 0u;
        IR(RSPI1, SPRI1) = 0u;

                                                                /* Copy write data into SPI data register.              */
        data              = *(CPU_INT08U *)p_buf_wr;
        RSPI1.SPDR.WORD.H =  (CPU_INT08U  )data;

        p_buf_wr += ptr_incr_wr;

        while (IR(RSPI1, SPTI1) != 1u);                         /* Wait for SPTI interrupt.                             */
        while (IR(RSPI1, SPRI1) != 1u);                         /* Wait for SPRI interrupt.                             */

                                                                /* Read data from the SPI data register.                */
        data                  = (CPU_INT32U)RSPI1.SPDR.WORD.H;
      *(CPU_INT08U *)p_buf_rd = (CPU_INT08U)data;

        p_buf_rd += ptr_incr_rd;
    }
    CPU_CRITICAL_EXIT();

    RSPI1.SPCR.BIT.SPTIE  = 0;                                  /* Disable transmit interrupt request.                  */
    RSPI1.SPCR.BIT.SPRIE  = 0;                                  /* Disable receive  interrupt request.                  */

    RSPI1.SPCR2.BIT.SPIIE = 1;                                  /* Re-enable idle interrupt.                            */
    while ((RSPI1.SPSR.BIT.IDLNF) != 0u);                       /* Wait until SPI transfer is complete.                 */

    RSPI1.SPCR.BIT.SPE = 0;                                     /* Disable RSPI.                                        */
    RSPI1.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD1 > 0
static  void  NetDev_WiFi_GT202_PMOD1_SPI_WrRd (NET_IF      *p_if,
                                                CPU_INT08U  *p_buf_wr,
                                                CPU_INT08U  *p_buf_rd,
                                                CPU_INT16U   wr_rd_len,
                                                NET_ERR     *p_err)
{
    CPU_INT32U              i;
    CPU_INT32U              data;
    CPU_INT08U              ptr_incr_wr;
    CPU_INT08U              ptr_incr_rd;
    CPU_INT32U              dummy_wr;
    CPU_INT32U              dummy_rd;
    CPU_INT32U              xfer_cnt;
    CPU_SR_ALLOC();


    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */
                                                                /* Determine buffer pointer incrementation size.        */
    ptr_incr_wr = ptr_incr_rd = 1u;

    xfer_cnt = wr_rd_len / ptr_incr_wr;                         /* Determine the number of transfer loops needed.       */

                                                                /* Setup read/write buffer data pointers.               */
    if (p_buf_wr == DEF_NULL) {
        dummy_wr    = 0xFFFFFFFF;                               /* Populate the write buffer with 1's.                  */
        p_buf_wr    = (CPU_INT08U *)&dummy_wr;
        ptr_incr_wr = 0u;
    } else if (p_buf_rd == DEF_NULL) {
        p_buf_rd    = (CPU_INT08U *)&dummy_rd;
        ptr_incr_rd = 0u;
    }

    RSPI0.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */
    RSPI0.SPCR.BIT.SPTIE  = 1;                                  /* Enable transmit interrupt request.                   */
    RSPI0.SPCR.BIT.SPRIE  = 1;                                  /* Enable receive  interrupt request.                   */
    RSPI0.SPCR.BIT.SPE    = 1;                                  /* Enable RSPI.                                         */

    CPU_CRITICAL_ENTER();
    for (i = 0; i < xfer_cnt; ++i) {
        IR(RSPI0, SPTI0) = 0u;
        IR(RSPI0, SPRI0) = 0u;

                                                                /* Copy write data into SPI data register.              */
        data              = *(CPU_INT08U *)p_buf_wr;
        RSPI0.SPDR.WORD.H =  (CPU_INT08U  )data;

        p_buf_wr += ptr_incr_wr;

        while (IR(RSPI0, SPTI0) != 1u);                         /* Wait for SPTI interrupt.                             */
        while (IR(RSPI0, SPRI0) != 1u);                         /* Wait for SPRI interrupt.                             */

                                                                /* Read data from the SPI data register.                */
        data                  = (CPU_INT32U)RSPI0.SPDR.WORD.H;
      *(CPU_INT08U *)p_buf_rd = (CPU_INT08U)data;

        p_buf_rd += ptr_incr_rd;
    }
    CPU_CRITICAL_EXIT();

    RSPI0.SPCR.BIT.SPTIE  = 0;                                  /* Disable transmit interrupt request.                  */
    RSPI0.SPCR.BIT.SPRIE  = 0;                                  /* Disable receive  interrupt request.                  */

    RSPI0.SPCR2.BIT.SPIIE = 1;                                  /* Re-enable idle interrupt.                            */
    while ((RSPI0.SPSR.BIT.IDLNF) != 0u);                       /* Wait until SPI transfer is complete.                 */

    RSPI0.SPCR.BIT.SPE = 0;                                     /* Disable RSPI.                                        */
    RSPI0.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD2 > 0
static  void  NetDev_WiFi_GT202_PMOD2_SPI_WrRd (NET_IF      *p_if,
                                                CPU_INT08U  *p_buf_wr,
                                                CPU_INT08U  *p_buf_rd,
                                                CPU_INT16U   wr_rd_len,
                                                NET_ERR     *p_err)
{
    CPU_INT32U              i;
    CPU_INT32U              data;
    CPU_INT08U              ptr_incr_wr;
    CPU_INT08U              ptr_incr_rd;
    CPU_INT32U              dummy_wr;
    CPU_INT32U              dummy_rd;
    CPU_INT32U              xfer_cnt;
    CPU_SR_ALLOC();


    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */
                                                                /* Determine buffer pointer incrementation size.        */
    ptr_incr_wr = ptr_incr_rd = 1u;

    xfer_cnt = wr_rd_len / ptr_incr_wr;                         /* Determine the number of transfer loops needed.       */

                                                                /* Setup read/write buffer data pointers.               */
    if (p_buf_wr == DEF_NULL) {
        dummy_wr    = 0xFFFFFFFF;                               /* Populate the write buffer with 1's.                  */
        p_buf_wr    = (CPU_INT08U *)&dummy_wr;
        ptr_incr_wr = 0u;
    } else if (p_buf_rd == DEF_NULL) {
        p_buf_rd    = (CPU_INT08U *)&dummy_rd;
        ptr_incr_rd = 0u;
    }

    RSPI1.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */
    RSPI1.SPCR.BIT.SPTIE  = 1;                                  /* Enable transmit interrupt request.                   */
    RSPI1.SPCR.BIT.SPRIE  = 1;                                  /* Enable receive  interrupt request.                   */
    RSPI1.SPCR.BIT.SPE    = 1;                                  /* Enable RSPI.                                         */

    CPU_CRITICAL_ENTER();
    for (i = 0; i < xfer_cnt; ++i) {
        IR(RSPI1, SPTI1) = 0u;
        IR(RSPI1, SPRI1) = 0u;

                                                                /* Copy write data into SPI data register.              */
        data              = *(CPU_INT08U *)p_buf_wr;
        RSPI1.SPDR.WORD.H =  (CPU_INT08U  )data;

        p_buf_wr += ptr_incr_wr;

        while (IR(RSPI1, SPTI1) != 1u);                         /* Wait for SPTI interrupt.                             */
        while (IR(RSPI1, SPRI1) != 1u);                         /* Wait for SPRI interrupt.                             */

                                                                /* Read data from the SPI data register.                */
        data                  = (CPU_INT32U)RSPI1.SPDR.WORD.H;
      *(CPU_INT08U *)p_buf_rd = (CPU_INT08U)data;

        p_buf_rd += ptr_incr_rd;
    }
    CPU_CRITICAL_EXIT();

    RSPI1.SPCR.BIT.SPTIE  = 0;                                  /* Disable transmit interrupt request.                  */
    RSPI1.SPCR.BIT.SPRIE  = 0;                                  /* Disable receive  interrupt request.                  */

    RSPI1.SPCR2.BIT.SPIIE = 1;                                  /* Re-enable idle interrupt.                            */
    while ((RSPI1.SPSR.BIT.IDLNF) != 0u);                       /* Wait until SPI transfer is complete.                 */

    RSPI1.SPCR.BIT.SPE = 0;                                     /* Disable RSPI.                                        */
    RSPI1.SPCR2.BIT.SPIIE = 0;                                  /* Disable idle interrupt.                              */

   *p_err = NET_DEV_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                              NetDev_WiFi_GT202_OnBoard_SPI_ChipSelEn()
*                               NetDev_WiFi_GT202_PMOD1_SPI_ChipSelEn()
*                               NetDev_WiFi_GT202_PMOD2_SPI_ChipSelEn()
*
* Description : Enable wireless device chip select.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_ChipSelEn()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_ChipSelEn()' will be called before the device driver begins to access
*                   the SPI.
*
*               (2) The chip select is typically 'active low'; to enable the card, the chip select pin
*                   should be cleared.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
static  void  NetDev_WiFi_GT202_OnBoard_SPI_ChipSelEn (NET_IF   *p_if,
                                                       NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    PORTE.PODR.BIT.B0 = 0;

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD1 > 0
static  void  NetDev_WiFi_GT202_PMOD1_SPI_ChipSelEn (NET_IF   *p_if,
                                                     NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    PORTA.PODR.BIT.B4 = 0;

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD2 > 0
static  void  NetDev_WiFi_GT202_PMOD2_SPI_ChipSelEn (NET_IF   *p_if,
                                                     NET_ERR  *p_err)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    PORTE.PODR.BIT.B4 = 0;

   *p_err = NET_DEV_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                             NetDev_WiFi_GT202_OnBoard_SPI_ChipSelDis()
*                              NetDev_WiFi_GT202_PMOD1_SPI_ChipSelDis()
*                              NetDev_WiFi_GT202_PMOD2_SPI_ChipSelDis()
*
* Description : Disable wireless device chip select.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_ChipSelDis()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_ChipSelDis()' will be called when the device driver finished to access
*                   the SPI.
*
*               (1) The chip select is typically 'active low'; to disable the card, the chip select pin
*                   should be set.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
static  void  NetDev_WiFi_GT202_OnBoard_SPI_ChipSelDis (NET_IF  *p_if)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    PORTE.PODR.BIT.B0 = 1;
}

#elif BSP_CFG_GT202_PMOD1 > 0
static  void  NetDev_WiFi_GT202_PMOD1_SPI_ChipSelDis (NET_IF  *p_if)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    PORTA.PODR.BIT.B4 = 1;
}

#elif BSP_CFG_GT202_PMOD2 > 0
static  void  NetDev_WiFi_GT202_PMOD2_SPI_ChipSelDis (NET_IF  *p_if)
{
    UNUSED_PARAM(p_if);                                         /* Prevent 'variable unused' compiler warning           */

    PORTE.PODR.BIT.B4 = 1;
}
#endif


/*
*********************************************************************************************************
*                               NetDev_WiFi_GT202_OnBoard_SPI_SetCfg()
*                                NetDev_WiFi_GT202_PMOD1_SPI_SetCfg()
*                                NetDev_WiFi_GT202_PMOD2_SPI_SetCfg()
*
* Description : Configure SPI bus.
*
* Argument(s) : p_if    Pointer to interface to start the hardware.
*               ----    Argument validated in NetIF_Add().
*
*               freq    Clock frequency, in Hz.
*
*               p_err   Pointer to variable  that will receive the return error code from this function :
*
*                           NET_DEV_ERR_NONE
*
* Return(s)   : none.
*
* Caller(s)   : Device driver via 'p_dev_bsp->SPI_SetCfg()'.
*
*               This function is an INTERNAL network protocol suite function & SHOULD NOT be called by
*               application function(s)
*
* Note(s)     : (1) 'NetDev_SPI_SetClkFreq()' will be called before the device driver begins to access
*                   the SPI and after 'NetDev_SPI_Lock()' has been called.
*
*               (1) The effective clock frequency MUST be no more than 'freq'.  If the frequency cannot be
*                   configured equal to 'freq', it should be configured less than 'freq'.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
void  NetDev_WiFi_GT202_OnBoard_SPI_SetCfg (NET_IF                          *p_if,
                                            NET_DEV_CFG_SPI_CLK_FREQ         freq,
                                            NET_DEV_CFG_SPI_CLK_POL          pol,
                                            NET_DEV_CFG_SPI_CLK_PHASE        pha,
                                            NET_DEV_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                            NET_DEV_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir,
                                            NET_ERR                         *p_err)
{
    CPU_INT32U  pclk;
    CPU_INT08U  spbr;

                                                                /* Set MOSI idle transfer value to '1'.                 */
    RSPI1.SPPCR.BIT.MOIFV = 1;
    RSPI1.SPPCR.BIT.MOIFE = 1;

    RSPI1.SPCR.BIT.SPE  = 0;                                    /* Disable SPI module during initialization.            */
    RSPI1.SPCR.BIT.MSTR = 1;                                    /* Enable Master mode.                                  */

                                                                /* Set baud rate dividers                               */
    RSPI1.SPCMD0.BIT.BRDV = 0;                                  /* N = 0                                                */

    pclk = BSP_ClkFreqGet(BSP_CLK_ID_PCLKA);
    spbr = (CPU_INT08U)(((double)pclk / (freq * 2)) - 1);
    RSPI1.SPBR = spbr;


    if ( pha == NET_DEV_SPI_CLK_PHASE_FALLING_EDGE) {
        RSPI1.SPCMD0.BIT.CPHA = 0;                              /* Data variation on odd edge, data sampling on even edge. */
    } else {
        RSPI1.SPCMD0.BIT.CPHA = 1;                              /* Data variation on even edge, data sampling on odd edge. */
    }

    if (pol == NET_DEV_SPI_CLK_POL_INACTIVE_HIGH) {
        RSPI1.SPCMD0.BIT.CPOL = 0;                              /* RSPCK is high when idle.                             */
    } else {
        RSPI1.SPCMD0.BIT.CPOL = 1;                              /* RSPCK is high when idle.                             */
    }

                                                                /* Select transfer unit length.                         */
    switch (xfer_unit_len) {
        case NET_DEV_SPI_XFER_UNIT_LEN_8_BITS:
             RSPI1.SPCMD0.BIT.SPB = 0x4;                        /* Data length = 8 bits.                                */
             RSPI1.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case NET_DEV_SPI_XFER_UNIT_LEN_16_BITS:
             RSPI1.SPCMD0.BIT.SPB = 0xF;
             RSPI1.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case NET_DEV_SPI_XFER_UNIT_LEN_32_BITS:
             RSPI1.SPCMD0.BIT.SPB = 0x2;
             RSPI1.SPDCR.BIT.SPLW = 1;                          /* SPDR is accessed in longwords.                       */
             break;

        default:
             break;
    }

                                                                /* Select transfer shift direction.                     */
    switch (xfer_shift_dir) {
        case NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_MSB:
             RSPI1.SPCMD0.BIT.LSBF = 0;
             break;

        case NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_LSB:
             RSPI1.SPCMD0.BIT.LSBF = 1;
             break;

        default:
             break;
    }

    RSPI1.SPSCR.BIT.SPSLN = 0x0;                                /* Just use Command Register 0 (SPCMD0).                */

    RSPI1.SPCR2.BIT.SCKASE = 0;

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD1 > 0
void  NetDev_WiFi_GT202_PMOD1_SPI_SetCfg (NET_IF                          *p_if,
                                          NET_DEV_CFG_SPI_CLK_FREQ         freq,
                                          NET_DEV_CFG_SPI_CLK_POL          pol,
                                          NET_DEV_CFG_SPI_CLK_PHASE        pha,
                                          NET_DEV_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                          NET_DEV_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir,
                                          NET_ERR                         *p_err)
{
    CPU_INT32U  pclk;
    CPU_INT08U  spbr;

                                                                /* Set MOSI idle transfer value to '1'.                 */
    RSPI0.SPPCR.BIT.MOIFV = 1;
    RSPI0.SPPCR.BIT.MOIFE = 1;

    RSPI0.SPCR.BIT.SPE  = 0;                                    /* Disable SPI module during initialization.            */
    RSPI0.SPCR.BIT.MSTR = 1;                                    /* Enable Master mode.                                  */

                                                                /* Set baud rate dividers                               */
    RSPI0.SPCMD0.BIT.BRDV = 0;                                  /* N = 0                                                */

    pclk = BSP_ClkFreqGet(BSP_CLK_ID_PCLKA);
    spbr = (CPU_INT08U)(((double)pclk / (freq * 2)) - 1);
    RSPI0.SPBR = spbr;


    if ( pha == NET_DEV_SPI_CLK_PHASE_FALLING_EDGE) {
        RSPI0.SPCMD0.BIT.CPHA = 0;                              /* Data variation on odd edge, data sampling on even edge. */
    } else {
        RSPI0.SPCMD0.BIT.CPHA = 1;                              /* Data variation on even edge, data sampling on odd edge. */
    }

    if (pol == NET_DEV_SPI_CLK_POL_INACTIVE_HIGH) {
        RSPI0.SPCMD0.BIT.CPOL = 0;                              /* RSPCK is high when idle.                             */
    } else {
        RSPI0.SPCMD0.BIT.CPOL = 1;                              /* RSPCK is high when idle.                             */
    }

                                                                /* Select transfer unit length.                         */
    switch (xfer_unit_len) {
        case NET_DEV_SPI_XFER_UNIT_LEN_8_BITS:
             RSPI0.SPCMD0.BIT.SPB = 0x4;                        /* Data length = 8 bits.                                */
             RSPI0.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case NET_DEV_SPI_XFER_UNIT_LEN_16_BITS:
             RSPI0.SPCMD0.BIT.SPB = 0xF;
             RSPI0.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case NET_DEV_SPI_XFER_UNIT_LEN_32_BITS:
             RSPI0.SPCMD0.BIT.SPB = 0x2;
             RSPI0.SPDCR.BIT.SPLW = 1;                          /* SPDR is accessed in longwords.                       */
             break;

        default:
             break;
    }

                                                                /* Select transfer shift direction.                     */
    switch (xfer_shift_dir) {
        case NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_MSB:
             RSPI0.SPCMD0.BIT.LSBF = 0;
             break;

        case NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_LSB:
             RSPI0.SPCMD0.BIT.LSBF = 1;
             break;

        default:
             break;
    }

    RSPI0.SPSCR.BIT.SPSLN = 0x0;                                /* Just use Command Register 0 (SPCMD0).                */

    RSPI0.SPCR2.BIT.SCKASE = 0;

   *p_err = NET_DEV_ERR_NONE;
}

#elif BSP_CFG_GT202_PMOD2 > 0
void  NetDev_WiFi_GT202_PMOD2_SPI_SetCfg (NET_IF                          *p_if,
                                          NET_DEV_CFG_SPI_CLK_FREQ         freq,
                                          NET_DEV_CFG_SPI_CLK_POL          pol,
                                          NET_DEV_CFG_SPI_CLK_PHASE        pha,
                                          NET_DEV_CFG_SPI_XFER_UNIT_LEN    xfer_unit_len,
                                          NET_DEV_CFG_SPI_XFER_SHIFT_DIR   xfer_shift_dir,
                                          NET_ERR                         *p_err)
{
    CPU_INT32U  pclk;
    CPU_INT08U  spbr;

                                                                /* Set MOSI idle transfer value to '1'.                 */
    RSPI1.SPPCR.BIT.MOIFV = 1;
    RSPI1.SPPCR.BIT.MOIFE = 1;

    RSPI1.SPCR.BIT.SPE  = 0;                                    /* Disable SPI module during initialization.            */
    RSPI1.SPCR.BIT.MSTR = 1;                                    /* Enable Master mode.                                  */

                                                                /* Set baud rate dividers                               */
    RSPI1.SPCMD0.BIT.BRDV = 0;                                  /* N = 0                                                */

    pclk = BSP_ClkFreqGet(BSP_CLK_ID_PCLKA);
    spbr = (CPU_INT08U)(((double)pclk / (freq * 2)) - 1);
    RSPI1.SPBR = spbr;


    if ( pha == NET_DEV_SPI_CLK_PHASE_FALLING_EDGE) {
        RSPI1.SPCMD0.BIT.CPHA = 0;                              /* Data variation on odd edge, data sampling on even edge. */
    } else {
        RSPI1.SPCMD0.BIT.CPHA = 1;                              /* Data variation on even edge, data sampling on odd edge. */
    }

    if (pol == NET_DEV_SPI_CLK_POL_INACTIVE_HIGH) {
        RSPI1.SPCMD0.BIT.CPOL = 0;                              /* RSPCK is high when idle.                             */
    } else {
        RSPI1.SPCMD0.BIT.CPOL = 1;                              /* RSPCK is high when idle.                             */
    }

                                                                /* Select transfer unit length.                         */
    switch (xfer_unit_len) {
        case NET_DEV_SPI_XFER_UNIT_LEN_8_BITS:
             RSPI1.SPCMD0.BIT.SPB = 0x4;                        /* Data length = 8 bits.                                */
             RSPI1.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case NET_DEV_SPI_XFER_UNIT_LEN_16_BITS:
             RSPI1.SPCMD0.BIT.SPB = 0xF;
             RSPI1.SPDCR.BIT.SPLW = 0;                          /* SPDR is accessed in words.                           */
             break;

        case NET_DEV_SPI_XFER_UNIT_LEN_32_BITS:
             RSPI1.SPCMD0.BIT.SPB = 0x2;
             RSPI1.SPDCR.BIT.SPLW = 1;                          /* SPDR is accessed in longwords.                       */
             break;

        default:
             break;
    }

                                                                /* Select transfer shift direction.                     */
    switch (xfer_shift_dir) {
        case NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_MSB:
             RSPI1.SPCMD0.BIT.LSBF = 0;
             break;

        case NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_LSB:
             RSPI1.SPCMD0.BIT.LSBF = 1;
             break;

        default:
             break;
    }

    RSPI1.SPSCR.BIT.SPSLN = 0x0;                                /* Just use Command Register 0 (SPCMD0).                */

    RSPI1.SPCR2.BIT.SCKASE = 0;

   *p_err = NET_DEV_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                        NetDev_WiFiISR_Handler()
*
* Description : BSP-level ISR handler(s) for device interrupts.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : CPU &/or device interrupts.
*
* Note(s)     : (1) (a) Each device interrupt, or set of device interrupts, MUST be handled by a
*                       unique BSP-level ISR handler which maps each specific device interrupt to
*                       its corresponding network interface ISR handler.
*
*                   (b) BSP-level device ISR handlers SHOULD be named using the following convention :
*
*                           NetDev_[Device]ISR_Handler[Type][Number]()
*
*                               where
*                                   (1) [Device]        Network device name or type (optional if the
*                                                           development board does NOT support multiple
*                                                           devices)
*                                   (2) [Type]          Network device interrupt type (optional if
*                                                           interrupt type is generic or unknown)
*                                   (3) [Number]        Network device number for each specific instance
*                                                           of device (optional if the development board
*                                                           does NOT support multiple instances of the
*                                                           specific device)
*
*                       See also 'NETWORK DEVICE BSP FUNCTION PROTOTYPES  Note #2a2'.
*********************************************************************************************************
*/
#if BSP_CFG_GT202_ON_BOARD > 0
#if      __RENESAS__
#pragma  interrupt   NetDev_WiFiISR_Handler_OnBoard_GT202
#endif

CPU_ISR  NetDev_WiFiISR_Handler_OnBoard_GT202 (void)
{
    NET_IF_NBR        if_nbr;
    NET_DEV_ISR_TYPE  type;
    CPU_REG32         reg;
    NET_ERR           err;

    OSIntEnter();                                               /* Notify uC/OS-III or uC/OS-II of ISR entry            */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */

    IEN(ICU, IRQ11) = 0;                                        /* Enable interrupt source.                             */
    if_nbr = WiFi_SPI_IF_Nbr;                                   /* See Note #1b3.                                       */
    type   = NET_DEV_ISR_TYPE_UNKNOWN;                          /* See Note #1b2.                                       */

    NetIF_ISR_Handler(if_nbr, type, &err);

    IR(ICU, IRQ11)  = 0;                                        /* Clear any pending ISR.                               */
    IEN(ICU, IRQ11) = 1;                                        /* Enable interrupt source.                             */

    OSIntExit();                                                /* Notify uC/OS-III or uC/OS-II of ISR exit             */
}

#elif BSP_CFG_GT202_PMOD1 > 0
#if      __RENESAS__
#pragma  interrupt   NetDev_WiFiISR_Handler_PMOD1_GT202
#endif

CPU_ISR  NetDev_WiFiISR_Handler_PMOD1_GT202 (void)
{
    NET_IF_NBR        if_nbr;
    NET_DEV_ISR_TYPE  type;
    CPU_REG32         reg;
    NET_ERR           err;


    OSIntEnter();                                               /* Notify uC/OS-III or uC/OS-II of ISR entry            */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */

    IEN(ICU, IRQ12) = 0;                                        /* Enable interrupt source.                             */
    if_nbr = WiFi_SPI_IF_Nbr;                                   /* See Note #1b3.                                       */
    type   = NET_DEV_ISR_TYPE_UNKNOWN;                          /* See Note #1b2.                                       */

    NetIF_ISR_Handler(if_nbr, type, &err);

    IR(ICU, IRQ12)  = 0;                                        /* Clear any pending ISR.                               */
    IEN(ICU, IRQ12) = 1;                                        /* Enable interrupt source.                             */

    OSIntExit();                                                /* Notify uC/OS-III or uC/OS-II of ISR exit             */
}

#elif BSP_CFG_GT202_PMOD2 > 0
#if      __RENESAS__
#pragma  interrupt   NetDev_WiFiISR_Handler_PMOD2_GT202
#endif

CPU_ISR  NetDev_WiFiISR_Handler_PMOD2_GT202 (void)
{
    NET_IF_NBR        if_nbr;
    NET_DEV_ISR_TYPE  type;
    CPU_REG32         reg;
    NET_ERR           err;


    OSIntEnter();                                               /* Notify uC/OS-III or uC/OS-II of ISR entry            */
    CPU_INT_GLOBAL_EN();                                        /* Reenable global interrupts                           */

    IEN(ICU, IRQ15) = 0;                                        /* Enable interrupt source.                             */
    if_nbr = WiFi_SPI_IF_Nbr;                                   /* See Note #1b3.                                       */
    type   = NET_DEV_ISR_TYPE_UNKNOWN;                          /* See Note #1b2.                                       */

    NetIF_ISR_Handler(if_nbr, type, &err);

    IR(ICU, IRQ15)  = 0;                                        /* Clear any pending ISR.                               */
    IEN(ICU, IRQ15) = 1;                                        /* Enable interrupt source.                             */

    OSIntExit();                                                /* Notify uC/OS-III or uC/OS-II of ISR exit             */
}
#endif

#endif
