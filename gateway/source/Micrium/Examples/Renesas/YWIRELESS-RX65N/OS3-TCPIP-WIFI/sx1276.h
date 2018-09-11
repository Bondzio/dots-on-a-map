#ifndef SX1276_h
#define SX1276_h

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <bsp_spi.h>

#ifndef inttypes_h
	#include <inttypes.h>
#endif
#define byte uint8_t
#include <stdbool.h>
#define boolean bool
   
#include "m1_bsp.h"
/******************************************************************************
 * Definitions & Declarations
 *****************************************************************************/

#define SX1276_RST  3
#define SX1276_SS 15      //original 2, for esp8266 15

//! MACROS //
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)  // read a bit
#define bitSet(value, bit) ((value) |= (1UL << (bit)))    // set bit to '1'
#define bitClear(value, bit) ((value) &= ~(1UL << (bit))) // set bit to '0'

//! REGISTERS //
#define        REG_FIFO        					0x00
#define        REG_OP_MODE        				0x01
#define        REG_BITRATE_MSB    				0x02
#define        REG_BITRATE_LSB    				0x03
#define        REG_FDEV_MSB   					0x04
#define        REG_FDEV_LSB    					0x05
#define        REG_FRF_MSB    					0x06
#define        REG_FRF_MID    					0x07
#define        REG_FRF_LSB    					0x08
#define        REG_PA_CONFIG    				0x09
#define        REG_PA_RAMP    					0x0A
#define        REG_OCP    						0x0B
#define        REG_LNA    						0x0C
#define        REG_RX_CONFIG    				0x0D
#define        REG_FIFO_ADDR_PTR  				0x0D
#define        REG_RSSI_CONFIG   				0x0E
#define        REG_FIFO_TX_BASE_ADDR 		    0x0E
#define        REG_RSSI_COLLISION    			0x0F
#define        REG_FIFO_RX_BASE_ADDR   			0x0F
#define        REG_RSSI_THRESH    				0x10
#define        REG_FIFO_RX_CURRENT_ADDR   		0x10
#define        REG_RSSI_VALUE_FSK	    		0x11
#define        REG_IRQ_FLAGS_MASK    			0x11
#define        REG_RX_BW		    			0x12
#define        REG_IRQ_FLAGS	    			0x12
#define        REG_AFC_BW		    			0x13
#define        REG_RX_NB_BYTES	    			0x13
#define        REG_OOK_PEAK	    				0x14
#define        REG_RX_HEADER_CNT_VALUE_MSB  	0x14
#define        REG_OOK_FIX	    				0x15
#define        REG_RX_HEADER_CNT_VALUE_LSB  	0x15
#define        REG_OOK_AVG	 					0x16
#define        REG_RX_PACKET_CNT_VALUE_MSB  	0x16
#define        REG_RX_PACKET_CNT_VALUE_LSB  	0x17
#define        REG_MODEM_STAT	  				0x18
#define        REG_PKT_SNR_VALUE	  			0x19
#define        REG_AFC_FEI	  					0x1A
#define        REG_PKT_RSSI_VALUE	  			0x1A
#define        REG_AFC_MSB	  					0x1B
#define        REG_RSSI_VALUE_LORA	  			0x1B
#define        REG_AFC_LSB	  					0x1C
#define        REG_HOP_CHANNEL	  				0x1C
#define        REG_FEI_MSB	  					0x1D
#define        REG_MODEM_CONFIG1	 		 	0x1D
#define        REG_FEI_LSB	  					0x1E
#define        REG_MODEM_CONFIG2	  			0x1E
#define        REG_PREAMBLE_DETECT  			0x1F
#define        REG_SYMB_TIMEOUT_LSB  			0x1F
#define        REG_RX_TIMEOUT1	  				0x20
#define        REG_PREAMBLE_MSB_LORA  			0x20
#define        REG_RX_TIMEOUT2	  				0x21
#define        REG_PREAMBLE_LSB_LORA  			0x21
#define        REG_RX_TIMEOUT3	 				0x22
#define        REG_PAYLOAD_LENGTH_LORA		 	0x22
#define        REG_RX_DELAY	 					0x23
#define        REG_MAX_PAYLOAD_LENGTH 			0x23
#define        REG_OSC		 					0x24
#define        REG_HOP_PERIOD	  				0x24
#define        REG_PREAMBLE_MSB_FSK 			0x25
#define        REG_FIFO_RX_BYTE_ADDR 			0x25
#define        REG_PREAMBLE_LSB_FSK 			0x26
#define        REG_MODEM_CONFIG3	  			0x26
#define        REG_SYNC_CONFIG	  				0x27
#define        REG_SYNC_VALUE1	 				0x28
#define        REG_SYNC_VALUE2	  				0x29
#define        REG_SYNC_VALUE3	  				0x2A
#define        REG_SYNC_VALUE4	  				0x2B
#define        REG_SYNC_VALUE5	  				0x2C
#define        REG_SYNC_VALUE6	  				0x2D
#define        REG_SYNC_VALUE7	  				0x2E
#define        REG_SYNC_VALUE8	  				0x2F
#define        REG_PACKET_CONFIG1	  			0x30
#define        REG_PACKET_CONFIG2	  			0x31
#define        REG_DETECT_OPTIMIZE              0x31
#define        REG_PAYLOAD_LENGTH_FSK			0x32
#define        REG_NODE_ADRS	  				0x33
#define        REG_BROADCAST_ADRS	 		 	0x34
#define        REG_FIFO_THRESH	  				0x35
#define        REG_SEQ_CONFIG1	  				0x36
#define        REG_SEQ_CONFIG2	  				0x37
#define        REG_DETECTION_THRESHOLD          0x37
#define        REG_TIMER_RESOL	  				0x38
#define        REG_SYNC_WORD                    0x39
#define        REG_SYNC_WORD                    0x39
#define        REG_TIMER1_COEF	  				0x39
#define        REG_TIMER2_COEF	  				0x3A
#define        REG_IMAGE_CAL	  				0x3B
#define        REG_TEMP		  					0x3C
#define        REG_LOW_BAT	  					0x3D
#define        REG_IRQ_FLAGS1	  				0x3E
#define        REG_IRQ_FLAGS2	  				0x3F
#define        REG_DIO_MAPPING1	  				0x40
#define        REG_DIO_MAPPING2	  				0x41
#define        REG_VERSION	  					0x42
#define        REG_AGC_REF	  					0x43
#define        REG_AGC_THRESH1	  				0x44
#define        REG_AGC_THRESH2	  				0x45
#define        REG_AGC_THRESH3	  				0x46
#define        REG_PLL_HOP	  					0x4B
#define        REG_TCXO		  					0x58
#define        REG_PA_DAC		  				0x5A
#define        REG_PLL		  					0x5C
#define        REG_PLL_LOW_PN	  				0x5E
#define        REG_FORMER_TEMP	  				0x6C
#define        REG_BIT_RATE_FRAC	  			0x70

#define RF_IMAGECAL_IMAGECAL_MASK                   0xBF
#define RF_IMAGECAL_IMAGECAL_START                  0x40
#define RF_IMAGECAL_IMAGECAL_RUNNING                0x20

//FREQUENCY CHANNELS:
#define CH_10_868 0xD84CCC // channel 10, central freq = 865.20MHz
									 // = 865200000*RH_LORA_FCONVERT
#define CH_11_868 0xD86000 // channel 11, central freq = 865.50MHz
#define CH_12_868 0xD87333 // channel 12, central freq = 865.80MHz
#define CH_13_868 0xD88666 // channel 13, central freq = 866.10MHz
#define CH_14_868 0xD89999 // channel 14, central freq = 866.40MHz
#define CH_15_868 0xD8ACCC // channel 15, central freq = 866.70MHz
#define CH_16_868 0xD8C000 // channel 16, central freq = 867.00MHz
#define CH_17_868 0xD90000 // channel 17, central freq = 868.00MHz
#define CH_18_868 0xD90666 // 868.1MHz for LoRaWAN test
#define CH_00_900 0xE1C51E // channel 00, central freq = 903.08MHz
#define CH_01_900 0xE24F5C // channel 01, central freq = 905.24MHz
#define CH_02_900 0xE2D999 // channel 02, central freq = 907.40MHz
#define CH_03_900 0xE363D7 // channel 03, central freq = 909.56MHz
#define CH_04_900 0xE3EE14 // channel 04, central freq = 911.72MHz
#define CH_05_900 0xE47851 // channel 05, central freq = 913.88MHz
#define CH_06_900 0xE5028F // channel 06, central freq = 916.04MHz
#define CH_07_900 0xE58CCC // channel 07, central freq = 918.20MHz
#define CH_08_900 0xE6170A // channel 08, central freq = 920.36MHz
#define CH_09_900 0xE6A147 // channel 09, central freq = 922.52MHz
#define CH_10_900 0xE72B85 // channel 10, central freq = 924.68MHz
#define CH_11_900 0xE7B5C2 // channel 11, central freq = 926.84MHz
#define CH_12_900 0xE4C000 // default channel 915MHz, the module is configured with it

// use the following constants with setBW()
#define BW_7_8 0x00
#define BW_10_4 0x01
#define BW_15_6 0x02
#define BW_20_8 0x03
#define BW_31_25 0x04
#define BW_41_7 0x05
#define BW_62_5 0x06
#define BW_125 0x07
#define BW_250 0x08
#define BW_500 0x09
// end

//LORA CODING RATE:
#define CR_5 0x01
#define CR_6 0x02
#define CR_7 0x03
#define CR_8 0x04

//LORA SPREADING FACTOR
#define SF_7 0x07
#define SF_8 0x08
#define SF_9 0x09
#define SF_10 0x0A
#define SF_11 0x0B
#define SF_12 0x0C

//LORA MODES:
#define LORA_SLEEP_MODE 0x80
#define LORA_STANDBY_MODE 0x81
#define LORA_TX_MODE 0x83
#define LORA_RX_MODE 0x85

//const uint8_t LORA_CAD_MODE = 0x87;
#define LNA_MAX_GAIN            0x23
#define LNA_OFF_GAIN            0x00
#define LNA_LOW_GAIN		    0x20

#define LORA_STANDBY_FSK_REGS_MODE 0xC1

//FSK MODES:
#define FSK_SLEEP_MODE 0x00

//OTHER CONSTANTS:
#define BROADCAST_0 0x00
#define MAX_LENGTH 255
#define MAX_PAYLOAD 251
#define NET_KEY_LENGTH 2
#define OFFSET_PAYLOADLENGTH (4+NET_KEY_LENGTH)
#define OFFSET_RSSI 137
#define NOISE_FIGURE 6.0
#define NOISE_ABSOLUTE_ZERO 174.0
#define MAX_TIMEOUT 8000		//8000 msec = 8.0 sec
#define MAX_WAIT 12000		//12000 msec = 12.0 sec
#define MAX_RETRIES 5
#define CORRECT_PACKET 0
#define INCORRECT_PACKET 1

#define PKT_TYPE_DATA   0x10

//! Structure :
/*!
 */
typedef struct st_pack
{
	uint8_t netkey[NET_KEY_LENGTH];

	//! Structure Variable : Packet destination
	/*!
 	*/
	uint8_t dst;

    //! Structure Variable : Packet type
    /*!
    */
    uint8_t type;

	//! Structure Variable : Packet source
	/*!
 	*/
	uint8_t src;

	//! Structure Variable : Packet number
	/*!
 	*/
	uint8_t packnum;

	//! Structure Variable : Packet length
	/*!
 	*/
	uint8_t length;

	//! Structure Variable : Packet payload
	/*!
 	*/
	uint8_t data[MAX_PAYLOAD];

	//! Structure Variable : Retry number
	/*!
 	*/
	uint8_t retry;
} pack;

uint8_t sx1276_init(port * lora);
byte readRegister(byte address);

//! It writes an internal module register.
void writeRegister(byte address, byte data);
//! It sets the maximum current supply by the module.
int8_t setMaxCurrent(uint8_t rate);
//! It sets the BW, SF and CR of the module.
int8_t setMode(uint8_t mode);
//! It sets the SF.
uint8_t	setSF(uint8_t spr);
//! It sets the BW.
int8_t setBW(uint16_t band);
//! It sets the CR.
int8_t	setCR(uint8_t cod);
	//! Sets the network key
	void setNetworkKey(uint8_t key0, uint8_t key1);
	//! It sets frequency channel the module is using.
  	int8_t setChannel(uint32_t ch);
	//! It sets the node address of the mote.
  	int8_t setNodeAddress(uint8_t addr);
	//! It sends the packet wich payload is a parameter before ending MAX_TIMEOUT.
	uint8_t sendPacketTimeout(uint8_t dest, uint8_t *payload, uint16_t length);
	//! It gets the BW configured.
  	int8_t	getBW();
	//! It sets the packet length to send/receive.
  	int8_t setPacketLengthL(uint8_t l);
	//! It checks if there is an available packet and its destination before a timeout.
  	boolean	availableData(uint16_t wait);
	//! It reads a received packet from the FIFO, if it arrives before ending '_sendTime' time.
	int8_t getPacket();
	//! It receives and gets a packet from FIFO, if it arrives before ending 'wait' time.
	int8_t getPacketL(uint16_t wait);
	//! It sends the packet stored in FIFO before ending _sendTime time.
	uint8_t sendWithTimeout();
	//! It tries to send the packet stored in FIFO before ending 'wait' time.
	uint8_t sendWithTimeoutL(uint16_t wait);
	//! It sets the output power of the signal.
  	int8_t setPower(char p);

#if 0
/******************************************************************************
 * Class
 ******************************************************************************/

//! SX1276 Class
/*!
	SX1276 Class defines all the variables and functions used to manage
	SX1276 modules.
 */
class SX1276
{

public:

	//! class constructor
   	SX1276();

	//! It puts the module ON
	uint8_t ON();

	//! It puts the module OFF
	void OFF();

	//! It reads an internal module register.
	byte readRegister(byte address);

	//! It writes an internal module register.
	void writeRegister(byte address, byte data);

	//! It clears the interruption flags.
  	void clearFlags();

	//! It sets the BW, SF and CR of the module.
  	int8_t setMode(uint8_t mode);
	
	//! It is true if the SF selected exists.
  	boolean	isSF(uint8_t spr);

	//! It gets the SF configured.
  	int8_t	getSF();

	//! It sets the SF.
  	uint8_t	setSF(uint8_t spr);

	//! It is true if the BW selected exists.
  	boolean	isBW(uint16_t band);

	//! It gets the BW configured.
  	int8_t	getBW();

	//! It sets the BW.
  	int8_t setBW(uint16_t band);

	//! It is true if the CR selected exists.
  	boolean	isCR(uint8_t cod);

	//! It sets the CR.
  	int8_t	setCR(uint8_t cod);


	//! It is true if the channel selected exists.
  	boolean isChannel(uint32_t ch);


	//! It sets frequency channel the module is using.
  	int8_t setChannel(uint32_t ch);

	//! It sets the output power of the signal.
  	int8_t setPower(char p);


	//! It sets the packet length to send/receive.
  	int8_t setPacketLength();

	//! It sets the packet length to send/receive.
  	int8_t setPacketLengthL(uint8_t l);

	//! It sets the node address of the mote.
  	int8_t setNodeAddress(uint8_t addr);

	//! It gets the SNR of the latest received packet.
  	int8_t getSNR();

	//! It gets the RSSI of the latest received packet.
  	int16_t getRSSIpacket();


	//! It sets the maximum current supply by the module.
	int8_t setMaxCurrent(uint8_t rate);

	//! It sets the maximum number of bytes from a frame that fit in a packet structure.
	uint8_t truncPayload(uint16_t length16);


	//! It puts the module in reception mode.
  	uint8_t receive();

	//! It receives a packet before a timeout.
  	uint8_t receivePacketTimeout(uint16_t wait);

	//! It checks if there is an available packet and its destination before a timeout.
  	boolean	availableData(uint16_t wait);

	//! It writes a packet in FIFO in order to send it.
	uint8_t setPacket(uint8_t dest, char *payload);

	//! It writes a packet in FIFO in order to send it.
	uint8_t setPacket(uint8_t dest, uint8_t *payload);

	//! It reads a received packet from the FIFO, if it arrives before ending '_sendTime' time.
	int8_t getPacket();

	//! It receives and gets a packet from FIFO, if it arrives before ending 'wait' time.
	int8_t getPacket(uint16_t wait);

	//! It sends the packet stored in FIFO before ending _sendTime time.
	uint8_t sendWithTimeout();

	//! It tries to send the packet stored in FIFO before ending 'wait' time.
	uint8_t sendWithTimeout(uint16_t wait);

	//! It sends the packet wich payload is a parameter before ending MAX_TIMEOUT.
	uint8_t sendPacketTimeout(uint8_t dest, uint8_t *payload, uint16_t length);

	
	//! It sets the destination of a packet.
  	int8_t setDestination(uint8_t dest);

	//! It sets the waiting time to send a packet.
  	uint8_t setTimeout();

	//! It sets the payload of the packet that is going to be sent.
  	uint8_t setPayload(char *payload);
	
	//! Sets the network key
	void setNetworkKey(uint8_t key0, uint8_t key1);

	//! It sets the payload of the packet that is going to be sent.
  	uint8_t setPayload(uint8_t *payload);
    void setPacketType(uint8_t type);
	
	//! It sets the network key 
	uint8_t _my_netkey[NET_KEY_LENGTH];
	uint8_t _the_net_key_0;
	uint8_t _the_net_key_1;
	
	/// Variables /////////////////////////////////////////////////////////////

	//! Variable : bandwidth configured in LoRa mode
	uint8_t _bandwidth;

	//! Variable : coding rate configured in LoRa mode
	uint8_t _codingRate;

	//! Variable : spreading factor configured in LoRa mode
	uint8_t _spreadingFactor;

	//! Variable : frequency channel
	uint32_t _channel;

	//! Variable : output power
	uint8_t _power;

	//! Variable : SNR from the last packet received in LoRa mode.
	int8_t _SNR;

	//! Variable : RSSI from the last packet received in LoRa mode.
	int16_t _RSSIpacket;

	//! Variable : payload length sent/received.
	uint16_t _payloadlength;

	//! Variable : node address.
	uint8_t _nodeAddress;
	
	//! Variable : node address.
	uint8_t _broadcast_id;

	//! Variable : header received while waiting a packet to arrive.
	uint8_t _hreceived;


	//! Variable : packet destination.
	uint8_t _destination;

	//! Variable : packet number.
	uint8_t _packetNumber;

	//! Variable : indicates if received packet is correct or incorrect.
	//!
  	/*!
   	*/
   	uint8_t _reception;

	//! Variable : number of current retry.
	//!
  	/*!
   	*/
   	uint8_t _retries;

   	//! Variable : maximum number of retries.
	uint8_t _maxRetries;

   	//! Variable : maximum current supply.
   	uint8_t _maxCurrent;


	//! Variable : array with all the information about a sent packet.
	pack packet_sent;

	//! Variable : array with all the information about a received packet.
	pack packet_received;

	//! Variable : current timeout to send a packet.
	uint16_t _sendTime;

};

extern SX1276	sx1276;
#endif
#endif