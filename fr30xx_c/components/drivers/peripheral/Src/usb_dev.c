/*
  ******************************************************************************
  * @file    usb_dev.c
  * @author  FreqChip Firmware Team
  * @version V1.0.2
  * @date    2024
  * @brief   This file provides all the USBD device functions.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

uint16_t USB_Interface_Status;                                 /* Interface status */
uint16_t USB_Device_Status = DEVICE_STATUS_D1_REMOTE_WAKEUP;   /* Device status */
uint16_t USB_IN_Endpoint_Status[5];                            /* IN  endpoints status */
uint16_t USB_OUT_Endpoint_Status[5];                           /* OUT endpoints status */

uint8_t  USB_Configuration;                /* Configuration setting */
uint8_t  USB_InterfaceAlternateSet[20];    /* Interface Alternate setting */

usb_DescriptorsTypeDef_t DevDescriptors;

usb_StandardRequest_t StandardRequest;

usb_ReturnData_t ReturnData;

void (*Endpoint_0_StandardClassRequest_Handler)(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData) = NULL;
void (*Endpoint_0_ClassRequest_Handler)(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData)  = NULL;
void (*Endpoint_0_VendorRequest_Handler)(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData) = NULL;

void (*Endpoint_0_UserStringRequest_Handler)(uint8_t StringIndex, usb_ReturnData_t* pReturnData) = NULL;

void (*Endpoint_0_DataOut_Handler)(void) = NULL;

void (*Endpoints_Handler)(uint8_t RxStatus, uint8_t TxStatus) = NULL;

void (*USB_SOF_Handler)(void) = NULL;
void (*USB_Reset_Handler)(void) = NULL;
void (*USB_Resume_Handler)(void) = NULL;
void (*USB_Suspend_Handler)(void) = NULL;
void (*USB_Connect_Handler)(void) = NULL;

void (*USB_InterfaceAlternateSet_callback)(uint8_t Interface) = NULL;

/*********************************************************************
 * @fn      usbdev_get_dev_desc
 *
 * @brief   Get device descriptor buffer pointer.
 *********************************************************************/
void usbdev_get_dev_desc(uint8_t *Descriptor)
{
    DevDescriptors.DeviceDescriptor = Descriptor;
}

/*********************************************************************
 * @fn      usbdev_get_config_desc
 *
 * @brief   USB full speed device Get Configuration¡¢Interface¡¢
 *          Endpoint Descriptor.
 *********************************************************************/
void usbdev_get_config_desc(uint8_t *Descriptor)
{
    DevDescriptors.ConfigurationDescriptor = Descriptor;
}

/*********************************************************************
 * @fn      usbdev_get_string_Manufacture
 *
 * @brief   Get device Manufacture string Descriptor.
 *********************************************************************/
void usbdev_get_string_Manufacture(uint8_t *Descriptor)
{
    DevDescriptors.stringManufacture = Descriptor;
}

/*********************************************************************
 * @fn      usbdev_get_string_Product
 *
 * @brief   Get device Product string Descriptor.
 *********************************************************************/
void usbdev_get_string_Product(uint8_t *Descriptor)
{
    DevDescriptors.stringProduct = Descriptor;
}

/*********************************************************************
 * @fn      usbdev_get_string_SerialNumber
 *
 * @brief   Get device SerialNumber string Descriptor.
 *********************************************************************/
void usbdev_get_string_SerialNumber(uint8_t *Descriptor)
{
    DevDescriptors.stringSerialNumber = Descriptor;
}

/*********************************************************************
 * @fn      usbdev_get_string_LanuageID
 *
 * @brief   Get device LanuageID string Descriptor.
 *********************************************************************/
void usbdev_get_string_LanuageID(uint8_t *Descriptor)
{
    DevDescriptors.stringLanuageID = Descriptor;
}

/*********************************************************************
 * @fn      usbdev_get_string_OS
 *
 * @brief   Get OS String Descriptor.
 *********************************************************************/
void usbdev_get_string_OS(uint8_t *Descriptor)
{
    DevDescriptors.stringOS = Descriptor;
}

/*********************************************************************
 * @fn      usbdev_get_device_status
 *
 * @brief   Get device_status. bit0 : self-powered
 *                             bit1 : device remove wakeup
 *********************************************************************/
uint16_t usbdev_get_device_status(void)
{
    return USB_Device_Status;
}

/*********************************************************************
 * @fn      usbdev_get_in_endpoints_status
 *
 * @brief   Get in endpoints status. bit0 : 0 normal
 *                                   bit0 : 1 endpoint_halt
 * @param   index: In endpoints index.
 *********************************************************************/
uint16_t usbdev_get_in_endpoints_status(uint32_t index)
{
    return USB_IN_Endpoint_Status[index];
}

/*********************************************************************
 * @fn      usbdev_get_out_endpoints_status
 *
 * @brief   Get out endpoints status. 
 * 
 * @param   index: Out endpoints index.
 *********************************************************************/
uint16_t usbdev_get_out_endpoints_status(uint32_t index)
{
    return USB_OUT_Endpoint_Status[index];
}

/*********************************************************************
 * @fn      usbdev_get_device_configuration_num
 *
 * @brief   Get device configuration num
 *********************************************************************/
uint8_t usbdev_get_device_configuration_num(void)
{
    return USB_Configuration;
}

/*********************************************************************
 * @fn      usbdev_get_interface_alternate_num
 *
 * @brief   Get device interface alternate num
 * 
 * @param   index: interface index.
 *********************************************************************/
uint8_t usbdev_get_interface_alternate_num(uint32_t index)
{
    return USB_InterfaceAlternateSet[index];
}

/*********************************************************************
 * @fn      Endpoint_0_IRQHandler
 *
 * @brief   USB endpoint0 Interrupt Request handler
 *
 * @param   None.
 * @return  None.
 */
static void Endpoint_0_IRQHandler(void)
{
    uint8_t lu8_RxCount;
    
    static bool lb_WaitSetAddress = false;
    static bool lb_PktMAX = false;
    
    usb_selecet_endpoint(ENDPOINT_0);
    
    /* Clear SetupEnd status */
    if (USB_POINT0->CSR01 & USB_CSR01_SETUPEND)
    {
        USB_POINT0->CSR01 |= USB_CSR01_SVCSETUPEND;
    }


    /* endpoint 0 receive a packet */
    if (usb_Endpoint0_GET_RxPktRdy()) 
    {
        /* Data Out packet */
        if (Endpoint_0_DataOut_Handler != NULL) 
        {
            Endpoint_0_DataOut_Handler();
        }
        /* Standard Request packet */
        else 
        {
            lu8_RxCount = usb_Endpoint0_get_RxCount();

            usb_read_fifo(ENDPOINT_0, (uint8_t *)&StandardRequest, lu8_RxCount);

            /* Standard Request */
            if ((StandardRequest.bmRequestType & REQUEST_TYPE_MASK) == TYPE_STANDARD)
            {
                switch (StandardRequest.bRequest)
                {
                    /* request descriptor */
                    case REQUEST_GET_DESCRIPTOR: 
                    {
                        /* Build the returned data */
                        ReturnData.RequestLength  = (uint16_t)StandardRequest.wLength[1] << 8;
                        ReturnData.RequestLength |= (uint16_t)StandardRequest.wLength[0];

                        switch (StandardRequest.wValue[1])
                        {
                            case DESCRIPTOR_DEVICE: 
                            {
                                ReturnData.DataBuffer = DevDescriptors.DeviceDescriptor;
                                ReturnData.DataLength = ReturnData.DataBuffer[0];
                            }break;

                            case DESCRIPTOR_CONFIGURATION: 
                            {
                                ReturnData.DataBuffer  = DevDescriptors.ConfigurationDescriptor;
                                ReturnData.DataLength  = (uint32_t)ReturnData.DataBuffer[3] << 8;
                                ReturnData.DataLength |= (uint32_t)ReturnData.DataBuffer[2];
                            }break;

                            case DESCRIPTOR_STRING: 
                            {
                                switch (StandardRequest.wValue[0])
                                {
                                    case STRING_MANUFACTURE: 
                                    {
                                        if (DevDescriptors.stringManufacture != NULL)
                                            ReturnData.DataBuffer = DevDescriptors.stringManufacture;
                                        else
                                            usb_Endpoint0_SendStall();
                                    }break;
                                    
                                    case STRING_PRODUCT: 
                                    {
                                        if (DevDescriptors.stringProduct != NULL)
                                            ReturnData.DataBuffer = DevDescriptors.stringProduct;
                                        else
                                            usb_Endpoint0_SendStall();
                                    }break;
                                    
                                    case STRING_SERIAL_Number: 
                                    {
                                        if (DevDescriptors.stringSerialNumber != NULL)
                                            ReturnData.DataBuffer = DevDescriptors.stringSerialNumber;
                                        else
                                            usb_Endpoint0_SendStall();
                                    }break;
                                    
                                    case STRING_LANUAGE_ID: 
                                    {
                                        if (DevDescriptors.stringLanuageID != NULL)
                                            ReturnData.DataBuffer = DevDescriptors.stringLanuageID;
                                        else
                                            usb_Endpoint0_SendStall();
                                    }break;

                                    case STRING_OS: 
                                    {
                                        if (DevDescriptors.stringOS != NULL)
                                            ReturnData.DataBuffer = DevDescriptors.stringOS;
                                        else
                                            usb_Endpoint0_SendStall();
                                    }break;
                                    
                                    /* User-defined string descriptor */
                                    default: 
                                    {
                                        if (Endpoint_0_UserStringRequest_Handler != NULL)
                                        {
                                            Endpoint_0_UserStringRequest_Handler(StandardRequest.wValue[0], &ReturnData);
                                        }
                                        else
                                        {
                                            usb_Endpoint0_SendStall();
                                        }
                                    }break; 
                                }
                                
                                if (ReturnData.DataBuffer != NULL)
                                    ReturnData.DataLength = ReturnData.DataBuffer[0];
                            }break;

                            case DESCRIPTOR_DEVICE_QUALIFIER:
                            {
                                /* USB2.0 full speed send stall */
                                usb_Endpoint0_SendStall();
                            }break;

                            case DESCRIPTOR_BOS:
                            {
                                /* USB2.0 does not support DESCRIPTOR_BOS */
                                usb_Endpoint0_SendStall();
                            }break;

                            case DESCRIPTOR_HID:
                            case DESCRIPTOR_HID_REPORT:
                            {
                                if (Endpoint_0_StandardClassRequest_Handler != NULL) 
                                {
                                    Endpoint_0_StandardClassRequest_Handler(&StandardRequest, &ReturnData);
                                }
                            }break;

                            default: break; 
                        }
                    }break;

                    /* set device address */
                    case REQUEST_SET_ADDRESS: 
                    {
                        if (StandardRequest.wValue[1] != 0)
                        {
                            /* Reject illegal address > 255 */
                            usb_Endpoint0_SendStall();
                        }
                        else
                        {
                            if (StandardRequest.wValue[0] > 127)
                            {
                                /* Reject illegal address > 127 */
                                usb_Endpoint0_SendStall();
                            }
                            else
                            {
                                /* wait an empty IN packet  */
                                lb_WaitSetAddress = true;
                            }
                        }
                    }break;

                    /* set configuration */
                    case REQUEST_SET_CONFIGURATION: 
                    {
                        USB_Configuration = StandardRequest.wValue[0];
                    }break;
                    /* get configuration */
                    case REQUEST_GET_CONFIGURATION: 
                    {
                        ReturnData.RequestLength = 1;
                        ReturnData.DataBuffer = &USB_Configuration;
                        ReturnData.DataLength = 1;
                    }break;

                    /* set interface */
                    case REQUEST_SET_INTERFACE: 
                    {
                        USB_InterfaceAlternateSet[StandardRequest.wIndex[0]] = StandardRequest.wValue[0];
                        if (USB_InterfaceAlternateSet_callback != NULL) 
                        {
                            USB_InterfaceAlternateSet_callback(StandardRequest.wIndex[0]);
                        }
                    }break;
                    /* get interface */
                    case REQUEST_GET_INTERFACE: 
                    {
                        ReturnData.RequestLength  = (uint16_t)StandardRequest.wLength[1] << 8;
                        ReturnData.RequestLength |= (uint16_t)StandardRequest.wLength[0];

                        ReturnData.DataBuffer = &USB_InterfaceAlternateSet[StandardRequest.wIndex[0]];
                        ReturnData.DataLength = ReturnData.RequestLength;
                    }break;

                    /* get status */
                    case REQUEST_GET_STATUS: 
                    {
                        /* Get Device status */
                        if ((StandardRequest.bmRequestType & REQUEST_RECIPIENT_MASK) == RECIPIENT_DEVICE)
                        {
                            ReturnData.DataBuffer = (uint8_t *)&USB_Device_Status;
                        }
                        /* Get Interface status */
                        else if ((StandardRequest.bmRequestType & REQUEST_RECIPIENT_MASK) == RECIPIENT_INTERFACE)
                        {
                            ReturnData.DataBuffer = (uint8_t *)&USB_Interface_Status;
                        }
                        /* Get Endpoints status */
                        else if((StandardRequest.bmRequestType & REQUEST_RECIPIENT_MASK) == RECIPIENT_ENDPOINT)
                        {
                            if (StandardRequest.wIndex[0] & DIRECTION_IN)
                            {
                                ReturnData.DataBuffer = (uint8_t *)&USB_IN_Endpoint_Status[StandardRequest.wIndex[0] & 0x0F];
                            }
                            else
                            {
                                ReturnData.DataBuffer = (uint8_t *)&USB_OUT_Endpoint_Status[StandardRequest.wIndex[0] & 0x0F];
                            }
                        }
                        /* Build the returned data */
                        ReturnData.RequestLength = 2;
                        ReturnData.DataLength    = 2;
                    }break;

                    /* Set feature */
                    case REQUEST_SET_FEATURE:
                    {
                        /* clear device feature */
                        if ((StandardRequest.bmRequestType & REQUEST_RECIPIENT_MASK) == RECIPIENT_DEVICE)
                        {
                            if ((StandardRequest.wValue[0] == 1) && (StandardRequest.wValue[1] == 0))
                            {
                                USB_Device_Status |= DEVICE_STATUS_D1_REMOTE_WAKEUP;
                            }
                        }
                        /* Set Endpoint feature */
                        else if ((StandardRequest.bmRequestType & REQUEST_RECIPIENT_MASK) == RECIPIENT_ENDPOINT)
                        {
                            if ((StandardRequest.wValue[0] == 0) && (StandardRequest.wValue[1] == 0))
                            {
                                if (StandardRequest.wIndex[0] & DIRECTION_IN)
                                {
                                    USB_IN_Endpoint_Status[StandardRequest.wIndex[0] & 0x0F] = ENDPOINT_STATUS_D0_HALT;
                                }
                                else
                                {
                                    USB_OUT_Endpoint_Status[StandardRequest.wIndex[0] & 0x0F] = ENDPOINT_STATUS_D0_HALT;
                                }
                            }
                        }
                    }break;
                    /* Clear feature */
                    case REQUEST_CLEAR_FEATURE:
                    {
                        /* clear device feature */
                        if ((StandardRequest.bmRequestType & REQUEST_RECIPIENT_MASK) == RECIPIENT_DEVICE)
                        {
                            if ((StandardRequest.wValue[0] == 1) && (StandardRequest.wValue[1] == 0))
                            {
                                USB_Device_Status &= ~DEVICE_STATUS_D1_REMOTE_WAKEUP;
                            }
                        }
                        /* clear Endpoint feature */
                        else if ((StandardRequest.bmRequestType & REQUEST_RECIPIENT_MASK) == RECIPIENT_ENDPOINT)
                        {
                            if ((StandardRequest.wValue[0] == 0) && (StandardRequest.wValue[1] == 0))
                            {
                                if (StandardRequest.wIndex[0] & DIRECTION_IN)
                                {
                                    USB_IN_Endpoint_Status[StandardRequest.wIndex[0] & 0x0F] = 0x0000;
                                }
                                else
                                {
                                    USB_OUT_Endpoint_Status[StandardRequest.wIndex[0] & 0x0F] = 0x0000;
                                }
                            }
                        }
                    }break;

                    default: break; 
                }
            }
            /* Class Request */
            else if ((StandardRequest.bmRequestType & REQUEST_TYPE_MASK) == TYPE_CLASS)
            {
                if (Endpoint_0_ClassRequest_Handler != NULL) 
                {
                    Endpoint_0_ClassRequest_Handler(&StandardRequest, &ReturnData);
                }
            }
            /* Vendor Request */
            else if ((StandardRequest.bmRequestType & REQUEST_TYPE_MASK) == TYPE_VENDOR)
            {
                if (Endpoint_0_VendorRequest_Handler != NULL) 
                {
                    Endpoint_0_VendorRequest_Handler(&StandardRequest, &ReturnData);
                }
            }

            /* clear TxPktRdy/RxPktRdy */
            usb_Endpoint0_FlushFIFO();
        }

    }
    /* endpoint 0 transmit a packet */
    else 
    {
        if (lb_WaitSetAddress == true) 
        {
            lb_WaitSetAddress = false;

            usb_set_address(StandardRequest.wValue[0]);
        }
    }

    /* DataBuffer needs to be sent */
    if (ReturnData.DataLength)
    {
        if (ReturnData.RequestLength < ReturnData.DataLength) 
        {
            ReturnData.DataLength = ReturnData.RequestLength;
        }

        if (ReturnData.DataLength > ENDPOINT0_MAX) 
        {
            usb_write_fifo(ENDPOINT_0, ReturnData.DataBuffer, ENDPOINT0_MAX);

            ReturnData.DataLength -= ENDPOINT0_MAX;
            ReturnData.DataBuffer += ENDPOINT0_MAX;
        }
        else 
        {
            usb_write_fifo(ENDPOINT_0, ReturnData.DataBuffer, ReturnData.DataLength);

            if (ReturnData.DataLength == ENDPOINT0_MAX)
            {   /* Packet length is equal to 64 byte, without DataEnd */
                lb_PktMAX = true;
            }
            else
            {
                usb_Endpoint0_DataEnd();
            }

            ReturnData.DataBuffer = 0;
            ReturnData.DataLength = 0;
        }
        usb_Endpoint0_SET_TxPktRdy();
    }
    else
    {
        /* The packet length of 64 bytes was sent */
        if (lb_PktMAX)
        {
            /* The next packet is empty */
            usb_Endpoint0_DataEnd();
            usb_Endpoint0_SET_TxPktRdy();
            lb_PktMAX = false;
        }
    }
}

/*********************************************************************
 * @fn      Endpoints_IRQHandler
 *
 * @brief   ALL Endpoint Interrupt Request handler
 *
 * @param   None.
 * @return  None.
 */
static void Endpoints_IRQHandler(void)
{
    volatile uint8_t lu8_RxStatus;
    volatile uint8_t lu8_TxStatus;
    volatile uint8_t lu8_EndpointBackup;

    lu8_EndpointBackup = usb_get_endpoint();

    lu8_TxStatus = usb_get_TxStatus();
    lu8_RxStatus = usb_get_RxStatus();

    /* endpoint 0 Interrupt handler */
    if (lu8_TxStatus & ENDPOINT_0_MASK) 
    {
        lu8_TxStatus &= ~ENDPOINT_0_MASK;

        Endpoint_0_IRQHandler();
    }

    /* other endpoints Rx/Tx Interrupt handler */
    if (lu8_RxStatus | lu8_TxStatus) 
    {
        if (Endpoints_Handler != NULL) 
        {
            Endpoints_Handler(lu8_RxStatus, lu8_TxStatus);
        }
    }

    usb_selecet_endpoint((enum_Endpoint_t)lu8_EndpointBackup);
}

static void USB_Status_IRQHandler(void)
{
    volatile uint8_t lu8_EndpointBackup;
    volatile uint8_t lu8_USBStatus;

    lu8_EndpointBackup = usb_get_endpoint();

    lu8_USBStatus = usb_get_USBStatus();

    /* USB Bus SOF packet */
    if (lu8_USBStatus & USB_INT_STATUS_SOF)
    {
        if (USB_SOF_Handler != NULL) 
        {
            USB_SOF_Handler();
        }
    }
    /* USB Bus reset signal, Will be clear USB register */
    /* need configure USB register again */
    if (lu8_USBStatus & USB_INT_STATUS_RESET) 
    {
        USB->IntrUSBE = 0x04;    /* Enable Bus reset INT */
        USB->IntrTx1E = 0x01;    /* Enable Endpoint0 INT */
        USB->IntrTx2E = 0x00;
        USB->IntrRx1E = 0x00;
        USB->IntrRx2E = 0x00;

        if (USB_Reset_Handler != NULL) 
        {
            USB_Reset_Handler();
        }
    }
    /* USB Bus Resume signal */
    if (lu8_USBStatus & USB_INT_STATUS_RESUME)
    {
        if (USB_Resume_Handler != NULL) 
        {
            USB_Resume_Handler();
        }
    }
    /* USB Bus Suspend signal */
    if (lu8_USBStatus & USB_INT_STATUS_SUSPEND)
    {
        if (USB_Suspend_Handler != NULL) 
        {
            USB_Suspend_Handler();
        }
    }
    /* USB Bus connect signal */
    if (lu8_USBStatus & USB_INT_STATUS_CONN)
    {
        if (USB_Connect_Handler != NULL) 
        {
            USB_Connect_Handler();
        }
    }

    usb_selecet_endpoint((enum_Endpoint_t)lu8_EndpointBackup);
}

#define USB_IRQHandler usbotg_irq

/*********************************************************************
* @fn      USB_IRQHandler
*
* @brief   USB interrupt Request handler
*
* @param   None.
* @return  None.
*/
void USB_IRQHandler(void) 
{
    Endpoints_IRQHandler();

    USB_Status_IRQHandler();
}
