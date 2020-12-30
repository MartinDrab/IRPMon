
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
#include "preprocessor.h"
#include "modules.h"

/************************************************************************/
/*                         GLOBAL VARIABLES                             */
/************************************************************************/

/** Array for modules registered for automatic initialization and finalization. */
static DRIVER_MODULE_ENTRY _modules[256];
/** Number of registered modules. */
static size_t _moduleCount = 0;
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
static void _ModuleFrameworkDeleteAllModules(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	RtlSecureZeroMemory(_modules, sizeof(_modules));
	_moduleCount = 0;

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
 *  @remark
 *  The routine is not thread-safe. The caller is expected to use Module Framework
 *  only twice: during the initialization and during the finalization, and that
 *  moments should not occur in one point of time.
 */
void ModuleFrameworkAddModule(const DRIVER_MODULE_ENTRY_PARAMETERS *moduleParams)
{
	DEBUG_ENTER_FUNCTION("ModuleParams=0x%p", moduleParams);

	ASSERT(_moduleCount < sizeof(_modules) / sizeof(_modules[0]));;
	_modules[_moduleCount].Parameters = *moduleParams;
	++_moduleCount;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/** Add a set of modules to the internal lsit, so they become subjects to automated
 *  initialization and finalization. 
 *  
 *  @param moduleParams Array of @link(DRIVER_MODULE_ENTRY_PARAMETERS) structures. Each
 *  describes one module to be added to the list.
 *  @param count Number of elements in the moduleParams array.
 *
 *  @remark
 *  The routine is not thread-safe. The caller is expected to use Module Framework
 *  only twice: during the initialization and during the finalization, and that
 *  moments should not occur in one point of time.
 */
void ModuleFrameworkAddModules(const DRIVER_MODULE_ENTRY_PARAMETERS *moduleParams, size_t count)
{
	DEBUG_ENTER_FUNCTION("ModuleParams=0x%p; Count=%zu", moduleParams, count);

	ASSERT(_moduleCount + count <= sizeof(_modules) / sizeof(_modules[0]));;
	for (size_t i = 0; i < count; ++i)
		_modules[_moduleCount + i].Parameters = moduleParams[i];

	_moduleCount += count;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
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
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);
	DEBUG_IRQL_EQUAL(PASSIVE_LEVEL);

	status = STATUS_SUCCESS;
	for (size_t i = 0; i < _moduleCount; ++i) {
		status = _modules[i].Parameters.InitRoutine(_driverObject, RegistryPath, _modules[i].Parameters.Context);
		if (!NT_SUCCESS(status)) {
			for (size_t j = i; j > 0; --j)
				_modules[j - 1].Parameters.FinitRoutine(_driverObject, RegistryPath, _modules[j - 1].Parameters.Context);

			break;
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
void ModuleFrameworkFinalizeModules(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_EQUAL(PASSIVE_LEVEL);

	for (size_t i = _moduleCount; i > 0; --i)
		_modules[i - 1].Parameters.FinitRoutine(_driverObject, NULL, _modules[i - 1].Parameters.Context);

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

	if (driverObject != NULL) {
		ObReferenceObject(driverObject);
		_driverObject = driverObject;
	}

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
void ModuleFrameworkFinit(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_EQUAL(PASSIVE_LEVEL);

	_ModuleFrameworkDeleteAllModules();
	if (_driverObject != NULL) {
		ObDereferenceObject(_driverObject);
		_driverObject = NULL;
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
