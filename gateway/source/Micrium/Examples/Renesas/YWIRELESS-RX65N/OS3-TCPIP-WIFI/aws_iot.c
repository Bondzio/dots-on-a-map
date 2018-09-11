/*
*********************************************************************************************************
*                                            APPLICATION CODE
*
*                          (c) Copyright 2016; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          AWS IoT APPLICATION
* Filename      : aws_iot.c
* Version       : V2.00
* Programmer(s) : MTM
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#include  <cpu.h>
#include  <lib_str.h>
#include  <lib_mem.h>

#include  "aws_iot.h"
#include  "aws_iot_cert.h"

#include  <app_cfg.h>
#include <bsp_led.h>
#include <bsp_uart.h>

#include  <Client/Source/mqtt-c.h>
#include  <Secure/net_secure.h>


/*
*********************************************************************************************************
*                                            DEFINES
*********************************************************************************************************
*/

#define  AWS_IOT_PUBLISH_TASK_Q_SIZE                    64u     /* Publish task queue size                              */
#define  AWS_IOT_SUBSCRIBE_TASK_Q_SIZE                  64u     /* Subscribe task queue size                            */

#define  AWS_IOT_CLIENT_ID_STR_MAX_LEN                  32u     /* Client ID buffer length                              */

#define  AWS_IOT_MSG_QTY                                 3u     /* Number of messages to be processed at one time       */
#define  AWS_IOT_INTERNAL_TASK_DLY                       1u     /* MQTT task delay time                                 */
#define  AWS_IOT_INACTIVITY_TIMEOUT_s                   60u     /* Socket timeout                                       */
#define  AWS_IOT_PUBLISH_MAX_RETRY                       4u     /* Number of times to attempt to republish a message    */
#define  AWS_IOT_KEEP_ALIVE_S                           30u     /* Number of seconds to wait to send a MQTT keepalive   */
#define  AWS_IOT_TIMEOUT_MS                          60000u     /* Number of milliseconds before timing out             */

#define  AWS_IOT_BUF_TOTAL_SIZE                      16384u     /* Size of the buffer pool for MQTT messages            */
#define  AWS_IOT_BUF_ALIGNMENT                          32u     /* Align to data to a word                              */


/*
*********************************************************************************************************
*                                           DATA TYPES
*********************************************************************************************************
*/

typedef  enum  aws_iot_msg_t                                    /* Different types of local messages                    */
{
    AWS_IOT_MSG_CONNECT     = 0u,                               /* Message for connecting to AWS                        */
    AWS_IOT_MSG_PUBLISH,                                        /* Message used for publishing data to AWS              */
    AWS_IOT_MSG_PUBLISH_RX,                                     /* Message used for receiving data from AWS             */
    AWS_IOT_MSG_SUBSCRIBE,                                      /* Message used for subscribing to topics on AWS        */
    AWS_IOT_MSG_PING,
    MAX_AWS_IOT_MSG,                                            /* Total number of local messages                       */
    AWS_IOT_MSG_ERR                                             /* Used to signal an error with a local message         */
} AWS_IOT_MSG_t;

typedef  struct  aws_iot_sub_topic                              /* Individual topic subscription information            */
{
    AWS_IOT_QOS             QoS;                                /* Topic QoS subscription level                         */
    AWS_IOT_SUB_CALLBACK    Callback;                           /* Function to call when msg recvd on this topic        */
    CPU_CHAR                Topic[AWS_IOT_TOPIC_LEN_MAX];       /* Topic subscription string                            */
} AWS_IOT_SUB_TOPIC;

typedef  struct                                                 /* All topic subscription information                   */
{
    CPU_INT16U              CurrentNumSubs;                     /* Total number of active topic subscriptions           */
    AWS_IOT_SUB_TOPIC       Sub[AWS_IOT_MAX_SUBS];              /* Array of topic subscription structs                  */
} AWS_IOT_SUBS;

typedef  struct  aws_iot_msg                                    /* Local message information                            */
{
    MQTTc_MSG               Msg;                                /* MQTTc message type (Conn, Pub, Sub, PubRx)           */
    CPU_INT08U              MsgBuf[AWS_IOT_MSG_LEN_MAX];        /* Buffer for local message                             */
} AWS_IOT_MSG;

typedef  struct  aws_iot_param_config                           /* MQTTc paramater configuration                        */
{
    MQTTc_PARAM_TYPE        MQTTcParamType;                     /* MQTTc parameter type                                 */
    void                   *MQTTcParam;                         /* MQTTc parameter setting                              */
} AWS_IOT_PARAM_CONFIG;

typedef  struct  aws_iot_config                                 /* General AWS IoT configuration values                 */
{
    CPU_INT16U              PubCnt;                             /* Total number of messages published                   */
    CPU_INT16U              SubCnt;                             /* Total number of messages received                    */
    CPU_BOOLEAN             Status;                             /* Current status of the MQTT connection                */
} AWS_IOT_CONFIG;


/*
*********************************************************************************************************
*                                      FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void         AWS_IoT_ConnTask                    (       void             *p_arg);

static  void         AWS_IoT_PingTask                    (       void             *p_arg);

static  void         AWS_IoT_PublishTask                 (       void             *p_arg);

static  void         AWS_IoT_SubscribeTask               (       void             *p_arg);

static  void         AWS_IoT_OnCmplCallbackFnct          (       MQTTc_CONN       *p_conn,
                                                                 MQTTc_MSG        *p_msg,
                                                                 void             *p_arg,
                                                                 MQTTc_ERR         err);

static  void         AWS_IoT_OnErrCallbackFnct           (       MQTTc_CONN       *p_conn,
                                                                 void             *p_arg,
                                                                 MQTTc_ERR         err);

static  void         AWS_IoT_OnPublishRxCallbackFnct     (       MQTTc_CONN       *p_conn,
                                                          const  CPU_CHAR         *topic_name_str,
                                                                 CPU_INT32U        topic_len,
                                                          const  CPU_CHAR         *message_str,
                                                                 void             *p_arg,
                                                                 MQTTc_ERR         err);

static  void         AWS_IoT_OnErrCallbackFnct           (       MQTTc_CONN       *p_conn,
                                                                 void             *p_arg,
                                                                 MQTTc_ERR         err);

static  void         AWS_IoT_BufInit                     (       AWS_IOT_ERR      *p_err);

static  void         AWS_IoT_MQTTcInit                   (       MQTTc_ERR        *p_err);

static  void         AWS_IoT_MQTTcSetParams              (       MQTTc_ERR        *p_err);

static  void         AWS_IoT_MQTTcConfigParams           (       MQTTc_PARAM_TYPE  mqttc_param_type,
                                                                 void             *p_mqttc_param,
                                                                 AWS_IOT_ERR      *p_err);

static  CPU_BOOLEAN  AWS_IoT_ChkConnect                  (       void);

static  CPU_BOOLEAN  AWS_IoT_SubscribeCmp                (       CPU_CHAR         *p_saved_topic,
                                                                 CPU_CHAR         *p_recvd_topic);

CPU_BOOLEAN  AWS_IoT_GetStatus                   (       void);

static  void         AWS_IoT_SetStatus                   (       CPU_BOOLEAN       status);

static  void         AWS_IoT_IncrementPub                (       void);

static  void         AWS_IoT_IncrementSub                (       void);

        CPU_BOOLEAN  AWS_IoT_ClientCertTrustCallBackFnct (       void                             *p_cert_dn,
                                                                 NET_SOCK_SECURE_UNTRUSTED_REASON  reason);


/*
*********************************************************************************************************
*                                       GLOBAL VARIABLES
*********************************************************************************************************
*/


static         OS_TCB              AWS_IoT_ConnTaskTCB;         /* AWS IoT Task TCBs                                    */
static         OS_TCB              AWS_IoT_PingTaskTCB;
static         OS_TCB              AWS_IoT_PublishTaskTCB;
static         OS_TCB              AWS_IoT_SubscribeTaskTCB;
                                                                /* AWS IoT task stack buffers                           */
static         CPU_STK             AWS_IoT_ConnTaskStk[AWS_IOT_CONN_TASK_STK_SIZE];
static         CPU_STK             AWS_IoT_PingTaskStk[AWS_IOT_PING_TASK_STK_SIZE];
static         CPU_STK             AWS_IoT_PublishTaskStk[AWS_IOT_PUBLISH_TASK_STK_SIZE];
static         CPU_STK             AWS_IoT_SubscribeTaskStk[AWS_IOT_SUBSCRIBE_TASK_STK_SIZE];

static         CPU_INT08U          AWS_IoT_MQTTTaskStk[AWS_IOT_MQTT_TASK_STK_SIZE];

static         CPU_CHAR            AWS_IoT_ClientIDStr[AWS_IOT_CLIENT_ID_STR_MAX_LEN];

static         OS_SEM              AWS_IoT_ConnSem;             /* AWS Connection Parameters                            */
static         MQTTc_CONN          AWS_IoT_Conn;
                                                                /* Local AWS message buffers                            */
static         AWS_IOT_MSG         AWS_IoT_Msg[MAX_AWS_IOT_MSG];
static         AWS_IOT_CONFIG      AWS_IoT_Config = {{0u}};     /* AWS IoT general configuration properties             */
static         AWS_IOT_SUBS        AWS_IoT_Subs;                /* AWS IoT subscription data                            */

static         MEM_SEG             AWS_IoT_MemSeg;              /* AWS data memory segment                              */
static         MEM_DYN_POOL        AWS_IoT_DynamicMemPool;      /* AWS data dynamic memory pool                         */
                                                                /* AWS data payload buffer                              */
static         CPU_INT08U          AWS_IoT_MemSegData[AWS_IOT_BUF_TOTAL_SIZE];

static  const  NET_TASK_CFG        AWS_IoT_MQTT_TaskCfg =       /* Cfg for MQTTc internal task.                         */
{
    AWS_IOT_MQTT_TASK_PRIO,
    AWS_IOT_MQTT_TASK_STK_SIZE,
    AWS_IoT_MQTTTaskStk
};

static  const  MQTTc_CFG           AWS_IoT_Cfg =                /* Cfg for MQTTc Connection.                            */
{
    AWS_IOT_MSG_QTY,
    AWS_IOT_INACTIVITY_TIMEOUT_s,
    AWS_IOT_INTERNAL_TASK_DLY
};

NET_APP_SOCK_SECURE_MUTUAL_CFG     AWSIoTSecureMutual =
{
                 NET_SOCK_SECURE_CERT_KEY_FMT_DER,
    (CPU_CHAR*)  cert_der,
                 sizeof(cert_der),
                 DEF_FALSE,
    (CPU_CHAR*)  privkey_der,
                 sizeof(privkey_der)
};

NET_APP_SOCK_SECURE_CFG            AWSIoTSecure =               /* Certificate/Private Key Configuration                */
{
     AWS_IOT_BROKER_NAME,
     AWS_IoT_ClientCertTrustCallBackFnct,
     NULL
     //&AWSIoTSecureMutual
};


char m1_mqtt_username[24];
char m1_mqtt_password[65];

static  AWS_IOT_PARAM_CONFIG  AWS_IoT_ParamConfig[] =           /* MQTT Parameter Config                                */
{
    {MQTTc_PARAM_TYPE_BROKER_NAME,                  (void *) AWS_IOT_BROKER_NAME},
    {MQTTc_PARAM_TYPE_BROKER_PORT_NBR,              (void *) AWS_IOT_BROKER_PORT},
    {MQTTc_PARAM_TYPE_CLIENT_ID_STR,                (void *) 0},
    {MQTTc_PARAM_TYPE_KEEP_ALIVE_TMR_SEC,           (void *) AWS_IOT_KEEP_ALIVE_S},
    {MQTTc_PARAM_TYPE_CALLBACK_ON_CONNECT_CMPL,     (void *) AWS_IoT_OnCmplCallbackFnct},
    {MQTTc_PARAM_TYPE_CALLBACK_ON_PUBLISH_CMPL,     (void *) AWS_IoT_OnCmplCallbackFnct},
    {MQTTc_PARAM_TYPE_CALLBACK_ON_SUBSCRIBE_CMPL,   (void *) AWS_IoT_OnCmplCallbackFnct},
    {MQTTc_PARAM_TYPE_PUBLISH_RX_MSG_PTR,           (void *)&AWS_IoT_Msg[AWS_IOT_MSG_PUBLISH_RX].Msg},
    {MQTTc_PARAM_TYPE_CALLBACK_ON_PUBLISH_RX,       (void *) AWS_IoT_OnPublishRxCallbackFnct},
    {MQTTc_PARAM_TYPE_TIMEOUT_MS,                   (void *) AWS_IOT_TIMEOUT_MS},
    {MQTTc_PARAM_TYPE_CALLBACK_ON_ERR_CALLBACK,     (void *) AWS_IoT_OnErrCallbackFnct},
    {MQTTc_PARAM_TYPE_SECURE_CFG_PTR,               (void *)&AWSIoTSecure},
    {MQTTc_PARAM_TYPE_USERNAME_STR,                 (void *) m1_mqtt_username},
    {MQTTc_PARAM_TYPE_PASSWORD_STR,                 (void *) m1_mqtt_password}
};


extern OS_FLAG_GRP sonar_grp;

/*
*********************************************************************************************************
*                                           PUBLIC FUNCTIONS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            AWS_IoT_Init()
*
* Description : Initialize the AWS IoT module. Create's the publish, subscribe and connect tasks.
*
* Arguments   : p_err       Payload to be sent in the form of name and value.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  AWS_IoT_Init (CPU_CHAR*  thing_id,
                    AWS_IOT_ERR  *p_err)
{
    OS_ERR      os_err;
    MQTTc_ERR   err_mqttc;


   *p_err = AWS_IOT_ERR_NONE;                                   /* Clear the error pointer                              */

    do {
        sprintf(AWS_IoT_ClientIDStr,                            /* Create the Client ID string from the thing ID        */
                "%s%s",
                AWS_IOT_CLIENT_ID_PREFIX,
                thing_id);
                                                                /* Set the Client ID string in the config               */
        AWS_IoT_MQTTcConfigParams(MQTTc_PARAM_TYPE_CLIENT_ID_STR,
                                  (void*) AWS_IoT_ClientIDStr,
                                  p_err);
        if(*p_err != AWS_IOT_ERR_NONE) {
            break;
        }

        AWS_IoT_SetStatus(DEF_FALSE);                           /* Set the connection status to false                   */

        OSSemCreate(&AWS_IoT_ConnSem ,                          /* Create a semaphore for MQTT connect signaling        */
                     "AWS IoT Connect/Subscribe Semaphore",
                     0u,
                    &os_err);
        if (os_err != OS_ERR_NONE) {
           *p_err = AWS_IOT_ERR_INIT_SEM_CREATE_FAIL;
            break;
        }

        AWS_IoT_BufInit(p_err);                                 /* Initialize the buffer pool                           */
        if (*p_err != AWS_IOT_ERR_NONE) {
            break;
        }

        AWS_IoT_MQTTcInit(&err_mqttc);                          /* Init the connection, publish and subscribe msgs      */
        if (err_mqttc != MQTTc_ERR_NONE) {
           *p_err = AWS_IOT_ERR_INIT_MQTT_INIT_FAIL;
            break;
        }
        

        OSTaskCreate(              &AWS_IoT_PingTaskTCB,     /* Create the publish task                              */
                                   "AWS IoT Ping Task",
                     (OS_TASK_PTR ) AWS_IoT_PingTask,
                                    0u,
                                    AWS_IOT_PING_TASK_PRIO,
                                   &AWS_IoT_PingTaskStk[0u],
                                   (AWS_IOT_PING_TASK_STK_SIZE / 10u),
                                    AWS_IOT_PING_TASK_STK_SIZE,
                                    0u,
                                    0u,
                                    0u,
                                   (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                                   &os_err);        

        OSTaskCreate(              &AWS_IoT_PublishTaskTCB,     /* Create the publish task                              */
                                   "AWS IoT Publish Task",
                     (OS_TASK_PTR ) AWS_IoT_PublishTask,
                                    0u,
                                    AWS_IOT_PUBLISH_TASK_PRIO,
                                   &AWS_IoT_PublishTaskStk[0u],
                                   (AWS_IOT_PUBLISH_TASK_STK_SIZE / 10u),
                                    AWS_IOT_PUBLISH_TASK_STK_SIZE,
                                    AWS_IOT_PUBLISH_TASK_Q_SIZE,
                                    0u,
                                    0u,
                                   (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                                   &os_err);
        if (os_err != OS_ERR_NONE) {
           *p_err = AWS_IOT_ERR_INIT_PUBLISH_TASK_CREATE_FAIL;
            break;
        }

        OSTaskCreate(              &AWS_IoT_SubscribeTaskTCB,   /* Create the subscribe task                            */
                                   "AWS IoT Subscribe Task",
                     (OS_TASK_PTR ) AWS_IoT_SubscribeTask,
                                    0u,
                                    AWS_IOT_SUBSCRIBE_TASK_PRIO,
                                   &AWS_IoT_SubscribeTaskStk[0u],
                                   (AWS_IOT_SUBSCRIBE_TASK_STK_SIZE / 10u),
                                    AWS_IOT_SUBSCRIBE_TASK_STK_SIZE,
                                    AWS_IOT_SUBSCRIBE_TASK_Q_SIZE,
                                    0u,
                                    0u,
                                   (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                                   &os_err);
        if (os_err != OS_ERR_NONE) {
           *p_err = AWS_IOT_ERR_INIT_SUBSCRIBE_TASK_CREATE_FAIL;
            break;
        }

        OSTaskCreate(              &AWS_IoT_ConnTaskTCB,        /* Create the connection task                           */
                                   "AWS IoT Connection Task",
                     (OS_TASK_PTR ) AWS_IoT_ConnTask,
                                    0u,
                                    AWS_IOT_CONN_TASK_PRIO,
                                   &AWS_IoT_ConnTaskStk[0u],
                                   (AWS_IOT_CONN_TASK_STK_SIZE / 10u),
                                    AWS_IOT_CONN_TASK_STK_SIZE,
                                    0u,
                                    0u,
                                    0u,
                                   (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                                   &os_err);
        if (os_err != OS_ERR_NONE) {
           *p_err = AWS_IOT_ERR_INIT_CONN_TASK_CREATE_FAIL;
            break;
        }
    } while(0u);
}


/*
*********************************************************************************************************
*                                          AWS_IoT_Subscribe()
*
* Description : Subscribe to a topic
*
* Arguments   : p_topic        The topic string to subscribe to
*
*               callback       The callback function for when a message is received on the topic
*
*               qos            QoS level to subscribe at.
*
*               p_err          Pointer to store the error state.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  AWS_IoT_Subscribe (CPU_CHAR              *p_topic,
                         AWS_IOT_SUB_CALLBACK   callback,
                         AWS_IOT_QOS            qos,
                         AWS_IOT_ERR           *p_err)
{
    OS_ERR      os_err;
    MQTTc_ERR   err_mqttc;
    CPU_INT32U  topic_len;


   *p_err = AWS_IOT_ERR_NONE;                                   /* Clear the AWS IoT error string                       */

    do
    {
        topic_len = Str_Len(p_topic);                           /* Get the topic string length                          */

        if ((topic_len == 0u) ||                                /* Check for an invalid topic length                    */
            (topic_len > AWS_IOT_TOPIC_LEN_MAX)) {
           *p_err = AWS_IOT_ERR_SUBSCRIBE_TOPIC_LEN_INVALID;
            break;
        }

        OSSchedLock(&os_err);                                   /* Lock the scheduler while we add the subscription     */
                                                                /* Save the callback and the QoS level                  */
        AWS_IoT_Subs.Sub[AWS_IoT_Subs.CurrentNumSubs].Callback  = callback;
        AWS_IoT_Subs.Sub[AWS_IoT_Subs.CurrentNumSubs].QoS       = qos;

                                                                /* Copy the topic string                                */
        Str_Copy(AWS_IoT_Subs.Sub[AWS_IoT_Subs.CurrentNumSubs].Topic,
                 p_topic);

        OSSchedUnlock(&os_err);                                 /* Unlock the scheduler                                 */

        if (AWS_IoT_GetStatus() == DEF_OK) {                    /* Only attempt to subscribe if we're connected         */
            MQTTc_Subscribe(                 &AWS_IoT_Conn,     /* Subscribe to the topic                               */
                                             &AWS_IoT_Msg[AWS_IOT_MSG_SUBSCRIBE].Msg,
                            (const CPU_CHAR*) AWS_IoT_Subs.Sub[AWS_IoT_Subs.CurrentNumSubs].Topic,
                                              AWS_IoT_Subs.Sub[AWS_IoT_Subs.CurrentNumSubs].QoS,
                                             &err_mqttc);
            if (err_mqttc != MQTTc_ERR_NONE) {
               *p_err = AWS_IOT_ERR_SUBSCRIBE_FAIL;
                break;
            }
        }

        AWS_IoT_Subs.CurrentNumSubs++;                          /* Increment the sub counter                            */

    } while(0u);
}


/*
 *********************************************************************************************************
 *                                           AWS_IoT_Publish()
 *
 * Description : Puts a message in queue for publication.
 *
 * Arguments   : p_payload          Payload data to publish to the broker
 *
 *               p_err              Pointer to store the error state.
 *
 * Return(s)   : none.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

void  AWS_IoT_Publish (AWS_IOT_PAYLOAD  *p_payload,
                       AWS_IOT_ERR      *p_err)
{
    OS_ERR       os_err;
    AWS_IOT_ERR  aws_iot_err;


    OSTaskQPost(       &AWS_IoT_PublishTaskTCB,
                (void*) p_payload,
                        sizeof(*p_payload),
                        OS_OPT_POST_FIFO,
                       &os_err);
    if (os_err != OS_ERR_NONE) {
       *p_err = AWS_IOT_ERR_PUBLISH_FAIL;
        AWS_IoT_BufFree( p_payload,
                        &aws_iot_err);
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                 TASKS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           AWS_IoT_ConnTask()
*
* Description : Monitors the MQTT connection.
*
* Arguments   : p_arg       is the argument passed by 'OSTaskCreate()', not used.
*
* Return(s)   : none.
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AWS_IoT_ConnTask (void  *p_arg)
{
    OS_ERR  os_err;


    (void)&p_arg;

    while (DEF_TRUE) {
        AWS_IoT_ChkConnect();                                   /* Check the MQTT connection. If none exists create one */
        OSTimeDlyHMSM( 0u, 0u, 0u, 500u,
                       OS_OPT_TIME_HMSM_STRICT,
                      &os_err);
    }
}


static  void  AWS_IoT_PingTask (void  *p_arg)
{
    OS_ERR           os_err;
    MQTTc_ERR        mqttc_err;


    (void)&p_arg;

    while (DEF_TRUE) {
        OSTimeDlyHMSM(0, 0, AWS_IOT_KEEP_ALIVE_S, 0, OS_OPT_TIME_DLY, &os_err);
        MQTTc_PingReq(&AWS_IoT_Conn, &AWS_IoT_Msg[AWS_IOT_MSG_PING].Msg, &mqttc_err);
    }
}


/*
*********************************************************************************************************
*                                          AWS_IoT_PublishTask()
*
* Description : This task publishes messages sent via its message queue.
*
* Arguments   : p_arg   is the argument passed by 'OSTaskCreate()', not used.
*
* Returns     : none.
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AWS_IoT_PublishTask (void  *p_arg)
{
    OS_MSG_SIZE      size;
    CPU_INT08U       retry;
    AWS_IOT_PAYLOAD *p_payload;
    OS_ERR           os_err;
    MQTTc_ERR        mqtt_err;
    AWS_IOT_ERR      aws_iot_err;


    (void)&p_arg;

    while (DEF_TRUE) {
        p_payload = (AWS_IOT_PAYLOAD*) OSTaskQPend( 0u,         /* Wait for a message to publish                        */
                                                    OS_OPT_PEND_BLOCKING,
                                                   &size,
                                                    0u,
                                                   &os_err);
        if ((os_err != OS_ERR_NONE) ||
            (p_payload == 0)) {
            continue;
        }



        retry = 0u;                                             /* Clear the retry counter                              */

        do {
            if (AWS_IoT_GetStatus() == DEF_OK) {                /* Check for an MQTT connection                         */
                MQTTc_Publish(&AWS_IoT_Conn,
                              &AWS_IoT_Msg[AWS_IOT_MSG_PUBLISH].Msg,
                               p_payload->Topic,
                               p_payload->AWS_IoT_QoS,
                               DEF_NO,
                               p_payload->Msg,
                              &mqtt_err);
                if (mqtt_err == MQTTc_ERR_NONE) {
                    OSFlagPost(&sonar_grp, PUBLISH_QUEUE_NOT_FULL, OS_OPT_POST_FLAG_SET, &os_err);                    
                    OSTaskSemPend( 0u,                          /* Wait until we get the callback                       */
                                   OS_OPT_PEND_BLOCKING,
                                   0u,
                                  &os_err);
                    break;
                } else {
                    OSFlagPost(&sonar_grp, PUBLISH_QUEUE_FULL, OS_OPT_POST_FLAG_SET, &os_err);                    
                }
            } else {
                OSTimeDlyHMSM( 0u, 0u, 0u, 500u,                /* Wait 500ms to check for a connection                 */
                               OS_OPT_TIME_HMSM_STRICT,
                              &os_err);
            }
            retry++;
        } while((mqtt_err == MQTTc_ERR_NONE) && (retry < AWS_IOT_PUBLISH_MAX_RETRY));

        AWS_IoT_BufFree(p_payload, &aws_iot_err);               /* Free the buffer                                      */
    }
}


/*
*********************************************************************************************************
*                                        AWS_IoT_SubscribeTask()
*
* Description : This task proceseses messages received from MQTTc.
*
* Arguments   : p_arg   is the argument passed by 'OSTaskCreate()', not used.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AWS_IoT_SubscribeTask (void *p_arg)
{
    CPU_TS                ts;
    OS_ERR                os_err;
    OS_MSG_SIZE           size;
    CPU_INT32U            i;
    CPU_INT32U            total_subs;
    CPU_BOOLEAN           topic_found;
    AWS_IOT_ERR           aws_iot_err;
    AWS_IOT_PAYLOAD      *p_payload;
    AWS_IOT_SUB_CALLBACK  callback_fnct = DEF_NULL;


    (void)&p_arg;

    while (DEF_TRUE) {
        p_payload = (AWS_IOT_PAYLOAD*) OSTaskQPend( 0u,         /* Wait for a message from the subscribe callback       */
                                                    OS_OPT_PEND_BLOCKING,
                                                   &size,
                                                   &ts,
                                                   &os_err);
        if (os_err != OS_ERR_NONE) {
            continue;
        }

        total_subs  = AWS_IoT_Subs.CurrentNumSubs;              /* Get the total number of topics to subscribe to       */
        topic_found = DEF_FALSE;                                /* Reset the topic_found variabe                        */

        for (i = 0u; i < total_subs; i++) {                     /* Loop through the saved topics and look for a match   */
            if (AWS_IoT_SubscribeCmp((CPU_CHAR*) AWS_IoT_Subs.Sub[i].Topic,
                                    (CPU_CHAR*) p_payload->Topic)) {
                topic_found = DEF_TRUE;
                break;
            }
        }

        if (topic_found == DEF_TRUE) {                          /* Matched a topic, call the saved callback function    */
            callback_fnct = AWS_IoT_Subs.Sub[i].Callback;
            if (callback_fnct != DEF_NULL) {
                callback_fnct(p_payload);
            }
        }

        AWS_IoT_BufFree( p_payload,                             /* Free the payload buffer                              */
                        &aws_iot_err);
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            AWS IOT FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          AWS_IoT_MQTTcInit()
*
* Description : Initialize the MQTT task, MQTT messages and MQTT connection
*
* Arguments   : p_err      Pointer to variable that will receive the return error code from this function
*
* Returns     : none.
*
* Notes       : none.
*********************************************************************************************************
*/

static  void  AWS_IoT_MQTTcInit (MQTTc_ERR  *p_err)
{
    CPU_INT32U  i;
    MQTTc_ERR   err_mqttc;


    do {
        MQTTc_Init(&AWS_IoT_Cfg,                                /* Initialize the MQTT module/task                      */
                   &AWS_IoT_MQTT_TaskCfg,
                    DEF_NULL,
                   &err_mqttc);
        if (err_mqttc != MQTTc_ERR_NONE) {
            break;
        }

        for (i = 0u; i < MAX_AWS_IOT_MSG; i++) {                /* Loop through the fixed messages and configure them   */
            MQTTc_MsgClr(&AWS_IoT_Msg[i].Msg,                   /* Clear the MQTT message                               */
                         &err_mqttc);
            if (err_mqttc != MQTTc_ERR_NONE) {
                break;
            }

            MQTTc_MsgSetParam(        &AWS_IoT_Msg[i].Msg,      /* Set the MQTT message's buffer pointer                */
                                       MQTTc_PARAM_TYPE_MSG_BUF_PTR,
                              (void *) AWS_IoT_Msg[i].MsgBuf,
                                      &err_mqttc);
            if (err_mqttc != MQTTc_ERR_NONE) {
                break;
            }

            MQTTc_MsgSetParam(        &AWS_IoT_Msg[i].Msg,      /* Set the MQTT message's max size                      */
                                       MQTTc_PARAM_TYPE_MSG_BUF_LEN,
                              (void *) AWS_IOT_MSG_LEN_MAX,
                                      &err_mqttc);
            if (err_mqttc != MQTTc_ERR_NONE) {
                break;
            }
        }

        if (err_mqttc != MQTTc_ERR_NONE) {                      /* Check for any errors from the message config loop    */
            break;
        }

        MQTTc_ConnClr(&AWS_IoT_Conn,                            /* Clear the connectin before using it                  */
                      &err_mqttc);
        if (err_mqttc != MQTTc_ERR_NONE) {
            break;
        }
    } while (0u);

   *p_err = err_mqttc;                                          /* Save the MQTTc error value                           */
}


/*
 *********************************************************************************************************
 *                                     AWS_IoT_MQTTcConfigParams()
 *
 * Description : Set a value in the AWS_IoT_ParamConfig array.
 *
 * Arguments   : mqttc_param_type   The MQTTc parameter to set
 *
 *               p_mqttc_param      Pointer to the value to set for the MQTTc param
 *
 *               p_err              Stores the AWS IoT Error state.
 *
 * Return(s)   : none.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static  void  AWS_IoT_MQTTcConfigParams (MQTTc_PARAM_TYPE  mqttc_param_type,
                                         void             *p_mqttc_param,
                                         AWS_IOT_ERR      *p_err)
{
    CPU_INT32U  i;
    CPU_INT32U  len;
    AWS_IOT_ERR aws_iot_err = AWS_IOT_ERR_INIT_MQTTC_PARAM_NOT_FOUND;

                                                                /* Get the size of the config parameter array           */
    len = sizeof(AWS_IoT_ParamConfig) / sizeof(AWS_IoT_ParamConfig[0u]);

    for(i = 0u; i < len; i++) {                                 /* Loop through the param array and set the value       */
        if(AWS_IoT_ParamConfig[i].MQTTcParamType == mqttc_param_type) {
            AWS_IoT_ParamConfig[i].MQTTcParam = p_mqttc_param;
            aws_iot_err = AWS_IOT_ERR_NONE;
            break;
        }
    }

    *p_err = aws_iot_err;                                       /* Set the AWS IoT error value                          */
}


/*
 *********************************************************************************************************
 *                                     AWS_IoT_MQTTcSetParams()
 *
 * Description : Configure the MQTT Client with values from the ParamConfig array
 *
 * Arguments   : p_err          Pointer to store the MQTTc error state.
 *
 * Return(s)   : none.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

static  void  AWS_IoT_MQTTcSetParams (MQTTc_ERR  *p_err)
{
    CPU_INT32U  i;
    CPU_INT32U  len;
    MQTTc_ERR   mqttc_err;

                                                                /* Get the size of the config parameter array           */
    len = sizeof(AWS_IoT_ParamConfig) / sizeof(AWS_IoT_ParamConfig[0u]);

    for(i = 0u; i < len; i++) {                                 /* Loop through the param array and set the values      */
        MQTTc_ConnSetParam(&AWS_IoT_Conn,                       /* Set the MQTTc parameter                              */
                            AWS_IoT_ParamConfig[i].MQTTcParamType,
                            AWS_IoT_ParamConfig[i].MQTTcParam,
                           &mqttc_err);
        if(mqttc_err != MQTTc_ERR_NONE) {
            break;
        }
    }

    *p_err = mqttc_err;                                         /* Set the MQTTc error value                            */
}


/*
*********************************************************************************************************
*                                           AWS_IoT_ReSubscribe()
*
* Description : Subscribe to all of the topics stored in the topic buffer. Used after a reconnect to AWS
*
* Arguments   : p_err          Pointer to store the error state.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AWS_IoT_ReSubscribe (AWS_IOT_ERR  *p_err)
{
    CPU_INT32U  i;
    CPU_INT32U  total_subs;
    OS_ERR      os_err;
    MQTTc_ERR   err_mqttc;


   *p_err      = AWS_IOT_ERR_NONE;                              /* Default to no errors                                 */
    total_subs = AWS_IoT_Subs.CurrentNumSubs;                   /* Get the total number of topics to subscribe to       */

    for (i = 0u; i < total_subs; i++) {                         /* Loop through the topics and subscribe                */
        MQTTc_Subscribe(                 &AWS_IoT_Conn,         /* Subscribe to the topic                               */
                                         &AWS_IoT_Msg[AWS_IOT_MSG_SUBSCRIBE].Msg,
                        (const CPU_CHAR*) AWS_IoT_Subs.Sub[i].Topic,
                        (CPU_INT08U)      AWS_IoT_Subs.Sub[i].QoS,
                                         &err_mqttc);
        if (err_mqttc != MQTTc_ERR_NONE) {
            break;
        }

        OSSemPend(&AWS_IoT_ConnSem,                             /* Wait for the subscribe callback                      */
                   0u,
                   OS_OPT_PEND_BLOCKING,
                   0u,
                  &os_err);
        if (os_err != OS_ERR_NONE) {
            break;
        }
    }
}


/*
*********************************************************************************************************
*                                         AWS_IoT_ChkConnect()
*
* Description : Checks the connection with AWS. If there is none it creates one.
*
* Arguments   : none.
*
* Return(s)   : DEF_OK on a sucessful connection.
*
*               DEF_FAIL on a failed connection attempt.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  AWS_IoT_ChkConnect (void)
{
    CPU_TS       ts;
    OS_ERR       os_err;
    NET_ERR      net_err;
    MQTTc_ERR    mqttc_err;
    AWS_IOT_ERR  aws_iot_err;
    CPU_BOOLEAN  ret_val;


    do {
        ret_val = DEF_FAIL;                                     /* Default to a failed state                            */

        if (AWS_IoT_GetStatus() == DEF_OK) {                    /* Get the status of the MQTT connection                */
            ret_val = DEF_OK;
            break;
        }

        MQTTc_ConnClose(&AWS_IoT_Conn,                          /* Close the existing connection if it exists.          */
                         DEF_NULL,
                        &mqttc_err);

        AWS_IoT_MQTTcSetParams(&mqttc_err);                     /* Set the MQTT message parameters                      */
        if (mqttc_err != MQTTc_ERR_NONE) {
            break;
        }

        NetSecure_CA_CertIntall( rootCA_der,                    /* Set the certificates and keys for the TLS connection */
                                 sizeof(rootCA_der),
                                 NET_SECURE_CERT_FMT_DER,
                                &net_err);
        if (net_err != NET_SECURE_ERR_NONE) {
            break;
        }

        MQTTc_ConnOpen(&AWS_IoT_Conn,                           /* Open conn to MQTT server                             */
                        MQTTc_FLAGS_NONE,
                       &mqttc_err);
        if (mqttc_err != MQTTc_ERR_NONE) {
            break;
        }

        MQTTc_Connect(&AWS_IoT_Conn,                            /* Send CONNECT msg to MQTT server.                     */
                      &AWS_IoT_Msg[AWS_IOT_MSG_CONNECT].Msg,
                      &mqttc_err);
        if (mqttc_err != MQTTc_ERR_NONE) {
            break;
        }

        OSSemPend(&AWS_IoT_ConnSem,                             /* Wait for a connection to the broker                  */
                   30000u,
                   OS_OPT_PEND_BLOCKING,
                  &ts,
                  &os_err);
        if(os_err != OS_ERR_NONE) {
            break;
        }


        AWS_IoT_ReSubscribe(&aws_iot_err);                      /* Resubscribe to any topics previously subscribed to   */
        if(aws_iot_err != AWS_IOT_ERR_NONE)
        {
            break;
        }

        ret_val = DEF_OK;

    } while(0u);

    if (ret_val == DEF_OK)
              BSP_LED_On(BSP_LED_1_GREEN);
    else
              BSP_LED_Off(BSP_LED_1_GREEN);

    return (ret_val);
}


/*
*********************************************************************************************************
*                                         AWS_IoT_SubscribeCmp()
*
* Description : Compare a saved topic with the received topic to see if they match. Will compare string to string,
*               as well as matching saved topics with wildcards to a received topic.
*
* Arguments   : p_saved_topic   The stored topic to compare to the recieved topic
*
*               p_recvd_topic   The topic a message has arrived on
*
* Return(s)   : DEF_TRUE    if the topics match
*               DEF_FALSE   if the topics do not match
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  AWS_IoT_SubscribeCmp (CPU_CHAR  *p_saved_topic,
                                           CPU_CHAR  *p_recvd_topic)
{
    CPU_INT32U   i, j;
    CPU_BOOLEAN  match;
    CPU_INT32U   saved_topic_len;
    CPU_INT32U   recvd_topic_len;


    do {
        match           = DEF_FALSE;                            /* Default to not a match                               */
        saved_topic_len = Str_Len(p_saved_topic);               /* Get the length of the saved topic string             */
        recvd_topic_len = Str_Len(p_recvd_topic);               /* Get the length of the received topic string          */

        if ((saved_topic_len > AWS_IOT_TOPIC_LEN_MAX) ||        /* Sanity check the topic length                        */
            (recvd_topic_len > AWS_IOT_TOPIC_LEN_MAX)) {
            break;
        }

        for (i = 0u, j = 0u;                                    /* Loop through the strings simultaneously and compare  */
             i < saved_topic_len && j < recvd_topic_len;
             i++, j++)
        {
            if (*p_saved_topic == '#')                          /* If we hit the '#' wildcard, its a match because      */
            {                                                   /* '#' allows anything else for the rest of the topic   */
                match = DEF_TRUE;
                break;
            }
                                                                /* If the saved topic has the '+' wildcard we can       */
            else if (*p_saved_topic == '+')                     /* safely ignore everything in the next topic level     */
            {
                while ((i < saved_topic_len) &&                 /* Loop until we find the next '/'                      */
                      (*p_saved_topic != '/'))
                {
                    i++;
                    p_saved_topic++;
                }

                while ((j < recvd_topic_len) &&                 /* Loop until we find the next '/'                      */
                       (*p_recvd_topic != '/'))
                {
                    j++;
                    p_recvd_topic++;
                }
            }

            else if (*p_saved_topic != *p_recvd_topic)          /* Since its not a '#' or '+' the characters should     */
            {                                                   /* match if its a valid topic                           */
                break;
            }

            p_saved_topic++;                                    /* Increment the saved topic pointer                    */
            p_recvd_topic++;                                    /* Increment the received topic pointer                 */
        }

        if ((i == saved_topic_len) &&                           /* If both strings got to the end, its a match!         */
            (j == recvd_topic_len))
        {
            match = DEF_TRUE;
        }
    } while (0u);

    return match;                                               /* Return the match result                              */
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           CALLBACK FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                        AWS_IoT_BufInit()
*
* Description : Initialize the memory pool for the AWS Payload Buffers
*
* Arguments   : p_err          Pointer to store the error state.
*
* Returns     : none.
*
* Notes       : none.
*********************************************************************************************************
*/

void  AWS_IoT_BufInit (AWS_IOT_ERR  *p_err)
{
    LIB_ERR     err_lib;
    CPU_INT32U  num_blocks;


   *p_err = AWS_IOT_ERR_NONE;                                   /* Default to no error                                  */

    do
    {
                                                                /* ------------ CREATION OF MEMORY SEGMENT -----------  */
        Mem_SegCreate( "AWS IoT Mem Seg",                       /* Name of mem seg (for debugging purposes).            */
                      &AWS_IoT_MemSeg,                          /* Pointer to memory segment structure.                 */
                      (CPU_ADDR) AWS_IoT_MemSegData,            /* Base address of memory segment data.                 */
                       AWS_IOT_BUF_TOTAL_SIZE,                  /* Length, in byte, of the memory segment.              */
                       AWS_IOT_BUF_ALIGNMENT,                   /* Padding alignment value.                             */
                      &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {                      /* Validate memory segment creation is successful.      */
            break;
        }

        num_blocks = AWS_IOT_BUF_TOTAL_SIZE / sizeof(AWS_IOT_PAYLOAD);

                                                                /* - CREATION OF GENERAL-PURPOSE DYNAMIC MEMORY POOL -  */
        Mem_DynPoolCreate("AWS IoT Dynamic Memory Pool",        /* Name of dynamic pool (for debugging purposes).       */
                          &AWS_IoT_DynamicMemPool,              /* Pointer to dynamic memory pool data.                 */
                          &AWS_IoT_MemSeg,                      /* Pointer to segment from which to allocate blocks.    */
                           sizeof(AWS_IOT_PAYLOAD),             /* Block size, in bytes.                                */
                           sizeof(CPU_ALIGN),                   /* Block alignment, in bytes.                           */
                           num_blocks,                          /* Initial number of blocks.                            */
                           num_blocks,                          /* Maximum number of blocks.                            */
                          &err_lib);
    } while (0u);

    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = AWS_IOT_ERR_BUF_INIT_FAIL;
    }
}


/*
*********************************************************************************************************
*                                        AWS_IoT_BufGet()
*
* Description : Grab a free buffer from the buffer pool
*
* Arguments   : p_payload      Pointer to store the payload buffer. Set to 0 if failed to get a buffer.
*
*               p_err          Pointer to store the error state.
*
* Returns     : none.
*
* Notes       : none.
*********************************************************************************************************
*/

void  AWS_IoT_BufGet (AWS_IOT_PAYLOAD  **p_payload,
                      AWS_IOT_ERR      *p_err)
{
    LIB_ERR  err_lib;


   *p_err = AWS_IOT_ERR_NONE;                                   /* Default to no error                                  */
                                                                /* Get the next available memory block                  */
   *p_payload = (AWS_IOT_PAYLOAD *) Mem_DynPoolBlkGet(&AWS_IoT_DynamicMemPool,
                                                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {                          /* If none are available, set payload to 0 and return   */
       *p_payload  = 0u;                                        /* an error.                                            */
       *p_err      = AWS_IOT_ERR_BUF_GET_FAIL;
    }
}


/*
*********************************************************************************************************
*                                        AWS_IoT_BufFree()
*
* Description : Free an AWS IoT payload buffer.
*
* Arguments   : p_payload      The payload to free.
*
*               p_err          Pointer to store the error state.
*
* Returns     : none.
*
* Notes       : none.
*********************************************************************************************************
*/

void  AWS_IoT_BufFree (AWS_IOT_PAYLOAD  *p_payload,
                       AWS_IOT_ERR      *p_err)
{
    LIB_ERR  err_lib;


   *p_err = AWS_IOT_ERR_NONE;                                   /* Default to no errors                                 */

    Mem_DynPoolBlkFree(        &AWS_IoT_DynamicMemPool,         /* Free the memory block                                */
                       (void *) p_payload,
                               &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        *p_err = AWS_IOT_ERR_BUF_FREE_FAIL;
    }
}


/*
 *********************************************************************************************************
 *                                         CALLBACK FUNCTIONS
 *********************************************************************************************************
 */


/*
*********************************************************************************************************
*                                      AWS_IoT_OnCmplCallbackFnct()
*
* Description : Callback function for MQTTc module called when a CONNECT, PUBLISH or SUBSCRIBE  operation
*               has completed.
*
* Arguments   : p_conn          Pointer to MQTTc Connection object for which operation has completed.
*
*               p_msg           Pointer to MQTTc Message object used for operation.
*
*               p_arg           Pointer to argument set in MQTTc Connection using the parameter type
*                               MQTTc_PARAM_TYPE_CALLBACK_ARG_PTR.
*
*               err             The MQTTc error value.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AWS_IoT_OnCmplCallbackFnct (MQTTc_CONN  *p_conn,
                                          MQTTc_MSG   *p_msg,
                                          void        *p_arg,
                                          MQTTc_ERR    err)
{
    OS_ERR os_err;


    switch(p_msg->Type)
    {
        case MQTTc_MSG_TYPE_CONNECT:                            /* Connect callback                                     */
             OSSemPost(&AWS_IoT_ConnSem,                        /* Release the connection semaphore                     */
                        OS_OPT_POST_NONE,
                       &os_err);
             AWS_IoT_SetStatus(DEF_OK);                         /* Set the connection status to OK                      */
             break;

        case MQTTc_MSG_TYPE_PUBLISH:                            /* Publish callback                                     */
             OSTaskSemPost(&AWS_IoT_PublishTaskTCB,             /* Release the publish semaphore                        */
                            OS_OPT_POST_NONE,
                           &os_err);
             if(os_err == OS_ERR_NONE)
             {
                 AWS_IoT_IncrementPub();                        /* Increment the pub counter                            */
             } else {
               // TODO: remove while (1)!!
                 while (1);
             }
             break;

        case MQTTc_MSG_TYPE_SUBSCRIBE:                          /* Subscribe callback                                   */
             OSSemPost(&AWS_IoT_ConnSem,                        /* Release the subscribe semaphore                      */
                        OS_OPT_POST_NONE,
                       &os_err);
             if(os_err == OS_ERR_NONE)
             {
                 AWS_IoT_IncrementSub();                        /* Increment the sub counter                            */
             }
             break;

        default:                                                /* Unknown callback. Don't do anything                  */
            break;
    }
}


/*
*********************************************************************************************************
*                                    AWS_IoT_OnPublishRxCallbackFnct()
*
* Description : Callback function for MQTTc module called when a PUBLISH message has been received.
*
* Arguments   : p_conn          Pointer to MQTTc Connection object for which operation has completed.
*
*               p_topic_str     String containing the topic of the message received. Not NULL-terminated.
*
*               topic_len       Length of the topic.
*
*               p_msg_str       NULL-terminated string containing the message received.
*
*               p_arg           Pointer to argument set in MQTTc Connection using the parameter type
*                               MQTTc_PARAM_TYPE_CALLBACK_ARG_PTR.
*
*               err             The MQTTc error value.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AWS_IoT_OnPublishRxCallbackFnct (       MQTTc_CONN  *p_conn,
                                               const  CPU_CHAR    *p_topic_str,
                                                      CPU_INT32U   topic_len,
                                               const  CPU_CHAR    *p_msg_str,
                                                      void        *p_arg,
                                                      MQTTc_ERR    err)
{
    OS_ERR            os_err;
    AWS_IOT_ERR       aws_iot_err;
    AWS_IOT_PAYLOAD  *p_payload;


    (void)&p_arg;

    do {
        AWS_IoT_BufGet(&p_payload,                              /* Get a payload buffer                                 */
                       &aws_iot_err);
        if(aws_iot_err != AWS_IOT_ERR_NONE) {                   /* If we couldn't get a buffer, drop it                 */
            break;
        }

        Str_Copy_N(p_payload->Topic,                            /* Save the topic string                                */
                   p_topic_str,
                   topic_len);
        Str_Copy(p_payload->Msg,                                /* Save the message payload                             */
                 p_msg_str);

        OSTaskQPost(       &AWS_IoT_SubscribeTaskTCB,           /* Send the message to the subscribe topic              */
                    (void*) p_payload,
                            sizeof(p_payload),
                            OS_OPT_POST_FIFO,
                           &os_err);
        if(os_err != OS_ERR_NONE) {
            AWS_IoT_BufFree( p_payload,                         /* Free the buffer if we're unable to post the data     */
                            &aws_iot_err);
        }
    } while(0u);
}


/*
*********************************************************************************************************
*                                     AWS_IoT_OnErrCallbackFnct()
*
* Description : Callback function for MQTTc module called when an error occurs.
*
* Arguments   : p_conn          Pointer to MQTTc ion object on which error occurred.
*
*               p_arg           Pointer to argument set in MQTTc Connection using the parameter type
*                               MQTTc_PARAM_TYPE_CALLBACK_ARG_PTR.
*
*               err             MQTTc error code.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  AWS_IoT_OnErrCallbackFnct (MQTTc_CONN  *p_conn,
                                          void       *p_arg,
                                          MQTTc_ERR   err)
{
    OS_ERR  os_err;
    (void) &p_conn;
    (void) &p_arg;


    AWS_IoT_SetStatus(DEF_FALSE);                               /* Set the connection status to false on an error       */

    OSTaskSemPost(&AWS_IoT_PublishTaskTCB,             /* Release the publish semaphore                        */
                   OS_OPT_POST_NONE,
                  &os_err);
}


/*
*********************************************************************************************************
*                                 AWS_IoT_ClientCertTrustCallBackFnct
*
* Description : Callback to check the trust on the cert
*
* Arguments   : p_cert_dn       Pointer to the cert.
*
*               reason          Reason the secure socket was untrusted.
*
* Return(s)   : Always returns DEF_OK
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  AWS_IoT_ClientCertTrustCallBackFnct(void                             *p_cert_dn,
                                                 NET_SOCK_SECURE_UNTRUSTED_REASON  reason)
{
    return (DEF_OK);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             GETS/SETS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         AWS_IoT_GetStatus()
*
* Description : Get the current connection status
*
* Arguments   : none.
*
* Return(s)   : DEF_TRUE for an active connection
*
*               DEF_FALSE for a closed connection
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  AWS_IoT_GetStatus (void)
{
    CPU_BOOLEAN ret_val;

    CPU_SR_ALLOC();

    CPU_CRITICAL_ENTER();
    ret_val = AWS_IoT_Config.Status;
    CPU_CRITICAL_EXIT();

    return ret_val;
}


/*
*********************************************************************************************************
*                                         AWS_IoT_SetStatus()
*
* Description : Set the current connection status
*
* Arguments   : status      The status to set
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  AWS_IoT_SetStatus (CPU_BOOLEAN status)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    AWS_IoT_Config.Status = status;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                         AWS_IoT_IncrementPub()
*
* Description : Increment the pub counter
*
* Arguments   : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  AWS_IoT_IncrementPub (void)
{
    AWS_IoT_Config.PubCnt++;
}


/*
*********************************************************************************************************
*                                         AWS_IoT_IncrementSub()
*
* Description : Increment the subscribe counter
*
* Arguments   : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  AWS_IoT_IncrementSub (void)
{
    AWS_IoT_Config.SubCnt++;
}

