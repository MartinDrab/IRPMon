
#include <windows.h>
#include <winternl.h>
#include "debug.h"
#include "kernel-shared.h"
#include "general-types.h"
#include "device-connector.h"



/************************************************************************/
/*                   GLOBAL VARIABLES                                   */
/************************************************************************/

static HANDLE _deviceHandle = INVALID_HANDLE_VALUE;

IRPMON_DRIVER_COMM_INTERFACE DriverCommInterface = {
	ictDevice,
	DevConn_SynchronousOtherIOCTL,
	DevConn_Connect,
	DevConn_Disconnect,
	DevConn_Active,
};


/************************************************************************/
/*                  HELPER FUNCTIONS                                    */
/************************************************************************/



/************************************************************************/
/*                 PUBLIC FUNCTIONS                                     */
/************************************************************************/


DWORD DevConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	DWORD dummy = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Code=0x%x; InputBuffer=0x%p; InputBufferLength=%u; OutputBuffer=0x%p; OutputBufferLength=%u", Code, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

	if (DeviceIoControl(_deviceHandle, Code, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, &dummy, NULL))
		ret = ERROR_SUCCESS;
	else ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DevConn_Connect(const IRPMON_INIT_INFO *Info)
{
	wchar_t *deviceName = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	ret = ERROR_SUCCESS;
	deviceName = Info->Data.Device.DeviceName;
	if (deviceName == NULL || *deviceName == L'\0')
		deviceName = IRPMNDRV_USER_DEVICE_NAME;

	_deviceHandle = CreateFileW(deviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (_deviceHandle == INVALID_HANDLE_VALUE)
		ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


void DevConn_Disconnect(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	CloseHandle(_deviceHandle);
	_deviceHandle = INVALID_HANDLE_VALUE;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


BOOL DevConn_Active(VOID)
{
	return (_deviceHandle != INVALID_HANDLE_VALUE);
}
