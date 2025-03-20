/*
  ******************************************************************************
  * @file    driver_flash.h
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   Header file of flash HAL module.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/

#ifndef _DRIVER_FLASH_H
#define _DRIVER_FLASH_H

#include <stdint.h>
#include <stdbool.h>

#include "driver_qspi.h"

#define FLASH_READ_DEVICE_ID            0x90
#define FLASH_READ_IDENTIFICATION       0x9F
    
#define FLASH_AAI_PROGRAM_OPCODE        0xAF
#define FLASH_PAGE_PROGRAM_OPCODE       0x02
#define FLASH_READ_OPCODE               0x03
#define FLASH_FAST_READ_OPCODE          0x0B
#define FLASH_FAST_DTR_READ_OPCODE      0x0D
#define FLASH_READ_DUAL_OPCODE          0xBB
#define FLASH_READ_DTR_DUAL_OPCODE      0xBD
#define FLASH_READ_DUAL_OPCODE_2        0x3B
#define FLASH_PAGE_DUAL_PROGRAM_OPCODE  0xA2
#define FLASH_PAGE_QUAL_READ_OPCODE     0xEB
#define FLASH_PAGE_DTR_QUAL_READ_OPCODE 0xED
#define FLASH_PAGE_QUAL_READ_OPCODE_2   0x6B
#define FLASH_PAGE_QUAL_PROGRAM_OPCODE  0x32

    
#define FLASH_CHIP_ERASE_OPCODE         0x60
#define FLASH_SECTORE_ERASE_OPCODE      0x20
#define FLASH_BLOCK_32K_ERASE_OPCODE    0x52
#define FLASH_BLOCK_64K_ERASE_OPCODE    0xD8
#define FLASH_ST_SECTORE_ERASE_OPCODE   0xD8
#define FLASH_ST_BULK_ERASE_OPCODE      0xC7
    
#define FLASH_WRITE_DISABLE_OPCODE      0x04
#define FLASH_WRITE_ENABLE_OPCODE       0x06
#define FLASH_WRITE_STATUS_REG_OPCODE   0x01
#define FLASH_READ_STATUS_REG_OPCODE    0x05
#define FLASH_READ_STATUS_HIGH_REG_OPCODE    0x35

#define FLASH_SEC_REG_READ_OPCODE       (0x48)
#define FLASH_SEC_REG_PROGRAM_OPCODE    (0x42)
#define FLASH_SEC_REG_ERASE_OPCODE      (0x44)

#define FLASH_ST_ID                     0x20
#define FLASH_SST_ID                    0xBF

#define FLASH_OP_TYPE_ERASE             0
#define FLASH_OP_TYPE_WRITE             1

/* flash gen uuid type */
enum flash_gen_uuid_type_t {
    FLASH_GEN_UUID_TYPE_40,
    FLASH_GEN_UUID_TYPE_12,
};

/* flash controller read type */
enum flash_rd_type_t {
    FLASH_RD_TYPE_SINGLE,
    FLASH_RD_TYPE_SINGLE_FAST,
    FLASH_RD_TYPE_DUAL,
    FLASH_RD_TYPE_DUAL_FAST,
    FLASH_RD_TYPE_QUAD,
    FLASH_RD_TYPE_QUAD_FAST,
    FLASH_RD_TYPE_DTR_SINGLE_FAST,
    FLASH_RD_TYPE_DTR_DUAL_FAST,
    FLASH_RD_TYPE_DTR_QUAD_FAST,
};

/* flash controller write type */
enum flash_wr_type_t {
    FLASH_WR_TYPE_SINGLE,
    FLASH_WR_TYPE_DUAL,
    FLASH_WR_TYPE_QUAD,
};

/* Exported functions --------------------------------------------------------*/

/* flash_preinit */
/* flash_init */
/* flash_enter_deep_sleep */
/* flash_exit_deep_sleep */
/* flash_protect_bit_set */
void flash_preinit(struct qspi_regs_t *qspi);
uint32_t flash_init(struct qspi_regs_t *qspi);
void flash_enter_deep_sleep(struct qspi_regs_t *qspi);
void flash_exit_deep_sleep(struct qspi_regs_t *qspi);
void flash_protect_bit_set(struct qspi_regs_t *qspi, uint8_t bits, uint8_t cmp, uint8_t wr_volatile, uint8_t status_2_separate);


/* flash_read_uuid */
/* flash_gen_uuid */
void flash_read_uuid(struct qspi_regs_t *qspi, uint8_t *buffer, uint8_t *length);
void flash_gen_uuid(struct qspi_regs_t *qspi, uint8_t *buffer, enum flash_gen_uuid_type_t type);

/* flash_OTP_read */
/* flash_OTP_write */
/* flash_OTP_erase */
void flash_OTP_read(struct qspi_regs_t *qspi,uint32_t offset, uint32_t length, uint8_t *buffer);
void flash_OTP_write(struct qspi_regs_t *qspi,uint32_t offset, uint32_t length, uint8_t *buffer);
void flash_OTP_erase(struct qspi_regs_t *qspi,uint32_t offset);

/* flash_write */
/* flash_read */
/* flash_erase */
/* flash_chip_erase */
uint8_t FR_DRIVER_WRAPPER(flash_write)(struct qspi_regs_t *qspi, uint32_t offset, uint32_t length, const uint8_t *buffer);
uint8_t FR_DRIVER_WRAPPER(flash_read)(struct qspi_regs_t *qspi, uint32_t offset, uint32_t length, uint8_t *buffer);
uint8_t FR_DRIVER_WRAPPER(flash_erase)(struct qspi_regs_t *qspi, uint32_t offset, uint32_t size);
void flash_chip_erase(struct qspi_regs_t *qspi);

/* flash_set_read_type */
/* flash_set_write_type */
/* flash_init_controller */
void flash_set_read_type(struct qspi_regs_t *qspi, uint8_t rd_type);
void flash_set_write_type(struct qspi_regs_t *qspi, uint8_t wr_type);
void flash_init_controller(struct qspi_regs_t *qspi, enum flash_rd_type_t rd_type, enum flash_wr_type_t wr_type);

/* flash_set_capture_delay */
/* flash_set_baudrate */
/* flash_enable_quad */
/* flash_set_IO_DRV */
void flash_set_capture_delay(struct qspi_regs_t *qspi, uint8_t delay);
void flash_set_baudrate(struct qspi_regs_t *qspi, uint8_t baudrate);
void flash_enable_quad(struct qspi_regs_t *qspi);
void flash_set_IO_DRV(struct qspi_regs_t *qspi, uint8_t drv);

/* flash_read_id */
/* flash_read_status */
/* flash_write_status */
/* flash_write_status_2 */
/* flash_write_status_volatile */
/* flash_write_status_2_volatile */
/* flash_wait_wip_clear */
uint32_t flash_read_id(struct qspi_regs_t *qspi);
uint16_t flash_read_status(struct qspi_regs_t *qspi, bool read_high);
void flash_write_status(struct qspi_regs_t *qspi, uint16_t status, bool write_high);
void flash_write_status_2(struct qspi_regs_t *qspi, uint8_t status);
void flash_write_status_volatile(struct qspi_regs_t *qspi, uint16_t status, bool write_high);
void flash_write_status_2_volatile(struct qspi_regs_t *qspi, uint8_t status);
void flash_wait_wip_clear(struct qspi_regs_t *qspi);

#endif
