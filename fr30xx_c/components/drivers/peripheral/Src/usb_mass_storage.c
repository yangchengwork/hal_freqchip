/*
  ******************************************************************************
  * @file    usb_mass_storage.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   This file provides the high layer firmware functions to manage the 
  *          Mass Storage Device.
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
  *        usb_mass_storage_init();
  *
  *        // Wait for other initialization of the MCU
  *
  *        while(1)
  *        {
  *        }
  *    }
  ******************************************************************************
*/
#include "fr30xx.h"

#define DISK_BLOCKS    (100)

#define DISK_PAGE_SIZE    (512)    /* one page 512 byte fixed */
#define RAM_SIMULATE_DISK_CAPACITY    (DISK_PAGE_SIZE * DISK_BLOCKS)

uint8_t Disk[RAM_SIMULATE_DISK_CAPACITY];

/* USB Standard Device Descriptor */
const uint8_t USB_MassStorage_DeviceDesc[] =
{
    0x12,    /* bLength */
    0x01,    /* bDescriptorType */
    0x00,    /* bcdUSB */
    0x02,
    0x00,    /* bDeviceClass: Class info in ifc Descriptors */
    0x00,    /* bDeviceSubClass */
    0x00,    /* bDeviceProtocol */
    0x40,    /* bMaxPacketSize */
    0xA9,    /* idVendor */
    0x13,    /* idVendor */
    0xCC,    /* idProduct */
    0x10,    /* idProduct */
    0x00,    /* bcdDevice rel. 2.00 */
    0x20,
    0x01,    /* Index of manufacturer string */
    0x02,    /* Index of product string */
    0x03,    /* Index of serial number string */
    0x01,    /* bNumConfigurations */
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_MassStorage_ConfigurationDesc[] =
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
    0x96,    /* bMaxPower */           

    /* Interface Descriptor */
    0x09,    /* bLength */           
    0x04,    /* bDescriptorType */   
    0x00,    /* bInterfaceNumber */  
    0x00,    /* bAlternateSetting */ 
    0x02,    /* bNumEndpoints */     
    0x08,    /* bInterfaceClass: Mass Storage */   
    0x06,    /* bInterfaceSubClass SCSI Transparent Conmmand Set */
    0x50,    /* bInterfaceProtocol Bulk-Only Transport */
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
const uint8_t USB_MassStorage_ManufactureDesc[] =
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
const uint8_t USB_MassStorage_ProductDesc[] =
{
    0x2C,         /* bLength */
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
    'D', 0x00,    
    'a', 0x00,    
    't', 0x00,    
    'a', 0x00,    
    'T', 0x00,    
    'r', 0x00,    
    'a', 0x00,    
    'v', 0x00,    
    'e', 0x00,    
    'l', 0x00,    
    'e', 0x00,    
    'r', 0x00,    
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_MassStorage_SerialNumberDesc[] =
{
    0x1E,         /* bLength */
    0x03,         /* bDescriptorType */
    '2', 0x00,    /* BString */
    '0', 0x00,
    '2', 0x00,
    '3', 0x00,
    '-', 0x00,
    '2', 0x00,
    '0', 0x00,
    '0', 0x00,
    '2', 0x00,
    '-', 0x00,
    'C', 0x00,
    '1', 0x00,
    '3', 0x00,
    '5', 0x00,
};

/* USB Standard Configuration Descriptor */
const uint8_t USB_MassStorage_LanuageIDDesc[] =
{
    0x04,    /* bLength */
    0x03,    /* bDescriptorType */
    0x09,    /* BString */
    0x04,
};

const uint8_t SCSI_Inquiry_Response[] =
{
    0x00,     /* Device Type */ 
    0x80,     /* Removable */
    0x00,     /* Version */
    0x01,     /* Response Data Format */
    0x1F,     /* Additional Length */
    0x00,0x00,0x00,
    'F','r','e','q','c','h','i','p',
    '2','0','1','3','-','4','5','3','2','-','A','D','E','F','G','H',
    '1','.','0','0',
};

uint8_t SCSI_ReadForamtCapacity_Response[] =
{
    0x00,0x00,0x00,
    0x08,                   /* list length */
    0x00,0x00,0x01,0x90,    /* Blocks */ 
    0x02,
    0x00,0x02,0x00,         /* Page size */
};

uint8_t SCSI_ReadCapacity_Response[] =
{
    0x00,0x00,0x01,0x8F,    /* Blocks */ 
    0x00,0x00,0x02,0x00,    /* Page size */
};

const uint8_t SCSI_SENSE6_Response[] =
{
    0x03,0x00,0x00,0x00,
};

static uint8_t USB_MessageBuffer[10];

/*********************************************************************
 * @fn      usb_MassStorage_ClassRequest_Handler
 *
 * @brief   Mass Storage Class Request Handler
 */
static void usb_MassStorage_ClassRequest_Handler(usb_StandardRequest_t* pStandardRequest, usb_ReturnData_t* pReturnData)
{
    switch (pStandardRequest->bRequest)
    {
        case BULK_ONLY_MASS_STORAGE_RESET:
        {
            usb_Endpoint0_DataEnd();
            usb_Endpoint0_SET_TxPktRdy();
        }break;

        case GET_MAX_LUN:
        {
            USB_MessageBuffer[0] = FREQCHIP_MASS_STORAGE_MAX_LUN - 1;

            pReturnData->DataBuffer = USB_MessageBuffer;
            pReturnData->DataLength = 1;
        }break;

        default: break; 
    }
}

/*********************************************************************
 * @fn      usb_MassStorage_SendCSW
 *
 * @brief   SCSI Send CSW
 */
static void usb_MassStorage_SendCSW(uint32_t fu32_CBWTag)
{
    #define CBW_LEN    (13)

    uint8_t USB_CSWBuffer[CBW_LEN];

    usb_CSW_t *CSW;

    CSW = (usb_CSW_t *)USB_CSWBuffer;

    CSW->dCSWSignature = CSW_SIGNATURE;
    CSW->dCSWTag = fu32_CBWTag;
    CSW->dCSWDataResidue = 0;
    CSW->bmCSWStatus = 0x00;

    if (usb_Endpoints_GET_TxPktRdy() == false) 
    {
        usb_write_fifo(ENDPOINT_1, (uint8_t *)USB_CSWBuffer, CBW_LEN);

        usb_Endpoints_SET_TxPktRdy();
    }
}

/*********************************************************************
 * @fn      Endpoint1_Handler
 *
 * @brief   endpoint1 RX TX Handler
 */
static void Endpoint1_Handler(uint8_t RxStatus, uint8_t TxStatus)
{
    static uint8_t USB_CBWBuffer[64];
    
    static uint32_t lu32_WriteStatus = 0;
    static uint8_t *lu32_WriteDiskPoint;
    static uint32_t lu32_WriteCnt;

    static uint32_t lu32_ReadStatus = 0;
    static uint8_t *lu32_ReadDiskPoint;
    static uint32_t lu32_ReadCnt;

    static usb_CBW_t *CBW;

    uint32_t lu32_BlockAddr;
    uint16_t lu16_BlockCnt;

    uint8_t lu8_RxCount;

    if (RxStatus & ENDPOINT_1_MASK) 
    {
        usb_selecet_endpoint(ENDPOINT_1);

        if (lu32_WriteStatus)
        {
            usb_read_fifo(ENDPOINT_1, lu32_WriteDiskPoint, 64);
            usb_Endpoints_FlushRxFIFO();

            lu32_WriteDiskPoint += 64;
            lu32_WriteCnt -= 1;

            if (lu32_WriteCnt == 0)
            {
                usb_MassStorage_SendCSW(CBW->dCBWTag);
                lu32_WriteStatus = false;
            }
        }
        else
        {
            lu8_RxCount = usb_Endpoints_get_RxCount();

            if (lu8_RxCount == 0)
                return;

            usb_read_fifo(ENDPOINT_1, USB_CBWBuffer, lu8_RxCount);

            usb_Endpoints_FlushRxFIFO();

            CBW = (usb_CBW_t *)USB_CBWBuffer;

            if (CBW->dCBWSignature == CBW_SIGNATURE)
            {
                switch (CBW->CBWCB[0])
                {
                    case SCSI_CMD_Inquiry:
                    {
                        if (usb_Endpoints_GET_TxPktRdy() == false) 
                        {
                            usb_write_fifo(ENDPOINT_1, (uint8_t *)SCSI_Inquiry_Response, sizeof(SCSI_Inquiry_Response));

                            usb_Endpoints_SET_TxPktRdy();
                        }

                        while(usb_Endpoints_GET_TxPktRdy());

                        usb_MassStorage_SendCSW(CBW->dCBWTag);
                    }break;

                    case SCSI_CMD_ReadForamtCapacity:
                    {
                        SCSI_ReadForamtCapacity_Response[4] = (DISK_BLOCKS >> 24) & 0xFF;
                        SCSI_ReadForamtCapacity_Response[5] = (DISK_BLOCKS >> 16) & 0xFF;
                        SCSI_ReadForamtCapacity_Response[6] = (DISK_BLOCKS >> 8) & 0xFF;
                        SCSI_ReadForamtCapacity_Response[7] = DISK_BLOCKS & 0xFF;

                        if (usb_Endpoints_GET_TxPktRdy() == false) 
                        {
                            usb_write_fifo(ENDPOINT_1, (uint8_t *)SCSI_ReadForamtCapacity_Response, 12);
                            
                            usb_Endpoints_SET_TxPktRdy();
                        }
                        while(usb_Endpoints_GET_TxPktRdy());

                        usb_MassStorage_SendCSW(CBW->dCBWTag);
                    }break;

                    case SCSI_CMD_ReadCapacity:
                    {
                        uint32_t lu32_Blocks = DISK_BLOCKS - 1;

                        SCSI_ReadCapacity_Response[0] = (lu32_Blocks >> 24) & 0xFF;
                        SCSI_ReadCapacity_Response[1] = (lu32_Blocks >> 16) & 0xFF;
                        SCSI_ReadCapacity_Response[2] = (lu32_Blocks >> 8) & 0xFF;
                        SCSI_ReadCapacity_Response[3] = lu32_Blocks & 0xFF;

                        if (usb_Endpoints_GET_TxPktRdy() == false) 
                        {
                            usb_write_fifo(ENDPOINT_1, (uint8_t *)SCSI_ReadCapacity_Response, 8);
                            
                            usb_Endpoints_SET_TxPktRdy();
                        }
                        while(usb_Endpoints_GET_TxPktRdy());

                        usb_MassStorage_SendCSW(CBW->dCBWTag);
                    }break;

                    case SCSI_CMD_ModeSENSE6:
                    {
                        if (usb_Endpoints_GET_TxPktRdy() == false) 
                        {
                            usb_write_fifo(ENDPOINT_1, (uint8_t *)SCSI_SENSE6_Response, sizeof(SCSI_SENSE6_Response));
                            
                            usb_Endpoints_SET_TxPktRdy();
                        }
                        while(usb_Endpoints_GET_TxPktRdy());

                        usb_MassStorage_SendCSW(CBW->dCBWTag);
                    }break;

                    case SCSI_CMD_Read10:
                    {
                        lu32_ReadDiskPoint = Disk;

                        lu32_BlockAddr  = CBW->CBWCB[2] << 24;
                        lu32_BlockAddr |= CBW->CBWCB[3] << 16;
                        lu32_BlockAddr |= CBW->CBWCB[4] << 8;
                        lu32_BlockAddr |= CBW->CBWCB[5];

                        lu16_BlockCnt  = CBW->CBWCB[7] << 8;
                        lu16_BlockCnt |= CBW->CBWCB[8];

                        lu32_ReadDiskPoint += DISK_PAGE_SIZE * lu32_BlockAddr;

                        lu32_ReadCnt += (lu16_BlockCnt * DISK_PAGE_SIZE)/64;

                        lu32_ReadStatus = true;

                        while(usb_Endpoints_GET_TxPktRdy());
                        usb_write_fifo(ENDPOINT_1, lu32_ReadDiskPoint, 64);
                        usb_Endpoints_SET_TxPktRdy();

                        lu32_ReadDiskPoint += 64;
                        lu32_ReadCnt -= 1;

                        usb_TxInt_Enable(ENDPOINT_1);
                    }break;

                    case SCSI_CMD_Write10:
                    {
                        lu32_WriteDiskPoint = Disk;

                        lu32_BlockAddr  = CBW->CBWCB[2] << 24;
                        lu32_BlockAddr |= CBW->CBWCB[3] << 16;
                        lu32_BlockAddr |= CBW->CBWCB[4] << 8;
                        lu32_BlockAddr |= CBW->CBWCB[5];

                        lu16_BlockCnt  = CBW->CBWCB[7] << 8;
                        lu16_BlockCnt |= CBW->CBWCB[8];

                        lu32_WriteDiskPoint += DISK_PAGE_SIZE * lu32_BlockAddr;

                        lu32_WriteCnt = (lu16_BlockCnt*DISK_PAGE_SIZE)/64;

                        lu32_WriteStatus = true;
                    }break;

                    case SCSI_CMD_TestUnitReady:
                    case SCSI_CMD_Prevent:
                    {
                        usb_MassStorage_SendCSW(CBW->dCBWTag);
                    }break;

                    default:
                    {
                        usb_MassStorage_SendCSW(CBW->dCBWTag);
                    }break;
                }
            }
        }
        
    }

    if (TxStatus & ENDPOINT_1_MASK)
    {
        usb_selecet_endpoint(ENDPOINT_1);

        if (lu32_ReadStatus)
        {
            if (usb_Endpoints_GET_TxPktRdy() == false) 
            {
                if (lu32_ReadCnt > 0)
                {
                    usb_write_fifo(ENDPOINT_1, (uint8_t *)lu32_ReadDiskPoint, 64);
                    usb_Endpoints_SET_TxPktRdy();

                    lu32_ReadDiskPoint += 64;
                    lu32_ReadCnt -= 1;
                }
                else if (lu32_ReadCnt == 0)
                {
                    while(usb_Endpoints_GET_TxPktRdy());
                    usb_MassStorage_SendCSW(CBW->dCBWTag);
                    lu32_ReadStatus = false;

                    usb_TxInt_Disable(ENDPOINT_1);
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      usb_mass_storage_init
 *
 * @brief   mass storage parameter initialization 
 *
 * @param   None.
 * @return  None.
 */
void usb_mass_storage_init(void)
{
    /* Initialize the relevant pointer  */
    usbdev_get_dev_desc((uint8_t *)USB_MassStorage_DeviceDesc);
    usbdev_get_config_desc((uint8_t *)USB_MassStorage_ConfigurationDesc);
    usbdev_get_string_Manufacture((uint8_t *)USB_MassStorage_ManufactureDesc);
    usbdev_get_string_Product((uint8_t *)USB_MassStorage_ProductDesc);
    usbdev_get_string_SerialNumber((uint8_t *)USB_MassStorage_SerialNumberDesc);
    usbdev_get_string_LanuageID((uint8_t *)USB_MassStorage_LanuageIDDesc);

    Endpoint_0_ClassRequest_Handler = usb_MassStorage_ClassRequest_Handler;
    
    Endpoints_Handler = Endpoint1_Handler;

    USB_Reset_Handler = usb_mass_storage_init;
    
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
