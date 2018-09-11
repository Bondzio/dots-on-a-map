/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2016; Micrium, Inc.; Weston, FL
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
* Filename      : app.c
* Version       : V1.01
* Programmer(s) : JPC
*                 MTM
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/


#include  <stdlib.h>
#include  <math.h>

#include  <os.h>
#include  <lib_mem.h>
#include  <Source/usbd_core.h>
#include  <bsp_spi.h>
#include  <bsp_sys.h>
#include  <bsp_led.h>
#include  <bsp_switch.h>
#include  <qca_bsp.h>
#include  <probe_com.h>
#include  <probe_tcpip.h>

#include  "app_m1.h"
#include  "app_wifi.h"
#include  "app_wifi_util.h"
#include  "app_util.h"
#include  "cli.h"
#include  "app_lora_node.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  SHG_ERR_CFG                                1u
#define  SHG_ERR_WIFI                               2u
#define  SHG_ERR_SHG                                3u
#define  SHG_ERR_AMS                                4u

#define byte uint8_t

/*
*********************************************************************************************************
*                                               CONSTANTS                                               *
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               DATA TYPES                                              *
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES                                          *
*********************************************************************************************************
*/

int PMODInit(SHG_CFG *);
void SCI0_BSP_UART_init(char hwFC);
void  SCI0_BSP_UART_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir);
void SCI_BSP_UART_init(char hwFC);
void SCI_BSP_UART_SetCfg(long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir);
CPU_INT08U          *DF_BaseAddrGet         (void);


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES                                           *
*********************************************************************************************************
*/

static  SHG_CFG     SHG_Cfg;


/*
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS                                            *
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LOCAL FUNCTIONS                                            *
*********************************************************************************************************
*/
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


/*
*********************************************************************************************************
*                                              main()
*
* Description : Main point of entry for the Application. Called from MainTask() in sys_init.c
*
* Arguments   : none.
*
* Returns     : Should never return
*
* Notes       : This function is called in a way not normal to most Micrium projects. Before entering
*               main() the OS has been started and we are running out of MainTask found in sys_init.c.
*
*********************************************************************************************************
*/

int  main (int  argc, char  *argv[])
{
    USBD_ERR     err_usb;
    int          shg_cfg_err;
    CPU_BOOLEAN  ret_val;
    CPU_INT32S   shg_ret_val = SHG_ERR_AMS;

    do {
#if 0
        USBD_DevStart(0, &err_usb);

                                                                /* Get device configs from storage.                     */
        shg_cfg_err = App_SHGCfgGet(&SHG_Cfg, SHG_CFG_DFLT_FILE_PATH);
        if (shg_cfg_err) {
            shg_ret_val = SHG_ERR_CFG;
            break;                                              /* No config file                                       */
        }
#else
        CPU_INT08U * base_address = DF_BaseAddrGet();
        unsigned char b = base_address[0];
        // TODO: simple checksum instead of character check?
        if (b == 'P')
            memcpy(&SHG_Cfg, &base_address[1], sizeof(SHG_Cfg));
        else {
            memset(&SHG_Cfg, 0, sizeof(SHG_Cfg));
            // setup default values
            SHG_Cfg.LoopTimeSeconds = 60;
            SHG_Cfg.LoraID = 0x10;
            SHG_Cfg.LoraDestination = 0x01;
            SHG_Cfg.LoraKey[0] = 0x01;
            SHG_Cfg.LoraKey[1] = 0x01;
            SHG_Cfg.LoraMode = 5;
            SHG_Cfg.LoraPower = 2;
        }
#endif

        // setup sane values
        if ((SHG_Cfg.LoraID < 0) || (SHG_Cfg.LoraID > 255))
            SHG_Cfg.LoraID = 0x10;
        if ((SHG_Cfg.LoraDestination < 0) || (SHG_Cfg.LoraDestination > 255))
            SHG_Cfg.LoraDestination = 1;
        if ((SHG_Cfg.LoraMode <= 0) || (SHG_Cfg.LoraMode > 10))
            SHG_Cfg.LoraMode = 5;
        if ((SHG_Cfg.LoraPower < 1) || (SHG_Cfg.LoraPower > 4))
            SHG_Cfg.LoraPower = 2;

        // hardcode for vibration
        SHG_Cfg.HasWifiModule = 1;
        SHG_Cfg.Ports[PORT_PMOD3].config.mode = MODE_SPI;
        
        SCI0_BSP_UART_init(0);
        SCI0_BSP_UART_SetCfg(4800, 0, 0, 0, 0);
        SCI_BSP_UART_init(0);
        SCI_BSP_UART_SetCfg(9600, 0, 0, 0, 0);

        PMODInit(&SHG_Cfg);

        char regcode_p1[9], regcode_p2[9] = {0};
        uint16_t crc16[2];
        CRC.CRCCR.BYTE = 0x83;
        CRC.CRCDIR = FLASHCONST.UIDR0 & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR0 >> 8) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR0 >> 16) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR0 >> 24) & 0x000000FF;
        CRC.CRCDIR = FLASHCONST.UIDR1 & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR1 >> 8) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR1 >> 16) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR1 >> 24) & 0x000000FF;
        crc16[0] = CRC.CRCDOR;

        CRC.CRCCR.BYTE = 0x83;
        CRC.CRCDIR = FLASHCONST.UIDR2 & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR2 >> 8) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR2 >> 16) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR2 >> 24) & 0x000000FF;
        CRC.CRCDIR = FLASHCONST.UIDR3 & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR3 >> 8) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR3 >> 16) & 0x000000FF;
        CRC.CRCDIR = (FLASHCONST.UIDR3 >> 24) & 0x000000FF;
        crc16[1] = CRC.CRCDOR;

        base32encode(regcode_p1, (uint8_t *)(&crc16[0]), 2);
        base32encode(regcode_p2, (uint8_t *)(&crc16[1]), 2);
        strcpy(SHG_Cfg.registration_code, regcode_p1);
        strcat(SHG_Cfg.registration_code, regcode_p2);
        

#if 0        
        if (BSP_SwitchRd(2)) {
            cli(&SHG_Cfg);
        }

        // TODO: pre wifi init here
        App_PreWifiInit();
        
        if (SHG_Cfg.HasWifiModule) {
            WIFIInit(&SHG_Cfg);
            ret_val = App_WiFi_Init(&SHG_Cfg);                      /* Initialize WiFi. MUST be called before SHG init      */
            if(ret_val != DEF_OK) {
                shg_ret_val = SHG_ERR_WIFI;
                break;
            }
        }

#endif
        
        ret_val = App_SHG_Init(&SHG_Cfg);                       /* Initialize the Smart Home Garage application         */
        if(ret_val != DEF_OK) {
            shg_ret_val = SHG_ERR_SHG;
            break;
        }

        App_LoraNode(&SHG_Cfg);                                      /* Run the AMS application. Should not return!          */
    } while(0);
                                                                /* Should not reach this point. If we return from main  */
    return shg_ret_val;                                         /* we fall back into MainTask() in sys_init.c           */
}

/*
*********************************************************************************************************
*                                            PMODInit()
*
* Description : Initialize the PMOD connections to the GT-202 and the AMS sensor
*
* Arguments   : none.
*
* Returns     : Returns 0 on success
*
* Notes       : none.
*
*********************************************************************************************************
*/


int PMODInit(SHG_CFG *pSHG_Cfg)
{
    QCA_ERR         err_qca;
    int i;

    // initialize PMODRST, PMOD9, PMOD10
    BSP_PMOD_General_Init();

    /* Initialize SPI buses                                 */
    if(pSHG_Cfg->HasWifiModule || pSHG_Cfg->Ports[PORT_PMOD3].config.mode == MODE_SPI) {
        BSP_RSPI_General_Init();
    }

    // initialize port handles
    for (i = 0; i < PMOD_PORT_PROTOCOLS_SUPPORTED; i++) {
        if (pSHG_Cfg->Ports[i].config.mode != MODE_UNUSED) {
            pSHG_Cfg->Ports[i].handle = (portHandle *)&portMap[i][pSHG_Cfg->Ports[i].config.mode - 1];
            /* pre-wifi init is so that non-wifi PMOD setup can be done that:
            *      a) prevents interference with WIFI
            *      b) needs to be done before PMOD reset (which WIFI setup performs)
            */
            if (pSHG_Cfg->Ports[i].handle->init != NULL)
                pSHG_Cfg->Ports[i].handle->init(&pSHG_Cfg->Ports[i].config);
        }
    }

    return 0;
}

int WIFIInit(SHG_CFG * pSHG_Cfg) {
    QCA_ERR err_qca;
    //PMOD2: RSPI(WIFI)
    if (pSHG_Cfg->HasWifiModule) {                              /* Initialize WiFi                                      */
        QCA_Init(&QCA_TaskCfg,
                 &QCA_BSP_YWIRELESS_RX111_GT202,
                 &QCA_DevCfg,
                 &err_qca);
        ASSERT(err_qca == QCA_ERR_NONE);

        QCA_Start(&err_qca);
        ASSERT(err_qca == QCA_ERR_NONE);
                                                                /* Start uC/Probe UDP server                            */
#ifdef ENABLE_PROBE_COM
        CPU_BOOLEAN probe_did_init = ProbeCom_Init();
        ASSERT(probe_did_init);
        ProbeTCPIP_Init();
#endif
    }

    return 0;
}
