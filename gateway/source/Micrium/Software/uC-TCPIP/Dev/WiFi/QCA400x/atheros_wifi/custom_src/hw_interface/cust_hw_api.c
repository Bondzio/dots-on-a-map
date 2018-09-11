//------------------------------------------------------------------------------
// Copyright (c) 2011 Qualcomm Atheros, Inc.
// All Rights Reserved.
// Qualcomm Atheros Confidential and Proprietary.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is
// hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
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
#include <custom_api.h>
#include <common_api.h>
#include <atheros_wifi.h>


#define POWER_UP_DELAY (1)

#define HW_SPI_CAPS (HW_SPI_FRAME_WIDTH_8| \
                     HW_SPI_NO_DMA | \
                     HW_SPI_INT_EDGE_DETECT)


A_VOID Custom_HW_SetClock(A_VOID    *pCxt,
                          A_UINT32  *pClockRate)
{
    NET_IF                *p_if;
    NET_DEV_BSP_WIFI_SPI  *p_dev_bsp;
    NET_DEV_CFG_WIFI_QCA  *p_dev_cfg;
    NET_ERR                net_err;

                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_cfg  = (NET_DEV_CFG_WIFI_QCA *)p_if->Dev_Cfg;         /* Obtain ptr to dev cfg area.                          */
    p_dev_bsp  = (NET_DEV_BSP_WIFI_SPI *)p_if->Dev_BSP;

    p_dev_bsp->SPI_SetCfg( p_if,
                          *pClockRate,
                           p_dev_cfg->SPI_ClkPol,
                           p_dev_cfg->SPI_ClkPhase,
                           p_dev_cfg->SPI_XferUnitLen,
                           p_dev_cfg->SPI_XferShiftDir,
                          &net_err);
    if (net_err != NET_DEV_ERR_NONE) {
         return;
    }
}

A_VOID Custom_HW_EnableDisableSPIIRQ(A_VOID *pCxt,
                                     A_BOOL  enable)
{

    NET_IF                *p_if;
    NET_DEV_BSP_WIFI_SPI  *p_dev_bsp;
    NET_ERR                net_err;

                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_bsp  = (NET_DEV_BSP_WIFI_SPI *)p_if->Dev_BSP;

    if (enable == A_TRUE){
        p_dev_bsp->IntCtrl(p_if,
                           DEF_TRUE,
                           &net_err);
    } else {
        p_dev_bsp->IntCtrl(p_if,
                           DEF_FALSE,
                           &net_err);
    }
    (void)net_err;
}

A_VOID Custom_HW_UsecDelay(A_VOID   *pCxt,
                           A_UINT32  uSeconds)
{
    if (uSeconds < 1000) {
        KAL_Dly(1);
    } else {
        KAL_Dly(uSeconds/1000);
    }

    (void)pCxt;
}

A_VOID Custom_HW_PowerUpDown(A_VOID   *pCxt,
                             A_UINT32  powerUp)
{
    NET_IF                *p_if;
    NET_DEV_BSP_WIFI_SPI  *p_dev_bsp;
    NET_ERR                net_err;

                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_bsp  = (NET_DEV_BSP_WIFI_SPI *)p_if->Dev_BSP;
    if (powerUp){
        p_dev_bsp->Start( p_if,
                         &net_err);
    } else {
        p_dev_bsp->Stop( p_if,
                        &net_err);
    }

    (void)net_err;

}

/*****************************************************************************/
/* Custom_Bus_InOutBuffer - This is the platform specific solution to
 *  transfer a buffer on the SPI bus.  This solution is always synchronous
 *  regardless of sync param. The function will use the MQX fread and fwrite
 *  as appropriate.
 *      A_VOID * pCxt - the driver context.
 *      A_UINT8 *pBuffer - The buffer to transfer.
 *      A_UINT16 length - the length of the transfer in bytes.
 *      A_UINT8 doRead - 1 if operation is a read else 0.
 *      A_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS Custom_Bus_InOutBuffer(A_VOID   *pCxt,
                                A_UINT8  *pBuffer,
                                A_UINT16 length,
                                A_UINT8  doRead,
                                A_BOOL   sync)
{
    A_STATUS               status;
    NET_IF                *p_if;
    NET_DEV_BSP_WIFI_SPI  *p_dev_bsp;
    NET_ERR                net_err;


    status = A_OK;

                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_bsp  = (NET_DEV_BSP_WIFI_SPI *)p_if->Dev_BSP;

    UNUSED_ARGUMENT(sync);
    /* this function takes advantage of the SPI turbo mode which does not toggle the chip select
     * during the transfer.  Rather the chip select is asserted at the beginning of the transfer
     * and de-asserted at the end of the entire transfer via fflush(). */
    if(doRead){
        p_dev_bsp->SPI_WrRd( p_if,
                             DEF_NULL,
                             pBuffer,
                             length,
                            &net_err);
        if(net_err !=  NET_DEV_ERR_NONE){
            status = A_HARDWARE;
        }
    }else{
        p_dev_bsp->SPI_WrRd( p_if,
                             pBuffer,
                             DEF_NULL,
                             length,
                            &net_err);
        if(net_err !=  NET_DEV_ERR_NONE){
            status = A_HARDWARE;
        }
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
A_STATUS Custom_Bus_InOutToken(A_VOID   *pCxt,
                               A_UINT32  OutToken,
                               A_UINT8   DataSize,
                               A_UINT32 *pInToken)
{
    NET_IF                *p_if;
    NET_DEV_BSP_WIFI_SPI  *p_dev_bsp;
    NET_ERR                net_err;
    CPU_INT08U            *p_char_out;
    CPU_INT08U            *p_char_in;
    A_STATUS               status;


    status = A_OK;
                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_bsp  = (NET_DEV_BSP_WIFI_SPI *)p_if->Dev_BSP;

    p_char_out = (CPU_INT08U *)&OutToken ;
    p_char_in  = (CPU_INT08U *) pInToken;
    // data size if really a enum that is 1 less than number of bytes

    do{
        if (A_OK != Custom_Bus_StartTransfer(pCxt, A_TRUE))
        {
            status = A_HARDWARE;
            break;
        }
        p_dev_bsp->SPI_WrRd( p_if,
                             p_char_out,
                             p_char_in,
                             DataSize,
                            &net_err);
        if(net_err !=  NET_DEV_ERR_NONE){
            status = A_HARDWARE;
        }

        Custom_Bus_CompleteTransfer(pCxt, A_TRUE);
    }while(0);

    return status;
}

/*****************************************************************************/
/* Custom_Bus_Start_Transfer - This function is called by common layer prior
 *  to starting a new bus transfer. This solution merely sets up the SPI
 *  mode as a precaution.
 *      A_VOID * pCxt - the driver context.
 *      A_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS
Custom_Bus_StartTransfer(A_VOID *pCxt, A_BOOL sync)
{
    NET_IF                *p_if;
    NET_DEV_BSP_WIFI_SPI  *p_dev_bsp;
    NET_ERR                net_err;
    A_STATUS               status;


    status = A_OK;
                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_bsp  = (NET_DEV_BSP_WIFI_SPI *)p_if->Dev_BSP;

    p_dev_bsp->SPI_ChipSelEn( p_if,
                             &net_err);

    if (net_err != NET_DEV_ERR_NONE){
        status = A_ERROR;
    }

    UNUSED_ARGUMENT(sync);

    return status;
}

/*****************************************************************************/
/* Custom_Bus_Complete_Transfer - This function is called by common layer prior
 *  to completing a bus transfer. This solution calls fflush to de-assert
 *  the chipselect.
 *      A_VOID * pCxt - the driver context.
 *      A_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS
Custom_Bus_CompleteTransfer(A_VOID *pCxt, A_BOOL sync)
{

    NET_IF                *p_if;
    NET_DEV_BSP_WIFI_SPI  *p_dev_bsp;
    A_STATUS               status;


    status = A_OK;
                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_bsp  = (NET_DEV_BSP_WIFI_SPI *)p_if->Dev_BSP;

    p_dev_bsp->SPI_ChipSelDis(p_if);

    UNUSED_ARGUMENT(sync);

    return status;
}

extern A_VOID *p_Global_Cxt;


/* interrupt service routine that gets mapped to the
 * SPI INT GPIO */


A_STATUS Custom_HW_Init(A_VOID *pCxt)
{
    NET_IF                *p_if;
    NET_DEV_CFG_WIFI_QCA  *p_dev_cfg;
    A_DRIVER_CONTEXT      *pDCxt;



                                                                /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
    p_if       = (NET_IF               *)pCxt;                  /* Obtain ptr to the interface area.                    */
    p_dev_cfg  = (NET_DEV_CFG_WIFI_QCA *)p_if->Dev_Cfg;         /* Obtain ptr to dev cfg area.                          */
    pDCxt      = GET_DRIVER_COMMON(pCxt);

                                                                /* -------------------- HW INIT ----------------------- */
                                                                /* Hardware initialization is done in the driver API... */
                                                                /* in NetIF_WiFi_IF_Add() via 'p_dev_api->Init()'.      */

                                                                /* ---------------- SET UP HCD PARAMS ----------------- */
                                                                /* IT is necessary for the custom code to populate ...  */
                                                                /* ...the following parameters so that the common...    */
                                                                /* ...code knows what kind of hardware it is working... */
                                                                /* ...with.                                             */
    pDCxt->hcd.OperationalClock = p_dev_cfg->SPI_ClkFreq;
    pDCxt->hcd.PowerUpDelay = POWER_UP_DELAY;
    pDCxt->hcd.SpiHWCapabilitiesFlags = HW_SPI_CAPS;
    pDCxt->hcd.MiscFlags |= MISC_FLAG_RESET_SPI_IF_SHUTDOWN;

    return A_OK;
}

A_STATUS Custom_HW_DeInit(A_VOID *pCxt)
{
    return A_OK;
}
