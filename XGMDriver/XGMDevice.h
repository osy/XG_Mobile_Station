/*++

Module Name:

    XGMDevice.h

Abstract:

    This file contains the definitions specific to the XGM.

Environment:

    User-mode Driver Framework 2

--*/

#pragma once

#include <windows.h>

typedef UCHAR HID_REPORT_DESCRIPTOR, * PHID_REPORT_DESCRIPTOR;

EXTERN_C_START

//
// These are the device attributes returned by the mini driver in response
// to IOCTL_HID_GET_DEVICE_ATTRIBUTES.
//
#define XGMDEVICE_PID             0x1a9a
#define XGMDEVICE_VID             0x0b05
#define XGMDEVICE_VERSION         0x0001

#define MAXIMUM_STRING_LENGTH           (126 * sizeof(WCHAR))
#define XGMDEVICE_MANUFACTURER_STRING    L"osy"  
#define XGMDEVICE_PRODUCT_STRING         L"Fake XGM Device"  
#define XGMDEVICE_SERIAL_NUMBER_STRING   L"0123456789"  
#define XGMDEVICE_DEVICE_STRING          L"Fake XGM Device"  
#define XGMDEVICE_DEVICE_STRING_INDEX    5

#define XGM_REPORT_ID (0x5E)
#define XGM_CMD_GET_EGPUID (0xCE)

#include <pshpack1.h>
typedef struct _HID_XGM_REPORT
{
    UCHAR ReportID;
    UCHAR Command;
    UCHAR Data[298];
} HID_XGM_REPORT, * PHID_XGM_REPORT;
#include <poppack.h>

extern HID_REPORT_DESCRIPTOR G_DefaultReportDescriptor[];
extern HID_DESCRIPTOR G_DefaultHidDescriptor;

NTSTATUS
XGMDriver_ProcessCommand(
    _In_ UCHAR            Command,
    _In_ PUCHAR           InputBuffer,
    _Out_ PHID_XGM_REPORT OutputReportBuffer,
    _Inout_ PULONG        OutputReportBufferLength
);

EXTERN_C_END
