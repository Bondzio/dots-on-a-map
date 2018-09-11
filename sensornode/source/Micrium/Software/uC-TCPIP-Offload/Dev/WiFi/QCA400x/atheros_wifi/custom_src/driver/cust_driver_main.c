//------------------------------------------------------------------------------
// Copyright (c) Qualcomm Atheros, Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
// the limitations in the disclaimer below) provided that the following conditions are met:
//
// · Redistributions of source code must retain the above copyright notice, this list of conditions and the
//   following disclaimer.
// · Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//   following disclaimer in the documentation and/or other materials provided with the distribution.
// · Neither the name of nor the names of its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <custom_wlan_api.h>
#include <common_api.h>
#include <netbuf.h>
#include "atheros_wifi_api.h"
#include "atheros_wifi_internal.h"



ATH_CUSTOM_INIT_T ath_custom_init =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	QOSAL_FALSE,
	QOSAL_FALSE,
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

extern A_STATUS wmi_dset_host_cfg_cmd (void *handle);

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

QOSAL_VOID *Custom_GetRxRequest (QOSAL_VOID *p_cxt,
                             QOSAL_UINT16 length)
{
    QOSAL_VOID        *p_req;
    CPU_INT08U    *p_buf_new;
    A_NETBUF      *p_a_netbuf;
    QCA_BUF_POOL  *p_pool;
    QCA_CTX       *p_qca;
                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_req  = (QOSAL_VOID  *) DEF_NULL;
    p_qca  = (QCA_CTX *) p_cxt;
                                                                /* ----------- OBTAIN PTR TO NEW DATA AREA ------------ */
                                                                /* Request an empty buffer.                             */
    p_pool = QCA_BufPoolGet(p_qca->BufPools,
                            p_qca->Dev_Cfg->NbBufPool, 
                            length);
    
    A_ASSERT(p_pool != DEF_NULL);
    
    p_buf_new = (QOSAL_UINT8 *)QCA_BufBlkGet(p_pool, 
                                         length,
                                         DEF_NO);  
    
    if (p_buf_new == DEF_NULL) {                                /* If unable to get Rx Net buffer.                      */
        Driver_ReportRxBuffStatus(p_cxt, QOSAL_FALSE);
        return p_req;
    } 
                                                                /* --------------- A_NETBUF STRUCT INIT --------------- */
    p_req      = p_buf_new;
    p_buf_new += sizeof(A_NETBUF);
    
    A_NETBUF_CONFIGURE(p_req, p_buf_new,  AR6000_DATA_OFFSET, 0u, p_pool->Pool.BlkSize);
   
    p_a_netbuf                  = (A_NETBUF *)p_req;
    p_a_netbuf->pool_id         =  A_RX_NET_POOL;
    p_a_netbuf->buf_pool        =  p_pool;
    
    A_ASSERT(length <= A_NETBUF_TAILROOM(p_req));
    
    return p_req;
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

A_STATUS Custom_Driver_ContextInit (QOSAL_VOID *p_cxt)
{
    A_DRIVER_CONTEXT     *p_dcxt;                                                       
     
    
    p_dcxt                                 = GET_DRIVER_COMMON(p_cxt);
    p_dcxt->rxBufferStatus                 = QOSAL_TRUE;
    GET_DRIVER_CXT(p_cxt)->DriverShutdown  = QOSAL_FALSE;
    p_dcxt->tempStorage                    = QCA_CtxPtr->pScanOut;

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

QOSAL_VOID Custom_Driver_ContextDeInit(QOSAL_VOID *p_cxt)
{
	(void)p_cxt;
}

A_STATUS setup_host_dset(QOSAL_VOID* handle)
{	
  return wmi_dset_host_cfg_cmd (handle);
}

