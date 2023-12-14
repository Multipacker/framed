#ifndef WIN32_ESSENTIAL_H
#define WIN32_ESSENTIAL_H

#define UNICODE

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct W32_FileIterator W32_FileIterator;
struct W32_FileIterator
{
	HANDLE handle;
	WIN32_FIND_DATAW find_data;
	B32 done;
};

typedef struct W32_State W32_State;
struct W32_State
{
	LARGE_INTEGER start_counter;
};

#endif // WIN32_ESSENTIAL_H