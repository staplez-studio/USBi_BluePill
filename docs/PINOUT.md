# Pinout and wiring: USBi emulator on the DX-SMART DX-PJ26 (STM32F103C8T6)

All signals are **3.3 V logic**. Never connect 5 V signals to any pin. Always join the grounds of the board, the USB-C breakout,
the OLED and the target board. **The LoRa module must NOT be fitted** (it drives PA0 to PA7 and PC15).

## Pins used by the firmware

Only **PA0 to PA7, PB0, PB1, PB10, PB11, PC13, PC14, PC15** are used for the USBi function, plus your two approved exceptions:
**PA11 / PA12** (USB) and **PB13 / PB14** (OLED). Optionally **PA9** carries the debug log through the board's own USB-C serial port.

## 1. Pin assignment

| Function | Pin | Direction | Notes |
|---|---|---|---|
| **I2C SCL** (to the DSP) | **PA0** | open-drain output | Needs a pull-up to 3.3 V (about 4.7 kohm) unless the DSP board has one |
| **I2C SDA** (to the DSP) | **PA1** | open-drain, bidirectional | Same |
| **Debug log TX** (`_LOG_PA2` firmware) | **PA2** | UART TX, 115200 8N1 | To an external USB-serial adapter's **RX** |
| **Target RESET** (request B7) | **PA3** | open-drain, idles released | Target needs a pull-up to 3.3 V (most DSP boards have one) |
| **SPI CS1** | **PA4** | output, idles high | `wIndexH` = 1. **Use this for a single DSP** |
| **SPI SCLK** | **PA5** | output | SPI mode 3 (idles high), MSB first, about 500 kHz |
| **SPI MISO** | **PA6** | input (pull-up) | DSP data out (CDATAO) |
| **SPI MOSI** | **PA7** | output | DSP data in (CDATA) |
| SPI CS2 | PB0 | output, idles high | `wIndexH` = 2 |
| SPI CS3 | PB1 | output, idles high | `wIndexH` = 3 |
| SPI CS4 | PB10 | output, idles high | `wIndexH` = 4 |
| SPI CS5 | PC13 | output, idles high | `wIndexH` = 5 (slow pin: 2 MHz, about 3 mA limit) |
| **Heartbeat LED** | **PB11** | output | The user LED reported on the DX-PJ26 V1.2. Blinks once per second |
| AUX1 (request B8, probably "power board") | PC14 | push-pull, slow | Idles high. Level = (`wValueL` != 0). Function inferred |
| AUX2 (request B9, probably "LED") | PC15 | push-pull, slow | Idles high. Low when `wValueL` = 1. Function inferred |
| **USB D-** | **PA11** | USB | To your USB-C breakout's D- (exception) |
| **USB D+** | **PA12** | USB | To your USB-C breakout's D+ (exception), **plus a 1.5 kohm resistor from PA12 to 3.3 V** |
| **OLED SCL** | **PB13** | open-drain output | 1.3 inch I2C OLED, address 0x3C (exception) |
| **OLED SDA** | **PB14** | open-drain, bidirectional | Same |
| Debug log TX (`_LOG_CH340` firmware) | PA9 | UART TX | Already wired on the board to the CH340 chip: the log appears on the board's own COM port |
| CH340 RX (used when flashing) | PA10 | UART RX | Already wired on the board |

Limits of **PC13, PC14, PC15** (chip restriction): about 3 mA at up to 2 MHz, and they may share pins with an on-board 32.768 kHz crystal.
They carry only slow, static signals (CS5 and the AUX outputs). The original USBi's two mode LEDs are not implemented.

## 2. Wiring: USB-C breakout (the emulator's USB connection)

The DX-PJ26's own USB-C port goes to a **CH340 USB-serial chip** and is used only for power, flashing and (optionally) the log.
The emulator is a **second USB connection** straight from PA11 / PA12 to your USB-C breakout board.

```
  USB-C breakout                         DX-PJ26
  --------------                         -------
  D+  ---------------------------------- PA12
  D-  ---------------------------------- PA11
  GND ---------------------------------- GND
  VBUS: leave unconnected                (the board is powered through its own USB-C cable)
  CC1, CC2: 5.1 kohm to GND on the breakout (already fitted on most breakouts)

                      3.3 V ---[ 1.5 kohm ]--- PA12      <- REQUIRED: tells the PC "full-speed device present"
```
- The 1.5 kohm resistor is **required**. The DX-PJ26 schematic has no pull-up on PA12, and USB-C breakouts do not fit one on D+.
- The breakout usually has **two D+ and two D- pads** (reversible connector); they are tied together on the breakout, so use either one.
- Use a **USB-A to USB-C** cable, or a USB-C to USB-C cable, from the PC to the breakout.
- The pull-up is always on. If the breakout cable is plugged in while the firmware is not running (for example while flashing), Windows shows an unknown device. Unplug the breakout cable while flashing.
- On the DX-PJ26, PA11 / PA12 come out on header **P3** (the schematic you read shows P3 pins 9 and 10; check the silkscreen on your board).
  Before connecting, confirm with a multimeter (board unplugged) that PA11 / PA12 have **no continuity** to the board's own USB-C connector.

## 3. Wiring: OLED status display (optional)

```
  1.3" I2C OLED                          DX-PJ26
  -------------                          -------
  VCC ---------------------------------- 3V3
  GND ---------------------------------- GND
  SCL ---------------------------------- PB13
  SDA ---------------------------------- PB14
```
The module has its own pull-ups. Most 1.3 inch modules use the **SH1106** controller (the default). Address 0x3C (the default).
See the guide for the OLED screen layout and for the settings if your display is an SSD1306 or shows upside down.

## 4. Wiring: I2C target (for example an ADAU1701 board)

```
  DX-PJ26                         Target DSP board
  -------                         ----------------
  PA0  (SCL) --------------------- SCL
  PA1  (SDA) --------------------- SDA
  GND ----------------------------- GND
  PA3  (RESET, open-drain) -------- /RESET     (optional; needs a pull-up on the target)
                  3.3 V ---[4.7k]--- SDA       (only if the target board has no pull-ups)
                  3.3 V ---[4.7k]--- SCL       (only if the target board has no pull-ups)
```
The target's IOVDD must be **3.3 V**. Power the target from its own supply.

## 5. Wiring: SPI target

```
  DX-PJ26                         Target DSP board
  -------                         ----------------
  PA5  (SCLK) -------------------- CCLK / SCLK
  PA7  (MOSI) -------------------- CDATA / MOSI
  PA6  (MISO) -------------------- CDATAO / MISO
  PA4  (CS1)  -------------------- CLATCH / SS   (use CS1 for the first device)
  GND ----------------------------- GND
```
For an I2C-only setup, leave the SPI wires disconnected.

## 6. Cables to the PC

```
  PC USB port 1 ---- DX-PJ26 USB-C -----> power (5 V -> 3.3 V on board) + COM port (CH340): flashing, optional log
  PC USB port 2 ---- USB-C breakout ----> D+ / D- / GND only -> PA12 / PA11 / GND  (this is the USBi seen by SigmaStudio)
```
