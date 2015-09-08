
#ifndef __LIBHIPS_DEBUG_H_
#define __LIBHIPS_DEBUG_H_

#include <stdio.h>
#include <windows.h>



#ifdef _DEBUG

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define AT_FUNCTION  ": " __FUNCTION__
#define AT_LINE AT_FUNCTION ":" TOSTRING(__LINE__)




/*
 * Prints the source file and function name. Determined for non-parametric
 * functions.
 */
#define DEBUG_ENTER_FUNCTION_NO_ARGS() \
   DebugPrint("[%d:%d]: ", GetCurrentProcessId(), GetCurrentThreadId()); \
   DebugPrint(AT_FUNCTION "()\n")

/*
 * Prints the source file, function name and parameters.
 */
#define DEBUG_ENTER_FUNCTION(paramsFormat,...) \
   DebugPrint("[%d:%d]: ", GetCurrentProcessId(), GetCurrentThreadId()); \
   DebugPrint(AT_FUNCTION "(" paramsFormat ")\n", __VA_ARGS__)

/*
 * Prints the source file, function name and the return value.
 */
#define DEBUG_EXIT_FUNCTION(returnValueFormat,...) \
   DebugPrint("[%d:%d]: ", GetCurrentProcessId(), GetCurrentThreadId()); \
   DebugPrint(AT_FUNCTION "(-):" returnValueFormat "\n", __VA_ARGS__)

/*
 * Prints the source file and function name. Determined for ending a function
 * without a return value.
 */
#define DEBUG_EXIT_FUNCTION_VOID() \
   DebugPrint("[%d:%d]: ", GetCurrentProcessId(), GetCurrentThreadId()); \
   DebugPrint(AT_FUNCTION "(-):void\n")

/*
 * Prints the source file, function name and the number of the line.
 */
#define DEBUG_PRINT_LOCATION_VOID() \
   DebugPrint(AT_LINE "\n")

/*
 * Prints the source file, function name and the number of the line.
 */
#define DEBUG_PRINT_LOCATION(format,...) \
   DebugPrint(AT_LINE format "\n", __VA_ARGS__)

VOID FORCEINLINE DebugPrint(char *format, ...)
{
   char *msg = NULL;
   va_list args;

   msg = (char *)malloc(10000);
   if (msg != NULL) {
      memset(msg, 0, 10000);
      va_start(args, format);
      _vsnprintf_s(msg, 9999, _TRUNCATE, format, args);
      va_end(args);
      OutputDebugStringA(msg);
      free(msg);
   }

   return;
}

#else

#define DEBUG_ENTER_FUNCTION_NO_ARGS() { }
#define DEBUG_ENTER_FUNCTION(paramsFormat,...) { }
#define DEBUG_EXIT_FUNCTION(returnValueFormat,...) { }
#define DEBUG_EXIT_FUNCTION_VOID() { }
#define DEBUG_PRINT_LOCATION_VOID() { }
#define DEBUG_PRINT_LOCATION(format,...) { }

#endif



#endif
