/* Version: mss_v6_3 */
/*
 * md45.h
 *
 * Routines and constants common to md4 and md5
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


#ifndef __MD45_H__
#define __MD45_H__

#ifdef __cplusplus
extern "C" {
#endif

MOC_EXTERN void MD45_encode(ubyte *, const ubyte4 *, ubyte4);
MOC_EXTERN void MD45_decode(ubyte4 *, const ubyte *, ubyte4);

MOC_EXTERN const ubyte MD45_PADDING[64];

#ifdef __cplusplus
}
#endif

#endif
