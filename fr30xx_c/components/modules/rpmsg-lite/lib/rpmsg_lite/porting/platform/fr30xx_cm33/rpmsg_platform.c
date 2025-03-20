/*
 * Copyright 2019-2021 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>

#include "rpmsg_platform.h"
#include "rpmsg_env.h"

#include "co_util.h"

#include "fr30xx.h"

#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
#error "This RPMsg-Lite port requires RL_USE_ENVIRONMENT_CONTEXT set to 0"
#endif

/* The IPC instance used for CM33 and DSP core communication */
static IPC_HandleTypeDef ipc_mcu;

static int32_t isr_counter     = 0;
static int32_t disable_counter = 0;
static void *platform_lock;

static void platform_global_isr_disable(void)
{
    __asm volatile("cpsid i");
}

static void platform_global_isr_enable(void)
{
    __asm volatile("cpsie i");
}

int32_t platform_init_interrupt(uint32_t vector_id, void *isr_data)
{
    /* Register ISR to environment layer */
    env_register_isr(vector_id, isr_data);

    env_lock_mutex(platform_lock);

    RL_ASSERT(0 <= isr_counter);
//    if (isr_counter < 2)
//    {
//        printf("MU_EnableInterrupts(APP_MU, 1UL << (31UL - vector_id));");
//    }
    isr_counter++;

    env_unlock_mutex(platform_lock);

    return 0;
}

int32_t platform_deinit_interrupt(uint32_t vector_id)
{
    /* Prepare the MU Hardware */
    env_lock_mutex(platform_lock);

    RL_ASSERT(0 < isr_counter);
    isr_counter--;
//    if (isr_counter < 2)
//    {
//        printf("MU_DisableInterrupts(APP_MU, 1UL << (31UL - vector_id));");
//    }

    /* Unregister ISR from environment layer */
    env_unregister_isr(vector_id);

    env_unlock_mutex(platform_lock);

    return 0;
}

void platform_notify(uint32_t vector_id)
{
    env_lock_mutex(platform_lock);
    if (vector_id == 0) {
        ipc_msg_send(&ipc_mcu, IPC_CH_0, 0);
    }
    else if (vector_id == 1) {
        ipc_msg_send(&ipc_mcu, IPC_CH_1, 0);
    }
    else {
        while(1);
    }
    env_unlock_mutex(platform_lock);
}

/**
 * platform_time_delay
 *
 * @param num_msec Delay time in ms.
 *
 * This is not an accurate delay, it ensures at least num_msec passed when return.
 */
void platform_time_delay(uint32_t num_msec)
{
    system_delay_us(1000 * num_msec);
}

/**
 * platform_in_isr
 *
 * Return whether CPU is processing IRQ
 *
 * @return True for IRQ, false otherwise.
 *
 */
int32_t platform_in_isr(void)
{
    return (((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0UL) ? 1 : 0);
}

/**
 * platform_interrupt_enable
 *
 * Enable peripheral-related interrupt
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int32_t platform_interrupt_enable(uint32_t vector_id)
{
    RL_ASSERT(0 < disable_counter);

    platform_global_isr_disable();
    disable_counter--;

    if (disable_counter == 0)
    {
        NVIC_EnableIRQ(IPC_MCU_IRQn);
    }
    platform_global_isr_enable();
    return ((int32_t)vector_id);
}

/**
 * platform_interrupt_disable
 *
 * Disable peripheral-related interrupt.
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int32_t platform_interrupt_disable(uint32_t vector_id)
{
    RL_ASSERT(0 <= disable_counter);

    platform_global_isr_disable();
    /* virtqueues use the same NVIC vector
       if counter is set - the interrupts are disabled */
    if (disable_counter == 0)
    {
        NVIC_DisableIRQ(IPC_MCU_IRQn);
    }
    disable_counter++;
    platform_global_isr_enable();
    return ((int32_t)vector_id);
}

/**
 * platform_map_mem_region
 *
 * Dummy implementation
 *
 */
void platform_map_mem_region(uint32_t vrt_addr, uint32_t phy_addr, uint32_t size, uint32_t flags)
{
}

/**
 * platform_cache_all_flush_invalidate
 *
 * Dummy implementation
 *
 */
void platform_cache_all_flush_invalidate(void)
{
}

/**
 * platform_cache_disable
 *
 * Dummy implementation
 *
 */
void platform_cache_disable(void)
{
}

/**
 * platform_vatopa
 *
 * Translate CM33 addresses to DSP addresses
 *
 */
uint32_t platform_vatopa(void *addr)
{
    return (uint32_t)addr;
}

/**
 * platform_patova
 *
 * Translate DSP addresses to CM33 addresses
 *
 */
void *platform_patova(uint32_t addr)
{
    return (void *)addr;
}

static void ipc_mcu_rx(struct __IPC_HandleTypeDef *hipc, enum_IPC_Chl_Sel_t ch, uint32_t msg)
{
//    printf("ipc_mcu_rx: %d, 0x%08x\r\n", ch, msg);
    
    if (ch == IPC_CH_0) {
        env_isr(0);
    }
    else if (ch == IPC_CH_1) {
        env_isr(1);
    }
}

static void ipc_mcu_tx(struct __IPC_HandleTypeDef *hipc, enum_IPC_Chl_Sel_t ch)
{
//    printf("ipc_mcu_tx: %d\r\n", ch);
}

void ipc_mcu_irq(void)
{
    ipc_IRQHandler(&ipc_mcu);
}

/**
 * platform_init
 *
 * platform/environment init
 */
int32_t platform_init(void)
{
    __SYSTEM_APP_IPC_CLK_ENABLE();
    ipc_mcu.IPCx = IPC_MCU;
    ipc_mcu.RxEnableChannels = IPC_CH_0 | IPC_CH_1;
    ipc_mcu.TxEnableChannels = IPC_CH_0 | IPC_CH_1;
    ipc_mcu.RxCallback = ipc_mcu_rx;
    ipc_mcu.TxCallback = ipc_mcu_tx;
    ipc_init(&ipc_mcu);

    /* Create lock used in multi-instanced RPMsg */
    if (0 != env_create_mutex(&platform_lock, 1))
    {
        return -1;
    }

    return 0;
}

/**
 * platform_deinit
 *
 * platform/environment deinit process
 */
int32_t platform_deinit(void)
{
    /* Delete lock used in multi-instanced RPMsg */
    env_delete_mutex(platform_lock);
    platform_lock = ((void *)0);
    return 0;
}

/**
 * platform_reinit
 *
 * platform/environment reinit
 */
int32_t platform_reinit(void)
{
    __SYSTEM_APP_IPC_CLK_ENABLE();
    ipc_init(&ipc_mcu);
    NVIC_EnableIRQ(IPC_MCU_IRQn);

    return 0;
}
