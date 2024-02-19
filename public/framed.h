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

#if !defined(FRAMED_DEF)
#    if defined(FRAMED_STATIC)
#        define FRAMED_DEF static
#    else
#        define FRAMED_DEF extern
#    endif
#endif

typedef Framed_U8  Framed_B8;
typedef Framed_U16 Framed_B16;
typedef Framed_U32 Framed_B32;
typedef Framed_U64 Framed_B64;

FRAMED_DEF void framed_init(Framed_B32 wait_for_connection);
FRAMED_DEF void framed_flush(void);

FRAMED_DEF void framed_zone_begin(char *name);
FRAMED_DEF void framed_zone_end(void);

#ifdef FRAMED_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>

#define framed__assert(expr)

#define framed_memory_copy(dst, src, size) memcpy(dst, src, size)

#define FRAMED_BUFFER_CAPACITY (256)
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
framed__u16_big_to_local_endian(Framed_U16 x)
{
#if COMPILER_CL
	return _byteswap_ushort(x);
#elif COMPILER_CLANG || COMPILER_GCC
	return __builtin_bswap16(x);
#else
# error Your compiler does not have an implementation of framed_u16_big_to_local_endian.
#endif
}

static Framed_U32
framed__u32_big_to_local_endian(Framed_U32 x)
{
#if COMPILER_CL
	return _byteswap_ulong(x);
#elif COMPILER_CLANG || COMPILER_GCC
	return __builtin_bswap32(x);
#else
# error Your compiler does not have an implementation of framed_u32_big_to_local_endian.
#endif
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
	sockaddrin.sin_port = framed__u16_big_to_local_endian(FRAMED_DEFAULT_PORT);
	sockaddrin.sin_family = AF_INET;
	// TODO(hampus): Make use of `wait_for_connection`. It is always
	// waiting for now.
	int error = connect(sock, (struct sockaddr *) &sockaddrin, sizeof(sockaddrin));
}

static void
framed__socket_send(void)
{
	Framed_State *framed = &global_framed_state;
	SOCKET sock = (SOCKET) framed->socket.u64[0];
	int error = send(sock, (char *) framed->buffer, (int) framed->buffer_pos, 0);
}

#elif OS_LINUX

#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>

static void
framed__socket_init(Framed_B32 wait_for_connection)
{
	Framed_State *framed = &global_framed_state;
	int linux_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	framed->socket.u64[0] = (Framed_U64) linux_socket;
	struct sockaddr_in socket_address = {0};
	socket_address.sin_family      = AF_INET;
	socket_address.sin_port        = framed__u16_big_to_local_endian(FRAMED_DEFAULT_PORT);
	socket_address.sin_addr.s_addr = framed__u32_big_to_local_endian(127 << 24 | 0 << 16 | 0 << 8 | 1 << 0);
	// TODO(simon): Use `wait_for_connection`
	int error = connect(linux_socket, (struct sockaddr *) &socket_address, sizeof(socket_address));
	if (error == -1)
	{
		// TODO(simon): Report better errors to the user.
		perror("Framed connect");
	}
}

static void
framed__socket_send(void)
{
	Framed_State *framed = &global_framed_state;
	int linux_socket = (int) framed->socket.u64[0];
	int error = 0;
	do
	{
		error = send(linux_socket, framed->buffer, (size_t) framed->buffer_pos, 0);
	} while (error == -1 && (errno == EAGAIN || errno == EWOULDBLOCK));
}

#endif

////////////////////////////////
// NOTE: Internal functions

#if COMPILER_CL
#    include <intrin.h>
#elif COMPILER_GCC
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

FRAMED_DEF void
framed_init(Framed_B32 wait_for_connection)
{
	Framed_State *framed = &global_framed_state;

	framed__socket_init(wait_for_connection);
	framed->buffer     = (Framed_U8 *) malloc(FRAMED_BUFFER_CAPACITY);
	framed->buffer_pos = 0;
}

FRAMED_DEF void
framed_flush(void)
{
	Framed_State *framed = &global_framed_state;
	framed__socket_send();
	framed->buffer_pos = 0;
}

// U64 : rdtsc
// U64 : name_length, if 0 then this closes the last zone
// U8 * name_length : name

FRAMED_DEF void
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

FRAMED_DEF void
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
