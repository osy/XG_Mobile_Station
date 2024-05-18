Software support for XG Mobile is implemented in Armoury Crate and ASUS Hotkeys driver. This document notes some of the findings from reverse engineering key pieces of the software found in `C:\Program Files\ASUS\ARMOURY CRATE SE Service\GPUSwitchPlugin`.

## GPUSwitchPlugin.dll
This is the main executable loaded by Armoury Crate to handle eGPU events. ASUS Hotkeys is typically used in ASUS systems to handle special keyboard keys and function keys. You can read the [Linux driver][1] to see examples of the kinds of events that are handled by AKS. On the ROG Ally, some special keys are used to handle eGPU events. These are purely virtual key codes that do not correspond to a physical key event.

| Key Code | Name                | Description                                  |
|----------|---------------------|----------------------------------------------|
| 0xB9     | Connect Change      | eGPU connector is plugged/unplugged          |
| 0xBA     | Switch-Lock Change  | Connector lock is engaged (and ACKed by MCU) |
| 0xBB     | External Power Loss | AC power is lost on the eGPU                 |
| 0xBC     | ?                   | Seen when GPU eject request timed out        |
| 0xC2     | Restart Required    | Prompt the user to restart to enable eGPU    |

These key events are emitted from ACPI handlers for the EC ([reverse engineered details can be found here](ACPI_Annotated.asl)). When `0xB9` is received, `ExternalGPUConfigHelper2.dll` is used to get the EGPUID which is parsed to the following registry entries (all this information is hard coded based on the EGPUID):

| Key                                                 | Value         | Data Type | Data                                                                                                                                                                                                                               |
|-----------------------------------------------------|---------------|-----------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| HKLM\SOFTWARE\ASUS\eGPU                             | Current4Part  | String    | `10de-249d-218b-1043`, `10de-249c-217b-1043`, `1002-73DF-21CB-1043`, `10de-2717-21fb-1043`, `10de-27a0-220b-1043`                                                                                                                  |
| HKLM\SOFTWARE\ASUS\eGPU                             | XGMobileModel | String    | `GC31R`, `GC31S`, `GC32L`, `GC33Y`, `GC33Z`                                                                                                                                                                                        |
| HKLM\SOFTWARE\ASUS\eGPU                             | EVendor       | DWORD     | `0x0` = NVIDIA, `0x100` = AMD, `0x200` = Intel                                                                                                                                                                                     |
| HKLM\SOFTWARE\ASUS\eGPU                             | DockingGen    | DWORD     | PCIe generation: `3` or `4`                                                                                                                                                                                                        |
| HKLM\SOFTWARE\ASUS\eGPU                             | SkipRLS       | DWORD     | `0` or `1` if VBIOS update is needed                                                                                                                                                                                               |
| HKLM\SOFTWARE\ASUS\ARMOURY CRATE Service\GPU Switch | ErrorReason   | String    | `NoPower` = any error trying to read EGPUID, `OldVBIOS` = eGPU is PCIe Gen 4 and host is Gen 4, `UpgradingVBIOS` = eGPU is PCIe Gen 4 and host is Gen 3, `OldBIOS` = if ACPI returns no support for this GPU vendor, `` = No error |

Note that if the XG Mobile is `GC31R`, `GC31S`, or `GC32L` and the host model is NOT `GV301Q`, then both the VBIOS and BIOS must be updated to force PCIe Gen 3 (according to the comment, PCIe Gen 4 is not stable).

The result of `HKLM\SOFTWARE\ASUS\ARMOURY CRATE Service\GPU Switch\ErrorReason` is written to `C:\ProgramData\ASUS\ARMOURY CRATE Service\GPUSwitch\settings.ini` under `Notify`.

When `0xBA` is received and both the connect status and lock status are 1, then `GPUTrayIcon.exe` is sent a message which writes `Notify=Verify` to `settings.ini`.

`GPUSwitchDialog.exe` is launched and depending on the value of `Notify` in `settings.ini`, will show a GUI alert. When in Verify and the user starts the connection process, a file `action.ini` is written with `GPUStatus=1` and in `settings.ini`, it writes `Notify=SwitchGPU` and `GPUSwitchPlugin.dll` will handle it.

Upon receipt of `SwitchGPU`, it opens a handle to `\\.\ATKACPI` which is used to call the `WMNB` [ACPI method](ACPI_Annotated.asl). Using the `DEVS` command with argument `0x00090019`, it will activate the eGPU. The ACPI method can return 0 for error, 1 for success, or 2 for reboot required. If no PCIe device is detected (VID/PID read returns 0xFFFFFFFF), then 2 is returned and the user is prompted to restart.

After restarting, if no PCIe device is found still, an error message is displayed indicating that the eGPU was improperly disconnected.

## ExternalGPUConfigHelper2.dll
This is managed C# code that implements a userland HID driver for communicating with the XG Mobile's eGPU. A HID device with usage page `0xFF31` and usage `0x80` is enumerated and if found will set a feature report with the data: `^ASUS Tech.Inc.\0`. If the report was set successfully, the device is valid and the driver can be used to do various things including reading the EGPUID, setting fan curves, setting other config, etc. Each command is implemented by setting a feature report followed by getting a feature report.

### GetEGPUID
#### Request
`5E CE 03`

### Response
| Offset | Size | Data                       |
|--------|------|----------------------------|
| 0      | 3    | `5E CE 03`                 |
| 3      | 2    | ntohs(VID)                 |
| 5      | 2    | ntohs(PID)                 |
| 7      | 4    | ntohl(subsystem id)        |
| 11     | 8    | ?                          |
| 19     | 3    | `GEN`                      |
| 22     | 1    | ?                          |
| 23     | 1    | '1', '2', '3', '4', or '5' |

[1]: https://github.com/torvalds/linux/blob/master/drivers/platform/x86/acer-wmi.c
