/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                               (c) Copyright 2015; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                            RENESAS RX111
*                                               on the
*                                    YWireless-RX111 Wireless Demo Kit
*
* Filename      : sys_init.c
* Version       : V1.00
* Programmer(s) : JPC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>
#include  <kal.h>
#include  <fs.h>
#include  <fs_dev.h>
#include  <fs_vol.h>
#include  <fs_file.h>
#include  <FAT/fs_fat.h>
#include  <Dev/DataFlash/fs_dev_dataflash.h>
#include  <lib_math.h>
#include  <Source/usbd_core.h>
#include  <ctype.h>
#include  <stdint.h>
#include  <app_cfg.h>
#include  <qca.h>
#include  <qca_bsp.h>
#include  <atheros_stack_offload.h>
#include  <qcom_api.h>
#include  <Source/probe_com.h>
#include  <TCPIP/Source/probe_tcpip.h>
#include  <Source/usbd_core.h>
#include  <usbd_dev_cfg.h>
#include  <usbd_bsp.h>
#include  <Class/MSC/usbd_msc.h>
#include  <bsp.h>
#include  <bsp_led.h>
#include  <bsp_switch.h>
#include  <bsp_sys.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         DEFAULT CONFIGURATION
*********************************************************************************************************
*/

#ifndef  MAIN_TASK_STK_SIZE
# define MAIN_TASK_STK_SIZE                                 512u
#endif

#ifndef  MAIN_TASK_PRIO
# define MAIN_TASK_PRIO                                     1u
#endif


#define  HEARTBEAT_TMR_PERIOD_SEC                           0.9

#define  SYS_FS_DEV_CNT                                     1
#define  SYS_FS_VOL_CNT                                     1
#define  SYS_FS_FILE_CNT                                    3
#define  SYS_FS_DIR_CNT                                     3
#define  SYS_FS_BUF_CNT                                    (SYS_FS_VOL_CNT * 2u)
#define  SYS_FS_DEV_DRV_CNT                                 1
#define  SYS_FS_MAX_SEC_SIZE                                1024u

static  OS_TCB      MainTaskTCB;
static  CPU_STK     MainTaskStk[MAIN_TASK_STK_SIZE];

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  MainTask             (void  *p_arg);

static  void  HeartbeatTmrCallback (void  *p_tmr, void  *p_arg);

static  CPU_BOOLEAN  InitFS        (void);

static  CPU_BOOLEAN  InitUSB       (void);

static  CPU_BOOLEAN  InitUSB_MSC   (CPU_INT08U  dev_nbr,
                                    CPU_INT08U  cfg_nbr);


                                                                /* ---------- USB DEVICE CALLBACKS FUNCTIONS ---------- */
static  void  App_USBD_EventReset  (CPU_INT08U   dev_nbr);
static  void  App_USBD_EventSuspend(CPU_INT08U   dev_nbr);
static  void  App_USBD_EventResume (CPU_INT08U   dev_nbr);
static  void  App_USBD_EventCfgSet (CPU_INT08U   dev_nbr,
                                    CPU_INT08U   cfg_val);
static  void  App_USBD_EventCfgClr (CPU_INT08U   dev_nbr,
                                    CPU_INT08U   cfg_val);
static  void  App_USBD_EventConn   (CPU_INT08U   dev_nbr);
static  void  App_USBD_EventDisconn(CPU_INT08U   dev_nbr);


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  USBD_BUS_FNCTS  App_USBD_BusFncts = {
    App_USBD_EventReset,
    App_USBD_EventSuspend,
    App_USBD_EventResume,
    App_USBD_EventCfgSet,
    App_USBD_EventCfgClr,
    App_USBD_EventConn,
    App_USBD_EventDisconn
};


/*
*********************************************************************************************************
*                                    EXTERNAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

extern  int  main (int  argc, char  *argv[]);


/*
*********************************************************************************************************
*                                    SYSTEM INITIALIZATION FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            HardwareInit()
*
* Description : This function is called immediately after startup BEFORE the C runtime initialization
*
* Caller(s)   : Reset vector
*
* Note(s)     : DO NOT use static or global variables in this function, as they will be overwritten
*               by the CRT initialization.
*********************************************************************************************************
*/

void  HardwareInit (void)
{
}


/*
*********************************************************************************************************
*                                            SoftwareInit()
*
* Description : This function is called immediately after the C runtime initialization. It initializes
*               the kernel, creates and starts the first task, and handles main()'s return value.
*
* Caller(s)   : Reset vector
*********************************************************************************************************
*/

void  SoftwareInit (void)
{

    OS_ERR  err_os;
    CPU_ERR err_cpu;


    CPU_Init();
    Mem_Init();                                                 /* Initialize the Memory Management Module              */
    Math_Init();                                                /* Initialize the Mathematical Module                   */


    CPU_NameSet("RX1118", &err_cpu);
    CPU_IntDis();

    OSInit(&err_os);                                            /* Initialize "uC/OS-III, The Real-Time Kernel"         */

    OSTaskCreate((OS_TCB     *)&MainTaskTCB,                    /* Create the main (startup) task                       */
                 (CPU_CHAR   *)"Main Task",
                 (OS_TASK_PTR ) MainTask,
                 (void       *) 0,
                 (OS_PRIO     ) MAIN_TASK_PRIO,
                 (CPU_STK    *)&MainTaskStk[0],
                 (CPU_STK     )(MAIN_TASK_STK_SIZE / 10u),
                 (CPU_STK_SIZE) MAIN_TASK_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err_os);

    OSStart(&err_os);                                           /* Start multitasking (i.e. give control to uC/OS-III). */
    BSP_SysBrk();                                               /* Should never get here - throw an exception           */
}

/*
*********************************************************************************************************
*                                              MainTask()
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*********************************************************************************************************
*/
#include <iodefine.h>

static  void  MainTask (void  *p_arg)
{
    static  OS_TMR  HeartbeatTmr;

    CPU_BOOLEAN     init_status;
    OS_ERR          err_os;
    RTOS_ERR        err_rtos;
    int             ret;


    (void)p_arg;

    //TODO
#ifndef RX231
    PORTE.PDR.BIT.B2 = 1; /* gt202 workaround. generify for rx231... */
    PORTE.PODR.BIT.B2 = 1;
#endif
    
    KAL_Init(DEF_NULL, &err_rtos);
    ASSERT(err_rtos == RTOS_ERR_NONE);

    BSP_Init();                                                 /* Start BSP and tick initialization                    */

    OSTmrCreate(         &HeartbeatTmr,
                         "Heartbeat Timer",
                          0u,
                (OS_TICK)(HEARTBEAT_TMR_PERIOD_SEC * OSCfg_TmrTaskRate_Hz),
                          OS_OPT_TMR_PERIODIC,
                          HeartbeatTmrCallback,
                          0,
                          &err_os);
    OSTmrStart(&HeartbeatTmr, &err_os);

#if (OS_CFG_STAT_TASK_EN > 0)
    OSStatTaskCPUUsageInit(&err_os);                            /* Start tracking CPU statistics                        */
#endif

                                                                /* Initialize uC/FS.                                    */
#if 0
    init_status = InitFS();
    ASSERT(init_status == DEF_OK);
                                                                /* Initialize USBD w/ mass storage class.               */
    init_status = InitUSB();
    ASSERT(init_status == DEF_OK);
#else
    DF_AccessEn();
#endif

    ret = main(0, 0);                                           /* Call main(), the application entry point             */

                                                                /* Light up LEDs to display return value                */
    if (ret & (1 << 0)) BSP_LED_On(3);
    if (ret & (1 << 1)) BSP_LED_On(2);
    if (ret & (1 << 2)) BSP_LED_On(1);


    OSTaskDel(DEF_NULL, &err_os);
    ASSERT(err_os == OS_ERR_NONE);                              /* Should never get here                                */
}


/*
*********************************************************************************************************
*                                       HeartbeatTmrCallback()
*
* Description : Periodically toggles the heartbeat LED
*
* Argument(s) : p_arg   Not used
*
* Return(s)   : none.
*
* Caller(s)   : uC/OS Timer Task
*********************************************************************************************************
*/

static  void  HeartbeatTmrCallback (void  *p_tmr, void  *p_arg)
{
    (void)p_tmr;
    (void)p_arg;

    BSP_LED_Toggle(BSP_LED_HEARTBEAT);
}


/*
*********************************************************************************************************
*                                              InitFS()
*
* Description : Initializes the uC/FS with DataFlash as the storage medium
*
* Return(s)   : DEF_YES if initialization succeeds, DEF_NO otherwise
*********************************************************************************************************
*/

static  CPU_BOOLEAN  InitFS (void)
{
    FS_ERR          err_fs;
    FS_CFG          fs_cfg;
    FS_FAT_SYS_CFG  fat_cfg;
    CPU_BOOLEAN     did_init;


    did_init = DEF_NO;
    fs_cfg   = (FS_CFG) {
        .DevCnt     = SYS_FS_DEV_CNT,
        .VolCnt     = SYS_FS_VOL_CNT,
        .FileCnt    = SYS_FS_FILE_CNT,
        .DirCnt     = SYS_FS_DIR_CNT,
        .BufCnt     = SYS_FS_BUF_CNT,
        .DevDrvCnt  = SYS_FS_DEV_DRV_CNT,
        .MaxSecSize = SYS_FS_MAX_SEC_SIZE
    };

                                                                /* Init DataFlash FS device                             */
    do {
        err_fs = FS_Init(&fs_cfg);
        if (err_fs != FS_ERR_NONE) {
            break;
        }

        FS_DevDrvAdd((FS_DEV_API *)&FSDev_DataFlash, &err_fs);
        if (err_fs != FS_ERR_NONE) {
            break;
        }

        FSDev_Open("df:0:", 0, &err_fs);
        if (err_fs != FS_ERR_NONE) {
            break;
        }

        FSVol_Open("df:0:", "df:0:", 0, &err_fs);
        if ((BSP_SwitchRd(4) == DEF_ON) || (err_fs == FS_ERR_PARTITION_NOT_FOUND)) {/* Reformat the volume              */
            fat_cfg.ClusSize           = 1;
            fat_cfg.RsvdAreaSize       = 0;
            fat_cfg.RootDirEntryCnt    = 32;
            fat_cfg.FAT_Type           = FS_FAT_FAT_TYPE_FAT12;
            fat_cfg.NbrFATs            = 2;

            FSVol_Fmt("df:0:", (void *)&fat_cfg, &err_fs);

            while (BSP_SwitchRd(4) == DEF_ON);

            if (err_fs == FS_ERR_NONE) {
                BSP_SysReset();
            } else {
                BSP_SysBrk();
            }

        } else if (err_fs == FS_ERR_NONE) {
            did_init = DEF_YES;
        } else {
            did_init = DEF_NO;
        }
    } while (0);

    return did_init;
}


/*
*********************************************************************************************************
*                                              InitUSB()
*
* Description : Initializes USB-Device stack and USB controller, then starts the mass-storage device
*               class
*
* Return(s)   : DEF_YES if initialization succeeds, DEF_NO otherwise
*********************************************************************************************************
*/

static  CPU_BOOLEAN  InitUSB (void)
{
    extern  USBD_DEV_CFG      USBD_DevCfg_RX111;
    extern  USBD_DRV_API      USBD_DrvAPI_RX600;
    extern  USBD_DRV_CFG      USBD_DrvCfg_RX111;
    extern  USBD_DRV_BSP_API  USBD_DrvBSP_YWirelessRX111;

    CPU_BOOLEAN  did_init;
    USBD_ERR     err_usb;
    CPU_INT08U   dev_nbr;
    CPU_INT08U   cfg_nbr;
    CPU_BOOLEAN  msc_init;


    did_init = DEF_NO;

    do {
        USBD_Init(&err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }

        dev_nbr = USBD_DevAdd(&USBD_DevCfg_RX111,
                              &App_USBD_BusFncts,
                              &USBD_DrvAPI_RX600,
                              &USBD_DrvCfg_RX111,
                              &USBD_DrvBSP_YWirelessRX111,
                              &err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }

        USBD_DevSelfPwrSet(dev_nbr, DEF_NO, &err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }

        cfg_nbr = USBD_CfgAdd(dev_nbr,                          /* Add full-speed config                                */
                              USBD_DEV_ATTRIB_SELF_POWERED,
                              100u,
                              USBD_DEV_SPD_FULL,
                             "Full-speed Configuration",
                             &err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }

        msc_init = InitUSB_MSC(dev_nbr,                         /* Initialize MSC class.                                */
                               cfg_nbr);
        if (msc_init == DEF_FAIL) {
            break;
        }

#if 0
        USBD_DevStart(dev_nbr, &err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }
#endif

        did_init = DEF_YES;
    } while (0);

    return did_init;
}


/*
*********************************************************************************************************
*                                            InitUSB_MSC()
*
* Description : Start USB Mass Storage Device class
*
* Argument(s) : dev_nbr   USB device controller number
*
*               cfg_nbr   Configuration identifier
*
* Return(s)   : DEF_YES if initialization succeeds, DEF_NO otherwise
*
* Caller(s)   : InitUSB()
*********************************************************************************************************
*/

CPU_BOOLEAN  InitUSB_MSC (CPU_INT08U  dev_nbr,
                                  CPU_INT08U  cfg_nbr)
{
    CPU_BOOLEAN  did_start;
    USBD_ERR     err_usb;
    CPU_INT08U   msc_nbr;


    did_start = DEF_NO;

    do {
        USBD_MSC_Init(&err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }

        msc_nbr = USBD_MSC_Add(&err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }



        USBD_MSC_CfgAdd(msc_nbr,
                        dev_nbr,
                        cfg_nbr,
                       &err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }
                                                                    /* Add Logical Unit to MSC interface.                   */
        USBD_MSC_LunAdd((void *)"df:0:",
                                 msc_nbr,
                        (void *)"Micrium",
                        (void *)"MSC FS Storage",
                                 0x00,
                                 DEF_FALSE,
                                &err_usb);
        if (err_usb != USBD_ERR_NONE) {
            break;
        }

        did_start = DEF_YES;
    } while (0);


    return did_start;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            USB CALLBACKS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        App_USBD_EventReset()
*
* Description : Bus reset event callback function.
*
* Argument(s) : dev_nbr     Device number.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_EventProcess() via 'p_bus_fnct->Reset()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_USBD_EventReset (CPU_INT08U  dev_nbr)
{
    (void)dev_nbr;
}


/*
*********************************************************************************************************
*                                       App_USBD_EventSuspend()
*
* Description : Bus suspend event callback function.
*
* Argument(s) : dev_nbr     Device number.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_EventProcess() via 'p_bus_fnct->Suspend()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_USBD_EventSuspend (CPU_INT08U  dev_nbr)
{
    (void)dev_nbr;
}


/*
*********************************************************************************************************
*                                       App_USBD_EventResume()
*
* Description : Bus Resume event callback function.
*
* Argument(s) : dev_nbr     Device number.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_EventProcess() via 'p_bus_fnct->Resume()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_USBD_EventResume (CPU_INT08U  dev_nbr)
{
    (void)dev_nbr;
}


/*
*********************************************************************************************************
*                                       App_USBD_EventCfgSet()
*
* Description : Set configuration callback event callback function.
*
* Argument(s) : dev_nbr     Device number.
*
*               cfg_nbr     Active device configuration number selected by the host.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_CfgOpen() via 'p_dev->BusFnctsPtr->CfgSet()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_USBD_EventCfgSet (CPU_INT08U  dev_nbr,
                                    CPU_INT08U  cfg_val)
{
    (void)dev_nbr;
    (void)cfg_val;
}


/*
*********************************************************************************************************
*                                       App_USBD_EventCfgClr()
*
* Description : Clear configuration event callback function.
*
* Argument(s) : dev_nbr     Device number.
*
*               cfg_nbr     Current device configuration number that will be closed.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_CfgClose via 'p_dev->BusFnctsPtr->CfgClr()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_USBD_EventCfgClr (CPU_INT08U  dev_nbr,
                                    CPU_INT08U  cfg_val)
{
    (void)dev_nbr;
    (void)cfg_val;
}


/*
*********************************************************************************************************
*                                        App_USBD_EventConn()
*
* Description : Device connection event callback function.
*
* Argument(s) : dev_nbr     Device number.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_EventProcess() via 'p_bus_fnct->Conn()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_USBD_EventConn (CPU_INT08U  dev_nbr)
{
    (void)dev_nbr;
}


/*
*********************************************************************************************************
*                                       App_USBD_EventDisconn()
*
* Description : Device connection event callback function.
*
* Argument(s) : dev_nbr     USB device number.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_EventProcess() via 'p_bus_fnct->Disconn()'.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_USBD_EventDisconn (CPU_INT08U  dev_nbr)
{
    (void)dev_nbr;
}


/*
*********************************************************************************************************
*                                            USBD_Trace()
*
* Description : Function to output or log USB trace events.
*
* Argument(s) : p_str       Pointer to string containing the trace event information.
*
* Return(s)   : none.
*
* Caller(s)   : USBD_DbgTaskHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBD_Trace (const  CPU_CHAR  *p_str)
{
    (void)p_str;
}
