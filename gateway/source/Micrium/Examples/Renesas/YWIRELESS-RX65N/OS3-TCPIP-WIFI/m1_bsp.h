#ifndef _M1_BSP_H
#define _M1_BSP_H

#include  <os.h>

                                                                /* GPIO pin direction setting defines.                  */
#define  DEF_IN                                     0u
#define  DEF_OUT                                    1u

#ifndef NULL
#define NULL DEF_NULL
#endif

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((int)(!(sizeof(x) % sizeof(0[x])))))

#define RING_BUFFER_MAX 1
#define RING_BUFFER_SIZE 512           /*  the same size as BLE_response  */

typedef enum {
    MODE_UNUSED = 0,
    MODE_SPI ,
    MODE_I2C ,
    MODE_UART
} PORT_MODE;

typedef struct {
    char polarity;
    char phase;
} spiConfig;

typedef struct {
    int unused;
} i2cConfig;

typedef struct {
    int unused;
} uartConfig;

typedef struct {
    PORT_MODE mode;
    long frequency;
    union {
        spiConfig spi;
        i2cConfig i2c;
        uartConfig uart;
    } modeConfig;
} portConfig;

typedef struct {
    int (*init)(portConfig *);
    int (*readWrite)(portConfig *, unsigned char *, unsigned char *, int);
    int (*readAndWrite)(portConfig *, unsigned char *, unsigned char *, int, int);
    int (*gpioSet)(portConfig *, int, char);
} portHandle;

typedef struct {
    portHandle * handle;
    portConfig config;
} port;

typedef struct {
    int s_elem;
    int n_elem;
    void *buffer;
} rb_attr_t;

typedef enum {
    DEMO_BTLE  = 0,
    DEMO_LORA ,
    DEMO_ENV_SENSOR  ,
    DEMO_ENV_GPS ,
    DEMO_IOT_MONITORING ,
    DEMO_VIBRATION_DETECTION ,
    DEMO_SMART_EM ,
    DEMO_INVALID
} DEMO_TYPE;

typedef unsigned int rbd_t;

#define PERIPHERAL_PORTS_SUPPORTED_NUM 3 // PMOD1, PMOD3, PMOD4
extern const int  PERIPHERAL_PORTS_SUPPORTED[PERIPHERAL_PORTS_SUPPORTED_NUM];

CPU_ISR  M1_BSP_UART_RX (void);
CPU_ISR M1_BSP_UART_ERR(void);

#endif
