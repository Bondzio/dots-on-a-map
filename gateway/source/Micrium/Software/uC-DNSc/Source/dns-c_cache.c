/*
*********************************************************************************************************
*                                               uC/DNSc
*                                     Domain Name Server (client)
*
*                         (c) Copyright 2004-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/DNSc is provided in source form to registered licensees ONLY.  It is
*                  illegal to distribute this source code to any third party unless you receive
*                  written permission by an authorized Micrium representative.  Knowledge of
*                  the source code may NOT be used to develop a similar product.
*
*                  Please help us continue to provide the Embedded community with the finest
*                  software available.  Your honesty is greatly appreciated.
*
*                  You can find our product's user manual, API reference, release notes and
*                  more information at: https://doc.micrium.com
*
*                  You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                       DNS CLIENT CACHE MODULE
*
* Filename      : dns-c_cache.c
* Version       : V2.00.01
* Programmer(s) : AA
*********************************************************************************************************
* Note(s)       : (1) This file implements a basic DNS client based on RFC #1035.  It provides the
*                     mechanism used to retrieve an IP address from a given host name.
*
*                 (2) Assumes the following versions (or more recent) of software modules are included
*                     in the project build :
*
*                     (a) uC/LIB V1.37
*                     (b) uC/Common-KAL V1.00.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  "dns-c_cache.h"
#include  "dns-c_req.h"
#include  <Source/net_ascii.h>
#include  <Source/net_util.h>
#include  <IF/net_if.h>
#include  <lib_mem.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static        KAL_LOCK_HANDLE  DNScCache_LockHandle;

static        MEM_DYN_POOL     DNScCache_ItemPool;
static        MEM_DYN_POOL     DNScCache_HostObjPool;
static        MEM_DYN_POOL     DNScCache_HostNamePool;
static        MEM_DYN_POOL     DNScCache_AddrItemPool;
static        MEM_DYN_POOL     DNScCache_AddrObjPool;

static        DNSc_CACHE_ITEM  *DNSc_CacheItemListHead;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void              DNScCache_LockAcquire      (       DNSc_ERR         *p_err);

static  void              DNScCache_LockRelease      (void);


static  void              DNScCache_HostRemoveHandler(       DNSc_HOST_OBJ   *p_host);

static  void              DNScCache_HostObjNameSet   (       DNSc_HOST_OBJ   *p_host,
                                                             CPU_CHAR        *p_host_name,
                                                             DNSc_ERR        *p_err);

static  DNSc_HOST_OBJ    *DNScCache_HostSrchByName   (       CPU_CHAR         *p_host_name);

static  CPU_BOOLEAN       DNScCache_HostNameCmp      (       DNSc_HOST_OBJ    *p_host,
                                                             CPU_CHAR         *p_host_name);

static  DNSc_CACHE_ITEM  *DNScCache_ItemGet          (       DNSc_ERR         *p_err);

static  void              DNScCache_ItemFree         (       DNSc_CACHE_ITEM  *p_cache);

static  DNSc_HOST_OBJ    *DNScCache_ItemHostGet      (       void);

static  void              DNScCache_ItemRelease      (       DNSc_CACHE_ITEM  *p_cache);

static  void              DNScCache_ItemRemove       (       DNSc_CACHE_ITEM  *p_cache);

static  DNSc_ADDR_ITEM   *DNScCache_AddrItemGet      (       DNSc_ERR         *p_err);

static  void              DNScCache_AddrItemFree     (       DNSc_ADDR_ITEM   *p_item);

static  void              DNScCache_HostRelease      (       DNSc_HOST_OBJ    *p_host);

static  void              DNScCache_HostAddrClr      (       DNSc_HOST_OBJ    *p_host);

static  DNSc_STATUS       DNScCache_Resolve          (const  DNSc_CFG         *p_cfg,
                                                             DNSc_HOST_OBJ    *p_host,
                                                             DNSc_ERR         *p_err);

static  void              DNScCache_Req              (       DNSc_HOST_OBJ    *p_host,
                                                             DNSc_ERR         *p_err);

static  DNSc_STATUS       DNScCache_Resp             (const  DNSc_CFG         *p_cfg,
                                                             DNSc_HOST_OBJ    *p_host,
                                                             DNSc_ERR         *p_err);


/*
*********************************************************************************************************
*                                           DNScCache_Init()
*
* Description : Initialize cache module.
*
* Argument(s) : p_cfg   Pointer to DNSc's configuration.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE           Cache module successfully initialized.
*                           DNSc_ERR_MEM_ALLOC      Memory allocation error.
*                           DNSc_ERR_FAULT_INIT     Fault during OS object initialization.
*
* Return(s)   : None.
*
* Caller(s)   : DNSc_Init().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_Init (const  DNSc_CFG  *p_cfg,
                             DNSc_ERR  *p_err)
{
    CPU_SIZE_T  nb_addr;
    LIB_ERR     err;
    KAL_ERR     kal_err;


    DNScCache_LockHandle = KAL_LockCreate("DNSc Lock",
                                         KAL_OPT_CREATE_NONE,
                                         &kal_err);
    switch (kal_err) {
        case KAL_ERR_NONE:
             break;

        case KAL_ERR_MEM_ALLOC:
            *p_err = DNSc_ERR_MEM_ALLOC;
             goto exit;

        default:
            *p_err = DNSc_ERR_FAULT_INIT;
             goto exit;
    }


    Mem_DynPoolCreate("DNSc Cache Item Pool",
                      &DNScCache_ItemPool,
                       p_cfg->MemSegPtr,
                       sizeof(DNSc_CACHE_ITEM),
                       sizeof(CPU_ALIGN),
                       1u,
                       p_cfg->CacheEntriesMaxNbr,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }

    DNSc_CacheItemListHead = DEF_NULL;


    Mem_DynPoolCreate("DNSc Cache Host Obj Pool",
                      &DNScCache_HostObjPool,
                       p_cfg->MemSegPtr,
                       sizeof(DNSc_HOST_OBJ),
                       sizeof(CPU_ALIGN),
                       1u,
                       p_cfg->CacheEntriesMaxNbr,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }

    Mem_DynPoolCreate("DNSc Cache Host Obj Pool",
                      &DNScCache_HostNamePool,
                       p_cfg->MemSegPtr,
                       p_cfg->HostNameLenMax,
                       sizeof(CPU_ALIGN),
                       1u,
                       p_cfg->CacheEntriesMaxNbr,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }


    nb_addr = 0u;
#ifdef  NET_IPv4_MODULE_EN
    nb_addr += p_cfg->AddrIPv4MaxPerHost;
#endif
#ifdef  NET_IPv6_MODULE_EN
    nb_addr += p_cfg->AddrIPv6MaxPerHost;
#endif
    nb_addr *= p_cfg->CacheEntriesMaxNbr;

    Mem_DynPoolCreate("DNSc Cache Addr Item Pool",
                      &DNScCache_AddrItemPool,
                       p_cfg->MemSegPtr,
                       sizeof(DNSc_ADDR_ITEM),
                       sizeof(CPU_ALIGN),
                       1u,
                       nb_addr,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }

    nb_addr++;

    Mem_DynPoolCreate("DNSc Cache Addr Obj Pool",
                      &DNScCache_AddrObjPool,
                       p_cfg->MemSegPtr,
                       sizeof(DNSc_ADDR_OBJ),
                       sizeof(CPU_ALIGN),
                       1u,
                       nb_addr,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }

   *p_err = DNSc_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                            DNScCache_Clr()
*
* Description : Clear all elements of the cache.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE   Cache successfully cleared.
*
*                           RETURNED BY DNScCache_LockAcquire():
*                               See DNScCache_LockAcquire() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : DNSc_CacheClrAll().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_Clr (DNSc_ERR  *p_err)
{
    DNSc_CACHE_ITEM  *p_cache = DNSc_CacheItemListHead;
    DNSc_CACHE_ITEM  *p_cache_next;


    DNScCache_LockAcquire(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


    while (p_cache != DEF_NULL) {
        p_cache_next = p_cache->NextPtr;
        switch (p_cache->HostPtr->State) {
            case DNSc_STATE_INIT_REQ:
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                 break;

            case DNSc_STATE_FREE:
            case DNSc_STATE_RESOLVED:
            case DNSc_STATE_FAILED:
            default:
                 DNScCache_ItemRelease(p_cache);
                 break;
        }

        p_cache = p_cache_next;
    }

   *p_err = DNSc_ERR_NONE;

    DNScCache_LockRelease();


exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNScCache_HostInsert()
*
* Description : Add an entry in the cache.
*
* Argument(s) : p_host  Pointer to host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE   Host successfully inserted.
*
*                           RETURNED BY DNScCache_LockAcquire():
*                               See DNScCache_LockAcquire() for additional return error codes.
*
*                           RETURNED BY DNScCache_ItemGet():
*                               See DNScCache_ItemGet() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : DNScTask_ResolveHost().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostInsert (DNSc_HOST_OBJ  *p_host,
                            DNSc_ERR       *p_err)
{
    DNSc_CACHE_ITEM  *p_cache;


    DNScCache_LockAcquire(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }

    p_cache = DNScCache_ItemGet(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit_release;
    }

    p_cache->HostPtr = p_host;

    if (DNSc_CacheItemListHead == DEF_NULL) {
        p_cache->NextPtr        = DEF_NULL;
        DNSc_CacheItemListHead  = p_cache;
    } else {
        p_cache->NextPtr        = DNSc_CacheItemListHead;
        DNSc_CacheItemListHead  = p_cache;
    }

   *p_err = DNSc_ERR_NONE;

exit_release:
    DNScCache_LockRelease();

exit:
    return;
}



/*
*********************************************************************************************************
*                                      DNScCache_HostSrchRemove()
*
* Description : Search host name in cache and remove it.
*
* Argument(s) : p_host_name     Pointer to a string that contains the host name.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   DNSc_ERR_NONE                   Host removed
*                                   DNSc_ERR_CACHE_HOST_NOT_FOUND   Host not found in the cache.
*
*                                   RETURNED BY DNScCache_LockAcquire():
*                                       See DNScCache_LockAcquire() for additional return error codes.
*
*                                   RETURNED BY DNScCache_HostSrchByName():
*                                       See DNScCache_HostSrchByName() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : DNSc_CacheClrHost(),
*               DNSc_GetHost().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostSrchRemove (CPU_CHAR  *p_host_name,
                                DNSc_ERR  *p_err)
{
    DNSc_HOST_OBJ  *p_host;


    DNScCache_LockAcquire(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


    p_host = DNScCache_HostSrchByName(p_host_name);
    if (p_host != DEF_NULL) {
        switch(p_host->State) {
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                *p_err  = DNSc_ERR_CACHE_HOST_PENDING;
                 goto exit_release;

            case DNSc_STATE_RESOLVED:
            case DNSc_STATE_FAILED:
            default:
                *p_err  = DNSc_ERR_NONE;
                 DNScCache_HostRemoveHandler(p_host);
                 goto exit_release;
        }
    }

   *p_err  = DNSc_ERR_CACHE_HOST_NOT_FOUND;                     /* Not found.                                           */

    goto exit_release;


exit_release:
    DNScCache_LockRelease();

exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNScCache_HostRemove()
*
* Description : Remove host from the cache.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Caller(s)   : DNScTask_HostResolve().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostRemove (DNSc_HOST_OBJ  *p_host)
{
    DNSc_ERR          err;

    DNScCache_LockAcquire(&err);
    if (err != DNSc_ERR_NONE) {
         goto exit;
    }

    DNScCache_HostRemoveHandler(p_host);

    DNScCache_LockRelease();

exit:
    return;
}


/*
*********************************************************************************************************
*                                           DNScCache_Srch()
*
* Description : Search host in cache and return IP addresses, if found.
*
* Argument(s) : p_host_name         Pointer to a string that contains the host name.
*
*               p_addrs             Pointer to addresses array.
*
*               addr_nbr_max        Number of address the address array can contains.
*
*               p_addr_nbr_rtn      Pointer to a variable that will receive number of addresses copied.
*
*               flags           DNS client flag:
*
*                                   DNSc_FLAG_NONE              By default all IP address can be returned.
*                                   DNSc_FLAG_IPv4_ONLY         Return only IPv4 address(es).
*                                   DNSc_FLAG_IPv6_ONLY         Return only IPv6 address(es).
*
*               p_err               Pointer to variable that will receive the return error code from this function :
*
*                                       DNSc_ERR_NONE                   Host found.
*                                       DNSc_ERR_CACHE_HOST_PENDING     Host resolution is pending.
*                                       DNSc_ERR_CACHE_HOST_NOT_FOUND   Host not found in the cache.
*
*                                       RETURNED BY DNScCache_LockAcquire():
*                                           See DNScCache_LockAcquire() for additional return error codes.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Caller(s)   : DNSc_GetHost().
*
*               This function is an INTERNAL function & MUST NOT be called by application function(s).
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_STATUS  DNScCache_Srch (CPU_CHAR       *p_host_name,
                             DNSc_ADDR_OBJ  *p_addrs,
                             CPU_INT08U      addr_nbr_max,
                             CPU_INT08U     *p_addr_nbr_rtn,
                             DNSc_FLAGS      flags,
                             DNSc_ERR       *p_err)
{
    CPU_INT08U       i       = 0u;
    DNSc_HOST_OBJ   *p_host  = DEF_NULL;
    DNSc_ADDR_OBJ   *p_addr  = DEF_NULL;
    DNSc_ADDR_ITEM  *p_item  = DEF_NULL;
    DNSc_STATUS      status  = DNSc_STATUS_FAILED;
    CPU_BOOLEAN      no_ipv4 = DEF_BIT_IS_SET(flags, DNSc_FLAG_IPv6_ONLY);
    CPU_BOOLEAN      no_ipv6 = DEF_BIT_IS_SET(flags, DNSc_FLAG_IPv4_ONLY);


   *p_addr_nbr_rtn = 0u;

    DNScCache_LockAcquire(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


    p_host = DNScCache_HostSrchByName(p_host_name);
    if (p_host != DEF_NULL) {
        switch(p_host->State) {
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                 status = DNSc_STATUS_PENDING;
                *p_err  = DNSc_ERR_CACHE_HOST_PENDING;
                 goto exit_release;

            case DNSc_STATE_RESOLVED:
                 status = DNSc_STATUS_RESOLVED;
                 goto exit_found;

            case DNSc_STATE_FAILED:
            default:
                *p_err  = DNSc_ERR_NONE;
                 goto exit_release;
        }
    }

   *p_err  = DNSc_ERR_CACHE_HOST_NOT_FOUND;                     /* Not found.                                           */

    goto exit_release;


exit_found:

    p_item = p_host->AddrsFirstPtr;

    for (i = 0u; i < p_host->AddrsCount; i++) {                 /* Copy Addresses                                       */
        p_addr = p_item->AddrPtr;
        if (*p_addr_nbr_rtn < addr_nbr_max) {
            CPU_BOOLEAN  add_addr = DEF_YES;


            switch (p_addr->Len) {
                case NET_IPv4_ADDR_SIZE:
                     if (no_ipv4 == DEF_YES) {
                         add_addr = DEF_NO;
                     }
                     break;

                case NET_IPv6_ADDR_SIZE:
                     if (no_ipv6 == DEF_YES) {
                         add_addr = DEF_NO;
                     }
                     break;

                default:
                     add_addr = DEF_NO;
                     break;
            }

            if (add_addr == DEF_YES) {
                p_addrs[*p_addr_nbr_rtn] = *p_addr;
               *p_addr_nbr_rtn += 1u;
            }

            p_item = p_item->NextPtr;

        } else {
            goto exit_release;
        }
    }

   (void)&p_addr_nbr_rtn;

   *p_err = DNSc_ERR_NONE;

exit_release:
    DNScCache_LockRelease();


exit:
    return (status);
}


/*
*********************************************************************************************************
*                                        DNScCache_HostObjGet()
*
* Description : Get a free host object.
*
* Argument(s) : p_host_name     Pointer to a string that contains the domain name.
*
*               flags           DNS client flag:
*
*                                   DNSc_FLAG_NONE              By default this function is blocking.
*                                   DNSc_FLAG_NO_BLOCK          Don't block (only possible if DNSc's task is enabled).
*                                   DNSc_FLAG_FORCE_CACHE       Take host from the cache, don't send new DNS request.
*                                   DNSc_FLAG_FORCE_RENW       Force DNS request, remove existing entry in the cache.
*
*               p_cfg           Pointer to a request configuration. Should be set to overwrite default DNS configuration
*                               (such as DNS server, request timeout, etc.). Must be set to DEF_NULL to use default
*                               configuration.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   DNSc_ERR_NONE           Successfully acquired a host object.
*                                   DNSc_ERR_MEM_ALLOC      Not able to allocate a host object.
*
*                                   RETURNED BY DNScCache_LockAcquire():
*                                       See DNScCache_LockAcquire() for additional return error codes.
*
*                                   RETURNED BY DNScCache_HostObjNameSet():
*                                       See DNScCache_HostObjNameSet() for additional return error codes.
*
* Return(s)   : Pointer to the host object acquired.
*
* Caller(s)   : DNSc_GetHost().
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_HOST_OBJ  *DNScCache_HostObjGet (CPU_CHAR      *p_host_name,
                                      DNSc_FLAGS     flags,
                                      DNSc_REQ_CFG  *p_cfg,
                                      DNSc_ERR      *p_err)
{
#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    KAL_SEM_HANDLE   sem    = KAL_SemHandleNull;
#endif
    DNSc_HOST_OBJ   *p_host = DEF_NULL;
    LIB_ERR          err;


    DNScCache_LockAcquire(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


                                                                /* ---------- ACQUIRE SIGNAL TASK SEMAPHORE ----------- */
#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    if (DEF_BIT_IS_SET(flags, DNSc_FLAG_NO_BLOCK) == DEF_NO) {
        KAL_ERR  kal_err;


        sem = KAL_SemCreate("DNSc Block Task Signal", DEF_NULL, &kal_err);
        if (kal_err != KAL_ERR_NONE) {
            *p_err = DNSc_ERR_MEM_ALLOC;
             goto exit;
        }
    }
#endif



    p_host = (DNSc_HOST_OBJ *)Mem_DynPoolBlkGet(&DNScCache_HostObjPool, &err);
    if (err == LIB_MEM_ERR_NONE) {
        p_host->NamePtr = (CPU_CHAR *)Mem_DynPoolBlkGet(&DNScCache_HostNamePool, &err);
        if (err != LIB_MEM_ERR_NONE) {
            *p_err = DNSc_ERR_MEM_ALLOC;
             goto exit_free_host_obj;
        }

    } else {
        p_host = DNScCache_ItemHostGet();
    }

    if (p_host == DEF_NULL) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit_release;
    }

    if (p_host->NamePtr == DEF_NULL) {
        p_host->NameLenMax = DNScCache_HostNamePool.BlkSize - 1;
    }

    p_host->NameLenMax = DNScCache_HostNamePool.BlkSize;
    Mem_Clr(p_host->NamePtr, p_host->NameLenMax);
    p_host->IF_Nbr         = NET_IF_NBR_WILDCARD;
    p_host->AddrsCount     = 0u;
    p_host->AddrsIPv4Count = 0u;
    p_host->AddrsIPv6Count = 0u;
    p_host->AddrsFirstPtr  = DEF_NULL;
    p_host->AddrsEndPtr    = DEF_NULL;

#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    p_host->TaskSignal = sem;
#endif

    p_host->State      = DNSc_STATE_INIT_REQ;
    p_host->ReqCfgPtr  = p_cfg;

    DNScCache_HostObjNameSet(p_host, p_host_name, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit_release;
    }

    goto exit_release;


exit_free_host_obj:
     Mem_DynPoolBlkFree(&DNScCache_HostObjPool, p_host, &err);

exit_release:
    DNScCache_LockRelease();

exit:
    return (p_host);
}


/*
*********************************************************************************************************
*                                        DNScCache_HostObjFree()
*
* Description : Free a host object.
*
* Argument(s) : p_host  Pointer to the host object to free.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_HostRelease(),
*               DNSc_GetHost().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostObjFree (DNSc_HOST_OBJ  *p_host)
{
    LIB_ERR          err;


#ifdef DNSc_SIGNAL_TASK_MODULE_EN
    if (p_host->TaskSignal.SemObjPtr != KAL_SemHandleNull.SemObjPtr) {
        KAL_ERR  kal_err;


        KAL_SemDel(p_host->TaskSignal, &kal_err);
    }

    p_host->TaskSignal = KAL_SemHandleNull;
#endif


    DNScCache_HostAddrClr(p_host);
    Mem_DynPoolBlkFree(&DNScCache_HostNamePool, p_host->NamePtr, &err);
    Mem_DynPoolBlkFree(&DNScCache_HostObjPool,  p_host,          &err);
}


/*
*********************************************************************************************************
*                                      DNScCache_HostAddrInsert()
*
* Description : Insert address object in the addresses list of the host object.
*
* Argument(s) : p_cfg   Pointer to DNSc's configuration.
*
*               p_host  Pointer to the host object.
*
*               p_addr  Pointer to the address object (must be acquired with cache module)
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE       Address successfully added.
*                           DNSc_ERR_MEM_ALLOC  Unable to insert the IP address due to the memory configuration
*                           DNSc_ERR_FAULT      Unknown error (should not occur)
*
*                           RETURNED BY DNScCache_AddrItemGet():
*                               See DNScCache_AddrItemGet() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : DNScReq_RxRespAddAddr().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_HostAddrInsert (const  DNSc_CFG       *p_cfg,
                                       DNSc_HOST_OBJ  *p_host,
                                       DNSc_ADDR_OBJ  *p_addr,
                                       DNSc_ERR       *p_err)
{
    DNSc_ADDR_ITEM  *p_item_cur;


    switch (p_addr->Len) {
        case NET_IPv4_ADDR_SIZE:
             if (p_host->AddrsIPv4Count >= p_cfg->AddrIPv4MaxPerHost) {
                 *p_err = DNSc_ERR_MEM_ALLOC;
                 goto exit;
             }
             break;


        case NET_IPv6_ADDR_SIZE:
             if (p_host->AddrsIPv6Count >= p_cfg->AddrIPv6MaxPerHost) {
                *p_err = DNSc_ERR_MEM_ALLOC;
                 goto exit;
             }
             break;

        default:
            *p_err = DNSc_ERR_FAULT;
             goto exit;
    }

    p_item_cur = DNScCache_AddrItemGet(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


    p_item_cur->AddrPtr = p_addr;

    if (p_host->AddrsFirstPtr == DEF_NULL) {
        p_host->AddrsFirstPtr = p_item_cur;
        p_host->AddrsEndPtr   = p_item_cur;

    } else {
        p_host->AddrsEndPtr->NextPtr = p_item_cur;
        p_host->AddrsEndPtr          = p_item_cur;
    }

    switch (p_addr->Len) {
        case NET_IPv4_ADDR_SIZE:
             p_host->AddrsIPv4Count++;
             break;


        case NET_IPv6_ADDR_SIZE:
             p_host->AddrsIPv6Count++;
             break;

        default:
            *p_err = DNSc_ERR_FAULT;
             goto exit;
    }

    p_host->AddrsCount++;


exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrObjGet()
*
* Description : Acquire an address object that can be inserted in host list afterward.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE   Successfully acquired an address object.
*                           DNSc_ERR_MEM_ALLOC  Unable to acquire an address object.
*
* Return(s)   : Pointer to the address object acquired, if no error.
*
*               DEF_NULL, otherwise
*
* Caller(s)   : DNScReq_RxRespAddAddr().
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_ADDR_OBJ  *DNScCache_AddrObjGet (DNSc_ERR  *p_err)
{
    DNSc_ADDR_OBJ  *p_addr = DEF_NULL;
    LIB_ERR         err;


    p_addr = (DNSc_ADDR_OBJ *)Mem_DynPoolBlkGet(&DNScCache_AddrObjPool, &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }

    Mem_Clr(p_addr, sizeof(DNSc_ADDR_OBJ));

   *p_err = DNSc_ERR_NONE;

exit:
    return (p_addr);
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrObjFree()
*
* Description : Free an address object
*
* Argument(s) : p_addr  Pointer to address object to free.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_AddrItemFree(),
*               DNScReq_RxRespAddAddr().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_AddrObjFree (DNSc_ADDR_OBJ  *p_addr)
{
    LIB_ERR  err;


    Mem_DynPoolBlkFree(&DNScCache_AddrObjPool, p_addr, &err);
   (void)&err;
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrObjSet()
*
* Description : Set address object from IP string.
*
* Argument(s) : p_addr      Pointer to the address object.
*
*               p_str_addr  Pointer to the string that contains the IP address.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               DNSc_ERR_NONE           Address successfully set.
*                               DNSc_ERR_ADDR_INVALID   Invalid IP address.
*
* Return(s)   : None.
*
* Caller(s)   : DNScReq_ServerInit(),
*               DNSc_CfgServerByStr().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNScCache_AddrObjSet (DNSc_ADDR_OBJ  *p_addr,
                            CPU_CHAR       *p_str_addr,
                            DNSc_ERR       *p_err)
{
    NET_IP_ADDR_FAMILY  ip_addr_family;
    NET_ERR             net_err;


    ip_addr_family = NetASCII_Str_to_IP(         p_str_addr,
                                        (void *)&p_addr->Addr,
                                                 sizeof(p_addr->Addr),
                                                &net_err);
    if (net_err != NET_ASCII_ERR_NONE) {
        *p_err = DNSc_ERR_ADDR_INVALID;
         goto exit;
    }

    switch (ip_addr_family) {
        case NET_IP_ADDR_FAMILY_IPv4:
             p_addr->Len = NET_IPv4_ADDR_SIZE;
             break;

        case NET_IP_ADDR_FAMILY_IPv6:
             p_addr->Len = NET_IPv6_ADDR_SIZE;
             break;

        default:
            *p_err = DNSc_ERR_ADDR_INVALID;
             goto exit;
    }

   *p_err = DNSc_ERR_NONE;

    exit:
        return;
}


/*
*********************************************************************************************************
*                                        DNScCache_ResolveHost()
*
* Description : Launch resolution of an host.
*
* Argument(s) : p_cfg   Pointer to DNSc's configuration.
*
*               p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RETURNED BY DNScCache_LockAcquire():
*                               See DNScCache_LockAcquire() for additional return error codes.
*
*                           RETURNED BY DNScCache_Resolve():
*                               See DNScCache_Resolve() for additional return error codes.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Caller(s)   : DNScTask().
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_STATUS  DNScCache_ResolveHost (const  DNSc_CFG       *p_cfg,
                                           DNSc_HOST_OBJ  *p_host,
                                           DNSc_ERR       *p_err)
{
    DNSc_STATUS  status;


    DNScCache_LockAcquire(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


    status = DNScCache_Resolve(p_cfg, p_host, p_err);

    DNScCache_LockRelease();


exit:
    return (status);
}


/*
*********************************************************************************************************
*                                        DNScCache_ResolveAll()
*
* Description : Launch resolution on all entries that are pending in the cache.
*
* Argument(s) : p_cfg   Pointer to DNSc's configuration.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE   Resolution has been launched on all entries.
*
*                           RETURNED BY DNScCache_LockAcquire():
*                               See DNScCache_LockAcquire() for additional return error codes.
*
* Return(s)   : Number of entries that are completed.
*
* Caller(s)   : DNScTask().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16U  DNScCache_ResolveAll (const  DNSc_CFG  *p_cfg,
                                         DNSc_ERR  *p_err)
{
    DNSc_CACHE_ITEM  *p_item;
    DNSc_HOST_OBJ    *p_host;
    DNSc_STATUS       status;
    CPU_INT16U        resolved_ctr = 0u;


    DNScCache_LockAcquire(p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


    p_item = DNSc_CacheItemListHead;

    while (p_item != DEF_NULL) {
        p_host = p_item->HostPtr;

        if (p_host->State != DNSc_STATE_RESOLVED) {
            status = DNScCache_Resolve(p_cfg, p_host, p_err);
            switch (status) {
                case DNSc_STATUS_NONE:
                case DNSc_STATUS_PENDING:
                     break;

                case DNSc_STATUS_RESOLVED:
                case DNSc_STATUS_FAILED:
                default:
#ifdef  DNSc_SIGNAL_TASK_MODULE_EN
                    if (KAL_SEM_HANDLE_IS_NULL(p_host->TaskSignal) != DEF_YES) {
                        KAL_ERR  kal_err;


                        KAL_SemPost(p_host->TaskSignal, KAL_OPT_NONE, &kal_err);
                    }
#endif
                    resolved_ctr++;
                    break;
            }
        }

        p_item = p_item->NextPtr;
    }


   *p_err = DNSc_ERR_NONE;

    DNScCache_LockRelease();


exit:
    return (resolved_ctr);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        DNScCache_LockAcquire()
*
* Description : Acquire lock on the cache list.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE           Lock successfully acquired.
*                           DNSc_ERR_CACHE_LOCK     Unable to acquire the lock.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_Clr(),
*               DNScCache_HostInsert(),
*               DNScCache_HostObjGet(),
*               DNScCache_HostRemove(),
*               DNScCache_HostSrchRemove(),
*               DNScCache_ResolveAll(),
*               DNScCache_ResolveHost(),
*               DNScCache_Srch().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_LockAcquire (DNSc_ERR  *p_err)
{
    KAL_ERR  err;


    KAL_LockAcquire(DNScCache_LockHandle, KAL_OPT_PEND_NONE, 0, &err);
    if (err != KAL_ERR_NONE) {
        *p_err = DNSc_ERR_CACHE_LOCK;
         goto exit;
    }


   *p_err = DNSc_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNScCache_LockRelease()
*
* Description : Release cache list lock.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_Clr(),
*               DNScCache_HostInsert(),
*               DNScCache_HostObjGet(),
*               DNScCache_HostRemove(),
*               DNScCache_HostSrchRemove(),
*               DNScCache_ResolveAll(),
*               DNScCache_ResolveHost(),
*               DNScCache_Srch().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_LockRelease (void)
{
    KAL_ERR  err;


    KAL_LockRelease(DNScCache_LockHandle, &err);
}


/*
*********************************************************************************************************
*                                     DNScCache_HostRemoveHandler()
*
* Description : Remove host from the cache.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_HostRemove(),
*               DNScCache_HostSrchRemove().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_HostRemoveHandler (DNSc_HOST_OBJ  *p_host)
{
    DNSc_CACHE_ITEM  *p_cache = DNSc_CacheItemListHead;


    while (p_cache != DEF_NULL) {
        if (p_cache->HostPtr == p_host) {
            DNScCache_ItemRelease(p_cache);
            goto exit;
        }
        p_cache = p_cache->NextPtr;
    }

exit:
    return;
}


/*
*********************************************************************************************************
*                                      DNScCache_HostObjNameSet()
*
* Description : Set the name in host object.
*
* Argument(s) : p_host          Pointer to the host object to set.
*
*               p_host_name     Pointer to a string that contains the domain name.

*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                               DNSc_ERR_NONE   Host name successfully set.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_HostObjGet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_HostObjNameSet (DNSc_HOST_OBJ  *p_host,
                                        CPU_CHAR       *p_host_name,
                                        DNSc_ERR       *p_err)
{
     Str_Copy_N(p_host->NamePtr, p_host_name, p_host->NameLenMax);
    *p_err = DNSc_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      DNScCache_HostSrchByName()
*
* Description : Search for an host in the cache from a host name string.
*
* Argument(s) : p_host_name     Pointer to a string that contains the domain name.
*
* Return(s)   : Pointer to the host object, if found.
*
*               DEF_NULL, Otherwise.
*
* Caller(s)   : DNScCache_HostSrchRemove(),
*               DNScCache_Srch().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_HOST_OBJ  *DNScCache_HostSrchByName (CPU_CHAR  *p_host_name)
{
    DNSc_HOST_OBJ    *p_host  = DEF_NULL;
    DNSc_CACHE_ITEM  *p_cache = DNSc_CacheItemListHead;
    CPU_BOOLEAN       match;


    if (p_cache == DEF_NULL) {
         goto exit;
    }

    while (p_cache != DEF_NULL) {
        p_host = p_cache->HostPtr;
        match  = DNScCache_HostNameCmp(p_host, p_host_name);
        if (match == DEF_YES) {
            goto exit;
        }

        p_cache = p_cache->NextPtr;
    }

    p_host = DEF_NULL;

exit:
    return (p_host);
}


/*
*********************************************************************************************************
*                                        DNScCache_HostNameCmp()
*
* Description : Compare host object name field and a host name hane string
*
* Argument(s) : p_host          Pointer to the host object.
*
*               p_host_name     Pointer to a string that contains the host name.
*
* Return(s)   : DEF_OK, if names match
*
*               DEF_FAIL, otherwise
*
* Caller(s)   : DNScCache_HostSrchByName().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  DNScCache_HostNameCmp (DNSc_HOST_OBJ  *p_host,
                                            CPU_CHAR       *p_host_name)
{
    CPU_BOOLEAN  result = DEF_FAIL;
    CPU_INT16S   cmp;


    cmp = Str_Cmp_N(p_host_name, p_host->NamePtr, p_host->NameLenMax);
    if (cmp == 0) {
        result = DEF_OK;
    }

    return (result);
}


/*
*********************************************************************************************************
*                                          DNScCache_ItemGet()
*
* Description : Get an Cache item element (list element)
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
* Return(s)   : Pointer to the item element.
*
* Caller(s)   : DNScCache_HostInsert().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_CACHE_ITEM  *DNScCache_ItemGet (DNSc_ERR  *p_err)
{
    DNSc_CACHE_ITEM  *p_cache;
    LIB_ERR           err;


    p_cache = (DNSc_CACHE_ITEM *)Mem_DynPoolBlkGet(&DNScCache_ItemPool, &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }

   *p_err = DNSc_ERR_NONE;

exit:
    return (p_cache);
}


/*
*********************************************************************************************************
*                                         DNScCache_ItemFree()
*
* Description : Free cache item element.
*
* Argument(s) : p_cache Pointer to cache item element.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_ItemHostGet(),
*               DNScCache_ItemRelease().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_ItemFree (DNSc_CACHE_ITEM  *p_cache)
{
    LIB_ERR  err;


    Mem_DynPoolBlkFree(&DNScCache_ItemPool, p_cache, &err);
}



/*
*********************************************************************************************************
*                                        DNScCache_ItemHostGet()
*
* Description : Get host item element (list element).
*
* Argument(s) : none.
*
* Return(s)   : Pointer to host item element.
*
* Caller(s)   : DNScCache_HostObjGet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_HOST_OBJ  *DNScCache_ItemHostGet (void)
{
    DNSc_CACHE_ITEM  *p_item_cur = DNSc_CacheItemListHead;
    DNSc_HOST_OBJ    *p_host     = DEF_NULL;


    if (p_item_cur == DEF_NULL) {
        goto exit;
    }


    while (p_item_cur != DEF_NULL) {
        p_host = p_item_cur->HostPtr;
        switch (p_host->State) {
            case DNSc_STATE_TX_REQ_IPv4:
            case DNSc_STATE_RX_RESP_IPv4:
            case DNSc_STATE_TX_REQ_IPv6:
            case DNSc_STATE_RX_RESP_IPv6:
                 break;

            case DNSc_STATE_FREE:
            case DNSc_STATE_FAILED:
            case DNSc_STATE_RESOLVED:
                 goto exit_found;

            default:
                 p_host = DEF_NULL;
                 goto exit;
        }
        p_item_cur = p_item_cur->NextPtr;
    }


    p_host = DEF_NULL;
    goto exit;


exit_found:
    DNScCache_ItemRemove(p_item_cur);
    DNScCache_HostAddrClr(p_host);

exit:
    return (p_host);
}


/*
*********************************************************************************************************
*                                        DNScCache_ItemRelease()
*
* Description : Release a cache item and everything contained in the item.
*
* Argument(s) : p_cache     Pointer to the cache item.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_Clr(),
*               DNScCache_HostRemoveHandler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_ItemRelease (DNSc_CACHE_ITEM  *p_cache)
{
    if (p_cache->HostPtr != DEF_NULL) {
        DNScCache_HostRelease(p_cache->HostPtr);
    }

    DNScCache_ItemRemove(p_cache);
}


/*
*********************************************************************************************************
*                                        DNScCache_ItemRemove()
*
* Description : Remove an item (list element) in the cache.
*
* Argument(s) : p_cache     Pointer to the cache list element to remove.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_ItemHostGet(),
*               DNScCache_ItemRelease().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_ItemRemove (DNSc_CACHE_ITEM  *p_cache)
{
    if (DNSc_CacheItemListHead == p_cache) {
        DNSc_CacheItemListHead = p_cache->NextPtr;
        goto exit_found;

    } else {
        DNSc_CACHE_ITEM  *p_cache_cur  = DNSc_CacheItemListHead->NextPtr;
        DNSc_CACHE_ITEM  *p_cache_prev = DNSc_CacheItemListHead;

        while (p_cache_cur != DEF_NULL) {
            if (p_cache_cur == p_cache) {
                p_cache_prev->NextPtr = p_cache_cur->NextPtr;
                goto exit_found;
            }
        }
    }

    goto exit;


exit_found:
    DNScCache_ItemFree(p_cache);

exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNScCache_AddrItemGet()
*
* Description : get an address item element (list)
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE       Successfully acquired an address element.
*                           DNSc_ERR_MEM_ALLOC  Unable to acquire an address element.
*
* Return(s)   : Pointer to address element, if no error.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : DNScCache_HostAddrInsert().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_ADDR_ITEM  *DNScCache_AddrItemGet (DNSc_ERR  *p_err)
{
    DNSc_ADDR_ITEM  *p_item = DEF_NULL;
    LIB_ERR          err;


    p_item = (DNSc_ADDR_ITEM *)Mem_DynPoolBlkGet(&DNScCache_AddrItemPool, &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = DNSc_ERR_MEM_ALLOC;
        goto exit;
    }

    p_item->AddrPtr = DEF_NULL;
    p_item->NextPtr = DEF_NULL;

   *p_err = DNSc_ERR_NONE;

exit:
    return (p_item);
}



/*
*********************************************************************************************************
*                                       DNScCache_AddrItemFree()
*
* Description : Free an address item element.
*
* Argument(s) : p_item  Pointer to the address item element.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_HostRelease().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_AddrItemFree (DNSc_ADDR_ITEM  *p_item)
{
    LIB_ERR  err;


    DNScCache_AddrObjFree(p_item->AddrPtr);
    if (p_item->AddrPtr != DEF_NULL) {
        Mem_DynPoolBlkFree(&DNScCache_AddrItemPool, p_item, &err);
    }

   (void)&err;
}

/*
*********************************************************************************************************
*                                        DNScCache_HostRelease()
*
* Description : Release an host and all element contained in the host.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_ItemRelease().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  DNScCache_HostRelease (DNSc_HOST_OBJ  *p_host)
{


    DNScCache_HostAddrClr(p_host);
    DNScCache_HostObjFree(p_host);
}




/*
*********************************************************************************************************
*                                        DNScCache_HostAddrClr()
*
* Description : Remove and free all address elements contained in a host object.
*
* Argument(s) : p_host  Pointer to the host object.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_HostObjFree(),
*               DNScCache_HostRelease(),
*               DNScCache_ItemHostGet().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_HostAddrClr (DNSc_HOST_OBJ  *p_host)
{
    DNSc_ADDR_ITEM  *p_addr_coll_cur  = p_host->AddrsFirstPtr;
    DNSc_ADDR_ITEM  *p_addr_coll_next = DEF_NULL;


    while (p_addr_coll_cur != DEF_NULL) {
        p_addr_coll_next = p_addr_coll_cur->NextPtr;

        DNScCache_AddrItemFree(p_addr_coll_cur);

        p_addr_coll_cur = p_addr_coll_next;
    }

    p_host->AddrsFirstPtr = DEF_NULL;
    p_host->AddrsEndPtr   = DEF_NULL;
}


/*
*********************************************************************************************************
*                                          DNScCache_Resolve()
*
* Description : Process resolution of an host (state machine controller).
*
* Argument(s) : p_cfg   Pointer to DNSc's configuration.
*
*               p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE   No error.
*                           DNSc_ERR_FAULT  Unknown error (should not occur).
*
*                           RETURNED BY DNScReq_Init():
*                               See DNScReq_Init() for additional return error codes.
*
*                           RETURNED BY DNScCache_Req():
*                               See DNScCache_Req() for additional return error codes.
*
*                           RETURNED BY DNScCache_Resp():
*                               See DNScCache_Resp() for additional return error codes.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Caller(s)   : DNScCache_ResolveAll(),
*               DNScCache_ResolveHost().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_STATUS  DNScCache_Resolve (const  DNSc_CFG       *p_cfg,
                                               DNSc_HOST_OBJ  *p_host,
                                               DNSc_ERR       *p_err)
{
    DNSc_STATUS     status        = DNSc_STATUS_PENDING;
    DNSc_ADDR_OBJ  *p_server_addr = DEF_NULL;
    NET_PORT_NBR    server_port   = NET_PORT_NBR_NONE;


    switch (p_host->State) {
        case DNSc_STATE_INIT_REQ:
             if (p_host->ReqCfgPtr != DEF_NULL) {
                 p_server_addr = p_host->ReqCfgPtr->ServerAddrPtr;
                 server_port   = p_host->ReqCfgPtr->ServerPort;
             }

             p_host->SockID = DNScReq_Init(p_server_addr, server_port, p_err);
             if (*p_err != DNSc_ERR_NONE) {
                 status = DNSc_STATUS_FAILED;
                 goto exit;
             }

             p_host->ReqCtr = 0u;
             p_host->State  = DNSc_STATE_IF_SEL;
             status         = DNSc_STATUS_PENDING;
             break;


        case DNSc_STATE_IF_SEL:
             p_host->IF_Nbr = DNSc_ReqIF_Sel(p_host->IF_Nbr, p_host->SockID, p_err);
             if (*p_err != DNSc_ERR_NONE) {
                 status = DNSc_STATUS_FAILED;
                 break;
             }

#ifdef  NET_IPv4_MODULE_EN
             p_host->State = DNSc_STATE_TX_REQ_IPv4;
#else
   #ifdef  NET_IPv6_MODULE_EN
             p_host->State = DNSc_STATE_TX_REQ_IPv6;
   #else
            *p_err = DNSc_ERR_FAULT;
             goto exit;
   #endif
#endif

            status = DNSc_STATUS_PENDING;
            break;

        case DNSc_STATE_TX_REQ_IPv4:
        case DNSc_STATE_TX_REQ_IPv6:
             DNScCache_Req(p_host, p_err);
             status = DNSc_STATUS_PENDING;
             break;


        case DNSc_STATE_RX_RESP_IPv4:
        case DNSc_STATE_RX_RESP_IPv6:
             status = DNScCache_Resp(p_cfg, p_host, p_err);
             break;


        case DNSc_STATE_RESOLVED:
             status = DNSc_STATUS_RESOLVED;
            *p_err  = DNSc_ERR_NONE;
             break;


        case DNSc_STATE_FREE:
        default:
             status = DNSc_STATUS_FAILED;
            *p_err  = DNSc_ERR_FAULT;
             goto exit;
    }


    switch (status) {
        case DNSc_STATUS_PENDING:
             break;

        case DNSc_STATUS_RESOLVED:
        case DNSc_STATUS_FAILED:
        default:
             DNSc_ReqClose(p_host->SockID);
             break;
    }


exit:
    return (status);
}

/*
*********************************************************************************************************
*                                            DNScCache_Req()
*
* Description : Send an host resolution request.
*
* Argument(s) : p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE   No error.
*                           DNSc_ERR_FAULT  Unknown error (should not occur).
*
*                           RETURNED BY DNScReq_TxReq():
*                               See DNScReq_TxReq() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : DNScCache_Resolve().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DNScCache_Req (DNSc_HOST_OBJ  *p_host,
                             DNSc_ERR       *p_err)
{
    DNSc_REQ_TYPE  req_type;


    switch (p_host->State) {
        case DNSc_STATE_TX_REQ_IPv4:
             req_type = DNSc_REQ_TYPE_IPv4;
             break;

        case DNSc_STATE_TX_REQ_IPv6:
             req_type = DNSc_REQ_TYPE_IPv6;
             break;

        default:
            *p_err = DNSc_ERR_FAULT;
             goto exit;
    }


    p_host->QueryID = DNScReq_TxReq(p_host->NamePtr, p_host->SockID, DNSc_QUERY_ID_NONE, req_type, p_err);
    switch (*p_err) {
        case DNSc_ERR_NONE:
             break;

        case DNSc_ERR_IF_LINK_DOWN:
             p_host->State = DNSc_STATE_IF_SEL;
             goto exit_no_err;

        default:
             goto exit;
    }

    switch (p_host->State) {
        case DNSc_STATE_TX_REQ_IPv4:
             p_host->State = DNSc_STATE_RX_RESP_IPv4;
             break;

        case DNSc_STATE_TX_REQ_IPv6:
             p_host->State = DNSc_STATE_RX_RESP_IPv6;
             break;

        default:
            *p_err = DNSc_ERR_FAULT;
             goto exit;
    }


    p_host->TS_ms = NetUtil_TS_Get_ms();
    p_host->ReqCtr++;


exit_no_err:
   *p_err = DNSc_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                           DNScCache_Resp()
*
* Description : Receive host resolution request response.
*
* Argument(s) : p_cfg   Pointer to DNSc's configuration.
*
*               p_host  Pointer to the host object.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE   No error.
*                           DNSc_ERR_FAULT  Unknown error (should not occur).
*
*                           RETURNED BY DNScReq_TxReq():
*                               See DNScReq_TxReq() for additional return error codes.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Caller(s)   : DNScCache_Resolve().
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  DNSc_STATUS  DNScCache_Resp (const  DNSc_CFG       *p_cfg,
                                            DNSc_HOST_OBJ  *p_host,
                                            DNSc_ERR       *p_err)
{
    DNSc_STATUS  status;
    NET_TS_MS    ts_cur_ms;
    NET_TS_MS    ts_delta_ms;
    CPU_INT16U   timeout_ms   = p_cfg->ReqRetryTimeout_ms;
    CPU_INT08U   req_retry    = p_cfg->ReqRetryNbrMax;
    CPU_BOOLEAN  re_tx        = DEF_NO;
    CPU_BOOLEAN  change_state = DEF_NO;


    if (p_host->ReqCfgPtr != DEF_NULL) {
        timeout_ms = p_host->ReqCfgPtr->ReqTimeout_ms;
        req_retry  = p_host->ReqCfgPtr->ReqRetry;
    }

    status = DNScReq_RxResp(p_cfg, p_host, p_host->SockID, p_host->QueryID, p_err);
    switch (*p_err) {
        case DNSc_ERR_NONE:
             change_state = DEF_YES;
             break;

        case DNSc_ERR_RX:
             if (p_host->ReqCtr >= req_retry) {
                 status        = DNSc_STATUS_FAILED;
                 p_host->State = DNSc_STATE_FAILED;
                *p_err         = DNSc_ERR_NO_SERVER;
                 goto exit;

             } else {
                 ts_cur_ms   = NetUtil_TS_Get_ms();
                 ts_delta_ms = ts_cur_ms - p_host->TS_ms;
                 if (ts_delta_ms >= timeout_ms) {
                     re_tx        = DEF_YES;
                     change_state = DEF_YES;
                 }
             }
             break;

        default:
            goto exit;
    }

    if (change_state == DEF_YES) {
        switch (p_host->State) {
            case DNSc_STATE_RX_RESP_IPv4:
                 if (re_tx == DEF_YES) {
                     p_host->State = DNSc_STATE_TX_REQ_IPv4;

                 } else {
#ifdef  NET_IPv6_MODULE_EN
                     p_host->ReqCtr = 0;
                     p_host->State  = DNSc_STATE_TX_REQ_IPv6;
                     status         = DNSc_STATUS_PENDING;
#else
                     p_host->State = DNSc_STATE_RESOLVED;
                     status        = DNSc_STATUS_RESOLVED;
#endif
                 }
                 break;

            case DNSc_STATE_RX_RESP_IPv6:
                 if (re_tx == DEF_YES) {
                     p_host->State = DNSc_STATE_TX_REQ_IPv6;
                     status        = DNSc_STATUS_PENDING;

                 } else if (status != DNSc_STATUS_RESOLVED) {   /* If the resolution has failed, let try on another     */
                     p_host->State = DNSc_STATE_IF_SEL;         /* interface. It may be possible to reach the DNS       */
                     status        = DNSc_STATUS_PENDING;       /* server using another link.                           */

                 } else {
                     p_host->State = DNSc_STATE_RESOLVED;
                     status        = DNSc_STATUS_RESOLVED;
                 }
                 break;

            default:
                 status = DNSc_STATUS_FAILED;
                *p_err  = DNSc_ERR_FAULT;
                 goto exit;
        }
    }


exit:
    return (status);
}

