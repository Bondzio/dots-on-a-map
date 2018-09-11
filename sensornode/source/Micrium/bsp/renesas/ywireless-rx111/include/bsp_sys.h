/*
*********************************************************************************************************
*                                         BOARD SUPPORT PACKAGE
*
*                                            Renesas RX111
*                                                on the
*                                      RPBRX111 Promotional Board
*
*                                 (c) Copyright 2015, Micrium, Weston, FL
*                                          All Rights Reserved
*
* Filename      : bsp_sys.h
* Version       : V1.00
* Programmer(s) : JPB
*                 AA
*                 JJL
*********************************************************************************************************
*/

#ifndef  __BSP_SYS_H__
#define  __BSP_SYS_H__


#include  <cpu.h>


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/
                                                                /* Peripheral Clock IDs.                                */
#define  CLK_ID_FCLK                            0
#define  CLK_ID_ICLK                            1
#define  CLK_ID_PCLKB                           2
#define  CLK_ID_PCLKD                           3
#define  CLK_ID_UCLK                            4


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void        BSP_SysInit             (void      );
CPU_INT32U  BSP_SysPeriphClkFreqGet (int clk_id);
void        BSP_SysReset            (void);
void        BSP_SysBrk              (void);

#endif
