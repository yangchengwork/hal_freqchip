#ifndef _FAL_APP_H_
#define _FAL_APP_H_

#include <stdint.h>
#include <stddef.h>

#include "fdb_def.h"

#define FDB_KEY_BT_LINKKEY          0x00010000      //!< bt link key storage index
#define FDB_KEY_CONTROLLER_INFO     0x00010001      //!< controller information storage index
#define FDB_KEY_BTDM_LIB_BASE       0x00020000      //!< base index used to store host data
#define FDB_KEY_USER_BASE           0x00030000
#define FDB_KEY_USER_RANDOM_SEED    0x00030001

int flashdb_init(void);

fdb_err_t flashdb_set(uint32_t key, uint8_t *value, uint32_t length);

size_t flashdb_get(uint32_t key, uint8_t *value, uint32_t length);

size_t flashdb_get_length(uint32_t key);

fdb_err_t flashdb_del(uint32_t key);

#endif

