   
/**
 * @file
 *
 * Header file for the the General Value Table implementation.
 */

#ifndef __LIBTRANSLATE_GENERAL_VALUE_HASH_TABLE_H__
#define __LIBTRANSLATE_GENERAL_VALUE_HASH_TABLE_H__

#include <windows.h>
#include "libtranslate-hash-table.h"

/** Stores human-readable information about an integer value, usually a system-defined
    constant. */
typedef struct {
   /** Links the structure into General Value Table. */
   HASH_ITEM HashItem;
   /** String representation of the integer value, usually set to the name of constant
       the value represents. */
   PWCHAR Name;
   /** The integer value described by the structure. */
   ULONG32 Value;
   /** Meaning of the integer value. */
   PWCHAR Description;
} GENERAL_VALUE, *PGENERAL_VALUE;

PGENERAL_VALUE GVHashTableGet(PHASH_TABLE Table, ULONG Key);
DWORD GVHashTableCreate(ULONG32 Size, PHASH_TABLE *Table);
VOID GVHashTableInsert(PHASH_TABLE Table, PGENERAL_VALUE TextItem);
VOID GVHashTableDestroy(PHASH_TABLE Table);



#endif 
