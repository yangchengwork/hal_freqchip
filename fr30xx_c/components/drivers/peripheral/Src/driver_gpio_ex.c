/*
 ******************************************************************************
  * @file    driver_gpio_ex.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   GPIO module extend driver.
  *          This file provides firmware functions to manage the 
  *          QSPI/OSPI Pin Input/Output/Alternate Function.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

static volatile uint32_t QSPI_Output_Shadow = 0;
static volatile uint32_t OSPI_Output_Shadow = 0;

/************************************************************************************
 * @fn      gpioex_qspi_init
 *
 * @brief   Initializes the QSPI IO according to the specified 
 *          parameters in the GPIO_Init. 
 *          Pin can be select GPIO_PIN_0 ~ GPIO_PIN_9.
 *
 * @param   GPIO_Init: pointer to a GPIO_InitTypeDef structure that contains the
 *                     configuration information for the specified QSPI IO.
 */
__RAM_CODE void gpioex_qspi_init(GPIO_InitTypeDef *GPIO_Init)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Pin;

    volatile uint32_t *QSPI_Pull_EN;
    volatile uint32_t *QSPI_Pull_Select;
    volatile uint32_t *QSPI_FuncMux;
    volatile uint32_t *QSPI_OUTPUT_ENABLE;
    volatile uint32_t *QSPI_InputOpenCircuit;    
    
    /* Select the group register */
    QSPI_Pull_EN       = &(SYSTEM->QspiPadConfig.QSPI_PullEN);
    QSPI_Pull_Select   = &(SYSTEM->QspiPadConfig.QSPI_PullSelect);  
    QSPI_FuncMux       = &(SYSTEM->QspiPadConfig.QSPI_FuncMux);   
    QSPI_OUTPUT_ENABLE = &(SYSTEM->QspiOutEN);   
    QSPI_InputOpenCircuit = &(SYSTEM->QspiPadConfig.QSPI_InputOpenCircuit);    

    /* Configure Select pins */
    while ((GPIO_Init->Pin) >> lu32_Position != 0)
    {
        /* Get current pin position */
        lu32_Current_Pin = (GPIO_Init->Pin) & (1uL << lu32_Position);
        
        if (lu32_Current_Pin)
        {
            switch(GPIO_Init->Mode)
            {
                case GPIO_MODE_INPUT:
                {
                    *QSPI_InputOpenCircuit |= lu32_Current_Pin;
                    *QSPI_OUTPUT_ENABLE    |= lu32_Current_Pin;  
                }break;

                case GPIO_MODE_INPUT_HRS:
                {
                    *QSPI_InputOpenCircuit  &= ~lu32_Current_Pin;
                    *QSPI_OUTPUT_ENABLE     |=  lu32_Current_Pin;
                    /* High Resistance Mode does not pull up or down */
                    GPIO_Init->Pull = GPIO_NOPULL;
                }break;                
                
                case GPIO_MODE_OUTPUT_PP:
                {
                    *QSPI_InputOpenCircuit &= ~lu32_Current_Pin;                     
                    *QSPI_OUTPUT_ENABLE    &= ~lu32_Current_Pin;                 
                }break;
                
                case GPIO_MODE_AF_PP:
                {
                    *QSPI_InputOpenCircuit |= lu32_Current_Pin;                    
                    *QSPI_FuncMux = (*QSPI_FuncMux & ~(0x3 << (lu32_Position * 2))) | (GPIO_Init->Alternate << (lu32_Position * 2)); 
                }break; 
                
                default: break;
            } 
             
            /* GPIO Function */
            if (GPIO_Init->Mode & GPIO_MODE_IO_MASK) 
            {
                *QSPI_FuncMux = (*QSPI_FuncMux & ~(0x3 << (lu32_Position * 2))) | (0x03 << (lu32_Position * 2));
            }
            
            /* Set Pull UP/DOWN or NO Pull */
            if (GPIO_Init->Pull == GPIO_NOPULL) 
            {
                *QSPI_Pull_EN &= ~lu32_Current_Pin;
            }
            else if (GPIO_Init->Pull == GPIO_PULLUP) 
            {
                *QSPI_Pull_EN     |= lu32_Current_Pin;
                *QSPI_Pull_Select |= lu32_Current_Pin;
            }
            else if (GPIO_Init->Pull == GPIO_PULLDOWN) 
            {
                *QSPI_Pull_EN     |=  lu32_Current_Pin;
                *QSPI_Pull_Select &= ~lu32_Current_Pin;
            } 
        }
        
        lu32_Position++;
    }
}

/************************************************************************************
 * @fn      gpioex_ospi_init
 *
 * @brief   Initializes the OSPI IO according to the specified 
 *          parameters in the GPIO_Init.
 *          Pin can be select GPIO_PIN_0 ~ GPIO_PIN_14.
 *
 * @param   GPIO_Init: pointer to a GPIO_InitTypeDef structure that contains the
 *                     configuration information for the specified OSPI IO.
 */
__RAM_CODE void gpioex_ospi_init(GPIO_InitTypeDef *GPIO_Init)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Pin;

    volatile uint32_t *OSPI_Pull_EN;
    volatile uint32_t *OSPI_Pull_Select;
    volatile uint32_t *OSPI_FuncMux;
    volatile uint32_t *OSPI_OUTPUT_ENABLE;
    volatile uint32_t *OSPI_InputOpenCircuit;

    /* Select the group register */
    OSPI_Pull_EN       = &(SYSTEM->OspiPadConfig.OSPI_PullEN);
    OSPI_Pull_Select   = &(SYSTEM->OspiPadConfig.OSPI_PullSelect);  
    OSPI_FuncMux       = &(SYSTEM->OspiPadConfig.OSPI_FuncMux);   
    OSPI_OUTPUT_ENABLE = &(SYSTEM->OspiOutEN);       
    OSPI_InputOpenCircuit = &(SYSTEM->OspiPadConfig.OSPI_InputOpenCircuit); 

    /* Configure Select pins */
    while ((GPIO_Init->Pin) >> lu32_Position != 0)
    {
        /* Get current pin position */
        lu32_Current_Pin = (GPIO_Init->Pin) & (1uL << lu32_Position);
        
        if (lu32_Current_Pin)
        {
            switch(GPIO_Init->Mode)
            {
                case GPIO_MODE_INPUT:
                {
                    *OSPI_InputOpenCircuit |=  lu32_Current_Pin;
                    *OSPI_OUTPUT_ENABLE    |=  lu32_Current_Pin;
                }break;

                case GPIO_MODE_INPUT_HRS:
                {
                    *OSPI_InputOpenCircuit  &= ~lu32_Current_Pin;
                    *OSPI_OUTPUT_ENABLE     |=  lu32_Current_Pin;
                    /* High Resistance Mode does not pull up or down */
                    GPIO_Init->Pull = GPIO_NOPULL;
                }break;                 
                
                case GPIO_MODE_OUTPUT_PP:
                {
                    *OSPI_InputOpenCircuit &= ~lu32_Current_Pin;                    
                    *OSPI_OUTPUT_ENABLE    &= ~lu32_Current_Pin;                                        
                }break;
                
                case GPIO_MODE_AF_PP:
                {
                    *OSPI_InputOpenCircuit |=  lu32_Current_Pin;                    
                    *OSPI_FuncMux = (*OSPI_FuncMux & ~(0x3 << (lu32_Position * 2))) | (GPIO_Init->Alternate << (lu32_Position * 2)); 
                }break; 
                
                default: break;
            } 
             
            /* GPIO Function */
            if (GPIO_Init->Mode & GPIO_MODE_IO_MASK) 
            {
                *OSPI_FuncMux = (*OSPI_FuncMux & ~(0x3 << (lu32_Position * 2))) | (0x03 << (lu32_Position * 2));
            }
            
            /* Set Pull UP/DOWN or NO Pull */
            if (GPIO_Init->Pull == GPIO_NOPULL) 
            {
                *OSPI_Pull_EN &= ~lu32_Current_Pin;
            }
            else if (GPIO_Init->Pull == GPIO_PULLUP) 
            {
                *OSPI_Pull_EN     |= lu32_Current_Pin;
                *OSPI_Pull_Select |= lu32_Current_Pin;
            }
            else if (GPIO_Init->Pull == GPIO_PULLDOWN) 
            {
                *OSPI_Pull_EN     |=  lu32_Current_Pin;
                *OSPI_Pull_Select &= ~lu32_Current_Pin;
            } 
        }
        
        lu32_Position++;
    }
}

/************************************************************************************
 * @fn      gpioex_qspi_set_portpull
 *
 * @brief   set port pull
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_9, 1bit ~ 1Pin). @ref GPIO_pins
 * @param   fe_Pull: pull up/pull down/no pull @ref enum_Pull_t
 */
void gpioex_qspi_set_portpull(uint16_t fu16_Pin, enum_Pull_t fe_Pull)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Pin;

    volatile uint32_t *QSPI_Pull_EN;
    volatile uint32_t *QSPI_Pull_Select;
    
    /* Select the group register */    
    QSPI_Pull_EN       = &(SYSTEM->QspiPadConfig.QSPI_PullEN);
    QSPI_Pull_Select   = &(SYSTEM->QspiPadConfig.QSPI_PullSelect); 

    /* Configure Select pins */
    while (fu16_Pin >> lu32_Position != 0) 
    {
        /* Get current pin position */
        lu32_Current_Pin = fu16_Pin & (1uL << lu32_Position);
        
        if (lu32_Current_Pin) 
        {
            /* Set Pull UP or DOWN or NO */
            if (fe_Pull == GPIO_NOPULL) 
            {
                *QSPI_Pull_EN &= ~lu32_Current_Pin;
            }
            else if (fe_Pull == GPIO_PULLUP) 
            {
                *QSPI_Pull_EN     |= lu32_Current_Pin;
                *QSPI_Pull_Select |= lu32_Current_Pin;
            }
            else if (fe_Pull == GPIO_PULLDOWN) 
            {
                *QSPI_Pull_EN     |=  lu32_Current_Pin;
                *QSPI_Pull_Select &= ~lu32_Current_Pin;
            }
        }

        lu32_Position++;
    }
}

/************************************************************************************
 * @fn      gpioex_ospi_set_portpull
 *
 * @brief   set port pull
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_14, 1bit ~ 1Pin). @ref GPIO_pins
 * @param   fe_Pull: pull up/pull down/no pull @ref enum_Pull_t
 */
void gpioex_ospi_set_portpull(uint16_t fu16_Pin, enum_Pull_t fe_Pull)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Pin;

    volatile uint32_t *OSPI_Pull_EN;
    volatile uint32_t *OSPI_Pull_Select;
    
    /* Select the group register */    
    OSPI_Pull_EN       = &(SYSTEM->OspiPadConfig.OSPI_PullEN);
    OSPI_Pull_Select   = &(SYSTEM->OspiPadConfig.OSPI_PullSelect); 

    /* Configure Select pins */
    while (fu16_Pin >> lu32_Position != 0) 
    {
        /* Get current pin position */
        lu32_Current_Pin = fu16_Pin & (1uL << lu32_Position);
        
        if (lu32_Current_Pin) 
        {
            /* Set Pull UP or DOWN or NO */
            if (fe_Pull == GPIO_NOPULL) 
            {
                *OSPI_Pull_EN &= ~lu32_Current_Pin;
            }
            else if (fe_Pull == GPIO_PULLUP) 
            {
                *OSPI_Pull_EN     |= lu32_Current_Pin;
                *OSPI_Pull_Select |= lu32_Current_Pin;
            }
            else if (fe_Pull == GPIO_PULLDOWN) 
            {
                *OSPI_Pull_EN     |=  lu32_Current_Pin;
                *OSPI_Pull_Select &= ~lu32_Current_Pin;
            }
        }

        lu32_Position++;
    }
}

/************************************************************************************
 * @fn      gpioex_qspi_write_group
 *
 * @brief   write qspi io status, The unit is group.
 *
 * @param   fu16_GroupStatus: Group Status.(GPIO_PIN_0 ~ GPIO_PIN_9, 1bit ~ 1Pin)
 */
void gpioex_qspi_write_group(uint16_t fu16_GroupStatus)
{
    QSPI_Output_Shadow = fu16_GroupStatus;
    SYSTEM->QspiData = QSPI_Output_Shadow;
}

/************************************************************************************
 * @fn      gpioex_qspi_read_output_shadow
 *
 * @brief   read qspi io output shadow.
 *
 * @return  output shadow status.
 */
uint16_t gpioex_qspi_read_output_shadow(void)
{
    return QSPI_Output_Shadow;
}

/************************************************************************************
 * @fn      gpioex_ospi_write_group
 *
 * @brief   write ospi io status
 *
 * @param   fu16_GroupStatus: Group Status.(GPIO_PIN_0 ~ GPIO_PIN_14, 1bit ~ 1Pin)
 */
void gpioex_ospi_write_group(uint16_t fu16_GroupStatus)
{
    OSPI_Output_Shadow = fu16_GroupStatus;
    SYSTEM->OspiData = OSPI_Output_Shadow;
}

/************************************************************************************
 * @fn      gpioex_ospi_read_output_shadow
 *
 * @brief   read ospi io output shadow.
 *
 * @return  output shadow status.
 */
uint16_t gpioex_ospi_read_output_shadow(void)
{
    return OSPI_Output_Shadow;
}

/************************************************************************************
 * @fn      gpioex_qspi_write_pin
 *
 * @brief   write qspi io status, The unit is pin.
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_9, 1bit ~ 1Pin). @ref GPIO_pins
 * @param   fe_PinStatus: pin Status. @ref enum_PinStatus_t
 */
void gpioex_qspi_write_pin(uint16_t fu16_Pin, enum_PinStatus_t fe_PinStatus)
{  
    if (fe_PinStatus)
    {
        QSPI_Output_Shadow |= fu16_Pin;
        SYSTEM->QspiData = QSPI_Output_Shadow;
    }
    else
    {
        QSPI_Output_Shadow &= ~fu16_Pin;       
        SYSTEM->QspiData = QSPI_Output_Shadow;
    }
}

/************************************************************************************
 * @fn      gpioex_ospi_write_pin
 *
 * @brief   write ospi io status, The unit is pin.
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_14, 1bit ~ 1Pin). @ref GPIO_pins
 * @param   fe_PinStatus: pin Status. @ref enum_PinStatus_t
 */
void gpioex_ospi_write_pin(uint16_t fu16_Pin, enum_PinStatus_t fe_PinStatus)
{
    if (fe_PinStatus)
    {
        OSPI_Output_Shadow |= fu16_Pin;
        SYSTEM->OspiData = OSPI_Output_Shadow;
    }
    else
    {
        OSPI_Output_Shadow &= ~fu16_Pin;       
        SYSTEM->OspiData = OSPI_Output_Shadow;
    }
}

/************************************************************************************
 * @fn      gpioex_qspi_read_group
 *
 * @brief   read qspi io status, The unit is group.
 *          The status of the pin can be read only in input mode.
 *
 * @return  group status.(GPIO_PIN_0 ~ GPIO_PIN_9, 1bit ~ 1Pin)
 */
uint16_t gpioex_qspi_read_group(void)
{
    return SYSTEM->QspiData;
}

/************************************************************************************
 * @fn      gpioex_ospi_read_group
 *
 * @brief   read ospi io status, The unit is group.
 *          The status of the pin can be read only in input mode.
 *
 * @return  group status.(GPIO_PIN_0 ~ GPIO_PIN_14, 1bit ~ 1Pin)
 */
uint16_t gpioex_ospi_read_group(void)
{
    return SYSTEM->OspiData;
}

/************************************************************************************
 * @fn      gpioex_qspi_read_pin
 *
 * @brief   read qspi io status, The unit is pin.
 *          The status of the pin can be read only in input mode.
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_9, 1bit ~ 1Pin). @ref GPIO_pins
 * @return  pin status.
 */
enum_PinStatus_t gpioex_qspi_read_pin(uint16_t fu16_Pin)
{
    enum_PinStatus_t le_PinStatus;

    le_PinStatus = (SYSTEM->QspiData & fu16_Pin) ? GPIO_PIN_SET : GPIO_PIN_CLEAR;

    return le_PinStatus;
}

/************************************************************************************
 * @fn      gpioex_ospi_read_pin
 *
 * @brief   read ospi io status, The unit is pin.
 *          The status of the pin can be read only in input mode.
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_14, 1bit ~ 1Pin). @ref GPIO_pins
 * @return  pin status.
 */
enum_PinStatus_t gpioex_ospi_read_pin(uint16_t fu16_Pin)
{
    enum_PinStatus_t le_PinStatus;

    le_PinStatus = (SYSTEM->OspiData & fu16_Pin) ? GPIO_PIN_SET : GPIO_PIN_CLEAR;

    return le_PinStatus;
}

/************************************************************************************
 * @fn      gpioex_qspi_drive_current_config
 *
 * @brief   qspi io drive current config.
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_9, 1bit ~ 1Pin). @ref GPIO_pins
 * @param   fe_GPIO_Drive: Drive Current. @ref enum_GPIO_Drive_Current_t
 */
void gpioex_qspi_drive_current_config(uint16_t fu16_Pin, enum_GPIO_Drive_Current_t fe_GPIO_Drive)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Pin;

    /* Configure Select pins */
    while (fu16_Pin >> lu32_Position != 0) 
    {
        /* Get current pin position */
        lu32_Current_Pin = fu16_Pin & (1uL << lu32_Position);
        
        if (lu32_Current_Pin)
        {
            SYSTEM->QspiPadConfig.QSPI_DriveCfg = (SYSTEM->QspiPadConfig.QSPI_DriveCfg & ~(0x3 << (lu32_Position * 2))) | (fe_GPIO_Drive << (lu32_Position * 2));  
        }
        
        lu32_Position++;
    }
}

/************************************************************************************
 * @fn      gpioex_ospi_drive_current_config
 *
 * @brief   ospi io drive current config.
 *
 * @param   fu16_Pin: to select the Pin (GPIO_PIN_0 ~ GPIO_PIN_14, 1bit ~ 1Pin). @ref GPIO_pins
 * @param   fe_GPIO_Drive: Drive Current. @ref enum_GPIO_Drive_Current_t
 */
void gpioex_ospi_drive_current_config(uint16_t fu16_Pin, enum_GPIO_Drive_Current_t fe_GPIO_Drive)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Pin;

    /* Configure Select pins */
    while (fu16_Pin >> lu32_Position != 0) 
    {
        /* Get current pin position */
        lu32_Current_Pin = fu16_Pin & (1uL << lu32_Position);
        
        if (lu32_Current_Pin)
        {
            SYSTEM->OspiPadConfig.OSPI_DriveCfg = (SYSTEM->OspiPadConfig.OSPI_DriveCfg & ~(0x3 << (lu32_Position * 2))) | (fe_GPIO_Drive << (lu32_Position * 2));  
        }
        
        lu32_Position++;
    }
}
