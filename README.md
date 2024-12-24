XG Mobile Dock
==============

This open source hardware allows you to connect any PCIe card to an ASUS ROG device with the XG Mobile connector.

[![Board 3D model](Docs/images/board-3d.png)](Docs/images/board-3d.png)

The standard variant is a drop-in replacement PCB for the [XG Station Pro Thunderbolt 3 eGPU dock][1]. It contains a built in USB 3.1 Gen 2 hub and a USB PD charger.

[![Board 3D model](Docs/images/board-lite-3d.png)](Docs/images/board-lite-3d.png)

The lite variant is a fully-functional PCB design based on the [ADT-UT3G][6]. It requires a standard ATX power supply and passes through the USB to an external port.

## Features

* PCIe 3.0 x8 support for 2021/2022/2023 ROG Flow (4.0 currently untested)
* PCIe 4.0 x4 support for 2023 ROG Ally
* MCU handling cable detection and LEDs
* 65W USB PD charger (standard variant)
* 2 USB-C ports connected to a USB 3.1 Gen 2 hub (standard variant)

## Getting Started

1. [Build the PCB](Docs/Build_Guide.md)
2. [Flash the board](#flashing-firmware)
3. [Install the software](#install-xgmactivator)

### Flashing Firmware

Lite boards only need to flash STM32 while the standard board requires writing two SPI flash as well.

#### STM32 MCU

1. Download `XG_Mobile_Dock_MCU.bin` from the latest release or [build your own](Docs/Build_Guide.md#mcu).
2. Download and install [ST32CubeProgrammer][2].
3. Connect your ST-LINK v2 to your computer. Note if you are using a cheap clone from Amazon or Aliexpress, the pin numbers printed on the device may be incorrect.
4. Connect the SWDIO, SWCLK, and GND pins to J10 on the board to the ST-LINK v2. Do not connect +3V3. If you are using an official ST-LINK, you will need a jumper wire from VAPP (pin 1) to VDD (pin 19).
5. Ensure the board is powered on so it can be programmed.
6. Open ST32CubeProgrammer and go to the "Erasing & programming" page (second icon on the left sidebar).
7. Browse and select the firmware file.
8. Check "Run after programming"
9. Click "Connect" on the right sidebar and then "Start Programming" on the left.

#### SPI Flash for TI USB PD

We will be using a Raspberry Pi, although most other SBC can also work as well as dedicated SPI flashers.

1. [Enable the SPI interface on the Raspberry Pi.][3]
2. Connect GND (Ground), SS (SPI0 CE0), CLK (SPI0 SCLK), MISO (SPI0 MISO), and MOSI (SPI0 MOSI) on J9 [to the Raspberry Pi][4].
3. Open a shell to the Raspberry Pi and install Flashrom: `sudo apt-get install flashrom`
4. Download `XG_Mobile_Dock_Charger.bin` from the latest release or [build your own](Docs/Build_Guide.md#ti-pd-controller).
5. Flash the firmware: `sudo flashrom -p linux_spi:dev=/dev/spidev0.0,spispeed=1000 -w XG_Mobile_Dock_Charger.bin`

#### SPI Flash for VIA USB Hub

We will be using a Raspberry Pi, although most other SBC can also work as well as dedicated SPI flashers.

1. [Enable the SPI interface on the Raspberry Pi.][3]
2. Connect GND (Ground), SS (SPI0 CE0), CLK (SPI0 SCLK), MISO (SPI0 MISO), and MOSI (SPI0 MOSI) on J13 [to the Raspberry Pi][4]. Note the order of the pins is different from J9.
3. Open a shell to the Raspberry Pi and install Flashrom if it is not already installed: `sudo apt-get install flashrom`
4. Download `VL822_Q7_9043_Phantom_20220616.bin` from the latest release.
5. Flash the firmware: `sudo flashrom -p linux_spi:dev=/dev/spidev0.0,spispeed=1000 -w VL822_Q7_9043_Phantom_20220616.bin`

### Install XGMDriver

XGMDriver tricks ARMORY CRATE software into identifying the custom dock as an official XG Mobile device. Once installed, it should work even if ARMORY CRATE software is updated. You can check out the [source code here](XGMDriver).

1. Download `XGMDriverSetup.exe` from the latest release.
2. Run the installer making sure to correctly select AMD or NVIDIA depending on the vendor of the GPU you are installing.
3. If you need to swap between AMD and NVIDIA, uninstall from Control Panel or by running the installer again. Then you can re-install and select the right option.

## Troubleshooting

### Error 43 or no video output on NVIDIA GPUs

This is a well known issue with NVIDIA eGPUs. Once the eGPU is installed along with the correct drivers, you will need to install [this script][5].

### PCIe is only getting 3.0 speeds

You need to restart your device. For some reason, hot plugging sometimes results in 3.0 speeds.

### No popup when XGM is connected

Sometimes, the device will not be detected and you can flip the lock switch off and on again to force the software to re-detect the device.

### "It appears that your XG Mobile is not properly connected..."

If you get a pop-up saying XG Mobile is not properly connected, make sure [XGMDriver](#install-xgmdriver) is installed. If this is still an issue with XGMDriver installed, there is likely a connection issue with the cable.

### ASUS Driver install popup on reboot

Re-install XGMDriver to inhibit the ASUS driver popup.

### Lite: USB is not detected

The lite board does not have USB orientation detection. Try flipping the USB-C cable upside down and try again.

## References

Knowledge base for all things XGM gathered from reverse engineering the hardware and software.

* [XGM connector information](Docs/Connector.md)
* [XGM software interfaces](Docs/Software.md)
* [MCU command interface](Docs/MCU.md)
* [PCB design diary](Docs/Diary.md)
* [Reversed ACPI tables](Docs/ACPI_Annotated.asl)
* [Reversed BIOS detection](Docs/BIOS_Detect.c)

[1]: https://www.asus.com/motherboards-components/external-graphics-docks/all-series/xg-station-pro/
[2]: https://www.st.com/en/development-tools/stm32cubeprog.html
[3]: https://www.raspberrypi-spy.co.uk/2014/08/enabling-the-spi-interface-on-the-raspberry-pi/
[4]: https://pinout.xyz
[5]: https://egpu.io/nvidia-error43-fixer
[6]: https://www.adt.link/product/UT3G.html
