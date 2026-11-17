# Makefile - USBi emulator for STM32F103C8 (Blue Pill)
#   make            -> builds build/USBi_BluePill_LOG.hex   (debug log on PA9, use for first bring-up)
#   make release    -> builds build/USBi_BluePill_RUN.hex   (log disabled)
#   make all-hex    -> both, copied into hex/
# Needs: arm-none-eabi-gcc (GNU Arm Embedded toolchain) and make.

PREFIX  ?= arm-none-eabi-
CC       = $(PREFIX)gcc
OBJCOPY  = $(PREFIX)objcopy
SIZE     = $(PREFIX)size

LOG ?= 1
NAME = USBi_BluePill_$(if $(filter 1,$(LOG)),LOG,RUN)
BUILD = build/log$(LOG)

DRV = Drivers/STM32F1xx_HAL_Driver
USB = Middlewares/ST/STM32_USB_Device_Library/Core

C_SOURCES = \
 Core/Src/main.c Core/Src/stm32f1xx_it.c Core/Src/usbd_conf.c \
 USBi/usbi_proto.c USBi/usbi_hw.c USBi/usbd_usbi.c USBi/usbi_desc.c \
 Drivers/CMSIS/Device/ST/STM32F1xx/Source/system_stm32f1xx.c \
 $(DRV)/Src/stm32f1xx_hal.c $(DRV)/Src/stm32f1xx_hal_cortex.c $(DRV)/Src/stm32f1xx_hal_rcc.c \
 $(DRV)/Src/stm32f1xx_hal_rcc_ex.c $(DRV)/Src/stm32f1xx_hal_gpio.c $(DRV)/Src/stm32f1xx_hal_gpio_ex.c \
 $(DRV)/Src/stm32f1xx_hal_pcd.c $(DRV)/Src/stm32f1xx_hal_pcd_ex.c $(DRV)/Src/stm32f1xx_ll_usb.c \
 $(DRV)/Src/stm32f1xx_hal_flash.c $(DRV)/Src/stm32f1xx_hal_flash_ex.c \
 $(USB)/Src/usbd_core.c $(USB)/Src/usbd_ctlreq.c $(USB)/Src/usbd_ioreq.c
ASM_SOURCES = Drivers/CMSIS/Device/ST/STM32F1xx/Source/startup_stm32f103xb.s

CPU    = -mcpu=cortex-m3 -mthumb
DEFS   = -DUSE_HAL_DRIVER -DSTM32F103xB -DUSBI_LOG=$(LOG)
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
	$(MAKE) LOG=1
	$(MAKE) LOG=0
	mkdir -p hex
	cp build/log1/USBi_BluePill_LOG.hex build/log1/USBi_BluePill_LOG.bin hex/
	cp build/log0/USBi_BluePill_RUN.hex build/log0/USBi_BluePill_RUN.bin hex/

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
