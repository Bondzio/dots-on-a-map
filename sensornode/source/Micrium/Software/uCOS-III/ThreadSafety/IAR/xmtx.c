/*
*********************************************************************************************************
*                                          uC/OS-II and uC/OS-III
*                                          The Real-Time Kernel
*
*
*                           (c) Copyright 2009-2010; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                      IAR DLIB MULTITHREADED SUPPORT
* 
*
* File      : xmtx.c
* By        : FT
*
* Description : Thread locking and unlocking implementation for the system and file interface
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/


#include  <yvals.h>
#include  <stdint.h>
#include  <os_dlib.h>


_STD_BEGIN

/*
*********************************************************************************************************
*                                    uC/OS-III DLIB SYSTEM LOCKS INTERFACE
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     SYSTEM LOCK INITIALIZATION
*
* Description: Initialize a system lock.
*
* Arguments  : p_lock    Pointer to the lock info object pointer.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void  __iar_system_Mtxinit (__iar_Rmtx *p_lock) 
{    
    OS_DLIB_LockCreate((void **)p_lock);
}


/*
*********************************************************************************************************
*                                      SYSTEM LOCK DELETE
*
* Description: Delete a system lock.
*
* Arguments  : p_lock    Pointer to the lock info object.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void __iar_system_Mtxdst(__iar_Rmtx *p_lock) 
{
    OS_DLIB_LockDel((void *)*p_lock);
}


/*
*********************************************************************************************************
*                                      SYSTEM LOCK PEND
*
* Description: Pend on a system lock.
*
* Arguments  : p_lock    Pointer to the lock info object.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void __iar_system_Mtxlock(__iar_Rmtx *p_lock) 
{
    OS_DLIB_LockPend((void *)*p_lock);
}


/*
*********************************************************************************************************
*                                      SYSTEM LOCK POST
*
* Description: Signal a system lock.
*
* Arguments  : p_lock    Pointer to the lock info object.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void __iar_system_Mtxunlock(__iar_Rmtx *p_lock) 
{
    OS_DLIB_LockPost((void *)*p_lock);
}


/*
*********************************************************************************************************
*                                      FILE LOCK INITIALIZATION
*
* Description: Initialize a file lock.
*
* Arguments  : p_lock    Pointer to the lock info object pointer.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void  __iar_file_Mtxinit (__iar_Rmtx *p_lock) 
{    
    OS_DLIB_LockCreate((void **)p_lock);
}


/*
*********************************************************************************************************
*                                        FILE LOCK DELETE
*
* Description: Delete a system lock.
*
* Arguments  : p_lock    Pointer to the lock info object.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void __iar_file_Mtxdst(__iar_Rmtx *p_lock) 
{
    OS_DLIB_LockDel((void *)*p_lock);
}


/*
*********************************************************************************************************
*                                      FILE LOCK PEND
*
* Description: Pend on a file lock.
*
* Arguments  : p_lock    Pointer to the lock info object.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void __iar_file_Mtxlock(__iar_Rmtx *p_lock) 
{
    OS_DLIB_LockPend((void *)*p_lock);
}


/*
*********************************************************************************************************
*                                        FILE LOCK POST
*
* Description: Signal in a file lock.
*
* Arguments  : p_lock    Pointer to the lock info object.
*
* Note(s)    : none.
*********************************************************************************************************
*/

void __iar_file_Mtxunlock(__iar_Rmtx *p_lock) 
{
    OS_DLIB_LockPost((void *)*p_lock);
}



/*
*********************************************************************************************************
*                                      GET CURRENT TLS POINTER
*
* Description : Get the current TLS pointer.
*
* Argument(s) : symbp    Pointer to the symbol in the segment.
*
* Return(s)   : Returns symbol address in the current TLS segment.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  _DLIB_TLS_MEMORY  *__iar_dlib_perthread_access (void  _DLIB_TLS_MEMORY  *symbp)
{
    void  _DLIB_TLS_MEMORY  *p_tls;
    uintptr_t                tls_start;
    uintptr_t                tls_offset;
    
    
    p_tls      = OS_DLIB_TLS_Get((void *)0);
    tls_start  = (uintptr_t)(p_tls);
    tls_offset = (uintptr_t)(__IAR_DLIB_PERTHREAD_SYMBOL_OFFSET(symbp));
    p_tls      = (void  _DLIB_TLS_MEMORY  *)(tls_start + tls_offset);

    return (p_tls);
}


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

_STD_END
