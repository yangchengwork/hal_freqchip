#ifndef __HOST_H__
#define __HOST_H__

#include <stdint.h>
#include <stdbool.h>
#include "bt_types.h"

void host_btdm_start(uint32_t baudrate, uint32_t stack_size, uint8_t priority, const uint8_t *ble_static_addr);
void host_ble_start(uint32_t baudrate, uint32_t stack_size, uint8_t priority, const uint8_t *ble_static_addr);

bool host_before_sleep_check(void);
void host_hci_reinit(void);

BtStatus DDB_EnumRecord(void);
BtStatus DDB_DeleteRecord(const BD_ADDR *bdAddr);

#endif  // __HOST_H__

