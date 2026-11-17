# USBi (0456:7031) vendor protocol: recovered from the USBi firmware image

Source: `ADI_USBi.spt` is a record list of Cypress `0xA0` RAM-download requests (CPUCS hold reset,
write 8051 code, release reset). Reassembling it gives two images: a small Cypress stage
(04B4:1004) and the final image that enumerates as 0456:7031. Everything below comes from
disassembling that final image (8051). Addresses are firmware code addresses, so you can verify.

## Enumeration (final image, descriptors at 0x900)
- Device: bcdUSB 2.00, class 0, EP0 64, VID 0456, PID 7031, bcdDevice 0000, iMfr 1, iProd 2, iSerial 0
- Strings: LANGID 0409, manufacturer "Cypress", product "EZ-USB"
- Config: 1 interface, class 0xFF, 4 bulk endpoints (EP2 OUT, EP4 OUT, EP6 IN, EP8 IN), bus powered 500 mA.
  HS maxpacket 512, FS 64. The bulk-endpoint interrupt vectors are empty: **all traffic is on EP0**.

## Vendor requests (dispatch: bRequest-0xB0, 10-entry table at 0x05A4; handlers 0x05C2-0x06D1)
Setup fields from SETUPDAT: wValueL=E6BA, wIndexL/H=E6BC/E6BD, wLength=E6BE/E6BF.

| bRequest | Dir | Behaviour |
|---|---|---|
| B0 | - | no-op |
| B1 | IN 1 | returns 0x00 (purpose unknown) |
| B2 | OUT | write. wValueL = address, wIndexL = 0 (I2C) / nonzero (SPI), wIndexH = SPI CS 1..5. Data stage = payload. Bus released at end. |
| B3 | OUT | same as B2 but bus **held**: I2C no STOP, SPI CS stays low. Data = register/sub-address bytes. SPI first byte has bit0 set. |
| B4 | IN | read wLength bytes (I2C: repeated START + (wValueL<<1)\|1; SPI: continue with CS held, then release) |
| B5 | IN 1 | status of last B2/B3, then cleared |
| B6 | IN 1 | status of last B4, then cleared |
| B7 | - | pulse P1.0 low (~0.4 ms): target reset (matches host name CMD_RESET) |
| B8 | - | P1.4 = (wValueL != 0): likely CMD_POWERBOARD (target power switch) |
| B9 | - | P1.2 = (wValueL != 1): likely CMD_LED (low = LED on) |

Status byte: 1 = OK, 2 = more data pending (long writes), 3 = no ACK, 4 = bus error
(from the I2C interrupt routine at 0x02B7 testing the FX2 I2CS register).

## Wire formats
- I2C: START, (wValueL<<1)|R/W, bytes..., STOP. 100 kHz default (hardware I2C block).
- SPI: bit-banged on port A. SCLK idles high, data changes after the falling edge and is sampled on
  the rising edge (= SPI mode 3), MSB first. First byte = (wValueL<<1)|R, then payload. CS1..5 = P0.2..P0.6.
- SigmaDSP register read = B3 (sub-address) -> B4 (n bytes) -> B6 (status).
- P1.1 / P1.3 look like I2C / SPI mode LEDs.

## Cross-check against the host library (ADIUSBi.dll, class Analog.ADIUSBi)
Method bodies in that DLL are encrypted, but its metadata survives:
- Command fields, in declaration order: CMD_WRITE, CMD_WRITE_READ, CMD_READ, CMD_CHECK_WRITE,
  CMD_CHECK_READ, CMD_RESET, CMD_POWERBOARD, CMD_LED, CMD_RESET_TOGGLE. The first eight line up with
  B2..B9 (write, write-then-read address phase, read, two status checks, reset, power, LED).
  Order is only a strong hint: the numeric values are not visible. CMD_RESET_TOGGLE has no match in the firmware table.
- enum TransferStatus: None, Success, Writing, Failed, Error, Exception = 0..5, which matches the
  status byte (1 OK, 2 writing/more, 3 failed/NACK, 4 error).
- enum CommsFormat: I2C, SPI (consistent with wIndexL 0 / nonzero).
- USBiFixPackSize = 64 (set in the static constructor): the host sends data in packs of at most 64 bytes,
  with a configurable delay between them (SetSplitPackDelay, ChangeMaxDataBytes).
- Host API: ReadData, WriteData, WriteByteAddressData, GetReadStatus, GetWriteStatus, Reset,
  ResetDevice, ResetDeviceToggle, PowerBoard, EnableLED; it polls status (waitForWrite/waitForRead).

## Not verified / limits
- Written from the code; not yet tested with a real SigmaStudio session.
- The original is high-speed; the F103 is full-speed only. SigmaStudio may or may not care.
- Original streams writes in 64-byte EP0 packets; the emulator accepts up to 4096 bytes per transfer.
- B1/B8/B9 purposes and the exact timing of the real device are unknown.

## Debug log format (LOG firmware, UART TX on PA9, 115200 8N1)
One line per vendor request: `<O|I> rt=<bmRequestType> rq=<bRequest> v=<wValue> i=<wIndex> l=<wLength> [: data...] [!]`
`O` = host to device (first 16 data bytes shown), `I` = device to host, `!` = request not implemented.
