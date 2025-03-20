/**
 * Copyright (c) 2019, Freqchip
 * 
 * All rights reserved.
 * 
 * 
 */

#ifndef SP_GATT_PROFILE_H
#define SP_GATT_PROFILE_H

/*
 * INCLUDES (����ͷ�ļ�)
 */
#include <stdio.h>
#include <string.h>
#include "gap_api.h"
#include "gatt_api.h"
#include "gatt_sig_uuid.h"


/*
 * MACROS (�궨��)
 */

/*
 * CONSTANTS (��������)
 */
// Simple Profile attributes index. 
#if 1
enum
{
    SP_IDX_SERVICE,

    SP_IDX_CHAR1_DECLARATION,
    SP_IDX_CHAR1_VALUE,
    SP_IDX_CHAR1_USER_DESCRIPTION,

    SP_IDX_CHAR3_DECLARATION,
    SP_IDX_CHAR3_VALUE,
    SP_IDX_CHAR3_USER_DESCRIPTION,

    SP_IDX_CHAR4_DECLARATION,
    SP_IDX_CHAR4_VALUE,
    SP_IDX_CHAR4_USER_DESCRIPTION,
      
    SP_IDX_NB,
};
#else
enum
{
    SP_IDX_SERVICE,

    SP_IDX_CHAR1_DECLARATION,
    SP_IDX_CHAR1_VALUE,
    SP_IDX_CHAR1_CFG,
    SP_IDX_CHAR1_USER_DESCRIPTION,

    SP_IDX_CHAR2_DECLARATION,
    SP_IDX_CHAR2_VALUE,
    //SP_IDX_CHAR2_CFG,     
    SP_IDX_CHAR2_USER_DESCRIPTION,

    SP_IDX_CHAR3_DECLARATION,
    SP_IDX_CHAR3_VALUE,
    SP_IDX_CHAR3_CFG,
    SP_IDX_CHAR3_USER_DESCRIPTION,

    SP_IDX_CHAR4_DECLARATION,
    SP_IDX_CHAR4_VALUE,
    //SP_IDX_CHAR4_CFG,
    SP_IDX_CHAR4_USER_DESCRIPTION,
    
    SP_IDX_CHAR5_DECLARATION,
    SP_IDX_CHAR5_VALUE,
    SP_IDX_CHAR5_USER_DESCRIPTION,
    
    SP_IDX_NB,
};
#endif
// Simple GATT Profile Service UUID
#define SP_SVC_UUID              0xFFF0

#define SP_CHAR1_UUID            0xFFF1
#define SP_CHAR2_UUID            0xFFF2
#define SP_CHAR3_UUID            0xFFF3
#define SP_CHAR4_UUID            0xFFF4
#define SP_CHAR5_UUID            0xFFF5


/*
 * TYPEDEFS (���Ͷ���)
 */

/*
 * GLOBAL VARIABLES (ȫ�ֱ���)
 */
extern const gatt_attribute_t simple_profile_att_table[];

/*
 * LOCAL VARIABLES (���ر���)
 */


/*
 * PUBLIC FUNCTIONS (ȫ�ֺ���)
 */
/*********************************************************************
 * @fn      sp_gatt_add_service
 *
 * @brief   Simple Profile add GATT service function.
 *			����GATT service��ATT�����ݿ����档
 *
 * @param   None. 
 *        
 *
 * @return  None.
 */
void sp_gatt_add_service(void);
void Send_Data2App_notificate(uint8_t att_idx,uint8_t *p_data, uint16_t len);


#endif







