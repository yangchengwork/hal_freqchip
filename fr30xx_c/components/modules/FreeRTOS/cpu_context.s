    EXPORT  low_power_save_cpu
    EXPORT  low_power_restore_cpu
    IMPORT  low_power_enter_sleep

    ;ret
    ;msp
    ;psp
    ;control
    ;msplim
    ;psplim
low_power_store_size    EQU      0x00000018

    AREA |.bss|, NOINIT, READWRITE, ALIGN=3
low_power_store_addr
low_power_store_buffer  SPACE    low_power_store_size

    AREA |ram_code|, CODE, READONLY, ALIGN=2
    PRESERVE8    

low_power_save_cpu
    push {r0-r12, lr}
    mrs r0, BASEPRI
    mrs r1, PRIMASK
    mrs r2, FAULTMASK
    mrs r3, CONTROL
    mrs r4, APSR
    mrs r5, EPSR
    mrs r6, IPSR
    push {r0-r6}

    ldr r1, =low_power_store_addr
    mrs r2, msp
    str r2, [r1]
    mrs r2, psp
    str r2, [r1, #4]
    mrs r2, CONTROL
    str r2, [r1, #8]
    ldr r2, =ret
    orr r2, r2, #1
    str r2, [r1, #12]
    mrs r2, msplim
    str r2, [r1, #16]
    mrs r2, psplim
    str r2, [r1, #20]

    bl low_power_enter_sleep
    b  .

ret
    ldr r1, =low_power_store_addr
    ldr r2, [r1]
    msr msp, r2
    ldr r2, [r1, #4]
    msr psp, r2
    ldr r2, [r1, #8]
    msr CONTROL, r2
    ldr r2, [r1, #16]
    msr msplim, r2
    ldr r2, [r1, #20]
    msr psplim, r2

    pop {r0-r6}
    msr BASEPRI, r0
    msr PRIMASK, r1
    msr FAULTMASK, r2
    msr CONTROL, r3
    msr APSR_nzcvq, r4
    msr EPSR, r5
    msr IPSR, r6

    pop {r0-r12, pc}
    
low_power_restore_cpu
    ldr r1, =low_power_store_addr
    ldr r1, [r1,#12]
    bx r1
    
    END

