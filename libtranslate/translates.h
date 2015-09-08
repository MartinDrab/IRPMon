
/**
 * @file
 *
 * Header file containing definitions of functions and some data types required
 * to translate vairous types of system constants to (human-readable) strings.
 */

#ifndef __LIBTRANSLATE_TRANSLATES_H__
#define __LIBTRANSLATE_TRANSLATES_H__

#include <windows.h>
#include "libtranslate-hash-table.h"
#include "gv-hash-table.h"
#include "libtranslate.h"

/** Represents a string information about a value that is a part of a system
    enumeration. */
typedef struct {
   /** String representation of the enumeration constant. It is usualy the same
       as name of the constant. */
   PWCHAR Name;
   /** Meaning of the enumeration constant. */
   PWCHAR Description;
} ENUMERATION_VALUE, *PENUMERATION_VALUE;

/** Stores mapping of one bit of a bit mask. */
typedef struct {
   /** Name of the bit mask value. */
   PWCHAR Name;
   /** Bit mask value (can be a single bit or a combination of bits) */
   ULONG Value;
   /** Longer description of the value. */
   PWCHAR Description;
   /** Set to TRUE when the structure represent combined bit mask value. */
   BOOLEAN Combined;
} BITMASK_VALUE, *PBITMASK_VALUE;


/** Contains information about one hash table and its contents. The structure
    is used during library initialization and finalization stages to create or
    destroy corresponding hash table. 
  */
typedef struct {
   /** Address that receives address of newly created hash tablei nstance. */
   PHASH_TABLE *HashTable;
   /** Size of the hash table to create, in buckets. */
   ULONG Size;
   /** Pointer to an array of TEXT_ITEM structures the hash table will be filled
       with after its successful creation. */
   PGENERAL_VALUE ItemArray;
   /** Number of elements in the array pointed by the ItemArray member. */
   ULONG NumberOfItems;
} GENERAL_VALUE_HASH_TABLE_INIT_STRUCTURE, *PGENERAL_VALUE_HASH_TABLE_INIT_STRUCTURE;

PWCHAR EnumerationValueToString(ELibTranslateEnumerationType Type, BOOLEAN Description, ULONG Value);
PWCHAR GeneralIntegerValueToString(ELibTranslateIntegerValueType Type, BOOLEAN Description, ULONG Value);
PWCHAR BitMaskValueToString(ELibTranslateBitMaskType Type, BOOLEAN Description, ULONG Value);
VOID BitMaskValueStringFree(PWCHAR Str);

PWCHAR WindowsMessagesToString(ULONG32 Key);
VOID  WindowsMessagesStringFree(ULONG32 key, PWCHAR p);
PWCHAR WindowsEventHookToString(ULONG32 Key);
PWCHAR WindowsEventHookDescriptionToString(ULONG32 Key);

PWCHAR IPV4ToString(ULONG ipv4);
PWCHAR IPV6ToString(PUCHAR ipv6);
PWCHAR NetworkPortToString(USHORT Port, ULONG Protocol);

NTSTATUS WindowsErrorCodeToNTSTATUS(DWORD ErrorCode);
DWORD NTSTATUSCodeToWindowsError(NTSTATUS Status);

PWCHAR IRPFLagsToString(UCHAR MajorFunction, UCHAR MinorFunction, ULONG IRPFlags, BOOLEAN Description);


DWORD TranslatesModuleInit(VOID);
VOID TranslatesModuleFinit(VOID);



#endif 
