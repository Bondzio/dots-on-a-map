#include  "app_wifi_util.h"
#include <os.h>

#include "cli.h"


enum CLI_STATE {
    CLI_MAIN = 0,
    CLI_SET_ID,
    CLI_SET_NETWORK_KEY,
    CLI_SET_DESTINATION,
    CLI_SET_TRANSMISSION_RATE,
    CLI_SET_LORA_POWER,
    CLI_SET_MODE
};


static const char help_msg[] = "Supported commands:\r\n"
                                  "    help - display this message\r\n"
                                  "    id - modify the Lora ID of this device\r\n"
                                  "    net - modify the Lora network key\r\n"
                                  "    dest - modify the Lora destination id\r\n"
                                  "    rate - modify the Lora transmission rate\r\n"
                                  "    power - modify the Lora power\r\n"
                                  "    mode - modify the Lora mode\r\n"
                                  "    prov - save all modifications\r\n"
                                  "    reg - display registration ID for linking on portal\r\n"                                    
                                  "    summ - display a summary of all settings\r\n";
static const char welcome_msg[] = "Hello!\r\n> ";
static const char id_msg[] = "Enter Lora ID [0-255] (16) (press Enter/Return for no change):\r\n";
static const char proj_id_msg[] = "Enter Lora Network Key [0-65535] (257) (press Enter/Return for no change):\r\n";
static const char user_id_msg[] = "Enter Lora Destination ID [0-255] (1) (press Enter/Return for no change):\r\n";
static const char api_key_msg[] = "Enter Transmission Rate (s) (60)(press Enter/Return for no change):\r\n";
static const char user_pw_msg[] = "Enter Lora Power [1-4] (3) (press Enter/Return for no change):\r\n";
static const char ssid_msg[] = "Enter Lora Mode [1-10] (press Enter/Return for no change):\r\n";
static const char prov_msg[] = "Provisioning complete. Please push the RESET button or power cycle the board to return to normal execution.\r\n";


int SCI_BSP_UART_WrRd(CPU_INT08U * data, CPU_INT08U * read, int bytes);
int                  DF_Program             (CPU_INT08U  *p_dst,
                                             CPU_INT08U  *p_src,
                                             CPU_SIZE_T   cnt_octets);
CPU_INT08U          *DF_BaseAddrGet         (void);
int  DF_ProgramEraseModeEnter (void);
int  DF_ProgramEraseModeExit (void);


static void parse_cli_cmd(char * g_cli_cmd, SHG_CFG * p_cfg) {
    static enum CLI_STATE cli_state = CLI_MAIN;
    int prompt = 1;
    char temp[50];
    uint8_t buf[sizeof(SHG_CFG) + 1];

    switch (cli_state) {
    case CLI_MAIN:
        if (!strcmp((const char *)g_cli_cmd, "help"))
            SCI_BSP_UART_WrRd((CPU_INT08U *)help_msg, NULL, sizeof(help_msg) - 1);
        else if (!strcmp((const char *)g_cli_cmd, "prov")) {
            buf[0] = 'P';
            memcpy(&buf[1], p_cfg, sizeof(SHG_CFG));
            DF_ProgramEraseModeEnter();
            DF_Program(DF_BaseAddrGet(), buf, sizeof(buf));
            DF_ProgramEraseModeExit();
            SCI_BSP_UART_WrRd((CPU_INT08U *)prov_msg, NULL, sizeof(prov_msg) - 1);
            return;
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
            SCI_BSP_UART_WrRd((CPU_INT08U *)ssid_msg, NULL, sizeof(ssid_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_MODE;
        } else if (!strcmp((const char *)g_cli_cmd, "net")) {
            SCI_BSP_UART_WrRd("Current network key: ", NULL, strlen("Current network key: "));
            sprintf(temp, "%hu", *((uint16_t *)p_cfg->LoraKey));
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)proj_id_msg, NULL, sizeof(proj_id_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_NETWORK_KEY;
        } else if (!strcmp((const char *)g_cli_cmd, "dest")) {
            SCI_BSP_UART_WrRd("Current Destination ID: ", NULL, strlen("Current Destination ID: "));
            sprintf(temp, "%d", p_cfg->LoraDestination);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)user_id_msg, NULL, sizeof(user_id_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_DESTINATION;
        } else if (!strcmp((const char *)g_cli_cmd, "rate")) {
            SCI_BSP_UART_WrRd("Current Transmission Rate: ", NULL, strlen("Current Transmission Rate: "));
            sprintf(temp, "%lu", p_cfg->LoopTimeSeconds);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)api_key_msg, NULL, sizeof(api_key_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_TRANSMISSION_RATE;
        } else if (!strcmp((const char *)g_cli_cmd, "power")) {
            SCI_BSP_UART_WrRd("Current Power: ", NULL, strlen("Current Power: "));
            sprintf(temp, "%d", p_cfg->LoraPower);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
            SCI_BSP_UART_WrRd("\r\n", NULL, 2);
            SCI_BSP_UART_WrRd((CPU_INT08U *)user_pw_msg, NULL, sizeof(user_pw_msg) - 1);
            prompt = 0;
            cli_state = CLI_SET_LORA_POWER;
        } else if (!strcmp((const char *)g_cli_cmd, "summ")) {
            SCI_BSP_UART_WrRd("Summary:", NULL, strlen("Summary:"));

            SCI_BSP_UART_WrRd("\r\nid: ", NULL, strlen("\r\nid: "));
            sprintf(temp, "%d", p_cfg->LoraID);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));

            SCI_BSP_UART_WrRd("\r\nnetwork key: ", NULL, strlen("\r\nnetwork key: "));
            sprintf(temp, "%hu", *((uint16_t *)p_cfg->LoraKey));
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));

            SCI_BSP_UART_WrRd("\r\ndestination: ", NULL, strlen("\r\ndestination: "));
            sprintf(temp, "%d", p_cfg->LoraDestination);
            SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));

            SCI_BSP_UART_WrRd("\r\nrate: ", NULL, strlen("\r\nrate: "));
            sprintf(temp, "%lu", p_cfg->LoopTimeSeconds);
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
    case CLI_SET_DESTINATION:
        if (strlen((const char *)g_cli_cmd))
            sscanf(g_cli_cmd, "%d", &p_cfg->LoraDestination);
        sprintf(temp, "%d", p_cfg->LoraDestination);
        SCI_BSP_UART_WrRd((CPU_INT08U *)temp, NULL, strlen(temp));
        SCI_BSP_UART_WrRd("\r\n", NULL, 2);
        cli_state = CLI_MAIN;
        break;
    case CLI_SET_TRANSMISSION_RATE:
        if (strlen((const char *)g_cli_cmd))
            sscanf(g_cli_cmd, "%lu", &p_cfg->LoopTimeSeconds);
        sprintf(temp, "%lu", p_cfg->LoopTimeSeconds);
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
}


void cli(SHG_CFG * p_cfg) {
    OS_ERR err;
    char ssid[64] = {0};
    char ssid_pw[32] = {0};
    char api_key[49] = {0};
    char proj_id[12] = {0};
    char user_id[12] = {0};
    char pw[32] = {0};
    unsigned char prompt;
    char message[150];
    int uart_bytes_read;
    char g_cli_cmd[128];
    int cli_cmd_index = 0;

    
    SCI_BSP_UART_WrRd((CPU_INT08U *)help_msg, NULL, sizeof(help_msg) - 1);
    SCI_BSP_UART_WrRd((CPU_INT08U *)welcome_msg, NULL, sizeof(welcome_msg) - 1);
    while (1) {
        uart_bytes_read = SCI_BSP_UART_WrRd(NULL, message, 150);
        if (uart_bytes_read > 0) {
            SCI_BSP_UART_WrRd(message, NULL, uart_bytes_read);
            for (int i = 0; i < uart_bytes_read; i++) {
              if (message[i] == '\r') {
                  SCI_BSP_UART_WrRd("\n", NULL, 1);
                  g_cli_cmd[cli_cmd_index++] = '\0';
                  cli_cmd_index = 0;
                  parse_cli_cmd(g_cli_cmd, p_cfg);
              } else
                  g_cli_cmd[cli_cmd_index++] = message[i];                
            }
        } else
            OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err);
    }
}

void cli_welcome() {
    SCI_BSP_UART_WrRd((CPU_INT08U *)help_msg, NULL, sizeof(help_msg) - 1);
    SCI_BSP_UART_WrRd((CPU_INT08U *)welcome_msg, NULL, sizeof(welcome_msg) - 1);
}

void cli_loop(SHG_CFG * p_cfg) {
    OS_ERR err;
    char message[150];
    int uart_bytes_read;
    static char g_cli_cmd[128];
    static int cli_cmd_index = 0;

    while (1) {
        uart_bytes_read = SCI_BSP_UART_WrRd(NULL, message, 150);
        if (uart_bytes_read > 0) {
            SCI_BSP_UART_WrRd(message, NULL, uart_bytes_read);
            for (int i = 0; i < uart_bytes_read; i++) {
              if (message[i] == '\r') {
                  SCI_BSP_UART_WrRd("\n", NULL, 1);
                  g_cli_cmd[cli_cmd_index++] = '\0';
                  cli_cmd_index = 0;
                  parse_cli_cmd(g_cli_cmd, p_cfg);
              } else
                  g_cli_cmd[cli_cmd_index++] = message[i];                
            }
        } else
          break;
    }
}