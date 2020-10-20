
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "request.h"
#include "irpmondll.h"



static volatile BOOL _terminate = FALSE;


static BOOL WINAPI _CtrlHandler(DWORD fdwCtrlType)
{
	BOOL ret = FALSE;

	switch (fdwCtrlType) {
		case CTRL_CLOSE_EVENT:
		case CTRL_C_EVENT:
			_terminate = TRUE;
			ret = TRUE;
			break;
		default:
			break;
	}

	return ret;
}


int main(int argc, char* argv[])
{
	int ret = 0;
	IRPMON_INIT_INFO initInfo;
	HANDLE driverHandle = NULL;
	HANDLE deviceHandle = NULL;
	DRIVER_MONITOR_SETTINGS driverSettings;
	PREQUEST_HEADER buffer = NULL;
	DWORD bufferSize = 0;
	IRPMNDRV_SETTINGS globalSettings;

	if (!SetConsoleCtrlHandler(_CtrlHandler, TRUE)) {
		ret = GetLastError();
		fprintf(stderr, "[ERROR]: Unable to register Control Handler: %u\n", ret);
		return ret;
	}

	memset(&initInfo, 0, sizeof(initInfo));
	// We are connecting to driver instance running
	// this computer. Since we specify no device name,
	// default one will be used.
	fprintf(stderr, "[INFO]: Initializing the library...\n");
	initInfo.ConnectorType = ictDevice;
	ret = IRPMonDllInitialize(&initInfo);
	if (ret == 0) {
		memset(&driverSettings, 0, sizeof(driverSettings));
		// Watch for AddDevice, IRPs, IRP completions, FastIOs and driver unload
		driverSettings.MonitorAddDevice = TRUE;
		driverSettings.MonitorIRP = TRUE;
		driverSettings.MonitorIRPCompletion = TRUE;
		driverSettings.MonitorUnload = TRUE;
		// Let's capture also data specific to each request.
		driverSettings.MonitorData = TRUE;
		// Since we plan to monitor only one device, devices
		// appearing after we "go to town" do not interest us.
		// StartIo does not interest us too.
		// Also, FastIO is not used with keyboard drivers,
		// so let's skip it too.
		driverSettings.MonitorFastIo = FALSE;
		driverSettings.MonitorNewDevices = FALSE;
		driverSettings.MonitorStartIo = FALSE;
		// We are interested in all types of IRPs
		for (size_t i = 0; i < sizeof(driverSettings.IRPSettings) / sizeof(driverSettings.IRPSettings[0]); ++i)
			driverSettings.IRPSettings[i] = TRUE;
		
		fprintf(stderr, "[INFO]: Hooking the Keyboard Class driver...\n");
		ret = IRPMonDllHookDriver(L"\\Driver\\kbdclass", &driverSettings, FALSE, &driverHandle, NULL);
		if (ret == ERROR_ALREADY_EXISTS) {
			ULONG count = 0;
			PHOOKED_DRIVER_UMINFO driverHookInfo = NULL;
			PHOOKED_DRIVER_UMINFO tmp = NULL;

			fprintf(stderr, "[WARNING]: Driver already hooked. Let's unhook it first\n");
			ret = IRPMonDllDriverHooksEnumerate(&driverHookInfo, &count);
			if (ret == 0) {
				ret = ERROR_FILE_NOT_FOUND;
				tmp = driverHookInfo;
				for (size_t i = 0; i < count; ++i) {
					if (tmp->DriverName != NULL && wcsicmp(tmp->DriverName, L"\\Driver\\kbdclass") == 0) {
						fprintf(stderr, "[INFO]: Found (ID 0x%p)\n", tmp->ObjectId);
						ret = IRPMonDllOpenHookedDriver(tmp->ObjectId, &driverHandle);
						if (ret == 0) {
							ret = IRPMonDllUnhookDriver(driverHandle);
							if (ret != 0)
								fprintf(stderr, "[ERROR]: Unable to unhook the driver: %u\n", ret);

							IRPMonDllCloseHookedDriverHandle(driverHandle);
						} else fprintf(stderr, "[ERROR]: Unable to get hook driver handle: %u\n", ret);

						break;
					}

					++tmp;
				}

				IRPMonDllDriverHooksFree(driverHookInfo, count);
			} else fprintf(stderr, "[ERROR]: Unable to get list of hooked drivers: %u\n", ret);

			if (ret == 0) {
				ret = IRPMonDllHookDriver(L"\\Driver\\kbdclass", &driverSettings, FALSE, &driverHandle, NULL);
				if (ret != 0)
					fprintf(stderr, "[ERROR]: Unable to hook the driver %u\n", ret);
			}
		} else if (ret != 0)
			fprintf(stderr, "[ERROR]: Error %u\n", ret);

		if (ret == 0) {
			fprintf(stderr, "[INFO]: Hooking the primary keyboard device...\n");
			ret = IRPMonDllHookDeviceByName(L"\\Device\\KeyboardClass0", &deviceHandle, NULL);
			if (ret == 0) {
				// Attempt to chan ge the global driver settings in order
				// not to get information about all drivers and devices and running processed
				// after connecting to the request queue.
				ret = IRPMonDllSettingsQuery(&globalSettings);
				if (ret == 0) {
					globalSettings.DriverSnapshotOnConnect = FALSE;
					globalSettings.ProcessEmulateOnConnect = FALSE;
					ret = IRPMonDllSettingsSet(&globalSettings, FALSE);
					if (ret != 0)
						fprintf(stderr, "[WARNING]: Unable to alter global driver settings: %u\n", ret);
				} else fprintf(stderr, "[WARNING]: Unable to query global driver settings: %u\n", ret);

				// Now, everything is set. Let's tell the IRPMon driver
				// to start actually monitoring the requests.
				fprintf(stderr, "[INFO]: Starting to watch for requests...\n");
				ret = IRPMonDllDriverStartMonitoring(driverHandle);
				if (ret == 0) {
					// Connect to the request queue.
					fprintf(stderr, "[INFO]: Connecting to the request queue...\n");
					ret = IRPMonDllConnect();
					if (ret == 0) {
						fprintf(stderr, "[INFO]: We are now set! Press CTRL+C to stop monitoring and exit the application peacefully\n");
						while (!_terminate) {
							// Attempt to get a request from the queue
							ret = IRPMonDllGetRequest(buffer, bufferSize);
							switch (ret) {
								case ERROR_SUCCESS: {
									PREQUEST_HEADER request = NULL;

									// Since multiple requests can be returned
									// in one call to IRPMonDllGetRequest, let's
									// carefully examine all of them.
									request = buffer;
									do {
										PREQUEST_IRP irp = NULL;
										PREQUEST_IRP_COMPLETION irpc = NULL;
										PREQUEST_ADDDEVICE ad = NULL;
										PREQUEST_UNLOAD du = NULL;

										switch (request->Type) {
											case ertIRP: 
												irp = CONTAINING_RECORD(request, REQUEST_IRP, Header);
												fprintf(stderr, "[IRP]: irp=0x%p, major=%u, minor=%u, datasize=%zu\n", irp->IRPAddress, irp->MajorFunction, irp->MinorFunction, irp->DataSize);
												break;
											case ertIRPCompletion:
												irpc = CONTAINING_RECORD(request, REQUEST_IRP_COMPLETION, Header);
												fprintf(stderr, "[IRPCOMPLETE]: irp=0x%p, status=0x%lx, info=%zu, datasize=%zu\n", irpc->IRPAddress, irpc->CompletionStatus, irpc->CompletionInformation, irpc->DataSize);
												break;
											case ertAddDevice:
												ad = CONTAINING_RECORD(request, REQUEST_ADDDEVICE, Header);
												fprintf(stderr, "[ADDDEVICE]: driver=0x%p, device=0x%p\n", ad->Header.Driver, ad->Header.Device);
												break;
											case ertDriverUnload:
												du = CONTAINING_RECORD(request, REQUEST_UNLOAD, Header);
												fprintf(stderr, "[UNLOAD]: driver=0x%p", du->Header.Driver);
												break;
											default:
												fprintf(stderr, "[REQUEST]: Type %u\n", request->Type);
												break;
										}

										// This value is nonzero when the buffer returned
										// by the driver contains at least one request behind
										// this one. If the REQUEST_FLAG_NEXT_AVAILABLE is set
										// in the request->Flags, more requests are still present
										// in the request queue in the kernel.
										if (request->Entry.Flink == NULL)
											break;

										request = (PREQUEST_HEADER)((unsigned char *)request + RequestGetSize(request));
									} while (TRUE);
								} break;
								case ERROR_INSUFFICIENT_BUFFER: {
									PREQUEST_HEADER tmp = NULL;

									// Our buffer is not large enough, let's resize it!
									fprintf(stderr, "[WARNING]: Buffer of size %u is not enough, enlarging to %u\n", bufferSize, bufferSize*2 + 128);
									bufferSize = bufferSize*2 + 128;
									tmp = realloc(buffer, bufferSize);
									if (tmp == NULL) {
										fprintf(stderr, "[ERROR]: Buffer allocation failed with errno of %u\n", errno);
										_terminate = TRUE;
										continue;
									}

									buffer = tmp;
								} break;
								case ERROR_NO_MORE_ITEMS:
									// The queue is empty. Well, this is very common
									// for keyboard devices. Let's just wait.
									fputc('.', stderr);
									Sleep(1000);
									break;
								default:
									// An unexpected error occurred.
									// Well, no software is perfect, right?
									fprintf(stderr, "[ERROR]: Unable to get request: %u\n", ret);
									_terminate = TRUE;
									break;
							}
						}

						// Let's free the buffer. This will also work
						// when the buffer is NUULL (i.e. have never been allocated).
						free(buffer);
						fprintf(stderr, "[INFO]: Disconnecting from the request queue\n");
						ret = IRPMonDllDisconnect();
						if (ret != 0)
							fprintf(stderr, "[WARNING]: Error %u\n", ret);
					} else fprintf(stderr, "[ERROR]: Error %u\n", ret);
					
					fprintf(stderr, "[INFO]: Stopping the monitoring\n");
					ret = IRPMonDllDriverStopMonitoring(driverHandle);
					if (ret != 0)
						fprintf(stderr, "[WARNING]: Error %u\n", ret);
				} else fprintf(stderr, "[ERROR]: Error %u\n", ret);

				fprintf(stderr, "[INFO]: Unhooking the primary keyboard device\n");
				ret = IRPMonDllUnhookDevice(deviceHandle);
				if (ret != 0)
					fprintf(stderr, "[WARNING]: Error %u\n", ret);
			} else fprintf(stderr, "[ERROR]: Error %u\n", ret);

			fprintf(stderr, "[INFO]: Unhooking the Keyboard Class driver\n");
			ret = IRPMonDllUnhookDriver(driverHandle);
			if (ret != 0)
				fprintf(stderr, "[WARNING]: Error %u\n", ret);
		}

		fprintf(stderr, "[INFO]: Cleanup the library\n");
		IRPMonDllFinalize();
	} else fprintf(stderr, "[ERROR]: Error %u\n", ret);

	SetConsoleCtrlHandler(_CtrlHandler, FALSE);

	return ret;
}
