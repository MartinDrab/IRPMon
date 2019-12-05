
#include <windows.h>
#include <winternl.h>
#include "debug.h"
#include "kernel-shared.h"
#include "general-types.h"
#include "driver-com.h"
#include "device-connector.h"



/************************************************************************/
/*                   GLOBAL VARIABLES                                   */
/************************************************************************/

static HANDLE _deviceHandle = INVALID_HANDLE_VALUE;


/************************************************************************/
/*                  HELPER FUNCTIONS                                    */
/************************************************************************/



/************************************************************************/
/*                 PUBLIC FUNCTIONS                                     */
/************************************************************************/


DWORD DevConn_SynchronousNoIOIOCTL(DWORD Code)
{
	DWORD dummy = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Code=0x%x", Code);

	if (DeviceIoControl(_deviceHandle, Code, NULL, 0, NULL, 0, &dummy, NULL))
		ret = ERROR_SUCCESS;
	else ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DevConn_SynchronousWriteIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength)
{
	DWORD dummy = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Code=0x%x; InputBuffer=0x%p; InputBufferLength=%u", Code, InputBuffer, InputBufferLength);

	if (DeviceIoControl(_deviceHandle, Code, InputBuffer, InputBufferLength, NULL, 0, &dummy, NULL))
		ret = ERROR_SUCCESS;
	else ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DevConn_SynchronousReadIOCTL(DWORD Code, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	DWORD dummy = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Code=0x%x; OutputBuffer=0x%p; OutputBufferLength=%u", Code, OutputBuffer, OutputBufferLength);

	if (DeviceIoControl(_deviceHandle, Code, NULL, 0, OutputBuffer, OutputBufferLength, &dummy, NULL))
		ret = ERROR_SUCCESS;
	else ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


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


DWORD DevConn_Connect(void)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = ERROR_SUCCESS;
	_deviceHandle = CreateFileW(IRPMNDRV_USER_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
