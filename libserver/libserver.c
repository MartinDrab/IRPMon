
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "irpmondll-types.h"
#include "network-connector.h"
#include "device-connector.h"
#include "libserver.h"


/************************************************************************/
/*              GLOBAL VARIABLES                                        */
/************************************************************************/


typedef int (WSAAPI WSAPOLL)(__inout LPWSAPOLLFD fdArray, __in ULONG fds, __in INT timeout);


static IRPMON_INIT_INFO _initInfo;
static WSAPOLL* _WSAPoll = NULL;


/************************************************************************/
/*                 HELPER FUNCTIONS                                     */
/************************************************************************/

static int _Listener(const ADDRINFOA *Address, HANDLE ExitEvent)
{
	int ret = 0;
	uint32_t timeout = 10000;
	NETWORK_MSG_IOCTL msg;
	void *inputBuffer = NULL;
	void *outputBuffer = NULL;
	int bytesReceived = 0;
	SOCKET acceptSocket = INVALID_SOCKET;
	SOCKET listenSocket = INVALID_SOCKET;
	WSAPOLLFD fds;
	BOOLEAN terminated = FALSE;

	listenSocket = socket(Address->ai_family, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		ret = WSAGetLastError();
		goto Exit;
	}

	ret = bind(listenSocket, Address->ai_addr, (int)Address->ai_addrlen);
	if (ret != 0) {
		ret = WSAGetLastError();
		goto DestroySocket;
	}

	ret = listen(listenSocket, 0);
	if (ret != 0) {
		ret = WSAGetLastError();
		goto DestroySocket;
	}

	do {
		acceptSocket = INVALID_SOCKET;
		if (ExitEvent != NULL) {
			ret = WaitForSingleObject(ExitEvent, 0);
			if (ret == WAIT_OBJECT_0) {
				terminated = TRUE;
				break;
			}
		}

		memset(&fds, 0, sizeof(fds));
		fds.events = POLLIN;
		fds.revents = 0;
		fds.fd = listenSocket;
		if (_WSAPoll == NULL) {
			ret = 1;
			fds.revents = POLLIN;
		} else ret = _WSAPoll(&fds, 1, 1000);
		
		if (ret > 0) {
			if (fds.revents & POLLERR)
				ret = WSAGetLastError();
			else if (fds.revents & POLLIN)
				acceptSocket = accept(listenSocket, NULL, NULL);
		
			fds.revents = 0;
		}

		if (acceptSocket != INVALID_SOCKET) {
			terminated = FALSE;
			ret = setsockopt(acceptSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
			if (ret == 0) {
				memset(&_initInfo, 0, sizeof(_initInfo));
				_initInfo.ConnectorType = ictDevice;
				ret = DevConn_Connect(&_initInfo);
				memset(&msg, 0, sizeof(msg));
				msg.Result = ret;
				if (ret == 0) {
#if defined(_AMD64_)
					msg.ControlCode = IOCTL_IRPMON_SERVER_ARCH_64BIT;
#elif defined(_X86_)
					msg.ControlCode = IOCTL_IRPMON_SERVER_ARCH_32BIT;
#else
#error Unsupported architecture
#endif
				}

				bytesReceived = send(acceptSocket, (char *)&msg, sizeof(msg), 0);
				if (bytesReceived != sizeof(msg))
					ret = WSAGetLastError();

				if (ret == ERROR_SUCCESS) {
					do {
						bytesReceived = recv(acceptSocket, (char *)&msg, sizeof(msg), MSG_WAITALL);
						if (bytesReceived == sizeof(msg)) {
							if (msg.ControlCode == 0) {
								bytesReceived = send(acceptSocket, (char *)&msg, sizeof(msg), 0);
								if (bytesReceived != sizeof(msg))
									ret = WSAGetLastError();

								continue;
							}

							inputBuffer = NULL;
							if (msg.InputBufferSize > 0)
								inputBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, msg.InputBufferSize);

							if (inputBuffer != NULL || msg.InputBufferSize == 0) {
								outputBuffer = NULL;
								if (msg.OutputBufferSize > 0)
									outputBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, msg.OutputBufferSize);

								if (outputBuffer != NULL || msg.OutputBufferSize == 0) {
									bytesReceived = msg.InputBufferSize;
									if (msg.InputBufferSize > 0)
										bytesReceived = recv(acceptSocket, inputBuffer, msg.InputBufferSize, MSG_WAITALL);

									if (bytesReceived != msg.InputBufferSize)
										ret = WSAGetLastError();

									if (ret == 0) {
										msg.Result = DevConn_SynchronousOtherIOCTL(msg.ControlCode, inputBuffer, msg.InputBufferSize, outputBuffer, msg.OutputBufferSize);
										bytesReceived = send(acceptSocket, (char *)&msg, sizeof(msg), 0);
										if (bytesReceived != sizeof(msg))
											ret = WSAGetLastError();

										if (ret == 0 && msg.OutputBufferSize > 0) {
											bytesReceived = send(acceptSocket, (char *)outputBuffer, msg.OutputBufferSize, 0);
											if (bytesReceived != msg.OutputBufferSize)
												ret = WSAGetLastError();
										}
									}

									if (outputBuffer != NULL)
										HeapFree(GetProcessHeap(), 0, outputBuffer);
								} else ret = GetLastError();

								if (inputBuffer != NULL)
									HeapFree(GetProcessHeap(), 0, inputBuffer);
							} else ret = GetLastError();
						} else {
							ret = WSAGetLastError();
							if (ret == ERROR_SUCCESS)
								break;
						}

						if (ret == 0 && ExitEvent != NULL) {
							ret = WaitForSingleObject(ExitEvent, 0);
							if (ret == WAIT_OBJECT_0)
								terminated = TRUE;

							ret = 0;
						}

					} while (ret == 0 && !terminated);

					DevConn_Disconnect();
				}
			} else ret = WSAGetLastError();

			shutdown(acceptSocket, SD_BOTH);
			closesocket(acceptSocket);
		}
	} while (!terminated);

DestroySocket:
	closesocket(listenSocket);
Exit:
	return ret;
}


/************************************************************************/
/*                   PUBLIC FUNCTIONS                                   */
/************************************************************************/


DWORD IRPMonServerStart(const char *Address, const char *Port, HANDLE ExitEvent)
{
	int ret = 0;
	WSADATA wsaData;
	uint32_t wVersionRequested = MAKEWORD(2, 2);
	ADDRINFOA hints;
	PADDRINFOA addrs = NULL;
	HMODULE hWSA32 = NULL;

	hWSA32 = GetModuleHandleW(L"Ws2_32.dll");
	if (hWSA32 == NULL) {
		ret = GetLastError();
		goto Exit;
	}

	_WSAPoll = (WSAPOLL*)GetProcAddress(hWSA32, "WSAPoll");

	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0) {
		ret = WSAGetLastError();
		goto Exit;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	ret = getaddrinfo(Address, Port, &hints, &addrs);
	if (ret != 0) {
		ret = WSAGetLastError();
		goto Cleanup;
	}

	ret = _Listener(addrs, ExitEvent);
	freeaddrinfo(addrs);
Cleanup:
	WSACleanup();
Exit:
	return ret;
}
