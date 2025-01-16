The dock was designed from the beginning to be assembled at JLCPCB because of their low cost PCB assembly option. If you have not ordered from JLCPCB before, this guide will help you get started. There are two configurations you can build and some minor differences between them. The regular configuration is designed to be a complete replacement for the XG Station Pro main board and requires its proprietary power connector. It also supports USB charging and a 2-port USB hub. The lite version requires a standard ATX power supply and passes through the USB port to an external connection.

> Hint: You should visit https://jlcpcb.com/coupon-center and clip the coupons to save on your order.

## Board Guide

1. Download `GERBER-XG_Mobile_Dock[_Lite].zip`, `CPL-XG_Mobile_Dock[_Lite].csv`, and `BOM-XG_Mobile_Dock[_Lite].csv` from the [latest release on GitHub](https://github.com/osy/XG_Mobile_Station/releases). Choose the regular or lite variant depending on the build you want. Do not mix and match files.
2. Visit https://cart.jlcpcb.com/quote and upload `GERBER-XG_Mobile_Dock[_Lite].zip`
3. Fill the PCB options as follows (for each design)

    | Option                     | Standard                    | Lite                | Notes                                                           |
    |----------------------------|-----------------------------|---------------------|-----------------------------------------------------------------|
    | Base Material              | FR-4                        | FR-4                |                                                                 |
    | Layers                     | 6                           | 4                   |                                                                 |
    | Dimensions                 |                             |                     | Should be auto filled                                           |
    | PCB Qty                    | Any                         | Any                 |                                                                 |
    | Product Type               | Industrial/Consumer         | Industrial/Consumer |                                                                 |
    | Different Design           | 1                           | 1                   |                                                                 |
    | Delivery Format            | Single PCB                  | Single PCB          |                                                                 |
    | PCB Thickness              | 1.6                         | 1.6                 |                                                                 |
    | PCB Color                  | Any                         | Any                 | Green is cheapest                                               |
    | Silkscreen                 | White                       | White               |                                                                 |
    | Material Type              | **FR4-Standard TG 135-140** | **FR-4 TG155**      |                                                                 |
    | Surface Finish             | ENIG                        | **ENIG**            | HASL (with lead) can be used in low volume runs for cost saving |
    | Outer Copper Weight        | 1 oz                        | 1 oz                |                                                                 |
    | Inner Copper Weight        | 0.5 oz                      | 0.5 oz              |                                                                 |
    | Specify Layer Sequence     | No                          | No                  |                                                                 |
    | Impedance Control          | **Yes**                     | **Yes**             |                                                                 |
    | Layer Stackup              | **JLC06161H-3313**          | **JLC04161H-7628**  |                                                                 |
    | Via Covering               | Epoxy Filled & Capped       | Plugged             |                                                                 |
    | Min via hole size/diameter | 0.3mm/(0.4/0.45mm)          | 0.3mm/(0.4/0.45mm)  |                                                                 |
    | Board Outline Tolerance    | ±0.2mm(Regular)             | ±0.2mm(Regular)     |                                                                 |
    | Confirm Production file    | No                          | No                  |                                                                 |
    | Remove Order Number        | No                          | No                  |                                                                 |
    | Flying Probe Test          | Fully Test                  | Fully Test          |                                                                 |
    | Gold Fingers               | No                          | No                  |                                                                 |
    | 30° finger chamfered       | No                          | No                  |                                                                 |
    | Castellated Holes          | No                          | No                  |                                                                 |
    | Press-Fit Hole             | No                          | No                  |                                                                 |
    | Edge Plating               | No                          | No                  |                                                                 |

4. Check PCB Assembly and fill the following options

    | Option                  | Low Volume      | High Volume     | Notes                                  |
    |-------------------------|-----------------|-----------------|----------------------------------------|
    | PCBA Type               | Economic        | **Standard**    | Economic saves on per-component cost   |
    | Assembly Side           | Both Sides      | Both Sides      | Lite only needs top side               |
    | PCBA Qty                | >= 2            | >= 100          |                                        |
    | Edge Rails/Fiducials    | Added by JLCPCB | Added by JLCPCB |                                        |
    | Confirm Parts Placement | No              | No              |                                        |

5. Press Continue or Next
6. Accept the board preview and press Next
7. Click "Add BOM File" and select your `BOM-XG_Mobile_Dock[_Lite].csv`
8. Click "Add CPL File" and select your `CPL-XG_Mobile_Dock[_Lite].csv`
9. Press "Process BOM & CPL" to continue (ignore any error about missing data)
10. Select the parts to pick by following the [Parts Guide](#parts-guide) section below.
11. Press Next. If you chose to skip some parts you will get a confirmation pop-up where you can press "Do Not Place".
12. In the Component Placements screen, you can confirm the parts orientation. If there are any mis-rotated parts, click them and rotate it to the correct orientation. (e.g. the pin headers.) Press Next to continue.
13. Confirm the order details and for "Product Description" select "Reserch\Education\DIY\Entertainment -> DIY - HS Code 902300"
14. Press "Save to Cart".

## Parts Guide

### Ordering Parts
Some parts may not be currently available in JLCPCB. This will be indicated with a red "xxx shortfall" text under the select column or they may appear on the bottom under unmatched parts. For each unavailable, part you need to manually order the part before you can order the assembled boards. It will typically take a few days to two weeks for the parts to arrive at JLCPCB.

1. Locate the part number. If this is a shortfall part, click the row and then select "Part Info". Copy the "MFR.Part #". Also note the number required in the "Qty" column. This is the minimum number you need to buy. It may not always be possible to buy the exact number, so you may end up with extra parts. If this is an expensive part and you are cost conscious, consider buying some number of PCBs to get the number of missing parts to be an exact amount you can buy for all the missing parts. This will increase your total cost but decrease your per-unit cost.
2. Order the Order Parts page: https://jlcpcb.com/user-center/smtPrivateLibrary/orderParts
3. You have a choice between JLCPCB Parts or Global Sourcing Parts. JLCPCB Parts is recommended for cost and ease but not all parts will be available.
4. First, try to search in JLCPCB Parts: paste the part number and search. If there are multiple results, click the one with the exact match on the part number. You will either see a box to type in the quantity or "Unavailable".
5. If the part is available, type in the Qty from step 1 or the minimum shown above the box depending on which number is larger. Then click "Add to My Part Lib" and you can stop here and proceed with the next part. Once you have all the parts added, you can skip to part 8.
6. If the part is unavailable, you will need to visit [Global Sourcing Parts](https://jlcpcb.com/user-center/smtPrivateLibrary/orderParts). Search for the part number found in step 1.
7. You will be presented with a list of distributors. Some of these will be mismatches, check the number in bold above each table of distributors. This must be an exact match with your part number. Note the "Distributor Part #" may not match but that is okay. Each distributor has a minimum order quantity and a unit price breakdown. The easiest way to find the cheapest distributor is to enter your desired Qty from step 1 into each text box and look at the "Ext. Price" which is automatically calculated. Once you select a distributor, add the component to your cart. You can now repeat these steps for other missing parts.
8. Once you have all the missing parts added, visit the [Parts Cart](https://jlcpcb.com/user-center/smtPrivateLibrary/orderParts) and checkout your "JLCPCB Parts" and/or your "Global Sourcing Parts".

### Cost Reduction
If you are ordering a low quantity (2-5 units) of PCBA boards and don't mind doing a bit of manual soldering with a heat gun or reflow oven, here's some tips you can follow.

* Choose Economic PCBA with Top Side for the lowest cost
* You can manually solder the capacitors on the bottom side
* Some "Extended" components incur a $3/component fee. For certain parts, this fee exceeds the cost of the components and so it may be cheaper to purchase these parts yourself and manually solder them. Some of these "Extended" components are actually "Preferred" and do not incur this cost.
* Some parts are unavailable at JLCPCB and will require pre-ordering or purchasing from Global Sourcing Parts which usually come with a handling fee and a minimum order quantity. It may make sense to purchase these yourself and manually solder them. This makes sense especially for the more expensive ICs where you can buy it cheaper from AliExpress than Global Sourcing Parts.
* If you do not need the USB hub, you can exclude much of the components supporting those and significantly reduce costs.
* If you do not need PWM fan support, you can exclude all of the Extended and Global Sourcing components for that.

## Additional Parts
* [XG Mobile connector cable](https://www.a-accessories.com/asus-connection-cable-62505-79153.htm)
* (Standard build only) SPI flasher is needed to program the firmware for the TI PD charger. You can also use a RaspberryPi with [Flashrom](https://wiki.flashrom.org/RaspberryPi) for this purpose.
* ST-LINK v2 is needed to program the STM32 MCU. You can buy a cheap clone from AliExpress for under $10 but note that the pinout listing printed on the dongle may be incorrect.

## Building Firmware
Check the [readme](../README.md) for directions on flashing.

### MCU
To build the MCU, you need an GCC ARM toolchain. The easiest way is to install [Homebrew](https://formulae.brew.sh) and run `brew install --cask gcc-arm-embedded`. Then you can run `make` in the `XG_Mobile_Dock_MCU` directory to build the MCU firmware.

Alternatively, if you want to build and develop the firmware, the easiest way is to install Visual Studio Code and the [STM32 extension](https://marketplace.visualstudio.com/items?itemName=bmd.stm32-for-vscode). Then you can open the project directory and run build and debug commands from the IDE.

### TI PD Controller (standard build)
1. Download the [TPS65982 Configuration Tool](https://www.ti.com/tool/TPS6598X-CONFIG) and install it.
2. Project -> Load Project and select "xg_mobile_dock_charger.pjt". This will load the settings for this board.
3. Binary -> Save Binary and check "Full Binary Image"
4. Click "Change File" and save it as "XG_Mobile_Dock_Charger.bin".
5. Press Ok and Ok to save the firmware image.
6. Some flashers like Flashrom requires the image to be the same size as the flash device. You can run `dd if=/dev/zero of=XG_Mobile_Dock_Charger.bin bs=1 seek=1048575 count=1 conv=notrunc` to ensure this.

## Building XG Mobile Station
The standard build is designed to fit inside the XG Station Pro enclosure and replace the original board.

1. Start by completely disassembling the XG Station Pro. Remove the top and side covers. Unscrew the right fan (the left fan does not block any other screws and does not have to be removed).
2. Remove the GPU if connected.
3. Unscrew the power adapter (3 screws) and lift the power adapter up and out. The connector may get caught on the fabric on the case so you might have to tilt it as you try to remove the board.
4. Remove both fan connectors and the LED connector.
5. Remove both PCIe power cables. Cut the zip ties which attaches the cable to the case.
5. Unscrew the main board (8 screws) and lift the board out of the case. The USB connector is inset to the case and may require tilting the board 45 degrees before lifting it out of the case.
6. Move the EMFI shielding from the original board's USB connectors and attach it to the new board.
7. Insert the new board into the case (tilting it so the USB connectors go into the inset first) and makes sure it sits flush on top of the screw holes.
8. Screw in the new board (6 screws, the 2 extra can be screwed in empty to prevent losing it).
9. Connect the PCIe power cables, the fan connectors, the LED cable, and the power adapter.
10. Connect the power adapter and screw it in (3 screws).
11. Connect the XG Mobile cable (3 connectors) and then carefully fold the cable 180 degrees so the XGM connector is pointing towards the PCIe slot. You should fold the cable towards you as to not impede the PCIe slot with the cable. Do not attempt to force bend the cable as it may be damaged. The XGM connector can be threaded through the PCIe slot closest to the outside edge.
12. Attach the ground pin to the board with the screw on the terminal.
13. Insert the GPU to the board and connect the PCIe power cable(s). You may need to cut the mounting bracket of the GPU in order to make space for the XGM cable.
