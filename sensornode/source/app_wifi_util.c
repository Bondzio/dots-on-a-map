  /*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                          (c) Copyright 2003-2015; Micrium, Inc.; Weston, FL
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
*                                        WiFi Utility Functions
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : JPC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdint.h>
#include  <ctype.h>
#include  <stdio.h>
#include  <fs_file.h>
#include  "app_wifi_util.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  SSID_FORMAT_STR               "%32s"                   /* Length specifier MUST match SSID_LEN_MAX.            */
#define  PASSPHRASE_FORMAT_STR         "%64s"                   /* Length specifier MUST match PASSPHRASE_LEN_MAX.      */

#define  AUTH_MODE_STR_OPEN            "open"
#define  AUTH_MODE_STR_OPEN_LEN        (sizeof(AUTH_MODE_STR_OPEN)-1)
#define  AUTH_MODE_STR_WPA             "wpa"
#define  AUTH_MODE_STR_WPA_LEN         (sizeof(AUTH_MODE_STR_WPA)-1)
#define  AUTH_MODE_STR_WPA2            "wpa2"
#define  AUTH_MODE_STR_WPA2_LEN        (sizeof(AUTH_MODE_STR_WPA2)-1)
#define  AUTH_MODE_STR_WPA_PSK         "wpa-psk"
#define  AUTH_MODE_STR_WPA_PSK_LEN     (sizeof(AUTH_MODE_STR_WPA_PSK)-1)
#define  AUTH_MODE_STR_WPA2_PSK        "wpa2-psk"
#define  AUTH_MODE_STR_WPA2_PSK_LEN    (sizeof(AUTH_MODE_STR_WPA2_PSK)-1)
#define  AUTH_MODE_STR_WPA_CCKM        "wpa-cckm"
#define  AUTH_MODE_STR_WPA_CCKM_LEN    (sizeof(AUTH_MODE_STR_WPA_CCKM)-1)
#define  AUTH_MODE_STR_WPA2_CCKM       "wpa2-cckm"
#define  AUTH_MODE_STR_WPA2_CCKM_LEN   (sizeof(AUTH_MODE_STR_WPA2_CCKM)-1)
#define  AUTH_MODE_STR_WPA2_PSK_SHA256 "wpa2-psk-sha256"
#define  AUTH_MODE_STR_WPA2_PSK_SHA256_LEN (sizeof(AUTH_MODE_STR_WPA2_PSK_SHA256)-1)

#define  AUTH_MODE_STR_LEN_MAX          15
#define  AUTH_MODE_FORMAT_STR          "%15s"                   /* Longest auth mode = wpa2-psk-sha256                  */

#define  CRYPT_TYPE_STR_WEP            "wep"
#define  CRYPT_TYPE_STR_WEP_LEN        (sizeof(CRYPT_TYPE_STR_WEP)-1)
#define  CRYPT_TYPE_STR_TKIP           "tkip"
#define  CRYPT_TYPE_STR_TKIP_LEN       (sizeof(CRYPT_TYPE_STR_TKIP)-1)
#define  CRYPT_TYPE_STR_AES            "aes"
#define  CRYPT_TYPE_STR_AES_LEN        (sizeof(CRYPT_TYPE_STR_AES)-1)
#define  CRYPT_TYPE_STR_WAPI           "wapi"
#define  CRYPT_TYPE_STR_WAPI_LEN       (sizeof(CRYPT_TYPE_STR_WAPI)-1)
#define  CRYPT_TYPE_STR_LEN_MAX         4                       /* Longest crypt type = TKIP                            */
#define  CRYPT_TYPE_FORMAT_STR         "%4s"                    /* Length specifier MUST match CRYPT_TYPE_STR_LEN_MAX.  */

#define  LINE_BUF_LEN                   260
#define  CFG_FILE_COMMENT_CHAR          '#'
#define  CFG_FILE_SSID_KEY_STR          "ssid:"
#define  CFG_FILE_SSID_KEY_STR_LEN      (sizeof(CFG_FILE_SSID_KEY_STR)-1)
#define  CFG_FILE_AUTH_KEY_STR          "auth:"
#define  CFG_FILE_AUTH_KEY_STR_LEN      (sizeof(CFG_FILE_AUTH_KEY_STR)-1)
#define  CFG_FILE_IP_KEY_STR            "ip:"
#define  CFG_FILE_IP_KEY_STR_LEN        (sizeof(CFG_FILE_IP_KEY_STR)-1)
#define  CFG_FILE_DNS_KEY_STR           "dns:"
#define  CFG_FILE_DNS_KEY_STR_LEN       (sizeof(CFG_FILE_DNS_KEY_STR)-1)
#define  CFG_FILE_HOST_KEY_STR          "host:"
#define  CFG_FILE_HOST_KEY_STR_LEN      (sizeof(CFG_FILE_HOST_KEY_STR)-1)
#define  CFG_FILE_PORT_KEY_STR          "port:"
#define  CFG_FILE_PORT_KEY_STR_LEN      (sizeof(CFG_FILE_PORT_KEY_STR)-1)
#define  CFG_FILE_USERNAME_KEY_STR      "un:"
#define  CFG_FILE_USERNAME_KEY_STR_LEN  (sizeof(CFG_FILE_USERNAME_KEY_STR)-1)
#define  CFG_FILE_PW_HASH_KEY_STR       "pw-hash:"
#define  CFG_FILE_PW_HASH_KEY_STR_LEN   (sizeof(CFG_FILE_PW_HASH_KEY_STR)-1)
#define  CFG_FILE_TOPIC_PUB_KEY_STR     "topic-publish:"
#define  CFG_FILE_TOPIC_PUB_KEY_STR_LEN (sizeof(CFG_FILE_TOPIC_PUB_KEY_STR)-1)
#define  CFG_FILE_TOPIC_SUB_KEY_STR     "topic-subscribe:"
#define  CFG_FILE_TOPIC_SUB_KEY_STR_LEN (sizeof(CFG_FILE_TOPIC_SUB_KEY_STR)-1)

#define  CFG_FILE_PERIPHERAL_PORT_KEY_STR     "peripheral-port:"
#define  CFG_FILE_PERIPHERAL_PORT_KEY_STR_LEN (sizeof(CFG_FILE_PERIPHERAL_PORT_KEY_STR)-1)
#define  CFG_FILE_HAS_WIFI_MODULE_STR       "has-wifi:"
#define  CFG_FILE_HAS_WIFI_MODULE_STR_LEN   (sizeof(CFG_FILE_HAS_WIFI_MODULE_STR)-1)

#define  DEMO_TYPE_STR_BTLE                "btle"
#define  DEMO_TYPE_STR_LORA                "lora"
#define  DEMO_TYPE_STR_ENV_SENSOR          "env-sensor"
#define  DEMO_TYPE_STR_ENV_GPS             "env-gps"
#define  DEMO_TYPE_STR_IOT_MONITORING             "iot-monitoring"
#define  DEMO_TYPE_STR_VIBRATION_DETECTION             "vibration-detection"
#define  DEMO_TYPE_STR_SMART_EM             "smart-em"
#define  CFG_FILE_DEMO_TYPE_STR            "demo-type:"
#define  CFG_FILE_DEMO_TYPE_STR_LEN        (sizeof(CFG_FILE_DEMO_TYPE_STR)-1)
#define  CFG_FILE_DEMO_TYPE_STR_LEN_MAX    20
#define  CFG_FILE_DEMO_TYPE_FORMAT_STR     "%19s"               /* Longest demo type = vibration-detection */

#define  BLE_MODE_STR_MASTER               "master"
#define  BLE_MODE_STR_MASTER_LEN           (sizeof(BLE_MODE_STR_MASTER)-1)
#define  BLE_MODE_STR_SLAVE                "slave"
#define  BLE_MODE_STR_SLAVE_LEN            (sizeof(BLE_MODE_STR_SLAVE)-1)
#define  CFG_FILE_BLE_MODE_STR             "ble-mode:"
#define  CFG_FILE_BLE_MODE_STR_LEN         (sizeof(CFG_FILE_BLE_MODE_STR)-1)
#define  CFG_FILE_BLE_KEY_STR             "ble-key:"
#define  CFG_FILE_BLE_KEY_STR_LEN         (sizeof(CFG_FILE_BLE_KEY_STR)-1)

#define  CFG_FILE_LORA_MODE_STR "lora-mode:"
#define  CFG_FILE_LORA_MODE_STR_LEN (sizeof(CFG_FILE_LORA_MODE_STR)-1)
#define  CFG_FILE_LORA_KEY_STR "lora-key:"
#define  CFG_FILE_LORA_KEY_STR_LEN (sizeof(CFG_FILE_LORA_KEY_STR)-1)

#define  CFG_FILE_ACL_FF_THRESH_KEY_STR "ff-thresh:"
#define  CFG_FILE_ACL_FF_THRESH_KEY_STR_LEN (sizeof(CFG_FILE_ACL_FF_THRESH_KEY_STR)-1)
#define  CFG_FILE_ACL_FF_DUR_KEY_STR "ff-dur:"
#define  CFG_FILE_ACL_FF_DUR_KEY_STR_LEN (sizeof(CFG_FILE_ACL_FF_DUR_KEY_STR)-1)
#define  CFG_FILE_ACL_ST_THRESH_KEY_STR "tap-thresh:"
#define  CFG_FILE_ACL_ST_THRESH_KEY_STR_LEN (sizeof(CFG_FILE_ACL_ST_THRESH_KEY_STR)-1)
#define  CFG_FILE_ACL_ST_DUR_KEY_STR "tap-dur:"
#define  CFG_FILE_ACL_ST_DUR_KEY_STR_LEN (sizeof(CFG_FILE_ACL_ST_DUR_KEY_STR)-1)
#define  CFG_FILE_ACL_ACT_THRESH_KEY_STR "activity-thresh:"
#define  CFG_FILE_ACL_ACT_THRESH_KEY_STR_LEN (sizeof(CFG_FILE_ACL_ACT_THRESH_KEY_STR)-1)

#define  CFG_FILE_CLICK_SENSOR_KEY_STR "click-sensor:"
#define  CFG_FILE_CLICK_SENSOR_KEY_STR_LEN (sizeof(CFG_FILE_CLICK_SENSOR_KEY_STR)-1)

#define  CFG_FILE_DEVICE_NAME_KEY_STR "device-name:"
#define  CFG_FILE_DEVICE_NAME_KEY_STR_LEN (sizeof(CFG_FILE_DEVICE_NAME_KEY_STR)-1)
#define  CFG_FILE_SENSOR_KEY_STR "sensor:"
#define  CFG_FILE_SENSOR_KEY_STR_LEN (sizeof(CFG_FILE_SENSOR_KEY_STR)-1)
#define  CFG_FILE_AIR_QUALITY_SENSOR_SAMPLE_RATE_KEY_STR "air-quality-sample-rate:"
#define  CFG_FILE_AIR_QUALITY_SENSOR_SAMPLE_RATE_KEY_STR_LEN (sizeof(CFG_FILE_AIR_QUALITY_SENSOR_SAMPLE_RATE_KEY_STR)-1)
#define  CFG_FILE_HUMIDITY_SENSOR_SAMPLE_RATE_KEY_STR "humidity-sample-rate:"
#define  CFG_FILE_HUMIDITY_SENSOR_SAMPLE_RATE_KEY_STR_LEN (sizeof(CFG_FILE_HUMIDITY_SENSOR_SAMPLE_RATE_KEY_STR)-1)
#define  CFG_FILE_MOTION_SENSOR_SAMPLE_RATE_KEY_STR "motion-sample-rate:"
#define  CFG_FILE_MOTION_SENSOR_SAMPLE_RATE_KEY_STR_LEN (sizeof(CFG_FILE_MOTION_SENSOR_SAMPLE_RATE_KEY_STR)-1)
#define  CFG_FILE_COLOR_SENSOR_SAMPLE_RATE_KEY_STR "color-sample-rate:"
#define  CFG_FILE_COLOR_SENSOR_SAMPLE_RATE_KEY_STR_LEN (sizeof(CFG_FILE_COLOR_SENSOR_SAMPLE_RATE_KEY_STR)-1)
#define  CFG_FILE_AIR_QUALITY_SENSOR_SENSITIVITY_KEY_STR "air-quality-sensitivity:"
#define  CFG_FILE_AIR_QUALITY_SENSOR_SENSITIVITY_KEY_STR_LEN (sizeof(CFG_FILE_AIR_QUALITY_SENSOR_SENSITIVITY_KEY_STR)-1)
#define  CFG_FILE_HUMIDITY_SENSOR_SENSITIVITY_KEY_STR "humidity-sensitivity:"
#define  CFG_FILE_HUMIDITY_SENSOR_SENSITIVITY_KEY_STR_LEN (sizeof(CFG_FILE_HUMIDITY_SENSOR_SENSITIVITY_KEY_STR)-1)
#define  CFG_FILE_MOTION_SENSOR_SENSITIVITY_KEY_STR "motion-sensitivity:"
#define  CFG_FILE_MOTION_SENSOR_SENSITIVITY_KEY_STR_LEN (sizeof(CFG_FILE_MOTION_SENSOR_SENSITIVITY_KEY_STR)-1)
#define  CFG_FILE_COLOR_SENSOR_SENSITIVITY_KEY_STR "color-sensitivity:"
#define  CFG_FILE_COLOR_SENSOR_SENSITIVITY_KEY_STR_LEN (sizeof(CFG_FILE_COLOR_SENSOR_SENSITIVITY_KEY_STR)-1)
#define  CFG_FILE_ACL_SAMPLE_PERIOD_KEY_STR "sample-period:"
#define  CFG_FILE_ACL_SAMPLE_PERIOD_KEY_STR_LEN (sizeof(CFG_FILE_ACL_SAMPLE_PERIOD_KEY_STR)-1)
#define  CFG_FILE_LORA_MODE_KEY_STR "lora-mode:"
#define  CFG_FILE_LORA_MODE_KEY_STR_LEN (sizeof(CFG_FILE_LORA_MODE_KEY_STR)-1)
#define  CFG_FILE_LORA_NETWORK_KEY_STR "lora-network:"
#define  CFG_FILE_LORA_NETWORK_KEY_STR_LEN (sizeof(CFG_FILE_LORA_NETWORK_KEY_STR)-1)
#define  CFG_FILE_LORA_DESTINATION_KEY_STR "lora-destination:"
#define  CFG_FILE_LORA_DESTINATION_KEY_STR_LEN (sizeof(CFG_FILE_LORA_DESTINATION_KEY_STR)-1)


#define  CFG_FILE_IP_ADDR_FORMAT_STR    "%15s"                  /* Length specifier MUST match IP_STR_LEN_MAX.          */

#define  NETWORK_CFG_TYPE_STR_LEN_MAX   (sizeof("static")-1)    /* Network cfg type will be either dhcp or static.      */
#define  NETWORK_CFG_TYPE_FORMAT_STR    "%6s"


#define  USERNAME_FORMAT_STR            "%254s"                 /* Length specifier MUST match USERNAME_LEN_MAX.        */
#define  PW_HASH_FORMAT_STR             "%80s"                  /* Length specifier MUST match PASSWORD_STR_LEN         */
#define  HOST_FORMAT_STR                "%253s"                 /* Length specifier MUST match HOST_NAME_LEN_MAX.       */
#define  PORT_FORMAT_STR                "%hu"                   /* Port number is an unsigned short (16-bits).          */
#define  TOPIC_FORMAT_STR               "%" TOPIC_STR_LEN_MAX_STR "s"
#define  BLE_KEY_FORMAT_STR             "%12s"                  /* Ble key MUST match BLE_KEY_STR_LEN           */

/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/


uint32_t  const  DFLT_DNS_SERVER_IP = 0x08080808;

static char BSSID_Str[13] = { '\0' };

static char IPAddr_Str[16] = { '\0' };

static  char  const  APP_SHG_CFG_DFLT_CONTENTS[] =
"#\r\n"
"# Application Configuration\r\n"
"#\r\n"
"# For more information on how to configure your Smart Home Garage, please visit\r\n"
"# mediumone.com/renesas/activate\r\n"
"#\r\n"
"#   NOTE: Hold down SW4 and press RESET to revert this file to factory settings\r\n"
"#\r\n"
"# WiFi Settings\r\n"
"#\r\n"
"# ssid: access-point-ssid\r\n"
"# auth: auth-type [crypto-type] [passphrase]\r\n"
"# ip: dhcp|static [ip-address subnet-mask gateway]\r\n"
"# [dns: [dns-ip] [dns-ip2] [dns-ip3]]\r\n"
"#\r\n"
"# Options:\r\n"
"#   ssid\r\n"
"#     The access point SSID is case-sensitive and must not exceed 32 characters.\r\n"
"#\r\n"
"#   auth\r\n"
"#     auth-type: open, wpa, wpa2, wpa-psk, wpa2-psk, wpa-cckm, wpa2-cckm,\r\n"
"#                wpa2-psk-sha256\r\n"
"#     crypto-type: wep, tkip, aes, wapi\r\n"
"#     passphrase: Case-sensitive. Must be 64 characters or fewer.\r\n"
"#\r\n"
"#   ip\r\n"
"#     dhcp|static\r\n"
"#     ip/subnet/gateway: Settings for static IP address configuration.\r\n"
"#                        Format: xxx.xxx.xxx.xxx\r\n"
"#\r\n"
"#   dns\r\n"
"#     Specify up to 3 DNS servers. Leave empty to use the default server.\r\n"
"#\r\n"
"# Examples:\r\n"
"#   Open WiFi using a dynamic IP and the default DNS server\r\n"
"#     ssid: linksys\r\n"
"#     auth: open\r\n"
"#     ip: dhcp\r\n"
"#   NOTE: Hold down SW4 and press RESET to revert this file to factory settings\r\n"
"#\r\n"
"#   WPA2 (AES) authentication using a static IP and the FreeDNS DNS server\r\n"
"#     ssid: MyAccessPoint\r\n"
"#     auth: wpa2 aes MySecretPassphrase\r\n"
"#     ip: static 10.10.1.123 255.255.255.0 10.10.1.1\r\n"
"#     dns: 37.235.1.174 37.235.1.177\r\n"
"\r\n"
"ssid: MyAccessPoint\r\n"
"auth: wpa2 aes MyPassphrase\r\n"
"ip: dhcp\r\n"
"\r\n"
"#\r\n"
"# MQTT Broker Settings\r\n"
"#\r\n"
"host: mqtt.mediumone.com\r\n"
"port: 61619\r\n"
"un: mqtt-project-id/mqtt-user-id\r\n"
"pw-hash: apikey/password\r\n"
"topic-publish: 0/mqtt-project-id/mqtt-user-id/device-id\r\n"
"topic-subscribe: 1/mqtt-project-id/mqtt-user-id/device-id/event\r\n";

const int  PERIPHERAL_PORTS_SUPPORTED[PERIPHERAL_PORTS_SUPPORTED_NUM] = {1, 3, 4};

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  int  App_WifiAuthModeParse          (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg);

static  int  App_WifiCryptTypeParse         (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg);

static  int  App_WifiCredentialsParse       (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg);

static  int  App_WifiNetworkCfgParse        (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg);

static  int  App_WifiDNSCfgParse            (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg);

static  int  App_DemoTypeParse              (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg);

static  int  App_PeripheralPortParse        (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg);

int  pow                                    (int base, int power);

int32_t  buffer_to_hex                      (char* buf);

/*
*********************************************************************************************************
*                                    EXTERNAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  App_SHGWriteDefaultCfgToFile (char  *fname)
{
    FS_FILE  *shg_cfg_file;
    FS_ERR    err;


    shg_cfg_file = FSFile_Open(fname, FS_FILE_ACCESS_MODE_CREATE | FS_FILE_ACCESS_MODE_TRUNCATE | FS_FILE_ACCESS_MODE_WR, &err);
    FSFile_Wr(shg_cfg_file, (void *)APP_SHG_CFG_DFLT_CONTENTS, Str_Len(APP_SHG_CFG_DFLT_CONTENTS), &err);
    FSFile_Close(shg_cfg_file, &err);
}


void  App_SHGWriteCfgToFile (char  *fname, SHG_CFG *p_cfg)
{
    FS_FILE  *shg_cfg_file;
    FS_ERR    err;
    size_t    shg_cfg_size = sizeof(SHG_CFG);


    shg_cfg_file = FSFile_Open(fname, FS_FILE_ACCESS_MODE_CREATE | FS_FILE_ACCESS_MODE_TRUNCATE | FS_FILE_ACCESS_MODE_WR, &err);
    FSFile_Wr(shg_cfg_file, (void *)p_cfg, shg_cfg_size, &err);
    FSFile_Close(shg_cfg_file, &err);
    FSFile_BufFlush(shg_cfg_file, &err);
}


void App_SHGReadFileToCfg (char *fname, SHG_CFG *p_cfg) {
    FS_ERR      err;
    FS_FILE    *cfg_file = FSFile_Open(SHG_CFG_LORA_FILE_PATH, FS_FILE_ACCESS_MODE_RD, &err);
    if (cfg_file == NULL) {
      Mem_Clr(p_cfg, sizeof(SHG_CFG));
      p_cfg->LoopTimeSeconds = 60;
      p_cfg->LoraMode = 5;
      p_cfg->LoraDestination = 1;
    } else
      FSFile_Rd(cfg_file, p_cfg, sizeof(SHG_CFG), &err);
}


//NOTE: This is not a thread-safe function.
int  App_SHGCfgGet (SHG_CFG  *p_cfg, char *fname)
{
    static  CPU_CHAR  LineBuf[LINE_BUF_LEN + 1];

    FS_FILE    *cfg_file;
    FS_ERR      err;
    CPU_CHAR   *p_buf;
    CPU_SIZE_T  nbr_rd;
    int         line_len;
    int         i;
    int         len;
    int         ret;


    Mem_Clr(p_cfg, sizeof(*p_cfg));

    cfg_file = FSFile_Open(fname, FS_FILE_ACCESS_MODE_RD, &err);
    if (cfg_file == DEF_NULL) {
#if 0
        App_SHGWriteDefaultCfgToFile(SHG_CFG_DFLT_FILE_PATH);
        cfg_file = FSFile_Open(fname, FS_FILE_ACCESS_MODE_RD, &err);
        if (cfg_file == DEF_NULL) {
            return -1;
        }
#else
        App_SHGReadFileToCfg(fname, p_cfg);
        return 0;
#endif
    }
    p_cfg->DemoType = DEMO_INVALID;
    p_cfg->LoopTimeSeconds = -1;

    ret = 0;                                                    /* Initialize return value.                             */

    do {
        Mem_Clr(&LineBuf[0], sizeof(LineBuf));
        p_buf  = &LineBuf[0];
        nbr_rd = FSFile_Rd(cfg_file, p_buf, LINE_BUF_LEN, &err);
        if (nbr_rd == 0) {
            break;
        }
        if (err != FS_ERR_NONE) {
            ret = -1;
            break;
        }
                                                                /* Find the beginning of the next line.                 */
        line_len = 0;
        while (line_len < nbr_rd && p_buf[line_len] != '\n') {
            ++line_len;
        }
        if (p_buf[line_len] != '\n' && nbr_rd == LINE_BUF_LEN) {/* Error: Line exceeds the buffer length.               */
            ret = -1;
            break;
        }
                                                                /* fseek to the beginning of the next line.             */
        FSFile_PosSet(cfg_file, (line_len - nbr_rd) + 1, FS_FILE_ORIGIN_CUR, &err);


        LineBuf[line_len + 1] = '\0';                           /* Make sure the line buffer is null-terminated.        */

        i = 0;
        do {
                                                                /* Scan to the first non-whitespace character.          */
            while (isspace(p_buf[i])) {
                ++i;
            }
                                                                /* Skip if this is a comment or empty line.             */
            if (p_buf[i] == CFG_FILE_COMMENT_CHAR || p_buf[i] == '\0') {
                break;
            }

                                                                /* If line starts with "ap:" then it contains the SSID. */
            if (Str_CmpIgnoreCase_N(CFG_FILE_SSID_KEY_STR, &p_buf[i], CFG_FILE_SSID_KEY_STR_LEN) == 0) {
                                                                /* Scan past the AP key string.                         */
                i += CFG_FILE_SSID_KEY_STR_LEN;
                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the SSID into p_cfg.                            */
                sscanf(&p_buf[i], SSID_FORMAT_STR, p_cfg->SSID_Str);
                i += Str_Len(p_cfg->SSID_Str);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_AUTH_KEY_STR, &p_buf[i], CFG_FILE_AUTH_KEY_STR_LEN) == 0) {

                i += CFG_FILE_AUTH_KEY_STR_LEN;
                                                                /* Parse Auth mode and store in p_cfg.                  */
                len = App_WifiAuthModeParse(&p_buf[i], p_cfg);
                if (len < 0) {
                    ret = -1;
                    break;
                }

                i += len;
                                                                /* Parse Crypto type and store in p_cfg.                */
                len = App_WifiCryptTypeParse(&p_buf[i], p_cfg);
                if (len < 0) {
                    ret = -1;
                    break;
                }

                i += len;
                                                                /* Parse Credentials and store in p_cfg.                */
                if ((p_cfg->AuthMode != WLAN_AUTH_NONE) ||  (p_cfg->CryptType == WLAN_CRYPT_WEP_CRYPT)){
                    len = App_WifiCredentialsParse(&p_buf[i], p_cfg);
                    if (len < 0) {
                        ret = -1;
                        break;
                    }

                    i += len;
                }

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_IP_KEY_STR, &p_buf[i], CFG_FILE_IP_KEY_STR_LEN) == 0) {

                i += CFG_FILE_IP_KEY_STR_LEN;

                len = App_WifiNetworkCfgParse(&p_buf[i], p_cfg);
                if (len < 0) {
                    ret = -1;
                    break;
                }

                i += len;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_DNS_KEY_STR, &p_buf[i], CFG_FILE_DNS_KEY_STR_LEN) == 0) {

                i += CFG_FILE_DNS_KEY_STR_LEN;

                len = App_WifiDNSCfgParse(&p_buf[i], p_cfg);
                if (len < 0) {
                    ret = -1;
                    break;
                }

                i += len;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_HOST_KEY_STR, &p_buf[i], CFG_FILE_HOST_KEY_STR_LEN) == 0) {

                i += CFG_FILE_HOST_KEY_STR_LEN;
                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the password md5 hash into p_cfg.               */
                sscanf(&p_buf[i], HOST_FORMAT_STR, p_cfg->HostStr);
                i += Str_Len(p_cfg->HostStr);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_PORT_KEY_STR, &p_buf[i], CFG_FILE_PORT_KEY_STR_LEN) == 0) {

                i += CFG_FILE_PORT_KEY_STR_LEN;
                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the password md5 hash into p_cfg.               */
                sscanf(&p_buf[i], PORT_FORMAT_STR, &p_cfg->Port);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_USERNAME_KEY_STR, &p_buf[i], CFG_FILE_USERNAME_KEY_STR_LEN) == 0) {

                i += CFG_FILE_USERNAME_KEY_STR_LEN;
                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the user name into p_cfg.                       */
                sscanf(&p_buf[i], USERNAME_FORMAT_STR, p_cfg->Username);
                i += Str_Len(p_cfg->Username);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_PW_HASH_KEY_STR, &p_buf[i], CFG_FILE_PW_HASH_KEY_STR_LEN) == 0) {

                i += CFG_FILE_PW_HASH_KEY_STR_LEN;
                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the password md5 hash into p_cfg.               */
                sscanf(&p_buf[i], PW_HASH_FORMAT_STR, p_cfg->Password);
                i += Str_Len(p_cfg->Password);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_TOPIC_PUB_KEY_STR, &p_buf[i], CFG_FILE_TOPIC_PUB_KEY_STR_LEN) == 0) {

                i += CFG_FILE_TOPIC_PUB_KEY_STR_LEN;
                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], TOPIC_FORMAT_STR, p_cfg->TopicPub);
                i += Str_Len(p_cfg->TopicSub);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_TOPIC_SUB_KEY_STR, &p_buf[i], CFG_FILE_TOPIC_SUB_KEY_STR_LEN) == 0) {

                i += CFG_FILE_TOPIC_SUB_KEY_STR_LEN;
                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], TOPIC_FORMAT_STR, p_cfg->TopicSub);
                i += Str_Len(p_cfg->TopicSub);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_PERIPHERAL_PORT_KEY_STR, &p_buf[i], CFG_FILE_PERIPHERAL_PORT_KEY_STR_LEN) == 0) {

                i += CFG_FILE_PERIPHERAL_PORT_KEY_STR_LEN;

                len = App_PeripheralPortParse(&p_buf[i], p_cfg);
                if (len < 0) {
                    ret = -1;
                    break;
                }

                i += len;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_HAS_WIFI_MODULE_STR, &p_buf[i], CFG_FILE_HAS_WIFI_MODULE_STR_LEN) == 0) {

                i += CFG_FILE_HAS_WIFI_MODULE_STR_LEN;

                while(isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }

                if(p_buf[i] == '0') p_cfg->HasWifiModule = 0;
                else if(p_buf[i] == '1') p_cfg->HasWifiModule = 1;
                i += 1;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_DEMO_TYPE_STR, &p_buf[i], CFG_FILE_DEMO_TYPE_STR_LEN) == 0) {

                i += CFG_FILE_DEMO_TYPE_STR_LEN;

                len = App_DemoTypeParse(&p_buf[i], p_cfg);
                if (len < 0) {
                    ret = -1;
                    break;
                }

                i += len;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_BLE_MODE_STR, &p_buf[i], CFG_FILE_BLE_MODE_STR_LEN) == 0) {

                i += CFG_FILE_BLE_MODE_STR_LEN;

                while(isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                char bleMode[4];

                sscanf(&p_buf[i], "%s", bleMode);
                if (!strcmp(bleMode, BLE_MODE_STR_MASTER))
                    p_cfg->BleMode = 1;
                else if (!strcmp(bleMode, BLE_MODE_STR_SLAVE))
                    p_cfg->BleMode = 0;
                else if (Str_Len(bleMode) == 0) continue;
                else {                            // ble mode is invalid
                    ret = -1;
                    break;
                }
                i += Str_Len(bleMode);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_BLE_KEY_STR, &p_buf[i], CFG_FILE_BLE_KEY_STR_LEN) == 0) {

                i += CFG_FILE_BLE_KEY_STR_LEN;

                while(isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }

                sscanf(&p_buf[i], BLE_KEY_FORMAT_STR, p_cfg->BleKey);
                i += Str_Len(p_cfg->BleKey);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_LORA_MODE_STR, &p_buf[i], CFG_FILE_LORA_MODE_STR_LEN) == 0) {

                i += CFG_FILE_LORA_MODE_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                char loraMode[4];
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%s", loraMode);
                if (!strcmp(loraMode, "GW"))
                    p_cfg->LoraMode = 1;
                else
                    p_cfg->LoraMode = 0;
                i += Str_Len(loraMode);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_LORA_KEY_STR, &p_buf[i], CFG_FILE_LORA_KEY_STR_LEN) == 0) {

                i += CFG_FILE_LORA_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%s", p_cfg->LoraKey);
                i += Str_Len(p_cfg->LoraKey);
                buffer_to_hex(p_cfg->LoraKey);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_ACL_FF_THRESH_KEY_STR, &p_buf[i], CFG_FILE_ACL_FF_THRESH_KEY_STR_LEN) == 0) {

                i += CFG_FILE_ACL_FF_THRESH_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &(p_cfg->ACLFFThresh), &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_ACL_FF_DUR_KEY_STR, &p_buf[i], CFG_FILE_ACL_FF_DUR_KEY_STR_LEN) == 0) {

                i += CFG_FILE_ACL_FF_DUR_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &(p_cfg->ACLFFDur), &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_ACL_ST_THRESH_KEY_STR, &p_buf[i], CFG_FILE_ACL_ST_THRESH_KEY_STR_LEN) == 0) {

                i += CFG_FILE_ACL_ST_THRESH_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &(p_cfg->ACLSTThresh), &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_ACL_ST_DUR_KEY_STR, &p_buf[i], CFG_FILE_ACL_ST_DUR_KEY_STR_LEN) == 0) {

                i += CFG_FILE_ACL_ST_DUR_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &(p_cfg->ACLSTDur), &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_ACL_ACT_THRESH_KEY_STR, &p_buf[i], CFG_FILE_ACL_ACT_THRESH_KEY_STR_LEN) == 0) {

                i += CFG_FILE_ACL_ACT_THRESH_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &(p_cfg->ACLACTThresh), &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_CLICK_SENSOR_KEY_STR, &p_buf[i], CFG_FILE_CLICK_SENSOR_KEY_STR_LEN) == 0) {

                i += CFG_FILE_CLICK_SENSOR_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                char clickSensor[20];
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%s%n", clickSensor, &j);
                i += j;
                if (Str_CmpIgnoreCase_N("air_quality", clickSensor, 11) == 0)
                    p_cfg->ClickSensor = 1;
                else if (Str_CmpIgnoreCase_N("humidity", clickSensor, 8) == 0)
                    p_cfg->ClickSensor = 2;
                else if (Str_CmpIgnoreCase_N("motion", clickSensor, 8) == 0)
                    p_cfg->ClickSensor = 4;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_SENSOR_KEY_STR, &p_buf[i], CFG_FILE_SENSOR_KEY_STR_LEN) == 0) {

                i += CFG_FILE_SENSOR_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                char sensor[20];
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%s%n", sensor, &j);
                i += j;
                if (Str_CmpIgnoreCase_N("air_quality", sensor, 11) == 0)
                    p_cfg->Sensor = 1;
                else if (Str_CmpIgnoreCase_N("humidity", sensor, 8) == 0)
                    p_cfg->Sensor = 2;
                else if (Str_CmpIgnoreCase_N("motion", sensor, 8) == 0)
                    p_cfg->Sensor = 4;
                else if (Str_CmpIgnoreCase_N("color", sensor, 8) == 0)
                    p_cfg->Sensor = 8;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_AIR_QUALITY_SENSOR_SAMPLE_RATE_KEY_STR, &p_buf[i], CFG_FILE_AIR_QUALITY_SENSOR_SAMPLE_RATE_KEY_STR_LEN) == 0) {

                i += CFG_FILE_AIR_QUALITY_SENSOR_SAMPLE_RATE_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->AirQualitySampleRate, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_HUMIDITY_SENSOR_SAMPLE_RATE_KEY_STR, &p_buf[i], CFG_FILE_HUMIDITY_SENSOR_SAMPLE_RATE_KEY_STR_LEN) == 0) {

                i += CFG_FILE_HUMIDITY_SENSOR_SAMPLE_RATE_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->HumiditySampleRate, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_MOTION_SENSOR_SAMPLE_RATE_KEY_STR, &p_buf[i], CFG_FILE_MOTION_SENSOR_SAMPLE_RATE_KEY_STR_LEN) == 0) {

                i += CFG_FILE_MOTION_SENSOR_SAMPLE_RATE_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->MotionSampleRate, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_COLOR_SENSOR_SAMPLE_RATE_KEY_STR, &p_buf[i], CFG_FILE_COLOR_SENSOR_SAMPLE_RATE_KEY_STR_LEN) == 0) {

                i += CFG_FILE_COLOR_SENSOR_SAMPLE_RATE_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->ColorSampleRate, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_AIR_QUALITY_SENSOR_SENSITIVITY_KEY_STR, &p_buf[i], CFG_FILE_AIR_QUALITY_SENSOR_SENSITIVITY_KEY_STR_LEN) == 0) {

                i += CFG_FILE_AIR_QUALITY_SENSOR_SENSITIVITY_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->AirQualitySensitivity, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_HUMIDITY_SENSOR_SENSITIVITY_KEY_STR, &p_buf[i], CFG_FILE_HUMIDITY_SENSOR_SENSITIVITY_KEY_STR_LEN) == 0) {

                i += CFG_FILE_HUMIDITY_SENSOR_SENSITIVITY_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->HumiditySensitivity, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_MOTION_SENSOR_SENSITIVITY_KEY_STR, &p_buf[i], CFG_FILE_MOTION_SENSOR_SENSITIVITY_KEY_STR_LEN) == 0) {

                i += CFG_FILE_MOTION_SENSOR_SENSITIVITY_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->MotionSensitivity, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_COLOR_SENSOR_SENSITIVITY_KEY_STR, &p_buf[i], CFG_FILE_COLOR_SENSOR_SENSITIVITY_KEY_STR_LEN) == 0) {

                i += CFG_FILE_COLOR_SENSOR_SENSITIVITY_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &p_cfg->ColorSensitivity, &j);
                i += j;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_DEVICE_NAME_KEY_STR, &p_buf[i], CFG_FILE_DEVICE_NAME_KEY_STR_LEN) == 0) {

                i += CFG_FILE_DEVICE_NAME_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%s%n", &(p_cfg->DeviceName), &j);
                i += j;
                
            } else if (Str_CmpIgnoreCase_N(CFG_FILE_ACL_SAMPLE_PERIOD_KEY_STR, &p_buf[i], CFG_FILE_ACL_SAMPLE_PERIOD_KEY_STR_LEN) == 0) {

                i += CFG_FILE_ACL_SAMPLE_PERIOD_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                float samplePeriod;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%f%n", &samplePeriod, &j);
                i += j;
                p_cfg->LoopTimeSeconds = (int)samplePeriod;
                p_cfg->LoopTimeMilliSeconds = (int)((samplePeriod - (int)samplePeriod) * 1000);

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_LORA_MODE_KEY_STR, &p_buf[i], CFG_FILE_LORA_MODE_KEY_STR_LEN) == 0) {

                i += CFG_FILE_LORA_MODE_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                int loraMode;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%d%n", &loraMode, &j);
                i += j;
                p_cfg->LoraMode = loraMode;

            } else if (Str_CmpIgnoreCase_N(CFG_FILE_LORA_NETWORK_KEY_STR, &p_buf[i], CFG_FILE_LORA_NETWORK_KEY_STR_LEN) == 0) {

                i += CFG_FILE_LORA_NETWORK_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                uint16_t lorakey;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%hu%n", &lorakey, &j);
                i += j;
                p_cfg->LoraKey[0] = lorakey & 0x00ff;
                p_cfg->LoraKey[1] = (lorakey >> 8) & 0x00ff;
                
            } else if (Str_CmpIgnoreCase_N(CFG_FILE_LORA_DESTINATION_KEY_STR, &p_buf[i], CFG_FILE_LORA_DESTINATION_KEY_STR_LEN) == 0) {

                i += CFG_FILE_LORA_DESTINATION_KEY_STR_LEN;

                                                                /* Scan past any whitespace.                            */
                while (isspace(p_buf[i])) {
                    ++i;
                    if (i == nbr_rd) {
                        break;
                    }
                }
                int j;
                unsigned int lora_destination;
                                                                /* Copy the topic string into p_cfg.                    */
                sscanf(&p_buf[i], "%u%n", &lora_destination, &j);
                i += j;
                p_cfg->LoraDestination = lora_destination;
                
            } else {
                ret = -1;                                       /* Invalid token in config file. Return an error.       */
                break;
            }
        } while (0);

        if (ret == -1) {                                        /* Return immediately if there was an error.            */
            break;
        }

    } while (DEF_TRUE);


    FSFile_Close(cfg_file, &err);

    return ret;
}


int  App_WifiAuthModeParse (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg)
{
    static  CPU_CHAR  AuthModeStr[AUTH_MODE_STR_LEN_MAX + 1];

    int  ret;


    ret = 0;

    while (isspace(*p_buf)) {                                   /* Scan to the first non-white space character.         */
        ++ret;
        ++p_buf;
    };

    sscanf(p_buf, AUTH_MODE_FORMAT_STR, AuthModeStr);
                                                                /* Select parsing string based on the auth mode.        */
    if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_OPEN, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_NONE;
    } else if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_WPA, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_WPA;
    } else if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_WPA2, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_WPA2;
    } else if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_WPA_PSK, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_WPA_PSK;
    } else if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_WPA2_PSK, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_WPA2_PSK;
    } else if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_WPA_CCKM, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_WPA_CCKM;
    } else if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_WPA2_CCKM, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_WPA2_CCKM;
    } else if (Str_CmpIgnoreCase_N(AuthModeStr, AUTH_MODE_STR_WPA2_PSK_SHA256, AUTH_MODE_STR_LEN_MAX) == 0) {
        p_cfg->AuthMode = WLAN_AUTH_WPA2_PSK_SHA256;
    } else {
        p_cfg->AuthMode = WLAN_AUTH_INVALID;
        ret = -1;
    }

    if (ret != -1) {
        ret += Str_Len(AuthModeStr);
    }

    return ret;
}


static  int  App_WifiCryptTypeParse (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg)
{
    static  CPU_CHAR  CryptTypeStr[CRYPT_TYPE_STR_LEN_MAX + 1];

    int  ret;

#if 0
                                                                /* Don't parse crypto type string if it's not needed.   */
    if (p_cfg->AuthMode == WLAN_AUTH_NONE) {
        p_cfg->CryptType = WLAN_CRYPT_NONE;
        return 0;
    }
#endif
    ret = 0;

    while (isspace(*p_buf)) {                                   /* Scan to the first non-white space character.         */
        ++ret;
        ++p_buf;
    };

    sscanf(p_buf, CRYPT_TYPE_FORMAT_STR, CryptTypeStr);
                                                                /* Select parsing string based on the auth mode.        */
    if (Str_CmpIgnoreCase_N(CryptTypeStr, CRYPT_TYPE_STR_WEP, CRYPT_TYPE_STR_LEN_MAX) == 0) {
      p_cfg->CryptType = WLAN_CRYPT_WEP_CRYPT;
    } else if (Str_CmpIgnoreCase_N(CryptTypeStr, CRYPT_TYPE_STR_TKIP, CRYPT_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->CryptType = WLAN_CRYPT_TKIP_CRYPT;
    } else if (Str_CmpIgnoreCase_N(CryptTypeStr, CRYPT_TYPE_STR_AES, CRYPT_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->CryptType = WLAN_CRYPT_AES_CRYPT;
    } else if (Str_CmpIgnoreCase_N(CryptTypeStr, CRYPT_TYPE_STR_WAPI, CRYPT_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->CryptType = WLAN_CRYPT_WAPI_CRYPT;
    } else {
        p_cfg->CryptType = WLAN_CRYPT_INVALID;
        ret = -1;
    }

    if (ret != -1) {
        ret += Str_Len(CryptTypeStr);
    }

    return ret;
}


int  App_WifiCredentialsParse(CPU_CHAR  *p_buf, SHG_CFG  *p_cfg)
{
    int ret;


    ret = 0;
    while (isspace(*p_buf)) {                                   /* Scan to the first non-white space character.         */
        ++ret;
        ++p_buf;
    };


    switch (p_cfg->AuthMode) {
            case WLAN_AUTH_WPA:
            case WLAN_AUTH_WPA2:
            case WLAN_AUTH_WPA_PSK:
            case WLAN_AUTH_WPA2_PSK:
            case WLAN_AUTH_WPA_CCKM:
            case WLAN_AUTH_WPA2_CCKM:
            case WLAN_AUTH_WPA2_PSK_SHA256:
            case WLAN_AUTH_NONE:
                 sscanf(p_buf, PASSPHRASE_FORMAT_STR, p_cfg->Passphrase);
                 break;
            case WLAN_AUTH_INVALID:
            default:
                 ret = -1;
                 break;
    }

    if (ret != -1) {
        ret += Str_Len(p_cfg->Passphrase);
    }

    return ret;
}


static  int  App_WifiNetworkCfgParse(CPU_CHAR  *p_buf, SHG_CFG  *p_cfg)
{
    static  char  NetworkCfgTypeStr[NETWORK_CFG_TYPE_STR_LEN_MAX + 1];
    static  char  IPAddr_Str[IP_STR_LEN_MAX + 1];

    int  len;
    int  ret;


    ret = 0;
    while (isspace(*p_buf)) {                                   /* Scan to the first non-white space character.         */
        ++ret;
        ++p_buf;
    };


    sscanf(p_buf, NETWORK_CFG_TYPE_FORMAT_STR, NetworkCfgTypeStr);

    if (Str_Cmp_N(NetworkCfgTypeStr, "static", NETWORK_CFG_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->UseDHCP = DEF_NO;

        ret   += sizeof("static");
        p_buf += sizeof("static");
                                                                /* Scan to the first non-white space character.         */
        while (isspace(*p_buf)) {
            ++ret;
            ++p_buf;
        };
                                                                /* Grab the IP, netmask, and gateway strings.           */
        sscanf(p_buf, CFG_FILE_IP_ADDR_FORMAT_STR, IPAddr_Str);
        p_cfg->IP_Addr = App_IPv4AddrParse(IPAddr_Str);         /* Save IP address in wifi configs.                     */
        len    = Str_Len(IPAddr_Str);
        p_buf += len;
        ret   += len;
        while (isspace(*p_buf)) {
            ++ret;
            ++p_buf;
        };

        sscanf(p_buf, CFG_FILE_IP_ADDR_FORMAT_STR, IPAddr_Str);
        p_cfg->Netmask = App_IPv4AddrParse(IPAddr_Str);         /* Save Netmask in wifi configs.                        */
        len = Str_Len(IPAddr_Str);
        p_buf += len;
        ret   += len;
        while (isspace(*p_buf)) {
            ++ret;
            ++p_buf;
        };

        sscanf(p_buf, CFG_FILE_IP_ADDR_FORMAT_STR, IPAddr_Str);
        p_cfg->Gateway = App_IPv4AddrParse(IPAddr_Str);         /* Save gateway in wifi configs.                        */
        ret += Str_Len(IPAddr_Str);

        if (p_cfg->Gateway == 0 || p_cfg->Netmask == 0 || p_cfg->IP_Addr == 0) {
            ret = -1;
        }
    } else if (Str_Cmp_N(NetworkCfgTypeStr, "dhcp", NETWORK_CFG_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->UseDHCP = DEF_YES;

        ret += sizeof("dhcp");
    } else {
        ret = -1;
    }

    return ret;
}


static  int  App_WifiDNSCfgParse(CPU_CHAR  *p_buf, SHG_CFG  *p_cfg)
{
    static  char  IPAddr_Str[IP_STR_LEN_MAX + 1];

    int       len;
    int       nbr_str;
    int       i;
    int       ret;
    uint32_t  ip_addr;


    ret     = 0;
    i       = 0;
    nbr_str = sscanf(p_buf, CFG_FILE_IP_ADDR_FORMAT_STR, IPAddr_Str);

                                                                /* Process DNS server list.                             */
    while (nbr_str == 1 && i < NBR_DNS_SERVERS) {
        while (isspace(*p_buf)) {                               /* Scan to the first non-white space character.         */
            ++ret;
            ++p_buf;
        };

        ip_addr = App_IPv4AddrParse(IPAddr_Str);
        if (ip_addr == 0) {
            ret = -1;
            break;
        }

        p_cfg->DNS_Servers[i] = ip_addr;

        len    = Str_Len(IPAddr_Str);
        ret   += len;
        p_buf += len;


        ++i;
        nbr_str = sscanf(p_buf, CFG_FILE_IP_ADDR_FORMAT_STR, IPAddr_Str);
    }


    return ret;
}

static  int  App_PeripheralPortParse(CPU_CHAR  *p_buf, SHG_CFG  *p_cfg)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int port;
    char buf[128];

    while (isspace(p_buf[i]))
        ++i;

    // parse port number
    sscanf(&p_buf[i], "%i%n", &k, &port);
    i += port;
    port = -1;
    for(j = 0; j < PERIPHERAL_PORTS_SUPPORTED_NUM; j++) {
        if(k == PERIPHERAL_PORTS_SUPPORTED[j]) {
            port = j;
            break;
        }
    }
    if(port == -1)
        return -1;

    while (isspace(p_buf[i]))
        ++i;

    // parse port type
    j = 0;
    while(!isspace(p_buf[i]) && (p_buf[i] != '\0')) {
        buf[j++] = p_buf[i++];
    }
    buf[j] = '\0';
    if(!Str_CmpIgnoreCase_N(buf, "spi", j + 1))
        p_cfg->Ports[port].config.mode = MODE_SPI;
    else if(!Str_CmpIgnoreCase_N(buf, "i2c", j + 1))
        p_cfg->Ports[port].config.mode = MODE_I2C;
    else if(!Str_CmpIgnoreCase_N(buf, "uart", j + 1))
        p_cfg->Ports[port].config.mode = MODE_UART;

    while (isspace(p_buf[i]))
        ++i;

    // parse port frequency
    j = 0;
    while(!isspace(p_buf[i]) && (p_buf[i] != '\0')) {
        buf[j++] = p_buf[i++];
    }
    buf[j--] = '\0';
    k = 0;
    p_cfg->Ports[port].config.frequency = 0;
    for (; j >= 0; --j, ++k) {
        p_cfg->Ports[port].config.frequency += (buf[j] - '0') * pow(10, k);
    }
    if (p_cfg->Ports[port].config.mode == MODE_SPI) {
        // parse polarity if specified
        // default to 0/0
        p_cfg->Ports[port].config.modeConfig.spi.polarity = 0;
        p_cfg->Ports[port].config.modeConfig.spi.phase = 0;
        j = sscanf(&p_buf[i], " %d %d", &p_cfg->Ports[port].config.modeConfig.spi.polarity, &p_cfg->Ports[port].config.modeConfig.spi.phase);
    }

    return 0;
}


static  int  App_DemoTypeParse (CPU_CHAR  *p_buf, SHG_CFG  *p_cfg)
{
    static  CPU_CHAR  DemoTypeStr[CFG_FILE_DEMO_TYPE_STR_LEN_MAX + 1];

    int  ret;
    char buf[64];

    ret = 0;

    while (isspace(*p_buf)) {                                   /* Scan to the first non-white space character.         */
        ++ret;
        ++p_buf;
    };

    sscanf(p_buf, CFG_FILE_DEMO_TYPE_FORMAT_STR, DemoTypeStr);
                                                                /* Select parsing string based on the auth mode.        */
    if (Str_CmpIgnoreCase_N(DemoTypeStr, DEMO_TYPE_STR_BTLE, CFG_FILE_DEMO_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->DemoType = DEMO_BTLE;
    } else if (Str_CmpIgnoreCase_N(DemoTypeStr, DEMO_TYPE_STR_LORA, CFG_FILE_DEMO_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->DemoType = DEMO_LORA;
    } else if (Str_CmpIgnoreCase_N(DemoTypeStr, DEMO_TYPE_STR_ENV_SENSOR, CFG_FILE_DEMO_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->DemoType = DEMO_ENV_SENSOR;
    } else if (Str_CmpIgnoreCase_N(DemoTypeStr, DEMO_TYPE_STR_ENV_GPS, CFG_FILE_DEMO_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->DemoType = DEMO_ENV_GPS;
    } else if (Str_CmpIgnoreCase_N(DemoTypeStr, DEMO_TYPE_STR_IOT_MONITORING, CFG_FILE_DEMO_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->DemoType = DEMO_IOT_MONITORING;
    } else if (Str_CmpIgnoreCase_N(DemoTypeStr, DEMO_TYPE_STR_VIBRATION_DETECTION, CFG_FILE_DEMO_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->DemoType = DEMO_VIBRATION_DETECTION;
    } else if (Str_CmpIgnoreCase_N(DemoTypeStr, DEMO_TYPE_STR_SMART_EM, CFG_FILE_DEMO_TYPE_STR_LEN_MAX) == 0) {
        p_cfg->DemoType = DEMO_SMART_EM;
    } else {
        p_cfg->DemoType = DEMO_INVALID;
        // ret = -1;
    }

    if (ret != -1) {
        ret += Str_Len(DemoTypeStr);
        p_buf += Str_Len(DemoTypeStr);
    }

    while (isspace(*p_buf)) {
        ++ret;
        ++p_buf;
    };

    // parse loop time seconds
    int j = 0;
    while(!isspace(*p_buf) && (*p_buf != '\0')) {
        buf[j++] = *p_buf++;
        ++ret;
    }
    buf[j--] = '\0';
    int k = 0;
    uint32_t * loopTimeComp = &(p_cfg->LoopTimeSeconds);
    for (; j >= 0; --j, ++k) {
        if (buf[j] == '.') {
            loopTimeComp = &(p_cfg->LoopTimeMilliSeconds);
            continue;
        }
        *loopTimeComp += (buf[j] - '0') * pow(10, k);
    }

    if (!p_cfg->LoopTimeSeconds && !p_cfg->LoopTimeMilliSeconds)
        p_cfg->LoopTimeMilliSeconds = 1;

    return ret;
}

uint32_t  App_IPv4AddrParse  (char  *ip_str)
{
    uint8_t  octets[4];
    int      ret;


    ret = sscanf(ip_str, "%hhu.%hhu.%hhu.%hhu", &octets[3], &octets[2], &octets[1], &octets[0]);

    if (ret != 4) {
        return 0;
    } else {
        return octets[0] | octets[1] << 8 | octets[2] << 16 | octets[3] << 24;
    }
}


const char *GetBSSID (void)
{
    if (! BSSID_Str[0]) {
        A_UINT8     bssid[ATH_MAC_LEN + 1];

        qcom_get_bssid(0, &bssid[0]);
                                                                    /* Convert BSSID to ASCII string.                       */
        for (int i = 0, j = 0; i < ATH_MAC_LEN; ++i, j+=2) {
            sprintf(&BSSID_Str[j], "%02X", bssid[i]);
        }
    }

    return &BSSID_Str[0];
}


const char *GetIPAddr (void)
{
    A_UINT32 ip;
    A_UINT32 sn;
    A_UINT32 gtw;
    A_STATUS status;

    status = qcom_ipconfig(0, IPCFG_QUERY, &ip , &sn, &gtw);
    if (status == A_OK) {
        sprintf(IPAddr_Str, "%d.%d.%d.%d", (uint8_t)(ip >> 24), /* Print each octet of the IP address.              */
                                           (uint8_t)(ip >> 16),
                                           (uint8_t)(ip >>  8),
                                           (uint8_t)(ip >>  0));
    }

    return &IPAddr_Str[0];
}

int pow (int base, int power) {
    int res = base;
    if (!power)
        return 1;
    for(; power > 1; --power)
        res *= base;
    return res;
}


int32_t  buffer_to_hex (char* buf)
{
    // only valid characters are 0:9,a:f,a:f
    uint16_t i;
    uint16_t num_chars = strlen(buf);

                    // odd length string check
    if (num_chars & 0x01)
    {
        return -1;
    }

    for(i=0; i<num_chars; i++)
    {
        char c = toupper(buf[i]);   // convert to all uppercase
        uint8_t x;

        // 0:9
        if (isdigit(c))
        {
            x = c-'0';
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            x = c-'A'+10;
        }
        else            // invalid character check
        {
            return -2;
        }

        // pack the buffer
        if (i & 0x01)
        {
            buf[(i-1)/2] |= x;
        }
        else
        {
            buf[i/2] = x<<4;
        }
    }
                        // null terminate the string
    buf[num_chars/2] = '\0';
    return num_chars/2;
}
