#ifndef BASE_CORE_H
#define BASE_CORE_H

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

#if COMPILER_CLANG
#	if defined(__has_feature) && __has_feature(address_sanitizer)
#		define SANITIZER_ENABLED 1
#	else
#		define SANITIZER_ENABLED 0
#	endif
#elif defined(__SANITIZE_ADDRESS__)
#	define SANITIZER_ENABLED 1
#else
#	define SANITIZER_ENABLED 0
#endif

#if !defined(BUILD_MODE_DEBUG)
#   define BUILD_MODE_DEBUG 0
#endif

#if !defined(BUILD_MODE_OPTIMIZED)
#   define BUILD_MODE_OPTIMIZED 0
#endif

#if !defined(BUILD_MODE_RELEASE)
#   define BUILD_MODE_RELEASE 0
#endif

#if OS_LINUX
#	define _GNU_SOURCE
#	define _FILE_OFFSET_BITS 64
#endif

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define check_null(x) ((x) == 0)
#define set_null(x) ((x) = 0)

#define dll_push_back_np(f,l,n,next,prev) ((f)==0?\
((f)=(l)=(n),(n)->next=(n)->prev=0):\
((n)->prev=(l),(l)->next=(n),(l)=(n),(n)->next=0))

#define dll_insert_npz(f,l,p,n,next,prev,zchk,zset) \
(zchk(f) ? (((f) = (l) = (n)), zset((n)->next), zset((n)->prev)) :\
zchk(p) ? (zset((n)->prev), (n)->next = (f), (zchk(f) ? (0) : ((f)->prev = (n))), (f) = (n)) :\
((zchk((p)->next) ? (0) : (((p)->next->prev) = (n))), (n)->next = (p)->next, (n)->prev = (p), (p)->next = (n),\
((p) == (l) ? (l) = (n) : (0))))

#define dll_push_back_npz(f,l,n,next,prev,zchk,zset) dll_insert_npz(f,l,l,n,next,prev,zchk,zset)
#define dll_push_front_np(f,l,n,next,prev) dll_push_back_np(l,f,n,next,prev)

#define dll_remove_npz(f,l,n,next,prev,zchk,zset) (((f)==(n))?\
((f)=(f)->next, (zchk(f) ? (zset(l)) : zset((f)->prev))):\
((l)==(n))?\
((l)=(l)->prev, (zchk(l) ? (zset(f)) : zset((l)->next))):\
((zchk((n)->next) ? (0) : ((n)->next->prev=(n)->prev)),\
(zchk((n)->prev) ? (0) : ((n)->prev->next=(n)->next))))

#define sll_push_front_np(f, n, next) ((n)->next = (f),\
(f) = (n));

#define sll_push_front(f, n) sll_push_front_np(f, n, next)

#define stack_push_n(f,n,next) ((n)->next=(f),(f)=(n))

#define stack_pop_nz(f,next,zchk) ((zchk(f))?0:\
((f)=(f)->next))


#define queue_push_nz(f,l,n,next,zchk,zset) (zchk(f)?\
(f)=(l)=(n):\
((l)->next=(n),(l)=(n)),\
zset((n)->next))


#define queue_pop_nz(f,l,next,zset) ((f)==(l)?\
(f)=zset(l):\
((f)=(f)->next))

#define dll_push_back(f,l,n)  dll_push_back_np(f,l,n,next,prev)
#define dll_push_front(f,l,n) dll_push_front_np(f,l,n,prev,next)
#define dll_insert(f,l,p,n)   dll_insert_npz(f,l,p,n,next,prev,check_null,set_null)
#define dll_remove(f,l,n)     dll_remove_npz(f,l,n,next,prev,check_null,set_null)

#define stack_push(f,n) stack_push_n(f,n,next)
#define stack_pop(f)    stack_pop_nz(f, next,check_null)

#define queue_push(f,l,n) queue_push_nz(f,l,n,next,check_null,set_null)
#define queue_pop(f,l)    queue_pop_nz(f,l,next,set_null)

#define PRIU8   PRIu8
#define PRIU16  PRIu16
#define PRIU32  PRIu32
#define PRIU64  PRIu64
#define PRIS8   PRId8
#define PRIS16  PRId16
#define PRIS32  PRId32
#define PRIS64  PRId64

#define PRISTR8 ".*s"

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

typedef float  F32;
typedef double F64;

typedef void Void;

typedef Void VoidFunction(Void);

#define false 0
#define true  1

#define internal static
#define global   static
#define local    static

#if COMPILER_CLANG
#	define debug_break() __builtin_debugtrap()
#elif COMPILER_CL
#	define debug_break() DebugBreak()
#elif COMPILER_GCC
#	define debug_break() __builtin_trap()
#else
#	define debug_break() (*(S32 *)0 = 0)
#endif

#if ENABLE_ASSERT
#	define assert(expr) if (!(expr)) { debug_break(); }
#else
#	define assert(expr)
#endif

// TODO(simon): What about when ENABLE_ASSERT == 0?
#define assert_not_implemented() debug_break()
#define assert_not_reached() debug_break()
#define invalid_case default: debug_break(); break;

#define array_count(array) (sizeof(array) / sizeof((*array)))

#define stringify_(s) #s
#define stringify(s)  stringify_(s)
#define glue_(a, b)   a##b
#define glue(a, b)    glue_(a, b)

#define defer_loop_checked(begin, end) for(S32 glue(_i_, __LINE__) = 2 * !(begin); glue(_i_, __LINE__) == 2 ? ((end), 0) : !glue(_i_, __LINE__); ++glue(_i_, __LINE__), (end))
#define defer_loop(begin, end) for(S32 glue(_i_, __LINE__) = ((begin), 0); !glue(_i_, __LINE__); ++glue(_i_, __LINE__), (end))

#define int_from_ptr(p) (unsigned long long)((char*)(p) - (char*)0)
#define ptr_from_int(n) (Void*)((char *)0 + (n))

#define stmnt(x) do {x} while(0)

#define member(t, m)        (((t*)0)->m)
#define member_offset(t, m) offsetof(t, m)

#define kilobytes(n) ((n)*1024LL)
#define megabytes(n) (1024LL*kilobytes(n))
#define gigabytes(n) (1024LL*megabytes(n))
#define terabytes(n) (1024LL*gigabytes(n))

#define thousand(n) ((n)*1000)
#define million(n)  ((thousand(n))*1000)
#define billion(n)  ((million(n))*1000)

#define swap(a, b, type) \
{ \
type temp = a; \
a = b; \
b = temp; \
}

#define axis_flip(axis) (!(axis))
typedef enum Axis2 Axis2;
enum Axis2
{
	Axis2_X,
	Axis2_Y,

	Axis2_COUNT,
};

typedef enum Axis3 Axis3;
enum Axis3
{
	Axis3_X,
	Axis3_Y,
	Axis3_Z,

	Axis3_COUNT,
};

typedef enum Axis4 Axis4;
enum Axis4
{
	Axis4_X,
	Axis4_Y,
	Axis4_Z,
	Axis4_W,

	Axis4_COUNT,
};

#define side_flip(side) (!(side))
typedef enum Side Side;
enum Side
{
	Side_Min,
	Side_Max,

	Side_COUNT,
};

typedef enum Corner Corner;
enum Corner
{
	Corner_TopLeft,
	Corner_TopRight,
	Corner_BottomLeft,
	Corner_BottomRight,

	Corner_COUNT,
};

typedef enum OperatingSystem OperatingSystem;
enum OperatingSystem
{
	OperatingSystem_Null,
	OperatingSystem_Windows,
	OperatingSystem_Linux,
	OperatingSystem_Mac,

	OperatingSystem_COUNT,
};

typedef enum Architecture Architecture;
enum Architecture
{
	Architecture_Null,
	Architecture_X64,
	Architecture_X86,
	Architecture_ARM,
	Architecture_ARM64,

	Architecture_COUNT,
};

typedef enum BuildMode BuildMode;
enum BuildMode
{
	BuildMode_Null,
	BuildMode_Debug,
	BuildMode_Optimized,
	BuildMode_Release,

	BuildMode_COUNT
};

typedef enum Month Month;
enum Month
{
	Month_Jan,
	Month_Feb,
	Month_Mar,
	Month_Apr,
	Month_May,
	Month_Jun,
	Month_Jul,
	Month_Aug,
	Month_Sep,
	Month_Oct,
	Month_Nov,
	Month_Dec,

	Month_COUNT
};

typedef enum DayOfWeek DayOfWeek;
enum DayOfWeek
{
	DayOfWeek_Monday,
	DayOfWeek_Tuesday,
	DayOfWeek_Wednesday,
	DayOfWeek_Thursday,
	DayOfWeek_Friday,
	DayOfWeek_Saturday,
	DayOfWeek_Sunday,

	DayOfWeek_COUNT,
};

typedef struct DenseTime DenseTime;
struct DenseTime
{
	U64 time;
};

typedef struct DateTime DateTime;
struct DateTime
{
	U16 millisecond; // [0, 999]
	U8  second;      // [0, 60] 60 in the case of leap seconds
	U8  minute;      // [0, 59]
	U8  hour;        // [0, 23]
	U8  day;         // [0, 30]
	U8  month;       // [0, 11]
	S16 year;        // 1 = 1 CE; 2020 = 2020 CE, 0 = 1 BCE; -100 = 101 BCE; etc.
};

// TODO(hampus): Finish this implementation
// Different formats to display the date Dec 15 2023
typedef enum DateFormat DateFormat;
enum DateFormat
{
	DateFormat_YYMMDD,   // 23-12-15
	DateFormat_YYYYMMDD, // 2023-12-15
};

// TODO(hampus): Finish this implementation
// Different formats to display the time 12:08:30
typedef enum TimeFormat TimeFormat;
enum TimeFormat
{
	TimeFormat_HHMMSS,   // 12:08:30
};

internal OperatingSystem  operating_system_from_context(Void);
internal Architecture     architecture_from_context(Void);
internal BuildMode        build_mode_from_context(Void);
internal DateTime         build_date_from_context(Void);

typedef struct Str8 Str8;

internal Str8 string_from_architecture(Architecture arc);
internal Str8 string_from_operating_system(OperatingSystem os);
internal Str8 string_from_build_mode(BuildMode mode);
internal Str8 string_from_day_of_the_week(DayOfWeek day);
internal Str8 string_from_month(Month month);

internal DenseTime dense_time_from_date_time(DateTime *date_time);
internal DateTime  date_time_from_dense_time(DenseTime dense_time);

#endif //BASE_CORE_H
