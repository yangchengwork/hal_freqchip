#ifndef _DSP_RPMSG_H
#define _DSP_RPMSG_H

#include <stdint.h>

#include "rpmsg.h"

#define RPMSG_SYNC_FUNC_MEM_ALLOC           RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0001)
#define RPMSG_SYNC_FUNC_MEM_FREE            RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0002)
#define RPMSG_SYNC_FUNC_DSP_CHG_FRQ         RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0003)  // DSP request to change working frequency
#define RPMSG_SYNC_FUNC_MEM_READ            RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0004)
#define RPMSG_SYNC_FUNC_MEM_WRITE           RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0005)
#define RPMSG_SYNC_FUNC_CACHE_ATTR          RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0006)
#define RPMSG_SYNC_FUNC_MEM_USAGE           RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0007)
#define RPMSG_SYNC_FUNC_TASK_INFO           RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0008)
#define RPMSG_SYNC_FUNC_CPU_USAGE           RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x0009)
#define RPMSG_SYNC_FUNC_DEEP_SLEEP          RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_DSP, 0x000a)

struct rpmsg_sync_msg_mem_write_t {
    uint32_t address;
    uint32_t value;
};

struct rpmsg_sync_msg_cache_attr_t {
    uint32_t icache_attr;
    uint32_t dcache_attr;
    uint8_t icache_ways;
    uint8_t dcache_ways;
};

struct rpmsg_sync_msg_mem_usage_t {
    uint32_t *curr_free;
    uint32_t *min_free;
};

struct rpmsg_sync_msg_task_info_t {
    uint8_t *info;
    uint32_t length;
};

struct rpmsg_sync_msg_cpu_usage_t {
    uint8_t *usage;
    uint32_t length;
};

struct rpmsg_sync_msg_deep_sleep_t {
    uint32_t *check_address;
    uint32_t saved_flag;
};

void dsp_rpmsg_handler(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t *msg);

#endif  // _DSP_RPMSG_H
