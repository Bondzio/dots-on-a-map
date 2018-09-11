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
#ifndef _A_DRV_API_H_
#define _A_DRV_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/** WMI related hooks                                                      **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/
A_VOID 
Api_ChannelListEvent(A_VOID *pCxt, A_UINT8 devId, A_INT8 numChan, A_UINT16 *chanList);
A_VOID 
Api_TargetStatsEvent(A_VOID *pCxt,  A_UINT8 devId, A_UINT8 *ptr, A_UINT32 len);
A_VOID 
Api_ScanCompleteEvent(A_VOID *pCxt, A_UINT8 devId, A_STATUS status);
A_VOID 
Api_RegDomainEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT32 regCode);
A_VOID
Api_DisconnectEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 reason, A_UINT8 *bssid,
                        A_UINT8 assocRespLen, A_UINT8 *assocInfo, A_UINT16 protocolReasonStatus);
A_VOID
Api_ConnectEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT16 channel, A_UINT8 *bssid,
                     A_UINT16 listenInterval, A_UINT16 beaconInterval,
                     NETWORK_TYPE networkType, A_UINT8 beaconIeLen,
                     A_UINT8 assocReqLen, A_UINT8 assocRespLen,
                     A_UINT8 *assocInfo);
A_VOID 
Api_BitrateEvent(A_VOID *pCxt, A_UINT8 devId, A_INT32 rateKbps);
A_VOID
Api_ReadyEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT8 phyCap, A_UINT32 sw_ver, A_UINT32 abi_ver);                     
A_VOID 
Api_BssInfoEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len);
A_VOID 
Api_TkipMicErrorEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 keyid,
                              A_BOOL ismcast);                                             
A_VOID 
Api_GetPmkEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap);
A_VOID 
Api_RxDbglogEvent(A_VOID *wmip, A_UINT8 devId, A_UINT8 *datap, A_INT32 len);
A_VOID 
Api_RxGpioDataEvent(A_VOID *wmip, A_UINT8 devId, A_UINT8 *datap, A_INT32 len);
A_VOID
Api_RxPfmDataEvent(A_VOID *wmip, A_UINT8 devId, A_UINT8 *datap, A_INT32 len);
A_VOID 
Api_RxPfmDataDoneEvent(A_VOID *wmip, A_UINT8 devId, A_UINT8 *datap, A_INT32 len);

A_VOID
Api_StoreRecallEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);   
A_VOID
Api_StoreRecallStartEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);   
A_VOID
Api_HostDsetStoreEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);
A_VOID
Api_HostDsetReadEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);
A_VOID
Api_HostDsetCreateEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);
A_VOID
Api_HostDsetWriteEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);
A_VOID
Api_HostDsetReadbackEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);
A_VOID
Api_HostDsetSyncEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);
A_VOID
Api_DsetOPEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);

A_VOID
Api_WpsProfileEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_INT32 len, A_VOID *pReq);     
A_VOID
Api_AggrRecvAddbaReqEvent(A_VOID *pCxt, A_UINT8 devId, WMI_ADDBA_REQ_EVENT *evt);          
A_VOID
Api_AggrRecvDelbaReqEvent(A_VOID *pCxt, A_UINT8 devId, WMI_DELBA_EVENT *evt);              
A_STATUS
Api_WmiTxStart(A_VOID *pCxt, A_VOID *pReq, HTC_ENDPOINT_ID eid);
A_VOID
Api_RSNASuccessEvent(A_VOID *pCxt, A_UINT8 devId, A_UINT8 code);
A_VOID
Api_GetBitRateEvent(A_VOID *pCxt, A_UINT8 devId, A_VOID *datap);
A_STATUS
Api_SockResponseEventRx(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len, A_VOID *pReq);
#if MANUFACTURING_SUPPORT
A_VOID
Api_TestCmdEventRx(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
#endif
A_VOID
Api_GetTemperatureReply(A_VOID *pCxt,A_UINT8 *datap, A_UINT32 len);
#if ENABLE_P2P_MODE
A_VOID
p2p_go_neg_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len,WMI_P2P_PROV_INFO *wps_info);

A_VOID
p2p_invite_sent_result_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
p2p_node_list_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
p2p_list_persistent_network_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
p2p_req_auth_event(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
get_p2p_ctx(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
get_p2p_prov_disc_req(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
#if 1//KK
A_VOID
get_p2p_serv_disc_req(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);

A_VOID
p2p_invite_req_rx(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT8 len);

A_VOID
p2p_invite_rcvd_result_ev(A_VOID *pCxt, A_UINT8 devId, A_UINT8 *datap, A_UINT32 len);
#endif
#define A_WMI_P2P_GO_NEG_EVENT(devt, devId, res, len,wps_info) \
    p2p_go_neg_event((devt), (devId), (res), (len),(wps_info))

#define A_WMI_P2P_NODE_LIST_EVENT(devt, devId, res, len) \
    p2p_node_list_event((devt), (devId), (res), (len))

#define A_WMI_P2P_PERSISTENT_LIST_NETWORK_EVENT(devt, devId, res, len) \
      p2p_list_persistent_network_event((devt), (devId), (res), (len))  
      
#define A_WMI_P2P_REQ_AUTH_EVENT(devt, devId, res, len) \
    p2p_req_auth_event((devt), (devId), (res), (len))

#define A_WMI_GET_P2P_CTX(devt, devId, res, len) \
     get_p2p_ctx((devt), (devId), (res), (len))

#define A_WMI_P2P_PROV_DISC_REQ(devt, devId, res, len) \
     get_p2p_prov_disc_req((devt), (devId), (res), (len))
#if 1//KK
#define A_WMI_P2P_SERV_DISC_REQ(devt, devId, res, len)	\
	 get_p2p_serv_disc_req((devt), (devId), (res), (len))

#define A_WMI_P2P_INVITE_REQ_RX(devt, devId, datap, len) \
    p2p_invite_req_rx((devt), (devId), (datap), (len))

#define A_WMI_P2P_INVITE_RCVD_RESULT_EVENT(devt, devId, datap, len) \
    p2p_invite_rcvd_result_ev((devt), (devId), (datap), (len))

#define A_WMI_P2P_INVITE_SENT_RESULT_EVENT(devt, devId, res, len)\
    p2p_invite_sent_result_event((devt), (devId), (res), (len))
#endif
#endif

#define A_WMI_CHANNELLIST_RX(devt, devId, numChan, chanList)   \
    Api_ChannelListEvent((devt), (devId), (numChan), (chanList))

#define A_WMI_SET_NUMDATAENDPTS(devt, num)    

#define A_WMI_CONTROL_TX(devt, osbuf, streamID) \
    Api_WmiTxStart((devt), (osbuf), (streamID))

#define A_WMI_TARGETSTATS_EVENT(devt, devId, pStats, len)  \
    Api_TargetStatsEvent((devt), (devId), (pStats), (len))

#define A_WMI_SCANCOMPLETE_EVENT(devt, devId, status)  \
    Api_ScanCompleteEvent((devt), (devId), (status))

#define A_WMI_CONNECT_EVENT(devt, devId, channel, bssid, listenInterval, beaconInterval, networkType, beaconIeLen, assocReqLen, assocRespLen, assocInfo) \
    Api_ConnectEvent((devt), (devId), (channel), (bssid), (listenInterval), (beaconInterval), (networkType), (beaconIeLen), (assocReqLen), (assocRespLen), (assocInfo))

#define A_WMI_REGDOMAIN_EVENT(devt, devId, regCode)    \
    Api_RegDomainEvent((devt), (devId), (regCode))

#define A_WMI_DISCONNECT_EVENT(devt, devId, reason, bssid, assocRespLen, assocInfo, protocolReasonStatus)  \
    Api_DisconnectEvent((devt), (devId), (reason), (bssid), (assocRespLen), (assocInfo), (protocolReasonStatus))

#define A_WMI_RSNA_SUCCESS_EVENT(devt, devId, code)  \
    Api_RSNASuccessEvent((devt), (devId), (code))

#define A_WMI_BITRATE_RX(devt, devId, rateKbps)    \
    Api_BitrateEvent((devt), (devId), (rateKbps))

/* A_WMI_TXPWR_RX - not currently supported */
#define A_WMI_TXPWR_RX(devt, devId, txPwr)

#define A_WMI_READY_EVENT(devt, devId, datap, phyCap, sw_ver, abi_ver) \
    Api_ReadyEvent((devt), (devId), (datap), (phyCap), (sw_ver), (abi_ver))

#define A_WMI_GET_BITRATE_EVEVT(devt, devId, datap) \
    Api_GetBitRateEvent((devt), (devId), (datap))

#define A_WMI_SEND_EVENT_TO_APP(ar, eventId, datap, len)
#define A_WMI_SEND_GENERIC_EVENT_TO_APP(ar, eventId, datap, len)

#define A_WMI_BSSINFO_EVENT_RX(ar, devId, datap, len)   \
    Api_BssInfoEvent((ar), (devId), (datap), (len))

#define A_WMI_TKIP_MICERR_EVENT(devt, devId, keyid, ismcast)   \
    Api_TkipMicErrorEvent((devt), (devId), (keyid), (ismcast))

#define A_WMI_Ac2EndpointID(devht, devId, ac)\
    Driver_AC2EndpointID((devht), (devId), (ac))

#define A_WMI_Endpoint2Ac(devt, devId, ep) \
    Driver_EndpointID2AC((devt), (devId), (ep))
	
#define A_WMI_GET_PMK_RX(devt, devId, datap)\
	Api_GetPmkEvent((devt), (devId), (datap))

#define A_WMIX_DBGLOG_EVENT(wmip, devId, datap, len)\
	Api_RxDbglogEvent((wmip), (devId), (datap), (len))

#define A_WMIX_GPIO_DATA_EVENT(wmip, devId, datap, len)\
	Api_RxGpioDataEvent((wmip), (devId), (datap), (len))

#define A_WMIX_PFM_DATA_EVENT(wmip, devId, datap, len)\
	Api_RxPfmDataEvent((wmip), (devId), (datap), (len))

#define A_WMIX_PFM_DATA_DONE_EVENT(wmip, devId, datap, len)\
	Api_RxPfmDataDoneEvent((wmip), (devId), (datap), (len))
          
#if DRIVER_CONFIG_ENABLE_STORE_RECALL
#define A_WMI_STORERECALL_EVENT_RX(devt, devId, datap, len, osbuf) \
	Api_StoreRecallEvent((devt), (devId), (datap), (len), (osbuf))
#else
#define A_WMI_STORERECALL_EVENT_RX(devt, devId, datap, len, osbuf) \
       A_OK
#endif
#if DRIVER_CONFIG_ENABLE_STORE_RECALL
#define A_WMI_STORERECALL_START_EVENT_RX(devt, devId, datap, len, osbuf) \
	Api_StoreRecallStartEvent((devt), (devId), (datap), (len), (osbuf))
#endif
#define A_WMI_HOST_DSET_EVENT_STORE_RX(devt, devId, datap, len, osbuf) \
	Api_HostDsetStoreEvent((devt), (devId), (datap), (len), (osbuf))

#define A_WMI_HOST_DSET_EVENT_READ_RX(devt, devId, datap, len, osbuf) \
	Api_HostDsetReadEvent((devt), (devId), (datap), (len), (osbuf))

#define A_WMI_HOST_DSET_EVENT_CREATE_RX(devt, devId, datap, len, osbuf) \
	Api_HostDsetCreateEvent((devt), (devId), (datap), (len), (osbuf))

#define A_WMI_HOST_DSET_EVENT_WRITE_RX(devt, devId, datap, len, osbuf) \
	Api_HostDsetWriteEvent((devt), (devId), (datap), (len), (osbuf))

#define A_WMI_HOST_DSET_EVENT_READBACK_RX(devt, devId, datap, len, osbuf) \
	Api_HostDsetReadbackEvent((devt), (devId), (datap), (len), (osbuf))

#define A_WMI_HOST_DSET_EVENT_SYNC_RX(devt, devId, datap, len, osbuf) \
	Api_HostDsetSyncEvent((devt), (devId), (datap), (len), (osbuf))

#define A_WMI_DSET_EVENT_OP_RX(devt, devId, datap, len, osbuf) \
	Api_DsetOPEvent((devt), (devId), (datap), (len), (osbuf))

#define A_WMI_WPS_PROFILE_EVENT_RX(devt, devId, datap, len, osbuf) \
	Api_WpsProfileEvent((devt), (devId), (datap), (len), (osbuf))	

#if WLAN_CONFIG_11N_AGGR_SUPPORT
#define A_WMI_AGGR_RECV_ADDBA_REQ_EVT(devt, devId, cmd) \
	Api_AggrRecvAddbaReqEvent((devt), (devId), (cmd))
	
#define A_WMI_AGGR_RECV_DELBA_REQ_EVT(devt, devId, cmd) \
	Api_AggrRecvDelbaReqEvent((devt), (devId), (cmd))
#endif

#define A_WMI_GET_TEMPERATURE_REPLY(devt, datap, len)\
      Api_GetTemperatureReply((devt), (datap), (len)) 
#if ENABLE_STACK_OFFLOAD
#define A_WMI_SOCK_RESPONSE_EVENT_RX(devt, devId, datap, len, osbuf) \
   Api_SockResponseEventRx((devt), (devId), (datap), (len), (osbuf))      
#else
#define A_WMI_SOCK_RESPONSE_EVENT_RX(devt, devId, datap, len, osbuf)  A_OK
#endif
#if MANUFACTURING_SUPPORT
#define A_WMI_TEST_CMD_EVENT_RX(devt, devId, datap, len) \
    Api_TestCmdEventRx((devt), (devId), (datap), (len))
#endif
#ifdef __cplusplus
}
#endif

#endif
