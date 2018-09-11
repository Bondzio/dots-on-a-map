/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                        BLE Utility Functions
*
* Filename      : app_util.c
* Version       : V1.00
* Programmer(s) : Medium One
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <string.h>
#include  <stdio.h>
#include  <stdint.h>
#include  <os.h>
#include  "app_util.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
// CRC parameters (values for CRC-16 x.25?):
// 'order' [1..32] is the CRC polynom order, counted without the leading '1' bit
// 'crcinit' is the initial CRC value belonging to that algorithm
#define order 16
#define crcinit 0xffff

/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

// CRC parameters (values for CRC-16 x.25?):
// 'polynom' is the CRC polynom without leading '1' bit
// 'direct' [0,1] specifies the kind of algorithm: 1=direct, no augmented zero bits
// 'crcxor' is the final XOR value
// 'refin' [0,1] specifies if a data byte is reflected before processing (UART) or not
// 'refout' [0,1] specifies if the CRC will be reflected before XOR

const unsigned long polynom = 0x1021;
const int direct = 1;
const unsigned long crcxor = 0xffff;
const int refin = 1;
const int refout = 1;


/*
*********************************************************************************************************
*                                          LOCAL VARIABLES
*********************************************************************************************************
*/

unsigned long crcmask = 0xffff;
unsigned long crchighbit = (unsigned long)1<<(order-1);
unsigned long crcinit_direct = crcinit;
unsigned long crctab[256];

static  char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static  char decoding_table[256];
static  int mod_table[] = {0, 2, 1};

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

char zeroThroughNineToStr(int zeroThroughNine)
{
  switch (zeroThroughNine) {
  case 0:
    return '0';
  case 1:
    return '1';
  case 2:
    return '2';
  case 3:
    return '3';
  case 4:
    return '4';
  case 5:
    return '5';
  case 6:
    return '6';
  case 7:
    return '7';
  case 8:
    return '8';
  case 9:
    return '9';
  default:
    return '?';
  }
}


void intToStr (int baseTenVal, char * stringOut)
{
    char stringTmp[64];
    int count = 0;
    char *pStringTmpChar = stringTmp;
    char *pStringOutChar = stringOut;
    int iterationVal = baseTenVal;

    do {
      *pStringTmpChar++ = zeroThroughNineToStr(iterationVal % 10);
      iterationVal /= 10;
      count++;
    } while (iterationVal);

    for (int i = 0; i < count; i++)
      *pStringOutChar++ = *--pStringTmpChar;

    *pStringOutChar = '\0';
}

int base64_encode(const unsigned char *data,
                    size_t input_length,
                    char * encoded_data,
                    size_t encoded_data_length,
                    size_t *output_length) {

    *output_length = 4 * ((input_length + 2) / 3);
    if ((*output_length + 1)> encoded_data_length) return -1; // null term

    if (encoded_data == NULL) return -2;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';
    encoded_data[*output_length] = '\0';
    return 0;
}


int base64_decode(const char *data,
                             size_t input_length,
                             unsigned char * decoded_data,
                             size_t decoded_data_length,
                             size_t *output_length) {

    if (input_length % 4 != 0) return -1;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    if (decoded_data == NULL) return -2;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return 0;
}


void decoding_table_init (void)
{
    for (int i=0; i<64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


static unsigned long reflect (unsigned long crc, int bitnum) {

	// reflects the lower 'bitnum' bits of 'crc'

	unsigned long i, j=1, crcout=0;

	for (i=(unsigned long)1<<(bitnum-1); i; i>>=1) {
		if (crc & i) crcout|=j;
		j<<= 1;
	}
	return (crcout);
}


void generate_crc_table() {

	// make CRC lookup table used by table algorithms

	int i, j;
	unsigned long bit, crc;

	for (i=0; i<256; i++) {

		crc=(unsigned long)i;
		if (refin) crc=reflect(crc, 8);
		crc<<= order-8;

		for (j=0; j<8; j++) {

			bit = crc & crchighbit;
			crc<<= 1;
			if (bit) crc^= polynom;
		}

		if (refin) crc = reflect(crc, order);
		crc&= crcmask;
		crctab[i]= crc;
	}
}


unsigned long crctablefast (unsigned char* p, unsigned long len) {

	// fast lookup table algorithm without augmented zero bytes, e.g. used in pkzip.
	// only usable with polynom orders of 8, 16, 24 or 32.

	unsigned long crc = crcinit_direct;

	if (refin) crc = reflect(crc, order);

	if (!refin) while (len--) crc = (crc << 8) ^ crctab[ ((crc >> (order-8)) & 0xff) ^ *p++];
	else while (len--) crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ *p++];

	if (refout^refin) crc = reflect(crc, order);
	crc^= crcxor;
	crc&= crcmask;

	return(crc);
}
