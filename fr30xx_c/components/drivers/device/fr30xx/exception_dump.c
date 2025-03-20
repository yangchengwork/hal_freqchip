#include <stdint.h>
#include <stdio.h>

#include "fr30xx.h" // SCB

void HardFault_Handler_C(unsigned int* current_sp, uint32_t *exception_sp)
{
    printf("Crash, dump regs:\r\n");
    printf("R0    = 0x%08X\r\n",exception_sp[0]);
    printf("R1    = 0x%08X\r\n",exception_sp[1]);
    printf("R2    = 0x%08X\r\n",exception_sp[2]);
    printf("R3    = 0x%08X\r\n",exception_sp[3]);
    printf("R12   = 0x%08X\r\n",exception_sp[4]);
    printf("LR    = 0x%08X\r\n",exception_sp[5]);
    printf("PC    = 0x%08X\r\n",exception_sp[6]);
    printf("PSR   = 0x%08X\r\n",exception_sp[7]);
    
    printf("BFAR  = 0x%08X\r\n",*(unsigned int*)0xE000ED38);
    printf("CFSR  = 0x%08X\r\n",*(unsigned int*)0xE000ED28);
    printf("HFSR  = 0x%08X\r\n",*(unsigned int*)0xE000ED2C);
    printf("DFSR  = 0x%08X\r\n",*(unsigned int*)0xE000ED30);
    printf("AFSR  = 0x%08X\r\n",*(unsigned int*)0xE000ED3C);
    printf("SHCSR = 0x%08X\r\n",SCB->SHCSR);

    printf("R4    = 0x%08X\r\n",current_sp[0]);
    printf("R5    = 0x%08X\r\n",current_sp[1]);
    printf("R6    = 0x%08X\r\n",current_sp[2]);
    printf("R7    = 0x%08X\r\n",current_sp[3]);
    printf("R8    = 0x%08X\r\n",current_sp[4]);
    printf("R9    = 0x%08X\r\n",current_sp[5]);
    printf("R10   = 0x%08X\r\n",current_sp[6]);
    printf("R11   = 0x%08X\r\n",current_sp[7]);

    printf("dump sp stack[sp sp-512]:\r\n");
    uint16_t i = 0;
    do
    {
        printf("0x%08X,",*(exception_sp++));
        i++;
        if(i%4 == 0)
            printf("\r\n");
    }
    while(i<128);
}

void HardFault_Handler(void)
{
    __asm (
        "MRS     R0, MSP\n"
        "TST     LR, #4\n"
        "ITE     EQ\n"
        "MRSEQ   R1, MSP\n"
        "MRSNE   R1, PSP\n"
        "PUSH    {r4-r11}\n"
        "SUB     R0, #32\n"
        "BL      HardFault_Handler_C\n"
        "B       ."
        :
        :
        :
    );
}
