#include  <stdlib.h>

#include  <os.h>
#include  <lib_mem.h>

#include  <bsp_spi.h>
#include  <riic_i2c_bsp.h>
#include  "app_wifi_util.h"
#include  "app_util.h"
#include  "sx1276.h"
#include  <iodefine.h>
#include  "m1_bsp.h"
#include  "nmea.h"
#include  "app_lora_node.h"
#include  "cli.h"



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
#define LORAMODE  4
#define node_addr 0x11
#define gateway_addr 0x01
#define NETWORK_KEY_0 0x01
#define NETWORK_KEY_1 0x01


#define CRC7WIDTH                               7u
#define CRC7POLY                                0x89
#define CRC7IVEC                                0x7F
#define DATA7WIDTH                              17u
#define DATA7MASK                           ((1<<DATA7WIDTH)-1)
#define DATA7MSB                            (1<<(DATA7WIDTH-1))

#define MAX_MQTT_PAYLOAD 10

unsigned long lastTransmissionTime = 0;
unsigned long delayBeforeTransmit = 10000;
uint8_t message[150];

int SCI0_BSP_UART_WrRd(CPU_INT08U * data, CPU_INT08U * read, int bytes);
void SCI_BSP_I2C_init();
int SCI_BSP_I2C_WrRd(int address, CPU_INT08U * data, CPU_INT08U * read, int bytes);
int SCI_BSP_I2C_WrAndRd(int address, CPU_INT08U * data, int writeBytes, CPU_INT08U * read, int readBytes);
void SCI_BSP_I2C_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir);
void SCI6_BSP_I2C_init();
int SCI6_BSP_I2C_WrRd(int address, CPU_INT08U * data, CPU_INT08U * read, int bytes);
int SCI6_BSP_I2C_WrAndRd(int address, CPU_INT08U * data, int writeBytes, CPU_INT08U * read, int readBytes);
void SCI6_BSP_I2C_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir);
void SCI9_BSP_I2C_init();
int SCI9_BSP_I2C_WrRd(int address, CPU_INT08U * data, CPU_INT08U * read, int bytes);
int SCI9_BSP_I2C_WrAndRd(int address, CPU_INT08U * data, int writeBytes, CPU_INT08U * read, int readBytes);
void SCI9_BSP_I2C_SetCfg (long         freq,
                          int          cpol,
                          int        cpha,
                          int    xfer_unit_len,
                          int   xfer_shift_dir);

static int sci1Init(portConfig * config) {
  SCI_BSP_I2C_init();
  return 0;
}

static int sci1ReadAndWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int readLength, int writeLength) {
    OS_ERR err;
    int ret = 0;
    int address;

    SCI_BSP_I2C_SetCfg(config->frequency, 0, 1, 0, 0);
    if (writeLength >= 1) { // required, at minimum we need device address
        address = *txBuf++;
        writeLength--;
        if (writeLength && readLength) // we are writing some data (maybe register address) and reading some data
            ret = SCI_BSP_I2C_WrAndRd(address, txBuf, writeLength, rxBuf, readLength);
        else if (readLength) // just reading
            ret = SCI_BSP_I2C_WrRd(address, NULL, rxBuf, readLength);
        else // just writing
            ret = SCI_BSP_I2C_WrRd(address, txBuf, NULL, writeLength);
    } else
        ret = -1;
    return ret;
}

static int sci6Init(portConfig * config) {
  SCI6_BSP_I2C_init();
  return 0;
}

static int sci6ReadAndWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int readLength, int writeLength) {
    OS_ERR err;
    int ret = 0;
    int address;

    SCI6_BSP_I2C_SetCfg(config->frequency, 0, 1, 0, 0);
    if (writeLength >= 1) { // required, at minimum we need device address
        address = *txBuf++;
        writeLength--;
        if (writeLength && readLength) // we are writing some data (maybe register address) and reading some data
            ret = SCI6_BSP_I2C_WrAndRd(address, txBuf, writeLength, rxBuf, readLength);
        else if (readLength) // just reading
            ret = SCI6_BSP_I2C_WrRd(address, NULL, rxBuf, readLength);
        else // just writing
            ret = SCI6_BSP_I2C_WrRd(address, txBuf, NULL, writeLength);
    } else
        ret = -1;
    return ret;
}

static int sci9Init(portConfig * config) {
  SCI9_BSP_I2C_init();
  return 0;
}

static int sci9ReadAndWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int readLength, int writeLength) {
    OS_ERR err;
    int ret = 0;
    int address;

    SCI9_BSP_I2C_SetCfg(config->frequency, 0, 1, 0, 0);
    if (writeLength >= 1) { // required, at minimum we need device address
        address = *txBuf++;
        writeLength--;
        if (writeLength && readLength) // we are writing some data (maybe register address) and reading some data
            ret = SCI9_BSP_I2C_WrAndRd(address, txBuf, writeLength, rxBuf, readLength);
        else if (readLength) // just reading
            ret = SCI9_BSP_I2C_WrRd(address, NULL, rxBuf, readLength);
        else // just writing
            ret = SCI9_BSP_I2C_WrRd(address, txBuf, NULL, writeLength);
    } else
        ret = -1;
    return ret;
}

static int riic0Init(portConfig * config) {
  RIIC_BSP_I2C_init();
  return 0;
}

static int riic0ReadAndWrite(portConfig * config, uint8_t * txBuf, uint8_t * rxBuf, int readLength, int writeLength) {
    OS_ERR err;
    int ret = 0;
    int address;

    RIIC_BSP_I2C_SetCfg(config->frequency, 0, 1, 0, 0);
    if (writeLength >= 1) { // required, at minimum we need device address
        address = *txBuf++;
        writeLength--;
        if (writeLength && readLength) // we are writing some data (maybe register address) and reading some data
            ret = RIIC_BSP_I2C_WrAndRd(address, txBuf, writeLength, rxBuf, readLength);
        else if (readLength) // just reading
            ret = RIIC_BSP_I2C_WrRd(address, NULL, rxBuf, readLength);
        else // just writing
            ret = RIIC_BSP_I2C_WrRd(address, txBuf, NULL, writeLength);
    } else
        ret = -1;
    return ret;
}

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
    BSP_SPI_Lock(0);
    BSP_SPI_Cfg(0, &cfg);
    BSP_SPI_ChipSel(0, 3, DEF_ON);
    // TODO: make this return a status so we can indicate errors (timeouts in RSPI while loops?)
    BSP_SPI_Xfer(0, txBuf, rxBuf, len);
    BSP_SPI_ChipSel(0, 3, DEF_OFF);
    BSP_SPI_Unlock(0);
    return len;
}

static int rspiSPIGpioSet(portConfig * config, int pin, char high){
    if(high & 0x1)
        RSPI0_BSP_SPI_Data();
    else
        RSPI0_BSP_SPI_Command();

    return 0;
}

static portHandle riic0 = {
  .init = &riic0Init,
    .readWrite = NULL,
    .readAndWrite = &riic0ReadAndWrite,
    .gpioSet = NULL
};
    
port proximity_port = {
  .handle = &riic0,
  .config = {
        .mode = MODE_I2C,
        .frequency = 100000
  }
};

static portHandle sci1 = {
  .init = &sci1Init,
    .readWrite = NULL,
    .readAndWrite = &sci1ReadAndWrite,
    .gpioSet = NULL
};

port air_quality_port = {
  .handle = &riic0,
  .config = {
        .mode = MODE_I2C,
        .frequency = 100000
  }
};

static portHandle sci6 = {
  .init = &sci6Init,
    .readWrite = NULL,
    .readAndWrite = &sci6ReadAndWrite,
    .gpioSet = NULL
};

port temperature_and_humidity_port = {
  .handle = &sci6,
  .config = {
        .mode = MODE_I2C,
        .frequency = 100000
  }
};

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

static portHandle sci9 = {
  .init = &sci9Init,
    .readWrite = NULL,
    .readAndWrite = &sci9ReadAndWrite,
    .gpioSet = NULL
};

port light_port = {
  .handle = &sci9,
  .config = {
        .mode = MODE_I2C,
        .frequency = 100000
  }
};

static int setup(SHG_CFG * SHG_Cfg)
{
  int e;
  OS_ERR err_os;
  //delay(3000);
  // Open serial communications and wait for port to open:
  //Serial.begin(9600);
 
  // Power ON the module
  if (sx1276_init(&lora))
    return -1;
  
  // Set transmission mode and print the result
  e = setMode(SHG_Cfg->LoraMode);
  if (e)
    return -2;
//  PRINT_CSTSTR("%s","Setting Mode: state ");
  //PRINT_VALUE("%d", e);
  //PRINTLN;
  // Setting the network key
  setNetworkKey(SHG_Cfg->LoraKey[0], SHG_Cfg->LoraKey[1]);
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
//  PRINT_CSTSTR("%s","Setting Channel: state ");
//  PRINT_VALUE("%d", e);
//  PRINTLN;
  
  // Select output power (eXtreme, Max, High or Low)
  e = setPower((SHG_Cfg->LoraPower == 1) ? 'L' : ((SHG_Cfg->LoraPower == 2) ? 'H' : ((SHG_Cfg->LoraPower == 3) ? 'M' : 'X')));
  if (e)
    return -5;
  
//  PRINT_CSTSTR("%s","Setting Power: state ");
//  PRINT_VALUE("%d", e);
//  PRINTLN;
  
  // Set the node address and print the result
  e = setNodeAddress(SHG_Cfg->LoraID);
  if (e)
    return -6;
//  PRINT_CSTSTR("%s","Setting node addr: state ");
//  PRINT_VALUE("%d", e);
//  PRINTLN;
  
  // Print a success message
//  PRINT_CSTSTR("%s","SX1276 successfully configured\n");
        OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_NONE, &err_os);
        return 0;
}
  
static int crc7(int val) {
   // Setup polynomial
   uint32_t pol = CRC7POLY;
   // Align polynomial with data
   pol = pol << (DATA7WIDTH-CRC7WIDTH-1);
   // Loop variable (indicates which bit to test, start with highest)
   int bit = DATA7MSB;
   // Make room for CRC value
   val = val << CRC7WIDTH;
   bit = bit << CRC7WIDTH;
   pol = pol << CRC7WIDTH;
   // Insert initial vector
   val |= CRC7IVEC;
   // Apply division until all bits done
   while( bit & (DATA7MASK<<CRC7WIDTH)) {
     if( bit & val )
           val ^= pol;
       bit >>= 1;
       pol >>= 1;
   }
   return val;
}

static int sample_temp_and_humidity(port * i2cport, float * temperature, float * humidity) {
  uint8_t buf[6], rbuf[6];
  int ret = 0;
  int sensor1Disconnected;
  static int started = 0;

  // sample temp&humidity
    buf[0] = 0x43;
    if (!started) {
        buf[1] = 0x21;
        buf[2] = 0x03;
        sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, NULL, 0, 3);
        buf[1] = 0x22;
        sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, NULL, 0, 3);
        started = 1;
    }
    buf[1] = 0x30;
    sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, buf, 6, 2);
    if (!sensor1Disconnected) {
        int t_val = buf[0] | (buf[1] << 8) | (buf[2] << 16);
        int h_val = buf[3] | (buf[4] << 8) | (buf[5] << 16);
        int t_data = t_val & 0xffff;
        int t_valid = (t_val >> 16) & 0x1;
        int t_crc = (t_val >> 17) & 0x7f;
        int h_data = h_val & 0xffff;
        int h_valid = (h_val >> 16) & 0x1;
        int h_crc = (h_val >> 17) & 0x7f;
        if (t_valid) {
            int t_payl = t_val & 0x1ffff;
            int calc_t_crc = crc7(t_payl);
            if (calc_t_crc == t_crc)
                *temperature = (((float)t_data) / 64.0) - 273.15;
            else
              ret -= 1;
        } else
          ret -= 1;
        if (h_valid) {
            int h_payl = h_val & 0x1ffff;
            int calc_h_crc = crc7(h_payl);
            if (calc_h_crc == h_crc)
                *humidity = ((float)h_data) / 512.0;
            else
                ret -= 2;
        } else
          ret -= 2;
    } else
          return -3;
    return ret;
}

static int sample_air_quality(port * i2cport, uint16_t * air_quality) {
    uint8_t buf[7];
    buf[0] = 0x5A;
    int sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, buf, 7, 1);
    if (!sensor1Disconnected) {
        uint16_t prediction = (buf[0] << 8) | buf[1];
        int resistance = (buf[4] << 16) | (buf[5] << 8) | buf[6];
        int status_val = buf[2];
        //status = "FATAL" if ord(val[3]) else "OK" if status_val == 0x00 else "RUNNING" if status_val == 0x10 else "BUSY" if status_val == 0x01 else "ERROR" if status_val == 0x80 else "FATAL"
        if (status_val == 0) {
            *air_quality = prediction;
            return 0;
        }
        return -2;
    }  
  return -1;
}

static int sample_color_and_proximity(port * i2cport,
                                      uint16_t * alpha,
                                      uint16_t * red,
                                      uint16_t * green,
                                      uint16_t * blue,
                                      int16_t * proximity) {
    uint8_t buf[8];
    int sensor1Disconnected;
    int ret = 0;
                                        
    buf[0] = 0x39;
    buf[1] = 0xA0 | 0x00;
    buf[2] = 0x0f;
    sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, NULL, 0, 3);
    buf[1] = 0xA0 | 0x01;
    buf[2] = 0x20;
    sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, NULL, 0, 3);
    buf[1] = 0xA0 | 0x0E;
    buf[2] = 0x10;
    sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, NULL, 0, 3);
    buf[1] = 0xA0 | 0x0F;
    buf[2] = 0x21;
    sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, NULL, 0, 3);
    buf[1] = 0xA0 | 0x14;
    sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, buf, 8, 2);
    if (!sensor1Disconnected) {
        *alpha = *(unsigned short *)buf;
        *red = *(unsigned short *)&buf[2];
        *green = *(unsigned short *)&buf[4];
        *blue = *(unsigned short *)&buf[6];
    } else
      ret -= 1;

    buf[0] = 0x39;
    buf[1] = 0xA0 | 0x1c;
    sensor1Disconnected = i2cport->handle->readAndWrite(&i2cport->config, buf, buf, 2, 2);
    *proximity = *(uint16_t *)buf;
    return ret;
}

int  App_LoraNode (SHG_CFG * SHG_Cfg)
{
    long startSend;
    long endSend;
    int e;
    int uart_bytes_read;
    size_t pl;
    OS_ERR err_os;
    int light_adc;
    float temperature, humidity, longitude, latitude;
    uint16_t air_quality;
    uint16_t alpha, red, green, blue;
    int16_t proximity;
    gps_t gps;
    OS_MSG_SIZE msg_size;
    int gps_valid = 0;
    OS_TICK curr, prev = OSTimeGet(&err_os);
    uint8_t checksum;
    
    e = setup(SHG_Cfg);
    if (e)
      return e;
  

    proximity_port.handle->init(&proximity_port.config);
    //air_quality_port.handle->init(&air_quality_port.config);
    temperature_and_humidity_port.handle->init(&temperature_and_humidity_port.config);
    light_port.handle->init(&light_port.config);
    isl29033_init(&light_port);

    // init gps
    PORT2.PDR.BIT.B3 = 1;
    PORT2.PODR.BIT.B3 = 0;
    OSTimeDlyHMSM(0, 0, 0, 100, OS_OPT_NONE, &err_os);
    PORT2.PODR.BIT.B3 = 1;
//#define SEGMENTATION

    cli_welcome();

    
    while (1) {
        curr = OSTimeGet(&err_os);
        if ((curr - prev) > (SHG_Cfg->LoopTimeSeconds * 1000)) { // 5 s period
            prev = curr;
            pl = 0;
            e = sample_temp_and_humidity(&temperature_and_humidity_port, &temperature, &humidity);
            if (!(-e & 0x1))
              pl += sprintf((char *)&message[pl], "T%.1f;", temperature);
#ifdef SEGMENTATION
            if (pl >= MAX_MQTT_PAYLOAD) {
                pl += sprintf((char *)&message[pl], "#%s;", SHG_Cfg->registration_code);
                checksum = 0;
                for (int i = 0; i < pl; i++) {
                  checksum ^= message[i];
                }
                pl += sprintf((char *)&message[pl], "*%02X", checksum);
                e = sendPacketTimeout(SHG_Cfg->LoraDestination, message, pl);
                pl = 0;
            }
#endif
            if (!(-e & 0x2))
              pl += sprintf((char *)&message[pl], "H%.1f;", humidity);
#ifdef SEGMENTATION
            if (pl >= MAX_MQTT_PAYLOAD) {
                pl += sprintf((char *)&message[pl], "#%s;", SHG_Cfg->registration_code);
                checksum = 0;
                for (int i = 0; i < pl; i++) {
                  checksum ^= message[i];
                }
                pl += sprintf((char *)&message[pl], "*%02X", checksum);
                e = sendPacketTimeout(SHG_Cfg->LoraDestination, message, pl);
                pl = 0;
            }
#endif
            if (gps_valid) {
              pl += sprintf((char *)&message[pl], "X%.6f;Y%.6f;Z%s;S%d;D%s;", gps.longitude, gps.latitude, gps.altitude, gps.satellites, gps.hdop);
              gps_valid = 0;
            }
#ifdef SEGMENTATION
            if (pl >= MAX_MQTT_PAYLOAD) {
                pl += sprintf((char *)&message[pl], "#%s;", SHG_Cfg->registration_code);
                checksum = 0;
                for (int i = 0; i < pl; i++) {
                  checksum ^= message[i];
                }
                pl += sprintf((char *)&message[pl], "*%02X", checksum);
                e = sendPacketTimeout(SHG_Cfg->LoraDestination, message, pl);
                pl = 0;
            }
#endif
            while ((e = sample_air_quality(&air_quality_port, &air_quality)) == -1);
            if (!e)
              pl += sprintf((char *)&message[pl], "Q%hu;", air_quality);
#ifdef SEGMENTATION
            if (pl >= MAX_MQTT_PAYLOAD) {
                pl += sprintf((char *)&message[pl], "#%s;", SHG_Cfg->registration_code);
                checksum = 0;
                for (int i = 0; i < pl; i++) {
                  checksum ^= message[i];
                }
                pl += sprintf((char *)&message[pl], "*%02X", checksum);
                e = sendPacketTimeout(SHG_Cfg->LoraDestination, message, pl);
                pl = 0;
            }
#endif
#if 0 // no proximity sensor (EOL)
            e = sample_color_and_proximity(&proximity_port, &alpha, &red, &green, &blue, &proximity);
            pl += sprintf((char *)&message[pl], "P%hd;", proximity);
#ifdef SEGMENTATION
            if (pl >= MAX_MQTT_PAYLOAD) {
                pl += sprintf((char *)&message[pl], "#%s;", SHG_Cfg->registration_code);
                checksum = 0;
                for (int i = 0; i < pl; i++) {
                  checksum ^= message[i];
                }
                pl += sprintf((char *)&message[pl], "*%02X", checksum);
                e = sendPacketTimeout(SHG_Cfg->LoraDestination, message, pl);
                pl = 0;
            }
#endif
            if (!e)
              pl += sprintf((char *)&message[pl], "A%lu;R%lu;G%lu;B%lu;", alpha, red, green, blue);
#else
            e = isl29033_read_adc(&light_adc);
            if (!e)
              pl += sprintf((char *)&message[pl], "A%d;", light_adc);
#endif
#ifdef SEGMENTATION
            if (pl) {
                pl += sprintf((char *)&message[pl], "#%s;", SHG_Cfg->registration_code);
                checksum = 0;
                for (int i = 0; i < pl; i++) {
                  checksum ^= message[i];
                }
                pl += sprintf((char *)&message[pl], "*%02X", checksum);
                e = sendPacketTimeout(SHG_Cfg->LoraDestination, message, pl);
                pl = 0;
            }
#else
            pl += sprintf((char *)&message[pl], "#%s;", SHG_Cfg->registration_code);
            uint8_t checksum = 0;
            for (int i = 0; i < pl; i++) {
              checksum ^= message[i];
            }
            pl += sprintf((char *)&message[pl], "*%02X", checksum);
            e = sendPacketTimeout(SHG_Cfg->LoraDestination, message, pl);
#endif
        }
        do {
          uart_bytes_read = SCI0_BSP_UART_WrRd(NULL, message, 150);
          if (uart_bytes_read > 0) {
            for (int i = 0; i < uart_bytes_read; i++) {
              e = parse_NMEA(message[i], &gps);
              if (!e) {
                gps_valid = 1;
              }
            }
          }
        } while (uart_bytes_read == 150);
        cli_loop(SHG_Cfg);
        OSTimeDlyHMSM(0, 0, 0, 1, OS_OPT_NONE, &err_os);
    }
    
    return 0;
}

int App_PreWifiInit() {
  return 0;
}
