/*
*********************************************************************************************************
*                                             EXAMPLE CODE
*********************************************************************************************************
* Licensing terms:
*   This file is provided as an example on how to use Micrium products. It has not necessarily been
*   tested under every possible condition and is only offered as a reference, without any guarantee.
*
*   Please feel free to use any application code labeled as 'EXAMPLE CODE' in your application products.
*   Example code may be used as is, in whole or in part, or may be used as a reference only. This file
*   can be modified as required.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: http://www.micrium.com
*
*   Please help us continue to provide the Embedded community with the finest software available.
*
*   Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                      APPLICATION CONFIGURATION
*
* Filename : app_cfg.h
*********************************************************************************************************
*/

#ifndef  _APP_CFG_H_
#define  _APP_CFG_H_


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdarg.h>
#include  <stdio.h>
#include  <os_cfg.h>


/*
*********************************************************************************************************
*                                       MODULE ENABLE / DISABLE
*********************************************************************************************************
*/

#define  APP_CFG_TCPIP_EN                 DEF_ENABLED


/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*********************************************************************************************************
*/

#define  APP_CFG_TASK_START_PRIO                  5u
#define  APP_CFG_HEARTBEAT_TASK_PRIO        (OS_CFG_PRIO_MAX-4u)

#define  APP_CFG_QCA_TASK_PRIO                    4u

#define  DHCPc_OS_CFG_TASK_PRIO                  25u
#define  DHCPc_OS_CFG_TMR_TASK_PRIO              26u
#define  DNSc_OS_CFG_INSTANCE_TASK_PRIO          11u

#define  MQTT_CFG_TASK_PRIO                       9u            /*  Internal MQTT task priority                         */

#define  CLK_CFG_TASK_PRIO                       10u

#define  AWS_IOT_MQTT_TASK_PRIO                  12u
#define  AWS_IOT_CONN_TASK_PRIO                  13u            /*  Connection task priority                            */
#define  AWS_IOT_PING_TASK_PRIO                  14u            /*  Publish task priority                               */
#define  AWS_IOT_SUBSCRIBE_TASK_PRIO             15u            /*  Subscribe task priority                             */
#define  AWS_IOT_PUBLISH_TASK_PRIO               16u            /*  Publish task priority                               */


/*
*********************************************************************************************************
*                                            TASK STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/

#define  APP_CFG_TASK_START_STK_SIZE            512u
#define  APP_CFG_HEARTBEAT_TASK_STK_SIZE         64u

#define  APP_CFG_QCA_TASK_STK_SIZE             1024u

#define  DHCPc_OS_CFG_TASK_STK_SIZE             512u
#define  DHCPc_OS_CFG_TMR_TASK_STK_SIZE         512u

#define  AWS_IOT_CONN_TASK_STK_SIZE            4096u            /*  Connection task stack size                          */
#define  AWS_IOT_PING_TASK_STK_SIZE            1024u            /*  Subscribe task stack size                           */
#define  AWS_IOT_PUBLISH_TASK_STK_SIZE         1024u            /*  Publish task stack size                             */
#define  AWS_IOT_SUBSCRIBE_TASK_STK_SIZE       1024u            /*  Subscribe task stack size                           */
#define  AWS_IOT_MQTT_TASK_STK_SIZE            2048u

#define  MQTT_CFG_TASK_STK_SIZE                2048u            /*  Internal MQTT task stack size in bytes               */
#define  DNSc_OS_CFG_INSTANCE_TASK_STK_SIZE    4096u
#define  CLK_CFG_TASK_STK_SIZE                  512u


/*
*********************************************************************************************************
*                                               TCPIP
*********************************************************************************************************
*/

#define  NET_OS_CFG_IF_TX_DEALLOC_TASK_PRIO                       6u
#define  NET_OS_CFG_IF_TX_DEALLOC_TASK_STK_SIZE                1024u

#define  NET_OS_CFG_IF_RX_TASK_PRIO                               7u
#define  NET_OS_CFG_IF_RX_TASK_STK_SIZE                        1024u

#define  NET_OS_CFG_TMR_TASK_PRIO                                 8u
#define  NET_OS_CFG_TMR_TASK_STK_SIZE                          1024u

#define  NET_OS_CFG_IF_LOOPBACK_Q_SIZE                            9u
#define  NET_OS_CFG_IF_RX_Q_SIZE                                 10u
#define  NET_OS_CFG_IF_TX_DEALLOC_Q_SIZE                         20u

//#warning "Set this value for demo IP addresses for each interface"
#define  APP_NET_CFG_IF_0_IP_ADDR_STR              "192.168.1.123"
#define  APP_NET_CFG_IF_0_MASK_STR                 "255.255.255.0"
#define  APP_NET_CFG_IF_0_GATEWAY_STR              "192.168.1.1"


/*
*********************************************************************************************************
*                                          WIFI CONFIGURATION
*********************************************************************************************************
*/

#define  APP_CFG_WIFI_AP_TBL_SIZE               50u


/*
*********************************************************************************************************
*                                     TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                        0u
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                       1u
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                        2u
#endif

#define  APP_TRACE_LEVEL                        TRACE_LEVEL_INFO
#define  APP_TRACE                              printf

#define  APP_TRACE_INFO(x)               ((APP_TRACE_LEVEL >= TRACE_LEVEL_INFO)  ? (void)(APP_TRACE x) : (void)0)
#define  APP_TRACE_DBG(x)                ((APP_TRACE_LEVEL >= TRACE_LEVEL_DBG)   ? (void)(APP_TRACE x) : (void)0)

#endif
