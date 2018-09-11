/*
*********************************************************************************************************
*                                            APPLICATION CODE
*
*                          (c) Copyright 2008-2013; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                          MQTT APPLICATION
*
*                                              TEMPLATE
*
* Filename      : app_mqtt.h
* Version       : V1.00
* Description   : Example MQTT Client for Embedded Applications.
*                 Includes provisioning message and sensor loop.
* Programmer(s) : JPB
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This file system application header file is protected from multiple pre-processor
*               inclusion through use of the MQTT application present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  APP_WIFI_UTIL_H_
#define  APP_WIFI_UTIL_H_


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdint.h>
#include  <cpu.h>
#include  <lib_def.h>
#include  <app_cfg.h>
#include  <qcom_api.h>
#include  "m1_bsp.h"

/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  SSID_LEN_MAX                   1

#define  PASSPHRASE_LEN_MAX             1

#define  IP_STR_LEN_MAX                 1                      /* Four 3-digit numbers + 3 dots = 15 characters.       */

#define  NBR_DNS_SERVERS                 1

#define  USERNAME_LEN_MAX              1                      /* See RFC3696 errata ID: 1690 for max email length.    */

#define  PASSWORD_STR_LEN               1

#define  TOPIC_STR_LEN_MAX             1                      /* Maximum topic length                                 */
#define  TOPIC_STR_LEN_MAX_STR        "1"

#define  HOST_NAME_LEN_MAX             1                      /* See RFC2181                                          */

#define  BLE_KEY_STR_LEN               1

#define  SHG_CFG_DFLT_FILE_PATH        "df:0:\\application.cfg"
#define  SHG_CFG_LORA_FILE_PATH        "df:0:\\lora.cfg"


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/

typedef  struct  _smart_home_gateway_cfg {
                                                                /* WiFi settings.                                       */
    char                SSID_Str[SSID_LEN_MAX + 1];
    WLAN_AUTH_MODE      AuthMode;
    WLAN_CRYPT_TYPE     CryptType;
    char                Passphrase[PASSPHRASE_LEN_MAX + 1];
    CPU_BOOLEAN         UseDHCP;
    uint32_t            IP_Addr;
    uint32_t            Netmask;
    uint32_t            Gateway;
    uint32_t            DNS_Servers[NBR_DNS_SERVERS];
                                                                /* MQTT broker settings.                                */
    char                HostStr[HOST_NAME_LEN_MAX + 1];
    uint16_t            Port;
    char                Username[USERNAME_LEN_MAX + 1];
    char                Password[PASSWORD_STR_LEN + 1];
    char                TopicPub[TOPIC_STR_LEN_MAX + 1];
    char                TopicSub[TOPIC_STR_LEN_MAX + 1];
                                                                /* PMOD settings.                                       */
    port                Ports[PERIPHERAL_PORTS_SUPPORTED_NUM];
    int                 HasWifiModule;
                                                                /* DEMO settings.                                       */
    DEMO_TYPE           DemoType;
    int32_t            LoopTimeSeconds;
    int32_t            LoopTimeMilliSeconds;
                                                                /* BLE settings.                                        */
    int                 BleMode;
    char                BleKey[BLE_KEY_STR_LEN + 1];
                                                                /* Lora settings.                                       */
    int                 LoraID;
    int                 LoraMode;
    char                LoraKey[33];
    int                 LoraDestination;     
    int                 LoraPower;        
    char                registration_code[9];
    int                 ACLFFThresh;
    int                 ACLFFDur;
    int                 ACLSTThresh;
    int                 ACLSTDur;
    int                 ACLACTThresh;
    int                 ClickSensor;
    int                 Sensor;
    int                 AirQualitySampleRate;
    int                 HumiditySampleRate;
    int                 MotionSampleRate;
    int                 ColorSampleRate;
    int                 AirQualitySensitivity;
    int                 HumiditySensitivity;
    int                 MotionSensitivity;
    int                 ColorSensitivity;
    char                DeviceName[20];
} SHG_CFG;


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

extern  uint32_t  const  DFLT_DNS_SERVER_IP;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  App_SHGWriteDefaultCfgToFile (char  *fname);

void  App_SHGWriteCfgToFile (char  *fname, SHG_CFG *p_cfg);

int  App_SHGCfgGet          (SHG_CFG  *p_cfg, char  *fname);

uint32_t  App_IPv4AddrParse  (CPU_CHAR  *ip_str);

const char *GetBSSID (void);

const char *GetIPAddr (void);


#endif /* APP_WIFI_UTIL_H_ */
