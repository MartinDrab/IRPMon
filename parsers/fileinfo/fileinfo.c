
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "parser-base.h"
#include "fileinfo.h"



#define IRP_MJ_QUERY_INFORMATION			0x5
#define IRP_MJ_SET_INFORMATION				0x6


typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,                   // 2
	FileBothDirectoryInformation,                   // 3
	FileBasicInformation,                           // 4
	FileStandardInformation,                        // 5
	FileInternalInformation,                        // 6
	FileEaInformation,                              // 7
	FileAccessInformation,                          // 8
	FileNameInformation,                            // 9
	FileRenameInformation,                          // 10
	FileLinkInformation,                            // 11
	FileNamesInformation,                           // 12
	FileDispositionInformation,                     // 13
	FilePositionInformation,                        // 14
	FileFullEaInformation,                          // 15
	FileModeInformation,                            // 16
	FileAlignmentInformation,                       // 17
	FileAllInformation,                             // 18
	FileAllocationInformation,                      // 19
	FileEndOfFileInformation,                       // 20
	FileAlternateNameInformation,                   // 21
	FileStreamInformation,                          // 22
	FilePipeInformation,                            // 23
	FilePipeLocalInformation,                       // 24
	FilePipeRemoteInformation,                      // 25
	FileMailslotQueryInformation,                   // 26
	FileMailslotSetInformation,                     // 27
	FileCompressionInformation,                     // 28
	FileObjectIdInformation,                        // 29
	FileCompletionInformation,                      // 30
	FileMoveClusterInformation,                     // 31
	FileQuotaInformation,                           // 32
	FileReparsePointInformation,                    // 33
	FileNetworkOpenInformation,                     // 34
	FileAttributeTagInformation,                    // 35
	FileTrackingInformation,                        // 36
	FileIdBothDirectoryInformation,                 // 37
	FileIdFullDirectoryInformation,                 // 38
	FileValidDataLengthInformation,                 // 39
	FileShortNameInformation,                       // 40
	FileIoCompletionNotificationInformation,        // 41
	FileIoStatusBlockRangeInformation,              // 42
	FileIoPriorityHintInformation,                  // 43
	FileSfioReserveInformation,                     // 44
	FileSfioVolumeInformation,                      // 45
	FileHardLinkInformation,                        // 46
	FileProcessIdsUsingFileInformation,             // 47
	FileNormalizedNameInformation,                  // 48
	FileNetworkPhysicalNameInformation,             // 49
	FileIdGlobalTxDirectoryInformation,             // 50
	FileIsRemoteDeviceInformation,                  // 51
	FileUnusedInformation,                          // 52
	FileNumaNodeInformation,                        // 53
	FileStandardLinkInformation,                    // 54
	FileRemoteProtocolInformation,                  // 55
	//
	//  These are special versions of these operations (defined earlier)
	//  which can be used by kernel mode drivers only to bypass security
	//  access checks for Rename and HardLink operations.  These operations
	//  are only recognized by the IOManager, a file system should never
	//  receive these.
	//
	FileRenameInformationBypassAccessCheck,         // 56
	FileLinkInformationBypassAccessCheck,           // 57
	//
	// End of special information classes reserved for IOManager.
	//
	FileVolumeNameInformation,                      // 58
	FileIdInformation,                              // 59
	FileIdExtdDirectoryInformation,                 // 60
	FileReplaceCompletionInformation,               // 61
	FileHardLinkFullIdInformation,                  // 62
	FileIdExtdBothDirectoryInformation,             // 63
	FileDispositionInformationEx,                   // 64
	FileRenameInformationEx,                        // 65
	FileRenameInformationExBypassAccessCheck,       // 66
	FileDesiredStorageClassInformation,             // 67
	FileStatInformation,                            // 68
	FileMemoryPartitionInformation,                 // 69
	FileStatLxInformation,                          // 70
	FileCaseSensitiveInformation,                   // 71
	FileLinkInformationEx,                          // 72
	FileLinkInformationExBypassAccessCheck,         // 73
	FileStorageReserveIdInformation,                // 74
	FileCaseSensitiveInformationForceAccessCheck,   // 75
	FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_DIRECTORY_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	ULONG EaSize;
	WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_ID_FULL_DIR_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	ULONG EaSize;
	LARGE_INTEGER FileId;
	WCHAR FileName[1];
} FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	ULONG EaSize;
	CCHAR ShortNameLength;
	WCHAR ShortName[12];
	WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG FileAttributes;
	ULONG FileNameLength;
	ULONG EaSize;
	CCHAR ShortNameLength;
	WCHAR ShortName[12];
	LARGE_INTEGER FileId;
	WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
	ULONG NextEntryOffset;
	ULONG FileIndex;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_OBJECTID_INFORMATION {
	LONGLONG FileReference;
	UCHAR ObjectId[16];
	union {
		struct {
			UCHAR BirthVolumeId[16];
			UCHAR BirthObjectId[16];
			UCHAR DomainId[16];
		};
		UCHAR ExtendedInfo[48];
	};
} FILE_OBJECTID_INFORMATION, *PFILE_OBJECTID_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG NumberOfLinks;
	BOOLEAN DeletePending;
	BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION_EX {
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG NumberOfLinks;
	BOOLEAN DeletePending;
	BOOLEAN Directory;
	BOOLEAN AlternateStream;
	BOOLEAN MetadataAttribute;
} FILE_STANDARD_INFORMATION_EX, *PFILE_STANDARD_INFORMATION_EX;

typedef struct _FILE_INTERNAL_INFORMATION {
	LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION {
	ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
	ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION {
	LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;
															// ntddk wdm nthal
typedef struct _FILE_MODE_INFORMATION {
	ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {                // ntddk nthal
	ULONG AlignmentRequirement;                             // ntddk nthal
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION; // ntddk nthal
															// ntddk nthal
typedef struct _FILE_NAME_INFORMATION {                     // ntddk
	ULONG FileNameLength;                                   // ntddk
	WCHAR FileName[1];                                      // ntddk
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;           // ntddk
															// ntddk
typedef struct _FILE_ALL_INFORMATION {
	FILE_BASIC_INFORMATION BasicInformation;
	FILE_STANDARD_INFORMATION StandardInformation;
	FILE_INTERNAL_INFORMATION InternalInformation;
	FILE_EA_INFORMATION EaInformation;
	FILE_ACCESS_INFORMATION AccessInformation;
	FILE_POSITION_INFORMATION PositionInformation;
	FILE_MODE_INFORMATION ModeInformation;
	FILE_ALIGNMENT_INFORMATION AlignmentInformation;
	FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {               // ntddk nthal
	ULONG FileAttributes;                                       // ntddk nthal
	ULONG ReparseTag;                                           // ntddk nthal
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;  // ntddk 

typedef struct _FILE_ALLOCATION_INFORMATION {
	LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION {
	LARGE_INTEGER CompressedFileSize;
	USHORT CompressionFormat;
	UCHAR CompressionUnitShift;
	UCHAR ChunkShift;
	UCHAR ClusterShift;
	UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_SFIO_RESERVE_INFORMATION {
	ULONG RequestsPerPeriod;
	ULONG Period;
	BOOLEAN RetryFailures;
	BOOLEAN Discardable;
	ULONG RequestSize;
	ULONG NumOutstandingRequests;
} FILE_SFIO_RESERVE_INFORMATION, *PFILE_SFIO_RESERVE_INFORMATION;

typedef struct _FILE_SFIO_VOLUME_INFORMATION {
	ULONG MaximumRequestsPerPeriod;
	ULONG MinimumPeriod;
	ULONG MinimumTransferSize;
} FILE_SFIO_VOLUME_INFORMATION, *PFILE_SFIO_VOLUME_INFORMATION;

typedef enum _IO_PRIORITY_HINT {
	IoPriorityVeryLow = 0,          // Defragging, content indexing and other background I/Os
	IoPriorityLow,                  // Prefetching for applications.
	IoPriorityNormal,               // Normal I/Os
	IoPriorityHigh,                 // Used by filesystems for checkpoint I/O
	IoPriorityCritical,             // Used by memory manager. Not available for applications.
	MaxIoPriorityTypes
} IO_PRIORITY_HINT;

typedef struct _FILE_IO_PRIORITY_HINT_INFORMATION {
	IO_PRIORITY_HINT   PriorityHint;
} FILE_IO_PRIORITY_HINT_INFORMATION, *PFILE_IO_PRIORITY_HINT_INFORMATION;

typedef struct _FILE_IO_PRIORITY_HINT_INFORMATION_EX {
	IO_PRIORITY_HINT   PriorityHint;
	BOOLEAN            BoostOutstanding;
} FILE_IO_PRIORITY_HINT_INFORMATION_EX, *PFILE_IO_PRIORITY_HINT_INFORMATION_EX;

#define FILE_SKIP_COMPLETION_PORT_ON_SUCCESS    0x1
#define FILE_SKIP_SET_EVENT_ON_HANDLE           0x2
#define FILE_SKIP_SET_USER_EVENT_ON_FAST_IO     0x4

typedef  struct _FILE_IO_COMPLETION_NOTIFICATION_INFORMATION {
	ULONG Flags;
} FILE_IO_COMPLETION_NOTIFICATION_INFORMATION, *PFILE_IO_COMPLETION_NOTIFICATION_INFORMATION;

typedef  struct _FILE_PROCESS_IDS_USING_FILE_INFORMATION {
	ULONG NumberOfProcessIdsInList;
	ULONG_PTR ProcessIdList[1];
} FILE_PROCESS_IDS_USING_FILE_INFORMATION, *PFILE_PROCESS_IDS_USING_FILE_INFORMATION;

typedef struct _FILE_IS_REMOTE_DEVICE_INFORMATION {
	BOOLEAN IsRemote;
} FILE_IS_REMOTE_DEVICE_INFORMATION, *PFILE_IS_REMOTE_DEVICE_INFORMATION;

typedef struct _FILE_NUMA_NODE_INFORMATION {
	USHORT NodeNumber;
} FILE_NUMA_NODE_INFORMATION, *PFILE_NUMA_NODE_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {                  // ntddk nthal
	BOOLEAN DeleteFile;                                         // ntddk nthal
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION; // ntddk nthal
																// ntddk nthal
typedef struct _FILE_END_OF_FILE_INFORMATION {                  // ntddk nthal
	LARGE_INTEGER EndOfFile;                                    // ntddk nthal
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION; // ntddk nthal
																// ntddk nthal
typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {                                    // ntddk nthal
	LARGE_INTEGER ValidDataLength;                                                      // ntddk nthal
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef struct _FILE_LINK_INFORMATION {
	BOOLEAN ReplaceIfExists;
	HANDLE RootDirectory;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;


typedef struct _FILE_MOVE_CLUSTER_INFORMATION {
	ULONG ClusterCount;
	HANDLE RootDirectory;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_MOVE_CLUSTER_INFORMATION, *PFILE_MOVE_CLUSTER_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION {
	BOOLEAN ReplaceIfExists;
	HANDLE RootDirectory;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION {
	ULONG NextEntryOffset;
	ULONG StreamNameLength;
	LARGE_INTEGER StreamSize;
	LARGE_INTEGER StreamAllocationSize;
	WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_TRACKING_INFORMATION {
	HANDLE DestinationFile;
	ULONG ObjectInformationLength;
	CHAR ObjectInformation[1];
} FILE_TRACKING_INFORMATION, *PFILE_TRACKING_INFORMATION;

typedef struct _FILE_COMPLETION_INFORMATION {
	HANDLE Port;
	PVOID Key;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

typedef struct _FILE_PIPE_INFORMATION {
	ULONG ReadMode;
	ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
	ULONG NamedPipeType;
	ULONG NamedPipeConfiguration;
	ULONG MaximumInstances;
	ULONG CurrentInstances;
	ULONG InboundQuota;
	ULONG ReadDataAvailable;
	ULONG OutboundQuota;
	ULONG WriteQuotaAvailable;
	ULONG NamedPipeState;
	ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_PIPE_REMOTE_INFORMATION {
	LARGE_INTEGER CollectDataTime;
	ULONG MaximumCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;


typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
	ULONG MaximumMessageSize;
	ULONG MailslotQuota;
	ULONG NextMessageSize;
	ULONG MessagesAvailable;
	LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION {
	PLARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_REPARSE_POINT_INFORMATION {
	LONGLONG FileReference;
	ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;



static BOOLEAN _hideZeroValues = TRUE;




static DWORD _ProcessFileTime(PNV_PAIR Pairs, const wchar_t *Name, const LARGE_INTEGER *Time)
{
	SYSTEMTIME st;
	DWORD ret = ERROR_GEN_FAILURE;

	ret = ERROR_SUCCESS;
	switch (Time->QuadPart) {
		case 0:
			ret = PBaseAddNameValue(Pairs, Name, L"<keep unchanged>");
			break;
		case -1:
			ret = PBaseAddNameValue(Pairs, Name, L"<stop updating>");
			break;
		case -2:
			ret = PBaseAddNameValue(Pairs, Name, L"<start updating>");
			break;
		default:
			if (FileTimeToSystemTime((PFILETIME)Time, &st))
				ret = PBaseAddNameFormat(Pairs, Name, L"%.4u-%.2u-%.2u %.2u:%.2u:%.2u:%.3u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
			else ret = GetLastError();
			break;
	}

	return ret;
}


static DWORD _ProcessBasicInformation(PNV_PAIR Pairs, const void *Buffer, ULONG Length)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const FILE_BASIC_INFORMATION *fbi = (PFILE_BASIC_INFORMATION)Buffer;

	ret = _ProcessFileTime(Pairs, L"Created", &fbi->CreationTime);
	if (ret == ERROR_SUCCESS)
		ret = _ProcessFileTime(Pairs, L"Last written", &fbi->LastWriteTime);
	
	if (ret == ERROR_SUCCESS)
		ret = _ProcessFileTime(Pairs, L"Last accessed", &fbi->LastAccessTime);
	
	if (ret == ERROR_SUCCESS)
		ret = _ProcessFileTime(Pairs, L"Metadata changed", &fbi->ChangeTime);

	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"File attributes", L"0x%x", fbi->FileAttributes);

	return ret;
}


static DWORD _ProcessStandardInformation(PNV_PAIR Pairs, const void *Buffer, ULONG Length)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const FILE_STANDARD_INFORMATION *fsi = (PFILE_STANDARD_INFORMATION)Buffer;
	const FILE_STANDARD_INFORMATION_EX *fsiex = (PFILE_STANDARD_INFORMATION_EX)Buffer;

	ret = PBaseAddNameFormat(Pairs, L"Allocation size", L"%llu", fsiex->AllocationSize.QuadPart);
	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"End of file", L"%llu", fsiex->EndOfFile.QuadPart);
	
	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"Number of links", L"%u", fsiex->NumberOfLinks);
	
	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"Delete pending", L"%u", fsiex->DeletePending);
	
	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"Directory", L"%u", fsiex->Directory);
	
	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"Alternate stream", L"%u", fsiex->AlternateStream);
	
	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"Metadata attribute", L"%u", fsiex->MetadataAttribute);

	return ret;
}


static DWORD _ProcessDispositionInformation(PNV_PAIR Pairs, const void *Buffer, ULONG Length)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const FILE_DISPOSITION_INFORMATION *fdi = (PFILE_DISPOSITION_INFORMATION)Buffer;

	ret = PBaseAddNameFormat(Pairs, L"DeleteFile", L"%u", fdi->DeleteFileW);

	return ret;
}

static DWORD _ProcessNetworkOpenInformation(PNV_PAIR Pairs, const void *Buffer, ULONG Length)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const FILE_NETWORK_OPEN_INFORMATION *fnoi = (PFILE_NETWORK_OPEN_INFORMATION)Buffer;

	ret = _ProcessFileTime(Pairs, L"Created", &fnoi->CreationTime);
	if (ret == ERROR_SUCCESS)
		ret = _ProcessFileTime(Pairs, L"Last written", &fnoi->LastWriteTime);

	if (ret == ERROR_SUCCESS)
		ret = _ProcessFileTime(Pairs, L"Last accessed", &fnoi->LastAccessTime);

	if (ret == ERROR_SUCCESS)
		ret = _ProcessFileTime(Pairs, L"Metadata changed", &fnoi->ChangeTime);

	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"File attributes", L"0x%x", fnoi->FileAttributes);

	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"Allocation size", L"%llu", fnoi->AllocationSize.QuadPart);

	if (ret == ERROR_SUCCESS)
		ret = PBaseAddNameFormat(Pairs, L"End of file", L"%llu", fnoi->EndOfFile.QuadPart);

	return ret;
}


static DWORD cdecl _ParseRoutine(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount)
{
	NV_PAIR p;
	BOOLEAN parsed = FALSE;
	DWORD ret = ERROR_GEN_FAILURE;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	const REQUEST_FASTIO *fastIo = NULL;
	const void *buffer = NULL;
	ULONG bufferLength = 0;
	FILE_INFORMATION_CLASS infoClass = FileMaximumInformation;

	memset(&p, 0, sizeof(p));
	ret = ERROR_SUCCESS;
	switch (Request->Type) {
		case ertIRP:
			irp = CONTAINING_RECORD(Request, REQUEST_IRP, Header);
			if (irp->DataSize > 0 &&
				irp->MajorFunction == IRP_MJ_SET_INFORMATION) {
				buffer = (irp + 1);
				bufferLength = (ULONG)(ULONG_PTR)irp->Arg1;
				infoClass = (FILE_INFORMATION_CLASS)irp->Arg2;
				parsed = TRUE;
			}
			break;
		case ertIRPCompletion:
			irpComp = CONTAINING_RECORD(Request, REQUEST_IRP_COMPLETION, Header);
			if (irpComp->MajorFunction == IRP_MJ_QUERY_INFORMATION &&
				irpComp->DataSize > 0) {
				buffer = (irpComp + 1);
				bufferLength = (ULONG)(ULONG_PTR)irp->Arg1;
				infoClass = (FILE_INFORMATION_CLASS)irp->Arg2;
				parsed = TRUE;
			}
			break;
		case ertFastIo:
			fastIo = CONTAINING_RECORD(Request, REQUEST_FASTIO, Header);
			buffer = (fastIo + 1);
			switch (fastIo->FastIoType) {
				case FastIoQueryBasicInfo:
					infoClass = FileBasicInformation;
					parsed = TRUE;
					break;
				case FastIoQueryStandardInfo:
					infoClass = FileStandardInformation;
					parsed = TRUE;
					break;
				case FastIoQueryNetworkOpenInfo:
					infoClass = FileNetworkOpenInformation;
					parsed = TRUE;
					break;
				case FastIoQueryOpen:
					break;
					parsed = TRUE;
			}
			break;
	}

	*RowCount = 0;
	*Names = NULL;
	*Values = NULL;
	if (parsed) {
		switch (infoClass) {
			case FileBasicInformation:
				ret = _ProcessBasicInformation(&p, buffer, bufferLength);
				break;
			case FileStandardInformation:
				ret = _ProcessStandardInformation(&p, buffer, bufferLength);
				break;
			case FileDispositionInformation:
				ret = _ProcessDispositionInformation(&p, buffer, bufferLength);
				break;
			case FileNetworkOpenInformation:
				ret = _ProcessNetworkOpenInformation(&p, buffer, bufferLength);
				break;
		}

		if (ret == ERROR_SUCCESS) {
			*Handled = parsed;
			*RowCount = p.Count;
			*Names = p.Names;
			*Values = p.Values;
		}
	}

	if (ret != ERROR_SUCCESS) {
		for (size_t i = 0; i < p.Count; ++i)
			HeapFree(GetProcessHeap(), 0, p.Names[i]);

		HeapFree(GetProcessHeap(), 0, p.Names);
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
			tmpParser->Name = L"FileInfo";
			tmpParser->Description = L"Parses buffers for NtXxxInformationFile";
			tmpParser->Priority = 1;
			tmpParser->ParseRoutine = _ParseRoutine;
			tmpParser->FreeRoutine = _FreeRoutine;
			*Parser = tmpParser;
		}
	}
	else ret = ERROR_NOT_SUPPORTED;

	return ret;
}
