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
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <driver_cxt.h>
#include <common_api.h>
#include <custom_api.h>
#include <wmi_api.h>
#include <spi_hcd_if.h>
#include <atheros_wifi_api.h>
#include <atheros_wifi_internal.h>
#include <common_stack_offload.h>

A_STATUS setup_host_dset(A_VOID* handle);

#if DRIVER_CONFIG_ENABLE_STORE_RECALL
extern A_CONST A_UINT8 max_performance_power_param;

#if !ENABLE_STACK_OFFLOAD  
extern A_INT32 power_state_for_module;
/*****************************************************************************/
/*  restore_power_state - Restores the original power state, which is changed by
 *                        "move_power_state_to_maxperf"
 *  A_INT32 - module which request the change
 * RETURNS: A_OK on success A_ERROR otherwise
 *****************************************************************************/
A_STATUS restore_power_state(void *pCxt, A_INT32 module)
{
    A_DRIVER_CONTEXT *pDCxt = (A_DRIVER_CONTEXT *)pCxt;
    
    /* Already some HIGH priority module changes state,
     * so don't change the state now */
    if (!power_state_for_module || (power_state_for_module > module))
    {
        return A_OK;
    }
    if(pDCxt->userPwrMode!=max_performance_power_param)
    {
        if(A_OK!=wmi_cmd_start(pDCxt->pWmiCxt, &pDCxt->userPwrMode, WMI_SET_POWER_MODE_CMDID, sizeof(WMI_POWER_MODE_CMD)))
        {
            return A_ERROR;
        }
    }
    
    power_state_for_module = 0;
    return A_OK;
}
#endif
/*****************************************************************************/
/*  Strrcl_ChipUpFinish - Utility function to complete the chip_up process.
 *	 when WMI_READY is received from chip this function clears the chipDown
 * 	 boolean and sends the wmi store recall command to the chip so that its
 *	 associated State can be refreshed.
 *      A_VOID *pCxt - the driver context.      
 *****************************************************************************/
static A_STATUS 
Strrcl_ChipUpFinish(A_VOID* pCxt)
{
	A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
	WMI_STORERECALL_STORE_EVENT *pEv;
    A_UINT8  *pDsetData;
    WMI_STORERECALL_RECALL_CMD   *pDsetInfo;
	A_UINT32   dset_info_len;

	if(pDCxt->wmiReady == A_TRUE){
        switch(pDCxt->strrclState){
        case STRRCL_ST_ACTIVE:
            /* clear the chipDown boolean to allow outside activity to resume */
            pDCxt->chipDown = A_FALSE;
                        
            if(A_OK != STACK_INIT(pCxt)){
                break;
            }

            pDCxt->strrclState = STRRCL_ST_ACTIVE_B;
            //INTENTIONAL FALL_THRU
        case STRRCL_ST_ACTIVE_B:
            /* send recall */
#if 0
		    pEv = (WMI_STORERECALL_STORE_EVENT*)pDCxt->strrclCxt;

            if(A_OK != wmi_storerecall_recall_cmd(pDCxt->pWmiCxt, A_LE2CPU32(pEv->length), &pEv->data[0])){
                break; //try again later this is likely because a previous wmi cmd has not finished.
            }
#elifndef   ENABLE_LARGE_DSET

		    setup_host_dset(pCxt);

		    pDsetData = pDCxt->tempStorage;

            if(A_OK != wmi_storerecall_recall_cmd(pDCxt->pWmiCxt, A_LE2CPU32(pDCxt->strrclCxtLen), pDsetData)){
                break; //try again later this is likely because a previous wmi cmd has not finished.
            }
#else
		    setup_host_dset(pCxt);

		    pDsetInfo = (WMI_STORERECALL_RECALL_CMD *)pDCxt->tempStorage;
            if (pDsetInfo->length == 0)
            {
				dset_info_len = sizeof(WMI_STORERECALL_RECALL_CMD);
            }
            else 
			{
				dset_info_len = sizeof(WMI_STORERECALL_RECALL_CMD) - sizeof(A_UINT8);
                dset_info_len += sizeof(WMI_STORERECALL_RECALL_DSET) * pDsetInfo->length;
			}

            if(A_OK != wmi_storerecall_recall_cmd(pDCxt->pWmiCxt, dset_info_len, pDsetInfo)){
                break; //try again later this is likely because a previous wmi cmd has not finished.
            }
#endif
            
            pDCxt->strrclCxt = NULL;
            pDCxt->strrclCxtLen = 0;

//            pDCxt->strrclState = STRRCL_ST_AWAIT_FW;

        	pDCxt->strrclState = STRRCL_ST_INIT;  
        	pDCxt->strrclBlock = A_FALSE;

        /* Indicate completion to the custom layer */
        	CUSTOM_API_STORE_RECALL_EVENT(pCxt);     
            
            /* clear this function from the driver's temporary calls */
            pDCxt->asynchRequest = NULL;      
            /* Since we are setting MAX_PERF before going to store-recall we need to set the state 
               after we wakes up */
            restore_power_state(pDCxt, 1); /* POWER_STATE_MOVED_FOR_STRRCL */
            
            break;//done!!
        default:
            A_ASSERT(0); //should never happen
#if DRIVER_CONFIG_DISABLE_ASSERT            
            break;      
#endif   
        }										           
	}
	
	return A_OK;
}

/*****************************************************************************/
/*  Strrcl_ChipUpStart - Utility function to bring bring the chip back up. Also
 * 	 sets up the driver tempFunc so that when the WMI_READY is received the 
 *	 store recall process can be completed via Strrcl_ChipUpFinish.
 *      A_VOID *pCxt - the driver context.      
 *****************************************************************************/
static A_STATUS 
Strrcl_ChipUpStart(A_VOID *pCxt)
{
	A_STATUS status = A_ERROR;
	A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
        A_MDELAY(2);
		/* assert pwd line */
	HW_PowerUpDown(pCxt, A_TRUE);
	
        A_MDELAY(2);
	do{
              /* reset htc_creditInit to re-process the HTC interrupt */
              pDCxt->htc_creditInit = 0;
              
              if((status = Hcd_ReinitTarget(pCxt)) != A_OK){
                break;
              }
              
        A_MDELAY(1);

              if(A_OK != (status = Driver_BootComm(pCxt))){
                break;
              }

#if (DRIVER_CONFIG_ENABLE_HOST_FW_DOWNLOAD)              
              if(A_OK != (status = Driver_StoreRecallFirmwareDownload(pCxt))) {
                break;
              }
#endif /* DRIVER_CONFIG_ENABLE_HOST_FW_DOWNLOAD */
              
              /* unmask the packet interrupt on-chip */
              Hcd_UnmaskSPIInterrupts(pCxt, ATH_SPI_INTR_PKT_AVAIL);		
              /* when the driver receives the WMI_READY event chip_up_finish
               * will complete the re-boot process and open the way for 
               * outside requests to resume. */
              pDCxt->asynchRequest = Strrcl_ChipUpFinish;              
        
	}while(0);
	
	return status;
}

/*****************************************************************************/
/*  Strrcl_ChipDown - Utility function to bring the chip down. Also sets 
 *	 driver booleans to prevent any unwanted activity until the chip is 
 *	 brought back up. 
 *      A_VOID *pCxt - the driver context.      
 *****************************************************************************/
static A_VOID 
Strrcl_ChipDown(A_VOID *pCxt)
{
	/* disable interrupt line - prevent un-expected INTs when chip powers down */
//	EnableDisableSPIIRQHwDetect(pDevice,FALSE);
	GET_DRIVER_COMMON(pCxt)->chipDown = A_TRUE;
	GET_DRIVER_COMMON(pCxt)->wmiReady = A_FALSE;
	/* assert pwd line */
	HW_PowerUpDown(pCxt, A_FALSE);
	
}

/*****************************************************************************/
/*  Strrcl_Recall - Implements the store-recall procedure.  powers down the chip
 *	 waits for x msec powers up the chip, reboots the chip. Called when the 
 * 	 store-recall event is received from the chip indicating that it can be 
 *	 powered down for the specified timeout.
 *      A_VOID *pCxt - the driver context.      
 *		A_UINT32 msec_sleep - number of milliseconds to wait 
 *****************************************************************************/
A_VOID 
Strrcl_Recall(A_VOID *pCxt, A_UINT32 msec_sleep)
{	
	/* bring down the chip */
	Strrcl_ChipDown(pCxt);
	/* this will block the driver thread */
	/* FIXME: should instead implement a way to block the driver at driver_main thus
	 * allowing the thread to continue running in single-threaded systems.*/
        if(ath_custom_init.Custom_Delay != NULL)
        {
          /*It is possible that some hosts support low power modes, this would be a
            good place to invoke it. Call app defined function if available*/
            ath_custom_init.Custom_Delay(msec_sleep);
        }
        else
      	    A_MDELAY(msec_sleep);
        
	/* wake up chip */
	Strrcl_ChipUpStart(pCxt);				
}

#endif //DRIVER_CONFIG_ENABLE_STORE_RECALL
