#include <stdint.h>
#include <stdbool.h>

#include "fr30xx.h"

#include "dsp.h"
#include "dsp_rpmsg.h"

#include "FreeRTOS.h"

#define DSP_LOAD_SUPPORT_FATFS                  0

#if DSP_LOAD_SUPPORT_FATFS
#include "ff.h"
#endif

#define LOAD_CODE_FRAME_SIZE_FROM_FS            512
#define LOAD_CODE_FRAME_SIZE_FROM_FLASH         256

#if DSP_LOAD_SUPPORT_FATFS
bool dsp_load_code_from_fs(char *path)
{
    struct bin_header {
        uint32_t start;
        uint32_t length;
    } header;

    FRESULT res;
    FIL fstt;    
    uint32_t length;
    uint32_t *ptr;
    uint32_t *buffer, *_buffer;
    bool ret = true;
    
    res=f_open(&fstt, path, FA_OPEN_EXISTING | FA_READ);
    if ( res != FR_OK ) {
        return false;
    }
    else {
        buffer = (void *)pvPortMalloc(LOAD_CODE_FRAME_SIZE_FROM_FS);
        
        /* get text section storage information */
        f_read(&fstt, (void *)&header, sizeof(struct bin_header), &length);
        if (length != sizeof(struct bin_header)) {
            goto error;
        }
        
        /* read text section storage into IRAM */
        ptr = (void *)DSP_IRAM_2_MCU_SRAM(header.start);
        while (header.length) {
            uint32_t _length;
            length = header.length > LOAD_CODE_FRAME_SIZE_FROM_FS ? LOAD_CODE_FRAME_SIZE_FROM_FS : header.length;
            _buffer = buffer;
            f_read(&fstt, (void *)_buffer, length, &_length);
            if (length != _length) {
                ret = false;
                goto error;
            }
            while (_length) {
                *ptr++ = *_buffer++;
                _length -= 4;
            }
            header.length -= length;
        }
        
        /* get RO and RW section storage information */
        f_read(&fstt, (void *)&header, sizeof(struct bin_header), &length);
        if (length != sizeof(struct bin_header)) {
            ret = false;
            goto error;
        }
        
        /* read text section storage into DRAM */
        ptr = (void *)DSP_DRAM_2_MCU_SRAM(header.start);
        while (header.length) {
            uint32_t _length;
            length = header.length > LOAD_CODE_FRAME_SIZE_FROM_FS ? LOAD_CODE_FRAME_SIZE_FROM_FS : header.length;
            _buffer = buffer;
            f_read(&fstt, (void *)_buffer, length, &_length);
            if (length != _length) {
                ret = false;
                goto error;
            }
            while (_length) {
                *ptr++ = *_buffer++;
                _length -= 4;
            }
            header.length -= length;
        }
    }
    
error:
    vPortFree(buffer);
    f_close(&fstt);
    
    return ret;
}
#endif

bool dsp_load_code_from_internal_flash(uint32_t address, uint32_t length)
{
    struct bin_header {
        uint32_t start;
        uint32_t length;
    } header;
 
    uint32_t sub_length, _sub_length;
    uint32_t *ptr;
    uint32_t *buffer, *_buffer;
    bool ret = true;

    buffer = (void *)pvPortMalloc(LOAD_CODE_FRAME_SIZE_FROM_FLASH);
    
    /* get text section storage information */
    flash_read(QSPI0, address, sizeof(struct bin_header), (void *)&header);
    address += sizeof(struct bin_header);
    
    /* read text section storage into IRAM */
    ptr = (void *)DSP_IRAM_2_MCU_SRAM(header.start);
    while (header.length) {
        sub_length = header.length > LOAD_CODE_FRAME_SIZE_FROM_FLASH ? LOAD_CODE_FRAME_SIZE_FROM_FLASH : header.length;
        _sub_length = sub_length;
        _buffer = buffer;
        flash_read(QSPI0, address, sub_length, (void *)_buffer);
        while (sub_length) {
            *ptr++ = *_buffer++;
            sub_length -= 4;
        }
        header.length -= _sub_length;
        address += _sub_length;
    }
    
    /* get RO and RW section storage information */
    flash_read(QSPI0, address, sizeof(struct bin_header), (void *)&header);
    address += sizeof(struct bin_header);
    
    /* read RO and RW section storage into DRAM */
    ptr = (void *)DSP_DRAM_2_MCU_SRAM(header.start);
    while (header.length) {
        sub_length = header.length > LOAD_CODE_FRAME_SIZE_FROM_FLASH ? LOAD_CODE_FRAME_SIZE_FROM_FLASH : header.length;
        _sub_length = sub_length;
        _buffer = buffer;
        flash_read(QSPI0, address, sub_length, (void *)_buffer);
        while (sub_length) {
            *ptr++ = *_buffer++;
            sub_length -= 4;
        }
        header.length -= _sub_length;
        address += _sub_length;
    }
    
error:
    vPortFree(buffer);
    
    return ret;
}

bool dsp_load_rw_from_internal_flash(uint32_t address, uint32_t length)
{
    struct bin_header {
        uint32_t start;
        uint32_t length;
    } header;
 
    uint32_t sub_length, _sub_length;
    uint32_t *ptr;
    uint32_t *buffer, *_buffer;
    bool ret = true;

    buffer = (void *)pvPortMalloc(LOAD_CODE_FRAME_SIZE_FROM_FLASH);
    
    /* get text section storage information */
    flash_read(QSPI0, address, sizeof(struct bin_header), (void *)&header);
    address += sizeof(struct bin_header);

    /* jump to RW header */
    address += header.length;
    
    /* get RO and RW section storage information */
    flash_read(QSPI0, address, sizeof(struct bin_header), (void *)&header);
    address += sizeof(struct bin_header);
    
    /* read RO and RW section storage into DRAM */
    ptr = (void *)DSP_DRAM_2_MCU_SRAM(header.start);
    while (header.length) {
        sub_length = header.length > LOAD_CODE_FRAME_SIZE_FROM_FLASH ? LOAD_CODE_FRAME_SIZE_FROM_FLASH : header.length;
        _sub_length = sub_length;
        _buffer = buffer;
        flash_read(QSPI0, address, sub_length, (void *)_buffer);
        while (sub_length) {
            *ptr++ = *_buffer++;
            sub_length -= 4;
        }
        header.length -= _sub_length;
        address += _sub_length;
    }
    
error:
    vPortFree(buffer);
    
    return ret;
}

/* release DSP clock and keep DSP in suspend mode */
void dsp_prepare(void)
{
    *(volatile uint32_t *)&SYSTEM->DSPClockEnable = 0xffffffff;
    SYSTEM->DSPCTRL.DSP_RUNSTALL = 1;
}

void dsp_run(uint32_t vector)
{
    SYSTEM->DSPVectorConfig.DSP_VEC_TBL = vector;
    SYSTEM->DSPVectorConfig.DSP_VEC_SEL = 1;
    SYSTEM->DSPCTRL.DSP_DBGEN    = 1;
    SYSTEM->DSPCTRL.DSP_NIDEN    = 1;
    SYSTEM->DSPCTRL.DSP_SPIDEN   = 1;
    SYSTEM->DSPCTRL.DSP_RUNSTALL = 0;
}

/*
    uint8_t *ptr = pvPortMalloc(512);
    dsp_task_info(ptr, 512);
    printf("%s", ptr);
    vPortFree(ptr);
 */
void dsp_task_info(uint8_t *info, uint32_t length)
{
    struct rpmsg_sync_msg_task_info_t sync_msg;
    
    if (((uint32_t)info >= DSP_DRAM_MCU_BASE_ADDR) && ((uint32_t)info < (DSP_DRAM_MCU_BASE_ADDR+DSP_DRAM_SIZE))) {
        info = (uint8_t *)MCU_SRAM_2_DSP_DRAM(info);
    }
    
    sync_msg.info = info;
    sync_msg.length = length;

    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_TASK_INFO, (void *)&sync_msg, NULL);
}

/*
    uint8_t *ptr = pvPortMalloc(512);
    dsp_cpu_usage(ptr, 512);
    printf("%s", ptr);
    vPortFree(ptr);
 */
void dsp_cpu_usage(uint8_t *usage, uint32_t length)
{
    struct rpmsg_sync_msg_cpu_usage_t sync_msg;
    
    if (((uint32_t)usage >= DSP_DRAM_MCU_BASE_ADDR) && ((uint32_t)usage < (DSP_DRAM_MCU_BASE_ADDR+DSP_DRAM_SIZE))) {
        usage = (uint8_t *)MCU_SRAM_2_DSP_DRAM(usage);
    }
    
    sync_msg.usage = usage;
    sync_msg.length = length;

    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_CPU_USAGE, (void *)&sync_msg, NULL);
}

void dsp_deep_sleep(void)
{
    struct rpmsg_sync_msg_deep_sleep_t *deep_sleep_param;
    volatile uint32_t *check_addr;
    uint32_t ret;
    
    /* send deep sleep command to DSP */
    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_DEEP_SLEEP, NULL, &ret);
    /* wait for DSP is asleep. */
    deep_sleep_param = (void *)DSP_DRAM_2_MCU_SRAM(ret);
    check_addr = (void *)DSP_DRAM_2_MCU_SRAM(deep_sleep_param->check_address);
    while (*check_addr != (deep_sleep_param->saved_flag ^ 0xffffffff));
    *check_addr = deep_sleep_param->saved_flag;

    /* disable clock of all DSP modules to save power */
    *(volatile uint32_t *)&SYSTEM->DSPClockEnable = 0x00000000;
    SYSTEM->DSPCTRL.DSP_RUNSTALL = 1;
}

void dsp_wake_up(uint32_t vector)
{
    SYSTEM->DSPRegReset.DSP_MAS_SFT_RST = 1;
    dsp_prepare();
    dsp_run(vector);
}
