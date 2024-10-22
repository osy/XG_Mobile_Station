/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    User-mode Driver Framework 2

--*/

#pragma once

#include <windows.h>
#include <wdf.h>
#include <initguid.h>

#include "device.h"
#include "queue.h"
#include "trace.h"

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD XGMDriverEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP XGMDriverEvtDriverContextCleanup;
EVT_WDF_TIMER EvtTimerFunc;

EXTERN_C_END
