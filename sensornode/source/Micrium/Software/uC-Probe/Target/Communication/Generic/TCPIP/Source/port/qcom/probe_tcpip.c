/*
*********************************************************************************************************
*                                       uC/Probe Communication
*
*                         (c) Copyright 2007-2008; Micrium, Inc.; Weston, FL
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
*                                        COMMUNICATION: TCP/IP
*
* Filename      : probe_tcpip.c
* Version       : V2.30
* Programmer(s) : FBJ
*                 BAN
*********************************************************************************************************
* Note(s)       : (1) The abbreviations RX and TX refer to communication from the target's perspective.
*
*                 (2) The abbreviations RD and WR refer to reading data from the target memory and
*                     writing data to the target memory, respectively.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define    PROBE_TCPIP_MODULE
#include  <probe_tcpip.h>
#include  <qcom_api.h>


/*
*********************************************************************************************************
*                                               ENABLE
*
* Note(s) : (1) See 'probe_tcpip.h  ENABLE'.
*********************************************************************************************************
*/

#if (PROBE_COM_CFG_TCPIP_EN == DEF_ENABLED)                     /* See Note #1.                                         */


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  PROBE_TCPIP_RECVFROM_FLAGS                        0


/*
*********************************************************************************************************
*                                         TCPIP PACKET FORMAT
*
* Note(s):  (1) All packets include the following parts:
*
*                   (A)  4 1-byte start delimiters, forming the ASCII representation of "uCPr".  These
*                        are the constants PROBE_USB_PROTOCOL_RX_SD0-PROBE_USB_PROTOCOL_?X_SD4;
*                   (B)  1 2-byte length, the length of the data segment;
*                   (C)  1 1-byte packet counter.
*                   (D)  1 1-byte padding, unused;
*                   (E)  n   bytes of data; and
*                   (F)  1 1-byte checksum; and
*                   (G)  1 1-byte end delimiter, the character '/', which is the constant PROBE_RS232_PROTOCOL_?X_ED.
*
*                                       +-------------------+-------------------+
*                                       |   'u'   |   'C'   |   'P'   |   'r'   |
*                                       +-------------------+-------------------+
*                                       |       Length      | PktCtr  | Padding |
*                                       +-------------------+-------------------+
*                                       |                  Data                 |   The data segment does not need to end on
*                                       |                   .                   |   a four-byte boundary, as might be inferred
*                                       |                   .                   |   from this diagram.
*                                       |                   .                   |
*                                       +-------------------+-------------------+
*                                       | Checksum|   '/'   |
*                                       +-------------------+
*********************************************************************************************************
*/

                                                                    /* ------------- INBOUND PACKET DELIMITERS ------------ */
#define  PROBE_TCPIP_PROTOCOL_RX_SD0                    0x75u       /* (A) Start delimiters                                 */
#define  PROBE_TCPIP_PROTOCOL_RX_SD1                    0x43u
#define  PROBE_TCPIP_PROTOCOL_RX_SD2                    0x50u
#define  PROBE_TCPIP_PROTOCOL_RX_SD3                    0x72u
#define  PROBE_TCPIP_PROTOCOL_RX_ED                     0x2Fu       /* (E) End   delimiter                                  */

                                                                    /* ------------ OUTBOUND PACKET DELIMITERS ------------ */
#define  PROBE_TCPIP_PROTOCOL_TX_SD0                    0x75u       /* (A) Start delimiters                                 */
#define  PROBE_TCPIP_PROTOCOL_TX_SD1                    0x43u
#define  PROBE_TCPIP_PROTOCOL_TX_SD2                    0x50u
#define  PROBE_TCPIP_PROTOCOL_TX_SD3                    0x72u
#define  PROBE_TCPIP_PROTOCOL_TX_ED                     0x2Fu       /* (E) End   delimiter                                  */


#define  PROBE_TCPIP_CHKSUM_EN                     DEF_FALSE        /* DO NOT CHANGE                                        */


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  CPU_CHAR             ProbeTCPIP_RxBuf[PROBE_TCPIP_CFG_RX_BUF_SIZE + 10];    /* Buf to hold rx'd data.           */
static  CPU_CHAR             ProbeTCPIP_TxBuf[PROBE_TCPIP_CFG_TX_BUF_SIZE + 10];    /* Buf to hold tx   data.           */

static  int                  ProbeTCPIP_ConnSockID;                         /* Sock ID of conn'd or accepted sock.      */
static  SOCKADDR_T           ProbeTCPIP_RemoteSockAddr;                     /* Struct containing info about remote host.*/
static  int                  ProbeTCPIP_RemoteLen;                          /* Size of the 'ProbeTCPIP_RemoteSockAddr'. */


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  CPU_INT16U   ProbeTCPIP_ParseRxPkt(CPU_INT16S  rx_len);

static  CPU_BOOLEAN  ProbeTCPIP_ServerInit(void);

static  CPU_INT16S   ProbeTCPIP_RxPkt     (void);

static  CPU_INT16S   ProbeTCPIP_TxStart   (CPU_INT16S  tx_len);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          ProbeTCPIP_Init()
*
* Description : Initializes the Probe TCP-IP communication module.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  ProbeTCPIP_Init (void)
{
    ProbeTCPIP_OS_Init();
}


/*
*********************************************************************************************************
*                                          ProbeTCPIP_Task()
*
* Description : Server task.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : ProbeTCPIP_OS_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  ProbeTCPIP_Task (void)
{
    CPU_INT16S   len;
    CPU_BOOLEAN  init;


    ProbeTCPIP_RemoteLen = (int)sizeof(ProbeTCPIP_RemoteSockAddr);

    init = ProbeTCPIP_ServerInit();
    while (init == DEF_FALSE) {
        ProbeTCPIP_OS_Dly(500);
        init = ProbeTCPIP_ServerInit();
    }

    while(DEF_TRUE)  {
        len = ProbeTCPIP_RxPkt();
        if (len > 0) {
            len = (CPU_INT16S)ProbeTCPIP_ParseRxPkt(len);       /* Parse pkt & form a resp.                             */
            if (len > 0) {
                (void)ProbeTCPIP_TxStart(len);
            }
        }

#if (QCA_CFG_SOCK_NON_BLOCKING_RX_EN == DEF_ENABLED)
        if (len <= 0) {
            ProbeTCPIP_OS_Dly(PROBE_TCPIP_CFG_SOCK_WAIT_DLY);
        }
#endif
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       ProbeTCPIP_ServerInit()
*
* Description : Initialize the UDP server.
*
* Argument(s) : none.
*
* Return(s)   : The socket identifier, if NO errors.
*               -1,                    if an error occurred.
*
* Caller(s)   : ProbeTCPIP_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  ProbeTCPIP_ServerInit (void)
{
    int         bind_status;
    SOCKADDR_T  sock_addr;


    ProbeTCPIP_ConnSockID = qcom_socket(ATH_AF_INET,
                                        SOCK_DGRAM_TYPE,
                                        0);

    if (ProbeTCPIP_ConnSockID >= 0) {                           /* Bind a local addr so client can send to us.          */
        Mem_Set((void     *)&sock_addr,
                (CPU_INT08U) 0,
                (CPU_SIZE_T) sizeof(sock_addr));

        sock_addr.sin_family = ATH_AF_INET;
        sock_addr.sin_port   = PROBE_TCPIP_CFG_PORT;
        sock_addr.sin_addr   = 0;                               /* Wildcard                                             */

        bind_status = qcom_bind(                   ProbeTCPIP_ConnSockID,
                                (struct sockaddr *)&sock_addr,
                                                   sizeof (sock_addr));

        if (bind_status != 0) {
            qcom_socket_close((int)ProbeTCPIP_ConnSockID);      /* Could not bind to the port.                          */
            return (DEF_FALSE);
        }
    }

    return (DEF_TRUE);
}


/*
*********************************************************************************************************
*                                         ProbeTCPIP_RxPkt()
*
* Description : Receive data from Probe.
*
* Argument(s) : none.
*
* Return(s)   : The length of the received packet or (if an error occurred) -1.
*
* Caller(s)   : ProbeTCPIP_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT16S  ProbeTCPIP_RxPkt (void)
{
    int  len;
    void *p_buf;

#if  QCA_CFG_SOCK_NON_BLOCKING_RX_EN == DEF_DISABLED
    p_buf = &ProbeTCPIP_RxBuf[0];
    //TODO select
#else
    p_buf = CUSTOM_ALLOC(PROBE_TCPIP_CFG_RX_BUF_SIZE + 10);
    if ( ! p_buf) {
        return -1;
    }
#endif

    ProbeTCPIP_RemoteLen = (int)sizeof(ProbeTCPIP_RemoteSockAddr);
    len                  = qcom_recvfrom((int               ) ProbeTCPIP_ConnSockID,
                                         (void             *) p_buf,
                                                             (PROBE_TCPIP_CFG_RX_BUF_SIZE + 10),
                                         (int               ) PROBE_TCPIP_RECVFROM_FLAGS,
                                         (struct  sockaddr *)&ProbeTCPIP_RemoteSockAddr,
                                         (int              *)&ProbeTCPIP_RemoteLen);

#if  QCA_CFG_SOCK_NON_BLOCKING_RX_EN == DEF_ENABLED
    if (len > 0) {
        Mem_Copy(ProbeTCPIP_RxBuf, p_buf, len);
    }

    CUSTOM_FREE(p_buf);
#endif

    return ((CPU_INT16S)len);
}


/*
*********************************************************************************************************
*                                        ProbeTCPIP_TxStart()
*
* Description : Sends a response packet to Probe.
*
* Argument(s) : tx_len      Length of the transmit packet.
*
* Return(s)   : The length of the received packet, if NO errors.
*               -1,                                if an error occurred.
*
* Caller(s)   : ProbeTCPIP_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT16S  ProbeTCPIP_TxStart (CPU_INT16S  tx_len)
{
    int  len;
    void *p_buf;


    p_buf = CUSTOM_ALLOC(tx_len);
    if ( ! p_buf) {
        return -1;
    }

    Mem_Copy(p_buf, ProbeTCPIP_TxBuf, tx_len);
    len = qcom_sendto((int                ) ProbeTCPIP_ConnSockID,
                      (void              *) p_buf,
                                            tx_len,
                      (int                ) 0,
                      (struct  sockaddr  *)&ProbeTCPIP_RemoteSockAddr,
                      (int                ) ProbeTCPIP_RemoteLen);

#if  QCA_CFG_SOCK_NON_BLOCKING_TX_EN == DEF_DISABLED
    CUSTOM_FREE(p_buf);
#endif
    return (len);
}


/*
*********************************************************************************************************
*                                       ProbeTCPIP_ParseRxPkt()
*
* Description : Parse received packet & form respose packet.
*
* Argument(s) : rx_len      Length of the received packet.
*
* Return(S)   : The number of bytes in the data segment of the packet to transmit in response.
*
* Caller(s)   : ProbeTCPIP_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT16U  ProbeTCPIP_ParseRxPkt (CPU_INT16S  rx_len)
{
    CPU_INT16U  len;
    CPU_INT16U  rx_len_pkt;
    CPU_INT08U  pkt_ctr;


    if (rx_len <= 10) {                                         /* Pkt is too short.                                    */
        return ((CPU_INT16U)0);
    }

                                                                /* Start delims bad.                                    */
    if ((ProbeTCPIP_RxBuf[0] != PROBE_TCPIP_PROTOCOL_RX_SD0) ||
        (ProbeTCPIP_RxBuf[1] != PROBE_TCPIP_PROTOCOL_RX_SD1) ||
        (ProbeTCPIP_RxBuf[2] != PROBE_TCPIP_PROTOCOL_RX_SD2) ||
        (ProbeTCPIP_RxBuf[3] != PROBE_TCPIP_PROTOCOL_RX_SD3)) {
        return ((CPU_INT16U)0);
    }

                                                                /* Len in pkt does not match pkt len.                   */
    rx_len_pkt = ((CPU_INT16U)ProbeTCPIP_RxBuf[4] | ((CPU_INT16U)ProbeTCPIP_RxBuf[5] << 8));
    if (rx_len_pkt != rx_len - 10) {
        return ((CPU_INT16U)0);
    }

    pkt_ctr = ProbeTCPIP_RxBuf[6];

    len     = ProbeCom_ParseRxPkt((void     *)&ProbeTCPIP_RxBuf[8],
                                  (void     *)&ProbeTCPIP_TxBuf[8],
                                  (CPU_INT16U) rx_len_pkt,
                                  (CPU_INT16U) PROBE_TCPIP_CFG_TX_BUF_SIZE);

    if (len == 0u) {
        return ((CPU_INT16U)0);
    }

                                                                /* Start delims.                                        */
    ProbeTCPIP_TxBuf[0]           = PROBE_TCPIP_PROTOCOL_TX_SD0;
    ProbeTCPIP_TxBuf[1]           = PROBE_TCPIP_PROTOCOL_TX_SD1;
    ProbeTCPIP_TxBuf[2]           = PROBE_TCPIP_PROTOCOL_TX_SD2;
    ProbeTCPIP_TxBuf[3]           = PROBE_TCPIP_PROTOCOL_TX_SD3;
    ProbeTCPIP_TxBuf[4]           = (len & 0xFF);               /* Two-byte pkt len.                                    */
    ProbeTCPIP_TxBuf[5]           = (len >> 8);
    ProbeTCPIP_TxBuf[6]           = pkt_ctr;                    /* Packet counter.                                      */
    ProbeTCPIP_TxBuf[7]           = 0;

    ProbeTCPIP_TxBuf[8 + len]     = 0;                          /* Checksum.                                            */
    ProbeTCPIP_TxBuf[8 + len + 1] = PROBE_TCPIP_PROTOCOL_TX_ED; /* End delim.                                           */

    return (len + 10u);
}


/*
*********************************************************************************************************
*                                              ENABLE END
*
* Note(s) : See 'ENABLE  Note #1'.
*********************************************************************************************************
*/

#endif
