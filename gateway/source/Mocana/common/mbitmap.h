/* Version: mss_v6_3 */
/*
 * mbitmap.h
 *
 * Mocana Bit Map Factory Header
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __MBITMAP_HEADER__
#define __MBITMAP_HEADER__

typedef struct
{
    ubyte4* pBitmap;
    ubyte4  bitmapSize;
    ubyte4  bitmapLoIndex;
    ubyte4  bitmapHiIndex;

} bitmapDescr;


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS MBITMAP_findVacantIndex(bitmapDescr *pBitMapDescr, ubyte4 *pRetIndex);
MOC_EXTERN MSTATUS MBITMAP_testAndSetIndex(bitmapDescr *pBitMapDescr, ubyte4 theIndex);
MOC_EXTERN MSTATUS MBITMAP_clearIndex(bitmapDescr *pBitMapDescr, ubyte4 theIndex);
MOC_EXTERN MSTATUS MBITMAP_isIndexSet(bitmapDescr *pBitMapDescr, ubyte4 theIndex, intBoolean *pIsIndexSet);
MOC_EXTERN MSTATUS MBITMAP_createMap(bitmapDescr **ppRetBitMapDescr, ubyte4 loIndex, ubyte4 hiIndex);
MOC_EXTERN MSTATUS MBITMAP_releaseMap(bitmapDescr **ppFreeBitMapDescr);

#endif /* __MBITMAP_HEADER__ */
