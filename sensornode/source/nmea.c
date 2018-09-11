#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "m1_bsp.h"
#include "nmea.h"

#define NMEA_GGA "GPGGA"
#define NMEA_GGA_UTC_TIME_POSITION 1
#define NMEA_GGA_LATITUDE_POSITION 2
#define NMEA_GGA_LATITUDE_DIRECTION_POSITION 3
#define NMEA_GGA_LONGITUDE_POSITION 4
#define NMEA_GGA_LONGITUDE_DIRECTION_POSITION 5
#define NMEA_GGA_SATELLITES_USED_POSITION 7
#define NMEA_GGA_HDOP_POSITION 8
#define NMEA_GGA_ALTITUDE_POSITION 9
#define NMEA_GGA_ALTITUDE_UNITS_POSITION 10
#define NMEA_GGA_CHECKSUM_POSITION 14
#define NMEA_GGA_CHECKSUM_POSITION_SKIP (NMEA_GGA_CHECKSUM_POSITION - NMEA_GGA_ALTITUDE_UNITS_POSITION - 1)

enum NMEA_state {WAIT_FOR_SOS,
                 WAIT_FOR_MESSAGE_ID,
                 WAIT_FOR_UTC_TIME,
                 WAIT_FOR_LATITUDE,
                 WAIT_FOR_LATITUDE_DIRECTION,
                 WAIT_FOR_LONGITUDE,
                 WAIT_FOR_LONGITUDE_DIRECTION,
                 WAIT_FOR_POSITION_FIX,
                 WAIT_FOR_SATELLITES_USED,
                 WAIT_FOR_HDOP,
                 WAIT_FOR_ALTITUDE,
                 WAIT_FOR_ALTITUDE_UNITS,
                 WAIT_FOR_CHECKSUM,
                 GET_CHECKSUM,
                 RECEIVED_CHECKSUM
};


static int parse_NMEA_sentence(char c, gps_t * p_gps) {
  static int state = WAIT_FOR_SOS;
  static unsigned char checksum = 0, expected_checksum;
  static char token_buf[20];
  static int token_index = 0;
  static char latitude[20], longitude[20], altitude[20], altitude_units, hdop[20];
  int satellites, gga_complete;
  static float utc_time;
  static int skip = 0;
  OS_ERR err_os;
 
  if (((state > WAIT_FOR_SOS) && (state < WAIT_FOR_CHECKSUM)) || ((state == WAIT_FOR_CHECKSUM) && (skip != NMEA_GGA_CHECKSUM_POSITION_SKIP)))
    checksum ^= (unsigned char)c;
  if ((c == ',') || (c == '*') || (c == '\r')) {
    token_buf[token_index++] = '\0';
    switch (state) {
    case WAIT_FOR_MESSAGE_ID:
      if (!strcmp(token_buf, NMEA_GGA)) {
        state = WAIT_FOR_UTC_TIME;
      }
      break;
    case WAIT_FOR_UTC_TIME:
      utc_time = atof(token_buf);
      state = WAIT_FOR_LATITUDE;
      break;
    case WAIT_FOR_LATITUDE:
      latitude[0] = ' ';
      strcpy(&latitude[1], token_buf);
      if (!strlen(token_buf)) {
          state = WAIT_FOR_SOS;
          break;
      }
      state = WAIT_FOR_LATITUDE_DIRECTION;
      break;
    case WAIT_FOR_LATITUDE_DIRECTION:
      if (strcmp(token_buf, "N"))
          latitude[0] = '-';
      state = WAIT_FOR_LONGITUDE;
      break;
    case WAIT_FOR_LONGITUDE:
      longitude[0] = ' ';
      strcpy(&longitude[1], token_buf);
      if (!strlen(token_buf)) {
          state = WAIT_FOR_SOS;
          break;
      }
      state = WAIT_FOR_LONGITUDE_DIRECTION;
      break;
    case WAIT_FOR_LONGITUDE_DIRECTION:
      if (strcmp(token_buf, "E"))
          longitude[0] = '-';
      state = WAIT_FOR_POSITION_FIX;
      skip = 0;
      break;
    case WAIT_FOR_POSITION_FIX:
      state = WAIT_FOR_SATELLITES_USED;
      break;
    case WAIT_FOR_SATELLITES_USED:
      sscanf(token_buf, "%d", &satellites);
      state = WAIT_FOR_HDOP;
      break;
    case WAIT_FOR_HDOP:
      strcpy(hdop, token_buf);
      state = WAIT_FOR_ALTITUDE;
      break;
    case WAIT_FOR_ALTITUDE:
      strcpy(altitude, token_buf);
      state = WAIT_FOR_ALTITUDE_UNITS;
      break;
    case WAIT_FOR_ALTITUDE_UNITS:
      altitude_units = token_buf[0];
      state = WAIT_FOR_CHECKSUM;
      break;
    case WAIT_FOR_CHECKSUM:
      if (skip++ == NMEA_GGA_CHECKSUM_POSITION_SKIP) {
        state = GET_CHECKSUM;
      }
      break;
    case GET_CHECKSUM:
      expected_checksum = strtol(token_buf, NULL, 16);
      state = RECEIVED_CHECKSUM;
      break;
    default:
    }
    token_index = 0;
  } else if (c == '\n') {
    gga_complete = (state == RECEIVED_CHECKSUM);
    state = WAIT_FOR_SOS;
    token_index = 0;
    if (gga_complete && (expected_checksum == checksum)) {
      int temp;
      // +/- ddmm.mmmmmm
      sscanf(&latitude[3], "%f", &p_gps->latitude);
      latitude[3] = '\0';
      sscanf(latitude, "%d", &temp);
      if (temp >= 0)
        p_gps->latitude = p_gps->latitude / 60.0 + temp;
      else
        p_gps->latitude =  -p_gps->latitude / 60.0 + temp;
      // +/- dddmm.mmmmmm
      sscanf(&longitude[4], "%f", &p_gps->longitude);
      longitude[4] = '\0';
      sscanf(longitude, "%d", &temp);
      if (temp >= 0)
        p_gps->longitude = p_gps->longitude / 60.0 + temp;
      else
        p_gps->longitude =  -p_gps->longitude / 60.0 + temp;
      p_gps->utc_time = utc_time;
      p_gps->satellites = satellites;
      sprintf(p_gps->altitude, "%s%c", altitude, altitude_units);
      strcpy(p_gps->hdop, hdop);
      return 0;
    } else
      return -2;
  } else if ((c == '$') && (state == WAIT_FOR_SOS)) {
    state = WAIT_FOR_MESSAGE_ID;
    checksum = 0;
  } else {
    token_buf[token_index++] = c;
    if (token_index >= sizeof(token_buf))
      token_index = 0;
  }
  return -1;
}


int parse_NMEA(char c, gps_t * p_gps) {
  int ret = -1;
  static char NMEA_sentence[400];
  static int NMEA_index = 0;
  size_t sentence_length;
  
  if (c == '$') {
    NMEA_index = 0;
    NMEA_sentence[NMEA_index++] = c;
  } else if (c == '\n') {
    NMEA_sentence[NMEA_index++] = c;    
    NMEA_sentence[NMEA_index++] = '\0';
    sentence_length = strlen(NMEA_sentence);
    for (int i = 0; i < sentence_length; i++) {
      if (!parse_NMEA_sentence(NMEA_sentence[i], p_gps))
        ret = 0;
    }
  } else {
    NMEA_sentence[NMEA_index++] = c;
    NMEA_index %= 400;
  }
  return ret;
}
