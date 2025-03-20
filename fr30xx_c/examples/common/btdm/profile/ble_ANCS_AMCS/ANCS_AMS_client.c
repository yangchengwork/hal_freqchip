/**
 * Copyright (c) 2019, Freqchip
 *
 * All rights reserved.
 *
 * ANCS client profile. protocol websit:
 * https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification
 * /Specification/Specification.html#//apple_ref/doc/uid/TP40013460-CH1-SW14
 *
 */

/*
 * INCLUDES (°üº¬Í·ÎÄ¼þ)
 */
#include <stdio.h>
#include <string.h>

#include "gap_api.h"
#include "gatt_api.h"
#include "gatt_sig_uuid.h"
#include "ANCS_AMS_client.h"
#include "AMS_client.h"
#include "system_fr30xx.h"

#define   ANCS_LOG  printf
/*
 * MACROS (ºê¶¨Òå)
 */

/*
 * CONSTANTS (³£Á¿¶¨Òå)
 */
#define BIT(x) (1<<(x))
/*
 * TYPEDEFS (ÀàÐÍ¶¨Òå)
 */

/*
 * GLOBAL VARIABLES (È«¾Ö±äÁ¿)
 */


/*
 * LOCAL VARIABLES (±¾µØ±äÁ¿)
 */
uint8_t ANCS_AMS_client_id;

#define CTL_POINT_UUID     {0xd9, 0xd9, 0xaa, 0xfd, 0xbd, 0x9b, 0x21, 0x98, \
                                            0xa8, 0x49, 0xe1, 0x45, 0xf3,0xd8, 0xd1, 0x69}
#define NTF_SRC_UUID       {0xbd, 0x1d, 0xa2, 0x99, 0xe6, 0x25, 0x58, 0x8c, \
                                            0xd9, 0x42, 0x01, 0x63, 0x0d,0x12, 0xbf, 0x9f}
#define DATA_SRC_UUID      {0xfb, 0x7b, 0x7c, 0xce, 0x6a, 0xb3, 0x44, 0xbe, \
                                            0xb5, 0x4b, 0xd6, 0x24, 0xe9,0xc6, 0xea, 0x22}

/*************AMS UUID************/
#define REMOTE_CMD_UUIDx     {0xc2, 0x51, 0xca, 0xf7, 0x56, 0x0e, 0xdf, 0xb8, \
                                                0x8a, 0x4a, 0xb1, 0x57, 0xd8,0x81, 0x3c, 0x9b}
#define ENT_UPDATE_UUIDx     {0x02, 0xc1, 0x96, 0xba, 0x92, 0xbb, 0x0c, 0x9a, \
                                                0x1f, 0x41, 0x8d, 0x80, 0xce,0xab, 0x7c, 0x2f}
#define ENT_ATT_UUIDx       {0xd7, 0xd5, 0xbb, 0x70, 0xa8, 0xa3, 0xab, 0xa6, \
                                                0xd8, 0x46, 0xab, 0x23, 0x8c,0xf3, 0xb2, 0xc6}
/*************AMS UUID************/                                                

/*********************************************************************
 * Profile Attributes - Table
 * Ã¿Ò»Ïî¶¼ÊÇÒ»¸öcharacteristic attribute¶ÔÓ¦µÄUUIDµÄ¶¨Òå¡£
 * µÚÒ»¸öÊÇUUIDµÄ³¤¶È£¬
 * Ã¿¶þ¸öÊÇUUIDµÄÖµ¡£
 */
const gatt_uuid_t ANCS_AMS_att_tb[] =
{
    [ANCS_ATT_IDX_CTL_POINT] = { UUID_SIZE_16, CTL_POINT_UUID},
    [ANCS_ATT_IDX_NTF_SRC] = { UUID_SIZE_16, NTF_SRC_UUID},
    [ANCS_ATT_IDX_DATA_SRC] = { UUID_SIZE_16, DATA_SRC_UUID},
    
    [AMS_ATT_IDX_REMOTE_CMDx] = { UUID_SIZE_16, REMOTE_CMD_UUIDx},
    [AMS_ATT_IDX_ENT_UPDATEx] = { UUID_SIZE_16, ENT_UPDATE_UUIDx},
    [AMS_ATT_IDX_ENT_ATTx] = { UUID_SIZE_16, ENT_ATT_UUIDx},
};
static uint16_t ANCS_hdl_cache[6] = {0};
static uint16_t android_buf[6]={0,0,0,0,0,0};
uint32_t call_notification_uid ;
uint32_t social_notification_uid ;


#define EVENT_ID_NOTIFICATION_ADD       (0)      /**< The arrival of a new iOS notification on the NP */
#define EVENT_ID_NOTIFICATION_MODIFIED  (1) /**< The modification of an iOS notification on the NP */
#define EVENT_ID_NOTIFICATION_REMOVED   (2)  /**< The removal of an iOS notification on the NP */
#define EVENT_ID_NOTIFICATION_RESERVED  (0xff)

#define EVT_FLAG_SILENT         BIT(0)
#define EVT_FLAG_IMPORTANT      BIT(1)
#define EVT_FLAG_PRE_EXSITING   BIT(2)
#define EVT_FLAG_POSITIVE       BIT(3)
#define EVT_FLAG_NEGATIVE       BIT(4)

#define CATGRY_ID_OTHER         (0)
#define CATGRY_ID_INCOMING_CALL (1)
#define CATGRY_ID_MISS_CALL     (2)
#define CATGRY_ID_VOICE_MAIL    (3)
#define CATGRY_ID_SOCIAL        (4)
#define CATGRY_ID_SCHEDULE      (5)
#define CATGRY_ID_EMAIL         (6)
#define CATGRY_ID_NEWS          (7)
#define CATGRY_ID_HEALTH        (8)
#define CATGRY_ID_BUSINESS      (9)
#define CATGRY_ID_LOCATION      (10)
#define CATGRY_ID_ENTERTAINMENT (11)

struct ancs_ntf_src
{
    uint8_t event_id;
    uint8_t event_flags;
    uint8_t category_id;
    uint8_t category_cnt;
    uint32_t ntf_uid;
};

#define NTF_ATT_ID_APPLE        0
#define NTF_ATT_ID_TITLE        1
#define NTF_ATT_ID_SUBTITLE     2
#define NTF_ATT_ID_MSG          3
#define NTF_ATT_ID_MSG_SIZE     4
#define NTF_ATT_ID_DATE         5
#define NTF_ATT_ID_POSITIVE_ACT 6
#define NTF_ATT_ID_NEGATIVE_ACT 7

extern uint16_t AMS_gatt_msg_handler(struct gatt_msg *p_msg);
extern uint16_t app_ble_get_mtu(uint8_t conidx);


static void show_utf8_hex(uint8_t *data,uint32_t len,uint8_t dbg_on)
{
    uint32_t i=0;
    if(len == 0 || (dbg_on==0)) return;
    for(; i<len; i++)
    {
        ANCS_LOG("0x%02X ",data[i]);
    }
    printf("\r\n");
}

void ANCS_recv_ntf_src(uint8_t conidx,uint8_t *p_data, uint16_t len)
{
    if(len != 8)
        goto _exit;
    struct ancs_ntf_src *ntf_src = (struct ancs_ntf_src *)p_data;
    ANCS_LOG("event_id:%d,event_flags:%x,category_id:%d,category_cnt:%d,ntf_uid:%x\r\n",ntf_src->event_id
              ,ntf_src->event_flags,ntf_src->category_id,ntf_src->category_cnt,ntf_src->ntf_uid);
    if( (ntf_src->category_id == CATGRY_ID_SOCIAL
         || ntf_src->category_id == CATGRY_ID_OTHER
         || ntf_src->category_id == CATGRY_ID_INCOMING_CALL
         || ntf_src->category_id == CATGRY_ID_EMAIL) &&
        ((ntf_src->event_flags & EVT_FLAG_PRE_EXSITING) != 0x04))
    {
        if(ntf_src->category_id == CATGRY_ID_INCOMING_CALL)
            call_notification_uid = ntf_src->ntf_uid;
        if(ntf_src->category_id == CATGRY_ID_SOCIAL)
            social_notification_uid = ntf_src->ntf_uid;
        if(ntf_src->category_id == CATGRY_ID_MISS_CALL)
            printf("Miss call !\r\n");

        if(ntf_src->event_id != EVENT_ID_NOTIFICATION_REMOVED)
        {
            printf("Send RSP\r\n");
            uint16_t max_att_len = app_ble_get_mtu(conidx)-3;
            uint8_t rsp[19];
            uint8_t i = 0;
            rsp[i++] = ANCS_CMD_ID_GET_NOTIFICATION_ATTR;   //cmd id
            *(uint32_t *)(rsp + i) = ntf_src->ntf_uid;  //ntf_uid
            i+=4;

            rsp[i++] = NTF_ATT_ID_APPLE;
            rsp[i++] = NTF_ATT_ID_TITLE;
            rsp[i++] = (max_att_len & 0xff);
            rsp[i++] = (max_att_len & 0xff00)>>8;

            rsp[i++] = NTF_ATT_ID_SUBTITLE;
            rsp[i++] = (max_att_len & 0xff);
            rsp[i++] = (max_att_len & 0xff00)>>8;

            rsp[i++] = NTF_ATT_ID_MSG;
            rsp[i++] = (max_att_len & 0xff);
            rsp[i++] = (max_att_len & 0xff00)>>8;

            rsp[i++] = NTF_ATT_ID_MSG_SIZE;
            rsp[i++] = NTF_ATT_ID_DATE;
            rsp[i++] = NTF_ATT_ID_POSITIVE_ACT;
            rsp[i++] = NTF_ATT_ID_NEGATIVE_ACT;

            ANCS_gatt_write_req(conidx,ANCS_ATT_IDX_CTL_POINT,rsp,sizeof(rsp));
        }
    }
_exit:
    ;
}
enum ntf_msg_type_t
{
    WEIXIN,
    MOBILE_SMS,
    MOBILE_PHONE,
    QQ,
};
void ANCS_recv_data_src(uint8_t conidx,uint8_t *p_data, uint16_t len)
{
    uint8_t *str = NULL;
    uint16_t str_len = 0;
    enum ntf_msg_type_t msg_type = 0;
    uint16_t i = 0;
    uint16_t data_len = 0;

    uint8_t event_id = p_data[i++];
    uint32_t uid = *(uint32_t *)(p_data + i);
    i+=4;
    ANCS_LOG("evt_id:%d,uid:%x,len:%d\r\n",event_id,uid,len);
    while(i < len)
    {
        switch(p_data[i++])
        {
            case NTF_ATT_ID_APPLE:
                data_len = *(uint16_t *)(p_data + i);
                i+=2;
                //show_reg(p_data + i,data_len,1);
                if( memcmp(p_data+i,"com.tencent.xin",strlen("com.tencent.xin"))==0 )
                    msg_type = WEIXIN;
                else if( memcmp(p_data+i,"com.apple.MobileSMS",strlen("com.apple.MobileSMS"))==0 )
                    msg_type = MOBILE_SMS;
                else if( memcmp(p_data+i,"com.apple.mobilephone",strlen("com.apple.mobilephone"))==0 )
                    msg_type = MOBILE_PHONE;
                else if( memcmp(p_data+i,"com.tencent.mqq",strlen("com.tencent.mqq"))==0 )
                    msg_type = QQ;
                else if( memcmp(p_data+i,"com.tencent.qq",strlen("com.tencent.qq"))==0 )
                    msg_type = QQ;
                ANCS_LOG("NTF_ATT_ID_APPLE,msg_type:%d,len:%d\r\n",msg_type,data_len);
                i+=data_len;
                break;
            case NTF_ATT_ID_TITLE:  //UTF-8
                data_len = *(uint16_t *)(p_data + i);
                ANCS_LOG("NTF_ATT_ID_TITLE,len:%d\r\n",data_len);
                i+=2;
                show_utf8_hex(p_data + i,data_len,1);

                //str = (uint8_t *)(p_data + i);
                //str_len += data_len;

                i+=data_len;
                break;
            case NTF_ATT_ID_SUBTITLE:   //UTF-8
                data_len = *(uint16_t *)(p_data + i);
                ANCS_LOG("NTF_ATT_ID_SUBTITLE,len:%d\r\n",data_len);
                i+=2;
                show_utf8_hex(p_data + i,data_len,1);
                i+=data_len;
                break;
            case NTF_ATT_ID_MSG:    //UTF-8
                data_len = *(uint16_t *)(p_data + i);
                ANCS_LOG("NTF_ATT_ID_MSG,len:%d\r\n",data_len);
                i+=2;
                show_utf8_hex(p_data + i,data_len,1);

                //str = (uint8_t *)(p_data + i);
                //str_len += data_len;

                i+=data_len;
                break;
            case NTF_ATT_ID_MSG_SIZE:   //ASCII
                data_len = *(uint16_t *)(p_data + i);
                ANCS_LOG("NTF_ATT_ID_MSG_SIZE,len:%d\r\n",data_len);
                i+=2;
                show_utf8_hex(p_data + i,data_len,1);
                i+=data_len;
                break;
            case NTF_ATT_ID_DATE:       //ASCII
                data_len = *(uint16_t *)(p_data + i);
                ANCS_LOG("NTF_ATT_ID_DATE,len:%d\r\n",data_len);
                i+=2;
                show_utf8_hex(p_data + i,data_len,1);
                i+=data_len;
                break;
            case NTF_ATT_ID_POSITIVE_ACT:       //UTF-8
                data_len = *(uint16_t *)(p_data + i);
                ANCS_LOG("NTF_ATT_ID_POSITIVE_ACT,len:%d\r\n",data_len);
                i+=2;
                show_utf8_hex(p_data + i,data_len,1);
                i+=data_len;
                break;
            case NTF_ATT_ID_NEGATIVE_ACT:       //UTF-8
                data_len = *(uint16_t *)(p_data + i);
                ANCS_LOG("NTF_ATT_ID_NEGATIVE_ACT,len:%d\r\n",data_len);
                i+=2;
                show_utf8_hex(p_data + i,data_len,1);
                i+=data_len;
                break;
            default:
                ANCS_LOG("ERRR\r\n");
                break;
        }
        
    }

//    *(str + str_len) = 0;
    if(msg_type == 0)
    {
        //ui_show_phone_call(3000,str);
    }
    else
    {
        //ui_show_chinese_str(msg_type,3000,str);
    }
}

bool	phone_type = 0;	//0:android 1:ios

void	set_phone_type(bool type)
{
	phone_type = type;
}
bool	get_phone_type(void)
{
	return phone_type;
}

/*********************************************************************
 * @fn      dis_gatt_msg_handler
 *
 * @brief   Device information gatt message handler.
 *
 *
 * @param   p_msg  - messages from GATT layer.
 *
 * @return  none.
 */
uint16_t ANCS_AMS_gatt_msg_handler(struct gatt_msg *p_msg)
{
    ANCS_LOG("ANCS_msg,evt:%d,att_idx[%d]\r\n",p_msg->msg_evt,p_msg->att_idx);
    switch(p_msg->msg_evt)
    {
        case GATTC_MSG_NTF_REQ:
        {
            if(p_msg->att_idx == ANCS_ATT_IDX_CTL_POINT)
            {
                ;
            }
            else if(p_msg->att_idx == ANCS_ATT_IDX_NTF_SRC)
            {
                ANCS_recv_ntf_src(p_msg->conn_idx,p_msg->param.gatt_data.p_msg_data,p_msg->param.gatt_data.msg_len);
            }
            else if(p_msg->att_idx == ANCS_ATT_IDX_DATA_SRC)
            {
                ANCS_recv_data_src(p_msg->conn_idx,p_msg->param.gatt_data.p_msg_data,p_msg->param.gatt_data.msg_len);
//                extern void user_ANCS_recv_data_src(uint8_t conidx,uint8_t *p_data, uint16_t len);
//                user_ANCS_recv_data_src(p_msg->conn_idx,p_msg->param.gatt_data.p_msg_data,p_msg->param.gatt_data.msg_len);
            }
        }  
        break;
        case GATTC_MSG_READ_IND:
        {
            ;
        }
        break;
        case GATTC_MSG_CMP_EVT:
        {

            ANCS_LOG("op:%d done\r\n",p_msg->param.gatt_op_cmp.operation);
            if(p_msg->param.gatt_op_cmp.operation == GATT_OP_PEER_SVC_REGISTERED)
            {
                ANCS_LOG("ANCS all svc reg\r\n");
                memcpy(ANCS_hdl_cache,p_msg->param.gatt_op_cmp.arg,12);
                show_utf8_hex((uint8_t *)ANCS_hdl_cache,12,1);
                
                if(memcmp((uint8_t *)(android_buf),(uint8_t *)(ANCS_hdl_cache),12)==0)
                {
					//android
					ANCS_LOG("phone type android\r\n");
					set_phone_type(0); 

                }else {
					//ios
					//if(ANCS_hdl_cache[3] == 0)//找到ams后不再发起配对请求
					{
						ANCS_LOG("phone type ios %d\r\n",gap_get_encrypted_status(p_msg->conn_idx));
						set_phone_type(1);
						//gap_security_req(p_msg->conn_idx);
						#if 1
						if(gap_get_encrypted_status(p_msg->conn_idx) == 0)  //已配对的情况不再重新发起
							gap_security_req(p_msg->conn_idx);
						else
                        {
                            ANCS_LOG("already encryption \r\n");
                            ancs_enable_src_notify(p_msg->conn_idx);
                        }
						#endif		
					}


                #if 0
				if(ANCS_hdl_cache[0] != 0)
				{
					gatt_client_enable_event_t ntf_enable;
		            ntf_enable.conidx = p_msg->conn_idx;
		            ntf_enable.client_id = ANCS_AMS_client_id;
		            ntf_enable.att_idx = ANCS_ATT_IDX_NTF_SRC;
		            gatt_client_enable_ntf_ind(&ntf_enable, ENABLE_TYPE_NTF);

		            ntf_enable.att_idx = ANCS_ATT_IDX_DATA_SRC;
		            gatt_client_enable_ntf_ind(&ntf_enable, ENABLE_TYPE_NTF);
				}
                #endif
				}
			}
        }
        break;
        default:
            break;
    }

    AMS_gatt_msg_handler(p_msg);

    return 0;
}

void	ancs_enable_src_notify(uint8_t condix)
{
    if(ANCS_hdl_cache[0] != 0)
    {
        ANCS_LOG("%s \r\n", __func__);

        gatt_client_enable_event_t ntf_enable;
        ntf_enable.conidx = condix;//p_msg->conn_idx;
        ntf_enable.client_id = ANCS_AMS_client_id;
        ntf_enable.att_idx = ANCS_ATT_IDX_NTF_SRC;
        gatt_client_enable_ntf_ind(&ntf_enable, ENABLE_TYPE_NTF);

        ntf_enable.att_idx = ANCS_ATT_IDX_DATA_SRC;
        gatt_client_enable_ntf_ind(&ntf_enable, ENABLE_TYPE_NTF);
    }
}

/*********************************************************************
 * @fn      batt_gatt_notify
 *
 * @brief   Send batt level notification to peer.
 *
 *
 * @param   conidx  - link idx.
 *          batt_level  - battery energy percentage.
 *
 * @return  none.
 */
void ANCS_gatt_write_cmd(uint8_t conidx,enum ancs_ams_att_idx att_idx,uint8_t *p_data, uint16_t len)
{
    struct gatt_client_write_param write;
    write.conidx = conidx;
    write.client_id = ANCS_AMS_client_id;
    write.att_idx = att_idx;
    write.p_data = p_data;
    write.data_len = len;
    gatt_client_write_cmd(&write);
}
void ANCS_gatt_write_req(uint8_t conidx,enum ancs_ams_att_idx att_idx,uint8_t *p_data, uint16_t len)
{
    struct gatt_client_write_param write;
    write.conidx = conidx;
    write.client_id = ANCS_AMS_client_id;
    write.att_idx = att_idx;
    write.p_data = p_data;
    write.data_len = len;
    gatt_client_write_req(&write);
}

void ANCS_gatt_read(uint8_t conidx,enum ancs_ams_att_idx att_idx)
{
    struct gatt_client_read_param read;
    read.conidx = conidx;
    read.client_id = ANCS_AMS_client_id;
    read.att_idx = att_idx;
    gatt_client_read(&read);
}

void ANCS_perform_ntf_act(uint8_t conidx, uint32_t notification_uid, enum ancs_act_id action_id)
{
    ANCS_LOG("enter\r\n");
    if(ANCS_hdl_cache[ANCS_ATT_IDX_CTL_POINT])
    {
        ANCS_LOG("ntf act\r\n");
        uint8_t rsp[12];
        uint8_t i = 0;
        rsp[i++] = ANCS_CMD_ID_PERFORM_NOTIFICATION_ACTION;   //cmd id
        *(uint32_t *)(rsp + i) = notification_uid;  //ntf_uid
        i+=4;
        rsp[i++] = action_id;

        ANCS_gatt_write_req(conidx,ANCS_ATT_IDX_CTL_POINT,rsp,i);
    }
}


/*********************************************************************
 * @fn      batt_gatt_add_service
 *
 * @brief   Simple Profile add GATT service function.
 *          Ìí¼ÓGATT serviceµ½ATTµÄÊý¾Ý¿âÀïÃæ¡£
 *
 * @param   None.
 *
 *
 * @return  None.
 */
void ANCS_AMS_gatt_add_client(void)
{
    gatt_client_t client;

    client.p_att_tb = ANCS_AMS_att_tb;
    client.att_nb = ANCS_AMS_ATT_IDX_MAX;
    client.gatt_msg_handler = ANCS_AMS_gatt_msg_handler;
    ANCS_AMS_client_id = gatt_add_client(&client);
    AMS_client_id = ANCS_AMS_client_id;
}





