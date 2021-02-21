
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "thread-context.h"
#include "utils.h"
#include "driver-settings.h"
#include "data-loggers.h"
#include "regman.h"
#include "hook.h"
#include "hook-handlers.h"
#include "req-queue.h"
#include "devext-hooks.h"
#include "process-events.h"
#include "pnp-driver-watch.h"
#include "boot-log.h"
#include "modules.h"




static volatile LONG _initialized = 0;

static DRIVER_MODULE_ENTRY_PARAMETERS _modules[] = {
	{ThreadContextModuleInit, ThreadContextModuleFinit, NULL},
	{UtilsModuleInit, UtilsModuleFinit, NULL},
	{DriverSettingsInit, DriverSettingsFinit, NULL},
	{DataLoggerModuleInit, DataLoggerModuleFinit, NULL},
	{RegManModuleInit, RegManModuleFinit, NULL},
	{RequestQueueModuleInit, RequestQueueModuleFinit, NULL},
	{ProcessEventsModuleInit, ProcessEventsModuleFinit, NULL},
	{HookHandlerModuleInit, HookHandlerModuleFinit, NULL},
	{DevExtHooksModuleInit, DevExtHooksModuleFinit, NULL},
	{HookModuleInit, HookModuleFinit, NULL},
	{PWDModuleInit, PWDModuleFinit, NULL},
	{BLModuleInit, BLModuleFinit, NULL},
};


NTSTATUS DllInitialize(_In_ PUNICODE_STRING RegistryPath)
{
	UNICODE_STRING uRegistryPath;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);

	status = STATUS_SUCCESS;
	if (InterlockedCompareExchange(&_initialized, 0, 2) == 0) {
		RtlInitUnicodeString(&uRegistryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\services\\irpmndrv");
		status = DebugAllocatorModuleInit();
		if (NT_SUCCESS(status)) {
			status = ModuleFrameworkInit(NULL);
			if (NT_SUCCESS(status)) {
				ModuleFrameworkAddModules(_modules, sizeof(_modules) / sizeof(_modules[0]));
				status = ModuleFrameworkInitializeModules(&uRegistryPath);
				if (NT_SUCCESS(status))
					InterlockedExchange(&_initialized, 1);

				if (!NT_SUCCESS(status))
					ModuleFrameworkFinit();
			}

			if (!NT_SUCCESS(status))
				DebugAllocatorModuleFinit();
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS DllUnload(void)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	status = STATUS_SUCCESS;
	if (InterlockedCompareExchange(&_initialized, 0, 1)) {
		ModuleFrameworkFinalizeModules();
		ModuleFrameworkFinit();
		DebugAllocatorModuleFinit();
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=%wZ", DriverObject, RegistryPath);

	status = DllInitialize(RegistryPath);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS KBaseInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	status = DllInitialize(RegistryPath);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void KBaseFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	DllUnload();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
