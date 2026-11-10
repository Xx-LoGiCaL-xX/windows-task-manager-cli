#include "TaskManager.h"


int wmain(int argc, const WCHAR* argv[])
{
	DWORD option;


	//Loop until 3 is selected (Exit)
	do {
		option = 0;
		std::unordered_map<HANDLE, DWORD64> processCacheA;
		DWORD64 systemTimeA = SetComparativeTimeInfo(&processCacheA);
		SYSTEM_PROCESS_INFORMATION* procInfo = NULL;
		DWORD numOfBytes = 0;

		// Obtain processe data, continue if successful
		if (GetProcessInfo(&procInfo, &numOfBytes)) {

			SYSTEM_PROCESS_INFORMATION* curProcInfo = procInfo;
			if (curProcInfo == NULL) {
				free(procInfo);
				return 1;
			}

			PrintProcessesData(curProcInfo, &processCacheA, systemTimeA);

		}

		if (procInfo != NULL)
			free(procInfo);

		Sleep(1000);
		option = GetUserOption();

		// Handle Kill Process Option (2)
		if (option == 2) {
			KillTargetProcess();
		}

		// Create enough space between last snapshot in console
		wprintf_s(L"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

	} while (option != 3);


	return 0;

}


