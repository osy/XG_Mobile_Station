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
#include <WinSock2.h>

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

static void ParseModel(
    _In_ PCWSTR pModel,
    _Out_ PUINT16 pVid,
    _Out_ PUINT16 pPid,
    _Out_ PUINT16 pSvid,
    _Out_ PUINT16 pSsid
)
{
    if (_wcsicmp(pModel, L"GC31R") == 0)
    {
        *pVid = ntohs(0x10de);
        *pPid = ntohs(0x249d);
        *pSvid = ntohs(0x1043);
        *pSsid = ntohs(0x218b);
    }
    else if (_wcsicmp(pModel, L"GC31S") == 0)
    {
        *pVid = ntohs(0x10de);
        *pPid = ntohs(0x249c);
        *pSvid = ntohs(0x1043);
        *pSsid = ntohs(0x217b);
    }
    else if (_wcsicmp(pModel, L"GC33Y") == 0)
    {
        *pVid = ntohs(0x10de);
        *pPid = ntohs(0x2717);
        *pSvid = ntohs(0x1043);
        *pSsid = ntohs(0x21fb);
    }
    else if (_wcsicmp(pModel, L"GC33Z") == 0)
    {
        *pVid = ntohs(0x10de);
        *pPid = ntohs(0x27a0);
        *pSvid = ntohs(0x1043);
        *pSsid = ntohs(0x220b);
    }
    else // if (_wcsicmp(pModel, L"GC32L") == 0)
    {
        *pVid = ntohs(0x1002);
        *pPid = ntohs(0x73df);
        *pSvid = ntohs(0x1043);
        *pSsid = ntohs(0x21cb);
    }
}

static void ParseSerial(
    _In_ PCWSTR pInput,
    _In_ USHORT inputLength,
    _Out_ PSTR pOutput
)
{
    memset(pOutput, '0', 8);
    WideCharToMultiByte(CP_ACP, 0, pInput, inputLength, pOutput, 9, NULL, NULL);
}

static NTSTATUS
XGMDriver_Get_EGPUID(
    _In_ PUCHAR           InputBuffer,
    _Out_ PHID_XGM_REPORT OutputReportBuffer,
    _Inout_ PULONG        OutputReportBufferLength
)
{
    NTSTATUS        status = STATUS_SUCCESS;
    WDFKEY          hKey;
    WDFSTRING       pWdfModel, pWdfSerial;
    UNICODE_STRING  pModel, pSerial;
    DECLARE_CONST_UNICODE_STRING(pModelKey, L"Model");
    DECLARE_CONST_UNICODE_STRING(pSerialKey, L"Serial");
    DECLARE_CONST_UNICODE_STRING(pEmptyString, L"");

    if (*OutputReportBufferLength < 24)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (InputBuffer[0] != 3)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_Get_EGPUID Error: unknown argument %Xh", InputBuffer[0]);
        return STATUS_INVALID_PARAMETER;
    }

    // get data from registry
    status = WdfStringCreate(&pEmptyString, WDF_NO_OBJECT_ATTRIBUTES, &pWdfModel);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_Get_EGPUID Error: WdfStringCreate failed %!STATUS!", status);
        return status;
    }
    status = WdfStringCreate(&pEmptyString, WDF_NO_OBJECT_ATTRIBUTES, &pWdfSerial);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_Get_EGPUID Error: WdfStringCreate failed %!STATUS!", status);
        WdfObjectDelete(pWdfModel);
        return status;
    }
    status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(), KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &hKey);
    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryString(hKey, &pModelKey, pWdfModel);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_Get_EGPUID Error: WdfRegistryQueryString failed %!STATUS!", status);
        }
        status = WdfRegistryQueryString(hKey, &pSerialKey, pWdfSerial);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_Get_EGPUID Error: WdfRegistryQueryString failed %!STATUS!", status);
        }
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "XGMDriver_Get_EGPUID Error: WdfDriverOpenParametersRegistryKey failed %!STATUS!", status);
    }
    status = STATUS_SUCCESS; // ignore error
    WdfStringGetUnicodeString(pWdfSerial, &pSerial);
    WdfStringGetUnicodeString(pWdfModel, &pModel);
    OutputReportBuffer->ReportID = XGM_REPORT_ID;
    OutputReportBuffer->Command = XGM_CMD_GET_EGPUID;
    OutputReportBuffer->Data[0] = 0x03;
    ParseModel(pModel.Buffer, (PUINT16)&OutputReportBuffer->Data[1], (PUINT16)&OutputReportBuffer->Data[3], (PUINT16)&OutputReportBuffer->Data[7], (PUINT16)&OutputReportBuffer->Data[5]);
    ParseSerial(pSerial.Buffer, pSerial.Length, (PSTR)&OutputReportBuffer->Data[9]);
    OutputReportBuffer->Data[17] = 'G';
    OutputReportBuffer->Data[18] = 'E';
    OutputReportBuffer->Data[19] = 'N';
    OutputReportBuffer->Data[20] = ' ';
    OutputReportBuffer->Data[21] = '4';
    WdfObjectDelete(pWdfSerial);
    WdfObjectDelete(pWdfModel);
    *OutputReportBufferLength = 24;

    return status;
}

NTSTATUS
XGMDriver_ProcessCommand(
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
        status = XGMDriver_Get_EGPUID(InputBuffer, OutputReportBuffer, OutputReportBufferLength);
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