 I dont have this hardware to test on FURTHERMORE have merged PROJECTS for the better good so i can work on things together so im no-longer touching anything on this code alone as all updates to the code will be presented to you in multi-flasher as it keeps everything in a nice neat all in one place.https://github.com/staplez-studio/Multi-Flash/
<br>

<p align="center">
  <img src="https://github.com/staplez-studio/BluePill_Extra_Applications/blob/main/github-banner-1024x512.png?raw=true" alt="Staplez Studio Banner" width="85%" />
</p>

# USBi Emulator for the STM32 Blue Pill (STM32F103C8)

> **Author:** [Staplez Studio](https://github.com/staplez-studio)  
> **Repository:** [https://github.com/staplez-studio/USBi_BluePill](https://github.com/staplez-studio/USBi_BluePill)  
> **License:** Non-Commercial Attribution (See [LICENSE](LICENSE))

> ⚠️ **USBi / USB Vendor ID Notice:** While this firmware is original code, it utilizes an Analog Devices USB Vendor ID for SigmaStudio compatibility. Provided strictly for **personal, non-commercial use**. Commercial sale or distribution is prohibited unless the Analog Devices VID is completely removed and omitted from any user-facing distribution.

---

## Overview

Makes an STM32 Blue Pill appear to Windows and SigmaStudio as an Analog Devices **USBi** and translates its vendor requests into **I2C** or **SPI** transfers for SigmaDSP targets.

### Note from Staplez Studio
I made this because the official USBi is extremely expensive for what it is and can be hard to source. I truly just wanted to save people time and money—if it were reasonably priced and easy to get quickly, this project would have never happened.

* **Bug Reports & Feature Requests:** I will look over any bug reports and add features on any reasonable request if it pertains to the project and adds benefit to the user or usability of the device. Reach out at **StaplezStudio@gmail.com**. PLEASE DON'T SEND ME BUG REPORTS FOR THE MAIN as I'm not currently working on it. feel free to fork your own as long as you stay within the licenses.

---

## ⚠️ Pre-Release Disclaimer

**Status:** Compiled, hex validated, and protocol layer unit-tested on a PC. **Not yet tested against real SigmaStudio hardware.**

* **No Liability:** Absolutely nothing has been hardware-tested yet. Staplez Studio takes no liability or responsibility whatsoever for the use, misuse, or outcome of the contents within. Once everything is tested and audited, this README will be updated with a full, in-depth guide.
* **Build & Hex Notes:** I cannot yet guarantee the project will compile without errors, nor confirm whether the pre-compiled `.hex` files match a fresh build. I am actively testing these instructions and may update them with a full STM32 project setup guide using freshly compiled hex binaries.

---

## Quick Start

**Start here:** See `docs/USBi_BluePill_Guide.html` (or `.md`) for complete steps from opening this repository to a working setup.

1. Read `docs/PINOUT.md` and wire the board.
2. Flash **`firmware/hex/USBi_BluePill_LOG.hex`** using MCUISP (`BOOT0 = 1`, adapter TX to `PA10`, RX to `PA9`, `115200` baud).
3. Set `BOOT0` back to `0` and replug the board. The `PC13` LED should blink, and Windows should recognize the device as **"Analog Devices USBi (programmed)"**.
4. Open SigmaStudio, monitor the serial debug log on `PA9` (`115200 8N1`), and send the log if anything fails.

## Credits and licence
Copyright (c) 2026 **Staplez Studio**.

**Free for personal, non-commercial use. Commercial use needs a licence: contact Staplez Studio first.** Full terms in [LICENSE.md](LICENSE.md).

All original application code, screen layouts, weather and status icons, bitmaps and artwork in this repository are the work of Staplez Studio, except where listed below.

### Third-party credits
| Who | What |
|---|---|
| STMicroelectronics | `Drivers/STM32F1xx_HAL_Driver`, the CMSIS device headers (`stm32f103xb.h`, `stm32f1xx.h`, `system_stm32f1xx.h`) and `Core/Startup/startup_stm32f103c8tx.s`, from the STM32CubeF1 / CubeMX tooling. **Restore ST's original licence notices** (they are not in this tree). Some headers were trimmed. |
| Arm | `Drivers/CMSIS/Include/core_cm3.h` (CMSIS-Core). **Restore the original Apache-2.0 notice.** |
| RealNetworks (Helix), Phil Schatzmann (`arduino-libhelix`), Earle Philhower (ESP8266Audio lineage) | MP3 decoder library used by the ESP32-CAM sketch (`tools/esp32cam_bridge`). Not bundled; the decoder code is under the RealNetworks Public Source License. Read it before sharing a compiled cam binary. |
| Espressif | Arduino-ESP32 core and I2S driver used by the cam sketch; the AT command style follows ESP-AT; the DTR/RTS auto-reset convention in the USB adapter. |
| Igor Chubin (wttr.in) | Public weather service queried over HTTP (project licence: Apache-2.0). No attribution is required by its page; credited as a courtesy. |
| NTP Pool Project | `pool.ntp.org` time servers. For a commercial product use a vendor zone. |
| pid.codes | USB ID 1209:0001 in the USB adapter, their test pair (testing only). |
| USB-IF, Solomon Systech / Sino Wealth, DFRobot, Analog Devices, RDM6300/EM4100 makers | Specifications and datasheets followed by the USB adapter, the OLED driver, the DFPlayer, the ADAU1401/1701 and the RFID reader code (written from the documents, no code copied). |
| DX-SMART | The PJ26 development board. |
| LLVM Project | clang and lld, used to build the USB adapter firmware (not distributed). |

---

## Support the Project

If you used anything from this repo and want to show your appreciation, feel free to leave a small donation:  
**[PayPal.me/StaplezStudio](https://PayPal.me/StaplezStudio)**
