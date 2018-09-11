//------------------------------------------------------------------------------
// Copyright (c) 2014 Qualcomm Atheros, Inc.
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
#include <custom_wlan_api.h>
#include <wmi_api.h>
#include <bmi.h>
#include <htc.h>
#include <wmi_host.h>
#if ENABLE_P2P_MODE
#include <wmi.h>
#include "p2p.h"
#endif
#include <qca.h>
#include <wlan_api.h>

#include "atheros_wifi.h"
#include "atheros_wifi_api.h"
#include "atheros_wifi_internal.h"
#include "atheros_stack_offload.h"
#include "dset_api.h"
#include "common_stack_offload.h"
#include "qcom_api.h"
//------------------------------------------------------------------------------------------------------------
qcom_wps_credentials_t gWpsCredentials;

//------------------------------------------------------------------------------------------------------------
extern QCA_CTX *QCA_CtxPtr;
extern QOSAL_CONST WMI_STORERECALL_CONFIGURE_CMD default_strrcl_config_cmd;

/*FUNCTION*-------------------------------------------------------------
 *
 * Function Name   : ascii_to_hex()
 * Returned Value  : hex counterpart for ascii
 * Comments  : Converts ascii character to correesponding hex value
 *
 *END*-----------------------------------------------------------------*/
static unsigned char ascii_to_hex(char val) {
    if ('0' <= val && '9' >= val) {
        return (unsigned char) (val - '0');
    } else if ('a' <= val && 'f' >= val) {
        return (unsigned char) (val - 'a') + 0x0a;
    } else if ('A' <= val && 'F' >= val) {
        return (unsigned char) (val - 'A') + 0x0a;
    }

    return 0xff;/* error */
}

#if 0
static enum p2p_wps_method qcom_p2p_get_wps(P2P_WPS_METHOD wps)
{
    switch (wps) {
        case P2P_WPS_NOT_READY:
        return WPS_NOT_READY;
        case P2P_WPS_PIN_LABEL:
        return WPS_PIN_LABEL;
        case P2P_WPS_PIN_DISPLAY:
        return WPS_PIN_DISPLAY;
        case P2P_WPS_PIN_KEYPAD:
        return WPS_PIN_KEYPAD;
        case P2P_WPS_PBC:
        return WPS_PBC;
        default:
        return WPS_NOT_READY;
    }
}
#endif

/*FUNCTION*-------------------------------------------------------------
 *
 * Function Name   : qcom_set_deviceid()
 * Returned Value  : A_ERROR is command failed, else A_OK
 * Comments	: Sets device ID in driver context
 *
 *END*-----------------------------------------------------------------*/
static A_STATUS qcom_set_deviceid(QOSAL_UINT16 id)
{
    A_STATUS           error = A_OK;
    QOSAL_VOID            *pCxt;
    QOSAL_VOID           **pWmi;
    A_DRIVER_CONTEXT  *pDCxt;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;
                                                                /* No change needed, don't return failure               */
    if (pDCxt->devId == id) {
        goto exit;
    }
    if (pDCxt->wps_in_progress) {
        error = A_ERROR;
        goto exit;
    }
    if (id >= WLAN_NUM_OF_DEVICES) {
        error = A_ERROR;
        goto exit;
    }

    ((struct wmi_t *) pWmi)->deviceid = id;

    pDCxt->devId = id;

    exit: return error;
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          SOCKET FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

int qcom_socket(int family, int type, int protocol)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_socket(pCxt, family, type, protocol));
}

int qcom_connect(int s, struct sockaddr* addr, int addrlen)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_connect(pCxt, s, addr, addrlen));
}

int qcom_bind(int s, struct sockaddr* addr, int addrlen)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_bind(pCxt, s, addr, addrlen));
}

int qcom_listen(int s, int backlog)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_listen(pCxt, s, backlog));
}

int qcom_accept(int s, struct sockaddr* addr, int *addrlen)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_accept(pCxt, s, addr, *addrlen));
}

int qcom_setsockopt(int s, int level, int name, void* arg, int arglen)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_setsockopt(pCxt, s, level, name, arg, arglen));
}

QOSAL_INT32 qcom_getsockopt(int s, int level, int name, void* arg, int arglen)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return (Api_getsockopt(pCxt, s, level, name, arg, arglen));
}

#if ZERO_COPY
int qcom_recv(int s,char** buf,int len,int flags)
#else
int qcom_recv(int s, char* buf, int len, int flags)
#endif
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_recvfrom(pCxt, s, (void*) buf, len, flags, NULL, NULL));
}

#if ZERO_COPY
int qcom_recvfrom(int s, char** buf, int len, int flags, struct sockaddr* from, int* fromlen)
#else
int qcom_recvfrom(int s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen)
#endif
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return (Api_recvfrom(pCxt, s, (void*) buf, len, flags, from,(QOSAL_UINT32*) fromlen));
}

int qcom_sendto(int s, char* buffer, int length, int flags, struct sockaddr* to, int tolen)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_sendto(pCxt, s, (QOSAL_UINT8*) buffer, length, flags, (SOCKADDR_T *)to, tolen));
}

int qcom_send(int s, char* buffer, int length, int flags)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_sendto(QCA_CtxPtr, s, (QOSAL_UINT8*) buffer, length, flags, NULL, NULL));
}

int qcom_socket_close(int s)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS)Api_shutdown(QCA_CtxPtr, s));
}

int qcom_select(QOSAL_UINT32 handle, QOSAL_UINT32 tv)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return (Api_select(pCxt, handle, tv));
}


/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          DNS CLIENT FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

#if ENABLE_DNS_CLIENT
A_STATUS qcom_dnsc_enable(QOSAL_BOOL enable)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return ((A_STATUS) Api_ip_dns_client(pCxt, enable));

}

A_STATUS qcom_dnsc_add_server_address(QOSAL_UINT8 *ipaddress, QOSAL_UINT8 type)
{
    IP46ADDR  dnsaddr;
    QOSAL_VOID   *pCxt = QCA_CtxPtr;


    A_MEMZERO(&dnsaddr, sizeof(IP46ADDR));
    if (type == ATH_AF_INET) {
        dnsaddr.type = ATH_AF_INET;
        dnsaddr.addr4 = (QOSAL_UINT32) *(QOSAL_UINT32 *) ipaddress;
    } else {
        dnsaddr.type = ATH_AF_INET6;
        A_MEMCPY(&dnsaddr.addr6, ipaddress, sizeof(IP6_ADDR_T));
    }
                                                                /* Wait for chip to be up.                              */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS) Api_ip_dns_server_addr(pCxt, &dnsaddr));

}

#define DELETE_DNS_SERVER_ADDR 1

A_STATUS qcom_dnsc_del_server_address(QOSAL_UINT8 *ipaddress, QOSAL_UINT8 type)
{
    IP46ADDR  dnsaddr;
    QOSAL_VOID   *pCxt = QCA_CtxPtr;

    A_MEMZERO(&dnsaddr, sizeof(IP46ADDR));
    dnsaddr.au1Rsvd[0] = DELETE_DNS_SERVER_ADDR;
    if (type == ATH_AF_INET) {
        dnsaddr.type = ATH_AF_INET;
        dnsaddr.addr4 = (QOSAL_UINT32) *(QOSAL_UINT32 *) ipaddress;
    } else {
        dnsaddr.type = ATH_AF_INET6;
        A_MEMCPY(&dnsaddr.addr6, ipaddress, sizeof(IP6_ADDR_T));
    }
                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return ((A_STATUS) Api_ip_dns_server_addr(pCxt, &dnsaddr));
}

A_STATUS qcom_dnsc_get_host_by_name(A_CHAR *pname, QOSAL_UINT32 *pipaddress) {
    DNC_CFG_CMD     DnsCfg;
    DNC_RESP_INFO   DnsRespInfo;
    QOSAL_VOID         *pCxt = QCA_CtxPtr;

    A_MEMZERO(&DnsRespInfo, sizeof(DnsRespInfo));
    A_STRCPY(DnsCfg.ahostname, pname);

    DnsCfg.domain = 2;
    DnsCfg.mode   = GETHOSTBYNAME;

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    if (A_OK != Api_ip_resolve_host_name(pCxt, &DnsCfg, &DnsRespInfo)) {
        return A_ERROR;
    }

    *pipaddress = A_CPU2BE32(DnsRespInfo.ipaddrs_list[0]);
    return A_OK;
}

A_STATUS qcom_dnsc_get_host_by_name2(A_CHAR *pname, QOSAL_UINT32 *pipaddress,QOSAL_INT32 domain, QOSAL_UINT32 mode)
{
    DNC_CFG_CMD    DnsCfg;
    DNC_RESP_INFO  DnsRespInfo;
    QOSAL_VOID        *pCxt = QCA_CtxPtr;


    A_MEMZERO(&DnsRespInfo, sizeof(DnsRespInfo));
    A_STRCPY(DnsCfg.ahostname, pname);
    DnsCfg.domain = domain;
    DnsCfg.mode   = mode;

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    if (A_OK != Api_ip_resolve_host_name(pCxt, &DnsCfg, &DnsRespInfo)) {
        return A_ERROR;
    }

    if (domain == 3)
        A_MEMCPY(pipaddress, &DnsRespInfo.ip6addrs_list[0], 16);
    else
        A_MEMCPY(pipaddress, &DnsRespInfo.ipaddrs_list[0], 4);
    return A_OK;
}
#endif

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          DNS SERVER FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

#if ENABLE_DNS_SERVER
A_STATUS qcom_dnss_enable(QOSAL_BOOL enable)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS) Api_ip_dns_server(pCxt, enable));
}

A_STATUS qcom_dns_local_domain(A_CHAR* local_domain)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return ((A_STATUS) Api_ip_dns_local_domain(pCxt, local_domain));
}

A_STATUS qcom_dns_entry_create(A_CHAR* hostname, QOSAL_UINT32 ipaddress)
{
    QOSAL_VOID   *pCxt = QCA_CtxPtr;
    IP46ADDR  dnsaddr;
   

    dnsaddr.type = ATH_AF_INET;
    dnsaddr.addr4 = ipaddress;

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS) Api_ipdns(pCxt, 1, hostname, &dnsaddr));
}

A_STATUS qcom_dns_entry_delete(A_CHAR* hostname, QOSAL_UINT32 ipaddress)
{
    QOSAL_VOID   *pCxt = QCA_CtxPtr;
    IP46ADDR  dnsaddr;

    
    dnsaddr.type = ATH_AF_INET;
    dnsaddr.addr4 = ipaddress;

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS) Api_ipdns(pCxt, 2, hostname, &dnsaddr));
}

A_STATUS qcom_dns_6entry_create(A_CHAR* hostname, QOSAL_UINT8* ip6addr)
{
    QOSAL_VOID   *pCxt = QCA_CtxPtr;
    IP46ADDR  dnsaddr;
    

    dnsaddr.type = ATH_AF_INET6;
    A_MEMCPY(&dnsaddr.addr6, ip6addr, sizeof(IP6_ADDR_T));

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS) Api_ipdns(pCxt, 1, hostname, &dnsaddr));
}

A_STATUS qcom_dns_6entry_delete(A_CHAR* hostname, QOSAL_UINT8* ip6addr)
{
    QOSAL_VOID   *pCxt = QCA_CtxPtr;
    IP46ADDR  dnsaddr;
    

    dnsaddr.type = ATH_AF_INET6;
    A_MEMCPY(&dnsaddr.addr6, ip6addr, sizeof(IP6_ADDR_T));

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS)Api_ipdns(pCxt, 2, hostname, &dnsaddr));
}

#endif

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          SNTP CLIENT FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

#if ENABLE_SNTP_CLIENT
void qcom_sntp_srvr_addr(int flag, char* srv_addr)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up.                              */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return;
    }

    (void)Api_ip_sntp_srvr_addr(pCxt, flag, srv_addr);
}

void qcom_sntp_get_time(QOSAL_UINT8 device_id, tSntpTime* time)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up.                              */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return;
    }

    (void)Api_ip_sntp_get_time(pCxt, time);
}

void qcom_sntp_get_time_of_day(QOSAL_UINT8 device_id, tSntpTM* time)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up.                              */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return;
    }

    (void)Api_ip_sntp_get_time_of_day(pCxt, time);
}
void qcom_sntp_zone(int hour, int min, int add_sub, int enable)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up.                              */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return;
    }

    (void)Api_ip_sntp_modify_zone_dse(pCxt, hour, min, add_sub, enable);
}

void qcom_sntp_query_srvr_address(QOSAL_UINT8 device_id, tSntpDnsAddr* addr)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up.                              */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return;
    }

    (void)Api_ip_sntp_query_srvr_address(pCxt, addr);
}

void qcom_enable_sntp_client(int enable)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                                /* Wait for chip to be up.                              */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return;
    }

    (void)Api_ip_sntp_client(pCxt, enable);
}
#endif
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             OTA FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
#if 0



#endif
/****************QCOM_OTA_APIs************************/


A_STATUS qcom_ota_upgrade(QOSAL_UINT8 device_id, QOSAL_UINT32 addr,A_CHAR *filename,QOSAL_UINT8 mode,QOSAL_UINT8 preserve_last,QOSAL_UINT8 protocol,QOSAL_UINT32 * resp_code,QOSAL_UINT32 *length)
{
   QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS)Api_ota_upgrade(pCxt,addr,filename,mode,preserve_last,protocol,resp_code,length));
}

A_STATUS qcom_ota_read_area(QOSAL_UINT32 offset,QOSAL_UINT32 size,A_UCHAR *buffer,QOSAL_UINT32 *retlen)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

                                                               
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS)Api_ota_read_area(pCxt,offset,size,buffer,retlen));
}

A_STATUS qcom_ota_done(QOSAL_BOOL good_image)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    return ((A_STATUS)Api_ota_done(pCxt,good_image));
}

QCOM_OTA_STATUS_CODE_t qcom_ota_session_start(QOSAL_UINT32 flags, QOSAL_UINT32 partition_index)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR;;
    }

    return ((QCOM_OTA_STATUS_CODE_t)Api_ota_session_start(pCxt, flags, partition_index));
}

QOSAL_UINT32 qcom_ota_partition_get_size(QOSAL_VOID)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR ;
    } 
    return(Api_ota_partition_get_size(pCxt));
}

QCOM_OTA_STATUS_CODE_t qcom_ota_partition_erase(QOSAL_VOID)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR ;
    }
    return((QCOM_OTA_STATUS_CODE_t )Api_ota_partition_erase(pCxt));
}

QCOM_OTA_STATUS_CODE_t qcom_ota_partition_verify_checksum(QOSAL_VOID)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR ;
    }
    return((QCOM_OTA_STATUS_CODE_t)Api_ota_partition_verify_checksum(pCxt));
}

QCOM_OTA_STATUS_CODE_t qcom_ota_parse_image_hdr(QOSAL_UINT8 *header,QOSAL_UINT32 *offset)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR;
    }
    return((QCOM_OTA_STATUS_CODE_t)Api_ota_parse_image_hdr(pCxt, header, offset));
}

A_INT32 qcom_ota_partition_read_data(QOSAL_UINT32 offset, QOSAL_UINT8 *buf,QOSAL_UINT32 size)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    QOSAL_UINT32 rtn_len;

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR;
    } 
    
    if( qcom_ota_read_area(offset,size,buf,&rtn_len) != A_OK )
        rtn_len = -1;
    
    return(rtn_len); 
}

QCOM_OTA_STATUS_CODE_t qcom_ota_partition_write_data(QOSAL_UINT32 offset, QOSAL_UINT8 *buf,QOSAL_UINT32 size,QOSAL_UINT32 *ret_size)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;

    
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR;
    }
    return ((QCOM_OTA_STATUS_CODE_t) Api_ota_partition_write_data(pCxt, offset, buf, size, ret_size));
}

QCOM_OTA_STATUS_CODE_t qcom_ota_session_end(QOSAL_UINT32 good_image)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR;
    }  
   return ((QCOM_OTA_STATUS_CODE_t)qcom_ota_done(good_image)); 
}

QCOM_OTA_STATUS_CODE_t qcom_ota_partition_format(QOSAL_UINT32 partition_index)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    QCOM_OTA_STATUS_CODE_t rtn;

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return QCOM_OTA_ERROR;
    }  
    
    if(partition_index == 0)
   	 return QCOM_OTA_ERR_INVALID_PARTITION_INDEX;

    if( (rtn = qcom_ota_session_start(QCOM_OTA_ERASING_RW_DSET, partition_index)) == QCOM_OTA_OK ) 
    {
        rtn = (QCOM_OTA_STATUS_CODE_t) qcom_ota_partition_erase();
        qcom_ota_session_end(0);	
    } 
    return rtn;
}

A_STATUS qcom_ota_set_callback(QOSAL_VOID *callback)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;


    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    return((A_STATUS)custom_ota_set_response_cb(Custom_Api_GetDriverCxt(0), callback));       
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             IP FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
A_STATUS qcom_ipconfig(QOSAL_UINT8 device_id, QOSAL_INT32 mode, QOSAL_UINT32* address, QOSAL_UINT32* submask, QOSAL_UINT32* gateway)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;


    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipconfig(pCxt, mode, address, submask, gateway, NULL, NULL);

    return (A_STATUS) error;
}

A_STATUS qcom_ip6_address_get (QOSAL_UINT8  device_id,
                               QOSAL_UINT8 *v6Global,
                               QOSAL_UINT8 *v6Link,
                               QOSAL_UINT8 *v6DefGw,
                               QOSAL_UINT8 *v6GlobalExtd,
                               QOSAL_INT32 *LinkPrefix,
                               QOSAL_INT32 *GlbPrefix,
                               QOSAL_INT32 *DefgwPrefix,
                               QOSAL_INT32 *GlbPrefixExtd)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;


    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }
    
    error = Api_ip6config(              QCA_CtxPtr,
                                        IPCFG_QUERY,
                          (IP6_ADDR_T*) v6Global,
                          (IP6_ADDR_T*) v6Link,
                          (IP6_ADDR_T*) v6DefGw,
                          (IP6_ADDR_T*) v6GlobalExtd,
                                        LinkPrefix,
                                        GlbPrefix,
                                        DefgwPrefix,
                                        GlbPrefixExtd);

    return (A_STATUS) error;
}

A_STATUS qcom_ping(QOSAL_UINT32 host, QOSAL_UINT32 size) {
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    QOSAL_INT32 error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ping(pCxt, host, size);

    return (A_STATUS) error;
}

A_STATUS qcom_ping6(QOSAL_UINT8* host, QOSAL_UINT32 size) {
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    QOSAL_INT32 error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ping6(pCxt, host, size);

    return (A_STATUS) error;
}

A_STATUS qcom_ip6config_router_prefix (QOSAL_UINT8  device_id,
                                       QOSAL_UINT8 *addr,
                                       QOSAL_INT32  prefixlen,
                                       QOSAL_INT32  prefix_lifetime,
                                       QOSAL_INT32  valid_lifetime)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;


    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ip6config_router_prefix(pCxt, addr, prefixlen, prefix_lifetime,
            valid_lifetime);

    return (A_STATUS) error;
}
A_STATUS qcom_bridge_mode_enable (QOSAL_UINT16 bridgemode)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    QOSAL_INT32 error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipbridgemode(pCxt, bridgemode);

    return (A_STATUS) error;
}

A_STATUS qcom_tcp_set_exp_backoff (QOSAL_UINT8 device_id, QOSAL_INT32 retry)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    QOSAL_INT32 error;

    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipconfig_set_tcp_exponential_backoff_retry(pCxt, retry);

    return (A_STATUS) error;
}

A_STATUS qcom_ip4_route(QOSAL_UINT8            device_id,
                        QOSAL_UINT32           cmd,
                        QOSAL_UINT32          *addr,
                        QOSAL_UINT32          *subnet,
                        QOSAL_UINT32          *gw,
                        QOSAL_UINT32          *ifindex,
                        IPV4_ROUTE_LIST_T *rtlist)
{
    QOSAL_VOID *pCxt = QCA_CtxPtr;
    QOSAL_INT32 error;


    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipv4_route(pCxt, cmd, addr, subnet, gw, ifindex, rtlist);

    return (A_STATUS) error;
}

A_STATUS qcom_ip6_route (QOSAL_UINT8            device_id,
                         QOSAL_UINT32           cmd,
                         QOSAL_UINT8           *ip6addr,
                         QOSAL_UINT32          *prefixLen,
                         QOSAL_UINT8           *gw,
                         QOSAL_UINT32          *ifindex,
                         IPV6_ROUTE_LIST_T *rtlist)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;


    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipv6_route(pCxt, cmd, ip6addr, prefixLen, gw, ifindex, rtlist);

    return (A_STATUS) error;
}

A_STATUS qcom_tcp_conn_timeout(QOSAL_UINT32 timeout_val)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_tcp_connection_timeout(pCxt, timeout_val);

    return (A_STATUS) error;


}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             DHCPs FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
QOSAL_INT32 qcom_dhcps_set_pool(QOSAL_UINT8 device_id, QOSAL_UINT32 startip, QOSAL_UINT32 endip,QOSAL_INT32 leasetime)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipconfig_dhcp_pool(pCxt, &startip, &endip, leasetime);

    return error;
}

A_STATUS qcom_dhcps_release_pool(QOSAL_UINT8 device_id)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipconfig_dhcp_release(pCxt);

    return (A_STATUS) error;
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                            HTTP FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
A_STATUS qcom_http_server(QOSAL_INT32 enable)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ip_http_server(pCxt, enable);

    return (A_STATUS) error;
}

A_STATUS qcom_ip_http_server_method(QOSAL_INT32  cmd,
                                    QOSAL_UINT8 *pagename,
                                    QOSAL_UINT8 *objname,
                                    QOSAL_INT32  objtype,
                                    QOSAL_INT32  objlen,
                                    QOSAL_UINT8 *value)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;

                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ip_http_server_method(pCxt, cmd, pagename, objname, objtype,
            objlen, value);

    return (A_STATUS) error;
}

A_STATUS qcom_http_client_method(QOSAL_UINT32 cmd, QOSAL_UINT8* url, QOSAL_UINT8 *data, QOSAL_UINT8 *value)
{
    QOSAL_VOID         *pCxt = QCA_CtxPtr;
    QOSAL_INT32         error;
    HTTPC_RESPONSE *output = NULL;

    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_httpc_method(pCxt, cmd, url, data, (QOSAL_UINT8**) &output);
    if (error == A_OK) {
        A_MEMCPY(value, output, sizeof(HTTPC_RESPONSE) + output->length - 1);
        zero_copy_http_free((QOSAL_VOID *) output);
    }

    return (A_STATUS) error;
}
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             SSL FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

SSL_CTX* qcom_SSL_ctx_new(SSL_ROLE_T role, QOSAL_INT32 inbufSize, QOSAL_INT32 outbufSize, QOSAL_INT32 reserved)
{
    return(SSL_ctx_new(role, inbufSize, outbufSize, reserved));
}

QOSAL_INT32 qcom_SSL_ctx_free(SSL_CTX *ctx) {
    return(SSL_ctx_free(ctx));
}

SSL* qcom_SSL_new(SSL_CTX *ctx) {
    return(SSL_new(ctx));
}

QOSAL_INT32 qcom_SSL_setCaList(SSL_CTX *ctx, SslCAList caList, QOSAL_UINT32 size) {
    return(SSL_setCaList(ctx, caList, size));
}

QOSAL_INT32 qcom_SSL_addCert(SSL_CTX *ctx, SslCert cert, QOSAL_UINT32 size) {
    return(SSL_addCert(ctx, cert, size));
}

QOSAL_INT32 qcom_SSL_storeCert(A_CHAR *name, SslCert cert, QOSAL_UINT32 size) {
    return(SSL_storeCert(name, cert, size));
}
QOSAL_INT32 qcom_SSL_loadCert(SSL_CTX *ctx, SSL_CERT_TYPE_T type, char *name) {
    return(SSL_loadCert(ctx, type, name));
}

QOSAL_INT32 qcom_SSL_listCert(SSL_FILE_NAME_LIST *fileNames) {
    return(SSL_listCert(fileNames));
}

QOSAL_INT32 qcom_SSL_set_fd(SSL *ssl, QOSAL_UINT32 fd) {
    return(SSL_set_fd(ssl, fd));
}

QOSAL_INT32 qcom_SSL_accept(SSL *ssl) {
    return(SSL_accept(ssl));
}

QOSAL_INT32 qcom_SSL_connect(SSL *ssl) {
    return(SSL_connect(ssl));
}

QOSAL_INT32 qcom_SSL_shutdown(SSL *ssl) {
    return(SSL_shutdown(ssl));
}

QOSAL_INT32 qcom_SSL_configure(SSL *ssl, SSL_CONFIG *cfg) {
    return(SSL_configure(ssl, cfg));
}

#if ZERO_COPY
QOSAL_INT32 qcom_SSL_read(SSL *ssl, void **buf, QOSAL_INT32 num)
#else
QOSAL_INT32 qcom_SSL_read(SSL *ssl, void *buf, QOSAL_INT32 num)
#endif
{
    return(SSL_read(ssl, buf, num));
}
QOSAL_INT32 qcom_SSL_write(SSL *ssl, const void *buf, QOSAL_INT32 num)
{
    return(SSL_write(ssl, buf, num));
}


/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             WIFI FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
A_STATUS _qcom_set_scan(A_UINT8 device_id, qcom_start_scan_params_t* pScanParams)
{
    QOSAL_VOID              *pCxt;
    QOSAL_VOID              *pWmi;
    A_DRIVER_CONTEXT        *pDCxt;
    A_STATUS                 error = A_OK;
    WMI_START_SCAN_CMD       scan_cmd;
    WMI_PROBED_SSID_CMD      probeParam;
    WMI_BSS_FILTER           filter;

    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }


    if (pDCxt->conn[pDCxt->devId].ssidLen > 0) {
                                                                /* apply filter ssid                                    */
        probeParam.entryIndex = 1;
        probeParam.flag = SPECIFIC_SSID_FLAG;
        probeParam.ssidLength = pDCxt->conn[pDCxt->devId].ssidLen;
        A_MEMCPY(probeParam.ssid, pDCxt->conn[pDCxt->devId].ssid, pDCxt->conn[pDCxt->devId].ssidLen);
        filter = PROBED_SSID_FILTER;
    } else {
                                                                /* clear any pre-existing filter ssid                   */
        probeParam.entryIndex = 1;
        probeParam.flag = DISABLE_SSID_FLAG;
        probeParam.ssidLength = 0;
        filter = ALL_BSS_FILTER;
    }

    error = wmi_cmd_start(               pWmi,
                          (QOSAL_VOID*) &probeParam,
                                         WMI_SET_PROBED_SSID_CMDID,
                                         sizeof(WMI_PROBED_SSID_CMD));
    if (error != A_OK) {
        goto exit;
    }

    error = wmi_bssfilter_cmd(pWmi, filter, 0);
    if (error != A_OK) {
        goto exit;
    }
                                                                /* Start the scan                                       */
    GET_DRIVER_CXT(pCxt)->scanOutSize  = ATH_MAX_SCAN_BUFFERS;
    GET_DRIVER_CXT(pCxt)->scanOutCount = 0;
    pDCxt->scanDone                    = QOSAL_FALSE;
    
    A_MEMZERO(&scan_cmd, sizeof(WMI_START_SCAN_CMD));
      
    if ( pScanParams == DEF_NULL ) {
        scan_cmd.scanType                  = WMI_LONG_SCAN;
        scan_cmd.forceFgScan               = QOSAL_FALSE;
        scan_cmd.isLegacy                  = QOSAL_FALSE;
        scan_cmd.homeDwellTime             = 0;
        scan_cmd.forceScanInterval         = 1;
        scan_cmd.numChannels               = 0;
    } else { 
        scan_cmd.forceFgScan = pScanParams->forceFgScan;
        scan_cmd.isLegacy = A_FALSE;
        scan_cmd.homeDwellTime = pScanParams->homeDwellTimeInMs;
        scan_cmd.forceScanInterval = pScanParams->forceScanIntervalInMs;
        scan_cmd.scanType = pScanParams->scanType;
        scan_cmd.numChannels = pScanParams->numChannels;
    }     

    error = wmi_cmd_start(           pWmi,
                          (QOSAL_VOID*) &scan_cmd,
                                     WMI_START_SCAN_CMDID,
                                     sizeof(WMI_START_SCAN_CMD));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;  
}

A_STATUS qcom_get_scan(QOSAL_UINT8 device_id, QCOM_BSS_SCAN_INFO** buf, A_INT16* numResults)
{
    QOSAL_VOID           *pCxt;
    QOSAL_VOID           *pWmi;
    A_DRIVER_CONTEXT *pDCxt;
    A_STATUS          error = A_OK;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->scanDone == QOSAL_FALSE) {
                                                                /* block until scan completes                           */
        DRIVER_WAIT_FOR_CONDITION(pCxt, &(pDCxt->scanDone), QOSAL_TRUE, 5000);

        if (pDCxt->scanDone == QOSAL_FALSE) {
            wmi_bssfilter_cmd(pWmi, NONE_BSS_FILTER, 0);
            error = A_ERROR;
        }
    }
    
    pDCxt->scanDone = QOSAL_FALSE;
   *buf             = (QCOM_BSS_SCAN_INFO *)(GET_DRIVER_CXT(pCxt)->pScanOut);
   *numResults      = GET_DRIVER_CXT(pCxt)->scanOutCount;
    
exit:
    return error;
}

A_STATUS qcom_set_ssid(QOSAL_UINT8 device_id, A_CHAR *pssid)
{
    QOSAL_VOID           *pCxt;
    A_DRIVER_CONTEXT *pDCxt;
    A_STATUS          error = A_OK;
    QOSAL_UINT32          len;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    len = A_STRLEN(pssid);
    if (len < 33) {
        pDCxt->conn[device_id].ssidLen = len;
        if (pDCxt->conn[device_id].ssidLen != 0u) {
            A_MEMCPY(pDCxt->conn[device_id].ssid, pssid, (QOSAL_UINT32)pDCxt->conn[device_id].ssidLen);
        } else {
            A_MEMZERO(pDCxt->conn[device_id].ssid, 32);
        }
    } else {
        error = A_ERROR;
    }

exit:
    return error;
}
A_STATUS qcom_get_ssid(QOSAL_UINT8 device_id, A_CHAR *pssid)
{
    QOSAL_VOID           *pCxt;
    A_DRIVER_CONTEXT *pDCxt;
    A_STATUS          error = A_OK;

    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    A_MEMCPY(pssid, pDCxt->conn[device_id].ssid, (QOSAL_UINT32) pDCxt->conn[device_id].ssidLen);
    pssid[pDCxt->conn[device_id].ssidLen] = '\0';

exit:
    return error;
}

A_STATUS qcom_set_connect_callback(QOSAL_UINT8 device_id, void *callback)
{
    QOSAL_VOID           *pCxt;
    A_DRIVER_CONTEXT *pDCxt;
    A_STATUS          error = A_OK;
    

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
    GET_DRIVER_CXT(pCxt)->connectStateCB = (void*)callback;
    DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

exit:
    return error;
}

#if ENABLE_SCC_MODE
static A_STATUS qcom_get_concurrent_channel(QOSAL_UINT8 device_id,
        QOSAL_UINT16 *conc_channel) {
    ATH_IOCTL_PARAM_STRUCT ath_param;
    A_STATUS error = A_OK;

    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    ath_param.cmd_id = ATH_GET_CONC_DEV_CHANNEL;
    ath_param.data = conc_channel;

    if (ath_ioctl_handler(QCA_CtxPtr, &ath_param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}
#endif

#if ENABLE_SCC_MODE
static A_STATUS qcom_get_channelhint(QOSAL_UINT8 device_id, QOSAL_UINT16 *pchannel) {
    ATH_IOCTL_PARAM_STRUCT inout_param;
    A_STATUS error = A_OK;

    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    inout_param.cmd_id = ATH_GET_CHANNELHINT;
    inout_param.data = pchannel;

    if (ath_ioctl_handler(QCA_CtxPtr, &inout_param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}
#endif

A_STATUS qcom_commit(QOSAL_UINT8 device_id)
{
    QOSAL_VOID           *pCxt;
    A_DRIVER_CONTEXT *pDCxt;
    A_STATUS          error = A_OK;
    

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }
    
#if !ENABLE_SCC_MODE

    if (pDCxt->conn[device_id].ssidLen == 0) {
        Api_DisconnectWiFi(pCxt);
    } else {
        error = Api_ConnectWiFi(pCxt);
        if (error != A_OK) {
            goto exit;
        }
    }
    pDCxt->wps_in_progress = QOSAL_FALSE;

#else 
    error = A_ERROR;
#endif

exit:
   return error;

#if 0
#if ENABLE_SCC_MODE 
    QOSAL_UINT16 conc_channel, cur_channel;
    QOSAL_UINT8 ssid[MAX_SSID_LENGTH];
    QOSAL_UINT8 val;
    int num_dev = WLAN_NUM_OF_DEVICES;

    if( qcom_get_ssid (device_id,(char*)ssid) != A_OK )
    return A_ERROR;

    /*this is connect request */
    if( ( A_STRLEN((char *)ssid) != 0 ) && (num_dev > 1)) {
        qcom_get_channelhint(device_id, &cur_channel);
        qcom_get_concurrent_channel(device_id, &conc_channel);
        /* if device 1 is connected and this connect request is from device 0, 
         check both devices whether stay at same channel, if not, return fail due to device 1 is primary
         */
        if( (conc_channel != 0) && (device_id == 0) && (conc_channel != cur_channel)) {
            return A_ERROR;
        }
        /* if device 0 is connected and this connect request is from device 1, 
         check both devices whether stay at same channel, if not, p2p_cancel is issue to device 0 because device 1 is promary
         */
        if( (conc_channel != 0) && (device_id == 1) && (conc_channel != cur_channel)) {
            qcom_set_deviceid(0);
            qcom_set_channel(0, conc_channel);
            A_MDELAY(50);
            qcom_p2p_func_cancel(0);
            A_MDELAY(500);
            qcom_set_deviceid(1);
            qcom_set_channel(1, cur_channel);
            A_MDELAY(50);
        }
    }

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    return(Custom_Api_Mediactl(QCA_CtxPtr,ENET_SET_MEDIACTL_COMMIT,NULL));

#else  
    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    return ((A_STATUS) Custom_Api_Mediactl(QCA_CtxPtr, ENET_SET_MEDIACTL_COMMIT,
            NULL));
#endif  //ENABLE_SCC_MODE
#endif
}

A_STATUS qcom_sta_get_rssi(QOSAL_UINT8 device_id, QOSAL_UINT8 *prssi) 
{
    QOSAL_VOID           *pCxt;
    QOSAL_VOID           *pWmi;
    A_DRIVER_CONTEXT *pDCxt;
    A_STATUS          error = A_OK;

    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    pDCxt->rssiFlag = QOSAL_TRUE;
    wmi_cmd_start(pWmi, NULL, WMI_GET_STATISTICS_CMDID, 0);

    while(pDCxt->rssiFlag == QOSAL_TRUE){
        DRIVER_WAIT_FOR_CONDITION(pCxt, &(pDCxt->rssiFlag), QOSAL_FALSE, 1000);
    }

   *prssi = pDCxt->rssi;

exit:
    return error;
}

/* for hostless , the listentime is 1Tus, 1Tus = 1024 us. need to confirm the UNIT */
A_STATUS qcom_sta_set_listen_time(QOSAL_UINT8 device_id, QOSAL_UINT32 listentime)
{
    QOSAL_VOID              *pCxt;
    QOSAL_VOID              *pWmi;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;
    WMI_LISTEN_INT_CMD   listen;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    listen.listenInterval = A_CPU2LE16(listentime);
    listen.numBeacons     = A_CPU2LE16(0);
    error = wmi_cmd_start(            pWmi,
                          (QOSAL_VOID *) &listen,
                                      WMI_SET_LISTEN_INT_CMDID,
                                      sizeof(WMI_LISTEN_INT_CMD));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;
}

A_STATUS qcom_ap_set_beacon_interval(QOSAL_UINT8 device_id, QOSAL_UINT16 beacon_interval)
{
    QOSAL_VOID              *pCxt;
    QOSAL_VOID              *pWmi;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;
    ATH_AP_PARAM_STRUCT  ap_param;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    ap_param.cmd_subset = AP_SUB_CMD_BCON_INT;
    ap_param.data       = &beacon_interval;

    error = wmi_ap_set_param(pWmi, (QOSAL_VOID *)&ap_param);

exit:
    return error;
}

A_STATUS qcom_ap_hidden_mode_enable(QOSAL_UINT8 device_id, QOSAL_BOOL enable)
{
    QOSAL_VOID              *pCxt;
    QOSAL_VOID              *pWmi;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;
    ATH_AP_PARAM_STRUCT  ap_param;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    ap_param.cmd_subset = AP_SUB_CMD_HIDDEN_FLAG;
    ap_param.data       = &enable;

    error = wmi_ap_set_param(pWmi, (QOSAL_VOID *)&ap_param);

exit:
    return error;

}

A_STATUS qcom_op_get_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 *pmode)
{
    QOSAL_VOID              *pCxt;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);


    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    switch (pDCxt->conn[device_id].networkType) {
        case INFRA_NETWORK:
            *pmode = QCOM_WLAN_DEV_MODE_STATION;
             break;

        case ADHOC_NETWORK:
            *pmode = QCOM_WLAN_DEV_MODE_ADHOC;
             break;

        case AP_NETWORK:
            *pmode = QCOM_WLAN_DEV_MODE_AP;
             break;

        default:
             error = A_ERROR;
            *pmode = QCOM_WLAN_DEV_MODE_INVALID;
             break;
    }

exit:
    return error;
}

A_STATUS qcom_op_set_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode)
{
    QOSAL_VOID              *pCxt;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;

    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    switch (mode) {

        case QCOM_WLAN_DEV_MODE_STATION:
             pDCxt->conn[device_id].networkType = INFRA_NETWORK;
             break;

        case QCOM_WLAN_DEV_MODE_ADHOC:
             pDCxt->conn[device_id].networkType = ADHOC_NETWORK;
             break;

        case QCOM_WLAN_DEV_MODE_AP:
             pDCxt->conn[device_id].networkType = AP_NETWORK;
             break;

        default:
             error = A_ERROR;
             break;
    }
exit:
    return error;
}

A_STATUS qcom_disconnect(QOSAL_UINT8 device_id)
{
    A_STATUS error = A_OK;

    error = qcom_set_ssid(device_id,"");
    if (error != A_OK) {
        goto exit;
    }
    error = qcom_commit(device_id);
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;

}

A_STATUS qcom_get_phy_mode(QOSAL_UINT8 device_id, QOSAL_UINT8 *pphymode)
{
    QOSAL_VOID              *pCxt;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;

    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    switch (pDCxt->conn[pDCxt->devId].phyMode) {

        case WMI_11A_MODE:
             A_STRCPY((char * )pphymode, "a");
             break;

        case WMI_11G_MODE:
             A_STRCPY((char * )pphymode, "g");
             break;

        case WMI_11B_MODE:
             A_STRCPY((char * )pphymode, "b");
             break;

        case WMI_11GONLY_MODE:
        case WMI_11AG_MODE:
        default:
             A_STRCPY((char * )pphymode, "mixed");
             break;
    }
exit:
    return error;
}

A_STATUS qcom_set_phy_mode(QOSAL_UINT8 device_id, QOSAL_UINT8 phymode)
{
    QOSAL_VOID                 *pCxt;
    QOSAL_VOID                 *pWmi;
    A_DRIVER_CONTEXT       *pDCxt;
    A_STATUS                error = A_OK;
    WMI_SET_HT_CAP_CMD      ht_cap_cmd;
    WMI_CHANNEL_PARAMS_CMD  channel_param_cmd;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    A_MEMZERO(&ht_cap_cmd, sizeof(WMI_SET_HT_CAP_CMD));

    switch (pDCxt->conn[pDCxt->devId].phyMode) {

        case QCOM_11A_MODE:
             error = A_ERROR;
             goto exit;

        case QCOM_11B_MODE:
             pDCxt->conn[pDCxt->devId].phyMode = WMI_11B_MODE;
             break;

        case QCOM_11G_MODE:
             pDCxt->conn[pDCxt->devId].phyMode = WMI_11G_MODE;
             break;

        case QCOM_11N_MODE:
             pDCxt->conn[pDCxt->devId].phyMode = (pDCxt->conn[pDCxt->devId].phyMode == WMI_11A_MODE)?WMI_11A_MODE:WMI_11G_MODE;
             ht_cap_cmd.band                   = (pDCxt->conn[pDCxt->devId].phyMode == WMI_11A_MODE)?0x01:0x00;
             ht_cap_cmd.enable                 = 1;
             ht_cap_cmd.short_GI_20MHz         = 1;
             ht_cap_cmd.max_ampdu_len_exp      = 2;
             break;

        case QCOM_HT40_MODE:
             pDCxt->conn[pDCxt->devId].phyMode   = (pDCxt->conn[pDCxt->devId].phyMode == WMI_11A_MODE)?WMI_11A_MODE:WMI_11G_MODE;
             ht_cap_cmd.band                     = (pDCxt->conn[pDCxt->devId].phyMode == WMI_11A_MODE)?0x01:0x00;
             ht_cap_cmd.enable                   = 1;
             ht_cap_cmd.short_GI_20MHz           = 1;
             ht_cap_cmd.short_GI_40MHz           = 1;
             ht_cap_cmd.intolerance_40MHz        = 0;
             ht_cap_cmd.max_ampdu_len_exp        = 2;
             ht_cap_cmd.chan_width_40M_supported = 1;
             break;

        default:
             error = A_ERROR;
             goto exit;
    }

    A_MEMZERO(&channel_param_cmd, sizeof(WMI_CHANNEL_PARAMS_CMD));

    channel_param_cmd.scanParam = 1;
    channel_param_cmd.phyMode   = pDCxt->conn[pDCxt->devId].phyMode;

    error = wmi_cmd_start(            pWmi,
                          (QOSAL_VOID *) &channel_param_cmd,
                                      WMI_SET_CHANNEL_PARAMS_CMDID,
                                      sizeof(WMI_CHANNEL_PARAMS_CMD));
    if (error != A_OK) {
        goto exit;
    }

    error = wmi_cmd_start(            pWmi,
                          (QOSAL_VOID *) &ht_cap_cmd,
                                      WMI_SET_HT_CAP_CMDID,
                                      sizeof(WMI_SET_HT_CAP_CMD));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;
}
A_STATUS qcom_get_country_code(A_UINT8 device_id, A_UINT8* countryCode)
{
    
    QOSAL_VOID                     *pCxt;
    QOSAL_VOID                     *pWmi;
    A_DRIVER_CONTEXT               *pDCxt;
    A_STATUS                        error = A_OK;
    WMI_GET_COUNTRY_CODE_REPLY      code_reply;   
    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }
    
    pDCxt->countryCodeValid = A_FALSE;
  
    if (A_OK == wmi_cmd_start(pWmi, &code_reply, WMI_GET_COUNTRY_CODE_CMDID, sizeof(WMI_GET_COUNTRY_CODE_REPLY))) {
        /* block here until event arrives from wifi device */
        DRIVER_WAIT_FOR_CONDITION(pCxt, &(pDCxt->countryCodeValid), A_TRUE, 1000);
    }
  
    if (pDCxt->countryCodeValid == A_FALSE) {
        error = A_ERROR;
    } else{
        A_MEMCPY(countryCode, pDCxt->raw_countryCode, 3);
        pDCxt->countryCodeValid = A_FALSE;
    }
    
exit:    
    return error;
}

A_STATUS qcom_set_country_code(A_UINT8 device_id, A_UINT8* countryCode)
{
    QOSAL_VOID                     *pCxt;
    A_DRIVER_CONTEXT               *pDCxt;
    A_STATUS                        error = A_OK;
    ATH_PROGRAM_COUNTRY_CODE_PARAM  cmd;
    
    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    } 
    
    A_MEMZERO(cmd.countryCode, 3);
    A_MEMCPY(cmd.countryCode, countryCode, 3);
     
    if(A_OK != Api_ProgramCountryCode(pCxt, cmd.countryCode, sizeof(ATH_PROGRAM_COUNTRY_CODE_PARAM ), &cmd.result)){
        error = A_ERROR;
    }
exit:    
    return error; 
}
A_STATUS qcom_set_channel(QOSAL_UINT8 device_id, QOSAL_UINT16 channel)
{
    QOSAL_VOID              *pCxt;
    QOSAL_VOID              *pWmi;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;
    WMI_SET_CHANNEL_CMD  set_channel_param;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    if (channel < 1 || channel > 165) {
        error = A_ERROR;
        goto exit;
    }

    if (channel < 27) {
        channel = ATH_IOCTL_FREQ_1 + (channel - 1) * 5;
    } else {
        channel = (5000 + (channel * 5));
    }

    pDCxt->conn[pDCxt->devId].channelHint = channel;
    set_channel_param.channel             = A_CPU2LE16(pDCxt->conn[pDCxt->devId].channelHint);

    error = wmi_cmd_start(           pWmi,
                          (QOSAL_VOID*) &set_channel_param,
                                     WMI_SET_CHANNEL_CMDID,
                                     sizeof(QOSAL_UINT16));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;
}

A_STATUS qcom_get_channel(QOSAL_UINT8 device_id, QOSAL_UINT16 *pchannel)
{
    QOSAL_VOID              *pCxt;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

   *pchannel = pDCxt->conn[pDCxt->devId].bssChannel;

exit:
    return error;

}

A_STATUS qcom_get_tx_power(QOSAL_UINT8 device_id, QOSAL_UINT32 *pdbm) {
    /* No command id for get tx_power */
    return A_OK;
}

A_STATUS qcom_set_tx_power(QOSAL_UINT8 device_id, QOSAL_UINT32 dbm)
{
    QOSAL_VOID              *pCxt;
    QOSAL_VOID              *pWmi;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error = A_OK;
    WMI_SET_TX_PWR_CMD   set_tx_pwr_param;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    set_tx_pwr_param.dbM = dbm;

    error = wmi_cmd_start(            pWmi,
                          (QOSAL_VOID *) &set_tx_pwr_param,
                                      WMI_SET_TX_PWR_CMDID,
                                      sizeof(WMI_SET_TX_PWR_CMD));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;
}

A_STATUS qcom_allow_aggr_set_tid(QOSAL_UINT8 device_id, QOSAL_UINT16 tx_allow_aggr, QOSAL_UINT16 rx_allow_aggr)
{
    QOSAL_VOID                    *pCxt;
    QOSAL_VOID                    *pWmi;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    WMI_ALLOW_AGGR_CMD         allow_aggr_param;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }


    pDCxt->txAggrTidMask           = tx_allow_aggr;
    pDCxt->rxAggrTidMask           = rx_allow_aggr;

    allow_aggr_param.tx_allow_aggr = A_CPU2LE16(pDCxt->txAggrTidMask);
    allow_aggr_param.rx_allow_aggr = A_CPU2LE16(pDCxt->rxAggrTidMask);

    error = wmi_cmd_start(            pWmi,
                          (QOSAL_VOID *) &allow_aggr_param,
                                      WMI_ALLOW_AGGR_CMDID,
                                      sizeof(WMI_ALLOW_AGGR_CMD));
    if (error != A_OK) {
        goto exit;
    }


exit:
    return error;
}

A_STATUS qcom_scan_set_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode)
{
    QOSAL_VOID                    *pCxt;
    QOSAL_VOID                    *pWmi;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    WMI_SCAN_PARAMS_CMD        scan_param_cmd = {0,0,0,0,0,WMI_SHORTSCANRATIO_DEFAULT, DEFAULT_SCAN_CTRL_FLAGS, 0, 0,0};

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }


    scan_param_cmd.fg_start_period  = ((mode & ATH_DISABLE_FG_SCAN)? A_CPU2LE16(0xffff):A_CPU2LE16(0));
    scan_param_cmd.fg_end_period    = ((mode & ATH_DISABLE_FG_SCAN)? A_CPU2LE16(0xffff):A_CPU2LE16(0));
    scan_param_cmd.bg_period        = ((mode & ATH_DISABLE_BG_SCAN)? A_CPU2LE16(0xffff):A_CPU2LE16(0));

    error = wmi_cmd_start(            pWmi,
                          (QOSAL_VOID *) &scan_param_cmd,
                                      WMI_SET_SCAN_PARAMS_CMDID,
                                      sizeof(WMI_SCAN_PARAMS_CMD));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;
}

/***************************************************************************************
 * QCOM SECURITY APIs
 ***************************************************************************************/
/*FUNCTION*-----------------------------------------------------------------
 *
 * Function Name  : set_wep_key
 * Returned Value : A_ERROR on error else A_OK
 * Comments       : Store WEP key for later use. Size of Key must be 10 or 26
 *                  Hex characters
 *
 *END------------------------------------------------------------------*/
A_STATUS qcom_sec_set_wepkey(QOSAL_UINT8 device_id, QOSAL_UINT32 keyindex, A_CHAR *pkey)
{
    QOSAL_UINT8           val;
    QOSAL_UINT32          len;
    A_DRIVER_CONTEXT *pDCxt;

    pDCxt = GET_DRIVER_COMMON(QCA_CtxPtr);
    len   = A_STRLEN(pkey);
    keyindex--;

                                                                /* copy wep keys to driver context for use later during connect*/
    if (keyindex > WMI_MAX_KEY_INDEX){
        return A_ERROR;
    }

    if (((len != WEP_SHORT_KEY * 2) && (len != WEP_LONG_KEY * 2)) &&
        ((len != WEP_SHORT_KEY    ) && (len != WEP_LONG_KEY    ))) {
        return A_ERROR;
    }

    if ((len == WEP_SHORT_KEY) ||
        (len == WEP_LONG_KEY )) {
        A_MEMCPY(pDCxt->conn[device_id].wepKeyList[keyindex].key, pkey, len);
        pDCxt->conn[device_id].wepKeyList[keyindex].keyLen = (QOSAL_UINT8) len;
    } else {
        pDCxt->conn[device_id].wepKeyList[keyindex].keyLen = (QOSAL_UINT8) (len >> 1);
        A_MEMZERO(pDCxt->conn[device_id].wepKeyList[keyindex].key, MAX_WEP_KEY_SZ);
                                                                /* convert key data from string to bytes                */
        for (int ii = 0; ii < len; ii++) {
            if ((val = ascii_to_hex(pkey[ii])) == 0xff) {
                return A_ERROR;
            }
            if ((ii & 1) == 0) {
                val <<= 4;
            }
            pDCxt->conn[device_id].wepKeyList[keyindex].key[ii >> 1] |= val;
        }
    }

    return A_OK;
}

A_STATUS qcom_sec_get_wepkey(QOSAL_UINT8 device_id, QOSAL_UINT32 keyindex, A_CHAR *pkey)
{
    A_DRIVER_CONTEXT *pDCxt;


    pDCxt = GET_DRIVER_COMMON(QCA_CtxPtr);

    if (keyindex < 1 || keyindex > WMI_MAX_KEY_INDEX + 1)
        return A_ERROR;

    keyindex--;

    A_MEMCPY(pkey,
             pDCxt->conn[device_id].wepKeyList[keyindex].key,
             pDCxt->conn[device_id].wepKeyList[keyindex].keyLen);

    return A_OK;
}

A_STATUS qcom_sec_set_wepkey_index(QOSAL_UINT8 device_id, QOSAL_UINT32 keyindex)
{
    QOSAL_VOID                    *pCxt;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    if ((keyindex - 1) > WMI_MAX_KEY_INDEX)
        return A_ERROR;

    pDCxt->conn[device_id].wepDefTxKeyIndex = (QOSAL_UINT8) (keyindex - 1);

    pDCxt->conn[device_id].wpaAuthMode       = NONE_AUTH;
    pDCxt->conn[device_id].wpaPairwiseCrypto = WEP_CRYPT;
    pDCxt->conn[device_id].wpaGroupCrypto    = WEP_CRYPT;
                                                                /* allow either form of auth for WEP                    */
    pDCxt->conn[device_id].dot11AuthMode     = (OPEN_AUTH | SHARED_AUTH);
    pDCxt->conn[device_id].connectCtrlFlags &= ~CONNECT_DO_WPA_OFFLOAD;
exit:
    return error;
}

A_STATUS qcom_sec_get_wepkey_index(QOSAL_UINT8 device_id, QOSAL_UINT32 *pkeyindex)
{
    A_DRIVER_CONTEXT *pDCxt;


    pDCxt = GET_DRIVER_COMMON(QCA_CtxPtr);

   *pkeyindex = (QOSAL_UINT32) (pDCxt->conn[device_id].wepDefTxKeyIndex + 1);

    return A_OK;
}

A_STATUS qcom_sec_set_auth_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode)
{
    QOSAL_VOID                    *pCxt;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }
    switch (mode) {
        case WLAN_AUTH_NONE:
             pDCxt->conn[device_id].wpaAuthMode = NONE_AUTH;
             pDCxt->conn[device_id].wpaPairwiseCrypto = NONE_CRYPT;
             pDCxt->conn[device_id].wpaGroupCrypto = NONE_CRYPT;
             pDCxt->conn[device_id].dot11AuthMode = OPEN_AUTH;
             pDCxt->conn[device_id].connectCtrlFlags &= ~CONNECT_DO_WPA_OFFLOAD;
             break;
        case WLAN_AUTH_WPA_PSK:
        case WLAN_AUTH_WPA:
             pDCxt->conn[device_id].wpaAuthMode = WPA_PSK_AUTH;
             pDCxt->conn[device_id].dot11AuthMode = OPEN_AUTH;
             /* by ignoring the group cipher the wifi can connect to mixed mode AP's */
             pDCxt->conn[device_id].connectCtrlFlags |= CONNECT_DO_WPA_OFFLOAD | CONNECT_IGNORE_WPAx_GROUP_CIPHER;
             break;
        case WLAN_AUTH_WPA2_PSK:
        case WLAN_AUTH_WPA2:
             pDCxt->conn[device_id].wpaAuthMode = WPA2_PSK_AUTH;
             pDCxt->conn[device_id].dot11AuthMode = OPEN_AUTH;
             /* by ignoring the group cipher the wifi can connect to mixed mode AP's */
             pDCxt->conn[device_id].connectCtrlFlags |= CONNECT_DO_WPA_OFFLOAD | CONNECT_IGNORE_WPAx_GROUP_CIPHER;
             break;
        default:
             error = A_ERROR;
             break;
    }

exit:
    return error;

}

A_STATUS qcom_sec_set_encrypt_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 mode)
{
    QOSAL_VOID                    *pCxt;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }
    pDCxt->conn[pDCxt->devId].wpaPairwiseCrypto = 0x1 << mode;
    pDCxt->conn[pDCxt->devId].wpaGroupCrypto    = 0x1 << mode;

exit:
    return error;
}

A_STATUS qcom_sec_set_passphrase(QOSAL_UINT8 device_id, A_CHAR *passphrase)
{
    QOSAL_VOID                    *pCxt;
    QOSAL_VOID                    *pWmi;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    QOSAL_UINT32                   len;
    WMI_SET_PASSPHRASE_CMD     passCmd;
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    len = A_STRLEN(passphrase);

    if (len > WMI_PASSPHRASE_LEN) {
        error = A_ERROR;
    } else if (pDCxt->conn[device_id].ssidLen == 0) {
        error = A_ERROR;
    } else {
        passCmd.ssid_len       = (QOSAL_UINT8)pDCxt->conn[device_id].ssidLen;
        passCmd.passphrase_len = (QOSAL_UINT8)len;
        A_MEMCPY(passCmd.ssid, pDCxt->conn[device_id].ssid, (QOSAL_UINT32)pDCxt->conn[device_id].ssidLen);
        A_MEMCPY(passCmd.passphrase, passphrase, passCmd.passphrase_len);

        error = wmi_cmd_start(            pWmi,
                              (QOSAL_VOID *) &passCmd,
                                          WMI_SET_PASSPHRASE_CMDID,
                                          sizeof(WMI_SET_PASSPHRASE_CMD));
    }

exit:
    return error;
}

A_STATUS qcom_sec_set_pmk(QOSAL_UINT8 device_id, A_CHAR *pmk)
{
    QOSAL_VOID                    *pCxt;
    QOSAL_VOID                    *pWmi;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    WMI_SET_PMK_CMD            cmd;
    QOSAL_UINT8                    val = 0;
    QOSAL_INT32                    j;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    A_MEMZERO(cmd.pmk, WMI_PMK_LEN);
    cmd.pmk_len = WMI_PMK_LEN;

    for (j = 0; j < 64; j++) {
        val = ascii_to_hex(pmk[j]);
        if (val == 0xff) {
            A_PRINTF("Invalid character\n");
            return A_ERROR;
        } else {
            if ((j & 1) == 0) {
                val <<= 4;
            }
            cmd.pmk[j >> 1] |= val;
        }
    }
    error = wmi_cmd_start(           pWmi,
                          (QOSAL_VOID *)&cmd,
                                     WMI_SET_PMK_CMDID,
                                     sizeof(WMI_SET_PMK_CMD));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;
}

A_STATUS qcom_power_set_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 powerMode, QOSAL_UINT8 powerModule)
{
    QOSAL_VOID                    *pCxt;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    POWER_MODE                 power;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
 

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    power.pwr_module = powerModule;
    power.pwr_mode   = powerMode;

    error = Api_SetPowerMode(pCxt,(POWER_MODE *)&power);

exit:
    return error;
}

A_STATUS qcom_power_get_mode(QOSAL_UINT8 device_id, QOSAL_UINT32 *powerMode)
{
    QOSAL_VOID                    *pCxt;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

   *powerMode = pDCxt->userPwrMode;

exit:
    return error;
}

A_STATUS qcom_suspend_enable(QOSAL_BOOL enable)
{
#if DRIVER_CONFIG_ENABLE_STORE_RECALL
    QOSAL_VOID                    *pCxt;
    QOSAL_VOID                    *pWmi;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->strrclState == STRRCL_ST_DISABLED) {
        if(A_OK == wmi_cmd_start( pWmi,
                                 &default_strrcl_config_cmd,
                                  WMI_STORERECALL_CONFIGURE_CMDID,
                                  sizeof(WMI_STORERECALL_CONFIGURE_CMD))) {
            pDCxt->strrclState = STRRCL_ST_INIT;
            error = A_OK;
        } else {
            if (pDCxt->strrclState != STRRCL_ST_INIT) {
                error = A_ERROR;
            }
        }
    } else {
        if (pDCxt->strrclState != STRRCL_ST_INIT) {
            error = A_ERROR;
        }
    }
exit:
    return error;
#else
    return A_ERROR;
#endif
}

A_STATUS qcom_suspend_start(QOSAL_UINT32 susp_time)
{
    QOSAL_VOID                        *pCxt;
    QOSAL_VOID                        *pWmi;
    A_DRIVER_CONTEXT              *pDCxt;
    A_STATUS                       error = A_OK;
    WMI_STORERECALL_HOST_READY_CMD storerecall_ready_param;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;
    
    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }
    
    error = move_power_state_to_maxperf(pDCxt, 1);/* refer POWER_STATE_MOVED_FOR_STRRCL */
    if (error != A_OK) {
        goto exit;
    }

    if ((pDCxt->strrclState == STRRCL_ST_INIT ) && 
        (susp_time          >= MIN_STRRCL_MSEC)) {
            /* set strrclBlock before call to wmi_storerecall_ready_cmd to prevent
             * any potential race conditions with other tasks calling into the 
             * driver. 
             */
        pDCxt->strrclBlock = QOSAL_TRUE;
        storerecall_ready_param.sleep_msec                  = A_CPU2LE32((susp_time));
        storerecall_ready_param.store_after_tx_empty        = 1;
        storerecall_ready_param.store_after_fresh_beacon_rx = 1;
            
        error = wmi_cmd_start(pWmi, &storerecall_ready_param, 
                              WMI_STORERECALL_HOST_READY_CMDID, 
                              sizeof(WMI_STORERECALL_HOST_READY_CMD));
            
		if (error == A_OK){
			pDCxt->strrclState = STRRCL_ST_START;	
		} else {
            pDCxt->strrclBlock = QOSAL_FALSE;
        }
    }
exit:
    return error;
}

A_STATUS qcom_power_set_parameters(QOSAL_UINT8  device_id,
                                   QOSAL_UINT16 idlePeriod,
                                   QOSAL_UINT16 psPollNum,
                                   QOSAL_UINT16 dtimPolicy,
                                   QOSAL_UINT16 tx_wakeup_policy,
                                   QOSAL_UINT16 num_tx_to_wakeup,
                                   QOSAL_UINT16 ps_fail_event_policy)
{
    QOSAL_VOID                    *pCxt;
    QOSAL_VOID                    *pWmi;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    ATH_WMI_POWER_PARAMS_CMD   pm;
    

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    pm.idle_period          = A_CPU2LE16(idlePeriod);
    pm.pspoll_number        = A_CPU2LE16(psPollNum);
    pm.dtim_policy          = A_CPU2LE16(dtimPolicy);
    pm.tx_wakeup_policy     = A_CPU2LE16(tx_wakeup_policy);
    pm.num_tx_to_wakeup     = A_CPU2LE16(num_tx_to_wakeup);
    pm.ps_fail_event_policy = A_CPU2LE16(ps_fail_event_policy);

    error = wmi_cmd_start(           pWmi,
                          (QOSAL_VOID*) &pm,
                                     WMI_SET_POWER_PARAMS_CMDID,
                                     sizeof(WMI_POWER_PARAMS_CMD));
    if (error != A_OK) {
        goto exit;
    }

exit:
    return error;
}

A_STATUS qcom_get_bssid(QOSAL_UINT8 device_id, QOSAL_UINT8 mac_addr[ATH_MAC_LEN])
{
    QOSAL_VOID                    *pCxt;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    
    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    A_MEMCPY((QOSAL_UINT8 *)mac_addr, QCA_CtxPtr->DeviceMacAddr, ATH_MAC_LEN);

exit:
    return error;
}

/***************************************************************************************************************
 * QCOM WPS APIs
 ****************************************************************************************************************/
#define WPS_STANDARD_TIMEOUT (30)
A_STATUS qcom_wps_start(QOSAL_UINT8 device_id, int connect, int use_pinmode, char *pin)
{

    /* FIXME: there exists the possibility of a race condition if the device has
     * sent a WPS event in response to a previous ATH_START_WPS then it becomes
     * possible for the driver to misinterpret that event as being the result of
     * the most recent ATH_START_WPS.  To fix this the wmi command should include
     * an ID value that is returned by the WPS event so that the driver can
     * accurately match an event from the device to the most recent WPS command.
     */
    QOSAL_VOID                    *pCxt;
    QOSAL_VOID                    *pWmi;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    WMI_WPS_START_CMD          wps_start;
    WMI_SCAN_PARAMS_CMD        scan_param_cmd = {0,0,0,0,0,WMI_SHORTSCANRATIO_DEFAULT, DEFAULT_SCAN_CTRL_FLAGS, 0, 0,0};;

    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == QOSAL_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt);
    if(pDCxt->wpsBuf){
        A_NETBUF_FREE(pDCxt->wpsBuf);
        pDCxt->wpsBuf = NULL;
    }

    pDCxt->wpsEvent = NULL;

    if(pDCxt->wpsState == QOSAL_TRUE){
        pDCxt->wpsState = QOSAL_FALSE;
    }
    DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt);

    A_MEMZERO(&wps_start, sizeof(WMI_WPS_START_CMD));
    wps_start.timeout = WPS_STANDARD_TIMEOUT;
    wps_start.role    = (AP_NETWORK == pDCxt->conn[pDCxt->devId].networkType)?WPS_REGISTRAR_ROLE:WPS_ENROLLEE_ROLE;

    if (0x02 == connect) {
        wps_start.role = WPS_AP_ENROLLEE_ROLE;
    }

    if (use_pinmode == 1) {
        wps_start.config_mode        = WPS_PIN_MODE;
        wps_start.wps_pin.pin_length =  A_STRLEN(pin);
        A_MEMCPY(wps_start.wps_pin.pin, pin, ATH_WPS_PIN_LEN);
    }else {
        wps_start.config_mode = WPS_PBC_MODE;
    }

    if (AP_NETWORK != pDCxt->conn[pDCxt->devId].networkType)
    {
        if (0x08 == connect) {
                                                                /* WPS Request from P2P Module                          */
             wps_start.ctl_flag |= 0x1;
        }
        if(gWpsCredentials.ssid_len != 0)
        {
          A_MEMCPY(wps_start.ssid_info.ssid,gWpsCredentials.ssid,gWpsCredentials.ssid_len);
          A_MEMCPY(wps_start.ssid_info.macaddress,gWpsCredentials.mac_addr,6);
          wps_start.ssid_info.channel  = gWpsCredentials.ap_channel;
          wps_start.ssid_info.ssid_len = gWpsCredentials.ssid_len;
        }

                                                                /* prevent background scan during WPS                   */
        scan_param_cmd.bg_period = A_CPU2LE16(0xffff);

        error = wmi_cmd_start(            pWmi,
                              (QOSAL_VOID *) &scan_param_cmd,
                                          WMI_SET_SCAN_PARAMS_CMDID,
                                          sizeof(WMI_SCAN_PARAMS_CMD));
        if (error != A_OK) {
            goto exit;
        }
    } else {
        wps_start.ctl_flag |= 0x1;
    }

    error = wmi_cmd_start(            pWmi,
                          (QOSAL_VOID *) &wps_start,
                                      WMI_WPS_START_CMDID,
                                      sizeof(WMI_WPS_START_CMD));
    if (error != A_OK) {
        goto exit;
    }

    pDCxt->wpsState        = QOSAL_TRUE;
    pDCxt->wps_in_progress = QOSAL_TRUE;

exit:
    return error;
}

A_STATUS qcom_wps_connect(QOSAL_UINT8 device_id)
{
    qcom_set_ssid(device_id, (char *) gWpsCredentials.ssid);

    if (gWpsCredentials.key_idx != 0) {
        qcom_sec_set_wepkey(         device_id,
                                     gWpsCredentials.key_idx,
                            (char *) gWpsCredentials.key);
        qcom_sec_set_wepkey_index(device_id, gWpsCredentials.key_idx);
    } else if (gWpsCredentials.auth_type != WLAN_AUTH_NONE) {
        qcom_sec_set_auth_mode(device_id, gWpsCredentials.auth_type);
        qcom_sec_set_encrypt_mode(device_id, gWpsCredentials.encr_type);
    }

    return qcom_commit(device_id);
}

A_STATUS qcom_wps_set_credentials(QOSAL_UINT8 device_id, qcom_wps_credentials_t *pwps_prof)
{
                                                                /* save wps credentials.                                 */
    A_MEMZERO(&gWpsCredentials, sizeof(qcom_wps_credentials_t));
    if (pwps_prof != NULL) {
        A_MEMCPY(&gWpsCredentials, pwps_prof, sizeof(qcom_wps_credentials_t));
    }
    return A_OK;
}

#if ENABLE_P2P_MODE

A_STATUS qcom_p2p_enable(QOSAL_UINT8 device_id, P2P_DEV_CTXT *dev, QOSAL_INT32 enable)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_SET_PROFILE_CMD p2p;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&p2p,sizeof(WMI_P2P_SET_PROFILE_CMD));
    p2p.enable = enable;

    param.cmd_id = ATH_P2P_SWITCH;
    param.data = &p2p;
    param.length = sizeof(p2p);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

static A_STATUS qcom_p2p_group_init(QOSAL_UINT8 device_id, QOSAL_UINT8 persistent_group, QOSAL_UINT8 group_formation)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_GRP_INIT_CMD grpInit;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&grpInit ,sizeof(WMI_P2P_GRP_INIT_CMD));
    grpInit.group_formation = group_formation;
    grpInit.persistent_group = persistent_group;

    param.cmd_id = ATH_P2P_APMODE;
    param.data = &grpInit;
    param.length = sizeof(grpInit);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

static A_STATUS qcom_set_ap_params(QOSAL_UINT8 device_id, QOSAL_UINT16 param_cmd, QOSAL_UINT8 *data_val)
{
    ATH_IOCTL_PARAM_STRUCT inout_param;
    ATH_AP_PARAM_STRUCT ap_param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    inout_param.cmd_id = ATH_CONFIG_AP;
    ap_param.cmd_subset = param_cmd;
    ap_param.data = data_val;
    inout_param.data = &ap_param;

    if(ath_ioctl_handler(QCA_CtxPtr,&inout_param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_start_go(QOSAL_UINT8 device_id, QOSAL_UINT8 *pssid, QOSAL_UINT8 *ppass, QOSAL_INT32 channel, QOSAL_BOOL ucpersistent)
{
    A_STATUS error = A_OK;
    QOSAL_UINT8 wps_flag;

    qcom_sec_set_auth_mode(device_id, WLAN_AUTH_WPA2_PSK);
    qcom_sec_set_encrypt_mode(device_id, WLAN_CRYPT_AES_CRYPT);
    if( pssid != NULL )
    {
        qcom_p2p_func_set_pass_ssid(device_id, ppass, pssid);
    }

    qcom_op_set_mode (device_id,QCOM_WLAN_DEV_MODE_AP);

    if( channel != P2P_AUTO_CHANNEL )
    {
        qcom_set_channel(device_id, channel);
    }

    wps_flag = 0x01;
    if (qcom_set_ap_params(device_id, AP_SUB_CMD_WPS_FLAG, &wps_flag) != A_OK)
    {
        return A_ERROR;
    }

    qcom_op_set_mode (device_id,QCOM_WLAN_DEV_MODE_AP);

    if(qcom_p2p_group_init(device_id, ucpersistent, 1) != A_OK)
    {
        return A_ERROR;
    }

    /* Set MAX PERF */
    qcom_power_set_mode(device_id, MAX_PERF_POWER, P2P);
    return error;
}

A_STATUS qcom_p2p_func_init(QOSAL_UINT8 device_id, QOSAL_INT32 enable)
{
    static QOSAL_INT32 p2p_enable_flag = 0xFEFE;

    if (enable == p2p_enable_flag) {
        return A_OK;
    }
    p2p_enable_flag = enable;

    qcom_p2p_enable(device_id, NULL, enable);
    qcom_p2p_func_set_config(device_id, 0, 1, 1, 3000, 81, 81, 5);

    return A_OK;
}

A_STATUS qcom_p2p_func_find(QOSAL_UINT8 device_id, void *dev, QOSAL_UINT8 type, QOSAL_UINT32 timeout)
{
    WMI_P2P_FIND_CMD find_params;
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    find_params.type = type;
    find_params.timeout = A_CPU2LE32(timeout);
    param.cmd_id = ATH_P2P_FIND;
    param.data = &find_params;
    param.length = sizeof(WMI_P2P_FIND_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_set_pass_ssid(QOSAL_UINT8 device_id, A_CHAR *ppass, A_CHAR *pssid)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_SET_PASSPHRASE_CMD setPassPhrase;

    if( qcom_set_ssid(device_id, pssid) != A_OK )
    return A_ERROR;

    A_STRCPY((char *)setPassPhrase.passphrase, ppass);
    setPassPhrase.passphrase_len = A_STRLEN(ppass);
    A_STRCPY((char *)setPassPhrase.ssid, pssid);
    setPassPhrase.ssid_len = A_STRLEN(pssid);

    param.cmd_id = ATH_P2P_APMODE_PP;
    param.data = &setPassPhrase;
    param.length = sizeof(WMI_SET_PASSPHRASE_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }

    return error;
}

A_STATUS qcom_p2p_func_listen(QOSAL_UINT8 device_id, QOSAL_UINT32 timeout)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    QOSAL_UINT32 tout;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    tout = timeout;
    param.cmd_id = ATH_P2P_LISTEN;
    param.data = &tout;
    param.length = sizeof(tout);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_connect(QOSAL_UINT8 device_id, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *peer_mac, QOSAL_BOOL persistent)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_FW_CONNECT_CMD_STRUCT p2p_connect;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    A_MEMZERO(&p2p_connect, sizeof( WMI_P2P_FW_CONNECT_CMD_STRUCT));
    p2p_connect.wps_method = wps_method;
    A_MEMCPY(p2p_connect.peer_addr, peer_mac, ATH_MAC_LEN);

    /* Setting Default Value for now !!! */
    p2p_connect.dialog_token = 1;
    p2p_connect.go_intent = 0;
    p2p_connect.go_dev_dialog_token = 0;
    p2p_connect.dev_capab = 0x23;
    if(persistent)
    {
        p2p_connect.dev_capab |= P2P_PERSISTENT_FLAG;
    }

    param.cmd_id = ATH_P2P_CONNECT_CLIENT;
    param.data = &p2p_connect;
    param.length = sizeof(WMI_P2P_FW_CONNECT_CMD_STRUCT);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_auth(QOSAL_UINT8 device_id, QOSAL_INT32 dev_auth, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *peer_mac, QOSAL_BOOL persistent)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_FW_CONNECT_CMD_STRUCT p2p_connect;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&p2p_connect, sizeof( WMI_P2P_FW_CONNECT_CMD_STRUCT));

    if( persistent)
    p2p_connect.dev_capab |= P2P_PERSISTENT_FLAG;
    p2p_connect.dev_auth = dev_auth;
    p2p_connect.wps_method = wps_method;
    A_MEMCPY(p2p_connect.peer_addr, peer_mac, ATH_MAC_LEN);
    /* If go_intent <= 0, wlan firmware will use the intent value configured via
     * qcom_p2p_set
     */
    p2p_connect.go_intent = 0;

    param.cmd_id = ATH_P2P_AUTH;
    param.data = &p2p_connect;
    param.length = sizeof(WMI_P2P_FW_CONNECT_CMD_STRUCT);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_cancel(QOSAL_UINT8 device_id)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    param.cmd_id = ATH_P2P_CANCEL;
    param.data = NULL;
    param.length = 0;
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_stop_find(QOSAL_UINT8 device_id)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    param.cmd_id = ATH_P2P_STOP;
    param.data = NULL;
    param.length = 0;
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

/* NODE_LIST */
A_STATUS qcom_p2p_func_get_node_list(QOSAL_UINT8 device_id, void *app_buf, QOSAL_UINT32 buf_size)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    param.cmd_id = ATH_P2P_NODE_LIST;
    param.data = app_buf;
    param.length = buf_size;
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_set_config(QOSAL_UINT8 device_id, QOSAL_UINT32 uigo_intent, QOSAL_UINT32 uiclisten_ch, QOSAL_UINT32 uiop_ch, QOSAL_UINT32 uiage, QOSAL_UINT32 reg_class, QOSAL_UINT32 op_reg_class, QOSAL_UINT32 max_node_count)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_FW_SET_CONFIG_CMD stp2p_cfg_cmd;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&stp2p_cfg_cmd, sizeof(stp2p_cfg_cmd));
    stp2p_cfg_cmd.go_intent = uigo_intent;
    stp2p_cfg_cmd.reg_class = reg_class;
    stp2p_cfg_cmd.op_reg_class = op_reg_class;
    stp2p_cfg_cmd.op_channel = uiop_ch;
    stp2p_cfg_cmd.listen_channel = uiclisten_ch;
    stp2p_cfg_cmd.node_age_to = uiage;
    stp2p_cfg_cmd.max_node_count = max_node_count;

    param.cmd_id = ATH_P2P_SET_CONFIG;
    param.data = &stp2p_cfg_cmd;
    param.length = sizeof(WMI_P2P_FW_SET_CONFIG_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_wps_config(QOSAL_UINT8 device_id, QOSAL_UINT8 *p2p_ptr)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    param.cmd_id = ATH_P2P_WPS_CONFIG;
    param.data = p2p_ptr;
    param.length = sizeof(WMI_WPS_SET_CONFIG_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_disc_req(QOSAL_UINT8 device_id, QOSAL_UINT8 *p2p_ptr)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    param.cmd_id = ATH_P2P_DISC_REQ;
    param.data = p2p_ptr;
    param.length = sizeof(WMI_P2P_FW_PROV_DISC_REQ_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_set(QOSAL_UINT8 device_id, P2P_CONF_ID config_id, void *data, QOSAL_UINT32 data_length)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_SET_CMD p2p_set_params;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&p2p_set_params, sizeof(p2p_set_params));

    switch (config_id) {
        case P2P_CONFIG_LISTEN_CHANNEL:
        config_id = WMI_P2P_CONFID_LISTEN_CHANNEL;
        break;
        case P2P_CONFIG_CROSS_CONNECT:
        config_id = WMI_P2P_CONFID_CROSS_CONNECT;
        break;
        case P2P_CONFIG_SSID_POSTFIX:
        config_id = WMI_P2P_CONFID_SSID_POSTFIX;
        break;
        case P2P_CONFIG_INTRA_BSS:
        config_id = WMI_P2P_CONFID_INTRA_BSS;
        break;
        case P2P_CONFIG_CONCURRENT_MODE:
        config_id = WMI_P2P_CONFID_CONCURRENT_MODE;
        break;
        case P2P_CONFIG_GO_INTENT:
        config_id = WMI_P2P_CONFID_GO_INTENT;
        break;
        case P2P_CONFIG_DEV_NAME:
        config_id = WMI_P2P_CONFID_DEV_NAME;
        break;
        case P2P_CONFIG_P2P_OPMODE:
        config_id = WMI_P2P_CONFID_P2P_OPMODE;
        break;
        case P2P_CONFIG_CCK_RATES:
        config_id = WMI_P2P_CONFID_CCK_RATES;
        break;
        default:
        /* Unknown parameter */
        return A_ERROR;
    }

    p2p_set_params.config_id = config_id;
    A_MEMCPY(&p2p_set_params.val, data, data_length);

    param.cmd_id = ATH_P2P_SET;
    param.data = &p2p_set_params;
    param.length = sizeof(WMI_P2P_SET_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_join(QOSAL_UINT8 device_id, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *pmac, char *ppin, QOSAL_UINT16 channel)
{
    QOSAL_UINT16 conc_channel;
    WMI_P2P_FW_CONNECT_CMD_STRUCT p2p_join_profile;
    WMI_P2P_PROV_INFO p2p_info;
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

#if ENABLE_SCC_MODE
    int num_dev = WLAN_NUM_OF_DEVICES;
    if( (num_dev > 1) && (device_id == 0))
    {
        qcom_get_concurrent_channel(device_id, &conc_channel);
        if( (conc_channel != 0)&& (channel == 0)) {
            return A_ERROR;
        }
        if( (conc_channel != 0) && (channel != conc_channel ) )
        {
            return A_ERROR;
        }
    }
#endif /* ENABLE_SCC_MODE */    

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    A_MEMZERO(&p2p_join_profile,sizeof( WMI_P2P_FW_CONNECT_CMD_STRUCT));
    A_MEMCPY(p2p_join_profile.peer_addr, pmac, ATH_MAC_LEN);
    p2p_join_profile.go_oper_freq = channel;
    p2p_join_profile.wps_method = wps_method;

    if( wps_method != WPS_PBC ) {
        A_STRCPY(p2p_info.wps_pin, ppin);
        wmi_save_key_info(&p2p_info);
    }

    param.cmd_id = ATH_P2P_JOIN;
    param.data = &p2p_join_profile;
    param.length = sizeof(WMI_P2P_FW_CONNECT_CMD_STRUCT);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_invite_auth(QOSAL_UINT8 device_id, QOSAL_UINT8 *inv)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    param.cmd_id = ATH_P2P_INVITE_AUTH;
    param.data = inv;
    param.length = sizeof(WMI_P2P_FW_INVITE_REQ_RSP_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

/* LIST_NETWORK */
A_STATUS qcom_p2p_func_get_network_list(QOSAL_UINT8 device_id, void *app_buf, QOSAL_UINT32 buf_size)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    param.cmd_id = ATH_P2P_PERSISTENT_LIST;
    param.data = app_buf;
    param.length = buf_size;
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_invite(QOSAL_UINT8 device_id, A_CHAR *pssid, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *pmac, QOSAL_BOOL persistent, P2P_INV_ROLE role)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_INVITE_CMD p2pInvite;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }

    A_MEMZERO(&p2pInvite, sizeof(p2pInvite));

    A_MEMCPY(p2pInvite.ssid.ssid, pssid, A_STRLEN(pssid));
    p2pInvite.ssid.ssidLength = A_STRLEN(pssid);
    A_MEMCPY(p2pInvite.peer_addr, pmac, ATH_MAC_LEN);
    p2pInvite.wps_method = wps_method;
    p2pInvite.is_persistent = persistent;
    p2pInvite.role = role;

    param.cmd_id = ATH_P2P_INVITE;
    param.data = &p2pInvite;
    param.length = sizeof(WMI_P2P_INVITE_CMD);

    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_join_profile(QOSAL_UINT8 device_id, QOSAL_UINT8 *pmac)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_FW_CONNECT_CMD_STRUCT p2p_connect;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&p2p_connect,sizeof(p2p_connect));
    A_MEMCPY(p2p_connect.peer_addr, pmac, ATH_MAC_LEN);

    param.cmd_id = ATH_P2P_JOIN_PROFILE;
    param.data = &p2p_connect;
    param.length = sizeof(WMI_P2P_FW_CONNECT_CMD_STRUCT);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_ap_mode(QOSAL_UINT8 device_id, QOSAL_UINT8 *p2p_ptr)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    param.cmd_id = ATH_P2P_APMODE;
    param.data = p2p_ptr;
    param.length = sizeof(WMI_P2P_GRP_INIT_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_on_off(QOSAL_UINT8 device_id, QOSAL_UINT8 *p2p_ptr)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    param.cmd_id = ATH_P2P_SWITCH;
    param.data = p2p_ptr;
    param.length = sizeof(WMI_P2P_SET_PROFILE_CMD);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_set_noa(QOSAL_UINT8 device_id, QOSAL_UINT8 uccount, QOSAL_UINT32 uistart, QOSAL_UINT32 uiduration, QOSAL_UINT32 uiinterval)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    P2P_NOA_INFO p2p_noa_desc;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    if (uccount >= P2P_MAX_NOA_DESCRIPTORS) {
        return A_ERROR;
    }

    A_MEMZERO(&p2p_noa_desc, sizeof(p2p_noa_desc));
    if (uccount > 0) {
        QOSAL_UINT8 i;
        p2p_noa_desc.enable = 1;
        p2p_noa_desc.count = uccount;

        for (i = 0; i < uccount; i++) {
            p2p_noa_desc.noas[i].count_or_type = uccount;
            p2p_noa_desc.noas[i].duration = uiduration;
            p2p_noa_desc.noas[i].interval = uiinterval;
            p2p_noa_desc.noas[i].start_or_offset = uistart;
        }
    }

    param.cmd_id = ATH_P2P_SET_NOA;
    param.data = &p2p_noa_desc;
    param.length = sizeof(p2p_noa_desc);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_set_oppps(QOSAL_UINT8 device_id, QOSAL_UINT8 enable, QOSAL_UINT8 ctwin)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_OPPPS_INFO_STRUCT p2p_oppps;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&p2p_oppps, sizeof(WMI_OPPPS_INFO_STRUCT));
    p2p_oppps.enable = enable;
    p2p_oppps.ctwin = ctwin;

    param.cmd_id = ATH_P2P_SET_OPPPS;
    param.data = &p2p_oppps;
    param.length = sizeof(WMI_OPPPS_INFO_STRUCT);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

A_STATUS qcom_p2p_func_prov(QOSAL_UINT8 device_id, P2P_WPS_METHOD wps_method, QOSAL_UINT8 *pmac)
{
    ATH_IOCTL_PARAM_STRUCT param;
    A_STATUS error = A_OK;
    WMI_P2P_FW_PROV_DISC_REQ_CMD p2p_prov_disc;

    if(qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
    A_MEMZERO(&p2p_prov_disc, sizeof(p2p_prov_disc));
    p2p_prov_disc.dialog_token = 1;
    p2p_prov_disc.wps_method = wps_method;
    A_MEMCPY(p2p_prov_disc.peer, pmac, ATH_MAC_LEN);

    param.cmd_id = ATH_P2P_DISC_REQ;
    param.data = &p2p_prov_disc;
    param.length = sizeof(p2p_prov_disc);
    if(ath_ioctl_handler(QCA_CtxPtr,&param) != A_OK) {
        error = A_ERROR;
    }
    return error;
}

#endif


/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             EXTRA FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

A_STATUS qcom_get_version(QOSAL_UINT32 *host_ver, QOSAL_UINT32 *target_ver, QOSAL_UINT32 *wlan_ver, QOSAL_UINT32 *abi_ver) 
{ 
    QOSAL_VOID                    *pCxt;
    A_DRIVER_CONTEXT          *pDCxt;
    A_STATUS                   error = A_OK;
    
    
    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);


    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }
    
   *host_ver   = pDCxt->hostVersion;
   *target_ver = pDCxt->targetVersion;
   *wlan_ver   = pDCxt->wlanVersion;
   *abi_ver    = pDCxt->abiVersion;
   
exit:
    return error;
}

A_STATUS qcom_ipconfig_set_ip6_status(QOSAL_UINT8  device_id, QOSAL_BOOL enable)
{
    QOSAL_VOID  *pCxt = QCA_CtxPtr;
    QOSAL_INT32  error;


    if (qcom_set_deviceid(device_id) == A_ERROR) {
        return A_ERROR;
    }
                                                                /* Wait for chip to be up                               */
    if (A_OK != Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL)) {
        return A_ERROR;
    }

    error = Api_ipconfig_set_ip6_status(pCxt,(QOSAL_UINT16)enable);

    return (A_STATUS) error;
}
A_STATUS qcom_roaming_ctrl(A_UINT8 device_id, WMI_SET_ROAM_CTRL_CMD * roam_ctrl)
{
    A_VOID              *pCxt;
    A_VOID              *pWmi;
    A_DRIVER_CONTEXT    *pDCxt;
    A_STATUS             error;


    pCxt  = QCA_CtxPtr;
    pDCxt = GET_DRIVER_COMMON(pCxt);
    pWmi  = pDCxt->pWmiCxt;	

    error = qcom_set_deviceid(device_id);
    if (error != A_OK) {
        goto exit;
    }

    if (pDCxt->wmiReady == A_FALSE && ath_custom_init.skipWmi == 0) {
        error = A_ERROR;
        goto exit;
    }

    error = Api_DriverAccessCheck(pCxt, 1, ACCESS_REQUEST_IOCTL);
    if (error != A_OK) {
        goto exit;
    }

    error = wmi_cmd_start(pWmi,(A_VOID*)roam_ctrl, WMI_SET_ROAM_CTRL_CMDID, sizeof(WMI_SET_ROAM_CTRL_CMD))  ;	
    if (error != A_OK) {
        goto exit;
    }	
	
    return A_OK;
exit:
    return error;
}
