#ifndef WIN32_ESSENTIAL_H
#define WIN32_ESSENTIAL_H

#define UNICODE

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>
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
};

#endif // WIN32_ESSENTIAL_H