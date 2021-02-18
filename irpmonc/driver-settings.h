
#ifndef __DRIVER_SETTINGS_H__
#define __DRIVER_SETTINGS_H__


#include "irpmonc.h"


#ifdef __cplusplus
extern "C" {
#endif

int parse_setting(EOptionType Type, const wchar_t *Value);
int sync_settings(void);
void print_settings(void);

#ifdef __cplusplus
}
#endif



#endif
