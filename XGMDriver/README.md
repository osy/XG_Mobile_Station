XGM Driver
==========
This UMDF driver provides a virtual HID device that will interface with ARMORY CRATE SE to trigger the activation pop-up.

See [Software.md](../Docs/Software.md) for more details.

## Installer components

* **Driver** (required): Fake XGM device to allow system to treat eGPU as an XG Mobile. The selected GPU model is part of the device's hardware ID (`Root\FakeXGMDevice_<model>`). Re-run the installer to change it.
* **XGM disconnect loop workaround** (optional, not selected by default): only select this if, after you attach the XGM, it connects and disconnects over and over and never finishes activating. If the eGPU activates normally, you do not need it.
  * Cause: Armoury Crate SE 3.1.3's hardware monitor plugin crashes the service's user session helper (in `amdadlx64.dll`) whenever a display adapter is added or removed. That helper also runs the eGPU switch, so activation is cut off, retried, and loops.
  * Fix: removes `MonitorPlugin.dll` from `C:\Program Files\ASUS\ARMOURY CRATE SE Service\ThrottleMonitorPlugin\Load.conf` and restarts the Armoury Crate service. The cost is Armoury Crate's hardware monitor.
  * Once applied, re-running the installer keeps it selected. Untick it on a re-run to remove it.
  * The original `Load.conf` is backed up in the install folder. Uninstalling, or re-running the installer with the component unticked, restores it, but only if `Load.conf` is still exactly what the installer wrote.
  * If an Armoury Crate update replaces `Load.conf`, the loop comes back. Re-running the installer re-applies the workaround and backs up the newer file instead. Uninstalling without re-running deletes the backup and keeps the newer `Load.conf`.
  * If `Load.conf` already lacks `MonitorPlugin.dll` and the installer did not remove it, the installer leaves it alone.

## Building

1. Install [Visual Studio 2022 Community Edition and WDK][1].
2. Install [NSIS][2].
3. Run `build.bat`. If prompted to enter a password to protect the generated private key, press "None".

[1]: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
[2]: https://nsis.sourceforge.io/Download
