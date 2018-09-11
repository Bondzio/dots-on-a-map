/* Version: mss_v6_3 */
/*
 * arc2.h
 *
 * "alleged rc2" algorithm
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __ARC2_H__
#define __ARC2_H__

#ifdef __cplusplus
extern "C" {
#endif


#define RC2_BLOCK_SIZE      (8)


/*------------------------------------------------------------------*/
#ifdef __ENABLE_ARC2_CIPHERS__
MOC_EXTERN void rc2_keyschedule(ubyte2 xkey[64], const ubyte *key, ubyte4 len, ubyte4 bits);
MOC_EXTERN void rc2_encrypt(const ubyte2 xkey[64], const ubyte *plain, ubyte *cipher);
MOC_EXTERN void rc2_decrypt(const ubyte2 xkey[64], ubyte *plain, const ubyte *cipher);
#endif

#ifdef __cplusplus
}
#endif

#endif
