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
*                                  LPC3XXX MLC NAND FLASH CONTROLLER
*                                        WITH SMALL-PAGE NAND
*
* Filename      : fs_dev_nand_lpc3xxx.c
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    FS_DEV_NAND_LPC3XXX_MODULE
#include  <fs_dev_nand_lpc3xxx.h>


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

#define  FS_DEV_NAND_PHY_OFFSET_DATA                       0u
#define  FS_DEV_NAND_PHY_OFFSET_MARK                       2u
#define  FS_DEV_NAND_PHY_OFFSET_BAD_BLK_MARK               5u

#define  FS_DEV_NAND_PHY_BAD_BLK_MARK                   0xFFu
#define  FS_DEV_NAND_PHY_MARK_LO                        0xA5u
#define  FS_DEV_NAND_PHY_MARK_HI                        0x5Au

/*
*********************************************************************************************************
*                                          REGISTER DEFINES
*********************************************************************************************************
*/

#define  LPC3XXX_REG_MLC_BUFF_ADDR                             0x200A8000u  /* MLC NAND Data Buffer.                    */
#define  LPC3XXX_REG_MLC_DATA_ADDR                             0x200B0000u  /* Start of MLC data buffer.                */
#define  LPC3XXX_REG_MLC_ADDR                   (*(CPU_REG32 *)0x200B8004u) /* MLC NAND Flash Address Register.         */
#define  LPC3XXX_REG_MLC_CMD                    (*(CPU_REG32 *)0x200B8000u) /* MLC NAND Flash Command Register.         */
#define  LPC3XXX_REG_MLC_ECC_ENC_REG            (*(CPU_REG32 *)0x200B8008u) /* MLC NAND ECC Encode Register.            */
#define  LPC3XXX_REG_MLC_ECC_DEC_REG            (*(CPU_REG32 *)0x200B800Cu) /* MLC NAND ECC Decode Register.            */
#define  LPC3XXX_REG_MLC_ECC_AUTO_ENC_REG       (*(CPU_REG32 *)0x200B8010u) /* MLC NAND ECC Auto Encode Register.       */
#define  LPC3XXX_REG_MLC_ECC_AUTO_DEC_REG       (*(CPU_REG32 *)0x200B8014u) /* MLC NAND ECC Auto Decode Register.       */
#define  LPC3XXX_REG_MLC_RPR                    (*(CPU_REG32 *)0x200B8018u) /* MLC NAND Read Parity Register.           */
#define  LPC3XXX_REG_MLC_WPR                    (*(CPU_REG32 *)0x200B801Cu) /* MLC NAND Write Parity Register.          */
#define  LPC3XXX_REG_MLC_RUBP                   (*(CPU_REG32 *)0x200B8020u) /* MLC NAND Reset User Buffer Ptr Reg.      */
#define  LPC3XXX_REG_MLC_ROBP                   (*(CPU_REG32 *)0x200B8024u) /* MLC NAND Reset Overhead Buffer Ptr Reg.  */
#define  LPC3XXX_REG_MLC_SW_WP_ADD_LOW          (*(CPU_REG32 *)0x200B8028u) /* MLC NAND SW Wr Protection Addr Lo Reg.   */
#define  LPC3XXX_REG_MLC_SW_WP_ADD_HIG          (*(CPU_REG32 *)0x200B802Cu) /* MLC NAND SW Wr Protection Addr Hi Reg.   */
#define  LPC3XXX_REG_MLC_ICR                    (*(CPU_REG32 *)0x200B8030u) /* MLC NAND Controller Configuration Reg.   */
#define  LPC3XXX_REG_MLC_TIME_REG               (*(CPU_REG32 *)0x200B8034u) /* MLC NAND Timing Register.                */
#define  LPC3XXX_REG_MLC_IRQ_MR                 (*(CPU_REG32 *)0x200B8038u) /* MLC NAND Interrupt Mask Register.        */
#define  LPC3XXX_REG_MLC_IRQ_SR                 (*(CPU_REG32 *)0x200B803Cu) /* MLC NAND Interrupt Status Register.      */
#define  LPC3XXX_REG_MLC_LOCK_PR                (*(CPU_REG32 *)0x200B8044u) /* MLC NAND Lock Protection Register.       */
#define  LPC3XXX_REG_MLC_ISR                    (*(CPU_REG32 *)0x200B8048u) /* MLC NAND Status Register.                */
#define  LPC3XXX_REG_MLC_CEH                    (*(CPU_REG32 *)0x200B804Cu) /* MLC NAND Chip-Enable Host Control Reg.   */

#define  LPC3XXX_REG_FLASHCLK_CTRL              (*(CPU_REG32 *)0x400040C8u)

/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/

#define  LPC3XXX_BIT_MLC_ICR_WRPROTECT          DEF_BIT_03      /* Software write protection enabled/disabled.          */
#define  LPC3XXX_BIT_MLC_ICR_LARGEBLK           DEF_BIT_02      /* Large/small block flash device.                      */
#define  LPC3XXX_BIT_MLC_ICR_WORDCNT            DEF_BIT_01      /* NAND flash address word count 4/3.                   */
#define  LPC3XXX_BIT_MLC_ICR_BUSWIDTH           DEF_BIT_00      /* NAND flash I/O bus with 16-bit/8-bit.                */

#define  LPC3XXX_BIT_MLC_INT_NANDRDY            DEF_BIT_05      /* NAND Ready.                                          */
#define  LPC3XXX_BIT_MLC_INT_CTLRRDY            DEF_BIT_04      /* Controller Ready.                                    */
#define  LPC3XXX_BIT_MLC_INT_DECODEFAIL         DEF_BIT_03      /* Decode failure.                                      */
#define  LPC3XXX_BIT_MLC_INT_DECODEERR          DEF_BIT_02      /* Decode error detected.                               */
#define  LPC3XXX_BIT_MLC_INT_ECCRDY             DEF_BIT_01      /* ECC Encode/Decode ready.                             */
#define  LPC3XXX_BIT_MLC_INT_WRPROTECT          DEF_BIT_00      /* Software write protection fault.                     */

#define  LPC3XXX_BIT_MLC_STATUS_DECODEFAIL      DEF_BIT_06      /* Decoder Failure.                                     */
#define  LPC3XXX_BIT_MLC_STATUS_ERRCNT_MASK     0x00000030u     /* Number of R/S symbols errors.                        */
#define  LPC3XXX_BIT_MLC_STATUS_DECODEERR       DEF_BIT_03      /* Errors detected.                                     */
#define  LPC3XXX_BIT_MLC_STATUS_ECCRDY          DEF_BIT_02      /* ECC ready.                                           */
#define  LPC3XXX_BIT_MLC_STATUS_CTRLRRDY        DEF_BIT_01      /* Controller ready.                                    */
#define  LPC3XXX_BIT_MLC_STATUS_NANDRDY         DEF_BIT_00      /* NAND ready.                                          */

#define  LPC3XXX_BIT_MLC_CEH_NORMAL             DEF_BIT_00      /* Normal nCE operation/Force nCE assert.               */

#define  LPC3XXX_BIT_FLASHCLK_CTRL_SLC_EN       DEF_BIT_00
#define  LPC3XXX_BIT_FLASHCLK_CTRL_MLC_EN       DEF_BIT_01
#define  LPC3XXX_BIT_FLASHCLK_CTRL_SEL          DEF_BIT_02
#define  LPC3XXX_BIT_FLASHCLK_CTRL_DMA_INT      DEF_BIT_03
#define  LPC3XXX_BIT_FLASHCLK_CTRL_DMA_RnB      DEF_BIT_04
#define  LPC3XXX_BIT_FLASHCLK_CTRL_INT_SEL      DEF_BIT_05

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

/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

const  FS_DEV_NAND_PHY_API  FSDev_NAND_LPC3XXX_MLC = {
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
    CPU_INT32U   blk_cnt;
    CPU_INT32U   blk_size;
    CPU_INT08U   desc_nbr;
    CPU_INT32U   dev_size;
    CPU_BOOLEAN  found;
    CPU_INT08U   id_dev;
    CPU_INT08U   id_manuf;
    CPU_INT08U   id[2];
    CPU_INT32U   page_cnt;
    CPU_INT32U   isr;


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

    LPC3XXX_REG_MLC_LOCK_PR   = 0xA25Eu;
    LPC3XXX_REG_MLC_ICR       = DEF_BIT_NONE;

    LPC3XXX_REG_MLC_LOCK_PR   = 0xA25Eu;
    LPC3XXX_REG_MLC_TIME_REG  = (3u <<  0) | (2u <<  4) | (3u <<  8) | (2u << 12)
                              | (4u << 16) | (2u << 19) | (5u << 24);

    LPC3XXX_REG_MLC_CEH       = LPC3XXX_BIT_MLC_CEH_NORMAL;

    LPC3XXX_REG_FLASHCLK_CTRL = LPC3XXX_BIT_FLASHCLK_CTRL_MLC_EN;



                                                                        /* ----------------- RESET DEV ---------------- */
    LPC3XXX_REG_MLC_CMD = FS_DEV_NAND_PHY_CMD_RESET;

    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_NANDRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ------------------ RD SIG ------------------ */
    LPC3XXX_REG_MLC_CMD  = FS_DEV_NAND_PHY_CMD_RDID;
    LPC3XXX_REG_MLC_ADDR = 0x00u;

    id[0] = *(CPU_REG08 *)LPC3XXX_REG_MLC_DATA_ADDR;
    id[1] = *(CPU_REG08 *)LPC3XXX_REG_MLC_DATA_ADDR;


                                                                        /* -------------- VALIDATE DEV ID ------------- */
    id_manuf = id[0];
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
        FS_TRACE_DBG(("NAND PHY LPC3XXX MLC: Unrecognized dev ID: 0x%02X.\r\n", id_dev));
       *p_err = FS_ERR_DEV_IO;
        return;
    }

    if (FSDev_NAND_PHY_DevTbl[desc_nbr].BusWidth != 8u) {
        FS_TRACE_DBG(("NAND PHY LPC3XXX MLC: NOT 8-bit dev ID: 0x%02X.\r\n", id_dev));
       *p_err = FS_ERR_DEV_IO;
        return;
    }

    if (FSDev_NAND_PHY_DevTbl[desc_nbr].Size_Mb > 1024u) {
        FS_TRACE_DBG(("NAND PHY LPC3XXX MLC: Dev = %d-Gb > 1-Gb: NOT supported.\r\n", FSDev_NAND_PHY_DevTbl[desc_nbr].Size_Mb / 1024u));
       *p_err = FS_ERR_DEV_IO;
        return;
    }



                                                                        /* --------------- SAVE PHY INFO -------------- */
    blk_size =  FS_DEV_NAND_PHY_PAGE_SIZE * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    dev_size = (FSDev_NAND_PHY_DevTbl[desc_nbr].Size_Mb * 1024u / 8u);
    page_cnt = (dev_size / FS_DEV_NAND_PHY_PAGE_SIZE) * 1024u;
    blk_cnt  =  page_cnt / FS_DEV_NAND_PHY_PAGES_PER_BLK;

    if (blk_cnt > FS_DEV_NAND_PHY_BLK_CNT_TH) {
        LPC3XXX_REG_MLC_LOCK_PR  = 0xA25Eu;
        LPC3XXX_REG_MLC_ICR     |= LPC3XXX_BIT_MLC_ICR_WORDCNT;
    }

    FS_TRACE_DBG(("NAND PHY LPC3XXX MLC: Dev size: %d kB\r\n",  dev_size));
    FS_TRACE_DBG(("                      Manuf ID: 0x%02X\r\n", id_manuf));
    FS_TRACE_DBG(("                      Dev   ID: 0x%02X\r\n", id_dev));
    FS_TRACE_DBG(("                      Blk cnt : %d\r\n",     blk_cnt));
    FS_TRACE_DBG(("                      Blk size: %d\r\n",     blk_size));

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
*               (2) The spare area associated with each sector is occupied with an ECC covering the data
*                   & data from the high-level driver.
*
*                  -------------------------------------------------------------------------------------------------
*                  |   DRIVER  |   SPARE   |    BAD    |                        CONTROLLER                         |
*                  |    DATA   |   MARKER  |    BLK    |                            ECC                            |
*                  -------------------------------------------------------------------------------------------------
*
*                   (a) The page data is still returned in the buffer when an ECC error is detected or
*                       if the spare marker is invalid.
*
*                   (b) 'p_dest_spare' will be written two bytes driver data for each sector in the page.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_RdPage (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                     void                  *p_dest,
                                     void                  *p_dest_spare,
                                     CPU_INT32U             page_nbr_phy,
                                     FS_ERR                *p_err)
{
    CPU_INT32U   isr;
    CPU_INT32U   ix;
    CPU_INT08U  *p_dest_08;
    CPU_INT08U  *p_dest_spare_08;
    CPU_INT08U   spare[6];


    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ----------------- EXEC CMD ----------------- */
    LPC3XXX_REG_MLC_CMD  =  FS_DEV_NAND_PHY_CMD_RDA;

    LPC3XXX_REG_MLC_ADDR =  0x00u;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    if (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) {
        LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    }


                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    LPC3XXX_REG_MLC_ECC_AUTO_DEC_REG = 0x00u;

    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }
    isr = LPC3XXX_REG_MLC_ISR;
    if (DEF_BIT_IS_SET(isr, LPC3XXX_BIT_MLC_STATUS_DECODEFAIL) == DEF_YES) {
       *p_err = FS_ERR_DEV_INVALID_ECC;
        return;
    }



                                                                        /* ------------------ RD DATA ----------------- */
    p_dest_08       = (CPU_INT08U *)p_dest;
    p_dest_spare_08 = (CPU_INT08U *)p_dest_spare;

    for (ix = 0u; ix < FS_DEV_NAND_PHY_PAGE_SIZE; ix++) {
        *p_dest_08 = *(CPU_REG08 *)LPC3XXX_REG_MLC_BUFF_ADDR;
         p_dest_08++;
    }

    for (ix = 0u; ix < sizeof(spare); ix++) {
        spare[ix] = *(CPU_REG08 *)LPC3XXX_REG_MLC_BUFF_ADDR;
    }

   *p_dest_spare_08 = spare[0];
    p_dest_spare_08++;
   *p_dest_spare_08 = spare[1];
    p_dest_spare_08++;

    if ((spare[2] != FS_DEV_NAND_PHY_MARK_LO) ||
        (spare[3] != FS_DEV_NAND_PHY_MARK_HI)) {
       *p_err = FS_ERR_DEV_INVALID_MARK;
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
    CPU_INT32U   isr;
    CPU_INT32U   ix;
    CPU_INT08U  *p_src_08;
    CPU_INT08U  *p_src_spare_08;
    CPU_INT08U   spare[6];
    CPU_INT08U   status;


    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ----------------- EXEC CMD ----------------- */
    LPC3XXX_REG_MLC_CMD  =  FS_DEV_NAND_PHY_CMD_PAGEPGM_SETUP;

    LPC3XXX_REG_MLC_ADDR =  0x00u;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    if (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) {
        LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    }

    LPC3XXX_REG_MLC_ECC_ENC_REG = 0x00u;



                                                                        /* ------------------ WR DATA ----------------- */
    Mem_Set((void *)&spare[0], 0xFFu, sizeof(spare));
    p_src_08       = (CPU_INT08U *)p_src;
    p_src_spare_08 = (CPU_INT08U *)p_src_spare;

    for (ix = 0u; ix < FS_DEV_NAND_PHY_PAGE_SIZE; ix++) {
        *(CPU_REG08 *)LPC3XXX_REG_MLC_BUFF_ADDR = *p_src_08;
         p_src_08++;
    }

    spare[0] = *p_src_spare_08;
    p_src_spare_08++;
    spare[1] = *p_src_spare_08;
    p_src_spare_08++;
    spare[2] =  FS_DEV_NAND_PHY_MARK_LO;
    spare[3] =  FS_DEV_NAND_PHY_MARK_HI;
    spare[4] =  FS_DEV_NAND_PHY_BAD_BLK_MARK;
    spare[5] =  FS_DEV_NAND_PHY_BAD_BLK_MARK;

    for (ix = 0u; ix < sizeof(spare); ix++) {
        *(CPU_REG08 *)LPC3XXX_REG_MLC_BUFF_ADDR = spare[ix];
    }

    LPC3XXX_REG_MLC_ECC_AUTO_ENC_REG = FS_DEV_NAND_PHY_CMD_PAGEPGM_CONFIRM | DEF_BIT_08;



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }

    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_NANDRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ----------------- RD STATUS ---------------- */
    LPC3XXX_REG_MLC_CMD = FS_DEV_NAND_PHY_CMD_RDSTATUS;
    status              = *(CPU_REG08 *)LPC3XXX_REG_MLC_DATA_ADDR;
    if (DEF_BIT_IS_SET(status, FS_DEV_NAND_PHY_SR_ERR) == DEF_YES) {
       *p_err = FS_ERR_DEV_IO;
        return;
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
    CPU_INT32U  isr;
    CPU_INT32U  page_nbr_phy;
    CPU_INT08U  status;


    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ----------------- EXEC CMD ----------------- */
    LPC3XXX_REG_MLC_CMD  =  FS_DEV_NAND_PHY_CMD_BLKERASE_SETUP;

    page_nbr_phy         =  blk_nbr_phy * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    if (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) {
        LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    }

    LPC3XXX_REG_MLC_CMD  =  FS_DEV_NAND_PHY_CMD_BLKERASE_CONFIRM;



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_NANDRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ----------------- RD STATUS ---------------- */
    LPC3XXX_REG_MLC_CMD = FS_DEV_NAND_PHY_CMD_RDSTATUS;
    status              = *(CPU_REG08 *)LPC3XXX_REG_MLC_DATA_ADDR;
    if (DEF_BIT_IS_SET(status, FS_DEV_NAND_PHY_SR_ERR) == DEF_YES) {
       *p_err = FS_ERR_DEV_IO;
        return;
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
    CPU_BOOLEAN  bad;
    CPU_INT08U   datum;
    CPU_INT32U   isr;
    CPU_INT32U   ix;
    CPU_INT32U   page_nbr_phy;
    CPU_INT08U   spare[6];


    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ----------------- EXEC CMD ----------------- */
    LPC3XXX_REG_MLC_CMD  =  FS_DEV_NAND_PHY_CMD_RDA;

    page_nbr_phy         =  blk_nbr_phy * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    LPC3XXX_REG_MLC_ADDR =  0x00u;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    if (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) {
        LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    }


                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_NANDRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }

    LPC3XXX_REG_MLC_ECC_DEC_REG = 0x00u;




                                                                        /* ------------------ RD DATA ----------------- */
    for (ix = 0u; ix < FS_DEV_NAND_PHY_PAGE_SIZE; ix++) {
         datum = *(CPU_REG08 *)LPC3XXX_REG_MLC_DATA_ADDR;
        (void)&datum;
    }

    for (ix = 0u; ix < sizeof(spare); ix++) {
        spare[ix] = *(CPU_REG08 *)LPC3XXX_REG_MLC_DATA_ADDR;
    }

    LPC3XXX_REG_MLC_RPR = 0x00u;

    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_ECCRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }

    bad = (spare[5] == FS_DEV_NAND_PHY_BAD_BLK_MARK) ? DEF_NO : DEF_YES;

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
* Note(s)     : (1) The entire first page of the bad block, including the user portion of the spare area,
*                   is cleared.
*********************************************************************************************************
*/

static  void  FSDev_NAND_PHY_MarkBlkBad (FS_DEV_NAND_PHY_DATA  *p_phy_data,
                                         CPU_INT32U             blk_nbr_phy,
                                         FS_ERR                *p_err)
{
    CPU_INT32U  isr;
    CPU_INT32U  ix;
    CPU_INT32U  page_nbr_phy;


    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }



                                                                        /* ----------------- EXEC CMD ----------------- */
    LPC3XXX_REG_MLC_CMD  =  FS_DEV_NAND_PHY_CMD_PAGEPGM_SETUP;

    page_nbr_phy         =  blk_nbr_phy * FS_DEV_NAND_PHY_PAGES_PER_BLK;
    LPC3XXX_REG_MLC_ADDR =  0x00u;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (0u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (1u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    if (p_phy_data->BlkCnt > FS_DEV_NAND_PHY_BLK_CNT_TH) {
        LPC3XXX_REG_MLC_ADDR = (page_nbr_phy >> (2u * DEF_OCTET_NBR_BITS)) & DEF_OCTET_MASK;
    }

    LPC3XXX_REG_MLC_ECC_ENC_REG = 0x00u;



                                                                        /* ------------------ WR DATA ----------------- */
    for (ix = 0u; ix < FS_DEV_NAND_PHY_PAGE_SIZE; ix++) {
        *(CPU_REG08 *)LPC3XXX_REG_MLC_BUFF_ADDR = 0x00u;
    }

    for (ix = 0u; ix < 6u; ix++) {
        *(CPU_REG08 *)LPC3XXX_REG_MLC_BUFF_ADDR = 0x00u;
    }

    LPC3XXX_REG_MLC_ECC_AUTO_ENC_REG = FS_DEV_NAND_PHY_CMD_PAGEPGM_CONFIRM | DEF_BIT_08;



                                                                        /* -------------- WAIT WHILE BUSY ------------- */
    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_CTRLRRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
    }

    isr = LPC3XXX_REG_MLC_ISR;
    while (DEF_BIT_IS_CLR(isr, LPC3XXX_BIT_MLC_STATUS_NANDRDY) == DEF_YES) {
        isr = LPC3XXX_REG_MLC_ISR;
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
