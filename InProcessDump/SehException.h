#ifndef SEH_EXCEPTION_H_
#define SEH_EXCEPTION_H_

#include <Windows.h>
#include <dbghelp.h>
#include <string>

class SehException final
{
public:
	virtual ~SehException();

	static SehException& GetInstance();

	int Filter(unsigned int code, EXCEPTION_POINTERS* ep);

	void SetDumpPath(const std::wstring& dump_path);

	void Terminate();

private:
	CRITICAL_SECTION critical_section_;

	HMODULE module_;

	typedef BOOL(WINAPI* pMiniDumpWriteDump)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
	pMiniDumpWriteDump pf_mini_dump_write_dump_;

	EXCEPTION_POINTERS* ep_;
	DWORD thread_id_;

	HANDLE thread_;
	HANDLE dump_start_semaphore_;
	HANDLE dump_finish_semaphore_;
	std::wstring dump_path_str_;
	const wchar_t* dump_path_;
	volatile bool terminate_;

	void WriteMiniDump(EXCEPTION_POINTERS* ep);

	SehException();

	static DWORD WINAPI WriteMiniDump(LPVOID param);

	SehException(const SehException&) = delete;
	SehException& operator=(const SehException&) = delete;
	SehException(SehException&&) = delete;
	SehException& operator=(SehException&&) = delete;
};

#endif // SEH_EXCEPTION_H_