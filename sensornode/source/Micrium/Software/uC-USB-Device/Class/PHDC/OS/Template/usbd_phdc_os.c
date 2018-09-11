/*
*********************************************************************************************************
*                                            uC/USB-Device
*                                       The Embedded USB Stack
*
*                         (c) Copyright 2004-2013; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/USB is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                 USB PHDC CLASS OPERATING SYSTEM LAYER
*                                          Micrium Template
*
* File          : usbd_phdc_os.c
* Version       : V4.05.00
* Programmer(s) :
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../usbd_phdc.h"
#include  "../../usbd_phdc_os.h"


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


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
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  USBD_PHDC_OS_WrBulkSchedTask(void  *p_arg);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBD_PHDC_OS_Init()
*
* Description : Initialize PHDC OS interface.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE               OS initialization successful.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_Init (USBD_ERR  *p_err)
{
    /* $$$$ Initialize all internal variables and tasks used by OS layer. */
   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBD_PHDC_OS_RdLock()
*
* Description : Lock PHDC read pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
*               timeout_ms  Timeout, in ms.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS signal     successfully acquired.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Rd(), USBD_PHDC_RdPreamble().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_RdLock (CPU_INT08U   class_nbr,
                           CPU_INT16U   timeout_ms,
                           USBD_ERR    *p_err)
{
    /* $$$$ Lock read pipe. */
   *p_err = USBD_ERR_NONE;

}


/*
*********************************************************************************************************
*                                       USBD_PHDC_OS_RdUnlock()
*
* Description : Unlock PHDC read pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Rd(), USBD_PHDC_RdPreamble().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_RdUnlock (CPU_INT08U  class_nbr)
{
    /* $$$$ Unlock read pipe. */
}


/*
*********************************************************************************************************
*                                      USBD_PHDC_OS_WrIntrLock()
*
* Description : Lock PHDC write interrupt pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
*               timeout_ms  Timeout, in ms.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS signal     successfully acquired.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrIntrLock (CPU_INT08U   class_nbr,
                               CPU_INT16U   timeout_ms,
                               USBD_ERR    *p_err)
{
    /* $$$$ Lock interrupt write pipe. */
   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBD_PHDC_OS_WrIntrUnlock()
*
* Description : Unlock PHDC write interrupt pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrIntrUnlock (CPU_INT08U  class_nbr)
{
    /* $$$$ Unlock interrupt write pipe. */
}


/*
*********************************************************************************************************
*                                      USBD_PHDC_OS_WrBulkLock()
*
* Description : Lock PHDC write bulk pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
*               prio        Priority of the transfer, based on its QoS.
*
*               timeout_ms  Timeout, in ms.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS signal     successfully acquired.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr(), USBD_PHDC_WrPreamble().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrBulkLock (CPU_INT08U   class_nbr,
                               CPU_INT08U   prio,
                               CPU_INT16U   timeout_ms,
                               USBD_ERR    *p_err)
{
    /* $$$$ Lock bulk write pipe. */
   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBD_PHDC_OS_WrBulkUnlock()
*
* Description : Unlock PHDC write bulk pipe.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Wr(), USBD_PHDC_WrPreamble().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_WrBulkUnlock (CPU_INT08U  class_nbr)
{
    /* $$$$ Unlock bulk wirte pipe. */
}


/*
*********************************************************************************************************
*                                        USBD_PHDC_OS_Reset()
*
* Description : Reset PHDC OS layer for given instance.
*
* Argument(s) : class_nbr   PHDC instance number;
*
* Return(s)   : none.
*
* Caller(s)   : USBD_PHDC_Reset().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_PHDC_OS_Reset (CPU_INT08U  class_nbr)
{
    /* $$$$ Reset OS layer. */
}


/*
*********************************************************************************************************
*                                   USBD_PHDC_OS_WrBulkSchedTask()
*
* Description : OS-dependent shell task to schedule bulk transfers in function of their priority.
*
* Argument(s) : p_arg       Pointer to task initialization argument (required by uC/OS-III).
*
* Return(s)   : none.
*
* Created by  : USBD_PHDC_OS_Init().
*
* Note(s)     : (1) Only one task handle all class instances bulk write scheduling.
*********************************************************************************************************
*/

static  void  USBD_PHDC_OS_WrBulkSchedTask (void *p_arg)
{
    /* $$$$ If QoS prioritization is used, implement this task to act as a scheduler. */
}
