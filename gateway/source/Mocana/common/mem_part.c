/* Version: mss_v6_3 */
/*
 * mem_part.c
 *
 * Memory Partition Factory
 * Overlays a memory partition over a given memory region
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef __ENABLE_MOCANA_MEM_PART__

#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/mem_part.h"

#if (defined(__KERNEL__) && (defined(__LINUX_RTOS__) || defined(__ANDROID_RTOS__)))
#include <linux/version.h>
#include <linux/hardirq.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#endif


/*------------------------------------------------------------------*/

#define ROUND_MEM_BLOCK_SIZE(X)     ((0x0f + ((uintptr)X)) & ~0x0f)
#define FLOOR_MEM_BLOCK_SIZE(X)     (((uintptr)X) & ~0x0f)

#define MEM_BLOCK_HEADER_SIZE       ROUND_MEM_BLOCK_SIZE(sizeof(memBlockHeader))


/*------------------------------------------------------------------*/

static volatile sbyte4 mMutexCount;
#ifdef __ENABLE_MOCANA_MEM_PART_MUTEX__
static RTOS_MUTEX m_memMutex;
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_init(void)
{
    MSTATUS status;

    mMutexCount = 0;

#ifdef __ENABLE_MOCANA_MEM_PART_MUTEX__
    status = RTOS_mutexCreate(&m_memMutex, MEM_PART_MUTEX, 0);
#else
    status = OK;
#endif

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_uninit(void)
{
    MSTATUS status;

#ifdef __ENABLE_MOCANA_MEM_PART_MUTEX__
    status = RTOS_mutexFree(&m_memMutex);
#else
    status = OK;
#endif

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_createPartition(memPartDescr **ppRetMemPartition, ubyte *pMemPartBaseTemp, usize memPartSize)
{
    ubyte*  pMemPartBase;
    ubyte4  memPartHeadSize;
    MSTATUS status = OK;

    if ((NULL == ppRetMemPartition) || (NULL == pMemPartBaseTemp))
    {
        status = ERR_MEM_PART_NULL_PTR;
        goto exit;
    }

    pMemPartBase = (ubyte*)(ROUND_MEM_BLOCK_SIZE((usize)pMemPartBaseTemp));

    /* adjust memory partition size to fit new alignment */
    memPartSize = memPartSize - (usize)(((uintptr)pMemPartBase) - ((uintptr)pMemPartBaseTemp));

    /* only deal w/ 16 byte chunks, to keep memory aligned on a safe boundary */
    memPartSize = FLOOR_MEM_BLOCK_SIZE(memPartSize);

    if (MOC_MIN_PARTITION_SIZE > memPartSize)
    {
        /* too small of a partition */
        status = ERR_MEM_PART_CREATE;
        goto exit;
    }

    memPartHeadSize = ROUND_MEM_BLOCK_SIZE(sizeof(memPartDescr));

    /* clear out partition head */
    MOC_MEMSET(pMemPartBase, 0x00, memPartHeadSize);

    *ppRetMemPartition = (memPartDescr *)pMemPartBase;

    (*ppRetMemPartition)->isMemPartDamaged  = FALSE;
    (*ppRetMemPartition)->isMemMutexEnabled = FALSE;

    (*ppRetMemPartition)->memPartitionSize  = memPartSize;
    (*ppRetMemPartition)->pMemBaseAddress   = pMemPartBaseTemp;
    (*ppRetMemPartition)->pMemStartAddress  = memPartHeadSize + pMemPartBase;
    (*ppRetMemPartition)->pMemEndAddress    = memPartSize + pMemPartBase;

    (*ppRetMemPartition)->pPhysicalAddress  = pMemPartBaseTemp;
    (*ppRetMemPartition)->pKernelAddress    = pMemPartBaseTemp;

    /* initialize head of list */
    (*ppRetMemPartition)->memBlockHead.pNextMemBlock = (struct memBlock_s *)(pMemPartBase + memPartHeadSize);
    (*ppRetMemPartition)->memBlockHead.memBlockSize  = 0;

    /* initialize first free memory *ppFreeMemBlock */
    (*ppRetMemPartition)->memBlockHead.pNextMemBlock->pNextMemBlock = NULL;
    (*ppRetMemPartition)->memBlockHead.pNextMemBlock->memBlockSize  = memPartSize - memPartHeadSize;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_freePartition(memPartDescr **ppFreeMemPartition)
{
    MSTATUS status;

    if ((NULL == ppFreeMemPartition) || (NULL == *ppFreeMemPartition))
    {
        status = ERR_MEM_PART_NULL_PTR;
        goto exit;
    }

#ifndef __ENABLE_MOCANA_MEM_PART_MUTEX__
    if (TRUE == (*ppFreeMemPartition)->isMemMutexEnabled)
    {
        if (OK > (status = RTOS_mutexFree(&((*ppFreeMemPartition)->memMutex))))
            goto exit;
    }
#endif

    *ppFreeMemPartition = NULL;
    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_enableMutexGuard(memPartDescr *pMemPartition)
{
    MSTATUS status;

#ifndef __ENABLE_MOCANA_MEM_PART_MUTEX__
    if (OK > (status = RTOS_mutexCreate(&(pMemPartition->memMutex), MEM_PART_MUTEX, mMutexCount++)))
        return status;
#else
    status = OK;
#endif

    pMemPartition->isMemMutexEnabled = TRUE;

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_assignOtherAddresses(memPartDescr *pMemPartition,
                              ubyte *pPhysicalAddress, ubyte *pKernelAddress)
{
    pMemPartition->pPhysicalAddress = pPhysicalAddress;
    pMemPartition->pKernelAddress   = pKernelAddress;

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_mapToPhysicalAddress(memPartDescr *pMemPartition, ubyte *pPartitionAddress,
                              ubyte **ppRetPhysicalAddress)
{
    /* converts */
    MSTATUS status = OK;

    if ((NULL == pMemPartition) || (NULL == pPartitionAddress) || (NULL == ppRetPhysicalAddress))
    {
        status = ERR_MEM_PART_NULL_PTR;
        goto exit;
    }

    if (((uintptr)(pPartitionAddress) < (uintptr)(pMemPartition->pMemStartAddress)) ||
        ((uintptr)(pPartitionAddress) > (uintptr)(pMemPartition->pMemEndAddress)) )
    {
        /* not within partition's address range */
        status = ERR_MEM_PART_BAD_ADDRESS;
        goto exit;
    }

    /* calculate offset within partition, add physical address offset */
    *ppRetPhysicalAddress = (pPartitionAddress - pMemPartition->pMemBaseAddress) + pMemPartition->pPhysicalAddress;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_mapToKernelAddress(memPartDescr *pMemPartition, ubyte *pPartitionAddress,
                            ubyte **ppRetKernelAddress)
{
    MSTATUS status = OK;

    if ((NULL == pMemPartition) || (NULL == pPartitionAddress) || (NULL == ppRetKernelAddress))
    {
        status = ERR_MEM_PART_NULL_PTR;
        goto exit;
    }

    if (((uintptr)(pPartitionAddress) < (uintptr)(pMemPartition->pMemStartAddress)) ||
        ((uintptr)(pPartitionAddress) > (uintptr)(pMemPartition->pMemEndAddress)) )
    {
        /* not within partition's address range */
        status = ERR_MEM_PART_BAD_ADDRESS;
        goto exit;
    }

    /* calculate offset within partition, add kernel address offset */
    *ppRetKernelAddress = (pPartitionAddress - pMemPartition->pMemBaseAddress) + pMemPartition->pKernelAddress;

exit:
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
MEM_PART_lock(memPartDescr *pMemPartition)
{
    MSTATUS status = OK;

    if (pMemPartition->isMemMutexEnabled)
    {
        RTOS_MUTEX mutex =
#ifndef __ENABLE_MOCANA_MEM_PART_MUTEX__
                            pMemPartition->memMutex;
#else
                            m_memMutex;
#endif
#if (defined(__KERNEL__) && (defined(__LINUX_RTOS__) || defined(__ANDROID_RTOS__)))
        if (in_atomic())
        {
            if (0 != (status = down_trylock((struct semaphore *)mutex)))
                status = ERR_MEM_PART;
        }
        else
#endif
        status = RTOS_mutexWait(mutex);
    }

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
MEM_PART_unlock(memPartDescr *pMemPartition)
{
    MSTATUS status = OK;

    if (pMemPartition->isMemMutexEnabled)
    {
#ifndef __ENABLE_MOCANA_MEM_PART_MUTEX__
        status = RTOS_mutexRelease(pMemPartition->memMutex);
#else
        status = RTOS_mutexRelease(m_memMutex);
#endif
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_alloc(memPartDescr *pMemPartition, ubyte4 numBytesToAlloc, void **ppRetNewMemBlock)
{
    ubyte4      magicNumOffset = numBytesToAlloc + MEM_BLOCK_HEADER_SIZE;
    memBlock*   pPrevBlock;
    memBlock*   pCurBlock;
    MSTATUS     status;
    MSTATUS     status1;

    if ((NULL == pMemPartition) || (NULL == ppRetNewMemBlock))
    {
        status = ERR_MEM_PART_NULL_PTR;
        goto no_cleanup;
    }

    *ppRetNewMemBlock = NULL;

    if (OK > (status = MEM_PART_lock(pMemPartition)))
        goto no_cleanup;

    if (pMemPartition->isMemPartDamaged)
    {
        /* we previously detected a buffer overflow, which most likely damaged the free list */
        status = ERR_MEM_PART_FREE_LIST_DAMAGED;
        goto exit;
    }

    if (0 == numBytesToAlloc)
    {
        status = ERR_MEM_PART_BAD_LENGTH;
        goto exit;
    }

    /* increase size to account for malloc header and magic number */
    /* round up the bytes to alloc */
    numBytesToAlloc = (ubyte4)ROUND_MEM_BLOCK_SIZE(MEM_BLOCK_HEADER_SIZE + numBytesToAlloc + sizeof(ubyte4));

    /* init link list traversal pointers */
    pPrevBlock = &(pMemPartition->memBlockHead);

    /* default to error */
    status = ERR_MEM_PART_ALLOC_FAIL;

    if (NULL == (pCurBlock = pMemPartition->memBlockHead.pNextMemBlock))
        goto exit;

    while (NULL != pCurBlock)
    {
        if (pCurBlock->memBlockSize >= numBytesToAlloc)
        {
            ubyte *pRetBlock;

            if (pCurBlock->memBlockSize == numBytesToAlloc)
            {
                pPrevBlock->pNextMemBlock = pCurBlock->pNextMemBlock;
                pRetBlock = (ubyte *)pCurBlock;
            }
            else
            {
                /* pCurBlock->memBlockSize > numBytesToAlloc */
                /* link up before remaining *ppFreeMemBlock */
                pCurBlock->memBlockSize = pCurBlock->memBlockSize - numBytesToAlloc;
                pRetBlock = ((ubyte *)pCurBlock) + pCurBlock->memBlockSize;
            }

            /* add malloc header */
            ((ubyte4 *)pRetBlock)[0] = numBytesToAlloc;
            ((ubyte4 *)pRetBlock)[1] = magicNumOffset;

            /* add magic number */
            pRetBlock[magicNumOffset]     = 0xff;
            pRetBlock[magicNumOffset + 1] = 0x5a;
            pRetBlock[magicNumOffset + 2] = 0x4b;
            pRetBlock[magicNumOffset + 3] = 0xff;

            /* results */
            *ppRetNewMemBlock = (pRetBlock + MEM_BLOCK_HEADER_SIZE);

            status = OK;
            break;
        }

        /* advance link list */
        pPrevBlock = pCurBlock;
        pCurBlock  = pCurBlock->pNextMemBlock;
    }

exit:
    status1 = MEM_PART_unlock(pMemPartition);

    /* don't obfiscate the first error */
    if (OK <= status)
        status = status1;

no_cleanup:
    return status;

} /* MEM_PART_alloc */


/*------------------------------------------------------------------*/

extern MSTATUS
MEM_PART_free(memPartDescr *pMemPartition, void **ppFreeMemBlock)
{
    memBlock*       pCurBlock;
    memBlock*       pPrevBlock;
    memBlockHeader* pAdjacentBlock;
    memBlockHeader* pFreeBlock;
    ubyte*          pMagicNumber;
    ubyte4          blockSize;
    MSTATUS         status;
    MSTATUS         status1;

    if ((NULL == pMemPartition) || (NULL == ppFreeMemBlock))
    {
        status = ERR_MEM_PART_NULL_PTR;
        goto no_cleanup;
    }

    pFreeBlock = (memBlockHeader *)(((ubyte *)(*ppFreeMemBlock)) - MEM_BLOCK_HEADER_SIZE);

    if (((uintptr)(pFreeBlock) < (uintptr)(pMemPartition->pMemStartAddress)) ||
        ((uintptr)(pFreeBlock) > (uintptr)(pMemPartition->pMemEndAddress)) ||
        (FLOOR_MEM_BLOCK_SIZE((uintptr)pFreeBlock) != (uintptr)pFreeBlock) )
    {
        /* bad free! not within partition's address range */
        status = ERR_MEM_PART_BAD_ADDRESS;
        goto no_cleanup;
    }

    if (OK > (status = MEM_PART_lock(pMemPartition)))
        goto no_cleanup;

    pMagicNumber = (((ubyte *)pFreeBlock) + pFreeBlock->magicNumOffset);

    if (((MEM_BLOCK_HEADER_SIZE * 2) > (blockSize = pFreeBlock->totalMemBlockLength)) ||
        (FLOOR_MEM_BLOCK_SIZE(blockSize) != blockSize) ||
        (0xff != pMagicNumber[0]) || (0x5a != pMagicNumber[1]) ||
        (0x4b != pMagicNumber[2]) || (0xff != pMagicNumber[3]))
    {
        /* impossible for blockSize to be zero or non-multiple 16 bytes! */
        /* and verifies the magic number */
        pMemPartition->isMemPartDamaged = TRUE;
        status = ERR_MEM_PART_FREE_LIST_DAMAGED;
        goto exit;
    }

    pPrevBlock = &(pMemPartition->memBlockHead);
    pCurBlock  = pPrevBlock->pNextMemBlock;

    while ((NULL != pCurBlock) && ((uintptr)pCurBlock < (uintptr)(pFreeBlock)))
    {
        pPrevBlock = pCurBlock;
        pCurBlock  = pCurBlock->pNextMemBlock;
    }

    pAdjacentBlock = (memBlockHeader *)(((ubyte *)pPrevBlock) + pPrevBlock->memBlockSize);

    if (((uintptr)(pAdjacentBlock) > (uintptr)(pFreeBlock)) ||
        ((NULL != pCurBlock) && (((uintptr)(pFreeBlock) + blockSize) > (uintptr)(pCurBlock))) )
    {
        pMemPartition->isMemPartDamaged = TRUE;
        status = ERR_MEM_PART_FREE_LIST_DAMAGED;
        goto exit;
    }

    if ((uintptr)pAdjacentBlock == (uintptr)pFreeBlock)
    {
        /* easy instances, merge freed block w/ previous block */
        pPrevBlock->memBlockSize += blockSize;
    }
    else
    {
        /* link in free block, after previous block */
        ((memBlock *)(pFreeBlock))->memBlockSize  = blockSize;
        ((memBlock *)(pFreeBlock))->pNextMemBlock = pCurBlock;

        pPrevBlock->pNextMemBlock = (struct memBlock_s *)pFreeBlock;
        pPrevBlock = (memBlock *)pFreeBlock;
    }

    if ((((ubyte *)pPrevBlock) + pPrevBlock->memBlockSize) == (ubyte *)pCurBlock)
    {
        /* merge current block with previous block */
        pPrevBlock->memBlockSize  = pPrevBlock->memBlockSize  + pCurBlock->memBlockSize;
        pPrevBlock->pNextMemBlock = pCurBlock->pNextMemBlock;
    }

    /* clear pointer */
    *ppFreeMemBlock = NULL;

exit:
    status1 = MEM_PART_unlock(pMemPartition);

    /* don't obfiscate the first error */
    if (OK <= status)
        status = status1;

no_cleanup:
    return status;

} /* MEM_PART_freemem */

#endif /* __ENABLE_MOCANA_MEM_PART__ */

