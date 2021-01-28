
#ifndef __REQUEST_OUTPUT_H__
#define __REQUEST_OUTPUT_H__


#include <string>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "general-types.h"
#include "callback-stream.h"


class CRequestOutput {
public:
	CRequestOutput(ERequestLogFormat Format, const std::wstring & FileName)
	: format_(Format),
	fileName_(FileName)
	{}
	~CRequestOutput(void)
	{
		if (callbackStreamHandle_ != nullptr)
			CallbackStreamFree(callbackStreamHandle_);

		if (!fileName_.empty() && fileRecord_ != nullptr)
			fclose(fileRecord_);

		return;
	}
	int Prepare(void)
	{
		int ret = 0;
		FILE *tmp = nullptr;

		if (!fileName_.empty()) {
			const wchar_t* fileMode = L"wb";

			if (format_ != rlfBinary)
				fileMode = L"w";

			tmp = _wfopen(fileName_.c_str(), fileMode);
			if (tmp == nullptr) {
				ret = errno;
				fprintf(stderr, "[ERROR]: Unable to access \"%ls\": %u\n", fileName_.c_str(), ret);
			}
		} else {
			tmp = stdout;
			if (format_ == rlfBinary) {
#ifdef _WIN32
				int fileDesc = 0;

				fileDesc = fileno(tmp);
				if (fileDesc >= 0) {
					if (setmode(fileDesc, O_BINARY) < 0) {
						ret = errno;
						fprintf(stderr, "[ERROR]: Unable to set binary mode for the standard output: %u\n", ret);
					}
				} else {
					ret = errno;
					fprintf(stderr, "[ERROR]: Unable to get file descriptor for the standard output: %u\n", ret);
				}
#endif
			}
		}

		if (ret == 0) {
			callbackStreamHandle_ = CallbackStreamCreate(nullptr, _on_stream_write, nullptr, tmp);
			if (callbackStreamHandle_ == nullptr)
				ret = ERROR_GEN_FAILURE;

			if (ret == 0) {
				if (format_ == rlfBinary) {
					BINARY_LOG_HEADER hdr;

					memset(&hdr, 0, sizeof(hdr));
					hdr.Signature = LOGHEADER_SIGNATURE;
					hdr.Version = LOGHEADER_VERSION;
					hdr.Architecture = LOGHEADER_ARCHITECTURE;
					if (fwrite(&hdr, sizeof(hdr), 1, tmp) != 1)
						ret = errno;
				}

				if (ret == 0)
					fileRecord_ = tmp;

				if (ret != 0)
					CallbackStreamFree(callbackStreamHandle_);
			}
		}

		return ret;
	}
	void write(const std::string & String)
	{
		fputs(String.c_str(), fileRecord_);

		return;
	}
	ERequestLogFormat Format(void) const { return format_; }
	void *StreamHandle(void) const { return callbackStreamHandle_; }
	std::wstring FileName(void) const { return fileName_; }
private:
	std::wstring fileName_;
	FILE *fileRecord_;
	ERequestLogFormat format_;
	void *callbackStreamHandle_;
	static DWORD cdecl _on_stream_write(const void *Buffer, ULONG Length, void *Stream, void *Context)
	{
		DWORD ret = 0;
		FILE* f = nullptr;

		f = (FILE*)Context;
		if (fwrite(Buffer, Length, 1, f) == 1)
			ret = Length;

		return ret;
	}
};





#endif
