<p align="center">
  <img src="https://github.com/staplez-studio/BluePill_Extra_Applications/blob/main/github-banner-1024x512.png?raw=true" alt="Staplez Studio Banner" width="85%" />
</p>
# USBi emulator for the Blue Pill (STM32F103C8) Dx-Smart-PJ26-v1.2-Edition 

Makes a Blue Pill appear to Windows and SigmaStudio as an Analog Devices **USBi** (USB `0456:7031`) and translates
its vendor requests into **I2C** or **SPI** transfers for SigmaDSP targets.

**Start here:** `docs/USBi_BluePill_Guide.html` (or `.md`): complete steps from opening this zip to a working setup.

Quick path:
1. Read `docs/PINOUT.md`, wire the board.
2. Flash **`firmware/hex/USBi_BluePill_LOG.hex`** with MCUISP (BOOT0 = 1, adapter TX to PA10, RX to PA9, 115200).
3. Set BOOT0 back to 0, replug. The PC13 LED blinks, and Windows shows "Analog Devices USBi (programmed)".
4. Run SigmaStudio, watch the log on PA9 (115200 8N1), and send the log if anything fails.

Status: compiled, hex validated, protocol layer unit-tested on a PC. Not yet tested against real SigmaStudio hardware.
