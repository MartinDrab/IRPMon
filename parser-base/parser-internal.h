
#ifndef __PARSER_INTERNAL_H__
#define __PARSER_INTERNAL_H__



#include <stdint.h>
#include <windows.h>
#include "parser-base.h"


typedef struct _PARSER_PLUGIN {
	LIST_ENTRY ConditionListHead;
	wchar_t *Name;
	wchar_t *Description;
} PARSER_PLUGIN, *PPARSER_PLUGIN;





#endif
