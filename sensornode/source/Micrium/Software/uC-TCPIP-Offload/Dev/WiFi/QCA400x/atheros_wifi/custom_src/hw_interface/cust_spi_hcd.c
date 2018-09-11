//------------------------------------------------------------------------------
// Copyright (c) Qualcomm Atheros, Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
// the limitations in the disclaimer below) provided that the following conditions are met:
//
// · Redistributions of source code must retain the above copyright notice, this list of conditions and the
//   following disclaimer.
// · Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//   following disclaimer in the documentation and/or other materials provided with the distribution.
// · Neither the name of nor the names of its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
// INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
// USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <custom_spi_api.h>
#include <common_api.h>
#include <atheros_wifi.h>
#include <qca.h>

#define POWER_UP_DELAY (1)

#define HW_SPI_CAPS (HW_SPI_FRAME_WIDTH_8| \
                     HW_SPI_NO_DMA | \
                     HW_SPI_INT_EDGE_DETECT)


QOSAL_VOID Custom_HW_SetClock (QOSAL_VOID    *pCxt,
                           QOSAL_UINT32  *pClockRate)
{
    QCA_CTX      *p_qca;
    QCA_DEV_BSP  *p_dev_bsp;
    QCA_DEV_CFG  *p_dev_cfg;


    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_bsp  = (QCA_DEV_BSP *)p_qca->Dev_BSP;
    p_dev_cfg  = (QCA_DEV_CFG *)p_qca->Dev_Cfg;


    p_dev_bsp->SPI_SetCfg (*pClockRate,
                            p_dev_cfg->SPI_ClkPol,
                            p_dev_cfg->SPI_ClkPhase,
                            p_dev_cfg->SPI_XferUnitLen,
                            p_dev_cfg->SPI_XferShiftDir);

}

QOSAL_VOID Custom_Hcd_EnableDisableSPIIRQ(QOSAL_VOID *pCxt,
                                     QOSAL_BOOL  enable)
{

    QCA_CTX      *p_qca;
    QCA_DEV_BSP  *p_dev_bsp;

    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_bsp  = (QCA_DEV_BSP *)p_qca->Dev_BSP;


    if (enable == QOSAL_TRUE){
        p_dev_bsp->IntCtrl(DEF_TRUE);
    } else {
        p_dev_bsp->IntCtrl(DEF_FALSE);
    }

}

QOSAL_VOID Custom_HW_UsecDelay(QOSAL_VOID   *pCxt,
                           QOSAL_UINT32  uSeconds)
{
    if (uSeconds < 1000) {
        KAL_Dly(1);
    } else {
        KAL_Dly(uSeconds/1000);
    }

    (void)pCxt;
}

QOSAL_VOID Custom_HW_PowerUpDown(QOSAL_VOID   *pCxt,
                             QOSAL_UINT32  powerUp)
{
    QCA_CTX      *p_qca;
    QCA_DEV_BSP  *p_dev_bsp;


    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_bsp  = (QCA_DEV_BSP *)p_qca->Dev_BSP;


    if (powerUp){
        p_dev_bsp->Start();
    } else {
        p_dev_bsp->Stop();
    }


}

/*****************************************************************************/
/* Custom_Bus_InOutBuffer - This is the platform specific solution to
 *  transfer a buffer on the SPI bus.  This solution is always synchronous
 *  regardless of sync param. The function will use the MQX fread and fwrite
 *  as appropriate.
 *      QOSAL_VOID * pCxt - the driver context.
 *      QOSAL_UINT8 *pBuffer - The buffer to transfer.
 *      QOSAL_UINT16 length - the length of the transfer in bytes.
 *      QOSAL_UINT8 doRead - 1 if operation is a read else 0.
 *      QOSAL_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS Custom_Bus_InOutBuffer(QOSAL_VOID   *pCxt,
                                QOSAL_UINT8  *pBuffer,
                                QOSAL_UINT16 length,
                                QOSAL_UINT8  doRead,
                                QOSAL_BOOL   sync)
{
    A_STATUS               status;
    QCA_CTX      *p_qca;
    QCA_DEV_BSP  *p_dev_bsp;


    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_bsp  = (QCA_DEV_BSP *)p_qca->Dev_BSP;


    status = A_OK;



    UNUSED_ARGUMENT(sync);
    /* this function takes advantage of the SPI turbo mode which does not toggle the chip select
     * during the transfer.  Rather the chip select is asserted at the beginning of the transfer
     * and de-asserted at the end of the entire transfer via fflush(). */
    if(doRead){
        p_dev_bsp->SPI_WrRd( DEF_NULL,
                             pBuffer,
                             length );

    }else{
        p_dev_bsp->SPI_WrRd( pBuffer,
                             DEF_NULL,
                             length);

    }

    return status;
}

/*****************************************************************************/
/* Custom_Bus_InOut_Token - This is the platform specific solution to
 *  transfer 4 or less bytes in both directions. The transfer must be
 *  synchronous. This solution uses the MQX spi ioctl to complete the request.
 *      A_VOID * pCxt - the driver context.
 *      A_UINT32 OutToken - the out going data.
 *      A_UINT8 DataSize - the length in bytes of the transfer.
 *      A_UINT32 *pInToken - A Buffer to hold the incoming bytes.
 *****************************************************************************/
A_STATUS Custom_Bus_InOutToken(QOSAL_VOID   *pCxt,
                               QOSAL_UINT32  OutToken,
                               QOSAL_UINT8   DataSize,
                               QOSAL_UINT32 *pInToken)
{
    QCA_CTX      *p_qca;
    QCA_DEV_BSP  *p_dev_bsp;
    CPU_INT08U            *p_char_out;
    CPU_INT08U            *p_char_in;
    A_STATUS               status;


    status = A_OK;

    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_bsp  = (QCA_DEV_BSP *)p_qca->Dev_BSP;

    p_char_out = (CPU_INT08U *)&OutToken ;
    p_char_in  = (CPU_INT08U *) pInToken;
    // data size if really a enum that is 1 less than number of bytes

    do{
        if (A_OK != Custom_Bus_StartTransfer(pCxt, QOSAL_TRUE))
        {
            status = A_HARDWARE;
            break;
        }
        p_dev_bsp->SPI_WrRd( p_char_out,
                             p_char_in,
                             DataSize);

        Custom_Bus_CompleteTransfer(pCxt, QOSAL_TRUE);
    }while(0);

    return status;
}

/*****************************************************************************/
/* Custom_Bus_Start_Transfer - This function is called by common layer prior
 *  to starting a new bus transfer. This solution merely sets up the SPI
 *  mode as a precaution.
 *      QOSAL_VOID * pCxt - the driver context.
 *      QOSAL_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS Custom_Bus_StartTransfer(QOSAL_VOID *pCxt,
                                  QOSAL_BOOL  sync)
{
    QCA_CTX      *p_qca;
    QCA_DEV_BSP  *p_dev_bsp;
    A_STATUS      status;


    status = A_OK;

    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_bsp  = (QCA_DEV_BSP *)p_qca->Dev_BSP;

    p_dev_bsp->SPI_Lock();
    p_dev_bsp->SPI_SetCfg(p_qca->Dev_Cfg->SPI_ClkFreq,
                          p_qca->Dev_Cfg->SPI_ClkPol,
                          p_qca->Dev_Cfg->SPI_ClkPhase,
                          p_qca->Dev_Cfg->SPI_XferUnitLen,
                          p_qca->Dev_Cfg->SPI_XferShiftDir);
    p_dev_bsp->SPI_ChipSelEn();

    UNUSED_ARGUMENT(sync);

    return status;
}

/*****************************************************************************/
/* Custom_Bus_Complete_Transfer - This function is called by common layer prior
 *  to completing a bus transfer. This solution calls fflush to de-assert
 *  the chipselect.
 *      QOSAL_VOID * pCxt - the driver context.
 *      QOSAL_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS Custom_Bus_CompleteTransfer(QOSAL_VOID *pCxt,
                                     QOSAL_BOOL sync)
{

    QCA_CTX      *p_qca;
    QCA_DEV_BSP  *p_dev_bsp;
    A_STATUS       status;


    status = A_OK;

    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_bsp  = (QCA_DEV_BSP *)p_qca->Dev_BSP;

    p_dev_bsp->SPI_ChipSelDis();
    p_dev_bsp->SPI_Unlock();


    UNUSED_ARGUMENT(sync);

    return status;
}


A_STATUS Custom_HW_Init(QOSAL_VOID *pCxt)
{

    QCA_CTX      *p_qca;
    QCA_DEV_CFG  *p_dev_cfg;
    A_DRIVER_CONTEXT *pDCxt;

    p_qca      = (QCA_CTX     *)pCxt;
    p_dev_cfg  = (QCA_DEV_CFG *)p_qca->Dev_Cfg;


    pDCxt      = GET_DRIVER_COMMON(pCxt);
                                                                /* -------------------- HW INIT ----------------------- */
                                                                /* Hardware initialization is done in the driver API... */
                                                                /* in NetIF_WiFi_IF_Add() via 'p_dev_api->Init()'.      */

                                                                /* ---------------- SET UP HCD PARAMS ----------------- */
                                                                /* IT is necessary for the custom code to populate ...  */
                                                                /* ...the following parameters so that the common...    */
                                                                /* ...code knows what kind of hardware it is working... */
                                                                /* ...with.                                             */
    pDCxt->spi_hcd.OperationalClock = p_dev_cfg->SPI_ClkFreq;
    pDCxt->spi_hcd.PowerUpDelay = POWER_UP_DELAY;
    pDCxt->spi_hcd.SpiHWCapabilitiesFlags = HW_SPI_CAPS;
    pDCxt->spi_hcd.MiscFlags |= MISC_FLAG_RESET_SPI_IF_SHUTDOWN;

    return A_OK;
}

A_STATUS Custom_HW_DeInit(QOSAL_VOID *pCxt)
{
    return A_OK;
}
