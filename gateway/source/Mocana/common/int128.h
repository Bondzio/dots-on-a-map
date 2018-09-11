/* Version: mss_v6_3 */
/*
 * int128.h
 *
 * Support for 128 bit integer
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*------------------------------------------------------------------*/

#ifndef __INT128_HEADER__
#define __INT128_HEADER__

ubyte16 u16_Shl( ubyte16 a, ubyte4 n);
void u16_Incr32( ubyte16* pa, ubyte4 b);

#if __MOCANA_MAX_INT__ == 64
#define ZERO_U16(a) (a).upper64 = (a).lower64 = 0
#define U16INIT( U16, U81, U82) (U16).upper64 = U81; (U16).lower64 = U82
#define W1_U16(a) ((ubyte4) ((((a).upper64) >> 32 ) & 0xFFFFFFFF))
#define W2_U16(a) ((ubyte4) (((a).upper64  & 0xFFFFFFFF)))
#define W3_U16(a) ((ubyte4) ((((a).lower64) >> 32 ) & 0xFFFFFFFF))
#define W4_U16(a) ((ubyte4) (((a).lower64  & 0xFFFFFFFF)))
#else
#define ZERO_U16(a) (a).w1 = (a).w2 = (a).w3 = (a).w4 = 0
#define U16INIT( U16, U41, U42, U43, U44)   (U16).w1 = (U41); (U16).w2 = (U42); \
                                            (U16).w3 = (U43); (U16).w4 = (U44)
#define W1_U16(a) (a).w1
#define W2_U16(a) (a).w2
#define W3_U16(a) (a).w3
#define W4_U16(a) (a).w4

#endif

#endif
