
#include <map>
#include <string>
#include <windows.h>
#include "debug.h"
#include "irpmondll-types.h"
#include "irpmondll.h"
#include "cache.h"



/************************************************************************/
/*                           GLOBAL VARIABLES                           */
/************************************************************************/

static std::map<PVOID, std::wstring> _deviceNames;
static std::map<PVOID, std::wstring> _driverNames;
static CRITICAL_SECTION _cacheLock;


/************************************************************************/
/*                           HELPER FUNCTIONS                           */
/************************************************************************/

static DWORD _Refresh(VOID)
{
	ULONG driverCount = 0;
	PIRPMON_DRIVER_INFO *driverSnapshot = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = IRPMonDllSnapshotRetrieve(&driverSnapshot, &driverCount);
	if (ret == ERROR_SUCCESS) {
		_driverNames.clear();
		_deviceNames.clear();
		for (ULONG i = 0; i < driverCount; ++i) {
			PIRPMON_DRIVER_INFO dr = driverSnapshot[i];

			_driverNames.insert(std::make_pair(dr->DriverObject, std::wstring(dr->DriverName)));
			for (ULONG j = 0; j < dr->DeviceCount; ++j) {
				PIRPMON_DEVICE_INFO devr = dr->Devices[j];

				_deviceNames.insert(std::make_pair(devr->DeviceObject, std::wstring(devr->Name)));
			}
		}

		IRPMonDllSnapshotFree(driverSnapshot, driverCount);
		_deviceNames.insert(std::make_pair(nullptr, std::wstring(L"N/A")));
	}

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

/************************************************************************/
/*                           PUBLIC FUNCTIONS                           */
/************************************************************************/

std::wstring CacheDriverNameGet(PVOID DriverObject)
{
	std::wstring ret = L"";

	EnterCriticalSection(&_cacheLock);
	auto it = _driverNames.find(DriverObject);
	if (it != _driverNames.cend())
		ret = it->second;
	else if (_Refresh() == ERROR_SUCCESS) {
		it = _driverNames.find(DriverObject);
		if (it != _driverNames.cend())
			ret = it->second;
	}

	LeaveCriticalSection(&_cacheLock);

	return ret;
}

std::wstring CacheDeviceNameGet(PVOID DeviceObject)
{
	std::wstring ret = L"";

	EnterCriticalSection(&_cacheLock);
	auto it = _deviceNames.find(DeviceObject);
	if (it != _deviceNames.cend())
		ret = it->second;
	else if (_Refresh() == ERROR_SUCCESS) {
		it = _deviceNames.find(DeviceObject);
		if (it != _deviceNames.cend())
			ret = it->second;
	}

	LeaveCriticalSection(&_cacheLock);

	return ret;
}

/************************************************************************/
/*                          INITIALIZATION AND FINALIZATION             */
/************************************************************************/

DWORD CacheInit(VOID)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	if (InitializeCriticalSectionAndSpinCount(&_cacheLock, 0x1000)) {
		ret = _Refresh();
		if (ret != ERROR_SUCCESS)
			DeleteCriticalSection(&_cacheLock);
	} else ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

VOID CacheFinit(VOID)
{
	_driverNames.clear();
	_deviceNames.clear();
	DeleteCriticalSection(&_cacheLock);

	return;
}