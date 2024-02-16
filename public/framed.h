#ifndef FRAMED_H
#define FRAMED_H

#if defined(__clang__)
#	define COMPILER_CLANG 1

#	if defined(_WIN32)
#		define OS_WINDOWS 1
#	elif defined(__gnu_linux__)
#		define OS_LINUX 1
#	elif defined(__APPLE__) && defined(__MACH__)
#		define OS_MAC 1
#	else
#		error missing OS detection
#	endif

# 	if defined(__amd64__)
#		define ARCH_X64 1
#	elif defined(__i386__)
#		define ARCH_X86 1
#	elif defined(__arm__)
#		define ARCH_ARM
#	elif defined(__aarch64__)
#		define ARCH_ARM64
#	else
#		error missing ARCH detection
#	endif

#elif defined(_MSC_VER)
#	define COMPILER_CL 1

#	if defined(_WIN32)
#		define OS_WINDOWS 1
#	else
#		error missing OS detection
#	endif

#	if defined(_M_AMD64)
#		define ARCH_X64 1
#	elif defined(_M_IX86)
#		define ARCH_X86 1
#	elif defined(_M_ARM)
#		define ARCH_ARM
#	else
#		error missing ARCH detection
#	endif

#elif defined(__GNUC__)
#	define COMPILER_GCC 1

#	if defined(_WIN32)
#		define OS_WINDOWS 1
#	elif defined(__gnu_linux__)
#		define OS_LINUX 1
#	elif defined(__APPLE__) && defined(__MACH__)
#		define OS_MAC 1
#	else
#		error missing OS detection
#	endif

#	if defined(__amd64__)
#		define ARCH_X64 1
#	elif defined(__i386__)
#		define ARCH_X86 1
#	elif defined(__arm__)
#		define ARCH_ARM
#	elif defined(__aarch64__)
#		define ARCH_ARM64
#	else
#		error missing ARCH detection
#endif

#endif

#if !defined(COMPILER_CL)
#	define COMPILER_CL 0
#endif

#if !defined(COMPILER_CLANG)
#	define COMPILER_CLANG 0
#endif

#if !defined(COMPILER_GCC)
#	define COMPILER_GCC 0
#endif

#if !defined(OS_WINDOWS)
#	define OS_WINDOWS 0
#endif

#if !defined(OS_LINUX)
#	define OS_LINUX 0
#endif

#if !defined(OS_MAC)
#	define OS_MAC 0
#endif

#if !defined(ARCH_X64)
#	define ARCH_X64 0
#endif

#if !defined(ARCH_X86)
#	define ARCH_X86 0
#endif

#if !defined(ARCH_ARM)
#	define ARCH_ARM 0
#endif

#if !defined(ARCH_ARM64)
#	define ARCH_ARM64 0
#endif

#if COMPILER_CL

typedef signed char          Framed_S8;
typedef signed short         Framed_S16;
typedef signed int           Framed_S32;
typedef signed long long int Framed_S64;

typedef unsigned char      Framed_U8;
typedef unsigned short     Framed_U16;
typedef unsigned int       Framed_U32;
typedef unsigned long long Framed_U64;

#else

#include <stdint.h>

typedef int8_t  Framed_S8;
typedef int16_t Framed_S16;
typedef int32_t Framed_S32;
typedef int64_t Framed_S64;

typedef uint8_t  Framed_U8;
typedef uint16_t Framed_U16;
typedef uint32_t Framed_U32;
typedef uint64_t Framed_U64;

#endif

typedef Framed_U8  Framed_B8;
typedef Framed_U16 Framed_B16;
typedef Framed_U32 Framed_B32;
typedef Framed_U64 Framed_B64;

void framed_init(Framed_B32 wait_for_connection);
void framed_flush(void);

void framed_zone_begin(char *name);
void framed_zone_end(void);

#ifdef FRAMED_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>

#if COMPILER_CL
# include <intrin.h>
#endif

#define framed_memory_copy(dst, src, size) memcpy(dst, src, size)

#define FRAMED_BUFFER_CAPACITY (4 * 1024)
#define FRAMED_DEFAULT_PORT (1234)

typedef struct Framed_Socket Framed_Socket;
struct Framed_Socket
{
	Framed_U64 u64[1];
};

typedef struct Framed_State Framed_State;
struct Framed_State
{
	Framed_Socket socket;
	Framed_U8 *buffer;
	Framed_U64 buffer_pos;
};

static Framed_State global_framed_state;

////////////////////////////////
// NOTE: Socket implementation

static Framed_U16
framed_u16_reverse(Framed_U16 x)
{
	x = (Framed_U16) (((x >> 1) & 0x5555) | ((x & 0x5555) << 1));
	x = (Framed_U16) (((x >> 2) & 0x3333) | ((x & 0x3333) << 2));
	x = (Framed_U16) (((x >> 4) & 0x0F0F) | ((x & 0x0F0F) << 4));
	x = (Framed_U16) (((x >> 8) & 0x00FF) | ((x & 0x00FF) << 8));
	return x;
}

#if OS_WINDOWS

#pragma comment(lib, "Ws2_32.lib")

#pragma warning(push, 0)
#include <winsock2.h>
#pragma warning(pop)

static void
framed__socket_init(Framed_B32 wait_for_connection)
{
	Framed_State *framed = &global_framed_state;
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	framed->socket.u64[0] = sock;
	struct sockaddr_in sockaddrin = {0};
	sockaddrin.sin_addr.S_un.S_un_b.s_b1 = 127;
	sockaddrin.sin_addr.S_un.S_un_b.s_b2 = 0;
	sockaddrin.sin_addr.S_un.S_un_b.s_b3 = 0;
	sockaddrin.sin_addr.S_un.S_un_b.s_b4 = 1;
	sockaddrin.sin_port = framed_u16_reverse(FRAMED_DEFAULT_PORT);
	sockaddrin.sin_family = AF_INET;
	// TODO(hampus): Make use of `wait_for_connectipn`. It is always 
	// waiting for now. 
	int error = connect(sock, (struct sockaddr *) &sockaddrin, sizeof(sockaddrin));
	if (error == SOCKET_ERROR)
	{
		printf("error");
	}
}

static void
framed__socket_send(void)
{
	Framed_State *framed = &global_framed_state;
	SOCKET sock = (SOCKET) framed->socket.u64[0];
	int error = send(sock, (char *) framed->buffer, (int) framed->buffer_pos, 0);
	if (error == SOCKET_ERROR)
	{
		printf("error");
	}
}

#elif OS_LINUX

static void
framed__socket_init(FramedUI_B32 wait_for_connection)
{
	Framed_State *framed = &global_framed_state;
}

static void
framed__socket_send(void)
{
	Framed_State *framed = &global_framed_state;
}

#endif

////////////////////////////////
// NOTE: Internal functions

#define framed__assert(expr)

#if COMPILER_GCC
#	include <x86intrin.h>
#endif

static Framed_U64
framed__rdtsc(void)
{
	Framed_U64 result = 0;
#if COMPILER_CL
	result = __rdtsc();
#elif COMPILER_CLANG
	result = __rdtsc();
#elif COMPILER_GCC
	result = __rdtsc();
#endif
	return(result);
}

static void
framed__ensure_space(Framed_U64 size)
{
	framed__assert(size <= FRAMED_BUFFER_CAPACITY);

	Framed_State *framed = &global_framed_state;
	if ((framed->buffer_pos + size) > FRAMED_BUFFER_CAPACITY)
	{
		framed_flush();
	}
}

////////////////////////////////
// NOTE: Public functions

void
framed_init(Framed_B32 wait_for_connection)
{
	Framed_State *framed = &global_framed_state;

	framed__socket_init(wait_for_connection);
	framed->buffer     = (Framed_U8 *) malloc(FRAMED_BUFFER_CAPACITY);
	framed->buffer_pos = 0;
}

void
framed_flush(void)
{
	Framed_State *framed = &global_framed_state;
	framed__socket_send();
	framed->buffer_pos = 0;
}

// U64 : rdtsc
// U64 : name_length, if 0 then this closes the last zone
// U8 * name_length : name

void
framed_zone_begin(char *name)
{
	Framed_State *framed = &global_framed_state;

	Framed_U64 length  = strlen(name);
	framed__assert(length != 0);

	Framed_U64 entry_size = 2 * sizeof(Framed_U64) + length;
	framed__ensure_space(entry_size);

	Framed_U64 counter = framed__rdtsc();

	*(Framed_U64 *) &framed->buffer[framed->buffer_pos] = counter;
	*(Framed_U64 *) &framed->buffer[framed->buffer_pos + sizeof(Framed_U64)] = length;
	framed_memory_copy(&framed->buffer[framed->buffer_pos + 2 * sizeof(Framed_U64)], name, length);

	framed->buffer_pos += entry_size;
}

void
framed_zone_end(void)
{
	Framed_State *framed = &global_framed_state;

	Framed_U64 counter = framed__rdtsc();

	Framed_U64 entry_size = 2 * sizeof(Framed_U64);
	framed__ensure_space(entry_size);

	*(Framed_U64 *) &framed->buffer[framed->buffer_pos] = counter;
	*(Framed_U64 *) &framed->buffer[framed->buffer_pos + sizeof(Framed_U64)] = 0;

	framed->buffer_pos += entry_size;
}

#endif // FRAMED_IMPLEMENTATION

#endif // FRAMED_H
