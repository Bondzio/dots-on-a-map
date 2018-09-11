/*
 * This file is part of libemqtt.
 *
 * libemqtt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libemqtt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with libemqtt.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *
 * Created by Vicente Ruiz Rodríguez
 * Copyright 2012 Vicente Ruiz Rodríguez <vruiz2.0@gmail.com>. All rights reserved.
 *
 */

#include <qcom_api.h>
#include <stdlib.h>
#include <lib_mem.h>
#include <emqtt.h>


#define  MQTT_KEEP_ALIVE_DFLT  30 // seconds


static int init_socket(mqtt_broker_handle_t* broker, const char* hostname, uint16_t port);
static int close_socket(mqtt_broker_handle_t* broker);
static int send_packet(void* socket_info, const void* buf, unsigned int len);
static int recv_packet(void* socket_info, char* buf, unsigned int len, int timeout);


// export global qualcomm network interface for mqtt broker object
mqtt_net_if_t const MQTT_NET_IF_QCOM = {
    init_socket,
    close_socket,
    send_packet,
    recv_packet
};


static int init_socket(mqtt_broker_handle_t* broker, const char* hostname, uint16_t port)
{
    int *psock_id;
	int keepalive = MQTT_KEEP_ALIVE_DFLT; // Seconds


    psock_id = malloc(sizeof(*psock_id));
    if (!psock_id) return -1;

	// Create the socket
	if((*psock_id = qcom_socket(ATH_AF_INET, SOCK_STREAM_TYPE, 0)) < 0) {
        free(psock_id);
		return -1;
    }

    SOCKADDR_T server_sock_addr;
	// Setup socket
	server_sock_addr.sin_family = ATH_AF_INET;
	server_sock_addr.sin_port = port;//MEM_VAL_HOST_TO_BIG_16(port);//htons(port);
    if ((qcom_dnsc_get_host_by_name((A_CHAR *)hostname, &server_sock_addr.sin_addr) < 0)) {
        free(psock_id);
        return -1;
    }

//	mqtt_set_alive(broker, keepalive); TODO
    qcom_setsockopt(*psock_id, ATH_IPPROTO_TCP, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
	// Connect to broker
    if((qcom_connect(*psock_id, (struct sockaddr*)&server_sock_addr, sizeof(server_sock_addr))) < 0) {
        free(psock_id);
		return -1;
    }


	broker->socket_info = (void*)psock_id;


	return 0;
}


static int close_socket(mqtt_broker_handle_t* broker)
{
	int *psock_id = ((int*)broker->socket_info);
    int ret;

    ret = qcom_socket_close(*psock_id);
    free(psock_id);
	return ret;
}


static int send_packet(void* sock_info, const void *buf, unsigned int len)
{
	int sock_id = *((int*)sock_info);
    char *p_tx;
    int ret;


    p_tx = (char *)CUSTOM_ALLOC(len);
    if (!p_tx) return (-1);

    Mem_Copy(p_tx, buf, len);

	ret = qcom_send(sock_id, (char *)p_tx, len, 0);

#if QCA_CFG_SOCK_NON_BLOCKING_TX_EN == DEF_DISABLED
    CUSTOM_FREE(p_tx);
#endif

    return (ret);
}


static int recv_packet(void* sock_info, char *buf, unsigned int len, int timeout)
{
    int sock_id = *((int *)sock_info);
    CPU_INT16U  pkt_len;
    CPU_INT16U  total_bytes;
    int         bytes_rcvd;
    int         avail;


    bytes_rcvd  = 0;
    total_bytes = 0;

                                                                /* Reading fixed header.                                */
    while (total_bytes < 2) {
        avail = qcom_select(sock_id, timeout);
        if (avail != -1) {
            bytes_rcvd = qcom_recv(sock_id,
                                   buf + total_bytes,
                                   len - total_bytes,
                                   0);
        }
                                                                /* Return if no data are available                      */
        if (bytes_rcvd <= 0) {
            return -1;
        }

        total_bytes += bytes_rcvd;                              /* Keep count of total bytes received.                  */
    }

    pkt_len = buf[1] + 2;                                       /* Remaining length + fixed header length.              */

                                                                /* Reading the packet.                                  */
    while (total_bytes < pkt_len) {
        avail = qcom_select(sock_id, timeout);
        if (avail != -1) {
            bytes_rcvd = qcom_recv(sock_id,
                                   buf + total_bytes,
                                   len - total_bytes,
                                   0);
        }

        if (bytes_rcvd <= 0) {
            return -1;
        }

        total_bytes += bytes_rcvd;                              /* Keep count of total bytes received.                  */
    }

    return pkt_len;
}
