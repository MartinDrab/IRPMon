
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winternl.h>
#include "debug.h"
#include "libvsock.h"
#include "hyperv-connector.h"



/************************************************************************/
/*                   GLOBAL VARIABLES                                   */
/************************************************************************/

static SOCKET _socket = INVALID_SOCKET;
static CRITICAL_SECTION _ioctlLock;
static HANDLE _pingingThreadHandle = NULL;
static volatile BOOL _pindingThreadTerminate = FALSE;

IRPMON_DRIVER_COMM_INTERFACE DriverCommInterface = {
	ictHyperV,
	HyperVConn_SynchronousOtherIOCTL,
	HyperVConn_Connect,
	HyperVConn_Disconnect,
	HyperVConn_Active,
};


/************************************************************************/
/*                  HELPER FUNCTIONS                                    */
/************************************************************************/

static DWORD WINAPI _PingingThread(PVOID Context)
{
	int ret = ERROR_SUCCESS;
	int bytesTransferred = 0;
	SOCKET s = (SOCKET)Context;
	HYPERV_MSG_IOCTL msg;
	DEBUG_ENTER_FUNCTION("Context=0x%p", Context);

	while (!_pindingThreadTerminate) {
		Sleep(1000);
		memset(&msg, 0, sizeof(msg));
		EnterCriticalSection(&_ioctlLock);
		bytesTransferred = send(s, (char *)&msg, sizeof(msg), 0);
		if (bytesTransferred != sizeof(msg)) {
			ret = WSAGetLastError();
			LeaveCriticalSection(&_ioctlLock);
			_pindingThreadTerminate = TRUE;
			continue;
		}

		bytesTransferred = recv(s, (char *)&msg, sizeof(msg), MSG_WAITALL);
		if (bytesTransferred != sizeof(msg)) {
			ret = WSAGetLastError();
			LeaveCriticalSection(&_ioctlLock);
			_pindingThreadTerminate = TRUE;
			continue;
		}

		LeaveCriticalSection(&_ioctlLock);
	}

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


/************************************************************************/
/*                 PUBLIC FUNCTIONS                                     */
/************************************************************************/


DWORD HyperVConn_SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	void *tmp = NULL;
	HYPERV_MSG_IOCTL msg;
	int bytesTransferred = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Code=0x%x; InputBuffer=0x%p; InputBufferLength=%u; OutputBuffer=0x%p; OutputBufferLength=%u", Code, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

	memset(&msg, 0, sizeof(msg));
	msg.Result = ERROR_IO_PENDING;
	msg.ControlCode = Code;
	msg.InputBufferSize = InputBufferLength;
	msg.OutputBufferSize = OutputBufferLength;
	EnterCriticalSection(&_ioctlLock);
	bytesTransferred = send(_socket, (char *)&msg, sizeof(msg), 0);
	if (bytesTransferred == sizeof(msg)) {
		bytesTransferred = send(_socket, (char *)InputBuffer, InputBufferLength, 0);
		if (bytesTransferred == InputBufferLength) {
			memset(&msg, 0, sizeof(msg));
			bytesTransferred = recv(_socket, (char *)&msg, sizeof(msg), MSG_WAITALL);
			if (bytesTransferred == sizeof(msg)) {
				if (OutputBufferLength > 0)
					tmp = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, OutputBufferLength);
				
				if (tmp != NULL || OutputBufferLength == 0) {
					bytesTransferred = msg.OutputBufferSize;
					if (msg.OutputBufferSize > 0)
						bytesTransferred = recv(_socket, (char *)tmp, msg.OutputBufferSize, MSG_WAITALL);
					
					if (bytesTransferred == msg.OutputBufferSize) {
						ret = msg.Result;
						if (OutputBufferLength >= msg.OutputBufferSize)
							memcpy(OutputBuffer, tmp, msg.OutputBufferSize);
						else ret = ERROR_INSUFFICIENT_BUFFER;
					} else ret = WSAGetLastError();

					if (OutputBufferLength > 0)
						HeapFree(GetProcessHeap(), 0, tmp);
				} else ret = GetLastError();
			} else ret = WSAGetLastError();
		} else ret = WSAGetLastError();
	} else ret = WSAGetLastError();

	LeaveCriticalSection(&_ioctlLock);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD HyperVConn_Connect(const IRPMON_INIT_INFO *Info)
{
	uint32_t timeout;
	uint32_t wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	HYPERV_MSG_IOCTL msg;
	int bytesReceived = 0;
	struct sockaddr *addr = NULL;
	int addrLen = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);
	
	if (_socket == INVALID_SOCKET) {
		if (InitializeCriticalSectionAndSpinCount(&_ioctlLock, 0x1000)) {
			ret = WSAStartup(wVersionRequested, &wsaData);
			if (ret == 0) {
				_socket = socket(AF_HYPERV, SOCK_STREAM, 0);
				if (_socket != INVALID_SOCKET) {
					ret = LibVSockHyperVAddressAlloc(&Info->Data.HyperV.VMId, &Info->Data.HyperV.AppId, &addr, &addrLen);
					if (ret == 0) {
						ret = connect(_socket, addr, addrLen);
						if (ret == 0) {
							timeout = 10000;
							ret = setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
							if (ret == 0) {
								memset(&msg, 0, sizeof(msg));
								bytesReceived = recv(_socket, (char *)&msg, sizeof(msg), MSG_WAITALL);
								if (bytesReceived == sizeof(msg)) {
									ret = msg.Result;
									if (ret == ERROR_SUCCESS) {
										switch (msg.ControlCode) {
											case IOCTL_IRPMON_SERVER_ARCH_32BIT:
#ifdef _AMD64_
												ret = ERROR_NOT_SUPPORTED;
#endif
												break;
											case IOCTL_IRPMON_SERVER_ARCH_64BIT:
#ifdef _X86_
												ret = ERROR_NOT_SUPPORTED;
#endif
												break;
											default:
												ret = ERROR_INVALID_MESSAGE;
												break;
										}
											
										if (ret == ERROR_SUCCESS) {
											_pindingThreadTerminate = FALSE;
											_pingingThreadHandle = CreateThread(NULL, 0, _PingingThread, (PVOID)_socket, 0, NULL);
											if (_pingingThreadHandle == NULL)
												ret = GetLastError();
										}
									}
								} else ret = WSAGetLastError();
							} else ret = WSAGetLastError();

							if (ret != 0)
								shutdown(_socket, SD_BOTH);
						} else ret = WSAGetLastError();
					
						if (ret != 0)
							LibVSockAddressFree(addr);
					}

					if (ret != 0) {
						closesocket(_socket);
						_socket = INVALID_SOCKET;
					}
				} else ret = WSAGetLastError();

				if (ret != 0)
					WSACleanup();
			} else ret = WSAGetLastError();

			if (ret != 0)
				DeleteCriticalSection(&_ioctlLock);
		} else ret = GetLastError();
	} else ret = ERROR_ALREADY_EXISTS;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


void HyperVConn_Disconnect(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	if (_socket != INVALID_SOCKET) {
		_pindingThreadTerminate = TRUE;
		WaitForSingleObject(_pingingThreadHandle, INFINITE);
		CloseHandle(_pingingThreadHandle);
		shutdown(_socket, SD_BOTH);
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		WSACleanup();
		DeleteCriticalSection(&_ioctlLock);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


BOOL HyperVConn_Active(void)
{
	return (_socket != INVALID_SOCKET);
}
