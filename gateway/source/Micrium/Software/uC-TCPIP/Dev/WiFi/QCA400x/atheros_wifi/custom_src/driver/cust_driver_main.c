/*
*********************************************************************************************************
*                                              uC/TCP-IP
*                                      The Embedded TCP/IP Suite
*
*                          (c) Copyright 2003-2014; Micrium, Inc.; Weston, FL
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
*                                        NETWORK DEVICE DRIVER
*
*                                               QCA400X
*
* Filename      :
* Version       : V1.00.00
* Programmer(s) : AL
*********************************************************************************************************
* Note(s)       : (1) The net_dev_ar4100 module is essentially a port of the Qualcomm Atheros driver.
*                     Please see the Qualcomm Atheros copyright below and on every other document
*                     provided with this WiFi driver.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      QUALCOMM/ATHEROS COPYRIGHT
*********************************************************************************************************
*/
//------------------------------------------------------------------------------
// Copyright (c) 2011 Qualcomm Atheros, Inc.
// All Rights Reserved.
// Qualcomm Atheros Confidential and Proprietary.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is
// hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
// USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//------------------------------------------------------------------------------

//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <custom_api.h>
#include <common_api.h>
#include <netbuf.h>
#include "atheros_wifi_api.h"
#include "atheros_wifi_internal.h"


extern A_VOID *pGlobalCtx;
extern A_VOID Custom_Api_RxComplete (A_VOID* p_cxt,
                                     A_VOID* p_req);

#define ATHEROS_TASK_NAME       "Atheros Wifi Driver"
#define DRIVER_AR4100_LOCKNAME  "Driver AR4100 Atheros"


ATH_CUSTOM_INIT_T ath_custom_init =
{
	NULL,
	NULL,
	NULL,
	NULL,
	Custom_Api_RxComplete,
	NULL,
	NULL,
	A_FALSE,
	A_FALSE,
};

ATH_CUSTOM_MEDIACTL_T ath_custom_mediactl =
{
	NULL
};

ATH_CUSTOM_HTC_T ath_custom_htc =
{
	NULL,
	NULL
};

/*
*********************************************************************************************************
*                                   Custom_Driver_WaitForCondition()
*
* Description : Wait Wake user event Semaphore and verify that condition is met.
*
* Argument(s) : p_cxt       Pointer to a network interface. (equivalent to NET_IF *p_if)
*
*               p_cond      Pointer on the boolean condition to met.
*
*               value       Condition value to met.
*
*               msec        Time in millisecond that the condition must be met.
*
* Return(s)   : A_OK,       If the condition is met in time.
*
*               A_ERROR,    If the condition is not met in time or it has failed to pend.
*
* Caller(s)   : Api_DataTxStart(),
*               Api_DeInitFinish(),
*               Api_DriverAccessCheck(),
*               Api_InitFinish(),
*               Api_ProgramMacAddress(),
*               Api_TxGetStatus(),
*               Api_WMIInitFinish(),
*               NetDev_WMICmdSend(),
*               wmi_cmd_process().
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_STATUS  Custom_Driver_WaitForCondition (         A_VOID   *p_cxt,
                                          volatile A_BOOL   *p_cond,
                                                   A_BOOL    value,
                                                   A_UINT32  msec)
{
	A_STATUS status;
	KAL_ERR  kal_err;


    status = A_OK;

	while (*p_cond != value) {

        KAL_SemPend( GET_DRIVER_CXT(p_cxt)->UserWakeSemHandle,
                     KAL_OPT_PEND_NONE,
                     msec,
                    &kal_err);

        switch (kal_err) {
            case KAL_ERR_NONE:
                 break;

            case KAL_ERR_TIMEOUT:
                 status = A_ERROR;
                 return status;


            case KAL_ERR_ABORT:
            case KAL_ERR_ISR:
            case KAL_ERR_WOULD_BLOCK:
            case KAL_ERR_OS:
            default:
                 status = A_ERROR;
                 return status;
        }
    }
	return status;
}

/*
*********************************************************************************************************
*                                         Atheros_Driver_Task()
*
* Description : Handle the driver task.
*
* Argument(s) : p_context    Pointer to a network interface. (equivalent to NET_IF *p_if)
*
* Return(s)   : None.
*
* Caller(s)   : Custom_Driver_CreateThread().
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

static A_VOID Atheros_Driver_Task (void *p_context)
{
	A_VOID   *p_cxt;
	A_BOOL    can_block;
	A_UINT16  block_msec;
    KAL_ERR   kal_err;


	can_block    = A_FALSE;
    p_cxt        = (A_VOID *)p_context;
    pGlobalCtx   = p_cxt;
    do
	{
                                                                /* ----------- WAIT FOR DRIVER START SIGNAL ----------- */
        KAL_SemPend( GET_DRIVER_CXT(p_cxt)->DriverStartSemHandle,
                     KAL_OPT_PEND_NONE,
                     KAL_TIMEOUT_INFINITE,
                     &kal_err);
        if (kal_err != KAL_ERR_NONE){
            A_ASSERT(0);
        }
                                                                /* --------------- DRIVER TASK HANDLER ---------------- */
		while (DEF_TRUE) {
                                                                /* Check if the Driver task can sleep.                  */
			if (can_block) {

			    GET_DRIVER_COMMON(p_cxt)->driverSleep = A_TRUE;

                KAL_SemPend( GET_DRIVER_CXT(p_cxt)->DriverWakeSemHandle,
                             KAL_OPT_PEND_NONE,
                             block_msec,
                            &kal_err);
                if ((kal_err != KAL_ERR_NONE   ) &&
                    (kal_err != KAL_ERR_TIMEOUT)) {
                    break;
                }
            }

		    GET_DRIVER_COMMON(p_cxt)->driverSleep = A_FALSE;

			if (GET_DRIVER_CXT(p_cxt)->DriverShutdown == A_TRUE){
				break;
			}
			                                                    /* Check if the driver should shutdown.                        */
			Driver_Main( p_cxt,
			             DRIVER_SCOPE_RX|DRIVER_SCOPE_TX,
			            &can_block,
			            &block_msec);
		}
                                                                /* Signal that the driver is stopped.                          */
        CUSTOM_DRIVER_WAKE_USER(p_cxt);

	} while(DEF_TRUE);
}


/*
*********************************************************************************************************
*                                     Custom_Driver_CreateThread()
*
* Description : Create the Driver task.
*
* Argument(s) : p_cxt       Pointer to a network interface. (equivalent to NET_IF *p_if)
*
* Return(s)   : A_OK,       if the driver task is successfully created.
*
*               A_ERROR,    if the driver task creation failed.
*
* Caller(s)   :
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_STATUS Custom_Driver_CreateThread (A_VOID *p_cxt)
{
    KAL_ERR              kal_err;
    KAL_TASK_HANDLE *    p_task_handle;
    NET_IF               *p_if;
    NET_DEV_CFG_WIFI_QCA *p_dev_cfg;


                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if          = (NET_IF               *)p_cxt;              /* Obtain ptr to the interface area.                    */
    p_dev_cfg     = (NET_DEV_CFG_WIFI_QCA *)p_if->Dev_Cfg;
    p_task_handle = &GET_DRIVER_CXT(p_cxt)->DriverTaskHandle;
                                                                /* Get the driver task handle.                          */
   *p_task_handle = KAL_TaskAlloc((const  CPU_CHAR *) ATHEROS_TASK_NAME,
                                                      DEF_NULL,
                                                      p_dev_cfg->DriverTask_StkSize,
                                                      DEF_NULL,
                                                     &kal_err);
    if (kal_err != KAL_ERR_NONE){
        return A_ERROR;
    }
                                                                /* Create the driver task.                              */
    KAL_TaskCreate(*p_task_handle,
                    Atheros_Driver_Task,
                    p_cxt,
                    p_dev_cfg->DriverTask_Prio,
                    DEF_NULL,
                   &kal_err);
    if (kal_err != KAL_ERR_NONE){
        return A_ERROR;
    }

	return A_OK;
}


/*
*********************************************************************************************************
*                                     Custom_Driver_DestroyThread()
*
* Description : Shutdown the driver task and wait the for the completion. The driver task will wait for
*               the driver start signal.
*
* Argument(s) : p_cxt       Pointer to a network interface. (equivalent to NET_IF *p_if)
*
* Return(s)   : A_OK.
*
* Caller(s)   : none.
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_STATUS Custom_Driver_DestroyThread (A_VOID *p_cxt)
{

    GET_DRIVER_CXT(p_cxt)->DriverShutdown = A_TRUE;

	CUSTOM_DRIVER_WAKE_DRIVER(p_cxt);

	return A_OK;
}


/*
*********************************************************************************************************
*                                         Custom_GetRxRequest()
*
* Description : Get a Rx Request buffer :
*
*                   (1) Get a buffer from Rx Net pool.
*                   (2) Get a buffer from A_NETBUF pool.
*                   (3) Initialize the A_NETBUF buffer with the Rx Net buffer.
*
* Argument(s) : p_cxt       Pointer to a network interface. (equivalent to NET_IF *p_if)
*
*               length      Requested buffer length.
*
* Return(s)   : None.
*
* Caller(s)   : Driver_RxReady().
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_VOID *Custom_GetRxRequest (A_VOID *p_cxt,
                             A_UINT16 length)
{
    A_VOID      *p_req;
    CPU_INT08U  *p_buf_new;
    LIB_ERR      err_lib;
    NET_ERR      err_net;
    A_NETBUF    *p_a_netbuf;
    NET_IF      *p_if;
    MEM_POOL    *p_a_netbuf_pool;

                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if            = (NET_IF               *)p_cxt;            /* Obtain ptr to the interface area.                    */
    p_req           = (A_VOID*) DEF_NULL;
    p_a_netbuf_pool = &(GET_DRIVER_CXT(p_cxt)->A_NetBufPool);
	RXBUFFER_ACCESS_ACQUIRE(p_cxt);
    {
                                                                /* ----------- OBTAIN PTR TO NEW DATA AREA ------------ */
                                                                /* Request an empty buffer.                             */
        p_buf_new = NetBuf_GetDataPtr((NET_IF        *) p_if,
                                      (NET_TRANSACTION) NET_TRANSACTION_RX,
                                      (NET_BUF_SIZE   ) NET_IF_ETHER_FRAME_MAX_SIZE,
                                      (NET_BUF_SIZE   ) NET_IF_IX_RX,
                                      (NET_BUF_SIZE  *) 0,
                                      (NET_BUF_SIZE  *) 0,
                                      (NET_BUF_TYPE  *) 0,
                                      (NET_ERR       *)&err_net);

                                                                /* ------------ OBTAIN PTR TO NEW A_NETBUF ------------ */
        if (err_net == NET_BUF_ERR_NONE) {
            p_req = Mem_PoolBlkGet((MEM_POOL *) p_a_netbuf_pool,
                                   (CPU_SIZE_T) sizeof(A_NETBUF),
                                   (LIB_ERR  *)&err_lib);
        }


    }
    RXBUFFER_ACCESS_RELEASE(p_cxt);
    if (err_net != NET_BUF_ERR_NONE) {                          /* If unable to get Rx Net buffer.                      */
        Driver_ReportRxBuffStatus(p_cxt, A_FALSE);
        return p_req;
    }
                                                                /* If unable to get the A_NETBUF buf.                   */
    if (err_lib != LIB_MEM_ERR_NONE) {
        Driver_ReportRxBuffStatus(p_cxt, A_FALSE);
        NetBuf_FreeBufDataAreaRx(p_if->Nbr, p_buf_new);
        return ((A_VOID *)0);
    }
                                                                /* --------------- A_NETBUF STRUCT INIT --------------- */
	if (p_req != NULL) {

        A_NETBUF_INIT(p_cxt,
                      p_req,
                      p_buf_new);
        p_a_netbuf                  = (A_NETBUF *)p_req;
        p_a_netbuf->pool_id         = A_RX_NET_POOL;
        p_a_netbuf->RxBufDelivered  = DEF_NO;

       if (length > A_NETBUF_TAILROOM(p_req)) {
         A_ASSERT(length <= A_NETBUF_TAILROOM(p_req));
       }

    }else{
        A_ASSERT(0);                                            /* Should never happen thanks to HW_ReportRxBuffStatus. */
    }

    return p_req;
}

/*
*********************************************************************************************************
*                                      Custom_Driver_WakeDriver()
*
* Description : Signal the driver to wake.
*
* Argument(s) : p_cxt       Pointer to a network interface. (equivalent to NET_IF *p_if)
*
* Return(s)   : none.
*
* Caller(s)   : Api_ProgramMacAddress(),
*               Api_TxGetStatus(),
*               Bus_TransferComplete(),
*               Custom_Driver_DestroyThread(),
*               Driver_ReportRxBuffStatus(),
*               Driver_SubmitTxRequest(),
*               HW_InterruptHandler().
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_VOID Custom_Driver_WakeDriver (A_VOID *p_cxt)
{
    KAL_ERR err_kal;


    KAL_SemPost( GET_DRIVER_CXT(p_cxt)->DriverWakeSemHandle,
                 KAL_OPT_PEND_NONE,
                &err_kal);

    (void)&err_kal;
}

/*
*********************************************************************************************************
*                                       Custom_Driver_WakeUser()
*
* Description : Signal from the driver for updated value in the driver context.
*
* Argument(s) : p_cxt       Pointer to a network interface. (equivalent to NET_IF *p_if)
*
* Return(s)   : none.
*
* Caller(s)   : Api_BitrateEvent(),
*               Api_ChannelListEvent(),
*               Api_GetPmkEvent(),
*               Api_RawTxComplete(),
*               Api_ScanCompleteEvent(),
*               Api_SetChannelCompleteEvent(),
*               Api_StoreRecallEvent(),
*               Api_TargetStatsEvent(),
*               Api_TestCmdEventRx(),
*               Api_TxComplete(),
*               Api_WpsProfileEvent(),
*               Atheros_Driver_Task(),
*               program_mac_addr(),
*               query_credit_deficit().
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_VOID Custom_Driver_WakeUser (A_VOID *pCxt)
{
    KAL_ERR err_kal;


    KAL_SemPost( GET_DRIVER_CXT(pCxt)->UserWakeSemHandle,
                 KAL_OPT_PEND_NONE,
                &err_kal);

    (void)&err_kal;
}

/*
*********************************************************************************************************
*                                      Custom_Driver_ContextInit()
*
* Description : Initialize the driver context elements.
* Argument(s) : p_cxt       Pointer to a network interface. (equivalent to NET_IF *p_if)
*
* Return(s)   : A_OK        In any case.
*
* Caller(s)   : Driver_ContextInit().
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_STATUS Custom_Driver_ContextInit (A_VOID *p_cxt)
{


    A_DRIVER_CONTEXT     *p_dcxt;
    NET_IF               *p_if;
    NET_DEV_CFG_WIFI_QCA *p_dev_cfg;
    NET_DEV_DATA         *p_dev_data;

                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)p_cxt;                 /* Obtain ptr to the interface area.                    */
    p_dev_cfg  = (NET_DEV_CFG_WIFI_QCA *)p_if->Dev_Cfg;         /* Obtain ptr to dev cfg area.                          */
    p_dev_data = (NET_DEV_DATA         *)p_if->Dev_Data;        /* Obtain ptr to dev data area.                         */
    p_dcxt      = GET_DRIVER_COMMON(p_cxt);


    p_dcxt->tempStorage                    = p_dev_data->GlobalBufPtr;
    p_dcxt->tempStorageLength              = p_dev_cfg->RxBufLargeSize;

    p_dcxt->rxBufferStatus                 = A_TRUE;
    GET_DRIVER_CXT(p_cxt)->DriverShutdown  = A_FALSE;

    return A_OK;
}


/*
*********************************************************************************************************
*                                     Custom_Driver_ContextDeInit()
*
* Description : Deinitialization of the driver context elements.
*
* Argument(s) : pCxt    $$$$ Add description for 'pCxt'
*
* Return(s)   : none.
*
* Caller(s)   : Driver_ContextDeInit().
*
* Note(s)     : (1) This function use QualComm/Atheros variable type in order to match with the supplied
*                   driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
*                   equivalent.
*
*********************************************************************************************************
*/

A_VOID Custom_Driver_ContextDeInit(A_VOID *p_cxt)
{
	(void)p_cxt;
}

/*
*********************************************************************************************************
*                                             assert_func()
*
* Description : Block the Driver task in case of fatal error.
*
* Argument(s) : line    Line number for debugger purpose.
*
* Return(s)   : None.
*
* Caller(s)   : Api_RxComplete(),
*               Api_WMIInitFinish(),
*               BMIExecute(),
*               BMIGetTargetInfo(),
*               BMIInit(),
*               BMIReadMemory(),
*               BMIReadSOCRegister(),
*               BMIWriteMemory(),
*               BMIWriteSOCRegister(),
*               Bus_InOutDescriptorSet(),
*               ConfigureByteSwap(),
*               Custom_Api_RxComplete(),
*               Custom_GetRxRequest(),
*               Driver_BootComm(),
*               Driver_CompleteRequest(),
*               Driver_ContextInit(),
*               Driver_DumpAssertInfo(),
*               Driver_Main(),
*               Driver_RxComplete(),
*               Driver_RxReady(),
*               Driver_TxComplete(),
*               Driver_TxReady(),
*               EnableDisableSPIIRQHwDetect(),
*               HTC_ConnectService(),
*               HTC_ProcessCpuInterrupt(),
*               HandleExternalReadDone(),
*               Hcd_DoPioExternalAccess(),
*               Hcd_GetLookAhead(),
*               Hcd_GetMboxAddress(),
*               Hcd_Init(),
*               Hcd_ProgramWriteBufferWaterMark(),
*               Hcd_SpiInterrupt(),
*               Htc_ProcessCreditRpt(),
*               Htc_ProcessRecvHeader(),
*               Htc_ProcessTrailer(),
*               Htc_ReadCreditCounter(),
*               Htc_RxComplete(),
*               Htc_SendPacket(),
*               NetDev_WMICmdSend(),
*               SetupServices(),
*               a_netbuf_pull(),
*               a_netbuf_push(),
*               a_netbuf_put(),
*               a_netbuf_trim(),
*               aggr_process_recv_frm(),
*               aggr_recv_addba_req_evt(),
*               aggr_recv_delba_req_evt(),
*               aggr_reset_state(),
*               program_mac_addr(),
*               wmi_cmd_process(),
*               wmi_cmd_send(),
*               wmi_control_rx(),
*               wmi_data_hdr_add(),
*               wmi_data_hdr_remove(),
*               wmi_dix_2_dot3(),
*               wmi_dot3_2_dix(),
*               wmi_errorEvent_rx(),
*               wmi_implicit_create_pstream(),
*               wmi_meta_add(),
*               wmi_set_control_ep().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void assert_func(A_UINT32 line)
{
    CPU_INT32U silly_loop;


    while(1){
        silly_loop++;
    }
}
/*
*********************************************************************************************************
*                               ATHEROS MUTEX TO MICRIUM KAL FUNCTION
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            a_mutex_init()
*
* Description : Used by the Qualcomm/Atheros driver to init a lock.
*
* Argument(s) : mutex       Pointer on the lock to init.
*
* Return(s)   : A_STATUS,
*                           A_OK        If successful.
*                           A_ERROR     If not successful.
*
*
* Caller(s)   : Driver_Init(),
*               wmi_init().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

A_STATUS a_mutex_init(KAL_LOCK_HANDLE *mutex){

    KAL_ERR err_kal;


   *mutex = KAL_LockCreate( DRIVER_AR4100_LOCKNAME,
                            DEF_NULL,
                           &err_kal);
    switch (err_kal) {
        case KAL_ERR_NONE:
             return A_OK;

        case KAL_ERR_MEM_ALLOC:
        case KAL_ERR_CREATE:
        default:
             return A_ERROR;
    }
}

/*
*********************************************************************************************************
*                                            a_mutex_lock()
*
* Description : Used by the Qualcomm/Atheros driver to acquire a lock.
*
* Argument(s) : mutex       Pointer on the lock to acquire.
*
* Return(s)   : A_STATUS,
*                           A_OK        If successful.
*                           A_ERROR     If not successful.
*
* Caller(s)   : Api_ConnectEvent(),
*               Api_DisconnectEvent(),
*               Api_WpsProfileEvent(),
*               Custom_GetRxRequest(),
*               Driver_DropTxDataPackets(),
*               Driver_Main(),
*               Driver_SubmitTxRequest(),
*               HW_EnableDisableSPIIRQ(),
*               wmi_qos_state_init().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

A_STATUS a_mutex_lock(KAL_LOCK_HANDLE *mutex){

    KAL_ERR err_kal;


    KAL_LockAcquire(*mutex,
                     KAL_OPT_PEND_NONE,
                     KAL_TIMEOUT_INFINITE,
                     &err_kal);
    switch (err_kal) {
        case KAL_ERR_NONE:
             return A_OK;

        case KAL_ERR_LOCK_OWNER:
        case KAL_ERR_NULL_PTR:
        case KAL_ERR_INVALID_ARG:
        case KAL_ERR_ABORT:
        case KAL_ERR_TIMEOUT:
        case KAL_ERR_WOULD_BLOCK:
        case KAL_ERR_OS:
        default:
             return A_ERROR;
    }
}

/*
*********************************************************************************************************
*                                           a_mutex_unlock()
*
* Description : Used by the Qualcomm/Atheros driver to release a lock.
*
* Argument(s) : mutex       Pointer on the lock to release.
*
* Return(s)   : A_STATUS,
*                           A_OK        If successful.
*                           A_ERROR     If not successful.
*
*
* Caller(s)   : Api_ConnectEvent(),
*               Api_DisconnectEvent(),
*               Api_WpsProfileEvent(),
*               Custom_GetRxRequest(),
*               Driver_DropTxDataPackets(),
*               Driver_Main(),
*               Driver_SubmitTxRequest(),
*               HW_EnableDisableSPIIRQ(),
*               wmi_qos_state_init().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

A_STATUS a_mutex_unlock(KAL_LOCK_HANDLE *mutex){

    KAL_ERR err_kal;


    KAL_LockRelease(*mutex, &err_kal);

    (void)&err_kal;

    return A_OK;
}

/*
*********************************************************************************************************
*                                           a_mutex_destroy()
*
* Description : Used by the Qualcomm/Atheros driver to delete a lock.
*
* Argument(s) : mutex       Pointer on the lock to delete.
*
* Return(s)   : A_STATUS,
*                           A_OK        If successful.
*                           A_ERROR     If not successful.
*
*
* Caller(s)   : Driver_DeInit(),
*               wmi_shutdown().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

A_STATUS a_mutex_destroy(KAL_LOCK_HANDLE *mutex){

    KAL_ERR err_kal;


    KAL_LockDel(*mutex, &err_kal);
    switch (err_kal) {
        case KAL_ERR_NONE:
             return A_OK;

        case KAL_ERR_MEM_ALLOC:
        case KAL_ERR_CREATE:
        default:
             return A_ERROR;
    }
}
