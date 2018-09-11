#include "sx1276.h"
#include <bsp_spi.h>
#include <os.h>
#include <stdbool.h>


#define boolean bool
#define delay(x) do{OS_ERR err_os;OSTimeDlyHMSM(0, 0, 0, (x + 10) / 10, OS_OPT_NONE, &err_os);}while(0)
#define yield() delay(1)

//#define B0 0
#define B00 0
#define B000 0
#define B0000 0
#define B00000 0
#define B000000 0
#define B0000000 0
#define B00000000 0
//#define B1 1
#define B01 1
#define B001 1
#define B0001 1
#define B00001 1
#define B000001 1
#define B0000001 1
#define B00000001 1
//#define B10 2
#define B010 2
#define B0010 2
#define B00010 2
#define B000010 2
#define B0000010 2
#define B00000010 2
//#define B11 3
#define B011 3
#define B0011 3
#define B00011 3
#define B000011 3
#define B0000011 3
#define B00000011 3
#define B100 4
#define B0100 4
#define B00100 4
#define B000100 4
#define B0000100 4
#define B00000100 4
#define B101 5
#define B0101 5
#define B00101 5
#define B000101 5
#define B0000101 5
#define B00000101 5
#define B110 6
#define B0110 6
#define B00110 6
#define B000110 6
#define B0000110 6
#define B00000110 6
#define B111 7
#define B0111 7
#define B00111 7
#define B000111 7
#define B0000111 7
#define B00000111 7
#define B1000 8
#define B01000 8
#define B001000 8
#define B0001000 8
#define B00001000 8
#define B1001 9
#define B01001 9
#define B001001 9
#define B0001001 9
#define B00001001 9
#define B1010 10
#define B01010 10
#define B001010 10
#define B0001010 10
#define B00001010 10
#define B1011 11
#define B01011 11
#define B001011 11
#define B0001011 11
#define B00001011 11
#define B1100 12
#define B01100 12
#define B001100 12
#define B0001100 12
#define B00001100 12
#define B1101 13
#define B01101 13
#define B001101 13
#define B0001101 13
#define B00001101 13
#define B1110 14
#define B01110 14
#define B001110 14
#define B0001110 14
#define B00001110 14
#define B1111 15
#define B01111 15
#define B001111 15
#define B0001111 15
#define B00001111 15
#define B10000 16
#define B010000 16
#define B0010000 16
#define B00010000 16
#define B10001 17
#define B010001 17
#define B0010001 17
#define B00010001 17
#define B10010 18
#define B010010 18
#define B0010010 18
#define B00010010 18
#define B10011 19
#define B010011 19
#define B0010011 19
#define B00010011 19
#define B10100 20
#define B010100 20
#define B0010100 20
#define B00010100 20
#define B10101 21
#define B010101 21
#define B0010101 21
#define B00010101 21
#define B10110 22
#define B010110 22
#define B0010110 22
#define B00010110 22
#define B10111 23
#define B010111 23
#define B0010111 23
#define B00010111 23
#define B11000 24
#define B011000 24
#define B0011000 24
#define B00011000 24
#define B11001 25
#define B011001 25
#define B0011001 25
#define B00011001 25
#define B11010 26
#define B011010 26
#define B0011010 26
#define B00011010 26
#define B11011 27
#define B011011 27
#define B0011011 27
#define B00011011 27
#define B11100 28
#define B011100 28
#define B0011100 28
#define B00011100 28
#define B11101 29
#define B011101 29
#define B0011101 29
#define B00011101 29
#define B11110 30
#define B011110 30
#define B0011110 30
#define B00011110 30
#define B11111 31
#define B011111 31
#define B0011111 31
#define B00011111 31
#define B100000 32
#define B0100000 32
#define B00100000 32
#define B100001 33
#define B0100001 33
#define B00100001 33
#define B100010 34
#define B0100010 34
#define B00100010 34
#define B100011 35
#define B0100011 35
#define B00100011 35
#define B100100 36
#define B0100100 36
#define B00100100 36
#define B100101 37
#define B0100101 37
#define B00100101 37
#define B100110 38
#define B0100110 38
#define B00100110 38
#define B100111 39
#define B0100111 39
#define B00100111 39
#define B101000 40
#define B0101000 40
#define B00101000 40
#define B101001 41
#define B0101001 41
#define B00101001 41
#define B101010 42
#define B0101010 42
#define B00101010 42
#define B101011 43
#define B0101011 43
#define B00101011 43
#define B101100 44
#define B0101100 44
#define B00101100 44
#define B101101 45
#define B0101101 45
#define B00101101 45
#define B101110 46
#define B0101110 46
#define B00101110 46
#define B101111 47
#define B0101111 47
#define B00101111 47
#define B110000 48
#define B0110000 48
#define B00110000 48
#define B110001 49
#define B0110001 49
#define B00110001 49
#define B110010 50
#define B0110010 50
#define B00110010 50
#define B110011 51
#define B0110011 51
#define B00110011 51
#define B110100 52
#define B0110100 52
#define B00110100 52
#define B110101 53
#define B0110101 53
#define B00110101 53
#define B110110 54
#define B0110110 54
#define B00110110 54
#define B110111 55
#define B0110111 55
#define B00110111 55
#define B111000 56
#define B0111000 56
#define B00111000 56
#define B111001 57
#define B0111001 57
#define B00111001 57
#define B111010 58
#define B0111010 58
#define B00111010 58
#define B111011 59
#define B0111011 59
#define B00111011 59
#define B111100 60
#define B0111100 60
#define B00111100 60
#define B111101 61
#define B0111101 61
#define B00111101 61
#define B111110 62
#define B0111110 62
#define B00111110 62
#define B111111 63
#define B0111111 63
#define B00111111 63
#define B1000000 64
#define B01000000 64
#define B1000001 65
#define B01000001 65
#define B1000010 66
#define B01000010 66
#define B1000011 67
#define B01000011 67
#define B1000100 68
#define B01000100 68
#define B1000101 69
#define B01000101 69
#define B1000110 70
#define B01000110 70
#define B1000111 71
#define B01000111 71
#define B1001000 72
#define B01001000 72
#define B1001001 73
#define B01001001 73
#define B1001010 74
#define B01001010 74
#define B1001011 75
#define B01001011 75
#define B1001100 76
#define B01001100 76
#define B1001101 77
#define B01001101 77
#define B1001110 78
#define B01001110 78
#define B1001111 79
#define B01001111 79
#define B1010000 80
#define B01010000 80
#define B1010001 81
#define B01010001 81
#define B1010010 82
#define B01010010 82
#define B1010011 83
#define B01010011 83
#define B1010100 84
#define B01010100 84
#define B1010101 85
#define B01010101 85
#define B1010110 86
#define B01010110 86
#define B1010111 87
#define B01010111 87
#define B1011000 88
#define B01011000 88
#define B1011001 89
#define B01011001 89
#define B1011010 90
#define B01011010 90
#define B1011011 91
#define B01011011 91
#define B1011100 92
#define B01011100 92
#define B1011101 93
#define B01011101 93
#define B1011110 94
#define B01011110 94
#define B1011111 95
#define B01011111 95
#define B1100000 96
#define B01100000 96
#define B1100001 97
#define B01100001 97
#define B1100010 98
#define B01100010 98
#define B1100011 99
#define B01100011 99
#define B1100100 100
#define B01100100 100
#define B1100101 101
#define B01100101 101
#define B1100110 102
#define B01100110 102
#define B1100111 103
#define B01100111 103
#define B1101000 104
#define B01101000 104
#define B1101001 105
#define B01101001 105
#define B1101010 106
#define B01101010 106
#define B1101011 107
#define B01101011 107
#define B1101100 108
#define B01101100 108
#define B1101101 109
#define B01101101 109
#define B1101110 110
#define B01101110 110
#define B1101111 111
#define B01101111 111
#define B1110000 112
#define B01110000 112
#define B1110001 113
#define B01110001 113
#define B1110010 114
#define B01110010 114
#define B1110011 115
#define B01110011 115
#define B1110100 116
#define B01110100 116
#define B1110101 117
#define B01110101 117
#define B1110110 118
#define B01110110 118
#define B1110111 119
#define B01110111 119
#define B1111000 120
#define B01111000 120
#define B1111001 121
#define B01111001 121
#define B1111010 122
#define B01111010 122
#define B1111011 123
#define B01111011 123
#define B1111100 124
#define B01111100 124
#define B1111101 125
#define B01111101 125
#define B1111110 126
#define B01111110 126
#define B1111111 127
#define B01111111 127
#define B10000000 128
#define B10000001 129
#define B10000010 130
#define B10000011 131
#define B10000100 132
#define B10000101 133
#define B10000110 134
#define B10000111 135
#define B10001000 136
#define B10001001 137
#define B10001010 138
#define B10001011 139
#define B10001100 140
#define B10001101 141
#define B10001110 142
#define B10001111 143
#define B10010000 144
#define B10010001 145
#define B10010010 146
#define B10010011 147
#define B10010100 148
#define B10010101 149
#define B10010110 150
#define B10010111 151
#define B10011000 152
#define B10011001 153
#define B10011010 154
#define B10011011 155
#define B10011100 156
#define B10011101 157
#define B10011110 158
#define B10011111 159
#define B10100000 160
#define B10100001 161
#define B10100010 162
#define B10100011 163
#define B10100100 164
#define B10100101 165
#define B10100110 166
#define B10100111 167
#define B10101000 168
#define B10101001 169
#define B10101010 170
#define B10101011 171
#define B10101100 172
#define B10101101 173
#define B10101110 174
#define B10101111 175
#define B10110000 176
#define B10110001 177
#define B10110010 178
#define B10110011 179
#define B10110100 180
#define B10110101 181
#define B10110110 182
#define B10110111 183
#define B10111000 184
#define B10111001 185
#define B10111010 186
#define B10111011 187
#define B10111100 188
#define B10111101 189
#define B10111110 190
#define B10111111 191
#define B11000000 192
#define B11000001 193
#define B11000010 194
#define B11000011 195
#define B11000100 196
#define B11000101 197
#define B11000110 198
#define B11000111 199
#define B11001000 200
#define B11001001 201
#define B11001010 202
#define B11001011 203
#define B11001100 204
#define B11001101 205
#define B11001110 206
#define B11001111 207
#define B11010000 208
#define B11010001 209
#define B11010010 210
#define B11010011 211
#define B11010100 212
#define B11010101 213
#define B11010110 214
#define B11010111 215
#define B11011000 216
#define B11011001 217
#define B11011010 218
#define B11011011 219
#define B11011100 220
#define B11011101 221
#define B11011110 222
#define B11011111 223
#define B11100000 224
#define B11100001 225
#define B11100010 226
#define B11100011 227
#define B11100100 228
#define B11100101 229
#define B11100110 230
#define B11100111 231
#define B11101000 232
#define B11101001 233
#define B11101010 234
#define B11101011 235
#define B11101100 236
#define B11101101 237
#define B11101110 238
#define B11101111 239
#define B11110000 240
#define B11110001 241
#define B11110010 242
#define B11110011 243
#define B11110100 244
#define B11110101 245
#define B11110110 246
#define B11110111 247
#define B11111000 248
#define B11111001 249
#define B11111010 250
#define B11111011 251
#define B11111100 252
#define B11111101 253
#define B11111110 254
#define B11111111 255

static port * p_lora = NULL;


	//! It sets the network key 
static	uint8_t _my_netkey[NET_KEY_LENGTH];
static	uint8_t _the_net_key_0;
static	uint8_t _the_net_key_1;
	
	/// Variables /////////////////////////////////////////////////////////////

	//! Variable : bandwidth configured in LoRa mode
static	uint8_t _bandwidth;

	//! Variable : coding rate configured in LoRa mode
static	uint8_t _codingRate;

	//! Variable : spreading factor configured in LoRa mode
static	uint8_t _spreadingFactor;

	//! Variable : frequency channel
static	uint32_t _channel;

	//! Variable : output power
static	uint8_t _power;

	//! Variable : SNR from the last packet received in LoRa mode.
int8_t _SNR;

	//! Variable : RSSI from the last packet received in LoRa mode.
int16_t _RSSIpacket;

	//! Variable : payload length sent/received.
static	uint16_t _payloadlength;

	//! Variable : node address.
static	uint8_t _nodeAddress;
	
	//! Variable : node address.
static	uint8_t _broadcast_id;

	//! Variable : header received while waiting a packet to arrive.
static	uint8_t _hreceived;


	//! Variable : packet destination.
static	uint8_t _destination;

	//! Variable : packet number.
static	uint8_t _packetNumber;

	//! Variable : indicates if received packet is correct or incorrect.
	//!
  	/*!
   	*/
static   	uint8_t _reception;

	//! Variable : number of current retry.
	//!
  	/*!
   	*/
static   	uint8_t _retries;

   	//! Variable : maximum number of retries.
static	uint8_t _maxRetries;

   	//! Variable : maximum current supply.
static   	uint8_t _maxCurrent;


	//! Variable : array with all the information about a sent packet.
static	pack packet_sent;

	//! Variable : array with all the information about a received packet.
pack packet_received;

	//! Variable : current timeout to send a packet.
static	uint16_t _sendTime;


uint8_t sx1276_init(port * lora)
{
    uint8_t state = 2;
    _bandwidth = BW_125;
    _codingRate = CR_5;
    _spreadingFactor = SF_7;
    _channel = CH_12_900;
    _power = 15;
    _packetNumber = 0;
    _reception = CORRECT_PACKET;
    _retries = 0;
    _my_netkey[0] = 0x00;
    _my_netkey[1] = 0x00;
    _maxRetries = 3;
    packet_sent.retry = _retries;

    p_lora = lora;
    // Powering the module
//    pinMode(SX1276_SS,OUTPUT);
//    digitalWrite(SX1276_SS,HIGH);
  //  delay(100);

    //Configure the MISO, MOSI, CS, SPCR.
    //SPI.begin();
	
    //Set Most significant bit first
    //SPI.setBitOrder(MSBFIRST);

    // for the MEGA, set to 2MHz
	//SPI.setClockDivider(SPI_CLOCK_DIV8);

	//SPI.setDataMode(SPI_MODE0);

    //delay(100);
	
    packet_sent.type = PKT_TYPE_DATA;

    //pinMode(SX1276_RST,OUTPUT);
  //  digitalWrite(SX1276_RST,HIGH);
    PMOD3_PIN7_PODR = 1;
    delay(100);
    PMOD3_PIN7_PODR = 0;
//    digitalWrite(SX1276_RST,LOW);
    delay(100);
    
//    digitalWrite(SX1276_RST, LOW);
    delay(100);
//    digitalWrite(SX1276_RST, HIGH);
    PMOD3_PIN7_PODR = 1;
    delay(100);
    uint8_t version = readRegister(REG_VERSION);
    if (version != 0x12) {
      return -1;
    }
    
    setMaxCurrent(0x1B);

    // set LoRa mode
    byte st0;
    uint8_t retry=0;

    do {
        delay(200);
        writeRegister(REG_OP_MODE, FSK_SLEEP_MODE);    // Sleep mode (mandatory to set LoRa mode)
        writeRegister(REG_OP_MODE, LORA_SLEEP_MODE);    // LoRa sleep mode
        writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
        delay(50+retry*10);
        st0 = readRegister(REG_OP_MODE);
//        Serial.println(F("..."));

        if ((retry % 2)==0) {
            if (retry==20)
                retry=0;
            else
                retry++;
        }

    } while (st0!=LORA_STANDBY_MODE);	// LoRa standby mode

    if( st0 == LORA_STANDBY_MODE)
    { 
		state = 0;
    } else {
		state = 1;
		//Serial.println(F("** There has been an error while setting LoRa **"));
	}
    return state;
}

/*
 Function: Reads the indicated register.
*/
byte readRegister(byte address)
{
  byte value[2] = {0x00, 0x00};

//    digitalWrite(SX1276_SS,LOW);
//    bitClear(address, 7);		// Bit 7 cleared to write in registers
    address &= 0x7F;
    value[0] = address;
    //SPI.transfer(address);
//    value = SPI.transfer(0x00);
    p_lora->handle->readWrite(&p_lora->config, value, value, 2);
  //  digitalWrite(SX1276_SS,HIGH);
    return value[1];
}

/*
 Function: Writes on the indicated register.
*/
void writeRegister(byte address, byte data)
{
    byte buf[2];
//    digitalWrite(SX1276_SS,LOW);
  //  bitSet(address, 7);			// Bit 7 set to read from registers
    address |= 0x80;
    buf[0] = address;
    buf[1] = data;
//    SPI.transfer(address);
  //  SPI.transfer(data);
    p_lora->handle->readWrite(&p_lora->config, buf, NULL, 2);
   // digitalWrite(SX1276_SS,HIGH);
}

/*
 Function: Clears the interruption flags
*/
void clearFlags()
{
    byte st0;

    st0 = readRegister(REG_OP_MODE);		// Save the previous status

    writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby mode to write in registers
    writeRegister(REG_IRQ_FLAGS, 0xFF);	// LoRa mode flags register
    writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
}

/*
 Function: Sets the bandwidth, coding rate and spreading factor of the LoRa modulation.
*/
int8_t setMode(uint8_t mode)
{
    int8_t state = 2;
    byte st0;
    byte config1 = 0x00;
    byte config2 = 0x00;

    st0 = readRegister(REG_OP_MODE);		// Save the previous status

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode

    switch (mode)
    {
		// mode 1 (better reach, medium time on air)
		case 1:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_12);       // SF = 12
			setBW(BW_125);      // BW = 125 KHz
			break;

		// mode 2 (medium reach, less time on air)
		case 2:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_12);       // SF = 12
			setBW(BW_250);      // BW = 250 KHz
			break;

		// mode 3 (worst reach, less time on air)
		case 3:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_10);       // SF = 10
			setBW(BW_125);      // BW = 125 KHz
			break;

		// mode 4 (better reach, low time on air)
		case 4:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_12);       // SF = 12
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 5 (better reach, medium time on air)
		case 5:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_10);       // SF = 10
			setBW(BW_250);      // BW = 250 KHz
			break;

		// mode 6 (better reach, worst time-on-air)
		case 6:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_11);       // SF = 11
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 7 (medium-high reach, medium-low time-on-air)
		case 7:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_9);        // SF = 9
			setBW(BW_250);      // BW = 250 KHz
			break;

			// mode 8 (medium reach, medium time-on-air)
		case 8:     
			setCR(CR_5);        // CR = 4/5
			setSF(SF_9);        // SF = 9
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 9 (medium-low reach, medium-high time-on-air)
		case 9:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_8);        // SF = 8
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 10 (worst reach, less time_on_air)
		case 10:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_7);        // SF = 7
			setBW(BW_500);      // BW = 500 KHz
			break;
		default:    state = -1; // The indicated mode doesn't exist
    };

    if( state != -1 )	// if state = -1, don't change its value
	state = 1;
	config1 = readRegister(REG_MODEM_CONFIG1);
	switch (mode)
	{   
	// mode 1: BW = 125 KHz, CR = 4/5, SF = 12.
	case 1:

		if( (config1 >> 1) == 0x39 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_12 )
			{
				state = 0;
			}
		}
		break;


		// mode 2: BW = 250 KHz, CR = 4/5, SF = 12.
	case 2:

		if( (config1 >> 1) == 0x41 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_12 )
			{
				state = 0;
			}
		}
		break;

		// mode 3: BW = 125 KHz, CR = 4/5, SF = 10.
	case 3:

		if( (config1 >> 1) == 0x39 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_10 )
			{
				state = 0;
			}
		}
		break;

		// mode 4: BW = 500 KHz, CR = 4/5, SF = 12.
	case 4:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_12 )
			{
				state = 0;
			}
		}
		break;

		// mode 5: BW = 250 KHz, CR = 4/5, SF = 10.
	case 5:

		if( (config1 >> 1) == 0x41 )
			state=0;
		
		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_10 )
			{
				state = 0;
			}
		}
		break;

		// mode 6: BW = 500 KHz, CR = 4/5, SF = 11.
	case 6:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_11 )
			{
				state = 0;
			}
		}
		break;

		// mode 7: BW = 250 KHz, CR = 4/5, SF = 9.
	case 7:

		if( (config1 >> 1) == 0x41 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_9 )
			{
				state = 0;
			}
		}
		break;

		// mode 8: BW = 500 KHz, CR = 4/5, SF = 9.
	case 8:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_9 )
			{
				state = 0;
			}
		}
		break;

		// mode 9: BW = 500 KHz, CR = 4/5, SF = 8.
	case 9:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_8 )
			{
				state = 0;
			}
		}
		break;

		// mode 10: BW = 500 KHz, CR = 4/5, SF = 7.
	case 10:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_7 )
			{
				state = 0;
			}
		}
		break;
	}
    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Checks if SF is a valid value.
*/
boolean	isSF(uint8_t spr)
{
    // Checking available values for _spreadingFactor
    switch(spr)
    {
    case SF_7:
    case SF_8:
    case SF_9:
    case SF_10:
    case SF_11:
    case SF_12:
        return true;
        break;

    default:
        return false;
    }
}

/*
 Function: Gets the SF within the module is configured.
*/
int8_t	getSF()
{
    int8_t state = 2;
    byte config2;

	// take out bits 7-4 from REG_MODEM_CONFIG2 indicates _spreadingFactor
	config2 = (readRegister(REG_MODEM_CONFIG2)) >> 4;
	_spreadingFactor = config2;
	state = 1;

	if( (config2 == _spreadingFactor) && isSF(_spreadingFactor) )
	{
		state = 0;
	}
    return state;
}

/*
 Function: Sets the indicated SF in the module.
*/
uint8_t	setSF(uint8_t spr)
{
    byte st0;
    int8_t state = 2;
    byte config1;
    byte config2;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode
	config2 = (readRegister(REG_MODEM_CONFIG2));	// Save config2 to modify SF value (bits 7-4)
	switch(spr)
	{
	case SF_7: 	config2 = config2 & B01111111;	// clears bits 7 from REG_MODEM_CONFIG2
		config2 = config2 | B01110000;	// sets bits 6, 5 & 4
		break;
	case SF_8: 	config2 = config2 & B10001111;	// clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B10000000;	// sets bit 7 from REG_MODEM_CONFIG2
		break;
	case SF_9: 	config2 = config2 & B10011111;	// clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B10010000;	// sets bits 7 & 4 from REG_MODEM_CONFIG2
		break;
	case SF_10:	config2 = config2 & B10101111;	// clears bits 6 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B10100000;	// sets bits 7 & 5 from REG_MODEM_CONFIG2
		break;
	case SF_11:	config2 = config2 & B10111111;	// clears bit 6 from REG_MODEM_CONFIG2
		config2 = config2 | B10110000;	// sets bits 7, 5 & 4 from REG_MODEM_CONFIG2
		getBW();

		if( _bandwidth == BW_125)
		{ // LowDataRateOptimize (Mandatory with SF_11 if BW_125)
			config1 = (readRegister(REG_MODEM_CONFIG1));	// Save config1 to modify only the LowDataRateOptimize
			config1 = config1 | B00000001;
			writeRegister(REG_MODEM_CONFIG1,config1);
		}
		break;
	case SF_12: config2 = config2 & B11001111;	// clears bits 5 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B11000000;	// sets bits 7 & 6 from REG_MODEM_CONFIG2
		if( _bandwidth == BW_125)
		{ // LowDataRateOptimize (Mandatory with SF_12 if BW_125)
			byte config3=readRegister(REG_MODEM_CONFIG3);
			config3 = config3 | B00001000;
			writeRegister(REG_MODEM_CONFIG3,config3);
		}
		break;
	}

	// Turn on header
	config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the header bit
	config1 = config1 & B11111110;              // clears bit 0 from config1 = headerON
	writeRegister(REG_MODEM_CONFIG1,config1);	// Update config1
	
	
	// LoRa detection Optimize: 0x03 --> SF7 to SF12
	writeRegister(REG_DETECT_OPTIMIZE, 0x03);

	// LoRa detection threshold: 0x0A --> SF7 to SF12
	writeRegister(REG_DETECTION_THRESHOLD, 0x0A);

	uint8_t config3 = (readRegister(REG_MODEM_CONFIG3));
	config3=config3 | B00000100;
	writeRegister(REG_MODEM_CONFIG3, config3);

	// here we write the new SF
	writeRegister(REG_MODEM_CONFIG2, config2);		// Update config2

	delay(100);

	byte configAgc;
	uint8_t theLDRBit;

	config1 = (readRegister(REG_MODEM_CONFIG3));	// Save config1 to check update
	config2 = (readRegister(REG_MODEM_CONFIG2));

	// LowDataRateOptimize is in REG_MODEM_CONFIG3
	// AgcAutoOn is in REG_MODEM_CONFIG3
	configAgc=config1;
	theLDRBit=3;
	

	switch(spr)
	{
	case SF_7:	if(		((config2 >> 4) == 0x07)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_8:	if(		((config2 >> 4) == 0x08)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_9:	if(		((config2 >> 4) == 0x09)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_10:	if(		((config2 >> 4) == 0x0A)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_11:	if(		((config2 >> 4) == 0x0B)
						&& (bitRead(configAgc, 2) == 1)
						&& (bitRead(config1, theLDRBit) == 1))
		{
			state = 0;
		}
		break;
	case SF_12:	if(		((config2 >> 4) == 0x0C)
						&& (bitRead(configAgc, 2) == 1)
						&& (bitRead(config1, theLDRBit) == 1))
		{
			state = 0;
		}
		break;
	default:	state = 1;
	}

    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);

    if( isSF(spr) )
    { // Checking available value for _spreadingFactor
        state = 0;
        _spreadingFactor = spr;
    }
    return state;
}

/*
 Function: Checks if BW is a valid value.
*/
boolean	isBW(uint16_t band)
{
    // Checking available values for _bandwidth
	switch(band)
	{
	case BW_7_8:
	case BW_10_4:
	case BW_15_6:
	case BW_20_8:
	case BW_31_25:
	case BW_41_7:
	case BW_62_5:
	case BW_125:
	case BW_250:
	case BW_500:
		return true;
		break;

	default:
		return false;
	}
}

/*
 Function: Gets the BW within the module is configured.
*/
int8_t	getBW()
{
    uint8_t state = 2;
    byte config1;

	config1 = (readRegister(REG_MODEM_CONFIG1)) >> 4;
	
	_bandwidth = config1;

	if( (config1 == _bandwidth) && isBW(_bandwidth) )
	{
		state = 0;
	}
	else
	{
		state = 1;
	}
    return state;
}

/*
 Function: Sets the indicated BW in the module.
*/
int8_t	setBW(uint16_t band)
{
    byte st0;
    int8_t state = 2;
    byte config1;

    if(!isBW(band) )
    {
        state = 1;
        return state;
    }

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
	
    writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode
    config1 = (readRegister(REG_MODEM_CONFIG1));	// Save config1 to modify only the BW

	config1 = config1 & B00001111;	// clears bits 7 - 4 from REG_MODEM_CONFIG1
	switch(band)
	{
	case BW_125:
		// 0111
		config1 = config1 | B01110000;
		getSF();
		if( _spreadingFactor == 11 || _spreadingFactor == 12)
		{ // LowDataRateOptimize (Mandatory with BW_125 if SF_11 or SF_12)
			byte config3=readRegister(REG_MODEM_CONFIG3);
			config3 = config3 | B00001000;
			writeRegister(REG_MODEM_CONFIG3,config3);
		}
		break;
	case BW_250:
		// 1000
		config1 = config1 | B10000000;
		break;
	case BW_500:
		// 1001
		config1 = config1 | B10010000;
		break;
	}

    writeRegister(REG_MODEM_CONFIG1,config1);		// Update config1

    delay(100);

    config1 = (readRegister(REG_MODEM_CONFIG1));

	switch(band)
	{
	case BW_125: if( (config1 >> 4) == BW_125 )
		{
			state = 0;

			byte config3 = (readRegister(REG_MODEM_CONFIG3));

			if( _spreadingFactor == 11 )
			{
				if( bitRead(config3, 3) == 1 )
				{ // LowDataRateOptimize
					state = 0;
				}
				else
				{
					state = 1;
				}
			}
			if( _spreadingFactor == 12 )
			{
				if( bitRead(config3, 3) == 1 )
				{ // LowDataRateOptimize
					state = 0;
				}
				else
				{
					state = 1;
				}
			}
		}
		break;
	case BW_250: if( (config1 >> 4) == BW_250 )
		{
			state = 0;
		}
		break;
	case BW_500: if( (config1 >> 4) == BW_500 )
		{
			state = 0;
		}
		break;
	}

    if(state==0)
    {
        _bandwidth = band;
    }
    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Checks if CR is a valid value.
*/
boolean	isCR(uint8_t cod)
{
    // Checking available values for _codingRate
    switch(cod)
    {
    case CR_5:
    case CR_6:
    case CR_7:
    case CR_8:
        return true;
        break;

    default:
        return false;
    }
}

/*
 Function: Sets the indicated CR in the module.
*/
int8_t	setCR(uint8_t cod)
{
    byte st0;
    int8_t state = 2;
    byte config1;

    st0 = readRegister(REG_OP_MODE);		// Save the previous status

    writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);		// Set Standby mode to write in registers

    config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the CR
	config1 = config1 & B11110001;	// clears bits 3 - 1 from REG_MODEM_CONFIG1
	switch(cod)
	{
	case CR_5:
		config1 = config1 | B00000010;
		break;
	case CR_6:
		config1 = config1 | B00000100;
		break;
	case CR_7:
		config1 = config1 | B00000110;
		break;
	case CR_8:
		config1 = config1 | B00001000;
		break;
	}

    writeRegister(REG_MODEM_CONFIG1, config1);		// Update config1

    delay(100);

    config1 = readRegister(REG_MODEM_CONFIG1);

    uint8_t nshift=1;

    switch(cod)
    {
    case CR_5: if( ((config1 >> nshift) & B0000111) == 0x01 )
        {
            state = 0;
        }
        break;
    case CR_6: if( ((config1 >> nshift) & B0000111) == 0x02 )
        {
            state = 0;
        }
        break;
    case CR_7: if( ((config1 >> nshift) & B0000111) == 0x03 )
        {
            state = 0;
        }
        break;
    case CR_8: if( ((config1 >> nshift) & B0000111) == 0x04 )
        {
            state = 0;
        }
        break;
    }


    if( isCR(cod) )
    {
        _codingRate = cod;
    }
    else
    {
        state = 1;
    }
    writeRegister(REG_OP_MODE,st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Checks if channel is a valid value.
*/
boolean	isChannel(uint32_t ch)
{
    // Checking available values for _channel
    switch(ch)
    {
    case CH_10_868:
    case CH_11_868:
    case CH_12_868:
    case CH_13_868:
    case CH_14_868:
    case CH_15_868:
    case CH_16_868:
    case CH_17_868:
    case CH_18_868:
    case CH_00_900:
    case CH_01_900:
    case CH_02_900:
    case CH_03_900:
    case CH_04_900:
    case CH_05_900:
    case CH_06_900:
    case CH_07_900:
    case CH_08_900:
    case CH_09_900:
    case CH_10_900:
    case CH_11_900:
    case CH_12_900:
        return true;
        break;

    default:
        return false;
    }
}

/*
 Function: Sets the indicated channel in the module.
*/
int8_t setChannel(uint32_t ch)
{
    byte st0;
    int8_t state = 2;
    unsigned int freq3;
    unsigned int freq2;
    uint8_t freq1;
    uint32_t freq;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
    
    freq3 = ((ch >> 16) & 0x0FF);		// frequency channel MSB
    freq2 = ((ch >> 8) & 0x0FF);		// frequency channel MIB
    freq1 = (ch & 0xFF);				// frequency channel LSB

    writeRegister(REG_FRF_MSB, freq3);
    writeRegister(REG_FRF_MID, freq2);
    writeRegister(REG_FRF_LSB, freq1);

    delay(100);

    // storing MSB in freq channel value
    freq3 = (readRegister(REG_FRF_MSB));
    freq = (freq3 << 8) & 0xFFFFFF;

    // storing MID in freq channel value
    freq2 = (readRegister(REG_FRF_MID));
    freq = (freq << 8) + ((freq2 << 8) & 0xFFFFFF);

    // storing LSB in freq channel value
    freq = freq + ((readRegister(REG_FRF_LSB)) & 0xFFFFFF);

    if( freq == ch )
    {
        state = 0;
        _channel = ch;
    }
    else
    {
        state = 1;
    }

    if(!isChannel(ch) )
    {
        state = -1;
    }

    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Sets the signal power indicated in the module.
*/
int8_t setPower(char p)
{
    byte st0;
    int8_t state = 2;
    byte value = 0x00;
	byte RegPaDacReg=0x4D;

    st0 = readRegister(REG_OP_MODE);	  // Save the previous status
	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
    
    switch (p)
    {
    // L = Low. On SX1272/76: PA0 on RFO setting
    // H = High. On SX1272/76: PA0 on RFO setting
    // M = MAX. On SX1272/76: PA0 on RFO setting
    // X = extreme. On SX1272/76: PA1&PA2 PA_BOOST setting + 20dBm settings

    case 'X':
    case 'M':  value = 0x0F;
        // SX1272/76: 14dBm
        break;
	case 'L':  value = 0x03;
        // SX1272/76: 2dBm
        break;

    case 'H':  value = 0x07;
        // SX1272/76: 6dBm
        break;

    default:   state = -1;
        break;
    }

    // 100mA
    setMaxCurrent(0x0B);

    if (p=='X') {
        // normally value = 0x0F;
        // we set the PA_BOOST pin
        value = value | B10000000;
        // and then set the high output power config with register REG_PA_DAC
        writeRegister(RegPaDacReg, 0x87);
        // set RegOcp for OcpOn and OcpTrim
        // 150mA
        setMaxCurrent(0x12);
    }
    else {
        // disable high power output in all other cases
        writeRegister(RegPaDacReg, 0x84);
    }

	// set MaxPower to 7 -> Pmax=10.8+0.6*MaxPower [dBm] = 15
	value = value | B01110000;

	// then Pout = Pmax-(15-_power[3:0]) if  PaSelect=0 (RFO pin for +14dBm)
	// so L=3dBm; H=7dBm; M=15dBm (but should be limited to 14dBm by RFO pin)

	// and Pout = 17-(15-_power[3:0]) if  PaSelect=1 (PA_BOOST pin for +14dBm)
	// when p=='X' for 20dBm, value is 0x0F and RegPaDacReg=0x87 so 20dBm is enabled

	writeRegister(REG_PA_CONFIG, value);

    _power=value;

    value = readRegister(REG_PA_CONFIG);

    if( value == _power )
    {
        state = 0;
    }
    else
    {
        state = 1;
    }

    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Sets the packet length in the module.
*/
int8_t setPacketLength()
{
    uint16_t length;

	length = _payloadlength + OFFSET_PAYLOADLENGTH;

    return setPacketLengthL(length);
}

/*
 Function: Sets the packet length in the module.
*/
int8_t setPacketLengthL(uint8_t l)
{
    byte st0;
    byte value = 0x00;
    int8_t state = 2;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
    packet_sent.length = l;

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // Set LoRa Standby mode to write in registers
	writeRegister(REG_PAYLOAD_LENGTH_LORA, packet_sent.length);
	// Storing payload length in LoRa mode
	value = readRegister(REG_PAYLOAD_LENGTH_LORA);
    
    if( packet_sent.length == value )
    {
        state = 0;
    }
    else
    {
        state = 1;
    }

    writeRegister(REG_OP_MODE, st0);
    return state;
}


/*
 Function: Sets the node address in the module.
*/
int8_t setNodeAddress(uint8_t addr)
{
    byte st0;
    byte value;
    uint8_t state = 2;

    if( addr > 255 )
    {
        state = -1;
    }
    else
    {
        // Saving node address
        _nodeAddress = addr;
        st0 = readRegister(REG_OP_MODE);	  // Save the previous status

		writeRegister(REG_OP_MODE, LORA_STANDBY_FSK_REGS_MODE);
        
        // Storing node and broadcast address
        writeRegister(REG_NODE_ADRS, addr);
        writeRegister(REG_BROADCAST_ADRS, BROADCAST_0);

        value = readRegister(REG_NODE_ADRS);
        writeRegister(REG_OP_MODE, st0);		// Getting back to previous status

        if( value == _nodeAddress )
        {
            state = 0;
        }
        else
        {
            state = 1;
        }
    }
    return state;
}

/*
 Function: Gets the SNR value in LoRa mode.
 */
int8_t getSNR()
{	// getSNR exists only in LoRa mode
    int8_t state = 0;
    byte value;

	value = readRegister(REG_PKT_SNR_VALUE);
	if( value & 0x80 ) // The SNR sign bit is 1
	{
		// Invert and divide by 4
		value = ( ( ~value + 1 ) & 0xFF ) >> 2;
		_SNR = -value;
	}
	else
	{
		// Divide by 4
		_SNR = ( value & 0xFF ) >> 2;
	}
    return state;
}

/*
 Function: Gets the RSSI of the last packet received in LoRa mode.
 */
int16_t getRSSIpacket()
{	// RSSIpacket only exists in LoRa
    int8_t state = 2;

	state = getSNR();
	if( state == 0 )
	{
		// added by C. Pham
		_RSSIpacket = readRegister(REG_PKT_RSSI_VALUE);

		if( _SNR < 0 )
		{
			_RSSIpacket = -OFFSET_RSSI + (double)_RSSIpacket + (double)_SNR*0.25;
			state = 0;
		}
		else
		{
			_RSSIpacket = -OFFSET_RSSI + (double)_RSSIpacket;
			//end
			state = 0;
		}
	}
    return state;
}



/*
 Function: Limits the current supply of the power amplifier, protecting battery chemistries.
*/
int8_t setMaxCurrent(uint8_t rate)
{
    int8_t state = 2;
    byte st0;

    // Maximum rate value = 0x1B, because maximum current supply = 240 mA
    if (rate > 0x1B)
    {
        state = -1;
    }
    else
    {
        // Enable Over Current Protection
        rate |= B00100000;

        st0 = readRegister(REG_OP_MODE);	// Save the previous status
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Set LoRa Standby mode to write in registers
        writeRegister(REG_OCP, rate);		// Modifying maximum current supply
        writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
        state = 0;
    }
    return state;
}


/*
 Function: It truncs the payload length if it is greater than 0xFF.
*/
uint8_t truncPayload(uint16_t length16)
{
    uint8_t state = 0;

    if( length16 > MAX_PAYLOAD )
    {
        _payloadlength = MAX_PAYLOAD;
    }
    else
    {
        _payloadlength = (length16 & 0xFF);
    }
    return state;
}


/*
 Function: Configures the module to receive information.
*/
uint8_t receive()
{
    uint8_t state = 1;

    // Initializing packet_received struct
    memset( &packet_received, 0x00, sizeof(packet_received) );

    // Set LowPnTxPllOff
    writeRegister(REG_PA_RAMP, 0x08);

    writeRegister(REG_LNA, LNA_MAX_GAIN);
    writeRegister(REG_FIFO_ADDR_PTR, 0x00);  // Setting address pointer in FIFO data buffer
    
    if (_spreadingFactor == SF_10 || _spreadingFactor == SF_11 || _spreadingFactor == SF_12) {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x05);
    } else {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x08);
    }
    
    writeRegister(REG_FIFO_RX_BYTE_ADDR, 0x00); // Setting current value of reception buffer pointer
	state = setPacketLengthL(MAX_LENGTH);	// With MAX_LENGTH gets all packets with length < MAX_LENGTH
	writeRegister(REG_OP_MODE, LORA_RX_MODE);  	  // LORA mode - Rx
    return state;
}



/*
 Function: Configures the module to receive information.
*/
uint8_t receivePacketTimeout(uint16_t wait)
{
    uint8_t state = 2;
    uint8_t state_f = 2;

    state = receive();
    if( state == 0 )
    {
        if( availableData(wait) )
        {
            // If packet received, getPacket
            state_f = getPacket();
        }
        else
        {
            state_f = 1;
        }
    }
    else
    {
        state_f = state;
    }
    return state_f;
}

static unsigned long millis() {
  OS_ERR err_os;
  return OSTimeGet(&err_os) * (1000 / OS_CFG_TICK_RATE_HZ);
}

/*
 Function: If a packet is received, checks its destination.
*/
boolean	availableData(uint16_t wait)
{
    byte value;
    byte header = 0;
    boolean forme = false;
    boolean	_hreceived = false;
    unsigned long previous;

    previous = millis();
   
	value = readRegister(REG_IRQ_FLAGS);
	// Wait to Valid Header interrupt
	while( (bitRead(value, 4) == 0) && (millis() - previous < (unsigned long)wait) )
	{
		yield();
		value = readRegister(REG_IRQ_FLAGS);
		// Condition to avoid an overflow (DO NOT REMOVE)
		if( millis() < previous )
		{
			previous = millis();
		}
	} // end while (millis)

	if( bitRead(value, 4) == 1 )
	{ // header received
		_hreceived = true;

		// actually, need to wait until 3 bytes have been received
		while( (header < 3) && (millis() - previous < (unsigned long)wait) )
		{ // Waiting to read first payload bytes from packet
			yield();
			header = readRegister(REG_FIFO_RX_BYTE_ADDR);
			// Condition to avoid an overflow (DO NOT REMOVE)
			if( millis() < previous )
			{
				previous = millis();
			}
		}

		if( header != 0 )
		{ // Reading first byte of the received packet
			_the_net_key_0 = readRegister(REG_FIFO);
			_the_net_key_1 = readRegister(REG_FIFO);
			_destination = readRegister(REG_FIFO);
		}
	}
	else
	{
		forme = false;
		_hreceived = false;
	}
    // We use _hreceived because we need to ensure that _destination value is correctly
    // updated and is not the _destination value from the previously packet
    if( _hreceived == true )
    { // Checking destination
        forme=true;

		if (_the_net_key_0!=_my_netkey[0] || _the_net_key_1!=_my_netkey[1]) {
			forme=false;
		}
		else
		{
		}
			
        if( forme && ((_destination == _nodeAddress) || (_destination == BROADCAST_0)) )
		{ 
            forme = true;
        }
        else
        {
            forme = false;
            writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Setting standby LoRa mode
        }
    }
    return forme;
}

/*
 Function: It gets and stores a packet if it is received.
*/
int8_t getPacket()
{
    return getPacketL(MAX_TIMEOUT);
}

/*
 Function: It gets and stores a packet if it is received before ending 'wait' time.
*/
int8_t getPacketL(uint16_t wait)
{
    uint8_t state = 2;
    byte value = 0x00;
    unsigned long previous;
    boolean p_received = false;

    previous = millis();
	value = readRegister(REG_IRQ_FLAGS);
	// Wait until the packet is received (RxDone flag) or the timeout expires
	while( (bitRead(value, 6) == 0) && (millis() - previous < (unsigned long)wait) )
	{
		value = readRegister(REG_IRQ_FLAGS);
		// Condition to avoid an overflow (DO NOT REMOVE)
		if( millis() < previous )
		{
			previous = millis();
		}
	} // end while (millis)

	if( (bitRead(value, 6) == 1) && (bitRead(value, 5) == 0) )
	{ // packet received & CRC correct
		p_received = true;	// packet correctly received
		_reception = CORRECT_PACKET;
	}
	else
	{
		if( bitRead(value, 5) != 0 )
		{ // CRC incorrect
			_reception = INCORRECT_PACKET;
			state = 3;
		}
	}
    if( p_received == true )
    {
        // Store the packet
		writeRegister(REG_FIFO_ADDR_PTR, 0x00);  	// Setting address pointer in FIFO data buffer

		packet_received.netkey[0]=readRegister(REG_FIFO);
		packet_received.netkey[1]=readRegister(REG_FIFO);
		packet_received.dst = readRegister(REG_FIFO);	// Storing first byte of the received packet		
		packet_received.type = readRegister(REG_FIFO);		// Reading second byte of the received packet
		packet_received.src = readRegister(REG_FIFO);		// Reading second byte of the received packet
		packet_received.packnum = readRegister(REG_FIFO);	// Reading third byte of the received packet
        packet_received.length = readRegister(REG_RX_NB_BYTES);
		_payloadlength = packet_received.length - OFFSET_PAYLOADLENGTH;

        if( packet_received.length > (MAX_LENGTH + 1) )
        {
        }
        else
        {
            for(unsigned int i = 0; i < _payloadlength; i++)
            {
                packet_received.data[i] = readRegister(REG_FIFO); // Storing payload
            }
            state = 0;
        }
    }
    else
    {
        state = 1;
        if( (_reception == INCORRECT_PACKET) && (_retries < _maxRetries) )
        {
            _retries++;
        }
    }
	writeRegister(REG_FIFO_ADDR_PTR, 0x00);  // Setting address pointer in FIFO data buffer
    
	clearFlags();	// Initializing flags
    if( wait > MAX_WAIT )
    {
        state = -1;
    }

    return state;
}

/*
 Function: It sets the packet destination.
*/
int8_t setDestination(uint8_t dest)
{
    int8_t state = 0;
    _destination = dest; // Storing destination in a global variable
    packet_sent.dst = dest;	 // Setting destination in packet structure
    packet_sent.src = _nodeAddress; // Setting source in packet structure
    packet_sent.packnum = _packetNumber;	// Setting packet number in packet structure
    _packetNumber++;
    return state;
}

/*
 Function: It sets the network key
*/
void setNetworkKey(uint8_t key0, uint8_t key1)
{
	_my_netkey[0] = key0;
    _my_netkey[1] = key1;
}

/*
 Function: It sets the timeout according to the configured mode.
*/
uint8_t setTimeout()
{
    uint8_t state = 0;
    uint16_t delay;

	switch(_spreadingFactor)
	{	// Choosing Spreading Factor
	case SF_7:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 408;
				break;
			case CR_6: _sendTime = 438;
				break;
			case CR_7: _sendTime = 468;
				break;
			case CR_8: _sendTime = 497;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 325;
				break;
			case CR_6: _sendTime = 339;
				break;
			case CR_7: _sendTime = 355;
				break;
			case CR_8: _sendTime = 368;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 282;
				break;
			case CR_6: _sendTime = 290;
				break;
			case CR_7: _sendTime = 296;
				break;
			case CR_8: _sendTime = 305;
				break;
			}
			break;
		}
		break;

	case SF_8:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 537;
				break;
			case CR_6: _sendTime = 588;
				break;
			case CR_7: _sendTime = 640;
				break;
			case CR_8: _sendTime = 691;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 388;
				break;
			case CR_6: _sendTime = 415;
				break;
			case CR_7: _sendTime = 440;
				break;
			case CR_8: _sendTime = 466;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 315;
				break;
			case CR_6: _sendTime = 326;
				break;
			case CR_7: _sendTime = 340;
				break;
			case CR_8: _sendTime = 352;
				break;
			}
			break;
		}
		break;

	case SF_9:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 774;
				break;
			case CR_6: _sendTime = 864;
				break;
			case CR_7: _sendTime = 954;
				break;
			case CR_8: _sendTime = 1044;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 506;
				break;
			case CR_6: _sendTime = 552;
				break;
			case CR_7: _sendTime = 596;
				break;
			case CR_8: _sendTime = 642;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 374;
				break;
			case CR_6: _sendTime = 396;
				break;
			case CR_7: _sendTime = 418;
				break;
			case CR_8: _sendTime = 441;
				break;
			}
			break;
		}
		break;

	case SF_10:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1226;
				break;
			case CR_6: _sendTime = 1388;
				break;
			case CR_7: _sendTime = 1552;
				break;
			case CR_8: _sendTime = 1716;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 732;
				break;
			case CR_6: _sendTime = 815;
				break;
			case CR_7: _sendTime = 896;
				break;
			case CR_8: _sendTime = 977;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 486;
				break;
			case CR_6: _sendTime = 527;
				break;
			case CR_7: _sendTime = 567;
				break;
			case CR_8: _sendTime = 608;
				break;
			}
			break;
		}
		break;

	case SF_11:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 2375;
				break;
			case CR_6: _sendTime = 2735;
				break;
			case CR_7: _sendTime = 3095;
				break;
			case CR_8: _sendTime = 3456;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1144;
				break;
			case CR_6: _sendTime = 1291;
				break;
			case CR_7: _sendTime = 1437;
				break;
			case CR_8: _sendTime = 1586;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 691;
				break;
			case CR_6: _sendTime = 766;
				break;
			case CR_7: _sendTime = 838;
				break;
			case CR_8: _sendTime = 912;
				break;
			}
			break;
		}
		break;

	case SF_12: switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 4180;
				break;
			case CR_6: _sendTime = 4836;
				break;
			case CR_7: _sendTime = 5491;
				break;
			case CR_8: _sendTime = 6146;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1965;
				break;
			case CR_6: _sendTime = 2244;
				break;
			case CR_7: _sendTime = 2521;
				break;
			case CR_8: _sendTime = 2800;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1102;
				break;
			case CR_6: _sendTime = 1241;
				break;
			case CR_7: _sendTime = 1381;
				break;
			case CR_8: _sendTime = 1520;
				break;
			}
			break;
		}
		break;
	default: _sendTime = MAX_TIMEOUT;
	}
    delay = ((0.1*_sendTime) + 1);
   _sendTime = (uint16_t) ((_sendTime * 1.2) + (rand()%delay));
    return state;
}

/*
 Function: It sets an uint8_t array payload packet in a packet struct.
*/
uint8_t setPayload(uint8_t *payload)
{
    uint8_t state = 1;
    
	for(unsigned int i = 0; i < _payloadlength; i++)
    {
        packet_sent.data[i] = payload[i];	// Storing payload in packet structure
    }
    // set length with the actual counter value
    state = setPacketLength();	// Setting packet length in packet structure
    return state;
}

/*
 Function: It sets a packet struct in FIFO in order to sent it.
*/
uint8_t setPacket(uint8_t dest, uint8_t *payload)
{
    int8_t state = 2;
    byte st0;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
    clearFlags();	// Initializing flags

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby LoRa mode to write in FIFO
    
    _reception = CORRECT_PACKET;	// Updating incorrect value to send a packet (old or new)
    if( _retries == 0 )
    { // Sending new packet
        state = setDestination(dest);	// Setting destination in packet structure
        packet_sent.retry = _retries;
        if( state == 0 )
        {
            state = setPayload(payload);
        }
    }
    else
    {
        if( _retries == 1 )
        {
            packet_sent.length++;
        }
        state = setPacketLength();
        packet_sent.retry = _retries;
    }

    packet_sent.type |= PKT_TYPE_DATA;

    writeRegister(REG_FIFO_ADDR_PTR, 0x80);  // Setting address pointer in FIFO data buffer
    if( state == 0 )
    {
        state = 1;
        // Writing packet to send in FIFO
        packet_sent.netkey[0]=_my_netkey[0];
        packet_sent.netkey[1]=_my_netkey[1];
        writeRegister(REG_FIFO, packet_sent.netkey[0]);
        writeRegister(REG_FIFO, packet_sent.netkey[1]);

		writeRegister(REG_FIFO, packet_sent.dst); 		// Writing the destination in FIFO
		writeRegister(REG_FIFO, packet_sent.type); 		// Writing the packet type in FIFO
		writeRegister(REG_FIFO, packet_sent.src);		// Writing the source in FIFO
		writeRegister(REG_FIFO, packet_sent.packnum);	// Writing the packet number in FIFO

        for(unsigned int i = 0; i < _payloadlength; i++)
        {
            writeRegister(REG_FIFO, packet_sent.data[i]);  // Writing the payload in FIFO
        }
        state = 0;
    }
    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    return state;
}


/*
 Function: Configures the module to transmit information.
*/
uint8_t sendWithTimeout()
{
    setTimeout();
    return sendWithTimeoutL(_sendTime);
}

/*
 Function: Configures the module to transmit information.
*/
uint8_t sendWithTimeoutL(uint16_t wait)
{
    uint8_t state = 2;
    byte value = 0x00;
    unsigned long previous;

    // wait to TxDone flag
    previous = millis();
    
    clearFlags();	// Initializing flags

	writeRegister(REG_OP_MODE, LORA_TX_MODE);  // LORA mode - Tx

	value = readRegister(REG_IRQ_FLAGS);
	// Wait until the packet is sent (TX Done flag) or the timeout expires
	while ((bitRead(value, 3) == 0) && (millis() - previous < wait))
	{
		value = readRegister(REG_IRQ_FLAGS);
		// Condition to avoid an overflow (DO NOT REMOVE)
		if( millis() < previous )
		{
			previous = millis();
		}
	}
	state = 1;

    if( bitRead(value, 3) == 1 )
    {
        state = 0;	// Packet successfully sent
    }
    clearFlags();		// Initializing flags
    return state;
}

/*
 Function: Configures the module to transmit information.
*/
uint8_t sendPacketTimeout(uint8_t dest, uint8_t *payload, uint16_t length16)
{
    uint8_t state = 2;
    uint8_t state_f = 2;

    state = truncPayload(length16);
    if( state == 0 )
    {
        state_f = setPacket(dest, payload);	// Setting a packet with 'dest' destination
    }												// and writing it in FIFO.
    else
    {
        state_f = state;
    }
    if( state_f == 0 )
    {
        state_f = sendWithTimeout();	// Sending the packet
    }
    return state_f;
}