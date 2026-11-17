# Pinout and wiring: USBi emulator on the Blue Pill (STM32F103C8)

All signals are **3.3 V logic**. Never connect 5 V signals to any pin. Always join the grounds of the Blue Pill,
the target board and the USB-serial adapter. Trust the silkscreen printed on *your* board over the lists below.

## 1. Pin assignment

| Function | Blue Pill pin | Direction | Notes |
|---|---|---|---|
| **USB D-** | PA11 | USB | Micro-USB connector, nothing to wire |
| **USB D+** | PA12 | USB | Micro-USB connector. The D+ pull-up resistor (R10) must be 1.5 kohm, see guide section 3 |
| **I2C SCL** | **PB6** | open-drain output | Needs a pull-up to 3.3 V (about 4.7 kohm). The Blue Pill has none |
| **I2C SDA** | **PB7** | open-drain, bidirectional | Needs a pull-up to 3.3 V (about 4.7 kohm) |
| **SPI SCLK** | **PA5** | output | SPI mode 3 (idles high), MSB first, about 500 kHz |
| **SPI MISO** | **PA6** | input (pull-up) | DSP data out (CDATAO on SigmaDSPs) |
| **SPI MOSI** | **PA7** | output | DSP data in (CDATA on SigmaDSPs) |
| **SPI CS1** | **PB0** | output, idles high | Selected by `wIndexH` = 1. **Use this one for a single DSP** |
| SPI CS2 | PB1 | output, idles high | `wIndexH` = 2 |
| SPI CS3 | PB10 | output, idles high | `wIndexH` = 3 |
| SPI CS4 | PB11 | output, idles high | `wIndexH` = 4 |
| SPI CS5 | PB12 | output, idles high | `wIndexH` = 5 |
| Target RESET (request B7) | PB13 | **open-drain**, idles released | Pulses low about 1 ms. The target needs a pull-up to 3.3 V (most DSP boards have one) |
| AUX1 (request B8, probably "power board") | PB14 | push-pull | Idles high. Level = (`wValueL` != 0). Function inferred |
| AUX2 (request B9, probably "LED") | PB15 | push-pull | Idles high. Goes low when `wValueL` = 1. Function inferred |
| I2C-mode LED (optional) | PA1 | push-pull | Low = on (wire LED + 330 ohm from 3.3 V to the pin) |
| SPI-mode LED (optional) | PA2 | push-pull | Low = on (same wiring) |
| Heartbeat LED | PC13 | push-pull | On-board LED, blinks once per second when the firmware runs |
| **Debug log TX** | **PA9** | UART output, 115200 8N1 | To the USB-serial adapter's **RX**. Present only in the `_LOG` firmware |
| Bootloader RX | PA10 | UART input | Used only while flashing (to the adapter's **TX**) |
| BOOT0 / BOOT1 | jumpers | | Flashing only, see guide section 6 |

Pins not listed are unused. Leave PA13/PA14 (SWD) free if you want to use an ST-Link later.

## 2. Header map (micro-USB connector at the top edge)

```
        LEFT header                         RIGHT header     
        ----------------                    -----------------
   PB12 [ ]                                 [ ] GND
   PB13 [ ]  <- target RESET                [ ] GND
   PB14 [ ]  <- AUX1                        [ ] 3V3
   PB15 [ ]  <- AUX2                        [ ] NRST
   PA8  [ ]                                 [ ] PB11  <- SPI CS4
   PA9  [ ]  <- LOG TX / bootloader TX      [ ] PB10  <- SPI CS3
   PA10 [ ]  <- bootloader RX               [ ] PB1   <- SPI CS2
   PA11 [ ]  (USB D-)                       [ ] PB0   <- SPI CS1
   PA12 [ ]  (USB D+)                       [ ] PA7   <- SPI MOSI
   PA15 [ ]                                 [ ] PA6   <- SPI MISO
   PB3  [ ]                                 [ ] PA5   <- SPI SCLK
   PB4  [ ]                                 [ ] PA4
   PB5  [ ]                                 [ ] PA3
   PB6  [ ]  <- I2C SCL                     [ ] PA2   <- SPI-mode LED
   PB7  [ ]  <- I2C SDA                     [ ] PA1   <- I2C-mode LED
   PB8  [ ]                                 [ ] PA0
   PB9  [ ]                                 [ ] PC15
   5V   [ ]                                 [ ] PC14
   GND  [ ]                                 [ ] PC13  (on-board LED)
   3V3  [ ]                                 [ ] VBAT
```

## 3. Wiring: I2C target (for example an ADAU1701 board)

```
  Blue Pill                       Target DSP board
  ---------                       ----------------
  PB6  (SCL) --------------------- SCL
  PB7  (SDA) --------------------- SDA
  GND ----------------------------- GND
  PB13 (RESET, open-drain) -------- /RESET     (optional, needs a pull-up on the target)
                  3.3 V ---[4.7k]--- SDA       (only if the target board has no pull-ups)
                  3.3 V ---[4.7k]--- SCL       (only if the target board has no pull-ups)
```
The target's IOVDD must be **3.3 V**. The Blue Pill does not level-shift. Power the target from its own supply
(or the Blue Pill's 3V3 pin if the load is tiny).

## 4. Wiring: SPI target

```
  Blue Pill                       Target DSP board
  ---------                       ----------------
  PA5  (SCLK) -------------------- CCLK / SCLK
  PA7  (MOSI) -------------------- CDATA / MOSI
  PA6  (MISO) -------------------- CDATAO / MISO
  PB0  (CS1)  -------------------- CLATCH / SS   (use CS1 for the first device)
  GND ----------------------------- GND
```
For an I2C-only setup, leave the SPI wires disconnected.

## 5. Wiring: USB-serial adapter (flashing and log)

Use a **3.3 V** adapter (CP2102, CH340, FT232 and similar, set to 3.3 V). Do **not** connect the adapter's 3.3 V or 5 V
pin when the Blue Pill is powered from its micro-USB connector.

```
  USB-serial adapter              Blue Pill
  ------------------              ---------
  TXD  ---------------------------- PA10   (bootloader RX; only needed while flashing)
  RXD  ---------------------------- PA9    (bootloader TX; later the debug log)
  GND  ---------------------------- GND
  (3V3 / 5V: leave unconnected)
```
