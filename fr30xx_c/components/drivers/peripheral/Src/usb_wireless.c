/*
  ******************************************************************************
  * @file    usb_wireless.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   This file provides the high layer firmware functions to manage the 
  *          USB wireless Device.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  * 
  ******************************************************************************
  * How to use, for example:
  * 
  *    int main(void)
  *    {
  *        NVIC_ClearPendingIRQ(USBMCU_IRQn);
  *        NVIC_SetPriority(USBMCU_IRQn, 1);
  *        NVIC_EnableIRQ(USBMCU_IRQn);
  *
  *        usb_device_init();
  *        usb_wireless_init();
  *        
  *        // Wait for other initialization of the MCU
  * 
  *        usb_DP_Pullup_Enable();
  *
  *        while(1)
  *        {
  *            
  *        }
  *    }
  ******************************************************************************
*/
#include "fr30xx.h"

/* USB Standard Device Descriptor */
const uint8_t USB_Wireless_DeviceDesc[] =
{
    0x12,    /* bLength */
    0x01,    /* bDescriptorType */
    0x00,    /* bcdUSB */
    0x02,
    0xE0,    /* bDeviceClass: Wireless Controller */
    0x01,    /* bDeviceSubClass: RF Controller */
    0x01,    /* bDeviceProtocol: Bluetooth Programming */
    0x40,    /* bMaxPacketSize */
    0x12,    /* idVendor */
    0x0A,    /* idVendor */
    0x01,    /* idProduct */
    0x00,    /* idProduct */
    0x91,    /* bcdDevice rel. 88.91 */
    0x88,
    0x00,    /* Index of manufacturer string */
    0x02,    /* Index of product string */
    0x00,    /* Index of serial number string */
    0x01,    /* bNumConfigurations */
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_Wireless_ConfigurationDesc[] =
{
    /* Configuration Descriptor */
    0x09,    /* bLength */
    0x02,    /* bDescriptorType */
    0xC8,    /* wTotalLength */
    0x00,
    0x02,    /* bNumInterfaces */
    0x01,    /* bConfigurationValue */
    0x00,    /* iConfiguration */
    0xC0,    /* bmAttributes */
    0x32,    /* bMaxPower */

    /* HID Interface_0/0 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x00,    /* bInterfaceNumber */
    0x00,    /* bAlternateSetting */
    0x03,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 1 Descriptor */    
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x81,    /* bEndpointAddress: IN1. HCI event */
    0x03,    /* bmAttributes: Interrupt */ 
    0x10,    /* wMaxPacketSize: 16byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 2 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x02,    /* bEndpointAddress: OUT2. ACL Data */
    0x02,    /* bmAttributes: Bulk */ 
    0x40,    /* wMaxPacketSize: 64byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 2 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x82,    /* bEndpointAddress: IN2. ACL Data  */
    0x02,    /* bmAttributes: Bulk */
    0x40,    /* wMaxPacketSize: 64byte */
    0x00,
    0x01,    /* bInterval */

    /* HID Interface_1/0 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x00,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x03,    /* bEndpointAddress: OUT3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x00,    /* wMaxPacketSize: 0byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x83,    /* bEndpointAddress: IN3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x00,    /* wMaxPacketSize: 0byte */
    0x00,
    0x01,    /* bInterval */

    /* HID Interface_1/1 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x01,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x03,    /* bEndpointAddress: OUT3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x09,    /* wMaxPacketSize: 9byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x83,    /* bEndpointAddress: IN3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x09,    /* wMaxPacketSize: 9byte */
    0x00,
    0x01,    /* bInterval */

    /* HID Interface_1/2 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x02,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x03,    /* bEndpointAddress: OUT3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x11,    /* wMaxPacketSize: 17byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x83,    /* bEndpointAddress: IN3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x11,    /* wMaxPacketSize: 17byte */
    0x00,
    0x01,    /* bInterval */

    /* HID Interface_1/3 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x03,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x03,    /* bEndpointAddress: OUT3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x19,    /* wMaxPacketSize: 25byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x83,    /* bEndpointAddress: IN3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x19,    /* wMaxPacketSize: 25byte */
    0x00,
    0x01,    /* bInterval */

    /* HID Interface_1/4 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x04,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x03,    /* bEndpointAddress: OUT3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x21,    /* wMaxPacketSize: 33byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x83,    /* bEndpointAddress: IN3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x21,    /* wMaxPacketSize: 33byte */
    0x00,
    0x01,    /* bInterval */

    /* HID Interface_1/5 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x05,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x03,    /* bEndpointAddress: OUT3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x31,    /* wMaxPacketSize: 49byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x83,    /* bEndpointAddress: IN3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x31,    /* wMaxPacketSize: 49byte */
    0x00,
    0x01,    /* bInterval */

    /* HID Interface_1/6 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x06,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0xE0,    /* bInterfaceClass: Wireless */
    0x01,    /* bInterfaceSubClass: RF Controller */
    0x01,    /* bInterfaceProtocol: Bluetooth Programming */
    0x00,    /* iConfiguration */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x03,    /* bEndpointAddress: OUT3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x3F,    /* wMaxPacketSize: 63byte */
    0x00,
    0x01,    /* bInterval */

    /* Endpoint 3 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x83,    /* bEndpointAddress: IN3 */
    0x01,    /* bmAttributes: Isochronous, No Sync, Data */ 
    0x3F,    /* wMaxPacketSize: 63byte */
    0x00,
    0x01,    /* bInterval */
};

/* USB Standard Manufacture Descriptor */
const uint8_t USB_Wireless_ManufactureDesc[] =
{
    0x12,        /* bLength */    
    0x03,        /* bDescriptorType */    
    'F', 0x00,   /* BString */
    'R', 0x00,
    'E', 0x00,
    'Q', 0x00,
    'C', 0x00,
    'H', 0x00,
    'I', 0x00,
    'P', 0x00,
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_Wireless_ProductDesc[] =
{
    0x18,         /* bLength */
    0x03,         /* bDescriptorType */
    'F', 0x00,    /* BString */
    'R', 0x00,    
    'E', 0x00,    
    'Q', 0x00,    
    'C', 0x00,    
    'H', 0x00,    
    'I', 0x00,    
    'P', 0x00,    
    '-', 0x00,    
    'B', 0x00,    
    'T', 0x00,    
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_Wireless_SerialNumberDesc[] =
{
    0x1E,         /* bLength */
    0x03,         /* bDescriptorType */
    '2', 0x00,    /* BString */
    '0', 0x00,
    '2', 0x00,
    '3', 0x00,
    '-', 0x00,
    '1', 0x00,
    '0', 0x00,
    '0', 0x00,
    '1', 0x00,
    '-', 0x00,
    'B', 0x00,
    '1', 0x00,
    '3', 0x00,
    '5', 0x00,
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_Wireless_LanuageIDDesc[] =
{
    0x04,    /* bLength */
    0x03,    /* bDescriptorType */
    0x09,    /* BString */
    0x04,
};

str_Wrieless_HCI_Handle_t Wireless_HCI;

/*********************************************************************
 * @fn      usb_wireless_Endpoints_Handler
 *
 * @brief   wireless Endpoints Handler
 */
void usb_wrieless_receive_CMD(void)
{
    uint8_t lu8_RxCount;

    lu8_RxCount = usb_Endpoint0_get_RxCount();

    usb_read_fifo(ENDPOINT_0, &Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Data[Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Count], lu8_RxCount);

    usb_Endpoint0_FlushFIFO();

    Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Count += lu8_RxCount;

    if (Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Count >= Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Length)
    {
        Endpoint_0_DataOut_Handler = NULL;

#if (HCI_ENDPOINT_DEBUG)
        printf("E0: ");
        for (int i=0; i<Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Count; i++)
            printf("%02X ", Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Data[i]);
        printf("\r\n");
#endif

        Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Residual = Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Count;

        Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Count  = 0;
        Wireless_HCI.Current_CMD_RxPacket->HCI_RxPacket_Length = 0;

        Wireless_HCI.Current_CMD_RxPacket = NULL;
    }
}

/*********************************************************************
 * @fn      usb_wrieless_send_event
 *
 * @brief   wireless send event
 */
void usb_wrieless_send_event(void)
{
    if (Wireless_HCI.HCI_Event_Length == 0)
        return;
    
    /* Wait HCI Event Tx END */
    while(Wireless_HCI.HCI_Event_Status);

    Wireless_HCI.HCI_Event_Status = HCI_EVENT_STATUS_BUSY;

    usb_selecet_endpoint(ENDPOINT_1);

    /* wait FIFO empty */
    while (usb_Endpoints_GET_TxPktRdy());

    if (Wireless_HCI.HCI_Event_Length >= HCI_EVENT_PACKET_MAX_LENGTH)
    {
        usb_write_fifo(ENDPOINT_1, &Wireless_HCI.HCI_Event[Wireless_HCI.HCI_Event_Count], HCI_EVENT_PACKET_MAX_LENGTH);
        Wireless_HCI.HCI_Event_Count  += HCI_EVENT_PACKET_MAX_LENGTH;
        Wireless_HCI.HCI_Event_Length -= HCI_EVENT_PACKET_MAX_LENGTH;
    }
    else
    {
        usb_write_fifo(ENDPOINT_1, &Wireless_HCI.HCI_Event[Wireless_HCI.HCI_Event_Count], Wireless_HCI.HCI_Event_Length);
        Wireless_HCI.HCI_Event_Count  = 0;
        Wireless_HCI.HCI_Event_Length = 0;
    }
    usb_Endpoints_SET_TxPktRdy();
}

/*********************************************************************
 * @fn      usb_wrieless_send_acl
 *
 * @brief   wireless send event
 */
void usb_wrieless_send_acl(void)
{
    if (Wireless_HCI.HCI_ACLT_Length == 0)
        return;

    /* Wait HCI ACL Tx END */
    while(Wireless_HCI.HCI_ACLT_Status);

    Wireless_HCI.HCI_ACLT_Status = HCI_ACLT_STATUS_BUSY;

    usb_selecet_endpoint(ENDPOINT_2);

    /* wait FIFO empty */
    while (usb_Endpoints_GET_TxPktRdy());

    if (Wireless_HCI.HCI_ACLT_Length >= HCI_ACLT_PACKET_MAX_LENGTH)
    {
        usb_write_fifo(ENDPOINT_2, &Wireless_HCI.HCI_ACLT[Wireless_HCI.HCI_ACLT_Count], HCI_ACLT_PACKET_MAX_LENGTH);
        Wireless_HCI.HCI_ACLT_Count  += HCI_ACLT_PACKET_MAX_LENGTH;
        Wireless_HCI.HCI_ACLT_Length -= HCI_ACLT_PACKET_MAX_LENGTH;
    }
    else
    {
        usb_write_fifo(ENDPOINT_2, &Wireless_HCI.HCI_ACLT[Wireless_HCI.HCI_ACLT_Count], Wireless_HCI.HCI_ACLT_Length);
        Wireless_HCI.HCI_ACLT_Count  = 0;
        Wireless_HCI.HCI_ACLT_Length = 0;
    }
    usb_Endpoints_SET_TxPktRdy();
}

/*********************************************************************
 * @fn      usb_wireless_Endpoints_Handler
 *
 * @brief   wireless Endpoints Handler
 *
 * @param   None.
 * @return  None.
 */
void usb_wireless_Endpoints_Handler(uint8_t RxStatus, uint8_t TxStatus)
{
    int i;
    uint8_t lu8_RxCount;

    /* Send HCI Event */
    if (TxStatus & (1 << ENDPOINT_1))
    {
        if (Wireless_HCI.HCI_Event_Length)
        {
            usb_selecet_endpoint(ENDPOINT_1);

            if (Wireless_HCI.HCI_Event_Length >= HCI_EVENT_PACKET_MAX_LENGTH)
            {
                usb_write_fifo(ENDPOINT_1, &Wireless_HCI.HCI_Event[Wireless_HCI.HCI_Event_Count], HCI_EVENT_PACKET_MAX_LENGTH);
                Wireless_HCI.HCI_Event_Count  += HCI_EVENT_PACKET_MAX_LENGTH;
                Wireless_HCI.HCI_Event_Length -= HCI_EVENT_PACKET_MAX_LENGTH;
            }
            else
            {
                usb_write_fifo(ENDPOINT_1, &Wireless_HCI.HCI_Event[Wireless_HCI.HCI_Event_Count], Wireless_HCI.HCI_Event_Length);
                Wireless_HCI.HCI_Event_Count  = 0;
                Wireless_HCI.HCI_Event_Length = 0;
            }
            usb_Endpoints_SET_TxPktRdy();
        }
        else
        {
            Wireless_HCI.HCI_Event_Status = HCI_EVENT_STATUS_IDLE;
        }
    }

    /* ACL Send */
    if (TxStatus & (1 << ENDPOINT_2))
    {
        if (Wireless_HCI.HCI_ACLT_Length)
        {
            usb_selecet_endpoint(ENDPOINT_2);

            if (Wireless_HCI.HCI_ACLT_Length >= HCI_ACLT_PACKET_MAX_LENGTH)
            {
                usb_write_fifo(ENDPOINT_2, &Wireless_HCI.HCI_ACLT[Wireless_HCI.HCI_ACLT_Count], HCI_ACLT_PACKET_MAX_LENGTH);
                Wireless_HCI.HCI_ACLT_Count  += HCI_ACLT_PACKET_MAX_LENGTH;
                Wireless_HCI.HCI_ACLT_Length -= HCI_ACLT_PACKET_MAX_LENGTH;
            }
            else
            {
                usb_write_fifo(ENDPOINT_2, &Wireless_HCI.HCI_ACLT[Wireless_HCI.HCI_ACLT_Count], Wireless_HCI.HCI_ACLT_Length);
                Wireless_HCI.HCI_ACLT_Count  = 0;
                Wireless_HCI.HCI_ACLT_Length = 0;
            }
            usb_Endpoints_SET_TxPktRdy();
        }
        else
        {
            Wireless_HCI.HCI_ACLT_Status = HCI_ACLT_STATUS_IDLE;
        }
    }

    /* ACL Receive */
    if (RxStatus & (1 << ENDPOINT_2))
    {
        usb_selecet_endpoint(ENDPOINT_2);

        lu8_RxCount = usb_Endpoints_get_RxCount();

        /* New ACL packet */
        if (Wireless_HCI.Current_ACL_RxPacket == NULL)
        {
            uint8_t Buffer[64];
            uint32_t RxPacket_length;

            usb_read_fifo(ENDPOINT_2, Buffer, lu8_RxCount);
            usb_Endpoints_FlushRxFIFO();

            RxPacket_length = (Buffer[3] << 8) + Buffer[2] + 5;    // 5=4+1

            str_Wrieless_HCI_RxPacket_t *HCI_ACL_RxPacket;

            HCI_ACL_RxPacket = wireless_malloc(sizeof(str_Wrieless_HCI_RxPacket_t) + RxPacket_length);

            HCI_ACL_RxPacket->HCI_RxPacket_Type     = HCI_TYPE_ACL_DATA_PACKET;
            HCI_ACL_RxPacket->HCI_RxPacket_Data[0]  = HCI_TYPE_ACL_DATA_PACKET;
            HCI_ACL_RxPacket->HCI_RxPacket_Length   = RxPacket_length;
            HCI_ACL_RxPacket->HCI_RxPacket_Count    = 1;
            HCI_ACL_RxPacket->HCI_RxPacket_Residual = 0;

            for (i = 0; i < lu8_RxCount; i++)
                HCI_ACL_RxPacket->HCI_RxPacket_Data[HCI_ACL_RxPacket->HCI_RxPacket_Count++] = Buffer[i];

            /* ACL packet finished */
            if (HCI_ACL_RxPacket->HCI_RxPacket_Count >= HCI_ACL_RxPacket->HCI_RxPacket_Length)
            {
#if (HCI_ENDPOINT_DEBUG)
                printf("E2R: ");
                for (i=0; i<HCI_ACL_RxPacket->HCI_RxPacket_Count; i++)
                    printf("%02X ", HCI_ACL_RxPacket->HCI_RxPacket_Data[i]);
                printf("\r\n");
#endif
                HCI_ACL_RxPacket->HCI_RxPacket_Residual = HCI_ACL_RxPacket->HCI_RxPacket_Count;

                HCI_ACL_RxPacket->HCI_RxPacket_Count  = 0;
                HCI_ACL_RxPacket->HCI_RxPacket_Length = 0;

                Wireless_HCI.Current_ACL_RxPacket = NULL;
            }
            else
            {
                Wireless_HCI.Current_ACL_RxPacket = HCI_ACL_RxPacket;
            }

            fr_list_insert_after(&Wireless_HCI.HCI_RxPacket_list, (fr_list_t *)HCI_ACL_RxPacket);
        }
        else
        {
            usb_read_fifo(ENDPOINT_2, &Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Data[Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Count], lu8_RxCount);
            usb_Endpoints_FlushRxFIFO();

            Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Count += lu8_RxCount;

            /* ACL packet finished */
            if (Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Count >= Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Length)
            {
#if (HCI_ENDPOINT_DEBUG)
                printf("E2R: ");
                for (i=0; i<Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Count; i++)
                    printf("%02X ", Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Data[i]);
                printf("\r\n");
#endif
                Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Residual = Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Count;

                Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Count  = 0;
                Wireless_HCI.Current_ACL_RxPacket->HCI_RxPacket_Length = 0;

                Wireless_HCI.Current_ACL_RxPacket = NULL;
            }
        }
    }
}

/*********************************************************************
 * @fn      usb_wrieless_ClassRequest_Handler
 *
 * @brief   wireless Endpoints Handler
 *
 * @param   None.
 * @return  None.
 */
void usb_wrieless_ClassRequest_Handler(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData)
{
    uint32_t RxPacket_length;

    Endpoint_0_DataOut_Handler = usb_wrieless_receive_CMD;

    RxPacket_length = (pStandardRequest->wLength[1] << 8) + pStandardRequest->wLength[0] + 1;

    str_Wrieless_HCI_RxPacket_t *HCI_CMD_RxPacket;

    HCI_CMD_RxPacket = wireless_malloc(sizeof(str_Wrieless_HCI_RxPacket_t) + RxPacket_length);

    HCI_CMD_RxPacket->HCI_RxPacket_Type     = HCI_TYPE_COMMAND_PACKET;
    HCI_CMD_RxPacket->HCI_RxPacket_Data[0]  = HCI_TYPE_COMMAND_PACKET;
    HCI_CMD_RxPacket->HCI_RxPacket_Length   = RxPacket_length;
    HCI_CMD_RxPacket->HCI_RxPacket_Count    = 1;
    HCI_CMD_RxPacket->HCI_RxPacket_Residual = 0;

    Wireless_HCI.Current_CMD_RxPacket = HCI_CMD_RxPacket;

    fr_list_insert_after(&Wireless_HCI.HCI_RxPacket_list, (fr_list_t *)HCI_CMD_RxPacket);
}

/*********************************************************************
 * @fn      usb_wireless_init
 *
 * @brief   wireless device parameter initialization 
 *
 * @param   None.
 * @return  None.
 */
void usb_wireless_init(void)
{
    /* Initialize the relevant pointer  */
    usbdev_get_dev_desc((uint8_t *)USB_Wireless_DeviceDesc);
    usbdev_get_config_desc((uint8_t *)USB_Wireless_ConfigurationDesc);
    usbdev_get_string_Manufacture((uint8_t *)USB_Wireless_ManufactureDesc);
    usbdev_get_string_Product((uint8_t *)USB_Wireless_ProductDesc);
    usbdev_get_string_SerialNumber((uint8_t *)USB_Wireless_SerialNumberDesc);
    usbdev_get_string_LanuageID((uint8_t *)USB_Wireless_LanuageIDDesc);

    // Endpoint_0_StandardClassRequest_Handler = usb_hid_StandardClassRequest_Handler;
    Endpoint_0_ClassRequest_Handler = usb_wrieless_ClassRequest_Handler;

    Endpoints_Handler = usb_wireless_Endpoints_Handler;

    USB_Reset_Handler = usb_wireless_init;

    /* config data endpoint fifo */
    usb_selecet_endpoint(ENDPOINT_1);
    usb_endpoint_Txfifo_config(64/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);
    usb_TxInt_Enable(ENDPOINT_1);

    usb_selecet_endpoint(ENDPOINT_2);
    usb_endpoint_Txfifo_config(128/8, 3);
    usb_endpoint_Rxfifo_config(192/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);
    usb_TxInt_Enable(ENDPOINT_2);
    usb_RxInt_Enable(ENDPOINT_2);

    usb_selecet_endpoint(ENDPOINT_3);
    usb_endpoint_Txfifo_config(256/8, 3);
    usb_endpoint_Rxfifo_config(320/8, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    fr_list_init(&Wireless_HCI.HCI_RxPacket_list);
}
