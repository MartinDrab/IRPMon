
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hook.h"
#include "hook-handlers.h"
#include "req-queue.h"
#include "devext-hooks.h"
#include "process-events.h"




NTSTATUS DllInitialize(_In_ PUNICODE_STRING RegistryPath)
{
	UNICODE_STRING uRegistryPath;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);

	RtlInitUnicodeString(&uRegistryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\services\\irpmndrv");
	status = RequestQueueModuleInit(NULL, &uRegistryPath, NULL);
	if (NT_SUCCESS(status)) {
		status = ProcessEventsModuleInit(NULL, &uRegistryPath, NULL);
		if (NT_SUCCESS(status)) {
			status = HookHandlerModuleInit(NULL, &uRegistryPath, NULL);
			if (NT_SUCCESS(status)) {
				status = DevExtHooksModuleInit(NULL, &uRegistryPath, NULL);
				if (NT_SUCCESS(status)) {
					status = HookModuleInit(NULL, &uRegistryPath, NULL);
					if (!NT_SUCCESS(status))
						DevExtHooksModuleFinit(NULL, &uRegistryPath, NULL);
				}

				if (!NT_SUCCESS(status))
					HookHandlerModuleFinit(NULL, &uRegistryPath, NULL);
			}
			
			if (!NT_SUCCESS(status))
				ProcessEventsModuleFinit(NULL, &uRegistryPath, NULL);
		}

		if (!NT_SUCCESS(status))
			RequestQueueModuleFinit(NULL, &uRegistryPath, NULL);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS DllUnload(void)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	HookModuleFinit(NULL, NULL, NULL);
	DevExtHooksModuleFinit(NULL, NULL, NULL);
	HookHandlerModuleFinit(NULL, NULL, NULL);
	ProcessEventsModuleFinit(NULL, NULL, NULL);
	RequestQueueModuleFinit(NULL, NULL, NULL);

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
