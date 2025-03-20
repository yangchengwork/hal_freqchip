/*
  ******************************************************************************
  * @file    usb_host.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
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

#define DEVICE_ACCESS_ADDR    (0x2A)

/* device class type */
static uint32_t Device_Class = DEVICE_CLASS_UNKNOWN;

/* communication status */
static uint32_t ConnectStatus = DEVICE_IDLE;
static uint32_t EnumStatus = ENUM_STATUS_IDLE;

/* point to usb host status handler */
void (*USBH_Resume_Handler)(void) = NULL;
void (*USBH_Connect_Handler)(void) = NULL;
void (*USBH_Disconnect_Handler)(void) = NULL;

/* point to usb host class handler */
void (*USBH_Class_Handler)(void) = NULL;

/* point to usb host other endpoints handler */
void (*Endpoints_Handler)(uint8_t RxStatus, uint8_t TxStatus) = NULL;

/* usb host standard request */
const uint8_t standard_request_get_descriptor_Dvc_64[]  = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00};
const uint8_t standard_request_get_descriptor_Dvc_18[]  = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
const uint8_t standard_request_get_descriptor_Cfg_09[]  = {0x80, 0x06, 0x00, 0x02, 0x00, 0x00, 0x09, 0x00};
const uint8_t standard_request_get_descriptor_Cfg_All[] = {0x80, 0x06, 0x00, 0x02, 0x00, 0x00, 0xFF, 0x00};
const uint8_t standard_request_get_descriptor_Str[]     = {0x80, 0x06, 0x00, 0x03, 0x00, 0x00, 0xFF, 0x00};
const uint8_t standard_request_set_access_addr[]        = {0x00, 0x05, DEVICE_ACCESS_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t standard_request_set_configuration[]      = {0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
/* standard request struct */
usb_StandardRequest_t standard_request;

/* return error code */
static uint32_t Error_code = ERROR_CODE_NO_ERR;

/* wait request end status flag */
static uint8_t wait_tx_request_end_status = SEND_STATUS_IDLE;
static uint8_t wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

/* device descriptor ready flag */
static uint8_t device_descriptor_Dvc_Ready;
static uint8_t device_descriptor_Cfg_Ready;
static uint8_t device_descriptor_Cfg_All_Ready;
static uint8_t device_descriptor_iManufacturer_Ready;
static uint8_t device_descriptor_iProduct_Ready;
static uint8_t device_descriptor_iSerialnumber_Ready;

/* storage device descriptor information */
descriptor_Dvc device_descriptor_Dvc;
descriptor_Cfg device_descriptor_Cfg;
uint8_t device_descriptor_Cfg_All[256];
uint8_t device_descriptor_Str_iManufacturer[256];
uint8_t device_descriptor_Str_iProduct[256];
uint8_t device_descriptor_Str_iSerialnumber[256];

/* storage device interface descriptor information */
descriptor_Ifc *device_descriptor_Ifc[USB_HOST_MAX_IFC_NUM];

/* private communication param */
descriptor_Param_t desc_Param;

/* login Mass Storage endpoint */
descriptor_Ifc *Storage_Ifc;
descriptor_Ep  *Storage_IN_Endpoint;
descriptor_Ep  *Storage_OUT_Endpoint;

/* class comm use endpoint0 */
static uint8_t USB_Host_get_descriptor(void);
uint8_t (*USB_Host_get_calss_desc)(void) = USB_Host_get_descriptor;

/*********************************************************************
 * @fn      USB_Host_get_error_code/USB_Host_set_error_code
 *
 * @brief   get error code.
 */
uint32_t USB_Host_get_error_code(void)
{
    return Error_code;
}
void USB_Host_set_error_code(uint32_t fu32_Error_code)
{
    Error_code = fu32_Error_code;
}

/*********************************************************************
 * @fn      USB_Host_get_connect_status
 *
 * @brief   get connect status.
 */
uint32_t USB_Host_get_connect_status(void)
{
    return ConnectStatus;
}
void USB_Host_set_connect_status(uint32_t fu32_ConnectStatus)
{
    ConnectStatus = fu32_ConnectStatus;
}

/*********************************************************************
 * @fn      USB_Host_get_enum_status/USB_Host_set_enum_status
 *
 * @brief   get connect status.
 */
uint32_t USB_Host_get_enum_status(void)
{
    return EnumStatus;
}
void USB_Host_set_enum_status(uint32_t fu32_EnumStatus)
{
    EnumStatus = fu32_EnumStatus;
}

/*********************************************************************
 * @fn      USB_Host_get_device_class
 *
 * @brief   get device class.
 */
uint32_t USB_Host_get_device_class(void)
{
    return Device_Class;
}

/*********************************************************************
 * @fn      USB_Host_get_Manufacturer_String_length
 *
 * @brief   get manufacturer string length.
 */
uint8_t USB_Host_get_Manufacturer_String_length(void)
{
    if (device_descriptor_Str_iManufacturer[0])
        return (device_descriptor_Str_iManufacturer[0] - 2) / 2;
    else
        return 0;
}
void USB_Host_get_Manufacturer_String(uint8_t *fp_Buffer)
{
    uint8_t length = USB_Host_get_Manufacturer_String_length();

    for (int i = 0; i < length; i++)
        fp_Buffer[i] = device_descriptor_Str_iManufacturer[(i+1)*2];
}

/*********************************************************************
 * @fn      USB_Host_get_Product_String_length
 *
 * @brief   get manufacturer string length.
 */
uint8_t USB_Host_get_Product_String_length(void)
{
    if (device_descriptor_Str_iProduct[0])
        return (device_descriptor_Str_iProduct[0] -2 ) / 2;
    else
        return 0;
}
void USB_Host_get_Product_String(uint8_t *fp_Buffer)
{
    uint8_t length = USB_Host_get_Product_String_length();

    for (int i = 0; i < length; i++)
        fp_Buffer[i] = device_descriptor_Str_iProduct[(i+1)*2];
}

/*********************************************************************
 * @fn      USB_Host_get_Serialnumber_String_length
 *
 * @brief   get Serialnumber string length.
 */
uint8_t USB_Host_get_Serialnumber_String_length(void)
{
    if (device_descriptor_Str_iSerialnumber[0])
        return (device_descriptor_Str_iSerialnumber[0] - 2) / 2;
    else
        return 0;
}
void USB_Host_get_Serialnumber_String(uint8_t *fp_Buffer)
{
    uint8_t length = USB_Host_get_Serialnumber_String_length();

    for (int i = 0; i < length; i++)
        fp_Buffer[i] = device_descriptor_Str_iSerialnumber[(i+1)*2];
}

/*********************************************************************
 * @fn      USB_Host_thread
 *
 * @brief   USB host Processing thread.
 */
void USB_Host_thread(void)
{
    static uint8_t RetryCount;
    static uint16_t lu16_FrameID[2];
    static uint8_t IN_status_packet_index;

    uint8_t *Standard_Request_Point = (uint8_t *)&standard_request;

    switch (ConnectStatus)
    {
        /* Device insertion detected */
        case DEVICE_CONNECT:
        {
            RetryCount = 0;

            Device_Class = DEVICE_CLASS_UNKNOWN;

            wait_tx_request_end_status = SEND_STATUS_IDLE;
            wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

            Error_code = ERROR_CODE_NO_ERR;

            system_delay_us(1000 * 20);
            usb_Host_ResetSignalStart();
            system_delay_us(1000 * 30);
            usb_Host_ResetSignalStop();
            system_delay_us(1000 * 1);
            
            lu16_FrameID[0] = 0;
            lu16_FrameID[1] = 0;

            EnumStatus = ENUM_STATUS_IDLE;
            ConnectStatus = DEVICE_WAIT_A;
        }break;

        /* get descriptor(DVC) failed, retry */
        case DEVICE_CONNECT_RETRY:
        {
            system_delay_us(1000 * 5);
            usb_Host_ResetSignalStart();
            system_delay_us(1000 * 30);
            usb_Host_ResetSignalStop();
            system_delay_us(1000 * 1);
            
            lu16_FrameID[0] = 0;
            lu16_FrameID[1] = 0;

            ConnectStatus = DEVICE_WAIT_A;
        }break;

        /* wait get descriptor(DVC) */
        case DEVICE_WAIT_A:
        {
            lu16_FrameID[0] = usb_get_frame();
            if (lu16_FrameID[0] != 0)
                ConnectStatus = DEVICE_WAIT_B;
        }break;
        /* wait get descriptor(DVC) */
        case DEVICE_WAIT_B:
        {
            lu16_FrameID[1] = usb_get_frame();
            if (lu16_FrameID[1] != 0)
            {
                if (lu16_FrameID[1] >= lu16_FrameID[0])
                {
                    if (lu16_FrameID[1] - lu16_FrameID[0] > 50)
                        ConnectStatus = DEVICE_TX_REQUEST_GET_DESC_DVC;
                }
                else
                {
                    if (lu16_FrameID[1] > 50)
                        ConnectStatus = DEVICE_TX_REQUEST_GET_DESC_DVC;
                }
            }
        }break;

        /* send standard request, get descriptor(DVC) */
        case DEVICE_TX_REQUEST_GET_DESC_DVC:
        {
            ConnectStatus = DEVICE_WAIT_TX_REQUEST_GET_DESC_DVC_END;

            usb_selecet_endpoint(ENDPOINT_0);
            usb_Endpoint0_FlushFIFO();

            usb_write_fifo(ENDPOINT_0, (uint8_t *)standard_request_get_descriptor_Dvc_64, USB_STANDARD_REQUEST_LENGTH);
            usb_Host_Endpoint0_SET_TxPktRdy_SetupPkt();
        }break;
        /* wait standard request end */
        case DEVICE_WAIT_TX_REQUEST_GET_DESC_DVC_END:
        {
            if (wait_tx_request_end_status == SEND_STATUS_FAIL)
            {   /* clear status */
                wait_tx_request_end_status = SEND_STATUS_IDLE;

                RetryCount++;

                if (RetryCount >= 3)
                {
                    RetryCount = 0;

                    Error_code = ERROR_CODE_DEVICE_NO_RESPONSE;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                else
                {
                    ConnectStatus = DEVICE_CONNECT_RETRY;
                }
            }
            else if (wait_tx_request_end_status == SEND_STATUS_SUCCEED)
            {   /* clear status */
                wait_tx_request_end_status = SEND_STATUS_IDLE;

                ConnectStatus = DEVICE_RX_DESC_DVC_RESPONSE;
            }
        }break;

        /* reveive descriptor(DVC) */
        case DEVICE_RX_DESC_DVC_RESPONSE:
        {
            ConnectStatus = DEVICE_WAIT_RX_DESC_DVC_RESPONSE_END;

            usb_selecet_endpoint(ENDPOINT_0);
            usb_Host_Endpoint0_SET_ReqPkt();
        }break;
        /* wait reveive descriptor(DVC) end */
        case DEVICE_WAIT_RX_DESC_DVC_RESPONSE_END:
        {
            if (wait_rx_request_end_status == RECEIVE_STATUS_SUCCEED)
            {
                wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                ConnectStatus = DEVICE_SECOND_RESET;

                lu16_FrameID[0] = 0;
                lu16_FrameID[1] = 0;
            }
            else if (wait_rx_request_end_status == RECEIVE_STATUS_FAIL)
            {
                wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                Error_code = ERROR_CODE_RECEIVE_DESC_DVC_TIMEOUT;
                ConnectStatus = DEVICE_DISCONNECT;
            }
            else if (wait_rx_request_end_status == RECEIVE_STATUS_NAK_LIMIT)
            {
                wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                RetryCount++;

                if (RetryCount >= 3)
                {
                    RetryCount = 0;

                    Error_code = ERROR_CODE_RECEIVE_DESC_DVC_TIMEOUT;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                else
                {
                    ConnectStatus = DEVICE_CONNECT_RETRY;
                }
            }
        }break;

        /* The second Reset is executed */
        case DEVICE_SECOND_RESET:
        {
            if (lu16_FrameID[0] == 0 && lu16_FrameID[1] == 0)
            {
                system_delay_us(1000 * 5);
                usb_Host_ResetSignalStart();
                system_delay_us(1000 * 30);
                usb_Host_ResetSignalStop();

                while(lu16_FrameID[0] == 0)
                {
                    lu16_FrameID[0] = usb_get_frame();
                }
            }
            else
            {
                lu16_FrameID[1] = usb_get_frame();
                if (lu16_FrameID[1] != 0)
                {
                    if (lu16_FrameID[1] >= lu16_FrameID[0])
                    {
                        if (lu16_FrameID[1] - lu16_FrameID[0] > 50)
                            ConnectStatus = DEVICE_SET_ADDRESS;
                    }
                    else
                    {
                        if (lu16_FrameID[1] > 50)
                            ConnectStatus = DEVICE_SET_ADDRESS;
                    }
                }
            }
        }break;

        /* set device access address */
        case DEVICE_SET_ADDRESS:
        {
            ConnectStatus = DEVICE_WAIT_SEND_END;

            IN_status_packet_index = END0_IN_STATUS_PACKET_INDEX_SET_ADDR;

            usb_selecet_endpoint(ENDPOINT_0);
            usb_write_fifo(ENDPOINT_0, (uint8_t *)standard_request_set_access_addr, USB_STANDARD_REQUEST_LENGTH);
            usb_Host_Endpoint0_SET_TxPktRdy_SetupPkt();
        }break;
        /* wait set device access address end */
        case DEVICE_WAIT_SEND_END:
        {
            /* Set address end */
            if (IN_status_packet_index == END0_IN_STATUS_PACKET_INDEX_SET_ADDR)
            {
                if (wait_tx_request_end_status == RECEIVE_STATUS_FAIL) 
                {
                    wait_tx_request_end_status = RECEIVE_STATUS_IDLE;

                    Error_code = ERROR_CODE_SET_ADDR_FAIL;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                else if (wait_tx_request_end_status == RECEIVE_STATUS_NAK_LIMIT)
                {
                    wait_tx_request_end_status = RECEIVE_STATUS_IDLE;

                    Error_code = ERROR_CODE_SET_ADDR_FAIL;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                else if (wait_tx_request_end_status == RECEIVE_STATUS_SUCCEED)
                {
                    wait_tx_request_end_status = RECEIVE_STATUS_IDLE;
                    ConnectStatus = DEVICE_RX_STATUS_PACKET;
                }
            }
            /* Set config end */
            else if (IN_status_packet_index == END0_IN_STATUS_PACKET_INDEX_SET_CONFIG)
            {
                if (wait_tx_request_end_status == RECEIVE_STATUS_FAIL) 
                {
                    wait_tx_request_end_status = RECEIVE_STATUS_IDLE;

                    Error_code = ERROR_CODE_SET_CONFIG_FAIL;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                else if (wait_tx_request_end_status == RECEIVE_STATUS_NAK_LIMIT)
                {
                    wait_tx_request_end_status = RECEIVE_STATUS_IDLE;

                    Error_code = ERROR_CODE_SET_CONFIG_FAIL;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                else if (wait_tx_request_end_status == RECEIVE_STATUS_SUCCEED)
                {
                    wait_tx_request_end_status = RECEIVE_STATUS_IDLE;
                    ConnectStatus = DEVICE_RX_STATUS_PACKET;
                }
            }
        }break;
        /* rx status packet */
        case DEVICE_RX_STATUS_PACKET:
        {
            usb_selecet_endpoint(ENDPOINT_0);
            usb_Host_Endpoint0_SET_ReqPkt_StatusPkt();

            while (wait_rx_request_end_status == RECEIVE_STATUS_IDLE);

            /* set access address status packet */
            if (IN_status_packet_index == END0_IN_STATUS_PACKET_INDEX_SET_ADDR)
            {
                IN_status_packet_index = 0;

                if (wait_rx_request_end_status == RECEIVE_STATUS_SUCCEED)
                {
                    wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                    usb_set_address(DEVICE_ACCESS_ADDR);

                    ConnectStatus = DEVICE_ENUM;

                    device_descriptor_Dvc_Ready = DEVICE_DESC_NONE;
                    device_descriptor_Cfg_Ready = DEVICE_DESC_NONE;
                    device_descriptor_Cfg_All_Ready       = DEVICE_DESC_NONE;
                    device_descriptor_iManufacturer_Ready = DEVICE_DESC_NONE;
                    device_descriptor_iProduct_Ready      = DEVICE_DESC_NONE;
                    device_descriptor_iSerialnumber_Ready = DEVICE_DESC_NONE;

                    device_descriptor_Str_iManufacturer[0] = 0;
                    device_descriptor_Str_iProduct[0] = 0;
                    device_descriptor_Str_iSerialnumber[0] = 0;
                }
                else
                {
                    wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                    Error_code = ERROR_CODE_SET_ADDR_STATUS_ERR;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
            }
            /* set configuration status packet */
            else if (IN_status_packet_index == END0_IN_STATUS_PACKET_INDEX_SET_CONFIG)
            {
                IN_status_packet_index = 0;

                if (wait_rx_request_end_status == RECEIVE_STATUS_SUCCEED)
                {
                    wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                    ConnectStatus = DEVICE_CLASS_HANDLE;
                }
                else
                {
                    wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                    Error_code = ERROR_CODE_SET_CONFIG_STATUS_ERR;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
            }
        }break;

        /* enum */
        case DEVICE_ENUM:
        {
            /* device descriptor */
            if (device_descriptor_Dvc_Ready == DEVICE_DESC_NONE)
            {
                if (EnumStatus == ENUM_STATUS_IDLE)
                {
                    EnumStatus = ENUM_STATUS_REQUEST_DESC;

                    desc_Param.descriptor_Ready   = &device_descriptor_Dvc_Ready;
                    desc_Param.descriptor_request = (uint8_t *)standard_request_get_descriptor_Dvc_18;
                    desc_Param.descriptor_point   = (uint8_t *)&device_descriptor_Dvc;
                    desc_Param.descriptor_length  = 18;
                    desc_Param.descriptor_count   = 0;
                }

                if (USB_Host_get_descriptor())
                {
                    Error_code = ERROR_CODE_RECEIVE_DESC_DVC_FAIL;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                break;
            }
            else if (device_descriptor_Dvc_Ready == DEVICE_DESC_CHECK)
            {
                if (device_descriptor_Dvc.bDeviceClass != DEVICE_MASS_STORAGE_CLASS)
                {
                    if (device_descriptor_Dvc.bDeviceClass != DEVICE_CLASS_INFO_IN_IFC)
                    {
                        Error_code = ERROR_CODE_UNKNOWN_DEVICE;
                        ConnectStatus = DEVICE_DISCONNECT;
                    }
                    else
                    {
                        device_descriptor_Dvc_Ready = DEVICE_DESC_READY;
                    }
                }
            }

            /* configuration descriptor */
            if (device_descriptor_Cfg_Ready == DEVICE_DESC_NONE)
            {
                if (EnumStatus == ENUM_STATUS_IDLE)
                {
                    EnumStatus = ENUM_STATUS_REQUEST_DESC;

                    desc_Param.descriptor_Ready   = &device_descriptor_Cfg_Ready;
                    desc_Param.descriptor_request = (uint8_t *)standard_request_get_descriptor_Cfg_09;
                    desc_Param.descriptor_point   = (uint8_t *)&device_descriptor_Cfg;
                    desc_Param.descriptor_length  = 0x9;
                    desc_Param.descriptor_count   = 0;
                }

                if (USB_Host_get_descriptor())
                {
                    Error_code = ERROR_CODE_RECEIVE_DESC_CFG_FAIL;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                break;
            }
            else if (device_descriptor_Cfg_Ready == DEVICE_DESC_CHECK)
            {
                if (device_descriptor_Cfg.bDescriptorType != DESCRIPTOR_TYPE_CONFIGURATION)
                {
                    Error_code = ERROR_CODE_RECEIVE_DESC_CFG_ERROR;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                if (device_descriptor_Cfg.wTotalLength > 0xFF)
                {
                    Error_code = ERROR_CODE_UNKNOWN_DEVICE;
                    ConnectStatus = DEVICE_DISCONNECT;
                }

                if (Error_code == ERROR_CODE_NO_ERR)
                    device_descriptor_Cfg_Ready = DEVICE_DESC_READY;
            }

            /* configuration descriptor ALL */
            if (device_descriptor_Cfg_All_Ready == DEVICE_DESC_NONE)
            {
                if (EnumStatus == ENUM_STATUS_IDLE)
                {
                    EnumStatus = ENUM_STATUS_REQUEST_DESC;

                    for (int t = 0; t < USB_STANDARD_REQUEST_LENGTH; t++)
                        Standard_Request_Point[t] = standard_request_get_descriptor_Cfg_All[t];

                    standard_request.wLength[0] = device_descriptor_Cfg.wTotalLength;

                    desc_Param.descriptor_Ready   = &device_descriptor_Cfg_All_Ready;
                    desc_Param.descriptor_request = (uint8_t *)&standard_request;
                    desc_Param.descriptor_point   = (uint8_t *)&device_descriptor_Cfg_All;
                    desc_Param.descriptor_length  = device_descriptor_Cfg.wTotalLength;
                    desc_Param.descriptor_count   = 0;
                }

                if (USB_Host_get_descriptor())
                {
                    Error_code = ERROR_CODE_RECEIVE_DESC_CFG_FAIL;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                break;
            }
            else if (device_descriptor_Cfg_All_Ready == DEVICE_DESC_CHECK)
            {
                /* analysis configuration descriptor */
                if (device_descriptor_Cfg.bNumInterfaces > USB_HOST_MAX_IFC_NUM)
                {
                    Error_code = ERROR_CODE_UNKNOWN_DEVICE;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                uint16_t Ifc_count = 0;
                uint16_t Cfg_count = 9;
                uint16_t next_length;
                uint16_t next_type;

                /* Look for interface descriptors */
                while (Cfg_count < device_descriptor_Cfg.wTotalLength)
                {
                    next_length = device_descriptor_Cfg_All[Cfg_count];
                    next_type   = device_descriptor_Cfg_All[Cfg_count + 1];

                    if (next_length == 0)
                    {   /* length error */
                        Error_code = ERROR_CODE_RECEIVE_DESC_LENGTH_ERROR;
                        ConnectStatus = DEVICE_DISCONNECT;
                    }

                    if (next_type == DESCRIPTOR_TYPE_INTERFACE)
                    {
                        device_descriptor_Ifc[Ifc_count++] = (descriptor_Ifc *)&device_descriptor_Cfg_All[Cfg_count];
                    }

                    Cfg_count += next_length;
                }
                /* check interface descriptors */
                for (int i = 0; i < device_descriptor_Cfg.bNumInterfaces; i++)
                {
                    /* check mass storage device */
                    if (device_descriptor_Ifc[i]->bInterfaceClass == DEVICE_MASS_STORAGE_CLASS)
                    {
                        if (device_descriptor_Ifc[i]->bInterfaceSubClass == DEVICE_SCSI_TRANSPARENT_CMD_SET)
                        {
                            if (device_descriptor_Ifc[i]->bInterfaceProtocol == DEVICE_BULK_ONLY_TRANSPORT)
                            {
                                if (device_descriptor_Ifc[i]->bNumEndpoints == 2)
                                {
                                    Device_Class = DEVICE_CLASS_STORAGE_DEVICE;

                                    /* analysis mass storage device endpoint */
                                    uint8_t *BufferPoint = (uint8_t *)device_descriptor_Ifc[i];
                                    descriptor_Ep *Endpoint;

                                    Storage_Ifc = device_descriptor_Ifc[i];

                                    /* Endpoint0 */
                                    Endpoint = (descriptor_Ep *)(BufferPoint + 9);
                                    if ((Endpoint->bLength == 7) && (Endpoint->bDescriptorType == DESCRIPTOR_TYPE_ENDPOINT))
                                    {
                                        if (Endpoint->bEndpointAddress & 0x80)
                                            Storage_IN_Endpoint = Endpoint;
                                        else
                                            Storage_OUT_Endpoint = Endpoint;
                                    }
                                    else
                                    {
                                        Error_code = ERROR_CODE_UNKNOWN_STORAGE_DEVICE;
                                        ConnectStatus = DEVICE_DISCONNECT;
                                    }

                                    /* Endpoint1 */
                                    Endpoint = (descriptor_Ep *)(BufferPoint + 9 + 7);
                                    if ((Endpoint->bLength == 7) && (Endpoint->bDescriptorType == DESCRIPTOR_TYPE_ENDPOINT))
                                    {
                                        if (Endpoint->bEndpointAddress & 0x80)
                                            Storage_IN_Endpoint = Endpoint;
                                        else
                                            Storage_OUT_Endpoint = Endpoint;
                                    }
                                    else
                                    {
                                        Error_code = ERROR_CODE_UNKNOWN_STORAGE_DEVICE;
                                        ConnectStatus = DEVICE_DISCONNECT;
                                    }
                                }
                                else
                                {
                                    Error_code = ERROR_CODE_UNKNOWN_STORAGE_DEVICE;
                                    ConnectStatus = DEVICE_DISCONNECT;
                                }
                            }
                            else
                            {
                                Error_code = ERROR_CODE_UNKNOWN_STORAGE_DEVICE;
                                ConnectStatus = DEVICE_DISCONNECT;
                            }
                        }
                        else
                        {
                            Error_code = ERROR_CODE_UNKNOWN_STORAGE_DEVICE;
                            ConnectStatus = DEVICE_DISCONNECT;
                        }
                    }
                }

                if (Device_Class == DEVICE_CLASS_UNKNOWN)
                {
                    Error_code = ERROR_CODE_UNKNOWN_DEVICE;
                    ConnectStatus = DEVICE_DISCONNECT;
                }
                else
                {
                    device_descriptor_Cfg_All_Ready = DEVICE_DESC_READY;
                }
            }

            /* string descriptor(Manufacturer) */
            if (device_descriptor_iManufacturer_Ready == DEVICE_DESC_NONE)
            {
                if (device_descriptor_Dvc.iManufacturer)
                {
                    if (EnumStatus == ENUM_STATUS_IDLE)
                    {
                        EnumStatus = ENUM_STATUS_REQUEST_DESC;

                        for (int t = 0; t < USB_STANDARD_REQUEST_LENGTH; t++)
                            Standard_Request_Point[t] = standard_request_get_descriptor_Str[t];

                        standard_request.wValue[0] = device_descriptor_Dvc.iManufacturer;

                        desc_Param.descriptor_Ready   = &device_descriptor_iManufacturer_Ready;
                        desc_Param.descriptor_request = (uint8_t *)&standard_request;
                        desc_Param.descriptor_point   = (uint8_t *)&device_descriptor_Str_iManufacturer;
                        desc_Param.descriptor_length  = 0;
                        desc_Param.descriptor_count   = 0;
                    }

                    if (USB_Host_get_descriptor())
                    {
                        Error_code = ERROR_CODE_RECEIVE_DESC_CFG_FAIL;
                        ConnectStatus = DEVICE_DISCONNECT;
                    }
                    break;
                }
                else
                {
                    /* without string descriptor(Manufacturer) */
                    device_descriptor_iManufacturer_Ready = DEVICE_DESC_WITHOUT;
                }
            }
            else if (device_descriptor_iManufacturer_Ready == DEVICE_DESC_CHECK)
            {
                device_descriptor_iManufacturer_Ready = DEVICE_DESC_READY;
            }

            /* string descriptor(Manufacturer) */
            if (device_descriptor_iProduct_Ready == DEVICE_DESC_NONE)
            {
                if (device_descriptor_Dvc.iProduct)
                {
                    if (EnumStatus == ENUM_STATUS_IDLE)
                    {
                        EnumStatus = ENUM_STATUS_REQUEST_DESC;

                        for (int t = 0; t < USB_STANDARD_REQUEST_LENGTH; t++)
                            Standard_Request_Point[t] = standard_request_get_descriptor_Str[t];

                        standard_request.wValue[0] = device_descriptor_Dvc.iProduct;

                        desc_Param.descriptor_Ready   = &device_descriptor_iProduct_Ready;
                        desc_Param.descriptor_request = (uint8_t *)&standard_request;
                        desc_Param.descriptor_point   = (uint8_t *)&device_descriptor_Str_iProduct;
                        desc_Param.descriptor_length  = 0;
                        desc_Param.descriptor_count   = 0;
                    }

                    if (USB_Host_get_descriptor())
                    {
                        Error_code = ERROR_CODE_RECEIVE_DESC_CFG_FAIL;
                        ConnectStatus = DEVICE_DISCONNECT;
                    }
                    break;
                }
                else
                {
                    /* without string descriptor(Manufacturer) */
                    device_descriptor_iProduct_Ready = DEVICE_DESC_WITHOUT;
                }
            }
            else if (device_descriptor_iProduct_Ready == DEVICE_DESC_CHECK)
            {
                device_descriptor_iProduct_Ready = DEVICE_DESC_READY;
            }

            /* string descriptor(Manufacturer) */
            if (device_descriptor_iSerialnumber_Ready == DEVICE_DESC_NONE)
            {
                if (device_descriptor_Dvc.iSerialnumber)
                {
                    if (EnumStatus == ENUM_STATUS_IDLE)
                    {
                        EnumStatus = ENUM_STATUS_REQUEST_DESC;

                        for (int t = 0; t < USB_STANDARD_REQUEST_LENGTH; t++)
                            Standard_Request_Point[t] = standard_request_get_descriptor_Str[t];

                        standard_request.wValue[0] = device_descriptor_Dvc.iSerialnumber;

                        desc_Param.descriptor_Ready   = &device_descriptor_iSerialnumber_Ready;
                        desc_Param.descriptor_request = (uint8_t *)&standard_request;
                        desc_Param.descriptor_point   = (uint8_t *)&device_descriptor_Str_iSerialnumber;
                        desc_Param.descriptor_length  = 0;
                        desc_Param.descriptor_count   = 0;
                    }

                    if (USB_Host_get_descriptor())
                    {
                        Error_code = ERROR_CODE_RECEIVE_DESC_CFG_FAIL;
                        ConnectStatus = DEVICE_DISCONNECT;
                    }
                    break;
                }
                else
                {
                    /* without string descriptor(Manufacturer) */
                    device_descriptor_iSerialnumber_Ready = DEVICE_DESC_WITHOUT;
                }
            }
            else if (device_descriptor_iSerialnumber_Ready == DEVICE_DESC_CHECK)
            {
                device_descriptor_iSerialnumber_Ready = DEVICE_DESC_READY;
            }

            if (device_descriptor_Dvc_Ready           != DEVICE_DESC_NONE && \
                device_descriptor_Cfg_Ready           != DEVICE_DESC_NONE && \
                device_descriptor_Cfg_All_Ready       != DEVICE_DESC_NONE && \
                device_descriptor_iManufacturer_Ready != DEVICE_DESC_NONE && \
                device_descriptor_iProduct_Ready      != DEVICE_DESC_NONE && \
                device_descriptor_iSerialnumber_Ready != DEVICE_DESC_NONE)
            {
                ConnectStatus = DEVICE_SET_CONFIG;
            }
        }break;

        case DEVICE_SET_CONFIG:
        {
            ConnectStatus = DEVICE_WAIT_SEND_END;

            IN_status_packet_index = END0_IN_STATUS_PACKET_INDEX_SET_CONFIG;

            for (int t = 0; t < USB_STANDARD_REQUEST_LENGTH; t++)
                Standard_Request_Point[t] = standard_request_set_configuration[t];

            standard_request.wValue[0] = device_descriptor_Cfg.bConfigurationValue;

            usb_selecet_endpoint(ENDPOINT_0);
            usb_write_fifo(ENDPOINT_0, (uint8_t *)&standard_request, USB_STANDARD_REQUEST_LENGTH);
            usb_Host_Endpoint0_SET_TxPktRdy_SetupPkt();
        }break;

        case DEVICE_CLASS_HANDLE:
        {
            if (USBH_Class_Handler != NULL)
            {
                USBH_Class_Handler();
            }
        }break;

        case DEVICE_DISCONNECT:
        {
            ConnectStatus = DEVICE_IDLE;
        }break;

        default:break;
    }
}

/*********************************************************************
 * @fn      USB_Host_get_descriptor
 *
 * @brief   Host get descriptor(Dvc)(Cfg)(String)
 *
 * @param   None.
 * @return  None.
 */
static uint8_t USB_Host_get_descriptor(void)
{
    uint8_t Error = 0;

    switch (EnumStatus)
    {
        case ENUM_STATUS_REQUEST_DESC:
        {
            EnumStatus = ENUM_STATUS_WAIT_REQUEST_DESC_END;

            usb_selecet_endpoint(ENDPOINT_0);
            usb_write_fifo(ENDPOINT_0, desc_Param.descriptor_request, USB_STANDARD_REQUEST_LENGTH);
            usb_Host_Endpoint0_SET_TxPktRdy_SetupPkt();
        }break;

        case ENUM_STATUS_WAIT_REQUEST_DESC_END:
        {
            if (wait_tx_request_end_status == RECEIVE_STATUS_SUCCEED)
            {
                wait_tx_request_end_status = SEND_STATUS_IDLE;

                EnumStatus = ENUM_STATUS_RX_REPLY_DESC;
            }
            else if (wait_tx_request_end_status == RECEIVE_STATUS_FAIL)
            {
                wait_tx_request_end_status = SEND_STATUS_IDLE;

                EnumStatus = ENUM_STATUS_IDLE;

                Error = 1;
            }
        }break;

        case ENUM_STATUS_RX_REPLY_DESC:
        {
            EnumStatus = ENUM_STATUS_WAIT_RX_REPLY_DESC_END;

            usb_selecet_endpoint(ENDPOINT_0);
            usb_Host_Endpoint0_SET_ReqPkt();
        }break;

        case ENUM_STATUS_WAIT_RX_REPLY_DESC_END:
        {
            if (wait_rx_request_end_status == RECEIVE_STATUS_SUCCEED)
            {
                wait_rx_request_end_status = RECEIVE_STATUS_IDLE;

                EnumStatus = ENUM_STATUS_OUT_STATUS;
            }
            else if (wait_rx_request_end_status == RECEIVE_STATUS_FAIL)
            {
                wait_rx_request_end_status = RECEIVE_STATUS_IDLE;
 
                EnumStatus = ENUM_STATUS_IDLE;

                Error = 1;
            }
        }break;

        case ENUM_STATUS_OUT_STATUS:
        {
            EnumStatus = ENUM_STATUS_WAIT_OUT_STATUS_END;

            usb_selecet_endpoint(ENDPOINT_0);
            usb_Host_Endpoint0_SET_TxPktRdy_StatusPkt();
        }break;

        case ENUM_STATUS_WAIT_OUT_STATUS_END:
        {
            if (wait_tx_request_end_status == RECEIVE_STATUS_SUCCEED)
            {
                wait_tx_request_end_status = SEND_STATUS_IDLE;

                EnumStatus = ENUM_STATUS_IDLE;

                *desc_Param.descriptor_Ready = DEVICE_DESC_CHECK;
            }
            else if (wait_tx_request_end_status == RECEIVE_STATUS_FAIL)
            {
                wait_tx_request_end_status = SEND_STATUS_IDLE;

                EnumStatus = ENUM_STATUS_IDLE;

                Error = 1;
            }
        }break;

        default: break;
    }

    return Error;
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

    bool lb_Continue_t_IN = false;

    usb_selecet_endpoint(ENDPOINT_0);

    switch (ConnectStatus)
    {
        /* tx request get desc dvc end */
        case DEVICE_WAIT_TX_REQUEST_GET_DESC_DVC_END:
        {
            usb_Host_Endpoint0_Clr_SetupPkt();

            /* Device no response */
            if (usb_Host_Endpoint0_GET_ERROR())
            {
                usb_Host_Endpoint0_Clr_ERROR();
                wait_tx_request_end_status = SEND_STATUS_FAIL;
            }
            else
            {
                wait_tx_request_end_status = SEND_STATUS_SUCCEED;
            }
        }break;
        
        /* rx desc dvc end */
        case DEVICE_WAIT_RX_DESC_DVC_RESPONSE_END:
        {
            usb_Host_Endpoint0_Clr_ReqPkt();

            if (usb_Host_Endpoint0_GET_RxPktRdy())
            {
                lu8_RxCount = usb_Endpoint0_get_RxCount();

                usb_read_fifo(ENDPOINT_0, (uint8_t *)&device_descriptor_Dvc, lu8_RxCount);

                wait_rx_request_end_status = RECEIVE_STATUS_SUCCEED;
            }
            else if (usb_Host_Endpoint0_GET_ERROR())
            {
                usb_Host_Endpoint0_Clr_ERROR();
                wait_rx_request_end_status = RECEIVE_STATUS_FAIL;
            }
            else if (usb_Host_Endpoint0_GET_NAKtimeout())
            {
                usb_Host_Endpoint0_Clr_NAKtimeout();

                wait_rx_request_end_status = RECEIVE_STATUS_NAK_LIMIT;
            }
        }break;

        /* set address/config end */
        case DEVICE_WAIT_SEND_END:
        {
            usb_Host_Endpoint0_Clr_SetupPkt();

            /* Device no response */
            if (usb_Host_Endpoint0_GET_ERROR())
            {
                usb_Host_Endpoint0_Clr_ERROR();
                wait_tx_request_end_status = RECEIVE_STATUS_FAIL;
            }
            else if (usb_Host_Endpoint0_GET_NAKtimeout())
            {
                usb_Host_Endpoint0_Clr_NAKtimeout();

                wait_tx_request_end_status = RECEIVE_STATUS_NAK_LIMIT;
            }
            else
            {
                wait_tx_request_end_status = RECEIVE_STATUS_SUCCEED;
            }
        }break;

        /* rx status packet */
        case DEVICE_RX_STATUS_PACKET:
        {
            usb_Host_Endpoint0_Clr_ReqPkt();
            usb_Host_Endpoint0_Clr_StatusPkt();

            if (usb_Host_Endpoint0_GET_RxPktRdy())
            {
                wait_rx_request_end_status = RECEIVE_STATUS_SUCCEED;
            }
            else if (usb_Host_Endpoint0_GET_ERROR())
            {
                usb_Host_Endpoint0_Clr_ERROR();
                wait_rx_request_end_status = RECEIVE_STATUS_FAIL;
            }
            else if (usb_Host_Endpoint0_GET_NAKtimeout())
            {
                usb_Host_Endpoint0_Clr_NAKtimeout();

                wait_rx_request_end_status = RECEIVE_STATUS_NAK_LIMIT;
            }
        }break;

        default:break;
    }

    switch (EnumStatus)
    {
        /* tx request get desc dvc end */
        case ENUM_STATUS_WAIT_REQUEST_DESC_END:
        {
            usb_Host_Endpoint0_Clr_SetupPkt();

            /* Device no response */
            if (usb_Host_Endpoint0_GET_ERROR())
            {
                usb_Host_Endpoint0_Clr_ERROR();
                wait_tx_request_end_status = SEND_STATUS_FAIL;
            }
            else if (usb_Host_Endpoint0_GET_NAKtimeout())
            {
                usb_Host_Endpoint0_Clr_NAKtimeout();
                wait_tx_request_end_status = SEND_STATUS_FAIL;
            }
            else
            {
                wait_tx_request_end_status = SEND_STATUS_SUCCEED;
            }
        }break;

        case ENUM_STATUS_WAIT_RX_REPLY_DESC_END:
        {
            if (usb_Host_Endpoint0_GET_RxPktRdy())
            {
                lu8_RxCount = usb_Endpoint0_get_RxCount();

                usb_read_fifo(ENDPOINT_0, desc_Param.descriptor_point, lu8_RxCount);

                desc_Param.descriptor_count += lu8_RxCount;

                /* Receive unknown length */
                if (desc_Param.descriptor_length == 0)
                {
                    desc_Param.descriptor_length = desc_Param.descriptor_point[0];
                }

                /* END */
                if (desc_Param.descriptor_count >= desc_Param.descriptor_length)
                {
                    usb_Host_Endpoint0_Clr_ReqPkt();
                    wait_rx_request_end_status = RECEIVE_STATUS_SUCCEED;
                }
                else
                {
                    lb_Continue_t_IN = true;
                    desc_Param.descriptor_point += lu8_RxCount;
                }
            }
            else if (usb_Host_Endpoint0_GET_ERROR())
            {
                usb_Host_Endpoint0_Clr_ERROR();
                usb_Host_Endpoint0_Clr_ReqPkt();
                wait_rx_request_end_status = RECEIVE_STATUS_FAIL;
            }
            else if (usb_Host_Endpoint0_GET_NAKtimeout())
            {
                usb_Host_Endpoint0_Clr_NAKtimeout();

                usb_Host_Endpoint0_Clr_ReqPkt();
                wait_rx_request_end_status = RECEIVE_STATUS_FAIL;
            }
        }break;

        case ENUM_STATUS_WAIT_OUT_STATUS_END:
        {
            usb_Host_Endpoint0_Clr_StatusPkt();

            /* Device no response */
            if (usb_Host_Endpoint0_GET_ERROR())
            {
                usb_Host_Endpoint0_Clr_ERROR();
                wait_tx_request_end_status = SEND_STATUS_FAIL;
            }
            else if (usb_Host_Endpoint0_GET_NAKtimeout())
            {
                usb_Host_Endpoint0_Clr_NAKtimeout();
                wait_tx_request_end_status = SEND_STATUS_FAIL;
            }
            else
            {
                wait_tx_request_end_status = SEND_STATUS_SUCCEED;
            }
        }break;
    }

    /* clear TxPktRdy/RxPktRdy */
    usb_Endpoint0_FlushFIFO();

    /* Receive incomplete, continue to send IN packet */
    if (lb_Continue_t_IN)
        usb_Host_Endpoint0_SET_ReqPkt();
}

/*********************************************************************
 * @fn      Endpoints_IRQHandler
 *
 * @brief   ALL Endpoint Interrupt Request handler
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

    /* endpoint 1 ~ 5 Rx/Tx Interrupt handler */
    if (lu8_RxStatus | lu8_TxStatus) 
    {
        if (Endpoints_Handler != NULL) 
        {
            Endpoints_Handler(lu8_RxStatus, lu8_TxStatus);
        }
    }

    usb_selecet_endpoint((enum_Endpoint_t)lu8_EndpointBackup);
}

/*********************************************************************
 * @fn      USB_Status_IRQHandler
 *
 * @brief   USB status Interrupt Request handler
 */
static void USB_Status_IRQHandler(void)
{
    volatile uint8_t lu8_EndpointBackup;
    volatile uint8_t lu8_USBStatus;

    /* save backup endpoint */
    lu8_EndpointBackup = usb_get_endpoint();

    lu8_USBStatus = usb_get_USBStatus();

    /* USB Bus connect signal */
    if (lu8_USBStatus & USB_INT_STATUS_CONN)
    {
        ConnectStatus = DEVICE_CONNECT;

        if (USBH_Connect_Handler != NULL) 
        {
            USBH_Connect_Handler();
        }
    }
    /* USB Bus disconnect signal */
    if (lu8_USBStatus & USB_INT_STATUS_DISCON)
    {
        ConnectStatus = DEVICE_DISCONNECT;

        if (USBH_Disconnect_Handler != NULL)
        {
            USBH_Disconnect_Handler();
        }
    }

    /* USB Bus Resume signal */
    if (lu8_USBStatus & USB_INT_STATUS_RESUME)
    {
        if (USBH_Resume_Handler != NULL) 
        {
            USBH_Resume_Handler();
        }
    }

    /* Restore backup endpoint */
    usb_selecet_endpoint((enum_Endpoint_t)lu8_EndpointBackup);
}

#define USB_IRQHandler usbotg_irq

/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB interrupt Request handler
 */
void USB_IRQHandler(void) 
{
    Endpoints_IRQHandler();

    USB_Status_IRQHandler();
}
