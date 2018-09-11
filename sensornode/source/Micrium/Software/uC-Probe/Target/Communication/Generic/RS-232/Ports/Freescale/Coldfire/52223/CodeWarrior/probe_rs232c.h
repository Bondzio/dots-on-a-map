/*
*********************************************************************************************************
*                                      uC/Probe Communication
*
*                           (c) Copyright 2007; Micrium, Inc.; Weston, FL
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
*                                              uC/Probe
*
*                                      Communication: RS-232
*                                  Port for the Freescale CF52223
*
* Filename      : probe_rs232c.h
* Version       : V1.00
* Programmer(s) : Fabiano Kovalski
*********************************************************************************************************
*/

#ifndef  __PROBE_RS232C_H__
#define  __PROBE_RS232C_H__


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void    ProbeRS232_RxTxISR       (void);                        /* Code to process Rx or Tx interrupts                  */


#endif
