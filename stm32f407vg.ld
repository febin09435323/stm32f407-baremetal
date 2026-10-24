/*
 * Linker script for STM32F407VG (STM32F407G-DISC1)
 *
 *   Flash : 1 MB   at 0x08000000
 *   SRAM  : 128 KB at 0x20000000 (SRAM1 112 KB + SRAM2 16 KB, contiguous)
 *   (The 64 KB CCM RAM at 0x10000000 is left unused for now.)
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Initial stack pointer: top of SRAM. The stack grows downwards. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* The vector table must sit at the very start of flash:
       word 0 = initial SP, word 1 = Reset_Handler address. */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH

    /* Initialised globals: live in RAM, but their initial values are stored
       in flash. Reset_Handler copies them from _sidata to _sdata.._edata. */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* Zero-initialised globals: Reset_Handler clears _sbss.._ebss. */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
