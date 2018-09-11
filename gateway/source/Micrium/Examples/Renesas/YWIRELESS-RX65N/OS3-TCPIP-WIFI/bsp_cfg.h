/*
*********************************************************************************************************
*
*                                         BOARD SUPPORT PACKAGE
*
*                                 (c) Copyright 2016, Micrium, Weston, FL
*                                          All Rights Reserved
*
* Filename      : bsp_cfg.h
* Version       : V1.00
* Programmer(s) : JJL
*********************************************************************************************************
*/

#ifndef  BSP_CFG_H_
#define  BSP_CFG_H_


/*
*********************************************************************************************************
*                                            MODULES ENABLE
*********************************************************************************************************
*/

#define  BSP_CFG_GT202_ON_BOARD             1    /* Enable (1) or Disable (0) uC/TCP-IP w/ GT202 WiFi  */
#define  BSP_CFG_GT202_PMOD1                0    /* Enable (1) or Disable (0) uC/TCP-IP w/ GT202 WiFi  */
#define  BSP_CFG_GT202_PMOD2                0    /* Enable (1) or Disable (0) uC/TCP-IP w/ GT202 WiFi  */
#define  BSP_CFG_OS2_EN                     0    /* Enable (1) or Disable (0) uC/OS-II                 */
#define  BSP_CFG_OS3_EN                     1    /* Enable (1) or Disable (0) uC/OS-III                */
#define  BSP_CFG_USBD_EN                    0    /* Enable (1) or Disable (0) uC/USB-D                 */
#define  BSP_CFG_USBH_EN                    0    /* Enable (1) or Disable (0) uC/USB-H                 */


/*
*********************************************************************************************************
*                                        BOARD SPECIFIC BSPs
*********************************************************************************************************
*/

#define  BSP_CFG_LED_EN                     1    /* Enable (1) or Disable (0) LEDs                     */
#define  BSP_CFG_PB_EN                      1    /* Enable (1) or Disable (0) Push buttons             */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                           /* End of module include.                             */
