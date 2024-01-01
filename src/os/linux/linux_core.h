#ifndef LINUX_ESSENTIAL_H
#define LINUX_ESSENTIAL_H

#include <semaphore.h>

// TODO(simon): Query this on startup, even though this is the most common
// maximum limit for file name lengths.
#define LINUX_NAME_MAX 255

// NOTE(simon): Copied from `man getdents64.2` because it is not included in
// the `dirent.h`.
typedef struct Linux_Dirent64 Linux_Dirent64;
struct Linux_Dirent64
{
	ino64_t        d_ino;    // 64-bit inode number
	off64_t        d_off;    // 64-bit offset to the next structure
	unsigned short d_reclen; // Size of this dirent
	unsigned char  d_type;   // File type
	char           d_name[]; // Filenam (null-terminated)
};

typedef struct Linux_FileIterator Linux_FileIterator;
struct Linux_FileIterator
{
	S32 file_descriptor;
	B32 done;
	U8 buffer[sizeof(Linux_Dirent64) + LINUX_NAME_MAX];
	U32 read_index;
	U32 write_index;
};

typedef struct Linux_ThreadArguments Linux_ThreadArguments;
struct Linux_ThreadArguments
{
	ThreadProc *proc;
	Void       *data;
};

struct OS_Semaphore
{
	sem_t semaphore;
};

struct OS_Mutex
{
	pthread_mutex_t mutex;
};

#endif // LINUX_ESSENTIAL_H
