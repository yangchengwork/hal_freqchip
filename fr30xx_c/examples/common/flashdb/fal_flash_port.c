/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-26     armink       the first version
 */

#include <fal.h>
#include <string.h>

#include "fr30xx.h"

#include "FreeRTOS.h"   // pvPortMalloc

static int init(void);
static int read(long offset, uint8_t *buf, size_t size);
static int write(long offset, uint8_t *buf, size_t size);
static int erase(long offset, size_t size);

//static sfud_flash_t sfud_dev = NULL;
const struct fal_flash_dev onchip_flash =
{
    .name       = "flashdb_onchip",
    .addr       = 0,
    .len        = 2 * 1024 * 1024,
    .blk_size   = 4*1024,
    .ops        = {init, read, write, erase},
    .write_gran = 16
};

static int init(void)
{
    return 0;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    flash_read(QSPI0, offset, size, buf);

    return size;
}

static int write(long offset, uint8_t *buf, size_t size)
{
    uint8_t *temp_ptr =  NULL;

    if(((uint32_t )buf & 0xff000000) == FLASH_DAC_BASE) {
        temp_ptr = pvPortMalloc(size);
        memcpy(temp_ptr,buf,size);
        flash_write(QSPI0, offset, size, temp_ptr);
        vPortFree(temp_ptr);
    }
    else
        flash_write(QSPI0, offset, size, buf);

    return size;
}

static int erase(long offset, size_t size)
{
    flash_erase(QSPI0, offset,size);
	
    return size;
}
