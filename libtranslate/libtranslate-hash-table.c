
/**
 * @file: hash_table.c
 *
 * Tento soubor implementuje hashovaci tabulku pro obecne pouziti. Tato
 * datova struktura resi kolize metodou retezeni prvku.
 */

#include <windows.h>
#include "debug.h"
#include "allocator.h"
#include "libtranslate-hash-table.h"


/************************************************************************/
/*                                HLEPER ROUTINES                       */
/************************************************************************/

static VOID HashTableLockShared(PHASH_TABLE Table, ULONG32 Index)
{
   EnterCriticalSection(&Table->Lock[Index]);

   return;
}


static VOID HashTableLockExclusive(PHASH_TABLE Table, ULONG32 Index)
{
	EnterCriticalSection(&Table->Lock[Index]);

   return;
}


static VOID HashTableUnlockExclusive(PHASH_TABLE Table, ULONG32 Index)
{
   LeaveCriticalSection(&Table->Lock[Index]);

   return;
}


static VOID HashTableUnlockShared(PHASH_TABLE Table, ULONG32 Index)
{
	LeaveCriticalSection(&Table->Lock[Index]);

   return;
}


/************************************************************************/
/*                                PUBLIC ROUTINES                       */
/************************************************************************/

/** Vytvori novou instanci hashovaci tabulky.
 *
 * Funkce vytvori hashovaci tabulku se zadanym poctem slotu a se zadanou
 * hashovaci, porovnavaci a uklidovou funkci.
 *
 * @param Size Udava pocet slotu hashovaci tabulky.
 * @param HashFunction Adresa hashovaci funkce. Tato hodnota musi nest
 * platna data.
 * @param CompareFunction Adresa porovnavaci funkce. Tato hodnota musi
 * nest platna data.
 * @param FreeFunction Adresa uklidove funkce. Pokud volajici specifikuje
 * NULL, hashovaci tabulka nebude pri svem mazani volat zadnou uklidovou
 * funkci.
 * @param Table. Adresa promenne, do ktere rutina v pripade uspechu zapise adresu
 * struktury nove hashovaci tabulky.
 *
 * @return Rutina vraci hodnotu NTSTATUS indikujici uspech ci neuspech operace.
 */
DWORD HashTableCreate(IN ULONG32 Size, IN HASH_FUNCTION HashFunction, IN COMPARE_FUNCTION CompareFunction, IN FREE_ITEM_FUNCTION FreeFunction, OUT PHASH_TABLE *Table)
{
	ULONG i, j = 0;
	PHASH_TABLE tmpTable = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Size=%u; HashFunction=0x%p; CompareFunction=0x%p; FreeFunction=0x%p; Table=0x%p", Size, HashFunction, CompareFunction, FreeFunction, Table);

	tmpTable = (PHASH_TABLE)HeapMemoryAlloc(sizeof(HASH_TABLE) + Size * sizeof(PHASH_ITEM));
	if (tmpTable != NULL) {
		tmpTable->Size = Size;
		tmpTable->HashFunction = HashFunction;
		tmpTable->CompareFunction = CompareFunction;
		tmpTable->FreeFunction = FreeFunction;
		tmpTable->Lock = (PCRITICAL_SECTION)HeapMemoryAlloc(Size * sizeof(CRITICAL_SECTION));
		if (tmpTable->Lock != NULL) {
			ret = ERROR_SUCCESS;
			for (i = 0; i < Size; i++) {
				tmpTable->Buckets[i] = NULL;
				if (!InitializeCriticalSectionAndSpinCount(&tmpTable->Lock[i], 0x1000))
					ret = GetLastError();

				if (ret != ERROR_SUCCESS) {
					for (j = 0; j < i; ++j)
						DeleteCriticalSection(&tmpTable->Lock[j]);
					
					break;
				}
			}

			if (ret == ERROR_SUCCESS)
				*Table = tmpTable;

			if (ret != ERROR_SUCCESS)
				HeapMemoryFree(tmpTable->Lock);
		} else ret = ERROR_NOT_ENOUGH_MEMORY;

		if (ret != ERROR_SUCCESS)
			HeapMemoryFree(tmpTable);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	DEBUG_EXIT_FUNCTION("%u, *Table=0x%p\n", ret, *Table);
	return ret;
}


/** Odstrani celou hashovaci tabulku z pameti.
 *
 * Na kazdy objekt, ktery se v tabulce nachazi, je zavolana uklidova funkce
 * (prvek FreeFunction ve strukture hashovaci tabulky), ktera by mela zajistit
 * jeho uvolneni z pameti.
 *
 * @param Table Hashovaci tabulka, kterou si volajici preje odstranit.
 */
VOID HashTableDestroy(IN PHASH_TABLE Table)
{
   ULONG i = 0;
   PHASH_ITEM Tmp = NULL;
   PHASH_ITEM Bucket = NULL;
   DEBUG_ENTER_FUNCTION("Table=%p", Table);

   for (i = 0; i < Table->Size; i++) {
      Bucket = Table->Buckets[i];
      if (Bucket != NULL) {
         do {
            Tmp = Bucket;
            Bucket = Bucket->Next;
            if (Table->FreeFunction != NULL) {
               Table->FreeFunction(Tmp);
            }
         } while (Bucket != NULL);
      }

	  DeleteCriticalSection(&Table->Lock[i]);
   }

   HeapMemoryFree(Table->Lock);
   HeapMemoryFree(Table);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/** Prida novy prvek do hashovaci tabulky.
 *
 * @param Table Cilova hashovaci tabulka.
 * @param Object Prvek, ktery si volajici preje pridat.
 * @param Key Hodnota klice, pod kterym ma byt prvek pridan.
 */
VOID HashTableInsert(IN PHASH_TABLE Table, IN PHASH_ITEM Object, IN PVOID Key)
{
	ULONG32 Index = 0;
	DEBUG_ENTER_FUNCTION("Tabulka=0x%p; Object=0x%p; Key=0x%p", Table, Object, Key);

	Index = Table->HashFunction(Key) % Table->Size;
	HashTableLockExclusive(Table, Index);
	Object->Next = Table->Buckets[Index];
	Table->Buckets[Index] = Object;
	HashTableUnlockExclusive(Table, Index);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/** Odstrani prvek pod zadanym klicem z hashovaci tabulky.
 *
 * Na cilovy objekt neni volana uklidova funkce.
 *
 * @param Table Cilova hashovaci tabulka.
 * @param Key Klic identifikujici hledany objekt.
 *
 * @return Pokud se cilovy objekt podari z tabulky vyjmout, funkce
 * vrati jeho adresu. V pripade neuspechu vraci NULL.
 */
PHASH_ITEM HashTableDelete(IN PHASH_TABLE Table, IN PVOID Key)
{
   ULONG32 Index = 0;
   PHASH_ITEM Akt = NULL;
   PHASH_ITEM Prev = NULL;
   BOOLEAN Ret = FALSE;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Key=0x%p", Table, Key);

   Index = Table->HashFunction(Key) % Table->Size;
   HashTableLockExclusive(Table, Index);
   Akt = Table->Buckets[Index];
   Prev = NULL;
   while (Akt != NULL) {
      Ret = Table->CompareFunction(Akt, Key);
      if (Ret) {
         if (Prev != NULL) {
            Prev->Next = Akt->Next;
         } else {
            Table->Buckets[Index] = Akt->Next;
         }

         break;
      }

      Prev = Akt;
      Akt = Akt->Next;
   }

   HashTableUnlockExclusive(Table, Index);

   DEBUG_EXIT_FUNCTION("0x%p", Akt);
   return Akt;
}


/** Vrati adresu objektu, ktery se v hashovaci tabulce nachazi pod
 *  zadanym klicem.
 *
 *  @param Table Cilova hashovaci tabulka.
 *  @param Key Klic identifikujici hledany objekt.
 *
 *  @return Funkce vrati adresu objektu, ktery byl do tabulky vlozen pod
 *  zadanym klicem. V pripade, ze objekt se v tabulce nenachazi, rutina
 *  vrati NULL.
 */
PHASH_ITEM HashTableGet(IN PHASH_TABLE Table, IN PVOID Key)
{
   BOOLEAN Ret = FALSE;
   ULONG32 Index = 0;
   PHASH_ITEM Akt = NULL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Key=0x%p", Table, Key);

   Index = Table->HashFunction(Key) % Table->Size;
   HashTableLockShared(Table, Index);
   Akt = Table->Buckets[Index];
   while (Akt != NULL) {
      Ret = Table->CompareFunction(Akt, Key);
      if (Ret) {
         break;
      }

      Akt = Akt->Next;
   }

   HashTableUnlockShared(Table, Index);

   DEBUG_EXIT_FUNCTION("0x%p", Akt);
   return Akt;
}


VOID HashTablePerform(PHASH_TABLE Table, HASH_ITEM_CALLBACK Callback, PVOID Context)
{
   ULONG i = 0;
   PHASH_ITEM tmp = NULL;
   PHASH_ITEM old = NULL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Callback=0x%p; Context=0x%p", Table, Callback, Context);

   for (i = 0; i < Table->Size; ++i) {
      HashTableLockExclusive(Table, i);
      tmp = Table->Buckets[i];
      while (tmp != NULL) {
         old = tmp;
         tmp = tmp->Next;
         Callback(old, Context);
      }

      HashTableUnlockExclusive(Table, i);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

DWORD HashTablePerformFeedback(PHASH_TABLE Table, HASH_ITEM_FEEDBACK_CALLBACK *Callback, PVOID Context)
{
   ULONG i = 0;
   PHASH_ITEM tmp = NULL;
   PHASH_ITEM old = NULL;
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Callback=0x%p; Context=0x%p", Table, Callback, Context);

   ret = ERROR_SUCCESS;
   for (i = 0; i < Table->Size; ++i) {
      HashTableLockExclusive(Table, i);
      tmp = Table->Buckets[i];
      while (ret == ERROR_SUCCESS && tmp != NULL) {
         old = tmp;
         tmp = tmp->Next;
         ret = Callback(old, Context);
      }

      HashTableUnlockExclusive(Table, i);
      if (ret != ERROR_SUCCESS) {
         break;
      }
   }

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}



VOID HashTableClear(PHASH_TABLE Table, BOOLEAN CallFreeFunction)
{
   ULONG i = 0;
   PHASH_ITEM old = NULL;
   PHASH_ITEM akt = NULL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; CallFreeFunction=%u", Table, CallFreeFunction);

   for (i = 0; i < Table->Size; ++i) {
      HashTableLockExclusive(Table, i);
      if (CallFreeFunction && Table->FreeFunction != NULL) {
         akt = Table->Buckets[i];
         while (akt != NULL) {
            old = akt;
            akt = akt->Next;
            Table->FreeFunction(old);
         }
      }

      Table->Buckets[i] = NULL;
      HashTableUnlockExclusive(Table, i);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}
