/****************************************************************************************
*c-2020 freqchip
*
*
******************************************************************************************/
#include <stdio.h>
#include <string.h>  
#include "driver_system.h"
#include "driver_rtc.h"
#include "plf.h"
#include "frspim.h"
#include "driver_pmu.h"
#include "driver_cali.h"
volatile uint32_t systick_irq_count;
volatile uint32_t alma_flag;
volatile uint32_t almb_flag;
void SysTick_Handler(void)
{
    systick_irq_count++;
    
}

void systick_start(void)
{
    systick_irq_count = 0;
    SysTick_Config(system_get_clock() / 1000);
    NVIC_EnableIRQ(SysTick_IRQn);
}

uint32_t systick_stop(void)
{
    NVIC_DisableIRQ(SysTick_IRQn);

    return systick_irq_count;
}

/*********************************************************************
 * @fn      rtc_init
 *
 * @brief   Initialize rtc module.
 *
 * @param   None
 *
 * @return  None.
 */
 void rtc_init(void)
 {
   
   //设置时钟
   frspim_wr(PMU_CLK_RST_CTRL,1,0x00);  // system base rc clock
   frspim_wr(PMU_CLK_EN,1,0x02);  //clk enable
   frspim_wr(PMU_SOFT_RST,1,0x00);  //release rst
     
   //set rtc initial value
   frspim_wr(PMU_REG_RTC_UPDATE_0,4,0);
   frspim_wr(PMU_RTC_CRL,1,PMU_UPDATE_EN);
   while(frspim_rd(PMU_RTC_CRL,1)&0x01); 
 }
 
 void rtc_stop(void)
 {
     frspim_wr(PMU_CLK_EN,1,0x00);  //clk enable  
 }
 
 /*********************************************************************
 * @fn      rtc_alarm
 *
 * @brief   start running a rtc.
 *
 * @param   rtc_idx     - RTC_A or RTC_B, @ref rtc_idx_t
 *          count_ms    - timer duration with ms as unit. range [1,4294967]
 *
 * @return  None.
 */
void rtc_alarm(enum rtc_idx_t rtc_idx, uint32_t count_ms,uint32_t cali_cnt)
{
    uint32_t tmp_high, tmp_low;
    uint32_t tmp, cnt;
    cnt = (count_ms*1000000)/cali_cnt;
    //读取RTC的当前值
    tmp = frspim_rd(PMU_REG_RTC_UPDATE_0,4);
    if(rtc_idx == RTC_A)
    {
        frspim_wr(PMU_REG_RTC_ALMA_VAL0, 4 , cnt + tmp);
        frspim_wr(PMU_RTC_CRL,1,frspim_rd(PMU_RTC_CRL,1)|ALAMA_EN);
    }
    else
    {
        frspim_wr(PMU_REG_RTC_ALMB_VAL0, 4 , cnt + tmp);
        frspim_wr(PMU_RTC_CRL,1,frspim_rd(PMU_RTC_CRL,1)|ALAMB_EN);
    }
} 

void clear_alama(void)
{
    frspim_wr(PMU_RTC_CRL,1,frspim_rd(PMU_RTC_CRL,1)|ALAMA_CLR);
    while(frspim_rd(PMU_RTC_CRL,1)&ALAMA_CLR);
}
void clear_alamb(void)
{
   frspim_wr(PMU_RTC_CRL,1,frspim_rd(PMU_RTC_CRL,1)|ALAMB_CLR); 
   while(frspim_rd(PMU_RTC_CRL,1)&ALAMB_CLR);
}
void rtc_isr_ram(uint8_t rtc_idx)
{
    if(rtc_idx == RTC_A)
    {
        alma_flag = 1;
        clear_alama();
        printf("rtcA\r\n");
    }
    if(rtc_idx == RTC_B)
    {
        almb_flag = 1;
        clear_alamb();
        rtc_stop();
        printf("rtcB\r\n");
    }
    
}
void tube_exit(void)
{
    *(int *)TUBE_BASE = 0x04;
}
 void rtc_test(void)
 {
   uint32_t cali_cnt;
     alma_flag = 0;
     almb_flag = 0;
    printf("start rtc\r\n");
    frspim_init(2);
    NVIC_EnableIRQ(PMU_IRQn);
    cali_cnt = cali_rc();
    rtc_init();
    rtc_alarm(RTC_A,5,cali_cnt);
    rtc_alarm(RTC_B,7,cali_cnt);
    while(!almb_flag);
    tube_exit();
    
    
  #if 0
   uint32_t tim_cnt;
   frspim_init(2);
   //设置时钟
   frspim_wr(PMU_CLK_RST_CTRL,1,0x00);  // system base rc clock
   frspim_wr(PMU_CLK_EN,1,0x02);  //clk enable
   frspim_wr(PMU_SOFT_RST,1,0x00);  //release rst
  
  
   //更新RTC的值
  systick_start();
  
  while(1)
  {
   frspim_wr(PMU_REG_RTC_UPDATE_0,4,0);
   frspim_wr(PMU_RTC_CRL,1,PMU_UPDATE_EN);
   while(frspim_rd(PMU_RTC_CRL,1)&0x01);
   systick_irq_count = 0;
   while(systick_irq_count<5);
   frspim_wr(PMU_RTC_CRL,1,PMU_VAL_RD);
   while(frspim_rd(PMU_RTC_CRL,1)&0x02);
   printf("%d\r\n",frspim_rd(PMU_REG_RTC_UPDATE_0,4));
  } 
  #endif  
 }


