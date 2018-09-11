/* Version: mss_v6_3 */
/*
 * mem_part.h
 *
 * Memory Partition Factory Header
 * Overlays a memory partition over a given memory region
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __MEMORY_PARTITION_HEADER__
#define __MEMORY_PARTITION_HEADER__


/*------------------------------------------------------------------*/

#ifndef MOC_MIN_PARTITION_SIZE
#define MOC_MIN_PARTITION_SIZE      (1024)
#endif


/*------------------------------------------------------------------*/

typedef struct memBlock_s
{
    struct memBlock_s*  pNextMemBlock;
    ubyte4              memBlockSize;

} memBlock;

typedef struct
{
    ubyte4              totalMemBlockLength;
    ubyte4              magicNumOffset;

} memBlockHeader;

typedef struct memPartDescr
{
    intBoolean          isMemPartDamaged;

    intBoolean          isMemMutexEnabled;
    RTOS_MUTEX          memMutex;

    ubyte4              memPartitionSize;
    ubyte*              pMemBaseAddress;
    ubyte*              pMemStartAddress;
    ubyte*              pMemEndAddress;

    ubyte*              pPhysicalAddress;    /* optional, allows mapping user-virtual address space to physical address space */
    ubyte*              pKernelAddress;      /* optional, allows mapping user-virtual address space to kernel-virtual address space */

    memBlock            memBlockHead;

} memPartDescr;


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS MEM_PART_init(void);
MOC_EXTERN MSTATUS MEM_PART_uninit(void);

MOC_EXTERN MSTATUS MEM_PART_createPartition(memPartDescr **ppRetMemPartition, ubyte *pMemPartBase, usize memPartSize);
MOC_EXTERN MSTATUS MEM_PART_freePartition(memPartDescr **ppFreeMemPartition);

MOC_EXTERN MSTATUS MEM_PART_enableMutexGuard(memPartDescr *pMemPartition);
MOC_EXTERN MSTATUS MEM_PART_assignOtherAddresses(memPartDescr *pMemPartition, ubyte *pPhysicalAddress, ubyte *pKernelAddress);
MOC_EXTERN MSTATUS MEM_PART_mapToPhysicalAddress(memPartDescr *pMemPartition, ubyte *pPartitionAddress, ubyte **ppRetPhysicalAddress);
MOC_EXTERN MSTATUS MEM_PART_mapToKernelAddress(memPartDescr *pMemPartition, ubyte *pPartitionAddress, ubyte **ppRetKernelAddress);

MOC_EXTERN MSTATUS MEM_PART_alloc(memPartDescr *pMemPartition, ubyte4 numBytesToAlloc, void **ppRetNewMemBlock);
MOC_EXTERN MSTATUS MEM_PART_free(memPartDescr *pMemPartition, void **ppFreeMemBlock);

#endif /* __MEMORY_PARTITION_HEADER__ */
