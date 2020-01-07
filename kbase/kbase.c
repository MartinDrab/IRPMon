
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"





NTSTATUS DllInitialize(_In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);

	status = DebugAllocatorModuleInit();

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS DllUnload(void)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

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
