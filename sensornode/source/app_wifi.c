/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2016; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                            RENESAS RX111
*                                               on the
*                                    YWireless-RX111 Wireless Demo Kit
*
* Filename      : app_wifi.c
* Version       : V1.00
* Programmer(s) : MTM
*********************************************************************************************************
*/


/*
 ********************************************************************************************************
 *                                             INCLUDE FILES                                            *
 ********************************************************************************************************
 */

#include  <os.h>
#include  <qcom_api.h>
#include  <qca.h>
#include  <qca_bsp.h>
#include  <bsp_led.h>
#include  <bsp_sys.h>

#include  "app_wifi.h"
#include  "app_wifi_util.h"


/*
 ********************************************************************************************************
 *                                                DEFINES                                               *
 ********************************************************************************************************
 */

#define  WIFI_TASK_PRIO                         10u
#define  WIFI_TASK_STK_SIZE                     512u

#define  WIFI_FLAG_CONNECT_SUCCESS              DEF_BIT_00
#define  WIFI_FLAG_RSNA_AUTH_SUCCESS            DEF_BIT_01
#define  WIFI_FLAG_CONN_UP                      DEF_BIT_02
#define  WIFI_FLAG_QCA_CRASH                    DEF_BIT_03
#define  CONN_UP_WAIT_TIMEOUT                   (OSCfg_TickRate_Hz * 30)    /* 30 seconds */


/*
 ********************************************************************************************************
 *                                               CONSTANTS                                              *
 ********************************************************************************************************
 */


/*
 ********************************************************************************************************
 *                                               DATA TYPES                                             *
 ********************************************************************************************************
 */

/*
 ********************************************************************************************************
 *                                          FUNCTION PROTOTYPES                                         *
 ********************************************************************************************************
 */

/*
 ********************************************************************************************************
 *                                            GLOBAL VARIABLES                                          *
 ********************************************************************************************************
 */

static  OS_TCB       Wifi_TaskTCB;
static  CPU_STK      Wifi_TaskStk[WIFI_TASK_STK_SIZE];

static  OS_FLAG_GRP  WifiFlagGrp;


/*
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS                                           *
 ********************************************************************************************************
 */

static  void  Wifi_Task (void  *p_arg);

static  void  WiFi_OnConnectCallback (int       value,
                                      A_UINT8   devId,
                                      A_UINT8  *bssid,
                                      A_BOOL    bssConn);


/*
 ********************************************************************************************************
 *                                            LOCAL FUNCTIONS                                           *
 ********************************************************************************************************
 */

/*
*********************************************************************************************************
*                                            App_WiFi_Init()
*
* Description : Initialize the WiFi task
*
* Arguments   : SHG_Cfg : Pointer to the SHG config structure.
*
* Returns     : DEF_OK if initialized ok, DEF_FAIL if a component failed.
*
* Notes       : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  App_WiFi_Init(SHG_CFG *SHG_Cfg)
{
    OS_ERR err_os;
    CPU_BOOLEAN ret_val = DEF_FAIL;


    do {
        OSFlagCreate(&WifiFlagGrp,                              /* Create WiFi event flags group                        */
                     "WiFi Connection Flags",
                      0u,
                     &err_os);
        if(err_os != OS_ERR_NONE) {
            break;
        }

        OSTaskCreate(&Wifi_TaskTCB,                             /* Create the WiFi task                                 */
                     "Wifi Monitor",
                      Wifi_Task,
                      SHG_Cfg,
                      WIFI_TASK_PRIO,
                     &Wifi_TaskStk[0],
                      WIFI_TASK_STK_SIZE / 10,
                      WIFI_TASK_STK_SIZE,
                      0, 0, 0,
                      OS_OPT_TASK_STK_CHK,
                     &err_os);
        if(err_os != OS_ERR_NONE) {
            break;
        }

        ret_val = DEF_OK;                                       /* Everything initialized OK, return DEF_OK             */
    } while(0);

    return ret_val;
}


/*
*********************************************************************************************************
*                                        App_WiFi_IsConnected()
*
* Description : Wait for the WiFi connection up flag to post.
*
* Arguments   : none.
*
* Returns     : DEF_TRUE when connected, DEF_FALSE if an error occurred.
*
* Notes       : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  App_WiFi_IsConnected()
{
    OS_ERR       err_os = OS_ERR_NONE;
    CPU_BOOLEAN  ret_val = DEF_FALSE;


    OSFlagPend(&WifiFlagGrp,                                /* Wait for the connection to go up                     */
                WIFI_FLAG_CONN_UP,
                0,
                OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_NON_BLOCKING,
                DEF_NULL,
               &err_os);

    if(err_os == OS_ERR_NONE) {                             /* Check the error code                                 */
        ret_val = DEF_TRUE;
    } else {
        ret_val = DEF_FALSE;
    }

    return ret_val;
}


/*
*********************************************************************************************************
*                                          WiFi_Task()
*
* Description : Task responsible for maintaining a WiFi connection.
*
* Arguments   : p_arg   : Argument pointer passed in from OSTaskCreate()
*
* Returns     : Should NEVER return.
*
* Notes       : none.
*
*********************************************************************************************************
*/

static  void  Wifi_Task (void  *p_arg)
{
    SHG_CFG  const  *p_shgcfg;
    OS_ERR           err_os;
    A_STATUS         status;
    BSP_LED          status_led;
    CPU_BOOLEAN      wait_to_retry;
    A_UINT32         ip;
    A_UINT32         sn;
    A_UINT32         gtw;
    int              i;
    OS_FLAGS         wifi_flags;


    p_shgcfg = (SHG_CFG  const  *)p_arg;
                                                                /* Set the connect callback                             */
    status = qcom_set_connect_callback(0u, (void*)WiFi_OnConnectCallback);
    ASSERT(status == A_OK);
                                                                /* Power mode Function                                  */
    status = qcom_power_set_mode(0u, MAX_PERF_POWER, PWR_MAX);
    ASSERT(status == A_OK);

    status = qcom_op_set_mode(0u, QCOM_WLAN_DEV_MODE_STATION);
    ASSERT(status == A_OK);

    status_led = BSP_LED_ORANGE;                                /* Illuminate orange LED during first initialization.   */

                                                                /* Main task loop.                                      */
    wait_to_retry = DEF_NO;
    while (DEF_ON) {
                                                                /* Illuminate the status LED while initializing.        */
        BSP_LED_Off(BSP_LED_ALL);
        BSP_LED_On(status_led);

        if (wait_to_retry == DEF_YES) {
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_NONE, &err_os);
            wait_to_retry = DEF_YES;                            /* Always wait to retry from now on.                    */
        }

                                                                /* Got configs, now apply them.                         */
        status = qcom_set_ssid(0u, (char *)p_shgcfg->SSID_Str);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }
        if (p_shgcfg->CryptType == WLAN_CRYPT_WEP_CRYPT)
            status = qcom_sec_set_wepkey(0, 1, (char *)p_shgcfg->Passphrase);
        else
            status = qcom_sec_set_passphrase(0u, (char *)p_shgcfg->Passphrase);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }
        status = qcom_sec_set_auth_mode(0, p_shgcfg->AuthMode);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }
        status = qcom_sec_set_encrypt_mode(0, p_shgcfg->CryptType);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }

        status = qcom_commit(0u);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }

                                                                /* Wait until WiFi connection is UP (with timeout).     */
        wifi_flags = WIFI_FLAG_CONNECT_SUCCESS;
        if (p_shgcfg->AuthMode != WLAN_AUTH_NONE) {
            wifi_flags |= WIFI_FLAG_RSNA_AUTH_SUCCESS;
        }

        OSFlagPend(&WifiFlagGrp, wifi_flags, CONN_UP_WAIT_TIMEOUT, OS_OPT_PEND_FLAG_SET_ALL, DEF_NULL, &err_os);
        if (err_os != OS_ERR_NONE) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }

                                                                /* Enable SNTP client                                   */
        qcom_enable_sntp_client(1);
        qcom_sntp_zone(4,0,0,1);
        qcom_sntp_srvr_addr(0,"206.108.0.133");//ip_sntp_svr);

        if (p_shgcfg->UseDHCP == DEF_YES) {                     /* Get IP address via DHCP.                             */
            APP_TRACE_INFO(("Obtaining IPv4 address via DHCP... "));

            status = qcom_ipconfig(0, IPCFG_DHCP, &ip , &sn, &gtw);
            if (status != A_OK) {
                status_led = BSP_LED_RED;
                wait_to_retry = DEF_YES;
                continue;
            }
            status = qcom_ipconfig(0, IPCFG_QUERY, &ip , &sn, &gtw);
            if (status == A_OK) {
                                                                /* Get each octet of the IP address.                    */
                APP_TRACE_INFO(("OK. IP: %d.%d.%d.%d\n", (A_UINT8)(ip >> 24),
                                                         (A_UINT8)(ip >> 16),
                                                         (A_UINT8)(ip >>  8),
                                                         (A_UINT8)(ip >>  0)));
            } else {
                APP_TRACE_INFO(("FAILED.\n"));
                status_led = BSP_LED_RED;
                wait_to_retry = DEF_YES;
                continue;
            }

            i = 0;                                              /* Add any DNS servers listed in configs.               */
            while (i < NBR_DNS_SERVERS && p_shgcfg->DNS_Servers[i] != 0) {
                qcom_dnsc_add_server_address((A_UINT8 *)&p_shgcfg->DNS_Servers[i], ATH_AF_INET);
                ++i;
            }

        } else {                                                /* Use static IP address.                               */
            ip  = p_shgcfg->IP_Addr;
            sn  = p_shgcfg->Netmask;
            gtw = p_shgcfg->Gateway;

            status = qcom_ipconfig(0, IPCFG_STATIC, &ip , &sn, &gtw);
            if (status != A_OK) {
                status_led = BSP_LED_RED;
                wait_to_retry = DEF_YES;
                continue;
            }

            qcom_dnsc_enable(1);

            i = 0;
            while ((p_shgcfg->DNS_Servers[i] != 0) && (i < NBR_DNS_SERVERS)) {
                qcom_dnsc_add_server_address((A_UINT8 *)&p_shgcfg->DNS_Servers[i], ATH_AF_INET);
                ++i;
            }

            if (i == 0) {                                       /* Configs did not specify any DNS servers.             */
                qcom_dnsc_add_server_address((A_UINT8 *)&DFLT_DNS_SERVER_IP, ATH_AF_INET); /* Add default server.       */
            }
        }

                                                                /* Setup success!                                       */
        OSFlagPost(&WifiFlagGrp, WIFI_FLAG_CONN_UP, OS_OPT_NONE, &err_os);
        BSP_LED_Off(BSP_LED_ALL);
        BSP_LED_On(BSP_LED_GREEN);

                                                                /* Wait for the connection to go down.                  */
        OSFlagPend(&WifiFlagGrp, WIFI_FLAG_CONN_UP, 0, OS_OPT_PEND_FLAG_CLR_ANY | OS_OPT_PEND_BLOCKING, DEF_NULL, &err_os);
        BSP_LED_Off(BSP_LED_ALL);

                                                                /* Lost connection. Set status LED to orange for retry. */
        status_led = BSP_LED_ORANGE;
    }
}


/*
*********************************************************************************************************
*                                          WiFi_OnConnectCallback()
*
* Description : Called from the QCA Task when an event occurs. Posts a flag based on the state change.
*
* Arguments   : state   :   The QCA state that caused the callback to occur
*
*               devId   :   QCA device ID
*
*               bssid   :   MAC address of the AP
*
*               bssConn :   AP connection status
*
* Returns     : none.
*
* Notes       : none.
*
*********************************************************************************************************
*/
void  WiFi_OnConnectCallback (int state, A_UINT8 devId, A_UINT8 *bssid, A_BOOL bssConn)
{
    OS_ERR  err;


    switch (state) {
        case QCA_CONNECT_SUCCESS:
             OSFlagPost(&WifiFlagGrp, WIFI_FLAG_CONNECT_SUCCESS, OS_OPT_POST_FLAG_SET, &err);
             break;

        case QCA_RSNA_AUTH_SUCCESS:
             OSFlagPost(&WifiFlagGrp, WIFI_FLAG_RSNA_AUTH_SUCCESS, OS_OPT_POST_FLAG_SET, &err);
             break;

        case QCA_CONNECT_FAILURE:
        case QCA_RSNA_AUTH_FAILURE:                             /* Clear all connection state flags                     */
             OSFlagPost(&WifiFlagGrp, WIFI_FLAG_CONNECT_SUCCESS | WIFI_FLAG_RSNA_AUTH_SUCCESS | WIFI_FLAG_CONN_UP, OS_OPT_POST_FLAG_CLR, &err);
             break;

        default:
             break;                                             /* Unknown state -- do nothing.                         */
    }
}


