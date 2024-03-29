#ifndef CUTIL_UTIL_H
#define CUTIL_UTIL_H

#include <thirty/dsutils.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef ABS
#define ABS(x) ((x)<0?-(x):(x))
#endif

// This is usually the actual number, just in case it's not in
// limits.h
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
// TODO: Also for windows: 260

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#ifdef NDEBUG

#define assert(condition)                               \
        do {                                            \
                if (!(condition)) {                     \
                        __builtin_unreachable();        \
                }                                       \
        } while (0)

#define assert_fail() __builtin_unreachable();
#define dbg(msg, ...)

#else

#define assert(condition)                                               \
        do {                                                            \
                if (!(condition)) {                                     \
                        die("Assertion failed: " #condition             \
                            " on " __FILE__ ":%d\n", __LINE__);         \
                }                                                       \
        } while (0)

#define assert_fail()                                           \
        do {                                                    \
                die("Inconditional assertion falure on "        \
                    __FILE__ ":%d\n", __LINE__);                \
        } while (0)

#define dbg(msg, ...)                                   \
        fprintf(stderr, msg "\n",##__VA_ARGS__)

#endif

bool is_safe_multiply(const size_t a, const size_t b)
        __attribute__((const));

float clamp_angle(float angle, float min, float max)
        __attribute__((const));

/*
 * Exit program.
 * Exit using a call to exit with EXIT_FAILURE as the exit code.
 * Optionally can be passed a message and arguments printf style to be printed
 * to stderr before exiting.
 */
void bail(const char *msg, ...)
        __attribute__((access (read_only, 1)))
        __attribute__((format (printf, 1, 2)))
        __attribute__((noreturn));

/*
 * Exit program abnormally.
 * Exit using abort().
 * Optionally can be passed a message and arguments printf style to be printed
 * to stderr before exiting.
 */
void die(const char *msg, ...)
        __attribute__((access (read_only, 1)))
        __attribute__((format (printf, 1, 2)))
        __attribute__((noreturn));

/* 
 * The following functions have the same behavior as their counterparts without
 * the starting s, but can be called without error checking. In case oferror
 * perror is called and the program aborts via the previously declared die()
 * function.
 */

void *smalloc(size_t size)
        __attribute__((alloc_size (1)))
        __attribute__((malloc))
        __attribute__((warn_unused_result));

// like malloc, but ensures nmemb * size doesn't overflow, doesn't align memory
void *smallocarray(size_t nmemb, size_t size)
        __attribute__((alloc_size (1, 2)))
        __attribute__((malloc))
        __attribute__((warn_unused_result));

void *srealloc(void *ptr, size_t size)
        __attribute__((alloc_size (2)))
        __attribute__((warn_unused_result));

// similar to smallocarray, but for realloc, also no memory alignment
void *sreallocarray(void *ptr, size_t nmemb, size_t size)
        __attribute__((alloc_size (2, 3)))
        __attribute__((warn_unused_result));

char *sstrdup(const char *s)
        __attribute__((warn_unused_result))
        __attribute__((nonnull));

size_t strlenu(const unsigned char *s)
        __attribute__((nonnull));

unsigned char *sstrdupu(const unsigned char *s)
        __attribute__((warn_unused_result))
        __attribute__((nonnull));

FILE *sfopen(const char *pathname, const char *mode)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));

int sopen(const char *pathname, int flags)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull))
        __attribute__((warn_unused_result));

DIR *sopendir(const char *pathname)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));

void sfseek(FILE *stream, long offset, int whence)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

size_t slseek(int filedes, off_t offset, int whence);

size_t sftell(FILE *stream)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void sfread(void *ptr, size_t size, size_t nmemb,
            FILE *stream)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 4)))
        __attribute__((nonnull));

struct dirent *sreaddir(DIR *stream)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void sfclose(FILE *stream)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));


/*
 * Check whether the given filepath is accessible for the given operations in
 * an OS independent way. In case it is not accessible, a suitable message is
 * also printed to stderr.
 */
bool accessible(const char *filepath, bool read, bool write, bool execute)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));


/* Join paths, like python's os.path.join, yes it's also OS independent. */
size_t pathjoin(size_t size, char *dest, int nargs, ...)
        __attribute__((access (write_only, 2, 1)))
        __attribute__((nonnull));

/* Join paths like pathjoin, but using a va_list instead */
size_t pathjoin_va(size_t size, char *dest, int nargs, va_list ap)
        __attribute__((access (write_only, 2, 1)))
        __attribute__((nonnull));

/* Join paths like pathjoin but create a new string in dynamic memory. */
char *pathjoin_dyn(int nargs, ...)
        __attribute__((nonnull));

/*
 * Read a 32 bit unsigned integer from a file, then read that many 8 bit
 * integers from that same file and return it as a dynamically allocated
 * string.
 */
char *strfile(FILE *f)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Set the current working directory as dir, starting from whatever directory
 * the executable resides at.
 */
void set_cwd(const char *dir)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

#endif /* CUTIL_UTIL_H */
