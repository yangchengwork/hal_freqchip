/*
  ******************************************************************************
  * @file    usb_winusb.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   This file provides the high layer firmware functions to manage the 
  *          WinUSB Device.
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
  *        usb_winusb_init();
  *        
  *        // Wait for other initialization of the MCU
  * 
  *        usb_DP_Pullup_Enable();
  *
  *        while(1)
  *        {
  *        }
  *    }
  ******************************************************************************
*/
#include "fr30xx.h"

uint8_t WinUSB_Buffer[512];

/* USB Standard Device Descriptor */
const uint8_t USB_WinUSB_DeviceDesc[] =
{
    0x12,    /* bLength */
    0x01,    /* bDescriptorType */
    0x00,    /* bcdUSB */
    0x02,
    0xFF,    /* bDeviceClass: Vendor customization */
    0x00,    /* bDeviceSubClass */
    0x00,    /* bDeviceProtocol */
    0x40,    /* bMaxPacketSize */
    0xAA,    /* idVendor */
    0xBB,    /* idVendor */
    0xCC,    /* idProduct */
    0x11,    /* idProduct */
    0x00,    /* bcdDevice rel. 2.00 */
    0x20,
    0x01,    /* Index of manufacturer string */
    0x02,    /* Index of product string */
    0x03,    /* Index of serial number string */
    0x01,    /* bNumConfigurations */
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_WinUSB_ConfigurationDesc[] =
{
    /* Configuration Descriptor */
    0x09,    /* bLength */             
    0x02,    /* bDescriptorType */     
    0x20,    /* wTotalLength */        
    0x00,                              
    0x01,    /* bNumInterfaces */      
    0x01,    /* bConfigurationValue */ 
    0x00,    /* iConfiguration */      
    0x80,    /* bmAttributes */        
    0xFA,    /* bMaxPower */           

    /* Interface Descriptor */
    0x09,    /* bLength */           
    0x04,    /* bDescriptorType */   
    0x00,    /* bInterfaceNumber */  
    0x00,    /* bAlternateSetting */ 
    0x02,    /* bNumEndpoints */     
    0x00,    /* bInterfaceClass */   
    0x00,    /* bInterfaceSubClass */
    0x00,    /* bInterfaceProtocol */
    0x00,    /* iConfiguration */    

    /* Endpoint 1 IN Descriptor */
    0x07,    /* bLength */
    0X05,    /* bDescriptorType */
    0x81,    /* bEndpointAddress */
    0x02,    /* bmAttributes: bulk */ 
    0x40,    /* wMaxPacketSize: 64byte */
    0x00,
    0x00,    /* bInterval */

    /* Endpoint 1 OUT Descriptor */
    0x07,    /* bLength */
    0X05,    /* bDescriptorType */
    0x01,    /* bEndpointAddress */
    0x02,    /* bmAttributes: bulk */ 
    0x40,    /* wMaxPacketSize: 64byte */
    0x00,
    0x00,    /* bInterval */
};

/* USB Standard Manufacture Descriptor */
const uint8_t USB_WinUSB_ManufactureDesc[] =
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
const uint8_t USB_WinUSB_ProductDesc[] =
{
    0x20,         /* bLength */
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
    'W', 0x00,    
    'i', 0x00,    
    'n', 0x00,    
    'U', 0x00,    
    'S', 0x00,    
    'B', 0x00,    
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_WinUSB_SerialNumberDesc[] =
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
    'C', 0x00,
    '1', 0x00,
    '3', 0x00,
    '5', 0x00,
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_WinUSB_LanuageIDDesc[] =
{
    0x04,    /* bLength */
    0x03,    /* bDescriptorType */
    0x09,    /* BString */
    0x04,
};

/* Compat ID OS Descriptor */
const uint8_t USB_WinUSB_OSDesc[] =
{
    0x12,         /* bLength */
    0x03,         /* bDescriptorType */
    'M', 0x00,    /* BString */ 
    'S', 0x00,
    'F', 0x00,
    'T', 0x00,
    '1', 0x00,
    '0', 0x00,
    '0', 0x00,
    0x01,
    0x00,
};

/* Compat ID OS Descriptor */
const uint8_t WINUSB_Extended_Compat_ID_OS_Feature_Descriptor[] =
{
    0x28, 0x00, 0x00, 0x00,                            // dwLength 
    0x00, 0x01,                                        // bcdVersion 
    0x04, 0x00,                                        // wIndex 
    0x01,                                              // bCount 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,          // Reserved[7] 
    0x00,                                              // bFirstInterfaceNumber 
    0x01,                                              // RESERVED ( 0x01 ) 
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,          // compactiableID[8] 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    // subCompactiableID[8] 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00                 // Reserved[6] 
};

/* L"DeviceInterfaceGUID" : wIndex = 0x0005  */
/* L"{12345678-1234-1234-1234-123456789ABC}" */
const uint8_t WINUSB_Extended_Property_OS_Feature_Descriptor[142] =
{
    0x8E, 0x00, 0x00, 0x00,     // dwTotalSize = Header + All sections
    0x00, 0x01,                 // bcdVersion
    0x05, 0x00,                 // wIndex
    0x01, 0x00,                 // wCount

    0x84, 0x00, 0x00, 0x00,     // dwSize -- this section

    0x01, 0x00, 0x00, 0x00,     // dwPropertyDataType

    0x28, 0x00,                 // wPropertyNameLength
    
    'D', 0x00, 'e',  0x00,      // bProperytName : WCHAR : L"DeviceInterfaceGUID"
    'v', 0x00, 'i',  0x00,      // bProperytName : WCHAR
    'c', 0x00, 'e',  0x00,      // bProperytName : WCHAR
    'I', 0x00, 'n',  0x00,      // bProperytName : WCHAR
    't', 0x00, 'e',  0x00,      // bProperytName : WCHAR
    'r', 0x00, 'f',  0x00,      // bProperytName : WCHAR
    'a', 0x00, 'c',  0x00,      // bProperytName : WCHAR
    'e', 0x00, 'G',  0x00,      // bProperytName : WCHAR
    'U', 0x00, 'I',  0x00,      // bProperytName : WCHAR
    'D', 0x00, 0x00, 0x00,      // bProperytName : WCHAR
    
    0x4E, 0x00, 0x00, 0x00,     // dwPropertyDataLength : 78 Bytes = 0x0000004E
    
    '{', 0x00, '1', 0x00,       // bPropertyData : WCHAR : L"{12345678-1234-1234-1234-123456789ABC}"
    '2', 0x00, '3', 0x00,       // bPropertyData
    '4', 0x00, '5', 0x00,       // bPropertyData
    '6', 0x00, '7', 0x00,       // bPropertyData
    '8', 0x00, '-', 0x00,       // bPropertyData
    '1', 0x00, '2', 0x00,       // bPropertyData
    '3', 0x00, '4', 0x00,       // bPropertyData
    '-', 0x00, '1', 0x00,       // bPropertyData
    '2', 0x00, '3', 0x00,       // bPropertyData
    '4', 0x00, '-', 0x00,       // bPropertyData
    '1', 0x00, '2', 0x00,       // bPropertyData
    '3', 0x00, '4', 0x00,       // bPropertyData
    '-', 0x00, '1', 0x00,       // bPropertyData
    '2', 0x00, '3', 0x00,       // bPropertyData
    '4', 0x00, '5', 0x00,       // bPropertyData
    '6', 0x00, '7', 0x00,       // bPropertyData
    '8', 0x00, '9', 0x00,       // bPropertyData
    'A', 0x00, 'B', 0x00,       // bPropertyData
    'C', 0x00, '}', 0x00,       // bPropertyData
    0x00, 0x00                  // bPropertyData
};

/*********************************************************************
 * @fn      usb_winusb_VendorRequest_Handler
 *
 * @brief   winusb Vendor Request Handler
 *
 * @param   None.
 * @return  None.
 */
static void usb_winusb_VendorRequest_Handler(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData)
{
    pReturnData->RequestLength  = (uint16_t)pStandardRequest->wLength[1] << 8;
    pReturnData->RequestLength |= (uint16_t)pStandardRequest->wLength[0];

    switch (pStandardRequest->bRequest)
    {
        case GET_MS_DESCRIPTOR: 
        {
            switch (pStandardRequest->wIndex[0])
            {
                case EXTENDED_COMPAT_ID: 
                {
                    pReturnData->DataBuffer = (uint8_t *)WINUSB_Extended_Compat_ID_OS_Feature_Descriptor;
                    pReturnData->DataLength = sizeof(WINUSB_Extended_Compat_ID_OS_Feature_Descriptor);
                }break;

                case EXTENDED_PROPERTIES: 
                {
                    pReturnData->DataBuffer = (uint8_t *)WINUSB_Extended_Property_OS_Feature_Descriptor;
                    pReturnData->DataLength = sizeof(WINUSB_Extended_Property_OS_Feature_Descriptor);
                }break;

                default: break; 
            }
        }break;

        default: break; 
    }
}

/*********************************************************************
 * @fn      Endpoint1_Handler
 *
 * @brief   endpoint1 RX TX Handler
 *
 * @param   None.
 * @return  None.
 */
static void Endpoint1_Handler(uint8_t RxStatus, uint8_t TxStatus)
{
    uint8_t lu8_RxCount;
    
    if (RxStatus & ENDPOINT_1_MASK) 
    {
        usb_selecet_endpoint(ENDPOINT_1);

        lu8_RxCount = usb_Endpoints_get_RxCount();

        usb_read_fifo(ENDPOINT_1, WinUSB_Buffer, lu8_RxCount);

        usb_Endpoints_FlushRxFIFO();


        if (usb_Endpoints_GET_TxPktRdy() == false) 
        {
            usb_write_fifo(ENDPOINT_1, WinUSB_Buffer, lu8_RxCount);
            
            usb_Endpoints_SET_TxPktRdy();
        }
    }
}

/*********************************************************************
 * @fn      usb_winusb_init
 *
 * @brief   winusb device parameter initialization 
 *
 * @param   None.
 * @return  None.
 */
void usb_winusb_init(void)
{
    /* Initialize the relevant pointer  */
    usbdev_get_dev_desc((uint8_t *)USB_WinUSB_DeviceDesc);
    usbdev_get_config_desc((uint8_t *)USB_WinUSB_ConfigurationDesc);
    usbdev_get_string_Manufacture((uint8_t *)USB_WinUSB_ManufactureDesc);
    usbdev_get_string_Product((uint8_t *)USB_WinUSB_ProductDesc);
    usbdev_get_string_SerialNumber((uint8_t *)USB_WinUSB_SerialNumberDesc);
    usbdev_get_string_LanuageID((uint8_t *)USB_WinUSB_LanuageIDDesc);
    usbdev_get_string_OS((uint8_t *)USB_WinUSB_OSDesc);

    Endpoint_0_VendorRequest_Handler = usb_winusb_VendorRequest_Handler;
    
    Endpoints_Handler = Endpoint1_Handler;

    USB_Reset_Handler = usb_winusb_init;
    
    /* config data endpoint fifo */
    usb_selecet_endpoint(ENDPOINT_1);
    usb_endpoint_Txfifo_config(0x08, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_1);
    usb_endpoint_Rxfifo_config(0x10, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);
    
    usb_RxInt_Enable(ENDPOINT_1);
}
