
#ifndef __HIPS_DLISTS_H_
#define __HIPS_DLISTS_H_

#include <windows.h>


typedef BOOLEAN (DLIST_ACTION_FEEDBACK_CALLBACK)(PLIST_ENTRY Item, PVOID Context);
typedef VOID (DLIST_ACTION_CALLBACK)(PLIST_ENTRY Item, PVOID Context);

typedef enum {
   dldForward,
   dldBackward
} EDListDirection, *PEDListDirection;


VOID FORCEINLINE _InitializeListHead(PLIST_ENTRY Head) 
{
  Head->Blink = Head;
  Head->Flink = Head;
}

BOOL FORCEINLINE _IsListEmpty(PLIST_ENTRY Entry)
{
  return (Entry->Flink == Entry && Entry->Flink == Entry->Blink);
}

VOID FORCEINLINE _InsertTailList(PLIST_ENTRY Head, PLIST_ENTRY Entry)
{
  Entry->Blink = Head->Blink;
  Entry->Flink = Head;
  Head->Blink->Flink = Entry;
  Head->Blink = Entry;
}

VOID FORCEINLINE _RemoveEntryList(PLIST_ENTRY Entry)
{
  Entry->Blink->Flink = Entry->Flink;
  Entry->Flink->Blink = Entry->Blink;
  _InitializeListHead(Entry);
}


PLIST_ENTRY FORCEINLINE _ListNextItem(PLIST_ENTRY Head, EDListDirection Direction)
{
   return (Direction == dldForward) ? Head->Flink : Head->Blink;
}


VOID FORCEINLINE _ListPerformWithFeedback(PLIST_ENTRY Head, EDListDirection Direction, DLIST_ACTION_FEEDBACK_CALLBACK *Routine, PVOID Context)
{
   PLIST_ENTRY tmp = _ListNextItem(Head, Direction);

   while (tmp != Head) {
      if (!Routine(tmp, Context))
         break;

      tmp = _ListNextItem(tmp, Direction);
   }

   return;
}

VOID FORCEINLINE _ListPerform(PLIST_ENTRY Head, EDListDirection Direction, DLIST_ACTION_CALLBACK *Routine, PVOID Context)
{
   PLIST_ENTRY tmp = _ListNextItem(Head, Direction);

   while (tmp != Head) {
      Routine(tmp, Context);
      tmp = _ListNextItem(tmp, Direction);
   }

   return;
}


#endif
