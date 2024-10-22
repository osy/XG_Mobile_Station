/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devcon.cpp

Abstract:

    Device Console
    command-line interface for managing devices

--*/

#include "devinstall.h"

struct GenericContext {
    DWORD count;
    DWORD control;
    BOOL  reboot;
    LPCTSTR strSuccess;
    LPCTSTR strReboot;
    LPCTSTR strFail;
};

#define FIND_DEVICE         0x00000001 // display device
#define FIND_STATUS         0x00000002 // display status of device
#define FIND_RESOURCES      0x00000004 // display resources of device
#define FIND_DRIVERFILES    0x00000008 // display drivers used by device
#define FIND_HWIDS          0x00000010 // display hw/compat id's used by device
#define FIND_DRIVERNODES    0x00000020 // display driver nodes for a device.
#define FIND_CLASS          0x00000040 // display device's setup class
#define FIND_STACK          0x00000080 // display device's driver-stack

struct SetHwidContext {
    int argc_right;
    LPTSTR* argv_right;
    DWORD prop;
    int skipped;
    int modified;
};

int cmdHelp(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    HELP command
    allow HELP or HELP <command>

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine (ignored)
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    DWORD helptext = 0;
    int dispIndex;
    LPCTSTR cmd = NULL;
    BOOL unknown = FALSE;

    UNREFERENCED_PARAMETER(Machine);
    UNREFERENCED_PARAMETER(Flags);

    if (argc) {
        //
        // user passed in a command for help on... long help
        //
        for (dispIndex = 0; DispatchTable[dispIndex].cmd; dispIndex++) {
            if (_tcsicmp(argv[0], DispatchTable[dispIndex].cmd) == 0) {
                cmd = DispatchTable[dispIndex].cmd;
                helptext = DispatchTable[dispIndex].longHelp;
                break;
            }
        }
        if (!cmd) {
            unknown = TRUE;
            cmd = argv[0];
        }
    }

    if (helptext) {
        //
        // long help
        //
        FormatToStream(stdout, helptext, BaseName, cmd);
    }
    else {
        //
        // help help
        //
        FormatToStream(stdout, unknown ? MSG_HELP_OTHER : MSG_HELP_LONG, BaseName, cmd);
        //
        // enumerate through each command and display short help for each
        //
        _fputts(TEXT("\n"), stdout);
        for (dispIndex = 0; DispatchTable[dispIndex].cmd; dispIndex++) {
            if (DispatchTable[dispIndex].shortHelp) {
                FormatToStream(stdout, DispatchTable[dispIndex].shortHelp, DispatchTable[dispIndex].cmd);
            }
        }
    }
    return EXIT_OK;
}


int cmdUpdate(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:
    UPDATE
    update driver for existing device(s)

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HMODULE newdevMod = NULL;
    int failcode = EXIT_FAIL;
    UpdateDriverForPlugAndPlayDevicesProto UpdateFn;
    BOOL reboot = FALSE;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;
    DWORD flags = 0;
    DWORD res;
    TCHAR InfPath[MAX_PATH];

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);

    if (Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if (argc < 2) {
        //
        // at least HWID required
        //
        return EXIT_USAGE;
    }
    inf = argv[0];
    if (!inf[0]) {
        return EXIT_USAGE;
    }

    hwid = argv[1];
    if (!hwid[0]) {
        return EXIT_USAGE;
    }
    //
    // Inf must be a full pathname
    //
    res = GetFullPathName(inf, MAX_PATH, InfPath, NULL);
    if ((res >= MAX_PATH) || (res == 0)) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }
    if (GetFileAttributes(InfPath) == (DWORD)(-1)) {
        //
        // inf doesn't exist
        //
        return EXIT_FAIL;
    }
    inf = InfPath;
    flags |= INSTALLFLAG_FORCE;

    //
    // make use of UpdateDriverForPlugAndPlayDevices
    //
    newdevMod = LoadLibrary(TEXT("newdev.dll"));
    if (!newdevMod) {
        goto final;
    }
    UpdateFn = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
    if (!UpdateFn)
    {
        goto final;
    }

    FormatToStream(stdout, inf ? MSG_UPDATE_INF : MSG_UPDATE, hwid, inf);

    if (!UpdateFn(NULL, hwid, inf, flags, &reboot)) {
        goto final;
    }

    FormatToStream(stdout, MSG_UPDATE_OK);

    failcode = reboot ? EXIT_REBOOT : EXIT_OK;

    final :

        if (newdevMod) {
            FreeLibrary(newdevMod);
        }

    return failcode;
}

int cmdInstall(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    CREATE
    Creates a root enumerated devnode and installs drivers on it

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    TCHAR hwIdList[LINE_LEN + 4];
    TCHAR InfPath[MAX_PATH];
    int failcode = EXIT_FAIL;
    LPCTSTR devid = NULL;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;

    if (Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if (argc < 2) {
        //
        // at least HWID required
        //
        return EXIT_USAGE;
    }
    inf = argv[0];
    if (!inf[0]) {
        return EXIT_USAGE;
    }

    hwid = argv[1];
    if (!hwid[0]) {
        return EXIT_USAGE;
    }

    if (argc > 2) {
        devid = argv[2];
        if (!devid[0]) {
            return EXIT_USAGE;
        }
    }

    //
    // Inf must be a full pathname
    //
    if (GetFullPathName(inf, MAX_PATH, InfPath, NULL) >= MAX_PATH) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }

    //
    // List of hardware ID's must be double zero-terminated
    //
    ZeroMemory(hwIdList, sizeof(hwIdList));
    if (FAILED(StringCchCopy(hwIdList, LINE_LEN, hwid))) {
        goto final;
    }

    //
    // Use the INF File to extract the Class GUID.
    //
    if (!SetupDiGetINFClass(InfPath, &ClassGUID, ClassName, sizeof(ClassName) / sizeof(ClassName[0]), 0))
    {
        goto final;
    }

    if (!devid) {
        devid = ClassName;
    }

    //
    // Create the container for the to-be-created Device Information Element.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID, 0);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        goto final;
    }

    //
    // Now create the element.
    // Use the Class GUID and Name from the INF file.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        devid,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        goto final;
    }

    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)hwIdList,
        ((DWORD)_tcslen(hwIdList) + 1 + 1) * sizeof(TCHAR)))
    {
        goto final;
    }

    //
    // Transform the registry element into an actual devnode
    // in the PnP HW tree.
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
        goto final;
    }

    FormatToStream(stdout, MSG_INSTALL_UPDATE);
    //
    // update the driver for the device we just created
    //
    failcode = cmdUpdate(BaseName, Machine, Flags, argc, argv);

    final:

    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    return failcode;
}

int RemoveCallback(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Index, _In_ LPVOID Context)
/*++

Routine Description:

    Callback for use by Remove
    Invokes DIF_REMOVE
    uses SetupDiCallClassInstaller so cannot be done for remote devices
    Don't use CM_xxx API's, they bypass class/co-installers and this is bad.

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    SP_REMOVEDEVICE_PARAMS rmdParams;
    GenericContext* pControlContext = (GenericContext*)Context;
    SP_DEVINSTALL_PARAMS devParams;
    LPCTSTR action = NULL;
    //
    // need hardware ID before trying to remove, as we wont have it after
    //
    TCHAR devID[MAX_DEVICE_ID_LEN];
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    UNREFERENCED_PARAMETER(Index);

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if ((!SetupDiGetDeviceInfoListDetail(Devs, &devInfoListDetail)) ||
        (CM_Get_Device_ID_Ex(DevInfo->DevInst, devID, MAX_DEVICE_ID_LEN, 0, devInfoListDetail.RemoteMachineHandle) != CR_SUCCESS)) {
        //
        // skip this
        //
        return EXIT_OK;
    }

    rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    rmdParams.HwProfile = 0;
    if (!SetupDiSetClassInstallParams(Devs, DevInfo, &rmdParams.ClassInstallHeader, sizeof(rmdParams)) ||
        !SetupDiCallClassInstaller(DIF_REMOVE, Devs, DevInfo)) {
        //
        // failed to invoke DIF_REMOVE
        //
        action = pControlContext->strFail;
    }
    else {
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        if (SetupDiGetDeviceInstallParams(Devs, DevInfo, &devParams) && (devParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
            //
            // reboot required
            //
            action = pControlContext->strReboot;
            pControlContext->reboot = TRUE;
        }
        else {
            //
            // appears to have succeeded
            //
            action = pControlContext->strSuccess;
        }
        pControlContext->count++;
    }
    _tprintf(TEXT("%-60s: %s\n"), devID, action);

    return EXIT_OK;
}

int cmdRemove(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

    REMOVE
    remove devices

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    TCHAR strRemove[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(Flags);

    if (!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if (Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if (!LoadString(NULL, IDS_REMOVED, strRemove, ARRAYSIZE(strRemove))) {
        return EXIT_FAIL;
    }
    if (!LoadString(NULL, IDS_REMOVED_REBOOT, strReboot, ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if (!LoadString(NULL, IDS_REMOVE_FAILED, strFail, ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strRemove;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName, Machine, DIGCF_PRESENT, argc, argv, RemoveCallback, &context);

    if (failcode == EXIT_OK) {

        if (!context.count) {
            FormatToStream(stdout, MSG_REMOVE_TAIL_NONE);
        }
        else if (!context.reboot) {
            FormatToStream(stdout, MSG_REMOVE_TAIL, context.count);
        }
        else {
            FormatToStream(stdout, MSG_REMOVE_TAIL_REBOOT, context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdRemoveAll(_In_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_ int argc, _In_reads_(argc) PTSTR argv[])
/*++

Routine Description:

REMOVEALL
remove devices
like remove, but also remove not-present devices

Arguments:

BaseName  - name of executable
Machine   - machine name, must be NULL
argc/argv - remaining parameters

Return Value:

EXIT_xxxx

--*/
{
    GenericContext context;
    TCHAR strRemove[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    UNREFERENCED_PARAMETER(Flags);

    if (!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if (Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if (!LoadString(NULL, IDS_REMOVED, strRemove, ARRAYSIZE(strRemove))) {
        return EXIT_FAIL;
    }
    if (!LoadString(NULL, IDS_REMOVED_REBOOT, strReboot, ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if (!LoadString(NULL, IDS_REMOVE_FAILED, strFail, ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strRemove;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName, Machine, 0, argc, argv, RemoveCallback, &context);

    if (failcode == EXIT_OK) {

        if (!context.count) {
            FormatToStream(stdout, MSG_REMOVE_TAIL_NONE);
        }
        else if (!context.reboot) {
            FormatToStream(stdout, MSG_REMOVE_TAIL, context.count);
        }
        else {
            FormatToStream(stdout, MSG_REMOVE_TAIL_REBOOT, context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}


DispatchEntry DispatchTable[] = {
    { TEXT("install"),      cmdInstall,     MSG_INSTALL_SHORT,     MSG_INSTALL_LONG },
    { TEXT("remove"),       cmdRemove,      MSG_REMOVE_SHORT,      MSG_REMOVE_LONG },
    { TEXT("removeall"),    cmdRemoveAll,   MSG_REMOVEALL_SHORT,   MSG_REMOVEALL_LONG },
    { TEXT("update"),       cmdUpdate,      MSG_UPDATE_SHORT,      MSG_UPDATE_LONG },
    { TEXT("?"),            cmdHelp,        0,                     0 },
    { NULL,NULL }
};



