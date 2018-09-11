/*
*********************************************************************************************************
*                                             uC/FS V4
*                                     The Embedded File System
*
*                         (c) Copyright 2008-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/FS is provided in source form to registered licensees ONLY.  It is
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
*                                      FILE SYSTEM DEVICE DRIVER
*
*                                           LOGICAL DEVICE
*
* Filename : fs_dev_logical.c
* Version  : v4.07.00.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    FS_DEV_LOGICAL_MODULE
#include  <lib_ascii.h>
#include  <lib_mem.h>
#include  <fs.h>
#include  <fs_dev_logical.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                    LOGICAL DEVICE DATA DATA TYPE
*********************************************************************************************************
*/

typedef  struct  fs_dev_logical_data  FS_DEV_LOGICAL_DATA;
struct  fs_dev_logical_data {
    FS_QTY                         UnitNbr;

    FS_SEC_SIZE                    SecSize;
    FS_SEC_QTY                     Size;
    FS_DEV_LOGICAL_DEV_TBL_ENTRY  *DevTbl;
    CPU_SIZE_T                     DevTblSize;

#if (FS_CFG_CTR_STAT_EN == DEF_ENABLED)
    FS_CTR                         StatRdCtr;
    FS_CTR                         StatWrCtr;
#endif

    FS_DEV_LOGICAL_DATA           *NextPtr;
};


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

#define  FS_DEV_LOGICAL_NAME_LEN            7u

static  const  CPU_CHAR  FSDev_Logical_Name[] = "logical";


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  FS_DEV_LOGICAL_DATA  *FSDev_Logical_ListFreePtr;


/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                                            /* -- DEV INTERFACE FNCTS - */
static  const  CPU_CHAR      *FSDev_Logical_NameGet (void);                                 /* Get name of driver.      */

static  void                  FSDev_Logical_Init    (FS_ERR               *p_err);          /* Init driver.             */

static  void                  FSDev_Logical_Open    (FS_DEV               *p_dev,           /* Open device.             */
                                                     void                 *p_dev_cfg,
                                                     FS_ERR               *p_err);

static  void                  FSDev_Logical_Close   (FS_DEV               *p_dev);          /* Close device.            */

static  void                  FSDev_Logical_Rd      (FS_DEV               *p_dev,           /* Read from device.        */
                                                     void                 *p_dest,
                                                     FS_SEC_NBR            start,
                                                     FS_SEC_QTY            cnt,
                                                     FS_ERR               *p_err);

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
static  void                  FSDev_Logical_Wr      (FS_DEV               *p_dev,           /* Write to device.         */
                                                     void                 *p_src,
                                                     FS_SEC_NBR            start,
                                                     FS_SEC_QTY            cnt,
                                                     FS_ERR               *p_err);
#endif

static  void                  FSDev_Logical_Query   (FS_DEV               *p_dev,           /* Get device info.         */
                                                     FS_DEV_INFO          *p_info,
                                                     FS_ERR               *p_err);

static  void                  FSDev_Logical_IO_Ctrl (FS_DEV               *p_dev,           /* Perform dev I/O control. */
                                                     CPU_INT08U            opt,
                                                     void                 *p_data,
                                                     FS_ERR               *p_err);

                                                                                            /* ------ LOCAL FNCTS ----- */
static  void                  FSDev_Logical_DataFree(FS_DEV_LOGICAL_DATA  *p_logical_data); /* Free data.               */

static  FS_DEV_LOGICAL_DATA  *FSDev_Logical_DataGet (void);                                 /* Alloc & init data.       */

/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_API  FSDev_Logical = {
    FSDev_Logical_NameGet,
    FSDev_Logical_Init,
    FSDev_Logical_Open,
    FSDev_Logical_Close,
    FSDev_Logical_Rd,
#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
    FSDev_Logical_Wr,
#endif
    FSDev_Logical_Query,
    FSDev_Logical_IO_Ctrl
};


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DRIVER INTERFACE FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       FSDev_Logical_NameGet()
*
* Description : Return name of device driver.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to string which holds device driver name.
*
* Caller(s)   : various.
*
* Note(s)     : (1) The name MUST NOT include the ':' character.
*
*               (2) The name MUST be constant; each time this function is called, the same name MUST be
*                   returned.
*********************************************************************************************************
*/

static  const  CPU_CHAR  *FSDev_Logical_NameGet (void)
{
    return (FSDev_Logical_Name);
}


/*
*********************************************************************************************************
*                                        FSDev_Logical_Init()
*
* Description : Initialize the driver.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*               ----------  Argument validated by caller.
*
*                               FS_ERR_NONE    Device driver initialized successfully.
*
* Return(s)   : none.
*
* Caller(s)   : FS_DevDrvAdd().
*
* Note(s)     : (1) This function should initialize any structures, tables or variables that are common
*                   to all devices or are used to manage devices accessed with the driver.  This function
*                   SHOULD NOT initialize any devices; that will be done individually for each with
*                   device driver's 'Open()' function.
*********************************************************************************************************
*/

static  void  FSDev_Logical_Init (FS_ERR  *p_err)
{
    FSDev_Logical_UnitCtr     =  0u;
    FSDev_Logical_ListFreePtr = (FS_DEV_LOGICAL_DATA *)0;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        FSDev_Logical_Open()
*
* Description : Open (initialize) a device instance.
*
* Argument(s) : p_dev       Pointer to device to open.
*               ----------  Argument validated by caller.
*
*               p_dev_cfg   Pointer to device configuration.
*               ----------  Argument validated by caller.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               ----------  Argument validated by caller.
*
*                               FS_ERR_NONE                    Device opened successfully.
*                               FS_ERR_DEV_ALREADY_OPEN        Device is already opened.
*                               FS_ERR_DEV_INVALID_CFG         Device configuration specified invalid.
*                               FS_ERR_DEV_INVALID_LOW_FMT     Device needs to be low-level formatted.
*                               FS_ERR_DEV_INVALID_LOW_PARAMS  Device low-level device parameters invalid.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*                               FS_ERR_MEM_ALLOC               Memory could not be allocated.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_Open().
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should NEVER be
*                   called when a device is already open.
*
*               (2) Some drivers may need to track whether a device has been previously opened
*                   (indicating that the hardware has previously been initialized).
*
*               (3) This function will be called EVERY time the device is opened.
*
*               (4) See 'FSDev_Open() Notes #3'.
*********************************************************************************************************
*/

static  void  FSDev_Logical_Open (FS_DEV  *p_dev,
                                  void    *p_dev_cfg,
                                  FS_ERR  *p_err)
{
    FS_DEV_LOGICAL_DATA           *p_logical_data;
    FS_DEV_LOGICAL_CFG            *p_logical_cfg;
    FS_DEV_LOGICAL_DEV_TBL_ENTRY  *p_logical_entry;
    FS_DEV                        *p_dev_item;
    FS_DEV_INFO                    dev_info;
    FS_ERR                         dev_err;
    CPU_SIZE_T                     dev_ix;
    FS_SEC_SIZE                    sec_size;
    FS_SEC_QTY                     size;
    CPU_BOOLEAN                    valid;


                                                                /* ------------------- VALIDATE CFG ------------------- */
    if (p_dev_cfg == (void *)0) {                               /* Validate dev cfg.                                    */
        FS_TRACE_DBG(("FSDev_Logical_Open(): Logical dev cfg NULL ptr.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }

    p_logical_cfg = (FS_DEV_LOGICAL_CFG *)p_dev_cfg;

                                                                /* Validate tbl ptr.                                    */
    if (p_logical_cfg->DevTbl == (FS_DEV_LOGICAL_DEV_TBL_ENTRY *)0) {
        FS_TRACE_DBG(("FSDev_Logical_Open(): Dev tbl specified as NULL ptr.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }

    if (p_logical_cfg->DevTblSize < 2u) {                       /* Validate tbl ptr size.                               */
        FS_TRACE_DBG(("FSDev_Logical_Open(): Invalid dev tbl size: %d (< 2).\r\n", p_logical_cfg->DevTblSize));
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }


                                                                /* ----------------- VALIDATE DEV TBL ----------------- */
    dev_ix   = 0u;
    valid    = DEF_YES;
    sec_size = 0u;
    size     = 0u;
    while ((dev_ix <  p_logical_cfg->DevTblSize) &&
           (valid  == DEF_YES)) {

        p_logical_entry = &p_logical_cfg->DevTbl[dev_ix];
                                                                /* Validate dev name ptr.                               */
        if (p_logical_entry->DevNamePtr == (const  CPU_CHAR  *)0) {
            FS_TRACE_DBG(("FSDev_Logical_Open(): Dev tbl entry %d specified as NULL ptr.\r\n", dev_ix));
            valid = DEF_NO;

        } else {                                                /* Acquire dev ref.                                     */
            p_dev_item = FSDev_AcquireLockChk((CPU_CHAR *)p_logical_entry->DevNamePtr, &dev_err);
            if (p_dev_item == (FS_DEV *)0) {
                FSDev_Release(p_dev_item);
                FS_TRACE_DBG(("FSDev_Logical_Open(): Ref to dev tbl entry %d dev (\"%s\") could not be acquired.\r\n", dev_ix, p_logical_entry->DevNamePtr));
                valid = DEF_NO;

            } else {
                                                                /* Get dev info.                                        */
                FSDev_QueryLocked(p_dev_item, &dev_info, &dev_err);
                FSDev_Unlock(p_dev_item);

                if (dev_err != FS_ERR_NONE) {
                    FS_TRACE_DBG(("FSDev_Logical_Open(): Info for dev tbl entry %d dev (\"%s\") could not be gotten.\r\n", dev_ix, p_logical_entry->DevNamePtr));
                    valid = DEF_NO;

                } else {
                    if (sec_size == 0u) {
                        if (dev_info.SecSize == 0u) {
                            FS_TRACE_DBG(("FSDev_Logical_Open(): Dev tbl entry %d dev (\"%s\") has zero sec size.\r\n", dev_ix, p_logical_entry->DevNamePtr));
                            valid = DEF_NO;
                        } else {
                            sec_size = dev_info.SecSize;
                        }
                    } else {
                        if (dev_info.SecSize != sec_size) {
                            FS_TRACE_DBG(("FSDev_Logical_Open(): Dev tbl entry %d dev (\"%s\") has diff sec size: %d != %d\r\n", dev_ix, p_logical_entry->DevNamePtr, dev_info.SecSize, sec_size));
                            valid = DEF_NO;
                        }
                    }

                    FS_TRACE_DBG(("FSDev_Logical_Open(): Dev tbl entry %d dev info: Name    : \"%s\"\r\n", dev_ix, p_logical_entry->DevNamePtr));
                    FS_TRACE_DBG(("                                                Sec size: %d\r\n",     dev_info.SecSize));
                    FS_TRACE_DBG(("                                                Size    : %d\r\n",     dev_info.Size));
                    p_logical_entry->DevPtr  = p_dev_item;
                    p_logical_entry->SecSize = dev_info.SecSize;
                    p_logical_entry->Size    = dev_info.Size;
                    p_logical_entry->Start   = size;

                    size += dev_info.Size;
                }
            }
        }

        dev_ix++;
    }

    if (valid == DEF_NO) {                                      /* If err, release all devs.                            */
        while (dev_ix > 0u) {
            p_logical_entry = &p_logical_cfg->DevTbl[dev_ix - 1u];
            FSDev_Release(p_logical_entry->DevPtr);
            dev_ix--;
        }

       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }



                                                                /* --------------------- INIT UNIT -------------------- */
    p_logical_data = FSDev_Logical_DataGet();
    if (p_logical_data == (FS_DEV_LOGICAL_DATA *)0) {           /* If info could not be alloc'd ...                     */
        dev_ix = p_logical_cfg->DevTblSize;
        while (dev_ix > 0u) {                                   /* ... release all devs.                                */
            p_logical_entry = &p_logical_cfg->DevTbl[dev_ix - 1u];
            FSDev_Release(p_logical_entry->DevPtr);
            dev_ix--;
        }

       *p_err = FS_ERR_MEM_ALLOC;
        return;
    }

    p_logical_data->UnitNbr    =  p_dev->UnitNbr;
    p_logical_data->SecSize    =  sec_size;
    p_logical_data->Size       =  size;

    p_logical_data->DevTbl     =  p_logical_cfg->DevTbl;
    p_logical_data->DevTblSize =  p_logical_cfg->DevTblSize;

    p_dev->DataPtr             = (void *)p_logical_data;

    FS_TRACE_INFO(("LOGICAL DEV FOUND: Name    : \"log:%d:\"\r\n", p_logical_data->UnitNbr));
    FS_TRACE_INFO(("                   Sec Size: %d bytes\r\n",    p_logical_data->SecSize));
    FS_TRACE_INFO(("                   Size    : %d secs \r\n",    p_logical_data->Size));
    FS_TRACE_INFO(("                   # Devs  : %d      \r\n",    p_logical_data->DevTblSize));

    *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        FSDev_Logical_Close()
*
* Description : Close (uninitialize) a device instance.
*
* Argument(s) : p_dev       Pointer to device to close.
*               ----------  Argument validated by caller.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_Close().
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*
*               (2) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  FSDev_Logical_Close (FS_DEV  *p_dev)
{
    FS_DEV_LOGICAL_DATA           *p_logical_data;
    CPU_SIZE_T                     dev_ix;
    FS_DEV_LOGICAL_DEV_TBL_ENTRY  *p_logical_entry;


    p_logical_data = (FS_DEV_LOGICAL_DATA *)p_dev->DataPtr;

    dev_ix         = p_logical_data->DevTblSize;                /* Release all devs & clr dev tbl.                      */
    while (dev_ix > 0u) {
        p_logical_entry = &p_logical_data->DevTbl[dev_ix - 1u];
        FSDev_Release(p_logical_entry->DevPtr);
        p_logical_entry->DevPtr  = (FS_DEV *)0;
        p_logical_entry->SecSize =  0u;
        p_logical_entry->Size    =  0u;
        p_logical_entry->Start   =  0u;

        dev_ix--;
    }

    FSDev_Logical_DataFree(p_logical_data);
}


/*
*********************************************************************************************************
*                                         FSDev_Logical_Rd()
*
* Description : Read from a device & store data in buffer.
*
* Argument(s) : p_dev       Pointer to device to read from.
*               ----------  Argument validated by caller.
*
*               p_dest      Pointer to destination buffer.
*               ----------  Argument validated by caller.
*
*               start       Start sector of read.
*
*               cnt         Number of sectors to read.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               ----------  Argument validated by caller.
*
*                               FS_ERR_NONE                    Sector(s) read.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_RdLocked().
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*********************************************************************************************************
*/

static  void  FSDev_Logical_Rd (FS_DEV      *p_dev,
                                void        *p_dest,
                                FS_SEC_NBR   start,
                                FS_SEC_QTY   cnt,
                                FS_ERR      *p_err)
{
    FS_DEV_LOGICAL_DATA           *p_logical_data;
    FS_DEV_LOGICAL_DEV_TBL_ENTRY  *p_logical_tbl;
    CPU_INT08U                    *p_dest_08;
    CPU_SIZE_T                     dev_ix;
    FS_SEC_NBR                     start_entry;
    FS_SEC_QTY                     cnt_rd;
    FS_SEC_QTY                     cnt_rem;
    CPU_BOOLEAN                    locked;


    p_logical_data = (FS_DEV_LOGICAL_DATA *)p_dev->DataPtr;
    p_logical_tbl  =  p_logical_data->DevTbl;

                                                                /* ------------------ FIND START DEV ------------------ */
    dev_ix      = p_logical_data->DevTblSize;
    start_entry = p_logical_tbl[dev_ix - 1u].Start;
    while ((dev_ix      > 0u) &&                                /* While ix is valid             ...                    */
           (start_entry > start)) {                             /* ... & entry start sec is high ...                    */
        start_entry = p_logical_tbl[dev_ix - 1u].Start;         /* ... chk next tbl entry start sec.                    */

        if (start_entry > start) {
            dev_ix--;
        }
    }

    if (start_entry > start) {
       *p_err = FS_ERR_DEV_IO;
        return;
    } else {
        dev_ix -= 1u;

        if (start_entry + p_logical_tbl[dev_ix].Size < start) {
           *p_err = FS_ERR_DEV_IO;
            return;
        }
    }



                                                                /* ---------------------- RD DEV ---------------------- */
    p_dest_08 = (CPU_INT08U *)p_dest;
    cnt_rem   =  cnt;
    while (cnt_rem > 0u) {
                                                                /* Rd up to end of dev.                                 */
        cnt_rd = DEF_MIN(cnt_rem, p_logical_tbl[dev_ix].Size - (start - p_logical_tbl[dev_ix].Start));

        locked = FSDev_Lock(p_logical_tbl[dev_ix].DevPtr);      /* Lock dev                ...                          */
        if (locked == DEF_NO) {
           *p_err = FS_ERR_DEV_IO;
            return;
        }
        if (p_dev->State != FS_DEV_STATE_LOW_FMT_VALID) {       /* ... & confirm dev state ...                          */
            FSDev_Unlock(p_logical_tbl[dev_ix].DevPtr);
           *p_err = FS_ERR_DEV_IO;
            return;
        }
                                                                /* ... & rd dev            ...                          */
        FSDev_RdLocked(        p_logical_tbl[dev_ix].DevPtr,
                       (void *)p_dest_08,
                               start,
                               cnt_rd,
                               p_err);
        FSDev_Unlock(p_logical_tbl[dev_ix].DevPtr);             /* ... & unlock dev.                                    */

        if (*p_err != FS_ERR_NONE) {
            return;
        }

        cnt_rem -= cnt_rd;
        if (cnt_rem > 0u) {                                     /* If secs rem ...                                      */
            dev_ix++;                                           /* ... rd from next dev.                                */
            if (dev_ix >= p_logical_data->DevTblSize) {
               *p_err = FS_ERR_DEV_IO;
                return;
            }
            p_dest_08 += cnt_rd * p_logical_data->SecSize;
        }
    }

    FS_CTR_STAT_ADD(p_logical_data->StatRdCtr, cnt);

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         FSDev_Logical_Wr()
*
* Description : Write data to a device from a buffer.
*
* Argument(s) : p_dev       Pointer to device to write to.
*               ----------  Argument validated by caller.
*
*               p_src       Pointer to source buffer.
*               ----------  Argument validated by caller.
*
*               start       Start sector of write.
*
*               cnt         Number of sectors to write.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               ----------  Argument validated by caller.
*
*                               FS_ERR_NONE                    Sector(s) written.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_WrLocked().
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*********************************************************************************************************
*/

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
static  void  FSDev_Logical_Wr (FS_DEV      *p_dev,
                                void        *p_src,
                                FS_SEC_NBR   start,
                                FS_SEC_QTY   cnt,
                                FS_ERR      *p_err)
{
    FS_DEV_LOGICAL_DATA           *p_logical_data;
    FS_DEV_LOGICAL_DEV_TBL_ENTRY  *p_logical_tbl;
    CPU_INT08U                    *p_src_08;
    CPU_SIZE_T                     dev_ix;
    FS_SEC_NBR                     start_entry;
    FS_SEC_QTY                     cnt_wr;
    FS_SEC_QTY                     cnt_rem;
    CPU_BOOLEAN                    locked;


    p_logical_data = (FS_DEV_LOGICAL_DATA *)p_dev->DataPtr;
    p_logical_tbl  =  p_logical_data->DevTbl;

                                                                /* ------------------ FIND START DEV ------------------ */
    dev_ix      = p_logical_data->DevTblSize;
    start_entry = p_logical_tbl[dev_ix - 1u].Start;
    while ((dev_ix      > 0u) &&                                /* While ix is valid             ...                    */
           (start_entry > start)) {                             /* ... & entry start sec is high ...                    */
        start_entry = p_logical_tbl[dev_ix - 1u].Start;         /* ... chk next tbl entry start sec.                    */

        if (start_entry > start) {
            dev_ix--;
        }
    }

    if (start_entry > start) {
       *p_err = FS_ERR_DEV_IO;
        return;
    } else {
        dev_ix -= 1u;

        if (start_entry + p_logical_tbl[dev_ix].Size < start) {
           *p_err = FS_ERR_DEV_IO;
            return;
        }
    }



                                                                /* ---------------------- RD DEV ---------------------- */
    p_src_08 = (CPU_INT08U *)p_src;
    cnt_rem   =  cnt;
    while (cnt_rem > 0u) {
                                                                /* Wr up to end of dev.                                 */
        cnt_wr = DEF_MIN(cnt_rem, p_logical_tbl[dev_ix].Size - (start - p_logical_tbl[dev_ix].Start));

        locked = FSDev_Lock(p_logical_tbl[dev_ix].DevPtr);      /* Lock dev                ...                          */
        if (locked == DEF_NO) {
           *p_err = FS_ERR_DEV_IO;
            return;
        }
        if (p_dev->State != FS_DEV_STATE_LOW_FMT_VALID) {       /* ... & confirm dev state ...                          */
            FSDev_Unlock(p_logical_tbl[dev_ix].DevPtr);
           *p_err = FS_ERR_DEV_IO;
            return;
        }
                                                                /* ... & wr dev            ...                          */
        FSDev_WrLocked(        p_logical_tbl[dev_ix].DevPtr,
                       (void *)p_src_08,
                               start,
                               cnt_wr,
                               p_err);
        FSDev_Unlock(p_logical_tbl[dev_ix].DevPtr);             /* ... & unlock dev.                                    */

        if (*p_err != FS_ERR_NONE) {
            return;
        }

        cnt_rem -= cnt_wr;
        if (cnt_rem > 0u) {                                     /* If secs rem ...                                      */
            dev_ix++;                                           /* ... wr from next dev.                                */
            if (dev_ix >= p_logical_data->DevTblSize) {
               *p_err = FS_ERR_DEV_IO;
                return;
            }
            p_src_08 += cnt_wr * p_logical_data->SecSize;
        }
    }

    FS_CTR_STAT_ADD(p_logical_data->StatRdCtr, cnt);

   *p_err = FS_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                        FSDev_Logical_Query()
*
* Description : Get information about a device.
*
* Argument(s) : p_dev       Pointer to device to query.
*               ----------  Argument validated by caller.
*
*               p_info      Pointer to structure that will receive device information.
*               ----------  Argument validated by caller.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               ----------  Argument validated by caller.
*
*                               FS_ERR_NONE                    Device information obtained.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_Open(),
*               FSDev_Refresh(),
*               FSDev_QueryLocked().
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*********************************************************************************************************
*/

static  void  FSDev_Logical_Query (FS_DEV       *p_dev,
                                   FS_DEV_INFO  *p_info,
                                   FS_ERR       *p_err)
{
    FS_DEV_LOGICAL_DATA  *p_logical_data;


    p_logical_data  = (FS_DEV_LOGICAL_DATA *)p_dev->DataPtr;

    p_info->SecSize =  p_logical_data->SecSize;
    p_info->Size    =  p_logical_data->Size;
    p_info->Fixed   =  DEF_YES;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       FSDev_Logical_IO_Ctrl()
*
* Description : Perform device I/O control operation.
*
* Argument(s) : p_dev       Pointer to device to control.
*               ----------  Argument validated by caller.
*
*               opt         Control command.
*
*               p_data      Buffer which holds data to be used for operation.
*                              OR
*                           Buffer in which data will be stored as a result of operation.
*               ----------  Argument validated by caller.
*
*               p_err       Pointer to variable that will receive return the error code from this function :
*               ----------  Argument validated by caller.
*
*                               FS_ERR_NONE                    Control operation performed successfully.
*                               FS_ERR_DEV_INVALID_IO_CTRL     I/O control operation unknown to driver.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_NOT_OPEN            Device is not open.
*                               FS_ERR_DEV_NOT_PRESENT         Device is not present.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : (1) Tracking whether a device is open is not necessary, because this should ONLY be
*                   called when a device is open.
*
*               (2) Defined I/O control operations are :
*
*                   (a) FS_DEV_IO_CTRL_REFRESH           Refresh device.               [*]
*                   (b) FS_DEV_IO_CTRL_LOW_FMT           Low-level format device.      [*]
*                   (c) FS_DEV_IO_CTRL_LOW_MOUNT         Low-level mount device.       [*]
*                   (d) FS_DEV_IO_CTRL_LOW_UNMOUNT       Low-level unmount device.     [*]
*                   (e) FS_DEV_IO_CTRL_LOW_COMPACT       Low-level compact device.     [*]
*                   (f) FS_DEV_IO_CTRL_LOW_DEFRAG        Low-level defragment device.  [*]
*                   (g) FS_DEV_IO_CTRL_SEC_RELEASE       Release data in sector.       [*]
*                   (h) FS_DEV_IO_CTRL_PHY_RD            Read  physical device.        [*]
*                   (i) FS_DEV_IO_CTRL_PHY_WR            Write physical device.        [*]
*                   (j) FS_DEV_IO_CTRL_PHY_RD_PAGE       Read  physical device page.   [*]
*                   (k) FS_DEV_IO_CTRL_PHY_WR_PAGE       Write physical device page.   [*]
*                   (l) FS_DEV_IO_CTRL_PHY_ERASE_BLK     Erase physical device block.  [*]
*                   (m) FS_DEV_IO_CTRL_PHY_ERASE_CHIP    Erase physical device.        [*]
*
*                           [*] NOT SUPPORTED
*********************************************************************************************************
*/

static  void  FSDev_Logical_IO_Ctrl (FS_DEV      *p_dev,
                                     CPU_INT08U   opt,
                                     void        *p_data,
                                     FS_ERR      *p_err)
{
   (void)&p_dev;                                                /*lint --e{550} Suppress "Symbol not accessed".         */
   (void)&opt;
   (void)&p_data;


                                                                /* ------------------ PERFORM I/O CTL ----------------- */
   *p_err = FS_ERR_DEV_INVALID_IO_CTRL;
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
*                                      FSDev_Logical_DataFree()
*
* Description : Free a logical data object.
*
* Argument(s) : p_logical_data  Pointer to a logical data object.
*               ----------  Argument validated by caller.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_Logical_Close().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_Logical_DataFree (FS_DEV_LOGICAL_DATA  *p_logical_data)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_logical_data->NextPtr   = FSDev_Logical_ListFreePtr;
    FSDev_Logical_ListFreePtr = p_logical_data;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                       FSDev_Logical_DataGet()
*
* Description : Allocate & initialize a logical data object.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to a logical data object, if NO errors.
*               Pointer to NULL,                  otherwise.
*
* Caller(s)   : FSDev_Logical_Open().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  FS_DEV_LOGICAL_DATA  *FSDev_Logical_DataGet (void)
{
    LIB_ERR               alloc_err;
    CPU_SIZE_T            octets_reqd;
    FS_DEV_LOGICAL_DATA  *p_logical_data;
    CPU_SR_ALLOC();


                                                                /* --------------------- ALLOC DATA ------------------- */
    CPU_CRITICAL_ENTER();
    if (FSDev_Logical_ListFreePtr == (FS_DEV_LOGICAL_DATA *)0) {
        p_logical_data = (FS_DEV_LOGICAL_DATA *)Mem_HeapAlloc( sizeof(FS_DEV_LOGICAL_DATA),
                                                               sizeof(CPU_INT32U),
                                                              &octets_reqd,
                                                              &alloc_err);

        if (p_logical_data == (FS_DEV_LOGICAL_DATA *)0) {
            CPU_CRITICAL_EXIT();
            FS_TRACE_DBG(("FSDev_Logical_DataGet(): Could not alloc mem for logical dev data: %d octets required.\r\n", octets_reqd));
            return ((FS_DEV_LOGICAL_DATA *)0);
        }
        (void)&alloc_err;

        FSDev_Logical_UnitCtr++;


    } else {
        p_logical_data            = FSDev_Logical_ListFreePtr;
        FSDev_Logical_ListFreePtr = FSDev_Logical_ListFreePtr->NextPtr;
    }
    CPU_CRITICAL_EXIT();


                                                                /* ---------------------- CLR DATA -------------------- */
    p_logical_data->SecSize    =  0u;
    p_logical_data->Size       =  0u;
    p_logical_data->DevTbl     = (FS_DEV_LOGICAL_DEV_TBL_ENTRY *)0;
    p_logical_data->DevTblSize =  0u;

#if (FS_CFG_CTR_STAT_EN        == DEF_ENABLED)
    p_logical_data->StatRdCtr  =  0u;
    p_logical_data->StatWrCtr  =  0u;
#endif

    p_logical_data->NextPtr    = (FS_DEV_LOGICAL_DATA *)0;

    return (p_logical_data);
}
