/* Version: mss_v6_3 */
/*
 * arc4.h
 *
 * "alleged rc4" algorithm
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __ARC4_H__
#define __ARC4_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rc4_key
{
   ubyte state[256];
   ubyte x;
   ubyte y;
} rc4_key;

MOC_EXTERN void prepare_key(ubyte *key_data_ptr, sbyte4 key_data_len, rc4_key *key);
MOC_EXTERN void rc4(ubyte *buffer_ptr, sbyte4 buffer_len, rc4_key *key);

#ifdef __cplusplus
}
#endif

#endif
