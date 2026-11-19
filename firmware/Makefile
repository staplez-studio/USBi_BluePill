# Makefile - USBi emulator for STM32F103C8 (DX-SMART DX-PJ26 or standard Blue Pill)
#   make all-hex                -> builds the three DX-PJ26 hex files into hex/
#   make BOARD=BLUEPILL all-hex -> the same three for a standard Blue Pill
#   make                        -> one build: LOG=1 LOGUART=2 (debug log on PA2)
#   make LOG=1 LOGUART=1        -> debug log on PA9 (DX-PJ26: the on-board USB-C serial port)
#   make LOG=0                  -> release build, no log
# Needs: arm-none-eabi-gcc (GNU Arm Embedded toolchain) and make.

BOARD   ?= PJ26
LOG     ?= 1
LOGUART ?= 2
PREFIX  ?= arm-none-eabi-
CC       = $(PREFIX)gcc
OBJCOPY  = $(PREFIX)objcopy
SIZE     = $(PREFIX)size

ifeq ($(LOG),0)
 TAG = RUN
else ifeq ($(LOGUART),1)
 TAG = LOG_CH340
else
 TAG = LOG_PA2
endif
NAME  = USBi_$(BOARD)_$(TAG)
BUILD = build/$(BOARD)_$(TAG)

DRV = Drivers/STM32F1xx_HAL_Driver
USB = Middlewares/ST/STM32_USB_Device_Library/Core

C_SOURCES = \
 Core/Src/main.c Core/Src/stm32f1xx_it.c Core/Src/usbd_conf.c \
 USBi/usbi_proto.c USBi/usbi_hw.c USBi/usbd_usbi.c USBi/usbi_desc.c \
 USBi/usbi_stats.c USBi/oled_hw.c USBi/oled.c USBi/ui_format.c USBi/ui.c \
 Drivers/CMSIS/Device/ST/STM32F1xx/Source/system_stm32f1xx.c \
 $(DRV)/Src/stm32f1xx_hal.c $(DRV)/Src/stm32f1xx_hal_cortex.c $(DRV)/Src/stm32f1xx_hal_rcc.c \
 $(DRV)/Src/stm32f1xx_hal_rcc_ex.c $(DRV)/Src/stm32f1xx_hal_gpio.c $(DRV)/Src/stm32f1xx_hal_gpio_ex.c \
 $(DRV)/Src/stm32f1xx_hal_pcd.c $(DRV)/Src/stm32f1xx_hal_pcd_ex.c $(DRV)/Src/stm32f1xx_ll_usb.c \
 $(DRV)/Src/stm32f1xx_hal_flash.c $(DRV)/Src/stm32f1xx_hal_flash_ex.c \
 $(USB)/Src/usbd_core.c $(USB)/Src/usbd_ctlreq.c $(USB)/Src/usbd_ioreq.c
ASM_SOURCES = Drivers/CMSIS/Device/ST/STM32F1xx/Source/startup_stm32f103xb.s

CPU    = -mcpu=cortex-m3 -mthumb
DEFS   = -DUSE_HAL_DRIVER -DSTM32F103xB -DUSBI_LOG=$(LOG) -DUSBI_LOG_UART=$(LOGUART) \
         -DUSBI_BOARD_PJ26=$(if $(filter PJ26,$(BOARD)),1,0) $(EXTRA_DEFS)
INCS   = -ICore/Inc -IUSBi -I$(DRV)/Inc -IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
         -IDrivers/CMSIS/Include -I$(USB)/Inc
CFLAGS = $(CPU) $(DEFS) $(INCS) -Os -g -std=gnu11 -Wall -ffunction-sections -fdata-sections
LDFLAGS= $(CPU) -TSTM32F103C8_FLASH.ld --specs=nano.specs --specs=nosys.specs \
         -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(NAME).map -lc -lm

OBJS = $(addprefix $(BUILD)/,$(C_SOURCES:.c=.o)) $(addprefix $(BUILD)/,$(ASM_SOURCES:.s=.o))

all: $(BUILD)/$(NAME).hex $(BUILD)/$(NAME).bin
release:
	$(MAKE) LOG=0
all-hex:
	$(MAKE) LOG=1 LOGUART=1
	$(MAKE) LOG=1 LOGUART=2
	$(MAKE) LOG=0
	mkdir -p hex
	for t in LOG_CH340 LOG_PA2 RUN; do cp build/$(BOARD)_$$t/USBi_$(BOARD)_$$t.hex build/$(BOARD)_$$t/USBi_$(BOARD)_$$t.bin hex/; done

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CPU) -x assembler-with-cpp -c $< -o $@
$(BUILD)/$(NAME).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@
$(BUILD)/$(NAME).hex: $(BUILD)/$(NAME).elf
	$(OBJCOPY) -O ihex $< $@
$(BUILD)/$(NAME).bin: $(BUILD)/$(NAME).elf
	$(OBJCOPY) -O binary $< $@
clean:
	rm -rf build
.PHONY: all release all-hex clean
