/**
 * Copyright (c) 2019, Freqchip
 * 
 * All rights reserved.
 * 
 * 
 */
 
/*
 * INCLUDES (包含头文件)
 */
#include <stdio.h>
#include <string.h>
#include "gap_api.h"
#include "gatt_api.h"
#include "gatt_sig_uuid.h"

#include "simple_gatt_service.h"
#include "co_log.h"

/*
 * MACROS (宏定义)
 */

/*
 * CONSTANTS (常量定义)
 */
 
/*
	user application
*/
const uint8_t wrx_service_uuid[16] = {0xa1, 0x11, 0xb9, 0x58, 0x84, 0x17, 0x27, 0x94, 0x57, 0x46, 0xe6, 0xdf, 0xe0, 0x8a, 0x72, 0x75};

#define WRX_NOTIFY_CHAR_UUID 			0xa1, 0x11, 0xb9, 0x58, 0x84, 0x17, 0x27, 0x94, 0x57, 0x46, 0xe6, 0xdf, 0xe1, 0x8a, 0x72, 0x75 
#define LOG_NOTIFY_CHAR_UUID 			0xb7, 0xe4, 0x44, 0x2e, 0x8e, 0x6a, 0x7f, 0x97, 0xc3, 0x44, 0x4d, 0x8e, 0x10, 0x12, 0x0c, 0x3e

#define WRX_BLE_OTA_CHAR_UUID 			0x35, 0x45, 0x28, 0x45, 0xb4, 0xf5, 0x38, 0x88, 0x2e, 0x48, 0xa9, 0xc8, 0xe3, 0xec, 0x31, 0x43

#define WRX_NOTIFY_CHAR_LEN  500
#define WRX_CHAR3_VALUE_LEN  500




// Simple GATT Profile Service UUID: 0xFFF0
const uint8_t sp_svc_uuid[] = UUID16_ARR(SP_SVC_UUID);

/******************************* Characteristic 1 defination *******************************/
// Characteristic 1 UUID: 0xFFF1
// Characteristic 1 data 
#define SP_CHAR1_VALUE_LEN  10
uint8_t sp_char1_value[SP_CHAR1_VALUE_LEN] = {0};
#define SP_CHAR1_CCC_LEN   2
uint8_t sp_char1_ccc[SP_CHAR1_CCC_LEN] = {0};
// Characteristic 1 User Description
#define SP_CHAR1_DESC_LEN   17
const uint8_t sp_char1_desc[SP_CHAR1_DESC_LEN] = "Characteristic 1";

/******************************* Characteristic 2 defination *******************************/
// Characteristic 2 UUID: 0xFFF2
// Characteristic 2 data 
#define SP_CHAR2_VALUE_LEN  20
uint8_t sp_char2_value[SP_CHAR2_VALUE_LEN] = {0};
#define SP_CHAR2_CCC_LEN   2
uint8_t sp_char2_ccc[SP_CHAR2_CCC_LEN] = {0};
// Characteristic 2 User Description
#define SP_CHAR2_DESC_LEN   17
const uint8_t sp_char2_desc[SP_CHAR2_DESC_LEN] = "Characteristic 2";

/******************************* Characteristic 3 defination *******************************/
// Characteristic 3 UUID: 0xFFF3
// Characteristic 3 data 
#define SP_CHAR3_VALUE_LEN  30
uint8_t sp_char3_value[SP_CHAR3_VALUE_LEN] = {0};
// Characteristic 3 client characteristic configuration
#define SP_CHAR3_CCC_LEN   2
uint8_t sp_char3_ccc[SP_CHAR3_CCC_LEN] = {0};

// Characteristic 3 User Description
#define SP_CHAR3_DESC_LEN   17
const uint8_t sp_char3_desc[SP_CHAR3_DESC_LEN] = "Characteristic 3";

/******************************* Characteristic 4 defination *******************************/
// Characteristic 4 UUID: 0xFFF4
// Characteristic 4 data 
#define SP_CHAR4_VALUE_LEN  40
uint8_t sp_char4_value[SP_CHAR4_VALUE_LEN] = {0};
#define SP_CHAR4_CCC_LEN   2
uint8_t sp_char4_ccc[SP_CHAR4_CCC_LEN] = {0};
// Characteristic 4 User Description
#define SP_CHAR4_DESC_LEN   17
const uint8_t sp_char4_desc[SP_CHAR4_DESC_LEN] = "Characteristic 4";

/******************************* Characteristic 5 defination *******************************/
// Characteristic 5 UUID: 0xFFF5
uint8_t sp_char5_uuid[UUID_SIZE_2] =
{ 
  LO_UINT16(SP_CHAR5_UUID), HI_UINT16(SP_CHAR5_UUID)
};
// Characteristic 5 data 
#define SP_CHAR5_VALUE_LEN  50
uint8_t sp_char5_value[SP_CHAR5_VALUE_LEN] = {0};
// Characteristic 5 User Description
#define SP_CHAR5_DESC_LEN   17
const uint8_t sp_char5_desc[SP_CHAR5_DESC_LEN] = "Characteristic 5";

/*
 * TYPEDEFS (类型定义)
 */

/*
 * GLOBAL VARIABLES (全局变量)
 */
uint8_t sp_svc_id = 0;
uint8_t sp_conidx = 0;


/*
 * LOCAL VARIABLES (本地变量)
 */
static gatt_service_t simple_profile_svc;

/*********************************************************************
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

const gatt_attribute_t simple_profile_att_table[SP_IDX_NB] =
{
	// Simple gatt Service Declaration
	[SP_IDX_SERVICE]					=   {
												{ UUID_SIZE_2, UUID16_ARR(GATT_PRIMARY_SERVICE_UUID) },     /* UUID */
												GATT_PROP_READ,                                             /* Permissions */
												UUID_SIZE_16,                                                /* Max size of the value */     /* Service UUID size in service declaration */
												(uint8_t*)wrx_service_uuid,								/* Value of the attribute */    /* Service UUID value in service declaration */
											},
	// Characteristic 1 Declaration           
	[SP_IDX_CHAR1_DECLARATION]          = {
												{ UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },           /* UUID */
												GATT_PROP_READ,                                             /* Permissions */
												0,                                                          /* Max size of the value */
												NULL,                                                       /* Value of the attribute */
											},
	// Characteristic 1 Value                  
	[SP_IDX_CHAR1_VALUE]                =   {
												{ UUID_SIZE_16, WRX_NOTIFY_CHAR_UUID },                 /* UUID */
												GATT_PROP_NOTI | GATT_PROP_WRITE_CMD,									/* Permissions */
												 WRX_NOTIFY_CHAR_LEN,                                         /* Max size of the value */
												NULL,                                                       /* Value of the attribute */    /* Can assign a buffer here, or can be assigned in the application by user */
											},	
	// Characteristic 1 User Description
	[SP_IDX_CHAR1_USER_DESCRIPTION]     =   {
												{ UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID) },      /* UUID */
												GATT_PROP_READ|GATT_PROP_WRITE_CMD,							/* Permissions */
												0,														/* Max size of the value */
												NULL,														/* Value of the attribute */
											},

	// Characteristic 3 Declaration
	[SP_IDX_CHAR3_DECLARATION]          =   {
												{ UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },           /* UUID */
												GATT_PROP_READ,                                             /* Permissions */
												0,                                                          /* Max size of the value */
												NULL,                                                       /* Value of the attribute */
											},
	// Characteristic 3 Value
	[SP_IDX_CHAR3_VALUE]                =   {
												{ UUID_SIZE_16, LOG_NOTIFY_CHAR_UUID },                 /* UUID */
												GATT_PROP_NOTI,                                            /* Permissions */
												WRX_CHAR3_VALUE_LEN,                                         /* Max size of the value */
												NULL,                                                       /* Value of the attribute */    /* Can assign a buffer here, or can be assigned in the application by user */
											},
	// Characteristic 3 User Description
	[SP_IDX_CHAR3_USER_DESCRIPTION]     =   {
												{ UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID) },      /* UUID */
												GATT_PROP_READ | GATT_PROP_WRITE_CMD,							/* Permissions */
												0,														/* Max size of the value */
												NULL,														/* Value of the attribute */
											},
	
	// Characteristic 4 Declaration
	[SP_IDX_CHAR4_DECLARATION]          =   {
												{ UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },           /* UUID */
												GATT_PROP_NOTI,	/* Permissions */
												0,                                                          /* Max size of the value */
												NULL,                                                       /* Value of the attribute */
											},
	// Characteristic 4 Value
	[SP_IDX_CHAR4_VALUE]                =   {
												{ UUID_SIZE_16, WRX_BLE_OTA_CHAR_UUID },                 /* UUID */
												GATT_PROP_NOTI | GATT_PROP_WRITE_REQ | GATT_PROP_WRITE_CMD,                                            /* Permissions */
												WRX_CHAR3_VALUE_LEN,                                         /* Max size of the value */
												NULL,                                                       /* Value of the attribute */    /* Can assign a buffer here, or can be assigned in the application by user */
											},
	// Characteristic 4 User Description
	[SP_IDX_CHAR4_USER_DESCRIPTION]     =   {
												{ UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID) },      /* UUID */
												GATT_PROP_READ | GATT_PROP_WRITE_CMD,							/* Permissions */
												0,														/* Max size of the value */
												NULL,														/* Value of the attribute */
											},  
};

//norify api
void Send_Data2App_notificate(uint8_t att_idx,uint8_t *p_data, uint16_t len)
{
    uint8_t enable = 0;
    if(att_idx == SP_IDX_CHAR1_VALUE)
	{
	    if(sp_char1_ccc[0])
	    {
	        enable = 1;
	    }
	}
	//else if(att_idx == SP_IDX_CHAR2_VALUE)
	//{
	//    if(sp_char2_ccc[0])
	//    {
	//        enable = 1;
	//    }
	//}
	else if(att_idx == SP_IDX_CHAR3_VALUE)
	{
	    if(sp_char3_ccc[0])
	    {
	        enable = 1;
	    }
	}
	//else if(att_idx == SP_IDX_CHAR4_VALUE)
	//{
	//    if(sp_char4_ccc[0])
	//    {
	//        enable = 1;
	//    }
	//}

	if(enable)
	{
		struct gatt_send_event ntf;
		ntf.conidx = sp_conidx;
		ntf.svc_id = sp_svc_id;
		ntf.att_idx = att_idx ;
		ntf.data_len = len;
		ntf.p_data = p_data;
		gatt_notification(&ntf);

		#if 1 //printf send notify data
		printf("att_idx%d:",att_idx);
		for(uint8_t i = 0; i < len; i++)
		{
			printf("0x%02x,",p_data[i]);
		}
		printf("\r\n");
		#endif
	}
	else{
		printf("character disable!\r\n");
	}
}




/*********************************************************************
 * @fn      sp_gatt_read_cb
 *
 * @brief   Simple Profile user application handles read request in this callback.
 *			应用层在这个回调函数里面处理读的请求。
 *
 * @param   p_read  - the pointer to read buffer. NOTE: It's just a pointer from lower layer, please create the buffer in application layer.
 *					  指向读缓冲区的指针。 请注意这只是一个指针，请在应用程序中分配缓冲区. 为输出函数, 因此为指针的指针.
 *          len     - the pointer to the length of read buffer. Application to assign it.
 *                    读缓冲区的长度，用户应用程序去给它赋值.
 *          att_idx - index of the attribute value in it's attribute table.
 *					  Attribute的偏移量.
 *
 * @return  读请求的长度.
 */
static void sp_gatt_read_cb(uint8_t *p_read, uint16_t *len, uint16_t att_idx)
{
    switch (att_idx)
    {
        case SP_IDX_CHAR1_VALUE:
            for (int i = 0; i < SP_CHAR1_VALUE_LEN; i++)
                sp_char1_value[i] = sp_char1_value[0] + i + 1;
            memcpy(p_read, sp_char1_value, SP_CHAR1_VALUE_LEN);
            *len = SP_CHAR1_VALUE_LEN;
        break;

		#if 0
        case SP_IDX_CHAR2_VALUE:
            for (int i = 0; i < SP_CHAR2_VALUE_LEN; i++)
                sp_char2_value[i] = sp_char2_value[0] + i + 1;
            memcpy(p_read, sp_char2_value, SP_CHAR2_VALUE_LEN);
            *len = SP_CHAR2_VALUE_LEN;
       break;
		#endif
        
#if 0
        case SP_IDX_CHAR3_CFG:
            *len = 2;
            memcpy(p_read, sp_char3_ccc, 2);
        break;
#endif /* if 0. 2020-12-24 15:43:27 joe */
        
		#if 0
        case SP_IDX_CHAR5_VALUE:
            for (int i = 0; i < SP_CHAR5_VALUE_LEN; i++)
                sp_char5_value[i] = sp_char3_value[0] + i + 1;
            memcpy(p_read, sp_char5_value, SP_CHAR5_VALUE_LEN);
           *len = SP_CHAR5_VALUE_LEN;
        break;
		#endif
        
        default:
        break;
    }
    
	LOG_INFO("Read request: len: %d  value: 0x%x 0x%x \r\n", *len, (p_read)[0], (p_read)[*len-1]);
    
}

/*********************************************************************
 * @fn      sp_gatt_write_cb
 *
 * @brief   Simple Profile user application handles write request in this callback.
 *			应用层在这个回调函数里面处理写的请求。
 *
 * @param   write_buf   - the buffer for write
 *			              写操作的数据.
 *					  
 *          len         - the length of write buffer.
 *                        写缓冲区的长度.
 *          att_idx     - index of the attribute value in it's attribute table.
 *					      Attribute的偏移量.
 *
 * @return  写请求的长度.
 */
static void sp_gatt_write_cb(uint8_t *write_buf, uint16_t len, uint16_t att_idx)
{
	
    printf("Write request: att index is %d, len is %d\r\n",att_idx, len);
    printf("recv data is:");
    for(uint32_t i=0;i<len;i++)
    {
        printf("0x%02x ", write_buf[i]);
    }
    printf("\r\n");
	
	if(att_idx == SP_IDX_CHAR1_VALUE)
	{	
		/*
			add user gatt write_cb
		*/
		printf("char1 value recevice \r\n");
	}	
		
	uint16_t uuid = BUILD_UINT16( simple_profile_att_table[att_idx].uuid.p_uuid[0], simple_profile_att_table[att_idx].uuid.p_uuid[1] );
	printf("uuid %x \r\n",uuid);
	if (uuid == GATT_CLIENT_CHAR_CFG_UUID)
	{
		printf("Notification status changed\r\n");
		#if 1
        if (att_idx == SP_IDX_CHAR1_USER_DESCRIPTION)
        {
            sp_char1_ccc[0] = write_buf[0];
            sp_char1_ccc[1] = write_buf[1];
            printf("Char1 ccc: 0x%x 0x%x \r\n", sp_char1_ccc[0], sp_char1_ccc[1]);	
		}		
		#endif
	}
	
	//Send_Data2App_notificate(SP_IDX_CHAR1_VALUE,(uint8_t *)"222",3);	//if notify enable, send test data to app
}

/*********************************************************************
 * @fn      sp_gatt_msg_handler
 *
 * @brief   Simple Profile callback funtion for GATT messages. GATT read/write
 *			operations are handeled here.
 *
 * @param   p_msg       - GATT messages from GATT layer.
 *
 * @return  uint16_t    - Length of handled message.
 */
static uint16_t sp_gatt_msg_handler(struct gatt_msg *p_msg)
{
    switch(p_msg->msg_evt)
    {
        case GATTS_MSG_READ_REQ:
            printf("read request: att idx is %d\r\n", p_msg->att_idx);
            sp_gatt_read_cb((uint8_t *)(p_msg->param.gatt_data.p_msg_data), &(p_msg->param.gatt_data.msg_len), p_msg->att_idx);
            break;
        
        case GATTS_MSG_WRITE_REQ:
			 sp_gatt_write_cb((uint8_t*)(p_msg->param.gatt_data.p_msg_data), (p_msg->param.gatt_data.msg_len), p_msg->att_idx);       
            break;
            
        default:
            break;
    }
    sp_conidx = p_msg->conn_idx;
    return p_msg->param.gatt_data.msg_len;
}

/*********************************************************************
 * @fn      sp_gatt_add_service
 *
 * @brief   Simple Profile add GATT service function.
 *			添加GATT service到ATT的数据库里面。
 *
 * @param   None. 
 *        
 *
 * @return  None.
 */
void sp_gatt_add_service(void)
{
	simple_profile_svc.p_att_tb = simple_profile_att_table;
	simple_profile_svc.att_nb = SP_IDX_NB;
	simple_profile_svc.gatt_msg_handler = sp_gatt_msg_handler;
	
	sp_svc_id = gatt_add_service(&simple_profile_svc);
}
