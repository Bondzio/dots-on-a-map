/* Version: mss_v6_3 */
/*
 * mrtos.c
 *
 * Mocana RTOS Helper Functions
 *
 * Copyright Mocana Corp 2008. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"


/*------------------------------------------------------------------*/

extern MSTATUS
MRTOS_mutexWait(RTOS_MUTEX mutex, intBoolean *pIsMutexSet)
{
    MSTATUS status;

    /* check if mutex wait has been previously called */
    if (FALSE != *pIsMutexSet)
    {
        status = ERR_RTOS_WRAP_MUTEX_WAIT;
        goto exit;
    }

    if (OK > (status = RTOS_mutexWait(mutex)))
        goto exit;

    *pIsMutexSet = TRUE;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
MRTOS_mutexRelease(RTOS_MUTEX mutex, intBoolean *pIsMutexSet)
{
    MSTATUS status;

    /* make sure we are trying to release a mutex that we previously waited on */
    if (TRUE != *pIsMutexSet)
    {
        status = ERR_RTOS_WRAP_MUTEX_RELEASE;
        goto exit;
    }

    if (OK > (status = RTOS_mutexRelease(mutex)))
        goto exit;

    *pIsMutexSet = FALSE;

exit:
    return status;
}
