/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*                          (c) Copyright 2016; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*
*               This BSP is provided in source form to registered licensees ONLY.  It is
*               illegal to distribute this source code to any third party unless you receive
*               written permission by an authorized Micrium representative.  Knowledge of
*               the source code may NOT be used to develop a similar product.  However,
*               please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only.
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
*                                          SPI BUS FUNCTIONS
*
* Filename      : bsp_spi.h
* Version       : V1.00.00
* Programmer(s) : JPC
*********************************************************************************************************
*/

#ifndef  _BSP_SPI_H_
#define  _BSP_SPI_H_

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <iorx651.h>


/*
*********************************************************************************************************
*                                              DEFINES
*********************************************************************************************************
*/

#define  BSP_SPI_SLAVE_ID_PMOD2                 2
#define  BSP_SPI_SLAVE_ID_PMOD3                 3

#ifdef RX231
#define PMODRST_PDR PORT0.PDR.BIT.B5
#define PMODRST_PODR PORT0.PODR.BIT.B5
#define PMOD_PIN9_PDR PORTC.PDR.BIT.B2
#define PMOD_PIN9_PODR PORTC.PODR.BIT.B2
#define PMOD_PIN10_PDR PORTC.PDR.BIT.B3

// On RX231 YWireless kit, PMOD2 in this file actually refers to the dedicated WIFI signals
#define PMOD2_PIN1_PDR PORTA.PDR.BIT.B0
#define PMOD2_PIN1_PODR PORTA.PODR.BIT.B0
#define PMOD2_PIN1_PMR PORTA.PMR.BIT.B0
#define PMOD2_PIN7_PDR PORTD.PDR.BIT.B0
#define PMOD2_PIN7_PFS_ISEL MPC.PD0PFS.BIT.ISEL

// On RX231 YWireless kit, PMOD3 in this file actually refers to PMOD1
#define PMOD3_PIN1_PDR PORTC.PDR.BIT.B1
#define PMOD3_PIN1_PODR PORTC.PODR.BIT.B1
#define PMOD3_PIN1_PMR PORTC.PMR.BIT.B1
#define PMOD3_PIN7_PDR PORTD.PDR.BIT.B1
#define PMOD3_PIN7_PODR PORTD.PODR.BIT.B1

#define RSPI0_MOSIA_PDR PORTC.PDR.BIT.B6
#define RSPI0_MOSIA_PMR PORTC.PMR.BIT.B6
#define RSPI0_MOSIA_PFS_PSEL MPC.PC6PFS.BIT.PSEL
#define RSPI0_MISOA_PDR PORTC.PDR.BIT.B7
#define RSPI0_MISOA_PMR PORTC.PMR.BIT.B7
#define RSPI0_MISOA_PFS_PSEL MPC.PC7PFS.BIT.PSEL
#define RSPI0_RSPCKA_PDR PORTC.PDR.BIT.B5
#define RSPI0_RSPCKA_PMR PORTC.PMR.BIT.B5
#define RSPI0_RSPCKA_PFS_PSEL MPC.PC5PFS.BIT.PSEL

#else
#ifdef RX65N
#define PMODRST_PDR PORT4.PDR.BIT.B0
#define PMODRST_PODR PORT4.PODR.BIT.B0
#define PMOD_PIN9_PDR PORT7.PDR.BIT.B5
#define PMOD_PIN9_PODR PORT7.PODR.BIT.B5
#define PMOD_PIN10_PDR PORT7.PDR.BIT.B4

// On RX65N YWireless kit, PMOD3 in this file actually refers to PMOD1
#define PMOD3_PIN1_PDR PORTA.PDR.BIT.B4
#define PMOD3_PIN1_PODR PORTA.PODR.BIT.B4
#define PMOD3_PIN1_PMR PORTA.PMR.BIT.B4
#define PMOD3_PIN7_PDR PORTB.PDR.BIT.B0
#define PMOD3_PIN7_PODR PORTB.PODR.BIT.B0

#define RSPI0_MOSIA_PDR PORTA.PDR.BIT.B6
#define RSPI0_MOSIA_PMR PORTA.PMR.BIT.B6
#define RSPI0_MOSIA_PFS_PSEL MPC.PA6PFS.BIT.PSEL
#define RSPI0_MISOA_PDR PORTA.PDR.BIT.B7
#define RSPI0_MISOA_PMR PORTA.PMR.BIT.B7
#define RSPI0_MISOA_PFS_PSEL MPC.PA7PFS.BIT.PSEL
#define RSPI0_RSPCKA_PDR PORTA.PDR.BIT.B5
#define RSPI0_RSPCKA_PMR PORTA.PMR.BIT.B5
#define RSPI0_RSPCKA_PFS_PSEL MPC.PA5PFS.BIT.PSEL

#else

#define PMODRST_PDR PORTC.PDR.BIT.B2
#define PMODRST_PODR PORTC.PODR.BIT.B2
#define PMOD_PIN9_PDR PORTB.PDR.BIT.B3
#define PMOD_PIN9_PODR PORTB.PODR.BIT.B3
#define PMOD_PIN10_PDR PORTB.PDR.BIT.B5

#define PMOD2_PIN1_PDR PORTC.PDR.BIT.B4
#define PMOD2_PIN1_PODR PORTC.PODR.BIT.B4
#define PMOD2_PIN1_PMR PORTC.PMR.BIT.B4
#define PMOD2_PIN7_PDR PORTA.PDR.BIT.B4
#define PMOD2_PIN7_PFS_ISEL MPC.PA4PFS.BIT.ISEL

#define PMOD3_PIN1_PDR PORTA.PDR.BIT.B0
#define PMOD3_PIN1_PODR PORTA.PODR.BIT.B0
#define PMOD3_PIN1_PMR PORTA.PMR.BIT.B0
#define PMOD3_PIN7_PDR PORTA.PDR.BIT.B3
#define PMOD3_PIN7_PODR PORTA.PODR.BIT.B3

#define RSPI0_MOSIA_PDR PORTC.PDR.BIT.B6
#define RSPI0_MOSIA_PMR PORTC.PMR.BIT.B6
#define RSPI0_MOSIA_PFS_PSEL MPC.PC6PFS.BIT.PSEL
#define RSPI0_MISOA_PDR PORTC.PDR.BIT.B7
#define RSPI0_MISOA_PMR PORTC.PMR.BIT.B7
#define RSPI0_MISOA_PFS_PSEL MPC.PC7PFS.BIT.PSEL
#define RSPI0_RSPCKA_PDR PORTC.PDR.BIT.B5
#define RSPI0_RSPCKA_PMR PORTC.PMR.BIT.B5
#define RSPI0_RSPCKA_PFS_PSEL MPC.PC5PFS.BIT.PSEL

#endif
#endif


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/

typedef  struct  bsp_spi_bus_cfg {
    CPU_INT32U   SClkFreqMax;
    CPU_INT08U   CPOL;
    CPU_INT08U   CPHA;
    CPU_BOOLEAN  LSBFirst;
    CPU_INT08U   BitsPerFrame;
} BSP_SPI_BUS_CFG;

/*
*********************************************************************************************************
*                                        FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  BSP_PMOD_General_Init (void);

void  BSP_PMOD_Reset (void);

void  RSPI_BSP_SPI_Init(void);

void  BSP_RSPI_General_Init (void);

void  BSP_SPI_Lock   (CPU_INT08U  bus_id);

void  BSP_SPI_Unlock (CPU_INT08U  bus_id);

void  BSP_SPI_ChipSel(CPU_INT08U  bus_id, CPU_INT08U  slave_id, CPU_BOOLEAN  en);

void  BSP_SPI_Cfg    (CPU_INT08U  bus_id, BSP_SPI_BUS_CFG  const  *p_cfg);

void  BSP_SPI_Xfer   (CPU_INT08U  bus_id, CPU_INT08U  const  *p_tx, CPU_INT08U  *p_rx, CPU_INT16U  len);

void  RSPI0_BSP_SPI_Data (void);

void  RSPI0_BSP_SPI_Command (void);

#endif                                                          /* _BSP_SPI_H_                                          */
