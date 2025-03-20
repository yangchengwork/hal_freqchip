#include "dsp.h"
#include "dsp_rpmsg.h"

void *dsp_mem_alloc(uint32_t size)
{
    void *result;
    
    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_MEM_ALLOC, (void *)&size, (uint32_t *)&result);

    return (void*)DSP_DRAM_2_MCU_SRAM(result);
}

void dsp_mem_free(void *ptr)
{
    void *buffer = (void *)MCU_SRAM_2_DSP_DRAM(ptr);

    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_MEM_FREE, (void *)&buffer, NULL);
}

uint32_t dsp_mem32_read(uint32_t address)
{
    uint32_t value;
    
    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_MEM_READ, (void *)&address, &value);

    return value;
}

void dsp_mem32_write(uint32_t address, uint32_t value)
{
    struct rpmsg_sync_msg_mem_write_t sync_msg;
    
    sync_msg.address = address;
    sync_msg.value = value;

    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_MEM_READ, (void *)&sync_msg, NULL);
}

void dsp_cache_attr_set(uint32_t icache_attr, uint32_t dcache_attr, uint8_t icache_ways, uint8_t dcache_ways)
{
    struct rpmsg_sync_msg_cache_attr_t sync_msg;
    
    sync_msg.icache_ways = icache_ways;
    sync_msg.dcache_ways = dcache_ways;
    sync_msg.icache_attr = icache_attr;
    sync_msg.dcache_attr = dcache_attr;

    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_CACHE_ATTR, (void *)&sync_msg, NULL);
}

void dsp_mem_get_usage(uint32_t *curr_free, uint32_t *min_free)
{
    struct rpmsg_sync_msg_mem_usage_t mem_usage;
    
    mem_usage.curr_free = curr_free;
    mem_usage.min_free = min_free;

    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_MEM_USAGE, (void *)&mem_usage, NULL);
}
