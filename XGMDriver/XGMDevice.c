/*++

Module Name:

    XGMDevice.c - Command handler for XGM device.

Abstract:

   This file contains the logic for XGM device.

Environment:

    User-mode Driver Framework 2

--*/

#include "Driver.h"
#include "XGMDevice.h"
#include "XGMDevice.tmh"

//
// This is the default report descriptor for the virtual Hid device returned
// by the mini driver in response to IOCTL_HID_GET_REPORT_DESCRIPTOR.
//
HID_REPORT_DESCRIPTOR       G_DefaultReportDescriptor[] =
{
    0x06, 0x31, 0xFF,  // Usage Page (Vendor Defined 0xFF31)
    0x09, 0x76,        // Usage (0x76)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x5A,        //   Report ID (90)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x05,        //   Report Count (5)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x00,        //   Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    0x06, 0x31, 0xFF,  // Usage Page (Vendor Defined 0xFF31)
    0x09, 0x80,        // Usage (0x80)
    0xA1, 0x01,        // Collection (Application)
    0x85, XGM_REPORT_ID,        //   Report ID (94)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x05,        //   Report Count (5)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x96, 0x2B, 0x01,  //   Report Count (299)
    0xB1, 0x00,        //   Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

#define MAX_HID_REPORT_SIZE sizeof(HIDINJECTOR_INPUT_REPORT)

//
// This is the default HID descriptor returned by the mini driver
// in response to IOCTL_HID_GET_DEVICE_DESCRIPTOR. The size
// of report descriptor is currently the size of G_DefaultReportDescriptor.
//

HID_DESCRIPTOR              G_DefaultHidDescriptor =
{
    0x09,   // length of HID descriptor
    0x21,   // descriptor type == HID  0x21
    0x0100, // hid spec release
    0x00,   // country code == Not Specified
    0x01,   // number of HID class descriptors
    {                                       //DescriptorList[0]
        0x22,                               //report descriptor type 0x22
        sizeof(G_DefaultReportDescriptor)   //total length of report descriptor
    }
};

typedef struct _XGM_MODEL
{
    PCWSTR Name;
    USHORT VendorID;
    USHORT DeviceID;
    USHORT SubsystemVendorID;
    USHORT SubsystemID;
} XGM_MODEL, * PXGM_MODEL;

//
// The installer creates the device with hardware ID
// XGM_HARDWARE_ID_PREFIX <Name> (see XGMDriver.inf). A device created with
// any other hardware ID gets G_Models[XGM_DEFAULT_MODEL].
//
static const XGM_MODEL G_Models[] =
{
    { L"GC31R", 0x10de, 0x249d, 0x1043, 0x218b },
    { L"GC31S", 0x10de, 0x249c, 0x1043, 0x217b },
    { L"GC32L", 0x1002, 0x73df, 0x1043, 0x21cb },
    { L"GC33Y", 0x10de, 0x2717, 0x1043, 0x21fb },
    { L"GC33Z", 0x10de, 0x27a0, 0x1043, 0x220b },
};

#define XGM_DEFAULT_MODEL       2 // GC32L
#define XGM_HARDWARE_ID_PREFIX  L"Root\\FakeXGMDevice_"

// pId is one string of a REG_MULTI_SZ and is not necessarily NUL-terminated
static BOOLEAN HardwareIdEquals(
    _In_reads_(idLength) PCWSTR pId,
    _In_ size_t idLength,
    _In_ PCWSTR pModelName
)
{
    size_t prefixLength = wcslen(XGM_HARDWARE_ID_PREFIX);
    size_t nameLength = wcslen(pModelName);

    return idLength == prefixLength + nameLength &&
        _wcsnicmp(pId, XGM_HARDWARE_ID_PREFIX, prefixLength) == 0 &&
        _wcsnicmp(pId + prefixLength, pModelName, nameLength) == 0;
}

static const XGM_MODEL* FindModelFromHardwareIds(
    _In_ WDFDEVICE Device
)
{
    NTSTATUS            status;
    WDFMEMORY           memory;
    PCWSTR              pId;
    size_t              size, remaining, idLength, i;
    const XGM_MODEL*    pModel = NULL;

    status = WdfDeviceAllocAndQueryProperty(Device, DevicePropertyHardwareID, PagedPool, WDF_NO_OBJECT_ATTRIBUTES, &memory);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "FindModelFromHardwareIds Error: WdfDeviceAllocAndQueryProperty failed %!STATUS!", status);
        return NULL;
    }

    // walk the REG_MULTI_SZ without reading past the end of the buffer
    pId = (PCWSTR)WdfMemoryGetBuffer(memory, &size);
    remaining = size / sizeof(WCHAR);
    while (pModel == NULL && remaining > 0 && pId[0] != L'\0')
    {
        idLength = wcsnlen(pId, remaining);
        for (i = 0; i < ARRAYSIZE(G_Models); i++)
        {
            if (HardwareIdEquals(pId, idLength, G_Models[i].Name))
            {
                pModel = &G_Models[i];
                break;
            }
        }
        if (idLength >= remaining)
        {
            break;
        }
        pId += idLength + 1;
        remaining -= idLength + 1;
    }
    WdfObjectDelete(memory);

    return pModel;
}

static void ParseSerial(
    _In_reads_opt_(inputLength) PCWSTR pInput,
    _In_ USHORT inputLength,
    _Out_writes_(8) PSTR pOutput
)
{
    memset(pOutput, '0', 8);
    if (pInput == NULL || inputLength == 0)
    {
        return;
    }
    // The serial field is 8 bytes and not NUL-terminated
    if (WideCharToMultiByte(CP_ACP, 0, pInput, min(inputLength, 8), pOutput, 8, NULL, NULL) == 0)
    {
        memset(pOutput, '0', 8);
    }
}

// Serial is optional; without it the serial reads as all zeros
static void ReadSerial(
    _In_ WDFDEVICE Device,
    _Out_writes_(8) PSTR pOutput
)
{
    NTSTATUS        status;
    WDFKEY          hKey;
    WDFSTRING       pWdfSerial;
    UNICODE_STRING  pSerial;
    DECLARE_CONST_UNICODE_STRING(pSerialKey, L"Serial");
    DECLARE_CONST_UNICODE_STRING(pEmptyString, L"");

    memset(pOutput, '0', 8);
    status = WdfStringCreate(&pEmptyString, WDF_NO_OBJECT_ATTRIBUTES, &pWdfSerial);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ReadSerial Error: WdfStringCreate failed %!STATUS!", status);
        return;
    }
    status = WdfDeviceOpenRegistryKey(Device, PLUGPLAY_REGKEY_DEVICE, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &hKey);
    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryString(hKey, &pSerialKey, pWdfSerial);
        if (NT_SUCCESS(status))
        {
            WdfStringGetUnicodeString(pWdfSerial, &pSerial);
            ParseSerial(pSerial.Buffer, (USHORT)(pSerial.Length / sizeof(WCHAR)), pOutput);
        }
        else if (status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ReadSerial Error: WdfRegistryQueryString failed %!STATUS!", status);
        }
        WdfRegistryClose(hKey);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ReadSerial Error: WdfDeviceOpenRegistryKey failed %!STATUS!", status);
    }
    WdfObjectDelete(pWdfSerial);
}

VOID
XGMDriver_LoadConfiguration(
    _Inout_ PDEVICE_CONTEXT DeviceContext
)
{
    const XGM_MODEL*    pModel;

    pModel = FindModelFromHardwareIds(DeviceContext->Device);
    if (pModel == NULL)
    {
        pModel = &G_Models[XGM_DEFAULT_MODEL];
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_DRIVER, "XGMDriver_LoadConfiguration: no model hardware ID, using default model %ws", pModel->Name);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "XGMDriver_LoadConfiguration: model %ws", pModel->Name);
    }
    DeviceContext->EgpuVendorID = pModel->VendorID;
    DeviceContext->EgpuDeviceID = pModel->DeviceID;
    DeviceContext->EgpuSubsystemVendorID = pModel->SubsystemVendorID;
    DeviceContext->EgpuSubsystemID = pModel->SubsystemID;
    ReadSerial(DeviceContext->Device, DeviceContext->EgpuSerial);
}

static void StoreBigEndian16(
    _Out_writes_(2) PUCHAR pOutput,
    _In_ USHORT value
)
{
    pOutput[0] = (UCHAR)(value >> 8);
    pOutput[1] = (UCHAR)(value & 0xFF);
}

static NTSTATUS
XGMDriver_Get_EGPUID(
    _In_ PDEVICE_CONTEXT  DeviceContext,
    _In_ PUCHAR           InputBuffer,
    _Out_ PHID_XGM_REPORT OutputReportBuffer,
    _Inout_ PULONG        OutputReportBufferLength
)
{
    if (*OutputReportBufferLength < 24)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (InputBuffer[0] != 3)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_Get_EGPUID Error: unknown argument %Xh", InputBuffer[0]);
        return STATUS_INVALID_PARAMETER;
    }

    OutputReportBuffer->ReportID = XGM_REPORT_ID;
    OutputReportBuffer->Command = XGM_CMD_GET_EGPUID;
    OutputReportBuffer->Data[0] = 0x03;
    // big endian VID, PID, then the 32-bit subsystem ID (SSID, SVID)
    StoreBigEndian16(&OutputReportBuffer->Data[1], DeviceContext->EgpuVendorID);
    StoreBigEndian16(&OutputReportBuffer->Data[3], DeviceContext->EgpuDeviceID);
    StoreBigEndian16(&OutputReportBuffer->Data[5], DeviceContext->EgpuSubsystemID);
    StoreBigEndian16(&OutputReportBuffer->Data[7], DeviceContext->EgpuSubsystemVendorID);
    memcpy(&OutputReportBuffer->Data[9], DeviceContext->EgpuSerial, sizeof(DeviceContext->EgpuSerial));
    OutputReportBuffer->Data[17] = 'G';
    OutputReportBuffer->Data[18] = 'E';
    OutputReportBuffer->Data[19] = 'N';
    OutputReportBuffer->Data[20] = ' ';
    OutputReportBuffer->Data[21] = '4';
    *OutputReportBufferLength = 24;

    return STATUS_SUCCESS;
}

NTSTATUS
XGMDriver_ProcessCommand(
    _In_ PDEVICE_CONTEXT  DeviceContext,
    _In_ UCHAR            Command,
    _In_ PUCHAR           InputBuffer,
    _Out_ PHID_XGM_REPORT OutputReportBuffer,
    _Inout_ PULONG        OutputReportBufferLength
)
{
    NTSTATUS        status = STATUS_SUCCESS;

    switch (Command)
    {
    case XGM_CMD_GET_EGPUID:
    {
        status = XGMDriver_Get_EGPUID(DeviceContext, InputBuffer, OutputReportBuffer, OutputReportBufferLength);
        break;
    }
    default:
    {
        status = STATUS_INVALID_PARAMETER;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_ProcessCommand Error: command %Xh not handled", Command);
    }
    }

    return status;
}