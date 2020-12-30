#ifndef __PREPROCESSOR_H_
#define __PREPROCESSOR_H_

#include <ntifs.h>

/*
 * Thanks for these macros:
 * http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
 */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define AT_FUNCTION __FUNCTION__
#define AT_LINE AT_FUNCTION ":" TOSTRING(__LINE__ " ")

#ifdef DBG

#ifndef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED	1
#endif

/*
 * Prints the source file and function name. Determined for non-parametric
 * functions.
 */
#define DEBUG_ENTER_FUNCTION_NO_ARGS() \
	do { if (DEBUG_TRACE_ENABLED)	\
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[%u;%u,%u]: " AT_FUNCTION "()\n", PtrToUlong(PsGetCurrentProcessId()), PtrToUlong(PsGetCurrentThreadId()), KeGetCurrentIrql()); \
	} while (0)

/*
 * Prints the source file, function name and parameters.
 */
#define DEBUG_ENTER_FUNCTION(paramsFormat,...) \
	do { if (DEBUG_TRACE_ENABLED)	\
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[%u;%u,%u]: " AT_FUNCTION "(" paramsFormat ")\n", PtrToUlong(PsGetCurrentProcessId()), PtrToUlong(PsGetCurrentThreadId()), KeGetCurrentIrql(), __VA_ARGS__); \
	} while (0)

/*
 * Prints the source file, function name and the return value.
 */
#define DEBUG_EXIT_FUNCTION(returnValueFormat,...) \
	do { if (DEBUG_TRACE_ENABLED)	\
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[%u;%u,%u]: " AT_FUNCTION "(-):" returnValueFormat "\n", PtrToUlong(PsGetCurrentProcessId()), PtrToUlong(PsGetCurrentThreadId()), KeGetCurrentIrql(), __VA_ARGS__); \
	} while (0)

/*
 * Prints the source file and function name. Determined for ending a function
 * without a return value.
 */
#define DEBUG_EXIT_FUNCTION_VOID() \
	do { if (DEBUG_TRACE_ENABLED)	\
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, "[%u;%u,%u]: " AT_FUNCTION "(-):void\n", PtrToUlong(PsGetCurrentProcessId()), PtrToUlong(PsGetCurrentThreadId()), KeGetCurrentIrql()); \
	} while (0)

/*
 * Prints the source file, function name and the number of the line.
 */
#define DEBUG_PRINT_LOCATION_VOID() \
	do { if (DEBUG_TRACE_ENABLED)	\
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_LINE "\n");	\
	} while (0)

/*
 * Prints the source file, function name and the number of the line.
 */
#define DEBUG_PRINT_LOCATION(format,...) \
   DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_LINE format "\n", __VA_ARGS__)

/*
 * Macro for reporting error conditions.
 */
#define DEBUG_ERROR(format,...) \
   DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, AT_LINE " ERROR: " format "\n", __VA_ARGS__); \
 //  __debugbreak() \


#define DEBUG_IRQL_LESS_OR_EQUAL(aIrql) \
   if (KeGetCurrentIrql() > (aIrql)) { \
      DEBUG_ERROR("Current IRQL (%d) is too high. Expected at most %d", KeGetCurrentIrql(), (aIrql)); \
   } \

#define DEBUG_IRQL_EQUAL(aIrql) \
   if (KeGetCurrentIrql() != (aIrql)) { \
      DEBUG_ERROR("Current IRQL (%d) is not valid. Expected %d", KeGetCurrentIrql(), (aIrql)); \
   } \

#else // ifdef _DEBUG

#define DEBUG_ENTER_FUNCTION_NO_ARGS() { }
#define DEBUG_ENTER_FUNCTION(paramsFormat,...) { }
#define DEBUG_EXIT_FUNCTION(returnValueFormat,...) { }
#define DEBUG_EXIT_FUNCTION_VOID() { }
#define DEBUG_PRINT_LOCATION_VOID() { }
#define DEBUG_PRINT_LOCATION(format,...) { }
#define DEBUG_IRQL_LESS_OR_EQUAL(aIrql) { }
#define DEBUG_IRQL_EQUAL(aIrql) { }

/*
 * Macro for reporting error conditions.
 */
#define DEBUG_ERROR(format,...) \
   DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, AT_LINE format "\n", __VA_ARGS__)

#endif // ifdef _DEBUG




#endif // ifndef __PREPROCESSOR_H_
