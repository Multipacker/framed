#ifndef WIN32_ESSENTIAL_H
#define WIN32_ESSENTIAL_H

#define UNICODE

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>
#include <shellscalingapi.h>
#include <winnt.h>
#pragma warning(pop)

typedef struct Win32_FileIterator Win32_FileIterator;
struct Win32_FileIterator
{
	HANDLE handle;
	WIN32_FIND_DATAW find_data;
	B32 done;
};

typedef struct Win32_State Win32_State;
struct Win32_State
{
	LARGE_INTEGER frequency;
	Arena *permanent_arena;
	DWORD tls_index;
};

#endif // WIN32_ESSENTIAL_H