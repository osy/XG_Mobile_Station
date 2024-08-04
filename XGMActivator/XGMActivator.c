#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

static LPCWSTR gModel = L"GC31R";
static LPCWSTR g4Part = L"10de-249d-218b-1043";
static DWORD   gEVendor = 0x0;
static SERVICE_STATUS gServiceStatus;
static SERVICE_STATUS_HANDLE gServiceStatusHandle;
static HANDLE gStopEvent;

static LSTATUS SetRegString(HKEY hKey, LPCWSTR lpValue, LPCWSTR lpData)
{
    DWORD dwDataSize;
    
    dwDataSize = (DWORD)(wcslen(lpData) + 1) * sizeof(WCHAR);
    return RegSetValueEx(hKey, lpValue, 0, REG_SZ, (const BYTE *)lpData, dwDataSize);
}

static LSTATUS SetRegDword(HKEY hKey, LPCWSTR lpValue, DWORD dwData)
{
    return RegSetValueEx(hKey, lpValue, 0, REG_DWORD, (const BYTE *)&dwData, sizeof(dwData));
}

static int SetupModel(LPCWSTR lpModel)
{
    if (_wcsicmp(lpModel, L"GC31R") == 0)
    {
        gModel = L"GC31R";
        g4Part = L"10de-249d-218b-1043";
        gEVendor = 0;
    }
    else if (_wcsicmp(lpModel, L"GC31S") == 0)
    {
        gModel = L"GC31S";
        g4Part = L"10de-249c-217b-1043";
        gEVendor = 0;
    }
    else if (_wcsicmp(lpModel, L"GC32L") == 0)
    {
        gModel = L"GC32L";
        g4Part = L"1002-73DF-21CB-1043";
        gEVendor = 0x100;
    }
    else if (_wcsicmp(lpModel, L"GC33Y") == 0)
    {
        gModel = L"GC33Y";
        g4Part = L"10de-2717-21fb-1043";
        gEVendor = 0;
    }
    else if (_wcsicmp(lpModel, L"GC33Z") == 0)
    {
        gModel = L"GC33Z";
        g4Part = L"10de-27a0-220b-1043";
        gEVendor = 0;
    }
    else
    {
        return 0;
    }
    return 1;
}

static void WriteModelToRegistry(void)
{
    LONG   lErrorCode;
    HKEY   hKey;

    lErrorCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASUS\\eGPU", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegCreateKeyEx (%d).\n"), lErrorCode);
        return;
    }

    lErrorCode = SetRegString(hKey, L"Current4Part", g4Part);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegSetValueEx(Current4Part) (%d).\n"), lErrorCode);
    }
    lErrorCode = SetRegString(hKey, L"XGMobileModel", gModel);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegSetValueEx(XGMobileModel) (%d).\n"), lErrorCode);
    }
    lErrorCode = SetRegDword(hKey, L"EVendor", gEVendor);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegSetValueEx(EVendor) (%d).\n"), lErrorCode);
    }
    lErrorCode = SetRegDword(hKey, L"DockingGen", 0x3);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegSetValueEx(DockingGen) (%d).\n"), lErrorCode);
    }
    lErrorCode = SetRegDword(hKey, L"SkipRLS", 0);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegSetValueEx(SkipRLS) (%d).\n"), lErrorCode);
    }
    RegCloseKey(hKey);
}

static void ListenGPUSwitch(void)
{
    DWORD  dwFilter = REG_NOTIFY_CHANGE_NAME |
        REG_NOTIFY_CHANGE_LAST_SET;

    HANDLE hEvents[2];
    HKEY   hKey;
    LONG   lErrorCode;
    DWORD  hDataSize;
    DWORD  dwEvent;

    // Open a key.
    lErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASUS\\ARMOURY CRATE Service\\GPU Switch", 0, KEY_NOTIFY | KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegOpenKeyEx (%d).\n"), lErrorCode);
        return;
    }

    // Create an event.
    hEvents[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvents[0] == NULL)
    {
        _tprintf(TEXT("Error in CreateEvent (%d).\n"), GetLastError());
        return;
    }
    hEvents[1] = gStopEvent;

    while (1)
    {
        // Watch the registry key for a change of value.
        lErrorCode = RegNotifyChangeKeyValue(hKey,
            TRUE,
            dwFilter,
            hEvents[0],
            TRUE);
        if (lErrorCode != ERROR_SUCCESS)
        {
            _tprintf(TEXT("Error in RegNotifyChangeKeyValue (%d).\n"), lErrorCode);
            return;
        }

        // Wait for an event to occur.
        _tprintf(TEXT("Waiting for a change in GPU Switch...\n"));
        dwEvent = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (dwEvent == WAIT_OBJECT_0 + 1)
        {
            // stop event
            break;
        }
        else if (dwEvent == WAIT_FAILED)
        {
            _tprintf(TEXT("Error in WaitForSingleObject (%d).\n"), GetLastError());
            return;
        }
        else _tprintf(TEXT("\nChange has occurred.\n"));

        lErrorCode = RegQueryValueEx(hKey,
            L"ErrorReason",
            NULL,
            NULL,
            NULL,
            &hDataSize);
        if (lErrorCode != ERROR_SUCCESS)
        {
            _tprintf(TEXT("Error in RegQueryValueEx (%d).\n"), lErrorCode);
            return;
        }

        if (hDataSize > 2)
        {
            _tprintf(TEXT("ErrorReason changed, forcing activation.\n"));
            lErrorCode = SetRegString(hKey, L"ErrorReason", L"");
            if (lErrorCode != ERROR_SUCCESS)
            {
                _tprintf(TEXT("Error in RegSetValueEx (%d).\n"), lErrorCode);
                return;
            }
            WriteModelToRegistry();
        }
    }

    // Close the key.
    lErrorCode = RegCloseKey(hKey);
    if (lErrorCode != ERROR_SUCCESS)
    {
        _tprintf(TEXT("Error in RegCloseKey (%d).\n"), GetLastError());
        return;
    }

    // Close the handle.
    if (!CloseHandle(hEvents[0]) || !CloseHandle(hEvents[1]))
    {
        _tprintf(TEXT("Error in CloseHandle.\n"));
        return;
    }
}

static VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode)
{
    switch (ctrlCode)
    {
    case SERVICE_CONTROL_STOP:
        if (gServiceStatus.dwCurrentState != SERVICE_RUNNING)
        {
            break;
        }

        // Perform tasks necessary to stop the service
        gServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(gServiceStatusHandle, &gServiceStatus);

        SetEvent(gStopEvent);

    default:
        break;
    }
}

static VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    gServiceStatusHandle = RegisterServiceCtrlHandler(TEXT("XGMActivator"), ServiceCtrlHandler);

    if (!gServiceStatusHandle)
    {
        _tprintf(TEXT("Failed to register service control handler.\n"));
        return;
    }

    // Initialize service status
    gServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    gServiceStatus.dwWin32ExitCode = 0;
    gServiceStatus.dwServiceSpecificExitCode = 0;
    gServiceStatus.dwCheckPoint = 0;
    gServiceStatus.dwWaitHint = 0;

    if (!SetServiceStatus(gServiceStatusHandle, &gServiceStatus))
    {
        _tprintf(TEXT("Failed to set service status.\n"));
        return;
    }

    // Report service running status
    gServiceStatus.dwCurrentState = SERVICE_RUNNING;
    if (!SetServiceStatus(gServiceStatusHandle, &gServiceStatus))
    {
        _tprintf(TEXT("Failed to set service status.\n"));
        return;
    }

    ListenGPUSwitch();

    // Service has stopped
    gServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(gServiceStatusHandle, &gServiceStatus);
}

static int ServiceInstall(void)
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    TCHAR szUnquotedPath[MAX_PATH];

    if (!GetModuleFileName(NULL, szUnquotedPath, MAX_PATH))
    {
        _tprintf(TEXT("Cannot install service (%d)\n"), GetLastError());
        return -1;
    }

    // In case the path contains a space, it must be quoted so that
    // it is correctly interpreted. For example,
    // "d:\my share\myservice.exe" should be specified as
    // ""d:\my share\myservice.exe"".
    TCHAR szPath[MAX_PATH*2];
    StringCbPrintf(szPath, MAX_PATH*2, TEXT("\"%s\" /Service /Model %s"), szUnquotedPath, gModel);

    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager)
    {
        _tprintf(TEXT("OpenSCManager failed (%d)\n"), GetLastError());
        return -1;
    }

    // Create the service

    schService = CreateService(
        schSCManager,              // SCM database 
        L"XGMActivator",           // name of service 
        L"XGMActivator",           // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_AUTO_START,        // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (schService == NULL)
    {
        _tprintf(TEXT("CreateService failed (%d)\n"), GetLastError());
        CloseServiceHandle(schSCManager);
        return -1;
    }
    else _tprintf(TEXT("Service installed successfully\n"));

    if (!StartService(schService, 0, NULL))
    {
        _tprintf(TEXT("StartService failed (%d)\n"), GetLastError());
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

static int ServiceUninstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    SERVICE_STATUS_PROCESS ssp;

    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager)
    {
        _tprintf(TEXT("OpenSCManager failed (%d)\n"), GetLastError());
        return -1;
    }

    // Get a handle to the service.

    schService = OpenService(
        schSCManager,       // SCM database 
        L"XGMActivator",    // name of service 
        SERVICE_STOP |
        DELETE);            // need delete access 

    if (schService == NULL)
    {
        _tprintf(TEXT("OpenService failed (%d)\n"), GetLastError());
        CloseServiceHandle(schSCManager);
        return -1;
    }

    if (!ControlService(
        schService,
        SERVICE_CONTROL_STOP,
        (LPSERVICE_STATUS)&ssp))
    {
        _tprintf(TEXT("ControlService failed (%d)\n"), GetLastError());
    }

    // Delete the service.

    if (!DeleteService(schService))
    {
        _tprintf(TEXT("DeleteService failed (%d)\n"), GetLastError());
    }
    else _tprintf(TEXT("Service deleted successfully\n"));

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

int __cdecl _tmain(int argc, TCHAR* argv[])
{
    int isService = 0;
    int isInstall = 0;
    int isUninstall = 0;

    for (int i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/Service") == 0)
        {
            isService = 1;
        }
        else if (_wcsicmp(argv[i], L"/Install") == 0)
        {
            isInstall = 1;
        }
        else if (_wcsicmp(argv[i], L"/Uninstall") == 0)
        {
            isUninstall = 1;
        }
        else if (_wcsicmp(argv[i], L"/Model") == 0 && i + 1 < argc)
        {
            if (!SetupModel(argv[i+1]))
            {
                _tprintf(TEXT("WARNING: Invalid model '%s', using defaults.\n"), argv[i+1]);
            }
            i++;
        }
        else
        {
            _tprintf(TEXT("usage: %s [/Service|/Install|/Uninstall] [/Model GC31R|GC31S|GC32L|GC33Y|GC33Z]\n"), argv[0]);
            return 1;
        }
    }

    gStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (gStopEvent == NULL)
    {
        _tprintf(TEXT("Error in CreateEvent (%d).\n"), GetLastError());
        return -1;
    }

    if (isService)
    {
        SERVICE_TABLE_ENTRY serviceTable[] =
        {
            { TEXT("XGMActivator"), ServiceMain },
            { NULL, NULL }
        };

        if (!StartServiceCtrlDispatcher(serviceTable))
        {
            _tprintf(TEXT("Failed to start service dispatcher.\n"));
            return -1;
        }
    }
    else if (isInstall)
    {
        return ServiceInstall();
    }
    else if (isUninstall)
    {
        ServiceUninstall();
    }
    else
    {
        ListenGPUSwitch();
    }
    return 0;
}
