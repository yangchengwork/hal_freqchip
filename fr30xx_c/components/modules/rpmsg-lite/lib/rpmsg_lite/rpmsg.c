#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "fr30xx.h"
#include "dsp.h"

#include "rpmsg.h"
#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"

#define RPMSG_LITE_LINK_ID      0
#define SH_MEM_TOTAL_SIZE       (1536U)

#define EPT_ADDR_SYNC           1
#define EPT_ADDR_ASYNC          2

#if defined(__ARMCC_VERSION) || defined(__ICCARM__)
static __attribute__((section(".ARM.__at_0x20000000"))) uint8_t rpmsg_share_mem[SH_MEM_TOTAL_SIZE];
#elif defined(__GNUC__)
__attribute__((section("share_mem"))) uint8_t rpmsg_share_mem[SH_MEM_TOTAL_SIZE];
#else
#error "unsupported platform"
#endif

static rpmsg_queue_handle queue_sync = NULL;
static rpmsg_queue_handle queue_async = NULL;
static struct rpmsg_lite_endpoint *ept_sync = NULL;
static struct rpmsg_lite_endpoint *ept_async = NULL;
static struct rpmsg_lite_instance *remote_rpmsg = NULL;
static struct rpmsg_lite_instance *master_rpmsg = NULL;

static LOCK *ept_sync_lock;

static void (*msg_callback)(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t *msg) = NULL;

/************************************************************************************
 * @fn      rpmsg_sync_invoke
 *
 * @brief   Start synchronous invocation to the other side.
 *
 * @param   rpmsg: rpmsg instance.
 * @param   func_id: request function ID.
 * @param   param: all parameters.
 * @param   ret: return value.
 *
 * @return  the function is handled by the other side normally or note
 */
uint32_t rpmsg_sync_invoke(struct rpmsg_lite_instance *rpmsg, uint32_t func_id, void *param, uint32_t *ret)
{
    struct rpmsg_msg_t *msg;
    uint32_t msg_len;
    uint32_t src_addr;
    uint32_t status;
    int32_t remote_call_ret;
    
    if (rpmsg == NULL) {
        return -1;
    }

    env_lock_mutex(ept_sync_lock);
    system_prevent_sleep_set(SYSTEM_PREVENT_SLEEP_TYPE_DSP);
    
    msg = rpmsg_lite_alloc_tx_buffer(rpmsg, &msg_len, RL_BLOCK);
    
    msg->msg_type = RPMSG_MSG_TYPE_SYNC_INVOKE;
    msg->p.sync_func.func_id = func_id;
    msg->p.sync_func.param = param;
    
//    fputc('S', NULL);
    rpmsg_lite_send_nocopy(rpmsg, ept_async, EPT_ADDR_ASYNC, msg, msg_len);
    remote_call_ret = rpmsg_queue_recv_nocopy(rpmsg, queue_sync, (uint32_t *)&src_addr, (char **)&msg, &msg_len, 1000);
    assert(RL_SUCCESS == remote_call_ret);
//    fputc('s', NULL);

    if (ret) {
        *ret = msg->p.sync_ret.result;
    }
    status = msg->p.sync_ret.status;

    system_prevent_sleep_clear(SYSTEM_PREVENT_SLEEP_TYPE_DSP);
    env_unlock_mutex(ept_sync_lock);
    
    rpmsg_lite_release_rx_buffer(rpmsg, msg);
    
    return status;
}

/************************************************************************************
 * @fn      rpmsg_send_async
 *
 * @brief   Send message or command to the other side.
 *
 * @param   rpmsg: rpmsg instance.
 * @param   async_msg: asynchronous message, this struct will be copied into share memory.
 *
 * @return  The message is sent to the other side successfully or not
 */
uint32_t rpmsg_send_async(struct rpmsg_lite_instance *rpmsg, struct rpmsg_async_msg_t *async_msg)
{
    struct rpmsg_msg_t *msg;
    uint32_t msg_len;
    uint32_t src_addr;
    
    msg = rpmsg_lite_alloc_tx_buffer(rpmsg, &msg_len, RL_BLOCK);

    msg->msg_type = RPMSG_MSG_TYPE_ASYNC_MSG;
    memcpy((void *)&msg->p.async_msg, (void *)async_msg, sizeof(struct rpmsg_async_msg_t));
    
    rpmsg_lite_send_nocopy(rpmsg, ept_async, EPT_ADDR_ASYNC, msg, msg_len);
    
    return 0;
}

/************************************************************************************
 * @fn      rpmsg_send_sync_ret
 *
 * @brief   Send response to the other side after execute synchronous invocation.
 *
 * @param   rpmsg: rpmsg instance.
 * @param   status: handle the invocation normally or not.
 * @param   ret: return value of request function.
 *
 * @return  The message is sent to the other side successfully or not
 */
uint32_t rpmsg_send_sync_ret(struct rpmsg_lite_instance *rpmsg, uint32_t status, uint32_t ret)
{
    struct rpmsg_msg_t *msg;
    uint32_t msg_len;
    uint32_t src_addr;
    
    msg = rpmsg_lite_alloc_tx_buffer(rpmsg, &msg_len, RL_BLOCK);
    
    msg->msg_type = RPMSG_MSG_TYPE_SYNC_RETURN;
    msg->p.sync_ret.status = status;
    msg->p.sync_ret.result = ret;
    
    rpmsg_lite_send_nocopy(rpmsg, ept_sync, EPT_ADDR_SYNC, msg, msg_len);

    return 0;
}

/************************************************************************************
 * @fn      rpmsg_master_init
 *
 * @brief   Initialize rpmsg-lite master side, this function is used in DSP side in general usage.
 *
 * @param   callback: callback function to receive message from the other side.
 *
 * @return  initialized rpmsg-lite master instance
 */
struct rpmsg_lite_instance *rpmsg_master_init(void (*recv)(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t *msg))
{
    struct rpmsg_lite_instance *my_rpmsg;
    struct rpmsg_msg_t *msg;
    uint32_t src_addr;
    uint32_t msg_len;
    
    my_rpmsg = rpmsg_lite_master_init((void *)0x20000000, SH_MEM_TOTAL_SIZE, RPMSG_LITE_LINK_ID, RL_NO_FLAGS);

    queue_sync  = rpmsg_queue_create(my_rpmsg);
    queue_async = rpmsg_queue_create(my_rpmsg);
    ept_sync  = rpmsg_lite_create_ept(my_rpmsg, EPT_ADDR_SYNC, rpmsg_queue_rx_cb, queue_sync);
    ept_async = rpmsg_lite_create_ept(my_rpmsg, EPT_ADDR_ASYNC, rpmsg_queue_rx_cb, queue_async);

    env_create_mutex((void **)&ept_sync_lock, 1);

    msg_callback = recv;
    master_rpmsg = my_rpmsg;

    /* notice remote side "I'm ready." */
    msg = rpmsg_lite_alloc_tx_buffer(my_rpmsg, &msg_len, RL_BLOCK);
    msg->msg_type = RPMSG_MSG_TYPE_MASTER_READY;
    rpmsg_lite_send_nocopy(my_rpmsg, ept_sync, EPT_ADDR_SYNC, msg, msg_len);

    return my_rpmsg;
}

/************************************************************************************
 * @fn      rpmsg_master_recover
 *
 * @brief   recover master side from DEEP SLEEP mode to NORAML mode. reinit IPC and send
 *          MASTER READY message to remote side.
 *
 * @param   rpmsg: rpmsg instance.
 */
void rpmsg_master_recover(struct rpmsg_lite_instance *rpmsg)
{
    struct rpmsg_lite_instance *my_rpmsg = master_rpmsg;
    struct rpmsg_msg_t *msg;
    uint32_t msg_len;

    rpmsg_lite_master_env_reset(rpmsg);
    /* notice remote side "I'm ready." */
    msg = rpmsg_lite_alloc_tx_buffer(my_rpmsg, &msg_len, RL_BLOCK);
    msg->msg_type = RPMSG_MSG_TYPE_MASTER_READY;
    rpmsg_lite_send_nocopy(my_rpmsg, ept_sync, EPT_ADDR_SYNC, msg, msg_len);
}

/************************************************************************************
 * @fn      rpmsg_remote_init
 *
 * @brief   Initialize rpmsg-lite remote side, this function is used in CM33 side in general usage.
 *
 * @param   callback: callback function to receive message from the other side.
 *
 * @return  initialized rpmsg-lite remote instance
 */
struct rpmsg_lite_instance *rpmsg_remote_init(void (*recv)(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t *msg))
{
    struct rpmsg_lite_instance *my_rpmsg;
    struct rpmsg_msg_t *msg;
    uint32_t src_addr;
    uint32_t msg_len;
    
    my_rpmsg = rpmsg_lite_remote_init((void *)&rpmsg_share_mem[0], RPMSG_LITE_LINK_ID, RL_NO_FLAGS);
    
//    while (0 == rpmsg_lite_is_link_up(my_rpmsg));

    queue_sync  = rpmsg_queue_create(my_rpmsg);
    queue_async = rpmsg_queue_create(my_rpmsg);
    ept_sync  = rpmsg_lite_create_ept(my_rpmsg, EPT_ADDR_SYNC, rpmsg_queue_rx_cb, queue_sync);
    ept_async = rpmsg_lite_create_ept(my_rpmsg, EPT_ADDR_ASYNC, rpmsg_queue_rx_cb, queue_async);
    
    env_create_mutex((void **)&ept_sync_lock, 1);
    
    msg_callback = recv;
    remote_rpmsg = my_rpmsg;

    return my_rpmsg;
}

/************************************************************************************
 * @fn      rpmsg_wait_master_ready
 *
 * @brief   used by remote side to wait for master become ready after calling rpmsg_remote_init and
 *          boot up DSP (master side).
 *
 * @param   rpmsg: rpmsg instance of remote side.
 */
void rpmsg_wait_master_ready(struct rpmsg_lite_instance *rpmsg)
{
    struct rpmsg_msg_t *msg;
    uint32_t src_addr;
    uint32_t msg_len;

    /* wait for "I'm ready." from master side */
    rpmsg_queue_recv_nocopy(rpmsg, queue_sync, (uint32_t *)&src_addr, (char **)&msg, &msg_len, RL_BLOCK);
    while ((src_addr != EPT_ADDR_SYNC) || (msg->msg_type != RPMSG_MSG_TYPE_MASTER_READY));
    rpmsg_lite_release_rx_buffer(rpmsg, msg);
}

/************************************************************************************
 * @fn      rpmsg_destroy
 *
 * @brief   destroy an initialized rpmsg-lite instance.
 *
 * @param   rpmsg: rpmsg-lite instance.
 */
void rpmsg_destroy(struct rpmsg_lite_instance *rpmsg)
{
    env_lock_mutex(ept_sync_lock);

    (void)rpmsg_lite_destroy_ept(rpmsg, ept_sync);
    ept_sync = ((void *)0);
    (void)rpmsg_lite_destroy_ept(rpmsg, ept_async);
    ept_async = ((void *)0);
    (void)rpmsg_queue_destroy(rpmsg, queue_sync);
    queue_sync = ((void *)0);
    (void)rpmsg_queue_destroy(rpmsg, queue_async);
    queue_async = ((void *)0);
    (void)rpmsg_lite_deinit(rpmsg);
    
    if (rpmsg == remote_rpmsg) {
        remote_rpmsg = NULL;
    }

    if (rpmsg == master_rpmsg) {
        master_rpmsg = NULL;
    }

    env_unlock_mutex(ept_sync_lock);
    
    env_delete_mutex(ept_sync_lock);
    ept_sync_lock = ((void *)0);
}

/************************************************************************************
 * @fn      rpmsg_recv_msg
 *
 * @brief   Called by app layer to receive message from the other side in blocking mode.
 *
 * @param   rpmsg: rpmsg-lite instance.
 * @param   msg: data storage address
 * @param   msg_len: message length
 *
 * @return  the endpoint address used by the other side to send this message
 */
uint32_t rpmsg_recv_msg(struct rpmsg_lite_instance *rpmsg, struct rpmsg_msg_t **msg, uint32_t *msg_len)
{
    uint32_t src_addr;

    rpmsg_queue_recv_nocopy(rpmsg, queue_async, (uint32_t *)&src_addr, (char **)msg, msg_len, RL_BLOCK);

    return src_addr;
}

/************************************************************************************
 * @fn      rpmsg_get_remote_instance
 *
 * @brief   Called by other module to get created remote_rpmsg instance.
 *
 * @return  remote rpmsg instance
 */
struct rpmsg_lite_instance *rpmsg_get_remote_instance(void)
{
    return remote_rpmsg;
}

/************************************************************************************
 * @fn      rpmsg_get_master_instance
 *
 * @brief   Called by other module to get created master_rpmsg instance.
 *
 * @return  master rpmsg instance
 */
struct rpmsg_lite_instance *rpmsg_get_master_instance(void)
{
    return master_rpmsg;
}

static int32_t rpmsg_queue_rx_tmp(void *payload, uint32_t payload_len, uint32_t src, void *priv)
{
    struct rpmsg_msg_t *msg = payload;
    if ((src == EPT_ADDR_SYNC) && (msg->msg_type == RPMSG_MSG_TYPE_MASTER_READY)) {
        ept_sync->rx_cb = rpmsg_queue_rx_cb;
        rpmsg_lite_release_rx_buffer_dur_recover(rpmsg_get_remote_instance(), msg);
    }
    return 0;
}

/************************************************************************************
 * @fn      rpmsg_remote_recover
 *
 * @brief   In wake up procedure, this function is used to recover rpmsg from SHUTDOWN
 *          mode. Reinit IPC, reset internal enviroment and wait for MASTER READY message.
 *          This function should be called before interrupt is enabled.
 */
void rpmsg_remote_recover(void)
{
    GLOBAL_INT_DISABLE();
    rpmsg_lite_remote_env_reset(rpmsg_get_remote_instance());
    ept_sync->rx_cb = rpmsg_queue_rx_tmp;
    while (ept_sync->rx_cb == rpmsg_queue_rx_tmp) {
        void ipc_mcu_irq(void);
        ipc_mcu_irq();
    }
    GLOBAL_INT_RESTORE();
}

/************************************************************************************
 * @fn      rpmsg_remote_resume
 *
 * @brief   In wake up procedure, this function is used to recover rpmsg from DEEP SLEEP
 *          mode. Reinit IPC and wait for MASTER READY message.
 */
void rpmsg_remote_resume(void)
{
    GLOBAL_INT_DISABLE();
    rpmsg_lite_remote_env_resume();
    ept_sync->rx_cb = rpmsg_queue_rx_tmp;
    while (ept_sync->rx_cb == rpmsg_queue_rx_tmp) {
        void ipc_mcu_irq(void);
        ipc_mcu_irq();
    }
    GLOBAL_INT_RESTORE();
}

//void rpmsg_task(void *arg)
//{
//    struct rpmsg_msg_t *msg;
//    uint32_t msg_len;
//    uint32_t src_addr;

//    while (1) {
//        rpmsg_queue_recv_nocopy(arg, queue_async, (uint32_t *)&src_addr, (char **)&msg, &msg_len, RL_BLOCK);
//
//        if (msg_callback) {
//            msg_callback(arg, msg);
//        }
//    }
//}
