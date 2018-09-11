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
*                                   USB AUDIO DEVICE CONFIGURATION FILE
*
* File           : usbd_audio_drv_uda1380_codec.h
* Version        : V4.05.00.00
* Programmer(s)  : CM
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This USB device configuration header file is protected from multiple pre-processor
*               inclusion through use of the USB device configuration module present pre-processor
*               macro definition.
*********************************************************************************************************
*/

#ifndef  USBD_AUDIO_DRV_TEMPLATE_MODULE_PRESENT                 /* See Note #1.                                         */
#define  USBD_AUDIO_DRV_TEMPLATE_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../usbd_audio_processing.h"


/*
*********************************************************************************************************
*                            USB AUDIO FUNCTION TOPOLOGY 1 CONFIGURATION
*********************************************************************************************************
*/

extern  const  USBD_AUDIO_DRV_COMMON_API  USBD_Audio_DrvCommonAPI_UDA1380_Codec;
extern  const  USBD_AUDIO_DRV_AC_FU_API   USBD_Audio_DrvFU_API_UDA1380_Codec;
extern  const  USBD_AUDIO_DRV_AS_API      USBD_Audio_DrvAS_API_UDA1380_Codec;

/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif
