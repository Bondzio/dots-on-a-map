/*
 * ucos_rtos.c
 *
 * uC/OS RTOS Abstraction Layer
 *
 * Copyright Mocana Corp 2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef __UCOS_RTOS__

#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mtcp.h"
#include "../common/mstdlib.h"
#include "../common/debug_console.h"


#include "../common/constants.h"
#include "../common/utils.h"

#include <KAL/kal.h>
#include <Source/clk.h>
#include <Collections/slist.h>
#include <Source/net.h>
#include <Source/net_util.h>
#include <Secure/net_secure.h>


#define MOCANA_UCOS_LIB_MEM_HEAP_EN                  DEF_ENABLED
#define MOCANA_POOL_NBR                              8u


#ifndef MOCANA_UCOS_RTOS_THREAD_LOW_PRIO
#define MOCANA_UCOS_RTOS_THREAD_LOW_PRIO    (10)    /* you will need to custom priorities based on your design */
#endif

#ifndef MOCANA_UCOS_RTOS_STACK_SIZE
#define MOCANA_UCOS_RTOS_STACK_SIZE         (2048) /* you will need to custom based on your usage overhead */
#endif

#define _REENTRANT

#define MAX_NUM_THREADS 10

#if (MOCANA_UCOS_LIB_MEM_HEAP_EN == DEF_ENABLED)
typedef struct MOCANA_POOL {
    MEM_DYN_POOL  MemPool;
    CPU_INT32U    BlkSize;
    CPU_INT32U    AllocMax;
    CPU_INT32U    AllocCur;
    CPU_INT32U    FreeCtr;
    CPU_INT32U    AllocCtr;
} MOCANA_POOL;


typedef  struct  memory_blk_track_back  MEMORY_BLK_TRACK_BACK;

struct memory_blk_track_back {
    void          *BlkPtr;
    MOCANA_POOL   *PoolPtr;
    SLIST_MEMBER   ListNode;
};
typedef  struct  memory_pools {
    MOCANA_POOL    BlkPoolsTbl[MOCANA_POOL_NBR];
    MEM_DYN_POOL   TrackBackPool;
    SLIST_MEMBER  *TrackBackListPtr;
} MOCANA_POOLS;
static  const  CPU_INT32U  MocanaPoolsSize[MOCANA_POOL_NBR] = {
                                                                   48u,
                                                                   128u,
                                                                   268u,
                                                                   488u,
                                                                   592u,
                                                                   1200u,
                                                                   2048u,
                                                                   17000u,
                                                              };
MOCANA_POOLS  MocanaPools;
MEM_DYN_POOL  MocanaTrackBackPool;
#endif
CPU_INT08U ThreadPrioCur = MOCANA_UCOS_RTOS_THREAD_LOW_PRIO;


static  MOCANA_POOL            *MocanaPoolSrch      (MOCANA_POOL     *p_pools_tbl,
                                                     CPU_INT32U       blk_size);

static  MEMORY_BLK_TRACK_BACK  *MocanaTrackBackSrch (void            *p_blk,
                                                     SLIST_MEMBER    *p_list_head);
/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_rtosInit(void)
{
    CPU_INT32U  i;
    LIB_ERR     err;


    Mem_DynPoolCreate("Mocana memory track back pool",
                      &MocanaPools.TrackBackPool,
                       DEF_NULL,
                       sizeof(MEMORY_BLK_TRACK_BACK),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
        return (ERR_MEM_ALLOC_FAIL);
    }
    for (i = 0; i < MOCANA_POOL_NBR; ++i) {
        MocanaPools.BlkPoolsTbl[i].BlkSize  = MocanaPoolsSize[i];
        MocanaPools.BlkPoolsTbl[i].AllocCtr = 0;
        MocanaPools.BlkPoolsTbl[i].FreeCtr  = 0;
        MocanaPools.BlkPoolsTbl[i].AllocMax = 0;
        MocanaPools.BlkPoolsTbl[i].FreeCtr  = 0;
        Mem_DynPoolCreate("Mocana dyn pool",
                          &MocanaPools.BlkPoolsTbl[i].MemPool,
                           DEF_NULL,
                           MocanaPools.BlkPoolsTbl[i].BlkSize,
                           sizeof(CPU_ALIGN),
                           0u,
                           LIB_MEM_BLK_QTY_UNLIMITED,
                          &err);
        if (err != LIB_MEM_ERR_NONE) {
            return (ERR_MEM_ALLOC_FAIL);
        }
    }
    SList_Init(&MocanaPools.TrackBackListPtr);
    return (OK);
}



/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_rtosShutdown(void)
{
    return (OK);
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_mutexCreate(RTOS_MUTEX* pMutex, enum mutexTypes mutexType, int mutexCount)
{
	KAL_LOCK_HANDLE	handle;
    MSTATUS         status = ERR_RTOS_MUTEX_CREATE;
    KAL_ERR         err;
    MOC_UNUSED(mutexType);
    MOC_UNUSED(mutexCount);


    handle = KAL_LockCreate("Mocana Mutex", DEF_NULL, &err);
    if (KAL_ERR_NONE != err) {
        status = ERR_RTOS_MUTEX_CREATE;
        goto exit;
    }

    status = OK;
   *pMutex = (RTOS_MUTEX)handle.LockObjPtr;

exit:
    return (status);
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_mutexWait(RTOS_MUTEX mutex)
{
	KAL_LOCK_HANDLE  handle;
    MSTATUS          status = ERR_RTOS_MUTEX_WAIT;
    KAL_ERR         err;


    handle.LockObjPtr = mutex;

   //*p_sem_handle = (KAL_SEM_HANDLE *)mutex;

    //if ((NULL != p_sem_handle))
    if (KAL_LOCK_HANDLE_IS_NULL(handle) == DEF_NO)
    {
    	KAL_LockAcquire(handle, KAL_OPT_PEND_BLOCKING, 0, &err);
        if (KAL_ERR_NONE == err)
            status = OK;
    }

    return (status);
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_mutexRelease(RTOS_MUTEX mutex)
{
	KAL_LOCK_HANDLE  handle;
    MSTATUS          status       =  ERR_RTOS_MUTEX_RELEASE;
    KAL_ERR          err;


    handle.LockObjPtr = mutex;

    KAL_LockRelease(handle, &err);
    if (KAL_ERR_NONE == err)
        status = OK;
    else
        status = ERR_RTOS_MUTEX_RELEASE;

    return (status);
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_mutexFree(RTOS_MUTEX* pMutex)
{
    KAL_LOCK_HANDLE  handle;
    MSTATUS          status = ERR_RTOS_MUTEX_FREE;
    KAL_ERR          err;



    handle.LockObjPtr = *pMutex;

    if (KAL_LOCK_HANDLE_IS_NULL(handle) == DEF_YES) {
    	goto exit;
    }

    KAL_LockDel(handle, &err);

    status = OK;

exit:
    return (status);
}


/*------------------------------------------------------------------*/

extern ubyte4
UCOS_getUpTimeInMS(void)
{
    ubyte4 ms;

    ms = NetUtil_TS_Get_ms();
    //ms = OSTimeGet() * (1000 / OS_TICKS_PER_SEC);

    return (ms);
}


/*------------------------------------------------------------------*/

extern ubyte4
UCOS_deltaMS(const moctime_t* origin, moctime_t* current)
{
    ubyte4       retVal = 0;
    NET_TS_MS    ms_cur;



    ms_cur = NetUtil_TS_Get_ms();


    if (origin)
    {
    	retVal = ms_cur - origin->u.time[0];
        //retVal = (1000 / OS_TICKS_PER_SEC) * (os_tick_cur - origin->u.time[0]);
    }

    if (current)
    {
    	current->u.time[0] = ms_cur;
        //current->u.time[0] = os_tick_cur;
    }

    return (retVal);
}


/*------------------------------------------------------------------*/

extern void
UCOS_sleepMS(ubyte4 sleepTimeInMS)
{
	KAL_Dly(sleepTimeInMS);
}



/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_createThread(void (*threadEntry)(void*), void* context, ubyte4 threadType, RTOS_THREAD *pRetTid)
{
	KAL_TASK_HANDLE  task_handle;
	KAL_ERR          err;
    MSTATUS          status = ERR_RTOS_THREAD_CREATE;



    task_handle = KAL_TaskAlloc("Mocana Thread", DEF_NULL, MOCANA_UCOS_RTOS_STACK_SIZE, DEF_NULL, &err);
    if (KAL_ERR_NONE != err) {
    	goto exit;
    }


    KAL_TaskCreate(task_handle, threadEntry, context, ThreadPrioCur, DEF_NULL, &err);
    if (KAL_ERR_NONE != err) {
        goto exit_fail;
    }

	ThreadPrioCur++;

   *pRetTid = (RTOS_THREAD)task_handle.TaskObjPtr;

    goto exit;
exit_fail:
   KAL_TaskDel(task_handle, &err);

exit:
    return (status);
}


/*------------------------------------------------------------------*/

extern void
UCOS_destroyThread(RTOS_THREAD tid)
{
	KAL_TASK_HANDLE	*p_task_handle = (KAL_TASK_HANDLE *)tid;
	KAL_ERR          err;


	KAL_TaskDel(*p_task_handle, &err);
}


/*------------------------------------------------------------------*/

extern MSTATUS
UCOS_timeGMT(TimeDate* td)
{
    CLK_DATE_TIME     localTime;

    if (NULL == td)
        return (ERR_NULL_POINTER);

    Clk_GetDateTime(&localTime);

    td->m_year   = localTime.Yr;
    td->m_month  = localTime.Month;
    td->m_day    = localTime.Day;
    td->m_hour   = localTime.Hr;
    td->m_minute = localTime.Min;
    td->m_second = localTime.Sec;

    return (OK);
}


extern void *UCOS_malloc(ubyte4 size)
{

    void                   *p_blk;
#if (MOCANA_UCOS_LIB_MEM_HEAP_EN == DEF_ENABLED)
    MEMORY_BLK_TRACK_BACK  *p_trackback;
	MOCANA_POOL            *p_pool;
	LIB_ERR                 err;

	p_pool = MocanaPoolSrch(MocanaPools.BlkPoolsTbl, size);
	if (p_pool == DEF_NULL) {
	    return (DEF_NULL);
	}
	p_pool->AllocCtr++;
    p_blk = Mem_DynPoolBlkGet(&p_pool->MemPool, &err);
    if (err != LIB_MEM_ERR_NONE) {
        return (DEF_NULL);
    }
    Mem_Set(p_blk, 0u, size);

    p_pool->AllocCur++;
    if (p_pool->AllocCur > p_pool->AllocMax) {
        p_pool->AllocMax = p_pool->AllocCur;
    }

    p_trackback = (MEMORY_BLK_TRACK_BACK *)Mem_DynPoolBlkGet(&MocanaPools.TrackBackPool, &err);
    if (err != LIB_MEM_ERR_NONE) {
        Mem_DynPoolBlkFree(&p_pool->MemPool, p_blk, &err);
        return (DEF_NULL);
    }
    Mem_Set(p_trackback, 0u, sizeof(MEMORY_BLK_TRACK_BACK));
    p_trackback->BlkPtr  = p_blk;
    p_trackback->PoolPtr = p_pool;
    SList_Push(&MocanaPools.TrackBackListPtr, &p_trackback->ListNode);
    return (p_blk);
#else
    p_blk = malloc(size);
    MOC_UNUSED(err);

	return p_blk;
#endif
}



extern void UCOS_free(void *ptr)
{
#if (MOCANA_UCOS_LIB_MEM_HEAP_EN == DEF_ENABLED)
    MEMORY_BLK_TRACK_BACK  *p_trackback;
	LIB_ERR                 err;

    if (ptr) {
        p_trackback = MocanaTrackBackSrch(ptr, MocanaPools.TrackBackListPtr);
        if (p_trackback == DEF_NULL) {
            return;
        }


        SList_Rem(&MocanaPools.TrackBackListPtr, &p_trackback->ListNode);
        Mem_DynPoolBlkFree(&p_trackback->PoolPtr->MemPool, ptr, &err);
        Mem_DynPoolBlkFree(&MocanaPools.TrackBackPool, p_trackback, &err);
    }
    MOC_UNUSED(err);
#else
    free(ptr);
#endif
}


#if (MOCANA_UCOS_LIB_MEM_HEAP_EN == DEF_ENABLED)
static  MOCANA_POOL  *MocanaPoolSrch (MOCANA_POOL  *p_pools_tbl,
                                      CPU_INT32U    blk_size)
{
    MOCANA_POOL  *p_pool;
    CPU_INT32U    i;
    if (p_pools_tbl == DEF_NULL  ||
        blk_size    == 0) {
        return (DEF_NULL);
    }
    for (i = 0; i < MOCANA_POOL_NBR; ++i) {
        p_pool = &p_pools_tbl[i];
        if (blk_size <= p_pool->BlkSize) {
            return (p_pool);
        }
    }
    return (DEF_NULL);
}


static  MEMORY_BLK_TRACK_BACK  *MocanaTrackBackSrch (void          *p_blk,
                                                     SLIST_MEMBER  *p_list_head)
{
    MEMORY_BLK_TRACK_BACK *p_trackback;
    SLIST_FOR_EACH_ENTRY(p_list_head, p_trackback, MEMORY_BLK_TRACK_BACK, ListNode) {
        if ((p_trackback->BlkPtr != DEF_NULL) &&
            (p_trackback->BlkPtr == p_blk)   ) {
            goto exit;
        }
    }
    p_trackback = DEF_NULL;
exit:
    return (p_trackback);
}
#endif
#endif /* __UCOS_RTOS__ */
