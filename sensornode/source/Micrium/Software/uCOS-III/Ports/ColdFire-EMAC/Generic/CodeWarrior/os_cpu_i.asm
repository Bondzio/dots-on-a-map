/*
;********************************************************************************************************
;                                             uC/OS-III
;                                        The Real-Time Kernel
;
;                            (c) Copyright 2010-2015; Micrium, Inc.; Weston, FL
;                         
;               All rights reserved. Protected by international copyright laws.
;
;               uC/OS-III is provided in source form to registered licensees ONLY.  It is
;               illegal to distribute this source code to any third party unless you receive
;               written permission by an authorized Micrium representative.  Knowledge of
;               the source code may NOT be used to develop a similar product.
;
;               Please help us continue to provide the Embedded community with the finest
;               software available.  Your honesty is greatly appreciated.
;
;               You can contact us at www.micrium.com.
;********************************************************************************************************
*/

/*
;********************************************************************************************************
;
;                        Generic Coldfire with EMAC Port for CodeWarrior Compiler
;
;                                          Micrium uC/OS-III
;
; Filename      : OS_CPU_I.ASM 
; Version       : V3.05.01
; Programmer(s) : Jean J. Labrosse
;********************************************************************************************************
; Note(s)       : None
;********************************************************************************************************
*/

/*
;**************************************************************************************************
;                                             MACROS
;**************************************************************************************************
*/

       .macro    OS_EMAC_SAVE
                                      /* CODE BELOW TO SAVE EMAC REGISTERS        */
       MOVE.L    MACSR,D7             /* Save the MACSR                           */
       CLR.L     D0                   /* Disable rounding in the MACSR            */
       MOVE.L    D0,MACSR             /* Save the accumulators                    */
       MOVE.L    ACC0,D0
       MOVE.L    ACC1,D1
       MOVE.L    ACC2,D2
       MOVE.L    ACC3,D3
       MOVE.L    ACCEXT01,D4          /* Save the accumulator extensions          */
       MOVE.L    ACCEXT23,D5
       MOVE.L    MASK,D6              /* Save the address mask                    */
       LEA       -32(A7),A7           /* Move the EMAC state to the task's stack  */
       MOVEM.L   D0-D7,(A7)

      .endm



       .macro    OS_EMAC_RESTORE
                                      /* CODE BELOW TO RESTORE EMAC REGISTERS     */
       MOVEM.L    (A7),D0-D7          /* Restore the EMAC state                   */
       MOVE.L     #0,MACSR            /* Disable rounding in the MACSR            */
       MOVE.L     D0,ACC0             /* Restore the accumulators                 */
       MOVE.L     D1,ACC1
       MOVE.L     D2,ACC2
       MOVE.L     D3,ACC3
       MOVE.L     D4,ACCEXT01
       MOVE.L     D5,ACCEXT23
       MOVE.L     D6,MASK
       MOVE.L     D7,MACSR
       LEA        32(A7),A7

      .endm
