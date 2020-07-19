#ifndef CUTIL_UTIL_H
#define CUTIL_UTIL_H

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

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

#else

#define assert(condition)                                       \
        do {                                                    \
                if (!(condition)) {                             \
                        die("Assertion failed: " #condition     \
                            " on " __FILE__ ": %d", __LINE__);  \
                }                                               \
        } while (0)
#define assert_fail()                                           \
        do {                                                    \
                die("Inconditional assertion falure on "        \
                    __FILE__ ": %d", __LINE__);                 \
        } while (0)

#endif

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
        __attribute__((leaf))
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
        __attribute__((leaf))
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
        __attribute__((leaf))
        __attribute__((warn_unused_result));

// like malloc, but ensures nmemb * size doesn't overflow, doesn't align memory
void *smallocarray(size_t nmemb, size_t size)
        __attribute__((alloc_size (1, 2)))
        __attribute__((malloc))
        __attribute__((leaf))
        __attribute__((warn_unused_result));

void *srealloc(void *ptr, size_t size)
        __attribute__((alloc_size (2)))
        __attribute__((leaf))
        __attribute__((warn_unused_result));

// similar to smallocarray, but for realloc, also no memory alignment
void *sreallocarray(void *ptr, size_t nmemb, size_t size)
        __attribute__((alloc_size (2, 3)))
        __attribute__((leaf))
        __attribute__((warn_unused_result));

FILE *sfopen(const char *pathname, const char *mode)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull))
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));

void sfseek(FILE *stream, long offset, int whence)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

size_t sftell(FILE *stream)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void sfread(void *ptr, size_t size, size_t nmemb,
            FILE *stream)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 4)))
        __attribute__((leaf))
        __attribute__((nonnull));

void sfclose(FILE *stream)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));


/*
 * Check whether the given filepath is accessible for the given operations in
 * an OS independent way. In case it is not accessible, a suitable message is
 * also printed to stderr.
 */
bool accessible(const char *filepath, bool read, bool write, bool execute)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));


/* Join paths, like python's os.path.join, yes it's also OS independent. */
size_t pathjoin(size_t size, char *dest, int nargs, ...)
        __attribute__((access (write_only, 2, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* CUTIL_UTIL_H */
