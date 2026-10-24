/*
 * Bare-metal LED chase for STM32F407G-DISC1. No HAL, no CMSIS headers.
 *
 *   - The four user LEDs (PD12..PD15) light one after another.
 *   - SysTick fires every 1 ms and keeps a millisecond counter.
 *   - The blue USER button (PA0) reverses the direction.
 *
 * Board facts (UM1472, STM32F407G-DISC1 user manual):
 *   LD4 green  = PD12    LD3 orange = PD13
 *   LD5 red    = PD14    LD6 blue   = PD15
 *   B1 USER    = PA0, active high (board has an external pull-down)
 *
 * Register addresses come from RM0090 (STM32F405/407 reference manual)
 * and PM0214 (Cortex-M4 programming manual).
 */

#include <stdint.h>

#define REG(addr) (*(volatile uint32_t *)(addr))

/* ---- RCC: clock enables (RM0090 section 7) ------------------------------ */
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     REG(RCC_BASE + 0x30)
#define RCC_APB2ENR     REG(RCC_BASE + 0x44)
#define RCC_AHB1ENR_GPIOAEN  (1UL << 0)
#define RCC_AHB1ENR_GPIODEN  (1UL << 3)
#define RCC_APB2ENR_SYSCFGEN (1UL << 14)

/* ---- GPIO (RM0090 section 8) -------------------------------------------- */
#define GPIOA_BASE      0x40020000UL
#define GPIOD_BASE      0x40020C00UL
#define GPIO_MODER(p)   REG((p) + 0x00)
#define GPIO_PUPDR(p)   REG((p) + 0x0C)
#define GPIO_IDR(p)     REG((p) + 0x10)
#define GPIO_BSRR(p)    REG((p) + 0x18)

/* ---- SYSCFG + EXTI (RM0090 sections 9 and 12) --------------------------- */
#define SYSCFG_BASE     0x40013800UL
#define SYSCFG_EXTICR1  REG(SYSCFG_BASE + 0x08)
#define EXTI_BASE       0x40013C00UL
#define EXTI_IMR        REG(EXTI_BASE + 0x00)
#define EXTI_RTSR       REG(EXTI_BASE + 0x08)
#define EXTI_PR         REG(EXTI_BASE + 0x14)

/* ---- Cortex-M4 core peripherals (PM0214) -------------------------------- */
#define SYST_CSR        REG(0xE000E010UL)   /* SysTick control and status */
#define SYST_RVR        REG(0xE000E014UL)   /* SysTick reload value       */
#define SYST_CVR        REG(0xE000E018UL)   /* SysTick current value      */
#define NVIC_ISER0      REG(0xE000E100UL)   /* IRQ 0..31 set-enable       */

#define EXTI0_IRQn      6

/* After reset the chip runs from the 16 MHz internal HSI oscillator.
   (Switching to 168 MHz with the PLL is a later exercise.) */
#define CPU_HZ          16000000UL

#define LED_FIRST_PIN   12
#define LED_COUNT       4

/* Shared between interrupts and main(), so they must be volatile:
   otherwise the compiler may cache them in a register and main()
   never sees the change. */
static volatile uint32_t g_ms;
static volatile uint32_t g_reverse;


/* ------------------------------------------------------------------------- */

void SysTick_Handler(void)
{
    g_ms++;
}

void EXTI0_IRQHandler(void)
{
    static uint32_t last_press_ms;

    /* Clear the pending bit by writing 1 to it (rc_w1).
       Use '=', not '|=': a read-modify-write here would also
       clear any other line that happens to be pending. */
    EXTI_PR = (1UL << 0);

    /* Crude debounce: ignore edges within 200 ms of the last press. */
    if ((g_ms - last_press_ms) > 200) {
        last_press_ms = g_ms;
		g_reverse = !g_reverse;
    }
}

/* ------------------------------------------------------------------------- */

static void leds_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC_AHB1ENR;  /* read back: gives the clock time to reach the
                           peripheral before we touch it (RM0090 errata) */

    /* PD12..PD15 -> general-purpose output (MODER = 01 for each pin) */
    GPIO_MODER(GPIOD_BASE) &= ~(0xFFUL << (LED_FIRST_PIN * 2));
    GPIO_MODER(GPIOD_BASE) |=  (0x55UL << (LED_FIRST_PIN * 2));
}

static void led_show(uint32_t idx)
{
    /* BSRR is atomic: the upper 16 bits reset pins, the lower 16 set them.
       No read-modify-write, so this is safe even if an ISR touches GPIOD. */
    GPIO_BSRR(GPIOD_BASE) = (0xFUL << (LED_FIRST_PIN + 16))  /* all off */
                          | (1UL   << (LED_FIRST_PIN + idx)); /* one on  */
}

static void button_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC_APB2ENR;

    /* PA0 as input (MODER = 00). Leave PA13/PA14 alone: they're the
       SWD debug pins, and reconfiguring them locks out the ST-LINK. */
    GPIO_MODER(GPIOA_BASE) &= ~(0x3UL << 0);
    GPIO_PUPDR(GPIOA_BASE) &= ~(0x3UL << 0);   /* board has a pull-down */

    /* Route PA0 to EXTI line 0 (EXTI0[3:0] = 0000 selects port A) */
    SYSCFG_EXTICR1 &= ~(0xFUL << 0);

    EXTI_RTSR |= (1UL << 0);   /* trigger on rising edge (button press) */
    EXTI_IMR  |= (1UL << 0);   /* unmask line 0 */

    NVIC_ISER0 = (1UL << EXTI0_IRQn);
}

static void systick_init(void)
{
    SYST_RVR = (CPU_HZ / 1000) - 1;   /* 16000 ticks = 1 ms */
    SYST_CVR = 0;                     /* clear current count */
    SYST_CSR = (1UL << 2)             /* CLKSOURCE: processor clock */
             | (1UL << 1)             /* TICKINT: raise the interrupt */
             | (1UL << 0);            /* ENABLE */
}

int main(void)
{
    leds_init();
    button_init();
    systick_init();

    uint32_t led = 0;
    uint32_t last_step_ms = 0;

    led_show(led);

    while (1) {
        /* Unsigned subtraction handles the counter wrapping at 2^32
           (after ~49 days) correctly. */
        if ((g_ms - last_step_ms) >= 250) {
            last_step_ms = g_ms;
			if (g_reverse) {
				led = (led + LED_COUNT - 1) % LED_COUNT;   /* backward: 3,2,1,0,3... */
			}
			else
			{
				led = (led + 1) % LED_COUNT;   /* forward:  0,1,2,3,0... */
			}
			led_show(led);
        }
    }
}
