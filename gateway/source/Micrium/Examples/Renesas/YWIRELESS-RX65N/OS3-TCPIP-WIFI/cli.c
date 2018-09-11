#include <stdint.h>
#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>

#include <os.h>
#include <bsp_uart.h>

#include "cli.h"


int parse_creds_from_flash_copy(char * prov_ssid,
                           char * prov_ssid_password,
                           char * prov_mqtt_project_id,
                           char * prov_mqtt_user_id,
                           char * prov_api_key,
                           char * prov_password);

extern OS_FLAG_GRP sonar_grp;

enum CLI_STATE {
    CLI_MAIN = 0,
    CLI_SET_MQTT_PROJECT_ID,
    CLI_SET_MQTT_USER_ID,
    CLI_SET_API_KEY,
    CLI_SET_USER_PASSWORD,
    CLI_SET_SSID,
    CLI_SET_SSID_PSK,
    CLI_SET_ID,
    CLI_SET_NETWORK_KEY,
    CLI_SET_LORA_POWER,
    CLI_SET_MODE
};


static const char help_msg[] = "Supported commands:\r\n"
                                  "    help - display this message\r\n"
                                  "    ssid - modify the WiFi SSID\r\n"
                                  "    psk - modify the WiFi password\r\n"
                                  "    id - modify the Lora ID of this device\r\n"
                                  "    net - modify the Lora network key\r\n"
                                  "    power - modify the Lora power\r\n"
                                  "    mode - modify the Lora mode\r\n"
                                  "    prov - save all modifications\r\n"
                                  "    reg - display registration ID for linking on portal\r\n"                                    
                                  "    summ - display a summary of all settings\r\n";
static const char welcome_msg[] = "Hello!\r\n> ";
static const char proj_id_msg[] = "Enter MQTT Project ID (press Enter/Return for no change):\r\n";
static const char user_id_msg[] = "Enter MQTT User ID (press Enter/Return for no change):\r\n";
static const char api_key_msg[] = "Enter API Key (press Enter/Return for no change):\r\n";
static const char user_pw_msg[] = "Enter User Password (press Enter/Return for no change):\r\n";
static const char ssid_msg[] = "Enter SSID (press Enter/Return for no change):\r\n";
static const char psk_msg[] = "Enter SSID PSK (press Enter/Return for no change):\r\n";
static const char id_msg[] = "Enter Lora ID [0-255] (16) (press Enter/Return for no change):\r\n";
static const char net_key_msg[] = "Enter Lora Network Key [0-65535] (257) (press Enter/Return for no change):\r\n";
static const char power_msg[] = "Enter Lora Power [1-4] (3) (press Enter/Return for no change):\r\n";
static const char mode_msg[] = "Enter Lora Mode [1-10] (press Enter/Return for no change):\r\n";
static const char prov_msg[] = "Provisioning complete. Please push the RESET button or power cycle the board.\r\n";


extern volatile char g_cli_cmd[65];
extern volatile char g_cli_processing;


static int parse_cli_cmd(char * g_cli_cmd, config_t * p_cfg) {
    static enum CLI_STATE cli_state = CLI_MAIN;
    int prompt = 1;
    char temp[100];

    switch (cli_state) {
    case CLI_MAIN:
        if (!strcmp((const char *)g_cli_cmd, "help"))
            SCI_BSP_UART_WrRd((CPU_INT08U *)help_msg, NULL, sizeof(help_msg) - 1);
        else if (!strcmp((const char *)g_cli_cmd, "prov")) {
            return 0;
        } else if (!strcmp((const char *)g_cli_cmd, "ssid")) {
            SCI_BSP_UART_WrRd("Current value: ", NULL, strlen("Current value: "));
            SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->ssid, NULL, strlen(p_cfg->ssid));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)ssid_msg, NULL, sizeof(ssid_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_SSID;
        } else if (!strcmp((const char *)g_cli_cmd, "psk")) {
            SCI_BSP_UART_WrRd("Current value: ", NULL, strlen("Current value: "));
            SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->ssid_pw, NULL, strlen(p_cfg->ssid_pw));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)psk_msg, NULL, sizeof(psk_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_SSID_PSK;
        } else if (!strcmp((const char *)g_cli_cmd, "id")) {
            SCI_BSP_UART_WrRd("Current ID: ", NULL, strlen("Current ID: "));
            sprintf(temp, "%d", p_cfg->LoraID);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)id_msg, NULL, sizeof(id_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_ID;
        } else if (!strcmp((const char *)g_cli_cmd, "mode")) {
            SCI_BSP_UART_WrRd("Current mode: ", NULL, strlen("Current mode: "));
            sprintf(temp, "%d", p_cfg->LoraMode);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)mode_msg, NULL, sizeof(mode_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_MODE;
        } else if (!strcmp((const char *)g_cli_cmd, "net")) {
            SCI_BSP_UART_WrRd("Current network key: ", NULL, strlen("Current network key: "));
            sprintf(temp, "%hu", *((uint16_t *)p_cfg->LoraKey));
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)net_key_msg, NULL, sizeof(net_key_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_NETWORK_KEY;
        } else if (!strcmp((const char *)g_cli_cmd, "power")) {
            SCI_BSP_UART_WrRd("Current Power: ", NULL, strlen("Current Power: "));
            sprintf(temp, "%d", p_cfg->LoraPower);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)power_msg, NULL, sizeof(power_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_LORA_POWER;
        } else if (!strcmp((const char *)g_cli_cmd, "summ")) {
            SCI_BSP_UART_WrRd("Summary:", NULL, strlen("Summary:"));
            SCI_BSP_UART_WrRd("\r\nssid: ", NULL, strlen("\r\nssid: "));
            SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->ssid, NULL, strlen(p_cfg->ssid));

            SCI_BSP_UART_WrRd("\r\npsk: ", NULL, strlen("\r\npsk: "));
            SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->ssid_pw, NULL, strlen(p_cfg->ssid_pw));            
            
            SCI_BSP_UART_WrRd("\r\nid: ", NULL, strlen("\r\nid: "));
            sprintf(temp, "%d", p_cfg->LoraID);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));

            SCI_BSP_UART_WrRd("\r\nnetwork key: ", NULL, strlen("\r\nnetwork key: "));
            sprintf(temp, "%hu", *((uint16_t *)p_cfg->LoraKey));
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));

            SCI_BSP_UART_WrRd("\r\npower: ", NULL, strlen("\r\npower: "));
            sprintf(temp, "%d", p_cfg->LoraPower);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));

            SCI_BSP_UART_WrRd("\r\nmode: ", NULL, strlen("\r\nmode: "));
            sprintf(temp, "%d", p_cfg->LoraMode);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            
            SCI_BSP_UART_WrRd("\r\n", NULL, strlen("\r\n"));
        } else if (!strcmp((const char *)g_cli_cmd, "reg")) {
            SCI_BSP_UART_WrRd("Registration ID: ", NULL, strlen("Registration ID: "));

            SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->registration_code, NULL, strlen(p_cfg->registration_code));

            SCI_BSP_UART_WrRd("\r\n", NULL, strlen("\r\n"));
        } else {
            SCI_BSP_UART_WrRd("Unrecognized command\r\n", NULL, strlen("Unrecognized command\r\n"));
        }
        break;
    case CLI_SET_MQTT_PROJECT_ID:
        if (strlen((const char *)g_cli_cmd))
            strcpy(p_cfg->proj_id, (const char *)g_cli_cmd);
        SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->proj_id, NULL, strlen(p_cfg->proj_id));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_MQTT_USER_ID:
        if (strlen((const char *)g_cli_cmd))
            strcpy(p_cfg->user_id, (const char *)g_cli_cmd);
        SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->user_id, NULL, strlen(p_cfg->user_id));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_API_KEY:
        if (strlen((const char *)g_cli_cmd))
            strcpy(p_cfg->api_key, (const char *)g_cli_cmd);
        SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->api_key, NULL, strlen(p_cfg->api_key));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_USER_PASSWORD:
        if (strlen((const char *)g_cli_cmd))
            strcpy(p_cfg->pw, (const char *)g_cli_cmd);
        SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->pw, NULL, strlen(p_cfg->pw));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_SSID:
        if (strlen((const char *)g_cli_cmd))
            strcpy(p_cfg->ssid, (const char *)g_cli_cmd);
        SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->ssid, NULL, strlen(p_cfg->ssid));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_SSID_PSK:
        if (strlen((const char *)g_cli_cmd))
            strcpy(p_cfg->ssid_pw, (const char *)g_cli_cmd);
        SCI_BSP_UART_WrRd((CPU_INT08U *)p_cfg->ssid_pw, NULL, strlen(p_cfg->ssid_pw));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_ID:
        if (strlen((const char *)g_cli_cmd))
            sscanf(g_cli_cmd, "%d", &p_cfg->LoraID);
        sprintf(temp, "%d", p_cfg->LoraID);
        SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_MODE:
        if (strlen((const char *)g_cli_cmd))
            sscanf(g_cli_cmd, "%d", &p_cfg->LoraMode);
        sprintf(temp, "%d", p_cfg->LoraMode);
        SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_NETWORK_KEY:
        if (strlen((const char *)g_cli_cmd))
            sscanf(g_cli_cmd, "%hu", (uint16_t *)p_cfg->LoraKey);
        sprintf(temp, "%hu", *((uint16_t *)p_cfg->LoraKey));
        SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_LORA_POWER:
        if (strlen((const char *)g_cli_cmd))
            sscanf(g_cli_cmd, "%d", &p_cfg->LoraPower);
        sprintf(temp, "%d", p_cfg->LoraPower);
        SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    }
    if (prompt)
        SCI_BSP_UART_WrRd("> ", NULL, 2);
    g_cli_processing = 0;

    return -1;
}


void cli(config_t * p_cfg) {
    OS_ERR err;
    unsigned char prompt;
    char message[150];
    int uart_bytes_read;
    int cli_cmd_index = 0;
    char ssid[64] = {0};
    char ssid_pw[32] = {0};
    char api_key[49] = {0};
    char proj_id[12] = {0};
    char user_id[12] = {0};
    char pw[32] = {0};
    
    SCI_BSP_UART_WrRd((CPU_INT08U *)help_msg, NULL, sizeof(help_msg) - 1);
    SCI_BSP_UART_WrRd((CPU_INT08U *)welcome_msg, NULL, sizeof(welcome_msg) - 1);
    parse_creds_from_flash_copy(ssid, ssid_pw, proj_id, user_id, api_key, pw);
    while (1) {
        OSFlagPend(&sonar_grp, SONAR_GROUP_READING_READY, 0, OS_OPT_PEND_FLAG_SET_ALL | OS_OPT_PEND_FLAG_CONSUME | OS_OPT_PEND_BLOCKING, NULL, &err);
        if (err == OS_ERR_NONE) {
          if (!parse_cli_cmd((char *)g_cli_cmd, p_cfg))
            return;
        } else
            OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err);
    }
}
