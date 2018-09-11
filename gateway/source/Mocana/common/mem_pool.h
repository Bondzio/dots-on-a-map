/* Version: mss_v6_3 */
/*
 * mem_pool.h
 *
 * Memory Pool Factory Header
 *
 * Copyright Mocana Corp 2006-2015. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __MEMORY_POOL_HEADER__
#define __MEMORY_POOL_HEADER__


/*------------------------------------------------------------------*/

typedef struct poolLink_s
{
    struct poolLink_s*  pNextPool;

} poolLink;

typedef struct
{
    poolLink*           pHeadOfPool;
    void*               pStartOfPool;
    sbyte4              numPoolElements;
    ubyte4              poolObjectSize;
    ubyte4              memAllocForPool;

} poolHeaderDescr;


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS MEM_POOL_createPool(poolHeaderDescr **ppRetPool, void *pMemPoolBase, ubyte4 memAllocForPool, ubyte4 poolObjectSize);
MOC_EXTERN MSTATUS MEM_POOL_initPool(poolHeaderDescr *pInitPool, void *pMemPoolBase, ubyte4 memAllocForPool, ubyte4 poolObjectSize);
MOC_EXTERN MSTATUS MEM_POOL_uninitPool(poolHeaderDescr *pInitPool, void **ppRetOrigMemPoolBase);

MOC_EXTERN MSTATUS MEM_POOL_recyclePoolMemory(poolHeaderDescr *pRecyclePool, ubyte4 poolObjectSize);
MOC_EXTERN MSTATUS MEM_POOL_freePool(poolHeaderDescr **ppFreePool, void **ppRetOrigMemPoolBase);

MOC_EXTERN MSTATUS MEM_POOL_getPoolObject(poolHeaderDescr *pPool, void **ppGetPoolObject);
MOC_EXTERN MSTATUS MEM_POOL_putPoolObject(poolHeaderDescr *pPool, void **ppPutPoolObject);

MOC_EXTERN MSTATUS MEM_POOL_getIndexForObject(poolHeaderDescr *pPool, void *pPoolObject, sbyte4 *pRetIndex);

#endif /* __MEMORY_POOL_HEADER__ */
