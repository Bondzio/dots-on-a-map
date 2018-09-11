/* Version: mss_v6_3 */
/*
 * dynarray.h
 *
 * Mocana Dynarray
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __DYNARRAY_H__
#define __DYNARRAY_H__

typedef struct DynArray
{
    sbyte4  numUsed;
    sbyte4  numAllocated;
    sbyte4  elementSize;
    void*   array;
} DynArray;

MSTATUS DYNARR_Init( sbyte4 elementSize, DynArray* pArr);
MSTATUS DYNARR_InitEx( sbyte4 elementSize, ubyte4 initialSize, DynArray* pArr);
MSTATUS DYNARR_Uninit( DynArray* pArr);
MSTATUS DYNARR_GetElementSize( const DynArray* pArr, sbyte4* pElementSize);
MSTATUS DYNARR_GetElementCount( const DynArray* pArr, sbyte4* pElementCount);
MSTATUS DYNARR_Append( DynArray* pArr, const void* pElement);
MSTATUS DYNARR_AppendEx( DynArray* pArr, const void* pElement, ubyte4 incrementSize);
MSTATUS DYNARR_AppendMultiple( DynArray* pArr, const void* pElements, ubyte4 numElems, ubyte4 incrementSize);
MSTATUS DYNARR_Get( const DynArray* pArr, sbyte4 index, void* ppElement);
MSTATUS DYNARR_GetArray( const DynArray* pArr, const void** pArray);
MSTATUS DYNARR_DetachArray( DynArray* pArr, void** pArray);

#endif /* __DYNARRAY_H__ */
