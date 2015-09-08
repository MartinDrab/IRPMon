
/**
 * @file
 *
 * Implements routines translating various system data types (such as process access rights,
 * file attributes) to their human-readable string representations.
 */

#include <Shlwapi.h>
#include <stdio.h>
#include <windows.h>
#include <Winternl.h>
#include "debug.h"
#include "allocator.h"
#include "libtranslate.h"
#include "libtranslate-hash-table.h"
#include "gv-hash-table.h"
#include "p2p-hash-table.h"
#include "translates-arrays.h"
#include "translates.h"

/************************************************************************/
/*                  TYPE DEFINITIONS                                    */
/************************************************************************/

/** RtlNtStatusToDosError routine prototype. 
 *
 *  The routine is exported by ntdll library and its purpose is to translate 
 *  NTSTATUS values (STATUS_XXX) to Win32 error values (ERROR_XXX). 
 *
 *  @param Status A NTSTATUS value to convert.
 *
 *  @return
 *  Returns Win32 error value associated with the given NTSTATUS one.
 *
 *  @remark
 *  If there is no Win32 error value associated with given NTSTATUS one, the routine
 *  returns ERROR_MR_MID_NOT_FOUND (317) error code. In some cases, the routine 
 *  return the same value as given in its argument.
 *
 *  More information can be found at:
 *  http://msdn.microsoft.com/en-us/library/windows/desktop/ms680600(v=vs.85).aspx.
 */
typedef ULONG (WINAPI RTLNTSTATUSTODOSERROR)(NTSTATUS Status);


/************************************************************************/
/*                  GLOBAL VARIABLES                                    */
/************************************************************************/

/** Stores string representations of Windows Event Hook values. */
static PHASH_TABLE htWinEventHookTable = NULL;
/** Stores descriptions and string representations of Windows error codes. */
static PHASH_TABLE winErrorTable = NULL;
/** Stores descriptions nad string representations of NTSTATUS values. */
static PHASH_TABLE ntStatusTable = NULL;
/** Stores string representations of known Windows message codes. */
static PHASH_TABLE windowsMessagesTable = NULL;
/** Stores network port services for the SCTP protocol. */
static PHASH_TABLE sctpPortTable = NULL;
/** Stores network port services for the DCCP protocol. */
static PHASH_TABLE dccpPortTable = NULL;
/** Stores network port services for the TCP protocol. */
static PHASH_TABLE tcpPortTable = NULL;
/** Stores network port services for the UDP protocol. */
static PHASH_TABLE udpPortTable = NULL;
/** Stores string representations of IRP_MJ_XXX constants (IRP types). */
static PHASH_TABLE irpMajorFunctionTable = NULL;
/** Stores string representations of volume types. */
static PHASH_TABLE volumeDeviceTypeTable = NULL;
/** Stores string representations of Windows Hook types (WH_XXX contants).*/
static PHASH_TABLE windowsHookTable = NULL;
/** Maps Windows errors to NTSTATUS codes. */
static PHASH_TABLE _ErrorToNTSTATUSTable = NULL;
/** Maps NTSTATUS codes to Windows errors. */
static PHASH_TABLE _NTSTATUSToErrorTable = NULL;
/** Maps IOCTL codes to IOCTL_XXX string constants. */
static PHASH_TABLE _ioControlCodeTable = NULL;

/************************************************************************/
/*                    HELPER ROUTINES                                   */
/************************************************************************/


static DWORD _CreateWindowsErrorToNTSTATUSMapping(VOID)
{
   ULONG i = 0;
   DWORD errCode = ERROR_GEN_FAILURE;
   DWORD ret = ERROR_GEN_FAILURE;
   RTLNTSTATUSTODOSERROR *_RtlNtStatusToDosError = NULL;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   _RtlNtStatusToDosError = (RTLNTSTATUSTODOSERROR *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlNtStatusToDosError");
   if (_RtlNtStatusToDosError != NULL) {
      ret = P2PHashTableCreate(37, &_ErrorToNTSTATUSTable);
      if (ret == ERROR_SUCCESS) {
         ret = P2PHashTableCreate(37, &_NTSTATUSToErrorTable);
         if (ret == ERROR_SUCCESS) {
            for (i = 0; i < sizeof(_ntstatus) / sizeof(GENERAL_VALUE); ++i) {
               errCode = _RtlNtStatusToDosError(_ntstatus[i].Value);
               if (errCode == 0 || (errCode != ERROR_MR_MID_NOT_FOUND && errCode != _ntstatus[i].Value)) {
                  ret = P2PHashTableInsert(_ErrorToNTSTATUSTable, errCode, _ntstatus[i].Value);
                  if (ret == ERROR_SUCCESS) {
                     ret = P2PHashTableInsert(_NTSTATUSToErrorTable, _ntstatus[i].Value, errCode);
                  }

                  if (ret != ERROR_SUCCESS)
                     break;
               }
            }
         
            if (ret != ERROR_SUCCESS)
               P2PHashTableDestroy(_NTSTATUSToErrorTable);
         }

         if (ret != ERROR_SUCCESS)
            P2PHashTableDestroy(_ErrorToNTSTATUSTable);
      }
   } else ret = ERROR_NOT_FOUND;

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

static VOID _FreeWindowsErrorToNTSTATUSMapping(VOID)
{
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   P2PHashTableDestroy(_ErrorToNTSTATUSTable);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Attempts to generate human-readable descriptions for all Windows error
 *  codes.
 *
 *  @return
 *  Returns ELibrary error value indicating success or failure of the
 *  operation.
 */
static DWORD _CreateWindowsErrorDescriptions(VOID)
{
   DWORD len = 0;
   ULONG i = 0;
   ULONG j = 0;
   PWCHAR buffer = NULL;
   DWORD formatMessageFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER;
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   ret = ERROR_SUCCESS;
   for (i = 0; i < sizeof(_windowsError) / sizeof(GENERAL_VALUE); ++i) {
      len = FormatMessageW(formatMessageFlags, NULL, _windowsError[i].Value, 0, (LPWSTR)&buffer, 0, NULL);
      if (len > 0) {
         _windowsError[i].Description = buffer;
         buffer = NULL;
         len = 0;
      } else {
         if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            DEBUG_PRINT_LOCATION("Error (%S:%u) has no description (GetLastError=%u)", _windowsError[i].Name, _windowsError[i].Value, GetLastError());
            ret = ERROR_NOT_ENOUGH_MEMORY;
         }
      }

      if (ret != ERROR_SUCCESS) {
         // Something got wrong. Rollback.
         for (j = 0; j < i; ++j) {
            LocalFree(_windowsError[j].Description);
         }

         break;
      }
   }

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/** Deallocates memory used to hold human-readable descriptions for Windows error
 *  codes. The routine is an opposite for @link(_CreateWindowsErrorDescriptions).
 */
static VOID _FreeWindowsErrorDescriptions(VOID)
{
   ULONG i = 0;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   for (i = 0; i < sizeof(_windowsError) / sizeof(GENERAL_VALUE); ++i) {
      if (*(_windowsError[i].Description) != L'\0') {
         LocalFree(_windowsError[i].Description);
         _windowsError[i].Description = L"";
      }
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Attempts to add human-readable description for all NTSTATUS values.
 *
 *  The routine uses RtlNtStatusToDosError to convert the NTSTATUS value to
 *  associated Win32 error code. Then, it uses FormatMessage function to get 
 *  description for that code.
 *
 *  @return
 *  Returns TRUE on success and FALSE otherwise.
 */
static DWORD _CreateNTSTATUSDescriptions(VOID)
{
   ULONG i = 0;
   ULONG j = 0;
   DWORD ret = ERROR_GEN_FAILURE;
   PWCHAR buffer = NULL;
   DWORD len = 0;
   DWORD errCode = 0;
   SIZE_T notAssociatedLen = wcslen(notAssociated) * sizeof(WCHAR);
   RTLNTSTATUSTODOSERROR *_RtlNtStatusToDosError = NULL;
   DWORD formatMessageFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   // RtlNtStatusToDosErrror is a part of any SDK library. It is probably a part of
   // ntdll.lib provided with WDK. So, it must be found manually.
   // The GetModuleHandleW call always succeeds because ntdll.dll is always present.
   _RtlNtStatusToDosError = (RTLNTSTATUSTODOSERROR *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlNtStatusToDosError");
   if (_RtlNtStatusToDosError != NULL) {
      ret = ERROR_SUCCESS;
      // Go through all NTSTATUS values...
      for (i = 0; i < sizeof(_ntstatus) / sizeof(GENERAL_VALUE); ++i) {
         errCode = _RtlNtStatusToDosError(_ntstatus[i].Value);
		   // For some NTSTATUS values, the RtlNtStatusToDosError routine
		   // seems to return NTSTATUS value itself. Example of such value
		   // is 0xC000001d. However, keep in mind that STATUS_SUCCESS, which is
         // a constant of zero, translates to ERROR_SUCCESS, which is also a
         // zero constant, hence there must be an exception for such value
         // (hope there is not more of them).
		   if (errCode == 0 || (errCode != ERROR_MR_MID_NOT_FOUND && errCode != _ntstatus[i].Value)) {
			   len = FormatMessageW(formatMessageFlags, NULL, errCode, 0, (LPWSTR)&buffer, 0, NULL);
			   if (len > 0) {
				   _ntstatus[i].Description = buffer;
				   buffer = NULL;
				   len = 0;
			   } else {
               ret = ERROR_NOT_ENOUGH_MEMORY;
            }
		   } else {
            // No associated Win32 error code found. Fill the desciption with
            // a string for such situations.
            buffer = (PWCHAR)LocalAlloc(LMEM_ZEROINIT, notAssociatedLen + sizeof(WCHAR));
            if (buffer != NULL) {
               CopyMemory(buffer, notAssociated, notAssociatedLen);
               _ntstatus[i].Description = buffer;
            } else {
               ret = ERROR_NOT_ENOUGH_MEMORY;
            }
         }

         if (ret != ERROR_SUCCESS) {
            // Something got wrong. Rollback.
            for (j = 0; j < i; ++j) {
               LocalFree(_ntstatus[j].Description);
            }

            break;
         }
      }
   } else {
      ret = GetLastError();
   }

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/** Deallocates memory used to hold human-readable descriptions for NTSTATUS
 *  values. The routine is an opposite for @link(_CreateNTSTATUSDescriptions).
 */
static VOID _FreeNTSTATUSDescriptions(VOID)
{
   ULONG i = 0;
   DEBUG_ENTER_FUNCTION_NO_ARGS();
      
   for (i = 0; i < sizeof(_ntstatus) / sizeof(GENERAL_VALUE); ++i) {
		LocalFree(_ntstatus[i].Description);
		_ntstatus[i].Description = L"";
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Converts a given bit mask value to a string containing either names for the
 *  nonzero bits, or human-readable descriptions for them.
 *
 *  @param Value Value to convert.
 *  @param Description Determines whether the routine should produce names of the nonzero
 *  bits (FALSE value) or their human-readable descriptions (TRUE value).
 *  @param BitmaskArray An array of @link(BITMASK_VALUE) structures, each describes
 *  meaning of one bit of the mask, or a group of its bits.
 *  @param BitmaskArrayLength Number of entries in the array passed in the third argument.
 *  @param Delimiter A string that is used as a delimiter between names (or descriptions) of
 *  nonzero bits.
 *  @param UnknownString A string that is concatenated to the conversion result if the mask
 *  contains a nonzero bit (or bits) that has no name is description in the BitmaskArray array.
 *  The string is followed by a " (0xX)" suffix where X is a hexadecimal number containing all
 *  such bit values.
 *
 *  @return
 *  If the given value contains no nonzero bits, the routine returns an empty string (pointer to a
 *  null character).
 *
 *  If the given value contains nonzero bits, the routine returns a string containing names of that
 *  bits (or their descriptions) separated by a string given in the Delimiter argument. 
 *
 *  The routine returns NULL in case of failure due to insufficient amount of free memory. The routine
 *  cannot fail for any other reason.
 *
 *  @remark
 *  If the given value contains nonzero bits and the routine succeeds (returns non-NULL value), the
 *  returned string must be freed by a call to @link(_BitMaskStringValueFree) procedure. Otherwise
 *  (even in the zero-value case) no countermeasure is needed.
 */
static PWCHAR _BitMaskValueToString(ULONG Value, BOOLEAN Description, PBITMASK_VALUE BitmaskArray, ULONG BitmaskArrayLength, PWCHAR Delimiter, PWCHAR UnknownString)
{
   ULONG i = 0;
   PWCHAR ret = NULL;
   ULONG tmpValue = Value;
   ULONG MatchedCombined = 0;
   SIZE_T stringLength = 0;
   SIZE_T delimiterLength = 0;
   SIZE_T unknownStringLength = 0;
   ULONG LastCombinedIndex = BitmaskArrayLength - 1;
   DEBUG_ENTER_FUNCTION("Value=0x%x; Description=%u; BitmaskArray=0x%p; BitmaskArrayLength=%u; Delimiter=\"%S\"; UnknownString=\"%S\"", Value, Description, BitmaskArray, BitmaskArrayLength, Delimiter, UnknownString);

/////////////////////////////////////////////////////////////////////////////////
// At first, compute length of the result, so we can allocate the whole buffer 
// by one call.

   delimiterLength = wcslen(Delimiter);

   // Go through the names (or descriptions) of bit groups. They will be only
   // on the beginning of the array. 
   unknownStringLength = wcslen(UnknownString); 
   for (i = 0; i < BitmaskArrayLength; ++i) {
      if (!BitmaskArray[i].Combined) {
         LastCombinedIndex = i - 1;
         break;
      }

      // If the value fully contains the bit group, the name (description) wil be
      // stored in the resulting buffer. 
      if ((tmpValue & BitmaskArray[i].Value) == BitmaskArray[i].Value) {
         MatchedCombined |= BitmaskArray[i].Value;
         stringLength += ( Description ? 
            wcslen(BitmaskArray[i].Description) :
            wcslen(BitmaskArray[i].Name)) + delimiterLength;
      }
   }

   // Remove nonzero bits consumed by bit groups
   tmpValue &= ~(MatchedCombined);

   // Go through names (descritpions) for single bits
   for (i = LastCombinedIndex + 1; i < BitmaskArrayLength; ++i) {
      if ((tmpValue & BitmaskArray[i].Value) == BitmaskArray[i].Value) {
         stringLength += (Description ?
            wcslen(BitmaskArray[i].Description) :
            wcslen(BitmaskArray[i].Name)) + delimiterLength;
         tmpValue &= ~(BitmaskArray[i].Value);
      }
   }

   // If the value still contains nonzero bits, that bits have no name (description).
   // Enlarge the buffer size in order to hold the sufix.
   if (tmpValue != 0) {
      stringLength += unknownStringLength + wcslen(L" (0x00000000)") + delimiterLength;
   }

//////////////////////////////////////////////////////////////////////////////////

   if (stringLength > 0) {
      stringLength *= sizeof(WCHAR);
      ret = (PWCHAR)HeapMemoryAlloc(stringLength + sizeof(WCHAR));
      if (ret != NULL) {
//////////////////////////////////////////////////////////////////////////////////////////
         // Now, copy the data to the allocated resulting buffer.
         PWCHAR currentPosition = ret;

         MatchedCombined = 0;
         LastCombinedIndex = BitmaskArrayLength - 1;
         // Go through the bit groups
         for (i = 0; i < BitmaskArrayLength; ++i) {
            if (!BitmaskArray[i].Combined) {
               LastCombinedIndex = i - 1;
               break;
            }

            if ((Value & BitmaskArray[i].Value) == BitmaskArray[i].Value) {
               SIZE_T memberLen = Description ? wcslen(BitmaskArray[i].Description) : wcslen(BitmaskArray[i].Name);
               PWCHAR text = Description ? BitmaskArray[i].Description : BitmaskArray[i].Name;

               MatchedCombined |= BitmaskArray[i].Value;
               CopyMemory(currentPosition, text, memberLen * sizeof(WCHAR));
               currentPosition += memberLen;
               CopyMemory(currentPosition, Delimiter, delimiterLength * sizeof(WCHAR));
               currentPosition += delimiterLength;
            }
         }

         // Remove bits consumed by bit groups
         Value &= ~(MatchedCombined);

         // Go through the remaining single bits
         for (i = LastCombinedIndex + 1; i < BitmaskArrayLength; ++i) {
            if ((Value & BitmaskArray[i].Value) == BitmaskArray[i].Value) {
               SIZE_T memberLen =  Description ? wcslen(BitmaskArray[i].Description) : wcslen(BitmaskArray[i].Name);
               PWCHAR text = Description ? BitmaskArray[i].Description : BitmaskArray[i].Name;
               
               CopyMemory(currentPosition, text, memberLen * sizeof(WCHAR));
               currentPosition += memberLen;
               CopyMemory(currentPosition, Delimiter, delimiterLength * sizeof(WCHAR));
               currentPosition += delimiterLength;
               Value &= ~(BitmaskArray[i].Value);
            }
         }

         // If there are unknown nonzero bits, add the suffix. 
         if (Value != 0) {
            CopyMemory(currentPosition, UnknownString, unknownStringLength * sizeof(WCHAR));
            currentPosition += unknownStringLength;
            swprintf(currentPosition, wcslen(L" (0x00000000)"), L" (0x%X)%ls", Value, Delimiter);
            currentPosition += wcslen(currentPosition);
         }
         
         currentPosition -= delimiterLength;
         *currentPosition = L'\0';
//////////////////////////////////////////////////////////////////////////////////
      }
   } else {
      // Well, not named bits were found for given bit mask value. There are two
      // possible explanations for such situation:
      // 1) The bit mask value is zero. In that case, we return an empty string.
      // because this is the only situation we do this, we do not allocate and
      // the deallocation function counts on it.
      // 2) The bit mask value is not zero but their set bits does not match
      // any named ones. In such a case, we allocate and return a string meaning
      // that the value is unknown to us.
      if (Value != 0) {
	      ret = (PWCHAR)HeapMemoryAlloc((unknownStringLength + 1) * sizeof(WCHAR));
         if (ret != NULL) {
            CopyMemory(ret, UnknownString, unknownStringLength * sizeof(WCHAR));
            ret[unknownStringLength] = L'\0';
         }
      } else {
         ret = L"";
      }
   }

   DEBUG_EXIT_FUNCTION("\"%S\"", ret);
   return ret;
}

/** Frees a string returned by the @link(_BitMaskValueToString) function.
 *
 *  @param Value Address of a string to free.
 */
static VOID _BitMaskStringValueFree(PWCHAR Value)
{
   DEBUG_ENTER_FUNCTION("Value=\"%S\"", Value);

   // Do not deallocate an empty string. That strings means "the bit mask value
   // is zero" and in such a case, the bit mask conversion function does not
   // allocate the string it returns.
   if (*Value != L'\0') {
      HeapMemoryFree(Value);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/************************************************************************/
/*                     PUBLIC ROUTINES                                  */
/************************************************************************/

/** Converts a given system enumeration value to its string representation.
 *
 *  @param Type Type of the system enumeration value.
 *  @param Description Determines whether the routine will return a string representation
 *  of the given value (FALSE value), or a description that contains more information about
 *  its meaning (TRUE value).
 *  @param Value Value to convert.
 *
 *  @return
 *  Returns either a string representation for the given value, or description of its meaning.
 *  Neither case requires to free the returned string. It is placed inside a read-only memory.
 *  If the user specifies an unknown type of system enumeration, or a value that is not present
 *  within the given system enumeration type, L"unknown" string is returned. Also this string needs
 *  not to be explicitly freed and it is also located in a read-only memory.
 */
PWCHAR EnumerationValueToString(ELibTranslateEnumerationType Type, BOOLEAN Description, ULONG Value)
{
   PWCHAR ret = unknown;
   ULONG maxConstant = 0;
   const PWCHAR *array = NULL;
   DEBUG_ENTER_FUNCTION("Type=%u; Description=%u; Value=%u", Type, Description, Value);

   switch (Type) {
      case ltetRegistryValueType:
         array = _registryValueTypes;
         maxConstant = sizeof(_registryValueTypes);
         break;
      case ltetRegistryValueInformationClass:
         array = _keyValueInformationClass;
         maxConstant = sizeof(_keyValueInformationClass);
         break;
      case ltetRegistryKeyInformationClass:
         array = _keyInformationClass;
         maxConstant = sizeof(_keyInformationClass);
         break;
      case ltetRegistryKeyCreateDisposition:
         array = _registryKeyCreateDisposition;
         maxConstant = sizeof(_registryKeyCreateDisposition);
         break;

      case ltetFileInformationClass:
         array = _fileInformationClass;
         maxConstant = sizeof(_fileInformationClass);
         break;
      case ltetFileVolumeInformationClass:
         array = _fileVOlumeInformationClass;
         maxConstant = sizeof(_fileVOlumeInformationClass);
         break;
      case ltetFileCreateDisposition:
         array = (Description) ? _fileCreateDispositionDescription : _fileCreateDisposition;
         maxConstant = Description ? sizeof(_fileCreateDispositionDescription) : sizeof(_fileCreateDisposition);
         break;

      case ltetSectionSyncType:
         array = _FSFilterSectionSyncType;
         maxConstant = sizeof(_FSFilterSectionSyncType);
         break;

      case ltetVirtualKeyCode:
         array = (Description) ? _virtualKeyCodesLong : _virtualKeyCodesShort;
         maxConstant = Description ? sizeof(_virtualKeyCodesLong) : sizeof(_virtualKeyCodesShort);
         break;

      case ltetNetworkProtocol:
         array = _networkProtokol;
         maxConstant = sizeof(_networkProtokol);
         break;

	  case ltetIRPPnPMinorFunction:
		  array = _irpPnPMinorFunction;
		  maxConstant = sizeof(_irpPnPMinorFunction);
		  break;
	  case ltetIRPPowerMinorFunction:
		  array = _irpPowerMinorFunction;
		  maxConstant = sizeof(_irpPowerMinorFunction);
		  break;
	  case ltetIRPFileSystemMinorFunction:
		  array = _irpFileSystemMinorFunction;
		  maxConstant = sizeof(_irpFileSystemMinorFunction);
		  break;
	  case ltetIRPLockMinorFunction:
		  array = _irpLockMinorFunction;
		  maxConstant = sizeof(_irpLockMinorFunction);
		  break;
	  case ltetIRPFlushMinorFunction:
		  array = _irpFlushMinorFunction;
		  maxConstant = sizeof(_irpFlushMinorFunction);
		  break;
	  case ltetIRPDirectoryMinorFunction:
		  array = _irpDirectoryMinorFunction;
		  maxConstant = sizeof(_irpDirectoryMinorFunction);
		  break;
	  case ltetIRPSystemMinorFunction:
		  array = _irpSystemMinorFunction;
		  maxConstant = sizeof(_irpSystemMinorFunction);
		  break;
      default:
         break;
   }

   if (array != NULL) {
      maxConstant /= sizeof(PWCHAR);
      if (Value < maxConstant) {
         ret = array[Value];
      }
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

/** Converts a given system constant value to human readable its string representation.
 *
 *  @param Type Type of the constant to convert.
 *  @param Description If set to FALSE the routine attempts to return string representation of
 *  the given string constant. If set to TRUE, the routine attempts to return a description for the
 *  given constant.
 *  @param Value Value to translate.
 *
 *  @return
 *  Returns either a string representation of the given system constant value, or its
 *  description. Neither case requires to free the memory occupied by the returned 
 *  string. The returned string lies in a read-only memory. If the  If either an invalid
 *  system constant type is specified, or the given system constant is not recognized by
 *  the library, the routine returns L"unknown" which must not bee freed and is also located
 *  in a read-only memory.
 */
PWCHAR GeneralIntegerValueToString(ELibTranslateIntegerValueType Type, BOOLEAN Description, ULONG Value)
{
   PGENERAL_VALUE ti = NULL;
   PHASH_TABLE table = NULL;
   PWCHAR ret = unknown;
   DEBUG_ENTER_FUNCTION("Type=%u; Description=%u; Value=%u", Type, Description, Value);

   switch (Type) {
      case ltivtNTSTATUS:
         table = ntStatusTable;
         break;

      case ltivtFileIRPMajorFunction:
         table = irpMajorFunctionTable;
         break;
      case ltivtVolumeDeviceType:
         table = volumeDeviceTypeTable;
         break;

      case ltivtSCTPPort:
         table = sctpPortTable;
         break;
      case ltivtTCPPort:
         table = tcpPortTable;
         break;
      case ltivtUDPPort:
         table = udpPortTable;
         break;
      case ltivtDCCPPort:
         table = dccpPortTable;
         break;

      case ltivtWindowsHook:
         table = windowsHookTable;
         break;

      case ltivtWindowsError:
         table = winErrorTable;
         break;

	  case ltivtDeviceControl:
		  table = _ioControlCodeTable;
		  break;
      default:
         break;
   }

   if (table != NULL) {
      ti = GVHashTableGet(table, Value);
      if (ti != NULL) {
         ret = Description ? ti->Description : ti->Name;
      }
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

/** Converts a given bit mask value to a string containing either names for the
 *  nonzero bits, or human-readable descriptions for them.
 *
 *  @param Type Type of bit mask to convert.
 *  @param Description Determines whether the routine should produce names of the nonzero
 *  bits (FALSE value) or their human-readable descriptions (TRUE value).
 *  @param Value A bit mask value to convert.  
 *
 *  @return
 *  If the given value contains no nonzero bits, the routine returns an empty string (pointer to a
 *  null character).
 *
 *  If the given value contains nonzero bits, the routine returns a string containing names of that
 *  bits (or their descriptions) separated by a string given in the Delimiter argument. 
 *
 *  The routine returns NULL in case of failure due to insufficient amount of free memory. The routine
 *  cannot fail for any other reason.
 *
 *  @remark
 *  If the given value contains nonzero bits and the routine succeeds (returns non-NULL value), the
 *  returned string must be freed by a call to @link(BitMaskValueStringFree) procedure. Otherwise
 *  (even in the zero-value case) no countermeasure is needed.
 */
PWCHAR BitMaskValueToString(ELibTranslateBitMaskType Type, BOOLEAN Description, ULONG Value)
{
   ULONG vLen = 0;
   PBITMASK_VALUE v = NULL;
   PWCHAR ret = unknown;
   DEBUG_ENTER_FUNCTION("Type=%u; Description=%u; Value=0x%x", Type, Description, Value);

   switch (Type) {
      case ltbtProcessAccessRights:
         v = _processAcessRights;
         vLen = sizeof(_processAcessRights);
         break;

      case ltbtThreadAccessRights:
         v = _threadAcessRights;
         vLen = sizeof(_threadAcessRights);
         break;

      case ltbtFileAttributes:
         v = _fileAttributes;
         vLen = sizeof(_fileAttributes);
         break;
      case ltbtFileShareAccess:
         v = _fileShareAccess;
         vLen = sizeof(_fileShareAccess);
         break;
      case ltbtFileCreateOptions:
         v = _fileCreateOptions;
         vLen = sizeof(_fileCreateOptions);
         break;
      case ltbtFileAccessRights:
         v = _fileAccessRights;
         vLen = sizeof(_fileAccessRights);
         break;
      case ltbtFileIrpFlags:
         v = _fileIrpFlags;
         vLen = sizeof(_fileIrpFlags);
         break;
      case ltbtFileNotifyFilter:
         v = _fileNotifyFilter;
         vLen = sizeof(_fileNotifyFilter);
         break;

      case ltbtSectionPageProtection:
         v = _sectionPageProtection;
         vLen = sizeof(_sectionPageProtection);
         break;
      case ltbtSecurityInformationClass:
         v = _securityInformationClass;
         vLen = sizeof(_securityInformationClass);
         break;

      case ltbtRegistryKeyAccessRights:
         v = _keyAccessRights;
         vLen = sizeof(_keyAccessRights);
         break;
      case ltbtRegistryKeyCreateOptions:
         v = _registryKeyCreateOptions;
         vLen = sizeof(_registryKeyCreateOptions);
         break;
      case ltbtRegistryKeyRestoreFlags:
         v = _registryRestoreFlags;
         vLen = sizeof(_registryRestoreFlags);
         break;
      case ltbtRegistryKeyHiveFormat:
         v = _registryHiveFormat;
         vLen = sizeof(_registryHiveFormat);
         break;
	  case ltbtIRPReadMinorFunction:
	  case ltbtIRPWriteMinorFunction:
		  v = _irpReadWriteMinorFunction;
		  vLen = sizeof(_irpReadWriteMinorFunction);
		  break;

	  case ltbtIRPReadWriteFlags:
		  v = _irpReadWriteFlags;
		  vLen = sizeof(_irpReadWriteFlags);
		  break;
	  case ltbtIRPCTLFlags:
		  v = _irpCTLFlags;
		  vLen = sizeof(_irpCTLFlags);
		  break;
	  case ltbtIRPOtherFlags:
		  v = _irpOtherFlags;
		  vLen = sizeof(_irpOtherFlags);
		  break;
	  case ltbtIRPPagingReadWrite:
		  v = _irpPagingReadWriteFlags;
		  vLen = sizeof(_irpPagingReadWriteFlags);
		  break;
      default:
         break;
   }

   if (v != NULL) {
      vLen /= sizeof(BITMASK_VALUE);
      ret = _BitMaskValueToString(Value, Description, v, vLen, L", ", unknown);
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

/** Frees a string returned by the @link(BitMaskValueToString) function.
 *
 *  @param Value Address of a string to free.
 */
VOID BitMaskValueStringFree(PWCHAR Str)
{
   DEBUG_ENTER_FUNCTION("Str=0x%p", Str);

   _BitMaskStringValueFree(Str);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Converts Windows Event Hook type constant to its string representation.
 *
 *  @param WinEventHookValue Windows Event Hook type value to convert. 
 *
 *  @return
 *  Returns a string representation of the given Windows Event Hook type value.
 *  If the type value is not recognized, L"unknown" string is returned. The returnes
 *  string must not be freed when no longer needed.
 */
PWCHAR WindowsEventHookToString(ULONG32 WinEventHookValue)
{
	PWCHAR ret = unknown;
   PGENERAL_VALUE ti = NULL;
   DEBUG_ENTER_FUNCTION("WinEventHookValue=0x%x", WinEventHookValue);

   // At first, try to lookup the type value in the table of Windows Event hook
   // type constants. This may fail because Windows Event Hook types are also
   // defined by ranges.
   ti = GVHashTableGet(htWinEventHookTable, WinEventHookValue);
	if (ti != NULL) {
		ret = ti->Name;
	} else {
	   // No direct match. Search the ragnes
		if ((EVENT_AIA_START < WinEventHookValue) && (WinEventHookValue <  EVENT_AIA_END )){
			ret = L"AIA_INDUSTRY";
		}
		
      if ((EVENT_OEM_DEFINED_START < WinEventHookValue) && (WinEventHookValue <  EVENT_OEM_DEFINED_END )){
			ret = L"EVENT_OEM_DEFINED";
		}
		
      if ((EVENT_MIN < WinEventHookValue) && (WinEventHookValue <  EVENT_MAX)){
			ret = L"EVENT_VALUE";
		}
		
      if ((EVENT_UIA_EVENTID_START < WinEventHookValue) && (WinEventHookValue <  EVENT_UIA_EVENTID_END)){
			ret = L"EVENT_UIA_EVENTID";
		}
		
      if ((EVENT_UIA_PROPID_START < WinEventHookValue) && (WinEventHookValue <  EVENT_UIA_PROPID_END)){
			ret = L"EVENT_UIA_PROPID";
		}
	}

   DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}

/** Converts Windows Event Hook type constant to description of its meaning.
 *
 *  @param WinEventHookValue Windows Event Hook type value to convert. 
 *
 *  @return
 *  Returns a description of the given Windows Event Hook type value.
 *  If the type value is not recognized, L"unknown" string is returned. The returnes
 *  string must not be freed when no longer needed.
 */
PWCHAR WindowsEventHookDescriptionToString(ULONG32 WinEventHookValue)
{
	PWCHAR ret = unknown;
   PGENERAL_VALUE ti = NULL;
   DEBUG_ENTER_FUNCTION("WinEventHookValue=0x%x", WinEventHookValue);

   // At first, try to lookup the value description in the table of Windows Event hook
   // type constants. This may fail because Windows Event Hook types are also
   // defined by ranges.
   ti = GVHashTableGet(htWinEventHookTable, WinEventHookValue);
	if (ti != NULL) {
		ret = ti->Description;
	} else {
      // No direct match. Search the ragnes		
		if ((EVENT_AIA_START < WinEventHookValue) && (WinEventHookValue <  EVENT_AIA_END )){
			ret = L"The range of WinEvent constant values specified by the Accessibility Interoperability Alliance (AIA) for use across the industry.";
		}

		if ((EVENT_OEM_DEFINED_START < WinEventHookValue) && (WinEventHookValue <  EVENT_OEM_DEFINED_END )){
			ret = L"The range of WinEvent constant values reserved for OEMs.";
		}

		if ((EVENT_MIN < WinEventHookValue) && (WinEventHookValue <  EVENT_MAX)){
			ret = L"The range of event values";
		}

		if ((EVENT_UIA_EVENTID_START < WinEventHookValue) && (WinEventHookValue <  EVENT_UIA_EVENTID_END)){
			ret = L"The range of WinEvent constant values reserved for UI Automation event identifiers.";
		}

		if ((EVENT_UIA_PROPID_START < WinEventHookValue) && (WinEventHookValue <  EVENT_UIA_PROPID_END)){
			ret = L"The range of WinEvent constant values reserved for UI Automation property-changed event identifiers.";
		}
	}

   DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}

/** Converts Windows message constant to its string representation.
 *
 *  @param WindowsMessage Windows message constant to convert.
 *
 *  @return
 *  Returns a string representation of the given Windows message constant. If the
 *  constant value is not recognized, L"unknown" is returned. If the message is
 *  between WM_USER and WM_APP, the routine retrieves L"WM_USER + X" string where
 *  X is the difference between the given constant and the WM_USER constant. If the
 *  given constant is greater than WM_APP constant, the routine retrieves 
 *  L"WM_APP + X" string where X is the difference between WM_APP and the given
 *  constant.
 *
 *  The routine can fail due to insufficient memory resources. In such a case,
 *  NULL is returned.
 *
 *  @remark
 *  To free the string returned by this function, call @link(WindowsMessagesStringFree).
 *
 *  The routine can fail only when the given Windows message constant is greater than WM_USER.
 */
PWCHAR WindowsMessagesToString(ULONG32 WindowsMessage)
{
	PWCHAR ret = unknown;
   PGENERAL_VALUE ti = NULL;
   SIZE_T wmUserStrLen = wcslen(L"WM_USER + ");
   SIZE_T wmAppStrlen = wcslen(L"WM_APP + ");
   SIZE_T bytesToAlloc = (10 + 1) * sizeof(WCHAR);
   DEBUG_ENTER_FUNCTION("Key=0x%x", WindowsMessage);

   // Attempt to lookup the string representation in the table mapping WM_XXX
   // constants to strings.
   ti = GVHashTableGet(windowsMessagesTable, WindowsMessage);
	if (ti != NULL) {
		ret = ti->Name;
	} else if ((WM_USER < WindowsMessage) && (WindowsMessage < WM_APP)) {
		// The given constant is not in the mapping table and lies between
      // WM_USER and WM_APP. Return the L"WM_USER + X" string.
      bytesToAlloc += ((wmUserStrLen) * sizeof(WCHAR));
		ret = (PWCHAR)HeapMemoryAlloc(bytesToAlloc);
      if (ret != NULL) {
		   ZeroMemory(ret, bytesToAlloc);
		   wcscpy(ret, L"WM_USER + ");
		   swprintf(ret + wmUserStrLen, 10, L"%u", WindowsMessage - WM_USER);
      }
	} else  if (WM_APP < WindowsMessage) {
      // The given constant is not in the mapping table and is greated
      // than WM_APP. Return the L"WM_APP + X" string.
      bytesToAlloc += ((wmAppStrlen) * sizeof(WCHAR));
		ret = (PWCHAR)HeapMemoryAlloc(bytesToAlloc);
      if (ret != NULL) {
		   ZeroMemory(ret, bytesToAlloc);
		   wcscpy(ret, L"WM_APP + ");
		   swprintf(ret + wmAppStrlen, 10, L"%u", WindowsMessage - WM_APP);
      }
   }
	
   DEBUG_EXIT_FUNCTION("\"%S\"", ret);
	return ret;
}

/** Frees buffer allocated by the @link(WindowsMessagesToString) routine.
 *
 *  @param WindowsMessage A Windows message constant that was passed to the
 *  WindowsMessagesToString function.
 *  @param String A string representation of the constant returned by the
 *  WindowsMessagesToString function.
 */
VOID  WindowsMessagesStringFree(ULONG32 WindowsMessage, PWCHAR String)
{
   DEBUG_ENTER_FUNCTION("key=%u; p=\"%S\"", WindowsMessage, String);

	if (WindowsMessage > WM_USER && WindowsMessage != WM_APP) {
		HeapMemoryFree(String);
	}

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Converts a given IPV4 address to string.
 *
 *  @param IPV4Address The IPV4 address to convert.
 *
 *  @return
 *  Returns a string representation of the given IPV4 address. If the routine
 *  fails, NULL is returned.
 */
PWCHAR IPV4ToString(ULONG IPV4Address)
{
   PWCHAR ret = NULL;
   PUCHAR addressBytes = (PUCHAR)&IPV4Address;
   DEBUG_ENTER_FUNCTION("IPV4Address=0x%x", IPV4Address);

   ret = (PWCHAR)HeapMemoryAlloc(16 * sizeof(WCHAR));
   if (ret != NULL) {
      if (swprintf(ret, 16, L"%u.%u.%u.%u", addressBytes[0], addressBytes[1], addressBytes[2], addressBytes[3]) < 0) {
         HeapMemoryFree(ret);
         ret = NULL;
      }
   }

   DEBUG_EXIT_FUNCTION("\"%S\"", ret);
   return ret;
}

/** Converts a given IPV6 address to string.
 *
 *  @param IPV6Address The IPV6 address to convert.
 *
 *  @return
 *  Returns a string representation of the given IPV6 address. If the routine
 *  fails, NULL is returned.
 */
PWCHAR IPV6ToString(PUCHAR IPV6Address)
{
   ULONG i = 0;
   PWCHAR ret = NULL;
   PUINT16 field = (PUINT16)IPV6Address;
   DEBUG_ENTER_FUNCTION("IPV6Address", IPV6Address);

   ret = (PWCHAR)HeapMemoryAlloc(40 * sizeof(WCHAR));
   if (ret != NULL) {
      if (swprintf(ret, 40, L"%X:%X:%X:%X:%X:%X:%X:%X", field[0], field[1], field[2], field[3], field[4], field[5], field[6], field[7]) < 0) {
         HeapMemoryFree(ret);
         ret = NULL;
      }
   }

   DEBUG_EXIT_FUNCTION("\"%S\"", ret);
   return ret;
}

/** Converts a given network port number to name of service that is usually
 *  placed on that port.
 *
 *  @param Port The network port number to convert.
 *  @param Protocol Network protocol number. Only TCP, UDP, DCCP and SCTP protocols
 *  are supported by the function.
 *
 *  @return
 *  Returns a name of network service that usually listens on given port and
 *  protocol.
 */
PWCHAR NetworkPortToString(USHORT Port, ULONG Protocol)
{
   ELibTranslateIntegerValueType t;
   PWCHAR ret = unknown;
   DEBUG_ENTER_FUNCTION("Port=%u; Protocol=%u", Port, Protocol);

   switch (Protocol) {
      case 6:
         t = ltivtTCPPort;
         break;
      case 17:
         t = ltivtUDPPort;
         break;
      case 33:
         t = ltivtDCCPPort;
         break;
      case 132:
         t = ltivtSCTPPort;
         break;
      default:
         break;
   }

   ret = GeneralIntegerValueToString(t, FALSE, Port);

   DEBUG_EXIT_FUNCTION("\"%S\"", ret);
   return ret;
}

NTSTATUS WindowsErrorCodeToNTSTATUS(DWORD ErrorCode)
{
	DWORD err = ERROR_GEN_FAILURE;
	ULONG_PTR ret = 0xC0000225;

	err = P2PHashTableGet(_ErrorToNTSTATUSTable, ErrorCode, &ret);
	if (err == ERROR_NOT_FOUND)
		ret = 0xC0000225;

	return (NTSTATUS)ret;
}

DWORD NTSTATUSCodeToWindowsError(NTSTATUS Status)
{
	DWORD err = ERROR_GEN_FAILURE;
	ULONG_PTR ret = ERROR_NOT_FOUND;

	err = P2PHashTableGet(_NTSTATUSToErrorTable, Status, &ret);
	if (err == ERROR_NOT_FOUND)
		ret = ERROR_NOT_FOUND;

	return (DWORD)ret;
}

PWCHAR IRPFLagsToString(UCHAR MajorFunction, UCHAR MinorFunction, ULONG IRPFlags, BOOLEAN Description)
{
	PWCHAR ret = NULL;
	ELibTranslateBitMaskType bType = ltbtIRPOtherFlags;

	switch (MajorFunction) {
		case IRP_MJ_READ:
		case IRP_MJ_WRITE:
			bType = ltbtIRPReadWriteFlags;
			if (IRPFlags & IRP_PAGING_IO)
				bType = ltbtIRPPagingReadWrite;
			break;
		case IRP_MJ_DEVICE_CONTROL:
		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			bType = ltbtIRPCTLFlags;
			break;
		case IRP_MJ_FILE_SYSTEM_CONTROL:
			if (MinorFunction == IRP_MN_USER_FS_REQUEST || MinorFunction == IRP_MN_KERNEL_CALL)
				bType = ltbtIRPCTLFlags;
			else bType = ltbtIRPOtherFlags;
			break;
		default:
			bType = ltbtIRPOtherFlags;
			break;
	}

	ret = BitMaskValueToString(bType, Description, IRPFlags);

	return ret;
}

/************************************************************************/
/*                 INITIALIZATION AND FINALIZATION                      */
/************************************************************************/

/** Contains information to initialize the hash tables (each element is here for one
    hash table). The array allows to perform the initialization and finalization in a
    (for) loop. 
  */
static GENERAL_VALUE_HASH_TABLE_INIT_STRUCTURE _hashTableInitializationArray [] = {
   {&htWinEventHookTable, 233, _winEventHooks, sizeof(_winEventHooks) / sizeof(GENERAL_VALUE)},
   {&ntStatusTable, 233, _ntstatus, sizeof(_ntstatus) / sizeof(GENERAL_VALUE)},
   {&winErrorTable, 233, _windowsError, sizeof(_windowsError) / sizeof(GENERAL_VALUE)},
   {&windowsMessagesTable, 233, _windowsMessages, sizeof(_windowsMessages) / sizeof(GENERAL_VALUE)},
   {&sctpPortTable, 7, _sctpPort, sizeof(_sctpPort) / sizeof(GENERAL_VALUE)},
   {&dccpPortTable, 11, _dccpPort, sizeof(_dccpPort) / sizeof(GENERAL_VALUE)},
   {&tcpPortTable, 797, _tcpPort, sizeof(_tcpPort) / sizeof(GENERAL_VALUE)},
   {&udpPortTable, 797, _udpPort, sizeof(_udpPort) / sizeof(GENERAL_VALUE)},
   {&irpMajorFunctionTable, 31, _irpMajorFunction, sizeof(_irpMajorFunction) / sizeof(GENERAL_VALUE)},
   {&volumeDeviceTypeTable, 31, _volumeDeviceType, sizeof(_volumeDeviceType) / sizeof(GENERAL_VALUE)},
   {&windowsHookTable, 31, _WindowsHook, sizeof(_WindowsHook) / sizeof(GENERAL_VALUE)},
   {&_ioControlCodeTable, 31, _ioControlCodes, sizeof(_ioControlCodes) / sizeof(GENERAL_VALUE)},
};

/** Initializes the Translation Library.
 *
 *  The initialization stage consists mainly from creating various hash tables and filling them
 *  with information. The hash tables are then used to translate information of certain
 *  kinds to corresponding string representation.
 *
 *  The routine also calls _CreateNTSTATUSDescriptions function in order to fill
 *  descriptions for NTSTATUS codes.
 *
 *  @return
 *  Returns ELibraryError value indication success or failure of the operation.
 */
DWORD TranslatesModuleInit(VOID)
{
   ULONG i = 0;
   ULONG j = 0;
   const ULONG numberOfTablesToInitialize = sizeof(_hashTableInitializationArray) / sizeof(GENERAL_VALUE_HASH_TABLE_INIT_STRUCTURE);
   PGENERAL_VALUE_HASH_TABLE_INIT_STRUCTURE item = &_hashTableInitializationArray[0];
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   // Lookup descriptions for NTSTATUS values.
   ret = _CreateNTSTATUSDescriptions();
   if (ret == ERROR_SUCCESS) {
      // Lookup descriptions for Windows error codes.
      ret = _CreateWindowsErrorDescriptions();
      if (ret == ERROR_SUCCESS) {
         // Create hash tables to hold all types of system constants supported by
         // the library
         for (i = 0; i < numberOfTablesToInitialize; ++i) {
            ret = GVHashTableCreate(item->Size, item->HashTable);
            if (ret != ERROR_SUCCESS) {
               --item;
               for (j = 0; j < i; ++j) {
                  HashTableDestroy(*(item->HashTable));
                  --item;
               }

               break;
            }

            ++item;
         }

         if (ret == ERROR_SUCCESS) {
            // Fill the hash tables created above with the integer-to-string mapping
            // for the system constants.
            item = &_hashTableInitializationArray[0];
            for (i = 0; i < numberOfTablesToInitialize; ++i) {
               PGENERAL_VALUE textItem = item->ItemArray;

               for (j = 0; j < item->NumberOfItems; ++j) {
                  GVHashTableInsert(*(item->HashTable), textItem);
                  ++textItem;
               }

               ++item;
            }   

            ret = _CreateWindowsErrorToNTSTATUSMapping();
         }
      
         if (ret != ERROR_SUCCESS) {
            _FreeWindowsErrorDescriptions();
         }
      }

      if (ret != ERROR_SUCCESS) {
         _FreeNTSTATUSDescriptions();
      }
   }

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/** Cleans up resources used by the Translation Library.
 *
 *  The library allocates only resources related to hash tables and NTSTATUS
 *  description. The routine destroys all the hash tables and deallocates
 *  the descriptions for the NTSTATUS values.
 */
VOID TranslatesModuleFinit(VOID)
{
   ULONG i = 0;
   PGENERAL_VALUE_HASH_TABLE_INIT_STRUCTURE item = &_hashTableInitializationArray[0];
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   _FreeWindowsErrorToNTSTATUSMapping();
   // Destroy all the hash tables
   for (i = 0; i < sizeof(_hashTableInitializationArray) / sizeof(GENERAL_VALUE_HASH_TABLE_INIT_STRUCTURE); ++i) {
      HashTableDestroy(*(item->HashTable));
      ++item;
   }

   // Delete Windows error descriptions
   _FreeWindowsErrorDescriptions();

   // Delete NTSTATUS descriptions
   _FreeNTSTATUSDescriptions();

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}
