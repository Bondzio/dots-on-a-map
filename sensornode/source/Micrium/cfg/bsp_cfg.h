/*
*********************************************************************************************************
*
*                                         BOARD SUPPORT PACKAGE
*
*                                 (c) Copyright 2015, Micrium, Weston, FL
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

#define  BSP_CFG_CAN_EN                     0    /* Enable (1) or Disable (0) uC/CAN                   */
#define  BSP_CFG_CAN_OPEN_EN                0    /* Enable (1) or Disable (0) uC/CANopen               */
#define  BSP_CFG_FS_EN                      0    /* Enable (1) or Disable (0) uC/FS                    */
#define  BSP_CFG_GUI_EN                     0    /* Enable (1) or Disable (0) uC/GUI                   */
#define  BSP_CFG_MBM_EN                     0    /* Enable (1) or Disable (0) uC/Modbus-Master         */
#define  BSP_CFG_MBS_EN                     0    /* Enable (1) or Disable (0) uC/Modbus-Slave          */
#define  BSP_CFG_NET_EN                     1    /* Enable (1) or Disable (0) uC/TCP-IP                */
#define  BSP_CFG_OS2_EN                     0    /* Enable (1) or Disable (0) uC/OS-II                 */
#define  BSP_CFG_OS3_EN                     1    /* Enable (1) or Disable (0) uC/OS-III                */
#define  BSP_CFG_USBD_EN                    1    /* Enable (1) or Disable (0) uC/USB-D                 */
#define  BSP_CFG_USBH_EN                    0    /* Enable (1) or Disable (0) uC/USB-H                 */

/*
*********************************************************************************************************
*                                       INTERRUPT VECTOR TABLE
*********************************************************************************************************
*/

#define  BSP_CFG_INT_VECT_TBL_RAM_EN        0    /* Enable (1) vector table in RAM or in ROM (0)       */
#define  BSP_CFG_INT_VECT_TBL_SIZE        256    /* Max. number of entries in the interrupt vector tbl */


/*
*********************************************************************************************************
*                                     INTERRUPT PRIORITY LEVELS
*********************************************************************************************************
*/

#define  BSP_CFG_OS_TICK_IPL                3

#define  BSP_CFG_SER_IPL                    3

#endif
