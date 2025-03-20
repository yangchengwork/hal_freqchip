#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

bool controller_start(uint32_t baudrate, const uint8_t *ble_addr, const uint8_t *bt_addr, uint32_t src_addr);

#endif  // __CONTROLLER_H__

