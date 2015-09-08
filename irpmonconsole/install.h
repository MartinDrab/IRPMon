
#ifndef __IRPMON_CONSOLE_INSTALL_H__
#define __IRPMON_CONSOLE_INSTALL_H__


#define MAX_DIR_NAME                   32767



DWORD Install(PWCHAR FileName, PWCHAR ServiceName, DWORD ServiceStart, PWCHAR *Function);
DWORD Uninstall(PWCHAR ServiceName);



#endif 
