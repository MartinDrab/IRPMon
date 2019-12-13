
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "irpmondll-types.h"
#include "network-connector.h"
#include "device-connector.h"




static IRPMON_INIT_INFO _initInfo;



static int _Listener(const ADDRINFOA *Address)
{
	int ret = 0;
	uint32_t timeout = 10000;
	NETWORK_MSG_IOCTL msg;
	void *inputBuffer = NULL;
	void *outputBuffer = NULL;
	int bytesReceived = 0;
	SOCKET acceptSocket = INVALID_SOCKET;
	SOCKET listenSocket = INVALID_SOCKET;

	listenSocket = socket(Address->ai_family, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		ret = WSAGetLastError();
		fprintf(stderr, "socket: %u\n", ret);
		goto Exit;
	}

	ret = bind(listenSocket, Address->ai_addr, (int)Address->ai_addrlen);
	if (ret != 0) {
		fprintf(stderr, "bind: %u\n", ret);
		goto DestroySocket;
	}

	ret = listen(listenSocket, 0);
	if (ret != 0) {
		fprintf(stderr, "bind: %u\n", ret);
		goto DestroySocket;
	}

	do {
		acceptSocket = accept(listenSocket, NULL, NULL);
		if (acceptSocket != INVALID_SOCKET) {
			ret = setsockopt(acceptSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
			if (ret == 0) {
				memset(&_initInfo, 0, sizeof(_initInfo));
				_initInfo.ConnectorType = ictDevice;
				ret = DevConn_Connect(&_initInfo);
				memset(&msg, 0, sizeof(msg));
				msg.Result = ret;
				bytesReceived = send(acceptSocket, (char *)&msg, sizeof(msg), 0);
				if (bytesReceived != sizeof(msg)) {
					ret = WSAGetLastError();
					fprintf(stderr, "send: %u\n", ret);
				}
				
				if (ret == ERROR_SUCCESS) {
					do {
						bytesReceived = recv(acceptSocket, (char *)&msg, sizeof(msg), MSG_WAITALL);
						if (bytesReceived == sizeof(msg)) {
							if (msg.ControlCode == 0) {
								bytesReceived = send(acceptSocket, (char *)&msg, sizeof(msg), 0);
								if (bytesReceived != sizeof(msg)) {
									ret = WSAGetLastError();
									fprintf(stderr, "send: %u\n", ret);
								}

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
									
									if (bytesReceived != msg.InputBufferSize) {
										ret = WSAGetLastError();
										fprintf(stderr, "recv: %u]n", ret);
									}

									if (ret == 0) {
										msg.Result = DevConn_SynchronousOtherIOCTL(msg.ControlCode, inputBuffer, msg.InputBufferSize, outputBuffer, msg.OutputBufferSize);
										fprintf(stderr, "IOCTL: 0x%x, input %u bytes, output %u bytes, result %u\n", msg.ControlCode, msg.InputBufferSize, msg.OutputBufferSize, msg.Result);
										bytesReceived = send(acceptSocket, (char *)&msg, sizeof(msg), 0);
										if (bytesReceived != sizeof(msg)) {
											ret = WSAGetLastError();
											fprintf(stderr, "send: %u\n", ret);
										}

										if (ret == 0 && msg.OutputBufferSize > 0) {
											bytesReceived = send(acceptSocket, (char *)outputBuffer, msg.OutputBufferSize, 0);
											if (bytesReceived != msg.OutputBufferSize) {
												ret = WSAGetLastError();
												fprintf(stderr, "send: %u\n", ret);
											}
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

							fprintf(stderr, "recv: %u\n", ret);
						}
					} while (ret == 0);

					DevConn_Disconnect();
				}
			} else {
				ret = WSAGetLastError();
				fprintf(stderr, "setsockopt: %u\n", ret);
			}

			shutdown(acceptSocket, SD_BOTH);
			closesocket(acceptSocket);
		} else {
			ret = WSAGetLastError();
			fprintf(stderr, "accept: %u\n", ret);
		}
	} while (1);
DestroySocket:
	closesocket(listenSocket);
Exit:
	return ret;
}




int main(int argc, char **argv)
{
	int ret = 0;
	WSADATA wsaData;
	uint32_t wVersionRequested = MAKEWORD(2, 2);
	ADDRINFOA hints;
	PADDRINFOA addrs = NULL;

	ret = WSAStartup(wVersionRequested, &wsaData);
	if (argc != 3) {
		ret = -1;
		fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
		goto Exit;
	}

	if (ret != 0) {
		ret = WSAGetLastError();
		fprintf(stderr, "[ERROR]: WSAStartup: %u\n", ret);
		goto Exit;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	ret = getaddrinfo(argv[1], argv[2], &hints, &addrs);
	if (ret != 0) {
		ret = WSAGetLastError();
		fprintf(stderr, "getaddrinfo: %u\n", ret);
		goto Cleanup;
	}

	ret = _Listener(addrs);
	freeaddrinfo(addrs);
Cleanup:
	WSACleanup();
Exit:
	return ret;
}
