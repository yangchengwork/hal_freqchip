#ifndef APP_OTA_H
#define APP_OTA_H
#include <stdint.h>
#include "fr30xx.h"
/******************************************************************************
 * MACROS (宏定义)
 */
 
/*****************************************************************************
 * CONSTANTS (常量定义)
 */
#define SHOW_REG_ON                      0
#define A_ZONE_INFO_OFFSET              (0x0000)
#define B_ZONE_INFO_OFFSET              (0x1000)

#define OTA_HDR_RESULT_LEN               1
#define OTA_HDR_OPCODE_LEN               1
#define OTA_HDR_LENGTH_LEN               2

#define IMAGE_INFO_CRC_ZONE_LEN             ((uint32_t)(&((struct image_info_t *)0)->info_crc))
typedef enum
{
    OTA_CMD_NVDS_TYPE,
    OTA_CMD_GET_STR_BASE,
    OTA_CMD_READ_FW_VER,    //read firmware version
    OTA_CMD_PAGE_ERASE,
    OTA_CMD_CHIP_ERASE,
    OTA_CMD_WRITE_DATA,
    OTA_CMD_READ_DATA,
    OTA_CMD_WRITE_MEM,
    OTA_CMD_READ_MEM,
    OTA_CMD_REBOOT,
    OTA_CMD_START,
    OTA_CMD_NULL,
} ota_cmd_t;

typedef enum
{
    FILE_BOOT_LOADER =1,
    FILE_APP,
    FILE_CONTROLLER,
    FILE_DSP,
}ota_file_type_t;

typedef enum 
{
    OTA_RSP_SUCCESS,
    OTA_RSP_ERROR,
    OTA_RSP_UNKNOWN_CMD,
}ota_rsp_t;

/*****************************************************************************
 * TYPEDEFS (类型定义)
 */
struct app_otas_status_t
{
    uint8_t  read_opcode;
    uint8_t  length;
    uint32_t base_addr;
};

/******************************************************************************
 * GLOBAL VARIABLES (全局变量)
 */

/******************************************************************************
 * FUNCTION DESCRIPTION(函数描述)
 */
void app_otas_recv_data(uint8_t conidx,uint8_t *p_data,uint16_t len);
uint16_t app_otas_read_data(uint8_t *p_data);
#endif 
