
#ifndef __HASH_TABLE_H_
#define __HASH_TABLE_H_

/**
 * @file: hash_table.h
 *
 * Hlavickovy soubor k souboru hash_table.c, ktery implementuje obecnou
 * hashovaci tabulku. Kolize se resi retezenim prvku.
 */

#include <windows.h>



// Definice struktry prvku hashovaci tabulky
typedef struct _HASH_ITEM {
   // Odkaz na naslednika ve spojovem seznamu
   struct _HASH_ITEM *Next;
} HASH_ITEM, *PHASH_ITEM;


/** Prototyp hashovaci funkce
 *
 * Rutina se musi chovat jako hashovaci funkce, tedy napriklad vracet
 * stejne hodnoty hashe pro stejne hodnoty klice.
 *
 * @param Key Klic, ktery je nutne zahashovat.
 *
 * @return Funkce musi vratit integer odpovidajici zahashovane hodnote
 * klice. Neni nutne provadet moduleni na pocet slotu.
 */
typedef ULONG32 (*HASH_FUNCTION) (IN PVOID Key);

/** Prototyp porovnavci funkce. Funkce musi vratit TRUE, pokud objekt
 *  z hashovaci tabulky byl vlozen pod zadanym klicem.
 *
 *  Implementace hashovaci tabulky nedovoluje duplicity klicu. Porovnavaci
 *  funkce se pouziva pri testu, zda dany objekt jiz v tabulce existuje ci
 *  nikoliv.
 *
 *  @param ObjectInTable Adresa objektu leziciho v hashovaci tabulce.
 *  @param Key Hodnota klice.
 *
 *  @return Funkce musi vratit TRUE, pokud hodnota parametru Key odpovida
 *  hodnote klice, se kterou byl objekt ObjectInTable vlozen do hashovaci
 *  tabulky.
 */
typedef BOOLEAN (*COMPARE_FUNCTION) (IN PHASH_ITEM ObjectInTable, IN PVOID Key);


/** Tato rutina slouzi k uklidu objektu pri odstranovani hashovaci
 *  tabulky.
 *
 *  Rutina je volana na pouze behem mazani cele hashovaci tabulky na
 *  kazdy objekt, ktery se v ni prave v ten okamzik nachazi.
 *
 * @param Object Objekt, ktery je treba uklidit.
 */
typedef VOID (*FREE_ITEM_FUNCTION) (IN PHASH_ITEM Object);

/** Prototype of the callback that is invoked for every item stored inside the
 *  table when a HashTablePerform is called.
 *
 *  @param Object Address of the item.
 *  @param Context User-defined variable supplied as an argument to the
 *  HashTablePerform call.
 */
typedef VOID (*HASH_ITEM_CALLBACK)(IN PHASH_ITEM Object, PVOID Context);

typedef DWORD (HASH_ITEM_FEEDBACK_CALLBACK)(PHASH_ITEM Object, PVOID Context);

// Struktura hashovaci tabulky
typedef struct _HASH_TABLE {
   // Pocet slotu
   ULONG32 Size;
   // Adresa hashovaci funkce
   HASH_FUNCTION HashFunction;
   // Porovnavaci funkce
   COMPARE_FUNCTION CompareFunction;
   // Funkce pro uklid objektu pri mazani hashovaci tabulky
   FREE_ITEM_FUNCTION FreeFunction;
   // Zamky reader-writer pro jednotlive sloty
   PCRITICAL_SECTION Lock;
   // Sloty
   PHASH_ITEM Buckets[1];
} HASH_TABLE, *PHASH_TABLE;

// Vyznam jednotlivych rutin najdete v komentarich u jejich implementace
// v souboru hash_table.c

DWORD HashTableCreate(IN ULONG32 Size, IN HASH_FUNCTION HashFunction, IN COMPARE_FUNCTION CompareFunction, IN FREE_ITEM_FUNCTION FreeFunction, OUT PHASH_TABLE *Table);
VOID HashTableDestroy(IN PHASH_TABLE Table);
VOID HashTableInsert(IN PHASH_TABLE Table, IN PHASH_ITEM Object, IN PVOID Key);
PHASH_ITEM HashTableDelete(IN PHASH_TABLE Table, IN PVOID Key);
PHASH_ITEM HashTableGet(IN PHASH_TABLE Table, IN PVOID Key);
VOID HashTablePerform(PHASH_TABLE Table, HASH_ITEM_CALLBACK Callback, PVOID Context);
VOID HashTableClear(PHASH_TABLE Table, BOOLEAN CallFreeFunction);
DWORD HashTablePerformFeedback(PHASH_TABLE Table, HASH_ITEM_FEEDBACK_CALLBACK *Callback, PVOID Context);

#endif
