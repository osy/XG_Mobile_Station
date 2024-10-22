XGM Driver
==========
This UMDF driver provides a virtual HID device that will interface with ARMORY CRATE SE to trigger the activation pop-up.

See [Software.md](../Docs/Software.md) for more details.

## Building

1. Install [Visual Studio 2022 Community Edition and WDK][1].
2. Install [NSIS][2].
3. Run `build.bat`. If prompted to enter a password to protect the generated private key, press "None".

[1]: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
[2]: https://nsis.sourceforge.io/Download
