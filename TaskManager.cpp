#include "TaskManager.h"


BOOL GetProcessInfo(SYSTEM_PROCESS_INFORMATION** sysProcInfo, DWORD* totalBytes)
{
	*totalBytes = 0;
	*sysProcInfo = NULL;

	// First call to obtain  total bytes for all process info
	::NtQuerySystemInformation(SystemProcessInformation,
		NULL,
		0,
		totalBytes
	);

	// Add padding for processes that may be added between 2nd call
	*totalBytes += 4096;
	*sysProcInfo = (SYSTEM_PROCESS_INFORMATION*)malloc((size_t)*totalBytes);

	// Second call to obtain the data, set bytes, and full SYSTEM_PROCESS_INFORMATION struct array
	NTSTATUS status = ::NtQuerySystemInformation(SystemProcessInformation,
		*sysProcInfo,
		*totalBytes,
		totalBytes
	);

	if (*sysProcInfo == NULL)
		return FALSE;


	return TRUE;

}

BOOL PrintProcessesData(SYSTEM_PROCESS_INFORMATION* sysProcInfo, std::unordered_map<HANDLE, DWORD64>* processCacheA, DWORD64 systemTimeA)
{

	if (sysProcInfo != NULL) {
		// Get fileSystemTimeB to compare to fileSystemA
		FILETIME fileTimeB;
		GetSystemTimePreciseAsFileTime(&fileTimeB);

		// Get the difference between systemTimeA and currentTime (systemTimeB)
		DWORD64 systemTimeB = ((ULARGE_INTEGER*)&fileTimeB)->QuadPart;
		DWORD64 sysTimeDiff = systemTimeB - systemTimeA;

		//Print Headers
		wprintf_s(L"\n\nProcess\t\t\t\t\t\tPID\tCPU\t   Private Bytes   Working Set   Path\n\n");
		wprintf_s(L"----------------------------------------------------------------------------------------------------");
		wprintf_s(L"----------------------------------------------------------------------------------------------------\n");

		// Loop through each SYSTEM_PROCESS_INFORMATION
		// struct in the curProcInfo array.
		while (TRUE) {

			if (sysProcInfo->ImageName.Buffer != NULL && sysProcInfo->ImageName.Length > 0
				&& processCacheA->find(sysProcInfo->UniqueProcessId) != processCacheA->end())
			{

				// initProcInfo->Reserved1[32] = process userTime
				// initProcInfo->Reserved1[40] = process kernelTime
				// currentProcTime = (userTime + kernelTime) - processCacheA->totalTime
				DWORD64 procTimeDiff = (*(DWORD64*)&sysProcInfo->Reserved1[32] + *(DWORD64*)&sysProcInfo->Reserved1[40])
					- processCacheA->find(sysProcInfo->UniqueProcessId)->second;

				// set stats
				double cpuPercent = ((double)procTimeDiff / (double)sysTimeDiff) * 100;
				SIZE_T privateBytes = sysProcInfo->PrivatePageCount / 1024;
				SIZE_T workingSet = sysProcInfo->WorkingSetSize / 1024;

				WCHAR* exePath = NULL;
				DWORD pathCount = GetFullProcessImageName(sysProcInfo->UniqueProcessId, &exePath);

				wprintf_s(L"%-48ws%-6zu\t%-10.2f%8zu k\t%10zu k    %ws\n",
					sysProcInfo->ImageName.Buffer,
					(ULONG_PTR)sysProcInfo->UniqueProcessId,
					cpuPercent,
					privateBytes,
					workingSet,
					pathCount > 0 ? exePath : L"Unknown"
				);
			}

			// has reached the end of sysProcInfo array
			if (sysProcInfo->NextEntryOffset == NULL)
				break;

			// Advance pointer in curProcInfo array to next 
			// SYSTEM_PROCESS_INFORMATION struct: 
			// cast to BYTE array, do pointer arithmatic, then cast back 
			sysProcInfo = (SYSTEM_PROCESS_INFORMATION*)((BYTE*)sysProcInfo + sysProcInfo->NextEntryOffset);

		}

		//Print Footer
		wprintf_s(L"----------------------------------------------------------------------------------------------------");
		wprintf_s(L"----------------------------------------------------------------------------------------------------\n");

	}
	else {
		return FALSE;
	}

	return TRUE;

}

DWORD64 SetComparativeTimeInfo(std::unordered_map<HANDLE, DWORD64>* processCache)
{
	SYSTEM_PROCESS_INFORMATION* initProcInfo = NULL;
	DWORD bytes = 0;

	::NtQuerySystemInformation(
		SystemProcessInformation,
		NULL,
		0,
		&bytes
	);

	// add extra space for any added processes
	bytes += 4096;
	initProcInfo = (SYSTEM_PROCESS_INFORMATION*)malloc((size_t)bytes);

	::NtQuerySystemInformation(
		SystemProcessInformation,
		initProcInfo,
		bytes,
		&bytes
	);

	if (initProcInfo == NULL)
		return 0;

	SYSTEM_PROCESS_INFORMATION* tempProcInfo = initProcInfo;

	while (TRUE) {

		if (tempProcInfo->ImageName.Buffer != NULL && tempProcInfo->ImageName.Length > 0) {
			// initProcInfo->Reserved1[32] = process userTime
			// initProcInfo->Reserved1[40] = process kernelTime
			DWORD64 totalTime = *(DWORD64*)&tempProcInfo->Reserved1[32] + *(DWORD64*)&tempProcInfo->Reserved1[40];

			// Assign value to unordered_map by reference
			processCache->insert_or_assign(tempProcInfo->UniqueProcessId, totalTime);
		}

		if (tempProcInfo->NextEntryOffset == NULL)
			break;

		// Advance pointer in curProcInfo array to next 
		// SYSTEM_PROCESS_INFORMATION struct: 
		// cast to BYTE array, do pointer arithmatic, then cast back 
		tempProcInfo = (SYSTEM_PROCESS_INFORMATION*)((BYTE*)tempProcInfo + tempProcInfo->NextEntryOffset);

	}

	if (initProcInfo != NULL)
		free(initProcInfo);

	// Get System Time
	FILETIME fileTimeSysTime;
	::GetSystemTimePreciseAsFileTime(&fileTimeSysTime);

	// Sleep for 500ms to create time difference before
	// next call to NtQuerySystemInformation
	Sleep(500);

	// Return 64-bit system time
	return ((ULARGE_INTEGER*)&fileTimeSysTime)->QuadPart;

}

DWORD GetUserOption()
{
	DWORD currOption = 0;

	wprintf_s(L"MENU\n\n 1) Refresh Tasks\n 2) Kill Process\n 3) Exit\n\nPlease select the numerical options from above: ");
	wscanf_s(L"%lu", &currOption);

	wint_t wc;
	while ((wc = getwchar()) != L'\n' && wc != WEOF) {
		// Loop discards leftover characters and the newline char.
		// resolves issue if a character(s) are entered
		// from inifinite loop occurring
	}

	if (currOption != 1 && currOption != 2 && currOption != 3)
		currOption = 1;


	return currOption;

}


VOID KillTargetProcess()
{
	DWORD targetPid = NULL;
	HANDLE hProcess = NULL;

	wprintf_s(L"Enter ProcessID of target process: ");
	wscanf_s(L"%lu", &targetPid);

	wint_t wc;
	while ((wc = getchar()) != L'\n' && wc != WEOF) {
		// Loop discards leftover characters and the newline char.
		// resolves issue if a character(s) are entered
		// from inifinite loop occurring
	}

	hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, targetPid);
	if (!hProcess) {
		wprintf_s(L"\nFailed to kill process: PID=%lu\n", targetPid);
	}
	else {
		BOOL success = ::TerminateProcess(hProcess, 1);
		wprintf_s(L"\nProcess Successfully Terminated: PID=%lu\n", targetPid);
	}

	if (hProcess != NULL)
		::CloseHandle(hProcess);


	Sleep(5000);

}


DWORD GetFullProcessImageName(HANDLE pid, WCHAR** fullPath)
{
	HANDLE hProcess = NULL;
	BOOL result = TRUE;
	DWORD count = 0;

	hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
		FALSE,
		(DWORD)pid
	);

	if (hProcess == NULL)
		return count;

	// create as static so fullPath pointer 
	// points to existing address after function 
	// returns.
	static WCHAR exeName[MAX_PATH];
	DWORD size = MAX_PATH;

	count = ::QueryFullProcessImageNameW(hProcess,
		0,
		exeName,
		&size
	);


	if (hProcess != NULL) {
		*fullPath = exeName;
		CloseHandle(hProcess);
	}


	return count;

}