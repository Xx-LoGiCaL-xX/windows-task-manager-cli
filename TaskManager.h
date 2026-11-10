#pragma once

#include <stdio.h>
#include <windows.h>
#include <winternl.h>
#include <unordered_map>

#pragma comment(lib, "ntdll.lib")


BOOL GetProcessInfo(SYSTEM_PROCESS_INFORMATION** sysProcInfo, DWORD* totalBytes);
BOOL PrintProcessesData(SYSTEM_PROCESS_INFORMATION* sysProcInfo, std::unordered_map<HANDLE, DWORD64>* processCacheA, DWORD64 systemTimeA);
DWORD64 SetComparativeTimeInfo(std::unordered_map<HANDLE, DWORD64>* processCache);
DWORD GetUserOption();
VOID KillTargetProcess();
DWORD GetFullProcessImageName(HANDLE pid, WCHAR** fullPath);