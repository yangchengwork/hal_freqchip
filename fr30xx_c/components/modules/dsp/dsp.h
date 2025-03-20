#ifndef _DSP_H
#define _DSP_H

#include <stdint.h>
#include <stdbool.h>

#include "fr30xx.h"

#define DSP_IRAM_BASE_ADDR                  0x78400000
#define DSP_IMEM_MCU_BASE_ADDR              0x1FFD0000
#define DSP_DRAM_BASE_ADDR                  0x78200000
#define DSP_DRAM_MCU_BASE_ADDR              0x20080000
#define DSP_IRAM_SIZE                       0x00010000
#define DSP_DRAM_SIZE                       0x00010000

#define DSP_IRAM_2_MCU_SRAM(x)              ((uint32_t)(x) - DSP_IRAM_BASE_ADDR + DSP_IMEM_MCU_BASE_ADDR)
#define DSP_DRAM_2_MCU_SRAM(x)              ((uint32_t)(x) - DSP_DRAM_BASE_ADDR + DSP_DRAM_MCU_BASE_ADDR)
#define MCU_SRAM_2_DSP_DRAM(x)              ((uint32_t)(x) - DSP_DRAM_MCU_BASE_ADDR + DSP_DRAM_BASE_ADDR)

/************************************************************************************
 * @fn      dsp_load_code_from_internal_flash
 *
 * @brief    When DSP image is saved in FILE SYSTEM, user should call this function to 
 *           load code, RO, RW, etc. sections into local memory.
 *
 * @param   address: DSP image store address.
 *          length: DSP image length
 *
 * @return  encoder result, @ref audio_ret_t.
 */
bool dsp_load_code_from_fs(char *path);

/************************************************************************************
 * @fn      dsp_load_code_from_internal_flash
 *
 * @brief    When DSP image is saved in internal flash, user should call this function to 
 *           load code, RO, RW, etc. sections into local memory.
 *
 * @param   address: DSP image store address.
 *          length: DSP image length
 *
 * @return  encoder result, @ref audio_ret_t.
 */
bool dsp_load_code_from_internal_flash(uint32_t address, uint32_t length);

/************************************************************************************
 * @fn      dsp_load_rw_from_internal_flash
 *
 * @brief   When DSP image is saved in internal flash, user should call this function to
 *          reinitilaize RO and RW section of DSP code. 
 *          If dsp_deep_sleep is not called before system enter deep sleep mode to save  
 *          the DSP state, then the DSP state and data will be lost after sleep, and 
 *          this function needs to be called after wake up to restore the rw region to 
 *          the initial state.
 *
 * @param   address: DSP image store address.
 *          length: DSP image length
 *
 * @return  encoder result, @ref audio_ret_t.
 */
bool dsp_load_rw_from_internal_flash(uint32_t address, uint32_t length);

/************************************************************************************
 * @fn      dsp_prepare
 *
 * @brief   release clock of all modules in DSP, and keep DSP in suspend mode.
 */
void dsp_prepare(void);

/************************************************************************************
 * @fn      dsp_run
 *
 * @brief   User should call this function to start DSP after dsp is prepared and code
 *          is loaded.
 *
 * @param   vector: the reset vector address of DSP
 */
void dsp_run(uint32_t vector);

/************************************************************************************
 * @fn      dsp_deep_sleep
 *
 * @brief   Put DSP into sleep mode to save power, DSP state will be saved into local 
 *          memory. User should call dsp_wake_up to recover DSP state. All the RAM used
 *          by DSP should be retentioned when system enter deep sleep mode.
 */
void dsp_deep_sleep(void);

/************************************************************************************
 * @fn      dsp_wake_up
 *
 * @brief   Wake up DSP from deep sleep mode.
 *
 * @param   vector: the reset vector address of DSP
 */
void dsp_wake_up(uint32_t vector);

/************************************************************************************
 * @fn      dsp_task_info
 *
 * @brief   Used to get FreeRTOS tasks running state of DSP.
 *
 * @param   info: piont to a buffer used to save task information
 *          length: the space that the info points to
 */
void dsp_task_info(uint8_t *info, uint32_t length);

/************************************************************************************
 * @fn      dsp_cpu_usage
 *
 * @brief   Used to get FreeRTOS tasks CPU usage of DSP.
 *
 * @param   info: piont to a buffer used to save task CPU usage
 *          length: the space that the info points to
 */
void dsp_cpu_usage(uint8_t *usage, uint32_t length);

#endif  // _DSP_H
