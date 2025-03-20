/******************************************************************************
 * Copyright (c) 2023, Freqchip
 *
 * All rights reserved.
 *
 *
 */
#ifndef APP_OTA_SERVICE
#define APP_OTA_SERVICE

/*******************************************************************************
 * INCLUDES (包含头文件)
 */


/*******************************************************************************
 * MACROS (宏定义)
 */
#define OTA_SVC_UUID                {0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x02} 
    
#define OTA_CHAR_UUID_TX            {0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x02}
#define OTA_CHAR_UUID_RX            {0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x02}
#define OTA_CHAR_UUID_NOTI          {0x02, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x02}
#define OTA_CHAR_UUID_VERSION_INFO  {0x03, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x02}

#define OTAS_MAX_DATA_SIZE              600
#define OTAS_NOTIFY_DATA_SIZE           20

/*******************************************************************************
 * CONSTANTS (常量定义)
 */
enum
{
    OTA_ATT_IDX_SERVICE,

    OTA_ATT_IDX_CHAR_DECLARATION_VERSION_INFO,
    OTA_ATT_IDX_CHAR_VALUE_VERSION_INFO,

    OTA_ATT_IDX_CHAR_DECLARATION_NOTI,
    OTA_ATT_IDX_CHAR_VALUE_NOTI,
    OTA_ATT_IDX_CHAR_CFG_NOTI,
    OTA_IDX_CHAR_USER_DESCRIPTION_NOTI,

    OTA_ATT_IDX_CHAR_DECLARATION_TX,
    OTA_ATT_IDX_CHAR_VALUE_TX,

    OTA_ATT_IDX_CHAR_DECLARATION_RX,
    OTA_ATT_IDX_CHAR_VALUE_RX,

    OTA_ATT_NB,
};

/*******************************************************************************
 * TYPEDEFS (类型定义)
 */

/*******************************************************************************
 * GLOBAL VARIABLES (全局变量)
 */

/*******************************************************************************
 * LOCAL VARIABLES (本地变量)
 */


/*******************************************************************************
 * PUBLIC FUNCTIONS (全局函数)
 */
void ota_gatt_add_service(void);
void ota_gatt_report_notify(uint8_t conidx, uint8_t *p_data, uint16_t len);

#endif
