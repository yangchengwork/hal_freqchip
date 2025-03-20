#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "fr30xx.h"
#include "FreeRTOS.h"
#include "heap.h"

#if 0
void * pvPortMalloc( size_t xWantedSize )
{
    void *ptr;
    
    GLOBAL_INT_DISABLE();
    ptr = malloc(xWantedSize);
    GLOBAL_INT_RESTORE();

    return ptr;
}

void vPortFree( void * pv )
{
    GLOBAL_INT_DISABLE();
    free(pv);
    GLOBAL_INT_RESTORE();
}

void * pvPortRealloc ( void *pv, size_t xNewSize )
{
    void *ptr;
    
    GLOBAL_INT_DISABLE();
    ptr = malloc(xNewSize);
    GLOBAL_INT_RESTORE();
    
    if (pv) {
        memcpy(ptr, pv, xNewSize);
        vPortFree(pv);
    }
    
    return ptr;
}
#else
static bool is_mem_poll_inited = false;

static uint32_t ucHeap[ (configTOTAL_HEAP_SIZE >> 2)];

void * pvPortMalloc( size_t xWantedSize )
{
    if (is_mem_poll_inited == false) {
        is_mem_poll_inited = true;
        heap_mem_init(HEAP_TYPE_SRAM_BLOCK, (void *)ucHeap, configTOTAL_HEAP_SIZE);
    }
    
    return heap_mem_alloc(HEAP_TYPE_SRAM_BLOCK, xWantedSize);
}

void vPortFree( void * pv )
{
    heap_mem_free(pv);
}

void * pvPortRealloc ( void *pv, size_t xNewSize )
{
    void *ptr;
    
    GLOBAL_INT_DISABLE();
    ptr = pvPortMalloc(xNewSize);
    GLOBAL_INT_RESTORE();
    
    if (pv) {
        memcpy(ptr, pv, xNewSize);
        vPortFree(pv);
    }
    
    return ptr;
}

void *pvPortCalloc(unsigned int count, unsigned int size)
{
    void *ptr;
    
    ptr = pvPortMalloc(count * size);
    if (ptr) {
        memset(ptr, 0, count * size);
    }
    
    return ptr;
}

#endif
