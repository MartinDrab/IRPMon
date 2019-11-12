
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include <sddl.h>
#include "general-types.h"
#include "parser-base.h"
#include "secdesc.h"



#define IRP_MJ_QUERY_SECURITY		0x14
#define IRP_MJ_SET_SECURITY			0x15


static DWORD _PrintSid(PNV_PAIR Pair, const wchar_t *Name, BOOLEAN InAce, PSID Sid)
{
	SID_NAME_USE sidType;
	DWORD ret = ERROR_GEN_FAILURE;
	wchar_t *stringSid = NULL;
	wchar_t accountName[MAX_PATH];
	wchar_t domainName[MAX_PATH];
	DWORD accountNameLen = sizeof(accountName) / sizeof(accountName[0]);
	DWORD domainNameLen = sizeof(domainName) / sizeof(domainName[0]);
	wchar_t *sidTypeNames[] = {
		L"Undefined",
		L"User",
		L"Group",
		L"Domain",
		L"Alias",
		L"Well-known group",
		L"Deleted account",
		L"Invalid",
		L"Unknown",
		L"Computer",
		L"Mandatory label",
	};
	const wchar_t *sidTypeName = NULL;
	const wchar_t *sidNameConst = L"      Raw";
	const wchar_t *accNameConst = L"      Name";
	const wchar_t *domNameConst = L"      Domain";
	const wchar_t *typNameConst = L"      Type";
	const wchar_t *tp2NameConst = L"      Type value";


	if (!InAce) {
		sidNameConst += 4;
		accNameConst += 4;
		domNameConst += 4;
		typNameConst += 4;
		tp2NameConst += 4;
		ret = PBaseAddNameValue(Pair, Name, L"");
	} else ret = PBaseAddNameValue(Pair, L"    SID", L"");

	if (ret == ERROR_SUCCESS) {
		if (ConvertSidToStringSid(Sid, &stringSid)) {
			ret = PBaseAddNameValue(Pair, sidNameConst, stringSid);
			LocalFree(stringSid);
		}
	}

	if (LookupAccountSidW(NULL, Sid, accountName, &accountNameLen, domainName, &domainNameLen, &sidType)) {
		sidTypeName = sidTypeNames[0];
		if (sidType <= SidTypeLabel)
			sidTypeName = sidTypeNames[sidType];

		ret = PBaseAddNameValue(Pair, accNameConst, accountName);
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameValue(Pair, domNameConst, domainName);

		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameValue(Pair, typNameConst, sidTypeName);
		
		if (ret == ERROR_SUCCESS)
			ret = PBaseAddNameFormat(Pair, tp2NameConst, L"%u", sidType);
	}

	return ret;
}


static DWORD _PrintMask(PNV_PAIR Pair, DWORD Mask)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = PBaseAddNameFormat(Pair, L"    Mask", L"0x%x", Mask);

	return ret;
}

static DWORD _PrintACL(PNV_PAIR Pair, const wchar_t *Name, const ACL *Acl)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const ACCESS_ALLOWED_ACE *aaa = NULL;
	const ACCESS_DENIED_ACE *ada = NULL;
	const SYSTEM_AUDIT_ACE *saua = NULL;
	const SYSTEM_ALARM_ACE *sala = NULL;
	const SYSTEM_MANDATORY_LABEL_ACE *smla = NULL;
	const ACE_HEADER *aceHeader = NULL;
	wchar_t *stringSid = NULL;

	if (Acl != NULL) {
		ret = PBaseAddNameValue(Pair, Name, L"");
		if (ret == ERROR_SUCCESS) {
			ret = PBaseAddNameFormat(Pair, L"  Revision", L"%u", Acl->AclRevision);
			if (ret == ERROR_SUCCESS)
				ret = PBaseAddNameFormat(Pair, L"  Size", L"%u", Acl->AclSize);

			if (ret == ERROR_SUCCESS)
				ret = PBaseAddNameFormat(Pair, L"  ACE count", L"%u", Acl->AceCount);

			if (ret == ERROR_SUCCESS) {
				for (DWORD i = 0; i < Acl->AceCount; ++i) {
					if (GetAce(Acl, i, (PVOID *)&aceHeader)) {
						PBaseAddNameFormat(Pair, L"  ACE", L"%u", i);
						PBaseAddNameFormat(Pair, L"    Flags", L"0x%x", aceHeader->AceFlags);
						PBaseAddNameFormat(Pair, L"    Type", L"%u", aceHeader->AceType);
						switch (aceHeader->AceType) {
						case ACCESS_ALLOWED_ACE_TYPE:
							aaa = CONTAINING_RECORD(aceHeader, ACCESS_ALLOWED_ACE, Header);
							ret = _PrintSid(Pair, L"SID", TRUE, &aaa->SidStart);
							if (ret == ERROR_SUCCESS)
								ret = _PrintMask(Pair, aaa->Mask);
							break;
						case ACCESS_DENIED_ACE_TYPE:
							ada = CONTAINING_RECORD(aceHeader, ACCESS_DENIED_ACE, Header);
							ret = _PrintSid(Pair, L"SID", TRUE, &ada->SidStart);
							if (ret == ERROR_SUCCESS)
								ret = _PrintMask(Pair, ada->Mask);
							break;
						case SYSTEM_AUDIT_ACE_TYPE:
							saua = CONTAINING_RECORD(aceHeader, SYSTEM_AUDIT_ACE, Header);
							ret = _PrintSid(Pair, L"SID", TRUE, &saua->SidStart);
							if (ret == ERROR_SUCCESS)
								ret = _PrintMask(Pair, saua->Mask);
							break;
						case SYSTEM_ALARM_ACE_TYPE:
							sala = CONTAINING_RECORD(aceHeader, SYSTEM_ALARM_ACE, Header);
							ret = _PrintSid(Pair, L"SID", TRUE, &sala->SidStart);
							if (ret == ERROR_SUCCESS)
								ret = _PrintMask(Pair, sala->Mask);
							break;
						case SYSTEM_MANDATORY_LABEL_ACE_TYPE:
							smla = CONTAINING_RECORD(aceHeader, SYSTEM_MANDATORY_LABEL_ACE, Header);
							ret = _PrintSid(Pair, L"SID", TRUE, &smla->SidStart);
							if (ret == ERROR_SUCCESS)
								ret = _PrintMask(Pair, smla->Mask);
							break;
						}
					}
				}
			}
		}
	} else ret = PBaseAddNameValue(Pair, Name, L"null");

	return ret;
}


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	DWORD ret = ERROR_GEN_FAILURE;
	size_t length = 0;
	const SECURITY_DESCRIPTOR *data = NULL;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	SECURITY_INFORMATION si;
	BOOL present = FALSE;
	BOOL defaulted = FALSE;
	PACL acl = NULL;
	PSID binarySid = NULL;

	ret = ERROR_NOT_SUPPORTED;
	RtlSecureZeroMemory(&p, sizeof(p));
	switch (Request->Type) {
		case ertIRP:
			irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
			if (irp->MajorFunction == IRP_MJ_SET_SECURITY) {
				si = (SECURITY_INFORMATION)irp->Arg1;
				data = (SECURITY_DESCRIPTOR *)(irp + 1);
				length = irp->DataSize;
				ret = ERROR_SUCCESS;
			}
			break;
		case ertIRPCompletion:
			irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
			if (irpComp->MajorFunction == IRP_MJ_QUERY_SECURITY) {
				si = (SECURITY_INFORMATION)irpComp->Arguments[0];
				data = (SECURITY_DESCRIPTOR *)(irpComp + 1);
				length = irpComp->DataSize;
				ret = ERROR_SUCCESS;
			}
			break;
		default:
			break;
	}

	if (ret == ERROR_SUCCESS) {
		if (IsValidSecurityDescriptor(data)) {
			ret = PBaseAddNameFormat(&p, L"Revision", L"%u", data->Revision);
			if (ret == ERROR_SUCCESS)
				ret = PBaseAddNameFormat(&p, L"Control", L"%u", data->Control);

			if (ret == ERROR_SUCCESS && GetSecurityDescriptorOwner(data, &binarySid, &defaulted))
				ret = _PrintSid(&p, L"Owner", FALSE, binarySid);
			
			if (ret == ERROR_SUCCESS && GetSecurityDescriptorGroup(data, &binarySid, &defaulted))
				ret = _PrintSid(&p, L"Group", FALSE, binarySid);

			if (ret == ERROR_SUCCESS && GetSecurityDescriptorDacl(data, &present, &acl, &defaulted) && present)
				ret = _PrintACL(&p, L"DACL", acl);

			if (ret == ERROR_SUCCESS && GetSecurityDescriptorSacl(data, &present, &acl, &defaulted) && present)
				ret = _PrintACL(&p, L"SACL", acl);
		} else ret = ERROR_INVALID_PARAMETER;

		if (ret == ERROR_SUCCESS) {
			*Names = p.Names;
			*Values = p.Values;
			*RowCount = p.Count;
			*Handled = TRUE;
		}

		if (ret != ERROR_SUCCESS) {
			for (size_t i = 0; i < p.Count; ++i)
				HeapFree(GetProcessHeap(), 0, p.Names[i]);

			HeapFree(GetProcessHeap(), 0, p.Names);
		}
	} else if (ret == ERROR_NOT_SUPPORTED) {
		*Handled = FALSE;
		ret = ERROR_SUCCESS;
	}

	return ret;
}


static void cdecl _FreeRoutine(wchar_t **Names, wchar_t **Values, size_t Count)
{
	PBaseFreeNameValue(Names, Values, Count);

	return;
}


DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PIRPMON_DATA_PARSER tmpParser = NULL;

	ret = ERROR_SUCCESS;
	if (RequestedVersion >= IRPMON_DATA_PARSER_VERSION_1) {
		ret = PBaseDataParserAlloc(IRPMON_DATA_PARSER_VERSION_1, &tmpParser);
		if (ret == ERROR_SUCCESS) {
			tmpParser->MajorVersion = 1;
			tmpParser->MinorVersion = 0;
			tmpParser->BuildVersion = 0;
			tmpParser->Name = L"SecDescs";
			tmpParser->Description =L"Display security descriptors in a nice shape";
			tmpParser->Priority = 1;
			tmpParser->ParseRoutine = _ParseRoutine;
			tmpParser->FreeRoutine = _FreeRoutine;
			*Parser = tmpParser;
		}
	} else ret = ERROR_NOT_SUPPORTED;

	return ret;
}
