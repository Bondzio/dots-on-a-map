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
*                                          VIRTUAL DISK IMAGE
*
* Filename      : fs_vdi.h
* Version       : v4.07.00.00
* Programmer(s) : EJ
*
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_VDI_MODULE
#include  <lib_ascii.h>
#include  <lib_mem.h>
#include  <fs.h>
#include  <fs_vdi.h>
#ifndef    FS_VDI_STDIO_INCLUDE_FILE_NAME
#include  <stdio.h>
#else
#include   FS_VDI_STDIO_INCLUDE_FILE_NAME
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            TYPE DEFINES
*
* Note(s) : (1) FS_DEV_xxx_TYPE_??? #define values specifically chosen as ASCII representations of the
*               device types.  Memory displays of device data will display the device TYPEs with their
*               chosen ASCII names.
*********************************************************************************************************
*/

#define  FS_VDI_TYPE_NONE       FS_TYPE_CREATE(ASCII_CHAR_LATIN_UPPER_N,  \
                                               ASCII_CHAR_LATIN_UPPER_O,  \
                                               ASCII_CHAR_LATIN_UPPER_N,  \
                                               ASCII_CHAR_LATIN_UPPER_E)

#define  FS_VDI_TYPE_VDI        FS_TYPE_CREATE(ASCII_CHAR_LATIN_UPPER_V,  \
                                               ASCII_CHAR_LATIN_UPPER_D,  \
                                               ASCII_CHAR_LATIN_UPPER_I,  \
                                               ASCII_CHAR_SPACE)


/*
*********************************************************************************************************
*                                            LOCAL DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           VDI DATA DATA TYPE
*********************************************************************************************************
*/

typedef  struct  fs_vdi_data  FS_VDI_DATA;
struct  fs_vdi_data {
    FS_TYPE       Type;
    FS_QTY        UnitNbr;

    FS_SEC_SIZE   SecSize;
    FS_SEC_QTY    Size;
    FILE         *FilePtr;

#if (FS_CFG_CTR_STAT_EN == DEF_ENABLED)
    FS_CTR        StatRdCtr;
    FS_CTR        StatWrCtr;
#endif

    FS_VDI_DATA  *NextPtr;
};


/*
*********************************************************************************************************
*                                            LOCAL CONSTANTS
*********************************************************************************************************
*/

static  const  CPU_CHAR  FS_VDI_Name[] = "vdi";


/*
*********************************************************************************************************
*                                              LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  FS_VDI_DATA  *FS_VDI_ListFreePtr;


/*
*********************************************************************************************************
*                                             LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                            /* ---------- DEV INTERFACE FNCTS --------- */
static  const  CPU_CHAR  *FS_VDI_NameGet    (void);                         /* Get name of driver.                      */

static  void              FS_VDI_Init       (FS_ERR           *p_err);      /* Init driver.                             */

static  void              FS_VDI_Open       (FS_DEV           *p_dev,       /* Open device.                             */
                                             void             *p_dev_cfg,
                                             FS_ERR           *p_err);

static  void              FS_VDI_Close      (FS_DEV           *p_dev);      /* Close device.                            */

static  void              FS_VDI_Rd         (FS_DEV           *p_dev,       /* Read from device.                        */
                                             void             *p_dest,
                                             FS_SEC_NBR        start,
                                             FS_SEC_QTY        cnt,
                                             FS_ERR           *p_err);

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
static  void              FS_VDI_Wr         (FS_DEV           *p_dev,       /* Write to device.                         */
                                             void             *p_src,
                                             FS_SEC_NBR        start,
                                             FS_SEC_QTY        cnt,
                                             FS_ERR           *p_err);
#endif

static  void              FS_VDI_Query      (FS_DEV           *p_dev,       /* Get device info.                         */
                                             FS_DEV_INFO      *p_info,
                                             FS_ERR           *p_err);

static  void              FS_VDI_IO_Ctrl    (FS_DEV           *p_dev,       /* Perform device I/O control.              */
                                             CPU_INT08U        opt,
                                             void             *p_data,
                                             FS_ERR           *p_err);

                                                                            /* -------------- LOCAL FNCTS ------------- */
static  void              FS_VDI_DataFree   (FS_VDI_DATA  *p_vdi_data);     /* Free VDI data.                           */

static  FS_VDI_DATA      *FS_VDI_DataGet    (void);                         /* Allocate & initialize VDI data.          */

/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_API  FS_VDI = {
    FS_VDI_NameGet,
    FS_VDI_Init,
    FS_VDI_Open,
    FS_VDI_Close,
    FS_VDI_Rd,
#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
    FS_VDI_Wr,
#endif
    FS_VDI_Query,
    FS_VDI_IO_Ctrl
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
*                                            FS_VDI_NameGet()
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

static  const  CPU_CHAR  *FS_VDI_NameGet (void)
{
    return (FS_VDI_Name);
}


/*
*********************************************************************************************************
*                                             FS_VDI_Init()
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

static  void  FS_VDI_Init (FS_ERR  *p_err)
{
    FS_VDI_UnitCtr     =  0u;
    FS_VDI_ListFreePtr = (FS_VDI_DATA *)0;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                             FS_VDI_Open()
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

static  void  FS_VDI_Open (FS_DEV  *p_dev,
                           void    *p_dev_cfg,
                           FS_ERR  *p_err)
{
    FS_VDI_DATA  *p_vdi_data;
    FS_VDI_CFG   *p_vdi_cfg;

                                                                /* ------------------- VALIDATE CFG ------------------- */
    if (p_dev_cfg == (void *)0) {                               /* Validate dev cfg.                                    */
        FS_TRACE_DBG(("FS_VDI_Open(): VDI disk cfg NULL ptr.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }

    p_vdi_cfg = (FS_VDI_CFG *)p_dev_cfg;

    switch (p_vdi_cfg->SecSize) {                               /* Validate sec size.                                   */
        case 512u:
        case 1024u:
        case 2048u:
        case 4096u:
             break;

        default:
            FS_TRACE_DBG(("FS_VDI_Open(): Invalid VDI disk sec size: %d.\r\n", p_vdi_cfg->SecSize));
           *p_err = FS_ERR_DEV_INVALID_CFG;
            return;
    }

    if (p_vdi_cfg->Size < 1u) {                                 /* Validate size.                                       */
        FS_TRACE_DBG(("FS_VDI_Open(): Invalid VDI disk size: %d.\r\n", p_vdi_cfg->Size));
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }

    if (p_vdi_cfg->FileName == (void *)0) {                     /* Validate file name.                                  */
        FS_TRACE_DBG(("FS_VDI_Open(): VDI disk file name specified as NULL ptr.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }



                                                                /* -------------------- INIT UNIT --------------------- */
    p_vdi_data = FS_VDI_DataGet();
    if (p_vdi_data == (FS_VDI_DATA *)0) {
       *p_err = FS_ERR_MEM_ALLOC;
        return;
    }

    p_dev->DataPtr      = (void *)p_vdi_data;


    p_vdi_data->UnitNbr =  p_dev->UnitNbr;
    p_vdi_data->SecSize =  p_vdi_cfg->SecSize;
    p_vdi_data->Size    =  p_vdi_cfg->Size;

                                                                /* ------------------ OPEN VDI FILE ------------------- */
    p_vdi_data->FilePtr = fopen(p_vdi_cfg->FileName, "rb+");    /* Open file for rd and update.                         */
    if (p_vdi_data->FilePtr == DEF_NULL ) {                     /* If unable to open ...                                */
        p_vdi_data->FilePtr = fopen(p_vdi_cfg->FileName, "wb+");/* ... the file might not exist; create it.             */
        if (p_vdi_data->FilePtr == DEF_NULL) {
            FS_TRACE_DBG(("FS_VDI_Open(): Unable to open VDI disk file.\r\n"));
           *p_err = FS_ERR_DEV_INVALID_CFG;
            return;
        }
    }


    FS_TRACE_INFO(("VDI DISK FOUND: Name    : \"vdi:%d:\"\r\n", p_vdi_data->UnitNbr));
    FS_TRACE_INFO(("                Sec Size: %d bytes\r\n",    p_vdi_data->SecSize));
    FS_TRACE_INFO(("                Size    : %d secs \r\n",    p_vdi_data->Size));


    *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          FS_VDI_Close()
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

static  void  FS_VDI_Close (FS_DEV  *p_dev)
{
    FS_VDI_DATA  *p_vdi_data;


    p_vdi_data = (FS_VDI_DATA *)p_dev->DataPtr;
    fclose(p_vdi_data->FilePtr);
    FS_VDI_DataFree(p_vdi_data);
}



/*
*********************************************************************************************************
*                                              FS_VDI_Rd()
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

static  void  FS_VDI_Rd (FS_DEV      *p_dev,
                         void        *p_dest,
                         FS_SEC_NBR   start,
                         FS_SEC_QTY   cnt,
                         FS_ERR      *p_err)
{
    FS_VDI_DATA  *p_vdi_data;
    FS_SEC_QTY    i;
    CPU_SIZE_T    src_offset;
    CPU_SIZE_T    cnt_octets;
    CPU_SIZE_T    rd_size;
    CPU_INT08U   *p_dest_08;
    CPU_INT08U    fail;


    p_vdi_data = (FS_VDI_DATA *)p_dev->DataPtr;

    p_dest_08  = (CPU_INT08U *) p_dest;
    src_offset = start * p_vdi_data->SecSize;
    cnt_octets = (CPU_SIZE_T  )(p_vdi_data->SecSize);

    fail = fseek(p_vdi_data->FilePtr, src_offset, SEEK_SET);    /* Seek file at correct pos for rd.                     */
    if (fail != 0u) {
        FS_TRACE_DBG(("FS_VDI_Rd(): Unable to seek VDI disk file.\r\n"));
       *p_err = FS_ERR_DEV_IO;
        return;
    }


    for (i = 0u; i < cnt; i++) {
        rd_size = fread(p_dest_08, 1, cnt_octets, p_vdi_data->FilePtr);
        if (rd_size != cnt_octets) {
            if (feof(p_vdi_data->FilePtr) != 0) {               /* If EOF reached ...                                   */
                                                                /* ... pad end of buffer with zeros.                    */
                Mem_Set(&p_dest_08[rd_size], 0x00u, cnt_octets - rd_size);
            } else {
                FS_TRACE_DBG(("FS_VDI_Rd(): Error reading data from VDI disk at sector %u.\r\n",
                              start + i));
               *p_err = FS_ERR_DEV_IO;
            }
        }

        p_dest_08 += cnt_octets;
    }

    FS_CTR_STAT_ADD(p_vdi_data->StatRdCtr, (FS_CTR)cnt);

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                              FS_VDI_Wr()
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
static  void  FS_VDI_Wr (FS_DEV      *p_dev,
                         void        *p_src,
                         FS_SEC_NBR   start,
                         FS_SEC_QTY   cnt,
                         FS_ERR      *p_err)
{
    FS_VDI_DATA  *p_vdi_data;
    FS_SEC_QTY    i;
    CPU_SIZE_T    cnt_octets;
    CPU_SIZE_T    dest_offset;
    CPU_SIZE_T    wr_size;
    CPU_INT08U   *p_src_08;
    CPU_INT08U    fail;


    p_vdi_data  = (FS_VDI_DATA *)p_dev->DataPtr;

    p_src_08    = (CPU_INT08U *) p_src;
    dest_offset = start * p_vdi_data->SecSize;
    cnt_octets  = (CPU_SIZE_T  )(p_vdi_data->SecSize);

    fail = fseek(p_vdi_data->FilePtr, dest_offset, SEEK_SET);   /* Seek file at correct pos for rd.                     */
    if (fail != 0u) {
        FS_TRACE_DBG(("FS_VDI_Wr(): Unable to seek VDI disk file.\r\n"));
       *p_err = FS_ERR_DEV_IO;
        return;
    }

    for (i = 0u; i < cnt; i++) {
                                                                /* Wr secs in file.                                     */
        wr_size = fwrite(p_src_08, 1, cnt_octets, p_vdi_data->FilePtr);
        if (wr_size != cnt_octets) {
            FS_TRACE_DBG(("FS_VDI_Wr(): Error writing data to sector %u.\r\n",
                          start + i));
           *p_err = FS_ERR_DEV_IO;
            return;
        }


        p_src_08 += cnt_octets;
    }

    fflush(p_vdi_data->FilePtr);

    FS_CTR_STAT_ADD(p_vdi_data->StatWrCtr, (FS_CTR)cnt);

   *p_err = FS_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                          FS_VDI_Query()
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

static  void  FS_VDI_Query (FS_DEV       *p_dev,
                            FS_DEV_INFO  *p_info,
                            FS_ERR       *p_err)
{
    FS_VDI_DATA  *p_vdi_data;


    p_vdi_data      = (FS_VDI_DATA *)p_dev->DataPtr;

    p_info->SecSize =  p_vdi_data->SecSize;
    p_info->Size    =  p_vdi_data->Size;
    p_info->Fixed   =  DEF_NO;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         FS_VDI_IO_Ctrl()
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

static  void  FS_VDI_IO_Ctrl (FS_DEV      *p_dev,
                              CPU_INT08U   opt,
                              void        *p_data,
                              FS_ERR      *p_err)
{
   (void)&p_dev;                                                /* lint --e{550} Suppress "Symbol not accessed".        */
   (void)&opt;
   (void)&p_data;


                                                                /* ----------------- PERFORM I/O CTL ------------------ */
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
*                                        FS_VDI_DataFree()
*
* Description : Free a VDI data object.
*
* Argument(s) : p_vdi_data  Pointer to a VDI data object.
*               ----------  Argument validated by caller.
*
* Return(s)   : none.
*
* Caller(s)   : FS_VDI_Close().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FS_VDI_DataFree (FS_VDI_DATA  *p_vdi_data)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_vdi_data->Type    = FS_VDI_TYPE_NONE;
    p_vdi_data->NextPtr = FS_VDI_ListFreePtr;
    FS_VDI_ListFreePtr  = p_vdi_data;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                         FS_VDI_DataGet()
*
* Description : Allocate & initialize a VDI data object.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to a VDI data object, if NO errors.
*               Pointer to NULL,              otherwise.
*
* Caller(s)   : FS_VDI_Open().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  FS_VDI_DATA  *FS_VDI_DataGet (void)
{
    LIB_ERR       alloc_err;
    CPU_SIZE_T    octets_reqd;
    FS_VDI_DATA  *p_vdi_data;
    CPU_SR_ALLOC();


                                                                /* --------------------- ALLOC DA --------------------- */
    CPU_CRITICAL_ENTER();
    if (FS_VDI_ListFreePtr == (FS_VDI_DATA *)0) {
        p_vdi_data = (FS_VDI_DATA *)Mem_HeapAlloc(sizeof(FS_VDI_DATA),
                                                  sizeof(CPU_INT32U),
                                                 &octets_reqd,
                                                 &alloc_err);
        if (p_vdi_data == (FS_VDI_DATA *)0) {
            CPU_CRITICAL_EXIT();
            FS_TRACE_DBG(("FS_VDI_DataGet(): Could not alloc mem for virtual disk image data: %d octets required.\r\n", octets_reqd));
            return ((FS_VDI_DATA *)0);
        }
        (void)&alloc_err;

        FS_VDI_UnitCtr++;


    } else {
        p_vdi_data         = FS_VDI_ListFreePtr;
        FS_VDI_ListFreePtr = FS_VDI_ListFreePtr->NextPtr;
    }
    CPU_CRITICAL_EXIT();


                                                                /* --------------------- CLR DATA --------------------- */
    p_vdi_data->Type      =  FS_VDI_TYPE_VDI;

    p_vdi_data->SecSize   =  0u;
    p_vdi_data->Size      =  0u;
    p_vdi_data->FilePtr   = (FILE *)0;

#if (FS_CFG_CTR_STAT_EN   == DEF_ENABLED)
    p_vdi_data->StatRdCtr =  0u;
    p_vdi_data->StatWrCtr =  0u;
#endif

    p_vdi_data->NextPtr   = (FS_VDI_DATA *)0;

    return (p_vdi_data);
}
