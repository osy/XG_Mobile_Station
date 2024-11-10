On the real XG Mobile, the MCU is an ITE chip which is a similar design to the Embedded Controller on the Ally. It is responsible for the following:

1. Toggling the [connector signals](Connector.md).
2. Communicating with the host's EC over I2C.
3. Communicating with the [Windows](Software.md#externalgpuconfighelper2dll) over USB HID.

In order to save costs and reduce complexity, our MCU is a STM32 which implements 1 and 2 but not 3. Since the main reason to communicate with Windows is to control fans (not available here) and to pass in serial and model information (can be done with software), we do not implement 3 which requires a USB PHY on the MCU.

## I2C
The I2C/SMBus pins from the connector is hooked up to the host's EC. The MCU communicates with the EC through this I2C bus (in addition to the status pins). The importance of the I2C bus is that upon reboot, a successful handshake must take place or the host will throw a splash image indicating the XG Mobile has been disconnected (see [the code](BIOS_Detect.c) for more details).

These are the commands supported by the real XG Mobile MCU:

| Command (Byte 0) | Byte 1          | Byte 2 | Remaining                 | Direction      | Description     |
|------------------|-----------------|--------|---------------------------|----------------|-----------------|
| 0xA0             | Register Number | Length | Data                      | Host to Device | Write Register  |
| 0xA1             | -               | -      | 0xD2, 0xDA, 0xDB, or 0xDC | Device to Host | Get event?      |
| 0xA2             | -               | -      | 0x01 or 0x02              | Device to Host | Session status? |
| 0xA3             | -               | -      | 0x01                      | Device to Host | Heartbeat?      |

These are the registers:

### Connect Sequence (Boot)

1. Host send: `A0 07 01 01`
2. Host send: `A0 01 01 00`
3. Host send: `A0 0A 01 04`
4. MCU asserts IRQ
5. Host send: `A1`, MCU respond: `DC`, deasserts IRQ
6. Host send: `A2`, MCU respond: `02`
7. Host send: `A0 08 01 01`
8. Host send: `A0 08 01 01`
9. Host send: `A0 09 01 00`
10. Host send: `A0 09 01 01`
11. Host send: `A0 09 01 01`
12. Host send: `A3`, MCU respond: `01` (repeat ad infinitum every second)

### Connect Sequence (XGM Disconnected on Boot)

1. Host send: `A0 07 01 01`
2. Host send: `A0 01 01 00`
3. Host send: `A0 0A 01 01`
4. MCU asserts IRQ
5. Host send: `A1`, MCU respond: `DC`, deasserts IRQ
6. Host send: `A2`, MCU respond: `02`

Press A to continue

1. Host send: `A0 08 01 01`
2. MCU asserts PWROK
3. Host de-asserts RST
4. Host send: `A0 09 01 00`
5. Host send: `A0 09 01 01`
6. Host send: `A0 09 01 01`
7. Host send: `A0 0A 01 02` (only sometimes?)
8. Host send: `A3`, MCU respond: `01` (repeat ad infinitum every second)

### Connect Sequence (Windows)

1. Host send: `A0 07 01 01`
2. Host send: `A0 01 01 00`
3. MCU asserts IRQ
4. Host send: `A1`, MCU respond: `DC`, deasserts IRQ
5. Host send: `A2`, MCU respond: `02`
6. Host send: `A0 0A 01 04`

XGM connect popup appears, user must click connect

1. Host send: `A0 08 01 01`
2. MCU asserts PWROK
3. Host de-asserts RST
4. Host send: `A0 09 01 01`
5. Host send: `A3`, MCU respond: `01` (repeat ad infinitum every second)

### Disconnect Sequence (Windows)

1. Host send: `A0 08 01 00`
2. Host stops sending `A3` after 2 more seconds

### Reconnect Sequence (Windows)

1. Host send: `A0 08 01 01`
2. Host send: `A3`, MCU respond: `01` (repeat ad infinitum every second)

### Shutdown Sequence (Windows)

1. Host send: `A0 0D 01 01`
2. Host send: `A0 09 01 00`
3. Host send: `A0 09 01 00`
4. Host send: `A0 01 01 05`
5. MCU asserts IRQ
6. Host send: `A1`, MCU respond: `DC`, deasserts IRQ
7. Host send: `A2`, MCU respond: `01`

## Embedded Controller
This information was gained by sniffing the I2C bus with a logic analyzer and also by reverse engineering the EC firmware. The EC for the ROG Ally can be found [here][1] and the MCU firmware for the 2021 XG Mobile can be found [here][2]. The ROG Ally has two chips a main controller and a secondary controller. Along with the XG Mobile's MCU, all three chips are different variants of ITE's embedded controller.

For reverse engineering the firmware, Google's [Chromebook EC firmware][3] is a good source of reference because it contains support code for a different model of ITE chip that uses the same ISA and has a similar memory layout. For disassembly, you need to build Ghidra with [NDS32 support][4].

[1]: https://dlcdnets.asus.com/pub/ASUS/GamingNB/RC71L/ROGMCUFWUpdateTool_OneKey_v3.8.0.011.zip
[2]: https://dlcdnets.asus.com/pub/ASUS/GamingNB/GC31S/ROGGC31FWUpdateTool_OneKey_V1.1.0.2.exe.zip
[3]: https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/chip/it83xx/
[4]: https://github.com/NationalSecurityAgency/ghidra/pull/1778
