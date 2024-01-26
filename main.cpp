/*++

Copyright (c) Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    This implements the wmain() function for the RegFS provider.  It sets up the RegfsProvider object
    and starts the provider.

--*/

#include "stdafx.h"
#include <iostream>

using namespace regfs;



int __cdecl wmain(int argc, const WCHAR **argv)
{
    if (argc <= 1)
    {
        wprintf(L"Usage: \n");
        wprintf(L"> regfs.exe <Virtualization Root Path> \n");

        return -1;
    }

    // argv[1] should be the path to the virtualization root.
    std::wstring rootPath = argv[1];

    // Specify the notifications that we want ProjFS to send to us.  Everywhere under the virtualization
    // root we want ProjFS to tell us when files have been opened, when they're about to be renamed,
    // and when they're about to be deleted.
    PRJ_NOTIFICATION_MAPPING notificationMappings[1] = {};
    notificationMappings[0].NotificationRoot = L"";
    notificationMappings[0].NotificationBitMask = PRJ_NOTIFY_FILE_OPENED |
                                                  PRJ_NOTIFY_PRE_RENAME |
                                                  PRJ_NOTIFY_PRE_DELETE;

    // Store the notification mapping we set up into a start options structure.  We leave all the
    // other options at their defaults.  
    PRJ_STARTVIRTUALIZING_OPTIONS opts = {};
    opts.NotificationMappings = notificationMappings;
    opts.NotificationMappingsCount = 1;

    // Start the provider using the options we set up.
    RegfsProvider provider;
    auto hr = provider.Start(rootPath.c_str(), &opts);
    if (FAILED(hr))
    {
        wprintf(L"Failed to start virtualization instance: 0x%08x\n", hr);
        return -1;
    }

    wprintf(L"RegFS is running at virtualization root [%s]\n", rootPath.c_str());
    wprintf(L"Press Enter to stop the provider...");

    // Đặt đường dẫn hiện tại về thư mục ảo khi người dùng nhấn Enter
    SetCurrentDirectoryW(rootPath.c_str());

    WCHAR driveLetter;
    for (driveLetter = L'A'; driveLetter <= L'Z'; ++driveLetter) {
        std::wstring targetDrivePath = L"";
        targetDrivePath += driveLetter;

        targetDrivePath += L":\\";

        if (CreateSymbolicLink((rootPath + std::wstring(L"\\") + driveLetter).c_str(),
                                targetDrivePath.c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY)==0) 
        {
            std::cerr << "Failed to create symbolic link for drive " << driveLetter << std::endl;
            return 1;
        }
    }

    std::wcout << L"Symbolic links created from: " << rootPath << " to all drives." << std::endl;


    getchar();

    provider.Stop();

    return 0;
};



/*#include "stdafx.h"
#include <Windows.h>
#include <thread>

using namespace regfs;

void RedirectIfVirtualizationRoot(const WCHAR* virtualizationRoot)
{
    // Check if the current directory matches the virtualization root
    WCHAR currentDirectory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDirectory);

    if (wcsicmp(currentDirectory, virtualizationRoot) != 0)
    {
        // Redirect to the virtualization root
        SetCurrentDirectory(virtualizationRoot);
        wprintf(L"Redirected to virtualization root: %s\n", virtualizationRoot);
    }
}

void MonitorDirectoryChanges(const WCHAR* directoryPath)
{
    HANDLE hDirectory = CreateFile(
        directoryPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Error opening directory for monitoring: %lu\n", GetLastError());
        return;
    }

    DWORD bufferSize = 4096;
    BYTE buffer[4096];

    while (true)
    {
        DWORD bytesRead = 0;
        if (ReadDirectoryChangesW(
            hDirectory,
            buffer,
            bufferSize,
            TRUE,
            FILE_NOTIFY_CHANGE_DIR_NAME,
            &bytesRead,
            NULL,
            NULL))
        {
            // Directory change detected, redirect if needed
            RedirectIfVirtualizationRoot(directoryPath);
        }
        else
        {
            wprintf(L"Error reading directory changes: %lu\n", GetLastError());
        }
    }

    CloseHandle(hDirectory);
}

int __cdecl wmain(int argc, const WCHAR** argv)
{
    if (argc <= 1)
    {
        wprintf(L"Usage: \n");
        wprintf(L"> regfs.exe <Virtualization Root Path> \n");

        return -1;
    }

    // argv[1] should be the path to the virtualization root.
    std::wstring rootPath = argv[1];

    // Specify the notifications that we want ProjFS to send to us.
    PRJ_NOTIFICATION_MAPPING notificationMappings[1] = {};
    notificationMappings[0].NotificationRoot = L"";
    notificationMappings[0].NotificationBitMask = PRJ_NOTIFY_FILE_OPENED |
        PRJ_NOTIFY_PRE_RENAME |
        PRJ_NOTIFY_PRE_DELETE;

    // Store the notification mapping we set up into a start options structure.
    PRJ_STARTVIRTUALIZING_OPTIONS opts = {};
    opts.NotificationMappings = notificationMappings;
    opts.NotificationMappingsCount = 1;

    // Start the provider using the options we set up.
    RegfsProvider provider;
    auto hr = provider.Start(rootPath.c_str(), &opts);
    if (FAILED(hr))
    {
        wprintf(L"Failed to start virtualization instance: 0x%08x\n", hr);
        return -1;
    }

    wprintf(L"RegFS is running at virtualization root [%s]\n", rootPath.c_str());
    wprintf(L"Press Enter to stop the provider...");

    // Start monitoring directory changes in a separate thread
    std::thread(MonitorDirectoryChanges, rootPath.c_str()).detach();

    getchar();

    provider.Stop();

    return 0;
}
*/
/*#include "stdafx.h"
#include <Windows.h>
#include <iostream>
#include <thread>

using namespace regfs;


void RedirectIfVirtualizationRoot(const WCHAR* virtualizationRoot)
{
    // Check if the current directory matches the virtualization root
    WCHAR currentDirectory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDirectory);

    if (wcsicmp(currentDirectory, virtualizationRoot) != 0)
    {
        // Redirect to the virtualization root
        SetCurrentDirectory(virtualizationRoot);
        wprintf(L"Redirected to virtualization root: %s\n", virtualizationRoot);
    }
}

void MonitorDirectoryChanges(const WCHAR* directoryPath, HANDLE pipeHandle)
{
    HANDLE hDirectory = CreateFile(
        directoryPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Error opening directory for monitoring: %lu\n", GetLastError());
        return;
    }

    DWORD bufferSize = 4096;
    BYTE buffer[4096];

    while (true)
    {
        DWORD bytesRead = 0;
        if (ReadDirectoryChangesW(
            hDirectory,
            buffer,
            bufferSize,
            TRUE,
            FILE_NOTIFY_CHANGE_DIR_NAME,
            &bytesRead,
            NULL,
            NULL))
        {
            // Notify the main process through the pipe
            DWORD bytesWritten;
            WriteFile(pipeHandle, "Changed", sizeof("Changed"), &bytesWritten, NULL);
        }
        else
        {
            wprintf(L"Error reading directory changes: %lu\n", GetLastError());
        }
    }

    CloseHandle(hDirectory);
}

int __cdecl wmain(int argc, const WCHAR** argv)
{
    if (argc <= 1)
    {
        wprintf(L"Usage: \n");
        wprintf(L"> regfs.exe <Virtualization Root Path> \n");

        return -1;
    }

    // argv[1] should be the path to the virtualization root.
    std::wstring rootPath = argv[1];

    // Create a named pipe for communication
    HANDLE pipeHandle = CreateNamedPipe(
        L"\\\\.\\pipe\\RegFSRedirectPipe",
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        0,
        0,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );

    if (pipeHandle == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Error creating named pipe: %lu\n", GetLastError());
        return -1;
    }

    // Specify the notifications that we want ProjFS to send to us.
    PRJ_NOTIFICATION_MAPPING notificationMappings[1] = {};
    notificationMappings[0].NotificationRoot = L"";
    notificationMappings[0].NotificationBitMask = PRJ_NOTIFY_FILE_OPENED |
        PRJ_NOTIFY_PRE_RENAME |
        PRJ_NOTIFY_PRE_DELETE;

    // Store the notification mapping we set up into a start options structure.
    PRJ_STARTVIRTUALIZING_OPTIONS opts = {};
    opts.NotificationMappings = notificationMappings;
    opts.NotificationMappingsCount = 1;

    // Start the provider using the options we set up.
    RegfsProvider provider;
    auto hr = provider.Start(rootPath.c_str(), &opts);
    if (FAILED(hr))
    {
        wprintf(L"Failed to start virtualization instance: 0x%08x\n", hr);
        CloseHandle(pipeHandle);
        return -1;
    }

    wprintf(L"RegFS is running at virtualization root [%s]\n", rootPath.c_str());
    wprintf(L"Press Enter to stop the provider...");

    // Start monitoring directory changes in a separate thread
    std::thread(MonitorDirectoryChanges, rootPath.c_str(), pipeHandle).detach();

    getchar();

    // Redirect to the virtualization root if needed
    RedirectIfVirtualizationRoot(rootPath.c_str());

    provider.Stop();

    CloseHandle(pipeHandle);

    return 0;
}
*/

/*#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <thread>
#include <iostream>

using namespace regfs;

void RedirectIfVirtualizationRoot(const WCHAR* virtualizationRoot)
{
    // Check if the current directory matches the virtualization root
    WCHAR currentDirectory[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDirectory);

    if (wcsicmp(currentDirectory, virtualizationRoot) != 0)
    {
        // Redirect to the virtualization root
        SetCurrentDirectory(virtualizationRoot);
        wprintf(L"Redirected to virtualization root: %s\n", virtualizationRoot);
    }
}

void MonitorDirectoryChanges(const WCHAR* directoryPath, HANDLE pipeHandle)
{
    HANDLE hDirectory = CreateFile(
        directoryPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Error opening directory for monitoring: %lu\n", GetLastError());
        return;
    }

    DWORD bufferSize = 4096;
    BYTE buffer[4096];

    while (true)
    {
        DWORD bytesRead = 0;
        if (ReadDirectoryChangesW(
            hDirectory,
            buffer,
            bufferSize,
            TRUE,
            FILE_NOTIFY_CHANGE_DIR_NAME,
            &bytesRead,
            NULL,
            NULL))
        {
            // Notify the main process through the pipe
            DWORD bytesWritten;
            WriteFile(pipeHandle, "Changed", sizeof("Changed"), &bytesWritten, NULL);
        }
        else
        {
            wprintf(L"Error reading directory changes: %lu\n", GetLastError());
        }
    }

    CloseHandle(hDirectory);
}

int __cdecl wmain(int argc, const WCHAR** argv)
{
    if (argc <= 1)
    {
        wprintf(L"Usage: \n");
        wprintf(L"> regfs.exe <Virtualization Root Path> \n");

        return -1;
    }

    // argv[1] should be the path to the virtualization root.
    std::wstring rootPath = argv[1];

    // Create a named pipe for communication
    HANDLE pipeHandle = CreateNamedPipe(
        L"\\\\.\\pipe\\RegFSRedirectPipe",
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        0,
        0,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );

    if (pipeHandle == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Error creating named pipe: %lu\n", GetLastError());
        return -1;
    }

    // Specify the notifications that we want ProjFS to send to us.
    PRJ_NOTIFICATION_MAPPING notificationMappings[1] = {};
    notificationMappings[0].NotificationRoot = L"";
    notificationMappings[0].NotificationBitMask = PRJ_NOTIFY_FILE_OPENED |
        PRJ_NOTIFY_PRE_RENAME |
        PRJ_NOTIFY_PRE_DELETE;

    // Store the notification mapping we set up into a start options structure.
    PRJ_STARTVIRTUALIZING_OPTIONS opts = {};
    opts.NotificationMappings = notificationMappings;
    opts.NotificationMappingsCount = 1;

    // Start the provider using the options we set up.
    RegfsProvider provider;
    auto hr = provider.Start(rootPath.c_str(), &opts);
    if (FAILED(hr))
    {
        wprintf(L"Failed to start virtualization instance: 0x%08x\n", hr);
        CloseHandle(pipeHandle);
        return -1;
    }

    wprintf(L"RegFS is running at virtualization root [%s]\n", rootPath.c_str());
    wprintf(L"Press Enter to stop the provider...");

    // Start monitoring directory changes in a separate thread
    std::thread(MonitorDirectoryChanges, rootPath.c_str(), pipeHandle).detach();

    while (true)
    {
        // Wait for a message from the monitoring thread
        char buffer[256];
        DWORD bytesRead;
        if (ReadFile(pipeHandle, buffer, sizeof(buffer), &bytesRead, NULL))
        {
            // Redirect to the virtualization root if needed
            RedirectIfVirtualizationRoot(rootPath.c_str());
        }
        else
        {
            wprintf(L"Error reading from pipe: %lu\n", GetLastError());
        }

        // Sleep for a short duration to avoid high CPU usage
        Sleep(100);
    }

    provider.Stop();

    CloseHandle(pipeHandle);

    return 0;
}
*/