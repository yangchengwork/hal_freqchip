/******************************************************************************
 * Copyright (c) 20203, Freqchip
 *
 * All rights reserved.
 *
 *
 */
#include <stdio.h>
#include <string.h>
//#include <core_cm33.h>

#include "fr30xx.h"
#include "co_log.h"
#include "co_util.h"
#include "crc32.h"
#include "gap_api.h"
#include "gatt_api.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "ota.h"
#include "service_ota.h"

/******************************************************************************
 * MACROS (宏定义)
 */

/*****************************************************************************
 * CONSTANTS (常量定义)
 */
typedef unsigned int uLong;
typedef unsigned int uInt;
typedef uLong uLongf;
typedef unsigned char Byte;
typedef Byte Bytef;
#define ZEXPORT
#define OF(a) a
#define Z_NULL 0

#define local static
/*****************************************************************************
* TYPEDEFS (类型定义)
*/
__PACKED_STRUCT firmware_version
{
    uint32_t firmware_version;
} __attribute__((packed));

__PACKED_STRUCT storage_baseaddr
{
    uint32_t baseaddr;
} __attribute__((packed));

__PACKED_STRUCT page_erase_rsp
{
    uint32_t base_address;
} __attribute__((packed));

__PACKED_STRUCT write_mem_rsp
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT read_mem_rsp
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT write_data_rsp
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT read_data_rsp
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT ota_start_rsp
{
    uint32_t ota_start;
} __attribute__((packed));

__PACKED_STRUCT ota_finish_rsp
{
    uint32_t ota_finsih_state;
} __attribute__((packed));

__PACKED_STRUCT app_ota_rsp_hdr_t
{
    uint8_t result;
    uint8_t org_opcode;
    uint16_t length;
    __PACKED_UNION
    {
        uint8_t nvds_type;
        struct firmware_version version;
        struct storage_baseaddr baseaddr;
        struct page_erase_rsp page_erase;
        struct write_mem_rsp write_mem;
        struct read_mem_rsp read_mem;
        struct write_data_rsp write_data;
        struct read_data_rsp read_data;
        struct ota_finish_rsp finsih_rsp;
        struct ota_start_rsp ota_start;
    } __attribute__((packed)) rsp;
} __attribute__((packed));

__PACKED_STRUCT page_erase_cmd
{
    uint32_t base_address;
} __attribute__((packed));

__PACKED_STRUCT write_mem_cmd
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT read_mem_cmd
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT write_data_cmd
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT read_data_cmd
{
    uint32_t base_address;
    uint16_t length;
} __attribute__((packed));

__PACKED_STRUCT firmware_check
{
    uint32_t firmware_length;
    uint32_t CRC32_data;
} __attribute__((packed));
__PACKED_STRUCT  app_ota_cmd_hdr_t
{
    uint8_t opcode;
    uint16_t length;
    __PACKED_UNION
    {
        struct page_erase_cmd page_erase;
        struct write_mem_cmd write_mem;
        struct read_mem_cmd read_mem;
        struct write_data_cmd write_data;
        struct read_data_cmd read_data;
        struct firmware_check fir_crc_data;
    } __attribute__((packed)) cmd;
} __attribute__((packed));

struct image_info_t 
{
    uint32_t image_version;
    uint32_t image_offset;      // storage offset
    uint32_t image_length;
    uint32_t execute_offset;    // execute offset
    uint32_t copy_unit;         // 4K, 8K, 16K, 32K, 64K, 128K
    uint32_t copy_flag_store_step;  // 4, 8, 16, 32, 64
    uint32_t image_crc;
    uint32_t image_valid;
    uint32_t image_tlv_length;    // Hash,signa length
    uint32_t options;
    uint32_t info_crc;
    uint32_t ota_magic;         // 需要更新标志
};

__PACKED_STRUCT otas_send_rsp
{
    uint8_t conidx;
    uint16_t length;
    uint8_t *buffer;
}; 
__PACKED_STRUCT source_file_info_t{
    uint8_t  check_buff[4];
    uint32_t version;
    uint32_t length;
    uint32_t crc;
};

/******************************************************************************
 * LOCAL VARIABLES (局部变量)
 */
static uint8_t first_loop;

/******************************************************************************
 * GLOBAL VARIABLES (全局变量)
 */
struct app_otas_status_t app_otas_status;
/******************************************************************************
 * EXTERN VARIABLES (外部变量)
 */
extern uint8_t ota_svc_id;
/******************************************************************************
 * LOCAL FUNCTIONS (本地函数)
 */
__RAM_CODE static uint8_t app_otas_crc_cal(uint32_t firmware_length,uint32_t new_bin_addr,uint32_t crc_data_t);
uLong ZEXPORT crc32(uLong crc, const Bytef *buf, uInt len);
uint32_t get_boot_crc(uint32_t imagesize,uint32_t length);
/******************************************************************************
 * EXTERN FUNCTIONS (外部函数)
 */
/*********************************************************************
 * @fn      app_otas_get_storage_address
 *
 * @brief   Get storage address in Zone B
 *
 * @param   None
 *
 *
 * @return  Storage address in Zone B.
 */
uint32_t app_otas_get_storage_address(void)
{
    struct image_info_t *a_info = (void *)(FLASH_DAC_BASE+A_ZONE_INFO_OFFSET);
    uint32_t a_image_tail;
    a_image_tail = a_info->image_length + a_info->execute_offset;
    //flash 4K对齐
    if((a_image_tail&0xFFF) != 0)
    {
        a_image_tail = (a_image_tail + 0xfff) & 0x0ffff000;;
    }
    return a_image_tail;
}
/*********************************************************************
 * @fn      ota_get_current_version
 *
 * @brief   Get the current program version
 *
 * @param   None
 *
 *
 * @return  Current program version.
 */
uint32_t ota_get_current_version(void)
{
    struct image_info_t *a_info = (void *)(FLASH_DAC_BASE + A_ZONE_INFO_OFFSET);
    if ((a_info->image_version == 0xffffffff)
        || (a_info->image_version == 0xeeeeeeee)) {
        struct image_info_t *b_info = (void *)(FLASH_DAC_BASE + B_ZONE_INFO_OFFSET);
        return b_info->image_version;
    }
    else {
        return a_info->image_version;
    }
}
/*********************************************************************
 * @fn      app_otas_save_data
 *
 */
void app_otas_save_data(uint32_t dest,uint8_t *src,uint32_t len)
{
    flash_write(QSPI0,dest,len,src);
}
/*********************************************************************
 * @fn      chip_reset
 *
 * @brief   Reset chip
 *
 * @param   None
 *
 * @return  None.
 */
void chip_reset(void)
{
    iwdt_Init_t iwdt;
    iwdt.iwdt_int_Enable = WDT_INT_DISABLE;
    iwdt.iwdt_Count = 2000;
    iwdt.iwdt_Timeout = 100;
    iwdt_init(iwdt);
    iwdt_Enable();
}
/*********************************************************************
 * @fn show_reg
 *
 */
void show_reg(uint8_t *buffer,uint16_t length,bool show_en)
{
    if(show_en)
    {
        for(uint16_t i = 0; i < length; i++)
        {
            printf("%02x ",buffer[i]);
        }
        printf("\r\n");
    }
}
/*********************************************************************
 * @fn      app_otas_read_data
 *
 */
uint16_t app_otas_read_data(uint8_t *p_data)
{
    uint16_t length;
    switch(app_otas_status.read_opcode)
    {
        case OTA_CMD_READ_DATA:
            
            length = app_otas_status.length;
            break;
        case OTA_CMD_READ_MEM:
            
            length = app_otas_status.length;
            break;
        default:
            length = 0;
            break;
    }
    app_otas_status.read_opcode = OTA_CMD_NULL;
    return length;
}
/*********************************************************************
 * @fn      app_otas_recv_data
 *
 * @brief   Otas Data handler
 *
 */
void app_otas_recv_data(uint8_t conidx,uint8_t *p_data,uint16_t len)
{
    struct app_ota_cmd_hdr_t *cmd_hdr = (struct app_ota_cmd_hdr_t *)p_data;
    struct app_ota_rsp_hdr_t *rsp_hdr;
    uint16_t rsp_data_len = (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN+OTA_HDR_RESULT_LEN);
    if(first_loop)
    {
        first_loop = false;
        gatt_mtu_exchange_req(ota_svc_id,conidx,247);  //mtu request
    }

    switch(cmd_hdr->opcode)
    {
        case OTA_CMD_NVDS_TYPE:
            rsp_data_len += 1;
            break;
        case OTA_CMD_GET_STR_BASE:
            rsp_data_len += sizeof(struct storage_baseaddr);
            break;
        case OTA_CMD_READ_FW_VER:
            rsp_data_len += sizeof(struct firmware_version);
            break;
        case OTA_CMD_PAGE_ERASE:
            rsp_data_len += sizeof(struct page_erase_rsp);
            break;
        case OTA_CMD_WRITE_DATA:
            rsp_data_len += sizeof(struct write_data_rsp);
            break;
        case OTA_CMD_READ_DATA:
            rsp_data_len += sizeof(struct read_data_rsp) + cmd_hdr->cmd.read_data.length;
            if(rsp_data_len > OTAS_NOTIFY_DATA_SIZE)
            {
                // 数据太长，不能通过notify返回，通知client采用read方式获取
                rsp_data_len = sizeof(struct read_data_rsp) + (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN+OTA_HDR_RESULT_LEN);
            }
            break;
        case OTA_CMD_WRITE_MEM:
            rsp_data_len += sizeof(struct write_mem_rsp);
            break;
        case OTA_CMD_READ_MEM:
            rsp_data_len += sizeof(struct read_mem_rsp) + cmd_hdr->cmd.read_mem.length;
            if(rsp_data_len > OTAS_NOTIFY_DATA_SIZE)
            {
                // 数据太长，不能通过notify返回，通知client采用read方式获取
                rsp_data_len = sizeof(struct read_data_rsp) + (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN+OTA_HDR_RESULT_LEN);
            }
            break;
        case OTA_CMD_START:
            rsp_data_len += sizeof(struct ota_start_rsp);
            break;
        case OTA_CMD_NULL:
            return;
    }
    struct otas_send_rsp *req = pvPortMalloc(sizeof(struct otas_send_rsp));
    uint16_t base_length;
    req->conidx = conidx;
    req->length = rsp_data_len;
    req->buffer = pvPortMalloc(rsp_data_len);
    rsp_hdr = (struct app_ota_rsp_hdr_t *)&req->buffer[0];
    rsp_hdr->result = OTA_RSP_SUCCESS;
    rsp_hdr->org_opcode = cmd_hdr->opcode;
    rsp_hdr->length = rsp_data_len - (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN+OTA_HDR_RESULT_LEN);
    switch(cmd_hdr->opcode)
    {
        case OTA_CMD_NVDS_TYPE: 
            rsp_hdr->rsp.nvds_type = 0x01;
            break;
        case OTA_CMD_GET_STR_BASE: //获取存储地址
             rsp_hdr->rsp.baseaddr.baseaddr = app_otas_get_storage_address();
            break;
        case OTA_CMD_READ_FW_VER: //获取版本号
             rsp_hdr->rsp.version.firmware_version = ota_get_current_version();
            break;
        case OTA_CMD_PAGE_ERASE: //擦除
        {
            rsp_hdr->rsp.page_erase.base_address = cmd_hdr->cmd.page_erase.base_address;
            uint32_t new_bin_base = app_otas_get_storage_address();
            printf("erase:%x\r\n",rsp_hdr->rsp.page_erase.base_address);
            if(rsp_hdr->rsp.page_erase.base_address < new_bin_base)
            {
                printf("ota erase addresss fail\r\n");
                gap_disconnect(conidx);
            }
            else
                flash_erase(QSPI0,rsp_hdr->rsp.page_erase.base_address, 0x1000);
        } 
        break;
        case OTA_CMD_CHIP_ERASE:
            break;
        case OTA_CMD_WRITE_DATA:  //导入数据
        {
            rsp_hdr->rsp.write_data.base_address = cmd_hdr->cmd.write_data.base_address;
            rsp_hdr->rsp.write_data.length = cmd_hdr->cmd.write_data.length;
            printf("write address:%x,%x\r\n",rsp_hdr->rsp.write_data.base_address,rsp_hdr->rsp.write_data.length);
            if(rsp_hdr->rsp.page_erase.base_address < app_otas_get_storage_address())
            {
                printf("ota write addresss fail\r\n");
            }
            else
                app_otas_save_data(rsp_hdr->rsp.write_data.base_address,
                                   p_data + (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN)+sizeof(struct write_data_cmd),
                                   rsp_hdr->rsp.write_data.length);
        }
        break;
        case OTA_CMD_READ_DATA:
            rsp_hdr->rsp.read_data.base_address = cmd_hdr->cmd.read_data.base_address;
            rsp_hdr->rsp.read_data.length = cmd_hdr->cmd.read_data.length;
            base_length = sizeof(struct read_data_rsp) + (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN+OTA_HDR_RESULT_LEN);
            if(rsp_data_len != base_length)
            {
                flash_read(QSPI0,rsp_hdr->rsp.read_data.base_address,
                                rsp_hdr->rsp.read_data.length,
                                (uint8_t*)rsp_hdr+base_length);
            }
            break;
        case OTA_CMD_WRITE_MEM:
            rsp_hdr->rsp.write_mem.base_address = cmd_hdr->cmd.write_mem.base_address;
            rsp_hdr->rsp.write_mem.length = cmd_hdr->cmd.write_mem.length;
            memcpy((void *)rsp_hdr->rsp.write_mem.base_address,
                   p_data + (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN)+sizeof(struct write_data_cmd),
                   rsp_hdr->rsp.write_mem.length);
            break;
        case OTA_CMD_READ_MEM:
            rsp_hdr->rsp.read_mem.base_address = cmd_hdr->cmd.read_mem.base_address;
            rsp_hdr->rsp.read_mem.length = cmd_hdr->cmd.read_mem.length;
            base_length = sizeof(struct read_mem_rsp) + (OTA_HDR_OPCODE_LEN+OTA_HDR_LENGTH_LEN+OTA_HDR_RESULT_LEN);
            if(rsp_data_len != base_length)
            {
                memcpy((uint8_t*)rsp_hdr+base_length,
                       (void *)rsp_hdr->rsp.read_mem.base_address,
                       rsp_hdr->rsp.read_data.length);
            }
            break;
        case OTA_CMD_REBOOT:  //复位重启
        {
            rsp_hdr->result = 0x01;
            printf("reboot:%x,%x,%x\r\n",app_otas_get_storage_address(),cmd_hdr->cmd.fir_crc_data.firmware_length,cmd_hdr->cmd.fir_crc_data.CRC32_data);
            if(app_otas_crc_cal(cmd_hdr->cmd.fir_crc_data.firmware_length,app_otas_get_storage_address(),cmd_hdr->cmd.fir_crc_data.CRC32_data))
            {
                struct image_info_t b_info;
                flash_read(QSPI0,A_ZONE_INFO_OFFSET,sizeof(struct image_info_t),(uint8_t *)&b_info);
                b_info.image_offset = app_otas_get_storage_address();
                b_info.image_length = cmd_hdr->cmd.fir_crc_data.firmware_length;
                if(ota_get_current_version() == 0xffffffff)
                {
                    b_info.image_version = 1;
                }
                else
                {
                    b_info.image_version = ota_get_current_version()+1;    
                }
                b_info.copy_unit = 4*1024;
                b_info.copy_flag_store_step = 8;
                b_info.image_crc = crc32(0x00000000,(const uint8_t *)(b_info.image_offset | FLASH_DAC_BASE),b_info.image_length);
                b_info.info_crc = crc32(0x00000000,(uint8_t *)&b_info,IMAGE_INFO_CRC_ZONE_LEN);
                               
                GLOBAL_INT_DISABLE();

                flash_erase(QSPI0,B_ZONE_INFO_OFFSET,0x1000);
                flash_write(QSPI0, 
                            B_ZONE_INFO_OFFSET + offsetof(struct image_info_t, image_tlv_length),
                            sizeof(struct image_info_t) - offsetof(struct image_info_t, image_tlv_length),
                            (uint8_t *)&b_info.image_tlv_length);
                flash_write(QSPI0, 
                            B_ZONE_INFO_OFFSET,
                            offsetof(struct image_info_t, image_valid),
                            (uint8_t *)&b_info);
                flash_write(QSPI0, 
                            B_ZONE_INFO_OFFSET + offsetof(struct image_info_t, image_valid),
                            sizeof(uint32_t),
                            (uint8_t *)&b_info.image_valid);        
                
                GLOBAL_INT_RESTORE();
                
                show_reg((uint8_t *)&b_info,sizeof(struct image_info_t),SHOW_REG_ON);
                printf("sucessful\r\n");
                chip_reset();
            }else{
                printf("error\r\n");
                rsp_hdr->rsp.finsih_rsp.ota_finsih_state = 0;
                chip_reset();
            }
        }            
            break;
       case OTA_CMD_START:
           rsp_hdr->rsp.ota_start.ota_start = 1;
            break;
        default:
            rsp_hdr->result = OTA_RSP_UNKNOWN_CMD;
            break;
    }
    ota_gatt_report_notify(conidx,req->buffer,req->length);
    vPortFree(req->buffer);
    vPortFree(req);
}

static uint32_t Crc32CalByByte(int crc,uint8_t* ptr, int len)
{
    int i = 0;
    const uint32_t *crc_table = crc32_get_table();
    
    while(len-- != 0)
    {
        int high = crc/256;
        crc <<= 8;
        crc ^= crc_table[(high^ptr[i])&0xff];
        crc &= 0xFFFFFFFF;
        i++;
    }
    return crc&0xFFFFFFFF;
}

__RAM_CODE static uint8_t app_otas_crc_cal(uint32_t firmware_length,uint32_t new_bin_addr,uint32_t crc_data_t)
{   
    uint32_t crc_data = 0,i = 0;
    uint8_t * crc_check_data = pvPortMalloc(0x1000);
    uint8_t ret = 0;
    uint32_t data_size = firmware_length;
    uint32_t read_address = new_bin_addr;
    while(data_size > 0x1000)
    {
        flash_read(QSPI0,read_address, 0x1000,crc_check_data);
        crc_data = Crc32CalByByte(crc_data, crc_check_data,0x1000);
        data_size -= 0x1000;
        read_address += 0x1000;
    }
    flash_read(QSPI0,read_address, data_size,crc_check_data);
    crc_data = Crc32CalByByte(crc_data, crc_check_data,data_size);
    printf("crc:%x,%x",crc_data,crc_data_t);
    vPortFree(crc_check_data);
    if(crc_data_t == crc_data)
        ret = 1;

    return ret;
}

uint32_t get_boot_crc(uint32_t imagesize,uint32_t length)
{
    uint32_t crc_data = 0,i = 0;
    uint8_t * crc_check_data = pvPortMalloc(0x1000);
    uint8_t ret = 0;
    uint32_t data_size = length;
    uint32_t read_address = imagesize;
    while(data_size > 0x1000)
    {
        flash_read(QSPI0,read_address, 0x1000,crc_check_data);
        crc_data = crc32(crc_data, crc_check_data,0x1000);
        data_size -= 0x1000;
        read_address += 0x1000;
    }
    flash_read(QSPI0,read_address, data_size,crc_check_data);
    crc_data = crc32(crc_data, crc_check_data,data_size);
    printf("file crc:%x\r\n",crc_data);
    vPortFree(crc_check_data);
    return crc_data;
}