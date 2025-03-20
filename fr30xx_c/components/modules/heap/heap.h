#ifndef __HEAP_H__
#define __HEAP_H__

#include <stdint.h>

enum heap_type_t {
    HEAP_TYPE_SRAM_BLOCK,
    HEAP_TYPE_DRAM_BLOCK,
    HEAP_TYPE_BTDM_BLOCK,
    HEAP_TYPE_BLOCKS,
};

void heap_mem_init(uint8_t type, uint8_t* heap, uint32_t heap_size);
void *heap_mem_alloc(uint8_t type, uint32_t size);
void heap_mem_free(void* mem_ptr);

uint32_t heap_get_mem_usage(uint8_t type);
uint32_t heap_get_mem_available(uint8_t type);
uint32_t heap_get_max_mem_usage_single(uint8_t type);
uint32_t heap_get_max_mem_usage(void);
void heap_dump_used_mem(uint8_t type);

#endif  // __HEAP_H__
