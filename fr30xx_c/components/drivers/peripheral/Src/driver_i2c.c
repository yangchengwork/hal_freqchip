/*
  ******************************************************************************
  * @file    driver_i2c.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   I2C module driver.
  *          This file provides firmware functions to manage the 
  *          Inter-Integrated Circuit bus (I2C) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      i2c_IRQHandler
 *
 * @brief   Handle I2C interrupt request.
 *
 * @param   hi2c: I2C handle.
 */
void i2c_IRQHandler(I2C_HandleTypeDef *hi2c)
{
    /* Is INT_TX_ABRT enable */
    if (i2c_is_int_enable(hi2c, INT_TX_ABRT))
    {
        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            hi2c->b_RxBusy = false;
            hi2c->b_TxBusy = false;
            
            __I2C_DISABLE(hi2c->I2Cx);
            i2c_int_disable(hi2c, INT_RX_FULL);
            i2c_int_disable(hi2c, INT_TX_ABRT);
            i2c_int_disable(hi2c, INT_TX_EMPTY);
        }
    }
    
    /* Master */
    if (hi2c->Init.I2C_Mode & I2C_MASK_MASTER) 
    {
        /* Is INT_TX_EMPTY enabled */
        if (i2c_is_int_enable(hi2c, INT_TX_EMPTY))
        {
            if (i2c_get_int_status(hi2c, INT_TX_EMPTY)) 
            {
                i2c_clear_int_status(hi2c, INT_TX_EMPTY);

                while (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
                {
                    if (hi2c->u32_TxCount < hi2c->u32_TxSize) 
                    {
                        hi2c->I2Cx->DATA_CMD = hi2c->p_TxData[hi2c->u32_TxCount++];
                    }
                    else 
                    {
                        hi2c->I2Cx->DATA_CMD = hi2c->p_TxData[hi2c->u32_TxCount++] | CMD_STOP;

                        hi2c->b_TxBusy = false;

                        i2c_int_disable(hi2c, INT_TX_EMPTY);
                        i2c_int_disable(hi2c, INT_TX_ABRT);
                        break;
                    }

                    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
                    {
                        i2c_clear_int_status(hi2c, INT_TX_ABRT);

                        __I2C_DISABLE(hi2c->I2Cx);

                        hi2c->b_TxBusy = false;

                        i2c_int_disable(hi2c, INT_TX_EMPTY);
                        i2c_int_disable(hi2c, INT_TX_ABRT);

                        break;
                    }
                } 
            }
        }

        /* Is INT_RX_FULL enabled */
        if (i2c_is_int_enable(hi2c, INT_RX_FULL))
        {
            if (i2c_get_int_status(hi2c, INT_RX_FULL)) 
            {
                i2c_clear_int_status(hi2c, INT_RX_FULL);

                while (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
                {
                    hi2c->p_RxData[hi2c->u32_RxCount++] = hi2c->I2Cx->DATA_CMD & 0xFF;

                    if (hi2c->u32_RxCount >= hi2c->u32_RxSize)
                    {
                        hi2c->b_RxBusy = false;

                        i2c_int_disable(hi2c, INT_RX_FULL);
                    }
                }

                while ((!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx)) && hi2c->b_TxBusy == true)
                {
                    if (hi2c->u32_TxCount < hi2c->u32_RxSize - 1) 
                    {
                        hi2c->I2Cx->DATA_CMD = CMD_READ;

                        hi2c->u32_TxCount++;
                    }
                    else 
                    {
                        hi2c->I2Cx->DATA_CMD = CMD_READ | CMD_STOP;

                        hi2c->b_TxBusy = false;

                        break;  
                    }
                }
            }
        }
    }
    /* Slave */
    else
    {
        /* Is INT_RD_REQ enabled */
        if(i2c_is_int_enable(hi2c, INT_RD_REQ))
        {
            if(i2c_get_int_status(hi2c, INT_RD_REQ))
            {
                i2c_clear_int_status(hi2c, INT_RD_REQ);

                i2c_int_disable(hi2c, INT_RD_REQ);

                while(!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
                {
                    if(hi2c->u32_TxCount < hi2c->u32_TxSize)
                    {
                        hi2c->I2Cx->DATA_CMD = hi2c->p_TxData[hi2c->u32_TxCount++];
                    }
                    else
                    {
                        hi2c->b_TxBusy = false;
                        
                        break;
                    }
                    
                    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
                    {
                        break;
                    }
                }
                
                if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
                {
                    i2c_clear_int_status(hi2c, INT_TX_ABRT);
                    
                    hi2c->b_TxBusy = false;

                    __I2C_DISABLE(hi2c->I2Cx);
                }
                else
                {
                    if(hi2c->u32_TxCount < hi2c->u32_TxSize)
                    {
                        i2c_int_enable(hi2c, INT_TX_EMPTY);
                        i2c_int_enable(hi2c, INT_TX_ABRT);                       
                    } 
                }
            }   
        }
        
        /* Is INT_TX_EMPTY enabled */
        if (i2c_is_int_enable(hi2c, INT_TX_EMPTY))
        {
            if (i2c_get_int_status(hi2c, INT_TX_EMPTY)) 
            {
                i2c_clear_int_status(hi2c, INT_TX_EMPTY);

                while (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
                {
                    if (hi2c->u32_TxCount < hi2c->u32_TxSize) 
                    {
                        hi2c->I2Cx->DATA_CMD = hi2c->p_TxData[hi2c->u32_TxCount++];
                    }
                    else 
                    {
                        hi2c->b_TxBusy = false;
                        
                        i2c_int_disable(hi2c, INT_TX_EMPTY);

                        break;
                    }
                    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
                    {
                        i2c_clear_int_status(hi2c, INT_TX_ABRT);
                        
                        hi2c->b_TxBusy = false;

                        __I2C_DISABLE(hi2c->I2Cx);
                        i2c_int_disable(hi2c, INT_TX_EMPTY);
                        i2c_int_disable(hi2c, INT_TX_ABRT);

                        break;
                    }               
                }
            }
        }

        /* Is INT_RX_FULL enabled */
        if (i2c_is_int_enable(hi2c, INT_RX_FULL))
        {
            if (i2c_get_int_status(hi2c, INT_RX_FULL)) 
            {
                i2c_clear_int_status(hi2c, INT_RX_FULL);

                while (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
                {
                    hi2c->p_RxData[hi2c->u32_RxCount++] = hi2c->I2Cx->DATA_CMD & 0xFF;

                    if (hi2c->u32_RxCount >= hi2c->u32_RxSize)
                    {
                        hi2c->b_RxBusy = false;
                        
                        i2c_int_disable(hi2c, INT_RX_FULL);
                        __I2C_DISABLE(hi2c->I2Cx);
                        
                        break;
                    }
                }
            }
        }                
    }
}

/************************************************************************************
 * @fn      i2c_init
 *
 * @brief   Initialize the I2C according to the specified parameters in the struct_I2CInit_t
 *
 * @param   hi2c: I2C handle.
 */
void i2c_init(I2C_HandleTypeDef *hi2c)
{
    uint32_t lu32_TempValue;

    __I2C_DISABLE(hi2c->I2Cx);

    /* Master */
    if (hi2c->Init.I2C_Mode & I2C_MASK_MASTER) 
    {
        hi2c->I2Cx->CTRL.SLAVE_DISABLE = 1;
        hi2c->I2Cx->CTRL.MASTER_MODE   = 1;
        hi2c->I2Cx->CTRL.SPEED         = 2;

        hi2c->I2Cx->TAR.ADDR_MASTER_10BIT = (hi2c->Init.I2C_Mode & I2C_MASK_10BIT) ? 1 : 0;
        hi2c->I2Cx->TAR.SPECIAL   = 0;
        hi2c->I2Cx->TAR.DEVICE_ID = 0;
    }
    /* Slave */
    else 
    {
        hi2c->I2Cx->CTRL.SLAVE_DISABLE = 0;
        hi2c->I2Cx->CTRL.MASTER_MODE   = 0;
        hi2c->I2Cx->CTRL.SPEED         = 2;

        hi2c->I2Cx->CTRL.ADDR_SLAVE_10BIT = (hi2c->Init.I2C_Mode & I2C_MASK_10BIT) ? 1 : 0;

        hi2c->I2Cx->SAR = hi2c->Init.Slave_Address >> 1;
    }


    /* Rate config */
    if (hi2c->Init.I2C_Mode & I2C_MASK_MASTER) 
    {
        /* SCL_HCNT Minimum limit */
        if (hi2c->Init.SCL_HCNT < 6)
        {
            hi2c->I2Cx->FS_SCL_HCNT = 6;
        }
        else 
        {
            hi2c->I2Cx->FS_SCL_HCNT = hi2c->Init.SCL_HCNT;
        }
        /* SCL_LCNT Minimum limit */
        if (hi2c->Init.SCL_LCNT < 8)
        {
            hi2c->I2Cx->FS_SCL_LCNT = 8;
        }
        else 
        {
            hi2c->I2Cx->FS_SCL_LCNT = hi2c->Init.SCL_LCNT;
        }


        lu32_TempValue = hi2c->Init.SCL_LCNT / 2;

        if (lu32_TempValue > 0xFF) 
        {
            hi2c->I2Cx->SDA_SETUP = 0xFF;
            hi2c->I2Cx->SDA_HOLD.SDA_TX_HOLD = hi2c->Init.SCL_LCNT - 0xFF;

            /* Maximum limit */
            if (hi2c->Init.SCL_LCNT - 0xFF > 0xFFFF) 
            {
                hi2c->I2Cx->SDA_HOLD.SDA_TX_HOLD = 0xFFFF;
            }
        }
        else 
        {
            hi2c->I2Cx->SDA_SETUP = lu32_TempValue;
            /* Minimum limit */
            if (lu32_TempValue < 8) 
            {
                hi2c->I2Cx->SDA_SETUP = 8;
            }

            hi2c->I2Cx->SDA_HOLD.SDA_TX_HOLD = lu32_TempValue;
            /* Minimum limit */
            if (lu32_TempValue < 5) 
            {
                hi2c->I2Cx->SDA_HOLD.SDA_TX_HOLD = 1;
            }
        }
    }
    else 
    {
        hi2c->I2Cx->FS_SCL_HCNT = 6;
        hi2c->I2Cx->FS_SCL_LCNT = 8;

        hi2c->I2Cx->SDA_SETUP = 8;
        hi2c->I2Cx->SDA_HOLD.SDA_TX_HOLD = 1;
    }
    /* Rate config end */


    /* Default configuration */
    hi2c->I2Cx->CTRL.STOP_DET_IF_MASTER_ACTIVE = 1;
    hi2c->I2Cx->CTRL.STOP_DET_IF_ADDR_ESSED    = 1;
    hi2c->I2Cx->CTRL.RX_FIFO_FULL_HLD_CTRL     = 1;
    hi2c->I2Cx->CTRL.TX_EMPTY_CTRL             = 1;
    hi2c->I2Cx->CTRL.RESTART_EN                = 1;
    /* nonblocking */
    hi2c->I2Cx->ENABLE.TX_CMD_BLOCK = 0;
    /* Disable all interrupt */
    hi2c->I2Cx->INT_MASK = 0;
}

/************************************************************************************
 * @fn      i2c_master_transmit
 *
 * @brief   master send an amount of data in blocking mode.
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_master_transmit(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint8_t *fp_Data, uint32_t fu32_Size)
{
    if (fu32_Size == 0) 
        return false;

    __I2C_DISABLE(hi2c->I2Cx);

    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_ENABLE(hi2c->I2Cx);

    while (fu32_Size - 1) 
    {
        if (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
        {
            hi2c->I2Cx->DATA_CMD = *fp_Data++;
            
            fu32_Size--;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            return false;
        }
    }

    /* Last byte with stop */
    while (__I2C_IS_TxFIFO_FULL(hi2c->I2Cx));
    hi2c->I2Cx->DATA_CMD = *fp_Data++ | CMD_STOP;

    while (!__I2C_IS_TxFIFO_EMPTY(hi2c->I2Cx));
    while (__I2C_IS_BUSY(hi2c->I2Cx));

    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
    {
        i2c_clear_int_status(hi2c, INT_TX_ABRT);

        __I2C_DISABLE(hi2c->I2Cx);

        return false;
    }

    return true;
}

/************************************************************************************
 * @fn      i2c_master_receive
 *
 * @brief   master receive an amount of data in blocking mode.
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_master_receive(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint8_t *fp_Data, uint32_t fu32_Size)
{
    uint32_t lu32_RxCount = fu32_Size;
    
    if (fu32_Size == 0) 
        return false;

    __I2C_DISABLE(hi2c->I2Cx);

    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_ENABLE(hi2c->I2Cx);

    while (fu32_Size - 1) 
    {
        if (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
        {
            hi2c->I2Cx->DATA_CMD = CMD_READ;

            fu32_Size--;
        }

        while (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
        {
            *fp_Data++ = hi2c->I2Cx->DATA_CMD & 0xFF;

            lu32_RxCount--;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            return false;
        }
    }

    /* Last byte with stop */
    while (__I2C_IS_TxFIFO_FULL(hi2c->I2Cx));
    hi2c->I2Cx->DATA_CMD = CMD_READ | CMD_STOP;

    while (lu32_RxCount) 
    {
        if (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
        {
            *fp_Data++ = hi2c->I2Cx->DATA_CMD & 0xFF;

            lu32_RxCount--;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            return false;
        }
    }

    while(__I2C_IS_BUSY(hi2c->I2Cx));
    
    return true;
}

/************************************************************************************
 * @fn      i2c_master_transmit_IT
 *
 * @brief   master send an amount of data in interrupt mode.
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_master_transmit_IT(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint8_t *fp_Data, uint32_t fu32_Size)
{
    if (fu32_Size == 0)  return false;
    if (hi2c->b_TxBusy)  return false;
    if (hi2c->b_RxBusy)  return false;

    while(__I2C_IS_BUSY(hi2c->I2Cx)); 
    
    __I2C_DISABLE(hi2c->I2Cx);
    
    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_TxFIFO_THRESHOLD_LEVEL(hi2c->I2Cx, 16);

    __I2C_ENABLE(hi2c->I2Cx);

    hi2c->u32_TxSize  = fu32_Size - 1;
    hi2c->u32_TxCount = 0;
    hi2c->p_TxData = fp_Data;
    hi2c->b_TxBusy = true;

    i2c_clear_int_status(hi2c, INT_TX_ABRT);
    i2c_clear_int_status(hi2c, INT_TX_EMPTY);

    while (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
    {
        if (hi2c->u32_TxCount < hi2c->u32_TxSize) 
        {
            hi2c->I2Cx->DATA_CMD = hi2c->p_TxData[hi2c->u32_TxCount++];
        }
        else 
        {
            hi2c->I2Cx->DATA_CMD = hi2c->p_TxData[hi2c->u32_TxCount++] | CMD_STOP;

            hi2c->b_TxBusy = false;
            return true;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            hi2c->b_TxBusy = false;
            return false;
        }
    }

    while(__I2C_IS_TxFIFO_FULL(hi2c->I2Cx));

    /* DevAddress NACK */
    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
    {
        i2c_clear_int_status(hi2c, INT_TX_ABRT);

        __I2C_DISABLE(hi2c->I2Cx);

        hi2c->b_TxBusy = false;
        return false;
    }

    i2c_int_enable(hi2c, INT_TX_EMPTY);
    i2c_int_enable(hi2c, INT_TX_ABRT);  
    return true;
}

/************************************************************************************
 * @fn      i2c_master_receive_IT
 *
 * @brief   master receive an amount of data in interrupt mode.
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_master_receive_IT(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint8_t *fp_Data, uint32_t fu32_Size)
{
    if (fu32_Size == 0)  return false;
    if (hi2c->b_TxBusy)  return false;
    if (hi2c->b_RxBusy)  return false;

    __I2C_DISABLE(hi2c->I2Cx);

    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_RxFIFO_THRESHOLD_LEVEL(hi2c->I2Cx, 0);

    __I2C_ENABLE(hi2c->I2Cx);

    hi2c->u32_RxSize  = fu32_Size;
    hi2c->u32_RxCount = 0;
    hi2c->u32_TxCount = 0;
    hi2c->p_RxData = fp_Data;
    hi2c->b_RxBusy = true;
    hi2c->b_TxBusy = true;

    i2c_clear_int_status(hi2c, INT_TX_ABRT);
    i2c_clear_int_status(hi2c, INT_RX_FULL);

    while (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
    {
        if (hi2c->u32_TxCount < hi2c->u32_RxSize - 1) 
        {
            hi2c->I2Cx->DATA_CMD = CMD_READ;

            hi2c->u32_TxCount++;
        }
        else 
        {
            hi2c->I2Cx->DATA_CMD = CMD_READ | CMD_STOP;
            hi2c->b_TxBusy = false;

            break;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            hi2c->b_RxBusy = false;
            hi2c->b_TxBusy = false;
            return false;
        }
    }
    i2c_int_enable(hi2c, INT_RX_FULL);
    
    return true;
}

/************************************************************************************
 * @fn      i2c_slave_transmit
 *
 * @brief   slave send an amount of data in blocking mode..
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_slave_transmit(I2C_HandleTypeDef *hi2c, uint8_t *fp_Data, uint32_t fu32_Size)
{
    __I2C_ENABLE(hi2c->I2Cx);

    if (fu32_Size == 0) 
        return false;
    
    while(!(i2c_get_int_status(hi2c, INT_RD_REQ)));
    i2c_clear_int_status(hi2c, INT_RD_REQ);
    
    while (fu32_Size) 
    {
        if (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
        {
            hi2c->I2Cx->DATA_CMD = *fp_Data++;  
            
            fu32_Size--;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);
            
            __I2C_DISABLE(hi2c->I2Cx);
            
            return false;
        }
    }

    while (!__I2C_IS_TxFIFO_EMPTY(hi2c->I2Cx));
    while(__I2C_IS_BUSY(hi2c->I2Cx));
    
    __I2C_DISABLE(hi2c->I2Cx);
    
    return true;
}

/************************************************************************************
 * @fn      i2c_slave_receive
 *
 * @brief   slave receive an amount of data in blocking mode.
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_slave_receive(I2C_HandleTypeDef *hi2c, uint8_t *fp_Data, uint32_t fu32_Size)
{
    __I2C_ENABLE(hi2c->I2Cx);
    
    while (fu32_Size) 
    {
        if (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
        {
            *fp_Data++ = hi2c->I2Cx->DATA_CMD & 0xFF;

            fu32_Size--;
        }

		if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);
            
            __I2C_DISABLE(hi2c->I2Cx);
            
            return false;
		}
    }

    __I2C_DISABLE(hi2c->I2Cx);
    
    return true;  
}

/************************************************************************************
 * @fn      i2c_slave_transmit_IT
 *
 * @brief   slave send an amount of data in interrupt mode.
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_slave_transmit_IT(I2C_HandleTypeDef *hi2c, uint8_t *fp_Data, uint32_t fu32_Size)
{
    if (fu32_Size == 0) return false;
    if (hi2c->b_TxBusy) return false;
    if (hi2c->b_RxBusy) return false;
    
    __I2C_TxFIFO_THRESHOLD_LEVEL(hi2c->I2Cx, 16);
    
     while(__I2C_IS_BUSY(hi2c->I2Cx)); 
    
    __I2C_ENABLE(hi2c->I2Cx);
    
    hi2c->u32_TxSize  = fu32_Size;
    hi2c->u32_TxCount = 0;
    hi2c->p_TxData = fp_Data;
    hi2c->b_TxBusy = true;

    i2c_clear_int_status(hi2c, INT_TX_ABRT);
    i2c_clear_int_status(hi2c, INT_TX_EMPTY);
    i2c_clear_int_status(hi2c, INT_RD_REQ);
    
    i2c_int_enable(hi2c,INT_RD_REQ);
       
    return true;
}

/************************************************************************************
 * @fn      i2c_slave_receive_IT
 *
 * @brief   slave receive an amount of data in interrupt mode.
 *
 * @param   hi2c: I2C handle.
 */
bool i2c_slave_receive_IT(I2C_HandleTypeDef *hi2c, uint8_t *fp_Data, uint32_t fu32_Size)
{
    if (fu32_Size == 0)  return false;
    if (hi2c->b_TxBusy)  return false;
    if (hi2c->b_RxBusy)  return false;
    
    while(__I2C_IS_BUSY(hi2c->I2Cx));
    
    i2c_clear_int_status(hi2c, INT_TX_ABRT);
    i2c_clear_int_status(hi2c, INT_RX_FULL);
    
    __I2C_ENABLE(hi2c->I2Cx);   
    
    __I2C_RxFIFO_THRESHOLD_LEVEL(hi2c->I2Cx, 0);
    
    hi2c->u32_RxSize  = fu32_Size;
    hi2c->u32_RxCount = 0;
    hi2c->p_RxData = fp_Data;
    hi2c->b_RxBusy = true;
    
    i2c_int_enable(hi2c, INT_RX_FULL);
    
    return true;
}

/************************************************************************************
 * @fn      i2c_memory_write
 *
 * @brief   i2c memory write.
 */
bool i2c_memory_write(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint16_t fu16_MemAddress, uint8_t *fp_Data, uint32_t fu32_Size)
{
    if (fu32_Size == 0) 
        return false;
        
    __I2C_DISABLE(hi2c->I2Cx);

    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_ENABLE(hi2c->I2Cx);

    hi2c->I2Cx->DATA_CMD = (fu16_MemAddress >> 8) & 0xFF;
    hi2c->I2Cx->DATA_CMD = fu16_MemAddress & 0xFF;

    while (!__I2C_IS_TxFIFO_EMPTY(hi2c->I2Cx));

    /* DevAddress NACK */
    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
    {
        i2c_clear_int_status(hi2c, INT_TX_ABRT);

        __I2C_DISABLE(hi2c->I2Cx);

        return false;
    }

    while (fu32_Size - 1) 
    {
        if (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
        {
            hi2c->I2Cx->DATA_CMD = *fp_Data++;
            
            fu32_Size--;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            return false;
        }
    }

    /* Last byte with stop */
    while (__I2C_IS_TxFIFO_FULL(hi2c->I2Cx));
    hi2c->I2Cx->DATA_CMD = *fp_Data++ | CMD_STOP;

    while(__I2C_IS_BUSY(hi2c->I2Cx));

    while(i2c_memory_is_busy(hi2c, fu16_DevAddress));

    return true;
}

/************************************************************************************
 * @fn      i2c_sensor_write
 *
 * @brief   i2c sensor write, fu8_AddressLength: The value can be 8, 16, or 32.
 */
bool i2c_sensor_write(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint32_t fu32_RegAddress, uint8_t fu8_AddressLength, uint8_t *fp_Data, uint32_t fu32_Size)
{
    if (fu32_Size == 0) 
        return false;
        
    __I2C_DISABLE(hi2c->I2Cx);

    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_ENABLE(hi2c->I2Cx);
    
    if(fu8_AddressLength == 8)
    {
        hi2c->I2Cx->DATA_CMD = fu32_RegAddress & 0xFF; 
    }
    else if(fu8_AddressLength == 16)
    {
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 8) & 0xFF;         
        hi2c->I2Cx->DATA_CMD = fu32_RegAddress & 0xFF; 
    } 
    else 
    {
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 24) & 0xFF;
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 16) & 0xFF;        
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 8) & 0xFF;          
        hi2c->I2Cx->DATA_CMD = fu32_RegAddress & 0xFF;         
    }
    
    while (!__I2C_IS_TxFIFO_EMPTY(hi2c->I2Cx));
    
    while (fu32_Size - 1) 
    {
        if (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
        {
            hi2c->I2Cx->DATA_CMD = *fp_Data++;
            
            fu32_Size--;
        }

        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            return false;
        }
    }

    /* Last byte with stop */
    while (__I2C_IS_TxFIFO_FULL(hi2c->I2Cx));
    hi2c->I2Cx->DATA_CMD = *fp_Data++ | CMD_STOP;

    while(__I2C_IS_BUSY(hi2c->I2Cx));

    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
    {
        i2c_clear_int_status(hi2c, INT_TX_ABRT);
    
        __I2C_DISABLE(hi2c->I2Cx);
    
        return false;
    }    
    while(i2c_memory_is_busy(hi2c, fu16_DevAddress));

    return true;
}

/************************************************************************************
 * @fn      i2c_memory_read
 *
 * @brief   i2c memory read.
 */
bool i2c_memory_read(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint16_t fu16_MemAddress, uint8_t *fp_Data, uint32_t fu32_Size)
{
    uint32_t lu32_RxCount = fu32_Size;

    if (fu32_Size == 0) 
        return false;

    __I2C_DISABLE(hi2c->I2Cx);
    
    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_ENABLE(hi2c->I2Cx);

    hi2c->I2Cx->DATA_CMD = (fu16_MemAddress >> 8) & 0xFF;
    hi2c->I2Cx->DATA_CMD = fu16_MemAddress & 0xFF; 
    
    while (!__I2C_IS_TxFIFO_EMPTY(hi2c->I2Cx));

    /* DevAddress NACK */
    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
    {
        i2c_clear_int_status(hi2c, INT_TX_ABRT);

        __I2C_DISABLE(hi2c->I2Cx);

        return false;
    }

    if (fu32_Size > 1) 
    {
        hi2c->I2Cx->DATA_CMD = CMD_RESTART | CMD_READ;

        while (fu32_Size - 2) 
        {
            if (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
            {
                hi2c->I2Cx->DATA_CMD = CMD_READ;

                fu32_Size--;
            }

            while (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
            {
                *fp_Data++ = hi2c->I2Cx->DATA_CMD & 0xFF;

                lu32_RxCount--;
            }

            if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
            {
                i2c_clear_int_status(hi2c, INT_TX_ABRT);

                __I2C_DISABLE(hi2c->I2Cx);

                return false;
            }
        }

        /* Last byte with stop */
        while (__I2C_IS_TxFIFO_FULL(hi2c->I2Cx));
        hi2c->I2Cx->DATA_CMD = CMD_READ | CMD_STOP;
    }
    else 
    {
        hi2c->I2Cx->DATA_CMD = CMD_RESTART | CMD_READ | CMD_STOP;
    }
    
    while (lu32_RxCount) 
    {
        if (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
        {
            *fp_Data++ = hi2c->I2Cx->DATA_CMD & 0xFF;

            lu32_RxCount--;
        }
    }

    while(__I2C_IS_BUSY(hi2c->I2Cx));
    
    return true;
}

/************************************************************************************
 * @fn      i2c_sensor_read
 *
 * @brief   i2c sensor read, fu8_AddressLength: The value can be 8, 16, or 32.
 */
bool i2c_sensor_read(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress, uint32_t fu32_RegAddress, uint8_t fu8_AddressLength, uint8_t *fp_Data, uint32_t fu32_Size)
{
    uint32_t lu32_RxCount = fu32_Size;

    if (fu32_Size == 0) 
        return false;

    __I2C_DISABLE(hi2c->I2Cx);
    
    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_ENABLE(hi2c->I2Cx);

    if(fu8_AddressLength == 8)
    {
        hi2c->I2Cx->DATA_CMD = fu32_RegAddress & 0xFF; 
    }
    else if(fu8_AddressLength == 16)
    {
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 8) & 0xFF;         
        hi2c->I2Cx->DATA_CMD = fu32_RegAddress & 0xFF; 
    } 
    else 
    {
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 24) & 0xFF;
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 16) & 0xFF;        
        hi2c->I2Cx->DATA_CMD = (fu32_RegAddress >> 8) & 0xFF;          
        hi2c->I2Cx->DATA_CMD = fu32_RegAddress & 0xFF;         
    }
    
    while (!__I2C_IS_TxFIFO_EMPTY(hi2c->I2Cx));

    if (fu32_Size > 1) 
    {
        hi2c->I2Cx->DATA_CMD = CMD_RESTART | CMD_READ;

        while (fu32_Size - 2) 
        {
            if (!__I2C_IS_TxFIFO_FULL(hi2c->I2Cx))
            {
                hi2c->I2Cx->DATA_CMD = CMD_READ;

                fu32_Size--;
            }

            while (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
            {
                *fp_Data++ = hi2c->I2Cx->DATA_CMD & 0xFF;

                lu32_RxCount--;
            }

            if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
            {
                i2c_clear_int_status(hi2c, INT_TX_ABRT);

                __I2C_DISABLE(hi2c->I2Cx);

                return false;
            }
        }

        /* Last byte with stop */
        while (__I2C_IS_TxFIFO_FULL(hi2c->I2Cx));
        hi2c->I2Cx->DATA_CMD = CMD_READ | CMD_STOP;
    }
    else 
    {
        hi2c->I2Cx->DATA_CMD = CMD_RESTART | CMD_READ | CMD_STOP;
    }   

    while (lu32_RxCount) 
    {
        if (!__I2C_IS_RxFIFO_EMPTY(hi2c->I2Cx)) 
        {
            *fp_Data++ = hi2c->I2Cx->DATA_CMD & 0xFF;

            lu32_RxCount--;
        }
        
        if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
        {
            i2c_clear_int_status(hi2c, INT_TX_ABRT);

            __I2C_DISABLE(hi2c->I2Cx);

            return false;
        }        
    }  

    while(__I2C_IS_BUSY(hi2c->I2Cx));

    return true;
}

/************************************************************************************
 * @fn      i2c_memory_is_busy
 *
 * @brief   i2c memory is busy.
 */
bool i2c_memory_is_busy(I2C_HandleTypeDef *hi2c, uint16_t fu16_DevAddress)
{
    __I2C_DISABLE(hi2c->I2Cx);

    hi2c->I2Cx->TAR.TAR = fu16_DevAddress >> 1;

    __I2C_ENABLE(hi2c->I2Cx);

    hi2c->I2Cx->DATA_CMD = 0x00;
    hi2c->I2Cx->DATA_CMD = 0x00 | CMD_STOP;

    while (!__I2C_IS_TxFIFO_EMPTY(hi2c->I2Cx));

    /* DevAddress NACK */
    if (i2c_get_int_status(hi2c, INT_TX_ABRT)) 
    {
        i2c_clear_int_status(hi2c, INT_TX_ABRT);

        __I2C_DISABLE(hi2c->I2Cx);

        return true;
    }
    else 
    {
        return false;
    }
}

/************************************************************************************
 * @fn      i2c_int_enable
 *
 * @brief   I2C interrupt enable.
 *
 * @param   hi2c: I2C handle.
 *          fe_INT_Index: interrupt index.
 */
void i2c_int_enable(I2C_HandleTypeDef *hi2c, enum_INT_Index_t fe_INT_Index)
{
    hi2c->I2Cx->INT_MASK |= fe_INT_Index;
}

/************************************************************************************
 * @fn      i2c_int_disable
 *
 * @brief   I2C interrupt disable.
 *
 * @param   hi2c: I2C handle.
 *          fe_INT_Index: interrupt index.
 */
void i2c_int_disable(I2C_HandleTypeDef *hi2c, enum_INT_Index_t fe_INT_Index)
{
    hi2c->I2Cx->INT_MASK &= ~fe_INT_Index;
}

/************************************************************************************
 * @fn      i2c_is_int_enable
 *
 * @brief   Is I2C interrupt enable.
 *
 * @param   hi2c: I2C handle.
 *          fe_INT_Index: interrupt index.
 */
bool i2c_is_int_enable(I2C_HandleTypeDef *hi2c, enum_INT_Index_t fe_INT_Index)
{
    return (hi2c->I2Cx->INT_MASK & fe_INT_Index) ? true : false;
}

/************************************************************************************
 * @fn      i2c_get_int_status
 *
 * @brief   I2C interrupt Status.
 *
 * @param   hi2c: I2C handle.
 *          fe_INT_Index: interrupt index.
 */
bool i2c_get_int_status(I2C_HandleTypeDef *hi2c, enum_INT_Index_t fe_INT_Index)
{
    bool lb_Status = (hi2c->I2Cx->RAW_INT_STAT & fe_INT_Index) ? true : false;

    return lb_Status;
}

/************************************************************************************
 * @fn      i2c_clear_int_status
 *
 * @brief   I2C interrupt status clear.
 *
 * @param   hi2c: I2C handle.
 *          fe_INT_Index: interrupt index.
 */
void i2c_clear_int_status(I2C_HandleTypeDef *hi2c, enum_INT_Index_t fe_INT_Index)
{
    volatile uint32_t lu32_Temp;

    switch (fe_INT_Index)
    {
        case INT_RX_UNDER:         lu32_Temp = hi2c->I2Cx->CLR_RX_UNDER;      break;
        case INT_RX_OVER:          lu32_Temp = hi2c->I2Cx->CLR_RX_OVER;       break;
        case INT_TX_OVER:          lu32_Temp = hi2c->I2Cx->CLR_TX_OVER;       break;
        case INT_RD_REQ:           lu32_Temp = hi2c->I2Cx->CLR_RD_REQ;        break;
        case INT_TX_ABRT:          lu32_Temp = hi2c->I2Cx->CLR_TX_ABRT;       break;
        case INT_RX_DONE:          lu32_Temp = hi2c->I2Cx->CLR_RX_DONE;       break;
        case INT_ACTIVITY:         lu32_Temp = hi2c->I2Cx->CLR_ACTIVITY;      break;
        case INT_STOP_DET:         lu32_Temp = hi2c->I2Cx->CLR_STOP_DET;      break;
        case INT_START_DET:        lu32_Temp = hi2c->I2Cx->CLR_START_DET;     break;
        case INT_RESTART_DET:      lu32_Temp = hi2c->I2Cx->CLR_RESTART_DET;   break;
        case INT_SCL_STUCK_AT_LOW: lu32_Temp = hi2c->I2Cx->CLR_SCL_STUCK_DET; break;

        default: break; 
    }
}
