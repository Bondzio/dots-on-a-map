/*
*********************************************************************************************************
*                                                uC/OS-III
*                                          The Real-Time Kernel
*
*
*                           (c) Copyright 2009-2015; Micrium, Inc.; Weston, FL
*                    All rights reserved.  Protected by international copyright laws.
*
*                                          Generic ColdFire Port
*
* File      : OS_CPU_I.ASM
* Version   : V3.05.01
* By        : FGK
*             JD
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

OS_EMAC_SAVE     MACRO
                                                /* non-EMAC port does not save any EMAC registers     */

                 ENDM



OS_EMAC_RESTORE  MACRO
                                                /* non-EMAC port does not save any EMAC registers     */

                 ENDM
