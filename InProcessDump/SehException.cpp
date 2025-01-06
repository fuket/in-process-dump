#include "SehException.h"

SehException::SehException()
	: module_(LoadLibrary(L"DbgHelp.dll"))
	, pf_mini_dump_write_dump_()
	, critical_section_()
	, thread_(NULL)
	, dump_start_semaphore_(NULL)
	, dump_finish_semaphore_(NULL)
	, dump_path_str_()
	, dump_path_(NULL)
	, ep_(NULL)
	, thread_id_(NULL)
	, terminate_(false)
{
	InitializeCriticalSection(&critical_section_);

	if (module_ != NULL)
	{
		pf_mini_dump_write_dump_ = reinterpret_cast<pMiniDumpWriteDump>(GetProcAddress(module_, "MiniDumpWriteDump"));
	}

	if (pf_mini_dump_write_dump_ != NULL)
	{
		dump_start_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);
		dump_finish_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);
	}

	if (dump_start_semaphore_ != NULL && dump_finish_semaphore_ != NULL)
	{
		DWORD thread_id;
		const size_t kStackSize = 64 * 1024;
		thread_ = CreateThread(NULL, kStackSize, WriteMiniDump, this, 0, &thread_id);
	}
}

SehException::~SehException()
{
	if (thread_ != NULL)
	{
		Terminate();
		ReleaseSemaphore(dump_start_semaphore_, 1, NULL);
		const int kWaitMilliseconds = 60000;
		WaitForSingleObject(thread_, kWaitMilliseconds);
		CloseHandle(thread_);
	}
	DeleteCriticalSection(&critical_section_);
	if (dump_start_semaphore_ != NULL)
	{
		CloseHandle(dump_start_semaphore_);
	}
	if (dump_finish_semaphore_ != NULL)
	{
		CloseHandle(dump_finish_semaphore_);
	}
	if (module_ != NULL)
	{
		FreeLibrary(module_);
	}
}

SehException& SehException::GetInstance()
{
	static SehException instance;
	return instance;
}

int SehException::Filter(unsigned int code, EXCEPTION_POINTERS* ep)
{
	WriteMiniDump(ep);
	//switch (code)
	//{
	//case EXCEPTION_ACCESS_VIOLATION:
	//case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
	//case ...
	//	return EXCEPTION_CONTINUE_SEARCH;  etc.
	//default:
		return EXCEPTION_EXECUTE_HANDLER;
	//}
}

void SehException::SetDumpPath(const std::wstring& dump_path)
{
	dump_path_str_ = dump_path;
	dump_path_ = dump_path_str_.c_str();
}

void SehException::WriteMiniDump(EXCEPTION_POINTERS* ep)
{
	EnterCriticalSection(&critical_section_);

	if (pf_mini_dump_write_dump_ != NULL)
	{
		ep_ = ep;
		thread_id_ = GetCurrentThreadId();

		ReleaseSemaphore(dump_start_semaphore_, 1, NULL);

		WaitForSingleObject(dump_finish_semaphore_, INFINITE);
	}

	LeaveCriticalSection(&critical_section_);
}

DWORD WINAPI SehException::WriteMiniDump(LPVOID param)
{
	SehException* self = static_cast<SehException*>(param);
	while (true)
	{
		if (WaitForSingleObject(self->dump_start_semaphore_, INFINITE) == WAIT_OBJECT_0)
		{
			if (self->terminate_)
			{
				break;
			}
			if (self->dump_path_ != NULL)
			{
				HANDLE file_handle = CreateFile(self->dump_path_, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				MINIDUMP_EXCEPTION_INFORMATION params = {};
				params.ThreadId = self->thread_id_;
				params.ExceptionPointers = self->ep_;
				params.ClientPointers = FALSE;
				self->pf_mini_dump_write_dump_(GetCurrentProcess(), GetCurrentProcessId(), file_handle
					, static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs
						| MiniDumpWithFullMemory
						| MiniDumpWithProcessThreadData
						| MiniDumpWithHandleData
						| MiniDumpWithPrivateReadWriteMemory
						| MiniDumpWithUnloadedModules
						| MiniDumpWithFullMemoryInfo
						| MiniDumpWithThreadInfo
						| MiniDumpWithTokenInformation
						| MiniDumpWithPrivateWriteCopyMemory)
					, &params, NULL, NULL
				);
				CloseHandle(file_handle);
			}
			ReleaseSemaphore(self->dump_finish_semaphore_, 1, NULL);
		}
	}
	return 0;
}

void SehException::Terminate()
{
	terminate_ = true;
}
