
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libserver.h"







int main(int argc, char **argv)
{
	int ret = 0;

	if (argc != 3) {
		ret = -1;
		fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
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
