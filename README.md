<p align="center">
<img src="https://github.com/staplez-studio/BluePill_Extra_Applications/blob/main/github-banner-1024x512.png?raw=true" alt="Staplez Studio Banner" width="85%" />
</p>

# USBi emulator for the STM32 Blue Pill (STM32F103C8) by Staplez Studio
The firmware uses Analog Devices’ vendor ID, so keep it for personal use and don’t sell or distribute devices that use it.
> **Author:** [Staplez Studio](https://github.com/staplez-studio)  
> **Repository:** [https://github.com/staplez-studio/USBi_BluePill](https://github.com/staplez-studio/USBi_BluePill)  
> **License:** Non-Commercial Attribution (See [LICENSE](LICENSE))

---

## Overview

Note from the Staplez Studio
I made this because the USBi is extremely expensive for what it is and sometimes can be hard to source and I truly just wanted to save people time and money and if it was reasonably priced and I could get one quickly this would have never happened. 

PRE-RELEASE Note - absolutely nothing has been tested. Staplez Studio takes no liability whatsoever for the use or misuse of the contents within. and I take no liability or responsibility for the outcome. As soon as I test everything and do my full audit I will update this to a full in depth guide.

I WILL LOOK OVER ANY BUG REPORTS
AND/OR ADD FEATURES ON ANY REASONABLE REQUEST IF IT PERTAINS TO THE PROJECT AND ADDS ANY BENEFIT TO THE USER OR PROJECT OR THE USABILITY OF THE DEVICE. StaplezStudio@gmail.com

Makes a Blue Pill appear to Windows and SigmaStudio as an **USBi** (USB `0456:7031`) and translates
its vendor requests into **I2C** or **SPI** transfers for SigmaDSP targets.

**Start here:** `docs/USBi_BluePill_Guide.html` (or `.md`): complete steps from opening this zip to a working setup.

Quick path: NOTE I don't know if thie hex files included are the same as the one produced after compilation 

I cannot confirm the following instructions work.
I'll be testing the instructionsas as followed but I also may update with some new instructions making a project in stm32 and using the hex made directly from compiling,

I cannot guarantee this will compile without errors. 



1. Read `docs/PINOUT.md`, wire the board.
2. Flash **`firmware/hex/USBi_BluePill_LOG.hex`** with MCUISP (BOOT0 = 1, adapter TX to PA10, RX to PA9, 115200).
3. Set BOOT0 back to 0, replug. The PC13 LED blinks, and Windows shows "Analog Devices USBi (programmed)".
4. Run SigmaStudio, watch the log on PA9 (115200 8N1), and send the log if anything fails.

Status: compiled, hex validated, protocol layer unit-tested on a PC. Not yet tested against real SigmaStudio hardware.

if you used anything from this repo.
and want to show your appreciation feel free to leave a small donation
PayPal.me/StaplezStudio
