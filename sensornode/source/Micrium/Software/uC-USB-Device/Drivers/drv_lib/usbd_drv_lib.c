/*
*********************************************************************************************************
*                                            uC/USB-Device
*                                       The Embedded USB Stack
*
*                         (c) Copyright 2004-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/USB-Device is provided in source form to registered licensees ONLY.  It is
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
*                                           USB DEVICE DRIVER
*
*                                            Common library
*
* File          : usbd_drv_lib.c
* Version       : V4.05.00.00
* Programmer(s) : OD
*                 JFD
*********************************************************************************************************
* Note(s)       : (1) This library offers some functions useful to some drivers.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../Source/usbd_core.h"
#include  "usbd_drv_lib.h"


/*
*********************************************************************************************************
*                                             LOCAL MACROS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  USBD_DrvLib_SetupPktQSend (USBD_DRV_LIB_SETUP_PKT_Q  *p_setup_pkt_q,
                                         USBD_DRV                  *p_drv);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     USBD_DrvLib_SetupPktQInit()
*
* Description : Initializes structure used to handle the setup packet Q-ing.
*
* Argument(s) : p_setup_pkt_q       Pointer to setup packet queue structure.
*
*               q_size              Number of setup packet that the queue contains.
*
*               p_err           Pointer to variable that will receive the error code from this function:
*
*                               USBD_ERR_NONE       Operation was successful.
*                               USBD_ERR_ALLOC      Cannot allocate setup packet queue.
*
* Return(s)   : None.
*
* Caller(s)   : Driver's init() function.
*
* Note(s)     : (1) This function must be called from the init() function of the driver to create a
*                   setup packet queue.
*********************************************************************************************************
*/

void  USBD_DrvLib_SetupPktQInit (USBD_DRV_LIB_SETUP_PKT_Q  *p_setup_pkt_q,
                                 CPU_INT08U                 q_size,
                                 USBD_ERR                  *p_err)
{
    LIB_ERR  err_lib;


    p_setup_pkt_q->SetupPktTblPtr = (USBD_DRV_LIB_SETUP_PKT *)Mem_HeapAlloc(             (q_size * sizeof(USBD_DRV_LIB_SETUP_PKT)),
                                                                                          sizeof(CPU_INT32U),
                                                                            (CPU_SIZE_T *)0,
                                                                                         &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBD_ERR_ALLOC;
        return;
    }

    p_setup_pkt_q->TblLen = q_size;

   *p_err = USBD_ERR_NONE;
}

/*
*********************************************************************************************************
*                                      USBD_DrvLib_SetupPktQClr()
*
* Description : Clears the setup packet queue control structure.
*
* Argument(s) : p_setup_pkt_q       Pointer to setup packet queue structure.
*
* Return(s)   : none.
*
* Caller(s)   : Driver's ISR handler.
*
* Note(s)     : (1) This function must be called each time the USB device controller is reset (on reset
*                   event coming from the bus).
*********************************************************************************************************
*/

void  USBD_DrvLib_SetupPktQClr (USBD_DRV_LIB_SETUP_PKT_Q  *p_setup_pkt_q)
{
    p_setup_pkt_q->Nbr   = 0u;                                  /* Clr internal data and ix.                            */
    p_setup_pkt_q->IxIn  = 0u;
    p_setup_pkt_q->IxOut = 0u;

    Mem_Clr((void *)p_setup_pkt_q->SetupPktTblPtr,              /* Clr every buf's content.                             */
                   (p_setup_pkt_q->TblLen * sizeof(USBD_DRV_LIB_SETUP_PKT)));
}


/*
*********************************************************************************************************
*                                      USBD_DrvLib_SetupPktQAdd()
*
* Description : Enqueues a setup packet in the circular buffer.
*
* Argument(s) : p_setup_pkt_q       Pointer to setup packet queue structure.
*
*               p_drv               Pointer to device driver structure.
*
*               p_setup_pkt_buf     Pointer to buffer that contains the setup packet to add to the Q.
*                                   ---- Must be aligned on a 32 bit boundary. ----
*
* Return(s)   : None.
*
* Caller(s)   : Driver's ISR handler.
*
* Note(s)     : (1) This function must be called each time either a real or "dummy" setup packet must be
*                   submitted to the core.
*********************************************************************************************************
*/

void  USBD_DrvLib_SetupPktQAdd (USBD_DRV_LIB_SETUP_PKT_Q  *p_setup_pkt_q,
                                USBD_DRV                  *p_drv,
                                CPU_INT32U                *p_setup_pkt_buf)
{
    CPU_INT32U  *p_dest_buf;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (p_setup_pkt_q->Nbr >= p_setup_pkt_q->TblLen) {
        CPU_CRITICAL_EXIT();
        return;
    }
                                                                /* Get next avail buf from circular buf.                */
    p_dest_buf = p_setup_pkt_q->SetupPktTblPtr[p_setup_pkt_q->IxIn].SetupPkt;

                                                                /* Store setup pkt in obtained buf.                     */
    p_dest_buf[0u] = p_setup_pkt_buf[0u];
    p_dest_buf[1u] = p_setup_pkt_buf[1u];

                                                                /* Adjust ix and internal data.                         */
    p_setup_pkt_q->IxIn++;
    if (p_setup_pkt_q->IxIn >= p_setup_pkt_q->TblLen) {
        p_setup_pkt_q->IxIn  = 0u;
    }
    p_setup_pkt_q->Nbr++;

    if (p_setup_pkt_q->Nbr == 1u) {
        USBD_DrvLib_SetupPktQSend(p_setup_pkt_q, p_drv);
    }
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                  USBD_DrvLib_SetupPktQSubmitNext()
*
* Description : Dequeues last setup packet buffer and submit a new one if any.
*
* Argument(s) : p_setup_pkt_q       Pointer to setup packet queue structure.
*
*               p_drv               Pointer to device driver structure.
*
* Return(s)   : None.
*
* Caller(s)   : Driver's endpoint stall function,
*               Driver's endpoint RxZLP and TxZLP functions.
*
* Note(s)     : (1) This function must be called each time the core has completed a setup packet
*                   processing. This will occur when it stalls the control endpoint or when it requests
*                   to send or receive the status phase (ZLP).
*********************************************************************************************************
*/

void  USBD_DrvLib_SetupPktQSubmitNext (USBD_DRV_LIB_SETUP_PKT_Q  *p_setup_pkt_q,
                                       USBD_DRV                  *p_drv)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();                                       /* Dequeue prev setup pkt.                              */
    p_setup_pkt_q->IxOut++;
    if (p_setup_pkt_q->IxOut >= p_setup_pkt_q->TblLen) {
        p_setup_pkt_q->IxOut  = 0u;
    }
    p_setup_pkt_q->Nbr--;

    USBD_DrvLib_SetupPktQSend(p_setup_pkt_q, p_drv);            /* Submit next setup pkt if any.                        */
    CPU_CRITICAL_EXIT();
}

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      USBD_DrvLib_SetupPktQSend()
*
* Description : Submits a setup packet to the core if any available in the queue.
*
* Argument(s) : p_setup_pkt_q       Pointer to setup packet queue structure.
*
*               p_drv               Pointer to device driver structure.
*
* Return(s)   : None.
*
* Caller(s)   : USBD_DrvLib_SetupPktQAdd(),
*               USBD_DrvLib_SetupPktQSubmitNext().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBD_DrvLib_SetupPktQSend (USBD_DRV_LIB_SETUP_PKT_Q  *p_setup_pkt_q,
                                         USBD_DRV                  *p_drv)
{
    USBD_DRV_LIB_SETUP_PKT  *p_setup_pkt;


    if (p_setup_pkt_q->Nbr == 0u) {
        return;
    }

    p_setup_pkt = &p_setup_pkt_q->SetupPktTblPtr[p_setup_pkt_q->IxOut];

    USBD_EventSetup(         p_drv,
                    (void *)&p_setup_pkt->SetupPkt[0u]);
}
