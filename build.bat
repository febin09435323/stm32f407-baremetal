@echo off
rem Build (and optionally flash) without make, for Windows cmd.
rem   build.bat         -> build only
rem   build.bat flash   -> build, then flash with STM32CubeProgrammer CLI
rem Does the same as the Makefile, one step at a time.

setlocal
set CPU=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
set CFLAGS=%CPU% -std=gnu11 -Og -g3 -Wall -Wextra -Wno-override-init -ffunction-sections -fdata-sections
set LDFLAGS=%CPU% -T stm32f407vg.ld -nostartfiles --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,-Map=build\blink.map
set PROGRAMMER=%ProgramFiles%\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe

if not exist build mkdir build

echo [CC] startup.c
arm-none-eabi-gcc %CFLAGS% -c startup.c -o build\startup.o || goto :fail
echo [CC] main.c
arm-none-eabi-gcc %CFLAGS% -c main.c -o build\main.o || goto :fail
echo [LD] blink.elf
arm-none-eabi-gcc %LDFLAGS% build\startup.o build\main.o -o build\blink.elf || goto :fail
echo [BIN] blink.bin
arm-none-eabi-objcopy -O binary build\blink.elf build\blink.bin || goto :fail
arm-none-eabi-size build\blink.elf

if /i not "%~1"=="flash" goto :done
echo [FLASH] build\blink.bin to 0x08000000
"%PROGRAMMER%" -c port=SWD -w build\blink.bin 0x08000000 -rst || goto :fail

:done
echo.
echo Build OK
exit /b 0

:fail
echo.
echo BUILD FAILED
exit /b 1
