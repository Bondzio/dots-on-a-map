/*
*********************************************************************************************************
*                                             uC/TCP-IP
*                                      The Embedded TCP/IP Suite
*
*                         (c) Copyright 2004-2015; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/TCP-IP is provided in source form to registered licensees ONLY.  It is
*                  illegal to distribute this source code to any third party unless you receive
*                  written permission by an authorized Micrium representative.  Knowledge of
*                  the source code may NOT be used to develop a similar product.
*
*                  Please help us continue to provide the Embedded community with the finest
*                  software available.  Your honesty is greatly appreciated.
*
*                  You can find our product's user manual, API reference, release notes and
*                  more information at: https://doc.micrium.com
*
*                  You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                        NETWORK DEVICE DRIVER
*
*                                               QCA400X
*
* Filename      : net_dev_qca400x.h
* Version       : V3.04.00.00
* Programmer(s) : AL
*********************************************************************************************************
* Note(s)       : (1) The net_dev_qca400x module is essentially a port of the Qualcomm/Atheros driver.
*                     Please see the Qualcomm/Atheros copyright in the files in the "atheros_wifi" folder.
*
*                 (2) Assumes uC/TCP-IP V3.02.00.00 (or more recent version) is included in the project build.
*
*                 (3) Device driver compatible with these hardware:
*
*                     (a) QCA4002
*                     (b) QCA4004
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#ifndef  NET_DEV_QCA400X_MODULE_PRESENT
#define  NET_DEV_QCA400X_MODULE_PRESENT

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/
#include  <IF/net_if_wifi.h>
#include  <Dev/WiFi/Manager/Generic/net_wifi_mgr.h>
#include  "atheros_wifi/custom_src/include/a_types.h"


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/
extern  const  NET_DEV_API_WIFI  NetDev_API_QCA400X;
extern  NET_IF_NBR               WiFi_SPI_IF_Nbr;


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  NET_DEV_ATHEROS_PACKET_HEADER         20u

#define  NET_DEV_IO_CTRL_FW_VER_GET            127u

/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/

typedef  enum net_dev_state_scan {
    NET_DEV_STATE_SCAN_CMD_CONFIGURE         = 1u,
    NET_DEV_STATE_SCAN_START                 = 2u,
    NET_DEV_STATE_SCAN_WAIT_RESP             = 4u,
    NET_DEV_STATE_SCAN_WAIT_COMPLETE         = 3u,
}NET_DEV_STATE_SCAN;

typedef  enum net_dev_state_connect{
    NET_DEV_STATE_CONNECT_NONE               = 1u,
    NET_DEV_STATE_CONNECT_802_11_WAITING     = 2u,
    NET_DEV_STATE_CONNECT_802_11_AUTH        = 3u,
    NET_DEV_STATE_CONNECT_WPA_WAITING        = 4u,
    NET_DEV_STATE_CONNECT_WPA_AUTH           = 5u,
    NET_DEV_STATE_CONNECT_WPA_FAIL           = 6u,
}NET_DEV_STATE_CONNECT;

typedef struct net_dev_state {
    NET_DEV_STATE_CONNECT   Connect;
    NET_DEV_STATE_SCAN      Scan;
    CPU_INT08U              ScanAPCount;
}NET_DEV_STATE;

typedef struct net_dev_peer_info NET_DEV_PEER_INFO ;

struct net_dev_peer_info {
    NET_IF_WIFI_PEER        Info;
    NET_DEV_PEER_INFO      *Next;
};


/*
*********************************************************************************************************
*                                        NET DEV DATA STRUCTURE
* Note(s) : (1) This function has been inspired from the Api_RxComplete() in api_rxtx.c. It has been
*               modified for a better integration and to respect the Micrum's coding standard.
*
*           (2) The following notes are comments from the original code.
*
*               (a) PORT_NOTE: utility_mutex is a mutex used to protect resources that might be accessed
*                   by multiple task/threads in a system.It is used by the ACQUIRE/RELEASE macros below.
*                   If the target system is single threaded then this mutex can be omitted and the macros
*                   should be blank.

*
*               (b) PORT_NOTE: These 2 events are used to block & wake their respective tasks.
*
*
*               (c) PORT_NOTE: pCommonCxt stores the drivers common context. It is accessed by the
*                   common source via GET_DRIVER_COMMON() below.
*
*********************************************************************************************************
*/
typedef  struct  net_dev_data {
#if (NET_CTR_CFG_STAT_EN == DEF_ENABLED)
    NET_CTR                     StatRxPktCtr;
    NET_CTR                     StatTxPktCtr;
#endif
#if (NET_CTR_CFG_ERR_EN  == DEF_ENABLED)
    NET_CTR                     ErrRxPktDiscardedCtr;
    NET_CTR                     ErrTxPktDiscardedCtr;
#endif

    CPU_INT08U                 *GlobalBufPtr;

    CPU_INT08U                  WaitRespType;

    CPU_BOOLEAN                 LinkStatus;
    CPU_INT08U                  LinkStatusQueryCtr;

    NET_DEV_STATE               State;

    MEM_DYN_POOL                PeerInfoPool;
    NET_DEV_PEER_INFO          *PeerList;

    MEM_POOL                    A_NetBufPool;

    KAL_Q_HANDLE                DriverRxQueue;

    CPU_INT08U                  DeviceMacAddr[NET_IF_802x_ADDR_SIZE];
    CPU_INT32U                  SoftVersion;
                                                                /* See note 2a.                                         */
    KAL_LOCK_HANDLE             UtilityMutex;
                                                                /* See note 2b.                                         */
    KAL_TASK_HANDLE             DriverTaskHandle;
    KAL_SEM_HANDLE              DriverWakeSemHandle;
    KAL_SEM_HANDLE              UserWakeSemHandle;
    KAL_SEM_HANDLE              DriverStartSemHandle;
    KAL_SEM_HANDLE              GlobalBufSemHandle;
                                                                /* See note 2c.                                         */
    A_VOID                     *CommonCxt;                      /* the common driver context link.                      */
    A_BOOL                      DriverShutdown;                 /* used to shutdown the driver.                         */

} NET_DEV_DATA;

/*
 *********************************************************************************************************
 *                                     WIRELESS DEVICE DATA TYPES
 *
 * Note(s) : (1) The Wireless interface configuration data type is a specific definition of a network
 *               device configuration data type.  Each specific network device configuration data type
 *               MUST define ALL generic network device configuration parameters, synchronized in both
 *               the sequential order & data type of each parameter.
 *
 *               Thus, ANY modification to the sequential order or data types of configuration parameters
 *               MUST be appropriately synchronized between the generic network device configuration data
 *               type & the Ethernet interface configuration data type.
 *
 *               See also 'net_if.h  GENERIC NETWORK DEVICE CONFIGURATION DATA TYPE  Note #1'.
 *********************************************************************************************************
 */
                                                        /* ----------------------- NET WIFI DEV CFG ----------------------- */
typedef  struct  net_dev_cfg_wifi_qca {
    NET_IF_MEM_TYPE                 RxBufPoolType;      /* Rx buf mem pool type :                                           */
                                                        /*   NET_IF_MEM_TYPE_MAIN        bufs alloc'd from main      mem    */
                                                        /*   NET_IF_MEM_TYPE_DEDICATED   bufs alloc'd from dedicated mem    */
    NET_BUF_SIZE                    RxBufLargeSize;     /* Size  of dev rx large buf data areas (in octets).                */
    NET_BUF_QTY                     RxBufLargeNbr;      /* Nbr   of dev rx large buf data areas.                            */
    NET_BUF_SIZE                    RxBufAlignOctets;   /* Align of dev rx       buf data areas (in octets).                */
    NET_BUF_SIZE                    RxBufIxOffset;      /* Offset from base ix to rx data into data area (in octets).       */

    NET_IF_MEM_TYPE                 TxBufPoolType;      /* Tx buf mem pool type :                                           */
                                                        /*   NET_IF_MEM_TYPE_MAIN        bufs alloc'd from main      mem    */
                                                        /*   NET_IF_MEM_TYPE_DEDICATED   bufs alloc'd from dedicated mem    */
    NET_BUF_SIZE                    TxBufLargeSize;     /* Size  of dev tx large buf data areas (in octets).                */
    NET_BUF_QTY                     TxBufLargeNbr;      /* Nbr   of dev tx large buf data areas.                            */
    NET_BUF_SIZE                    TxBufSmallSize;     /* Size  of dev tx small buf data areas (in octets).                */
    NET_BUF_QTY                     TxBufSmallNbr;      /* Nbr   of dev tx small buf data areas.                            */
    NET_BUF_SIZE                    TxBufAlignOctets;   /* Align of dev tx       buf data areas (in octets).                */
    NET_BUF_SIZE                    TxBufIxOffset;      /* Offset from base ix to tx data from data area (in octets).       */

    CPU_ADDR                        MemAddr;            /* Base addr of (WiFi dev's) dedicated mem, if avail.               */
    CPU_ADDR                        MemSize;            /* Size      of (WiFi dev's) dedicated mem, if avail.               */

    NET_DEV_CFG_FLAGS               Flags;              /* Opt'l bit flags.                                                 */

    NET_DEV_BAND                    Band;               /* Wireless Band to use by the device.                              */
                                                        /*   NET_DEV_2_4_GHZ        Wireless band is 2.4Ghz.                */
                                                        /*   NET_DEV_5_0_GHZ        Wireless band is 5.0Ghz.                */
                                                        /*   NET_DEV_DUAL           Wireless band is dual (2.4 & 5.0 Ghz).  */

    NET_DEV_CFG_SPI_CLK_FREQ        SPI_ClkFreq;        /* SPI Clk freq (in Hz).                                            */

    NET_DEV_CFG_SPI_CLK_POL         SPI_ClkPol;         /* SPI Clk pol:                                                     */
                                                        /*   NET_DEV_SPI_CLK_POL_ACTIVE_LOW      clk is low  when inactive. */
                                                        /*   NET_DEV_SPI_CLK_POL_ACTIVE_HIGH     clk is high when inactive. */

    NET_DEV_CFG_SPI_CLK_PHASE       SPI_ClkPhase;       /* SPI Clk phase:                                                   */
                                                        /*   NET_DEV_SPI_CLK_PHASE_FALLING_EDGE  Data availables on  ...    */
                                                        /*                                          ... failling edge.      */
                                                        /*   NET_DEV_SPI_CLK_PHASE_RASING_EDGE   Data availables on  ...    */
                                                        /*                                          ... rasing edge.        */

    NET_DEV_CFG_SPI_XFER_UNIT_LEN   SPI_XferUnitLen;    /* SPI xfer unit length:                                            */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_8_BITS    Unit len of  8 bits.       */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_16_BITS   Unit len of 16 bits.       */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_32_BITS   Unit len of 32 bits.       */
                                                        /*   NET_DEV_SPI_XFER_UNIT_LEN_64_BITS   Unit len of 64 bits.       */

    NET_DEV_CFG_SPI_XFER_SHIFT_DIR  SPI_XferShiftDir;   /* SPI xfer shift dir:                                              */
                                                        /*   NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_MSB   Xfer MSB first.         */
                                                        /*   NET_DEV_SPI_XFER_SHIFT_DIR_FIRST_LSB   Xfer LSB first.         */

    CPU_CHAR                        HW_AddrStr[NET_IF_802x_ADDR_SIZE_STR];  /*  WiFi IF's dev hw addr str.                  */

    CPU_INT32U                      DriverTask_Prio;    /* Driver task priority, if needed.                                 */

    CPU_STK_SIZE                    DriverTask_StkSize; /* Driver task stack size, if needed.                               */

} NET_DEV_CFG_WIFI_QCA;


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif                                                          /* End of WiFi SPI template module include.             */
