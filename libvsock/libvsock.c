
#include <winsock2.h>
#include <windows.h>
#include <vmci_sockets.h>
#include "libvsock.h"



unsigned short LibVSockGetAddressFamily(void)
{
	return VMCISock_GetAFValue();
}


unsigned int LibVSockGetVersion(void)
{
	return VMCISock_Version();
}


unsigned int LibVSockGetLocalId(void)
{
	return VMCISock_GetLocalCID();
}


int LibVSockAddressAlloc(unsigned int CID, unsigned int Port, struct sockaddr **Address, int *Len)
{
	int ret = 0;
	struct sockaddr_vm *addr;

	addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct sockaddr_vm));
	if (addr != NULL) {
		addr->svm_family = LibVSockGetAddressFamily();
		addr->svm_cid = CID;
		addr->svm_port = Port;
		*Address = (struct sockaddr *)addr;
		*Len = sizeof(struct sockaddr_vm);
		ret = 0;
	} else ret = GetLastError();

	return ret;
}


void LibVSockAddressFree(struct sockaddr *Address)
{
	HeapFree(GetProcessHeap(), 0, Address);

	return;
}
