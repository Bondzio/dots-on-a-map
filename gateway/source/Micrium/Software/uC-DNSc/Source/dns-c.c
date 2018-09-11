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
*                                             DNS CLIENT
*
* Filename      : dns-c.c
* Version       : V2.00.01
* Programmer(s) : AA
*********************************************************************************************************
* Note(s)       : (1) This file implements a basic DNS client based on RFC #1035.  It provides the
*                     mechanism used to retrieve an IP address from a given host name.
*
*                 (2) Assumes the following versions (or more recent) of software modules are included
*                     in the project build :
*
*                     (a) uC/TCP-IP V3.00
*                     (b) uC/CPU    V1.30
*                     (c) uC/LIB    V1.37
*                     (d) uC/Common-KAL V1.00.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    DNSc_MODULE
#include  "dns-c.h"
#include  "dns-c_req.h"
#include  "dns-c_cache.h"
#include  "dns-c_task.h"
#include  <Source/net_ascii.h>


/*
*********************************************************************************************************
*                                             DNSc_Init()
*
* Description : Initialize DNSc module.
*
* Argument(s) : p_cfg       Pointer to DNSc's configuration.
*
*               p_task_cfg  Pointer to a structure that contains the task configuration of the Asynchronous task.
*                           If Asynchronous mode is disabled this pointer should be set to DEF_NULL.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               DNSc_ERR_NONE           Server address successfully set.
*                               DNSc_ERR_NULL_PTR       Invalid pointer.
*
*                               RETURNED BY DNScCache_Init():
*                                   See DNScCache_Init() for additional return error codes.
*
*                               RETURNED BY DNScTask_Init():
*                                   See DNScTask_Init() for additional return error codes.
*
*                               RETURNED BY DNScReq_ServerInit():
*                                   See DNScReq_ServerInit() for additional return error codes.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) DNSc_Init() MUST be called PRIOR to using other DNSc functions.
*********************************************************************************************************
*/

void  DNSc_Init (const  DNSc_CFG       *p_cfg,
                 const  DNSc_CFG_TASK  *p_task_cfg,
                        DNSc_ERR       *p_err)
{
#if (DNSc_CFG_ARG_CHK_EN == DEF_ENABELD)
    if (p_cfg == DEF_NULL) {
       *p_err = DNSc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    DNScCache_Init(p_cfg, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }

    DNScReq_ServerInit(p_cfg, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }

    DNScTask_Init(p_cfg, p_task_cfg, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }

exit:
    return;
}


/*
*********************************************************************************************************
*                                         DNSc_CfgServerByStr()
*
* Description : Configure DNS server that must be used by default using a string.
*
* Argument(s) : p_server    Pointer to a string that contains the IP address of the DNS server.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               DNSc_ERR_NONE           Server address successfully set.
*                               DNSc_ERR_NULL_PTR       Invalid pointer.
*
*                               RETURNED BY DNScCache_AddrObjSet():
*                                   See DNScCache_AddrObjSet() for additional return error codes.
*
*                               RETURNED BY DNScReq_ServerSet():
*                                   See DNScReq_ServerSet() for additional return error codes.

* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNSc_CfgServerByStr (CPU_CHAR  *p_server,
                           DNSc_ERR  *p_err)
{
    DNSc_ADDR_OBJ  ip_addr;


#if (DNSc_CFG_ARG_CHK_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_server == DEF_NULL) {
        *p_err = DNSc_ERR_NULL_PTR;
         goto exit;
    }
#endif

    DNScCache_AddrObjSet(&ip_addr, p_server, p_err);
    if (*p_err != DNSc_ERR_NONE) {
        goto exit;
    }


    DNScReq_ServerSet(&ip_addr, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


   *p_err = DNSc_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNSc_CfgServerByAddr()
*
* Description : Configure DNS server that must be used by default using an address structure.
*
* Argument(s) : p_addr  Pointer to structure that contains the IP address of the DNS server.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE           Server address successfully set.
*                           DNSc_ERR_NULL_PTR       Invalid pointer.
*                           DNSc_ERR_ADDR_INVALID   Invalid IP address.
*
*                           RETURNED BY DNScCache_AddrObjSet():
*                               See DNScCache_AddrObjSet() for additional return error codes.
*
*                           RETURNED BY DNScReq_ServerSet():
*                               See DNScReq_ServerSet() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNSc_CfgServerByAddr (DNSc_ADDR_OBJ  *p_addr,
                            DNSc_ERR       *p_err)
{
#if (DNSc_CFG_ARG_CHK_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_addr == DEF_NULL) {
       *p_err = DNSc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    switch (p_addr->Len) {
        case NET_IPv4_ADDR_SIZE:
        case NET_IPv6_ADDR_SIZE:
             break;

        default:
            *p_err = DNSc_ERR_ADDR_INVALID;
             goto exit;
    }

    DNScReq_ServerSet(p_addr, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }


   *p_err = DNSc_ERR_NONE;

exit:
    return;
}


/*
*********************************************************************************************************
*                                         DNSc_GetServerByStr()
*
* Description : Get DNS server in string format that is configured to be use by default.
*
* Argument(s) : p_addr      Pointer to structure that will receive the IP address of the DNS server.
*
*               str_len_max Maximum string length.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE               Server address successfully returned.
*                           DNSc_ERR_INVALID_ARG        Invalid argument
*                           DNSc_ERR_ADDR_INVALID       Invalid server address.
*                           DNSc_ERR_FAULT              Unknown error.
*
*                           RETURNED BY DNScReq_ServerGet():
*                               See DNScReq_ServerGet() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNSc_GetServerByStr (CPU_CHAR    *p_str,
                           CPU_INT08U   str_len_max,
                           DNSc_ERR    *p_err)
{
    DNSc_ADDR_OBJ  addr;
    NET_ERR        err;


#if (DNSc_CFG_ARG_CHK_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_str == DEF_NULL) {
       *p_err = DNSc_ERR_NULL_PTR;
        goto exit;
    }
#endif

    DNScReq_ServerGet(&addr, p_err);
    if (*p_err != DNSc_ERR_NONE) {
        goto exit;
    }

    switch (addr.Len) {
        case NET_IPv4_ADDR_LEN:
#ifdef  NET_IPv4_MODULE_EN
             if (str_len_max < NET_ASCII_LEN_MAX_ADDR_IPv4) {
                *p_err = DNSc_ERR_INVALID_ARG;
                 goto exit;
             }

             NetASCII_IPv4_to_Str(*(NET_IPv4_ADDR *)addr.Addr, p_str, DEF_NO, &err);
             if (err != NET_ASCII_ERR_NONE) {
                *p_err = DNSc_ERR_ADDR_INVALID;
                 goto exit;
             }
             break;
#else
            *p_err = DNSc_ERR_ADDR_INVALID;
             goto exit;

#endif

        case NET_IPv6_ADDR_LEN:
#ifdef  NET_IPv6_MODULE_EN
             if (str_len_max < NET_ASCII_LEN_MAX_ADDR_IPv6) {
                *p_err = DNSc_ERR_INVALID_ARG;
                 goto exit;
             }

             NetASCII_IPv6_to_Str((NET_IPv6_ADDR *)addr.Addr, p_str, DEF_NO, DEF_NO, &err);
             if (err != NET_ASCII_ERR_NONE) {
                *p_err = DNSc_ERR_ADDR_INVALID;
                 goto exit;
             }
             break;
#else
             *p_err = DNSc_ERR_ADDR_INVALID;
              goto exit;
#endif

        default:
            *p_err = DNSc_ERR_FAULT;
             goto exit;
    }


   *p_err = DNSc_ERR_NONE;


exit:
    return;
}


/*
*********************************************************************************************************
*                                        DNSc_CfgServerByAddr()
*
* Description : Get DNS server in address object that is configured to be use by default.
*
* Argument(s) : p_addr  Pointer to structure that will receive the IP address of the DNS server.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           DNSc_ERR_NONE           Server address successfully returned.
*
*                           RETURNED BY DNScReq_ServerGet():
*                               See DNScReq_ServerGet() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNSc_GetServerByAddr (DNSc_ADDR_OBJ  *p_addr,
                            DNSc_ERR       *p_err)
{
#if (DNSc_CFG_ARG_CHK_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_addr == DEF_NULL) {
       *p_err = DNSc_ERR_NULL_PTR;
        return;
    }
#endif

    DNScReq_ServerGet(p_addr, p_err);


    return;
}


/*
*********************************************************************************************************
*                                            DNSc_GetHost()
*
* Description : Convert string representation of a host name to its corresponding IP address using DNS
*               service.
*
* Argument(s) : p_host_name     Pointer to a string that contains the host name.
*
*               p_addrs         Pointer to arrays that will receive the IP address from this function.
*
*               p_addr_nbr      Pointer to a variable that contains how many address can be contained in the addresses
*                               array and that will receive the number of address copied in the addresses array
*
*               flags           DNS client flag:
*
*                                   DNSc_FLAG_NONE              By default this function is blocking.
*                                   DNSc_FLAG_NO_BLOCK          Don't block (only possible if DNSc's task is enabled).
*                                   DNSc_FLAG_FORCE_CACHE       Take host from the cache, don't send new DNS request.
*                                   DNSc_FLAG_FORCE_RENEW       Force DNS request, remove existing entry in the cache.
*                                   DNSc_FLAG_FORCE_RESOLUTION  Force DNS to resolve given host name.
*                                   DNSc_FLAG_IPv4_ONLY         Return only IPv4 address(es).
*                                   DNSc_FLAG_IPv6_ONLY         Return only IPv6 address(es).
*
*               p_cfg           Pointer to a request configuration. Should be set to overwrite default DNS configuration
*                               (such as DNS server, request timeout, etc.).
*                               Must be set to DEF_NULL to use default configuration.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   DNSc_ERR_NONE               Request successful issued or resolved.
*                                   DNSc_ERR_NULL_PTR           Invalid pointer.
*                                   DNSc_ERR_INVALID_ARG        Invalid argument.
*                                   DNSc_ERR_FAULT              Fault error.
*
*                                   RETURNED BY DNScCache_Srch():
*                                       See DNScCache_Srch() for additional return error codes.
*
*                                   RETURNED BY DNScCache_HostObjGet():
*                                       See DNScCache_HostObjGet() for additional return error codes.
*
*                                   RETURNED BY DNScTask_ProcessHostReq():
*                                       See DNScTask_ProcessHostReq() for additional return error codes.
*
* Return(s)   : Resolution status:
*                       DNSc_STATUS_PENDING         Host resolution is pending, call again to see the status. (Processed by DNSc's task)
*                       DNSc_STATUS_RESOLVED        Host is resolved.
*                       DNSc_STATUS_FAILED          Host resolution has failed.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

DNSc_STATUS  DNSc_GetHost (CPU_CHAR       *p_host_name,
                           DNSc_ADDR_OBJ  *p_addrs,
                           CPU_INT08U     *p_addr_nbr,
                           DNSc_FLAGS      flags,
                           DNSc_REQ_CFG   *p_cfg,
                           DNSc_ERR       *p_err)
{
    NET_IP_ADDR_FAMILY   ip_family;
    DNSc_STATUS          status   =  DNSc_STATUS_FAILED;
    CPU_BOOLEAN          flag_set =  DEF_NO;
    CPU_INT08U           addr_nbr = *p_addr_nbr;
    DNSc_HOST_OBJ       *p_host;
    NET_ERR              err;


                                                                /* ------------------ VALIDATE ARGS ------------------- */
#if (DNSc_CFG_ARG_CHK_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_host_name == DEF_NULL) {
        *p_err = DNSc_ERR_NULL_PTR;
         goto exit;
    }

    if (p_addrs == DEF_NULL) {
        *p_err = DNSc_ERR_NULL_PTR;
         goto exit;
    }

    if (addr_nbr_max <= 0u) {
        *p_err = DNSc_ERR_INVALID_ARG;
         goto exit;
    }

#ifndef  DNSc_TASK_MODULE_EN
    if (DEF_BIT_SET(flags, DNSc_FLAG_NO_BLOCK)) {
       *p_err = DNSc_ERR_INVALID_CFG;
        goto exit;
    } else {
#ifndef DNSc_SIGNAL_TASK_MODULE_EN
       *p_err = DNSc_ERR_INVALID_CFG;
        goto exit;
#endif
    }

    if (DEF_BIT_IS_SET(flags, DNSc_FLAG_FORCE_CACHE) &&
        DEF_BIT_IS_SET(flags, DNSc_FLAG_FORCE_RENEW)) {
       *p_err = DNSc_ERR_INVALID_CFG;
        goto exit;
    }
#endif
#endif

    flag_set = DEF_BIT_IS_SET(flags, DNSc_FLAG_FORCE_RESOLUTION);
    if (flag_set == DEF_NO) {
                                                                /* First check to see if the incoming host name is      */
                                                                /* simply a decimal-dot-formatted IP address. If it     */
                                                                /* is, then just convert it and return.                 */
        ip_family = NetASCII_Str_to_IP(p_host_name,
                                       p_addrs[0].Addr,
                                       sizeof(p_addrs[0].Addr),
                                      &err);
        if (err == NET_ASCII_ERR_NONE) {
            switch (ip_family) {
                case NET_IP_ADDR_FAMILY_IPv4:
                     p_addrs[0].Len = NET_IPv4_ADDR_LEN;
                     status         = DNSc_STATUS_RESOLVED;
                    *p_addr_nbr     = 1u;
                    *p_err          = DNSc_ERR_NONE;
                     goto exit;

                case NET_IP_ADDR_FAMILY_IPv6:
                     p_addrs[0].Len = NET_IPv6_ADDR_LEN;
                     status         = DNSc_STATUS_RESOLVED;
                    *p_addr_nbr     = 1u;
                    *p_err          = DNSc_ERR_NONE;
                     goto exit;

                default:
                    break;
            }
        }
    }

    flag_set = DEF_BIT_IS_SET(flags, DNSc_FLAG_FORCE_CACHE);
    if (flag_set == DEF_YES) {
        status = DNScCache_Srch(p_host_name, p_addrs, addr_nbr, p_addr_nbr, flags, p_err);
        goto exit;
    }


    flag_set = DEF_BIT_IS_SET(flags, DNSc_FLAG_FORCE_RENEW);
    if (flag_set == DEF_NO) {
                                                                /* ---------- SRCH IN EXISTING CACHE ENTRIES ---------- */
        status = DNScCache_Srch(p_host_name, p_addrs, addr_nbr, p_addr_nbr, flags, p_err);
        switch (status) {
            case DNSc_STATUS_PENDING:
            case DNSc_STATUS_RESOLVED:
                 goto exit;

            case DNSc_STATUS_FAILED:
                 break;

            default:
                *p_err = DNSc_ERR_FAULT;
                 goto exit;
        }

    } else {
        DNScCache_HostSrchRemove(p_host_name, p_err);
    }

                                                                /* ----------- ACQUIRE HOST OBJ FOR THE REQ ----------- */
    p_host = DNScCache_HostObjGet(p_host_name, flags, p_cfg, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         status = DNSc_STATUS_FAILED;
         goto exit;
    }


    status = DNScTask_HostResolve(p_host, flags, p_cfg, p_err);
    if (*p_err != DNSc_ERR_NONE) {
         goto exit;
    }

    switch (status) {
        case DNSc_STATUS_PENDING:
             goto exit;

        case DNSc_STATUS_RESOLVED:
        case DNSc_STATUS_UNKNOWN:
             break;

        case DNSc_STATUS_FAILED:
             goto exit_free_host;

        default:
            *p_err = DNSc_ERR_FAULT;
             goto exit;
    }


    status = DNScCache_Srch(p_host_name, p_addrs, addr_nbr, p_addr_nbr, flags, p_err);

    goto exit;


exit_free_host:
    DNScCache_HostObjFree(p_host);

exit:
    return (status);
}


/*
*********************************************************************************************************
*                                            DNSc_CacheClr()
*
* Description : Flush DNS cache.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RETURNED BY DNScCache_Clr():
*                               See DNScCache_Clr() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  DNSc_CacheClrAll (DNSc_ERR  *p_err)
{
    DNScCache_Clr(p_err);
}


/*
*********************************************************************************************************
*                                          DNSc_CacheClrHost()
*
* Description : Remove a host from the cache.
*
* Argument(s) : p_host_name Pointer to a string that contains the host name to remove from the cache.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               RETURNED BY DNScCache_HostSrchRemove():
*                                   See DNScCache_HostSrchRemove() for additional return error codes.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  DNSc_CacheClrHost (CPU_CHAR  *p_host_name,
                         DNSc_ERR  *p_err)
{
    DNScCache_HostSrchRemove(p_host_name, p_err);
}

