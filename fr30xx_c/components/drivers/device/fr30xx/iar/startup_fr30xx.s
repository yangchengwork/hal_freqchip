;************************* (C) COPYRIGHT 2023 FreqChip ***************************
;* File Name          : startup_fr30xx.s
;* Author             : FreqChip Firmware Team
;* Version            : V1.0.0
;* Date               : 2022
;* Description        : fr30xx Devices vector table for IAR toolchain. 
;*                      This module performs:
;*                      - Set the initial SP
;*                      - Set the initial PC == Reset_Handler
;*                      - Set the vector table entries with the exceptions ISR address
;*                      - Configure the clock system
;*                      - Branches to __main in the C library (which eventually
;*                        calls main()).
;*                      After Reset the Cortex-M33 processor is in Thread mode,
;*                      priority is Privileged, and the Stack is set to Main.
;*********************************************************************************
;* @attention
;*
;* Copyright (c) 2022 FreqChip.
;* All rights reserved.
;*******************************************************************************
;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbol _program_start or
; a user defined start symbol.
; To override the cstartup defined in the library, simply add your modified
; version to the workbench project.
;
; The vector table is normally located at address 0.
; When debugging in RAM, it can be located in RAM, aligned to at least 2^6.
; The name "__vector_table" has special meaning for C-SPY:
; it is where the SP start value is found, and the NVIC vector
; table register (VTOR) is initialized to this address if != 0.
;
; Cortex-M version
;

        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        EXTERN  SystemInit
        PUBLIC  __vector_table
        PUBLIC  __Vectors
        PUBLIC  __Vectors_End
        PUBLIC  __Vectors_Size

        DATA
__vector_table
        DCD     sfe(CSTACK)
        DCD     Reset_Handler              ; Reset Handler

        DCD     NMI_Handler                ; NMI Handler
        DCD     HardFault_Handler          ; Hard Fault Handler
        DCD     MemManage_Handler          ; MPU Fault Handler
        DCD     BusFault_Handler           ; Bus Fault Handler
        DCD     UsageFault_Handler         ; Usage Fault Handler
        DCD     0                          ; Secure Fault Handler
        DCD     0                          ; Reserved
        DCD     0                          ; Reserved
        DCD     0                          ; Reserved
        DCD     SVC_Handler                ; SVCall Handler
        DCD     DebugMon_Handler           ; Debug Monitor Handler
        DCD     0                          ; Reserved
        DCD     PendSV_Handler             ; PendSV Handler
        DCD     SysTick_Handler            ; SysTick Handler

         ; External Interrupts
        DCD      timer0_irq                          ;   0 timer0
        DCD      timer1_irq                          ;   1 timer1
        DCD      timer2_irq                          ;   2 timer2
        DCD      timer3_irq                          ;   3 timer3
        DCD      dma0_irq                            ;   4 dma0
        DCD      dma1_irq                            ;   5 dma1
        DCD      sdioh0_irq                          ;   6 sdioh0
        DCD      Interrupt7_Handler                  ;   7 Interrupt rsv
        DCD      ipc_mcu_irq                         ;   8 ipc mcu
        DCD      usbotg_irq                          ;   9 usbotg
        DCD      Interrupt10_Handler                 ;   10 Interrupt rsv
        DCD      Interrupt11_Handler                 ;   11 Interrupt rsv
        DCD      Interrupt12_Handler                 ;   12 Interrupt rsv
        DCD      Interrupt13_Handler                 ;   13 Interrupt rsv
        DCD      Interrupt14_Handler                 ;   14 Interrupt rsv
        DCD      Interrupt15_Handler                 ;   15 Interrupt rsv
        DCD      gpioa_irq                		     ;   16 GPIOA
        DCD      gpiob_irq                		     ;   17 GPIOB
        DCD      gpioc_irq                		     ;   18 GPIOC
        DCD      gpiod_irq                		     ;   19 GPIOD
        DCD      uart0_irq                           ;   20 uart0
        DCD      uart1_irq                           ;   21 uart1
        DCD      uart2_irq                           ;   22 uart2
        DCD      uart3_irq                           ;   23 uart3
        DCD      uart4_irq                           ;   24 uart4
        DCD      uart5_irq                           ;   25 uart5
        DCD      i2c0_irq                            ;   26 i2c0
        DCD      Interrupt27_Handler                 ;   27 Interrupt rsv
        DCD      i2c1_irq                            ;   28 i2c1
        DCD      Interrupt29_Handler                 ;   29 Interrupt rsv
        DCD      i2c2_irq                            ;   30 i2c2
        DCD      Interrupt31_Handler                 ;   31 Interrupt rsv
        DCD      spim0_irq                           ;   32 spim0
        DCD      spim1_irq                           ;   33 spim1
        DCD      Interrupt34_Handler                 ;   34 Interrupt rsv
        DCD      spis0_irq                           ;   35 spis0
        DCD      spis1_irq                           ;   36 spis1
        DCD      spimx8_0_irq                        ;   37 spimx8_0
        DCD      spimx8_1_irq                        ;   38 spimx8_1
        DCD      i2s0_irq                            ;   39 i2s0
        DCD      i2s1_irq                            ;   40 i2s1
        DCD      Interrupt41_Handler                 ;   41 Interrupt rsv
        DCD      pdm0_irq                            ;   42 pdm0
        DCD      pdm1_irq                            ;   43 pdm1
        DCD      Interrupt44_Handler                 ;   44 Interrupt rsv
        DCD      saradc_irq                          ;   45 saradc
        DCD      psd_dac_irq                         ;   46 psd_dac
        DCD      spdif_irq                           ;   47 spdif
        DCD      Interrupt48_Handler                 ;   48 Interrupt rsv
        DCD      Interrupt49_Handler                 ;   49 Interrupt rsv
        DCD      Interrupt50_Handler                 ;   50 Interrupt rsv
        DCD      parallel0_irq                       ;   51 parallel0
        DCD      wdt_irq                             ;   52 wdt_irq
        DCD      cali_irq                            ;   53 cali
        DCD      trng_irq                            ;   54 trng
        DCD      tick_irq                            ;   55 tick
        DCD      Interrupt56_Handler                 ;   56 Interrupt rsv
        DCD      Interrupt57_Handler                 ;   57 Interrupt rsv
        DCD      Interrupt58_Handler                 ;   58 Interrupt rsv
        DCD      Interrupt59_Handler                 ;   59 Interrupt rsv
        DCD      dsp_timer0_irq                      ;   60 dsp_timer0_irq
        DCD      dsp_timer1_irq                      ;   61 dsp_timer0_irq
        DCD      dsp_wdt_irq                         ;   62 dsp_wdt_irq
        DCD      ipc_dsp_irq                         ;   63 ipc
        DCD      yuv2rgb_irq                         ;   64 yuv2rgb
        DCD      pmu_irq                             ;   65 pmu
        DCD      pmu_lvd_irq                         ;   66 pmu lvd
        DCD      pmu_acok_irq                        ;   67 pmu acok
        DCD      pmu_wdt_irq                         ;   68 pmu wdt
        DCD      gpioe_irq                           ;   69 gpioe  
        DCD      can0_line0_irq                      ;   70 can0_line0_irq
        DCD      can0_line1_irq                      ;   71 can0_line1_irq
        DCD      can1_line0_irq                      ;   72 can1_line0_irq
        DCD      can1_line1_irq                      ;   73 can1_line1_irq 
        DCD      can2_line0_irq                      ;   74 can2_line0_irq
        DCD      can2_line1_irq                      ;   75 can2_line1_irq 
        DCD      can3_line0_irq                      ;   76 can3_line0_irq
        DCD      can3_line1_irq                      ;   77 can3_line1_irq
        DCD      Interrupt78_Handler                 ;   78 Interrupt rsv                  
        DCD      pwm0_irq                            ;   79 pwm0
        DCD      pwm1_irq                            ;   80 pwm1  
        DCD      timer4_irq                          ;   81 timer4
        DCD      timer5_irq                          ;   82 timer5                      
            
        ; OTA check flag
        DCD      0xAA55AA55                          ;   app check data
        DCD      0x00000001                          ;   app version
        DCD      0                                   ;   code length
__Vectors_End

__Vectors       EQU   __vector_table
__Vectors_Size  EQU   __Vectors_End - __Vectors

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
        THUMB
        PUBWEAK Reset_Handler
        SECTION .text:CODE:NOROOT:REORDER(2)
Reset_Handler
        LDR     R0, =SystemInit
        BLX     R0
        LDR     R0, =__iar_program_start
        BX      R0

        PUBWEAK NMI_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
NMI_Handler
        B NMI_Handler

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
HardFault_Handler
        B HardFault_Handler

        PUBWEAK MemManage_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
MemManage_Handler
        B MemManage_Handler

        PUBWEAK BusFault_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
BusFault_Handler
        B BusFault_Handler

        PUBWEAK UsageFault_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
UsageFault_Handler
        B UsageFault_Handler

        PUBWEAK SVC_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
SVC_Handler
        B SVC_Handler

        PUBWEAK DebugMon_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
DebugMon_Handler
        B DebugMon_Handler

        PUBWEAK PendSV_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
PendSV_Handler
        B PendSV_Handler

        PUBWEAK SysTick_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
SysTick_Handler
        B SysTick_Handler

        PUBWEAK timer0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
timer0_irq
        B timer0_irq

        PUBWEAK timer1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
timer1_irq
        B timer1_irq

        PUBWEAK timer2_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
timer2_irq
        B timer2_irq

        PUBWEAK timer3_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
timer3_irq
        B timer3_irq

        PUBWEAK dma0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
dma0_irq
        B dma0_irq

        PUBWEAK dma1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
dma1_irq
        B dma1_irq

        PUBWEAK sdioh0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
sdioh0_irq
        B sdioh0_irq

        PUBWEAK Interrupt7_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt7_Handler
        B Interrupt7_Handler

        PUBWEAK ipc_mcu_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
ipc_mcu_irq
        B ipc_mcu_irq

        PUBWEAK usbotg_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
usbotg_irq
        B usbotg_irq

        PUBWEAK Interrupt10_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt10_Handler
        B Interrupt10_Handler

        PUBWEAK Interrupt11_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt11_Handler
        B Interrupt11_Handler

        PUBWEAK Interrupt12_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt12_Handler
        B Interrupt12_Handler

        PUBWEAK Interrupt13_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt13_Handler
        B Interrupt13_Handler

        PUBWEAK Interrupt14_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt14_Handler
        B Interrupt14_Handler

        PUBWEAK Interrupt15_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt15_Handler
        B Interrupt15_Handler

        PUBWEAK gpioa_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
gpioa_irq
        B gpioa_irq

        PUBWEAK gpiob_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
gpiob_irq
        B gpiob_irq

        PUBWEAK gpioc_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
gpioc_irq
        B gpioc_irq

        PUBWEAK gpiod_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
gpiod_irq
        B gpiod_irq

        PUBWEAK uart0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
uart0_irq
        B uart0_irq

        PUBWEAK uart1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
uart1_irq
        B uart1_irq

        PUBWEAK uart2_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
uart2_irq
        B uart2_irq

        PUBWEAK uart3_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
uart3_irq
        B uart3_irq

        PUBWEAK uart4_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
uart4_irq
        B uart4_irq

        PUBWEAK uart5_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
uart5_irq
        B uart5_irq

        PUBWEAK i2c0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
i2c0_irq
        B i2c0_irq

        PUBWEAK Interrupt27_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt27_Handler
        B Interrupt27_Handler

        PUBWEAK i2c1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
i2c1_irq
        B i2c1_irq

        PUBWEAK Interrupt29_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt29_Handler
        B Interrupt29_Handler

        PUBWEAK i2c2_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
i2c2_irq
        B i2c2_irq

        PUBWEAK Interrupt31_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt31_Handler
        B Interrupt31_Handler

        PUBWEAK spim0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
spim0_irq
        B spim0_irq

        PUBWEAK spim1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
spim1_irq
        B spim1_irq

        PUBWEAK Interrupt34_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt34_Handler
        B Interrupt34_Handler

        PUBWEAK spis0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
spis0_irq
        B spis0_irq

        PUBWEAK spis1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
spis1_irq
        B spis1_irq

        PUBWEAK spimx8_0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
spimx8_0_irq
        B spimx8_0_irq

        PUBWEAK spimx8_1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
spimx8_1_irq
        B spimx8_1_irq

        PUBWEAK i2s0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
i2s0_irq
        B i2s0_irq

        PUBWEAK i2s1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
i2s1_irq
        B i2s1_irq

        PUBWEAK Interrupt41_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt41_Handler
        B Interrupt41_Handler

        PUBWEAK pdm0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pdm0_irq
        B pdm0_irq

        PUBWEAK pdm1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pdm1_irq
        B pdm1_irq

        PUBWEAK Interrupt44_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt44_Handler
        B Interrupt44_Handler

        PUBWEAK saradc_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
saradc_irq
        B saradc_irq

        PUBWEAK psd_dac_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
psd_dac_irq
        B psd_dac_irq

        PUBWEAK spdif_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
spdif_irq
        B spdif_irq

        PUBWEAK Interrupt48_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt48_Handler
        B Interrupt48_Handler

        PUBWEAK Interrupt49_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt49_Handler
        B Interrupt49_Handler

        PUBWEAK Interrupt50_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt50_Handler
        B Interrupt50_Handler

        PUBWEAK parallel0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
parallel0_irq
        B parallel0_irq

        PUBWEAK wdt_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
wdt_irq
        B wdt_irq

        PUBWEAK cali_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
cali_irq
        B cali_irq

        PUBWEAK trng_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
trng_irq
        B trng_irq

        PUBWEAK tick_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
tick_irq
        B tick_irq

        PUBWEAK Interrupt56_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt56_Handler
        B Interrupt56_Handler

        PUBWEAK Interrupt57_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt57_Handler
        B Interrupt57_Handler

        PUBWEAK Interrupt58_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt58_Handler
        B Interrupt58_Handler

        PUBWEAK Interrupt59_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt59_Handler
        B Interrupt59_Handler

        PUBWEAK dsp_timer0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
dsp_timer0_irq
        B dsp_timer0_irq

        PUBWEAK dsp_timer1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
dsp_timer1_irq
        B dsp_timer1_irq

        PUBWEAK dsp_wdt_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
dsp_wdt_irq
        B dsp_wdt_irq

        PUBWEAK ipc_dsp_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
ipc_dsp_irq
        B ipc_dsp_irq

        PUBWEAK yuv2rgb_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
yuv2rgb_irq
        B yuv2rgb_irq

        PUBWEAK pmu_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pmu_irq
        B pmu_irq

        PUBWEAK pmu_lvd_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pmu_lvd_irq
        B pmu_lvd_irq

        PUBWEAK pmu_acok_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pmu_acok_irq
        B pmu_acok_irq

        PUBWEAK pmu_wdt_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pmu_wdt_irq
        B pmu_wdt_irq

        PUBWEAK gpioe_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
gpioe_irq
        B gpioe_irq

        PUBWEAK can0_line0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can0_line0_irq
        B can0_line0_irq

        PUBWEAK can0_line1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can0_line1_irq
        B can0_line1_irq

        PUBWEAK can1_line0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can1_line0_irq
        B can1_line0_irq

        PUBWEAK can1_line1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can1_line1_irq
        B can1_line1_irq

        PUBWEAK can2_line0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can2_line0_irq
        B can2_line0_irq

        PUBWEAK can2_line1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can2_line1_irq
        B can2_line1_irq

        PUBWEAK can3_line0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can3_line0_irq
        B can3_line0_irq

        PUBWEAK can3_line1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
can3_line1_irq
        B can3_line1_irq

        PUBWEAK Interrupt78_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
Interrupt78_Handler
        B Interrupt78_Handler

        PUBWEAK pwm0_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pwm0_irq
        B pwm0_irq

        PUBWEAK pwm1_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
pwm1_irq
        B pwm1_irq

        PUBWEAK timer4_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
timer4_irq
        B timer4_irq

        PUBWEAK timer5_irq
        SECTION .text:CODE:NOROOT:REORDER(1)
timer5_irq
        B timer5_irq

        END
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
