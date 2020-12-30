
/**
 * @file
 *
 * Header file for the Module Framework that allows to automate initialization and
 * finalization of kernel mode components.
 */

#ifndef __PNPMON_MODULES_H_
#define __PNPMON_MODULES_H_

#include <ntifs.h>

 /** Initialization routine prototype.
  *
  *  @param driverObject Address of a DRIVER_OBJECT structure representing the driver.
  *  @param context User-defined variable value of which is defined when the module is
  *  registered for automated initialization and finalization. Driver writers can use
  *  this parameter to pass to the initialization (and also the finalization) routine
  *  any data they need.
  *
  *  @return
  *  To report success, the routine must return a NTSTATUS value that represents a success
  *  (NT_SUCCESS macro evaluates to TRUE). Otherwise, initialization of the module is treated
  *  as failed.
  *
  *  @remark
  *  The routine is called at IRQL = PASSIVE_LEVEL.
  */
typedef NTSTATUS (DRIVER_MODULE_INIT_ROUTINE)(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);


/** Finalization routine prototype.
 *
 *  @param driverObject Address of a DRIVER_OBJECT structure representing the driver.
 *  @param context User-defined variable value of which is defined when the module is
 *  registered for automated initialization and finalization. Driver writers can use
 *  this parameter to pass to the finalization (and also the initialization) routine
 *  any data they need.
 *
 *  @remark
 *  The routine is called at IRQL = PASSIVE_LEVEL.
 */
typedef void (DRIVER_MODULE_FINIT_ROUTINE)(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);

/** Used to register a module into the automated initialization and finalization
   process. */
typedef struct _DRIVER_MODULE_ENTRY_PARAMETERS {
	/** Address of initialization routine. */
	DRIVER_MODULE_INIT_ROUTINE* InitRoutine;
	/** Address of finalization routine. */
	DRIVER_MODULE_FINIT_ROUTINE* FinitRoutine;
	/** Value of the Context argument to be passed to initialization and finalization routines .*/
	PVOID Context;
} DRIVER_MODULE_ENTRY_PARAMETERS, * PDRIVER_MODULE_ENTRY_PARAMETERS;

/** Represents one module. */
typedef struct {
	/** Used to link the information together with other structures. */
	LIST_ENTRY Entry;
	/** Module characteristics (initialization and finalization routine, value
		of the context argument). */
	DRIVER_MODULE_ENTRY_PARAMETERS Parameters;
} DRIVER_MODULE_ENTRY, * PDRIVER_MODULE_ENTRY;


NTSTATUS ModuleFrameworkInitializeModules(PUNICODE_STRING RegistryPath);
void ModuleFrameworkFinalizeModules(void);
void ModuleFrameworkAddModule(const DRIVER_MODULE_ENTRY_PARAMETERS *moduleParams);
void ModuleFrameworkAddModules(const DRIVER_MODULE_ENTRY_PARAMETERS *moduleParams, size_t count);

NTSTATUS ModuleFrameworkInit(PDRIVER_OBJECT driverObject);
void ModuleFrameworkFinit(VOID);

#endif
