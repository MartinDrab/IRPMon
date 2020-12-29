
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
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



NTSTATUS DllInitialize(_In_ PUNICODE_STRING RegistryPath)
{
	UNICODE_STRING uRegistryPath;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);

	RtlInitUnicodeString(&uRegistryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\services\\irpmndrv");
	status = DebugAllocatorModuleInit();
	if (NT_SUCCESS(status)) {
		status = UtilsModuleInit(NULL, &uRegistryPath, NULL);
		if (NT_SUCCESS(status)) {
			status = DriverSettingsInit(NULL, &uRegistryPath, NULL);
			if (NT_SUCCESS(status)) {
				status = DataLoggerModuleInit(NULL, &uRegistryPath, NULL);
				if (NT_SUCCESS(status)) {
					status = RegManModuleInit(NULL, &uRegistryPath, NULL);
					if (NT_SUCCESS(status)) {
						status = RequestQueueModuleInit(NULL, &uRegistryPath, NULL);
						if (NT_SUCCESS(status)) {
							status = ProcessEventsModuleInit(NULL, &uRegistryPath, NULL);
							if (NT_SUCCESS(status)) {
								status = HookHandlerModuleInit(NULL, &uRegistryPath, NULL);
								if (NT_SUCCESS(status)) {
									status = DevExtHooksModuleInit(NULL, &uRegistryPath, NULL);
									if (NT_SUCCESS(status)) {
										status = HookModuleInit(NULL, &uRegistryPath, NULL);
										if (NT_SUCCESS(status)) {
											status = PWDModuleInit(NULL, &uRegistryPath, NULL);
											if (NT_SUCCESS(status)) {
												status = BLModuleInit(NULL, &uRegistryPath, NULL);
												if (!NT_SUCCESS(status))
													PWDModuleFinit(NULL, &uRegistryPath, NULL);
											}

											if (!NT_SUCCESS(status))
												HookModuleFinit(NULL, &uRegistryPath, NULL);
										}

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

						if (!NT_SUCCESS(status))
							RegManModuleFinit(NULL, &uRegistryPath, NULL);
					}

					if (!NT_SUCCESS(status))
						DataLoggerModuleFinit(NULL, &uRegistryPath, NULL);
				}

				if (!NT_SUCCESS(status))
					DriverSettingsFinit(NULL, &uRegistryPath, NULL);
			}

			if (!NT_SUCCESS(status))
				UtilsModuleFinit(NULL, &uRegistryPath, NULL);
		}

		if (!NT_SUCCESS(status))
			DebugAllocatorModuleFinit();
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS DllUnload(void)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	BLModuleFinit(NULL, NULL, NULL);
	PWDModuleFinit(NULL, NULL, NULL);
	HookModuleFinit(NULL, NULL, NULL);
	DevExtHooksModuleFinit(NULL, NULL, NULL);
	HookHandlerModuleFinit(NULL, NULL, NULL);
	ProcessEventsModuleFinit(NULL, NULL, NULL);
	RequestQueueModuleFinit(NULL, NULL, NULL);
	RegManModuleFinit(NULL, NULL, NULL);
	DataLoggerModuleFinit(NULL, NULL, NULL);
	DriverSettingsFinit(NULL, NULL, NULL);
	UtilsModuleFinit(NULL, NULL, NULL);
	DebugAllocatorModuleFinit();
	status = STATUS_SUCCESS;

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
