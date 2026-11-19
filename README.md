<p align="center">
  <img src="https://github.com/staplez-studio/BluePill_Extra_Applications/blob/main/github-banner-1024x512.png?raw=true" alt="Staplez Studio Banner" width="85%" />
</p>UNDER CONSTRUCTION - DO NOT USE UNTIL NO WARNINGS

# USBi Emulator for the STM32 Blue Pill Dx-Smart PJ26-v1.2 Edition (Based off Original-STM32F103C8)

> **Author:** [Staplez Studio](https://github.com/staplez-studio)  
> **Main Repository:** [https://github.com/staplez-studio/USBi_BluePill](https://github.com/staplez-studio/USBi_BluePill)  
> **License:** Non-Commercial Attribution (See [LICENSE](LICENSE))

> ⚠️ **USBi / USB Vendor ID Notice:** While this firmware is original code, it utilizes an Analog Devices USB Vendor ID for SigmaStudio compatibility. Provided strictly for **personal, non-commercial use**. Commercial sale or distribution is prohibited unless the Analog Devices VID is completely removed and omitted from any user-facing distribution.

---

## Overview

Makes an STM32 Blue Pill Dx-Smart PJ26-v1.2 Edition (see "docs/PJ26.pdf" and "docs/")appear to Windows and SigmaStudio as an Analog Devices **USBi** and translates its vendor requests into **I2C** or **SPI** transfers for SigmaDSP targets.

### Note from Staplez Studio
I made this version because I actually don't own a original off the shelf STM32F103C8 witch what the code was originally written for. I did how ever have a BluePill The Dx-Smart-PJ26-v1.2 not expensive for what it is. I truly just wanted to help out the community,this edition of my firm ware has an option for a screen.

* **Bug Reports & Feature Requests:** I will look over any bug reports and add features on any reasonable request if it pertains to the project and adds benefit to the user or usability of the device. Reach out at **StaplezStudio@gmail.com**.

################### GENERIC INSTRUCTIONS  ##################### subject to change<<<<<<<<<<<<<<<
---

## ⚠️ Pre-Release Disclaimer
**HEADS UP** I HAVE NOT UPDATED THIS CODE YET 11/19/26 ILL BE BACK SOON TO UPDATE UNTIL THEN IT REMAINS JUST A CLONE OF THE MAIN
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

---

## Support the Project

If you used anything from this repo and want to show your appreciation, feel free to leave a small donation:  
**[PayPal.me/StaplezStudio](https://PayPal.me/StaplezStudio)**
