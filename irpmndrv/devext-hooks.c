
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hook-handlers.h"
#include "devext-hooks.h"


/************************************************************************/
/*                        GLOBAL VARIABLE                               */
/************************************************************************/

static PDRIVER_OBJECT _driverObject = NULL;

/************************************************************************/
/*                        PUBLIC FUNCTIONS                              */
/************************************************************************/


NTSTATUS ProxyDeviceCreate(PDEVICE_OBJECT TargetDevice, PDEVICE_OBJECT *ProxyDevice)
{
	PDEVICE_OBJECT tmpProxy = NULL;
	PDEVICE_OBJECT upperDevice = NULL;
	PPROXY_DEVICE_EXTENSION ext = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("TargetDevice=0x%p; ProxyDevice=0x%p", TargetDevice, ProxyDevice);

	upperDevice = TargetDevice->AttachedDevice;
	if (upperDevice != NULL) {
		status = IoCreateDevice(_driverObject, sizeof(PROXY_DEVICE_EXTENSION), NULL, TargetDevice->DeviceType, TargetDevice->Characteristics, (TargetDevice->Flags & DO_EXCLUSIVE), &tmpProxy);
		if (NT_SUCCESS(status)) {
			ext = (PPROXY_DEVICE_EXTENSION)tmpProxy->DeviceExtension;
			memset(ext, 0, sizeof(PROXY_DEVICE_EXTENSION));
			ext->Type = detProxy;
			ext->TargetDevice = TargetDevice;
			ext->UpperDevice = upperDevice;
			ext->UpdatePlace = (PDEVICE_OBJECT *)upperDevice->DeviceExtension;
			tmpProxy->Flags = TargetDevice->Flags;
			status = STATUS_NOT_FOUND;
			for (size_t i = 0; i < 0x1000 / sizeof(PDEVICE_OBJECT); ++i) {
				if (*ext->UpdatePlace == TargetDevice) {
					ext->LowerDeviceOffset = i * sizeof(PDEVICE_OBJECT);
					*ext->UpdatePlace = tmpProxy;
					status = STATUS_SUCCESS;
					break;
				}

				++ext->UpdatePlace;
			}


			if (NT_SUCCESS(status))
				*ProxyDevice = tmpProxy;

			if (!NT_SUCCESS(status))
				IoDeleteDevice(tmpProxy);
		}
	} else status = STATUS_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("0x%x, *ProxyDevice=0x%p", status, *ProxyDevice);
	return status;
}


void ProxyDeviceDelete(PDEVICE_OBJECT ProxyDevice)
{
	PPROXY_DEVICE_EXTENSION ext = NULL;
	DEBUG_ENTER_FUNCTION("ProxyDevice=0x%p", ProxyDevice);

	ext = (PPROXY_DEVICE_EXTENSION)ProxyDevice->DeviceExtension;
	*ext->UpdatePlace = ext->TargetDevice;
	IoDeleteDevice(ProxyDevice);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                   INITIALIZATION AND FINALIZATION                    */
/************************************************************************/


NTSTATUS DevExtHooksModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	ObReferenceObject(DriverObject);
	_driverObject = DriverObject;
	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void DevExtHooksModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	ObDereferenceObject(_driverObject);
	_driverObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
