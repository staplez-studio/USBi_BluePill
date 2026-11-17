# USBi emulator for the Blue Pill: complete guide from zip to SigmaStudio

> **Status, read this first.** The firmware compiles, the Intel hex files are validated, and the request-to-I2C/SPI
> translation is tested on a PC against simulated devices. The protocol was reverse-engineered from the USBi firmware
> image you supplied. It has **not yet been tested against real SigmaStudio on real hardware.** That first run is the
> job of this guide, and the debug-log firmware exists so that, if anything is missing, we can see exactly what
> SigmaStudio asks for.

---

## 1. What is in the zip

```
USBi_BluePill/
|-- README.md                          short overview
|-- docs/
|   |-- USBi_BluePill_Guide.md / .html this guide (the .html opens in any browser)
|   |-- PINOUT.md                      pins and wiring diagrams (print this)
|   `-- PROTOCOL.md                    the USBi protocol as decoded from the firmware
|-- firmware/
|   |-- hex/
|   |   |-- USBi_BluePill_LOG.hex      <-- flash THIS first (debug log on PA9)
|   |   |-- USBi_BluePill_LOG.bin
|   |   |-- USBi_BluePill_RUN.hex      <-- flash this once everything works (log off)
|   |   `-- USBi_BluePill_RUN.bin
|   |-- Makefile                       rebuild from source (section 10)
|   |-- STM32F103C8_FLASH.ld           linker script (64 KB flash, 20 KB RAM)
|   |-- Core/                          main.c, clock setup, interrupts, USB low-level glue
|   |-- USBi/                          the emulator itself (protocol, I2C/SPI, USB class, descriptors)
|   |-- Drivers/, Middlewares/         ST HAL, CMSIS and USB device library (unmodified, BSD/Apache)
|   `-- licenses/
`-- host_tests/                        PC test of the translation layer (optional)
```

Which file do I flash? **`firmware/hex/USBi_BluePill_LOG.hex`** the first time. It behaves identically to the RUN
version but also prints every request SigmaStudio sends on pin PA9.

---

## 2. What you need

**Hardware**
- Blue Pill board with an **STM32F103C8** (64 KB flash / 20 KB RAM) or **STM32F103CB**, plus a micro-USB cable. (An F103C6 with 32 KB / 10 KB is too small.)
- **USB-to-serial adapter, 3.3 V logic** (CP2102 / CH340 / FT232), used for flashing and for the log.
- Jumper wires (female-female or as your headers need).
- One **1.5 kohm resistor** (only if your board's D+ pull-up is not 1.5 kohm, see section 3).
- Two **4.7 kohm resistors** for I2C pull-ups, only if your DSP board has none.
- Optional: two LEDs with 330 ohm resistors; a soldering iron (for R10).
- Your DSP target board (I2C or SPI, **3.3 V I/O**).

**Software (Windows PC)**
- **SigmaStudio**, installed (it provides the Analog Devices USB driver files).
- **MCUISP** flashing tool (free Windows UART flasher for STM32).
- A serial terminal such as PuTTY or Tera Term (for the log).

---

## 3. Prepare the Blue Pill

1. **Look at the chip**: it should read STM32F103C8T6 (or a compatible clone). The firmware uses 14 KB of flash and 8.5 KB of RAM, so even a 64 KB / 20 KB part has plenty of room.
2. **Check the USB D+ pull-up (R10).** USB needs a **1.5 kohm** resistor from PA12 (USB D+) to 3.3 V. Many Blue Pills ship with a wrong value (often 10 kohm or 4.7 kohm). A wrong value is the number-one cause of "device descriptor request failed" or the board never being detected. Look for the resistor labelled **R10** near the USB connector; its marking `152` means 1.5 kohm, `472` means 4.7 kohm, `103` means 10 kohm. If it is not 152, replace it with 1.5 kohm (or solder a 1.5 kohm resistor from PA12 to 3V3 in parallel/instead).
3. **Locate the BOOT0 and BOOT1 jumpers** (two 3-pin headers with yellow jumpers). For normal running both jumpers sit on the **0** side. For flashing, move **BOOT0 to 1** and leave BOOT1 at 0.
4. **Do not** connect the board to your DSP yet.

---

## 4. Pinout and wiring

See **`PINOUT.md`** for the full table, the header map and the wiring diagrams. In short:

| What | Pins |
|---|---|
| I2C | **PB6 = SCL, PB7 = SDA** (+ 4.7 kohm pull-ups to 3.3 V if the target has none) |
| SPI | **PA5 = SCLK, PA6 = MISO, PA7 = MOSI, PB0 = CS1** (PB1, PB10, PB11, PB12 = CS2 to CS5) |
| Target reset (B7) | PB13 (open-drain) |
| AUX lines (B8, B9) | PB14, PB15 |
| Mode LEDs (optional) | PA1 = I2C, PA2 = SPI (low = on) |
| Debug log | **PA9 = TX** to the adapter's RX |
| Flashing | **PA9 = TX, PA10 = RX** to the adapter's RX / TX |
| USB | PA11 / PA12 (micro-USB connector) |

All logic is **3.3 V**. Join all grounds.

---

## 5. Windows driver

The Blue Pill identifies itself as `USB\VID_0456&PID_7031` ("USBi (programmed)"). Analog Devices' driver
package already contains an entry for exactly that ID and binds it to Cypress's generic `CyUSB` driver. The
firmware-download step in that package only applies to the *other* ID (`PID_7030`), so it is not used for your board.

1. **Check whether the driver is already installed.** If SigmaStudio was installed with its USB driver, the file
   `C:\Windows\System32\drivers\CyUSB.sys` typically exists. If so, skip to section 6.
2. **Otherwise install it.** Open the SigmaStudio folder that contains `CyUSB.inf`, `CyUSB.sys`, `CyUSB.cat`,
   `dpinst32.exe` and `dpinst64.exe` (the USB driver folder). On 64-bit Windows (check *Settings, System, About*,
   "System type") right-click **`dpinst64.exe`**, choose **Run as administrator**, and follow the wizard
   (**`dpinst32.exe`** on 32-bit Windows).
3. If the wizard does not work: Device Manager, right-click the device, *Update driver*, *Browse my computer*,
   *Let me pick*, *Have Disk...*, browse to `CyUSB.inf`, select **Analog Devices USBi (programmed)**.
4. If Windows refuses the driver as unsigned, it is the Windows driver-signature policy and not a problem with the
   firmware. The package has an Analog Devices catalog (`CyUSB.cat`), which normally satisfies it.

---

## 6. Flash the firmware with MCUISP (step by step)

You flash the **hex file** through the STM32's built-in UART bootloader. No programmer (ST-Link) is needed.

### 6.1 Wire the adapter
Connect as in `PINOUT.md` section 5: adapter **TXD to PA10**, adapter **RXD to PA9**, **GND to GND**. Leave the adapter's
3.3 V / 5 V pins unconnected. The Blue Pill gets its power from its **micro-USB cable**.

### 6.2 Enter the bootloader
1. Unplug the Blue Pill's micro-USB cable.
2. Move the **BOOT0 jumper to 1** (BOOT1 stays 0).
3. Plug the Blue Pill's micro-USB cable in again (or press the RESET button if it is already powered). The board is now waiting for the flasher; the heartbeat LED will **not** blink.
4. Plug the USB-serial adapter in. Find its COM port number in *Device Manager, Ports (COM and LPT)*.

### 6.3 Program with MCUISP
1. Start **MCUISP**. (The interface is mostly Chinese in some versions; the functions below are the same in every version. Labels may differ slightly.)
2. **Open the file:** use the open-file button/field and select
   `firmware\hex\USBi_BluePill_LOG.hex`. (A `.bin` file works too, but then the start address must be set to `0x08000000`; the `.hex` carries it automatically.)
3. **Choose the serial port:** select your adapter's COM port.
4. **Baud rate:** `115200`. (If connecting fails, try 57600.)
5. **Tick these options:**
   - **Verify** after programming (校验);
   - **Run after programming** / "execute" (编程后执行);
   - leave **"DTR low = reset, RTS high = BootLoader"** **unticked** (you are using the jumper, not DTR/RTS wiring);
   - leave any **option-bytes / read-protection** options untouched.
6. Press **Start programming** (开始编程, "Start ISP").
7. **Watch the log pane.** A good run shows the chip being detected (for an STM32F103 the ID is usually `0x410`), flash erase, programming progress to 100 %, **verify OK**, and "running". If it ends with an error see 6.5.

### 6.4 Return to normal mode
1. Unplug the USB cable.
2. Move the **BOOT0 jumper back to 0**.
3. Plug the micro-USB cable back in (to the PC). Within a second the **on-board PC13 LED should blink about once per second.** That means the firmware is running.

### 6.5 If flashing fails
| Symptom | Fix |
|---|---|
| "No response" / cannot connect | BOOT0 not on 1 when the board was reset. Redo 6.2 |
| Same | TX/RX swapped: adapter TXD must go to **PA10**, adapter RXD to **PA9** |
| Same | Wrong COM port, or another program is using it. Close PuTTY etc. |
| Same | Adapter is 5 V logic, or the grounds are not joined |
| Connects, then times out | Lower the baud to 57600 |
| Verify error | Re-run; check the wiring; try a shorter cable |
| Board was flashed before and is read-protected | Use ST's *STM32CubeProgrammer* to remove read protection, then retry |

### 6.6 Alternatives (optional)
- **STM32CubeProgrammer** (UART mode) accepts the same `.hex`.
- With an **ST-Link** on SWD (PA13 / PA14), STM32CubeProgrammer or STM32CubeIDE can flash the `.hex`/`.elf` directly; then no BOOT0 change is needed.

---

## 7. Check that it enumerates

1. With BOOT0 back at 0, the Blue Pill plugged into the PC, and the heartbeat LED blinking, open **Device Manager**.
2. After a moment a new device **"Analog Devices USBi (programmed)"** should appear (under *Universal Serial Bus controllers* or *Universal Serial Bus devices*). Open its Properties, Details, *Hardware Ids*: it must show `USB\VID_0456&PID_7031`.
3. **If you see "Unknown USB Device (Device Descriptor Request Failed)"** the board is not completing enumeration: check R10 (section 3), try a different USB cable and port, and reflash.
4. **If you see an unknown device with the right ID but no driver**, install the driver (section 5).

---

## 8. First test with SigmaStudio and capturing the log

1. **Connect the log:** adapter **RXD to PA9** (and GND). Open PuTTY / Tera Term on that COM port at **115200, 8 data bits, no parity, 1 stop bit, no flow control**.
2. Unplug and replug the Blue Pill (or press RESET) so the terminal sees it start.
3. **Wire your DSP** (section 4) with its power off, then power it on.
4. Start **SigmaStudio**, set up the project and hardware configuration for your DSP as for a normal USBi, and confirm that the USBi is detected. Then run **Link-Compile-Download**.
5. **Watch the terminal.** Every vendor request appears as one line, for example:

```
O rt=40 rq=B2 v=0034 i=0000 l=0004 : 00 10 AA BB     <- write 2-byte sub-address 0x0010 + data to I2C 0x34
I rt=C0 rq=B5 v=0000 i=0000 l=0001                    <- host asks for the write status
O rt=40 rq=B3 v=0034 i=0000 l=0002 : 00 10            <- set sub-address for a read
I rt=C0 rq=B4 v=0034 i=0000 l=0002                    <- read 2 bytes
I rt=C0 rq=B6 v=0000 i=0000 l=0001                    <- read status
O rt=40 rq=C3 v=0000 i=0000 l=0000 !                  <- "!" = a request the emulator does NOT know
```
   Line format: `O` = host-to-device, `I` = device-to-host; `rt` request type; `rq` request code; `v` wValue; `i` wIndex;
   `l` wLength; then the first 16 data bytes of writes. A trailing **`!`** marks a request that is not implemented.
6. **Save the log** (terminal "log to file" option). If anything fails, **send me the log, especially any `!` lines.**

What success looks like: SigmaStudio finishes the download without an error, the DSP's parameters change, and no `!` lines appear.

---

## 9. When it works: switch to the RUN firmware

The LOG firmware prints inside the USB interrupt, which adds a few milliseconds per request. Once everything
works, reflash with **`firmware\hex\USBi_BluePill_RUN.hex`** (same steps as section 6) to get full speed. You can also remove the adapter then.

---

## 10. Rebuilding from source

You do not need this to use the board; the hex files are already compiled. Use it if you change pins, speeds or
behaviour.

### Option A: command line (Makefile)
1. Install the **Arm GNU Toolchain** (`arm-none-eabi-gcc`) and `make`. On Windows the easiest source of both is **STM32CubeIDE**, which bundles them inside its installation folder (look under the IDE's `plugins` folder for `gnu-tools-for-stm32` and the bundled `make`); add their `bin` folders to PATH. Alternatively install MSYS2 and run `pacman -S make mingw-w64-ucrt-x86_64-arm-none-eabi-gcc`.
2. Open a terminal in `firmware/` and run:
   ```
   make clean
   make all-hex
   ```
   This builds both versions and copies them to `firmware/hex/`. `make` alone builds only the LOG version;
   `make release` builds only the RUN version. Outputs: `.hex`, `.bin`, `.elf`, `.map` under `firmware/build/`.

### Option B: STM32CubeIDE
1. *File, New, Makefile Project with Existing Code*. Select the `firmware` folder, choose the toolchain
   "STM32CubeIDE" (or `<none>` and set PATH as above).
2. *Project, Build All*. The Makefile produces the same hex files (`firmware/hex` after `make all-hex`, or `build/`).
3. The resulting `.hex` is flashed with MCUISP exactly as in section 6.

### Verifying your build
Both hex files were validated (every record checksum, address range `0x08000000` upward, vector table, reset vector).
SHA-256 of the supplied files (compare with `certutil -hashfile <file> SHA256` on Windows):

| File | Size (bytes) | SHA-256 |
|---|---|---|
| `USBi_BluePill_LOG.hex` | 40733 | `27353a6afa73352981b5e24522fa02f07af5075f3b7a32e156037617699b0696` |
| `USBi_BluePill_RUN.hex` | 39240 | `4db256c83e3360a2414ba954f6c99d16f07d8a0caffacc3fd5d0eba7a3824bd4` |

---

## 11. Configuration options

Edit and rebuild (section 10). All of these can be set in `firmware/Makefile` (`DEFS`) or at the top of the file named.

| Setting | Where | Default | Meaning |
|---|---|---|---|
| `USBI_LOG` | Makefile `LOG` | 1 (LOG build), 0 (RUN build) | Debug log on PA9 |
| `USBI_I2C_HZ` | `usbi_hw.c` | 100000 | I2C clock (try 400000 if the target allows) |
| `USBI_SPI_HZ` | `usbi_hw.c` | 500000 | SPI clock |
| `USBI_BUF_SIZE` | `usbd_usbi.c` (or `-DUSBI_BUF_SIZE=...`) | 4096 | Largest single transfer accepted |
| Pin assignments | `usbi_hw.c` | see PINOUT.md | GPIO ports/pins |

To change a speed without editing the file add `-DUSBI_I2C_HZ=400000` to `DEFS` in the Makefile.

---

## 12. Troubleshooting

| Problem | Likely cause and fix |
|---|---|
| Heartbeat LED (PC13) does not blink | Firmware not running: BOOT0 still on 1, flash failed, or a bad HSE crystal. Redo section 6 |
| Windows says "device descriptor request failed" | D+ pull-up not 1.5 kohm (section 3). Try another cable/port. Reflash |
| Device appears but no driver | Install the ADI USB driver (section 5) |
| Device Manager shows the device, SigmaStudio does not see a USBi | Check the Hardware Ids really are `VID_0456&PID_7031`; close and reopen SigmaStudio; replug the board; confirm the log shows requests (if it shows nothing, SigmaStudio never reached the device) |
| Log shows `!` lines | A request the emulator does not implement. Send me the log |
| Writes report failure (status 3) | Slave did not answer: wrong address, missing pull-ups on SDA/SCL, wrong wiring, or target not powered |
| Status 4 | Bus error: SDA or SCL stuck low (short, or missing ground) |
| I2C reads return zeros / garbage | Check the sub-address bytes in the log (B3 data) and the target's address pins |
| SPI does nothing | Check chip-select: `wIndexH` selects CS1 to CS5 (PB0, PB1, PB10, PB11, PB12) |
| Flashing works only sometimes | Poor ground or long jumper wires; use 3.3 V logic adapter; lower the baud rate |

---

## 13. Known limitations (be aware of these)

- **Untested with real SigmaStudio hardware.** Everything is compiled and unit-tested, not field-tested.
- **Full-speed only.** The real USBi is a high-speed device; the F103 is full-speed. The Cypress driver should not mind, but SigmaStudio might.
- The purposes of requests **B1, B8, B9** and of an extra host command (called "reset toggle" in the SigmaStudio library) are **not fully known**. B7 is a reset pulse. Unknown requests are stalled and, in the LOG build, flagged with `!`.
- I2C and SPI are **bit-banged** (like the original's SPI). I2C is 100 kHz and SPI about 500 kHz, so very large downloads are slower than a real USBi's SPI.
- Writes up to 4096 bytes per transfer are accepted (SigmaStudio's host library appears to send data in 64-byte packs).
- The Blue Pill has no level shifters or protection. Do not connect 5 V.

---

## 14. Safety, legal and licences

- **Electrical:** 3.3 V logic only, common ground, do not hot-plug wires with power applied, avoid static.
- **USB IDs:** `0x0456` is Analog Devices' USB vendor ID and "USBi" is their product name. This firmware imitates that ID **for personal, compatibility use with your own SigmaStudio installation**. Do not sell or distribute hardware using that ID.
- **What is not included:** no Analog Devices or Cypress software, driver or firmware is in this zip. The device descriptors and protocol were derived by analysing files from your own SigmaStudio installation, for interoperability.
- **Third-party code** in `firmware/Drivers` and `firmware/Middlewares` (STMicroelectronics HAL, CMSIS, USB device library) is unmodified and under its own licences (BSD-3-Clause / Apache-2.0), in `firmware/licenses/`.
- The emulator sources in `firmware/USBi` and `firmware/Core` are provided for your personal use, **without warranty**.

---

## Appendix: the protocol in one table

| bRequest | Dir | Meaning |
|---|---|---|
| B0 | | no-op |
| B1 | IN 1 byte | returns 0 |
| B2 | OUT | write: `wValueL` = address, `wIndexL` 0 = I2C / nonzero = SPI, `wIndexH` = SPI CS 1 to 5; data = payload; then STOP / CS release |
| B3 | OUT | same, but bus held: used for the register address before a read |
| B4 | IN | read `wLength` bytes (repeated START for I2C; SPI continues with CS held) |
| B5 | IN 1 byte | status of last B2/B3 (1 OK, 3 no ACK, 4 bus error) |
| B6 | IN 1 byte | status of last B4 |
| B7 | | target reset pulse (PB13) |
| B8 / B9 | | AUX1 / AUX2 outputs (PB14 / PB15) |

Full details and evidence: `PROTOCOL.md`.
