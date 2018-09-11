/*
*********************************************************************************************************
*                                            uC/USB-Device
*                                       The Embedded USB Stack
*
*                         (c) Copyright 2004-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/USB-Device is provided in source form to registered licensees ONLY.  It is
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
*                                USB AUDIO DEVICE OPERATING SYSTEM LAYER
*                                              Template
*
* File          : usbd_audio_os.c
* Version       : V4.05.00
* Programmer(s) : JFD
*                 CM
*********************************************************************************************************
*/

#include  "../../usbd_audio_internal.h"
#include  "../../usbd_audio_os.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL TABLES
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


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (USBD_AUDIO_CFG_RECORD_EN == DEF_ENABLED)
static  void  USBD_Audio_OS_RecordTask  (void  *p_arg);
#endif

#if (USBD_AUDIO_CFG_PLAYBACK_EN == DEF_ENABLED)
static  void  USBD_Audio_OS_PlaybackTask(void  *p_arg);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBD_Audio_OS_Init()
*
* Description : Initialize the audio class OS layer.
*
* Argument(s) : msg_qty     Maximum quantity of messages for playback and record tasks' queues.
*
*               p_err       Pointer to variable that will receive the return error code from this function:
*
*                           USBD_ERR_NONE               OS layer initialization successful.
*                           USBD_ERR_OS_INIT_FAIL       OS layer initialization failed.
*
* Return(s)   : None.
*
* Caller(s)   : USBD_Audio_ProcessingInit().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_Audio_OS_Init (CPU_INT16U   msg_qty,
                          USBD_ERR    *p_err)
{
    (void)&msg_qty;

   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBD_Audio_OS_AS_IF_LockCreate()
*
* Description : Create an OS resource to use as an AudioStreaming interface lock.
*
* Argument(s) : as_if_nbr   AudioStreaming interface index.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE               OS lock     successfully created.
*                               USBD_ERR_OS_SIGNAL_CREATE   OS lock NOT successfully created.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_Audio_AS_IF_Add().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void   USBD_Audio_OS_AS_IF_LockCreate (CPU_INT08U   as_if_nbr,
                                       USBD_ERR    *p_err)
{
    (void)&as_if_nbr;

   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBD_Audio_OS_AS_IF_LockAcquire()
*
* Description : Wait for an AudioStreaming interface to become available and acquire its lock.
*
* Argument(s) : as_if_nbr   AudioStreaming interface index.
*
*               timeout_ms  Lock wait timeout in milliseconds.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS lock     successfully acquired.
*                               USBD_ERR_OS_TIMEOUT     OS lock NOT successfully acquired in the time
*                                                           specified by 'timeout_ms'.
*                               USBD_ERR_OS_ABORT       OS lock aborted.
*                               USBD_ERR_OS_FAIL        OS lock not acquired because another error.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_Audio_AS_IF_Start(),
*               USBD_Audio_AS_IF_Stop(),
*               USBD_Audio_PlaybackTaskHandler(),
*               USBD_Audio_RecordTaskHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void   USBD_Audio_OS_AS_IF_LockAcquire (CPU_INT08U   as_if_nbr,
                                        CPU_INT16U   timeout_ms,
                                        USBD_ERR    *p_err)
{
    (void)&as_if_nbr;
    (void)&timeout_ms;

   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBD_Audio_OS_AS_IF_LockRelease()
*
* Description : Release an AudioStreaming interface lock.
*
* Argument(s) : as_if_nbr   AudioStreaming interface index.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_Audio_AS_IF_Start(),
*               USBD_Audio_AS_IF_Stop(),
*               USBD_Audio_PlaybackTaskHandler(),
*               USBD_Audio_RecordTaskHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void   USBD_Audio_OS_AS_IF_LockRelease (CPU_INT08U  as_if_nbr)
{
    (void)&as_if_nbr;
}


/*
*********************************************************************************************************
*                                  USBD_Audio_OS_RingBufQLockCreate()
*
* Description : Create an OS resource to use as a stream ring buffer queue lock.
*
* Argument(s) : as_if_nbr   AudioStreaming interface settings index.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE               OS lock     successfully created.
*                               USBD_ERR_OS_SIGNAL_CREATE   OS lock NOT successfully created.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_Audio_AS_IF_Cfg().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void   USBD_Audio_OS_RingBufQLockCreate (CPU_INT08U   as_if_settings_ix,
                                         USBD_ERR    *p_err)
{
    (void)&as_if_settings_ix;

   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                  USBD_Audio_OS_RingBufQLockAcquire()
*
* Description : Wait for a stream ring buffer queue to become available and acquire its lock.
*
* Argument(s) : as_if_nbr   AudioStreaming interface settings index.
*
*               timeout_ms  Lock wait timeout in milliseconds.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               USBD_ERR_NONE           OS lock     successfully acquired.
*                               USBD_ERR_OS_TIMEOUT     OS lock NOT successfully acquired in the time
*                                                       specified by 'timeout_ms'.
*                               USBD_ERR_OS_ABORT       OS lock aborted.
*                               USBD_ERR_OS_FAIL        OS lock not acquired because another error.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_Audio_PlaybackCodecBufSubmit(),
*               USBD_Audio_PlaybackIsocCmpl(),
*               USBD_Audio_RecordIsocCmpl(),
*               USBD_Audio_RecordPrime().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void   USBD_Audio_OS_RingBufQLockAcquire (CPU_INT08U   as_if_settings_ix,
                                          CPU_INT16U   timeout_ms,
                                          USBD_ERR    *p_err)
{
    (void)&as_if_settings_ix;
    (void)&timeout_ms;

   *p_err = USBD_ERR_NONE;
}


/*
*********************************************************************************************************
*                                  USBD_Audio_OS_RingBufQLockRelease()
*
* Description : Release a stream ring buffer queue lock.
*
* Argument(s) : as_if_nbr   AudioStreaming interface settings index.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_Audio_PlaybackCodecBufSubmit(),
*               USBD_Audio_PlaybackIsocCmpl(),
*               USBD_Audio_RecordIsocCmpl(),
*               USBD_Audio_RecordPrime().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void   USBD_Audio_OS_RingBufQLockRelease (CPU_INT08U  as_if_settings_ix)
{
    (void)&as_if_settings_ix;
}


/*
*********************************************************************************************************
*                                     USBD_Audio_OS_RecordReqPost()
*
* Description : Post a request into the record task's queue.
*
* Argument(s) : p_msg       Pointer to message.
*
*               p_err       Pointer to variable that will receive the return error code from this function:
*
*                           USBD_ERR_NONE       Placing buffer in queue successful.
*                           USBD_ERR_OS_FAIL    Failed to place item into the Record buffer queue.
*
* Return(s)   : None.
*
* Caller(s)   : USBD_Audio_RecordRxCmpl().
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBD_AUDIO_CFG_RECORD_EN == DEF_ENABLED)
void  USBD_Audio_OS_RecordReqPost (void      *p_msg,
                                   USBD_ERR  *p_err)
{
    (void)&p_msg;

   *p_err = USBD_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                     USBD_Audio_OS_RecordReqPend()
*
* Description : Pend on a request from the record task's queue.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function:
*
*                           USBD_ERR_NONE           Getting buffer in queue successful.
*                           USBD_ERR_OS_TIMEOUT     Timeout has elapsed.
*                           USBD_ERR_OS_ABORT       Getting buffer in queue aborted.
*                           USBD_ERR_OS_FAIL        Failed to get item from the record buffer queue.
*
* Return(s)   : Pointer to record request, if NO error(s).
*
*               Null pointer,              otherwise
*
* Caller(s)   : USBD_Audio_RecordTaskHandler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBD_AUDIO_CFG_RECORD_EN == DEF_ENABLED)
void  *USBD_Audio_OS_RecordReqPend (USBD_ERR  *p_err)
{
   *p_err = USBD_ERR_NONE;

    return ((void *)0);
}
#endif


/*
*********************************************************************************************************
*                                    USBD_Audio_OS_PlaybackReqPost()
*
* Description : Post a request to the playback's task queue.
*
* Argument(s) : p_msg       Pointer to message.
*
*               p_err       Pointer to variable that will receive the return error code from this function:
*
*                           USBD_ERR_NONE       Placing buffer in queue successful.
*                           USBD_ERR_OS_FAIL    Failed to place item into the playback buffer queue.
*
* Return(s)   : None.
*
* Caller(s)   : USBD_Audio_PlaybackTxCmpl().
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBD_AUDIO_CFG_PLAYBACK_EN == DEF_ENABLED)
void  USBD_Audio_OS_PlaybackReqPost (void      *p_msg,
                                     USBD_ERR  *p_err)
{
    (void)&p_msg;

   *p_err = USBD_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                    USBD_Audio_OS_PlaybackReqPend()
*
* Description : Pend on a request from the playback's task queue.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function:
*
*                           USBD_ERR_NONE           Getting buffer in queue successful.
*                           USBD_ERR_OS_TIMEOUT     Timeout has elapsed.
*                           USBD_ERR_OS_ABORT       Getting buffer in queue aborted.
*                           USBD_ERR_OS_FAIL        Failed to get item from the playback buffer queue.
*
* Return(s)   : Pointer to playback request, if NO error(s).
*
*               Null pointer,                otherwise.
*
* Caller(s)   : USBD_Audio_PlaybackTaskHandler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBD_AUDIO_CFG_PLAYBACK_EN == DEF_ENABLED)
void  *USBD_Audio_OS_PlaybackReqPend (USBD_ERR  *p_err)
{
    *p_err = USBD_ERR_NONE;

     return ((void *)0);
}
#endif


/*
*********************************************************************************************************
*                                         USBD_Audio_OS_DlyMs()
*
* Description : Delay a task for a certain time.
*
* Argument(s) : ms          Delay in milliseconds.
*
* Return(s)   : None.
*
* Caller(s)   : USBD_Audio_RecordTaskHandler().
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBD_Audio_OS_DlyMs (CPU_INT32U  ms)
{
    (void)&ms;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      USBD_Audio_OS_RecordTask()
*
* Description : OS-dependent shell task to process record data streams.
*
* Argument(s) : p_arg       Pointer to task initialization argument.
*
* Return(s)   : None.
*
* Caller(s)   : This is a task.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBD_AUDIO_CFG_RECORD_EN == DEF_ENABLED)
static  void  USBD_Audio_OS_RecordTask (void  *p_arg)
{
    (void)&p_arg;

    USBD_Audio_RecordTaskHandler();
}
#endif


/*
*********************************************************************************************************
*                                     USBD_Audio_OS_PlaybackTask()
*
* Description : OS-dependent shell task to process playback data streams.
*
* Argument(s) : p_arg       Pointer to task initialization argument.
*
* Return(s)   : None.
*
* Caller(s)   : This is a task.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (USBD_AUDIO_CFG_PLAYBACK_EN == DEF_ENABLED)
static  void  USBD_Audio_OS_PlaybackTask (void  *p_arg)
{
    (void)&p_arg;

    USBD_Audio_PlaybackTaskHandler();
}
#endif
