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
*                                         NAND FLASH DEVICES
*                  STM32 FLEXIBLE STATIC MEMORY CONTROLLER & HARDWARE ECC GENERATION
*                                        WITH SMALL-PAGE NAND
*
* Filename      : fs_dev_nand_stm32.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_NAND_STM32_MODULE
#include  <fs_dev_nand_stm32.h>
#include  <ecc_hamming.h>


/*
*********************************************************************************************************
*                                           LINT INHIBITION
*
* Note(s) : (1) Certain macro's within this file describe commands, command values or register fields
*               that are currently unused.  lint error option #750 is disabled to prevent error messages
*               because of unused macro's :
*
*                   "Info 750: local macro '...' (line ..., file ...) not referenced"
*********************************************************************************************************
*/

/*lint -e750*/


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  FS_DEV_NAND_PHY_PAGE_SIZE                       512u
#define  FS_DEV_NAND_PHY_SPARE_SIZE                       16u
#define  FS_DEV_NAND_PHY_PAGES_PER_BLK                    32u

#define  FS_DEV_NAND_PHY_BLK_CNT_TH             ((DEF_INT_16U_MAX_VAL / FS_DEV_NAND_PHY_PAGES_PER_BLK) + 1u)

#define  FS_DEV_NAND_PHY_OFFSET_ECC1                       0u
#define  FS_DEV_NAND_PHY_OFFSET_BAD_BLK_MARK               5u
#define  FS_DEV_NAND_PHY_OFFSET_ECC2                       6u
#define  FS_DEV_NAND_PHY_OFFSET_DATA                      10u
#define  FS_DEV_NAND_PHY_OFFSET_MARK                      12u
#define  FS_DEV_NAND_PHY_OFFSET_ECC_DATA                  14u

#define  FS_DEV_NAND_PHY_BAD_BLK_MARK                   0xFFu
#define  FS_DEV_NAND_PHY_MARK                         0x5AA5u

#define  FS_DEV_NAND_PHY_MAX_RD_us                        25u
#define  FS_DEV_NAND_PHY_MAX_PGM_us                      700u
#define  FS_DEV_NAND_PHY_MAX_BLK_ERASE_us               3000u

/*
*********************************************************************************************************
*                                           COMMAND DEFINES
*********************************************************************************************************
*/

#define  FS_DEV_NAND_PHY_CMD_RDA                        0x00u
#define  FS_DEV_NAND_PHY_CMD_RDB                        0x01u
#define  FS_DEV_NAND_PHY_CMD_RDC                        0x50u
#define  FS_DEV_NAND_PHY_CMD_RDID                       0x90u
#define  FS_DEV_NAND_PHY_CMD_RDSTATUS                   0x70u

#define  FS_DEV_NAND_PHY_CMD_PAGEPGM_SETUP              0x80u
#define  FS_DEV_NAND_PHY_CMD_PAGEPGM_CONFIRM            0x10u

#define  FS_DEV_NAND_PHY_CMD_BLKERASE_SETUP             0x60u
#define  FS_DEV_NAND_PHY_CMD_BLKERASE_CONFIRM           0xD0u

#define  FS_DEV_NAND_PHY_CMD_RESET                      0xFFu

/*
*********************************************************************************************************
*                                     STATUS REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  FS_DEV_NAND_PHY_SR_WRPROTECT            DEF_BIT_07
#define  FS_DEV_NAND_PHY_SR_BUSY                 DEF_BIT_06
#define  FS_DEV_NAND_PHY_SR_ERR                  DEF_BIT_00


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  struct  fs_dev_nand_phy_desc {
    CPU_INT08U  DevID;
    CPU_INT08U  BusWidth;
    CPU_INT32U  Size_Mb;
} FS_DEV_NAND_PHY_DESC;


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

static  const  FS_DEV_NAND_PHY_DESC  FSDev_NAND_PHY_DevTbl[] = {
    {0xDCu,      8u,     4u * 1024u},
    {0x71u,      8u,     2u * 1024u},
    {0x78u,      8u,     1u * 1024u},    {0x79u,      8u,     1u * 1024u},
    {0x72u,     16u,     1u * 1024u},    {0x74u,     16u,     1u * 1024u},
    {0x36u,      8u,           512u},    {0x76u,      8u,           512u},
    {0x46u,     16u,           512u},    {0x56u,     16u,           512u},
    {0x35u,      8u,           256u},    {0x75u,      8u,           256u},
    {0x45u,     16u,           256u},    {0x55u,     16u,           256u},
    {0x33u,      8u,           128u},    {0x73u,      8u,           128u},
    {0x43u,     16u,           128u},    {0x53u,     16u,           128u},
    {0x39u,      8u,            64u},    {0xE6u,      8u,            64u},
    {0x49u,     16u,            64u},    {0x59u,     16u,            64u},
    {   0u,      0u,             0u}
};


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

                                                                                    /* --- NAND PHY INTERFACE FNCTS --- */
static  void         FSDev_NAND_PHY_Open      (FS_DEV_NAND_PHY_DATA  *p_phy_data,   /* Open NAND device.                */
                                               FS_ERR                *p_err);

static  void         FSDev_NAND_PHY_Close     (FS_DEV_NAND_PHY_DATA  *p_phy_data);  /* Close NAND device.               */

static  void         FSDev_NAND_PHY_RdPage    (FS_DEV_NAND_PHY_DATA  *p_phy_data,   /* Read page from NAND device.      */
                                               void                  *p_dest,
                                               void                  *p_dest_spare,
                                               CPU_INT32U             page_nbr_phy,
                                               FS_ERR                *p_err);

static  void         FSDev_NAND_PHY_WrPage    (FS_DEV_NAND_PHY_DATA  *p_phy_data,   /* Write page to NAND device.       */
                                               void                  *p_src,
                                               void                  *p_src_spare,
                                               CPU_INT32U             page_nbr_phy,
                                               FS_ERR                *p_err);

static  void         FSDev_NAND_PHY_EraseBlk  (FS_DEV_NAND_PHY_DATA  *p_phy_data,   /* Erase block on NAND device.      */
                                               CPU_INT32U             blk_nbr_phy,
                                               FS_ERR                *p_err);

static  CPU_BOOLEAN  FSDev_NAND_PHY_IsBlkBad  (FS_DEV_NAND_PHY_DATA  *p_phy_data,   /* Determine whether block is bad.  */
                                               CPU_INT32U             blk_nbr_phy,
                                               FS_ERR                *p_err);

static  void         FSDev_NAND_PHY_MarkBlkBad(FS_DEV_NAND_PHY_DATA  *p_phy_data,   /* Mark block as bad.               */
                                               CPU_INT32U             blk_nbr_phy,
                                               FS_ERR                *p_err);

static  void         FSDev_NAND_PHY_IO_Ctrl   (FS_DEV_NAND_PHY_DATA  *p_phy_data,   /* Perform NAND device I/O control. */
                                               CPU_INT08U             opt,
                                               void                  *p_data,
                                               FS_ERR                *p_err);

                                                                                    /* ---------- LOCAL FNCTS --------- */
static  CPU_BOOLEAN  FSDev_NAND_PHY_PollFnct  (FS_DEV_NAND_PHY_DATA  *p_phy_data);  /* Poll fnct.                       */

/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_NAND_PHY_API  FSDev_NAND_STM32 = {
    FSDev_NAND_PHY_Open,
    FSDev_NAND_PHY_Close,
    FSDev_NAND_PHY_RdPage,
    FSDev_NAND_PHY_WrPage,
    FSDev_NAND_PHY_EraseBlk,
    FSDev_NAND_PHY_IsBlkBad,
    FSDev_NAND_PHY_MarkBlkBad,
    FSDev_NAND_PHY_IO_Ctrl
};


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                              NAND PHYSICAL DRIVER INTERFACE FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        FSDev_NAND_PHY_Open()
*
* Description : Open (initialize) a NAND device instance & get NAND device information.
*
* Argument(s) : p_phy_data  Pointer to NAND phy data.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               FS_ERR_NONE                    Device driver initialized successfully.
*                               FS_ERR_DEV_INVALID_CFG         Device configuration specified invalid.
*                               FS_ERR_DEV_INVALID_UNIT_NBR    Device unit number is invalid.
*                               FS_ERR_DEV_IO                  Device I/O error.
*                               FS_ERR_DEV_TIMEOUT             Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_NAND_Open().
*
* Note(s)     : (1) Several members of 'p_phy_data' may need to be used/assigned :
*
*                   (a) 'BlkCnt' & 'BlkSize' MUST be assigned the block count & block size of the device,
*                       respectively.  A block is the device erase unit, e.g., the smallest area of the
*                       device that can be erased at any time.
*
*                   (b) 'PageSize' MUST be assigned the page size of the device.  A page is the device
*                       program unit, i.e., the smallest area of the device that can be programmed at any
*                       time.  Since some NAND flash allow multiple program operations per physical page
*                       between erases, 'PageSize' may be a factor of the physical page size; specifically,
*                       it may be equal to the physical page size divided by the number of partial page
*                       program operations allowed.
*
*                       (1) 'BlkSize' MUST be a multiple of 'PageSize'.
*                       (2) 'PageSize' MUST be a multiple of 'SecSize'.
*                       (3) Each page must have a room for two bytes data per each sector in page.
*
*                   (c) 'DataPtr' may store a pointer to any driver-specific data.
*
*                   (d) 'UnitNbr' is the unit number of the NAND device.
*
*                   (d) 'BusWidth' specifies the bus width, in bits.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_Open (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                   FS_ERR                *p_err)
{
    CPU_INT08U   addr;
    CPU_INT32U   blk_cnt;
    CPU_INT32U   blk_size;
    CPU_INT08U   cmd;
    CPU_INT08U   desc_nbr;
    CPU_INT32U   dev_size;
    CPU_BOOLEAN  found;
    CPU_INT08U   id_dev;
    CPU_INT08U   id[2];
    CPU_BOOLEAN  ok;
    CPU_INT32U   page_cnt;


                                                                        /* ---------------- CHK PARAMS ---------------- */
    if (p_phy_data->BusWidth != 8u) {                                   /* Validate bus width.                          */
       *p_err = FS_ERR_DEV_INVALID_CFG;
        return;
    }



                                                                        /* ------------------ INIT HW ----------------- */
    p_phy_data->BlkCnt          =  0u;                                  /* Clr phy info.                                */
    p_phy_data->BlkSize         =  0u;
    p_phy_data->PageSize        =  0u;
    p_phy_data->PagesPerPhyPage =  0u;
    p_phy_data->SpareSize       =  0u;
    p_phy_data->DataPtr         = (void *)0;

    ok = FSDev_NAND_BSP_Open(p_phy_data->UnitNbr,
                             p_phy_data->BusWidth);
    if (ok != DEF_OK) {                                                 /* If HW could not be init'd ...                */
       *p_err = FS_ERR_DEV_INVALID_UNIT_NBR;                            /* ... rtn err.                                 */
        return;
    }

    STM32_ECC_Init();                                                   /* Init ECC generator.                          */



                                                                        /* ----------------- RESET DEV ---------------- */
    cmd = FS_DEV_NAND_PHY_CMD_RESET;
    FSDev_NAND_BSP_WrCmd(p_phy_data->UnitNbr, &cmd, 1u);

    FS_OS_Dly_ms(1u);



                                                                        /* ------------------ RD SIG ------------------ */
    cmd  = FS_DEV_NAND_PHY_CMD_RDID;
    addr = 0u;
    FSDev_NAND_BSP_ChipSelEn(p_phy_data->UnitNbr);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd,   1u);             /* Wr cmd.                                      */
    FSDev_NAND_BSP_WrAddr(p_phy_data->UnitNbr, &addr,  1u);             /* Wr addr (0x00).                              */
    FSDev_NAND_BSP_RdData(p_phy_data->UnitNbr, &id[0], sizeof(id));     /* Rd ID.                                       */
    FSDev_NAND_BSP_ChipSelDis(p_phy_data->UnitNbr);



                                                                        /* -------------- VALIDATE DEV ID ------------- */
    id_dev   = id[1];

    desc_nbr = 0u;                                                      /* Lookup dev params in dev tbl.                */
    found    = DEF_NO;
    while ((FSDev_NAND_PHY_DevTbl[desc_nbr].Size_Mb != 0u) && (found == DEF_NO)) {
        if (FSDev_NAND_PHY_DevTbl[desc_nbr].DevID == id_dev) {
            found = DEF_YES;
        } else {
            desc_nbr++;
        }
    }

    if (found == DEF_NO) {
        FS_TRACE_DBG(("NAND PHY 512 x08: Unrecognized dev ID: 0x%02X.\r\n", id_dev));
       *p_err = FS_ERR_DEV_IO;
        return;
    }

    if (FSDev_NAND_PHY_DevTbl[desc_nbr].BusWidth != 8u) {
        FS_TRACE_DBG(("NAND PHY 512 x08: NOT 8-bit dev ID: 0x%02X.\r\n", id_dev));
       *p_err = FS_ERR_DEV_IO;
        return;
    }



                                                                        /* --------------- SAVE PHY INFO -------------- */
    blk_size =  FS_DEV_NAND_PHY_PAGE_SIZE * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    dev_size = (FSDev_NAND_PHY_DevTbl[desc_nbr].Size_Mb * 1024u / 8u);
    page_cnt = (dev_size / FS_DEV_NAND_PHY_PAGE_SIZE) * 1024u;
    blk_cnt  =  page_cnt / FS_DEV_NAND_PHY_PAGES_PER_BLK;

    FS_TRACE_DBG(("NAND PHY 512 x08: Dev size: %d kB\r\n",  dev_size));
    FS_TRACE_DBG(("                  Manuf ID: 0x%02X\r\n", id[0]));
    FS_TRACE_DBG(("                  Dev   ID: 0x%02X\r\n", id_dev));
    FS_TRACE_DBG(("                  Blk cnt : %d\r\n",     blk_cnt));
    FS_TRACE_DBG(("                  Blk size: %d\r\n",     blk_size));

    p_phy_data->BlkCnt          =  blk_cnt;
    p_phy_data->BlkSize         =  blk_size;
    p_phy_data->PageSize        =  FS_DEV_NAND_PHY_PAGE_SIZE;
    p_phy_data->PagesPerPhyPage =  1u;
    p_phy_data->SpareSize       =  FS_DEV_NAND_PHY_SPARE_SIZE;
    p_phy_data->DataPtr         = (void *)0;

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_PHY_Close()
*
* Description : Close (uninitialize) a NAND device instance.
*
* Argument(s) : p_phy_data  Pointer to NAND phy information.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_NAND_Close().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_Close (FS_DEV_NAND_PHY_DATA  *p_phy_data)
{
                                                                        /* ----------------- CLOSE DEV ---------------- */
    FSDev_NAND_BSP_Close(p_phy_data->UnitNbr);
    p_phy_data->BlkCnt          =  0u;                                  /* Clr phy info.                                */
    p_phy_data->BlkSize         =  0u;
    p_phy_data->PageSize        =  0u;
    p_phy_data->PagesPerPhyPage =  0u;
    p_phy_data->SpareSize       =  0u;
    p_phy_data->DataPtr         = (void *)0;
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_PHY_RdPage()
*
* Description : Read page from a NAND device & store data in buffer.
*
* Argument(s) : p_phy_data      Pointer to NAND phy data.
*
*               p_dest          Pointer to destination buffer (see Note #2a).
*
*               p_dest_spare    Pointer to destination spare buffer (see Note #2b).
*
*               page_nbr_phy    Page to read.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   FS_ERR_NONE                Page read successfully.
*                                   FS_ERR_DEV_INVALID_ECC     Invalid ECC.
*                                   FS_ERR_DEV_INVALID_MARK    Invalid marker.
*                                   FS_ERR_DEV_IO              Device I/O error.
*                                   FS_ERR_DEV_TIMEOUT         Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_NAND_PhyRdPageHandler().
*
* Note(s)     : (1) The address is a 3- or 4-byte value, depending on the number of address bits necessary
*                   to specify the highest address on the NAND device.
*
*                  ----------------------------------------------------------------------
*                  | BUS CYCLE  | I/O7 | I/O6 | I/O5 | I/O4 | I/O3 | I/O2 | I/O1 | I/O0 |
*                  ----------------------------------------------------------------------
*                  |    1st     |  A7  |  A6  |  A5  |  A4  |  A3  |  A2  |  A1  |  A0  |
*                  ----------------------------------------------------------------------
*                  |    2nd     |  A16 |  A15 |  A14 |  A13 |  A12 |  A11 |  A10 |  A9  |
*                  ----------------------------------------------------------------------
*                  |    3rd     |  A24 |  A23 |  A22 |  A21 |  A20 |  A19 |  A18 |  A17 |
*                  ----------------------------------------------------------------------
*                  |    4th     |  --- |  --- |  --- |  --- |  --- |  A27 |  A26 |  A25 |
*                  ----------------------------------------------------------------------
*
*                   (a) The 4th byte is only necessary for devices with more that 65536 pages, devices
*                       LARGER than
*
*                           512 * 65535 = 32 MB = 256 Mb
*
*                   (b) A7..A0 gives the byte address within the page.  For x8 devices, this can
*                       only reach the low half of each page.  A8 is essentially specified 0/1 with the
*                       RDA/RDB commands; the latter starts the access pointer at the second half of the
*                       page, which can then be read or programmed.
*
*                   (c) Axx..A9 gives the page address.
*
*                   (d) Axx..A14 gives the block address.
*
*               (2) The spare area associated with each sector is occupied with an ECC covering the data,
*                   data from the high-level driver & an ECC covering the spare area.
*
*                  -------------------------------------------------------------------------------------------------
*                  |          ECC          |    BAD    |          ECC          |   DRIVER  |   SPARE   |   SPARE   |
*                  |     (1st 256 BYTES)   |    BLK    |     (2nd 256 BYTES)   |    DATA   |   MARKER  |    ECC    |
*                  -------------------------------------------------------------------------------------------------
*
*                   (a) The page data is still returned in the buffer when an ECC error is detected or
*                       if the spare marker is invalid.
*
*                   (b) 'p_dest_spare' will be written two bytes driver data for each sector in the page.
*
*               (3) After the read command is issued, the NAND begins transferring a page of data to its
*                   buffer.  When the NAND becomes ready (by setting the 6th bit of the status register),
*                   the data can be read.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_RdPage (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                     void                  *p_dest,
                                     void                  *p_dest_spare,
                                     CPU_INT32U             page_nbr_phy,
                                     FS_ERR                *p_err)
{
    CPU_INT08U    addr[4];
    CPU_SIZE_T    addr_len;
    CPU_INT08U    cmd;
    CPU_INT08U   *p_dest_08;
    CPU_INT08U   *p_dest_spare_08;
    CPU_ERR       ecc_err;
    CPU_INT32U    ecc1;
    CPU_INT32U    ecc2;
    CPU_INT32U    ecc1_chk;
    CPU_INT32U    ecc2_chk;
    CPU_INT16U    mark;
    CPU_BOOLEAN   ok;
    CPU_INT08U    spare[FS_DEV_NAND_PHY_SPARE_SIZE];


                                                                        /* ----------------- SETUP CMD ---------------- */
    addr_len = (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) ? 4u : 3u; /* Compute addr len.                        */
    cmd      =  FS_DEV_NAND_PHY_CMD_RDA;

    addr[0]  =  0x00u;
    addr[1]  = (CPU_INT08U)(page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[2]  = (CPU_INT08U)(page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[3]  = (CPU_INT08U)(page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;



                                                                        /* ----------------- EXEC CMD ----------------- */
    FSDev_NAND_BSP_ChipSelEn(p_phy_data->UnitNbr);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd,     1u);           /* Wr cmd.                                      */
    FSDev_NAND_BSP_WrAddr(p_phy_data->UnitNbr, &addr[0], addr_len);     /* Wr addr.                                     */



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    ok = FSDev_NAND_BSP_WaitWhileBusy(p_phy_data->UnitNbr,              /* Wait while busy ...                          */
                                      p_phy_data,
                                      FSDev_NAND_PHY_PollFnct,
                                      FS_DEV_NAND_PHY_MAX_RD_us);
    if (ok != DEF_OK) {                                                 /*                 ... if busy ...              */
       *p_err = FS_ERR_DEV_IO;
        return;                                                         /*                             ... rtn err.     */
    }



                                                                        /* ------------------ RD DATA ----------------- */
    cmd = 0x00u;
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd,       1u);

    p_dest_08  = (CPU_INT08U *)p_dest;
    STM32_ECC_Start();                                                  /* Start ECC calculation.                       */
    FSDev_NAND_BSP_RdData(p_phy_data->UnitNbr,  p_dest_08, 256u);       /* Rd 1st half.                                 */
    ecc1       = STM32_ECC_Get();                                       /* Get ECC.                                     */

    p_dest_08 += 256u;
    STM32_ECC_Start();                                                  /* Start ECC calculation.                       */
    FSDev_NAND_BSP_RdData(p_phy_data->UnitNbr,  p_dest_08, 256u);       /* Rd 2nd half.                                 */
    ecc2       = STM32_ECC_Get();                                       /* Get ECC.                                     */

    FSDev_NAND_BSP_RdData(p_phy_data->UnitNbr, &spare[0], FS_DEV_NAND_PHY_SPARE_SIZE);
    FSDev_NAND_BSP_ChipSelDis(p_phy_data->UnitNbr);



                                                                        /* ------------------ CHK ECC ----------------- */
    p_dest_08       = (CPU_INT08U *)p_dest;
    p_dest_spare_08 = (CPU_INT08U *)p_dest_spare;

    Hamming_Correct_004(&spare[FS_DEV_NAND_PHY_OFFSET_DATA],            /* Correct any errs in spare data ...           */
                         4u,
                        &spare[FS_DEV_NAND_PHY_OFFSET_ECC_DATA],
                        &ecc_err);
    if (ecc_err == ECC_ERR_UNCORRECTABLE) {                             /* ... if uncorrectable err       ...           */
       *p_err = FS_ERR_DEV_INVALID_ECC;                                 /* ... rtn err.                                 */
        return;
    }
    MEM_VAL_COPY_GET_INT16U((void *)p_dest_spare_08, (void *)&spare[FS_DEV_NAND_PHY_OFFSET_DATA]);

                                                                        /* Chk mark            ...                      */
    mark = MEM_VAL_GET_INT16U((void *)&spare[FS_DEV_NAND_PHY_OFFSET_MARK]);
    if (mark != FS_DEV_NAND_PHY_MARK) {                                 /* ... if mark invalid ...                      */
        *p_err = FS_ERR_DEV_INVALID_MARK;                               /* ... rtn err.                                 */
        return;
    }

    p_dest_08 = (CPU_INT08U *)p_dest;
    ecc1_chk  = MEM_VAL_GET_INT32U((void *)&spare[FS_DEV_NAND_PHY_OFFSET_ECC1]);
    ecc_err   = STM32_ECC_Correct(ecc1,                                 /* Correct any errs in 1st half ...             */
                                  ecc1_chk,
                                  p_dest_08);
    if (ecc_err == ECC_ERR_UNCORRECTABLE) {                             /* ... if uncorrectable err     ...             */
       *p_err = FS_ERR_DEV_INVALID_ECC;                                 /* ... rtn err.                                 */
        return;
    }

    p_dest_08 += 256u;
    ecc2_chk   = MEM_VAL_GET_INT32U((void *)&spare[FS_DEV_NAND_PHY_OFFSET_ECC2]);
    ecc_err    = STM32_ECC_Correct(ecc2,                                /* Correct any errs in 1st half ...             */
                                   ecc2_chk,
                                   p_dest_08);
    if (ecc_err == ECC_ERR_UNCORRECTABLE) {                             /* ... if uncorrectable err     ...             */
       *p_err = FS_ERR_DEV_INVALID_ECC;                                 /* ... rtn err.                                 */
        return;
    }

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       FSDev_NAND_PHY_WrPage()
*
* Description : Write data to a NAND device page from a buffer.
*
* Argument(s) : p_phy_data      Pointer to NAND phy data.
*
*               p_src           Pointer to source buffer.
*
*               p_src_spare     Pointer to source spare buffer.
*
*               page_nbr_phy    Page to write.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   FS_ERR_NONE           Page written successfully.
*                                   FS_ERR_DEV_IO         Device I/O error.
*                                   FS_ERR_DEV_TIMEOUT    Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_NAND_PhyWrPageHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_WrPage (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                     void                  *p_src,
                                     void                  *p_src_spare,
                                     CPU_INT32U             page_nbr_phy,
                                     FS_ERR                *p_err)
{
    CPU_INT08U    addr[4];
    CPU_SIZE_T    addr_len;
    CPU_INT08U    cmd1;
    CPU_INT08U    cmd2;
    CPU_ERR       ecc_err;
    CPU_INT32U    ecc;
    CPU_BOOLEAN   ok;
    CPU_INT08U   *p_src_08;
    CPU_INT08U   *p_src_spare_08;
    CPU_INT08U    spare[FS_DEV_NAND_PHY_SPARE_SIZE];


                                                                        /* ----------------- SETUP CMD ---------------- */
    addr_len = (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) ? 4u : 3u; /* Compute addr len.                        */
    cmd1     =  FS_DEV_NAND_PHY_CMD_PAGEPGM_SETUP;

    addr[0]  =  0x00u;
    addr[1]  = (CPU_INT08U)(page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[2]  = (CPU_INT08U)(page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[3]  = (CPU_INT08U)(page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;

    cmd2     =  FS_DEV_NAND_PHY_CMD_PAGEPGM_CONFIRM;



                                                                        /* ----------------- CALC ECC ----------------- */
    p_src_08       = (CPU_INT08U *)p_src;
    p_src_spare_08 = (CPU_INT08U *)p_src_spare;
    Mem_Set(&spare[0], 0xFFu, sizeof(spare));

    MEM_VAL_COPY_GET_INT16U((void *)&spare[FS_DEV_NAND_PHY_OFFSET_DATA], (void *)p_src_spare_08);
    MEM_VAL_SET_INT16U((void *)&spare[FS_DEV_NAND_PHY_OFFSET_MARK], FS_DEV_NAND_PHY_MARK);

    Hamming_Calc_004(&spare[FS_DEV_NAND_PHY_OFFSET_DATA],               /* Calc ECC for spare.                          */
                      4u,
                     &spare[FS_DEV_NAND_PHY_OFFSET_ECC_DATA],
                     &ecc_err);



                                                                        /* ----------------- EXEC CMD ----------------- */
    FSDev_NAND_BSP_ChipSelEn(p_phy_data->UnitNbr);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd1,     1u);          /* Wr cmd.                                      */
    FSDev_NAND_BSP_WrAddr(p_phy_data->UnitNbr, &addr[0],  addr_len);    /* Wr addr.                                     */
                                                                        /* Wr data.                                     */

    p_src_08  = (CPU_INT08U *)p_src;
    STM32_ECC_Start();                                                  /* Start ECC calculation.                       */
    FSDev_NAND_BSP_WrData(p_phy_data->UnitNbr,  p_src_08, 256u);        /* Wr 1st half.                                 */
    ecc       = STM32_ECC_Get();                                        /* Get ECC.                                     */
    MEM_VAL_SET_INT32U((void *)&spare[FS_DEV_NAND_PHY_OFFSET_ECC1], ecc);

    p_src_08 += 256u;
    STM32_ECC_Start();                                                  /* Start ECC calculation.                       */
    FSDev_NAND_BSP_WrData(p_phy_data->UnitNbr,  p_src_08, 256u);        /* Wr 2nd half.                                 */
    ecc       = STM32_ECC_Get();                                        /* Get ECC.                                     */
    MEM_VAL_SET_INT32U((void *)&spare[FS_DEV_NAND_PHY_OFFSET_ECC2], ecc);

    FSDev_NAND_BSP_WrData(p_phy_data->UnitNbr, &spare[0], FS_DEV_NAND_PHY_SPARE_SIZE);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd2,     1u);          /* Wr cmd confirm.                              */



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    ok = FSDev_NAND_BSP_WaitWhileBusy(p_phy_data->UnitNbr,              /* Wait while busy ...                          */
                                      p_phy_data,
                                      FSDev_NAND_PHY_PollFnct,
                                      FS_DEV_NAND_PHY_MAX_PGM_us);
    FSDev_NAND_BSP_ChipSelDis(p_phy_data->UnitNbr);
    if (ok != DEF_OK) {                                                 /*                 ... if busy ...              */
       *p_err = FS_ERR_DEV_IO;
        return;                                                         /*                             ... rtn err.     */
    }

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      FSDev_NAND_PHY_EraseBlk()
*
* Description : Erase block of NAND device.
*
* Argument(s) : p_phy_data      Pointer to NAND phy data.
*
*               blk_nbr_phy     Block to erase.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   FS_ERR_NONE           Block erased successfully.
*                                   FS_ERR_DEV_IO         Device I/O error.
*                                   FS_ERR_DEV_TIMEOUT    Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : FSDev_NAND_PhyEraseBlkHandler().
*
* Note(s)     : (1) The block address is a 2- or 3-byte value, depending on the number of bits necessary
*                   to specify the address of the highest page.  It is the upper bytes of the full address;
*                   see 'FSDev_NAND_PHY_RdPage()  Note #1'.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_EraseBlk (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                       CPU_INT32U             blk_nbr_phy,
                                       FS_ERR                *p_err)
{
    CPU_INT08U   addr[3];
    CPU_SIZE_T   addr_len;
    CPU_INT08U   cmd1;
    CPU_INT08U   cmd2;
    CPU_BOOLEAN  ok;
    CPU_INT32U   page_nbr_phy;


                                                                        /* ----------------- SETUP CMD ---------------- */
    addr_len     = (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) ? 3u : 2u; /* Compute addr len.                    */
    cmd1         =  FS_DEV_NAND_PHY_CMD_BLKERASE_SETUP;

    page_nbr_phy =  blk_nbr_phy * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    addr[0]      = (CPU_INT08U)(page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[1]      = (CPU_INT08U)(page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[2]      = (CPU_INT08U)(page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;

    cmd2         =  FS_DEV_NAND_PHY_CMD_BLKERASE_CONFIRM;



                                                                        /* ----------------- EXEC CMD ----------------- */
    FSDev_NAND_BSP_ChipSelEn(p_phy_data->UnitNbr);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd1,    1u);           /* Wr cmd.                                      */
    FSDev_NAND_BSP_WrAddr(p_phy_data->UnitNbr, &addr[0], addr_len);     /* Wr addr.                                     */
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd2,    1u);           /* Wr cmd confirm.                              */



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    ok = FSDev_NAND_BSP_WaitWhileBusy(p_phy_data->UnitNbr,              /* Wait while busy ...                          */
                                      p_phy_data,
                                      FSDev_NAND_PHY_PollFnct,
                                      FS_DEV_NAND_PHY_MAX_BLK_ERASE_us);
    FSDev_NAND_BSP_ChipSelDis(p_phy_data->UnitNbr);
    if (ok != DEF_OK) {                                                 /*                 ... if busy ...              */
       *p_err = FS_ERR_DEV_IO;
        return;                                                         /*                             ... rtn err.     */
    }

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      FSDev_NAND_PHY_IsBlkBad()
*
* Description : Determine whether a NAND device block is bad.
*
* Argument(s) : p_phy_data      Pointer to NAND phy data.
*
*               blk_nbr_phy     Block to investigate.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   FS_ERR_NONE           Block status investigated successfully.
*                                   FS_ERR_DEV_IO         Device I/O error.
*                                   FS_ERR_DEV_TIMEOUT    Device timeout.
*
* Return(s)   : DEF_NO,  if block is NOT bad.
*               DEF_YES, otherwise.
*
* Caller(s)   : ????
*
* Note(s)     : (1) The sixth byte of the spare area of the first page of any bad block will be NOT 0xFF.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FSDev_NAND_PHY_IsBlkBad (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                              CPU_INT32U             blk_nbr_phy,
                                              FS_ERR                *p_err)
{
    CPU_INT08U   addr[4];
    CPU_INT08U   addr_len;
    CPU_BOOLEAN  bad;
    CPU_INT08U   cmd;
    CPU_BOOLEAN  ok;
    CPU_INT32U   page_nbr_phy;
    CPU_INT08U   spare[FS_DEV_NAND_PHY_SPARE_SIZE];


                                                                        /* ----------------- SETUP CMD ---------------- */
    addr_len     = (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) ? 4u : 3u; /* Compute addr len.                    */

    page_nbr_phy =  blk_nbr_phy * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    cmd          =  FS_DEV_NAND_PHY_CMD_RDC;
    addr[0]      =  0x00u;
    addr[1]      = (CPU_INT08U)(page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[2]      = (CPU_INT08U)(page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[3]      = (CPU_INT08U)(page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;



                                                                        /* ----------------- EXEC CMD ----------------- */
    FSDev_NAND_BSP_ChipSelEn(p_phy_data->UnitNbr);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd,     1u);           /* Wr cmd.                                      */
    FSDev_NAND_BSP_WrAddr(p_phy_data->UnitNbr, &addr[0], addr_len);     /* Wr addr.                                     */



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    ok = FSDev_NAND_BSP_WaitWhileBusy(p_phy_data->UnitNbr,              /* Wait while busy ...                          */
                                      p_phy_data,
                                      FSDev_NAND_PHY_PollFnct,
                                      FS_DEV_NAND_PHY_MAX_RD_us);
    if (ok != DEF_OK) {                                                 /*                 ... if busy ...              */
       *p_err = FS_ERR_DEV_IO;
        return (DEF_NO);                                                /*                             ... rtn err.     */
    }
    cmd = 0x00u;
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd,      1u);
    FSDev_NAND_BSP_RdData(p_phy_data->UnitNbr, &spare[0], FS_DEV_NAND_PHY_SPARE_SIZE);
    FSDev_NAND_BSP_ChipSelDis(p_phy_data->UnitNbr);

    bad   = (spare[FS_DEV_NAND_PHY_OFFSET_BAD_BLK_MARK] == 0xFFu) ? DEF_NO : DEF_YES;
   *p_err = FS_ERR_NONE;
    return (bad);
}


/*
*********************************************************************************************************
*                                     FSDev_NAND_PHY_MarkBlkBad()
*
* Description : Mark a NAND device block as bad.
*
* Argument(s) : p_phy_data      Pointer to NAND phy data.
*
*               blk_nbr_phy     Block to mark.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   FS_ERR_NONE           Block marked successfully.
*                                   FS_ERR_DEV_IO         Device I/O error.
*                                   FS_ERR_DEV_TIMEOUT    Device timeout.
*
* Return(s)   : none
*
* Caller(s)   : ????
*
* Note(s)     : (1) The entire spare area is cleared.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_MarkBlkBad (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                         CPU_INT32U             blk_nbr_phy,
                                         FS_ERR                *p_err)
{
    CPU_INT08U   addr[4];
    CPU_INT08U   addr_len;
    CPU_INT08U   cmd1;
    CPU_INT08U   cmd2;
    CPU_INT08U   cmd3;
    CPU_BOOLEAN  ok;
    CPU_INT32U   page_nbr_phy;
    CPU_INT08U   spare[FS_DEV_NAND_PHY_SPARE_SIZE];


                                                                        /* ----------------- SETUP CMD ---------------- */
    addr_len     = (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) ? 4u : 3u; /* Compute addr len.                    */

    page_nbr_phy =  blk_nbr_phy * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    cmd1         =  FS_DEV_NAND_PHY_CMD_RDC;
    cmd2         =  FS_DEV_NAND_PHY_CMD_PAGEPGM_SETUP;

    addr[0]      =  0x00u;
    addr[1]      = (CPU_INT08U)(page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[2]      = (CPU_INT08U)(page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    addr[3]      = (CPU_INT08U)(page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;

    cmd3         =  FS_DEV_NAND_PHY_CMD_PAGEPGM_CONFIRM;

    Mem_Clr(&spare[0], FS_DEV_NAND_PHY_SPARE_SIZE);



                                                                        /* ----------------- EXEC CMD ----------------- */
    FSDev_NAND_BSP_ChipSelEn(p_phy_data->UnitNbr);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd1,     1u);          /* Wr cmd setup.                                */
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd2,     1u);          /* Wr cmd.                                      */
    FSDev_NAND_BSP_WrAddr(p_phy_data->UnitNbr, &addr[0],  addr_len);    /* Wr addr.                                     */
                                                                        /* Wr data.                                     */
    FSDev_NAND_BSP_WrData(p_phy_data->UnitNbr, &spare[0], FS_DEV_NAND_PHY_SPARE_SIZE);
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd3,     1u);          /* Wr cmd confirm.                              */



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    ok = FSDev_NAND_BSP_WaitWhileBusy(p_phy_data->UnitNbr,              /* Wait while busy ...                          */
                                      p_phy_data,
                                      FSDev_NAND_PHY_PollFnct,
                                      FS_DEV_NAND_PHY_MAX_PGM_us);
    FSDev_NAND_BSP_ChipSelDis(p_phy_data->UnitNbr);
    if (ok != DEF_OK) {                                                 /*                 ... if busy ...              */
       *p_err = FS_ERR_DEV_IO;
        return;                                                         /*                             ... rtn err.     */
    }

   *p_err = FS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      FSDev_NAND_PHY_IO_Ctrl()
*
* Description : Perform NAND device I/O control operation.
*
* Argument(s) : p_phy_data  Pointer to NAND phy data.
*
*               opt         Control command.
*
*               p_data      Buffer which holds data to be used for operation.
*                              OR
*                           Buffer in which data will be stored as a result of operation.
*
*               p_err       Pointer to variable that will receive the return the error code from this function :
*
*                               FS_ERR_NONE                   Control operation performed successfully.
*                               FS_ERR_DEV_INVALID_IO_CTRL    I/O control operation unknown to driver.
*                               FS_ERR_DEV_INVALID_OP         Invalid operation for device.
*                               FS_ERR_DEV_IO                 Device I/O error.
*                               FS_ERR_DEV_TIMEOUT            Device timeout.
*
* Return(s)   : none.
*
* Caller(s)   : ????
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_IO_Ctrl (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                      CPU_INT08U             opt,
                                      void                  *p_data,
                                      FS_ERR                *p_err)
{
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
*                                      FSDev_NAND_PHY_PollFnct()
*
* Description : Poll function.
*
* Argument(s) : p_phy_data  Pointer to NAND phy data.
*
* Return(s)   : DEF_YES, if operation is complete or error occurred.
*               DEF_NO,  otherwise.
*
* Caller(s)   : FSDev_NAND_BSP_WaitWhileBusy().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FSDev_NAND_PHY_PollFnct (FS_DEV_NAND_PHY_DATA  *p_phy_data)
{
    CPU_INT08U   cmd;
    CPU_BOOLEAN  rdy;
    CPU_INT08U   sr;


    cmd = FS_DEV_NAND_PHY_CMD_RDSTATUS;
    FSDev_NAND_BSP_WrCmd( p_phy_data->UnitNbr, &cmd, 1u);
    FSDev_NAND_BSP_RdData(p_phy_data->UnitNbr, &sr,  1u);               /* Rd status reg.                               */
    rdy = DEF_BIT_IS_SET(sr, FS_DEV_NAND_PHY_SR_BUSY);

    return (rdy);
}
