/*
  ******************************************************************************
  * @file    usb_cdc.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   This file provides the high layer firmware functions to manage the 
  *          USB CDC Class.(Communication Device Class)
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
  *        NVIC_SetPriority(USBMCU_IRQn, 0);
  *        NVIC_EnableIRQ(USBMCU_IRQn);
  *
  *        usb_device_init();
  *        usb_cdc_init();
  *
  *        // Wait for other initialization of the MCU

  *        while(1)
  *        {
  *            usb_cdc_serialReceive();
  *        }
  *    }
  ******************************************************************************
*/
#include "fr30xx.h"

extern UART_HandleTypeDef  Uart_CDC_handle;

#define CDC_MAX_PACK         (64)       /* Fixed 64 bytes */
#define CDC_MAX_IN_PACK      (1024)     /* IN Endpoint max pack buffer */

volatile bool COM_Activate = false;

volatile uint32_t Tx_Count = 0;
volatile uint32_t Tx_Total = 0;

uint8_t RxBuffer[CDC_MAX_PACK];
uint8_t TxBuffer[CDC_MAX_IN_PACK];

USBD_CDC_LineCodingTypeDef LineCoding;

/* USB Standard Device Descriptor */
const uint8_t USB_CDC_DeviceDesc[] =
{
    0x12,    /* bLength */
    0x01,    /* bDescriptorType */
    0x00,    /* bcdUSB */
    0x02,
    0x02,    /* bDeviceClass */
    0x02,    /* bDeviceSubClass */
    0x00,    /* bDeviceProtocol */
    0x40,    /* bMaxPacketSize */
    0xAA,    /* idVendor */
    0xFF,    /* idVendor */
    0x55,    /* idProduct */
    0x55,    /* idProduct */
    0x00,    /* bcdDevice rel. 2.00 */
    0x20,
    0x01,    /* Index of manufacturer string */
    0x02,    /* Index of product string */
    0x03,    /* Index of serial number string */
    0x01,    /* bNumConfigurations */
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_CDC_ConfigurationDesc[] =
{
    /* Configuration Descriptor */
    0x09,    /* bLength */
    0x02,    /* bDescriptorType */
    0x43,    /* wTotalLength */
    0x00,
    0x02,    /* bNumInterfaces */
    0x01,    /* bConfigurationValue */
    0x00,    /* iConfiguration */
    0xC0,    /* bmAttributes */
    0x32,    /* bMaxPower */

    /* CDC Interface Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x00,    /* bInterfaceNumber */
    0x00,    /* bAlternateSetting */
    0x01,    /* bNumEndpoints */
    0x02,    /* bInterfaceClass */
    0x02,    /* bInterfaceSubClass */
    0x01,    /* bInterfaceProtocol */
    0x00,    /* iConfiguration */

    /* Header Functional Descriptor */
    0x05,    /* bLength: Endpoint Descriptor size */
    0x24,    /* bDescriptorType: CS_INTERFACE */
    0x00,    /* bDescriptorSubtype: Header Func Desc */
    0x10,    /* bcdCDC: spec release number */
    0x01,

    /* Call Management Functional Descriptor */
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x01,   /* bDescriptorSubtype: Call Management Func Desc */
    0x00,   /* bmCapabilities: D0+D1 */
    0x01,   /* bDataInterface: 1 */

    /* ACM Functional Descriptor */
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */

    /* Union Functional Descriptor */
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bMasterInterface: Communication class interface */
    0x01,   /* bSlaveInterface0: Data Class Interface */

    /* Endpoint 2 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x82,    /* bEndpointAddress */
    0x03,    /* bmAttributes: Interrupt */
    0x08,    /* wMaxPacketSize: */
    0x00,
    0x10,    /* bInterval */ 

    /* Data class interface descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x00,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0x0A,    /* bInterfaceClass */
    0x00,    /* bInterfaceSubClass */
    0x00,    /* bInterfaceProtocol */
    0x00,    /* iConfiguration */
    
    /* Endpoint OUT Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x01,    /* bEndpointAddress */
    0x02,    /* bmAttributes: Bulk */
    0x40,    /* wMaxPacketSize */
    0x00,    
    0x00,    /* bInterval */
    
    /* Endpoint IN Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x81,    /* bEndpointAddress */
    0x02,    /* bmAttributes: Bulk */
    0x40,    /* wMaxPacketSize */
    0x00,    
    0x00,    /* bInterval */
};

/* USB Standard Manufacture Descriptor */
const uint8_t USB_CDC_ManufactureDesc[] =
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
const uint8_t USB_CDC_ProductDesc[] =
{
    0x1A,         /* bLength */
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
    'C', 0x00,    
    'O', 0x00,    
    'M', 0x00,    
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_CDC_SerialNumberDesc[] =
{
    0x1E,         /* bLength */
    0x03,         /* bDescriptorType */
    '2', 0x00,    /* BString */
    '0', 0x00,
    '2', 0x00,
    '1', 0x00,
    '-', 0x00,
    '1', 0x00,
    '0', 0x00,
    '0', 0x00,
    '1', 0x00,
    '-', 0x00,
    'A', 0x00,
    '1', 0x00,
    '3', 0x00,
    '5', 0x00,
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_CDC_LanuageIDDesc[] =
{
    0x04,    /* bLength */
    0x03,    /* bDescriptorType */
    0x09,    /* BString */
    0x04,
};

/*********************************************************************
 * @fn      usb_cdc_serialReceive
 *
 * @brief   serial port receives data
 *
 * @param   None.
 * @return  None.
 */
void usb_cdc_serialReceive(void)
{
    uint32_t SendLength;

    while (!__UART_IS_RxFIFO_EMPTY(Uart_CDC_handle.UARTx))
    {
        TxBuffer[Tx_Total++] = __UART_READ_FIFO(Uart_CDC_handle.UARTx);

        if (Tx_Total >= CDC_MAX_IN_PACK)
        {
            Tx_Total = 0;
        }
    }

    /*  have Data to be sent */
    SendLength =  Tx_Total >= Tx_Count ? Tx_Total - Tx_Count : CDC_MAX_IN_PACK - Tx_Count;

    if (SendLength) 
    {
        usb_selecet_endpoint(ENDPOINT_1);

        if (usb_Endpoints_GET_TxPktRdy() == false)
        {
            if (SendLength > CDC_MAX_PACK) 
            {
                usb_write_fifo(ENDPOINT_1, &TxBuffer[Tx_Count], CDC_MAX_PACK);

                Tx_Count += CDC_MAX_PACK;
            }
            else 
            {
                usb_write_fifo(ENDPOINT_1, &TxBuffer[Tx_Count], SendLength);

                Tx_Count += SendLength;
            }

            if (Tx_Count >= CDC_MAX_IN_PACK)
            {
                Tx_Count = 0;
            }

            usb_Endpoints_SET_TxPktRdy();
        }
    }
}

/*********************************************************************
 * @fn      usb_cdc_serialSend
 *
 * @brief   Sending data over a serial port
 *
 * @param   None.
 * @return  None.
 */
static void usb_cdc_serialSend(uint8_t *Buffer, uint8_t Size)
{
    while (Size--) 
    {
        /* Tx empty */
        while(__UART_IS_TxFIFO_FULL(Uart_CDC_handle.UARTx));
        /* Send data */
        __UART_WRITE_FIFO(Uart_CDC_handle.UARTx, *Buffer++);
    }
}

/* CDC Line coding */
uint8_t CDC_LineCoding[7];

/*********************************************************************
 * @fn      usb_cdc_SetLineCoding
 *
 * @brief   receive Host data and Set Line Coding
 *
 * @param   None.
 * @return  None.
 */
static void usb_cdc_SetLineCoding(void)
{   
    USBD_CDC_LineCodingTypeDef Temp_LineCoding;

    uint8_t lu8_RxCount;
    uint8_t lu8_UartLine;

    lu8_RxCount = usb_Endpoint0_get_RxCount();

    usb_read_fifo(ENDPOINT_0, CDC_LineCoding, lu8_RxCount);

    usb_Endpoint0_FlushFIFO();

    Endpoint_0_DataOut_Handler = NULL;

    Temp_LineCoding.dwDTERate  = CDC_LineCoding[0];
    Temp_LineCoding.dwDTERate |= CDC_LineCoding[1] << 8;
    Temp_LineCoding.dwDTERate |= CDC_LineCoding[2] << 16;
    Temp_LineCoding.dwDTERate |= CDC_LineCoding[3] << 24;

    Temp_LineCoding.bCharFormat = CDC_LineCoding[4];
    Temp_LineCoding.bParityType = CDC_LineCoding[5];
    Temp_LineCoding.bDataBits   = CDC_LineCoding[6];

    /* Modify the uart configuration */
    
    if (LineCoding.dwDTERate != Temp_LineCoding.dwDTERate) 
    {
        LineCoding.dwDTERate = Temp_LineCoding.dwDTERate;

        Uart_CDC_handle.Init.BaudRate = LineCoding.dwDTERate;
        uart_config_baudRate(&Uart_CDC_handle);
    }

    if (LineCoding.bCharFormat != Temp_LineCoding.bCharFormat || 
        LineCoding.bParityType != Temp_LineCoding.bParityType ||
        LineCoding.bDataBits   != Temp_LineCoding.bDataBits) 
    {
        LineCoding.bCharFormat = Temp_LineCoding.bCharFormat;
        LineCoding.bParityType = Temp_LineCoding.bParityType;
        LineCoding.bDataBits   = Temp_LineCoding.bDataBits;

        switch (LineCoding.bDataBits)
        {
            case 8: lu8_UartLine = 0x03; break;
            case 7: lu8_UartLine = 0x02; break;
            case 6: lu8_UartLine = 0x01; break;
            case 5: lu8_UartLine = 0x00; break;

            default: break; 
        }

        switch (LineCoding.bCharFormat)
        {
            case 0: lu8_UartLine |= 0x03; break;

            case 1: 
            case 2: lu8_UartLine |= 1 << 2; break;

            default: break; 
        }

        switch (LineCoding.bParityType)
        {
            case 1: lu8_UartLine |= (1 << 3); break;
            case 2: lu8_UartLine |= (1 << 3) | (1 << 4); break;

            default: break; 
        }
        
        __UART_SET_LINE_CTRL(Uart_CDC_handle.UARTx, lu8_UartLine);
    }
}

/*********************************************************************
 * @fn      usb_cdc_ClassRequest_Handler
 *
 * @brief   CDC Class Request Handler
 *
 * @param   None.
 * @return  None.
 */
static void usb_cdc_ClassRequest_Handler(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData)
{
    switch (pStandardRequest->bRequest)
    {
        case CDC_SET_LINE_CODING: 
        {
            /* Host data out */
            Endpoint_0_DataOut_Handler = usb_cdc_SetLineCoding;
        }break;

        case CDC_GET_LINE_CODING:
        {
            CDC_LineCoding[0] = (LineCoding.dwDTERate >> 0)  & 0xFF;
            CDC_LineCoding[1] = (LineCoding.dwDTERate >> 8)  & 0xFF;
            CDC_LineCoding[2] = (LineCoding.dwDTERate >> 16) & 0xFF;
            CDC_LineCoding[3] = (LineCoding.dwDTERate >> 24) & 0xFF;
            CDC_LineCoding[4] = LineCoding.bCharFormat;
            CDC_LineCoding[5] = LineCoding.bParityType;
            CDC_LineCoding[6] = LineCoding.bDataBits;

            pReturnData->DataBuffer = CDC_LineCoding;
            pReturnData->DataLength = sizeof(CDC_LineCoding);
        }break;

        case CDC_SET_CONTROL_LINE_STATE:
        {
            /* COM acitvate or inacitvate */
            if (pStandardRequest->wValue[0])
            {
                COM_Activate = true;
            }
            else 
            {
                COM_Activate = false;
            }

            /* clear count */
            Tx_Count = 0;
            Tx_Total = 0;

            /* Flush FIFO */
            usb_selecet_endpoint(ENDPOINT_1);
            usb_Endpoints_FlushRxFIFO();
            usb_Endpoints_FlushTxFIFO();
            usb_selecet_endpoint(ENDPOINT_0);
        }break;

        default: break; 
    }
}

/*********************************************************************
 * @fn      usb_cdc_OtherEndpoints_Handler
 *
 * @brief   CDC other Endpoints handler
 *
 * @param   None.
 * @return  None.
 */
static void usb_cdc_OtherEndpoints_Handler(uint8_t RxStatus, uint8_t TxStatus)
{
    uint8_t lu8_RxCount;

    /* DATA OUT */
    if (RxStatus & ENDPOINT_1_MASK) 
    {
        usb_selecet_endpoint(ENDPOINT_1);

        lu8_RxCount = usb_Endpoints_get_RxCount();

        usb_read_fifo(ENDPOINT_1, RxBuffer, lu8_RxCount);

        usb_cdc_serialSend(RxBuffer, lu8_RxCount);

        usb_Endpoints_FlushRxFIFO();
    }
}

/*********************************************************************
 * @fn      usb_cdc_init
 *
 * @brief   CDC device parameter initialization 
 *
 * @param   None.
 * @return  None.
 */
void usb_cdc_init(void)
{
    /* Initialize the relevant pointer  */
    usbdev_get_dev_desc((uint8_t *)USB_CDC_DeviceDesc);
    usbdev_get_config_desc((uint8_t *)USB_CDC_ConfigurationDesc);
    usbdev_get_string_Manufacture((uint8_t *)USB_CDC_ManufactureDesc);
    usbdev_get_string_Product((uint8_t *)USB_CDC_ProductDesc);
    usbdev_get_string_SerialNumber((uint8_t *)USB_CDC_SerialNumberDesc);
    usbdev_get_string_LanuageID((uint8_t *)USB_CDC_LanuageIDDesc);
    
    Endpoint_0_ClassRequest_Handler = usb_cdc_ClassRequest_Handler;

    Endpoints_Handler = usb_cdc_OtherEndpoints_Handler;

    USB_Reset_Handler = usb_cdc_init;
    
    /* config data endpoint fifo */
    usb_selecet_endpoint(ENDPOINT_1);
    usb_endpoint_Txfifo_config(0x08, 3);
    usb_endpoint_Rxfifo_config(0x10, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_2);
    usb_endpoint_Txfifo_config(0x28, 1);
    usb_RxMaxP_set(1);

    /* Endpoint_1 Rx interrupt enable */
    usb_RxInt_Enable(ENDPOINT_1);

    LineCoding.dwDTERate   = 115200;
    LineCoding.bCharFormat = 0x00;
    LineCoding.bParityType = 0x00;
    LineCoding.bDataBits   = 0x08;
}

