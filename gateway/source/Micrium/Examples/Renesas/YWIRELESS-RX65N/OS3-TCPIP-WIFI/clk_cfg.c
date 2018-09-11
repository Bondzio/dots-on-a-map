/*
*********************************************************************************************************
*                                               uC/Clk
*                                          Clock / Calendar
*
*                          (c) Copyright 2005-2014; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               uC/Clk is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
**                                     CLOCK CONFIGURATION FILE
*
*                                          TEMPLATE-EXAMPLE
*
* Filename      : clk_cfg_task.c
* Version       : V3.11.00
* Programmer(s) : AL
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    CLK_CFG_MODULE


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <lib_def.h>
#include  <Source/clk.h>

/*
*********************************************************************************************************
*********************************************************************************************************
*                                     EXAMPLE TASKS CONFIGURATION
*
* Notes: (1) (a) Task priorities can be defined either in this configuration file 'clk_cfg.c' or in a global
*                OS tasks priorities configuration header file which must be included in 'clk_cfg.c' and
*                used within task's configuration structures:
*
*                   in app_cfg.h:
*                       #define  CLK_TASK_PRIO           30u
*
*                   in clk_cfg.c:
*                       #include  <app_cfg.h>
*
*                       CLK_TASK_CFG  ClkTaskCfg        = {
*                                                              CLK_TASK_PRIO,
*                                                              2048,
*                                                              DEF_NULL,
*                                                           };
*
*        (2)  TODO Add note about stack size, how to calculate it
*
*        (3)  TODO Add note about stack location and allocation
*********************************************************************************************************
*********************************************************************************************************
*/

const  CLK_TASK_CFG  ClkTaskCfg = {
        30u,                                                    /* CLK task priority                    (see Note #1).   */
        128,                                                    /* CLK task stack size in bytes         (see Note #2).   */
        DEF_NULL,                                               /* CLK task stack pointer               (See Note #3).   */
};
