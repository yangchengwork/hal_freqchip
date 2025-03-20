/*
  ******************************************************************************
  * @file    usb_hid.c
  * @author  FreqChip Firmware Team
  * @version V1.0.1
  * @date    2024
  * @brief   This file provides the high layer firmware functions to manage the 
  *          USB HID Device.(Human Interface Device)
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
  *        usb_hid_init();
  *        
  *        // Wait for other initialization of the MCU
  *
  *        while(1)
  *        {
  *            usb_hid_send_mouse_report();
  *            usb_hid_send_keyboard_report();
  *        }
  *    }
  ******************************************************************************
*/
#include "fr30xx.h"

#define HID_INTERFACE_NUM    (2)    // NumInterfaces
#define REPORT0_LENGTH       (4)
#define REPORT1_LENGTH       (8)

uint8_t HID_Mouse_Report[REPORT0_LENGTH];
uint8_t HID_Keyboard_Report[REPORT1_LENGTH];

uint8_t HID_Protocol[HID_INTERFACE_NUM];

/* USB Standard Device Descriptor */
const uint8_t USB_HID_DeviceDesc[] =
{
    0x12,    /* bLength */
    0x01,    /* bDescriptorType */
    0x00,    /* bcdUSB */
    0x02,
    0x00,    /* bDeviceClass */
    0x00,    /* bDeviceSubClass */
    0x00,    /* bDeviceProtocol */
    0x40,    /* bMaxPacketSize */
    0xA4,    /* idVendor */
    0xA8,    /* idVendor */
    0x55,    /* idProduct */
    0x22,    /* idProduct */
    0x00,    /* bcdDevice rel. 2.00 */
    0x20,
    0x01,    /* Index of manufacturer string */
    0x02,    /* Index of product string */
    0x03,    /* Index of serial number string */
    0x01,    /* bNumConfigurations */
};

/* HID Descriptor */
#define HID_DESCRIPTOR_0        0x09,    /* bLength */                  \
                                0x21,    /* bDescriptorType: HID */     \
                                0x10,    /* bcdHID: 1.10 */             \
                                0x01,                                   \
                                0x21,    /* bCountryCode */             \
                                0x01,    /* bNumDescriptors */          \
                                0x22,    /* bDescriptorType: Report */  \
                                0x2E,    /* wDescriptorLength */        \
                                0x00

/* HID Descriptor */
#define HID_DESCRIPTOR_1        0x09,    /* bLength */                  \
                                0x21,    /* bDescriptorType: HID */     \
                                0x10,    /* bcdHID: 1.10 */             \
                                0x01,                                   \
                                0x21,    /* bCountryCode */             \
                                0x01,    /* bNumDescriptors */          \
                                0x22,    /* bDescriptorType: Report */  \
                                0x41,    /* wDescriptorLength */        \
                                0x00

const uint8_t USB_HID_Desc_0[] =
{
    HID_DESCRIPTOR_0,
};

const uint8_t USB_HID_Desc_1[] =
{
    HID_DESCRIPTOR_1,
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_HID_ConfigurationDesc[] =
{
    /* Configuration Descriptor */
    0x09,    /* bLength */             
    0x02,    /* bDescriptorType */     
    0x42,    /* wTotalLength */        
    0x00,                              
    0x02,    /* bNumInterfaces */      
    0x01,    /* bConfigurationValue */ 
    0x00,    /* iConfiguration */      
    0xA0,    /* bmAttributes */        
    0x32,    /* bMaxPower */           

    /* HID Interface_0 Descriptor */
    0x09,    /* bLength */           
    0x04,    /* bDescriptorType */   
    0x00,    /* bInterfaceNumber */  
    0x00,    /* bAlternateSetting */ 
    0x01,    /* bNumEndpoints */     
    0x03,    /* bInterfaceClass: HID */   
    0x01,    /* bInterfaceSubClass: Boot interface */
    0x02,    /* bInterfaceProtocol: Mouse */
    0x00,    /* iConfiguration */    

    /* HID Descriptor */
    HID_DESCRIPTOR_0,

    /* Endpoint 1 Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x81,    /* bEndpointAddress */
    0x03,    /* bmAttributes: Interrupt */ 
    0x40,    /* wMaxPacketSize: 64byte */
    0x00,
    0x08,    /* bInterval */


    /* HID Interface_1 Descriptor */
    0x09,    /* bLength */
    0x04,    /* bDescriptorType */
    0x01,    /* bInterfaceNumber */
    0x00,    /* bAlternateSetting */
    0x02,    /* bNumEndpoints */
    0x03,    /* bInterfaceClass: HID */
    0x01,    /* bInterfaceSubClass: Boot interface */
    0x01,    /* bInterfaceProtocol: Keyboard */
    0x00,    /* iConfiguration */

    /* HID Descriptor */
    HID_DESCRIPTOR_1,

    /* Endpoint 2 IN Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x82,    /* bEndpointAddress */
    0x03,    /* bmAttributes: Interrupt */ 
    0x40,    /* wMaxPacketSize: 64byte */
    0x00,
    0x0A,    /* bInterval */
    
    /* Endpoint 2 OUT Descriptor */
    0x07,    /* bLength */
    0x05,    /* bDescriptorType */
    0x02,    /* bEndpointAddress */
    0x03,    /* bmAttributes: Interrupt */ 
    0x40,    /* wMaxPacketSize: 64byte */
    0x00,
    0x0A,    /* bInterval */
};

/* USB Standard Manufacture Descriptor */
const uint8_t USB_HID_ManufactureDesc[] =
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
const uint8_t USB_HID_ProductDesc[] =
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
    'H', 0x00,    
    'I', 0x00,    
    'D', 0x00,    
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_HID_SerialNumberDesc[] =
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
    'B', 0x00,
    '1', 0x00,
    '3', 0x00,
    '5', 0x00,
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_HID_LanuageIDDesc[] =
{
    0x04,    /* bLength */
    0x03,    /* bDescriptorType */
    0x09,    /* BString */
    0x04,
};

/* HID device report Descriptor */
uint8_t USB_HID_Mouse_ReportDesc[] = 
{
    0x2E,          /* Length */

    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x09, 0x01,
    0xA1, 0x00,
    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x08,
    0x75, 0x01,
    0x81, 0x02,
    0x05, 0x01,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x38,
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x03,
    0x81, 0x06,
    0xC0,
    0xC0,
};

/* HID device report Descriptor */
uint8_t USB_HID_Keyboard_ReportDesc[] = 
{
    0x41,          /* Length */

    0x05, 0x01,
    0x09, 0x06,
    0xA1, 0x01,
    0x05, 0x07,
    0x19, 0xE0,
    0x29, 0xE7,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x08,
    0x81, 0x02,
    0x95, 0x01,
    0x75, 0x08,
    0x81, 0x01,
    0x95, 0x03,
    0x75, 0x01,
    0x05, 0x08,
    0x19, 0x01,
    0x29, 0x03,
    0x91, 0x02,
    0x95, 0x05,
    0x75, 0x01,
    0x91, 0x01,
    0x95, 0x06,
    0x75, 0x08,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x05, 0x07,
    0x19, 0x00,
    0x2A, 0xFF, 0x00,
    0x81, 0x00,
    0xC0,
    0xC0,
};

/*********************************************************************
 * @fn      usb_hid_set_mouse_report
 *
 * @brief   set report
 *
 * @param   None.
 * @return  None.
 */
void usb_hid_set_mouse_report(uint8_t fu8_Index, uint8_t fu8_Value)
{
    HID_Mouse_Report[fu8_Index] = fu8_Value;
    /*
        HID_Report[0]: 0000 0001  left   button
                       0000 0010  right  button
                       0000 0100  middle button
        HID_Report[1]: X (-127 ~ +127)
        HID_Report[2]: Y (-127 ~ +127)
        HID_Report[3]: wheel (-127 ~ +127)
    */
}

/*********************************************************************
 * @fn      usb_hid_set_keyboard_report
 *
 * @brief   set report
 *
 * @param   None.
 * @return  None.
 */
void usb_hid_set_keyboard_report(uint16_t fu16_Key)
{
    uint32_t i;
    
    switch (fu16_Key)
    {
        case KEY_LEFT_CONTROL:  HID_Keyboard_Report[0] |= 0x01;  break;
        case KEY_LEFT_SHIFT:    HID_Keyboard_Report[0] |= 0x02;  break;
        case KEY_LEFT_ALT:      HID_Keyboard_Report[0] |= 0x04;  break;
        case KEY_LEFT_GUI:      HID_Keyboard_Report[0] |= 0x08;  break;
        case KEY_RIGHT_CONTROL: HID_Keyboard_Report[0] |= 0x10;  break;
        case KEY_RIGHT_SHIFT:   HID_Keyboard_Report[0] |= 0x20;  break;
        case KEY_RIGHT_ALT:     HID_Keyboard_Report[0] |= 0x40;  break;
        case KEY_RIGHT_GUI:     HID_Keyboard_Report[0] |= 0x80;  break;

        default:
        {
            for (i = 2; i < 8; i++)
            {
                if (HID_Keyboard_Report[i] == fu16_Key) 
                {
                    break;
                }
                
                if (HID_Keyboard_Report[i] == 0x00) 
                {
                    HID_Keyboard_Report[i] = fu16_Key;

                    break;
                }
            }
        }break; 
    }
}

/*********************************************************************
 * @fn      usb_hid_clear_keyboard_report
 *
 * @brief   set report
 *
 * @param   None.
 * @return  None.
 */
void usb_hid_clear_keyboard_report(uint16_t fu16_Key)
{
    uint32_t i, k;
    
    switch (fu16_Key)
    {
        case KEY_LEFT_CONTROL:  HID_Keyboard_Report[0] &= ~0x01;  break;
        case KEY_LEFT_SHIFT:    HID_Keyboard_Report[0] &= ~0x02;  break;
        case KEY_LEFT_ALT:      HID_Keyboard_Report[0] &= ~0x04;  break;
        case KEY_LEFT_GUI:      HID_Keyboard_Report[0] &= ~0x08;  break;
        case KEY_RIGHT_CONTROL: HID_Keyboard_Report[0] &= ~0x10;  break;
        case KEY_RIGHT_SHIFT:   HID_Keyboard_Report[0] &= ~0x20;  break;
        case KEY_RIGHT_ALT:     HID_Keyboard_Report[0] &= ~0x40;  break;
        case KEY_RIGHT_GUI:     HID_Keyboard_Report[0] &= ~0x80;  break;

        default:
        {
            for (i = 2; i < 8; i++)
            {
                if (HID_Keyboard_Report[i] == fu16_Key) 
                {
                    HID_Keyboard_Report[i] = 0x00;

                    for (k = i; k < 7; k++)
                    {
                        if (HID_Keyboard_Report[k + 1] != 0x00)
                        {
                            HID_Keyboard_Report[k] = HID_Keyboard_Report[k + 1];
                            HID_Keyboard_Report[k + 1] = 0x00;
                        }
                        else 
                        {
                            break;
                        }
                    }

                    break;
                }
            }
        }break; 
    }
}

/*********************************************************************
 * @fn      usb_hid_send_mouse_report
 *
 * @brief   send report to host
 *
 * @param   None.
 * @return  None.
 */
void usb_hid_send_mouse_report(void)
{
    usb_selecet_endpoint(ENDPOINT_1);

    /* FIFO empty */
    if (usb_Endpoints_GET_TxPktRdy() == false) 
    {
        usb_write_fifo(ENDPOINT_1, HID_Mouse_Report, REPORT0_LENGTH);

        usb_Endpoints_SET_TxPktRdy();
    }
}

/*********************************************************************
 * @fn      usb_hid_send_keyboard_report
 *
 * @brief   send report to host
 *
 * @param   None.
 * @return  None.
 */
void usb_hid_send_keyboard_report(void)
{
    usb_selecet_endpoint(ENDPOINT_2);

    /* FIFO empty */
    if (usb_Endpoints_GET_TxPktRdy() == false) 
    {
        usb_write_fifo(ENDPOINT_2, HID_Keyboard_Report, REPORT1_LENGTH);

        usb_Endpoints_SET_TxPktRdy();
    }
}

/*********************************************************************
 * @fn      usb_hid_get_Protocol
 *
 * @brief   send report to host
 *
 * @param   fu32_Interface: interface index.
 * @return  interface Protocol(either the boot protocol or the report protocol).
 */
uint32_t usb_hid_get_Protocol(uint32_t fu32_Interface)
{
    // 0: Boot   Protocol
    // 1: Report Protocol
    return HID_Protocol[fu32_Interface];
}

/*********************************************************************
 * @fn      usb_hid_ClassRequest_Handler
 *
 * @brief   HID Class Request Handler
 *
 * @param   None.
 * @return  None.
 */
static void usb_hid_ClassRequest_Handler(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData)
{
    switch (pStandardRequest->bRequest)
    {
        case HID_GET_PROTOCOL:
        {
            /* The Get_Protocol request reads which protocol is currently active (either the boot
               protocol or the report protocol.) */
            // 0: Boot   Protocol
            // 1: Report Protocol
            pReturnData->DataBuffer = &HID_Protocol[pStandardRequest->wIndex[0]];
            pReturnData->DataLength = 1;
        }break;

        case HID_SET_PROTOCOL:
        {
            /* The Set_Protocol switches between the boot protocol and the report protocol (or vice versa). */
            HID_Protocol[pStandardRequest->wIndex[0]] = pStandardRequest->wValue[0];
        }break;

        case HID_GET_IDLE:
        {
            /* Device does not support Getidle command */
            usb_Endpoint0_SendStall();
        }break;

        case HID_SET_IDLE:
        {
            /* Device does not support Setidle command */
            usb_Endpoint0_SendStall();
        }break;

        default: break; 
    }
}

/*********************************************************************
 * @fn      usb_hid_StandardClassRequest_Handler
 *
 * @brief   HID Standard Class Request Handler
 *
 * @param   None.
 * @return  None.
 */
static void usb_hid_StandardClassRequest_Handler(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData)
{
    switch (pStandardRequest->wValue[1])
    {
        case DESCRIPTOR_HID_REPORT:
        {
            if (pStandardRequest->wIndex[0] == 0){
                pReturnData->DataLength =  USB_HID_Mouse_ReportDesc[0];
                pReturnData->DataBuffer = &USB_HID_Mouse_ReportDesc[1];
            }
            else if (pStandardRequest->wIndex[0] == 1){
                pReturnData->DataLength =  USB_HID_Keyboard_ReportDesc[0];
                pReturnData->DataBuffer = &USB_HID_Keyboard_ReportDesc[1];
            }
        }break;

        case DESCRIPTOR_HID:
        {
            if (pStandardRequest->wIndex[0] == 0){
                pReturnData->DataLength = 9;
                pReturnData->DataBuffer = (uint8_t *)USB_HID_Desc_0;
            }
            else if (pStandardRequest->wIndex[0] == 1){
                pReturnData->DataLength = 9;
                pReturnData->DataBuffer = (uint8_t *)USB_HID_Desc_1;
            }
        }break;

        default: break; 
    }
}

/*********************************************************************
 * @fn      usb_hid_Endpoints_Handler
 *
 * @brief   hid Endpoints Handler
 *
 * @param   None.
 * @return  None.
 */
void usb_hid_Endpoints_Handler(uint8_t RxStatus, uint8_t TxStatus)
{
    uint8_t lu8_RxCount;
    uint8_t lu8_Buffer[4] = {0,0,0,0};
    
    if (RxStatus & 1 << ENDPOINT_2) 
    {
        usb_selecet_endpoint(ENDPOINT_2);

        lu8_RxCount = usb_Endpoints_get_RxCount();
        
        usb_read_fifo(ENDPOINT_2, lu8_Buffer, lu8_RxCount);

        usb_Endpoints_FlushRxFIFO();
    }
}

/*********************************************************************
 * @fn      usb_hid_init
 *
 * @brief   hid device parameter initialization 
 *
 * @param   None.
 * @return  None.
 */
void usb_hid_init(void)
{
    /* Initialize the relevant pointer  */
    usbdev_get_dev_desc((uint8_t *)USB_HID_DeviceDesc);
    usbdev_get_config_desc((uint8_t *)USB_HID_ConfigurationDesc);
    usbdev_get_string_Manufacture((uint8_t *)USB_HID_ManufactureDesc);
    usbdev_get_string_Product((uint8_t *)USB_HID_ProductDesc);
    usbdev_get_string_SerialNumber((uint8_t *)USB_HID_SerialNumberDesc);
    usbdev_get_string_LanuageID((uint8_t *)USB_HID_LanuageIDDesc);

    Endpoint_0_StandardClassRequest_Handler = usb_hid_StandardClassRequest_Handler;
    Endpoint_0_ClassRequest_Handler = usb_hid_ClassRequest_Handler;

    Endpoints_Handler = usb_hid_Endpoints_Handler;

    USB_Reset_Handler = usb_hid_init;

    memset(HID_Mouse_Report, 0, sizeof(HID_Mouse_Report));
    memset(HID_Keyboard_Report, 0, sizeof(HID_Keyboard_Report));
    memset(HID_Protocol, 0, sizeof(HID_Protocol));

    /* config data endpoint fifo */
    usb_selecet_endpoint(ENDPOINT_1);
    usb_endpoint_Txfifo_config(0x08, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);

    usb_selecet_endpoint(ENDPOINT_2);
    usb_endpoint_Txfifo_config(0x18, 3);
    usb_endpoint_Rxfifo_config(0x20, 3);
    usb_TxMaxP_set(8);
    usb_RxMaxP_set(8);
    usb_RxInt_Enable(ENDPOINT_2);

    usb_SuspendDetectEn();
    usb_SingleInt_Enable(USB_INT_STATUS_SUSPEND);
    usb_SingleInt_Enable(USB_INT_STATUS_RESUME);
}

