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

       .macro    OS_EMAC_SAVE
                                                /* non-EMAC port does not save any EMAC registers     */

                .endm



       .macro    OS_EMAC_RESTORE
                                                /* non-EMAC port does not save any EMAC registers     */

                .endm
