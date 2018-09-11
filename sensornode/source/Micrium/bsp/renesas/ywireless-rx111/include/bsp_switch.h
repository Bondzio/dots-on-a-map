/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         BOARD SUPPORT PACKAGE
*
*                                            Renesas RX111
*                                                on the
*                                           YWIRELESS-RX111
*
* Filename      : bsp_switch.c
* Version       : V1.00
* Programmer(s) : SB
*                 JPC
*********************************************************************************************************
*/

#ifndef  BSP_SWITCH_H_
#define  BSP_SWITCH_H_

#include  <cpu.h>


/*
*********************************************************************************************************
*                                               TYPES
*********************************************************************************************************
*/

typedef  enum  _switch_event {
    SWITCH_EVENT_OFF_TO_ON,
    SWITCH_EVENT_ON_TO_OFF,
    SWITCH_EVENT_ANY
} SWITCH_EVENT;

typedef CPU_BOOLEAN  (*SWITCH_INT_FNCT)(CPU_INT08U  switch_nbr, SWITCH_EVENT  type);


/*
*********************************************************************************************************
*                                            PROTOTYPES
*********************************************************************************************************
*/

void         BSP_SwitchInit   (void);

CPU_BOOLEAN  BSP_SwitchRd     (CPU_INT08U  switch_nbr);

int          BSP_SwitchIntReg (CPU_INT08U  switch_nbr, SWITCH_INT_FNCT  callback);


#endif
