/*
  ******************************************************************************
  * @file    trim_fr30xx.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2023
  * @brief   Config Chip analog/digit/RF using Chip Probing(CP) and 
             Final Test(FT) trim parameters.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"
#include "crc32.h"

//#define DEFAULT_VALUE_DLDO              0x04
//#define DEFAULT_VALUE_PKVDD             0x04    // target:0.9v
//#define DEFAULT_VALUE_SRAMPKVDD         0x01    // target: 0.75v

#define PKVDD_SLEEP_VOL                 1250    //1250
#define PKVDD_WAKEUP_VOL                1050
#define PKVDDRAM_SLEEP_VOL              850

#define DEFAULT_VALUE_PKVDD             (PKVDD_SLEEP_VOL/50)     // =target_v*20 
#define PKVDD_PKVDDRAM_DIFF             ((PKVDD_SLEEP_VOL - PKVDDRAM_SLEEP_VOL) / 50)
#define PKVDD_CONFIG_DIFF               ((PKVDD_SLEEP_VOL - PKVDD_WAKEUP_VOL) / 50)
#define DEFAULT_VALUE_PKVDDRAM          (PKVDDRAM_SLEEP_VOL/50)

static uint8_t pkvdd_wakeup_cfg, pkvdd_sleep_cfg;
static struct_ADC_Cal_Param_t ADC_Cal_Param;
static struct_FT_Trim_t       FT_Trim_Param = {0};
static uint8_t CP_trim_version = 0;

/*********************************************************************
 * @fn      cp_byte_alignment_conversion
 *
 * @brief   Different compilers have exceptions for byte alignment, 
 *          so split each field and fill in the value
 *
 * @param   none.
 * @return  none.
 */
static void triming_byte_alignment_conversion(struct_CP_Trim_t *eFuseParam)
{
    uint32_t TempArry[3] = {0};
    
    __SYSTEM_EFUSE0_CLK_ENABLE();
    __SYSTEM_EFUSE1_CLK_ENABLE();
    eFuse_siso_read(TempArry);

    if(TempArry[0])
    {
        eFuseParam->BBG_CODE    = (TempArry[0] & 0x1f );
        eFuseParam->SBG_CODE    = (TempArry[0] & 0x3e0) >> 5;
        eFuseParam->PKVDD_CODE  = (TempArry[0] & 0x3c00   )   >> 10;
        eFuseParam->SPKVDD_CODE = (TempArry[0] & 0x3c000  )   >> 14;
        eFuseParam->OSCLDO_CODE = (TempArry[0] & 0x3c0000  )  >> 18;
        eFuseParam->ADLDO_CODE  = (TempArry[0] & 0x1c00000 )   >> 22;
        eFuseParam->DDLDO_CODE  = (TempArry[0] & 0xe000000)   >> 25;
        eFuseParam->WaferID     = ((TempArry[1] & 0xfffff) << 4)
                                   |((TempArry[0] & 0xf0000000 ) >> 28);   
        eFuseParam->CPEVEN_CKECK= (TempArry[1] & 0x100000) >> 20;
    }
    
    if(TempArry[1])
    {
        eFuseParam->SBUCK_CODE  = (TempArry[1] &   0xe00000) >> 21;
        eFuseParam->IOLDO1      = ((TempArry[1] & 0xf000000) >> 24); 
        eFuseParam->IOLDO2      = (TempArry[1] & 0xf0000000)  >> 28;
        eFuseParam->CHG_GT      = (TempArry[2] & 0x07);
        eFuseParam->CHG_TER     = (TempArry[2] & 0xe00)   >> 9; 
        eFuseParam->CHG_SMP     = (TempArry[2] & 0x7000)   >> 12; 
        eFuseParam->FTEVEN_CHECK= (TempArry[2] & 0x8000)   >> 15; 
    }
    
    memset(TempArry,0,sizeof(uint32_t));
    eFuse_pipo_read(EFUSE_PIPO0, 0x00, (uint8_t*)TempArry);

    if(TempArry[0])
    {       
        eFuseParam->IOBUCK_CODE        = TempArry[0] & 0x0f ; 
        eFuseParam->AULDO_CODE        = (TempArry[0] & 0xf0) >> 4;  
    }

    __SYSTEM_EFUSE0_CLK_DISABLE();
    __SYSTEM_EFUSE1_CLK_DISABLE();
}

/*********************************************************************
 * @fn      trim_cp_config
 *
 * @brief   Config Chip analog/digit/RF using Chip Probing(CP) trim parameters.
 *
 * @param   none.
 * @return  none.
 */
void trim_cp_config(void)
{
    struct_CP_Trim_t eFuseParam = {0};
    uint8_t pkvdd_cfg, pkvddram_cfg;
    
    triming_byte_alignment_conversion(&eFuseParam);
    
//    printf("cp efuse param: \r\n");
//    printf("waferID: %x,sbg:%x,bbg:%x\r\n",eFuseParam.WaferID,eFuseParam.SBG_CODE,eFuseParam.BBG_CODE);
//    printf("pkvdd: 0x%x, srampkvdd: 0x%x\r\n",eFuseParam.PKVDD_CODE,eFuseParam.SPKVDD_CODE);

    if (eFuseParam.WaferID != 0) {
        ool_write(PMU_REG_SBG_CFG,      (ool_read(PMU_REG_SBG_CFG)      &0xE0)  | eFuseParam.SBG_CODE);
        
        pkvdd_cfg = DEFAULT_VALUE_PKVDD - eFuseParam.PKVDD_CODE;

        if(DEFAULT_VALUE_PKVDDRAM < eFuseParam.SPKVDD_CODE) {
            pkvddram_cfg = 0;
        }
        else{
            pkvddram_cfg = DEFAULT_VALUE_PKVDDRAM - eFuseParam.SPKVDD_CODE;
        }
        if (pkvddram_cfg > 0xf) {
            pkvddram_cfg = 0xf;
        }
        
        pkvdd_wakeup_cfg = pkvdd_cfg > PKVDD_CONFIG_DIFF ? (pkvdd_cfg - PKVDD_CONFIG_DIFF) : 0;
        if (pkvdd_wakeup_cfg > 0xf) {
            pkvdd_wakeup_cfg = 0xf;
        }
        
        if (pkvdd_cfg > 0xf) {
            pkvdd_cfg = 0xf;
        }                
        pkvdd_sleep_cfg = pkvdd_cfg;
        ool_write(PMU_REG_PKVDD_CTRL, (ool_read(PMU_REG_PKVDD_CTRL) & 0x0f) | (pkvdd_wakeup_cfg << 4));
        ool_write(PMU_REG_PKVDDH_CTRL_1,(ool_read(PMU_REG_PKVDDH_CTRL_1) & 0xf0) | pkvddram_cfg);
        
        CP_trim_version = (eFuseParam.WaferID & 0xc00000) >> 22;
//        printf("pkvddram:%x,pk wk=%x,pk sleep=%x\r\n",pkvddram_cfg,pkvdd_wakeup_cfg,pkvdd_sleep_cfg);
    }
    else{
        /* set PKVDD to 1.05v */
        ool_write(PMU_REG_PKVDD_CTRL, 0xba);
        /* set PKVDDH to 1.1v, set RAMPKVDD to 0.85v */
        ool_write(PMU_REG_PKVDDH_CTRL_1, 0x37);
    }
    
    if(eFuseParam.SBUCK_CODE != 0){
        ool_write(PMU_REG_SYSBUCK_CTRL_0,   (ool_read(PMU_REG_SYSBUCK_CTRL_0)&0xF8)|
        ((ool_read(PMU_REG_SYSBUCK_CTRL_0)&0x07) + (3 - eFuseParam.SBUCK_CODE)));
        
        ool_write(PMU_REG_IOLDO1_CTRL_0,   (ool_read(PMU_REG_IOLDO1_CTRL_0)&0xF0)|
        ((ool_read(PMU_REG_IOLDO1_CTRL_0)&0x0F) + (2 - eFuseParam.IOLDO1)));
        
        ool_write(PMU_REG_IOLDO2_CTRL_0,   (ool_read(PMU_REG_IOLDO2_CTRL_0)&0xF0)|
        ((ool_read(PMU_REG_IOLDO2_CTRL_0)&0x0F) + (4 - eFuseParam.IOLDO2)));
        
        ool_write(PMU_REG_CHG_GT_CTL,   (ool_read(PMU_REG_CHG_GT_CTL)&0xF8)|
        ((ool_read(PMU_REG_CHG_GT_CTL)&0x07) + (2 - eFuseParam.CHG_GT)));

        ool_write(PMU_REG_IOBUCK_CTRL_0,   (ool_read(PMU_REG_IOBUCK_CTRL_0)&0xF0)|
        ((ool_read(PMU_REG_IOBUCK_CTRL_0)&0x0F) + (5 - eFuseParam.IOBUCK_CODE)));
        
        ool_write(PMU_REG_CHG_CFG_CB,   (ool_read(PMU_REG_CHG_CFG_CB)&0x8F)|((4 - eFuseParam.CHG_TER) 
        <= -2 ? 0x00 : ((ool_read(PMU_REG_CHG_CFG_CB)&0x07) + (4 - eFuseParam.CHG_TER))<<4));
        
        ool_write(PMU_REG_CHG_SMP_CTL,   (ool_read(PMU_REG_CHG_SMP_CTL)&0xF8)|((2 - eFuseParam.CHG_SMP) 
        >= 1 ? 0x07 : ((ool_read(PMU_REG_CHG_SMP_CTL)&0x07) + (2 - eFuseParam.CHG_SMP)) ));
    }
}

/*********************************************************************
 * @fn      trim_cp_get_version
 *
 * @brief   get cp trim version.
 *
 * @param   none.
 * @return  version.
 */
__RAM_CODE uint8_t trim_cp_get_version(void)
{
    return CP_trim_version;
}

/*********************************************************************
 * @fn      trim_ft_config
 *
 * @brief   Config Chip analog/digit/RF using Final Test(FT) trim parameters.
 *
 * @param   none.
 * @return  none.
 */
void trim_ft_config(void)
{
    uint16_t VERSION;
    uint32_t FT_CRC;
	
    flash_OTP_read(QSPI0,0x1000,sizeof(FT_Trim_Param),(uint8_t*)&FT_Trim_Param);
    
    VERSION = FT_Trim_Param.u16_Version;

    if (VERSION == 0xA002)
    {
        FT_CRC = crc32(0x00000000, (void *)&FT_Trim_Param, (uint32_t)&FT_Trim_Param.Param.V2.u32_crc - (uint32_t)&FT_Trim_Param);
        if(FT_CRC != FT_Trim_Param.Param.V2.u32_crc) {
            goto clean;
        }

        /* config SBG */
        ool_write(PMU_REG_SBG_CFG,FT_Trim_Param.u16_ioldo);
    }
    else if(VERSION == 0xA003)
    {
        FT_CRC = crc32(0x00000000, (void *)&FT_Trim_Param, (uint32_t)&FT_Trim_Param.Param.V3.u32_crc - (uint32_t)&FT_Trim_Param);
        if(FT_CRC != FT_Trim_Param.Param.V3.u32_crc) {
            goto clean;
        }

        /* config SBG */
        ool_write(PMU_REG_SBG_CFG,FT_Trim_Param.u16_ioldo);

        ADC_Cal_Param.u16_slopeA = FT_Trim_Param.u16_SlopeA;
        ADC_Cal_Param.u16_slopeB = FT_Trim_Param.u16_SlopeB;
        ADC_Cal_Param.s32_constantA = FT_Trim_Param.s32_ConstantA;
        ADC_Cal_Param.s32_constantB = FT_Trim_Param.s32_ConstantB;
    }
    else {
        goto clean;
    }
    
    return;
    
clean:
    memset((uint8_t*)&FT_Trim_Param, 0, sizeof(FT_Trim_Param));
}

/*********************************************************************
 * @fn      trim_get_adc_cal_param
 *
 * @brief   get adc ft calibration param.
 */
struct_ADC_Cal_Param_t *trim_get_adc_cal_param(void)
{
    return &ADC_Cal_Param;
}

/*********************************************************************
 * @fn      trim_get_txpower_param
 *
 * @brief   get Tx power triming param.
 */
int16_t trim_get_txpower_param()
{ 
    if (FT_Trim_Param.u16_Version == 0xA003) {
        return FT_Trim_Param.Param.V3.s16_BasePower;
    }
    else {
        return 0x7fff;
    }
}

/************************************************************************************
 * @fn      trim_get_ft_version 
 *
 * @brief   Get the ft version ID
 *
 * @return  version ID
 */
uint16_t trim_get_ft_version(void)
{
    return FT_Trim_Param.u16_Version;
}

/************************************************************************************
 * @fn      triming_get_device_uuid
 *
 * @brief   Get the unique 5-bytes device ID
 *
 * @param   buffer - buffer used to store unique device ID
 * @param   length - IN: how many bytes is available in buffer
 *                   OUT: how many valid bytes is returned
 */
void trim_get_device_uuid(uint8_t *buffer, uint8_t *length)
{
    uint32_t TempArry[3] = {0};
    
    __SYSTEM_EFUSE0_CLK_ENABLE();
    __SYSTEM_EFUSE1_CLK_ENABLE();
    eFuse_siso_read(TempArry);
    __SYSTEM_EFUSE0_CLK_DISABLE();
    __SYSTEM_EFUSE1_CLK_DISABLE();
    
    if ((TempArry[1] & 0xfe000) == 0) {
        if (*length >= 5) {
            flash_gen_uuid(QSPI0, buffer, FLASH_GEN_UUID_TYPE_40);
            *length = 5;
        }
        else if (*length >= 2) {
            flash_gen_uuid(QSPI0, buffer, FLASH_GEN_UUID_TYPE_12);
            *length = 2;
        }
        else {
            *length = 0;
        }
    }
    else {
        if (*length < 2) {
            *length = 0;
        }
        else {
            buffer[0] = (TempArry[0] & 0x3fc00000) >> 22;
            buffer[1] = ((TempArry[1] & 0x3f) << 2)
                                   | ((TempArry[0] & 0xc0000000 ) >> 30);
            if (*length >= 5) {
                uint8_t _buffer[2];
                flash_gen_uuid(QSPI0, _buffer, FLASH_GEN_UUID_TYPE_12);
                
                buffer[2] = (TempArry[1] & 0x3fc0) >> 6;
                buffer[3] = (TempArry[1] & 0x3c000) >> 14;
                buffer[3] |= ((_buffer[0] & 0x0f) << 4);
                buffer[4] = ((_buffer[0] & 0xf0) >> 4) | ((_buffer[1] & 0x0f) << 4);
                *length = 5;
            }
            else {
                *length = 2;
            }
        }
    }
}

/************************************************************************************
 * @fn      trim_reconfig_pkvdd 
 *
 * @brief   used to reconfig pkvdd after wake up or before enter sleep mode.
 *
 * @param   wakeup: true - after wake up; false - before enter sleep.
 *          stage: different processing in different stage
 */
__RAM_CODE void trim_reconfig_pkvdd(bool wakeup, uint8_t stage)
{
    static uint8_t org_app_ldo_cfg = 0, org_dsp_dldo_cfg = 0, org_pkvddram_cfg = 0;
    if (wakeup) {
        ool_write(PMU_REG_APP_DLDO_CTRL, org_app_ldo_cfg);  //set dldo to normal value
        ool_write(PMU_REG_DSP_DLDO_CTRL, org_dsp_dldo_cfg); //set dldo to normal value
        ool_write(PMU_REG_PKVDD_CTRL, (ool_read(PMU_REG_PKVDD_CTRL) & 0x0f) | (pkvdd_wakeup_cfg << 4));
    }
    else {
        if (stage == 0) {
            /* configure system clock to 24MHz */
            __SYSTEM_MCU_CLK_SELECT_COREH();
            
            ool_write(PMU_REG_PKVDD_CTRL, (ool_read(PMU_REG_PKVDD_CTRL) & 0x0f) | (pkvdd_sleep_cfg << 4));
            
            if ((org_pkvddram_cfg == 0) && (org_app_ldo_cfg == 0) && (org_dsp_dldo_cfg == 0)) {
                org_pkvddram_cfg = ool_read(PMU_REG_PKVDDH_CTRL_1);
                org_app_ldo_cfg = ool_read(PMU_REG_APP_DLDO_CTRL);
                org_dsp_dldo_cfg = ool_read(PMU_REG_DSP_DLDO_CTRL);
            }
            
            ool_write(PMU_REG_PKVDDH_CTRL_1, org_pkvddram_cfg | 0x0f);
            ool_write(PMU_REG_APP_DLDO_CTRL, (org_app_ldo_cfg & 0xf8) | 0x00);//set dldo to 1.0v
            ool_write(PMU_REG_DSP_DLDO_CTRL, (org_dsp_dldo_cfg & 0xf8) | 0x00);//set dldo to 1.0v
        }
        else if (stage == 1) {
            ool_write(PMU_REG_APP_DLDO_CTRL, (org_app_ldo_cfg & 0xf8) | 0x02);//set dldo to 1.1v
            ool_write(PMU_REG_DSP_DLDO_CTRL, (org_dsp_dldo_cfg & 0xf8) | 0x02);//set dldo to 1.1v
            ool_write(PMU_REG_PKVDDH_CTRL_1, org_pkvddram_cfg);
        }
    }
}
