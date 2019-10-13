
/**
 * @file
 *
 * MODULE FRAMEWORK
 *
 * Implements routines that helps with initialization and finalization of kernel
 * mode components (which are usually drivers). The idea is to split a driver into
 * a set of modules. A module can be, for example, a single .c file and corresponding
 * header file. Every module define its initialization and finalization routine. The
 * former one must be called before any functionality of the module is used, the latter
 * one is present in order to cleanup resources used by the module when its services are
 * no longer needed.
 *
 * Definition of the initialization routines must follow this typedef:
 * typedef NTSTATUS (DRIVER_MODULE_INIT_ROUTINE)(PDRIVER_OBJECT DriverObject, PVOID Context);
 *
 * Finalization routines must follow this definition:
 * typedef VOID (DRIVER_MODULE_FINIT_ROUTINE)(PDRIVER_OBJECT DriverObject, PVOID Context);
 *
 * The DriverObject argument is set to the address of a DRIVE_OBJECT structure
 * representing the driver on behalf of which the module is being initialized
 * (or finalized). The Context parameter is can have any value, that is defined
 * by the entity performing the initialization sequence (usually the driver entry
 * point routine).
 *
 * When the initialization and finalization routines follows the rules above, the
 * initialization and finalization of the whole component can be automated. Given
 *  an array of triples (InitRoutine, FinitRoutine, Context), the initialization
 * of all the modules is done by invoking the initialization routines (with appropriate
 * value of the second argument) one by one until either all of them are done and returned
 * a success status, or one of them returns an error status code. If the latter occurs, 
 * finalization routines are called in reverse order, starting with the one in pair of
 * the last successful initialization routine.
 *
 * Finalization of the whole component means invocation of all the finalization routines
 * in order reverse to that of initialization routines invocation. Because finalization
 * routines cannot return any value, the thing is that simple. A finalization routine must
 * be written so that it cannot fail, at least its failure must not destroy the system.
 */

#include <ntifs.h>
#include "allocator.h"
#include "preprocessor.h"
#include "modules.h"

/************************************************************************/
/*                         GLOBAL VARIABLES                             */
/************************************************************************/

/** List of modules registered for automatic initialization and finalization. */
static LIST_ENTRY _driverModuleList;
/** Address of Driver Object of the current driver. */
static PDRIVER_OBJECT _driverObject = NULL;

/************************************************************************/
/*                       HELPER ROUTINES                                */
/************************************************************************/


/** Removes all modules from the list used for automated initialization and
 *  finalization.
 *
 *  @remark
 *  The routine is not thread-safe. The caller is expected to use Module Framework
 *  only twice: during the initialization and during the finalization, and that
 *  moments should not occur in one point of time.
 *
 *  The routine must be run at IRQL below DISPATCH_LEVEL.
 */
static VOID _ModuleFrameworkDeleteAllModules(VOID)
{
	PDRIVER_MODULE_ENTRY iteratedEntry = NULL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	iteratedEntry = CONTAINING_RECORD(_driverModuleList.Flink, DRIVER_MODULE_ENTRY, Entry);
	while (&iteratedEntry->Entry != &_driverModuleList) {
		PDRIVER_MODULE_ENTRY deletedEntry = iteratedEntry;
		iteratedEntry = CONTAINING_RECORD(iteratedEntry->Entry.Flink, DRIVER_MODULE_ENTRY, Entry);
		HeapMemoryFree(deletedEntry);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                               PUBLIC ROUTINES                        */
/************************************************************************/


/** Inserts a module into the list of modules that are subject to automatic
 *  initialization and finalization.
 *
 *  @param moduleParams Information about the module to add.
 *
 *  @return
 *  The following NTSTATUS values can be returned:
 *  @value STATUS_SUCCESS The module has been successfully added to the list.
 *  @value STATUS_INSUFFICIENT_RESOURCES The operation failed due to insufficient
 *  amount of free memory.
 *
 *  @remark
 *  The routine is not thread-safe. The caller is expected to use Module Framework
 *  only twice: during the initialization and during the finalization, and that
 *  moments should not occur in one point of time.
 *
 *  The routine must be called at IRQL below DISPATCH_LEVEL.
 */
NTSTATUS ModuleFrameworkAddModule(PDRIVER_MODULE_ENTRY_PARAMETERS moduleParams)
{
	PDRIVER_MODULE_ENTRY modEntry = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ModuleParams=0x%p", moduleParams);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	modEntry = HeapMemoryAllocPaged(sizeof(DRIVER_MODULE_ENTRY));
	if (modEntry != NULL) {
		InitializeListHead(&modEntry->Entry);
		RtlCopyMemory(&modEntry->Parameters, moduleParams, sizeof(DRIVER_MODULE_ENTRY_PARAMETERS));
		InsertTailList(&_driverModuleList, &modEntry->Entry);
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/** Add a set of modules to the internal lsit, so they become subjects to automated
 *  initialization and finalization. 
 *  
 *  @param moduleParams Array of @link(DRIVER_MODULE_ENTRY_PARAMETERS) structures. Each
 *  describes one module to be added to the list.
 *  @param count Number of elements in the moduleParams array.
 *
 *  @return
 *  The routine can return the following NTSTATUS values:
 *  @value STATUS_SUCCESS All modules have been successfully added to the list.
 *  @value STATUS_INSUFFICIENT_RESOURCES There is not enough available memory
 *  to add all the modules into the list. 
 *
 *  @remark
 *  The routine is not thread-safe. The caller is expected to use Module Framework
 *  only twice: during the initialization and during the finalization, and that
 *  moments should not occur in one point of time.
 *
 *  If the routine fails, none of the modules is added to the list – none becomes
 *  subject to automated initialization and finalization.
 *
 *  The routine must be called at IRQL below DISPATCH_LEVEL.
 */
NTSTATUS ModuleFrameworkAddModules(DRIVER_MODULE_ENTRY_PARAMETERS moduleParams[], ULONG count)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ModuleParams=0x%p; Count=%u", moduleParams, count);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	for (ULONG i = 0; i < count; ++i) {
		status = ModuleFrameworkAddModule(&moduleParams[i]);
		if (!NT_SUCCESS(status)) {
			// TODO: We should remove only modules we added
			_ModuleFrameworkDeleteAllModules();
			break;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/** Performs automatic initialization of all registered modules.
 *
 *  @return
 *  Returns NTSTATUS value indicating success or failure of the initialization.
 *  If the returned value is an error code, it is the code returned by the initialization
 *  function that failed.
 *
 *  @remark
 *  The routine is not thread-safe. The caller is expected to use Module Framework
 *  only twice: during the initialization and during the finalization, and that
 *  moments should not occur in one point of time.
 *
 *  The initialization routines are called in the same order as their modules were
 *  registered for automated initialization and finalization. If any of the routines
 *  fails, finalization routines for already initialized modules are called, in reverse
 *  order.
 *
 *  The routine must be called at IRQL = PASSIVE_LEVEL.
 */
NTSTATUS ModuleFrameworkInitializeModules(PUNICODE_STRING RegistryPath)
{
	PDRIVER_MODULE_ENTRY moduleEntry = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);
	DEBUG_IRQL_EQUAL(PASSIVE_LEVEL);

	status = STATUS_SUCCESS;
	moduleEntry = CONTAINING_RECORD(_driverModuleList.Flink, DRIVER_MODULE_ENTRY, Entry);
	while (&moduleEntry->Entry != &_driverModuleList) {
		status = moduleEntry->Parameters.InitRoutine(_driverObject, RegistryPath, moduleEntry->Parameters.Context);
		if (!NT_SUCCESS(status))
			break;

		moduleEntry = CONTAINING_RECORD(moduleEntry->Entry.Flink, DRIVER_MODULE_ENTRY, Entry);
	}

	if (!NT_SUCCESS(status)) {
		moduleEntry = CONTAINING_RECORD(moduleEntry->Entry.Blink, DRIVER_MODULE_ENTRY, Entry);
		while (&moduleEntry->Entry != &_driverModuleList) {
			// Finalize only the modules that require that
			if (moduleEntry->Parameters.FinitRoutine != NULL)
				moduleEntry->Parameters.FinitRoutine(_driverObject, RegistryPath, moduleEntry->Parameters.Context);

			moduleEntry = CONTAINING_RECORD(moduleEntry->Entry.Blink, DRIVER_MODULE_ENTRY, Entry);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/** Perform automatic finalization of all registered modules.
 *
 *  @remark
 *  The routine is not thread-safe. The caller is expected to use Module Framework
 *  only twice: during the initialization and during the finalization, and that
 *  moments should not occur in one point of time.
 *  
 *  The routine must be called at IRQL = PASSIVE_LEVEL.
 */
VOID ModuleFrameworkFinalizeModules(VOID)
{
	PDRIVER_MODULE_ENTRY moduleEntry = NULL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_EQUAL(PASSIVE_LEVEL);

	moduleEntry = CONTAINING_RECORD(_driverModuleList.Blink, DRIVER_MODULE_ENTRY, Entry);
	while (&moduleEntry->Entry != &_driverModuleList) {
		// finalize only the modules that require that
		if (moduleEntry->Parameters.FinitRoutine != NULL)
			 moduleEntry->Parameters.FinitRoutine(_driverObject, NULL, moduleEntry->Parameters.Context);

		moduleEntry = CONTAINING_RECORD(moduleEntry->Entry.Blink, DRIVER_MODULE_ENTRY, Entry);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                         INITIALIYATION AND FINALIYATION              */
/************************************************************************/


/** Initializes the Module Framework.
 *
 *  The routine also initializes memory allocator used for detection of memory
 *  leaks and other memory-related problems. When the routine succeeds, it is
 *  safe to use UtilsAllocXXXMemory functions.
 *
 *  @param driverObject Address of DRIVER_OBJECT structure representing the driver.
 *  The address is passed to initialization and finalization routines.
 *
 *  @return
 *  Returns NTSTATUS value indicating success or failure of the operation.
 *
 *  @remark
 *  The routine must be called at IRQL = PASSIVE_LEVEL.
 */
NTSTATUS ModuleFrameworkInit(PDRIVER_OBJECT driverObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", driverObject);
	DEBUG_IRQL_EQUAL(PASSIVE_LEVEL);

	ObReferenceObject(driverObject);
	_driverObject = driverObject;
	InitializeListHead(&_driverModuleList);
	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/** Releases resources used by the Module Framework.
 *
 *  @remark
 *  The routine does not perform automatic finalization of registered modules.
 *
 *  The routine must be called at IRQL = PASSIVE_LEVEL.
 */
VOID ModuleFrameworkFinit(VOID)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_EQUAL(PASSIVE_LEVEL);

	_ModuleFrameworkDeleteAllModules();
	InitializeListHead(&_driverModuleList);
	ObDereferenceObject(_driverObject);
	_driverObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
