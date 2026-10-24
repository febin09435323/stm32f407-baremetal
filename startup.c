/*
 * Minimal startup code for STM32F407VG, written in C.
 *
 * What happens at power-on:
 *   1. The core reads word 0 of flash (0x08000000) into SP.
 *   2. It reads word 1 into PC, i.e. jumps to Reset_Handler.
 *   3. Reset_Handler sets up RAM, enables the FPU, then calls main().
 */

#include <stdint.h>

/* Symbols defined in stm32f407vg.ld */
extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _sbss, _ebss;

int main(void);

void Reset_Handler(void);

void Default_Handler(void)
{
    /* An interrupt fired that has no handler. Halt here so the
       debugger shows exactly where we ended up. */
    while (1) {
    }
}

/* Weak aliases: any handler you define in your own code (same name)
   overrides these automatically. */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));

/* STM32F405/407 has 82 peripheral interrupts (IRQ 0..81), see RM0090 table 61. */
#define CORE_EXCEPTIONS 16
#define IRQ_COUNT       82
#define IRQ(n)          (CORE_EXCEPTIONS + (n))

#define EXTI0_IRQn      6

__attribute__((section(".isr_vector"), used))
void (* const vector_table[CORE_EXCEPTIONS + IRQ_COUNT])(void) = {
    [0]  = (void (*)(void))(&_estack),  /* initial stack pointer */
    [1]  = Reset_Handler,
    [2]  = NMI_Handler,
    [3]  = HardFault_Handler,
    [4]  = MemManage_Handler,
    [5]  = BusFault_Handler,
    [6]  = UsageFault_Handler,
    /* 7..10 reserved */
    [11] = SVC_Handler,
    [12] = DebugMon_Handler,
    /* 13 reserved */
    [14] = PendSV_Handler,
    [15] = SysTick_Handler,

    /* Every peripheral IRQ defaults to Default_Handler ... */
    [IRQ(0) ... IRQ(IRQ_COUNT - 1)] = Default_Handler,
    /* ... except the ones we actually use. */
    [IRQ(EXTI0_IRQn)] = EXTI0_IRQHandler,
};

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

void Reset_Handler(void)
{
    /* Copy initialised data from flash to RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Zero the .bss section */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* Enable the FPU (CP10 and CP11 full access). We build with
       -mfloat-abi=hard, so this must happen before any float code runs,
       otherwise the first FP instruction raises a UsageFault. */
    SCB_CPACR |= (0xFUL << 20);
    __asm volatile ("dsb");
    __asm volatile ("isb");

    main();

    /* main() should never return on a microcontroller */
    while (1) {
    }
}
