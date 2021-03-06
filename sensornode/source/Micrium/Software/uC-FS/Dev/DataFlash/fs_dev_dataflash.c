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
*                                             DATA FLASH
*
* Filename      : fs_dev_dataflash.c
* Version       : v4.07.00.00
* Programmer(s) : JPC
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_DATAFLASH_MODULE
#include  <fs.h>
#include  "fs_dev_dataflash.h"


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
*                                     DATAFLASH DEVICE DATA DATA TYPE
*********************************************************************************************************
*/

typedef  struct  fs_dev_dataflash_data  FS_DEV_DATAFLASH_DATA;
struct  fs_dev_dataflash_data {
    FS_QTY            UnitNbr;

    FS_SEC_SIZE       SecSize;
    FS_SEC_QTY        Size;
    void             *DiskPtr;

#if (FS_CFG_CTR_STAT_EN == DEF_ENABLED)
    FS_CTR                 StatRdCtr;
    FS_CTR                 StatWrCtr;
#endif

#if (FS_CFG_CTR_ERR_EN == DEF_ENABLED)
   /* $$$$ ERROR COUNTERS */
#endif

    FS_DEV_DATAFLASH_DATA  *NextPtr;
};


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

#define  FS_DEV_DATAFLASH_NAME_LEN                 2u

static  const  CPU_CHAR  FSDev_DataFlash_Name[] = "df";


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

static  FS_DEV_DATAFLASH_DATA  *FSDev_DataFlash_ListFreePtr;


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

                                                                /* --------------- DEV INTERFACE FNCTS ---------------- */
                                                                /* Get name of driver.                                  */
static  const  CPU_CHAR       *FSDev_DataFlash_NameGet (void);
                                                                /* Init driver.                                         */
static  void                   FSDev_DataFlash_Init    (FS_ERR                *p_err);
                                                                /* Open device.                                         */
static  void                   FSDev_DataFlash_Open    (FS_DEV                *p_dev,
                                                        void                  *p_dev_cfg,
                                                        FS_ERR                *p_err);
                                                                /* Close device.                                        */
static  void                   FSDev_DataFlash_Close   (FS_DEV                *p_dev);
                                                                /* Read from device.                                    */
static  void                   FSDev_DataFlash_Rd      (FS_DEV                *p_dev,
                                                        void                  *p_dest,
                                                        FS_SEC_NBR             start,
                                                        FS_SEC_QTY             cnt,
                                                        FS_ERR                *p_err);

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)                         /* Write to device.                                     */
static  void                   FSDev_DataFlash_Wr      (FS_DEV                *p_dev,
                                                        void                  *p_src,
                                                        FS_SEC_NBR             start,
                                                        FS_SEC_QTY             cnt,
                                                        FS_ERR                *p_err);
#endif
                                                                /* Get device info.                                     */
static  void                   FSDev_DataFlash_Query   (FS_DEV                *p_dev,
                                                        FS_DEV_INFO           *p_info,
                                                        FS_ERR                *p_err);
                                                                /* Perform device I/O control.                          */
static  void                   FSDev_DataFlash_IO_Ctrl (FS_DEV                *p_dev,
                                                        CPU_INT08U             opt,
                                                        void                  *p_data,
                                                        FS_ERR                *p_err);

                                                                /* ------------------- LOCAL FNCTS -------------------- */
                                                                /* Free DataFlash data.                                 */
static  void                   FSDev_DataFlash_DataFree (FS_DEV_DATAFLASH_DATA  *p_dataflash_data);
                                                                /* Allocate & init DataFlash data.                      */
static  FS_DEV_DATAFLASH_DATA  *FSDev_DataFlash_DataGet (void);

/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_API  FSDev_DataFlash = {
    FSDev_DataFlash_NameGet,
    FSDev_DataFlash_Init,
    FSDev_DataFlash_Open,
    FSDev_DataFlash_Close,
    FSDev_DataFlash_Rd,
#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
    FSDev_DataFlash_Wr,
#endif
    FSDev_DataFlash_Query,
    FSDev_DataFlash_IO_Ctrl
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
*                                        FSDev_DataFlash_NameGet()
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

static  const  CPU_CHAR  *FSDev_DataFlash_NameGet (void)
{
    return (FSDev_DataFlash_Name);
}


/*
*********************************************************************************************************
*                                          FSDev_DataFlash_Init()
*
* Description : Initialize the driver.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
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

static  void  FSDev_DataFlash_Init (FS_ERR  *p_err)
{
    FSDev_DataFlash_UnitCtr     =  0u;
    FSDev_DataFlash_ListFreePtr = (FS_DEV_DATAFLASH_DATA *)0;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          FSDev_DataFlash_Open()
*
* Description : Open (initialize) a device instance.
*
* Argument(s) : p_dev       Pointer to device to open.
*
*               p_dev_cfg   Pointer to device configuration.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
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
*
*               (5) The driver should identify the device instance to be opened by checking
*                   'p_dev->UnitNbr'.  For example, if "template:2:" is to be opened, then
*                   'p_dev->UnitNbr' will hold the integer 2.
*********************************************************************************************************
*/

static  void  FSDev_DataFlash_Open (FS_DEV  *p_dev,
                                    void    *p_dev_cfg,
                                    FS_ERR  *p_err)
{
    FS_DEV_DATAFLASH_DATA  *p_df_data;

                                                                /* --------------------- INIT UNIT -------------------- */
    p_df_data = FSDev_DataFlash_DataGet();
    if (p_df_data == (FS_DEV_DATAFLASH_DATA *)0) {
       *p_err = FS_ERR_MEM_ALLOC;
        return;
    }

    p_df_data->UnitNbr =  p_dev->UnitNbr;
    p_df_data->SecSize =  DF_SecSizeGet();
    p_df_data->Size    =  DF_SecCntGet();
    p_df_data->DiskPtr =  DF_BaseAddrGet();
    p_dev->DataPtr     = (void *)p_df_data;

    FS_TRACE_INFO(("DATAFLASH DISK FOUND: Name    : \"df:%d:\"\r\n",  p_df_data->UnitNbr));
    FS_TRACE_INFO(("                      Sec Size: %d bytes\r\n",    p_df_data->SecSize));
    FS_TRACE_INFO(("                      Size    : %d secs \r\n",    p_df_data->Size));

                                                                /* Enable access to DataFlash.                          */
    DF_AccessEn();

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         FSDev_DataFlash_Close()
*
* Description : Close (uninitialize) a device instance.
*
* Argument(s) : p_dev       Pointer to device to close.
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

static  void  FSDev_DataFlash_Close (FS_DEV  *p_dev)
{
    if (p_dev->DataPtr != DEF_NULL) {
        FSDev_DataFlash_DataFree((FS_DEV_DATAFLASH_DATA *)p_dev->DataPtr);
    }

                                                                /* Disable access to DataFlash memory.                  */
    DF_AccessDis();
}


/*
*********************************************************************************************************
*                                           FSDev_DataFlash_Rd()
*
* Description : Read from a device & store data in buffer.
*
* Argument(s) : p_dev       Pointer to device to read from.
*
*               p_dest      Pointer to destination buffer.
*
*               start       Start sector of read.
*
*               cnt         Number of sectors to read.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
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

static  void  FSDev_DataFlash_Rd (FS_DEV      *p_dev,
                                  void        *p_dest,
                                  FS_SEC_NBR   start,
                                  FS_SEC_QTY   cnt,
                                  FS_ERR      *p_err)
{
    FS_DEV_DATAFLASH_DATA  *p_df_data;
    FS_SEC_QTY              i;
    CPU_SIZE_T              cnt_octets;
    CPU_INT08U             *p_dest_08;
    CPU_INT08U             *p_sec;


    p_df_data = (FS_DEV_DATAFLASH_DATA *)p_dev->DataPtr;

    p_dest_08  = (CPU_INT08U *) p_dest;
    p_sec      = (CPU_INT08U *)(p_df_data->DiskPtr) + (start * p_df_data->SecSize);
    cnt_octets = (CPU_SIZE_T  )(p_df_data->SecSize);

    for (i = 0u; i < cnt; i++) {
        Mem_Copy(p_dest_08,
                 p_sec,
                 cnt_octets);

        p_dest_08 += cnt_octets;
        p_sec     += cnt_octets;
    }

    FS_CTR_STAT_ADD(p_df_data->StatRdCtr, (FS_CTR)cnt);

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           FSDev_DataFlash_Wr()
*
* Description : Write data to a device from a buffer.
*
* Argument(s) : p_dev       Pointer to device to write to.
*
*               p_src       Pointer to source buffer.
*
*               start       Start sector of write.
*
*               cnt         Number of sectors to write.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
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
static  void  FSDev_DataFlash_Wr (FS_DEV      *p_dev,
                                  void        *p_src,
                                  FS_SEC_NBR   start,
                                  FS_SEC_QTY   cnt,
                                  FS_ERR      *p_err)
{
    FS_DEV_DATAFLASH_DATA  *p_df_data;
    FS_SEC_QTY              i;
    CPU_SIZE_T              cnt_octets;
    CPU_INT08U             *p_sec_08;
    CPU_INT08U             *p_src_08;
    int                     prog_err;

                                                                /* Put DataFlash into P/E mode.                         */
    DF_ProgramEraseModeEnter();


    p_df_data  = (FS_DEV_DATAFLASH_DATA *)p_dev->DataPtr;

    p_src_08   = (CPU_INT08U *) p_src;
    p_sec_08   = (CPU_INT08U *)(p_df_data->DiskPtr) + (start * p_df_data->SecSize);
    cnt_octets = (CPU_SIZE_T  )(p_df_data->SecSize);

    for (i = 0u; i < cnt; i++) {
        prog_err = DF_Program(p_sec_08,
                              p_src_08,
                              cnt_octets);

        if (prog_err < 0) {
            DF_ProgramEraseModeExit();                          /* Exit P/E mode.                                       */
            ///@todo increment error counter
//            FS_CTR_STAT_ADD(p_df_data->StatWrCtr, (FS_CTR)cnt);
           *p_err = FS_ERR_DEV_OP_FAILED;
            return;
        }

        p_src_08 += cnt_octets;
        p_sec_08 += cnt_octets;
    }

                                                                /* Put DataFlash back into read mode.                   */
    DF_ProgramEraseModeExit();

    FS_CTR_STAT_ADD(p_df_data->StatWrCtr, (FS_CTR)cnt);

   *p_err = FS_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                         FSDev_DataFlash_Query()
*
* Description : Get information about a device.
*
* Argument(s) : p_dev       Pointer to device to query.
*
*               p_info      Pointer to structure that will receive device information.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
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

static  void  FSDev_DataFlash_Query (FS_DEV       *p_dev,
                                     FS_DEV_INFO  *p_info,
                                     FS_ERR       *p_err)
{

    p_info->SecSize = DF_SecSizeGet();                          /* Get DataFlash sector size in bytes.                  */
    p_info->Size    = DF_SecCntGet();                           /* Get number of sectors in DataFlash.                  */
    p_info->Fixed   = DEF_YES;                                  /* Device cannot be removed.                            */

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        FSDev_DataFlash_IO_Ctrl()
*
* Description : Perform device I/O control operation.
*
* Argument(s) : p_dev       Pointer to device to control.
*
*               opt         Control command.
*
*               p_data      Buffer which holds data to be used for operation.
*                              OR
*                           Buffer in which data will be stored as a result of operation.
*
*               p_err       Pointer to variable that will receive return the error code from this function :
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
*                   (a) FS_DEV_IO_CTRL_REFRESH           Refresh device.
*                   (b) FS_DEV_IO_CTRL_LOW_FMT           Low-level format device.
*                   (c) FS_DEV_IO_CTRL_LOW_MOUNT         Low-level mount device.
*                   (d) FS_DEV_IO_CTRL_LOW_UNMOUNT       Low-level unmount device.
*                   (e) FS_DEV_IO_CTRL_LOW_COMPACT       Low-level compact device.
*                   (f) FS_DEV_IO_CTRL_LOW_DEFRAG        Low-level defragment device.
*                   (g) FS_DEV_IO_CTRL_SEC_RELEASE       Release data in sector.
*                   (h) FS_DEV_IO_CTRL_PHY_RD            Read  physical device.
*                   (i) FS_DEV_IO_CTRL_PHY_WR            Write physical device.
*                   (j) FS_DEV_IO_CTRL_PHY_RD_PAGE       Read  physical device page.
*                   (k) FS_DEV_IO_CTRL_PHY_WR_PAGE       Write physical device page.
*                   (l) FS_DEV_IO_CTRL_PHY_ERASE_BLK     Erase physical device block.
*                   (m) FS_DEV_IO_CTRL_PHY_ERASE_CHIP    Erase physical device.
*
*                   Not all of these operations are valid for all devices.
*********************************************************************************************************
*/

static  void  FSDev_DataFlash_IO_Ctrl (FS_DEV      *p_dev,
                                       CPU_INT08U   opt,
                                       void        *p_data,
                                       FS_ERR      *p_err)
{
   (void)p_dev;
   (void)opt;
   (void)p_data;

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
*                                       FSDev_DataFlash_DataFree()
*
* Description : Free a DataFlash data object.
*
* Argument(s) : p_dataflash_data   Pointer to a DataFlash data object.
*               -----------   Argument validated by caller.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_DataFlash_Close().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_DataFlash_DataFree (FS_DEV_DATAFLASH_DATA  *p_dataflash_data)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_dataflash_data->NextPtr   = FSDev_DataFlash_ListFreePtr;  /* Add to free pool.                                    */
    FSDev_DataFlash_ListFreePtr = p_dataflash_data;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                        FSDev_DataFlash_DataGet()
*
* Description : Allocate & initialize a DataFlash data object.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to a DataFlash data object, if NO errors.
*               Pointer to NULL,               otherwise.
*
* Caller(s)   : FSDev_DataFlash_Open().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  FS_DEV_DATAFLASH_DATA  *FSDev_DataFlash_DataGet (void)
{
    LIB_ERR                 alloc_err;
    CPU_SIZE_T              octets_reqd;
    FS_DEV_DATAFLASH_DATA  *p_dataflash_data;
    CPU_SR_ALLOC();


                                                                /* --------------------- ALLOC DATA ------------------- */
    CPU_CRITICAL_ENTER();
    if (FSDev_DataFlash_ListFreePtr == (FS_DEV_DATAFLASH_DATA *)0) {
        p_dataflash_data = (FS_DEV_DATAFLASH_DATA *)Mem_HeapAlloc( sizeof(FS_DEV_DATAFLASH_DATA),
                                                                   sizeof(CPU_INT32U),
                                                                  &octets_reqd,
                                                                  &alloc_err);
        if (p_dataflash_data == (FS_DEV_DATAFLASH_DATA *)0) {
            CPU_CRITICAL_EXIT();
            FS_TRACE_DBG(("FSDev_DataFlash_DataGet(): Could not alloc mem for DataFlash data: %d octets required.\r\n", octets_reqd));
            return ((FS_DEV_DATAFLASH_DATA *)0);
        }
        (void)alloc_err;

        FSDev_DataFlash_UnitCtr++;


    } else {
        p_dataflash_data            = FSDev_DataFlash_ListFreePtr;
        FSDev_DataFlash_ListFreePtr = FSDev_DataFlash_ListFreePtr->NextPtr;
    }
    CPU_CRITICAL_EXIT();


                                                                /* ---------------------- CLR DATA -------------------- */
#if (FS_CFG_CTR_STAT_EN    == DEF_ENABLED)                      /* Clr stat ctrs.                                       */
    p_dataflash_data->StatRdCtr =  0u;
    p_dataflash_data->StatWrCtr =  0u;
#endif

#if (FS_CFG_CTR_ERR_EN     == DEF_ENABLED)                      /* Clr err ctrs.                                        */
   /* $$$$ CLEAR ERROR COUNTERS */
#endif

    p_dataflash_data->NextPtr   = (FS_DEV_DATAFLASH_DATA *)0;

    return (p_dataflash_data);
}
