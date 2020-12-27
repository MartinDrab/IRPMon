
#ifndef __DEVEXT_HOOKS_H__
#define __DEVEXT_HOOKS_H__



#include <ntifs.h>


typedef enum _EDeviceExtensionType {
	detUnknown,
	detCDO,
	detProxy,
	detMax,
} EDeviceExtensionType, *PEDeviceExtensionType;

typedef struct _PROXY_DEVICE_EXTENSION {
	EDeviceExtensionType Type;
	PDEVICE_OBJECT TargetDevice;
	PDEVICE_OBJECT UpperDevice;
	size_t LowerDeviceOffset;
	size_t UpperDeviceExtensionSize;
	size_t LowerDevicePlaceCount;
} PROXY_DEVICE_EXTENSION, *PPROXY_DEVICE_EXTENSION;



NTSTATUS ProxyDeviceCreate(PDEVICE_OBJECT TargetDevice, PDEVICE_OBJECT *ProxyDevice);
void ProxyDeviceDelete(PDEVICE_OBJECT ProxyDevice);
void ProxySetDriverObject(PDRIVER_OBJECT DriverObject);
void ProxyTranslate(PDEVICE_OBJECT *DeviceObject, PDRIVER_OBJECT *DriverObject);

NTSTATUS DevExtHooksModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void DevExtHooksModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
