    .syntax unified
    .cpu cortex-m33
    .fpu softvfp
    .thumb

.global  low_power_save_cpu
.global  low_power_restore_cpu
.extern  low_power_enter_sleep

    .section .data
    .align 4
_slow_power_store_buffer:
    .space 0x00000018
_elow_power_store_buffer:

    .section    ram_code,"ax",%progbits
    .size    low_power_save_cpu, .-low_power_save_cpu
low_power_save_cpu:
	push {r0-r12, lr}
    mrs r0, BASEPRI
    mrs r1, PRIMASK
    mrs r2, FAULTMASK
    mrs r3, CONTROL
    mrs r4, APSR
    mrs r5, EPSR
    mrs r6, IPSR
    push {r0-r6}

    ldr r1, =_slow_power_store_buffer
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

ret:
    ldr r1, =_slow_power_store_buffer
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
    
low_power_restore_cpu:
    ldr r1, =_slow_power_store_buffer
    ldr r1, [r1,#12]
    bx r1
