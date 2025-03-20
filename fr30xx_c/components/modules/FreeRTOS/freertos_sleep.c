#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "co_util.h"

#include "fr30xx.h"

#define FLASH_POWER_DOWN                      0
#define DEBUG_TIMING                          0
#define DEBUG_UART_BASE                       UART3_BASE

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )

#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )

#define DELAY_BEFORE_ENTER_SLEEP              800

uint8_t rwip_calc_sleep_time(int32_t *sleep_duration);
/* base: BIT0~27 is valid, unit is 312.5us; fine: 0~624, unit is 0.5us */
void rwip_counter_get(uint32_t *base, uint16_t *fine);
void low_power_save_cpu(void);
void low_power_restore_cpu(void);
void rwip_wakeup_start(void);
bool rwip_wakeup_is_ongoing(void);

/*
 * The number of SysTick increments that make up one tick period.
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    extern uint32_t ulTimerCountsForOneTick;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    extern uint32_t xMaximumPossibleSuppressedTicks;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
#if ( configUSE_TICKLESS_IDLE == 1 )
    extern uint32_t ulStoppedTimerCompensation;
#endif /* configUSE_TICKLESS_IDLE */

__RAM_CODE static uint32_t ms_2_lpcycles(uint32_t ms)
{
    uint32_t lpcycles;        
    uint32_t tmp_low, tmp_high;

    mul_64(&tmp_low, &tmp_high, system_get_LPRCCLK(), ms);
    lpcycles = simple_div_64(tmp_low, tmp_high, 1000);

    return(lpcycles);
}

__RAM_CODE static uint32_t us_2_lpcycles(uint32_t us)
{
    uint32_t lpcycles;        
    uint32_t tmp_low, tmp_high;

    mul_64(&tmp_low, &tmp_high, system_get_LPRCCLK(), us);
    lpcycles = simple_div_64(tmp_low, tmp_high, 1000000);

    return(lpcycles);
}

__RAM_CODE uint32_t lpcycles_2_us(uint32_t lp_cyc)
{
    uint32_t us;        
    uint32_t tmp_low, tmp_high;

    mul_64(&tmp_low, &tmp_high, 1000000, lp_cyc);
    us = simple_div_64(tmp_low, tmp_high, system_get_LPRCCLK());

    return(us);
}

__RAM_CODE void low_power_enter_sleep(void)
{
#if DEBUG_TIMING
    *(volatile uint32_t *)0x50110000 = 'e';
#endif
#if DELAY_BEFORE_ENTER_SLEEP
    system_delay_us(DELAY_BEFORE_ENTER_SLEEP);
#endif
    trim_reconfig_pkvdd(false, 1);
    ool_write(PMU_REG_SW_OP, 0x01);
    while(1);
}

__RAM_CODE static void low_power_enter(void)
{
    uint32_t org_value;
    org_value = ool_read32(0xfc);
    if (org_value & 0x01) {
        ool_write32(0xfc/*PMU_REG_CPU_RESET_VECTOR*/, (uint32_t)low_power_restore_cpu | 0x01);
    }
    else {
        ool_write32(0xfc/*PMU_REG_CPU_RESET_VECTOR*/, (uint32_t)low_power_restore_cpu & (~0x01));
    }
    low_power_save_cpu();
}

__RAM_CODE __WEAK bool user_deep_sleep_check(void)
{
    return true;
}

__RAM_CODE __WEAK void user_entry_before_sleep(void)
{
}

__RAM_CODE __WEAK void user_entry_after_sleep(void)
{
}

__RAM_CODE __WEAK void user_entry_after_sleep_user(void)
{
}

#if DEBUG_TIMING
__RAM_CODE __STATIC_INLINE void init_uart_for_debug(void)
{
    __SYSTEM_UART3_CLK_ENABLE();

    *(volatile uint32_t *)0xE005012c = 0x00009911;
    
    *(volatile uint32_t *)(DEBUG_UART_BASE+0x0c) = 0x03;
    *(volatile uint32_t *)(DEBUG_UART_BASE+0x1c) = 0x09;
    *(volatile uint32_t *)(DEBUG_UART_BASE+0x08) = 0x09;
    
    /* baudrate */
    *(volatile uint32_t *)(DEBUG_UART_BASE+0x0c) = 0x83;
    *(volatile uint32_t *)(DEBUG_UART_BASE+0x00) = 0x01;
    *(volatile uint32_t *)(DEBUG_UART_BASE+0x04) = 0x00;
    *(volatile uint32_t *)(DEBUG_UART_BASE+0xc0) = 0x28;
    *(volatile uint32_t *)(DEBUG_UART_BASE+0x0c) = 0x03;
}
#endif

volatile TickType_t latest_idle_time;
__RAM_CODE void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
    uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements;
    uint32_t freecounter_start, freecounter_dur;
    
    latest_idle_time = xExpectedIdleTime;

    /* Make sure the Sleep LP counter does not overflow RTC . */
    if( xExpectedIdleTime > (0x01FFFFFF / portTICK_RATE_MS) )
    {
        xExpectedIdleTime = 0x01FFFFFF / portTICK_RATE_MS;
    }

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
     * method as that will mask interrupts that should exit sleep mode. */
    __disable_irq();
    __DSB();
    __ISB();
    
    /* Stop the SysTick momentarily.  The time the SysTick is stopped for
     * is accounted for as best it can be, but using the tickless mode will
     * inevitably result in some tiny drift of the time maintained by the
     * kernel with respect to calendar time. */
    portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;
    freecounter_start = FREE_COUNTER_VALUE;
    
    /* If a context switch is pending or a task is waiting for the scheduler
     * to be unsuspended then abandon the low power entry. */
    if ( eTaskConfirmSleepModeStatus() == eAbortSleep )
    {
        /* Restart SysTick. */
        portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

        /* Re-enable interrupts - see comments above __disable_irq() call
         * above. */
        __enable_irq();
    }
    else
    {
        int ms = xExpectedIdleTime * portTICK_RATE_MS;
        int us;
        bool deep_sleep = false;

        if ((ms < 5) 
                || (system_prevent_sleep_get()!=0)
                || (user_deep_sleep_check()==false)) {   // enter light sleep mode
            /* Make sure the SysTick reload value does not overflow the counter. */
            if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
            {
                xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
            }

            /* Calculate the reload value required to wait xExpectedIdleTime
             * tick periods.  -1 is used because this code will execute part way
             * through one of the tick periods. */
            ulReloadValue = portNVIC_SYSTICK_CURRENT_VALUE_REG + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );

            if( ulReloadValue > ulStoppedTimerCompensation )
            {
                ulReloadValue -= ulStoppedTimerCompensation;
            }

            /* Set the new reload value. */
            portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

            /* Clear the SysTick count flag and set the count value back to
             * zero. */
            portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

            /* Restart SysTick. */
            portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

//            __DSB();
//            __WFI();
//            __ISB();

            /* Disable the SysTick clock without reading the
             * portNVIC_SYSTICK_CTRL_REG register to ensure the
             * portNVIC_SYSTICK_COUNT_FLAG_BIT is not cleared if it is set.  Again,
             * the time the SysTick is stopped for is accounted for as best it can
             * be, but using the tickless mode will inevitably result in some tiny
             * drift of the time maintained by the kernel with respect to calendar
             * time*/
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT );

            /* Determine if the SysTick clock has already counted to zero and
             * been set back to the current reload value (the reload back being
             * correct for the entire expected idle time) or if the SysTick is yet
             * to count to zero (in which case an interrupt other than the SysTick
             * must have brought the system out of sleep mode). */
            if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
            {
                uint32_t ulCalculatedLoadValue;

                /* The tick interrupt is already pending, and the SysTick count
                 * reloaded with ulReloadValue.  Reset the
                 * portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
                 * period. */
                ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

                portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

                /* As the pending tick will be processed as soon as this
                 * function exits, the tick value maintained by the tick is stepped
                 * forward by one less than the time spent waiting. */
                ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
            }
            else
            {
                /* Something other than the tick interrupt ended the sleep.
                 * Work out how long the sleep lasted rounded to complete tick
                 * periods (not the ulReload value which accounted for part
                 * ticks). */
                ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - portNVIC_SYSTICK_CURRENT_VALUE_REG;

                /* How many complete tick periods passed while the processor
                 * was waiting? */
                ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

                /* The reload value is set to whatever fraction of a single tick
                 * period remains. */
                portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
            }
        }
        else {  // enter deep sleep mode
            uint32_t sleep_us;
            uint32_t escaped_system_tick, tail_system_tick;
            uint32_t before_counter, after_counter;
            
            vTaskSuspendAll();
            deep_sleep = true;
            
            escaped_system_tick = ulTimerCountsForOneTick - portNVIC_SYSTICK_CURRENT_VALUE_REG;

#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = 'a';
#endif
            /* notice APP layer that system will enter deep sleep mode */
#if defined(CHIP_SEL_FR3066DQC_V1) || defined(CHIP_SEL_FR3068EC_V1) || defined(CHIP_SEL_FR3066EQC)
            ///mcu pb2 ---> bt pa6, for wakeup
            SYSTEM->PortB_PullSelect |= GPIO_PIN_2;
            SYSTEM->PortB_PullEN |= GPIO_PIN_2;
            SYSTEM->PortB_L_FuncMux &= 0xfffff0ff; 
#endif
            user_entry_before_sleep();

            pmu_adjust_onoff_timing(1700);

            /* prepare for deep sleep mode */
            uint32_t reset_vector = SCB->VTOR;
            uint32_t rtos_priorities = portNVIC_SHPR3_REG;
            ool_write(PMU_REG_STATUS, PMU_STATUS_DEEP_SLEEP);

            us = ms * 1000;
            us -= (escaped_system_tick * 1000 * portTICK_RATE_MS) / ulTimerCountsForOneTick;
            us -= (1700+100);
            us -= (freecounter_start - FREE_COUNTER_VALUE);

            /* 
             * 1. record start RTC counter, used to calculate actual sleep time
             * 2. setup target RTC counter
             */
            {
                uint32_t target_counter;

#if DEBUG_TIMING
                *(volatile uint32_t *)DEBUG_UART_BASE = 'b';
#endif
                ool_write(PMU_REG_RTC_CTRL, ool_read(PMU_REG_RTC_CTRL) | PMU_RTC_SAMPLE_BIT);
                while (ool_read(PMU_REG_RTC_CTRL) & PMU_RTC_SAMPLE_BIT);
#if DEBUG_TIMING
                *(volatile uint32_t *)DEBUG_UART_BASE = 'c';
#endif
                before_counter = ool_read32(PMU_REG_RTC_COUNTER_0);
                target_counter = before_counter + us_2_lpcycles(us);
                ool_write32(PMU_REG_ALARM_B_COUNTER_0, target_counter);
                ool_write(PMU_REG_RTC_CTRL, ool_read(PMU_REG_RTC_CTRL) | PMU_RTC_ALARM_B_EN_BIT);
            }
            
            freecounter_dur = freecounter_start - FREE_COUNTER_VALUE;
#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = 'd';
#endif
            trim_reconfig_pkvdd(false, 0);
            flash_enter_deep_sleep(QSPI0);
            ool_write(PMU_REG_PMU_GATE_H, ool_read(PMU_REG_PMU_GATE_H) | 0x02);
            low_power_enter();
            
/* FPU settings */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
            SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));    /* set CP10 and CP11 Full Access */
#endif
            /* SOC FCLK enable */
            __SYSTEM_SOC_FCLK_ENABLE();
            
            /* MISC */ 
            __SYSTEM_MISC_CLK_ENABLE();
            __SYSTEM_MISC_IO_CONFIG();

#if DEBUG_TIMING
            init_uart_for_debug();
            *(volatile uint32_t *)DEBUG_UART_BASE = '1';
#endif

            /* init internal flash with default settings */
            __SYSTEM_PFC_CLK_ENABLE();
            __SYSTEM_QSPI0_CLK_ENABLE();
            __SYSTEM_APB_CLK_ENABLE();
            __SYSTEM_APB1_CLK_ENABLE();
            system_cache_enable(false);
            flash_init_controller(QSPI0, FLASH_RD_TYPE_DUAL, FLASH_WR_TYPE_SINGLE);
            flash_set_baudrate(QSPI0, QSPI_BAUDRATE_DIV_4);
            ool_write(PMU_REG_PMU_GATE_H, ool_read(PMU_REG_PMU_GATE_H) & (~0x02));
            flash_exit_deep_sleep(QSPI0);

#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = '2';
#endif
            trim_reconfig_pkvdd(true, 0);
            ool_write(PMU_REG_STATUS, PMU_STATUS_NORAML);
            SCB->VTOR = reset_vector;
            portNVIC_SHPR3_REG = rtos_priorities;
            
#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = '3';
#endif

            {
                uint32_t target;
                ool_write(PMU_REG_RTC_CTRL, (ool_read(PMU_REG_RTC_CTRL) & (~PMU_RTC_ALARM_B_EN_BIT)) | PMU_RTC_SAMPLE_BIT | PMU_RTC_ALARM_B_CLR_BIT);
                while (ool_read(PMU_REG_RTC_CTRL) & PMU_RTC_SAMPLE_BIT);
                after_counter = ool_read32(PMU_REG_RTC_COUNTER_0);
            }
#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = '4';
#endif
            sleep_us = lpcycles_2_us(after_counter - before_counter);
            sleep_us += freecounter_dur;
#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = '5';
#endif
            /* 
             * 1. how many ticks have passed during sleep mode, ulCompleteTickPeriods
             * 2. how many system ticks left in the following RTOS tick, tail_system_tick
             */
            ulCompleteTickPeriods = pdMS_TO_TICKS(sleep_us / 1000);
            tail_system_tick = (pdMS_TO_TICKS(sleep_us % 1000) * ulTimerCountsForOneTick) / 1000;
            tail_system_tick += escaped_system_tick;
            if (tail_system_tick >= ulTimerCountsForOneTick) {
                tail_system_tick -= ulTimerCountsForOneTick;
                ulCompleteTickPeriods++;
            }
#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = ulCompleteTickPeriods;
            *(volatile uint32_t *)DEBUG_UART_BASE = tail_system_tick>>24;
            *(volatile uint32_t *)DEBUG_UART_BASE = tail_system_tick>>16;
            *(volatile uint32_t *)DEBUG_UART_BASE = tail_system_tick>>8;
            *(volatile uint32_t *)DEBUG_UART_BASE = tail_system_tick>>0;
#endif

            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT );
            portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1 - tail_system_tick;
#if DEBUG_TIMING
            *(volatile uint32_t *)DEBUG_UART_BASE = '6';
#endif
        }

        /* Restart SysTick so it runs from portNVIC_SYSTICK_LOAD_REG
         * again, then set portNVIC_SYSTICK_LOAD_REG back to its standard
         * value. */
        portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
        portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
        vTaskStepTick( ulCompleteTickPeriods );
        portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;

#if DEBUG_TIMING
        *(volatile uint32_t *)DEBUG_UART_BASE = '7';
#endif
        
        if (deep_sleep == true) {
            /* notice APP layer that system has recovery from deep sleep mode */
            user_entry_after_sleep();
        }
        
        /* Exit with interrupts enabled. */
        __enable_irq();
        
        if (deep_sleep == true) {
            user_entry_after_sleep_user();
            xTaskResumeAll();
        }

#if DEBUG_TIMING
        *(volatile uint32_t *)DEBUG_UART_BASE = '8';
#endif
    }
}
