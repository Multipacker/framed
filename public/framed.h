#ifndef FRAMED_H
#define FRAMED_H

/*
 * This software available under 2 licenses -- choose whichever you prefer.
 * ----------------------------------------------------------------------------
 * Alternative A - MIT License
 *
 * Copyright (c) 2024 Hampus Johansson and Simon Renhult
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * ----------------------------------------------------------------------------
 * Alternative B - Public Domain
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <https://unlicense.org>
 */

#if defined(__clang__)
#    define FRAMED_COMPILER_CLANG 1

#    if defined(_WIN32)
#        define FRAMED_OS_WINDOWS 1
#    elif defined(__gnu_linux__)
#        define FRAMED_OS_LINUX 1
#    elif defined(__APPLE__) && defined(__MACH__)
#        define FRAMED_OS_MAC 1
#    else
#        error missing OS detection
#    endif

#    if defined(__amd64__)
#       define FRAMED_ARCH_X64 1
#    elif defined(__i386__)
#       define FRAMED_ARCH_X86 1
#    elif defined(__arm__)
#       define FRAMED_ARCH_ARM
#    elif defined(__aarch64__)
#       define FRAMED_ARCH_ARM64
#    else
#       error missing ARCH detection
#    endif

#elif defined(_MSC_VER)
#    define FRAMED_COMPILER_CL 1

#    if defined(_WIN32)
#       define FRAMED_OS_WINDOWS 1
#    else
#       error missing OS detection
#    endif

#    if defined(_M_AMD64)
#       define FRAMED_ARCH_X64 1
#    elif defined(_M_IX86)
#       define FRAMED_ARCH_X86 1
#    elif defined(_M_ARM)
#       define FRAMED_ARCH_ARM
#    else
#       error missing ARCH detection
#    endif

#elif defined(__GNUC__)
#    define FRAMED_COMPILER_GCC 1

#    if defined(_WIN32)
#       define FRAMED_OS_WINDOWS 1
#    elif defined(__gnu_linux__)
#       define FRAMED_OS_LINUX 1
#    elif defined(__APPLE__) && defined(__MACH__)
#       define FRAMED_OS_MAC 1
#    else
#       error missing OS detection
#    endif

#    if defined(__amd64__)
#       define FRAMED_ARCH_X64 1
#    elif defined(__i386__)
#       define FRAMED_ARCH_X86 1
#    elif defined(__arm__)
#       define FRAMED_ARCH_ARM
#    elif defined(__aarch64__)
#       define FRAMED_ARCH_ARM64
#    else
#       error missing ARCH detection
#endif

#endif

#if !defined(FRAMED_COMPILER_CL)
#    define FRAMED_COMPILER_CL 0
#endif

#if !defined(FRAMED_COMPILER_CLANG)
#    define FRAMED_COMPILER_CLANG 0
#endif

#if !defined(FRAMED_COMPILER_GCC)
#    define FRAMED_COMPILER_GCC 0
#endif

#if !defined(FRAMED_OS_WINDOWS)
#    define FRAMED_OS_WINDOWS 0
#endif

#if !defined(FRAMED_OS_LINUX)
#    define FRAMED_OS_LINUX 0
#endif

#if !defined(FRAMED_OS_MAC)
#    define FRAMED_OS_MAC 0
#endif

#if !defined(FRAMED_ARCH_X64)
#    define FRAMED_ARCH_X64 0
#endif

#if !defined(FRAMED_ARCH_X86)
#    define FRAMED_ARCH_X86 0
#endif

#if !defined(FRAMED_ARCH_ARM)
#    define FRAMED_ARCH_ARM 0
#endif

#if !defined(FRAMED_ARCH_ARM64)
#    define FRAMED_ARCH_ARM64 0
#endif

#if !defined(FRAMED_DEF)
#    if defined(FRAMED_STATIC)
#        define FRAMED_DEF static
#    else
#        define FRAMED_DEF extern
#    endif
#endif

#if FRAMED_COMPILER_CL

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

#define framed_stringify_(s) #s
#define framed_stringify(s)  framed_stringify_(s)
#define framed_glue_(a, b)   a##b
#define framed_glue(a, b)    framed_glue_(a, b)

typedef Framed_U8 Framed_PacketKind;
enum
{
    Framed_PacketKind_Init       = (1 << 0),
    Framed_PacketKind_FrameStart = (1 << 1),
    Framed_PacketKind_ZoneBegin  = (1 << 2),
    Framed_PacketKind_ZoneEnd    = (1 << 3),
};

#pragma pack(push, 1)
typedef struct PacketHeader PacketHeader;
struct PacketHeader
{
    Framed_PacketKind kind;
    Framed_U64 tsc;
};

#pragma pack(pop)

#if defined(FRAMED_DISABLE)
#    define framed_init(wait)
#    define framed_flush()
#    define framed_mark_frame_start()
#    define framed_zone_begin(name)
#    define framed_zone_end()
#else
#    define framed_init(wait)         framed_init_(wait)
#    define framed_flush()            framed_flush_()
#    define framed_mark_frame_start() framed_mark_frame_start_()
#    define framed_zone_begin(name)   framed_zone_begin_(name)
#    define framed_zone_end()         framed_zone_end_()
#endif

#define framed_function_begin() framed_zone_begin((char *) __func__)
#define framed_function_end()   framed_zone_end()

FRAMED_DEF void framed_init_(Framed_B32 wait_for_connection);
FRAMED_DEF void framed_flush_(void);

FRAMED_DEF void framed_mark_frame_start_(void);

FRAMED_DEF void framed_zone_begin_(char *name);
FRAMED_DEF void framed_zone_end_(void);

#ifdef __cplusplus

#if defined(FRAMED_DISABLE)
#    define framed_zone_block(name)
#    define framed_function
#else
#    define framed_zone_block(name) AutoClosingZoneBlock framed_glue(zone, __LINE__)((char *) name)
#    define framed_function framed_zone_block(__func__)
#endif

struct AutoClosingZoneBlock
{
    AutoClosingZoneBlock(char *name);
    ~AutoClosingZoneBlock();
};

#endif

#if defined(FRAMED_IMPLEMENTATION) && !defined(FRAMED_DISABLE)

#include <string.h>
#include <stdlib.h>

#define framed__assert(expr) if (!(expr)) { *(volatile int *)0 = 0; }

#define framed_memory_copy(dst, src, size) memcpy(dst, src, size)

#define FRAMED_BUFFER_CAPACITY (512)
#define FRAMED_DEFAULT_PORT (1234)

typedef struct Framed_Socket Framed_Socket;
struct Framed_Socket
{
    Framed_U64 u64[1];
};

typedef struct Framed_ClientState Framed_ClientState;
struct Framed_ClientState
{
    Framed_Socket socket;
    Framed_U8 *buffer;
    Framed_U64 buffer_pos;
};

static Framed_ClientState global_framed_state;

#ifdef __cplusplus

    AutoClosingZoneBlock::AutoClosingZoneBlock(char *name)
    {
        framed_zone_begin(name);
    }

    AutoClosingZoneBlock::~AutoClosingZoneBlock()
    {
        framed_zone_end();
    }

#endif

////////////////////////////////
// NOTE: Internal functions

#if FRAMED_COMPILER_CL
#   include <intrin.h>
#elif FRAMED_COMPILER_GCC
#    include <x86intrin.h>
#endif

static Framed_U64
framed__rdtsc(void)
{
    Framed_U64 result = 0;
#if FRAMED_COMPILER_CL
    result = __rdtsc();
#elif FRAMED_COMPILER_CLANG
    result = __rdtsc();
#elif FRAMED_COMPILER_GCC
    result = __rdtsc();
#endif
    return(result);
}

static void
framed__ensure_space(Framed_U64 size)
{
    framed__assert(size <= FRAMED_BUFFER_CAPACITY);

    Framed_ClientState *framed = &global_framed_state;
    if ((framed->buffer_pos + size) > FRAMED_BUFFER_CAPACITY)
    {
        framed_flush_();
    }
}

static Framed_U16
framed__u16_byte_swap(Framed_U16 x)
{
#if FRAMED_COMPILER_CL
    return _byteswap_ushort(x);
#elif FRAMED_COMPILER_CLANG || FRAMED_COMPILER_GCC
    return __builtin_bswap16(x);
#else
#    error Your compiler does not have an implementation of framed_u16_big_to_local_endian.
#endif
}

static Framed_U32
framed__u32_byte_swap(Framed_U32 x)
{
#if FRAMED_COMPILER_CL
    return _byteswap_ulong(x);
#elif FRAMED_COMPILER_CLANG || FRAMED_COMPILER_GCC
    return __builtin_bswap32(x);
#else
#    error Your compiler does not have an implementation of framed_u32_big_to_local_endian.
#endif
}

////////////////////////////////
// NOTE: OS specific implementation

#if FRAMED_OS_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

static Framed_U64
framed__win32_read_timer_frequency(void)
{
    Framed_U64 result = 0;
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    result = freq.QuadPart;
    return(result);
}

static Framed_U64
framed__win32_read_timer(void)
{
    Framed_U64 result = 0;
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    result = counter.QuadPart;
    return(result);
}

static Framed_U64
framed__guess_tsc_frequency(Framed_U64 ms_to_wait)
{
    Framed_U64 os_freq = framed__win32_read_timer_frequency();

    Framed_U64 cpu_start = framed__rdtsc();
    Framed_U64 os_start = framed__win32_read_timer();
    Framed_U64 os_end = 0;
    Framed_U64 os_elapsed = 0;
    Framed_U64 os_wait_time = os_freq * ms_to_wait/1000;
    while (os_elapsed < os_wait_time)
    {
        os_end = framed__win32_read_timer();
        os_elapsed = os_end - os_start;
    }

    Framed_U64 cpu_end = framed__rdtsc();
    Framed_U64 cpu_elapsed = cpu_end - cpu_start;
    Framed_U64 cpu_freq = 0;
    if (os_elapsed)
    {
        cpu_freq = os_freq * cpu_elapsed / os_elapsed;
    }
    return(cpu_freq);
}

static void
framed__socket_init(Framed_B32 wait_for_connection)
{
    Framed_ClientState *framed = &global_framed_state;
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    framed->socket.u64[0] = sock;
    struct sockaddr_in sockaddrin = {0};
    sockaddrin.sin_addr.S_un.S_un_b.s_b1 = 127;
    sockaddrin.sin_addr.S_un.S_un_b.s_b2 = 0;
    sockaddrin.sin_addr.S_un.S_un_b.s_b3 = 0;
    sockaddrin.sin_addr.S_un.S_un_b.s_b4 = 1;
    sockaddrin.sin_port = framed__u16_byte_swap(FRAMED_DEFAULT_PORT);
    sockaddrin.sin_family = AF_INET;
    // TODO(hampus): Make use of `wait_for_connection`. It is always
    // waiting for now.
    int error = connect(sock, (struct sockaddr *) &sockaddrin, sizeof(sockaddrin));

    // NOTE(hampus): Disable nagle's algorithm
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
}

static void
framed__socket_send(void)
{
    Framed_ClientState *framed = &global_framed_state;
    SOCKET sock = (SOCKET) framed->socket.u64[0];
    Framed_U16 *packet_size = (Framed_U16 *)framed->buffer;
    *packet_size = (Framed_U16)framed->buffer_pos;
    int error = send(sock, (char *) framed->buffer, (int) framed->buffer_pos, 0);
}

#elif FRAMED_OS_LINUX

#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <time.h>

static Framed_U64
framed__linux_read_timer(void)
{
    // NOTE(simon): According to `man clock_gettime.2` this syscall should not fail.
    struct timespec time = { 0 };
    Framed_S32 return_code = clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    framed__assert(return_code == 0);

    Framed_U64 nanoseconds = (Framed_U64) time.tv_sec * 1000000000 + (Framed_U64) time.tv_nsec;
    return(nanoseconds);
}

static Framed_U64
framed__guess_tsc_frequency(Framed_U64 ms_to_wait)
{
    Framed_U64 cpu_start = framed__rdtsc();
    Framed_U64 os_start = framed__linux_read_timer();
    Framed_U64 os_end = 0;
    Framed_U64 os_elapsed = 0;
    Framed_U64 os_wait_time = ms_to_wait * 1000000;
    while (os_elapsed < os_wait_time)
    {
        os_end = framed__linux_read_timer();
        os_elapsed = os_end - os_start;
    }

    Framed_U64 cpu_end = framed__rdtsc();
    Framed_U64 cpu_elapsed = cpu_end - cpu_start;
    Framed_U64 cpu_freq = 0;
    if (os_elapsed)
    {
        cpu_freq = 1000000000 * cpu_elapsed / os_elapsed;
    }
    return(cpu_freq);
}

static void
framed__socket_init(Framed_B32 wait_for_connection)
{
    Framed_ClientState *framed = &global_framed_state;
    int linux_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    framed->socket.u64[0] = (Framed_U64) linux_socket;
    struct sockaddr_in socket_address = {0};
    socket_address.sin_family      = AF_INET;
    socket_address.sin_port        = framed__u16_byte_swap(FRAMED_DEFAULT_PORT);
    socket_address.sin_addr.s_addr = framed__u32_byte_swap(127 << 24 | 0 << 16 | 0 << 8 | 1 << 0);
    // TODO(simon): Use `wait_for_connection`
    int error = connect(linux_socket, (struct sockaddr *) &socket_address, sizeof(socket_address));
    // TODO(simon): Report better errors to the user.
}

static void
framed__socket_send(void)
{
    Framed_ClientState *framed = &global_framed_state;
    int linux_socket = (int) framed->socket.u64[0];
    Framed_U16 *packet_size = (Framed_U16 *)framed->buffer;
    *packet_size = (Framed_U16)framed->buffer_pos;
    ssize_t error = 0;
    do
    {
        error = send(linux_socket, framed->buffer, (size_t) framed->buffer_pos, 0);
    } while (error == -1 && (errno == EAGAIN || errno == EWOULDBLOCK));
}

#endif

////////////////////////////////
//~ NOTE: Public functions

FRAMED_DEF void
framed_init_(Framed_B32 wait_for_connection)
{
    Framed_ClientState *framed = &global_framed_state;

    framed__socket_init(wait_for_connection);
    framed->buffer     = (Framed_U8 *) malloc(FRAMED_BUFFER_CAPACITY);
    // NOTE(hampus): First two bytes are the size of the packet
    framed->buffer_pos = sizeof(Framed_U16);

#pragma pack(push, 1)
    typedef struct Packet Packet;
    struct Packet
    {
        PacketHeader header;
        Framed_U64 tsc_frequency;
        Framed_U16 version;
    };
#pragma pack(pop)

    // NOTE(hampus): Try to guess the clients rdtsc frquency. Higher
    // wait times may yield in better guess, but it is not really
    // too significant to be worth it.

    // TODO(hampus): We might want to do this per frame instead if
    // the tsc frequency changes inbetween frames.

    Framed_U64 entry_size = sizeof(Packet);

    Packet *packet = (Packet *)(framed->buffer + framed->buffer_pos);
    packet->header.tsc = framed__rdtsc();
    packet->header.kind = Framed_PacketKind_Init;
    packet->tsc_frequency = framed__guess_tsc_frequency(1000);
    packet->version = 0;

    framed->buffer_pos += entry_size;

    framed_flush_();
}

FRAMED_DEF void
framed_flush_(void)
{
    Framed_ClientState *framed = &global_framed_state;
    framed__socket_send();
    // NOTE(hampus): First two bytes are the size of the packet
    framed->buffer_pos = sizeof(Framed_U16);
}

FRAMED_DEF void
framed_mark_frame_start_(void)
{
    Framed_ClientState *framed = &global_framed_state;

#pragma pack(push, 1)
    typedef struct Packet Packet;
    struct Packet
    {
        PacketHeader header;
    };
#pragma pack(pop)

    Framed_U64 entry_size = sizeof(Packet);
    framed__ensure_space(entry_size);

    Packet *packet = (Packet *)(framed->buffer + framed->buffer_pos);
    packet->header.kind = Framed_PacketKind_FrameStart;
    packet->header.tsc = framed__rdtsc();

    framed->buffer_pos += entry_size;
}

FRAMED_DEF void
framed_zone_begin_(char *name)
{
    Framed_ClientState *framed = &global_framed_state;

#pragma pack(push, 1)
    typedef struct Packet Packet;
    struct Packet
    {
        PacketHeader header;
        Framed_U8 name_length;
        Framed_U8 name[];
    };
#pragma pack(pop)

    // TODO(hampus): Check that strlen(name) <= 255.
    // But please don't have a name longer than this :)
    Framed_U8 length = (Framed_U8)strlen(name);
    framed__assert(length != 0);

    Framed_U64 entry_size = sizeof(Packet) + length;
    framed__ensure_space(entry_size);

    Packet *packet = (Packet *)(framed->buffer + framed->buffer_pos);
    packet->header.kind = Framed_PacketKind_ZoneBegin;
    packet->header.tsc = framed__rdtsc();
    packet->name_length = length;
    framed_memory_copy(packet->name, name, length);

    framed->buffer_pos += entry_size;
}

FRAMED_DEF void
framed_zone_end_(void)
{
    Framed_ClientState *framed = &global_framed_state;

#pragma pack(push, 1)
    typedef struct Packet Packet;
    struct Packet
    {
        PacketHeader header;
    };
#pragma pack(pop)

    Framed_U64 entry_size = sizeof(Packet);
    framed__ensure_space(entry_size);

    Packet *packet = (Packet *)(framed->buffer + framed->buffer_pos);
    packet->header.kind = Framed_PacketKind_ZoneEnd;
    packet->header.tsc = framed__rdtsc();

    framed->buffer_pos += entry_size;
}

#endif // FRAMED_IMPLEMENTATION

#endif // FRAMED_H
