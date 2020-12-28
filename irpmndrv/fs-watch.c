
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "utils.h"
#include "pnp-driver-watch.h"
#include "fs-watch.h"


/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/


static PDRIVER_OBJECT _driverObject = NULL;
static volatile BOOLEAN _fsMonitoring = FALSE;


/************************************************************************/
/*                    HELPER FUNCTIONS                                  */
/************************************************************************/


static void _FileSystemCallback(PDEVICE_OBJECT DeviceObject, BOOLEAN FsActive)
{
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; FsActive=%u", DeviceObject, FsActive);

	if (FsActive)
		PDWCheckDriver(DeviceObject->DriverObject, DeviceObject);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/


NTSTATUS FSWMonitor(BOOLEAN Monitor)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Monitor=%u", Monitor);

	status = STATUS_SUCCESS;
	if (Monitor) {
		if (!_fsMonitoring) {
			status = IoRegisterFsRegistrationChange(_driverObject, _FileSystemCallback);
			_fsMonitoring = NT_SUCCESS(status);
		}
		else status = STATUS_OBJECT_NAME_COLLISION;
	} else {
		if (_fsMonitoring) {
			IoUnregisterFsRegistrationChange(_driverObject, _FileSystemCallback);
			_fsMonitoring = FALSE;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/************************************************************************/
/*                INITIALIZATION AND FINALIZATION                       */
/************************************************************************/


NTSTATUS FSWModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	_driverObject = DriverObject;
	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void FSWModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	FSWMonitor(FALSE);
	_driverObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
