
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libserver.h"







int main(int argc, char **argv)
{
	int ret = 0;
	wchar_t* tmp = NULL;
	wchar_t fileName[MAX_PATH];

	if (argc != 3) {
		ret = -1;
		fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
		goto Exit;
	}

	memset(fileName, 0, sizeof(fileName));
	GetModuleFileNameW(NULL, fileName, sizeof(fileName) / sizeof(fileName[0]));
	tmp = fileName + wcslen(fileName);
	while (tmp != fileName && *tmp != L'\\')
		--tmp;

	if (*tmp == L'\\')
		++tmp;

	wcscpy(tmp, L"irpmndrv.sys");
	ret = IRPMonServerStartDriver(fileName, FALSE);
	if (ret != ERROR_SUCCESS) {
		fprintf(stderr, "[ERROR]: IRPMonServerStartDriver: %u\n", ret);
		goto Exit;
	}

	ret = IRPMonServerStart(argv[1], argv[2], NULL);
	if (ret != 0) {
		fprintf(stderr, "[ERROR]: Unable to start the server: %u\n", ret);
		goto Exit;
	}

Exit:
	return ret;
}
