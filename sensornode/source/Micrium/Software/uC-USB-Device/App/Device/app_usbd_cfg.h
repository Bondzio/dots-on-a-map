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
*                                   USBD APPLICATION CONFIGURATION
*
*                                              TEMPLATE
*
* Filename      : app_usbd_cfg.h
* Version       : V4.05.00
* Programmer(s) : OD
*********************************************************************************************************
*/

#ifndef  APP_USBD_CFG_MODULE_PRESENT
#define  APP_USBD_CFG_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <lib_def.h>


/*
*********************************************************************************************************
*                                         CLASSES DEMO ENABLE
*********************************************************************************************************
*/

#define  APP_CFG_USBD_AUDIO_EN                  DEF_ENABLED
#define  APP_CFG_USBD_CDC_EN                    DEF_ENABLED
#define  APP_CFG_USBD_CDC_EEM_EN                DEF_ENABLED
#define  APP_CFG_USBD_HID_EN                    DEF_ENABLED
#define  APP_CFG_USBD_MSC_EN                    DEF_ENABLED
#define  APP_CFG_USBD_PHDC_EN                   DEF_ENABLED
#define  APP_CFG_USBD_VENDOR_EN                 DEF_ENABLED


/*
*********************************************************************************************************
*                                   AUDIO APPLICATION CONFIGURATION
*********************************************************************************************************
*/

                                                                /* Type of demo enabled:                                */
                                                                /*     DEF_ENABLED:  Loopback between board and PC,     */
                                                                /*     DEF_DISABLED: Microphone-only.                   */
#define  APP_CFG_USBD_AUDIO_SIMULATION_LOOP_EN              DEF_DISABLED

#if (APP_CFG_USBD_AUDIO_SIMULATION_LOOP_EN == DEF_DISABLED)

#define  USBD_AUDIO_DRV_SIMULATION_DATA_WAVEFORM_SINE       0u  /* Continuous sine     waveform.                        */
#define  USBD_AUDIO_DRV_SIMULATION_DATA_WAVEFORM_SQUARE     1u  /* Continuous square   waveform.                        */
#define  USBD_AUDIO_DRV_SIMULATION_DATA_WAVEFORM_SAWTOOTH   2u  /* Continuous sawtooth waveform.                        */
#define  USBD_AUDIO_DRV_SIMULATION_DATA_WAVEFORM_BEEP_BEEP  3u  /* Half a second ON, half a second OFF (sawtooth used). */

#define  USBD_AUDIO_DRV_SIMULATION_DATA_FREQ_100_HZ         0u  /*  100 Hz frequency.                                   */
#define  USBD_AUDIO_DRV_SIMULATION_DATA_FREQ_1000_HZ        1u  /* 1000 Hz frequency.                                   */

                                                                /* Sound type.                                          */
#define  APP_CFG_USBD_AUDIO_DRV_SIMULATION_DATA_WAVEFORM    USBD_AUDIO_DRV_SIMULATION_DATA_WAVEFORM_SINE
                                                                /* Signal frequency.                                    */
#define  APP_CFG_USBD_AUDIO_DRV_SIMULATION_DATA_FREQ        USBD_AUDIO_DRV_SIMULATION_DATA_FREQ_100_HZ
#endif

#define  APP_CFG_USBD_AUDIO_TASKS_Q_LEN                    20u  /* Queue len for playback & record tasks.               */

#define  APP_CFG_USBD_AUDIO_RECORD_CORR_PERIOD             16u  /* Record correction period in ms.                      */


#if (APP_CFG_USBD_AUDIO_SIMULATION_LOOP_EN == DEF_ENABLED)
#define  APP_CFG_USBD_AUDIO_PLAYBACK_CORR_PERIOD           16u  /* Playback correction period in ms.                    */

#define  APP_CFG_USBD_AUDIO_NBR_ENTITY                      6u  /* Nbr of entities (units/terminals) used by audio fnct.*/
#else
#define  APP_CFG_USBD_AUDIO_NBR_ENTITY                      3u  /* Nbr of entities (units/terminals) used by audio fnct.*/
#endif


/*
*********************************************************************************************************
*                                  CDC-ACM APPLICATION CONFIGURATION
*********************************************************************************************************
*/

#define  APP_CFG_USBD_CDC_SERIAL_TEST_EN                    DEF_ENABLED


/*
*********************************************************************************************************
*                                    HID APPLICATION CONFIGURATION
*********************************************************************************************************
*/

#define  APP_CFG_USBD_HID_TEST_MOUSE_EN                     DEF_ENABLED


/*
*********************************************************************************************************
*                                    MSC APPLICATION CONFIGURATION
*********************************************************************************************************
*/

#define  USBD_RAMDISK_CFG_NBR_UNITS                         1u
#define  USBD_RAMDISK_CFG_BLK_SIZE                        512u
#define  USBD_RAMDISK_CFG_NBR_BLKS                       4096u
#define  USBD_RAMDISK_CFG_BASE_ADDR                         0u


/*
*********************************************************************************************************
*                                    PHDC APPLICATION CONFIGURATION
*********************************************************************************************************
*/

#define  APP_CFG_USBD_PHDC_ITEM_DATA_LEN_MAX             1024u  /* Should be the same value as host app.                */
#define  APP_CFG_USBD_PHDC_ITEM_NBR_MAX                    15u  /* Should be the same value as host app.                */
#define  APP_CFG_USBD_PHDC_MAX_NBR_TASKS                    1u  /* Number of tasks that will be created to handle QoS.  */


/*
*********************************************************************************************************
*                                  VENDOR APPLICATION CONFIGURATION
*********************************************************************************************************
*/

#define  APP_CFG_USBD_VENDOR_ECHO_SYNC_EN                   DEF_ENABLED
#define  APP_CFG_USBD_VENDOR_ECHO_ASYNC_EN                  DEF_ENABLED

#endif
