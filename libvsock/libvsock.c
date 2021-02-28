
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <Objbase.h>
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


int LibVSockHyperVAddressAlloc(const GUID *VMId, const GUID *AppId, struct sockaddr **Address, int *Len)
{
	int ret = 0;
	struct SOCKADDR_HV *addr;

	addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct SOCKADDR_HV));
	if (addr != NULL) {
		addr->Family = AF_HYPERV;
		addr->VmId = *VMId;
		addr->ServiceId = *AppId;
		*Address = (struct sockaddr*)addr;
		*Len = sizeof(struct SOCKADDR_HV);
		ret = 0;
	}
	else ret = GetLastError();

	return ret;
}


void LibVSockAddressFree(struct sockaddr *Address)
{
	HeapFree(GetProcessHeap(), 0, Address);

	return;
}


int LibVSockAddressPrintA(const struct sockaddr *Address, char *Buffer, size_t CharCount)
{
	int ret = 0;

	memset(Buffer, 0, CharCount*sizeof(char));
	switch (Address->sa_family) {
		case AF_INET: {
			const struct sockaddr_in *a = (const struct sockaddr_in *)Address;
			
			ret = snprintf(Buffer, CharCount, "%u.%u.%u.%u:%u", a->sin_addr.S_un.S_un_b.s_b1, a->sin_addr.S_un.S_un_b.s_b2, a->sin_addr.S_un.S_un_b.s_b3, a->sin_addr.S_un.S_un_b.s_b4, htons(a->sin_port));
		} break;
		case AF_INET6: {
			const struct sockaddr_in6 *a = (const struct sockaddr_in6 *)Address;

		} break;
		case AF_HYPERV: {
			wchar_t vmGuid[100];
			wchar_t appGuid[100];
			const struct SOCKADDR_HV *a = (const struct SOCKADDR_HV *)Address;

			memset(vmGuid, 0, sizeof(vmGuid));
			memset(appGuid, 0, sizeof(appGuid));
			ret = StringFromGUID2(&a->VmId, vmGuid, sizeof(vmGuid) / sizeof(vmGuid[0]));
			if (ret != 0)
				ret = StringFromGUID2(&a->ServiceId, appGuid, sizeof(appGuid) / sizeof(appGuid[0]));
			
			if (ret != 0)
				snprintf(Buffer, CharCount, "%ls:%ls", vmGuid, appGuid);
		} break;
		default:
			if (Address->sa_family == LibVSockGetAddressFamily()) {
				const struct sockaddr_vm *a = (const struct sockaddr_vm *)Address;

				ret = snprintf(Buffer, CharCount, "0x%x:%u", a->svm_cid, a->svm_port);
			}
			break;
	}

	return ret;
}


int LibVSockAddressPrintW(const struct sockaddr *Address, wchar_t *Buffer, size_t CharCount)
{
	int ret = 0;

	memset(Buffer, 0, CharCount * sizeof(wchar_t));
	switch (Address->sa_family) {
		case AF_INET: {
			const struct sockaddr_in *a = (const struct sockaddr_in*)Address;

			ret = swprintf(Buffer, CharCount, L"%u.%u.%u.%u:%u", a->sin_addr.S_un.S_un_b.s_b1, a->sin_addr.S_un.S_un_b.s_b2, a->sin_addr.S_un.S_un_b.s_b3, a->sin_addr.S_un.S_un_b.s_b4, htons(a->sin_port));
		} break;
		case AF_INET6: {
			const struct sockaddr_in6 *a = (const struct sockaddr_in6*)Address;

		} break;
		case AF_HYPERV: {
			wchar_t vmGuid[100];
			wchar_t appGuid[100];
			const struct SOCKADDR_HV* a = (const struct SOCKADDR_HV*)Address;

			memset(vmGuid, 0, sizeof(vmGuid));
			memset(appGuid, 0, sizeof(appGuid));
			ret = StringFromGUID2(&a->VmId, vmGuid, sizeof(vmGuid) / sizeof(vmGuid[0]));
			if (ret != 0)
				ret = StringFromGUID2(&a->ServiceId, appGuid, sizeof(appGuid) / sizeof(appGuid[0]));

			if (ret != 0)
				swprintf(Buffer, CharCount, L"%ls:%ls", vmGuid, appGuid);
		} break;
		default:
			if (Address->sa_family == LibVSockGetAddressFamily()) {
				const struct sockaddr_vm *a = (const struct sockaddr_vm*)Address;

				ret = swprintf(Buffer, CharCount, L"0x%x:%u", a->svm_cid, a->svm_port);
			}
			break;
	}

	return ret;
}
