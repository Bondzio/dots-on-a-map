/*
************************************************************************************************************************
*                                                      uC/OS-III
*                                                 The Real-Time Kernel
*
*                                     (c) Copyright 2012; Micrium, Inc.; Weston, FL
*                           All rights reserved.  Protected by international copyright laws.
*
*                                                   Microchip OSAL 
*
* File    : OSAL_MICRIUM.C
* By      : JJL
* Version : V3.03.01
*
* LICENSING TERMS:
* ---------------
*           uC/OS-III is provided in source form for FREE short-term evaluation, for educational use or 
*           for peaceful research.  If you plan or intend to use uC/OS-III in a commercial application/
*           product then, you need to contact Micrium to properly license uC/OS-III for its use in your 
*           application/product.   We provide ALL the source code for your convenience and to help you 
*           experience uC/OS-III.  The fact that the source is provided does NOT mean that you can use 
*           it commercially without paying a licensing fee.
*
*           Knowledge of the source code may NOT be used to develop a similar product.
*
*           Please help us continue to provide the embedded community with the finest software available.
*           Your honesty is greatly appreciated.
*
*           You can contact us at www.micrium.com, or by phone at +1 (954) 217-2036.
************************************************************************************************************************
*/

#define  MICRIUM_SOURCE
#include <os_cfg_app.h>
#include <os.h>                                  /* uC/OS-III header file                                             */

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *osal_micrium__c = "$Id: $";
#endif


#include <osal/osal_ucos3.h>

/* 
------------------------------------------------------------------------------------------------------------------------
                                                     Local Variables
------------------------------------------------------------------------------------------------------------------------
*/

static  OS_MUTEX    OSAL_Malloc_Mutex;          /* Local mutex needed to guard malloc() and free() for non-reentrancy */


/* 
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_Initialize()
------------------------------------------------------------------------------------------------------------------------

  Function: OSAL_RESULT  OSAL_Initialize (void)

  Summary:
    Initialize the OSAL layer with uC/OS-III specifics

  Description:
    Initialize the mutex used to protect the heap.

  Precondition:
    None

  Parameters:
    None

  Returns:
    None

  Example:
    <code>
    OSAL_Initialize();
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_Initialize (void)   
{
    OS_ERR  err;
    

    OSInit(&err);
        
    OSMutexCreate((OS_MUTEX *)&OSAL_Malloc_Mutex,
                  (CPU_CHAR *)"OSAL Malloc/Free Mutex",
                  (OS_ERR   *)&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

    
/* 
------------------------------------------------------------------------------------------------------------------------
                                                       OSAL_Name()
------------------------------------------------------------------------------------------------------------------------

  Function: const  char *OSAL_Name (void)

  Summary:
    Get the name of the underlying kernel

  Description:
    This functions returns the name of the underlying RTOS kernel.

  Precondition:
    None

  Parameters:
    None

  Returns:
    None

  Example:
    <code>
    const  char *sName;

    sName = OSAL_Name();
    sprintf(buff, "RTOS Kernel: %s", sName);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/

const  char  *OSAL_Name (void)   
{
    return ("Micrium uC/OS-III");
}

    
/* 
------------------------------------------------------------------------------------------------------------------------
                                                       OSAL_Malloc()
------------------------------------------------------------------------------------------------------------------------

  Function: void *OSAL_Malloc (size_t size)

  Summary:
    Allocate dynamic storage

  Description:
    Get memory from heap

  Precondition:
    None

  Parameters:
    size         - The desired number of continuous bytes from the heap.

  Returns:
    a Pointer to the allocated storage or a NULL pointer if the request failed

  Example:
    <code>
    void *p_mem;
    
    
    p_mem = OSAL_Malloc(100);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/

void  *OSAL_Malloc (size_t  size)
{
    OS_ERR  err;
    CPU_TS  ts;
    void   *p_data;
    
    
    OSMutexPend((OS_MUTEX *)&OSAL_Malloc_Mutex,
                (OS_TICK   )0,
                (OS_OPT    )OS_OPT_PEND_BLOCKING,
                (CPU_TS   *)&ts,
                (OS_ERR   *)&err);

    p_data = malloc(size);

    OSMutexPost((OS_MUTEX *)&OSAL_Malloc_Mutex,
                (OS_OPT    )OS_OPT_POST_NONE,
                (OS_ERR   *)&err);

    return (p_data);
}


/* 
------------------------------------------------------------------------------------------------------------------------
                                                      OSAL_Free()
------------------------------------------------------------------------------------------------------------------------

  Function: void OSAL_Free (void *pData)

  Summary:
    Deallocate dynamic storage

  Description:
    Return memory back to heap

  Precondition:
    None

  Parameters:
    p_data        - A pointer to the block of storage to return back to the heap

  Returns:
    None

  Example:
    <code>
    OSAL_Free(p_mem);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
void  OSAL_Free (void *p_data)
{
    OS_ERR  err;
    CPU_TS  ts;
    
    
    OSMutexPend((OS_MUTEX *)&OSAL_Malloc_Mutex,
                (OS_TICK   )0,
                (OS_OPT    )OS_OPT_PEND_BLOCKING,
                (CPU_TS   *)&ts,
                (OS_ERR   *)&err);

    free(p_data);

    OSMutexPost((OS_MUTEX *)&OSAL_Malloc_Mutex,
                (OS_OPT    )OS_OPT_POST_NONE,
                (OS_ERR   *)&err);
}


/* 
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_SEM_Create()
------------------------------------------------------------------------------------------------------------------------

  Function: OSAL_RESULT OSAL_SEM_Create(OSAL_SEM_HANDLE_TYPE *semID, 
                                        OSAL_SEM_TYPE         type,
                                        uint8_t               maxCount, 
                                        uint8_t               initialCount)
  Summary:
    Create an OSAL Semaphore

  Description:
    Create an OSAL binary or counting semaphore. If OSAL_SEM_TYPE_BINARY is specified then
    the maxcount and initialCount values are ignored otherwise these must contain valid
    values.

  Precondition:
    Semaphore must have been declared.

  Parameters:
    semID        - Semaphore ID

    type         - OSAL_SEM_TYPE_BINARY,   create a binary semaphore
                 - OSAL_SEM_TYPE_COUNTING, create a counting semaphore with the specified
                   count values.

    maxCount     - Maximum value for a counting semaphore. Ignored for a BINARY semaphore.

    initialCount - Starting count value for the semaphore. Ignored for a BINARY semaphore

  Returns:
    OSAL_RESULT_TRUE    - Semaphore created
    OSAL_RESULT_FALSE   - Semaphore creation failed

  Example:
    <code>
    OSAL_SEM_Create(&mySemID, OSAL_SEM_TYPE_COUNTING, 10, 5);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/

OSAL_RESULT  OSAL_SEM_Create (OSAL_SEM_HANDLE_TYPE *semID, OSAL_SEM_TYPE type, uint8_t maxCount, uint8_t initialCount)
{
    OS_ERR  err;
    
    
    switch (type) {
        case OSAL_SEM_TYPE_BINARY:
             OSSemCreate((OS_SEM   *)semID,
                         (CPU_CHAR *)"Sem-Binary",
                         (OS_SEM_CTR)1,
                         (OS_ERR   *)&err);
             break;

        case OSAL_SEM_TYPE_COUNTING:
             OSSemCreate((OS_SEM   *)semID,
                         (CPU_CHAR *)"Sem-Counting",
                         (OS_SEM_CTR)initialCount,
                         (OS_ERR   *)&err);
             break;

        default:
             return (OSAL_RESULT_NOT_IMPLEMENTED);
    }
    
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_SEM_Delete()
------------------------------------------------------------------------------------------------------------------------
 
  Function: OSAL_RESULT OSAL_SEM_Delete (OSAL_SEM_HANDLE_TYPE *semID)

  Summary:
    Delete an OSAL Semaphore

  Description:
    Delete an OSAL semaphore freeing up any allocated storage associated with it.

  Precondition:
    Semaphore must have been created.

  Parameters:
    semID       - Pointer to the semID

  Returns:
    OSAL_RESULT_TRUE    - Semaphore deleted
    OSAL_RESULT_FALSE   - Semaphore deletion failed

  Example:
    <code>
     OSAL_SEM_Delete(&mySemID);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_SEM_Delete (OSAL_SEM_HANDLE_TYPE *semID)
{
    OS_ERR  err;
    
    
    OSSemDel((OS_SEM *)semID,
             (OS_OPT  )OS_OPT_DEL_ALWAYS,
             (OS_ERR *)&err);
             
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_SEM_Pend()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT OSAL_SEM_Pend(OSAL_SEM_HANDLE_TYPE semID, 
                                      uint16_t              waitMS)

  Summary:
     Pend on a semaphore. Returns true if semaphore obtained within time limit.

  Description:
     Blocking function call that pends (waits) on a semaphore. The function will
     return true is the semaphore has been obtained or false if it was not available
     or the time limit was exceeded.

  Precondition:
     Semaphore must have been created.

  Parameters:
     semID       - Pointer to the semID

    waitMS       - Time limit to wait in milliseconds.
                   0                 - do not wait
                   OSAL_WAIT_FOREVER - return only when semaphore is obtained
                   Other values      - timeout delay

  Returns:
    OSAL_RESULT_TRUE    - Semaphore obtained
    OSAL_RESULT_FALSE   - Semaphore not obtained or timeout occurred

  Example:
    <code>
    if (OSAL_SEM_Pend(&semUARTRX, 50) == OSAL_RESULT_TRUE)
    {
        // character available
        c = DRV_USART_ReadByte(drvID);
        ...
    }
    else
    {
        // character not available, resend prompt
        ...
    }
   </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_SEM_Pend (OSAL_SEM_HANDLE_TYPE  semID, uint16_t waitMS)
{
    OS_ERR   err;
    OS_TICK  wait;


    if (waitMS == 0) {                                      /* See if user selects NO WAIT                            */
        return (OSAL_RESULT_FALSE);                         /* ... yes, return immediately                            */
    }
    
    if (waitMS == OSAL_WAIT_FOREVER) {                      /* See if user selects 'wait for ever'                    */
        wait = 0;                                           /* ... yes, for uC/OS-III, need to specify 0              */
    } else {
        wait = (OS_TICK)waitMS * (OS_TICK)1000 / (OS_TICK)OS_CFG_TICK_RATE_HZ;
    }

    OSSemPend((OS_SEM *)semID, 
              (OS_TICK )wait,
              (OS_OPT  )OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
              (OS_ERR *)&err);
              
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_SEM_Post()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT OSAL_SEM_Post(OSAL_SEM_HANDLE_TYPE *semID)

  Summary:
     Post a semaphore or increment a counting semaphore.

  Description:
     Post a binary semaphore or increment a counting semaphore. The highest
     priority task currently blocked on the semaphore will be released and
     made ready to run.

  Precondition:
     Semaphore must have been created.

  Parameters:
     semID              - Pointer to the semID

  Returns:
    OSAL_RESULT_TRUE    - Semaphore posted
    OSAL_RESULT_FALSE   - Semaphore not posted

  Example:
    <code>
    OSAL_SEM_Post(&semSignal);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_SEM_Post (OSAL_SEM_HANDLE_TYPE semID)
{
    OS_ERR  err;
    
    
    OSSemPost((OS_SEM *)semID,
              (OS_OPT  )OS_OPT_POST_1,
              (OS_ERR *)&err);

    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_SEM_PostISR()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT OSAL_SEM_PostISR(OSAL_SEM_HANDLE_TYPE *semID)

  Summary:
     Post a semaphore or increment a counting semaphore from within an ISR

  Description:
     Post a binary semaphore or increment a counting semaphore. The highest
     priority task currently blocked on the semaphore will be released and
     made ready to run. This form of the post function should be used inside
    an ISR.

  Precondition:
     Semaphore must have been created.

  Parameters:
     semID       - Pointer to the semID

  Returns:
    OSAL_RESULT_TRUE    - Semaphore posted
    OSAL_RESULT_FALSE   - Semaphore not posted

  Example:
    <code>
     void __ISR(UART_2_VECTOR) _UART2RXHandler()
     {
        char c;
        // read the character
        c = U2RXREG;
        // clear the interrupt flag
        IFS1bits.U2IF = 0;
        // post a semaphore indicating a character has been received
        OSAL_SEM_PostISR(&semSignal);
     }
    </code>

  Remarks:
     This version of the OSAL_SEM_Post function should be used if the program
     is, or may be, operating inside and ISR. The OSAL will take the necessary
     steps to ensure correct operation possibly disabling interrupts or entering
     a critical section. The exact requirements will depend upon the particular
     RTOS being used.
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_SEM_PostISR (OSAL_SEM_HANDLE_TYPE semID)
{
    OS_ERR  err;
    
    
    OSSemPost((OS_SEM *)semID,
              (OS_OPT  )OS_OPT_POST_1,
              (OS_ERR *)&err);

    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_SEM_GetCount()
------------------------------------------------------------------------------------------------------------------------
  Function: uint8_t OSAL_SEM_GetCount(OSAL_SEM_HANDLE_TYPE *semID)

  Summary:
    Return the current value of a counting semaphore.

  Description:
    Return the current value of a counting semaphore. The value returned is
    assumed to be a single vlaue ranging from 0-255.

  Precondition:
     Semaphore must have been created.

  Parameters:
     semID       - Pointer to the semID

  Returns:
    0           - Semaphore is unavailable
    1-255       - Current value of the counting semaphore

  Example:
    <code>
     uint8_t semCount;

     semCount = OSAL_SEM_GetCount(&semUART);

     if (semCount > 0)
     {
        // obtain the semaphore
         if (OSAL_SEM_Pend(&semUART) == OSAL_RESULT_TRUE)
         {
            // perform processing on the comm channel
            ...
         }
     }
     else
     {
        // no comm channels available
        ...
     }
    </code>

  Remarks:
     This version of the OSAL_SEM_Post() function should be used if the program
     is, or may be, operating inside and ISR. The OSAL will take the necessary
     steps to ensure correct operation possibly disabling interrupts or entering
     a critical section. The exact requirements will depend upon the particular
     RTOS being used.
------------------------------------------------------------------------------------------------------------------------
*/
uint8_t  OSAL_SEM_GetCount (OSAL_SEM_HANDLE_TYPE  semID)
{
    OS_SEM_CTR  ctr;
    CPU_SR_ALLOC();
    
    
    CPU_CRITICAL_ENTER();
    ctr = semID->Ctr;
    CPU_CRITICAL_EXIT();
    return ((uint8_t)ctr);
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_CRIT_Enter()
------------------------------------------------------------------------------------------------------------------------
  Function: void OSAL_CRIT_Enter(OSAL_CRIT_TYPE severity)

  Summary:
    Enter a critical section with the specified severity level.

  Description:
     The program is entering a critical section of code. It is assumed that the
     sequence of operations bounded by the enter and leave critical section operations
     is treated as one atomic sequence that will not be disturbed.

  Precondition:
    None

  Parameters:
    severity      - OSAL_CRIT_TYPE_LOW, The RTOS should disable all other running
                    tasks effectively locking the scheduling mechanism.
                  - OSAL_CRIT_TYPE_HIGH, The RTOS should disable all possible
                    interrupts sources including the scheduler ensuring that the
                    sequence of code operates without interruption.

  Returns:
    None

  Example:
    <code>
     // prevent other tasks pre-empting this sequence of code
     OSAL_CRIT_Enter(OSAL_CRIT_TYPE_LOW);
     // modify the peripheral
     DRV_USART_Reinitialize( objUSART,  &initData);
     OSAL_CRIT_Leave(OSAL_CRIT_TYPE_LOW);
    </code>

  Remarks:
    The sequence of operations bounded by the OSAL_CRIT_Enter() and OSAL_CRIT_Leave()
    form a critical section. The severity level defines whether the RTOS should
    perform task locking or completely disable all interrupts.
------------------------------------------------------------------------------------------------------------------------
*/
void  OSAL_CRIT_Enter (OSAL_CRIT_TYPE  severity)
{
    OS_ERR  err;
    CPU_SR_ALLOC();
    
    
    switch (severity) {
        case OSAL_CRIT_TYPE_LOW:
             OSSchedLock(&err);
             (void)err;
             break;

        case OSAL_CRIT_TYPE_HIGH:
             CPU_CRITICAL_ENTER();
             break;
    }
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_CRIT_Exit()
------------------------------------------------------------------------------------------------------------------------
  Function: void OSAL_CRIT_Leave (OSAL_CRIT_TYPE severity)

  Summary:
    Leave a critical section with the specified severity level.

  Description:
    The program is leaving a critical section of code. It is assumed that the
    sequence of operations bounded by the enter and leave critical section operations
    is treated as one atomic sequence that will not be disturbed.
    The severity should match the severity level used in the corresponding
    OSAL_CRIT_Enter call to ensure that the RTOS carries out the correct action.

  Precondition:
    None

  Parameters:
    severity      - OSAL_CRIT_TYPE_LOW, The RTOS should disable all other running
                    tasks effectively locking the scheduling mechanism.
                  - OSAL_CRIT_TYPE_HIGH, The RTOS should disable all possible
                    interrupts sources including the scheduler ensuring that the
                    sequence of code operates without interruption.

  Returns:
    None

  Example:
    <code>
     // prevent other tasks pre-empting this sequence of code
     OSAL_CRIT_Enter(OSAL_CRIT_TYPE_LOW);
     // modify the peripheral
     DRV_USART_Reinitialize( objUSART,  &initData);
     OSAL_CRIT_Leave(OSAL_CRIT_TYPE_LOW);
    </code>

  Remarks:
    The sequence of operations bounded by the OSAL_CRIT_Enter and OSAL_CRIT_Leave
    form a critical section. The severity level defines whether the RTOS should
    perform task locking or completely disable all interrupts.
------------------------------------------------------------------------------------------------------------------------
*/
void  OSAL_CRIT_Leave (OSAL_CRIT_TYPE  severity)
{
    OS_ERR  err;
    CPU_SR_ALLOC();
    
    
    switch (severity) {
        case OSAL_CRIT_TYPE_LOW:
             OSSchedUnlock(&err);
             (void)err;
             break;

        case OSAL_CRIT_TYPE_HIGH:
             CPU_CRITICAL_EXIT();
             break;
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_MUTEX_Create()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT OSAL_MUTEX_Create (OSAL_MUTEX_HANDLE_TYPE *mutexID)

  Summary:
    Create a mutex.

  Description:
    This function creates a mutex, allocating storage if required and placing
    the mutex handle into the passed parameter.

  Precondition:
    None.

  Parameters:
    mutexID      - Pointer to the mutex handle

  Returns:
    OSAL_RESULT_TRUE    - Mutex successfully created

    OSAL_RESULT_FALSE   - Mutex failed to be created.

  Example:
    <code>
    OSAL_MUTEX_HANDLE_TYPE mutexData;

    OSAL_MUTEX_Create(&mutexData);
    ...
     if (OSAL_MUTEX_Lock(&mutexData, 1000) == OSAL_RESULT_TRUE)
     {
        // manipulate the shared data
        ...
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_MUTEX_Create (OSAL_MUTEX_HANDLE_TYPE  *mutexID)
{
    OS_ERR  err;
    
    
    OSMutexCreate((OS_MUTEX *)mutexID,
                  (CPU_CHAR *)"Mutex",
                  (OS_ERR   *)&err);

    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_MUTEX_Delete()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_MUTEX_Delete(OSAL_MUTEX_HANDLE_TYPE *mutexID)

  Summary:
    Delete a mutex.

  Description:
    This function deletes a mutex and frees associated storage if required.

  Precondition:
    None.

  Parameters:
    mutexID             - Pointer to the mutex handle

  Returns:
    OSAL_RESULT_TRUE    - Mutex successfully deleted.

    OSAL_RESULT_FALSE   - Mutex failed to be deleted.

  Example:
    <code>
    OSAL_MUTEX_Delete(&mutexData);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_MUTEX_Delete (OSAL_MUTEX_HANDLE_TYPE  *mutexID)
{
    OS_ERR  err;
    
    
    OSMutexDel((OS_MUTEX *)mutexID,
               (OS_OPT    )OS_OPT_DEL_ALWAYS,
               (OS_ERR   *)&err);

    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}



/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_MUTEX_Lock()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_MUTEX_Lock (OSAL_MUTEX_HANDLE_TYPE *mutexID, 
                                          uint16_t                waitMS)

  Summary:
    Lock a mutex.

  Description:
    This function locks a mutex, waiting for the specified timeout. If it cannot
    be obtained or the timeout period elapses then false is returned;

  Precondition:
    None.

  Parameters:
    mutexID      - Pointer to the mutex handle

    waitMS       - Timeout value in milliseconds,
                   0                 - do not wait, return immediately
                   OSAL_WAIT_FOREVER - wait until mutex is obtained before returning
                   Other values      - Timeout delay

  Returns:
    OSAL_RESULT_TRUE    - Mutex successfully obtained.

    OSAL_RESULT_FALSE   - Mutex failed to be obtained or timeout occurred.

  Example:
    <code>
    OSAL_MUTEX_HANDLE_TYPE mutexData;

    OSAL_MUTEX_Create(&mutexData);
    ...
     if (OSAL_MUTEX_Lock(&mutexData, 1000) == OSAL_RESULT_TRUE)
     {
        // manipulate the shared data
        ...

        // unlock the mutex
        OSAL_MUTEX_Unlock(&mutexData);
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_MUTEX_Lock (OSAL_MUTEX_HANDLE_TYPE  mutexID, uint16_t  waitMS)
{
    OS_ERR   err;
    OS_TICK  wait;


    if (waitMS == 0) {                                      /* See if user selects NO WAIT                            */
        return (OSAL_RESULT_FALSE);                         /* ... yes, return immediately                            */
    }
    
    if (waitMS == OSAL_WAIT_FOREVER) {                      /* See if user selects 'wait for ever'                    */
        wait = 0;                                           /* ... yes, for uC/OS-III, need to specify 0              */
    } else {
        wait = (OS_TICK)waitMS * (OS_TICK)1000 / (OS_TICK)OS_CFG_TICK_RATE_HZ;
    }
    
    OSMutexPend((OS_MUTEX *)mutexID, 
                (OS_TICK   )wait,
                (OS_OPT    )OS_OPT_PEND_BLOCKING,
                (CPU_TS   *)0,
                (OS_ERR   *)&err);
              
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_MUTEX_Unlock()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_MUTEX_Unlock (OSAL_MUTEX_HANDLE_TYPE *mutexID)

  Summary:
    Unlock a mutex.

  Description:
    This function unlocks a previously obtained mutex.

  Precondition:
    None.

  Parameters:
    mutexID             - Pointer to the mutex handle

  Returns:
    OSAL_RESULT_TRUE    - Mutex released.

    OSAL_RESULT_FALSE   - Mutex failed to be released or error occurred.

  Example:
    <code>
    OSAL_MUTEX_HANDLE_TYPE mutexData;

    OSAL_MUTEX_Create(&mutexData);
    ...
     if (OSAL_MUTEX_Lock(&mutexData, 1000) == OSAL_RESULT_TRUE)
     {
        // manipulate the shared data
        ...

        // unlock the mutex
        OSAL_MUTEX_Unlock(&mutexData);
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
 OSAL_RESULT  OSAL_MUTEX_Unlock (OSAL_MUTEX_HANDLE_TYPE  mutexID)
{
    OS_ERR  err;
    
    
    OSMutexPost((OS_MUTEX *)mutexID,
                (OS_OPT    )OS_OPT_POST_1,
                (OS_ERR   *)&err);

    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_QUEUE_Create()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_Create (OSAL_QUEUE_HANDLE_TYPE  *p_q,
                                            uint16_t                 width, 
                                            uint16_t                 depth)

  Summary:
    Create a queue with the specified width and depth.

  Description:
    This functions allocates storage and creates an OSAL queue entity.
    Each entry in the queue will be 'width' bytes in size and the queue can
    contain at most 'depth' items.

  Precondition:
    None.

  Parameters:
    p_q                 - Pointer to the OSAL queue.

    width               - Width in bytes of each item in the queue.

    depth               - Maximum number of entries that can be stored in the queue.

  Returns:
    OSAL_RESULT_TRUE    - Queue created.

    OSAL_RESULT_FALSE   - Queue failed to be created.

  Example:
    <code>
    // declare a queue variable
     OSAL_QUEUE_Declare(serialQueue);

     // create a queue that contains 16bit integers and is 10 items long
     OSAL_QUEUE_Create(&serialQueue, sizeof(uint16_t), 10);
    </code>

  Remarks:
    The function creates an OSAL_QUEUE with the specified parameters. OSAL_QUEUEs
    are assumed to work on copy  with passed data being copied into the queue
    or other temporary storage so that the passed data storage is released.
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_Create (OSAL_QUEUE_HANDLE_TYPE  *p_q, uint16_t width, uint16_t depth)
{
    CPU_INT08U  *p_mem;
    OS_SEM       err;
    OSAL_Q      *p_queue;
    CPU_SR_ALLOC();

    
    p_mem = (CPU_INT08U *)OSAL_Malloc(width * depth);  /* Allocate storage to hold queue data                         */
    if (p_mem == NULL) {
        return (OSAL_RESULT_FALSE);
    }
    
    p_queue = (OSAL_Q *)OSAL_Malloc(sizeof(OSAL_Q));   /* Allocate storage to hold the queue object                   */
    if (p_queue == NULL) {
        OSAL_Free(p_mem);
        return OSAL_RESULT_FALSE;
    }

    *p_q = p_queue;
        
    CPU_CRITICAL_ENTER();                              /* Initalize the queue's data structure                        */
    p_queue->QBaseAddrPtr = p_mem;
    p_queue->QEndAddrPtr  = p_mem + (width * depth);
    p_queue->QEntries     = 0;
    p_queue->QEntriesMax  = (CPU_INT16U)depth;
    p_queue->QInPtr       = p_mem;
    p_queue->QOutPtr      = p_mem;
    CPU_CRITICAL_EXIT();

    OSSemCreate((OS_SEM   *)&(p_queue->QPostSem),      /* Init. semaphore used to block tasks posting on full queue   */
                (CPU_CHAR *)"OSAL Queue - Post", 
                (OS_SEM_CTR)depth, 
                (OS_ERR   *)&err);
    OSSemCreate((OS_SEM   *)&(p_queue->QPendSem),      /* Init. semaphore used to block tasks pending on empty queue  */
                (CPU_CHAR *)"OSAL Queue - Pend", 
                (OS_SEM_CTR)0, 
                (OS_ERR   *)&err);

    return (OSAL_RESULT_TRUE);
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_QUEUE_Delete()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_Delete (OSAL_QUEUE_HANDLE_TYPE *p_q)

  Summary:
    Delete an OSAL_QUEUE.

  Description:
    Delete an OSAL_QUEUE pointed to be the passed argument. Associated storage
    is released upon completion.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q                         - Pointer to the OSAL_QUEUE object handle.

  Returns:
    OSAL_RESULT_TRUE            - Queue deleted

    OSAL_RESULT_FALSE           - Queue failed to be deleted.

    OSAL_RESULT_NOT_IMPLEMENTED - This function has not been implemented

  Example:
    <code>
    // declare a queue variable
     OSAL_QUEUE_Declare(serialQueue);

     // create a queue that contains 16bit integers and is 10 items long
     OSAL_QUEUE_Create(&serialQueue, sizeof(uint16_t), 10);

     // application is terminating, delete queue object
     OSAL_QUEUE_Delete(&serialQueue);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_Delete (OSAL_QUEUE_HANDLE_TYPE  *p_q)
{
    OS_ERR  err;
    OSAL_Q *p_queue;


    p_queue = *p_q;

    OSSemDel((OS_SEM *)&(p_queue->QPendSem),           /* Release the two semaphores                                  */
             (OS_OPT  )OS_OPT_DEL_ALWAYS,
             (OS_ERR *)&err);
    (void)&err;

    OSSemDel((OS_SEM *)&(p_queue->QPostSem),
             (OS_OPT  )OS_OPT_DEL_ALWAYS,
             (OS_ERR *)&err);
    (void)&err;

    OSAL_Free(p_queue->QBaseAddrPtr);                  /* Free the queue buffer                                       */
    
    OSAL_Free(p_queue);                                /* Free the queue object                                       */

    *p_q = NULL;

    return (OSAL_RESULT_TRUE);
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_QUEUE_Add()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_Add (OSAL_QUEUE_HANDLE_TYPE  *p_q, 
                                         uint8_t                 *pdata,
                                         uint16_t                 waitMS)

  Summary:
    Add the specified item to the queue with a timeout if the queue is full.

  Description:
    Add the specified item to the queue by copying it. If the queue has space
    the function will return immediately. If there is no space on the queue
    then wait up to the specified time before returning.
    The item is added to the tail of the queue.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q             - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the data to be copied to the queue. The data
                      item must be the same size as that specified when
                      creating the queue.

    waitMS          - Time to wait whilst trying to write to a full queue
                      before timing out.

  Returns:
    OSAL_RESULT_TRUE    - Item successfully added to the queue

    OSAL_RESULT_FALSE   - Item could not be written to the queue or a timeout occurred.

  Example:
    <code>
    // create a queue containing 16 byte items, queue is 5 items long.
    OSAL_QUEUE_Create(&msgQueue, 16, 5);

    ...
    // define a message (Note \0 at end is allowed for)
    const char msg[16] = "Hello World....";

    // write data to the queue. Wait until space is available
    // Note in this case that the \0 at the end of msg is also copied
    OSAL_QUEUE_Add(&msgQueue, (uint8_t*) msg, OSAL_WAIT_FOREVER);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_Add (OSAL_QUEUE_HANDLE_TYPE  p_q, uint8_t *pdata, uint16_t  waitMS)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    OS_TICK      wait;
    CPU_INT16U   i;
    OS_ERR       err;
    CPU_SR_ALLOC();


    if (waitMS == 0) {                                      /* See if user selects NO WAIT                            */
        return (OSAL_RESULT_FALSE);                         /* ... yes, return immediately                            */
    }
    
    if (waitMS == OSAL_WAIT_FOREVER) {                      /* See if user selects 'wait for ever'                    */
        wait = 0;                                           /* ... yes, for uC/OS-III, need to specify 0              */
    } else {
        wait = (OS_TICK)waitMS * (OS_TICK)1000 / (OS_TICK)OS_CFG_TICK_RATE_HZ;
    }

    OSSemPend((OS_SEM *)&p_q->QPostSem,                     /* Wait for available storage in the queue                */
              (OS_TICK )wait,
              (OS_OPT  )OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
              (OS_ERR *)&err);

    CPU_CRITICAL_ENTER();
    p_src  = (CPU_INT08U *)pdata;
    p_dest = (CPU_INT08U *)p_q->QInPtr;                     /* Copy data into the queue                               */
    for (i = 0; i < p_q->QEntrySize; i++) {
        *p_dest++ = *p_src++;
    }  
    p_q->QEntries++;
    p_q->QInPtr = (void *)((CPU_INT08U *)p_q->QInPtr + p_q->QEntrySize);                         
    if (p_q->QInPtr == p_q->QEndAddrPtr) {                  /* See if we need to wrap                                 */
        p_q->QInPtr =  p_q->QBaseAddrPtr;
    }
    CPU_CRITICAL_EXIT();

    OSSemPost((OS_SEM *)&p_q->QPendSem,                     /* Signal waiting task                                    */
              (OS_OPT  )OS_OPT_POST_NONE,
              (OS_ERR *)&err);

    return (OSAL_RESULT_TRUE);
}



/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_QUEUE_AddISR()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_AddISR (OSAL_QUEUE_HANDLE_TYPE  *p_q, 
                                            uint8_t                 *pdata)

  Summary:
    Add the specified item to the queue from with an ISR.

  Description:
    Add the specified item to the queue by copying it. The item is added to the
    tail of the queue.
    This version of Add should be used within an ISR.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q             - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the data to be copied to the queue. The data
                      item must be the same size as that specified when
                      creating the queue.


  Returns:
    OSAL_RESULT_TRUE    - Item successfully to the queue

    OSAL_RESULT_FALSE   - Item could not be added to the queue.

  Example:
    <code>
     void __ISR(UART_2_VECTOR) _UART2RXHandler()
     {
        char c;
        // read the character
        c = U2RXREG;
        // clear the interrupt flag
        IFS1bits.U2IF = 0;

        // post the received character onto a queue for later processing
        OSAL_QUEUE_AddISR(&myRXQueue, &c);
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_AddISR (OSAL_QUEUE_HANDLE_TYPE  p_q, uint8_t *pdata)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    CPU_INT16U   i;
    OS_ERR       err;
    CPU_SR_ALLOC();


    OSSemPend((OS_SEM *)&p_q->QPostSem,                     /* Check for available storage in the queue               */
              (OS_TICK )0,
              (OS_OPT  )OS_OPT_PEND_NON_BLOCKING,
              (CPU_TS *)0,
              (OS_ERR *)&err);
    if (err != OS_ERR_NONE) {                               /* An error indicates a full queue                        */
        return (OSAL_RESULT_FALSE);
    }

    CPU_CRITICAL_ENTER();
    p_src  = (CPU_INT08U *)pdata;
    p_dest = (CPU_INT08U *)p_q->QInPtr;                     /* Copy data into the queue                               */
    for (i = 0; i < p_q->QEntrySize; i++) {
        *p_dest++ = *p_src++;
    }  
    p_q->QEntries++;
    p_q->QInPtr = (void *)((CPU_INT08U *)p_q->QInPtr + p_q->QEntrySize);                         
    if (p_q->QInPtr == p_q->QEndAddrPtr) {                  /* See if we need to wrap                                 */
        p_q->QInPtr =  p_q->QBaseAddrPtr;
    }
    CPU_CRITICAL_EXIT();

    OSSemPost((OS_SEM *)&p_q->QPendSem,                     /* Signal waiting task                                    */
              (OS_OPT  )OS_OPT_POST_NONE,
              (OS_ERR *)&err);

    return (OSAL_RESULT_TRUE);
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_QUEUE_AddHead()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_AddHead (OSAL_QUEUE_HANDLE_TYPE  *p_q, 
                                             uint8_t                 *pdata,
                                             uint16_t                 waitMS)

  Summary:
    Add the specified item to the front of the queue with a timeout if the queue is full.

  Description:
    Add the specified item to the front of the queue by copying it. If the
    queue has space the function will return immediately. If there is no
    space on the queue then wait up to the specified time before returning.
    The item is added to the front of the queue.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q             - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the data to be copied to the queue. The data
                      item must be the same size as that specified when
                      creating the queue.

    waitMS          - Time to wait whilst trying to write to a full queue
                      before timing out.

  Returns:
    OSAL_RESULT_TRUE    - Item successfully added to the queue

    OSAL_RESULT_FALSE   - Item could not be written to the queue or a timeout occurred.

    OSAL_RESULT_NOT_IMPLEMENTED - Function not implemented in the current RTOS

  Example:
    <code>
    // create a queue containing byte items, queue is 5 items long.
    OSAL_QUEUE_Create(&msgQueue, sizeof(uint8_t), 5);

    ...
    // Send an important message
    uint8_t sigMsg = 42;

    // write data to the front of the queue.
    OSAL_QUEUE_AddHead(&msgQueue, &sigMsg, OSAL_WAIT_FOREVER);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_AddHead (OSAL_QUEUE_HANDLE_TYPE  p_q, uint8_t *pdata, uint16_t waitMS)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    OS_TICK      wait;
    CPU_INT16U   i;
    OS_ERR       err;
    CPU_SR_ALLOC();


    if (waitMS == 0) {                                      /* See if user selects NO WAIT                            */
        return (OSAL_RESULT_FALSE);                         /* ... yes, return immediately                            */
    }
    
    if (waitMS == OSAL_WAIT_FOREVER) {                      /* See if user selects 'wait for ever'                    */
        wait = 0;                                           /* ... yes, for uC/OS-III, need to specify 0              */
    } else {
        wait = (OS_TICK)waitMS * (OS_TICK)1000 / (OS_TICK)OS_CFG_TICK_RATE_HZ;
    }

    OSSemPend((OS_SEM *)&p_q->QPostSem,                     /* Wait for available storage in the queue                */
              (OS_TICK )wait,
              (OS_OPT  )OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
              (OS_ERR *)&err);

    CPU_CRITICAL_ENTER();
    if (p_q->QOutPtr == p_q->QBaseAddrPtr) {                /* See if we need to wrap                                 */
        p_q->QOutPtr =  p_q->QEndAddrPtr;
    } else {
        p_q->QOutPtr = (void *)((CPU_INT08U *)p_q->QOutPtr - p_q->QEntrySize);                         
    }
    p_src  = (CPU_INT08U *)pdata;
    p_dest = (CPU_INT08U *)p_q->QOutPtr;                    /* Copy data into the queue                               */
    for (i = 0; i < p_q->QEntrySize; i++) {
        *p_dest++ = *p_src++;
    }  
    p_q->QEntries++;
    CPU_CRITICAL_EXIT();

    OSSemPost((OS_SEM *)&p_q->QPendSem,                     /* Signal waiting task                                    */
              (OS_OPT  )OS_OPT_POST_NONE,
              (OS_ERR *)&err);

    return (OSAL_RESULT_TRUE);
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_QUEUE_AddHeadISR()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_AddHeadISR (OSAL_QUEUE_HANDLE_TYPE *p_q, 
                                                uint8_t                *pdata)

  Summary:
    Add the specified item to the front of the queue from within an ISR.

  Description:
    Add the specified item to the queue by copying it. The item is added to the
    front of the queue.
    This version of Add should be used within an ISR.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q             - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the data to be copied to the queue. The data
                      item must be the same size as that specified when
                      creating the queue.

  Returns:
    OSAL_RESULT_TRUE    - Item successfully added to the queue

    OSAL_RESULT_FALSE   - Item could not be written to the queue.

    OSAL_RESULT_NOT_IMPLEMENTED - Function not implemented in the current RTOS

  Example:
    <code>
     void __ISR(EXTERNAL_VECTOR) _ExternalIRQHandler()
     {
        const uint8_t msg = EMERGENCY_STOP;

        // clear the interrupt flag
        IFS1bit.EIF = 0;

        // send an emergency message to any listening task
        OSAL_QUEUE_AddHeadISR(&myRXQueue, &msg);
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_AddHeadISR (OSAL_QUEUE_HANDLE_TYPE  p_q, uint8_t  *pdata)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    CPU_INT16U   i;
    OS_ERR       err;
    CPU_SR_ALLOC();


    OSSemPend((OS_SEM *)&p_q->QPostSem,                     /* Wait for available storage in the queue                */
              (OS_TICK )0,
              (OS_OPT  )OS_OPT_PEND_NON_BLOCKING,
              (CPU_TS *)0,
              (OS_ERR *)&err);
    if (err != OS_ERR_NONE) {                               /* Queue is full if we return with an error               */
        return (OSAL_RESULT_FALSE);
    }

    CPU_CRITICAL_ENTER();
    if (p_q->QOutPtr == p_q->QBaseAddrPtr) {                /* See if we need to wrap                                 */
        p_q->QOutPtr =  p_q->QEndAddrPtr;
    } else {
        p_q->QOutPtr = (void *)((CPU_INT08U *)p_q->QOutPtr - p_q->QEntrySize);                         
    }
    p_src  = (CPU_INT08U *)pdata;
    p_dest = (CPU_INT08U *)p_q->QOutPtr;                    /* Copy data into the queue                               */
    for (i = 0; i < p_q->QEntrySize; i++) {
        *p_dest++ = *p_src++;
    }  
    p_q->QEntries++;
    CPU_CRITICAL_EXIT();

    OSSemPost((OS_SEM *)&p_q->QPendSem,                     /* Signal waiting task                                    */
              (OS_OPT  )OS_OPT_POST_NONE,
              (OS_ERR *)&err);

    return (OSAL_RESULT_TRUE);
}


/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_QUEUE_Remove()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_Remove (OSAL_QUEUE_HANDLE_TYPE *p_q, 
                                            uint8_t                *pdata,
                                            uint16_t                waitMS)

  Summary:
    Remove the item at the head of the queue. If the queue is empty then
    it will block up to waitMS.

  Description:
    Remove the item at the head of the queue. Item is copied to the location pointed
    to by pdata. The user should ensure that there is sufficient space to receive
    the copied data. If no data is available then the function will block for at
    most waitMS before returning.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q             - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the location to receive data copied from the queue. The data
                      item must be the same size as that specified when creating the queue.

    waitMS          - Maximum time to wait before returning if the queue is empty
                      0                 - return immediately
                      OSAL_WAIT_FOREVER - wait until data is in the queue
                      Others            - maximum blocking time in milliseconds

  Returns:
    OSAL_RESULT_TRUE    - Item successfully removed from the queue

    OSAL_RESULT_FALSE   - Nothing removed from the queue or timeout occurred


  Example:
    <code>
    char rxChar;
    // wait up to one second to process the next character
    if (OSAL_QUEUE_Remove(&rxQueue, &rxChar, 1000) == OSAL_RESULT_TRUE)
     {
        // process the data
        ...
     }
     else
     {
        // nothing received in the last one second so perform other processing
        ...
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_Remove (OSAL_QUEUE_HANDLE_TYPE  p_q,  uint8_t *pdata, uint16_t waitMS)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    OS_TICK      wait;
    CPU_INT16U   i;
    OS_ERR       err;
    CPU_SR_ALLOC();


    if (waitMS == 0) {                                      /* See if user selects NO WAIT                            */
        return (OSAL_RESULT_FALSE);                         /* ... yes, return immediately                            */
    }
    
    if (waitMS == OSAL_WAIT_FOREVER) {                      /* See if user selects 'wait for ever'                    */
        wait = 0;                                           /* ... yes, for uC/OS-III, need to specify 0              */
    } else {
        wait = (OS_TICK)waitMS * (OS_TICK)1000 / (OS_TICK)OS_CFG_TICK_RATE_HZ;
    }
    
    OSSemPend((OS_SEM *)&(p_q->QPendSem),                   /* Wait for queue to contain a message                    */
              (OS_TICK )wait,
              (OS_OPT  )OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
              (OS_ERR *)&err);

    CPU_CRITICAL_ENTER();
    p_src   = (CPU_INT08U *)p_q->QOutPtr;                   /* Copy data from the queue                               */
    p_dest  = (CPU_INT08U *)pdata;
    for (i = 0; i < p_q->QEntrySize; i++) {
        *p_dest++ = *p_src++;
    }  
    p_q->QEntries--;
    p_q->QOutPtr = (void *)((CPU_INT08U *)p_q->QOutPtr + p_q->QEntrySize);                         
    if (p_q->QOutPtr == p_q->QEndAddrPtr) {                 /* See if we need to wrap                                 */
        p_q->QOutPtr =  p_q->QBaseAddrPtr;
    }
    CPU_CRITICAL_EXIT();

    OSSemPost((OS_SEM *)&(p_q->QPostSem),                   /* Indicate we have one more free entry in the queue      */
              (OS_OPT  )OS_OPT_POST_NONE,
              (OS_ERR *)&err);

    return (OSAL_RESULT_TRUE);
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_QUEUE_RemoveISR()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_RemoveISR (OSAL_QUEUE_HANDLE_TYPE *p_q, 
                                               uint8_t                *pdata)

  Summary:
    Remove the item at the head of the queue.

  Description:
    Remove the item at the head of the queue. Item is copied to the location pointed
    to by pdata. The user should ensure that there is sufficient space to receive
    the copied data.
    This version of the queue remove function should be used inside an ISR.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q             - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the location to receive data copied from the queue. The data
                      item must be the same size as that specified when creating the queue.

  Returns:
    OSAL_RESULT_TRUE    - Item successfully removed from the queue

    OSAL_RESULT_FALSE   - Nothing removed from the queue


  Example:
    <code>
    char rxChar;
     void __ISR(TIMER_1_VECTOR) _ET1IRQHandler()
     {
        const uint8_t msg;

        // clear the interrupt flag
        IFS0bits.T1IF = 0;

        // check for data to be sent from a queue
        if (OSAL_QUEUE_RemoveISR(&myTXQueue, &msg) == OSAL_RESULT_TRUE)
        {
           DRV_USART_WriteByte(uart_drv, msg);
        }
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_RemoveFromISR(OSAL_QUEUE_HANDLE_TYPE  p_q, uint8_t *pdata)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    CPU_INT16U   i;
    OS_ERR       err;
    CPU_SR_ALLOC();


    OSSemPend((OS_SEM *)&(p_q->QPendSem),                   /* See if queue contains any messages                     */
              (OS_TICK )0,
              (OS_OPT  )OS_OPT_PEND_NON_BLOCKING,
              (CPU_TS *)0,
              (OS_ERR *)&err);
    if (err != OS_ERR_NONE) {                               /* Queue is empty if we return with an error              */
        return (OSAL_RESULT_FALSE);
    }

    CPU_CRITICAL_ENTER();
    p_src   = (CPU_INT08U *)p_q->QOutPtr;                   /* Copy data from the queue                               */
    p_dest  = (CPU_INT08U *)pdata;
    for (i = 0; i < p_q->QEntrySize; i++) {
        *p_dest++ = *p_src++;
    }  
    p_q->QEntries--;
    p_q->QOutPtr = (void *)((CPU_INT08U *)p_q->QOutPtr + p_q->QEntrySize);                         
    if (p_q->QOutPtr == p_q->QEndAddrPtr) {                 /* See if we need to wrap                                 */
        p_q->QOutPtr =  p_q->QBaseAddrPtr;
    }
    CPU_CRITICAL_EXIT();

    OSSemPost((OS_SEM *)&(p_q->QPostSem),                   /* Indicate we have one more free entry in the queue      */
              (OS_OPT  )OS_OPT_POST_NONE,
              (OS_ERR *)&err);

    return (OSAL_RESULT_TRUE);
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_QUEUE_Peek()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_Peek (OSAL_QUEUE_HANDLE_TYPE  *p_q, 
                                          uint8_t                 *pdata,
                                          uint16_t                 waitMS)

  Summary:
    Copy the item at the head of the queue but leave it on the queue. If the queue
    is empty then it will block up to waitMS.

  Description:
    Examine the queue and copy the item at the head leaving the item on the queue
    for the next access. If there is nothing on the queue then block until waitMS
    milliseconds have elapsed before returning or until something is placed on
    the queue.

  Precondition:
    Queue must have been created.

  Parameters:
    p_q             - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the location to receive data copied from the queue. The data
                      item must be the same size as that specified when creating the queue.

    waitMS          - Maximum time to wait before returning if the queue is empty
                      0                 - return immediately
                      OSAL_WAIT_FOREVER - wait until data is in the queue
                      Others            - maximum blocking time in milliseconds

  Returns:
    OSAL_RESULT_TRUE    - Item successfully copied from the queue

    OSAL_RESULT_FALSE   - Nothing copied from the queue or timeout occurred

    OSAL_RESULT_NOT_IMPLEMENTED - Operation not available on the current RTOS

  Example:
    <code>
    // copy filenames from a queue and process the data in the file
    // however if another filename is on the queue then abort and
    // restart the operation
    OSAL_QUEUE_Remove(&rxQueue, fileName, OSAL_WAITFOREVER);

    while (abort == false)
    {
        // read data from the file and process it
        ...

        // check for a new filename to process
        if (OSAL_QUEUE_Peek(&rxQueue, fileName, 0) == OSAL_RESULT_TRUE)
        {
           abort = true;
        }
    }

    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_Peek (OSAL_QUEUE_HANDLE_TYPE  p_q, uint8_t *pdata, uint16_t waitMS)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    OS_TICK      wait;
    CPU_INT16U   i;
    OS_ERR       err;
    CPU_SR_ALLOC();


    if (waitMS == 0) {                                      /* See if user selects NO WAIT                            */
        CPU_CRITICAL_ENTER();
        if (p_q->QEntries > 0) {                            /* Check for any entry in the queue                       */
            p_src   = (CPU_INT08U *)p_q->QOutPtr;           /* Copy data from the queue                               */
            p_dest  = (CPU_INT08U *)pdata;
            for (i = 0; i < p_q->QEntrySize; i++) {
                *p_dest++ = *p_src++;
            }  
            CPU_CRITICAL_EXIT();
            return (OSAL_RESULT_TRUE);                      /* Yes, entry in the queue                                */
        } else {
            CPU_CRITICAL_EXIT();
            return (OSAL_RESULT_FALSE);                     /* Nothing in the queue                                   */
        }
    }
    
    if (waitMS == OSAL_WAIT_FOREVER) {                      /* See if user selects 'wait for ever'                    */
        while (p_q->QEntries == 0) {
            OSTimeDly((OS_TICK )1,                          /* Allow other tasks to run before polling the queue      */
                      (OS_OPT  )OS_OPT_TIME_DLY,
                      (OS_ERR *)&err);
        }
        CPU_CRITICAL_ENTER();
        p_src   = (CPU_INT08U *)p_q->QOutPtr;               /* Copy data from the queue                               */
        p_dest  = (CPU_INT08U *)pdata;
        for (i = 0; i < p_q->QEntrySize; i++) {
            *p_dest++ = *p_src++;
        }  
        CPU_CRITICAL_EXIT();
        return (OSAL_RESULT_TRUE);
    } else {
        wait = (OS_TICK)waitMS * (OS_TICK)1000 / (OS_TICK)OS_CFG_TICK_RATE_HZ;
        while (p_q->QEntries == 0) {
            OSTimeDly((OS_TICK )1,                          /* Allow other tasks to run before polling the queue      */
                      (OS_OPT  )OS_OPT_TIME_DLY,
                      (OS_ERR *)&err);
            wait--;
            if (wait == 0) {
                return (OSAL_RESULT_FALSE);                 /* Queue not posted in time                               */
            }
        }
        CPU_CRITICAL_ENTER();
        p_src   = (CPU_INT08U *)p_q->QOutPtr;               /* Copy data from the queue                               */
        p_dest  = (CPU_INT08U *)pdata;
        for (i = 0; i < p_q->QEntrySize; i++) {
            *p_dest++ = *p_src++;
        }  
        CPU_CRITICAL_EXIT();
        return (OSAL_RESULT_TRUE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_QUEUE_PeekISR()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_QUEUE_PeekISR (OSAL_QUEUE_HANDLE_TYPE  *p_q, 
                                             uint8_t                 *pdata)

  Summary:
    Copy the item at the head of the queue but leave it on the queue.

  Description:
    Examine the queue and copy the item at the head leaving the item on the queue
    for the next access.
    This version should be called from inside an ISR.

  Precondition:
    Queue must have been created.

  Parameters:
    queueID         - Pointer to the OSAL_QUEUE handle.

    pdata           - Pointer to the location to receive data copied from the queue. The data
                      item must be the same size as that specified when creating the queue.

  Returns:
    OSAL_RESULT_TRUE            - Item successfully copied from the queue

    OSAL_RESULT_FALSE           - Nothing copied from the queue

    OSAL_RESULT_NOT_IMPLEMENTED - Operation not available on the current RTOS

  Example:
    <code>
     uint16_t data;

     OSAL_QUEUE_PeekISR(&myQueue, &data);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_QUEUE_PeekISR (OSAL_QUEUE_HANDLE_TYPE  p_q, uint8_t *pdata)
{
    CPU_INT08U  *p_src;
    CPU_INT08U  *p_dest;
    CPU_INT16U   i;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    if (p_q->QEntries > 0) {                                /* Check for any entry in the queue                       */
        p_src   = (CPU_INT08U *)p_q->QOutPtr;               /* Copy data from the queue                               */
        p_dest  = (CPU_INT08U *)pdata;
        for (i = 0; i < p_q->QEntrySize; i++) {
            *p_dest++ = *p_src++;
        }  
        CPU_CRITICAL_EXIT();
        return (OSAL_RESULT_TRUE);                          
    } else {
        CPU_CRITICAL_EXIT();
        return (OSAL_RESULT_FALSE);                         /* Nothing in the queue                                   */
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_THREAD_Create()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_Create (OSAL_THREAD_FUNCTION      task, 
                                             const char               *name,
                                             uint16_t                  stack_size, 
                                             uint8_t                   priority, 
                                             void                     *pData, 
                                             OSAL_THREAD_HANDLE_TYPE  *pHandle)

  Summary:
    Create an OSAL thread with the specified parameters.

  Description:
    This function creates an OSAL thread. The thread function is implemented
    as a simple task that will normally not return (i.e. a while(1) loop).
    The name is not used by the OSAL but may be used by the RTOS for diagnostic
    and reporting purposes.
    The thread is created with a fixed stack size that is allocated from one
    of the available memory pools or more normally from the heap. The priority
    of the thread is an eight bit quantity with higher values denoting threads
    of a higher priority.
    Optionally a parameter may be passed to the thread function that can be used
    to implement per instance behavior. For example, one LED flashing task could
    be created several times with the LED number passed as a parameter at creation
    time.
    A handle to the created thread is returned in pHandle.
    Threads may be created prior to the RTOS starting however no execution will
    occur until it is started. A created thread is immediately placed into
    the READY state and is able to run provided that it is the highest priority
    thread when all threads are evaluated by the kernel either during normal
    operation or at RTOS startup time.

  Precondition:
    None.

  Parameters:
    task                - Name of the function that implements the thread code

    name                - Textual name of the thread. Not used by the OSAL but may be
                          used by the RTOS for diagnostic and tracing purposes

    stack_size          - Size of the stack associated with a thread in bytes. The RTOS
                          will allocate this many bytes from any underlying memory pool
                          or the heap

    priority            - Priority of the thread from 1-255. Higher values indicate threads
                          with higher priorities. It is left down to the underlying RTOS
                          whether threads of the same priority are permitted or if threads
                          should be given a unique priority.

    pData               - Pointer to data passed to the thread. Optional, NULL may be passed.

    pHandle             - Pointer to a variable that can contain the created threads handle.


  Returns:
    OSAL_RESULT_TRUE    - Thread successfully created

    OSAL_RESULT_FALSE   - Thread not created or insufficient memory available.

  Example:
    <code>
    // create a thread passing a data structure as a parameter during creation
    OSAL_THREAD_Create(threadCheck, "CHECK", 500, OTH_APP_THREAD_PRIORITY + 2, (void*) &appData, NULL);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_Create (OSAL_THREAD_FUNCTION task, const char *name, uint16_t stack_size, uint8_t priority, void *pData, OSAL_THREAD_HANDLE_TYPE  *pHandle)
{
    OS_ERR        err;
    CPU_STK      *p_stk_base;   
    CPU_STK_SIZE  stack_depth;                                 /* convert stack size from bytes into words            */


    if (priority >= OSAL_MAX_PRIORITIES) {                     /* Only priorities from 1 to the config value are used */
        return (OSAL_RESULT_FALSE);
    }

    p_stk_base = (CPU_STK *)OSAL_Malloc(stack_size);           /* Allocate desired stack space                        */
    if (p_stk_base == (void *)0) {
        return (OSAL_RESULT_FALSE);
    }

    stack_depth = stack_size / sizeof(CPU_STK);
    
    OSTaskCreate((OS_TCB      *)pHandle,
                 (CPU_CHAR    *)name,
                 (OS_TASK_PTR  )task,
                 (void        *)pData,
                 (OS_PRIO      )(OSAL_MAX_PRIORITIES - priority),
                 (CPU_STK     *)p_stk_base,
                 (CPU_STK_SIZE )0,
                 (CPU_STK_SIZE )stack_depth,
                 (OS_MSG_QTY   )1,
                 (OS_TICK      )10,
                 (void        *)0,
                 (OS_OPT       )(OS_OPT_TASK_STK_CHK + OS_OPT_TASK_STK_CLR),
                 (OS_ERR      *)&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                                 OSAL_THREAD_Suspend()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_Suspend (OSAL_THREAD_HANDLE_TYPE *handle);

  Summary:
    Suspend a specified thread.

  Description:
    Suspend a specific thread.
    If NULL is passed then the RTOS will attempt to suspend the thread in which
    the call was made. The thread will then only resume when unsuspended by
    another part of the program with an appropriate resume call.

  Precondition:
    None.

  Parameters:
    handle     - Pointer to a variable that can contain the threads handle.
                 NULL may be passed indicating suspend the calling thread.


  Returns:
    OSAL_RESULT_TRUE    - Thread suspended

    OSAL_RESULT_FALSE   - Thread could not be suspended

  Example:
    <code>
    // suspend this thread
    OSAL_THREAD_Suspend(NULL);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_Suspend (OSAL_THREAD_HANDLE_TYPE handle)
{
    OS_ERR  err;


    OSTaskSuspend((OS_TCB *)handle,
                  (OS_ERR *)&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_SuspendAll()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_SuspendAll (void);

  Summary:
    Suspend all threads apart from the calling thread.

  Description:
     Suspend all threads. Interrupts are still enabled.
     The calling thread will continue executing at the next instruction.

  Precondition:
    None.

  Parameters:
    None.


  Returns:
    OSAL_RESULT_TRUE    - Threads suspended

    OSAL_RESULT_FALSE   - Threads could not be suspended

  Example:
    <code>
    // suspend all threads
    OSAL_THREAD_SuspendAll();

    // perform critical code sequence
    // NB a critical section may be a better choice here
    ...
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_SuspendAll (void)
{
    OS_ERR  err;


    OSSchedLock(&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_Resume()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_Resume (OSAL_THREAD_HANDLE_TYPE *handle);

  Summary:
    Resume a specified thread.

  Description:
    Resume a specific thread.

  Precondition:
    None.

  Parameters:
    handle     - Pointer to a variable that can contain the threads handle.

  Returns:
    OSAL_RESULT_TRUE    - Thread suspended

    OSAL_RESULT_FALSE   - Thread could not be suspended

  Example:
    <code>
    // suspend a thread until a later time
    OSAL_THREAD_Suspend(&thread1Handle);

    ...

    // resume the suspended thread
    OSAL_THREAD_RESUME(&thread1Handle);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_Resume (OSAL_THREAD_HANDLE_TYPE handle)
{
    OS_ERR  err;


    OSTaskResume((OS_TCB *)handle,
                 (OS_ERR *)&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_ResumeAll()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_ResumeAll(void);

  Summary:
    Resume all suspended threads.

  Description:
    Resume all suspended threads. Execution will continue with the highest
    priority thread that is ready to execute.

  Precondition:
    None.

  Parameters:
    None.

  Returns:
    OSAL_RESULT_TRUE    - Successful call.


  Example:
    <code>
    // resume all threads
    OSAL_THREAD_ResumeAll();
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_ResumeAll (void)
{
    OS_ERR  err;


    OSSchedUnlock(&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_PrioritySet()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_PrioritySet (OSAL_THREAD_HANDLE_TYPE  *handle,
                                                  uint8_t                   priority)

  Summary:
    Set the priority of a thread.

  Description:
    The priority of the thread with the specified handle is changed to the new
    value. If as a reult of the change it now has the highest priority and is
    not blocked it will start executing.

  Precondition:
    None.

  Parameters:
    handle     - Pointer to a variable that can contain the threads handle.
                 NULL indicates change the priority of the thread in which the
                 call was made.

    priority   - Priority to be assigned to the thread

  Returns:
    OSAL_RESULT_TRUE    - Priority changed.

    OSAL_RESULT_FALSE   - Priority not changed.


  Example:
    <code>
    // raise the priority of this thread to ensure it completes a complex
    // operation
    uint8_t currPriority;

    currPriority = OSAL_THREAD_GetPriority(NULL);
    OSAL_THREAD_PrioritySet(NULL, currPriority + 1);

    ... perform a time critical operation

    // restore the priority
    OSAL_THREAD_PrioritySet(NULL, currPriority);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_PrioritySet (OSAL_THREAD_HANDLE_TYPE  handle, uint8_t priority)
{
    OS_ERR  err;


    OSTaskChangePrio((OS_TCB  *)handle, 
                     (OS_PRIO  )(OSAL_MAX_PRIORITIES - priority),
                     (OS_ERR  *)&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_PriorityGet()
------------------------------------------------------------------------------------------------------------------------
  Function: uint8_t  OSAL_THREAD_PriorityGet (OSAL_THREAD_HANDLE_TYPE *handle)

  Summary:
    Get the priority of a thread.

  Description:
    The priority of the thread with the specified handle is returned. A handle
    value of NULL returns the priority of the thread making the call.

  Precondition:
    None.

  Parameters:
    handle     - Pointer to a variable that can contain the threads handle.
                 NULL indicates use the current thread.

  Returns:
    0-255      - The current priority of the thread. Higher values indicate
                 higher priority.

  Example:
    <code>
    // raise the priority of this thread to ensure it completes a complex
    // operation
    uint8_t currPriority;

    currPriority = OSAL_THREAD_GetPriority(NULL);
    OSAL_THREAD_PrioritySet(NULL, currPriority + 1);

    ... perform a time critical operation

    // restore the priority
    OSAL_THREAD_PrioritySet(NULL, currPriority);
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
uint8_t  OSAL_THREAD_PriorityGet (OSAL_THREAD_HANDLE_TYPE  handle)
{
    OS_PRIO  prio;
    OS_TCB  *p_tcb;
    CPU_SR_ALLOC();

    
    p_tcb = (OS_TCB *)handle;
    CPU_CRITICAL_ENTER();
    prio  = p_tcb->Prio;
    CPU_CRITICAL_EXIT();

    return ((uint8_t)prio);
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_Sleep()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_Sleep (uint16_t  waitMS)

  Summary:
    Put the currently executing thread to sleep.

  Description:
    Execution of the current thread is placed into a blocked state for the
    specified amount of time.
     The exact amount of time that the thread will sleep will depend upon
     the granularity of the underlying pre-emptive tick.

  Precondition:
    None.

  Parameters:
    waitMS     - Number of milliseconds thread will be blocked for.

  Returns:
    OSAL_RESULT_TRUE - Call succeeded

  Example:
    <code>
     while (1)
     {
        // sleep for 500ms
        OSAL_THREAD_Sleep(500);
        // toggle an LED
        DRV_IO_Toggle(drvHandle, LED_ID);
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_Sleep (uint16_t  waitMS)
{
    OS_ERR  err;


    OSTimeDlyHMSM((CPU_INT16U)0,
                  (CPU_INT16U)0,
                  (CPU_INT16U)0,
                  (CPU_INT32U)waitMS,
                  (OS_OPT    )OS_OPT_TIME_HMSM_STRICT,
                  (OS_ERR   *)&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_CreateDaemon()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_THREAD_CreateDaemon (OSAL_THREAD_FUNCTION task)

  Summary:
    Create a Daemon thread for use with multi-threaded ISP drivers.

  Description:
    This function will create a daemon or system thread designed to assist
    ISP driver modules with their operation.
    Some ISP drivers will incorporate state machines or other loop strucutres
    that must be periodically called for the driver to continue operating.
    For instance, a USART driver may have a state machine that manages the
    prioritization and transmission of data from multiple clients. This could
    work in addition to any processing that occurs when operating under
    interrupts so it will have a separate DRV_xxxx_Tasks routine (or similar)
    that must be periodically called.
    Since the prioritization and frequency of these calls is only known to
    the programmer when designing the end application the OSAL provides
    a lightweight thread mechanism that is primarily intended to be used
    internally by the ISP. The daemon threads have a fixed size stack
    (defined in sys_config.h) and operate at a fixed priority.

  Precondition:
    None.

  Parameters:
    task      - The name of the function that implements the daemon

  Returns:
    OSAL_RESULT_TRUE    - Thread successfully created

    OSAL_RESULT_FALSE   - Thread not created or insufficient memory available.

  Example:
    <code>
    // inside driver initialization routine
    ...
    OSAL_THREAD_CreateDaemon(DRV_USART_Tasks);

    </code>

  Remarks:
    The "remarks" section is always optional.  Use it to provide additional
    information such as restrictions, special cases, and related information.
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT  OSAL_THREAD_CreateDaemon (OSAL_THREAD_FUNCTION  task)
{
    OSAL_RESULT  result;


    result = OSAL_THREAD_Create(task,
                                "SYS",
                                OSAL_DAEMON_STACK_SIZE,
                                OSAL_DAEMON_PRIORITY,
                                (void *)0,
                                NULL);
    return (result);
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_ResumeAll()
------------------------------------------------------------------------------------------------------------------------
  Function: OSAL_RESULT  OSAL_Start()

  Summary:
    Start the underlying RTOS

  Description:
     The underlying RTOS in an application is started. The application will not
     normally return from this function call since the RTOS and interrupts will
     be responsible for all program operation. A return from this function is
     usually fatal and indicates that the program has failed in some way.

  Precondition:
    None.

  Parameters:
    None.

  Returns:
    OSAL_RESULT_FALSE - The OS terminated for some reason.

  Example:
    <code>
     int main()
     {
         OSAL_Initialize();

         App_Init();
         OSAL_Start();
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
OSAL_RESULT OSAL_Start (void)
{
    OS_ERR  err;


//    SYS_INT_Enable();                              /* We must ensure that interrupts are enabled for the RTOS to work */

    OSStart(&err);
    if (err == OS_ERR_NONE) {
        return (OSAL_RESULT_TRUE);
    } else {
        return (OSAL_RESULT_FALSE);
    }
}

/*
------------------------------------------------------------------------------------------------------------------------
                                               OSAL_THREAD_ResumeAll()
------------------------------------------------------------------------------------------------------------------------
  Function: void OSAL_ErrorCallback(OSAL_ERROR_CALLBACK_TYPE type)

  Summary:
    Lightweight error handling callback function for the RTOS.

  Description:
    The callback function will be executed whenever an error occurs within
    an underlying RTOS. The OSAL implements this function as a weak type
    allowing it to be overridden by the user.
    The type and severity of errors that can be encountered by an RTOS will
    vary according to the specific implementation so this only provides
    lightweight error handling. There is no expectation that the function
    should return and if this does happen there is no guarantee that the
    RTOS will continue to function properly. A user application could override
    this function and provide simple signalling of the error such as logging
    a message or flashing a light.

  Precondition:
    None.

  Parameters:
    type      - Type of error
                OSAL_ERROR_CALLBACK_DEFAULT         - Default error
                OSAL_ERROR_CALLBACK_OUT_OF_MEMORY   - Memory allocation error
                OSAL_ERROR_CALLBACK_STACK_OVERFLOW  - Stack overflow in a thread
                OSAL_ERROR_CALLBACK_ERROR           - Generic error

  Returns:
    None

  Example:
    <code>
    // application override of the callback
    void OSAL_ErrorCallback(OSAL_ERROR_CALLBACK_TYPE type)
     {
         // turn on the fault light
         LATAbits.LATA0 = 1;
         // do not return
         while (1)
         {
         }
     }
    </code>

  Remarks:
------------------------------------------------------------------------------------------------------------------------
*/
void  __attribute__((weak))  OSAL_ErrorCallback (OSAL_ERROR_CALLBACK_TYPE type)
{
    while (1) {
        Nop();                                   /* a non recoverable error occured so loop here                      */
    }
}


/*
*********************************************************************************************************
*                                       Tmr_Init()
*
* Description: This function should intialize the timers used by your application
*
* Arguments  : None
*
* Returns    : None
*********************************************************************************************************
*/

void  Tmr_Init (void)
{
    unsigned int comp_val;


    
    asm volatile("mtc0   $0,$9");                      /* Clear the count register                                    */
    
    comp_val      = BSP_TMR_RELOAD;                    /* Setup the period in the compare register                    */
    asm volatile("mtc0   %0,$11" : "+r"(comp_val));

    IFS0CLR       = _IFS0_CTIF_MASK;
    IPC0bits.CTIP = 3;
    IEC0SET       = _IEC0_CTIE_POSITION;
}

/*
*********************************************************************************************************
*                                  BSP_TickISR_Handler()
*
* Description :
*
* Arguments   : None
*********************************************************************************************************
*/

void  BSP_TickISR_Handler (void)
{
    unsigned int  period;
    unsigned int  old_period;


    
    asm volatile("mfc0   %0, $11" : "=r"(old_period)); /* Get the old compare time                                    */
    period  = BSP_TMR_RELOAD + old_period;
    asm volatile("mtc0   %0,$11" : "+r"(period));      /* set up the period in the compare reg                        */
    IFS0CLR = _IFS0_CTIF_POSITION;
}

/*
*********************************************************************************************************
*                                    BSP_Except_Handler()
*
* Description: This function offers a means of servicing exceptions that are not triggered by interrupts
*              or the syscall instruction.
*
* Arguments  : None
*
* Returns    : None
*********************************************************************************************************
*/

void  BSP_Except_Handler (void)
{
}

/*
*********************************************************************************************************
*                                    BSP_DefaultHandler()
*
* Description: If an interrupt occurs, and no application-specific code has been written for that
*              interrupt, this function will be invoked.
*
* Arguments  : None
*
* Returns    : None
*********************************************************************************************************
*/

void  BSP_DefaultHandler (void)
{
}

