/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*********************************************************************************************************
* Licensing terms:
*   This file is provided as an example on how to use Micrium products. It has not necessarily been
*   tested under every possible condition and is only offered as a reference, without any guarantee.
*
*   Please feel free to use any application code labeled as 'EXAMPLE CODE' in your application products.
*   Example code may be used as is, in whole or in part, or may be used as a reference only. This file
*   can be modified as required.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: http://www.micrium.com
*
*   Please help us continue to provide the Embedded community with the finest software available.
*
*   Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                             BSP MODULE
*
* File : bsp.h
*********************************************************************************************************
*/

#ifndef  _BSP_H_
#define  _BSP_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     EXTERNAL C LANGUAGE LINKAGE
*
* Note(s) : (1) C++ compilers MUST 'extern'ally declare ALL C function prototypes & variable/object
*               declarations for correct C language linkage.
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
extern  "C" {                                                   /* See Note #1.                                         */
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <lib_def.h>
#include  <iorx651.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  enum  prcr_enable {
    PRC0_CLOCK_GEN,                                             /* Clock Generation circuit.                            */
    PRC1_OP_MODES,                                              /* Operating modes, low power, software reset.          */
    PRC3_LVD                                                    /* LVD                                                  */
} PRCR_ENABLE;


typedef  enum  protect_state {                                  /* State for enabling or disabling writing              */
    WRITE_DISABLED,
    WRITE_ENABLED
} PROTECT_STATE;


#define  PROTECTION_KEY                       0xA500u           /* See page 357 of the RX651 reference manual.          */
#define  SOFTWARE_RESET                       0xA501u           /* See page 207 of the RX651 reference manual.          */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void  BSP_Init        (void);
void  BSP_SystemInit  (void);


/*
*********************************************************************************************************
*                                             INLINES
*********************************************************************************************************
*/

static  inline  void  BSP_Register_Protect (PRCR_ENABLE    module,
                                            PROTECT_STATE  state)
{
    CPU_INT16U  protect;


    protect = 0u;

    switch (module) {
        case 0u:
             if (state == WRITE_ENABLED) {
                 protect |= DEF_BIT_00;
             }
             break;


        case 1u:
             if (state == WRITE_ENABLED) {
                 protect |= DEF_BIT_01;
             }
             break;


        case 2u:
             if (state == WRITE_ENABLED) {
                 protect |= DEF_BIT_03;
             }
             break;


        default:
             break;
    }

    protect |= PROTECTION_KEY;

    SYSTEM.PRCR.WORD = protect;
}


static  inline  void  BSP_IO_Protect (PROTECT_STATE  state)
{
    MPC.PWPR.BIT.B0WI = 0u;                                     /* Enable writing to PFSWE bit.                         */
    if (state == WRITE_ENABLED) {
        MPC.PWPR.BIT.PFSWE = 1u;                                /* Enable writing to PFS register.                      */
    } else {
        MPC.PWPR.BIT.PFSWE = 0u;                                /* Disable writing to PFS register.                     */
    }
    MPC.PWPR.BIT.B0WI = 1u;                                     /* Disable writing to PFSWE bit.                        */
}


static  inline  void  BSP_Software_Reset (void)
{
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_ENABLED);         /* Enable writing to software reset register.           */
    SYSTEM.SWRR = SOFTWARE_RESET;                               /* Perform software reset.                              */
    BSP_Register_Protect(PRC1_OP_MODES, WRITE_DISABLED);        /* Disable writing to software reset register.          */
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   EXTERNAL C LANGUAGE LINKAGE END
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef __cplusplus
}                                                               /* End of 'extern'al C lang linkage.                    */
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of module include.                               */
