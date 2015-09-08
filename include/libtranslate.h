
/**
 * @file
 *
 *  Interface exported by the Translation Library.
 */

#ifndef __LIB_TRANSLATE_H__
#define __LIB_TRANSLATE_H__

#include <windows.h>

#ifdef LIBTRANSLATE_EXPORTS

#define LIBTRANSLATEAPI EXTERN_C __declspec(dllexport)

#else 

#define LIBTRANSLATEAPI EXTERN_C __declspec(dllimport)

#endif

/** Defines types of system bit masks supported by the Translation Library. */
typedef enum {
   /** Process access rights (PROCESS_XXX constants define individual bits). */
   ltbtProcessAccessRights,
   /** Thread access rights (THREAD_XXX constants define individual bits). */
   ltbtThreadAccessRights,
   /** File access rights (FILE_XXX nad DIRECTORY_XXX constants define individual bits). */
   ltbtFileAccessRights,
   /** File sharing (FILE_SHARE_XXX constants define individual bits). */
   ltbtFileShareAccess,
   /** File attributes (FILE_ATTRIBUTE_XXX constants define individual bits). */
   ltbtFileAttributes,
   /** Options related to file create or open operation (FILE_FLAG_XXX constants define individual bits). */
   ltbtFileCreateOptions,
   /** File operation settings (IRP_XXX constants define individual bits). */
   ltbtFileIrpFlags,
   ltbtFileNotifyFilter,
   /** Memory page protection (individual bits are defined by PAGE_XXX). */
   ltbtSectionPageProtection,
   /** Registry key access rights (individual bits are defined by KEY_XXX constants). */
   ltbtRegistryKeyAccessRights,
   /** Registry key create and open options (REG_XXX constants define individual bits). */
   ltbtRegistryKeyCreateOptions,
   /** Registry key restore operation settings. */
   ltbtRegistryKeyRestoreFlags,
   /** Registry hive save/load operation settings. */
   ltbtRegistryKeyHiveFormat,
   /** Information types for query and set security operation (XXX_SECURITY_INFORMATION constants define individual bits). */
   ltbtSecurityInformationClass,

   ltbtIRPReadMinorFunction,
   ltbtIRPWriteMinorFunction,
   ltbtIRPReadWriteFlags,
   ltbtIRPCTLFlags,
   ltbtIRPOtherFlags,
   ltbtIRPPagingReadWrite,
} ELibTranslateBitMaskType, *PELibTranslateBitMaskType;

/** Defines types of system enumerations supported by the library. */
typedef enum {
   /** Type of registry value (REG_xxx constants). */
   ltetRegistryValueType, 
   /** Type of information to query or set on a given key (KeyXXXInformation constants). */
   ltetRegistryKeyInformationClass,
   /** Type of information to query or set on a given value (KeyValueXXXInformation constants). */
   ltetRegistryValueInformationClass,
   /** Indicates whether a just-performed create operation actually created a new registry key or
       just opened the existing one. */
   ltetRegistryKeyCreateDisposition,

   /** Type of information to query or set on a given file (FileXXXInformation constants). */
   ltetFileInformationClass,
   /** Indicates whether a just-performed create operation actually created a new registry file or
       just opened the existing one. */
   ltetFileCreateDisposition,
   /** Type of information to query or set on a given volume (VolumeXXXInformation constants). */
   ltetFileVolumeInformationClass,

   /** Type of section that is being acquired for synchronization. */
   ltetSectionSyncType,
   /** Virtual key code (VKEY_XXX constants). */
   ltetVirtualKeyCode,
   /** Network protocol. */
   ltetNetworkProtocol,

   ltetIRPPnPMinorFunction,
   ltetIRPPowerMinorFunction,
   ltetIRPFileSystemMinorFunction,
   ltetIRPLockMinorFunction,
   ltetIRPFlushMinorFunction,
   ltetIRPDirectoryMinorFunction,
   ltetIRPSystemMinorFunction,
} ELibTranslateEnumerationType, *PELibTranslateEnumerationType;

/** Defines types of system constants supported by the library. */
typedef enum {
   /** NTSTATUS codes (NTSTATUS_XXX constants). */
   ltivtNTSTATUS,
   /** Windows Hook types (WH_XXX constants). */
   ltivtWindowsHook,
   /** IRP major types (IRP_MJ_XXX constants). */
   ltivtFileIRPMajorFunction,
   /** Volume device types. */
   ltivtVolumeDeviceType,
   /** Network service port numbers for SCTP protocol. */
   ltivtSCTPPort,
   /** Network service port numbers for TCP protocol. */
   ltivtTCPPort,
   /** Network service port numbers for UDP protocol. */
   ltivtUDPPort,
   /** Network service port numbers for DCCP protocol. */
   ltivtDCCPPort,
   /** Windows error codes. */
   ltivtWindowsError,
   ltivtDeviceControl,
} ELibTranslateIntegerValueType, *PELibTranslateIntegerValueType;

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
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateEnumerationValueToString(ELibTranslateEnumerationType Type, BOOLEAN Description, ULONG Value);

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
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateGeneralIntegerValueToString(ELibTranslateIntegerValueType Type, BOOLEAN Description, ULONG Value);

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
 *  returned string must be freed by a call to @link(LibTranslateBitMaskValueStringFree) procedure. Otherwise
 *  (even in the zero-value case) no countermeasure is needed.
 */
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateBitMaskValueToString(ELibTranslateBitMaskType Type, BOOLEAN Description, ULONG Value);

/** Frees a string returned by the @link(LibTranslateBitMaskValueToString) function.
 *
 *  @param Value Address of a string to free.
 */
LIBTRANSLATEAPI VOID WINAPI LibTranslateBitMaskValueStringFree(PWCHAR Str);


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
 *  To free the string returned by this function, call @link(LibTranslateWindowsMessagesStringFree).
 *
 *  The routine can fail only when the given Windows message constant is greater than WM_USER.
 */
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateWindowsMessagesToString(ULONG32 Key);

/** Frees buffer allocated by the @link(LibTranslateWindowsMessagesToString) routine.
 *
 *  @param WindowsMessage A Windows message constant that was passed to the
 *  WindowsMessagesToString function.
 *  @param String A string representation of the constant returned by the
 *  WindowsMessagesToString function.
 */
LIBTRANSLATEAPI VOID  WINAPI LibTranslateWindowsMessagesStringFree(ULONG32 key, PWCHAR p);

/** Converts Windows Event Hook type constant to its string representation.
 *
 *  @param WinEventHookValue Windows Event Hook type value to convert. 
 *
 *  @return
 *  Returns a string representation of the given Windows Event Hook type value.
 *  If the type value is not recognized, L"unknown" string is returned. The returnes
 *  string must not be freed when no longer needed.
 */
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateWindowsEventHookToString(ULONG32 Key);

/** Converts Windows Event Hook type constant to description of its meaning.
 *
 *  @param WinEventHookValue Windows Event Hook type value to convert. 
 *
 *  @return
 *  Returns a description of the given Windows Event Hook type value.
 *  If the type value is not recognized, L"unknown" string is returned. The returnes
 *  string must not be freed when no longer needed.
 */
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateWindowsEventHookDescriptionToString(ULONG32 Key);

/** Converts a given IPV4 address to string.
 *
 *  @param IPV4Address The IPV4 address to convert.
 *
 *  @return
 *  Returns a string representation of the given IPV4 address. If the routine
 *  fails, NULL is returned.
 *
 *  @remark
 *  To free the string returned by the function, call the @link(LibTranslateFreeMemory) routine.
 */
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateIPV4ToString(ULONG ipv4);

/** Converts a given IPV6 address to string.
 *
 *  @param IPV6Address The IPV6 address to convert.
 *
 *  @return
 *  Returns a string representation of the given IPV6 address. If the routine
 *  fails, NULL is returned.
 *
 *  @remark
 *  To free the string returned by the function, call the @link(LibTranslateFreeMemory) routine.
 */
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateIPV6ToString(PUCHAR ipv6);

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
LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateNetworkPortToString(USHORT Port, ULONG Protocol);

/** Frees memory allocated by some functions exported by the library, such as
 *  @link(LibTranslateIPV4ToString).
 *
 *  @param Address Address of buffer to free.
 */
LIBTRANSLATEAPI VOID WINAPI LibTranslateFreeMemory(PVOID Address);

LIBTRANSLATEAPI NTSTATUS WINAPI LibTranslateWindowsErrorCodeToNTSTATUS(DWORD ErrorCode);
LIBTRANSLATEAPI DWORD WINAPI LibTranslateNTSTATUSCodeToWindowsError(NTSTATUS Status);

LIBTRANSLATEAPI PWCHAR WINAPI LibTranslateIRPFLagsToString(UCHAR MajorFunction, UCHAR MinorFunction, ULONG IRPFlags, BOOLEAN Description);



/** Initializes the library. The routine must be called before any other routine
 *  exported by the library.
 *
 *  @return
 *  Returns ELibraryError value indicating success or failure of the operation.
 */
LIBTRANSLATEAPI DWORD WINAPI LibTranslateInitialize(VOID);

/** Cleans up resources used by the library. No library-exported function can be
 *  called after this routine.
 */
LIBTRANSLATEAPI VOID WINAPI LibTranslateFinalize(VOID);


#endif 
