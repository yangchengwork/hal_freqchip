#ifndef __EXT_FLASH_PROGRAM_H__
#define __EXT_FLASH_PROGRAM_H__

#include <stdint.h>
#include <stdbool.h>

struct ext_flash_prog_uart_op_t {
    void (*init)(uint32_t baundrate);
    void (*read)(uint8_t *data, uint16_t length);
    uint16_t (*read_no_block)(uint8_t *data, uint16_t length);
    void (*write)(uint8_t *data, uint16_t length);
};

struct ext_flash_operator_t {
    uint32_t (*flash_init)(void);
    uint32_t (*write)(uint32_t offset, uint32_t length, const uint8_t *data);
    uint32_t (*read)(uint32_t offset, uint32_t length, uint8_t *data);
    void (*erase)(uint32_t offset, uint32_t length);
    void (*chip_erase)(void);
    void (*protect_disable)(bool volatile_mode);
    void (*protect_enable)(bool volatile_mode);
};

void ext_flash_program(const struct ext_flash_operator_t *op, const struct ext_flash_prog_uart_op_t *uart_op);

#endif  // __EXT_FLASH_PROGRAM_H__
