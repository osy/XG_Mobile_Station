;//
;// Copyright (c) Microsoft Corporation.  All rights reserved.
;//

;//
;// General
;//
MessageId=60000 SymbolicName=MSG_USAGE
Language=English
%1 Usage: %1 [-r] [-m:\\<machine>] <command> [<arg>...]
For more information, type: %1 help
.
MessageId=60001 SymbolicName=MSG_FAILURE
Language=English
%1 failed.
.
MessageId=60002 SymbolicName=MSG_COMMAND_USAGE
Language=English
%1: Invalid use of %2.
For more information, type: %1 help %2
.

;//
;// HELP
;//
MessageId=60100 SymbolicName=MSG_HELP_LONG
Language=English
------------------------------------------------------------------------------
|          Devcon is a code sample intended for learning purposes.           |
|      Use PnpUtil for optimal functionality/compatiblity with this OS.      |
------------------------------------------------------------------------------
Device Console Help:
%1 [-r] [-m:\\<machine>] <command> [<arg>...]
-r           Reboots the system only when a restart or reboot is required.
-u           Unicode (UTF-16) output
<machine>    Specifies a remote computer.
<command>    Specifies a Devcon command (see command list below).
<arg>...     One or more arguments that modify a command.
For help with a specific command, type: %1 help <command>
.
MessageId=60101 SymbolicName=MSG_HELP_SHORT
Language=English
%1!-20s! Display Devcon help.
.
MessageId=60102 SymbolicName=MSG_HELP_OTHER
Language=English
Unknown command.
.

;//
;// INSTALL
;//
MessageId=61000 SymbolicName=MSG_INSTALL_LONG
Language=English
Devcon Install Command
Installs the specified device manually. Valid only on the local computer.
(To reboot when necesary, Include -r .)
%1 [-r] %2 <inf> <hwid> [<devid>]
<inf>        Specifies an INF file with installation information for the device.
<hwid>       Specifies a hardware ID for the device.
<devid>      Specifies a device ID for the device (optional).
-r           Reboots the system only when a restart or reboot is required.

Recommended Equivalent: devgen /add /bus ROOT /hardwareid <hwid>
                        pnputil /add-driver <inf> /install
.
MessageId=61001 SymbolicName=MSG_INSTALL_SHORT
Language=English
%1!-20s! Install a device manually.
.
MessageId=61002 SymbolicName=MSG_INSTALL_UPDATE
Language=English
Device node created. Install is complete when drivers are installed...
.

;//
;// UPDATE
;//
MessageId=61100 SymbolicName=MSG_UPDATE_LONG
Language=English
Devcon Update Command
Updates drivers for all devices with the specified hardware ID (<hwid>).
Valid only on the local computer. (To reboot when necesary, Include -r .)
%1 [-r] %2 <inf> <hwid>
-r           Reboots the system only when a restart or reboot is required.
<inf>        Specifies an INF file with installation information for the devices.
<hwid>       Specifies the hardware ID of the devices.
Recommended Equivalent: pnputil /add-driver <inf> /install
.
MessageId=61101 SymbolicName=MSG_UPDATE_SHORT
Language=English
%1!-20s! Update a device manually.
.
MessageId=61102 SymbolicName=MSG_UPDATE_INF
Language=English
Updating drivers for %1 from %2.
.
MessageId=61103 SymbolicName=MSG_UPDATE
Language=English
Updating drivers for %1.
.
MessageId=61104 SymbolicName=MSG_UPDATENI_LONG
Language=English
%1 [-r] %2 <inf> <hwid>
Update drivers for devices (Non Interactive).
This command will only work for local machine.
Specify -r to reboot automatically if needed.
<inf> is an INF to use to install the device.
All devices that match <hwid> are updated.
Unsigned installs will fail. No UI will be
presented.

Recommended Equivalent: pnputil /add-driver <inf> /install
.
MessageId=61105 SymbolicName=MSG_UPDATENI_SHORT
Language=English
%1!-20s! Manually update a device (non interactive).
.
MessageId=61106 SymbolicName=MSG_UPDATE_OK
Language=English
Drivers installed successfully.
.
;//
;// REMOVE
;//
MessageId=61200 SymbolicName=MSG_REMOVE_LONG
Language=English
Devcon Remove Command
Removes devices with the specified hardware or instance ID. Valid only on
the local computer. (To reboot when necesary, Include -r .)
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)

Recommended Equivalent: pnputil /remove-device <id>
.
MessageId=61201 SymbolicName=MSG_REMOVE_SHORT
Language=English
%1!-20s! Remove devices.
.
MessageId=61202 SymbolicName=MSG_REMOVE_TAIL_NONE
Language=English
No devices were removed.
.
MessageId=61203 SymbolicName=MSG_REMOVE_TAIL_REBOOT
Language=English
The %1!u! device(s) are ready to be removed. To remove the devices, reboot the system.
.
MessageId=61204 SymbolicName=MSG_REMOVE_TAIL
Language=English
%1!u! device(s) were removed.
.
MessageId=61205 SymbolicName=MSG_REMOVEALL_LONG
Language=English
Devcon Removeall Command
Removes devices with the specified hardware or instance ID, including devices
that are not currently attached. Valid only on the local computer.
(To reboot when necesary, Include -r .)
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
<class>      Specifies a device setup class.
Examples of <id>:
 *              - All devices
 ISAPNP\PNP0501 - Hardware ID
 *PNP*          - Hardware ID with wildcards  (* matches anything)
 @ISAPNP\*\*    - Instance ID with wildcards  (@ prefixes instance ID)
 '*PNP0501      - Hardware ID with apostrophe (' prefixes literal match - matches exactly as typed,
                                               including the asterisk.)

Recommended Equivalent: pnputil /remove-device <id> 
.
MessageId=61206 SymbolicName=MSG_REMOVEALL_SHORT
Language=English
%1!-20s! Remove devices, including those that are not currently attached.
.

