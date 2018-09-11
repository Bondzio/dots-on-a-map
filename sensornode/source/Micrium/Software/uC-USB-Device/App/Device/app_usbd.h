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
*                                   USB APPLICATION INITIALIZATION
*
*                                              TEMPLATE
*
* File          : app_usbd.h
* Version       : V4.05.00
* Programmer(s) : FGK
*                 FT
*********************************************************************************************************
*/


#ifndef  APP_USBD_MODULE_PRESENT
#define  APP_USBD_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  <app_cfg.h>
#include  <app_usbd_cfg.h>


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   APP_USBD_MODULE
#define  APP_USBD_MODULE_EXT
#else
#define  APP_USBD_MODULE_EXT  extern
#endif


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                  DEFAULT USB DEVICE CONFIGURATION
*********************************************************************************************************
*/

#ifndef  APP_CFG_USBD_EN
#define  APP_CFG_USBD_EN                        DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_CDC_EN
#define  APP_CFG_USBD_CDC_EN                    DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_CDC_SERIAL_TEST_EN
#define  APP_CFG_USBD_CDC_SERIAL_TEST_EN        DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_HID_EN
#define  APP_CFG_USBD_HID_EN                    DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_HID_TEST_MOUSE_EN
#define  APP_CFG_USBD_HID_TEST_MOUSE_EN         DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_VENDOR_EN
#define  APP_CFG_USBD_VENDOR_EN                 DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_VENDOR_ECHO_SYNC_EN
#define  APP_CFG_USBD_VENDOR_ECHO_SYNC_EN       DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN
#define  APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN      DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_PHDC_EN
#define  APP_CFG_USBD_PHDC_EN                   DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_MSC_EN
#define  APP_CFG_USBD_MSC_EN                    DEF_DISABLED
#endif

#ifndef  APP_CFG_USBD_AUDIO_EN
#define  APP_CFG_USBD_AUDIO_EN                  DEF_DISABLED
#endif


/*
*********************************************************************************************************
*                                           TRACE FUNCTIONS
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                     0u
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                     1u
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                    2u
#endif

#ifndef  APP_CFG_TRACE_LEVEL
#define  APP_CFG_TRACE_LEVEL       TRACE_LEVEL_OFF
#endif


/*
*********************************************************************************************************
*                                      CONDITIONAL INCLUDE FILES
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

#if (APP_CFG_USBD_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_Init          (void);
#endif

#if (APP_CFG_USBD_CDC_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_CDC_Init      (CPU_INT08U  dev_nbr,
                                     CPU_INT08U  cfg_hs,
                                     CPU_INT08U  cfg_fs);
#endif

#if (APP_CFG_USBD_CDC_EEM_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_CDC_EEM_Init  (CPU_INT08U  dev_nbr,
                                     CPU_INT08U  cfg_hs,
                                     CPU_INT08U  cfg_fs);
#endif

#if (APP_CFG_USBD_HID_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_HID_Init      (CPU_INT08U  dev_nbr,
                                     CPU_INT08U  cfg_hs,
                                     CPU_INT08U  cfg_fs);
#endif

#if (APP_CFG_USBD_VENDOR_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_Vendor_Init   (CPU_INT08U  dev_nbr,
                                     CPU_INT08U  cfg_hs,
                                     CPU_INT08U  cfg_fs);
#endif

#if (APP_CFG_USBD_PHDC_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_PHDC_Init     (CPU_INT08U  dev_nbr,
                                     CPU_INT08U  cfg_hs,
                                     CPU_INT08U  cfg_fs);
#endif

#if (APP_CFG_USBD_MSC_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_MSC_Init      (CPU_INT08U  dev_nbr,
                                     CPU_INT08U  cfg_hs,
                                     CPU_INT08U  cfg_fs);
#endif

#if (APP_CFG_USBD_AUDIO_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBD_Audio_Init    (CPU_INT08U  dev_nbr,
                                     CPU_INT08U  cfg_hs,
                                     CPU_INT08U  cfg_fs);
#endif


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/


#ifndef  APP_CFG_USBD_EN
#error  "APP_CFG_USBD_EN                            not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_EN                      illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif

#if     (APP_CFG_USBD_EN == DEF_ENABLED)
                                                                /* ------------------ CDC CLASS CHECK ----------------- */
#ifndef  APP_CFG_USBD_CDC_EN
#error  "APP_CFG_USBD_CDC_EN                        not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_CDC_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_CDC_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_CDC_EN                  illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  !defined(APP_CFG_USBD_CDC_SERIAL_TEST_EN)
#error  "APP_CFG_USBD_CDC_SERIAL_TEST_EN            not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  (APP_CFG_USBD_CDC_SERIAL_TEST_EN == DEF_ENABLED) && \
       (!defined(APP_CFG_USBD_CDC_SERIAL_TASK_STK_SIZE)   )
#error  "APP_CFG_USBD_CDC_SERIAL_TASK_STK_SIZE      not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "

#elif  (APP_CFG_USBD_CDC_SERIAL_TEST_EN == DEF_ENABLED) && \
       (!defined(APP_CFG_USBD_CDC_SERIAL_TASK_PRIO)   )
#error  "APP_CFG_USBD_CDC_SERIAL_TASK_PRIO          not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "
#endif

                                                                /* ------------------ HID CLASS CHECK ----------------- */
#ifndef  APP_CFG_USBD_HID_EN
#error  "APP_CFG_USBD_HID_EN                        not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_HID_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_HID_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_HID_EN                  illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif

#ifndef  APP_CFG_USBD_HID_TEST_MOUSE_EN
#error  "APP_CFG_USBD_HID_TEST_MOUSE_EN             not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_HID_TEST_MOUSE_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_HID_TEST_MOUSE_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_HID_TEST_MOUSE_EN       illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif (APP_CFG_USBD_HID_TEST_MOUSE_EN == DEF_ENABLED) && \
      (!defined(APP_CFG_USBD_HID_TASK_STK_SIZE)     )
#error  "APP_CFG_USBD_HID_TASK_STK_SIZE             not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "

#elif (APP_CFG_USBD_HID_TEST_MOUSE_EN == DEF_ENABLED) && \
      (!defined(APP_CFG_USBD_HID_MOUSE_TASK_PRIO)   )
#error  "APP_CFG_USBD_HID_MOUSE_TASK_PRIO           not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "

#elif (APP_CFG_USBD_HID_TEST_MOUSE_EN == DEF_ENABLED) && \
      (!defined(APP_CFG_USBD_HID_TASK_STK_SIZE)     )
#error  "APP_CFG_USBD_HID_TASK_STK_SIZE             not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "
#endif

                                                                /* ---------------- VENDOR CLASS CHECK ---------------- */
#ifndef  APP_CFG_USBD_VENDOR_EN
#error  "APP_CFG_USBD_VENDOR_EN                     not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_VENDOR_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_VENDOR_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_VENDOR_EN               illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif

#if     (APP_CFG_USBD_VENDOR_EN == DEF_ENABLED)

#ifndef  APP_CFG_USBD_VENDOR_ECHO_SYNC_EN
#error  "APP_CFG_USBD_VENDOR_ECHO_SYNC_EN           not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_VENDOR_ECHO_SYNC_EN != DEF_ENABLED ) && \
        (APP_CFG_USBD_VENDOR_ECHO_SYNC_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_VENDOR_ECHO_SYNC_EN     illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif

#ifndef  APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN
#error  "APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN          not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "


#elif  ((APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN != DEF_ENABLED ) && \
        (APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN    illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif

#if    ((APP_CFG_USBD_VENDOR_ECHO_SYNC_EN  == DEF_ENABLED) || \
        (APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN == DEF_ENABLED))
#ifndef  APP_CFG_USBD_VENDOR_TASK_STK_SIZE
#error  "APP_CFG_USBD_VENDOR_TASK_STK_SIZE          not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "
#endif
#endif

#if     (APP_CFG_USBD_VENDOR_ECHO_SYNC_EN == DEF_ENABLED)
#ifndef  APP_CFG_USBD_VENDOR_ECHO_SYNC_TASK_PRIO
#error  "APP_CFG_USBD_VENDOR_ECHO_SYNC_TASK_PRIO    not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "
#endif
#endif

#if     (APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN == DEF_ENABLED)
#ifndef  APP_CFG_USBD_VENDOR_ECHO_ASYNC_TASK_PRIO
#error  "APP_CFG_USBD_VENDOR_ECHO_ASYNC_TASK_PRIO   not #defined in 'app_cfg.h'  "
#error  "                              [MUST be > 0u ]                           "
#endif
#endif
#endif

                                                                /* -------------------- PHDC CHECK -------------------- */
#ifndef  APP_CFG_USBD_PHDC_EN
#error  "APP_CFG_USBD_PHDC_EN                       not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_PHDC_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_PHDC_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_PHDC_EN                 illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif

#if    (APP_CFG_USBD_PHDC_EN == DEF_ENABLED)
#ifndef  APP_CFG_USBD_PHDC_TX_COMM_TASK_PRIO
#error  "APP_CFG_USBD_PHDC_TX_COMM_TASK_PRIO   not #define'd in 'app_cfg.h'         "
#endif

#ifndef  APP_CFG_USBD_PHDC_RX_COMM_TASK_PRIO
#error  "APP_CFG_USBD_PHDC_RX_COMM_TASK_PRIO   not #define'd in 'app_cfg.h'         "
#endif

#ifndef  APP_CFG_USBD_PHDC_TASK_STK_SIZE
#error  "APP_CFG_USBD_PHDC_TASK_STK_SIZE       not #define'd in 'app_cfg.h'         "
#endif

#ifndef  APP_CFG_USBD_PHDC_ITEM_DATA_LEN_MAX
#error  "APP_CFG_USBD_PHDC_ITEM_DATA_LEN_MAX   not #define'd in 'app_cfg.h'         "
#error  "                                      [MUST be >= 5]                       "
#endif

#if  (APP_CFG_USBD_PHDC_ITEM_DATA_LEN_MAX < 5u)
#error  "APP_CFG_USBD_PHDC_ITEM_DATA_LEN_MAX   illegally #define'd in 'app_cfg.h' "
#error  "                                      [MUST be >= 5]                     "
#endif

#ifndef  APP_CFG_USBD_PHDC_ITEM_NBR_MAX
#error  "APP_CFG_USBD_PHDC_ITEM_NBR_MAX        not #define'd in 'app_cfg.h'         "
#error  "                                      [MUST be >= 1]                       "
#endif

#if  (APP_CFG_USBD_PHDC_ITEM_NBR_MAX < 1u)
#error  "APP_CFG_USBD_PHDC_ITEM_NBR_MAX        illegally #define'd in 'app_cfg.h'   "
#error  "                                      [MUST be >= 1]                       "
#endif
#endif

                                                                /* ------------------ MSC CLASS CHECK ----------------- */
#ifndef  APP_CFG_USBD_MSC_EN
#error  "APP_CFG_USBD_MSC_EN                        not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_MSC_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_MSC_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_MSC_EN                  illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif

                                                                /* ----------------- AUDIO CLASS CHECK ---------------- */
#ifndef  APP_CFG_USBD_AUDIO_EN
#error  "APP_CFG_USBD_AUDIO_EN                      not #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBD_AUDIO_EN != DEF_ENABLED) && \
        (APP_CFG_USBD_AUDIO_EN != DEF_DISABLED))
#error  "APP_CFG_USBD_AUDIO_EN                illegally #defined in 'app_cfg.h'  "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "
#endif
#endif


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif                                                          /* End of app USB module include.                       */
