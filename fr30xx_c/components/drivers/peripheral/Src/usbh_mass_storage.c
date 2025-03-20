/*
  ******************************************************************************
  * @file    usbh_mass_storage.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   This file provides the high layer firmware functions to manage the 
  *          Host Mass Storage Device.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  * 
  ******************************************************************************
*/
#include "fr30xx.h"

/* get max lun request */
uint8_t class_request_get_max_lun[] = {0xA1, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

/* ready flag */
static uint8_t class_max_lum_Ready  = CLASS_DATA_NONE;
static uint8_t class_scsi_cmd_Ready = CLASS_DATA_NONE;

static uint8_t mass_storage_max_lun;

/* communication status */
static uint32_t StorageClassCommStatus = CLASS_COMM_IDLE;
static uint32_t SCSIStatus = SCSI_STATUS_IDLE;

/* wait request end status flag */
static uint8_t wait_scsi_packet_end_status = SCSI_PACKET_STATUS_IDLE;

/* Command Block  Wrapper */
/* Command status Wrapper */
usb_CBW_t CBW_Packet;
usb_CSW_t CSW_Packet;

/* private communication param */
CBW_Param_t CBW_Param;
CSW_Param_t CSW_Param;

/* mass storage decive information */
usb_inquiry_data_t inquiry_data;
usb_read_format_capacities_data_t read_format_capacities_data;
usb_read_capacity_data_t read_capacity_data;
usb_mode_sense6_data_t mode_sense6_data;

/* used endpoint */
static uint8_t Out_endpoint;
static uint8_t In_endpoint;

/* In/Out data param */
uint32_t RW_BlcokAddr;
uint16_t RW_Length;
uint8_t *RW_Buffer;

void usbh_msu_connect_handle(void)
{
    printf("connect\r\n");

    mass_storage_max_lun = 0;

    class_max_lum_Ready  = CLASS_DATA_NONE;
    class_scsi_cmd_Ready = CLASS_DATA_NONE;
    StorageClassCommStatus = CLASS_COMM_GET_MAX_LUN;
    SCSIStatus = SCSI_STATUS_IDLE;
}
void usbh_msu_disconnect_handle(void)
{
    printf("disconnect\r\n");

    class_max_lum_Ready  = CLASS_DATA_NONE;
    class_scsi_cmd_Ready = CLASS_DATA_NONE;
    StorageClassCommStatus = CLASS_COMM_GET_MAX_LUN;
    SCSIStatus = SCSI_STATUS_IDLE;
}
 
static uint8_t usbh_scsi_comm(void)
{
    uint8_t Error = 0;

    switch (SCSIStatus)
    {
        case SCSI_STATUS_SEND_CMD:
        {
            SCSIStatus = SCSI_STATUS_WAIT_SEND_CMD_END;

            wait_scsi_packet_end_status = SCSI_PACKET_STATUS_IDLE;

            usb_selecet_endpoint(ENDPOINT_1);
            usb_write_fifo(ENDPOINT_1, (uint8_t *)CBW_Param.CBW, CBW_Param.CBW_length);
            usb_Host_TxTargetEndpoint(Out_endpoint);
            usb_Endpoints_SET_TxPktRdy();
        }break;

        case SCSI_STATUS_WAIT_SEND_CMD_END:
        {
            if (wait_scsi_packet_end_status == SCSI_PACKET_STATUS_SUCCEED)
            {
                wait_scsi_packet_end_status = SCSI_PACKET_STATUS_IDLE;

                SCSIStatus = SCSI_STATUS_RECEIVE_DATA;
            }
            else if (wait_scsi_packet_end_status == SCSI_PACKET_STATUS_FAIL)
            {
                wait_scsi_packet_end_status = SCSI_PACKET_STATUS_IDLE;

                SCSIStatus = SCSI_STATUS_IDLE;

                Error = 1;
            }
        }break;

        case SCSI_STATUS_RECEIVE_DATA:
        {
            /* No data stage */
            if (CBW_Param.reply_data == NULL)
            {
                SCSIStatus = SCSI_STATUS_RECEIVE_STATUS;
            }
            else
            {
                SCSIStatus = SCSI_STATUS_WAIT_RECEIVE_DATA_END;

                usb_selecet_endpoint(ENDPOINT_1);
                usb_Host_RxTargetEndpoint(In_endpoint);
                usb_Host_Endpoints_SET_RxReqPkt();
            }
        }break;

        case SCSI_STATUS_WAIT_RECEIVE_DATA_END:
        {
            if (wait_scsi_packet_end_status == SCSI_PACKET_STATUS_SUCCEED)
            {
                wait_scsi_packet_end_status = SCSI_PACKET_STATUS_IDLE;

                SCSIStatus = SCSI_STATUS_RECEIVE_STATUS;
            }
            else if (wait_scsi_packet_end_status == SCSI_PACKET_STATUS_FAIL)
            {
                wait_scsi_packet_end_status = SCSI_PACKET_STATUS_IDLE;
 
                SCSIStatus = SCSI_STATUS_IDLE;

                Error = 1;
            }
        }break;

        case SCSI_STATUS_RECEIVE_STATUS:
        {
            SCSIStatus = SCSI_STATUS_WAIT_RECEIVE_STATUS_END;

            CSW_Param.CSW              = &CSW_Packet;
            CSW_Param.CSW_reply_length = 13;
            CSW_Param.CSW_count        = 0;

            usb_selecet_endpoint(ENDPOINT_1);
            usb_Host_RxTargetEndpoint(In_endpoint);
            usb_Host_Endpoints_SET_RxReqPkt();
        }break;

        case SCSI_STATUS_WAIT_RECEIVE_STATUS_END:
        {
            if (wait_scsi_packet_end_status == RECEIVE_STATUS_SUCCEED)
            {
                wait_scsi_packet_end_status = SEND_STATUS_IDLE;

                SCSIStatus = SCSI_STATUS_IDLE;

                *CBW_Param.CBW_Ready = CLASS_DATA_CHECK;
            }
            else if (wait_scsi_packet_end_status == RECEIVE_STATUS_FAIL)
            {
                wait_scsi_packet_end_status = SEND_STATUS_IDLE;

                SCSIStatus = SCSI_STATUS_IDLE;

                Error = 2;
            }
        }break;

        default: break;
    }

    return Error;
}

void usbh_mass_storage_endpoints_handler(uint8_t RxStatus, uint8_t TxStatus)
{
    uint8_t lu8_RxCount;

    bool lb_Continue_t_IN = false;

    /* CBW OUT */
    if (TxStatus & ENDPOINT_1_MASK) 
    {
        usb_selecet_endpoint(ENDPOINT_1);

        switch (SCSIStatus)
        {
            case SCSI_STATUS_WAIT_SEND_CMD_END:
            {
                /* Device no response */
                if (usb_Host_Endpoints_GET_Tx_ERROR())
                {
                    usb_Host_Endpoints_Clr_Tx_ERROR();
                    wait_scsi_packet_end_status = SCSI_PACKET_STATUS_FAIL;
                }
                else if (usb_Host_Endpoints_GET_Tx_NAKtimeout())
                {
                    usb_Host_Endpoints_Clr_Tx_NAKtimeout();
                    wait_scsi_packet_end_status = SCSI_PACKET_STATUS_FAIL;
                }
                else if (usb_Host_Endpoints_GET_Tx_STALL())
                {
                    usb_Host_Endpoints_Clr_Tx_STALL();
                    wait_scsi_packet_end_status = SCSI_PACKET_STATUS_FAIL;
                }
                else
                {
                    wait_scsi_packet_end_status = SCSI_PACKET_STATUS_SUCCEED;
                }
            }break;
        
            default:break;
        }
        /* clear TxPktRdy */
        usb_Host_Endpoints_FlushTxFIFO();
    }

    /* Data IN */
    /* CSW  IN */
    if (RxStatus & ENDPOINT_1_MASK)
    {
        usb_selecet_endpoint(ENDPOINT_1);

        switch (SCSIStatus)
        {
            /* Data IN */
            case SCSI_STATUS_WAIT_RECEIVE_DATA_END:
            {
                if (usb_Host_Endpoints_GET_RxPktRdy())
                {
                    lu8_RxCount = usb_Endpoints_get_RxCount();

                    usb_read_fifo(ENDPOINT_1, CBW_Param.reply_data, lu8_RxCount);

                    CBW_Param.CBW_count += lu8_RxCount;

                    /* Receive unknown length */
                    if (CBW_Param.reply_length == 0)
                    {
                        if (CBW_Param.CBW->CBWCB[0] == SCSI_CMD_ReadForamtCapacity)
                        {
                            CBW_Param.reply_length = CBW_Param.CBW_count;
                        }
                    }

                    /* END */
                    if (CBW_Param.CBW_count >= CBW_Param.reply_length)
                    {
                        usb_Host_Endpoints_Clr_RxReqPkt();
                        wait_scsi_packet_end_status = RECEIVE_STATUS_SUCCEED;
                    }
                    else
                    {
                        lb_Continue_t_IN = true;
                        CBW_Param.reply_data += lu8_RxCount;
                    }
                }
                else if (usb_Host_Endpoints_GET_Rx_ERROR())
                {
                    usb_Host_Endpoints_Clr_Rx_ERROR();
                    usb_Host_Endpoints_Clr_RxReqPkt();
                    wait_scsi_packet_end_status = RECEIVE_STATUS_FAIL;
                }
                else if (usb_Host_Endpoints_GET_Rx_NAKtimeout())
                {
                    usb_Host_Endpoints_Clr_Rx_NAKtimeout();
                    usb_Host_Endpoints_Clr_RxReqPkt();
                    wait_scsi_packet_end_status = RECEIVE_STATUS_FAIL;
                }
                else if (usb_Host_Endpoints_GET_Rx_STALL())
                {
                    usb_Host_Endpoints_Clr_Rx_STALL();
                    usb_Host_Endpoints_Clr_RxReqPkt();
                    wait_scsi_packet_end_status = RECEIVE_STATUS_FAIL;
                }
            }break;

            /* CSW  IN */
            case SCSI_STATUS_WAIT_RECEIVE_STATUS_END:
            {
                if (usb_Host_Endpoints_GET_RxPktRdy())
                {
                    lu8_RxCount = usb_Endpoints_get_RxCount();

                    usb_read_fifo(ENDPOINT_1, (uint8_t *)CSW_Param.CSW, lu8_RxCount);

                    CSW_Param.CSW_count += lu8_RxCount;

                    /* END */
                    if (CSW_Param.CSW_count >= CSW_Param.CSW_reply_length)
                    {
                        usb_Host_Endpoints_Clr_RxReqPkt();
                        wait_scsi_packet_end_status = RECEIVE_STATUS_SUCCEED;
                    }
                    else
                    {
                        lb_Continue_t_IN = true;
                        CSW_Param.CSW += lu8_RxCount;
                    }
                }
                else if (usb_Host_Endpoints_GET_Rx_ERROR())
                {
                    usb_Host_Endpoints_Clr_Rx_ERROR();
                    usb_Host_Endpoints_Clr_RxReqPkt();
                    wait_scsi_packet_end_status = RECEIVE_STATUS_FAIL;
                }
                else if (usb_Host_Endpoints_GET_Rx_NAKtimeout())
                {
                    usb_Host_Endpoints_Clr_Rx_NAKtimeout();
                    usb_Host_Endpoints_Clr_RxReqPkt();
                    wait_scsi_packet_end_status = RECEIVE_STATUS_FAIL;
                }
                else if (usb_Host_Endpoints_GET_Rx_STALL())
                {
                    usb_Host_Endpoints_Clr_Rx_STALL();
                    usb_Host_Endpoints_Clr_RxReqPkt();
                    wait_scsi_packet_end_status = RECEIVE_STATUS_FAIL;
                }
            }break;
        }

        /* clear RxPktRdy */
        usb_Host_Endpoints_FlushRxFIFO();

        /* Receive incomplete, continue to send IN packet */
        if (lb_Continue_t_IN)
            usb_Host_Endpoints_SET_RxReqPkt();
    }
}

/*********************************************************************
 * @fn      usbh_msu_handler
 *
 * @brief   USB host mass storage unit handler.
 */
void usbh_msu_handler(void)
{
    switch (StorageClassCommStatus)
    {
        /* get max lun */
        case CLASS_COMM_GET_MAX_LUN:
        {
            if (class_max_lum_Ready == CLASS_DATA_NONE)
            {
                if (USB_Host_get_enum_status() == ENUM_STATUS_IDLE)
                {
                    USB_Host_set_enum_status(ENUM_STATUS_REQUEST_DESC);

                    class_request_get_max_lun[3] = Storage_Ifc->bInterfaceNumber;

                    desc_Param.descriptor_Ready   = &class_max_lum_Ready;
                    desc_Param.descriptor_request = (uint8_t *)class_request_get_max_lun;
                    desc_Param.descriptor_point   = (uint8_t *)&mass_storage_max_lun;
                    desc_Param.descriptor_length  = 1;
                    desc_Param.descriptor_count   = 0;
                }

                if (USB_Host_get_calss_desc())
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_GET_MAX_LUN_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
            else if (class_max_lum_Ready == CLASS_DATA_CHECK)
            {
                if (mass_storage_max_lun >= SUPPORT_MASS_STORAGE_MAX_LUN)
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_OVER_MAX_LUN);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
                else
                {
                    mass_storage_max_lun += 1;
                    class_max_lum_Ready = CLASS_DATA_READY;

                    printf("device max lun: %d \r\n", usbh_msu_get_max_lun());

                    StorageClassCommStatus = CLASS_COMM_SCSI_INQUIRY;

                    Out_endpoint = Storage_OUT_Endpoint->bEndpointAddress;
                    In_endpoint  = Storage_IN_Endpoint->bEndpointAddress & 0x7F;
                }
            }
        }break;

        /* inquiry */
        case CLASS_COMM_SCSI_INQUIRY:
        {
            if (class_scsi_cmd_Ready == CLASS_DATA_NONE)
            {
                if (SCSIStatus == SCSI_STATUS_IDLE)
                {
                    SCSIStatus = SCSI_STATUS_SEND_CMD;

                    CBW_Packet.dCBWTag++;
                    CBW_Packet.dCBWDataTransferLength = 36;
                    CBW_Packet.bmCBWFlags             = 0x80;
                    CBW_Packet.bCBWLUN                = 0;
                    CBW_Packet.bCBWCBLength           = 6;

                    usb_CDB_inquiry_t *CDB_inquiry = (usb_CDB_inquiry_t *)&CBW_Packet.CBWCB[0];
                    CDB_inquiry->OperationCode    = SCSI_CMD_Inquiry;
                    CDB_inquiry->EVPD             = 0;
                    CDB_inquiry->PageCode         = 0x00;
                    CDB_inquiry->RSV              = 0;
                    CDB_inquiry->AllocationLength = 0x24;
                    CDB_inquiry->Control          = 0;

                    CBW_Param.CBW_Ready    = &class_scsi_cmd_Ready;
                    CBW_Param.CBW          = &CBW_Packet;
                    CBW_Param.reply_data   = (uint8_t *)&inquiry_data;
                    CBW_Param.reply_length = 36;
                    CBW_Param.CBW_length   = 31;
                    CBW_Param.CBW_count    = 0;
                }

                if (usbh_scsi_comm())
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_INQURIY_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
            else if (class_scsi_cmd_Ready == CLASS_DATA_CHECK)
            {
                if ((CSW_Packet.dCSWSignature == CSW_SIGNATURE) &&   \
                    (CSW_Packet.dCSWTag       == CBW_Packet.dCBWTag) && \
                    (CSW_Packet.bmCSWStatus   == 0))
                {
                    printf("VenderInfo: %.8s\n", inquiry_data.VenderInfo);
                    printf("ProductInfo: %.16s\n", inquiry_data.ProductInfo);
                    printf("ProducetVerInfo: %.4s\n", inquiry_data.ProducetVerInfo);

                    StorageClassCommStatus = CLASS_COMM_SCSI_READ_FORMAT_CAPACITY;

                    class_scsi_cmd_Ready = CLASS_DATA_NONE;
                }
                else
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_INQURIY_CSW_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
        }break;

        /* read format capacity */
        case CLASS_COMM_SCSI_READ_FORMAT_CAPACITY:
        {
            if (class_scsi_cmd_Ready == CLASS_DATA_NONE)
            {
                if (SCSIStatus == SCSI_STATUS_IDLE)
                {
                    SCSIStatus = SCSI_STATUS_SEND_CMD;

                    CBW_Packet.dCBWTag++;
                    CBW_Packet.dCBWDataTransferLength = 60;
                    CBW_Packet.bmCBWFlags             = 0x80;
                    CBW_Packet.bCBWLUN                = 0;
                    CBW_Packet.bCBWCBLength           = 10;

                    usb_CDB_read_format_capacities_t *CDB_read_format_capacities = (usb_CDB_read_format_capacities_t *)&CBW_Packet.CBWCB[0];
                    CDB_read_format_capacities->OperationCode = SCSI_CMD_ReadForamtCapacity;
                    CDB_read_format_capacities->RSV[0] = 0;
                    CDB_read_format_capacities->RSV[1] = 0;
                    CDB_read_format_capacities->RSV[2] = 0;
                    CDB_read_format_capacities->RSV[3] = 0;
                    CDB_read_format_capacities->RSV[4] = 0;
                    CDB_read_format_capacities->RSV[5] = 0;
                    CDB_read_format_capacities->AllocationLength_MSB = 0;
                    CDB_read_format_capacities->AllocationLength_LSB = 60;
                    CDB_read_format_capacities->Control = 0;

                    CBW_Param.CBW_Ready  = &class_scsi_cmd_Ready;
                    CBW_Param.CBW        = &CBW_Packet;
                    CBW_Param.reply_data = (uint8_t *)&read_format_capacities_data;
                    CBW_Param.reply_length = 0;
                    CBW_Param.CBW_length = 31;
                    CBW_Param.CBW_count  = 0;
                }

                if (usbh_scsi_comm())
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READ_FORMAT_CAPACITIES_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
            else if (class_scsi_cmd_Ready == CLASS_DATA_CHECK)
            {
                if ((CSW_Packet.dCSWSignature == CSW_SIGNATURE) &&   \
                    (CSW_Packet.dCSWTag       == CBW_Packet.dCBWTag) && \
                    (CSW_Packet.bmCSWStatus   == 0))
                {
                    for (int i = 0; i < read_format_capacities_data.CapacityListLength/8; i++)
                    {
                        uint32_t Blocks, BlockLength;

                        Blocks  = read_format_capacities_data.capacity_descriptor[i].Number_of_Blocks[0] << 24;
                        Blocks |= read_format_capacities_data.capacity_descriptor[i].Number_of_Blocks[1] << 16;
                        Blocks |= read_format_capacities_data.capacity_descriptor[i].Number_of_Blocks[2] << 8;
                        Blocks |= read_format_capacities_data.capacity_descriptor[i].Number_of_Blocks[3];

                        BlockLength  = read_format_capacities_data.capacity_descriptor[i].Blcok_Length[0] << 16;
                        BlockLength |= read_format_capacities_data.capacity_descriptor[i].Blcok_Length[1] << 8;
                        BlockLength |= read_format_capacities_data.capacity_descriptor[i].Blcok_Length[2];

                        printf("Capacity List: %d \n", i);
                        printf("Blocks: %d \n", Blocks);
                        printf("BlockLength: %d \n", BlockLength);
                        if (read_format_capacities_data.capacity_descriptor[i].DescripterCode == 1)
                            printf("Unformatted Media\n");
                        else
                            printf("formatted Media\n");

                        StorageClassCommStatus = CLASS_COMM_SCSI_READ_CAPACITY;

                        class_scsi_cmd_Ready = CLASS_DATA_NONE;
                    }
                }
                else
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READ_FORMAT_CAPACITIESCSW_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
        }break;

        /* read capacity */
        case CLASS_COMM_SCSI_READ_CAPACITY:
        {
            if (class_scsi_cmd_Ready == CLASS_DATA_NONE)
            {
                if (SCSIStatus == SCSI_STATUS_IDLE)
                {
                    SCSIStatus = SCSI_STATUS_SEND_CMD;

                    CBW_Packet.dCBWTag++;
                    CBW_Packet.dCBWDataTransferLength = 8;
                    CBW_Packet.bmCBWFlags             = 0x80;
                    CBW_Packet.bCBWLUN                = 0;
                    CBW_Packet.bCBWCBLength           = 10;

                    usb_CDB_read_capacities_t *CDB_read_capacities = (usb_CDB_read_capacities_t *)&CBW_Packet.CBWCB[0];
                    CDB_read_capacities->OperationCode = SCSI_CMD_ReadCapacity;
                    CDB_read_capacities->RelAdr              = 0;
                    CDB_read_capacities->LogicalBlockAddress = 0;
                    CDB_read_capacities->RSV[0]              = 0;
                    CDB_read_capacities->RSV[1]              = 0;
                    CDB_read_capacities->Control             = 0;

                    CBW_Param.CBW_Ready  = &class_scsi_cmd_Ready;
                    CBW_Param.CBW        = &CBW_Packet;
                    CBW_Param.reply_data = (uint8_t *)&read_capacity_data;
                    CBW_Param.reply_length = 8;
                    CBW_Param.CBW_length = 31;
                    CBW_Param.CBW_count  = 0;
                }

                if (usbh_scsi_comm())
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READ_CAPACITIES_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
            else if (class_scsi_cmd_Ready == CLASS_DATA_CHECK)
            {
                if ((CSW_Packet.dCSWSignature == CSW_SIGNATURE) && \
                    (CSW_Packet.dCSWTag       == CBW_Packet.dCBWTag) && \
                    (CSW_Packet.bmCSWStatus   == 0))
                {
                    read_capacity_data.u32_Last_LogicalBlockAddress  = read_capacity_data.u8_Last_LogicalBlockAddress[0] << 24;
                    read_capacity_data.u32_Last_LogicalBlockAddress |= read_capacity_data.u8_Last_LogicalBlockAddress[1] << 16;
                    read_capacity_data.u32_Last_LogicalBlockAddress |= read_capacity_data.u8_Last_LogicalBlockAddress[2] <<  8;
                    read_capacity_data.u32_Last_LogicalBlockAddress |= read_capacity_data.u8_Last_LogicalBlockAddress[3];

                    read_capacity_data.u32_Blcok_Length  = read_capacity_data.u8_Blcok_Length[0] << 24;
                    read_capacity_data.u32_Blcok_Length |= read_capacity_data.u8_Blcok_Length[1] << 16;
                    read_capacity_data.u32_Blcok_Length |= read_capacity_data.u8_Blcok_Length[2] <<  8;
                    read_capacity_data.u32_Blcok_Length |= read_capacity_data.u8_Blcok_Length[3];

                    if (read_capacity_data.u32_Blcok_Length == 512)
                    {
                        read_capacity_data.Device_Capacity = (read_capacity_data.u32_Last_LogicalBlockAddress + 1)/2;

                        printf("Read Capacity\n");
                        printf("Capacity: %d kByte\n", read_capacity_data.Device_Capacity);
                        printf("Capacity: %d MByte\n", read_capacity_data.Device_Capacity / 1024);
                        printf("Capacity: %d GByte\n", (read_capacity_data.Device_Capacity / 1024) / 1024);

                        StorageClassCommStatus = CLASS_COMM_SCSI_MODE_SENSE6;

                        class_scsi_cmd_Ready = CLASS_DATA_NONE;
                    }
                    else
                    {
                        USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_BLOCK_LENGTH_ERR);
                        USB_Host_set_connect_status(DEVICE_DISCONNECT);
                    }
                }
                else
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READ_FORMAT_CAPACITIESCSW_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
        }break;

        /* SENSE6 */
        case CLASS_COMM_SCSI_MODE_SENSE6:
        {
            if (class_scsi_cmd_Ready == CLASS_DATA_NONE)
            {
                if (SCSIStatus == SCSI_STATUS_IDLE)
                {
                    SCSIStatus = SCSI_STATUS_SEND_CMD;

                    CBW_Packet.dCBWTag++;
                    CBW_Packet.dCBWDataTransferLength = 64;
                    CBW_Packet.bmCBWFlags             = 0x80;
                    CBW_Packet.bCBWLUN                = 0;
                    CBW_Packet.bCBWCBLength           = 6;

                    usb_CDB_mode_sense6_t *CDB_mode_sense6 = (usb_CDB_mode_sense6_t *)&CBW_Packet.CBWCB[0];
                    CDB_mode_sense6->OperationCode = SCSI_CMD_ModeSENSE6;
                    CDB_mode_sense6->DisableBlockDesc = 0;
                    CDB_mode_sense6->PageAndSubpageCode = 0x1C;
                    CDB_mode_sense6->AllocationLength_MSB = 0;
                    CDB_mode_sense6->AllocationLength_LSB = 64;
                    CDB_mode_sense6->Control              = 0;
                    CDB_mode_sense6->RSV[0] = 0;
                    CDB_mode_sense6->RSV[1] = 0;
                    CDB_mode_sense6->PAD[0] = 0;
                    CDB_mode_sense6->PAD[1] = 0;

                    CBW_Param.CBW_Ready  = &class_scsi_cmd_Ready;
                    CBW_Param.CBW        = &CBW_Packet;
                    CBW_Param.reply_data = (uint8_t *)&mode_sense6_data;
                    CBW_Param.reply_length = 4;
                    CBW_Param.CBW_length = 31;
                    CBW_Param.CBW_count  = 0;
                }

                if (usbh_scsi_comm())
                {
                    printf("SENSE6 CMD ERR\n");

                    StorageClassCommStatus = CLASS_COMM_SCSI_WAIT_READ_WRITE;

                    class_scsi_cmd_Ready = CLASS_DATA_NONE;
                }
            }
            else if (class_scsi_cmd_Ready == CLASS_DATA_CHECK)
            {
                if ((CSW_Packet.dCSWSignature == CSW_SIGNATURE) && \
                    (CSW_Packet.dCSWTag       == CBW_Packet.dCBWTag) && \
                    (CSW_Packet.bmCSWStatus   == 0))
                {
                    if (mode_sense6_data.WriteProtect)
                        printf("Write Protect enable\n");
                    else
                        printf("Write Protect disable\n");

                    StorageClassCommStatus = CLASS_COMM_SCSI_WAIT_READ_WRITE;

                    class_scsi_cmd_Ready = CLASS_DATA_NONE;
                }
                else
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_MODE_SENSE6CSW_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
        }break;

        /* dile wait */
        case CLASS_COMM_SCSI_WAIT_READ_WRITE:
        {

        }break;

        /* test unit ready */
        case CLASS_COMM_SCSI_TEST_UNIT_READY:
        {
            if (class_scsi_cmd_Ready == CLASS_DATA_NONE)
            {
                if (SCSIStatus == SCSI_STATUS_IDLE)
                {
                    SCSIStatus = SCSI_STATUS_SEND_CMD;

                    CBW_Packet.dCBWTag++;
                    CBW_Packet.dCBWDataTransferLength = 0;
                    CBW_Packet.bmCBWFlags             = 0;
                    CBW_Packet.bCBWLUN                = 0;
                    CBW_Packet.bCBWCBLength           = 6;

                    usb_CDB_test_unit_ready_t *CDB_test_unit_ready = (usb_CDB_test_unit_ready_t *)&CBW_Packet.CBWCB[0];
                    CDB_test_unit_ready->OperationCode = SCSI_CMD_TestUnitReady;
                    CDB_test_unit_ready->Control       = 0;
                    CDB_test_unit_ready->PAD[0]        = 0;
                    CDB_test_unit_ready->PAD[1]        = 0;

                    CBW_Param.CBW_Ready  = &class_scsi_cmd_Ready;
                    CBW_Param.CBW        = &CBW_Packet;
                    CBW_Param.reply_data = NULL;
                    CBW_Param.reply_length = 0;
                    CBW_Param.CBW_length = 31;
                    CBW_Param.CBW_count  = 0;
                }

                if (usbh_scsi_comm())
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READ_CAPACITIES_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
            else if (class_scsi_cmd_Ready == CLASS_DATA_CHECK)
            {
                if ((CSW_Packet.dCSWSignature == CSW_SIGNATURE) && \
                    (CSW_Packet.dCSWTag       == CBW_Packet.dCBWTag))
                {
                    StorageClassCommStatus = CLASS_COMM_SCSI_WAIT_READ_WRITE;

                    class_scsi_cmd_Ready = CLASS_DATA_NONE;
                }
                else
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READ_FORMAT_CAPACITIESCSW_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
        }break;

        /* read */
        case CLASS_COMM_SCSI_READ:
        {
            if (class_scsi_cmd_Ready == CLASS_DATA_NONE)
            {
                if (SCSIStatus == SCSI_STATUS_IDLE)
                {
                    SCSIStatus = SCSI_STATUS_SEND_CMD;

                    CBW_Packet.dCBWTag++;
                    CBW_Packet.dCBWDataTransferLength = 512;
                    CBW_Packet.bmCBWFlags             = 0x80;
                    CBW_Packet.bCBWLUN                = 0;
                    CBW_Packet.bCBWCBLength           = 10;

                    usb_CDB_read_t *CDB_read = (usb_CDB_read_t *)&CBW_Packet.CBWCB[0];
                    CDB_read->OperationCode = SCSI_CMD_Read10;
                    CDB_read->RelAdr        = 0;
                    CDB_read->LogicalBlockAddress[0] = (RW_BlcokAddr >> 24)& 0xFF;
                    CDB_read->LogicalBlockAddress[1] = (RW_BlcokAddr >> 16)& 0xFF;
                    CDB_read->LogicalBlockAddress[2] = (RW_BlcokAddr >> 8) & 0xFF;
                    CDB_read->LogicalBlockAddress[3] =  RW_BlcokAddr & 0xFF;
                    CDB_read->TransferLength[0] = (RW_Length >> 8) & 0xFF;
                    CDB_read->TransferLength[1] =  RW_Length & 0xFF;
                    CDB_read->RSV0    = 0;
                    CDB_read->RSV1[0] = 0;
                    CDB_read->RSV1[1] = 0;
                    CDB_read->RSV1[2] = 0;
                    CDB_read->PAD     = 0;

                    CBW_Param.CBW_Ready  = &class_scsi_cmd_Ready;
                    CBW_Param.CBW        = &CBW_Packet;
                    CBW_Param.reply_data = RW_Buffer;
                    CBW_Param.reply_length = 512;
                    CBW_Param.CBW_length = 31;
                    CBW_Param.CBW_count  = 0;
                }

                if (usbh_scsi_comm())
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READ_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
            else if (class_scsi_cmd_Ready == CLASS_DATA_CHECK)
            {
                if ((CSW_Packet.dCSWSignature == CSW_SIGNATURE) && \
                    (CSW_Packet.dCSWTag       == CBW_Packet.dCBWTag) && \
                    (CSW_Packet.bmCSWStatus   == 0))
                {
                    StorageClassCommStatus = CLASS_COMM_SCSI_WAIT_READ_WRITE;

                    class_scsi_cmd_Ready = CLASS_DATA_NONE;
                }
                else
                {
                    USB_Host_set_error_code(CLASS_ERROR_CODE_SCSI_READCSW_ERR);
                    USB_Host_set_connect_status(DEVICE_DISCONNECT);
                }
            }
        }break;

        /* write */
        case CLASS_COMM_SCSI_WRITE:
        {

        }break;

        default:break;
    }
 
}

/*********************************************************************
 * @fn      usbh_msu_get_max_lun
 *
 * @brief   USB host get mass storage max lun.
 */
uint32_t usbh_msu_get_max_lun(void)
{
    return mass_storage_max_lun;
}

/*********************************************************************
 * @fn      usbh_msu_get_capacity
 *
 * @brief   USB host get mass storage capacity. unit Kbyte.
 */
uint32_t usbh_msu_get_capacity(void)
{
    return read_capacity_data.Device_Capacity;
}

/*********************************************************************
 * @fn      usbh_msu_read_block
 *
 * @brief   USB host read block.
 * 
 * @param   LogicalBlockAddress: Block start address.
 * @param   TransferBlocks: raed Blocks number.
 * @param   Buffer: read buffer.
 */
int32_t usbh_msu_read_block(uint32_t LogicalBlockAddress, uint16_t TransferBlocks, uint8_t *Buffer)
{
    if (StorageClassCommStatus != CLASS_COMM_SCSI_WAIT_READ_WRITE)
        return 1;    // busy

    RW_BlcokAddr = LogicalBlockAddress;
    RW_Length    = TransferBlocks;
    RW_Buffer    = Buffer;

    StorageClassCommStatus = CLASS_COMM_SCSI_READ;

    return 0;
}

/*********************************************************************
 * @fn      usbh_msu_get_busy_status
 *
 * @brief   get USB host busy status.
 */
int32_t usbh_msu_get_busy_status(void)
{
    if (StorageClassCommStatus != CLASS_COMM_SCSI_WAIT_READ_WRITE)
        return 1;    // busy)
    else
        return 0;
}

/*********************************************************************
 * @fn      usbh_msu_test_unit_ready
 *
 * @brief   get storage device status.
 * 
 * @return  0: ready.
 *          1: busy.
 */
int32_t usbh_msu_test_unit_ready(void)
{
    if (StorageClassCommStatus != CLASS_COMM_SCSI_WAIT_READ_WRITE)
        return 1;    // busy

    StorageClassCommStatus = CLASS_COMM_SCSI_TEST_UNIT_READY;

    while(StorageClassCommStatus == CLASS_COMM_SCSI_TEST_UNIT_READY);

    return CSW_Packet.bmCSWStatus;
}

/*********************************************************************
 * @fn      usbh_mass_storage_init
 *
 * @brief   USB host mass storage parameter initialization 
 *
 * @param   None.
 * @return  None.
 */
void usbh_mass_storage_init(void)
{
    USBH_Connect_Handler    = usbh_msu_connect_handle;
    USBH_Disconnect_Handler = usbh_msu_disconnect_handle;

    Endpoints_Handler = usbh_mass_storage_endpoints_handler;

    USBH_Class_Handler = usbh_msu_handler;

    /* CBW initial value */
    CBW_Packet.dCBWSignature = 0x43425355;
    CBW_Packet.dCBWTag       = 0x10105050;

    /* Init used endpoint */
    usb_selecet_endpoint(ENDPOINT_1);
    usb_Host_TxEndpointType(HOST_ENDP_TYPE_BULK);
    usb_Host_RxEndpointType(HOST_ENDP_TYPE_BULK);
    usb_Host_TxNAKLimit(0xFF);
    usb_Host_RxNAKLimit(0xFF);
}
