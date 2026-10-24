# STM32F407G-DISC1 bare-metal starter

Register-level firmware for the STM32F407G-DISC1 with no HAL, no CubeMX and no CMSIS headers. Every line that touches hardware is visible in this repo.

**What it does:** the four user LEDs light one after another. Each press of the blue USER button switches the chase speed (500 → 250 → 100 → 1000 ms).

| File | Purpose |
|---|---|
| `stm32f407vg.ld` | Linker script: where flash and RAM are, and where each section goes |
| `startup.c` | Vector table and `Reset_Handler` (copies `.data`, zeroes `.bss`, enables FPU, calls `main`) |
| `main.c` | GPIO, SysTick and EXTI setup by direct register writes |
| `Makefile` | Build, flash and disassemble |

No wiring is needed; it uses only the board's own LEDs and button.

## Setup

Connect the board's **Mini-USB port (CN1, the ST-LINK)** to your PC with a data cable.

**Linux / WSL**

```bash
sudo apt install gcc-arm-none-eabi stlink-tools openocd make
make           # build
make flash     # program via st-flash
```

If `st-flash` can't find the board, run `st-info --probe`. On Linux you may need the udev rules shipped with stlink-tools. WSL needs `usbipd` to pass USB through, so flashing from native Linux or Windows is simpler.

**Windows (native)**

1. Install the Arm GNU Toolchain (arm-none-eabi) from arm.com and `make` (via MSYS2 or Chocolatey).
2. Run `make` to get `build/blink.bin`.
3. Flash it with STM32CubeProgrammer: connect over ST-LINK and write the `.bin` at address `0x08000000`, or open the `.elf` directly.

## Debugging

```bash
openocd -f board/stm32f4discovery.cfg            # terminal 1
arm-none-eabi-gdb build/blink.elf                # terminal 2
(gdb) target extended-remote :3333
(gdb) monitor reset halt
(gdb) break EXTI0_IRQHandler
(gdb) continue                                   # now press the button
```

## Exercises

These are ordered roughly by difficulty. Each one is a common firmware interview topic.

1. **Prove the timing with the logic analyzer.** Clip a probe to PD12 and GND and measure the on-time at each speed. Does 500 ms really measure 500 ms? Why does it, given the CPU is on HSI at 16 MHz?
2. **Break `volatile` on purpose.** Remove `volatile` from `g_ms`, build with `-O2`, and see what happens. Then use `make disasm` to find out why.
3. **Read before you write.** Replace `BSRR` with read-modify-write on `ODR`. Explain why that can go wrong once an interrupt also writes to GPIOD.
4. **Trigger a fault.** Comment out the FPU enable in `Reset_Handler` and do one `float` multiply in `main`. Find which handler you end up in, then write a `HardFault_Handler` that captures the stacked PC.
5. **Run at 168 MHz.** Configure HSE (8 MHz crystal) → PLL → 168 MHz. Set the flash wait states first (`FLASH_ACR`), then update `CPU_HZ`. Confirm it with the logic analyzer.
6. **UART output.** Bring up USART2 on PA2/PA3 at 115200 baud, then retarget `printf` by implementing `_write()`. A 3.3 V USB-UART adapter connects to these pins.
7. **I2C to the BME280.** Write an I2C1 driver on PB6 (SCL) / PB9 (SDA), read the chip-ID register (expected `0x60`), and decode the transaction on the logic analyzer.

## References

- RM0090: STM32F405/415, F407/417, F427/437, F429/439 reference manual
- UM1472: STM32F407G-DISC1 user manual (pin mapping, LEDs, jumpers)
- PM0214: Cortex-M4 programming manual (SysTick, NVIC, SCB)
- STM32F407VG datasheet (alternate function table for pin muxing)
