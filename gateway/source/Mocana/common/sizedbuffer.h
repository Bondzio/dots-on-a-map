/* Version: mss_v6_3 */
/*
 * sizedbuffer.h
 *
 * Simple utility to keep track of allocated memory and its size.
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __SIZEDBUFFER_H__
#define __SIZEDBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SizedBuffer
{
    ubyte2    length;
    ubyte*    pHeader;
    ubyte*    data;

} SizedBuffer;


/*------------------------------------------------------------------*/


MOC_EXTERN MSTATUS  SB_Allocate(SizedBuffer* pSB, ubyte2 len);
MOC_EXTERN void     SB_Release (SizedBuffer* pSB);

#ifdef __cplusplus
}
#endif

#endif
