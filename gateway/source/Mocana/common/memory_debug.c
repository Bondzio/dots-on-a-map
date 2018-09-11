/* Version: mss_v6_3 */
/*
 * memory_debug.c
 *
 * Mocana Memory Leak Detection Code
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef __ENABLE_MOCANA_DEBUG_MEMORY__

#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/debug_console.h"

#include <stdlib.h>
#include <stdio.h>


/*------------------------------------------------------------------*/

#define NUM_MEM_TABLE_ROWS  800
#define NUM_FREE_TABLE_ROWS 100

#ifndef MEM_DBG_PAD
#define MEM_DBG_PAD         64      /* should be a multiple of 16 */
#endif

#ifndef MEM_DBG_PAD_CHAR
#define MEM_DBG_PAD_CHAR    0xab
#endif

#if (MEM_DBG_PAD % 16)
#error MEM_DBG_PAD must be n * 16 in size, where n is a nonnegative integer, n=0,1,2,3...
#endif


typedef struct
{
    ubyte   rowUsed;
    ubyte*  pMemBlock;
    ubyte4  numBytes;
    ubyte*  pFile;
    ubyte4  lineNum;

} memTableDescr;

static memTableDescr memTable[NUM_MEM_TABLE_ROWS];
static memTableDescr freeTable[NUM_FREE_TABLE_ROWS];

static volatile ubyte4 highWaterMark = 0;
static volatile ubyte4 currentMemoryUsage = 0;


/*------------------------------------------------------------------*/

extern void
dbg_dump(void)
{
    sbyte4 index;

    printf("\n\nMEMORY LEAKS FOUND =============================\n");

    for (index = 0; index < NUM_MEM_TABLE_ROWS; index++)
    {
        if (1 == memTable[index].rowUsed)
        {
            printf("dbg_dump: memory = %08x, size = %d, line = %d, file = %s\n", (int)(memTable[index].pMemBlock), (int)(memTable[index].numBytes), (int)(memTable[index].lineNum), (char *)memTable[index].pFile);
        }
    }

    printf("\n\nMEMORY PAD DAMAGE FOUND =============================\n");

    for (index = 0; index < NUM_MEM_TABLE_ROWS; index++)
    {
        if (2 == memTable[index].rowUsed)
        {
            printf("dbg_dump: memory = %08x, size = %d, line = %d, file = %s\n", (int)(memTable[index].pMemBlock), (int)(memTable[index].numBytes), (int)(memTable[index].lineNum), (char *)memTable[index].pFile);
        }
    }

    printf("BAD FREES FOUND ================================\n");

    for (index = 0; index < NUM_FREE_TABLE_ROWS; index++)
    {
        if (1 == freeTable[index].rowUsed)
        {
            printf("dbg_dump: memory = %08x, line = %d, file = %s\n", (int)(freeTable[index].pMemBlock), (int)(freeTable[index].lineNum), (char *)freeTable[index].pFile);
        }
    }

    printf("================================================\n");
    printf("Memory high water mark: %d\n\n", highWaterMark);

    return;
}


/*------------------------------------------------------------------*/

extern ubyte4
MEMORY_DEBUG_resetHighWaterMark(void)
{
    ubyte4 oldHighWaterMark = highWaterMark;

    highWaterMark = currentMemoryUsage;

    /* return non-reset high water mark */
    return oldHighWaterMark;
}


/*------------------------------------------------------------------*/

extern void *
dbg_malloc(ubyte4 numBytes, ubyte *pFile, ubyte4 lineNum)
{
    ubyte*  retBlock;
    sbyte4  findRow;

    if (34000 < numBytes)
    {
        printf("dbg_malloc: malloc awefully big (%d) bytes @ %s, line = %d.\n", (int)numBytes, (char *)pFile, (int)lineNum);
    }

    if (0 == numBytes)
    {
        printf("dbg_malloc: zero byte malloc @ %s, line = %d.\n", (char *)pFile, (int)lineNum);
        return NULL;
    }

    retBlock = malloc(MEM_DBG_PAD + numBytes + MEM_DBG_PAD);

    if (NULL != retBlock)
    {
        currentMemoryUsage += numBytes;

        if (MEM_DBG_PAD)
        {
            /* we insert pad before and after block */
            MOC_MEMSET(retBlock, MEM_DBG_PAD_CHAR, MEM_DBG_PAD);
            retBlock += MEM_DBG_PAD;
            MOC_MEMSET(retBlock + numBytes, MEM_DBG_PAD_CHAR, MEM_DBG_PAD);
        }

        for (findRow = 0; findRow < NUM_MEM_TABLE_ROWS; findRow++)
        {
            if (0 == memTable[findRow].rowUsed)
            {
                memTable[findRow].rowUsed   = 1;
                memTable[findRow].numBytes  = numBytes;
                memTable[findRow].pMemBlock = retBlock;
                memTable[findRow].pFile     = pFile;
                memTable[findRow].lineNum   = lineNum;
                break;
            }
        }

        if (NUM_MEM_TABLE_ROWS == findRow)
        {
            printf("dbg_malloc: not able to record address.\n");
        }
    }
    else
    {
        printf("dbg_malloc: malloc(size = %d) failed.\n", numBytes);
    }

    if (highWaterMark < currentMemoryUsage)
        highWaterMark = currentMemoryUsage;

#ifdef __ENABLE_MOCANA_TRACE_ALLOC__
    printf("Allocating %d bytes %s:%d\nTotal = %d\n", numBytes, pFile,
           lineNum, currentMemoryUsage);
#endif

    return retBlock;
}


/*------------------------------------------------------------------*/

static intBoolean
checkPad(ubyte *pBlockToTest, ubyte4 numBytes)
{
    ubyte*      pTest;
    ubyte4      count;
    intBoolean  isDamaged = FALSE;

    pTest = pBlockToTest - MEM_DBG_PAD;

    for (count = 0; count < MEM_DBG_PAD; count++)
    {
        if (MEM_DBG_PAD_CHAR != pTest[count])
        {
            printf("checkPad: pad before alloc block is damaged.\n%04x: ", count);
            for (; count < MEM_DBG_PAD; count++)
                printf("%02x ", pTest[count]);

            isDamaged = TRUE;
            break;
        }
    }

    pTest = pBlockToTest + numBytes;

    for (count = 0; count < MEM_DBG_PAD; count++)
    {
        if (MEM_DBG_PAD_CHAR != pTest[count])
        {
            printf("checkPad: pad after alloc block is damaged.\n%04x: ", count);
            for (; count < MEM_DBG_PAD; count++)
                printf("%02x ", pTest[count]);

            isDamaged = TRUE;
            break;
        }
    }

    return isDamaged;
}


/*------------------------------------------------------------------*/

extern void
dbg_free(void *pBlockToFree1, ubyte *pFile, ubyte4 lineNum)
{
    ubyte*  pBlockToFree = pBlockToFree1;
    sbyte4  findRow = NUM_MEM_TABLE_ROWS;
    ubyte4  numBytes;
    sbyte*  pOrigFilename = NULL;
    sbyte4  origLineNum;

    if (NULL != pBlockToFree)
    {
        for (findRow = 0; findRow < NUM_MEM_TABLE_ROWS; findRow++)
        {
            if ((1 == memTable[findRow].rowUsed) && (pBlockToFree == memTable[findRow].pMemBlock))
            {
                pOrigFilename = (sbyte *)(memTable[findRow].pFile);
                origLineNum   = memTable[findRow].lineNum;
                numBytes      = memTable[findRow].numBytes;

                currentMemoryUsage -= numBytes;
                break;
            }
        }
    }
    else
    {
        printf("dbg_free: bad free!  NULL point!\n");
    }

    /* log bad free */
    if (NUM_MEM_TABLE_ROWS == findRow)
    {
        for (findRow = 0; findRow < NUM_FREE_TABLE_ROWS; findRow++)
        {
            if (0 == freeTable[findRow].rowUsed)
            {
                freeTable[findRow].rowUsed   = 1;
                freeTable[findRow].pMemBlock = pBlockToFree;
                freeTable[findRow].pFile     = pFile;
                freeTable[findRow].lineNum   = lineNum;
                pBlockToFree = NULL;
                break;
            }
        }
    } else
    {
      numBytes = memTable[findRow].numBytes;

#ifdef __ENABLE_MOCANA_TRACE_ALLOC__
      printf("Releasing %d bytes %s:%d\nTotal = %d\n", numBytes, pFile, lineNum,
          currentMemoryUsage);
#endif
    }

    /* free it */
    if (NULL != pBlockToFree)
    {
        if (FALSE == checkPad(pBlockToFree, memTable[findRow].numBytes))
        {
            memTable[findRow].rowUsed = 0;
            free(pBlockToFree - MEM_DBG_PAD);
        }
        else
        {
            /* mark block damaged, don't bother to free */
            memTable[findRow].rowUsed = 2;
        }
    }

    return;
}


/*------------------------------------------------------------------*/

extern void
dbg_relabel_memory(void *pBlockToRelabel, ubyte *pFile, ubyte4 lineNum)
{
    sbyte4  findRow;

    if (NULL != pBlockToRelabel)
    {
        for (findRow = 0; findRow < NUM_MEM_TABLE_ROWS; findRow++)
        {
            if ((1 == memTable[findRow].rowUsed) && (pBlockToRelabel == memTable[findRow].pMemBlock))
            {
                memTable[findRow].pFile = pFile;
                memTable[findRow].lineNum = lineNum;
                break;
            }
        }
    }

    return;
}


/*------------------------------------------------------------------*/

extern void
dbg_check_memory(void *pBlockToCheck, ubyte *pFile, ubyte4 lineNum)
{
    sbyte4  findRow;

    if (NULL != pBlockToCheck)
    {
        for (findRow = 0; findRow < NUM_MEM_TABLE_ROWS; findRow++)
        {
            if ((1 == memTable[findRow].rowUsed) && (pBlockToCheck == memTable[findRow].pMemBlock))
            {
                if (TRUE == checkPad(pBlockToCheck, memTable[findRow].numBytes))
                {
                    /* block damaged */
                    memTable[findRow].rowUsed = 2;

                    /* relabel the memory to the point where the damage was first detected */
                    memTable[findRow].pFile = pFile;
                    memTable[findRow].lineNum = lineNum;
                }

                break;
            }
        }
    }
}


/*------------------------------------------------------------------*/

extern void
dbg_lookup_memory(void *pBlockToLookup, ubyte **ppRetFile, ubyte4 *pRetLineNum)
{
    sbyte4  findRow;

    if (NULL != pBlockToLookup)
    {
        for (findRow = 0; findRow < NUM_MEM_TABLE_ROWS; findRow++)
        {
            if ((1 == memTable[findRow].rowUsed) && (pBlockToLookup == memTable[findRow].pMemBlock))
            {
                *ppRetFile = memTable[findRow].pFile;
                *pRetLineNum = memTable[findRow].lineNum;
                break;
            }
        }
    }

    return;
}

#endif /* __ENABLE_MOCANA_DEBUG_MEMORY__ */
