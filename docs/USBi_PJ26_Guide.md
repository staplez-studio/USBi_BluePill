# USBi emulator for the DX-SMART DX-PJ26: complete guide from zip to SigmaStudio

> **Status, read this first.** The firmware compiles, the Intel hex files are validated, and the request-to-I2C/SPI translation and the
> OLED screen logic are tested on a PC against simulated devices. The protocol was reverse-engineered from the USBi firmware image
> you supplied. It has **not yet been tested against real SigmaStudio on real hardware.** That first run is the job of this guide,
> and the debug log exists so that, if anything is missing, we can see exactly what SigmaStudio asks for.

---

## 0. Open the zip

1. Save `USBi_Emulator_PJ26_v1.1.zip` somewhere simple, for example your Desktop.
2. **Right-click the zip, choose Extract All...**, pick a folder such as `C:\USBi_Emulator_PJ26`, and press Extract. (Do not run files from inside the zip preview; extract first.)
3. Open the extracted folder. This guide is `docs\USBi_PJ26_Guide.html` (double-click opens it in your browser). `docs\PINOUT.html` is the pin and wiring sheet: print it.

---

## 1. What is in the zip

```
USBi_Emulator_PJ26/
|-- README.md
|-- docs/
|   |-- USBi_PJ26_Guide.html / .md   this guide
|   |-- PINOUT.html / .md                pins and wiring diagrams (print this)
|   `-- PROTOCOL.md                      the USBi protocol as decoded from the firmware
|-- firmware/
|   |-- hex/
|   |   |-- USBi_PJ26_LOG_CH340.hex  <-- flash THIS first: debug log appears on the board's own USB-C COM port
|   |   |-- USBi_PJ26_LOG_PA2.hex        same, but the log is on pin PA2 (needs an external USB-serial adapter)
|   |   |-- USBi_PJ26_RUN.hex            final version, no log
|   |   `-- (a .bin of each)
|   |-- Makefile, STM32F103C8_FLASH.ld   rebuild from source (section 11)
|   |-- Core/, USBi/                     main program, USB class, I2C/SPI, OLED screen
|   |-- Drivers/, Middlewares/           ST HAL, CMSIS and USB device library (unmodified)
|   `-- licenses/
`-- host_tests/                          PC tests of the translation layer and the OLED screen (optional)
```

---

## 2. What you need

**Hardware**
- **DX-SMART DX-PJ26** board (STM32F103C8T6 + CH340 USB-serial) with the **LoRa module removed**.
- **USB-C cable** for the board's own port (power, flashing, optional log).
- **A USB-C breakout board** (D+, D-, GND pads) and a cable from it to a second PC USB port.
- **One 1.5 kohm resistor** (USB D+ pull-up, PA12 to 3.3 V).
- **1.3 inch I2C OLED** (4 pins: VCC, GND, SCL, SDA), optional but recommended.
- Jumper wires; two 4.7 kohm resistors for I2C pull-ups only if the DSP board has none.
- Your DSP target board (I2C or SPI, **3.3 V I/O**).
- Optional: a 3.3 V USB-serial adapter (only for the `_LOG_PA2` firmware); an ST-Link V2 (fallback flashing).

**Software (Windows PC)**
- **SigmaStudio**, installed (it provides the Analog Devices USB driver files).
- **MCUISP** (free Windows UART flasher for STM32).
- The **CH340 driver** if Windows does not already show a COM port for the board.
- A serial terminal (Tera Term, PuTTY, HTerm or similar) for the log.

---

## 3. Prepare the board

1. Make sure the **LoRa module is not fitted**. The module's pins are PA0 to PA7 and PC15, the same pins the emulator drives; leaving it fitted would make them fight.
2. Look at the board and find: the **USB-C connector**, the **CH340 chip** (small 10- or 16-leg chip near it), the **STM32** (largest chip), the **RST** button, the **user LED**, the **SWD header** (4 pins V G D C) and the **P3** header.
3. **Verify the USB lines are separate** (this one check protects the whole project). With the board unplugged, set the multimeter to continuity: the board's **USB-C D+ / D- pads must have no connection to PA12 / PA11**. If they do beep, stop and tell me.
4. Do not connect the breakout or the OLED yet.

---

## 4. Pinout and wiring

See **`PINOUT.md`** for the complete tables and diagrams. In short:

| What | Pins |
|---|---|
| USB to the PC (breakout) | **PA12 = D+ (plus 1.5 kohm to 3V3), PA11 = D-, GND** |
| OLED | **PB13 = SCL, PB14 = SDA**, 3V3, GND |
| I2C to the DSP | **PA0 = SCL, PA1 = SDA** |
| SPI to the DSP | **PA5 = SCLK, PA6 = MISO, PA7 = MOSI, PA4 = CS1** (PB0, PB1, PB10, PC13 = CS2 to CS5) |
| Target reset (B7) | PA3 (open-drain) |
| AUX lines (B8, B9) | PC14, PC15 (slow outputs) |
| Heartbeat LED | PB11 (the board's user LED) |
| Debug log | PA9 via the board's own COM port (`_LOG_CH340`), or PA2 (`_LOG_PA2`) |

All logic is **3.3 V**. Join all grounds. Do **not** connect the breakout's VBUS to anything.

**Wire it up now, with everything unplugged**: (1) 1.5 kohm resistor from PA12 to 3V3; (2) breakout D+ to PA12, D- to PA11, GND to GND; (3) OLED to PB13 / PB14 / 3V3 / GND. Leave the DSP unconnected until the first test works.

---

## 5. Windows driver

The firmware identifies itself as `USB\VID_0456&PID_7031` ("USBi (programmed)"). Analog Devices' driver package already has an entry for that exact ID, and binds it to Cypress's generic `CyUSB` driver. The firmware-download step in that package applies only to the other ID (`PID_7030`), so it is not used here.

1. **Is the driver already installed?** If SigmaStudio was installed with its USB driver, the file `C:\Windows\System32\drivers\CyUSB.sys` typically exists. If so, skip to section 6.
2. **Otherwise install it.** Open the SigmaStudio USB driver folder (the one holding `CyUSB.inf`, `CyUSB.sys`, `CyUSB.cat`, `dpinst32.exe`, `dpinst64.exe`). On 64-bit Windows right-click **`dpinst64.exe`**, **Run as administrator**, and follow the wizard (`dpinst32.exe` on 32-bit Windows).
3. If that fails: Device Manager, right-click the device, *Update driver, Browse my computer, Let me pick, Have Disk...*, browse to `CyUSB.inf`, choose **Analog Devices USBi (programmed)**.
4. Windows may refuse an unsigned or old driver depending on its policy. The package carries an Analog Devices catalog (`CyUSB.cat`), which normally satisfies it.

The **CH340 driver** (for the board's COM port) is separate: Windows 10/11 normally installs it automatically. If the board shows no COM port in Device Manager, install the CH340 driver from WCH.

---

## 6. Flash the firmware with MCUISP over the board's USB-C port

The board's USB-C goes to a CH340 chip wired to the STM32's serial bootloader pins, so MCUISP flashes it directly. **No jumper wires, no adapter.**

1. **Unplug the USB-C breakout cable** (the emulator side). Plug **only the board's own USB-C cable** into the PC.
2. In **Device Manager, Ports (COM and LPT)** find "USB-SERIAL CH340 (COMx)" and note the COM number.
3. Start **MCUISP**. (Some versions are mostly Chinese; the functions below are the same, labels may differ slightly.)
4. **Open the file:** `firmware\hex\USBi_PJ26_LOG_CH340.hex`.
5. **Port and speed:** select your COMx; baud rate **115200** (the first time; faster speeds are fine later).
6. **Auto-entry setting.** DX-SMART's own troubleshooting guide for the PJ26 gives: **`Reset@DTR High(>+3V), ISP@RTS Low`**. Select that option in MCUISP's DTR/RTS drop-down.
   A community guide for the earlier V1.1 board uses **`Reset@DTR Low(<-3V), ISP@RTS Low`**; try it if the first fails.
7. **Tick:** Verify; Run after programming. Leave option bytes and read-protection options alone.
8. Click **Read ChipInfo** (or the start/connect button) and **immediately press the board's RST button.** Several users report this timing is what makes it connect on the DX-PJ26.
   The chip ID is normally `0x410` for an STM32F103.
9. Click **Start programming**. Watch the log: erase, programming to 100 %, **verify OK**, run.
10. Press **RST** (or replug the board's USB-C). The user LED (PB11) should blink once per second, and the OLED (if fitted) shows the status screen.

### If connecting fails
| Symptom | Fix |
|---|---|
| "No response from the bootloader" | Press RST *right after* clicking connect (step 8). Try the other DTR/RTS option (step 6). Try a lower baud rate (57600) |
| Wrong COM port / busy | Close any terminal using the COM port. Check the COM number again |
| Works only sometimes | Use a short USB cable and a direct PC port (not a hub) |
| Never connects | **Fallback with an ST-Link V2:** wire ST-Link GND, SWDIO, SWCLK to the board's SWD header pins G, D, C (leave V disconnected if the board is USB-powered) and flash the `.hex` with STM32CubeProgrammer |

> **Caution about DTR / RTS.** On this board a control line of the CH340 also reaches the STM32's BOOT0 / reset. Some terminal programs assert DTR/RTS when they open the COM port, which can reset the board or send it into the bootloader the next time it resets. If the board stops running when you open a terminal, use a terminal that lets you turn DTR/RTS off, or use the `_LOG_PA2` firmware with an external adapter instead.

---

## 7. First power-up and USB check

1. Plug the **USB-C breakout cable** into a **second** PC USB port. (The board's own USB-C stays plugged in for power.)
2. **Device Manager** should show **"Analog Devices USBi (programmed)"** (under *Universal Serial Bus controllers* or *devices*). Properties, Details, *Hardware Ids* must show `USB\VID_0456&PID_7031`.
3. **If you see "Unknown USB Device (Device Descriptor Request Failed)":** check the 1.5 kohm pull-up (PA12 to 3V3), D+/D- not swapped, short wires, and GND joined; replug.
4. **If it shows the right ID but with a warning and no driver:** install the driver (section 5).
5. **The OLED tells you how far it got** (top right of the screen):

| Code | Meaning |
|---|---|
| `---` | USB not started or no host |
| `DEF` | The PC reset the bus but did not finish enumeration (check D+/D-, pull-up, cable) |
| `ADR` | Enumerated, address assigned, but no driver has configured it (install the driver) |
| `CFG` | **Driver loaded and the device is configured. This is the goal.** |
| `SUS` | Bus suspended (PC asleep, or cable unplugged) |

---

## 8. OLED screen

Eight lines of 21 characters, refreshed about ten times a second:

```
USBi 0456:7031    CFG      <- title and USB state (section 7)
Bus I2C   Addr 0x34        <- bus of the last transfer and its address (SPI shows the chip-select)
Last WR 00004B  OK         <- last write: bytes and result (OK / NACK / BERR)
Last RD 00002B  OK         <- last read: bytes and result
WR 00012 RD 00005          <- total write / read transfers
NACK 003 BERR 000          <- slave-did-not-answer and bus-error counts
B2 v0034 i0000 l0004       <- last request: code, wValue, wIndex, wLength
Unk C3 x02 Up 0:12:34      <- last request NOT understood (and how many), then uptime
```
`Unk` is the quick way to see an unsupported request without a terminal.

**If your OLED misbehaves** (rebuild with the settings in section 12):
- Nothing at all: check wiring (PB13 = SCL, PB14 = SDA), 3.3 V, address 0x3C (some modules are 0x3D). The firmware simply skips the display if it gets no answer.
- Picture shifted by two pixels or garbage: the module is an SSD1306, not an SH1106 (`OLED_SH1106=0`).
- Upside down: set `OLED_FLIP=0`.

---

## 9. First test with SigmaStudio and capturing the log

1. **Get the log.** Pick one:
   - **`_LOG_CH340` firmware (the one you flashed):** the log comes out on the board's own COM port. Open a terminal on that COMx at **115200, 8N1, no flow control**, with DTR/RTS off if the program allows it.
   - **`_LOG_PA2` firmware:** connect a 3.3 V USB-serial adapter's **RX to PA2** and GND to GND, and open the adapter's COM port at 115200 8N1.
2. **Wire the DSP** (PINOUT.md sections 4 / 5) with its power off, then power it on.
3. Start **SigmaStudio**, set up your project and hardware configuration as for a normal USBi, and confirm the USBi is detected. Run **Link-Compile-Download**.
4. **Watch the terminal and the OLED.** Each request is one line, for example:

```
O rt=40 rq=B2 v=0034 i=0000 l=0004 : 00 10 AA BB     <- write sub-address 0x0010 + data to I2C 0x34
I rt=C0 rq=B5 v=0000 i=0000 l=0001                    <- host asks for the write status
O rt=40 rq=B3 v=0034 i=0000 l=0002 : 00 10            <- set sub-address for a read
I rt=C0 rq=B4 v=0034 i=0000 l=0002                    <- read 2 bytes
I rt=C0 rq=B6 v=0000 i=0000 l=0001                    <- read status
O rt=40 rq=C3 v=0000 i=0000 l=0000 !                  <- "!" = a request the emulator does NOT know
```
   `O` = host to device, `I` = device to host; `rt` request type, `rq` request code, `v`/`i`/`l` = wValue/wIndex/wLength; a trailing **`!`** marks an unimplemented request.
5. **Save the log** (the terminal's "log to file" option) and **send it to me, especially any `!` lines.**

Success: SigmaStudio finishes the download without an error, the DSP's parameters change, and no `!` lines (or `Unk` counts) appear.

---

## 10. When it works: switch to the RUN firmware

The LOG firmware prints inside the USB interrupt, which adds a few milliseconds per request. Once everything works, reflash with **`USBi_PJ26_RUN.hex`** (section 6, with the breakout cable unplugged). The OLED screen stays.

---

## 11. Rebuilding from source

You do not need this to use the board; the hex files are compiled. Use it to change pins, speeds or the display.

1. Install the **Arm GNU Toolchain** (`arm-none-eabi-gcc`) and `make`. On Windows the easiest source of both is **STM32CubeIDE**, which bundles them inside its installation folder (look under the IDE's `plugins` folder for `gnu-tools-for-stm32` and the bundled `make`); add their `bin` folders to PATH. Alternatively use MSYS2 (`pacman -S make mingw-w64-ucrt-x86_64-arm-none-eabi-gcc`).
2. Open a terminal in `firmware/` and run:
   ```
   make clean
   make all-hex
   ```
   This builds the three hex files into `firmware/hex/`. Other forms: `make LOG=0` (RUN only), `make LOG=1 LOGUART=1` (log on PA9), `make LOG=1 LOGUART=2` (log on PA2).
3. **STM32CubeIDE:** *File, New, Makefile Project with Existing Code*, pick the `firmware` folder, then *Project, Build All*. Flash the resulting `.hex` with MCUISP as in section 6.

### Verify your files
SHA-256 of the supplied hex files (Windows: `certutil -hashfile <file> SHA256`):

| File | Size (bytes) | SHA-256 |
|---|---|---|
| `USBi_PJ26_LOG_CH340.hex` | 49971 | `234e8304523ebe878995d9e5b06247a3d6406cbb24b3956a9a24ffefae8daabc` |
| `USBi_PJ26_LOG_PA2.hex` | 49971 | `97937f083ba79c8e66754ebee14555695d11282d3667d5b7d48878b7379673bb` |
| `USBi_PJ26_RUN.hex` | 48404 | `36b6e629bd8e03f9fb9b446159f1b7663cd74f4b375327c0c7137e8817e5bcf1` |

---

## 12. Configuration options

Pass them to make, for example `make all-hex EXTRA_DEFS="-DUSBI_I2C_HZ=400000 -DOLED_SH1106=0"`.

| Setting | Default | Meaning |
|---|---|---|
| `USBI_I2C_HZ` | 100000 | I2C clock to the DSP (try 400000 if the target allows) |
| `USBI_SPI_HZ` | 500000 | SPI clock |
| `USBI_BUF_SIZE` | 4096 | Largest single transfer accepted |
| `OLED_SH1106` | 1 | 1 = SH1106 (typical 1.3 inch), 0 = SSD1306 |
| `OLED_FLIP` | 1 | 0 = rotate the picture 180 degrees |
| `OLED_ADDR8` | 0x78 | OLED I2C address, 8-bit form (0x78 = 7-bit 0x3C, 0x7A = 0x3D) |
| `OLED_I2C_HZ` | 400000 | OLED bus speed (try 100000 if the display is flaky) |
| `BOARD` | `PJ26` | `make BOARD=BLUEPILL ...` for a standard Blue Pill (LED on PC13, CS5 on PB11) |

Pin assignments are in `firmware/USBi/usbi_hw.c`, `board.h` and `oled_hw.c`.

---

## 13. Troubleshooting

| Problem | Likely cause and fix |
|---|---|
| No heartbeat blink, OLED blank | Firmware not running: flashing failed, or not reset after flashing. Press RST. Re-flash (section 6) |
| Terminal shows nothing | `_LOG_CH340`: right COM port (the board's, not the breakout's)? 115200 8N1? `_LOG_PA2`: adapter RX on PA2? Only the LOG firmware prints |
| Board resets or stops when the terminal opens | The terminal asserts DTR/RTS (see the caution in section 6). Turn DTR/RTS off, or use `_LOG_PA2` |
| Windows: "device descriptor request failed" / OLED `DEF` | 1.5 kohm pull-up missing, D+/D- swapped or loose, long wires, ground not joined |
| OLED `ADR`, never `CFG` | The driver is not installed or not bound (section 5) |
| OLED `CFG` but SigmaStudio sees no USBi | Hardware Ids must be `VID_0456&PID_7031`; close and reopen SigmaStudio; replug the breakout. Check the log: if no requests appear, SigmaStudio never reached the device |
| Log or OLED shows `!` / `Unk` | A request the emulator does not implement. Send me the log |
| Writes report NACK | DSP did not answer: wrong address, no pull-ups on SDA/SCL, wiring, target unpowered |
| BERR | SDA or SCL stuck low: short, or missing ground |
| Reads return zeros / garbage | Check the sub-address bytes in the log (B3 data) and the DSP address pins |
| SPI does nothing | Chip-select: `wIndexH` selects CS1 to CS5 (PA4, PB0, PB1, PB10, PC13) |
| MCUISP cannot connect | See the table in section 6 |

---

## 14. Known limitations

- **Untested with real SigmaStudio hardware.** Everything is compiled and unit-tested, not field-tested.
- **Full-speed only.** The real USBi is a high-speed device; the F103 is full-speed. The Cypress driver should not mind, but SigmaStudio might.
- The purposes of requests **B1, B8, B9** and of an extra host command (called "reset toggle" in the SigmaStudio library) are **not fully known**. B7 is a reset pulse. Unknown requests are stalled and flagged.
- I2C and SPI are **bit-banged**: I2C 100 kHz, SPI about 500 kHz, so large downloads are slower than a real USBi's SPI.
- Writes up to 4096 bytes per transfer are accepted (SigmaStudio's host library appears to send data in 64-byte packs).
- PC13 to PC15 are limited (3 mA, 2 MHz); the original's two mode LEDs are not implemented.
- The USB D+ pull-up is a fixed resistor, so the board announces itself to the PC even before the firmware runs. Unplug the breakout cable while flashing.
- The DX-PJ26 details (PA11/PA12 on header P3 and separate from the CH340, user LED on PB11) come from the schematic summary and a V1.2 write-up; **check them on your board** (section 3).

---

## 15. Safety, legal and licences

- **Electrical:** 3.3 V logic only, common ground, do not hot-plug wires with power applied, avoid static. Never connect the breakout's VBUS to the board's 3.3 V.
- **USB IDs:** `0x0456` is Analog Devices' USB vendor ID and "USBi" is their product name. This firmware imitates that ID **for personal, compatibility use with your own SigmaStudio installation**. Do not sell or distribute hardware using that ID.
- **What is not included:** no Analog Devices or Cypress software, driver or firmware is in this zip. The device descriptors and protocol were derived by analysing files from your own SigmaStudio installation, for interoperability.
- **Third-party code** in `firmware/Drivers`, `firmware/Middlewares` (STMicroelectronics HAL, CMSIS, USB device library) is unmodified, and the OLED font is generated from the Spleen font (BSD-2-Clause, Frederic Cambus). Licences are in `firmware/licenses/`.
- The emulator sources are provided for your personal use, **without warranty**.

---

## Appendix A: using a standard Blue Pill instead

Build with `make BOARD=BLUEPILL all-hex` (LED on PC13, CS5 on PB11). A Blue Pill has its USB connector wired to PA11/PA12 already, so no breakout is needed (check its D+ pull-up R10 is 1.5 kohm). It has no CH340: flash through the chip's serial bootloader with a 3.3 V USB-serial adapter (adapter TXD to PA10, RXD to PA9, GND to GND), BOOT0 jumper to 1, press reset, program with MCUISP, then BOOT0 back to 0. The log then needs the `_LOG_PA2` build and the adapter RX moved to PA2.

## Appendix B: the protocol in one table

| bRequest | Dir | Meaning |
|---|---|---|
| B0 | | no-op |
| B1 | IN 1 byte | returns 0 |
| B2 | OUT | write: `wValueL` = address, `wIndexL` 0 = I2C / nonzero = SPI, `wIndexH` = SPI CS 1 to 5; data = payload; then STOP / CS release |
| B3 | OUT | same, but bus held: the register address before a read |
| B4 | IN | read `wLength` bytes (repeated START for I2C; SPI continues with CS held) |
| B5 | IN 1 byte | status of last B2/B3 (1 OK, 3 no ACK, 4 bus error) |
| B6 | IN 1 byte | status of last B4 |
| B7 | | target reset pulse (PA3) |
| B8 / B9 | | AUX1 / AUX2 outputs (PC14 / PC15) |

Full details and evidence: `PROTOCOL.md`.
