
#include <windows.h>
#include <winternl.h>
#include "debug.h"
#include "ioctls.h"
#include "kernel-shared.h"
#include "general-types.h"
#include "irpmondll-types.h"
#include "driver-com.h"


/************************************************************************/
/*               TYPE DEFINITIONS                                       */
/************************************************************************/

typedef NTSTATUS (NTAPI RTLSTRINGFROMGUID)(GUID *Guid, PUNICODE_STRING GuidString);
typedef VOID(WINAPI RTLFREEUNICODESTRING)(PUNICODE_STRING String);


/************************************************************************/
/*                           GLOBAL VARIABLES                           */
/************************************************************************/


static BOOLEAN _initialized = FALSE;
static PIRPMON_DRIVER_COMM_INTERFACE _dcInterface = NULL;
static HMODULE _hConnector = NULL;

static RTLSTRINGFROMGUID *_RtlStringFromGuid = NULL;
static RTLFREEUNICODESTRING *_RtlFreeUnicodeString = NULL;


/************************************************************************/
/*                          HELPER ROUTINES                             */
/************************************************************************/


static PIRPMON_DRIVER_INFO _DriverInfoAlloc(PVOID DriverObject, PWCHAR Drivername, ULONG DriverNameLen, ULONG DeviceCount)
{
	PIRPMON_DRIVER_INFO ret = NULL;
	SIZE_T size = sizeof(IRPMON_DRIVER_INFO);
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; DriverName=0x%p; DriverNameLen=%u; DeviceCount=%u", DriverObject, Drivername, DriverNameLen, DeviceCount);

	size += DriverNameLen + sizeof(WCHAR) + DeviceCount*sizeof(PIRPMON_DEVICE_INFO);
	ret = (PIRPMON_DRIVER_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	if (ret != NULL) {
		ret->DriverObject = DriverObject;
		ret->DeviceCount = DeviceCount;
		ret->DriverName = (PWCHAR)(ret + 1);
		memcpy(ret->DriverName, Drivername, DriverNameLen);
		ret->DriverName[DriverNameLen / sizeof(WCHAR)] = L'\0';
		ret->Devices = (PIRPMON_DEVICE_INFO *)((PUCHAR)ret->DriverName + DriverNameLen + sizeof(WCHAR));
	}

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


static VOID _DriverInfoFree(PIRPMON_DRIVER_INFO Info)
{
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	HeapFree(GetProcessHeap(), 0, Info);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static PIRPMON_DEVICE_INFO _DeviceInfoAlloc(PVOID DeviceObject, PVOID AttachedDevice, PWCHAR DeviceName, ULONG DeviceNameLen)
{
	PIRPMON_DEVICE_INFO ret = NULL;
	SIZE_T size = sizeof(IRPMON_DEVICE_INFO);
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; AttachedDevice=0x%p; DeviceName=0x%p; DeviceNameLen=%u", DeviceObject, AttachedDevice, DeviceName, DeviceNameLen);

	size += DeviceNameLen + sizeof(WCHAR);
	ret = (PIRPMON_DEVICE_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	if (ret != NULL) {
		ret->DeviceObject = DeviceObject;
		ret->AttachedDevice = AttachedDevice;
		ret->Name = (PWCHAR)(ret + 1);
		memcpy(ret->Name, DeviceName, DeviceNameLen);
		ret->Name[DeviceNameLen / sizeof(WCHAR)] = L'\0';
	}

	DEBUG_EXIT_FUNCTION("0x%x", ret);
	return ret;
}


static VOID _DeviceInfoFree(PIRPMON_DEVICE_INFO Info)
{
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	HeapFree(GetProcessHeap(), 0, Info);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static PWCHAR _CopyString(PWCHAR Str)
{
	PWCHAR ret = NULL;
	SIZE_T len = (Str != NULL ? wcslen(Str)*sizeof(WCHAR) : 0);

	ret = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + sizeof(WCHAR));
	if (ret != NULL) {
		memcpy(ret, Str, len);
		ret[len / sizeof(WCHAR)] = L'\0';
	}

	return ret;
}


static DWORD _ObjectOpen(EHandletype ObjectType, PVOID ID, PHANDLE Handle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMONDRV_HOOK_OPEN_INPUT input;
	IOCTL_IRPMONDRV_HOOK_OPEN_OUTPUT output;
	DEBUG_ENTER_FUNCTION("ObjectType=%u; ID=0x%p; Handle=0x%p", ObjectType, ID, Handle);

	input.ObjectId = ID;
	input.ObjectType = ObjectType;
	ret = _SynchronousOtherIOCTL(IOCTL_IRPMONDRV_HOOK_OPEN, &input, sizeof(input), &output, sizeof(output));
	if (ret == ERROR_SUCCESS)
		*Handle = output.Handle;

	DEBUG_EXIT_FUNCTION("%u, *Handle=0x%p", ret, *Handle);
	return ret;
}


static DWORD _ObjectClose(EHandletype ObjectType, HANDLE Handle)
{
	DWORD ret = ERROR_SUCCESS;
	IOCTL_IRPMONDRV_HOOK_CLOSE_INPUT input;
	DEBUG_ENTER_FUNCTION("ObjectType=%u; Handle=0x%p", ObjectType, Handle);

	input.Handle = Handle;
	input.ObjectType = ObjectType;
	ret = _SynchronousWriteIOCTL(IOCTL_IRPMONDRV_HOOK_CLOSE, &input, sizeof(input));

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

/************************************************************************/
/*                          PUBLIC ROUTINES                             */
/************************************************************************/

DWORD DriverComSnapshotRetrieve(PIRPMON_DRIVER_INFO **DriverInfo, PULONG InfoCount)
{
	DWORD outputBufferLength = 512;
	PVOID outputBuffer = NULL;
	ULONG tmpInfoArrayCount = 0;
	PIRPMON_DRIVER_INFO *tmpInfoArray = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("DriverInfo=0x%p; InfoCount=0x%p", DriverInfo, InfoCount);

	ret = _SynchronousVariableOutputIOCTL(IOCTL_IRPMNDRV_GET_DRIVER_DEVICE_INFO, NULL, 0, outputBufferLength, &outputBuffer, &outputBufferLength);
	if (ret == ERROR_SUCCESS) {
		PUCHAR tmpBuffer = (PUCHAR)outputBuffer;

		tmpInfoArrayCount = *(PULONG)tmpBuffer;
		tmpBuffer += sizeof(ULONG);
		tmpInfoArray = (PIRPMON_DRIVER_INFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PIRPMON_DRIVER_INFO)*tmpInfoArrayCount);
		if (tmpInfoArray != NULL) {
			for (ULONG i = 0; i < tmpInfoArrayCount; ++i) {
				PVOID driverObject = NULL;
				ULONG deviceCount = 0;
				ULONG driverNameLen = 0;
				
				driverObject = *(PVOID *)tmpBuffer;
				tmpBuffer += sizeof(driverObject);
				deviceCount = *(PULONG)tmpBuffer;
				tmpBuffer += sizeof(deviceCount);
				driverNameLen = *(PULONG)tmpBuffer;
				tmpBuffer += sizeof(driverNameLen);
				tmpInfoArray[i] = _DriverInfoAlloc(driverObject, (PWCHAR)tmpBuffer, driverNameLen, deviceCount);
				if (tmpInfoArray[i] != NULL) {
					PIRPMON_DRIVER_INFO driverInfo = tmpInfoArray[i];
					
					tmpBuffer += driverNameLen;
					for (ULONG j = 0; j < deviceCount; ++j) {
						PVOID deviceObject = NULL;
						PVOID attachedDevice = NULL;
						ULONG deviceNameLen = 0;
						
						deviceObject = *(PVOID *)tmpBuffer;
						tmpBuffer += sizeof(deviceObject);
						attachedDevice = *(PVOID *)tmpBuffer;
						tmpBuffer += sizeof(attachedDevice);
						deviceNameLen = *(PULONG)tmpBuffer;
						tmpBuffer += sizeof(deviceNameLen);
						driverInfo->Devices[j] = _DeviceInfoAlloc(deviceObject, attachedDevice, (PWCHAR)tmpBuffer, deviceNameLen);
						if (driverInfo->Devices[j] == NULL) {
							ret = ERROR_NOT_ENOUGH_MEMORY;
							for (ULONG k = 0; k < j; ++k)
								_DeviceInfoFree(driverInfo->Devices[k]);

							break;
						}

						tmpBuffer += deviceNameLen;
					}
				} else ret = ERROR_NOT_ENOUGH_MEMORY;
			
				if (ret != ERROR_SUCCESS) {
					for (ULONG k = 0; k < i; ++k) {
						PIRPMON_DRIVER_INFO driverInfo = tmpInfoArray[k];
					
						for (ULONG l = 0; l < driverInfo->DeviceCount; ++l)
							_DeviceInfoFree(driverInfo->Devices[l]);

						_DriverInfoFree(driverInfo);
					}

					break;
				}
			}
		
			if (ret == ERROR_SUCCESS) {
				*DriverInfo = tmpInfoArray;
				*InfoCount = tmpInfoArrayCount;
			}

			if (ret != ERROR_SUCCESS)
				HeapFree(GetProcessHeap(), 0, tmpInfoArray);
		} else ret = GetLastError();

		HeapFree(GetProcessHeap(), 0, outputBuffer);
	}

	DEBUG_EXIT_FUNCTION("%u, *DriverInfo=0x%p, *InfoCount=%u", ret, *DriverInfo, *InfoCount);
	return ret;
}

VOID DriverComSnapshotFree(PIRPMON_DRIVER_INFO *DriverInfo, ULONG Count)
{
	DEBUG_ENTER_FUNCTION("DriverInfo=0x%p; Count=%u", DriverInfo, Count);

	for (ULONG i = 0; i < Count; ++i) {
		PIRPMON_DRIVER_INFO drvInfo = DriverInfo[i];
	
		for (ULONG j = 0; j < drvInfo->DeviceCount; ++j)
			_DeviceInfoFree(drvInfo->Devices[j]);

		_DriverInfoFree(drvInfo);
	}

	HeapFree(GetProcessHeap(), 0, DriverInfo);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

DWORD DriverComHookDriver(const wchar_t *DriverName, const DRIVER_MONITOR_SETTINGS *MonitorSettings, BOOLEAN DeviceExtensionHook, PHANDLE HookHandle, PVOID *ObjectId)
{
	DWORD ret = ERROR_GEN_FAILURE;
	ULONG nameLen = 0;
	PIOCTL_IRPMNDRV_HOOK_DRIVER_INPUT input = NULL;
	IOCTL_IRPMNDRV_HOOK_DRIVER_OUTPUT output;
	DEBUG_ENTER_FUNCTION("DriverName=\"%ls\"; MonitorSettings=0x%p; DeviceExtensionHook=%u; HookHandle=0x%p; ObjectId=0x%p", DriverName, MonitorSettings, DeviceExtensionHook, HookHandle, ObjectId);

	nameLen = (ULONG)wcslen(DriverName)*sizeof(wchar_t);
	input = (PIOCTL_IRPMNDRV_HOOK_DRIVER_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IOCTL_IRPMNDRV_HOOK_DRIVER_INPUT) + nameLen);
	if (input != NULL) {
		input->DriverNameLength = nameLen;
		input->MonitorSettings = *MonitorSettings;
		input->DeviceExtensionHook = DeviceExtensionHook;
		memcpy(input + 1, DriverName, nameLen);
		ret = _SynchronousOtherIOCTL(IOCTL_IRPMNDRV_HOOK_DRIVER, input, sizeof(IOCTL_IRPMNDRV_HOOK_DRIVER_INPUT) + nameLen, &output, sizeof(output));
		if (ret == ERROR_SUCCESS) {
			*HookHandle = output.HookHandle;
			if (ObjectId != NULL)
				*ObjectId = output.ObjectId;
		}

		HeapFree(GetProcessHeap(), 0, input);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	DEBUG_EXIT_FUNCTION("0x%x, *Hookandle=0x%p", ret, *HookHandle);
	return ret;
}

DWORD DriverComHookedDriverSetInfo(HANDLE Driverhandle, const DRIVER_MONITOR_SETTINGS *Settings)
{
	IOCTL_IRPMNDRV_HOOK_DRIVER_SET_INFO_INPUT input;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Driverhandle=0x%p; Settings=0x%p", Driverhandle, Settings);

	input.DriverHandle = Driverhandle;
	input.Settings = *Settings;
	ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_HOOK_DRIVER_SET_INFO, &input, sizeof(input));

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComHookedDriverGetInfo(HANDLE Driverhandle, PDRIVER_MONITOR_SETTINGS Settings, PBOOLEAN MonitoringEnabled)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO_INPUT input;
	IOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO_OUTPUT output;
	DEBUG_ENTER_FUNCTION("DriverHandle=0x%p; Settings=0x%p; MonitoringEnabled=0x%p", Driverhandle, Settings, MonitoringEnabled);

	input.DriverHandle = Driverhandle;
	ret = _SynchronousOtherIOCTL(IOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO, &input, sizeof(input), &output, sizeof(output));
	if (ret == ERROR_SUCCESS) {
		*Settings = output.Settings;
		*MonitoringEnabled = output.MonitoringEnabled;
	}

	DEBUG_EXIT_FUNCTION("%u, *MonitoringEnabled=%u", ret, *MonitoringEnabled);
	return ret;
}


DWORD DriverComHookedDriverActivate(HANDLE DriverHandle, BOOLEAN Activate)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_HOOK_DRIVER_MONITORING_CHANGE_INPUT input;
	DEBUG_ENTER_FUNCTION("DriverHandle=0x%p; Activate=%u", DriverHandle, Activate);

	input.DriverHandle = DriverHandle;
	input.EnableMonitoring = Activate;
	ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_HOOK_DRIVER_MONITORING_CHANGE, &input, sizeof(input));

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComUnhookDriver(HANDLE HookHandle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_UNHOOK_DRIVER_INPUT input;
	DEBUG_ENTER_FUNCTION("HookHandle=0x%p", HookHandle);

	input.HookHandle = HookHandle;
	ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_UNHOOK_DRIVER, &input, sizeof(input));

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComConnect(void)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = _SynchronousNoIOIOCTL(IOCTL_IRPMNDRV_CONNECT);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComDisconnect(VOID)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = _SynchronousNoIOIOCTL(IOCTL_IRPMNDRV_DISCONNECT);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComGetRequest(PREQUEST_HEADER Request, DWORD Size)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Request=0x%p; Size=%u", Request, Size);

	ret = _SynchronousReadIOCTL(IOCTL_IRPMNDRV_GET_RECORD, Request, Size);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComQueueClear(void)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = _SynchronousNoIOIOCTL(IOCTL_IRPMNDRV_QUEUE_CLEAR);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComHookDeviceByName(const wchar_t *DeviceName, PHANDLE HookHandle, PVOID *ObjectId)
{
	DWORD ret = ERROR_GEN_FAILURE;
	ULONG nameLen = 0;
	PIOCTL_IRPMNDRV_HOOK_ADD_DEVICE_INPUT input = NULL;
	IOCTL_IRPMNDRV_HOOK_ADD_DEVICE_OUTPUT output;
	DEBUG_ENTER_FUNCTION("DeviceName=\"%S\"; HookHandle=0x%p; ObjectId=0x%p", DeviceName, HookHandle, ObjectId);

	nameLen = (ULONG)wcslen(DeviceName)*sizeof(WCHAR);
	input = (PIOCTL_IRPMNDRV_HOOK_ADD_DEVICE_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IOCTL_IRPMNDRV_HOOK_ADD_DEVICE_INPUT) + nameLen);
	if (input != NULL) {
		input->HookByName = TRUE;
		input->IRPSettingsSpecified = FALSE;
		input->FastIoSettingsSpecified = FALSE;
		input->DeviceAddress = NULL;
		input->DeviceNameLength = nameLen;
		memcpy(input + 1, DeviceName, nameLen);
		ret = _SynchronousOtherIOCTL(IOCTL_IRPMNDRV_HOOK_ADD_DEVICE, input, sizeof(IOCTL_IRPMNDRV_HOOK_ADD_DEVICE_INPUT) + nameLen, &output, sizeof(output));
		if (ret == ERROR_SUCCESS) {
			*HookHandle = output.DeviceHandle;
			if (ObjectId != NULL)
				*ObjectId = output.ObjectId;
		}

		HeapFree(GetProcessHeap(), 0, input);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	DEBUG_EXIT_FUNCTION("%u, *HookHandle=0x%p", ret, *HookHandle);
	return ret;
}

DWORD DriverComHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle, PVOID *ObjectId)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_HOOK_ADD_DEVICE_INPUT input;
	IOCTL_IRPMNDRV_HOOK_ADD_DEVICE_OUTPUT output;
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; HookHandle=0x%p; ObjectId=0x%p", DeviceObject, HookHandle, ObjectId);

	input.HookByName = FALSE;
	input.FastIoSettingsSpecified = FALSE;
	input.IRPSettingsSpecified = FALSE;
	input.DeviceNameLength = 0;
	input.DeviceAddress = DeviceObject;
	ret = _SynchronousOtherIOCTL(IOCTL_IRPMNDRV_HOOK_ADD_DEVICE, &input, sizeof(input), &output, sizeof(output));
	if (ret == ERROR_SUCCESS) {
		*HookHandle = output.DeviceHandle;
		if (ObjectId != NULL)
			*ObjectId = output.ObjectId;
	}

	DEBUG_EXIT_FUNCTION("%u, *HookHandle=0x%p", ret, *HookHandle);
	return ret;
}

DWORD DriverComDeviceGetInfo(HANDLE DeviceHandle, PUCHAR IRPSettings, PUCHAR FastIoSettings, PBOOLEAN MonitoringEnabled)
{
	IOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO_INPUT input;
	IOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO_OUTPUT output;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("DeviceHandle=0x%p; IRPSettings=0x%p; FastIoSettings=0x%p; MonitoringEnabled=0x%p", DeviceHandle, IRPSettings, FastIoSettings, MonitoringEnabled);

	input.DeviceHandle = DeviceHandle;
	ret = _SynchronousOtherIOCTL(IOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO, &input, sizeof(input), &output, sizeof(output));
	if (ret == ERROR_SUCCESS) {
		*MonitoringEnabled = output.MonitoringEnabled;
		if (IRPSettings != NULL)
			memcpy(IRPSettings, output.IRPSettings, sizeof(output.IRPSettings));

		if (FastIoSettings != NULL)
			memcpy(FastIoSettings, output.FastIoSettings, sizeof(output.FastIoSettings));
	}

	DEBUG_EXIT_FUNCTION("%u, *MonitoringEnabled=%u", ret, *MonitoringEnabled);
	return ret;
}

DWORD DriverComDeviceSetInfo(HANDLE DeviceHandle, PUCHAR IRPSettings, PUCHAR FastIoSettings, BOOLEAN MonitoringEnabled)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_HOOK_DEVICE_SET_INFO_INPUT input;
	DEBUG_ENTER_FUNCTION("DeviceHandle=0x%p; IRPSettings=0x%p; FastIoSettings=0x%p; MonitoringEnabled=%u", DeviceHandle, IRPSettings, FastIoSettings, MonitoringEnabled);

	memset(&input, 0, sizeof(input));
	input.DeviceHandle = DeviceHandle;
	if (IRPSettings != NULL) {
		input.IRPSettingsSpecified = TRUE;
		memcpy(input.IRPSettings, IRPSettings, sizeof(input.IRPSettings));
	}

	if (FastIoSettings != NULL) {
		input.FastIoSettingsSpecified = TRUE;
		memcpy(input.FastIoSettings, FastIoSettings, sizeof(input.FastIoSettings));
	}

	input.MonitoringEnabled = MonitoringEnabled;
	ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_HOOK_DEVICE_SET_INFO, &input, sizeof(input));

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComUnhookDevice(HANDLE HookHandle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_HOOK_REMOVE_DEVICE_INPUT input;
	DEBUG_ENTER_FUNCTION("HookHandle=0x%p", HookHandle);

	input.DeviceHandle = HookHandle;
	ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_HOOK_REMOVE_DEVICE, &input, sizeof(input));

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComHookedObjectsEnumerate(PHOOKED_DRIVER_UMINFO *Info, PULONG Count)
{
	ULONG hoLen = 512;
	PHOOKED_OBJECTS_INFO hookedObjects = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Info=0x%p; Count=0x%p", Info, Count);

	ret = _SynchronousVariableOutputIOCTL(IOCTL_IRPMONDRV_HOOK_GET_INFO, NULL, 0, hoLen, (PVOID *)&hookedObjects, &hoLen);
	if (ret == ERROR_SUCCESS) {
		PHOOKED_DRIVER_UMINFO tmpInfo = NULL;

		if (hookedObjects->NumberOfHookedDrivers > 0) {
			tmpInfo = (PHOOKED_DRIVER_UMINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HOOKED_DRIVER_UMINFO)*hookedObjects->NumberOfHookedDrivers);
			if (tmpInfo != NULL) {
				PHOOKED_DRIVER_INFO driverEntry = (PHOOKED_DRIVER_INFO)(hookedObjects + 1);
				PHOOKED_DRIVER_UMINFO umDriverEntry = tmpInfo;

				ret = ERROR_SUCCESS;
				for (SIZE_T i = 0; i < hookedObjects->NumberOfHookedDrivers; ++i) {
					umDriverEntry->ObjectId = driverEntry->ObjectId;
					umDriverEntry->DriverObject = driverEntry->DriverObject;
					umDriverEntry->DeviceExtensionHooks = driverEntry->DeviceExtensionHooks;
					umDriverEntry->MonitoringEnabled = driverEntry->MonitoringEnabled;
					umDriverEntry->MonitorSettings = driverEntry->MonitorSettings;
					umDriverEntry->DriverNameLen = driverEntry->DriverNameLen - sizeof(WCHAR);
					umDriverEntry->DriverName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, driverEntry->DriverNameLen);
					if (umDriverEntry->DriverName != NULL) {
						memcpy(umDriverEntry->DriverName, driverEntry->DriverName, driverEntry->DriverNameLen);
						umDriverEntry->NumberOfHookedDevices = driverEntry->NumberOfHookedDevices;
						umDriverEntry->HookedDevices = (umDriverEntry->NumberOfHookedDevices > 0) ?
							(PHOOKED_DEVICE_UMINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, umDriverEntry->NumberOfHookedDevices*sizeof(HOOKED_DEVICE_UMINFO)) :
							NULL;

						if (umDriverEntry->NumberOfHookedDevices == 0 || umDriverEntry->HookedDevices != NULL) {
							PHOOKED_DEVICE_INFO deviceEntry = (PHOOKED_DEVICE_INFO)((PUCHAR)driverEntry + driverEntry->EntrySize);
							PHOOKED_DEVICE_UMINFO umDeviceEntry = umDriverEntry->HookedDevices;

							for (SIZE_T j = 0; j < umDriverEntry->NumberOfHookedDevices; ++j) {
								umDeviceEntry->ObjectId = deviceEntry->ObjectId;
								umDeviceEntry->DeviceObject = deviceEntry->DeviceObject;
								memcpy(umDeviceEntry->FastIoSettings, deviceEntry->FastIoSettings, sizeof(umDeviceEntry->FastIoSettings));
								memcpy(umDeviceEntry->IRPSettings, deviceEntry->IRPSettings, sizeof(umDeviceEntry->IRPSettings));
								umDeviceEntry->MonitoringEnabled = deviceEntry->MonitoringEnabled;
								umDeviceEntry->DeviceNameLen = deviceEntry->DeviceNameLen - sizeof(WCHAR);
								umDeviceEntry->DeviceName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, deviceEntry->DeviceNameLen);
								if (umDeviceEntry->DeviceName != NULL)
									memcpy(umDeviceEntry->DeviceName, deviceEntry->DeviceName, deviceEntry->DeviceNameLen);
								else ret = GetLastError();

								if (ret != ERROR_SUCCESS) {
									--umDeviceEntry;
									for (SIZE_T k = 0; k < j; ++k) {
										HeapFree(GetProcessHeap(), 0, umDeviceEntry->DeviceName);
										--umDeviceEntry;
									}

									break;
								}

								++umDeviceEntry;
								deviceEntry = (PHOOKED_DEVICE_INFO)((PUCHAR)deviceEntry + deviceEntry->EntrySize);
							}

							driverEntry = (PHOOKED_DRIVER_INFO)deviceEntry;
							if (ret != ERROR_SUCCESS) {
								if (umDriverEntry->HookedDevices != NULL)
									HeapFree(GetProcessHeap(), 0, umDriverEntry->HookedDevices);
							}
						} else ret = GetLastError();

						if (ret != ERROR_SUCCESS)
							HeapFree(GetProcessHeap(), 0, umDriverEntry->DriverName);
					} else ret = GetLastError();

					if (ret != ERROR_SUCCESS) {
						--umDriverEntry;
						for (SIZE_T j = 0; j < i; ++j) {
							if (umDriverEntry->NumberOfHookedDevices > 0) {
								ULONG k = 0;
								PHOOKED_DEVICE_UMINFO umDeviceEntry = umDriverEntry->HookedDevices;

								for (k = 0; k < umDriverEntry->NumberOfHookedDevices; ++k) {
									HeapFree(GetProcessHeap(), 0, umDeviceEntry->DeviceName);
									++umDeviceEntry;
								}
							
								HeapFree(GetProcessHeap(), 0, umDriverEntry->HookedDevices);
							}

							HeapFree(GetProcessHeap(), 0, umDriverEntry->DriverName);
							--umDriverEntry;
						}

						break;
					}

					++umDriverEntry;
				}

				if (ret == ERROR_SUCCESS) {
					*Info = tmpInfo;
					*Count = hookedObjects->NumberOfHookedDrivers;
				}

				if (ret != ERROR_SUCCESS)
					HeapFree(GetProcessHeap(), 0, tmpInfo);
			} else ret = GetLastError();
		} else {
			*Info = NULL;
			*Count = 0;
		}

		HeapFree(GetProcessHeap(), 0, hookedObjects);
	}

	DEBUG_EXIT_FUNCTION("%u, *Info=0x%p", ret, *Info);
	return ret;
}

VOID DriverComHookedObjectsFree(PHOOKED_DRIVER_UMINFO Info, ULONG Count)
{
	PHOOKED_DRIVER_UMINFO driverInfo = Info;
	DEBUG_ENTER_FUNCTION("Info=0x%p; Count=%u", Info, Count);

	if (Count > 0) {
		for (ULONG i = 0; i < Count; ++i) {
			if (driverInfo->NumberOfHookedDevices > 0) {
				PHOOKED_DEVICE_UMINFO deviceInfo = driverInfo->HookedDevices;

				for (ULONG j = 0; j < driverInfo->NumberOfHookedDevices; ++j) {
					HeapFree(GetProcessHeap(), 0, deviceInfo->DeviceName);
					++deviceInfo;
				}

				HeapFree(GetProcessHeap(), 0, driverInfo->HookedDevices);
			}

			HeapFree(GetProcessHeap(), 0, driverInfo->DriverName);
			++driverInfo;
		}

		HeapFree(GetProcessHeap(), 0, Info);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

DWORD DriverComDriverOpen(PVOID ID, PHANDLE Handle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("ID=0x%p; Handle=0x%p", ID, Handle);

	ret = _ObjectOpen(ehtDriver, ID, Handle);

	DEBUG_EXIT_FUNCTION("%u, *Handle=0x%p", ret, *Handle);
	return ret;
}

DWORD DriverComDriverHandleClose(HANDLE Handle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Handle=0x%p", Handle);

	ret = _ObjectClose(ehtDriver, Handle);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

DWORD DriverComDeviceOpen(PVOID ID, PHANDLE Handle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("ID=0x%p; Handle=0x%p", ID, Handle);

	ret = _ObjectOpen(ehtDevice, ID, Handle);

	DEBUG_EXIT_FUNCTION("%u, *Handle=0x%p", ret, *Handle);
	return ret;
}

DWORD DriverComDeviceHandleClose(HANDLE Handle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Handle=0x%p", Handle);

	ret = _ObjectClose(ehtDevice, Handle);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


/************************************************************************/
/*                     CLASS WATCH                                      */
/************************************************************************/

DWORD DriverComClassWatchRegister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_CLASS_WATCH_REGISTER_INPUT input;
	DEBUG_ENTER_FUNCTION("ClassGuid=\"%S\"; UpperFilter=%u; Beginning=%u", ClassGuid, UpperFilter, Beginning);

	if (wcslen(ClassGuid) == sizeof(input.Data.ClassGuidString) / sizeof(WCHAR)) {
		input.Flags = 0;
		if (UpperFilter)
			input.Flags |= CLASS_WATCH_FLAG_UPPERFILTER;

		if (Beginning)
			input.Flags |= CLASS_WATCH_FLAG_BEGINNING;

		memcpy(input.Data.ClassGuidString, ClassGuid, sizeof(input.Data.ClassGuidString));
		ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_CLASS_WATCH_REGISTER, &input, sizeof(input));
	} else ret = ERROR_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DriverComClassWatchUnregister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_CLASS_WATCH_UNREGISTER_INPUT input;
	DEBUG_ENTER_FUNCTION("ClassGuid=\"%S\"; UpperFilter=%u; Beginning=%u", ClassGuid, UpperFilter, Beginning);

	if (wcslen(ClassGuid) == sizeof(input.Data.ClassGuidString) / sizeof(WCHAR)) {
		input.Flags = 0;
		if (UpperFilter)
			input.Flags |= CLASS_WATCH_FLAG_UPPERFILTER;

		if (Beginning)
			input.Flags |= CLASS_WATCH_FLAG_BEGINNING;

		memcpy(input.Data.ClassGuidString, ClassGuid, sizeof(input.Data.ClassGuidString));
		ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_CLASS_WATCH_UNREGISTER, &input, sizeof(input));
	} else ret = ERROR_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DriverComClassWatchEnum(PCLASS_WATCH_RECORD *Array, PULONG Count)
{
	PCLASS_WATCH_RECORD tmpArray = NULL;
	ULONG outputSize = 128;
	PIOCTL_IRPMNDRV_CLASS_WATCH_OUTPUT output = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Array=0x%p; Count=0x%p", Array, Count);

	ret = _SynchronousVariableOutputIOCTL(IOCTL_IRPMNDRV_CLASS_WATCH_ENUM, NULL, 0, outputSize, (PVOID *)&output, &outputSize);
	if (ret == ERROR_SUCCESS) {
		if (output->Count > 0) {
			tmpArray = (PCLASS_WATCH_RECORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, output->Count*sizeof(CLASS_WATCH_RECORD));
			if (tmpArray != NULL) {
				PCLASS_WATCH_RECORD rec = tmpArray;
				PCLASS_WATCH_ENTRY entry = output->Entries;

				for (SIZE_T i = 0; i < output->Count; ++i) {
					UNICODE_STRING uGuid;

					rec->ClassGuid = entry->ClassGuid;
					rec->UpperFilter = (entry->Flags & CLASS_WATCH_FLAG_UPPERFILTER) != 0;
					rec->Beginning = (entry->Flags & CLASS_WATCH_FLAG_BEGINNING) != 0;
					SecureZeroMemory(&uGuid, sizeof(uGuid));
					if (NT_SUCCESS(_RtlStringFromGuid(&rec->ClassGuid, &uGuid))) {
						rec->ClassGuidString = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, uGuid.Length + sizeof(WCHAR));
						if (rec->ClassGuidString != NULL) {
							memcpy(rec->ClassGuidString, uGuid.Buffer, uGuid.Length);
							rec->ClassGuidString[uGuid.Length / sizeof(WCHAR)] = L'\0';
						} else ret = ERROR_NOT_ENOUGH_MEMORY;

						_RtlFreeUnicodeString(&uGuid);
					} else ret = ERROR_NOT_ENOUGH_MEMORY;

					if (ret != ERROR_SUCCESS) {
						for (SIZE_T j = 0; j < i; ++j) {
							--rec;
							HeapFree(GetProcessHeap(), 0, rec->ClassGuidString);
						}


						break;
					}

					++entry;
					++rec;
				}

				if (ret == ERROR_SUCCESS) {
					*Array = tmpArray;
					*Count = output->Count;
				}
			} else ret = ERROR_NOT_ENOUGH_MEMORY;
		} else {
			*Array = NULL;
			*Count = 0;
		}

		HeapFree(GetProcessHeap(), 0, output);
	}

	DEBUG_EXIT_FUNCTION("%u, *Array=0x%p, *Count=%u", ret, *Array, *Count);
	return ret;
}


VOID DriverComClassWatchEnumFree(PCLASS_WATCH_RECORD Array, ULONG Count)
{
	PCLASS_WATCH_RECORD tmp = Array;
	DEBUG_ENTER_FUNCTION("Array=0x%p; Count=%u", Array, Count);

	if (Count > 0) {
		for (SIZE_T i = 0; i < Count; ++i) {
			HeapFree(GetProcessHeap(), 0, tmp->ClassGuidString);
			++tmp;
		}

		HeapFree(GetProcessHeap(), 0, Array);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                   DRIVER NAME WATCH                                  */
/************************************************************************/

DWORD DriverComDriverNameWatchRegister(const wchar_t *DriverName, const DRIVER_MONITOR_SETTINGS *MonitorSettings)
{
	DWORD ret = ERROR_GEN_FAILURE;
	SIZE_T len = 0;
	SIZE_T inputLen = 0;
	PIOCTL_IRPMNDRV_DRIVER_WATCH_REGISTER_INPUT input = NULL;
	DEBUG_ENTER_FUNCTION("DriverName=\"%ls\"; MonitorSettings=0x%p", DriverName, MonitorSettings);

	len = wcslen(DriverName)*sizeof(WCHAR);
	if (len < (1 << (sizeof(USHORT) * 8))) {
		inputLen = sizeof(IOCTL_IRPMNDRV_DRIVER_WATCH_REGISTER_INPUT) + len;
		input = (PIOCTL_IRPMNDRV_DRIVER_WATCH_REGISTER_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputLen);
		if (input != NULL) {
			input->MonitorSettings = *MonitorSettings;
			input->NameLength = (USHORT)len;
			memcpy(input + 1, DriverName, len);
			ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_DRIVER_WATCH_REGISTER, input, (ULONG)inputLen);
			HeapFree(GetProcessHeap(), 0, input);
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
	} else ret = ERROR_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DriverComDriverNameWatchUnregister(const wchar_t *DriverName)
{
	DWORD ret = ERROR_GEN_FAILURE;
	SIZE_T len = 0;
	SIZE_T inputLen = 0;
	PIOCTL_IRPMNDRV_DRIVER_WATCH_UNREGISTER_INPUT input = NULL;
	DEBUG_ENTER_FUNCTION("DriverName=\"%ls\"", DriverName);

	len = wcslen(DriverName)*sizeof(WCHAR);
	if (len < (1 << (sizeof(USHORT) * 8))) {
		inputLen = sizeof(IOCTL_IRPMNDRV_DRIVER_WATCH_UNREGISTER_INPUT) + len;
		input = (PIOCTL_IRPMNDRV_DRIVER_WATCH_UNREGISTER_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputLen);
		if (input != NULL) {
			input->NameLength = (USHORT)len;
			memcpy(input + 1, DriverName, len);
			ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_DRIVER_WATCH_UNREGISTER, input, (ULONG)inputLen);
			HeapFree(GetProcessHeap(), 0, input);
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
	} else ret = ERROR_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DriverComDriverNameWatchEnum(PDRIVER_NAME_WATCH_RECORD *Array, PULONG Count)
{
	PDRIVER_NAME_WATCH_RECORD tmpArray = NULL;
	ULONG outputSize = 128;
	PIOCTL_IRPMNDRV_DRIVER_WATCH_ENUM_OUTPUT output = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Array=0x%p; Count=0x%p", Array, Count);

	ret = _SynchronousVariableOutputIOCTL(IOCTL_IRPMNDRV_DRIVER_WATCH_ENUM, NULL, 0, outputSize, (PVOID *)&output, &outputSize);
	if (ret == ERROR_SUCCESS) {
		if (output->Count > 0) {
			tmpArray = (PDRIVER_NAME_WATCH_RECORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, output->Count*sizeof(DRIVER_NAME_WATCH_RECORD));
			if (tmpArray != NULL) {
				SIZE_T entrySize = 0;
				PDRIVER_NAME_WATCH_RECORD rec = tmpArray;
				PDRIVER_NAME_WATCH_ENTRY entry = &output->Entry;

				for (SIZE_T i = 0; i < output->Count; ++i) {
					entrySize = sizeof(DRIVER_NAME_WATCH_ENTRY) + entry->NameLength;
					rec->MonitorSettings = entry->MonitorSettings;
					rec->DriverName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, entry->NameLength + sizeof(WCHAR));
					if (rec->DriverName != NULL) {
						memcpy(rec->DriverName, entry + 1, entry->NameLength);
						rec->DriverName[entry->NameLength / sizeof(WCHAR)] = L'\0';
					} else ret = ERROR_NOT_ENOUGH_MEMORY;

					if (ret != ERROR_SUCCESS) {
						for (SIZE_T j = 0; j < i; ++j) {
							--rec;
							HeapFree(GetProcessHeap(), 0, rec->DriverName);
						}

						break;
					}

					entry = (PDRIVER_NAME_WATCH_ENTRY)((PUCHAR)entry + entrySize);
					++rec;
				}

				if (ret == ERROR_SUCCESS) {
					*Array = tmpArray;
					*Count = output->Count;
				}
			} else ret = ERROR_NOT_ENOUGH_MEMORY;
		} else {
			*Array = NULL;
			*Count = output->Count;
		}

		HeapFree(GetProcessHeap(), 0, output);
	}

	DEBUG_EXIT_FUNCTION("%u, *Array=0x%p, *Count=%u", ret, *Array, *Count);
	return ret;
}


VOID DriverComDriverNameWatchEnumFree(PDRIVER_NAME_WATCH_RECORD Array, ULONG Count)
{
	DEBUG_ENTER_FUNCTION("Array=0x%p; Count=%u", Array, Count);

	if (Count > 0) {
		PDRIVER_NAME_WATCH_RECORD rec = Array;
		
		for (SIZE_T i = 0; i < Count; ++i) {
			HeapFree(GetProcessHeap(), 0, rec->DriverName);
			++rec;
		}
		
		HeapFree(GetProcessHeap(), 0, Array);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


DWORD DriverComEmulateDriverDevices(void)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = _SynchronousNoIOIOCTL(IOCTL_IRPMNDRV_EMULATE_DRVDEV);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DriverComEmulateProcesses(void)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = _SynchronousNoIOIOCTL(IOCTL_IRPMNDRV_EMULATE_PROCESS);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DriverComSettingsQuery(PIRPMNDRV_SETTINGS Settings)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_SETTINGS_QUERY_OUTPUT output;
	DEBUG_ENTER_FUNCTION("Settings=0x%p", Settings);

	ret = _SynchronousReadIOCTL(IOCTL_IRPMNDRV_SETTINGS_QUERY, &output, sizeof(output));
	if (ret == ERROR_SUCCESS)
		*Settings = output.Settings;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD DriverComSettingsSet(PIRPMNDRV_SETTINGS Settings, BOOLEAN Save)
{
	DWORD ret = ERROR_GEN_FAILURE;
	IOCTL_IRPMNDRV_SETTINGS_SET_INPUT input;
	DEBUG_ENTER_FUNCTION("Settings=0x%p; Save=%u", Settings, Save);

	input.Settings = *Settings;
	input.Save = Save;
	ret = _SynchronousWriteIOCTL(IOCTL_IRPMNDRV_SETTINGS_SET, &input, sizeof(input));

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


BOOL DriverComDeviceConnected(VOID)
{
	BOOL ret = FALSE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	if (_dcInterface != NULL)
		ret = _dcInterface->Connected();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD _SynchronousNoIOIOCTL(DWORD Code)
{
	return _SynchronousOtherIOCTL(Code, NULL, 0, NULL, 0);
}


DWORD _SynchronousWriteIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength)
{
	return _SynchronousOtherIOCTL(Code, InputBuffer, InputBufferLength, NULL, 0);
}


DWORD _SynchronousReadIOCTL(DWORD Code, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	return _SynchronousOtherIOCTL(Code, NULL, 0, OutputBuffer, OutputBufferLength);
}


DWORD _SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Code=0x%x; InputBuffer=0x%p; InputBufferLength=%u; OutputBuffer=0x%p; OutputBufferLength=%u", Code, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

	ret = ERROR_NOT_SUPPORTED;
	if (_dcInterface != NULL)
		ret = _dcInterface->SynchronousIoctl(Code, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


DWORD _SynchronousVariableOutputIOCTL(ULONG Code, PVOID InputBuffer, ULONG InputBufferLength, ULONG InitialSize, PVOID *OutputBuffer, PULONG OutputBufferLength)
{
	ULONG outputSize = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	PVOID output = NULL;
	DEBUG_ENTER_FUNCTION("Code=0x%x; InputBuffer=0x%p; InputBufferLength=%u; InitialSize=%u; OutputBuffer=0x%p; OutputBufferLength=0x%p", Code, InputBuffer, InputBufferLength, InitialSize, OutputBuffer, OutputBufferLength);

	outputSize = InitialSize;
	if (outputSize == 0)
		outputSize = 512;

	do {
		output = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, outputSize);
		if (output != NULL) {
			ret = _SynchronousOtherIOCTL(Code, InputBuffer, InputBufferLength, output, outputSize);
			if (ret != ERROR_SUCCESS) {
				outputSize *= 2;
				HeapFree(GetProcessHeap(), 0, output);
			}
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
	} while (ret == ERROR_INSUFFICIENT_BUFFER);

	if (ret == ERROR_SUCCESS) {
		*OutputBuffer = output;
		*OutputBufferLength = outputSize;
	}

	DEBUG_EXIT_FUNCTION("%u, *OutputBuffer=0x%p, *OutputBufferLength=%u", ret, *OutputBuffer, *OutputBufferLength);
	return ret;
}


/************************************************************************/
/*                   INITIALIZATION AND FINALIZATION                    */
/************************************************************************/


DWORD DriverComModuleInit(const IRPMON_INIT_INFO *Info)
{
	HMODULE HNtdll = NULL;
	const wchar_t *libraryName = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	ret = ERROR_SUCCESS;
	HNtdll = GetModuleHandleW(L"ntdll.dll");
	if (HNtdll != NULL) {
		_RtlStringFromGuid = (RTLSTRINGFROMGUID *)GetProcAddress(HNtdll, "RtlStringFromGUID");
		if (_RtlStringFromGuid != NULL) {
			_RtlFreeUnicodeString = (RTLFREEUNICODESTRING *)GetProcAddress(HNtdll, "RtlFreeUnicodeString");
			if (_RtlFreeUnicodeString != NULL) {
				switch (Info->ConnectorType) {
					case ictNone:
						libraryName = NULL;
						break;
					case ictDevice:
						libraryName = L"device-connector.dll";
						break;
					case ictNetwork:
						libraryName = L"network-connector.dll";
						break;
					case ictVSockets:
						libraryName = L"vsock-connector.dll";
						break;
					case ictHyperV:
						libraryName = L"hyperv-connector.h";
						break;
					default:
						ret = ERROR_NOT_SUPPORTED;
						break;
				}
				
				if (libraryName != NULL) {
					_hConnector = LoadLibraryW(libraryName);
					if (_hConnector != NULL) {
						_dcInterface = (PIRPMON_DRIVER_COMM_INTERFACE)GetProcAddress(_hConnector, "DriverCommInterface");
						if (_dcInterface != NULL) {
							ret = _dcInterface->Connect(Info);
							if (ret != ERROR_SUCCESS)
								_dcInterface = NULL;
						} else ret = GetLastError();

						if (ret != ERROR_SUCCESS)
							FreeLibrary(_hConnector);
					} else ret = GetLastError();
				}

				_initialized = (ret == ERROR_SUCCESS);
			} else ret = GetLastError();
		} else ret = GetLastError();
	} else ret = GetLastError();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


VOID DriverComModuleFinit(VOID)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	_initialized = FALSE;
	if (_dcInterface != NULL) {
		_dcInterface->Disconnect();
		_dcInterface = NULL;
		FreeLibrary(_hConnector);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
