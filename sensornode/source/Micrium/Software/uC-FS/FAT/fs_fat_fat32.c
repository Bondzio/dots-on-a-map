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
*                                     FILE SYSTEM FAT MANAGEMENT
*
*                                            FAT32 SUPPORT
*
* Filename      : fs_fat_fat32.c
* Version       : v4.07.00
* Programmer(s) : FBJ
*                 BAN
*                 AHFAI
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    FS_FAT_FAT32_MODULE
#include  <lib_mem.h>
#include  <fs.h>
#include  <fs_buf.h>
#include  <fs_fat.h>
#include  <fs_fat_fat32.h>
#include  <fs_fat_type.h>
#include  <fs_fat_journal.h>
#include  <fs_sys.h>
#include  <fs_vol.h>


/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) See 'fs_fat.h  MODULE'.
*********************************************************************************************************
*/

#ifdef   FS_FAT_FAT32_MODULE_PRESENT                            /* See Note #1.                                         */


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  FS_FAT_FAT32_CLUS_BAD                    0x0FFFFFF7u
#define  FS_FAT_FAT32_CLUS_EOF                    0x0FFFFFF8u
#define  FS_FAT_FAT32_CLUS_FREE                   0x00000000u
#define  FS_FAT_FAT32_CLUS_MASK                   0x0FFFFFFFu


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


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

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)                                             /* ---------- LOCAL FNCTS --------- */
static  void             FS_FAT_FAT32_ClusValWr       (FS_VOL           *p_vol,     /* Write value into cluster.        */
                                                       FS_BUF           *p_buf,
                                                       FS_FAT_CLUS_NBR   clus,
                                                       FS_FAT_CLUS_NBR   val,
                                                       FS_ERR           *p_err);
#endif

static  FS_FAT_CLUS_NBR  FS_FAT_FAT32_ClusValRd       (FS_VOL           *p_vol,     /* Read value from cluster.         */
                                                       FS_BUF           *p_buf,
                                                       FS_FAT_CLUS_NBR   clus,
                                                       FS_ERR           *p_err);


/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_FAT_TYPE_API  FS_FAT_FAT32_API = {
#if (FS_CFG_RD_ONLY_EN      == DEF_DISABLED)
    FS_FAT_FAT32_ClusValWr,
#endif
    FS_FAT_FAT32_ClusValRd,

    FS_FAT_FAT32_CLUS_BAD,
    FS_FAT_FAT32_CLUS_EOF,
    FS_FAT_FAT32_CLUS_FREE,
    FS_FAT_BS_FAT32_FILESYSTYPE
};


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        FS_FAT_FAT32_ClusValWr()
*
* Description : Write value into cluster.
*
* Argument(s) : p_vol       Pointer to volume.
*
*               p_buf       Pointer to temporary buffer.
*
*               clus        Cluster to modify.
*
*               val         Value to write into cluster.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE    Cluster written.
*                               FS_ERR_DEV     Device error.
*
* Return(s)   : none.
*
* Caller(s)   : FS_FAT_FAT32_ClusAlloc(),
*               FS_FAT_FAT32_ClusLink(),
*               FS_FAT_FAT32_ClusChainDel(),
*               FS_FAT_FAT32_ClusNextGetAlloc().
*
* Note(s)     : (1) The FAT entry on a FAT32 volume is a 28-bit value stored in 32-bit datum; the upper
*                   four bits, which are not valid bits, are not modified.
*********************************************************************************************************
*/

#if (FS_CFG_RD_ONLY_EN == DEF_DISABLED)
static  void  FS_FAT_FAT32_ClusValWr (FS_VOL           *p_vol,
                                      FS_BUF           *p_buf,
                                      FS_FAT_CLUS_NBR   clus,
                                      FS_FAT_CLUS_NBR   val,
                                      FS_ERR           *p_err)
{
    FS_SEC_SIZE      fat_offset;
    FS_FAT_SEC_NBR   fat_sec;
    FS_SEC_SIZE      fat_sec_offset;
    FS_FAT_SEC_NBR   fat_start_sec;
    FS_FAT_DATA     *p_fat_data;
    CPU_INT32U       val_temp;


    p_fat_data     = (FS_FAT_DATA *)p_vol->DataPtr;

    fat_start_sec  =  p_fat_data->FAT1_Start;
    fat_offset     = (FS_SEC_SIZE)clus * FS_FAT_FAT32_ENTRY_NBR_OCTETS;
    fat_sec        =  fat_start_sec + (FS_FAT_SEC_NBR)FS_UTIL_DIV_PWR2(fat_offset, p_fat_data->SecSizeLog2);
    fat_sec_offset =  fat_offset & (p_fat_data->SecSize - 1u);

    FSBuf_Set(p_buf,                                            /* Rd FAT sec.                                          */
              fat_sec,
              FS_VOL_SEC_TYPE_MGMT,
              DEF_YES,
              p_err);
    if (*p_err != FS_ERR_NONE) {
        return;
    }

    val_temp  =  MEM_VAL_GET_INT32U_LITTLE((void *)((CPU_INT08U *)p_buf->DataPtr + fat_sec_offset));
    val_temp &= ~FS_FAT_FAT32_CLUS_MASK;                        /* Keep upper entry bits (see Note #1).                 */
    val_temp |=  val;
    MEM_VAL_SET_INT32U_LITTLE((void *)((CPU_INT08U *)p_buf->DataPtr + fat_sec_offset), val_temp);

    FSBuf_MarkDirty(p_buf, p_err);                              /* Wr FAT sec.                                          */
}
#endif


/*
*********************************************************************************************************
*                                        FS_FAT_FAT32_ClusValRd()
*
* Description : Read value from cluster.
*
* Argument(s) : p_vol       Pointer to volume.
*
*               p_buf       Pointer to temporary buffer.
*
*               clus        Cluster to modify.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE    Cluster read.
*                               FS_ERR_DEV     Device error.
*
* Return(s)   : Cluster value.
*
* Caller(s)   : FS_FAT_FAT32_ClusChainDel(),
*               FS_FAT_FAT32_ClusChainFollow(),
*               FS_FAT_FAT32_ClusNextGet(),
*               FS_FAT_FAT32_ClusNextGetAlloc(),
*               FS_FAT_FAT32_Query().
*               FS_FAT_FAT32_ClusFreeFind().
*
* Note(s)     : (1) The FAT entry on a FAT32 volume is a 28-bit value stored in 32-bit datum; the upper
*                   four bits, which are not valid bits, are masked off the returned value.
*********************************************************************************************************
*/

static  FS_FAT_CLUS_NBR  FS_FAT_FAT32_ClusValRd (FS_VOL           *p_vol,
                                                 FS_BUF           *p_buf,
                                                 FS_FAT_CLUS_NBR   clus,
                                                 FS_ERR           *p_err)
{
    FS_SEC_SIZE       fat_offset;
    FS_FAT_SEC_NBR    fat_sec;
    FS_SEC_SIZE       fat_sec_offset;
    FS_FAT_SEC_NBR    fat_start_sec;
    FS_FAT_DATA      *p_fat_data;
    FS_FAT_CLUS_NBR   val;


    p_fat_data     = (FS_FAT_DATA *)p_vol->DataPtr;

    fat_start_sec  =  p_fat_data->FAT1_Start;
    fat_offset     = (FS_SEC_SIZE)clus * FS_FAT_FAT32_ENTRY_NBR_OCTETS;
    fat_sec        =  fat_start_sec + (FS_FAT_SEC_NBR)FS_UTIL_DIV_PWR2(fat_offset, p_fat_data->SecSizeLog2);
    fat_sec_offset =  fat_offset & (p_fat_data->SecSize - 1u);

    FSBuf_Set(p_buf,
              fat_sec,
              FS_VOL_SEC_TYPE_MGMT,
              DEF_YES,
              p_err);
    if (*p_err != FS_ERR_NONE) {
        return (0u);
    }

    val  = MEM_VAL_GET_INT32U_LITTLE((void *)((CPU_INT08U *)p_buf->DataPtr + fat_sec_offset));
    val &= FS_FAT_FAT32_CLUS_MASK;                              /* Mask off upper entry bits (see Note #1).             */

    return (val);
}


/*
*********************************************************************************************************
*                                             MODULE END
*
* Note(s) : (1) See 'MODULE  Note #1' & 'fs_fat.h  MODULE  Note #1'.
*********************************************************************************************************
*/

#endif                                                          /* End of FAT32 module include (see Note #1).           */
