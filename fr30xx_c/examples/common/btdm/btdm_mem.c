#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "fr30xx.h"
#include "heap.h"

#define configTOTAL_BTDM_HEAP_SIZE  ( ( 30 * 1024 ) )

static bool is_mem_poll_inited = false;

__attribute__((section("dram_section"))) static uint32_t ucHeap[ (configTOTAL_BTDM_HEAP_SIZE >> 2) + 0x10];

void * btdm_malloc( size_t xWantedSize )
{
    if (is_mem_poll_inited == false) {
        is_mem_poll_inited = true;
        heap_mem_init(HEAP_TYPE_BTDM_BLOCK, (void *)ucHeap, configTOTAL_BTDM_HEAP_SIZE);
    }
    
    return heap_mem_alloc(HEAP_TYPE_BTDM_BLOCK, xWantedSize);
}

void btdm_free( void * pv )
{
    heap_mem_free(pv);
}

__RAM_CODE void *btdm_calloc(unsigned int count, unsigned int size)
{
    void *ptr;
    
    ptr = btdm_malloc(count * size);
    if (ptr) {
        memset(ptr, 0, count * size);
    }
    
    return ptr;
}
