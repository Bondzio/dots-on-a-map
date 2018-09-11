/*
*********************************************************************************************************
*                                       uC/Probe Communication
*
*                         (c) Copyright 2007-2008; Micrium, Inc.; Weston, FL
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
*                                        COMMUNICATION: RS-232
*
*                                    PORT FOR FREESCALE MCF51QE128
*
* Filename      : probe_rs232c.h
* Version       : V2.20
* Programmer(s) : EFS
*********************************************************************************************************
*/

#ifndef  PROBE_RS232C_PRESENT
#define  PROBE_RS232C_PRESENT

/*
*********************************************************************************************************
*                                        FUNCTION PROTOTYPES
*********************************************************************************************************
*/

extern  void   ProbeRS232_ErrISR(void);                                 /* uC/Probe SCI Error Interrupt Service Routine             */
extern  void   ProbeRS232_RxISR(void);                                  /* uC/Probe SCI Rx Interrupt Service Routine                */
extern  void   ProbeRS232_TxISR(void);                                  /* uC/Probe SCI Tx Interrupt Service Routine                */

#endif
