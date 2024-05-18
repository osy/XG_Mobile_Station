These are notes written up about the XG Mobile connector which is found on the Asus ROG Ally as well as some ROG Flow models. The goal is to build a custom eGPU enclosure that is compatible with the XG Mobile connector.

## XG Mobile Cable
The XG Mobile Cable comes with ROG XG Mobile and can be purchased from a [parts store][2] for $129 USD. It has a male XG Mobile Connector on one end and three cables on the other end: two 40-pin FPC cables and a 8-pin connector. The cable carries a USB-C connection, PCIe x8 signal pairs, PCIe clock, an SMBus/I2C connection, and some sideband signals for power detection and hot-plug support.

### XG Mobile Connector
The pinout can be obtained from the [GV301QC schematic][1] (page 69).
| Pin | Name           | Pin | Name              |
|-----|----------------|-----|-------------------|
| C1  | PCIENB_TXP0_C  | D1  | PCIENB_TXP5_C     |
| C2  | PCIENB_TXN0_C  | D2  | PCIENB_TXN5_C     |
| C3  | GND12          | D3  | GND21             |
| C4  | PCIENB_RXP0_C  | D4  | PCIENB_RXP5_C     |
| C5  | PCIENB_RXN0_C  | D5  | PCIENB_RXN5_C     |
| C6  | GND11          | D6  | GND20             |
| C7  | PCIENB_TXP1_C  | D7  | PCIENB_TXP6_C     |
| C8  | PCIENB_TXN1_C  | D8  | PCIENB_TXN6_C     |
| C9  | GND10          | D9  | GND19             |
| C10 | PCIENB_RXP1_C  | D10 | PCIENB_RXP6_C     |
| C11 | PCIENB_RXN1_C  | D11 | PCIENB_RXN6_C     |
| C12 | GND9           | D12 | GND18             |
| C13 | PCIENB_TXP2_C  | D13 | PCIENB_TXP7_C     |
| C14 | PCIENB_TXN2_C  | D14 | PCIENB_TXN7_C     |
| C15 | GND8           | D15 | GND17             |
| C16 | PCIENB_RXP2_C  | D16 | PCIENB_RXP7_C     |
| C17 | PCIENB_RXN2_C  | D17 | PCIENB_RXN7_C     |
| C18 | GND7           | D18 | GND16             |
| C19 | PCIENB_TXP3_C  | D19 | GPU_PCIE_CLKP     |
| C20 | PCIENB_TXN3_C  | D20 | GPU_PCIE_CLKN     |
| C21 | GND6           | D21 | GND15             |
| C22 | PCIENB_RXP3_C  | D22 | Reserve_NB        |
| C23 | PCIENB_RXN3_C  | D23 | CON_SW1_DET_NB    |
| C24 | GND5           | D24 | AGPU_SMB1_CLK     |
| C25 | PCIENB_TXP4_C  | D25 | AGPU_SMB1_DAT     |
| C26 | PCIENB_TXN4_C  | D26 | EC_ACGPU_MCU_IRQ# |
| C27 | GND4           | D27 | P_AC_LOSS_10      |
| C28 | PCIENB_RXP4_C  | D28 | DGPU_PWROK        |
| C29 | PCIENB_RXN4_C  | D29 | GPU_RST#          |
| C30 | GND3           | D30 | DGPU_PWR_EN#      |
| C31 | CON_DET_TOWER# | D31 | CON_DET_NB        |
| A1  | GND2           | B12 | GND14             |
| A2  | TX1+           | B11 | RX1+              |
| A3  | TX1-           | B10 | RX1-              |
| A4  | VBUS2          | B9  | VBUS4             |
| A5  | CC1            | B8  | SBU2              |
| A6  | D+1            | B7  | D-2               |
| A7  | D-1            | B6  | D+2               |
| A8  | SBU1           | B5  | CC2               |
| A9  | VBUS1          | B4  | VBUS3             |
| A10 | RX2-           | B3  | TX2-              |
| A11 | RX2+           | B2  | TX2+              |
| A12 | GND1           | B1  | GND13             |

### 40-pin connectors
Each 40-pin cable is terminated into a [CABLINE-VS connector][6]. The official Asus part number for this connector is 12017-00180600 and can be purchased from the parts store (usually with a mislabeled part number) such as [here][3].

Note that signal names are with respect to the host meaning that TX means host transmitter and RX means host receiver. In order of the longer cable first, the pinout for the two connectors are as follows:

| Pin | Connector | Description    |
|-----|-----------|----------------|
| 1   | D8        | PCIENB_TXN6_C  |
| 2   | D7        | PCIENB_TXP6_C  |
| 3   | GND       |                |
| 4   | D5        | PCIENB_RXN5_C  |
| 5   | D4        | PCIENB_RXP5_C  |
| 6   | GND       |                |
| 7   | D2        | PCIENB_TXN5_C  |
| 8   | D1        | PCIENB_TXP5_C  |
| 9   | GND       |                |
| 10  | C29       | PCIENB_RXN4_C  |
| 11  | C28       | PCIENB_RXP4_C  |
| 12  | GND       |                |
| 13  | C26       | PCIENB_TXN4_C  |
| 14  | C25       | PCIENB_TXP4_C  |
| 15  | GND       |                |
| 16  | C23       | PCIENB_RXN3_C  |
| 17  | C22       | PCIENB_RXP3_C  |
| 18  | GND       |                |
| 19  | C20       | PCIENB_TXN3_C  |
| 20  | C19       | PCIENB_TXP3_C  |
| 21  | GND       |                |
| 22  | C17       | PCIENB_RXN2_C  |
| 23  | C16       | PCIENB_RXP2_C  |
| 24  | GND       |                |
| 25  | C14       | PCIENB_TXN2_C  |
| 26  | C13       | PCIENB_TXP2_C  |
| 27  | GND       |                |
| 28  | C11       | PCIENB_RXN1_C  |
| 29  | C10       | PCIENB_RXP1_C  |
| 30  | GND       |                |
| 31  | C8        | PCIENB_TXN1_C  |
| 32  | C7        | PCIENB_TXP1_C  |
| 33  | GND       |                |
| 34  | C5        | PCIENB_RXN0_C  |
| 35  | C4        | PCIENB_RXP0_C  |
| 36  | GND       |                |
| 37  | C2        | PCIENB_TXN0_C  |
| 38  | C1        | PCIENB_TXP0_C  |
| 39  | GND       |                |
| 40  | C31       | CON_DET_TOWER# |

| Pin | Connector | Description                        |
|-----|-----------|------------------------------------|
| 1   | N/C       | 3.3V to cable                      |
| 2   | N/C       | 3.3V to cable                      |
| 3   | N/C       | 3.3V to cable                      |
| 4   | N/C       | 3.3V to cable                      |
| 5   | N/C       | Lock switch                        |
| 6   | N/C       | White LED switch (GND = on)        |
| 7   | N/C       | Red LED switch (GND = on)          |
| 8   | N/C       | Hidden switch on side of connector |
| 9   | D23       | CON_SW1_DET_NB                     |
| 10  | D22       | Reserve_NB                         |
| 11  | D31       | CON_DET_NB                         |
| 12  | D30       | DGPU_PWR_EN#                       |
| 13  | D29       | GPU_RST#                           |
| 14  | D28       | DGPU_PWROK                         |
| 15  | D27       | P_AC_LOSS_10                       |
| 16  | D26       | EC_ACGPU_MCU_IRQ#                  |
| 17  | GND       |                                    |
| 18  | D25       | AGPU_SMB1_DAT                      |
| 19  | D24       | AGPU_SMB1_CLK                      |
| 20  | GND       |                                    |
| 21  | A7        | USB D-                             |
| 22  | A6        | USB D+                             |
| 23  | GND       |                                    |
| 24  | B10       | USB RX-                            |
| 25  | B11       | USB RX+                            |
| 26  | GND       |                                    |
| 27  | A3        | USB TX-                            |
| 28  | A2        | USB TX+                            |
| 29  | GND       |                                    |
| 30  | D20       | GPU_PCIE_CLKN                      |
| 31  | D19       | GPU_PCIE_CLKP                      |
| 32  | GND       |                                    |
| 33  | D17       | PCIENB_RXN7_C                      |
| 34  | D16       | PCIENB_RXP7_C                      |
| 35  | GND       |                                    |
| 36  | D14       | PCIENB_TXN7_C                      |
| 37  | D13       | PCIENB_TXP7_C                      |
| 38  | GND       |                                    |
| 39  | D11       | PCIENB_RXN6_C                      |
| 40  | D10       | PCIENB_RXP6_C                      |

### 8-pin connector
This seems like a standard pin connector. The pitch is 1.50mm. Part number is [5-1775443-8][7].

| Pin | Connector | Description     |
|-----|-----------|-----------------|
| 1   | GND       |                 |
| 2   | GND       |                 |
| 3   | GND       |                 |
| 4   | A5        | USB CC          |
| 5   | ?         | Shorted to VBUS |
| 6   | VBUS      | USB VBUS        |
| 7   | VBUS      | USB VBUS        |
| 8   | VBUS      | USB VBUS        |

## Signals
Here's what can be learned from looking at the [GV301QC schematic][1]. The PCIe pairs are connected to a series of muxes which can be controlled by the APU to switch between the dGPU and eGPU. The USB-C is connected directly to the system's USB circuits and does not seem to be re-purposed for any alternative usage.

`CON_DET_TOWER#`, `EC_ACGPU_MCU_IRQ#`, `P_AC_LOSS_10`, and the SMBus goes to a IT5125VG-192 which is the EC (embedded controller) that usually handles system tasks and power management (no datasheet is available for this model).

`CON_DET_TOWER#` and `P_AC_LOSS_10` are fed into a MOSFET circuit for some kind of power detection.

`CON_SW1_DET_NB` is passed to both the APU and the EC as GPIO and is likely the way that the BIOS detects a connection. `CON_DET_NB` is shorted to GND and indicates to the eGPU of a connection.

`DGPU_PWROK`, `GPU_RST#`, `DGPU_PWR_EN#` are connected to the APU as GPIOs.

### Connection handshake

1. When connector is plugged in, `CON_DET_TOWER#` is asserted (active low) by the eGPU and the host EC detects the connection. The eGPU detects `CON_DET_NB` shorted to GND and knows that the host is connected. The eGPU also de-asserts `P_AC_LOSS_10` (pull down) to indicate AC power is connected.
2. When the connector lock is switched on, pin 5 on the 40P connector is shorted to GND and the eGPU detects this and asserts `CON_SW1_DET_NB`.
3. The host requests the eGPU to be powered on by asserting `DGPU_PWR_EN#` (active low) and `GPU_RST#` (active low).
4. The eGPU powers on the GPU and asserts `DGPU_PWROK`.
5. The host de-asserts `GPU_RST#`.
6. At this point, the connection is established and the host beings PCIe RX detection and link training.

## References

* [ASUS GV301QC Schematics & Board][1]
* [XG Mobile Cable][2], [40-pin connector][3], [8-pin connector][4], [XG Mobile connector][5]

[1]: https://www.badcaps.net/forum/troubleshooting-hardware-devices-and-electronics-theory/troubleshooting-laptops-tablets-and-mobile-devices/schematic-requests-only/103946-asus-gv301qc
[2]: https://www.a-accessories.com/asus-connection-cable-62505-79153.htm
[3]: https://www.a-accessories.com/asus-attach-system-40-pins-87588-67468.htm
[4]: https://www.a-accessories.com/asus-rog-flow-8-pin-connector-88837-78465.htm
[5]: https://www.a-accessories.com/rog-ally-accessories/ally-spare-parts/asus-rog-flow-dock-connector-92127-88896.htm
[6]: https://www.i-pex.com/product/cabline-vs
[7]: https://www.te.com/en/product-5-1775443-8.html
