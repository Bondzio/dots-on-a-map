/* Version: mss_v6_3 */
/*
 * utils.c
 *
 * Utility code for storing and retrieving keys
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifndef __DISABLE_MOCANA_FILE_SYSTEM_HELPER__

#ifdef __MQX_RTOS__
#include <mqx.h>
#include <fio.h>
#endif

#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/random.h"
#include "../common/utils.h"

#if !( defined(__MQX_RTOS__) || defined(__UCOS_RTOS__) )
#include <stdio.h>
#endif

#ifdef __UCOS__
#include <fs.h>
#include <fs_api.h>
#define FILE FS_FILE
#define fopen fs_fopen
#define fseek fs_fseek
#define ftell fs_ftell
#define rewind fs_rewind
#define fread fs_fread
#define fclose fs_fclose
#define fwrite fs_fwrite
#define SEEK_END FS_SEEK_END
#endif /* __UCOS_RTOS__ */


/*------------------------------------------------------------------*/

extern MSTATUS
UTILS_readFileRaw(const ubyte* pFileObj, ubyte **ppRetBuffer, ubyte4 *pRetBufLength)
{
#ifndef __RTOS_WTOS__
    FILE*   f = (FILE*) pFileObj;
#else
    int     f = (int)pFileObj;
#endif
    sbyte4  fileSize;
    ubyte*  pFileBuffer = NULL;
    MSTATUS status = OK;

    /* check input */
    if ((NULL == ppRetBuffer) || (NULL == pRetBufLength))
    {
        status = ERR_NULL_POINTER;
        goto nocleanup;
    }

    *ppRetBuffer   = NULL;
    *pRetBufLength = 0;

#ifndef __RTOS_WTOS__
    if (!f)
#else
    if (f < 0)
#endif
    {
        status = ERR_FILE_OPEN_FAILED;
        goto nocleanup;
    }

    /* determine size */
    fseek(f, 0, MSEEK_END);
    fileSize = ftell(f);

    if (0 == fileSize)
    {
        status = ERR_FILE_OPEN_FAILED;
        goto exit;
    }

    if (NULL == (pFileBuffer = (ubyte *) MALLOC(fileSize + 1)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
#if !defined(__RTOS_WINCE__) && !defined(__RTOS_MQX__)
    rewind(f);
#else
    fseek(f, 0L, MSEEK_SET);
#endif

    if (((ubyte4)fileSize) > fread(pFileBuffer, 1, fileSize, f))
    {
        status = ERR_FILE_READ_FAILED;
        goto exit;
    }

    *ppRetBuffer   = pFileBuffer;  pFileBuffer = NULL;
    *pRetBufLength = fileSize;

exit:
    if (NULL != pFileBuffer)
        FREE(pFileBuffer);

nocleanup:
    return status;

} /* UTILS_readFileRaw */

/*------------------------------------------------------------------*/

extern MSTATUS
UTILS_readFile(const char* pFilename,
               ubyte **ppRetBuffer, ubyte4 *pRetBufLength)
{
#ifndef __RTOS_WTOS__
    FILE*   f;
#else
    int     f;
#endif
    MSTATUS status = OK;

    /* check input */
    if ((NULL == ppRetBuffer) || (NULL == pRetBufLength))
    {
        status = ERR_NULL_POINTER;
        goto nocleanup;
    }

    *ppRetBuffer   = NULL;
    *pRetBufLength = 0;

    f = fopen(pFilename, "rb");

#ifndef __RTOS_WTOS__
    if (!f)
#else
    if (f < 0)
#endif
    {
        status = ERR_FILE_OPEN_FAILED;
        goto nocleanup;
    }

    /* Read the Raw File */
    status = UTILS_readFileRaw((ubyte*)f, ppRetBuffer, pRetBufLength);

    fclose(f);

nocleanup:
    return status;

} /* UTILS_readFile */


/*------------------------------------------------------------------*/

extern MSTATUS
UTILS_freeReadFile(ubyte **ppRetBuffer)
{
    if (NULL != *ppRetBuffer)
    {
        FREE(*ppRetBuffer);
        *ppRetBuffer = NULL;
    }

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
UTILS_writeFile(const char* pFilename,
                ubyte *pBuffer, ubyte4 bufLength)
{
#ifndef __RTOS_WTOS__
    FILE*   f;
#else
    int     f;
#endif
    MSTATUS status = OK;

    f = fopen(pFilename, "wb");

#ifndef __RTOS_WTOS__
    if (!f)
#else
    if (f < 0)
#endif
    {
        status = ERR_FILE_OPEN_FAILED;
        goto nocleanup;
    }

    if (bufLength != (fwrite(pBuffer, 1, bufLength, f)))
        status = ERR_FILE_WRITE_FAILED;

    fclose(f);

nocleanup:
    return status;
}

#endif /* __DISABLE_MOCANA_FILE_SYSTEM_HELPER__ */
