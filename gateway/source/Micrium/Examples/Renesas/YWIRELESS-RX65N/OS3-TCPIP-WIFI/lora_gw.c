#include  <app_cfg.h>
#include  <aws_iot.h>
#include  <cli.h>

#include  <cpu_core.h>
#include  <lib_def.h>
#include  <lib_str.h>
#include  <lib_mem.h>
#include  <os.h>

#include <stdint.h>
#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <time.h>

#include <bsp_uart.h>
#include  <bsp_led.h>


#define SUBSCRIBE_TOPICS 3
#define PRESENCE_DETECT_STACK_SIZE 256
#define PRESENCE_DETECT_APP_TASK_PRIO 18
#define PUB_DATA_LEN_MAX (AWS_IOT_MSG_LEN_MAX - 3)


#include <bsp_spi.h>
#include "sx1276.h"
#include "m1_bsp.h"


// IMPORTANT
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RADIO_RFM92_95
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 

// IMPORTANT
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// please uncomment only 1 choice
//#define BAND868
#define BAND900
///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// CHANGE HERE THE LORA MODE, NODE ADDRESS 
config_t lora_config;

static char subscribe_topics[SUBSCRIBE_TOPICS][AWS_IOT_TOPIC_LEN_MAX] =
{
    "1/%s/%s//#",
    "1/%s/%s/_all/#",
    "2/%s/control/#"
};

static char publish_topic[AWS_IOT_TOPIC_LEN_MAX] = "0/%s/%s/";


static  OS_TCB                  presenceDetectionTaskTCB;
static  CPU_STK                 presenceDetectionTaskStk[PRESENCE_DETECT_STACK_SIZE];

static  CPU_CHAR                event_buffer[512];


static int s_presence_cooldown_seconds = 2;
static volatile uint8_t g_presence_threshold = 30;
static volatile uint8_t g_presence_threshold_updated = 0;
extern volatile uint8_t current_sonar_reading;
extern OS_FLAG_GRP sonar_grp;
extern     char * mqtt_project_id;
extern    char * mqtt_user_id;
extern pack packet_received;
extern int16_t _RSSIpacket;
extern int8_t _SNR;


CPU_BOOLEAN  AWS_IoT_GetStatus (void);


static  void  presenceDetectionTask(void *p_arg);
static void  m1_subscribe(void);
static int m1_publish(CPU_CHAR* p_topic, CPU_CHAR* p_data);
static void publishCurrentDistance(void);
static void m1_message_receive(AWS_IOT_PAYLOAD *p_payload);

static int rspiSPIInit(portConfig * config) {
    RSPI_BSP_SPI_Init();
    return 0;
}


static int rspiSPIReadWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int len) {
    BSP_SPI_BUS_CFG cfg = {
        .SClkFreqMax = (CPU_INT32U)config->frequency,
        .CPOL = (CPU_INT08U)config->modeConfig.spi.polarity,
        .CPHA = (CPU_INT08U)config->modeConfig.spi.phase,
        .LSBFirst = DEF_NO,
        .BitsPerFrame = (CPU_INT08U)8,
    };

    // TODO: support CS toggle between bytes as config option
  //  BSP_SPI_Lock(0);
    BSP_SPI_Cfg(0, &cfg);
    BSP_SPI_ChipSel(0, 3, DEF_ON);
    // TODO: make this return a status so we can indicate errors (timeouts in RSPI while loops?)
    BSP_SPI_Xfer(0, txBuf, rxBuf, len);
    BSP_SPI_ChipSel(0, 3, DEF_OFF);
//    BSP_SPI_Unlock(0);
    return len;
}

static int rspiSPIGpioSet(portConfig * config, int pin, char high){
    if(high & 0x1)
        RSPI0_BSP_SPI_Data();
    else
        RSPI0_BSP_SPI_Command();

    return 0;
}

static portHandle rspi0 = {
  .init = &rspiSPIInit,
    .readWrite = &rspiSPIReadWrite,
    .readAndWrite = NULL,
    .gpioSet = &rspiSPIGpioSet
};
    
port lora = {
  .handle = &rspi0,
  .config = {
        .mode = MODE_SPI,
        .frequency = 2000000,
        .modeConfig.spi.polarity = 0,
        .modeConfig.spi.phase = 0
  }
};

static int setup(config_t * lora_config)
{
  int e;
  OS_ERR err_os;
 
  // Power ON the module
  if (sx1276_init(&lora))
    return -1;
  
  // Set transmission mode and print the result
  e = setMode(lora_config->LoraMode);
  if (e)
    return -2;

  // Setting the network key
  setNetworkKey(lora_config->LoraKey[0], lora_config->LoraKey[1]);

#ifdef BAND868
  // Select frequency channel
  e = setChannel(CH_10_868);
  if (e)
    return -3;
#else // assuming #defined BAND900
  // Select frequency channel
  e = setChannel(CH_05_900);
  if (e)
    return -4;
#endif

  // Select output power (eXtreme, Max, High or Low)
  e = setPower((lora_config->LoraPower == 1) ? 'L' : ((lora_config->LoraPower == 2) ? 'H' : ((lora_config->LoraPower == 3) ? 'M' : 'X')));
  if (e)
    return -5;
  
  // Set the node address and print the result
  e = setNodeAddress(lora_config->LoraID);
  if (e)
    return -6;
  
  OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_NONE, &err_os);
  return 0;
}
  
void  AppInit(void)
{
    OS_ERR      err;

    OSTaskCreate((OS_TCB     *)&presenceDetectionTaskTCB,
                 (CPU_CHAR   *)"Sonar Presence Detection",
                 (OS_TASK_PTR )presenceDetectionTask,
                 (void       *)0u,
                 (OS_PRIO     )PRESENCE_DETECT_APP_TASK_PRIO,
                 (CPU_STK    *)&presenceDetectionTaskStk[0],
                 (CPU_STK_SIZE)PRESENCE_DETECT_STACK_SIZE / 10,
                 (CPU_STK_SIZE)PRESENCE_DETECT_STACK_SIZE,
                 (OS_MSG_QTY  )10u,
                 (OS_TICK     ) 0u,
                 (void       *) 0u,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
}

static  void  presenceDetectionTask (void *p_arg)
{
    OS_ERR       err, err_ts;
    CPU_TS       last_update_ts, current_ts, presence_start, ts, pub_q_full_ts = 0, m1_conn_ts = 0;
    uint8_t last_sonar_reading = 0;
    uint8_t presence_detected = 0;
    uint8_t presence_distance = 0;
    int connect_count = 60;
    int e;
    OS_FLAGS value;

    (void)p_arg;

    while ((AWS_IoT_GetStatus() != DEF_OK) && (--connect_count > 0)) {
      m1_conn_ts = OSTimeGet(&err_ts);
        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_DLY, &err);
    }
    if (connect_count)
        m1_conn_ts = 0;

    m1_subscribe();

    sprintf(event_buffer,
            "\"event_data\":{\"connected\":true,\"Gateway_Reg_Code\":\"%s\",\"NID\":%d,\"GID\":%d},\"add_client_ip\":true",
            lora_config.registration_code,
            lora_config.LoraKey[0] + (lora_config.LoraKey[1] << 8),
            lora_config.LoraID);
    m1_publish(publish_topic, event_buffer);
    BSP_LED_Off(BSP_LED_2_YELLOW);
    

    // initialize lora gateway
    setup(&lora_config);
    
    last_update_ts = OSTimeGet(&err_ts);
    
    while (1) {
        e = receivePacketTimeout(10000);
        if (!e) {
            uint8_t checksum = 0, expected_checksum;
            sscanf(&packet_received.data[packet_received.length - OFFSET_PAYLOADLENGTH - 2], "%hhX", &expected_checksum);
            for (int i = 0; i < packet_received.length - OFFSET_PAYLOADLENGTH - 3; i++) {
                checksum ^= packet_received.data[i];
            }
            if (checksum == expected_checksum) {
                packet_received.data[packet_received.length - OFFSET_PAYLOADLENGTH - 4] = '\0';
                getRSSIpacket();
                getSNR();
                sprintf(event_buffer, "\"event_data\":{\"payload\":\"%s\",\"RSSI\":%d,\"SNR\":%d,\"NID\":%d,\"LID\":%d,\"GID\":%d,\"Gateway_Reg_Code\":\"%s\"}",
                        packet_received.data,
                        _RSSIpacket,
                        _SNR,
                        lora_config.LoraKey[0] + (lora_config.LoraKey[1] << 8),
                        packet_received.src,
                        lora_config.LoraID,
                        lora_config.registration_code
                        );
                  if (m1_publish(publish_topic, event_buffer) && !pub_q_full_ts)
                      pub_q_full_ts = OSTimeGet(&err_ts);
            }
        }

        if (AWS_IoT_GetStatus() == DEF_OK)
            m1_conn_ts = 0;
        else if (!m1_conn_ts)
            m1_conn_ts = OSTimeGet(&err_ts);
        else {
            if (m1_conn_ts && ((current_ts - m1_conn_ts) > (OS_CFG_TICK_RATE_HZ * (1 * 60)))) {
                 SYSTEM.PRCR.WORD = 0xA502;  /* Enable writing to the Software Reset */

                 SYSTEM.SWRR = 0xA501;            /* Software Reset */

                 SYSTEM.PRCR.WORD = 0xA500;  /* Disable writing to the Software Reset */ 
            }
        }

        value = OSFlagPend(&sonar_grp,
                   PUBLISH_QUEUE_FULL   + PUBLISH_QUEUE_NOT_FULL,
                   0,
                   OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME + OS_OPT_PEND_NON_BLOCKING,
                   &ts,
                   &err);
        if ((value & PUBLISH_QUEUE_FULL) && (!pub_q_full_ts))
            pub_q_full_ts = OSTimeGet(&err_ts);
        else if (value & PUBLISH_QUEUE_NOT_FULL)
            pub_q_full_ts = 0;
        else {
            if (pub_q_full_ts && ((current_ts - pub_q_full_ts) > (OS_CFG_TICK_RATE_HZ * (1 * 60)))) {
                 SYSTEM.PRCR.WORD = 0xA502;  /* Enable writing to the Software Reset */

                 SYSTEM.SWRR = 0xA501;            /* Software Reset */

                 SYSTEM.PRCR.WORD = 0xA500;  /* Disable writing to the Software Reset */ 
            }
        }
    }
}


static void publishCurrentDistance(void)
{
    CPU_INT32S   len = 0;

    len = sprintf(event_buffer,                      /* Create the JSON string to publish                    */
                  "\"event_data\":{\"current_distance\":%d}",
                  current_sonar_reading);

    if(len > 0)                                         /* If we created the string sucessfully, publish        */
    {
        m1_publish(publish_topic, event_buffer);
    }
}


static int m1_publish(CPU_CHAR* p_topic, CPU_CHAR* p_data)
{
    CPU_INT08U       len;
    CPU_CHAR        *p_json;
    AWS_IOT_PAYLOAD *p_payload;
    AWS_IOT_ERR      aws_iot_err;


    AWS_IoT_BufGet(&p_payload,                                  /* Grab a buffer for the JSON payload                   */
                   &aws_iot_err);
    if(p_payload == DEF_NULL)
    {
        return -1;
    }

    sprintf(p_payload->Topic,                                   /* Create the topic string                              */
            p_topic,
            mqtt_project_id,
            mqtt_user_id,
            "");

    p_payload->AWS_IoT_QoS = AWS_IOT_QOS_1;                     /* Set the QoS                                          */

    p_json = p_payload->Msg;                                    /* Start of the JSON string                             */
    *p_json++ = '{';

    len = Str_Len(p_data);                                      /* Copy the JSON data to the payload                    */
    if (len > PUB_DATA_LEN_MAX)
        return -2;
    Str_Copy(p_json, p_data);
    p_json += len;

    *p_json++ = '}';                                            /* Add the closing bracket and null terminator          */
    *p_json++ = '\0';

    AWS_IoT_Publish( p_payload,                                 /* Queue the AWS IoT Payload to send                    */
                    &aws_iot_err);
    if (aws_iot_err)
      return -3;

      return 0;
}


void  m1_subscribe(void)
{
    AWS_IOT_ERR  aws_iot_err;
    CPU_CHAR     sub_str[AWS_IOT_TOPIC_LEN_MAX];
    CPU_INT32U   i;
    OS_ERR err;


    for(i = 0; i < SUBSCRIBE_TOPICS; i++) {
        sprintf(&sub_str[0],
                subscribe_topics[i],
                mqtt_project_id,
                mqtt_user_id,
                "");

        AWS_IoT_Subscribe(&sub_str[0],
                           m1_message_receive,
                           AWS_IOT_QOS_1,
                          &aws_iot_err);
        OSTimeDlyHMSM(0, 0, 3, 0, OS_OPT_TIME_DLY, &err);
    }
}


void m1_message_receive(AWS_IOT_PAYLOAD *p_payload) {
    char * p = p_payload->Msg;

    while (p != NULL) {
        switch (p[0]) {
            default:
                break;
        }
        p = strchr(p, ';');
        if (p != NULL)
            p++;
    }
}
