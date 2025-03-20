/******************************************************************************
 * Copyright (c) 20203, Freqchip
 *
 * All rights reserved.
 *
 *
 */
 
/******************************************************************************
 * INCLUDES (包含头文件)
 */
#include <stdio.h>
#include <string.h>

#include "co_util.h"
#include "gap_api.h"
#include "gatt_api.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "co_log.h"

#include "service_ota.h"
#include "ota.h"

/******************************************************************************
 * MACROS (宏定义)
 */
 
/*****************************************************************************
 * CONSTANTS (常量定义)
 */

 
/*****************************************************************************
 * TYPEDEFS (类型定义)
 */

/******************************************************************************
 * LOCAL VARIABLES (局部变量)
 */
static const uint8_t ota_svc_uuid[UUID_SIZE_16] = OTA_SVC_UUID;
static bool ota_link_ntf_enable = false;
/******************************************************************************
 * GLOBAL VARIABLES (全局变量)
 */
uint8_t ota_svc_id = 0;

/******************************************************************************
 * Profile Attributes - Table
 * 每一项都是一个attribute的定义。
 * 第一个attribute为Service 的的定义。
 * 每一个特征值(characteristic)的定义，都至少包含三个attribute的定义；
 * 1. 特征值声明(Characteristic Declaration)
 * 2. 特征值的值(Characteristic value)
 * 3. 特征值描述符(Characteristic description)
 * 如果有notification 或者indication 的功能，则会包含四个attribute的定义，除了前面定义的三个，还会有一个特征值客户端配置(client characteristic configuration)。
 *
 */
const gatt_attribute_t ota_svc_att_table[OTA_ATT_NB] =
{
    // Update Over The AIR Service Declaration
    [OTA_ATT_IDX_SERVICE] = { { UUID_SIZE_2, UUID16_ARR(GATT_PRIMARY_SERVICE_UUID) },
        GATT_PROP_READ,UUID_SIZE_16, (uint8_t *)ota_svc_uuid
    },

    // OTA Information Characteristic Declaration
    [OTA_ATT_IDX_CHAR_DECLARATION_VERSION_INFO] = { { UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },
        GATT_PROP_READ, 0, NULL
    },
    [OTA_ATT_IDX_CHAR_VALUE_VERSION_INFO]= { { UUID_SIZE_16, OTA_CHAR_UUID_VERSION_INFO },
        GATT_PROP_READ, sizeof(uint16_t), NULL
    },

    // Notify Characteristic Declaration
    [OTA_ATT_IDX_CHAR_DECLARATION_NOTI] = { { UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },
        GATT_PROP_READ,0, NULL
    },
    [OTA_ATT_IDX_CHAR_VALUE_NOTI] = { { UUID_SIZE_16, OTA_CHAR_UUID_NOTI },
        GATT_PROP_READ | GATT_PROP_NOTI, OTAS_NOTIFY_DATA_SIZE, NULL
    },
    [OTA_ATT_IDX_CHAR_CFG_NOTI] = { { UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID) },
        GATT_PROP_READ | GATT_PROP_WRITE_CMD | GATT_PROP_WRITE_REQ, 0,0
    },
    [OTA_IDX_CHAR_USER_DESCRIPTION_NOTI]= { { UUID_SIZE_2, UUID16_ARR(GATT_CHAR_USER_DESC_UUID) },
        GATT_PROP_READ, 12, NULL
    },

    // Tx Characteristic Declaration
    [OTA_ATT_IDX_CHAR_DECLARATION_TX] = { { UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },
        GATT_PROP_READ, 0, NULL
    },
    [OTA_ATT_IDX_CHAR_VALUE_TX] = { { UUID_SIZE_16, OTA_CHAR_UUID_TX },
        GATT_PROP_READ, OTAS_MAX_DATA_SIZE, NULL
    },

    // Rx Characteristic Declaration
    [OTA_ATT_IDX_CHAR_DECLARATION_RX] = { { UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },
        GATT_PROP_READ, 0, NULL
    },
    [OTA_ATT_IDX_CHAR_VALUE_RX] = { { UUID_SIZE_16, OTA_CHAR_UUID_RX },
        GATT_PROP_WRITE_CMD | GATT_PROP_WRITE_REQ, OTAS_MAX_DATA_SIZE, NULL
    },
};
/*********************************************************************
 * @fn      ota_gatt_msg_handler
 *
 * @brief   Ota Profile callback funtion for GATT messages. GATT read/write
 *          operations are handeled here.
 *
 * @param   gatt_msg     -GATT messages from GATT layer.
 *
 * @return  None.
 */
static uint16_t ota_gatt_msg_handler(struct gatt_msg *p_msg)
{
    switch(p_msg->msg_evt)
    {
        case GATTS_MSG_READ_REQ:
            if(p_msg->att_idx == OTA_IDX_CHAR_USER_DESCRIPTION_NOTI)
            {
                memcpy(p_msg->param.gatt_data.p_msg_data, "OTA Response", strlen("OTA Response"));
                return strlen("OTA Response");
            }
            else if (p_msg->att_idx == OTA_ATT_IDX_CHAR_VALUE_NOTI)
            {
                memcpy(p_msg->param.gatt_data.p_msg_data, "ntf_enable", strlen("ntf_enable"));
                return strlen("ntf_enable");
            }
            else if (p_msg->att_idx == OTA_ATT_IDX_CHAR_VALUE_TX)
            {
                return app_otas_read_data(p_msg->param.gatt_data.p_msg_data);
            }
            else if (p_msg->att_idx == OTA_ATT_IDX_CHAR_VALUE_VERSION_INFO)  //get version
            {
                memcpy(p_msg->param.gatt_data.p_msg_data, "\x00\x01", strlen("\x00\x01"));
                return strlen("\x00\x01");
            }
            break;

        case GATTS_MSG_WRITE_REQ:
            if(p_msg->att_idx == OTA_ATT_IDX_CHAR_CFG_NOTI)
            {
                
                if(*(uint16_t *)p_msg->param.gatt_data.p_msg_data == 0x1)
                {
                    printf("true\r\n");
                    ota_link_ntf_enable = true;
                }
                else
                {
                    printf("false\r\n");
                    ota_link_ntf_enable = false;
                }
            }
            else if(p_msg->att_idx == OTA_ATT_IDX_CHAR_VALUE_RX)
            {
                app_otas_recv_data(p_msg->conn_idx,p_msg->param.gatt_data.p_msg_data,p_msg->param.gatt_data.msg_len);
            }
            break;
        case GATTC_MSG_CMP_EVT:

            break;
        case GATTC_MSG_LINK_CREATE:
            break;
        case GATTC_MSG_LINK_LOST:
            ota_link_ntf_enable = false;
            break;
        default:
            break;
    }
    return 0;
}
/*********************************************************************
 * @fn      ota_gatt_report_notify
 *
 * @brief   Send ota protocol response data.
 *
 *
 * @param   conidx      - report idx of the hid_rpt_info array.
 *          p_data      - data of the Ota information to be sent.
 *          len         - length of the HID information data.          
 *
 * @return  none.
 */
void ota_gatt_report_notify(uint8_t conidx, uint8_t *p_data, uint16_t len)
{
    if (ota_link_ntf_enable)
    {
        struct gatt_send_event  ntf;
        ntf.conidx = conidx;
        ntf.svc_id = ota_svc_id;
        ntf.att_idx = OTA_ATT_IDX_CHAR_VALUE_NOTI;
        ntf.data_len = len;
        ntf.p_data = p_data;
        gatt_notification(&ntf);
    }
}
/*********************************************************************
 * @fn      ota_gatt_add_service
 *
 * @brief   Ota Profile add GATT service function.
 *          添加GATT service到ATT的数据库里面。
 *
 * @param   None.
 *
 *
 * @return  None.
 */
void ota_gatt_add_service(void)
{
    gatt_service_t ota_profie_svc;
    ota_profie_svc.p_att_tb = ota_svc_att_table;
    ota_profie_svc.att_nb = OTA_ATT_NB;
    ota_profie_svc.gatt_msg_handler = ota_gatt_msg_handler;

    ota_svc_id = gatt_add_service(&ota_profie_svc);
}

