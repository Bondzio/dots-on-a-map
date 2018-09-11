/*
*********************************************************************************************************
*                                               QCA400x
*
*                         (c) Copyright 2004-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  QCA400x is provided in source form to registered licensees ONLY.  It is
*                  illegal to distribute this source code to any third party unless you receive
*                  written permission by an authorized Micrium representative.  Knowledge of
*                  the source code may NOT be used to develop a similar product.
*
*                  Please help us continue to provide the Embedded community with the finest
*                  software available.  Your honesty is greatly appreciated.
*
*                  You can find our product's user manual, API reference, release notes and
*                  more information at: https://doc.micrium.com
*
*                  You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                      QCA400x CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename : qca_cfg.h
* Version  : V1.00.00
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*********************************************************************************************************
*                                           INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <qca_type.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  QCA_DEV_CFG_MODULE_PRESENT
#define  QCA_DEV_CFG_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      COMPILE-TIME CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                   QCA ARGUMENT CHECK CONFIGURATION
*
* Note(s) : (1) Configure QCA_CFG_ARG_CHK_EXT_EN to enable/disable the QCA400x Driver external argument
*               check feature :
*
*               (a) When ENABLED,  ALL arguments received from any port interface provided by the developer
*                   are checked/validated.
*
*               (b) When DISABLED, NO  arguments received from any port interface provided by the developer
*                   are checked/validated.
*********************************************************************************************************
*/

#define  QCA_CFG_ARG_CHK_EXT_EN                 DEF_ENABLED     /* See Note #1.                                         */

/*
*********************************************************************************************************
*                                QCA HEAP USAGE CALCULATION CONFIGURATION
*
* Note(s) : (1) Configure QCA_CFG_HEAP_USAGE_CALC_EN to enable/disable heap usage calculation source code.
*********************************************************************************************************
*/

#define  QCA_CFG_HEAP_USAGE_CALC_EN             DEF_ENABLED     /* See Note #1.                                         */


/*
*********************************************************************************************************
*                                     QCA SOCKET CONFIGURATION
*
* Note(s) : (1) Configure QCA_CFG_SOCK_ZERO_COPY_EN to enable/disable zero copy socket API.
*
*           (1) Configure QCA_CFG_SOCK_NON_BLOCKING_TX_EN to enable/disable RX socket in  API.
*
*           (1) Configure QCA_CFG_SOCK_NON_BLOCKING_TX_EN to enable/disable zero copy socket API.
*********************************************************************************************************
*/

#define  QCA_CFG_SOCK_ZERO_COPY_EN              DEF_DISABLED    /* See Note #1.                                         */

#define  QCA_CFG_SOCK_NON_BLOCKING_TX_EN        DEF_DISABLED    /* See Note #2.                                         */
#define  QCA_CFG_SOCK_NON_BLOCKING_RX_EN        DEF_DISABLED    /* See Note #3.                                         */

/*
*********************************************************************************************************
*********************************************************************************************************
*                                      RUN-TIME CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

extern const  QCA_TASK_CFG QCA_TaskCfg;
extern const  QCA_DEV_CFG  QCA_DevCfg;

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* QCA_DEV_CFG_MODULE_PRESENT */

