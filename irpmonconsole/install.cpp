
#include <windows.h>
#include "debug.h"
#include "install.h"



/************************************************************************/
/*                            HELPER FUNCTIONS                          */
/************************************************************************/

static PWCHAR _ConcatStrings(PWCHAR Str1, PWCHAR Str2, BOOLEAN BackSlash)
{
	SIZE_T len = 0;
	SIZE_T len1 = 0;
	SIZE_T len2 = 0;
	PWCHAR ret = NULL;
	DEBUG_ENTER_FUNCTION("Str1=\"%S\"; Str2=\"%S\"; BackSlash=%u", Str1, Str2, BackSlash);

	len1 = wcslen(Str1)*sizeof(WCHAR);
	len2 = wcslen(Str2)*sizeof(WCHAR);
	len = len1 + len2 + (BackSlash ? 1 : 0)*sizeof(WCHAR);
	ret = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + sizeof(WCHAR));
	if (ret != NULL) {
		CopyMemory(ret, Str1, len1);
		if (BackSlash) {
			ret[len1 / sizeof(WCHAR)] = L'\\';
			CopyMemory(&ret[(len1 / sizeof(WCHAR)) + 1], Str2, len2);
		} else CopyMemory(&ret[len1 / sizeof(WCHAR)], Str2, len2);
	
		ret[len / sizeof(WCHAR)] = L'\0';
	}

	DEBUG_EXIT_FUNCTION("\"%S\"", ret);
	return ret;
}

/************************************************************************/
/*                      PUBLIC FUNCTIONS                                */
/************************************************************************/

DWORD Install(PWCHAR FileName, PWCHAR ServiceName, DWORD ServiceStart, PWCHAR *Function)
{
	DWORD nameLen = 0;
	PWCHAR systemDir = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("FileName=\"%S\"; ServiceName=\"%S\"; ServiceStart=%u", FileName, ServiceName, ServiceStart);

	systemDir = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_DIR_NAME*sizeof(WCHAR));
	if (systemDir != NULL) {
		nameLen = GetSystemDirectory(systemDir, MAX_DIR_NAME);
		if (nameLen > 0) {
			PWCHAR targetFile = NULL;

			targetFile = _ConcatStrings(systemDir, L"drivers\\irpmndrv.sys", TRUE);
			if (targetFile != NULL) {
				if (CopyFile(FileName, targetFile, FALSE)) {
					SC_HANDLE hScm = NULL;

					hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
					if (hScm != NULL) {
						SC_HANDLE hService = NULL;

						hService = CreateServiceW(hScm, ServiceName, ServiceName, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, ServiceStart, SERVICE_ERROR_NORMAL, targetFile, NULL, NULL, NULL, NULL, NULL);
						if (hService != NULL) {
							if (ServiceStart == SERVICE_DEMAND_START) {
								if (StartServiceW(hService, 0, NULL))
									ret = ERROR_SUCCESS;
								else {
									*Function = L"StartService";
									ret = GetLastError();
								}
							} else ret = ERROR_SUCCESS;

//							if (ret != ERROR_SUCCESS)
//								DeleteService(hService);

							CloseServiceHandle(hService);
						} else {
							*Function = L"CreateService";
							ret = GetLastError();
						}
						CloseServiceHandle(hScm);
					} else {
						*Function = L"OpenSCManager";
						ret = GetLastError();
					}

					if (ret != ERROR_SUCCESS)
						DeleteFile(targetFile);
				} else {
					*Function = L"CopyFile";
					ret = GetLastError();
				}

				HeapFree(GetProcessHeap(), 0, targetFile);
			} else {
				*Function = L"HeapAlloc";
				ret = ERROR_NOT_ENOUGH_MEMORY;
			}
		} else {
			*Function = L"GetSystemDirectory";
			ret = GetLastError();
		}

		HeapFree(GetProcessHeap(), 0, systemDir);
	} else {
		ret = GetLastError();
		*Function = L"HeapAlloc";
	}

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD Uninstall(PWCHAR ServiceName)
{
	SC_HANDLE hScm = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("ServiceName=\"%S\"", ServiceName);

	hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
	if (hScm != NULL) {
		SC_HANDLE hService = NULL;

		hService = OpenServiceW(hScm, ServiceName, DELETE | SERVICE_QUERY_CONFIG | SERVICE_STOP);
		if (hService != NULL) {
			DWORD configLen = 0;

			if (!QueryServiceConfig(hService, NULL, 0, &configLen) && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				LPQUERY_SERVICE_CONFIG config = NULL;

				config = (LPQUERY_SERVICE_CONFIG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, configLen);
				if (config != NULL) {
					SERVICE_STATUS serviceStatus;

					if (QueryServiceConfig(hService, config, configLen, &configLen)) {
						ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
						DeleteService(hService);
						DeleteFile(config->lpBinaryPathName);
						ret = ERROR_SUCCESS;
					} else ret = GetLastError();

					HeapFree(GetProcessHeap(), 0, config);
				} else ret = GetLastError();
			} else ret = GetLastError();

			CloseServiceHandle(hService);
		} else ret = GetLastError();

		CloseServiceHandle(hScm);
	} else ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}
