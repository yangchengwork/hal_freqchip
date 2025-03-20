/*
  ******************************************************************************
  * @file    usb_core.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   This file provides all the USB core functions.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/*********************************************************************
 * @fn      usb_device_init
 *
 * @brief   Initializes the usb-otg as a device.
 */
void usb_device_init(void)
{
    /* OTG_CID      = 1 */
    /* OTG_VBUS_LO  = 1 */
    /* OTG_VBUS_SES = 1 */
    /* OTG_VBUS_VAL = 1 */
    /* USB_PHY_ADP_CFG = 9 */
    USB_OTG_CTRL = USB_OTG_CTRL_DEVICE_CFG;

    /* Disable USB all Interrupt except endpoint0 and Bus reset*/
    USB->IntrUSBE = 0x04;    /* Enable Bus reset INT */
    USB->IntrTx1E = 0x01;    /* Enable Endpoint0 INT */
    USB->IntrTx2E = 0x00;
    USB->IntrRx1E = 0x00;
    USB->IntrRx2E = 0x00;
}

/*********************************************************************
 * @fn      usb_host_init
 *
 * @brief   Initializes the usb-otg as a host.
 */
void usb_host_init(void)
{
    /* OTG_CID      = 0 */
    /* OTG_VBUS_LO  = 1 */
    /* OTG_VBUS_SES = 1 */
    /* OTG_VBUS_VAL = 1 */
    /* USB_PHY_ADP_CFG = 9 */
    USB_OTG_CTRL = USB_OTG_CTRL_HOST_CFG;

    /* Disable USB all Interrupt except endpoint0 and Bus reset*/
    USB->IntrUSBE = 0x30;    /* Enable Bus reset INT */
    USB->IntrTx1E = 0x01;    /* Enable Endpoint0 INT */
    USB->IntrTx2E = 0x00;
    USB->IntrRx1E = 0x00;
    USB->IntrRx2E = 0x00;
    
    /* config data endpoint fifo */
    usb_selecet_endpoint(ENDPOINT_1);   // 64Byte
    usb_endpoint_Txfifo_config(64/8,  3);
    usb_endpoint_Rxfifo_config(128/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_2);   // 64Byte
    usb_endpoint_Txfifo_config(192/8, 3);
    usb_endpoint_Rxfifo_config(256/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_3);   // 64Byte
    usb_endpoint_Txfifo_config(320/8, 3);
    usb_endpoint_Rxfifo_config(384/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_4);   // 64Byte
    usb_endpoint_Txfifo_config(448/8, 3);
    usb_endpoint_Rxfifo_config(512/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_5);   // 64Byte
    usb_endpoint_Txfifo_config(576/8, 3);
    usb_endpoint_Rxfifo_config(640/8, 3);
    
    usb_selecet_endpoint(ENDPOINT_6);   // 64Byte
    usb_endpoint_Txfifo_config(704/8, 3);
    usb_endpoint_Rxfifo_config(768/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_7);   // 64Byte
    usb_endpoint_Txfifo_config(832/8, 3);
    usb_endpoint_Rxfifo_config(896/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);    

    usb_selecet_endpoint(ENDPOINT_0);
    usb_Host_Endpoint0_NAKlimit(0xFF);    
}

/*********************************************************************
 * @fn      usb_selecet_endpoint
 *
 * @brief   Selected Endpoint
 *
 * @param   Endpoint : endpoint select.
 * @return  None.
 */
void usb_selecet_endpoint(enum_Endpoint_t Endpoint)
{
    USB->Index = Endpoint;
}

/*********************************************************************
 * @fn      usb_get_endpoint
 *
 * @brief   get current Endpoint
 *
 * @param   None.
 * @return  current Endpoint
 */
uint8_t usb_get_endpoint(void)
{
    return USB->Index;
}

/*********************************************************************
 * @fn      usb_set_address
 *
 * @brief   set device address
 *
 * @param   address : device address.
 * @return  None.
 */
void usb_set_address(uint8_t address)
{
    USB->FAddr = address;
}

/*********************************************************************
 * @fn      usb_get_frame
 *
 * @brief   get current frame number
 *
 * @param   None.
 * @return  None.
 */
uint32_t usb_get_frame(void)
{
    uint32_t lu32_Frame;

    lu32_Frame  = (uint32_t)USB->Frame2 << 8;
    lu32_Frame |= (uint32_t)USB->Frame1;

    return lu32_Frame;
}

/*********************************************************************
 * @fn      usb_TxEndpointSync_enable
 *
 * @brief   Tx Synchronous endpoint enable
 */
void usb_TxSyncEndpoint_enable(void)
{
	USB_POINTS->TxCSR2 |= USB_TXCSR2_ISO;
}

/*********************************************************************
 * @fn      usb_RxSyncEndpoint_enable
 *
 * @brief   Rx Synchronous endpoint enable
 */
void usb_RxSyncEndpoint_enable(void)
{
    USB_POINTS->RxCSR2 |= USB_RXCSR2_DEVICE_ISO;
}

/*********************************************************************
 * @fn      usb_SignalInt_Enable
 *
 * @brief   Enable Signal detect interrupt.
 *
 * @param   fu8_Signal : Signal select.
 * @return  None.
 */
void usb_SingleInt_Enable(uint8_t fu8_Signal)
{
    USB->IntrUSBE |= fu8_Signal;
}

/*********************************************************************
 * @fn      usb_SignalInt_Disable
 *
 * @brief   Disable Signal detect interrupt.
 *
 * @param   fu8_Signal : Signal select.
 * @return  None.
 */
void usb_SignalInt_Disable(uint8_t fu8_Signal)
{
    USB->IntrUSBE &= ~fu8_Signal;
}

/*********************************************************************
 * @fn      usb_TxInt_Enable
 *
 * @brief   Enable transmit completion interrupt
 *
 * @param   Endpoint : endpoint select.
 * @return  None.
 */
void usb_TxInt_Enable(enum_Endpoint_t Endpoint)
{
    USB->IntrTx1E |= 1 << Endpoint;
}

/*********************************************************************
 * @fn      usb_TxInt_Disable
 *
 * @brief   Disable transmit completion interrupt
 *
 * @param   Endpoint : endpoint select.
 * @return  None.
 */
void usb_TxInt_Disable(enum_Endpoint_t Endpoint)
{
    USB->IntrTx1E &= ~(1 << Endpoint);
}

/*********************************************************************
 * @fn      usb_RxInt_Enable
 *
 * @brief   Enable receive completion interrupt
 *
 * @param   Endpoint : endpoint select.
 * @return  None.
 */
void usb_RxInt_Enable(enum_Endpoint_t Endpoint)
{
    if (Endpoint == ENDPOINT_0)
        return;

    USB->IntrRx1E |= 1 << Endpoint;
}

/*********************************************************************
 * @fn      usb_RxInt_Disable
 *
 * @brief   Disable receive completion interrupt
 *
 * @param   Endpoint : endpoint select.
 * @return  None.
 */
void usb_RxInt_Disable(enum_Endpoint_t Endpoint)
{
    if (Endpoint == ENDPOINT_0)
        return;

    USB->IntrRx1E &= ~(1 << Endpoint);
}

/*********************************************************************
 * @fn      usb_endpoint_Txfifo_config
 *
 * @brief   config Txfifo
 *
 * @param   StartAddress: Start address of the endpoint FIFO.
 *          MaxPacket   : Maximum packet size.
 *                        
 * @return  None.
 */
void usb_endpoint_Txfifo_config(uint32_t StartAddress, uint32_t MaxPacket)
{
    /* Start address of the endpoint FIFO in units of 8 TxFIFO1 bytes as follows: */
    /* --------------------------------- */
    /* |   StartAddress   |   Address  | */
    /* |       0x00       |    0x000   | */
    /* |       0x01       |    0x008   | */
    /* |       0x02       |    0x010   | */
    /* |      ...         |     ...    | */
    /* |       0x7F       |    0x3FF   | */
    /* --------------------------------- */
    
    
    /* Maximum packet size to be allowed for */
    /* --------------------------------------- */
    /* |   MaxPacket   |  Packet Size(Bytes) | */
    /* |       0       |          8          | */
    /* |       1       |          16         | */
    /* |       2       |          32         | */
    /* |       3       |          64         | */
    /* |       4       |          128        | */
    /* |       5       |          256        | */
    /* |       6       |          512        | */
    /* |       7       |          1024       | */
    /* --------------------------------------- */

    /* use only 7bit */
    StartAddress &= 0x7F;
    
    /* use only 3bit */
    MaxPacket &= 0x7;

    USB_POINTS->TxFIFO1 = StartAddress;
    
    USB_POINTS->TxFIFO2 = MaxPacket << 5;
}

/*********************************************************************
 * @fn      usb_endpoint_Rxfifo_config
 *
 * @brief   config Rxfifo
 *
 * @param   StartAddress: Start address of the endpoint FIFO.
 *          MaxPacket   : Maximum packet size.
 *                        
 * @return  None.
 */
void usb_endpoint_Rxfifo_config(uint32_t StartAddress, uint32_t MaxPacket)
{
    /* 
        reference usb_endpoint_Txfifo_config()
     */

    /* use only 7bit */
    StartAddress &= 0x7F;

    /* use only 3bit */
    MaxPacket &= 0x7;

    USB_POINTS->RxFIFO1 = StartAddress;

    USB_POINTS->RxFIFO2 = MaxPacket << 5;
}

/*********************************************************************
 * @fn      usb_TxMaxP_set
 *
 * @brief   the maximum packet size for transactions through the 
 *          currently-selected Tx endpoint
 *
 * @param   MaxPacket: in units of 8 bytes
 * @return  None.
 */
void usb_TxMaxP_set(uint32_t MaxPacket)
{
    /* Maximum packet size to be allowed for */
    /* --------------------------------------- */
    /* |   MaxPacket   |  Packet Size(Bytes) | */
    /* |       0       |          0          | */
    /* |       1       |          8          | */
    /* |       2       |          16         | */
    /* |      ...      |          ...        | */
    /* |       128     |          1024       | */
    /* --------------------------------------- */

    USB_POINTS->TxMaxP = MaxPacket;
}

/*********************************************************************
 * @fn      usb_RxMaxP_set
 *
 * @brief   the maximum packet size for transactions through the 
 *          currently-selected Rx endpoint
 *
 * @param   MaxPacket: in units of 8 bytes
 * @return  None.
 */
void usb_RxMaxP_set(uint32_t MaxPacket)
{
    /* 
        reference usb_TxMaxP_set()
     */
    
    USB_POINTS->RxMaxP = MaxPacket;
}

/*********************************************************************
 * @fn      usb_Host_TxEndpointType
 *
 * @brief   In host mode, the tx endpoint type select.
 *
 * @param   fe_Type: Endpoint Type
 * @return  None.
 */
void usb_Host_TxEndpointType(enum_HostEndpointType_t fe_Type)
{
    USB_POINTS->TxType &= ~USB_HOST_TXTYPE_PROTOCOL_MSK;
    USB_POINTS->TxType |= fe_Type << USB_HOST_TXTYPE_PROTOCOL_POS;
}

/*********************************************************************
 * @fn      usb_Host_RxEndpointType
 *
 * @brief   In host mode, the rx endpoint type select.
 *
 * @param   fe_Type: Endpoint Type
 * @return  None.
 */
void usb_Host_RxEndpointType(enum_HostEndpointType_t fe_Type)
{
    USB_POINTS->RxType &= ~USB_HOST_RXTYPE_PROTOCOL_MSK;
    USB_POINTS->RxType |= fe_Type << USB_HOST_RXTYPE_PROTOCOL_POS;
}

/*********************************************************************
 * @fn      usb_Host_TxTargetEndpoint
 *
 * @brief   The tx target endpoint number of the host transmit.
 *
 * @param   fe_Type: Endpoint Type
 * @return  None.
 */
void usb_Host_TxTargetEndpoint(uint8_t fu8_TargetEndpointNum)
{
    USB_POINTS->TxType &= ~USB_HOST_TXTYPE_TARGET_ENDP_NUM_MSK;
    USB_POINTS->TxType |= fu8_TargetEndpointNum;
}

/*********************************************************************
 * @fn      usb_Host_RxTargetEndpoint
 *
 * @brief   The rx target endpoint number of the host receive.
 *
 * @param   fe_Type: Endpoint Type
 * @return  None.
 */
void usb_Host_RxTargetEndpoint(uint8_t fu8_TargetEndpointNum)
{
    USB_POINTS->RxType &= ~USB_HOST_RXTYPE_TARGET_ENDP_NUM_MSK;
    USB_POINTS->RxType |= fu8_TargetEndpointNum;
}

/*********************************************************************
 * @fn      usb_Host_TxPollingInterval
 *
 * @brief   Tx Polling Interval in interrup or isochronous transfers.
 *          The unit is ms.(1 ~ 255)
 *
 * @param   fu8_TxInterval: Polling interval.
 * @return  None.
 */
void usb_Host_TxPollingInterval(uint8_t fu8_TxInterval)
{
    USB_POINTS->TxInterval = fu8_TxInterval;
}

/*********************************************************************
 * @fn      usb_Host_TxNAKLimit
 *
 * @brief   Tx NAK limit in bulk transfer. NAK Limit 2 ~ 255.
 *          Note: A value of 0 or 1 disable the NAK timeout function.
 *
 * @param   fu8_TxNAKLimit: Tx NAK limit value.
 * @return  None.
 */
void usb_Host_TxNAKLimit(uint8_t fu8_TxNAKLimit)
{
    USB_POINTS->TxInterval = fu8_TxNAKLimit;
}

/*********************************************************************
 * @fn      usb_Host_RxPollingInterval
 *
 * @brief   Rx Polling Interval in interrup or isochronous transfers.
 *          The unit is ms.(1 ~ 255)
 *
 * @param   fu8_RxInterval: Polling interval.
 * @return  None.
 */
void usb_Host_RxPollingInterval(uint8_t fu8_RxInterval)
{
    USB_POINTS->RxInterval = fu8_RxInterval;
}

/*********************************************************************
 * @fn      usb_Host_RxNAKLimit
 *
 * @brief   Rx NAK limit in bulk transfer. NAK Limit 2 ~ 255.
 *          Note: A value of 0 or 1 disable the NAK timeout function.
 *
 * @param   fu8_RxNAKLimit: Rx NAK limit value.
 * @return  None.
 */
void usb_Host_RxNAKLimit(uint8_t fu8_RxNAKLimit)
{
    USB_POINTS->RxInterval = fu8_RxNAKLimit;
}

/*********************************************************************
 * @fn      usb_write_fifo
 *
 * @brief   Write data to the endpoint fifo
 *
 * @param   Endpoint : endpoint select.
 *          Buffer   : transmit buffer pointer.
 *          Size     : transmit Size.
 * @return  None.
 */
void usb_write_fifo(enum_Endpoint_t Endpoint, uint8_t *Buffer, uint32_t Size)
{
    volatile uint8_t *fifo;

    fifo = &USB_POINTS->FIFO[Endpoint * 4];

    while (Size--)
    {
        *fifo = *Buffer++;
    }
}

/*********************************************************************
 * @fn      usb_read_fifo
 *
 * @brief   Reads data from the endpoint fifo
 *
 * @param   Endpoint : endpoint select.
 *          Buffer   : receive buffer pointer.
 *          Size     : receive Size.
 * @return  None.
 */
void usb_read_fifo(enum_Endpoint_t Endpoint, uint8_t *Buffer, uint32_t Size)
{
    volatile uint8_t *fifo;

    fifo = &USB_POINTS->FIFO[Endpoint * 4];

    while (Size--)
    {
        *Buffer++ = *fifo;
    }
}
