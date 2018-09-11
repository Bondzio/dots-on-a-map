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
*                         NAND DEVICE GENERIC CONTROLLER SOFTWARE ECC EXTENSION
*
* Filename      : fs_dev_nand_ctrlr_gen_soft_ecc.c
* Version       : v4.07.00
* Programmer(s) : EJ
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define   MICRIUM_SOURCE

#define   FS_NAND_CGEN_SOFT_ECC_MODULE
#include  <fs_dev_nand_ctrlr_gen_soft_ecc.h>

#include  <cpu.h>
#include  <ecc.h>


/*
*********************************************************************************************************
*                                                 MODULE
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  struct  soft_ecc_data  SOFT_ECC_DATA;
struct soft_ecc_data {
           FS_NAND_PG_SIZE   OOS_SizePerCodeword;               /* Size in octets of OOS area per codeword.             */
           CPU_INT08U        SizePerSec;                        /* Size in octets of ECC per sector.                    */
           CPU_INT08U        CodewordsPerSec;                   /* ECC codewords per sector.                            */
           FS_NAND_PG_SIZE   BufSizePerCodeword;                /* ECC codeword size floored to a divider of sec size.  */

    const  ECC_CALC         *ModulePtr;                         /* Pointer to ECC module.                               */

           SOFT_ECC_DATA    *NextPtr;
};


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

#define  FS_NAND_CTRLR_GEN_SOFT_ECC_CFG_FIELD(type, name, dflt_val)  dflt_val,
const  FS_NAND_CTRLR_GEN_SOFT_ECC_CFG  FS_NAND_CtrlrGen_SoftECC_DfltCfg = { FS_NAND_CTRLR_GEN_SOFT_ECC_CFG_FIELDS };
#undef   FS_NAND_CTRLR_GEN_SOFT_ECC_CFG_FIELD

SOFT_ECC_DATA  *FS_NAND_CtrlrGen_SoftECC_ListFreePtr;


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void              Init      (FS_ERR                  *p_err);

static  void             *Open      (FS_NAND_CTRLR_GEN_DATA  *p_ctrlr_data,
                                     void                    *p_ext_cfg,
                                     FS_ERR                  *p_err);

static  void              Close     (void                    *p_ext_data);

static  FS_NAND_PG_SIZE   Setup     (FS_NAND_CTRLR_GEN_DATA  *p_ctrlr_data,
                                     void                    *p_ext_data,
                                     FS_ERR                  *p_err);

static  void              ECC_Calc  (void                    *p_ext_data,
                                     void                    *p_sec_buf,
                                     void                    *p_oos_buf,
                                     FS_NAND_PG_SIZE          oos_size,
                                     FS_ERR                  *p_err);

static  void              ECC_Verify(void                    *p_ext_data,
                                     void                    *p_sec_buf,
                                     void                    *p_oos_buf,
                                     FS_NAND_PG_SIZE          oos_size,
                                     FS_ERR                  *p_err);

static  SOFT_ECC_DATA    *DataGet   (void);

static  void              DataFree  (SOFT_ECC_DATA           *p_ext_data);


/*
*********************************************************************************************************
*                               NAND GENERIC CTRLR SOFTWARE ECC EXTENSION
*********************************************************************************************************
*/

FS_NAND_CTRLR_GEN_EXT  FS_NAND_CtrlrGen_SoftECC =  {
    Init,                                                       /* Init().                                              */
    Open,                                                       /* Open().                                              */
    Close,                                                      /* Close().                                             */
    Setup,                                                      /* Setup().                                             */
    DEF_NULL,                                                   /* RdStatusChk().                                       */
    ECC_Calc,                                                   /* ECC_Calc().                                          */
    ECC_Verify,                                                 /* ECC_Verify().                                        */
};


/*
*********************************************************************************************************
*                                       LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                Init()
*
* Description : Initialize NAND generic controller software ECC extension.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*               -----   Argument validated by caller.
*
*                           FS_ERR_NONE    Extension module successfully initialized.
*
* Return(s)   : none.
*
* Caller(s)   : FS_NAND_CtrlrGen_SoftECC.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  Init (FS_ERR  *p_err)
{
    CPU_SR_ALLOC();


    (void)&p_err;
    CPU_CRITICAL_ENTER();
    FS_NAND_CtrlrGen_SoftECC_ListFreePtr = DEF_NULL;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                                Open()
*
* Description : Open extension module instance.
*
* Argument(s) : p_ctrlr_data    Pointer to NAND Controller Generic data structure.
*               ------------    Argument validated by caller.
*
*               p_ext_cfg       Pointer to software ECC extension configuration structure.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_MEM_ALLOC    Failed to allocate memory for extension data.
*                                   FS_ERR_NONE         Extension module instance successfully opened.
*
* Return(s)   : Pointer to extension data.
*
* Caller(s)   : FS_NAND_CtrlrGen_SoftECC.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  *Open (FS_NAND_CTRLR_GEN_DATA  *p_ctrlr_data,
                     void                    *p_ext_cfg,
                     FS_ERR                  *p_err)
{
    FS_NAND_CTRLR_GEN_SOFT_ECC_CFG  *p_soft_ecc_cfg;
    SOFT_ECC_DATA                   *p_soft_ecc_data;


    (void)&p_ctrlr_data;

#if (FS_CFG_ERR_ARG_CHK_EXT_EN == DEF_ENABLED)                  /* ------------------ VALIDATE ARGS ------------------- */
    if (p_ext_cfg == DEF_NULL) {                                /* Validate cfg ptr.                                    */
        CPU_SW_EXCEPTION(DEF_NULL;);
    }
#endif


    p_soft_ecc_cfg = (FS_NAND_CTRLR_GEN_SOFT_ECC_CFG *)p_ext_cfg;


    p_soft_ecc_data = DataGet();                                /* --------------- ALLOC AND INIT DATA ---------------- */
    if (p_soft_ecc_data == DEF_NULL) {
       *p_err = FS_ERR_MEM_ALLOC;
        return (DEF_NULL);
    }


    p_soft_ecc_data->ModulePtr = p_soft_ecc_cfg->ECC_ModulePtr;

    return (p_soft_ecc_data);
}


/*
*********************************************************************************************************
*                                                Close()
*
* Description : Close extension module instance.
*
* Argument(s) : p_ext_data  Pointer to extension data.
*               ----------  Argument validated by caller.
*
* Return(s)   : none.
*
* Caller(s)   : FS_NAND_CtrlrGen_SoftECC.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  Close (void  *p_ext_data)
{
    DataFree((SOFT_ECC_DATA *)p_ext_data);
}


/*
*********************************************************************************************************
*                                                Setup()
*
* Description : Setup software ECC extension module in accordance with generic controller's data.
*
* Argument(s) : p_ctrlr_data    Pointer to NAND generic controller data.
*               ------------    Argument validated by caller.
*
*               p_ext_data      Pointer to extension data.
*               ----------      Argument validated by caller.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*               -----           Argument validated by caller.
*
*                                   FS_ERR_DEV_INVALID_LOW_PARAMS   Invalid low level parameters.
*
* Return(s)   : Size required storage space for ECC per sector.
*
* Caller(s)   : FS_NAND_CtrlrGen_SoftECC.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  FS_NAND_PG_SIZE  Setup (FS_NAND_CTRLR_GEN_DATA  *p_ctrlr_data,
                                void                    *p_ext_data,
                                FS_ERR                  *p_err)
{
    SOFT_ECC_DATA  *p_soft_ecc_data;


    p_soft_ecc_data = (SOFT_ECC_DATA *)p_ext_data;


                                                                /* ------------------- VALIDATE ECC ------------------- */
    if (p_soft_ecc_data->ModulePtr->BufLenMax < p_ctrlr_data->PartDataPtr->ECC_CodewordSize) {
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen_soft_ecc) Setup: ECC module does not support the required codeword length.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
        return (FS_NAND_PG_IX_INVALID);
    }

    if (p_soft_ecc_data->ModulePtr->BufLenMin > p_ctrlr_data->PartDataPtr->ECC_CodewordSize) {
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen_soft_ecc) Setup: ECC module does not support the required codeword length.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
        return (FS_NAND_PG_IX_INVALID);
    }

    if (p_soft_ecc_data->ModulePtr->NbrCorrectableBits < p_ctrlr_data->PartDataPtr->ECC_NbrCorrBits) {
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen_soft_ecc) Setup: ECC module doesn't correct enough bits to support this device.\r\n"));
       *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
        return (FS_NAND_PG_IX_INVALID);
    }


                                                                /* -------------------- SETUP ECC --------------------- */
    p_soft_ecc_data->CodewordsPerSec = p_ctrlr_data->SecSize / p_ctrlr_data->PartDataPtr->ECC_CodewordSize;
    if (p_ctrlr_data->SecSize % p_ctrlr_data->PartDataPtr->ECC_CodewordSize != 0u) {
        p_soft_ecc_data->CodewordsPerSec += 1u;
    }

    p_soft_ecc_data->BufSizePerCodeword  = p_ctrlr_data->SecSize / p_soft_ecc_data->CodewordsPerSec;
    p_soft_ecc_data->OOS_SizePerCodeword = p_ctrlr_data->PartDataPtr->ECC_CodewordSize - p_soft_ecc_data->BufSizePerCodeword;

    if (p_soft_ecc_data->OOS_SizePerCodeword * p_soft_ecc_data->CodewordsPerSec < p_ctrlr_data->OOS_SizePerSec) {
        FS_TRACE_DBG(("(fs_nand_ctrlr_gen_soft_ecc) Setup: ECC setup doesn't protect all required OOS space (%uB on %uB protected).\r\n",
                      p_soft_ecc_data->OOS_SizePerCodeword * p_soft_ecc_data->CodewordsPerSec,
                      p_ctrlr_data->OOS_SizePerSec));
       *p_err = FS_ERR_DEV_INVALID_LOW_PARAMS;
        return (FS_NAND_PG_IX_INVALID);
    }

    p_soft_ecc_data->SizePerSec = p_soft_ecc_data->ModulePtr->ECC_Len * p_soft_ecc_data->CodewordsPerSec;


    return (p_soft_ecc_data->SizePerSec);
}


/*
*********************************************************************************************************
*                                              ECC_Calc()
*
* Description : Calculate ECC code words for sector and OOS data.
*
* Argument(s) : p_ext_data  Pointer to extension data.
*
*               p_sec_buf   Pointer to sector data.
*
*               p_oos_buf   Pointer to OOS data.
*
*               oos_size    Size of OOS data to protect in octets, excluding storage space for ECC code words.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               -----       Argument validated by caller.
*
*                               FS_ERR_DEV_INVALID_ECC  An error occured during ECC code words calculation.
*                               FS_ERR_NONE             ECC code words successfully calculated.
*
* Return(s)   : none.
*
* Caller(s)   : FS_NAND_CtrlrGen_SoftECC.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  ECC_Calc (void             *p_ext_data,
                        void             *p_sec_buf,
                        void             *p_oos_buf,
                        FS_NAND_PG_SIZE   oos_size,
                        FS_ERR           *p_err)
{
    SOFT_ECC_DATA    *p_soft_ecc_data;
    FS_NAND_PG_SIZE   oos_chk_size;
    FS_NAND_PG_SIZE   oos_chk_size_rem;
    CPU_INT08U       *p_sec_buf_08;
    CPU_INT08U       *p_oos_buf_08;
    CPU_INT08U       *p_ecc_08;
    CPU_DATA          ix;
    ECC_ERR           err_ecc;


    p_soft_ecc_data  = (SOFT_ECC_DATA *)p_ext_data;
    p_sec_buf_08     = (CPU_INT08U *)p_sec_buf;
    p_oos_buf_08     = (CPU_INT08U *)p_oos_buf;
    p_ecc_08         = (CPU_INT08U *)p_oos_buf + oos_size;

    oos_chk_size_rem = oos_size;

    for (ix = 0u; ix < p_soft_ecc_data->CodewordsPerSec; ix++) {
        err_ecc = ECC_ERR_NONE;

        if (oos_chk_size_rem  > p_soft_ecc_data->OOS_SizePerCodeword) {
            oos_chk_size      = p_soft_ecc_data->OOS_SizePerCodeword;
        } else {
            oos_chk_size      = (FS_NAND_PG_SIZE) oos_chk_size_rem;
        }
        oos_chk_size_rem -= oos_chk_size;

        p_soft_ecc_data->ModulePtr->Calc(p_sec_buf_08,
                                         p_soft_ecc_data->BufSizePerCodeword,
                                         p_oos_buf_08,
                                         oos_chk_size,
                                         p_ecc_08,
                                        &err_ecc);
        if (err_ecc != ECC_ERR_NONE) {
           *p_err = FS_ERR_DEV_INVALID_ECC;
            return;
        }

        p_sec_buf_08 += p_soft_ecc_data->BufSizePerCodeword;
        p_oos_buf_08 += oos_chk_size;
        p_ecc_08     += p_soft_ecc_data->SizePerSec / p_soft_ecc_data->CodewordsPerSec;
    }
}


/*
*********************************************************************************************************
*                                             ECC_Verify()
*
* Description : Verify sector and OOS data against ECC code words and correct data if needed/possible.
*
* Argument(s) : p_ext_data  Pointer to extension data.
*
*               p_sec_buf   Pointer to sector data.
*
*               p_oos_buf   Pointer to OOS data.
*
*               oos_size    Size of OOS data to protect in octets, excluding storage space for ECC code words.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*               -----       Argument validated by caller.
*
*                               FS_ERR_ECC_CRITICAL_CORR    Sector and OOS data have been successfully corrected.
*                               FS_ERR_ECC_UNCORR           Sector and OOS data are corrupted and can't be corrected.
*                               FS_ERR_INVALID_ARG          Invalid parameters have been passed to ECC module.
*                               FS_ERR_NONE                 Sector and OOS data successfully verified.
*
* Return(s)   : none.
*
* Caller(s)   : FS_NAND_CtrlrGen_SoftECC.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  ECC_Verify (void             *p_ext_data,
                          void             *p_sec_buf,
                          void             *p_oos_buf,
                          FS_NAND_PG_SIZE   oos_size,
                          FS_ERR           *p_err)
{
    SOFT_ECC_DATA    *p_soft_ecc_data;
    FS_NAND_PG_SIZE   oos_chk_size;
    FS_NAND_PG_SIZE   oos_chk_size_rem;
    ECC_ERR           err_ecc;
    FS_ERR            err_rtn;
    CPU_DATA          ix;
    CPU_INT08U       *p_sec_buf_08;
    CPU_INT08U       *p_oos_buf_08;
    CPU_INT08U       *p_ecc_08;
    CPU_INT08U        ecc_size;


    p_soft_ecc_data = (SOFT_ECC_DATA *)p_ext_data;

    p_sec_buf_08 = (CPU_INT08U *)p_sec_buf;
    p_oos_buf_08 = (CPU_INT08U *)p_oos_buf;
    p_ecc_08     = (CPU_INT08U *)p_oos_buf + oos_size;

    oos_chk_size_rem = oos_size;
    ecc_size         = p_soft_ecc_data->SizePerSec / p_soft_ecc_data->CodewordsPerSec;

    err_rtn = FS_ERR_NONE;
    for (ix = 0u; ix < p_soft_ecc_data->CodewordsPerSec; ix++) {
        err_ecc = ECC_ERR_NONE;

        if (oos_chk_size_rem  > p_soft_ecc_data->OOS_SizePerCodeword) {
            oos_chk_size      = p_soft_ecc_data->OOS_SizePerCodeword;
        } else {
            oos_chk_size      = (FS_NAND_PG_SIZE) oos_chk_size_rem;
        }
        oos_chk_size_rem -= oos_chk_size;

        p_soft_ecc_data->ModulePtr->Correct(p_sec_buf_08,
                                            p_soft_ecc_data->BufSizePerCodeword,
                                            p_oos_buf_08,
                                            oos_chk_size,
                                            p_ecc_08,
                                           &err_ecc);

        switch (err_ecc) {
            case ECC_ERR_UNCORRECTABLE:
                 err_rtn = FS_ERR_ECC_UNCORR;
                 break;


            case ECC_ERR_CORRECTABLE:                           /* #### Add support for critical/non-critical errs.     */
                 if (err_rtn != FS_ERR_ECC_UNCORR) {
                     err_rtn  = FS_ERR_ECC_CRITICAL_CORR;
                 }
                 break;


            case ECC_ERR_NONE:
                 break;


            default:
                *p_err = FS_ERR_INVALID_ARG;
                 return;
        }

        p_sec_buf_08  += p_soft_ecc_data->BufSizePerCodeword;
        p_oos_buf_08  += oos_chk_size;
        p_ecc_08      += ecc_size;
    }

   *p_err = err_rtn;
}


/*
*********************************************************************************************************
*                                               DataGet()
*
* Description : Allocate & initialize extension data.
*
* Argument(s) : none.
*
* Return(s)   : Pointer to extension data, if NO errors.
*               DEF_NULL,                  otherwise.
*
* Caller(s)   : Open().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  SOFT_ECC_DATA  *DataGet (void)
{
    SOFT_ECC_DATA  *p_soft_ecc_data;
    CPU_SIZE_T      octets_reqd;
    LIB_ERR         alloc_err;
    CPU_SR_ALLOC();


                                                                /* --------------------- ALLOC DATA ------------------- */
    CPU_CRITICAL_ENTER();
    if (FS_NAND_CtrlrGen_SoftECC_ListFreePtr == (SOFT_ECC_DATA *)0) {
        p_soft_ecc_data = (SOFT_ECC_DATA *)Mem_HeapAlloc(sizeof(SOFT_ECC_DATA),
                                                         sizeof(CPU_INT32U),
                                                        &octets_reqd,
                                                        &alloc_err);
        if (p_soft_ecc_data == DEF_NULL) {
            CPU_CRITICAL_EXIT();
            FS_TRACE_DBG(("(fs_nand_ctrlr_gen_soft_ecc) DataGet(): Could not alloc mem for "
                          "NAND Generic Ctrlr Soft ECC data: %d octets required.\r\n",
                          octets_reqd));
            return (DEF_NULL);
        }
        (void)&alloc_err;

    } else {
        p_soft_ecc_data                      = FS_NAND_CtrlrGen_SoftECC_ListFreePtr;
        FS_NAND_CtrlrGen_SoftECC_ListFreePtr = FS_NAND_CtrlrGen_SoftECC_ListFreePtr->NextPtr;
    }
    CPU_CRITICAL_EXIT();

    return (p_soft_ecc_data);
}


/*
*********************************************************************************************************
*                                              DataFree()
*
* Description : Free extension data.
*
* Argument(s) : p_ext_data  Pointer to extension data.
*               ----------  Argument validated by caller.
*
* Return(s)   : none.
*
* Caller(s)   : Close().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  DataFree (SOFT_ECC_DATA  *p_ext_data)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
                                                                /* Add to free pool.                                    */
    p_ext_data->NextPtr                  = FS_NAND_CtrlrGen_SoftECC_ListFreePtr;
    FS_NAND_CtrlrGen_SoftECC_ListFreePtr = p_ext_data;
    CPU_CRITICAL_EXIT();
}

