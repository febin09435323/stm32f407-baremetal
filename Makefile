TARGET  := blink
BUILD   := build

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size

SRCS    := startup.c main.c
OBJS    := $(SRCS:%.c=$(BUILD)/%.o)

CPU     := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS  := $(CPU) -std=gnu11 -Og -g3 \
           -Wall -Wextra -Wno-override-init \
           -ffunction-sections -fdata-sections

LDFLAGS := $(CPU) -T stm32f407vg.ld -nostartfiles \
           --specs=nano.specs --specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

.PHONY: all clean flash flash-openocd disasm

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin
	@$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) stm32f407vg.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD):
	mkdir -p $(BUILD)

# Flash with stlink-tools (st-flash)
flash: $(BUILD)/$(TARGET).bin
	st-flash --reset write $< 0x08000000

# Flash with OpenOCD instead
flash-openocd: $(BUILD)/$(TARGET).elf
	openocd -f board/stm32f4discovery.cfg -c "program $< verify reset exit"

# Look at what the compiler actually produced
disasm: $(BUILD)/$(TARGET).elf
	$(OBJDUMP) -d $< | less

clean:
	rm -rf $(BUILD)
