#ifndef _RPMSG_H
#define _RPMSG_H

#include <stdint.h>

#include "rpmsg_lite.h"

/* Exported macro ------------------------------------------------------------*/

#define RPMSG_MSG_TYPE_MASTER_READY         0x00000001
#define RPMSG_MSG_TYPE_REMOTE_READY         0x00000002
#define RPMSG_MSG_TYPE_SYNC_INVOKE          0x00000003
#define RPMSG_MSG_TYPE_SYNC_RETURN          0x00000004
#define RPMSG_MSG_TYPE_ASYNC_MSG            0x00000005

#define RPMSG_SYNC_FUNC_MSG(type, sub_type) (((type)<<16) | (sub_type))
#define RPSMG_SYNC_FUNC_TYPE(func_id)       ((func_id)>>16)
#define RPSMG_SYNC_FUNC_SUB_TYPE(func_id)   ((func_id) & 0xffff)

#define RPMSG_SYNC_FUNC_TYPE_TEST           0x0001
#define RPMSG_SYNC_FUNC_TYPE_DSP            0x0002
#define RPMSG_SYNC_FUNC_TYPE_AUDIO          0x0003
#define RPMSG_SYNC_FUNC_TYPE_LVGL           0x0004
#define RPMSG_SYNC_FUNC_TYPE_FREETYPE       0x0005

#define RPMSG_SYNC_FUNC_SUM                 RPMSG_SYNC_FUNC_MSG(RPMSG_SYNC_FUNC_TYPE_TEST, 0x0001)

/** @addtogroup rpmsg syncronize invoke message definations
  * @{
  */
struct rpmsg_sync_msg_sum_t {
    uint32_t x;
    uint32_t y;
};

/**
  * @}
  */

/** @addtogroup rpmsg asyncronize message definations
  * @{
  */
struct rpmsg_async_msg_t {
    uint32_t msg_id;
    union {
        void *param;
        uint32_t dsp_req_frq;
    } p;
};
/**
  * @}
  */

struct rpmsg_msg_t {
    uint32_t msg_type;

    union {
        struct {
            uint32_t func_id;
            void *param;
        } sync_func;

        struct {
            uint32_t status;
            uint32_t result;
        } sync_ret;

        struct rpmsg_async_msg_t async_msg;
    } p;
};

/*-----------------------------------------------------------------------------------*/
/* Exported functions ---------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/

uint32_t rpmsg_sync_invoke(struct rpmsg_lite_instance *rpmsg, uint32_t func_id, void *param, uint32_t *ret);
uint32_t rpmsg_send_async(struct rpmsg_lite_instance *rpmsg, struct rpmsg_async_msg_t *async_msg);
uint32_t rpmsg_send_sync_ret(struct rpmsg_lite_instance *rpmsg, uint32_t status, uint32_t ret);

struct rpmsg_lite_instance *rpmsg_master_init(void (*recv)(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t *msg));
void rpmsg_master_recover(struct rpmsg_lite_instance *rpmsg);
struct rpmsg_lite_instance *rpmsg_remote_init(void (*recv)(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t *msg));
void rpmsg_wait_master_ready(struct rpmsg_lite_instance *rpmsg);
void rpmsg_destroy(struct rpmsg_lite_instance *rpmsg);

uint32_t rpmsg_recv_msg(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t **msg, uint32_t *msg_len);

struct rpmsg_lite_instance *rpmsg_get_remote_instance(void);
struct rpmsg_lite_instance *rpmsg_get_master_instance(void);

void rpmsg_remote_recover(void);
void rpmsg_remote_resume(void);

#endif  // _RPMSG_H
