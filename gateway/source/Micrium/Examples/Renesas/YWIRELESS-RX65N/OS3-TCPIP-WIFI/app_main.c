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
*                                            EXAMPLE CODE
*
* Filename : app_main.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdlib.h>
#include  <stdint.h>
#include  <string.h>
#include  <math.h>

#include  <cpu.h>
#include  <lib_mem.h>
#include  <lib_math.h>
#include  <os.h>
#include  <os_app_hooks.h>
#include  <app_cfg.h>
#include  <bsp.h>
#include  <bsp_led.h>
#include  <bsp_pb.h>
#include  <bsp_uart.h>
#include  <app_tcpip.h>

#include  <Source/clk.h>
#include  <Source/clk_type.h>

#include  <lora_gw.h>
#include  <aws_iot.h>
#include  <cli.h>
#include <r_flash_rx_if.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define byte uint8_t
#define PROVISION_START_ADDR FLASH_CF_BLOCK_27
#define PROVISION_END_ADDR FLASH_CF_BLOCK_26


OS_FLAG_GRP sonar_grp;

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

CLK_TASK_CFG Clk_CfgTask = {
  .Prio         = CLK_CFG_TASK_PRIO,
  .StkSizeBytes = CLK_CFG_TASK_STK_SIZE,
  .StkPtr       = DEF_NULL
};

static  OS_TCB   AppTaskStartTCB;
static  CPU_STK  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static const char prov_msg[] = "Provisioning complete. Please push the RESET button or power cycle the board.\r\n";

char g_provision_buf[sizeof(config_t) + 1];
extern config_t lora_config;


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  Heartbeat_Task (void);
static  void  AppTaskStart   (void  *p_arg);


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : none
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR  err;


    BSP_SystemInit();                                           /* Initialize the BSP for the RX65N                     */

    Mem_Init();                                                 /* Initialize Memory Managment Module                   */
    CPU_IntDis();                                               /* Disable all Interrupts.                              */

    OSInit(&err);                                               /* Init uC/OS-III.                                      */
    App_OS_SetAllHooks();

    OSTaskCreate(&AppTaskStartTCB,                              /* Create the start task                                */
                  "App Task Start",
                  AppTaskStart,
                  0u,
                  APP_CFG_TASK_START_PRIO,
                 &AppTaskStartStk[0u],
                  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                  APP_CFG_TASK_START_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &err);

    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */

    while (DEF_ON) {                                            /* Should Never Get Here.                               */
        ;
    }
}


char provision(char * src_addr) {
    volatile char * foo = (volatile char *)PROVISION_START_ADDR;
    flash_err_t err = FLASH_SUCCESS;
    flash_access_window_config_t faw_cfg = {.start_addr = PROVISION_START_ADDR, .end_addr = PROVISION_END_ADDR};

    err = R_FLASH_Open();
    if (err != FLASH_SUCCESS)
        return -1;
    err = R_FLASH_Erase(FLASH_CF_BLOCK_27, 1);
    if (err != FLASH_SUCCESS)
        return -1;
    err = R_FLASH_Write((uint32_t)src_addr, PROVISION_START_ADDR, 256);
    if (err != FLASH_SUCCESS)
        return -1;
    return *foo;
}

static int base32encode(byte * out, byte* in, long length)
{
  char base32StandardAlphabet[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"};
  char standardPaddingChar = '='; 

  int result = 0;
  int count = 0;
  int bufSize = 8;
  int index = 0;
  int size = 0; // size of temporary array
  byte* temp = NULL;

  if (length < 0 || length > 268435456LL) 
  { 
    return 0;
  }

  size = 8 * ceil(length / 4.0); // Calculating size of temporary array. Not very precise.
  temp = (byte*)malloc(size); // Allocating temporary array.

  if (length > 0)
  {
    int buffer = in[0];
    int next = 1;
    int bitsLeft = 8;
    
    while (count < bufSize && (bitsLeft > 0 || next < length))
    {
      if (bitsLeft < 5)
      {
        if (next < length)
        {
          buffer <<= 8;
          buffer |= in[next] & 0xFF;
          next++;
          bitsLeft += 8;
        }
        else
        {
          int pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      index = 0x1F & (buffer >> (bitsLeft -5));

      bitsLeft -= 5;
      temp[result] = (byte)base32StandardAlphabet[index];
      result++;
    }
  }

  memcpy(out, temp, result);
  out[result] = '\0';
  free(temp);
  
  return result;
}

static void get_registration_id(char * registration_code) {
    char regcode_p1[9], regcode_p2[9] = {0};
    uint16_t crc16[2];
    CRC.CRCCR.BYTE = 0x83;
    CRC.CRCDIR.BYTE = FLASHCONST.UIDR0 & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR0 >> 8) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR0 >> 16) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR0 >> 24) & 0x000000FF;
    CRC.CRCDIR.BYTE = FLASHCONST.UIDR1 & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR1 >> 8) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR1 >> 16) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR1 >> 24) & 0x000000FF;
    crc16[0] = CRC.CRCDOR.WORD;

    CRC.CRCCR.BYTE = 0x83;
    CRC.CRCDIR.BYTE = FLASHCONST.UIDR2 & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR2 >> 8) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR2 >> 16) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR2 >> 24) & 0x000000FF;
    CRC.CRCDIR.BYTE = FLASHCONST.UIDR3 & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR3 >> 8) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR3 >> 16) & 0x000000FF;
    CRC.CRCDIR.BYTE = (FLASHCONST.UIDR3 >> 24) & 0x000000FF;
    crc16[1] = CRC.CRCDOR.WORD;

    base32encode(regcode_p1, (uint8_t *)(&crc16[0]), 2);
    base32encode(regcode_p2, (uint8_t *)(&crc16[1]), 2);
    strcpy(registration_code, regcode_p1);
    strcat(registration_code, regcode_p2);    
}


int parse_creds_from_flash_copy(char * prov_ssid,
                           char * prov_ssid_password,
                           char * prov_mqtt_project_id,
                           char * prov_mqtt_user_id,
                           char * prov_api_key,
                           char * prov_password) {
    char * start, * end;
    
    memcpy(g_provision_buf, (char *)PROVISION_START_ADDR, sizeof(g_provision_buf));
    if (g_provision_buf[0] != 'P')
      return -1;
    memcpy(&lora_config, &g_provision_buf[1], sizeof(lora_config));
    get_registration_id(lora_config.registration_code);
    strcpy(prov_ssid, lora_config.ssid);
    strcpy(prov_ssid_password, lora_config.ssid_pw);
    strcpy(prov_mqtt_project_id, lora_config.proj_id);
    strcpy(prov_mqtt_user_id, lora_config.user_id);
    strcpy(prov_api_key, lora_config.api_key);
    strcpy(prov_password, lora_config.pw);
    return 0;
}


static int parse_creds_from_flash(char ** prov_ssid,
                           char ** prov_ssid_password,
                           char ** prov_mqtt_project_id,
                           char ** prov_mqtt_user_id,
                           char ** prov_api_key,
                           char ** prov_password) {
    char * dummy;
    
    memcpy(g_provision_buf, (char *)PROVISION_START_ADDR, sizeof(g_provision_buf));
    if (g_provision_buf[0] != 'P')
      return -1;
    memcpy(&lora_config, &g_provision_buf[1], sizeof(lora_config));
    get_registration_id(lora_config.registration_code);
    *prov_ssid = lora_config.ssid;
    *prov_ssid_password = lora_config.ssid_pw;
    *prov_mqtt_project_id = lora_config.proj_id;
    *prov_mqtt_user_id = lora_config.user_id;
    *prov_api_key = lora_config.api_key;
    *prov_password = lora_config.pw;
    return 0;
}

/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

char * g_prov_ssid;
char * g_prov_ssid_password;
char * mqtt_project_id;
char * mqtt_user_id;
char * api_key;
char * password;
extern char m1_mqtt_username[24];
extern char m1_mqtt_password[65];


static  void  AppTaskStart (void *p_arg)
{
    OS_ERR       err;
    CLK_ERR      err_clk;
    AWS_IOT_ERR  aws_iot_err;
    CPU_BOOLEAN  app_tcpip_init;
    CPU_CHAR     mac_addr[] = "00ABCDEF80AA";

    
   (void)p_arg;                                                 /* See note (1).                                        */

    BSP_Init();                                                 /* Initialize BSP functions                             */
    RSPI_BSP_SPI_Init();
    BSP_RSPI_General_Init();
    
        OSFlagCreate(&sonar_grp, "sonar group", 0, &err);

    if (BSP_PB_Read(BSP_PB_3)) {
        SCI_BSP_UART_init(0, 1);
        SCI_BSP_UART_SetCfg(9600, 0, 0, 0, 0);
        cli(&lora_config);
        g_provision_buf[0] = 'P';
        memcpy(&g_provision_buf[1], &lora_config, sizeof(lora_config));
        SCI7.SCR.BYTE = 0x00; // disable all interrupts and tx/rx
        IEN(CMT0, CMI0) = 0u;                                       /* Disable interrupt source.                             */      
        provision(g_provision_buf);
        SCI_BSP_UART_WrRd((CPU_INT08U *)prov_msg, NULL, sizeof(prov_msg) - 1);
        while (1);
    }

    if (parse_creds_from_flash(&g_prov_ssid, &g_prov_ssid_password, &mqtt_project_id, &mqtt_user_id, &api_key, &password))
        goto err_not_provisioned;

    // hardcoding to user "gateway"
    strcpy(mqtt_project_id, RNA_MQTT_PROJECT_ID);
    strcpy(mqtt_user_id, RNA_MQTT_USER_ID);
    strcpy(api_key, RNA_API_KEY);    
    strcpy(password, RNA_USER_PASSWORD);

    sprintf(m1_mqtt_username, "%s/%s", mqtt_project_id, mqtt_user_id);
    sprintf(m1_mqtt_password, "%s/%s", api_key, password);


    CPU_Init();                                                 /* Initialize the uC/CPU services                       */

    Mem_Init();                                                 /* Initialize Memory Managment Module                   */
    Math_Init();                                                /* Initialize Mathematical Module                       */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    Clk_Init(&Clk_CfgTask, DEF_NULL, &err_clk);                 /* Initialize uC/CLK                                    */

    app_tcpip_init = AppTCPIP_Init();                           /* Initialize uC/TCP-IP                                 */
    if (app_tcpip_init == DEF_FAIL) {
           SYSTEM.PRCR.WORD = 0xA502;  /* Enable writing to the Software Reset */

           SYSTEM.SWRR = 0xA501;            /* Software Reset */

           SYSTEM.PRCR.WORD = 0xA500;  /* Disable writing to the Software Reset */
      
        BSP_LED_On(BSP_LED_4_RED);
        BSP_LED_On(BSP_LED_3_RED);
        BSP_LED_On(BSP_LED_2_YELLOW);
        BSP_LED_On(BSP_LED_1_GREEN);
        while (1);
    }

    AppTCPIP_MacAddrDfltGet(&mac_addr[0]);                      /* Get the board's MAC address                          */

    AWS_IoT_Init(&mac_addr[0],                                  /* Initialize the AWS IoT module                        */
                 &aws_iot_err);
    if (aws_iot_err != AWS_IOT_ERR_NONE) {
        BSP_LED_Off(BSP_LED_4_RED);
        BSP_LED_On(BSP_LED_3_RED);
        BSP_LED_On(BSP_LED_2_YELLOW);
        BSP_LED_On(BSP_LED_1_GREEN);
        while (1);
    }

    AppInit();                                 /* Initialize the Smart Home Gateway application        */
    
err_not_provisioned:
    Heartbeat_Task();                                           /* Run the heartbeat task from our current task.        */
                                                                /* This call will NOT return                            */
}


/*
*********************************************************************************************************
*                                         HEARTBEAT TASK
*
* Description : This simple task simulates a heartbeat using LED 4.
*
* Arguments   : none.
*
* Returns     : none.
*
* Notes       : none.
*********************************************************************************************************
*/

static  void  Heartbeat_Task (void)
{
    OS_ERR  err;
    CPU_TS start_ts = OSTimeGet(&err);
            

    while (DEF_ON) {

        // workaround for wifi long term connectivity issue
        if ((OSTimeGet(&err) - start_ts) > (OS_CFG_TICK_RATE_HZ * (60 * 60))) {
           SYSTEM.PRCR.WORD = 0xA502;  /* Enable writing to the Software Reset */

           SYSTEM.SWRR = 0xA501;            /* Software Reset */

           SYSTEM.PRCR.WORD = 0xA500;  /* Disable writing to the Software Reset */
        }
        
        BSP_LED_On(BSP_LED_4_RED);
        OSTimeDly(125u, OS_OPT_TIME_DLY, &err);
        BSP_LED_Off(BSP_LED_4_RED);
        OSTimeDly(125u, OS_OPT_TIME_DLY, &err);
        BSP_LED_On(BSP_LED_4_RED);
        OSTimeDly(125u, OS_OPT_TIME_DLY, &err);
        BSP_LED_Off(BSP_LED_4_RED);
        OSTimeDly(1000u, OS_OPT_TIME_DLY, &err);
    }
}
