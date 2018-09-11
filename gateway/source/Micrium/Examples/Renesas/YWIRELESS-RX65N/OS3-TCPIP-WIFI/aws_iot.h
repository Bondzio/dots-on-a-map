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
* Filename      : aws_iot.h
* Version       : V2.00
* Programmer(s) : MTM
*********************************************************************************************************
*/

#ifndef  AWS_IOT_H_
#define  AWS_IOT_H_

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>
#include  <cpu.h>
#include  <lib_def.h>

#include  <app_cfg.h>


/*
*********************************************************************************************************
*                                              DEFINES
*********************************************************************************************************
*/

#define  AWS_IOT_MAX_SUBS                               16u     /* Total number of subscriptions allowed                */
#define  AWS_IOT_TOPIC_LEN_MAX                          64u     /* Max length of a publish or subscribe topic           */
#define  AWS_IOT_MSG_LEN_MAX                           512u     /* Max length for an MQTT message                       */

#if 0
#define  AWS_IOT_BROKER_NAME                "data.iot.us-east-1.amazonaws.com"
#define  AWS_IOT_BROKER_PORT                          8883u
#else
#define  AWS_IOT_BROKER_NAME                "mqtt-renesas-na-sandbox.mediumone.com"
#define  AWS_IOT_BROKER_PORT                          61620u
#define  RNA_MQTT_PROJECT_ID                                 ""
#define  RNA_MQTT_USER_ID                                 ""
#define  RNA_API_KEY                            ""
#define  RNA_USER_PASSWORD                                 ""
#define  RNQ_MQTT_USER_PASSWORD                  ""
#endif
#define  AWS_IOT_CLIENT_ID_PREFIX                   "micrium-"  /* Client ID prefix                                     */


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/

typedef  enum  aws_iot_qos                                      /* AWS IoT QoS values.                                  */
{
    AWS_IOT_QOS_0                               = 0u,           /* MQTT QoS 0                                           */
    AWS_IOT_QOS_1,                                              /* MQTT QoS 1                                           */
    MAX_AWS_IOT_QOS,                                            /* Total number of QoS values                           */
    AWS_IOT_QOS_ERR,                                            /* MQTT QoS error                                       */
} AWS_IOT_QOS;

typedef  enum  aws_iot_err                                      /* AWS IoT error codes.                                 */
{
    AWS_IOT_ERR_NONE                            =  0u,          /* No error                                             */
    AWS_IOT_ERR_INIT_FAIL                       = 10u,          /* AWS_IoT_Init failed                                  */
    AWS_IOT_ERR_INIT_SEM_CREATE_FAIL            = 11u,          /* OSSemCreate in AWS_IoT_Init failed                   */
    AWS_IOT_ERR_INIT_MQTT_INIT_FAIL             = 12u,          /* MQTTcInit in AWS_IoT_Init failed                     */
    AWS_IOT_ERR_INIT_PUBLISH_TASK_CREATE_FAIL   = 13u,          /* OSTaskCreate for Publish failed                      */
    AWS_IOT_ERR_INIT_SUBSCRIBE_TASK_CREATE_FAIL = 14u,          /* OSTaskCreate for Subscribe failed                    */
    AWS_IOT_ERR_INIT_CONN_TASK_CREATE_FAIL      = 15u,          /* OSTaskCreate for Connection failed                   */
    AWS_IOT_ERR_INIT_MQTTC_PARAM_NOT_FOUND      = 16u,          /* MQTTcParamSet error                                  */
    AWS_IOT_ERR_SUBSCRIBE_FAIL                  = 20u,          /* AWS_IoT_Subscribe failed                             */
    AWS_IOT_ERR_SUBSCRIBE_TOPIC_LEN_INVALID     = 21u,          /* Subscribe topic length invalid for AWS_IoT_Subscribe */
    AWS_IOT_ERR_PUBLISH_FAIL                    = 30u,          /* AWS_IoT_Publish failed                               */
    AWS_IOT_ERR_BUF_INIT_FAIL                   = 40u,          /* AWS_IoT_BufInit failed to create the payload buffers */
    AWS_IOT_ERR_BUF_GET_FAIL                    = 41u,          /* AWS_IoT_BufGet failed to get a payload buffer        */
    AWS_IOT_ERR_BUF_FREE_FAIL                   = 42u,          /* AWS_IoT_BufFree failed to free the payload           */
} AWS_IOT_ERR;

typedef  struct  aws_iot_payload                                /* AWS payload buffer for publish or subscribe data     */
{
    CPU_CHAR        Topic[AWS_IOT_TOPIC_LEN_MAX];               /* MQTT topic string                                    */
    CPU_CHAR        Msg[AWS_IOT_MSG_LEN_MAX];                   /* MQTT message payload                                 */
    AWS_IOT_QOS     AWS_IoT_QoS;                                /* MQTT QoS value                                       */
    CPU_INT16U      MsgID;                                      /* Identifies the message type for offline storage      */
} AWS_IOT_PAYLOAD;


/*
*********************************************************************************************************
*                                        AWS IOT CALLBACK TYPES
*********************************************************************************************************
*/

typedef  void  (*AWS_IOT_SUB_CALLBACK)          (AWS_IOT_PAYLOAD  *p_payload);


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  AWS_IoT_Init          (CPU_CHAR               *thing_id,
                             AWS_IOT_ERR            *p_err);

void  AWS_IoT_Publish       (AWS_IOT_PAYLOAD        *p_payload,
                             AWS_IOT_ERR            *p_err);

void  AWS_IoT_Subscribe     (CPU_CHAR               *p_topic,
                             AWS_IOT_SUB_CALLBACK    callback,
                             AWS_IOT_QOS             qos,
                             AWS_IOT_ERR            *p_err);

void  AWS_IoT_BufGet        (AWS_IOT_PAYLOAD       **p_payload,
                             AWS_IOT_ERR            *p_err);

void  AWS_IoT_BufFree       (AWS_IOT_PAYLOAD        *p_payload,
                             AWS_IOT_ERR            *p_err);


/*
*********************************************************************************************************
*                                               END
*********************************************************************************************************
*/

#endif

