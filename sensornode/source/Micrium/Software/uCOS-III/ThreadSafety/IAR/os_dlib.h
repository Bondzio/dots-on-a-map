/*
*********************************************************************************************************
*                                         uC/OS-II and uC/OS-III
*                                          The Real-Time Kernel
*
*
*                           (c) Copyright 2009-2010; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                     IAR DLIB MULTITHREADED SUPPORT
*
* File      : os_dlib.h
* By        : FT
*
* Toolchain : IAR EWARM V6.xx and higher
*********************************************************************************************************
*/


#ifndef  OS_DLIB_MODULE_PRESENT
#define  OS_DLIB_MODULE_PRESENT


#ifdef   OS_DLIB_GLOBALS
#define  OS_DLIB_EXT
#else
#define  OS_DLIB_EXT  extern
#endif


/*
*********************************************************************************************************
*                                       CONFIGURATION DEFAULTS
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  OS_DLIB_OPT_TLS                       0x8000u


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void   OS_DLIB_LockInit    (void);
void   OS_DLIB_LockCreate  (void **p_lock);
void   OS_DLIB_LockDel     (void  *p_lock);
void   OS_DLIB_LockPend    (void  *p_lock);
void   OS_DLIB_LockPost    (void  *p_lock);
void  *OS_DLIB_TLS_Get     (void  *p_thread);
void   OS_DLIB_TLS_Set     (void  *p_thread,
                            void  *p_tls);
void   OS_DLIB_TLS_Start   (void  *p_thread);
void   OS_DLIB_TLS_End     (void  *p_thread);


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif
