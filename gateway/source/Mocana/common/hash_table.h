/* Version: mss_v6_3 */
/*
 * hash_table.h
 *
 * Hash Table Factory Header
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __HASH_TABLE_HEADER__
#define __HASH_TABLE_HEADER__

#define SET_NEXT_ELEM( p, n)    { if (p!=n) p->pNextElement = n; }

/*------------------------------------------------------------------*/

typedef struct hashTablePtrElement
{
    void*                           pAppData;       /* reference by pointer */
    ubyte4                          hashValue;

    struct hashTablePtrElement*     pNextElement;

} hashTablePtrElement;



typedef MSTATUS ((*funcPtrAllocHashPtrElement)(void *, hashTablePtrElement **));
typedef MSTATUS ((*funcPtrFreeHashPtrElement)(void *, hashTablePtrElement *));

typedef struct hashTableOfPtrs
{
    ubyte4                          hashTableSizeMask;
    void*                           pHashCookie;

    /* allows for pre-allocated elements */
    funcPtrAllocHashPtrElement      pFuncAllocElement;
    funcPtrFreeHashPtrElement       pFuncFreeElement;

    hashTablePtrElement*            pHashTableArray[1];

} hashTableOfPtrs;


/*------------------------------------------------------------------*/

typedef struct hashTableIndexElement
{
    ubyte4                          appDataIndex;   /* note: on 64-bit processors, 'ubyte4' is smaller than a 'void*' */
    ubyte4                          hashValue;

    struct hashTableIndexElement*   pNextElement;

} hashTableIndexElement;

typedef MSTATUS ((*funcPtrAllocElement)(void *, hashTableIndexElement **));
typedef MSTATUS ((*funcPtrFreeElement)(void *, hashTableIndexElement **));
typedef MSTATUS ((*funcPtrExtraMatchTest)(void *, void *, intBoolean *));

typedef struct hashTableIndices
{
    ubyte4                  hashTableSizeMask;
    void*                   pHashCookie;

    /* allows for pre-allocated elements */
    funcPtrAllocElement     pFuncAllocElement;
    funcPtrFreeElement      pFuncFreeElement;

    hashTableIndexElement*  pHashTableArray[1]; /* note: array size should be non-zero to avoid compiler error */

} hashTableIndices;


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS HASH_TABLE_createIndiceTable(hashTableIndices **ppRetHashTable, ubyte4 hashTableSizeMask, void *pHashCookie, funcPtrAllocElement pFuncAllocElement, funcPtrFreeElement pFuncFreeElement);
MOC_EXTERN MSTATUS HASH_TABLE_clearIndiceTable(hashTableIndices *pClearHashTable, void *pClearCtx, MSTATUS(*funcPtrClearIndex)(void * /* pClearCtx */, ubyte4 /* appDataIndex */));
MOC_EXTERN MSTATUS HASH_TABLE_removeIndiceTable(hashTableIndices *pFreeHashTable, void **ppRetHashCookie);

MOC_EXTERN MSTATUS HASH_TABLE_addIndex(hashTableIndices *pHashTable, ubyte4 hashValue, ubyte4 appDataIndex);
MOC_EXTERN MSTATUS HASH_TABLE_deleteIndex(hashTableIndices *pHashTable, ubyte4 hashValue, ubyte4 testDataIndex, intBoolean *pRetFoundHashValue);
MOC_EXTERN MSTATUS HASH_TABLE_findIndex(hashTableIndices *pHashTable, ubyte4 hashValue, ubyte4 testDataIndex, intBoolean *pRetFoundHashValue);

MOC_EXTERN MSTATUS HASH_TABLE_createPtrsTable(hashTableOfPtrs **ppRetHashTable, ubyte4 hashTableSizeMask, void *pHashCookie, funcPtrAllocHashPtrElement, funcPtrFreeHashPtrElement);
MOC_EXTERN MSTATUS HASH_TABLE_removePtrsTable(hashTableOfPtrs *pFreeHashTable, void **ppRetHashCookie);

MOC_EXTERN MSTATUS HASH_TABLE_addPtr(hashTableOfPtrs *pHashTable, ubyte4 hashValue, void *pAppData);
MOC_EXTERN MSTATUS HASH_TABLE_deletePtr(hashTableOfPtrs *pHashTable, ubyte4 hashValue, void *pTestData, funcPtrExtraMatchTest, void **ppRetAppDataToDelete, intBoolean *pRetFoundHashValue);
MOC_EXTERN MSTATUS HASH_TABLE_findPtr(hashTableOfPtrs *pHashTable, ubyte4 hashValue, void *pTestData, funcPtrExtraMatchTest, void **ppRetAppData, intBoolean *pRetFoundHashValue);
MOC_EXTERN MSTATUS HASH_TABLE_traversePtrTable(hashTableOfPtrs *pFreeHashTable, MSTATUS(*funcPtrTraverseTable)(void * /* pAppData */));
void * HASH_TABLE_iteratePtrTable(hashTableOfPtrs *pHashTable, void ** pBucketCookie, ubyte4 *index);
#endif /* __HASH_TABLE_HEADER__ */

