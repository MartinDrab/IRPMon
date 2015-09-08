
/**
 * @file
 *
 * Main source file of the translation library. Implements public library interface
 * and the DLLMain function.
 */

#include <windows.h>
#include "allocator.h"
#include "debug.h"
#include "translates.h"
#include "libtranslate.h"

/************************************************************************/
/*                     EXPORTED ROUTINES                                */
/************************************************************************/

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateEnumerationValueToString(ELibTranslateEnumerationType Type, BOOLEAN Description, ULONG Value)
{
   return EnumerationValueToString(Type, Description, Value);
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateGeneralIntegerValueToString(ELibTranslateIntegerValueType Type, BOOLEAN Description, ULONG Value)
{
   return GeneralIntegerValueToString(Type, Description, Value);
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateBitMaskValueToString(ELibTranslateBitMaskType Type, BOOLEAN Description, ULONG Value)
{
   return BitMaskValueToString(Type, Description, Value);
}

LIBTRANSLATEAPI VOID WINAPI LibTranslateBitMaskValueStringFree(PWCHAR Str)
{
   BitMaskValueStringFree(Str);

   return;
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateWindowsMessagesToString(ULONG32 Key)
{
   return WindowsMessagesToString(Key);
}

LIBTRANSLATEAPI VOID  WINAPI LibTranslateWindowsMessagesStringFree(ULONG32 key, PWCHAR p)
{
   WindowsMessagesStringFree(key, p);

   return;
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateWindowsEventHookToString(ULONG32 Key)
{
   return WindowsEventHookToString(Key);
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateWindowsEventHookDescriptionToString(ULONG32 Key)
{
   return WindowsEventHookDescriptionToString(Key);
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateIPV4ToString(ULONG ipv4)
{
   return IPV4ToString(ipv4);
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateIPV6ToString(PUCHAR ipv6)
{
   return IPV6ToString(ipv6);
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateNetworkPortToString(USHORT Port, ULONG Protocol)
{
   return NetworkPortToString(Port, Protocol);
}

LIBTRANSLATEAPI VOID WINAPI LibTranslateFreeMemory(PVOID Address)
{
   DEBUG_ENTER_FUNCTION("Address=0x%p", Address);

   HeapMemoryFree(Address);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

LIBTRANSLATEAPI NTSTATUS WINAPI LibTranslateWindowsErrorCodeToNTSTATUS(DWORD ErrorCode)
{
   return WindowsErrorCodeToNTSTATUS(ErrorCode);
}

LIBTRANSLATEAPI DWORD WINAPI LibTranslateNTSTATUSCodeToWindowsError(NTSTATUS Status)
{
   return NTSTATUSCodeToWindowsError(Status);
}

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateIRPFLagsToString(UCHAR MajorFunction, UCHAR MinorFunction, ULONG IRPFlags, BOOLEAN Description)
{
	return IRPFLagsToString(MajorFunction, MinorFunction, IRPFlags, Description);
}



/************************************************************************/
/*                     INITIALIZATION AND FINALIZATION                  */
/************************************************************************/

/** Initializes the translation library.
 *
 *  @return
 *  Returns an ELibraryError value indicating success or failure of the operation.
 */
LIBTRANSLATEAPI DWORD WINAPI LibTranslateInitialize(VOID)
{
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   ret = DebugAllocatorInit();
   if (ret == ERROR_SUCCESS) {
      ret = TranslatesModuleInit();
      if (ret != ERROR_SUCCESS)
         DebugAllocatorFinit();
   }

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/** Finalizes the translaton library.
 */
LIBTRANSLATEAPI VOID WINAPI LibTranslateFinalize(VOID)
{
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   TranslatesModuleFinit();
   DebugAllocatorFinit();

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/************************************************************************/
/*                     DLLMAIN                                          */
/************************************************************************/

/** Entrypoint function of the library. For more information, see
 *  http://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx.
 */
BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
   BOOL ret = FALSE;
   DEBUG_ENTER_FUNCTION("hinstDLL=0x%p; fdwReason=%u; lpReserved=0x%p", hinstDLL, fdwReason, lpvReserved);

   switch (fdwReason) {
      case DLL_PROCESS_ATTACH:
         ret = DisableThreadLibraryCalls(hinstDLL);
         break;
   }

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}
