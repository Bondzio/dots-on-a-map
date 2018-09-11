/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                               (c) Copyright 2015; Micrium, Inc.; Weston, FL
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
* Filename      : sys_init.c
* Version       : V1.00
* Programmer(s) : JPC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>
#include  <lib_math.h>
#include  <Source/usbd_core.h>
#include  <ctype.h>
#include  <stdint.h>
#include  <app_cfg.h>
#include  <qca.h>
#include  <qca_bsp.h>
#include  <atheros_stack_offload.h>
#include  <qcom_api.h>
#include  <Source/probe_com.h>
#include  <TCPIP/Source/probe_tcpip.h>
#include  <bsp.h>
#include  <bsp_led.h>
#include  <bsp_sys.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         DEFAULT CONFIGURATION
*********************************************************************************************************
*/

#ifndef  MAIN_TASK_STK_SIZE
# define MAIN_TASK_STK_SIZE                                  512u
#endif

#ifndef  MAIN_TASK_PRIO
# define MAIN_TASK_PRIO                                        1u
#endif


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB      MainTaskTCB;
static  CPU_STK     MainTaskStk[MAIN_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  MainTask     (void  *p_arg);
static  void  HeartbeatTmr (void  *p_tmr, void  *p_arg);


/*
*********************************************************************************************************
*                                    EXTERNAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

extern  int  main (int  argc, char  **argv);


/*
*********************************************************************************************************
*                                   SYSTEM INITIALIZATION FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            HardwareInit()
*
* Description : This function is called immediately after startup BEFORE the C runtime initialization
*
* Caller(s)   : Reset vector
*
* Note(s)     : DO NOT use static or global variables in this function, as they will be overwritten
*               by the CRT initialization.
*********************************************************************************************************
*/

void  HardwareInit (void)
{
}


/*
*********************************************************************************************************
*                                            SoftwareInit()
*
* Description : This function is called immediately after the C runtime initialization. It initializes
*               the kernel, creates and starts the first task, and handles main()'s return value.
*
* Caller(s)   : Reset vector
*********************************************************************************************************
*/

void  SoftwareInit (void)
{
    OS_ERR  err_os;
    CPU_ERR err_cpu;


    CPU_Init();
    Mem_Init();                                                 /* Initialize the Memory Management Module              */
    Math_Init();                                                /* Initialize the Mathematical Module                   */


    CPU_NameSet("RX1118", &err_cpu);

    OSInit(&err_os);                                            /* Initialize "uC/OS-III, The Real-Time Kernel"         */

    OSTaskCreate((OS_TCB     *)&MainTaskTCB,                    /* Create the start task                                */
                 (CPU_CHAR   *)"Main Task",
                 (OS_TASK_PTR ) MainTask,
                 (void       *) 0,
                 (OS_PRIO     ) MAIN_TASK_PRIO,
                 (CPU_STK    *)&MainTaskStk[0],
                 (CPU_STK     )(MAIN_TASK_STK_SIZE / 10u),
                 (CPU_STK_SIZE) MAIN_TASK_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err_os);

    OSStart(&err_os);                                           /* Start multitasking (i.e. give control to uC/OS-III). */
    BSP_SysBrk();
}

/*
*********************************************************************************************************
*                                              MainTask()
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*
* Notes       : (1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                   used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  MainTask (void  *p_arg)
{
    static  OS_TMR    HeartbeatTmr;

    CPU_BOOLEAN    init_status;
    OS_ERR         err_os;
    QCA_ERR        err_qca;
    A_STATUS       status;
    BSP_LED        status_led;
    int            shg_cfg_err;
    A_UINT32       ip;
    A_UINT32       sn;
    A_UINT32       gtw;
    int            i;
    CPU_BOOLEAN    wait_to_retry;
    OS_FLAGS       wifi_flags;


    (void)p_arg;                                                /* See Note #1                                          */

    BSP_Init();                                                 /* Start BSP and tick initialization                    */

    OSTmrCreate(&HeartbeatTmr,

#if (OS_CFG_STAT_TASK_EN > 0)
    OSStatTaskCPUUsageInit(&os_err);                            /* Start tracking CPU statistics                        */
#endif


                                                                /* Initialize uC/FS.                                    */
    init_status = App_FS_Init();
    if (init_status == DEF_FAIL) {
        SignalCriticalErrAndDie();
    }

                                                                /* Initialize USBD w/ mass storage class.               */
    init_status = App_USBD_Init();
    if (init_status == DEF_FAIL) {
        SignalCriticalErrAndDie();
    }

                                                                /* QCA Init                                             */
    QCA_Init(&QCA_TaskCfg,
             &QCA_BSP_YWIRELESS_RX111_GT202,
             &QCA_DevCfg,
             &err_qca);
    if (err_qca != QCA_ERR_NONE) {
        SignalCriticalErrAndDie();
    }

    QCA_Start(&err_qca);
    if (err_qca != QCA_ERR_NONE) {
        SignalCriticalErrAndDie();
    }

                                                                /* Set the connect callback                             */
    status = qcom_set_connect_callback(0u, (void*)&QCA_ConnectCB);
    if (status != A_OK) {
        SignalCriticalErrAndDie();
    }
                                                                /* Power mode Function                                  */
    status = qcom_power_set_mode(0u, MAX_PERF_POWER, PWR_MAX);
    if (status != A_OK) {
        SignalCriticalErrAndDie();
    }

    status = qcom_op_set_mode(0u, QCOM_WLAN_DEV_MODE_STATION);
    if (status != A_OK) {
        SignalCriticalErrAndDie();
    }


                                                                /* Get device configs from storage.                     */
    shg_cfg_err = App_SHGCfgGet(&ShgCfg, SHG_CFG_DFLT_FILE_PATH);
    if (shg_cfg_err < 0) {
        SignalCriticalErrAndDie();
    }


    APP_TRACE_INFO(("Creating Application Tasks...\n\r"));
    App_TaskCreate(&ShgCfg);                                   /* Create Application tasks                             */

                                                                /* Illuminate orange LED during first initialization.   */
    status_led = BSP_LED_ORANGE;

                                                                /* Main task loop.                                      */
    wait_to_retry = DEF_NO;
    while (DEF_ON) {
                                                                /* Illuminate the status LED while initializing.        */
        BSP_LED_Off(BSP_LED_ALL);
        BSP_LED_On(status_led);

        if (wait_to_retry == DEF_YES) {
            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_NONE, &os_err);
            wait_to_retry = DEF_YES;                            /* Always wait to retry from now on.                    */
        }

                                                                /* Got configs, now apply them.                         */
        status = qcom_set_ssid(0u, ShgCfg.SSID_Str);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }
        status = qcom_sec_set_passphrase(0u, ShgCfg.Passphrase);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }
        status = qcom_sec_set_auth_mode(0, ShgCfg.AuthMode);
        if (status != A_OK) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }
        status = qcom_sec_set_encrypt_mode(0, ShgCfg.CryptType);
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
        if (ShgCfg.AuthMode != WLAN_AUTH_NONE) {
            wifi_flags |= WIFI_FLAG_RSNA_AUTH_SUCCESS;
        }
        OSFlagPend(&WifiFlagGrp, wifi_flags, CONN_UP_WAIT_TIMEOUT, OS_OPT_PEND_FLAG_SET_ALL, DEF_NULL, &os_err);
        if (os_err != OS_ERR_NONE) {
            status_led = BSP_LED_RED;
            wait_to_retry = DEF_YES;
            continue;
        }

        if (ShgCfg.UseDHCP == DEF_YES) {                      /* Get IP address via DHCP.                             */
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
            while (i < NBR_DNS_SERVERS && ShgCfg.DNS_Servers[i] != 0) {
                qcom_dnsc_add_server_address((A_UINT8 *)&ShgCfg.DNS_Servers[i], ATH_AF_INET);
                ++i;
            }

        } else {                                                /* Use static IP address.                               */
            ip  = ShgCfg.IP_Addr;
            sn  = ShgCfg.Netmask;
            gtw = ShgCfg.Gateway;

            status = qcom_ipconfig(0, IPCFG_STATIC, &ip , &sn, &gtw);
            if (status != A_OK) {
                status_led = BSP_LED_RED;
                wait_to_retry = DEF_YES;
                continue;
            }

            qcom_dnsc_enable(1);

            i = 0;
            while (i < NBR_DNS_SERVERS && ShgCfg.DNS_Servers[i] != 0) {
                qcom_dnsc_add_server_address((A_UINT8 *)&ShgCfg.DNS_Servers[i], ATH_AF_INET);
                ++i;
            }

            if (i == 0) {                                       /* Configs did not specify any DNS servers.             */
                qcom_dnsc_add_server_address((A_UINT8 *)&DFLT_DNS_SERVER_IP, ATH_AF_INET); /* Add default server.       */
            }
        }

                                                                /* Setup success!                                       */
        OSFlagPost(&WifiFlagGrp, WIFI_FLAG_CONN_UP, OS_OPT_NONE, &os_err);
        BSP_LED_Off(BSP_LED_ALL);
        BSP_LED_On(BSP_LED_GREEN);


                                                                /* Wait for the connection to go down.                  */
        OSFlagPend(&WifiFlagGrp, WIFI_FLAG_CONN_UP, 0, OS_OPT_PEND_FLAG_CLR_ANY | OS_OPT_PEND_BLOCKING, DEF_NULL, &os_err);


                                                                /* Lost connection. Set status LED to orange for retry. */
        status_led = BSP_LED_ORANGE;
    }
}


static  void  HeartbeatTmr (void  *p_tmr, void  *p_arg)
{
    (void)p_tmr;
    (void)p_arg;

    BSP_LED_Toggle(BSP_LED_HRTBT);
}


/*
*********************************************************************************************************
*                                      App_TaskCreate()
*
* Description :  This function creates the application tasks.
*
* Argument(s) :  p_shg_cfg    Pointer to a Smart Home Gateway configuration structure
*
* Return(s)   :  none.
*
* Caller(s)   :  AppTaskStart().
*
* Note(s)     :  none.
*********************************************************************************************************
*/

static  void  App_TaskCreate (SHG_CFG  const  *p_shg_cfg)
{
    OS_ERR  err;

                                                                /* Start the heartbeat.                                 */
    OSTaskCreate(&AppTaskHeartbeatTCB,
                 "Heartbeat",
                  AppTaskHeartbeat,
                  0,
                  2,                                            /* 2nd Highest priority for heartbeat.                  */
                  &AppTaskHeartbeatStk[0],
                  APP_TASK_HEARTBEAT_STK_SIZE / 10u,
                  APP_TASK_HEARTBEAT_STK_SIZE,
                  0, 0, 0, 0,
                  &err);

    ProbeCom_Init();
                                                                /* Start uC/Probe server.                               */
    ProbeTCPIP_Init();

    SmartHomeGwyInit((CPU_CHAR *)GetBSSID());                   /* Create the Smart Home Gateway simulation task.       */

    AppMQTT_Init(GetBSSID());                                   /* Initialize uC/MQTT-client                            */

    OSTaskCreate((OS_TCB     *)&AppTaskMQTT_ClientPubTCB,       /* Create the MQTT client task for publications.        */
                 (CPU_CHAR   *)"MQTT Client Task (Pub)",
                 (OS_TASK_PTR ) AppTaskMQTT_ClientPub,
                 (void       *) p_shg_cfg,
                 (OS_PRIO     ) APP_CFG_TASK_MQTT_CLIENT_PUB_PRIO,
                 (CPU_STK    *)&AppTaskMQTT_ClientPubStk[0],
                 (CPU_STK_SIZE) APP_CFG_TASK_MQTT_CLIENT_PUB_STK_SIZE / 10u,
                 (CPU_STK_SIZE) APP_CFG_TASK_MQTT_CLIENT_PUB_STK_SIZE,
                 (OS_MSG_QTY  ) 0u,
                 (OS_TICK     ) 0u,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSTaskCreate((OS_TCB     *)&AppTaskMQTT_ClientSubTCB,       /* Create the MQTT client task for subscriptions.       */
                 (CPU_CHAR   *)"MQTT Client Task (Sub)",
                 (OS_TASK_PTR ) AppTaskMQTT_ClientSub,
                 (void       *) p_shg_cfg,
                 (OS_PRIO     ) APP_CFG_TASK_MQTT_CLIENT_SUB_PRIO,
                 (CPU_STK    *)&AppTaskMQTT_ClientSubStk[0],
                 (CPU_STK_SIZE) APP_CFG_TASK_MQTT_CLIENT_SUB_STK_SIZE / 10u,
                 (CPU_STK_SIZE) APP_CFG_TASK_MQTT_CLIENT_SUB_STK_SIZE,
                 (OS_MSG_QTY  ) 0u,
                 (OS_TICK     ) 0u,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
}
