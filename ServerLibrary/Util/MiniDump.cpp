#include "stdafx.h"
#include "Minidump.h"

MiniDump::MiniDump()
{
	::SetUnhandledExceptionFilter(exceptionFilter);
	printf("* Dump filter setting complete!\n");
}

LONG WINAPI MiniDump::exceptionFilter(struct _EXCEPTION_POINTERS *exceptionInfo)
{
	shutdownServer();

	_CrtMemDumpAllObjectsSince(NULL);

	HMODULE dumpDll = nullptr;
	dumpDll = ::LoadLibraryA("DBGHELP.DLL");
	if (!dumpDll) {
		printf("! DBGHelp.dll not loaded\n");
		return 0;
	}

	wstr_t dumpPatch;
	dumpPatch += NOW_STRING(L"D%Y-%dT%H-%M-%S");
	dumpPatch += L".dmp";

	HANDLE file = ::CreateFile(dumpPatch.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("! dump file not making\n");
		return 0;
	}

	_MINIDUMP_EXCEPTION_INFORMATION info;
	info.ThreadId = ::GetCurrentThreadId();
	info.ExceptionPointers = exceptionInfo;
	info.ClientPointers = NULL;

	WRITEDUMP dumpFunc = (WRITEDUMP)::GetProcAddress(dumpDll, "MiniDumpWriteDump");
	if (dumpFunc(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, &info, NULL, NULL) == FALSE) {
		printf("! dump file saving error\n");
		return 0;
	}

	::CloseHandle(file);

	return EXCEPTION_CONTINUE_SEARCH;
}

static MiniDump minidump;