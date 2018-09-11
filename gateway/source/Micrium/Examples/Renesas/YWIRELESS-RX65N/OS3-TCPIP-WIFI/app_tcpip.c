/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
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
*                                              uC/TCP-IP
*                                           APPLICATION CODE
*
* Filename      : app_tcpip.c
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <app_tcpip.h>
#include  <Source\net_type.h>
#include  <IF\net_if_wifi.h>
#include  <IP\IPv4\net_ipv4.h>
#include  <net_dev_qca400x.h>
#include  <net_bsp_gt202.h>
#include  <net_dev_cfg.h>
#include  <app_cfg.h>
#include  <os.h>
#include  <bsp_cfg.h>
#include  <bsp_led.h>

#include  <app_dhcp-c.h>
#include  <Source/dhcp-c.h>
#include  <Source/dns-c.h>
#include  "dns-c_cfg.h"


/*
*********************************************************************************************************
*                                               ENABLE
*********************************************************************************************************
*/

#if (APP_CFG_TCPIP_EN == DEF_ENABLED)


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  NET_IF_NBR  AppTCPIP_IF_NbrDflt;


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

static void  AppTCPIP_MacAddrGet (NET_IF_NBR  if_nbr,
                                  CPU_CHAR   *mac_addr);

static  void  AppPrintIPAddr     (NET_IF_NBR  if_nbr);


extern char * g_prov_ssid;
extern char * g_prov_ssid_password;

/*
*********************************************************************************************************
*                                            AppTCPIP_Init()
*
* Description : Initialize uC/TCP-IP.
*
* Arguments   : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  AppTCPIP_Init (void)
{
    NET_IF_NBR         if_nbr_wifi;
    NET_IF_WIFI_SSID  *p_ssid;
    NET_IF_WIFI_SSID   ssid;
    NET_IF_WIFI_PSK    psk;
    CPU_INT08U         retry;
    CPU_INT16U         ctn;
    CPU_INT16U         i;
    CPU_INT16S         result;
    CPU_BOOLEAN        found;
    NET_IPv4_ADDR      ip;
    NET_IPv4_ADDR      msk;
    NET_IPv4_ADDR      gateway;
    NET_ERR            net_err;
    static NET_IF_WIFI_AP     ap[APP_CFG_WIFI_AP_TBL_SIZE];
    CPU_BOOLEAN        dhcp_status;
    DNSc_ERR           dns_err;

                                                                /* --------------------- INIT TCPIP ------------------- */
    APP_TRACE_INFO(("\r\n"));
    APP_TRACE_INFO(("===================================================================\r\n"));
    APP_TRACE_INFO(("=                       TCPIP INITIALIZATION                      =\r\n"));
    APP_TRACE_INFO(("===================================================================\r\n"));
    APP_TRACE_INFO(("Initializing TCPIP...\r\n"));

    net_err = Net_Init(&NetRxTaskCfg,                           /* Initialize uC/TCP-IP.                                */
                       &NetTxDeallocTaskCfg,
                       &NetTmrTaskCfg);
                                                               /* ---------------- WIFI CONFIGURATION ---------------- */
    APP_TRACE_INFO(("\r\n"));
    APP_TRACE_INFO(("Initializing WIFI configurations...\r\n"));

                                                                /* ------------------- ADD IF NBR 1 ------------------- */
    if_nbr_wifi = NetIF_Add((void    *)&NetIF_API_WiFi,         /* WiFi API                                             */
                            (void    *)&NetDev_API_QCA400X,     /* Device API structure                                 */
#if BSP_CFG_GT202_ON_BOARD > 0
                            (void    *)&NetDev_BSP_GT202_OnBoard_SPI,
#elif BSP_CFG_GT202_PMOD1 > 0
                            (void    *)&NetDev_BSP_GT202_PMOD1_SPI,
#elif BSP_CFG_GT202_PMOD2 > 0
                            (void    *)&NetDev_BSP_GT202_PMOD2_SPI,
#endif
                            (void    *)&NetDev_Cfg_B_1,         /* Device Configuration structure                       */
                            (void    *)&NetWiFiMgr_API_Generic, /* PHY API structure                                    */
                            (void    *) DEF_NULL,               /* PHY Configuration structure                          */
                            (NET_ERR *)&net_err);               /* Return error variable                                */

    BSP_LED_On(BSP_LED_3_RED);
    
                                                                /* ------------------ START IF NBR 1 ------------------ */
    NetIF_Start(if_nbr_wifi, &net_err);

    if (net_err != NET_IF_ERR_NONE) {
        return (DEF_FAIL);
    }

                                                                /* ------------ SCAN FOR WIRELESS NETWORKS ------------ */
    APP_TRACE_INFO(("Searching for specific wireless network SSID...\r\n"));
    retry = 10u;
    do {
        Mem_Clr(&ap, sizeof(ap));
        ctn = NetIF_WiFi_Scan(if_nbr_wifi,                      /* Scan Availabe Wireless Networks.                     */
                              ap,                               /* Access point table location.                         */
                              APP_CFG_WIFI_AP_TBL_SIZE,         /* Access point table size.                             */
                              DEF_NULL,                         /* No Hidden SSID.                                      */
                              NET_IF_WIFI_CH_ALL,               /* Channel to scan: All.                                */
                             &net_err);

                                                                /* --------- ANALYSE WIRELESS NETWORKS FOUND ---------- */
        found = DEF_NO;
        if (ctn) {
            for (i = 0u; i <= ctn - 1u; i++) {                      /* Browse table of access point found.                  */
                p_ssid = &ap[i].SSID;
                result = Str_Cmp_N((CPU_CHAR *)p_ssid,              /* Search for a specific Wireless Network SSID.         */
                                               g_prov_ssid,/* Set WiFi Network SSID.                              */
                                               NET_IF_WIFI_STR_LEN_MAX_SSID);
                if (result == 0u) {
                    found = DEF_YES;
                    break;
                }
            }
        }

        retry--;
    } while ((retry != 0u) && (found == DEF_NO));

    if (found == DEF_NO) {
        APP_TRACE_INFO(("Wireless Network SSID Not Found: %s \r\n", g_prov_ssid));
        return (DEF_FAIL);
    }

    BSP_LED_On(BSP_LED_2_YELLOW);
                                                                /* ------------- JOIN A WIRELESS NETWORK -------------- */
    Mem_Clr(&ssid, sizeof(ssid));
    Mem_Clr(&psk,  sizeof(psk));
    Str_Copy_N((CPU_CHAR *)&ssid,
                           g_prov_ssid,               /* Set WiFi Network SSID.                               */
                           Str_Len(g_prov_ssid));      /* SSID string length.                                  */
    Str_Copy_N((CPU_CHAR *)&psk,
                           g_prov_ssid_password,                /* Set WiFi Network Password.                           */
                           Str_Len(g_prov_ssid_password));       /* PSK string length.                                   */

    retry = 20;
    do {

        NetIF_WiFi_Leave(if_nbr_wifi,&net_err);

        OSTimeDlyHMSM((CPU_INT16U) 0,
                      (CPU_INT16U) 0,
                      (CPU_INT16U) 0,
                      (CPU_INT32U) 500,
                      (OS_OPT    ) OS_OPT_TIME_HMSM_STRICT,
                      (OS_ERR   *)&net_err);

        NetIF_WiFi_Join(if_nbr_wifi,
                        NET_IF_WIFI_NET_TYPE_INFRASTRUCTURE,
                        NET_IF_WIFI_DATA_RATE_AUTO,
                        NET_IF_WIFI_SECURITY_WPA2,
                        NET_IF_WIFI_PWR_LEVEL_HI,
                        ssid,
                        psk,
                        &net_err);

        retry--;
    } while ((retry != 0u) && (net_err != NET_IF_WIFI_ERR_NONE));
                                                                /*   */
    NetIF_LinkStateWaitUntilUp(if_nbr_wifi, 20, 200, &net_err);
    if (net_err != NET_IF_ERR_NONE) {
        APP_TRACE_INFO(("NetIF_LinkStateWaitUntilUp() failed w/err = %d \r\n", net_err));
        return (DEF_FAIL);
    }

    BSP_LED_Off(BSP_LED_3_RED);
                                                                /* Start DHCPc  */


                                                                /* -------------- BEGIN DHCP IP CONFIG ---------------- */
    dhcp_status = AppDHCPc_Init(if_nbr_wifi);
    if (dhcp_status != DEF_OK) {
        APP_TRACE_INFO(("DHCPc setup failed, static IP used"));
    }

    AppPrintIPAddr(if_nbr_wifi);

    DNSc_Init(&DNSc_Cfg, &DNSc_CfgTask, &dns_err);                   /* --------------- INITIALIZE uC/DNSc ----------------- */
    if (dns_err != DNSc_ERR_NONE) {
        return (DEF_FAIL);
    }

    AppTCPIP_IF_NbrDflt = if_nbr_wifi;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         AppNet_MacAddrDfltGet()
*
* Description : Get the mac address for the default network interface
*
* Arguments   : mac_addr    Pointer to string to store the MAC address in.
*
* Returns     : none.
*
* Notes       : none.
*********************************************************************************************************
*/

void  AppTCPIP_MacAddrDfltGet (CPU_CHAR *mac_addr)
{
    AppTCPIP_MacAddrGet(AppTCPIP_IF_NbrDflt, mac_addr);
}


/*
*********************************************************************************************************
*                                         AppNet_MacAddrGet()
*
* Description : Gets the MAC address for the specified network interface.
*
* Arguments   : if_nbr      The network interface to get the MAC address for.
*
*               mac_addr    Pointer to string to store the MAC address in.
*
* Returns     : The MAC address.
*
* Notes       : none.
*********************************************************************************************************
*/

static void  AppTCPIP_MacAddrGet (NET_IF_NBR  if_nbr,
                                  CPU_CHAR   *mac_addr)
{
    CPU_INT08U  i, mac_addr_len;
    CPU_CHAR    tmp_mac_addr[NET_ASCII_LEN_MAX_ADDR_MAC];
    CPU_INT08U  addr_hw[NET_IF_802x_ADDR_SIZE];
    CPU_INT08U  addr_hw_len = NET_IF_802x_ADDR_SIZE;
    NET_ERR     net_err;


    NetIF_AddrHW_Get(if_nbr,                                    /* Get the MAC address for the specified interface      */
                     &addr_hw[0],
                     &addr_hw_len,
                     &net_err);
                                                                /* Convert the MAC address to a string                  */
    NetASCII_MAC_to_Str(addr_hw, tmp_mac_addr, DEF_YES, DEF_NO, &net_err);

    mac_addr_len = Str_Len((char const*) tmp_mac_addr);         /* Clean up the MAC address by removing the '-'         */
    for(i = 0; i < mac_addr_len; i++) {
        if(tmp_mac_addr[i] != '-') {
            *mac_addr++ = tmp_mac_addr[i];
        }
    }
    *mac_addr = '\0';
}


/*
*********************************************************************************************************
*                                       AppPrintIPAddr()
*
* Description : Print IP address
*
* Argument(s) : if_nbr       Interface number.
*
* Return(s)   : none.
*
* Caller(s)   : none.
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  AppPrintIPAddr (NET_IF_NBR  if_nbr)
{
    NET_IPv4_ADDR     ip_addr_tbl[NET_IPv4_CFG_IF_MAX_NBR_ADDR];
    NET_IP_ADDRS_QTY  ip_addr_tbl_qty;
    NET_IPv4_ADDR     subnet_addr;
    NET_IPv4_ADDR     gateway_addr;
    CPU_CHAR          str_addr[40u];
    CPU_SIZE_T        str_len;
    NET_ERR           net_err;


    ip_addr_tbl_qty = NET_IPv4_CFG_IF_MAX_NBR_ADDR;
   (void)NetIPv4_GetAddrHost( if_nbr,
                             &ip_addr_tbl[0],
                             &ip_addr_tbl_qty,
                             &net_err);

    if (net_err == NET_IPv4_ERR_NONE) {
        subnet_addr  = NetIPv4_GetAddrSubnetMask ( ip_addr_tbl[0],
                                                  &net_err);
        gateway_addr = NetIPv4_GetAddrDfltGateway( ip_addr_tbl[0],
                                                  &net_err);

#if BSP_CFG_GRAPH_LCD_EN > 0u
        BSP_GraphLCD_ClrLine(2);
        BSP_GraphLCD_ClrLine(3);
        BSP_GraphLCD_ClrLine(6);
        BSP_GraphLCD_ClrLine(7);

        BSP_GraphLCD_String(2, "  Up and Running!");
#endif

        str_addr[0] = ASCII_CHAR_NULL;
        Str_Copy(str_addr, "IP: ");
        str_len = Str_Len(&str_addr[0]);
        NetASCII_IPv4_to_Str( ip_addr_tbl[0],
                             &str_addr[str_len],
                              DEF_NO,
                             &net_err);
        APP_TRACE_INFO((str_addr));
        APP_TRACE_INFO(("\n\r"));

#if BSP_CFG_GRAPH_LCD_EN > 0u
        BSP_GraphLCD_String(3, str_addr);
#endif

        str_addr[0] = ASCII_CHAR_NULL;
        Str_Copy(str_addr, "Sub: ");
        str_len = Str_Len(&str_addr[0]);
        NetASCII_IPv4_to_Str( subnet_addr,
                             &str_addr[str_len],
                              DEF_NO,
                             &net_err);
        APP_TRACE_INFO((str_addr));
        APP_TRACE_INFO(("\n\r"));

#if BSP_CFG_GRAPH_LCD_EN > 0u
        BSP_GraphLCD_String(6, str_addr);
#endif

        str_addr[0] = ASCII_CHAR_NULL;
        Str_Copy(str_addr, "GW: ");
        str_len = Str_Len(&str_addr[0]);
        NetASCII_IPv4_to_Str( gateway_addr,
                             &str_addr[str_len],
                              DEF_NO,
                             &net_err);
        APP_TRACE_INFO((str_addr));
        APP_TRACE_INFO(("\n\r"));

#if BSP_CFG_GRAPH_LCD_EN > 0u
        BSP_GraphLCD_String(7, str_addr);
#endif
    }
}


/*
*********************************************************************************************************
*                                             ENABLE END
*********************************************************************************************************
*/

#endif
