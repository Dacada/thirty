#ifndef CUTIL_UTIL_H
#define CUTIL_UTIL_H

#include <unistd.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define NULLDEFAULT(ptr, dflt) (((ptr) == NULL) ? (dflt) : (*(ptr)))

/*
 * Exit program.
 * Exit using a call to exit with EXIT_FAILURE as the exit code.
 * Optionally can be passed a message and arguments printf style to be printed
 * to stderr before exiting.
 */
void bail(const char *msg, ...)
        __attribute__((cold))
        __attribute__((format (printf, 1, 2)))
        __attribute__((noreturn));

/*
 * Exit program abnormally.
 * Exit using abort().
 * Optionally can be passed a message and arguments printf style to be printed
 * to stderr before exiting.
 */
void die(const char *msg, ...)
        __attribute__((cold))
        __attribute__((format (printf, 1, 2)))
        __attribute__((noreturn));

/* 
 * The following functions have the same behavior as their counterparts without
 * the starting s, but can be called without error checking. In case oferror
 * perror is called and the program aborts via the previously declared die()
 * function.
 */

void *smalloc(size_t size)
        __attribute__((malloc))
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));

// glibc behaviour: also ensure nmemb*size won't overflow
void *scalloc(size_t nmemb, size_t size)
        __attribute__((malloc))
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));

void *srealloc(void *const ptr, size_t size)
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));

// unique to glibc, provides special implementation for windows; also ensures
// nmemb*size won't overflow
void *sreallocarray(void *const ptr, size_t nmemb, size_t size)
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));


FILE *sfopen(const char *const pathname, const char *const mode)
        __attribute__((returns_nonnull))
        __attribute__((warn_unused_result));

void sfseek(FILE *const stream, const long offset, const int whence);

size_t sftell(FILE *const stream);

void sfread(void *ptr, size_t size, size_t nmemb, FILE *stream);

void sfclose(FILE *stream);


/*
 * Check whether the given filepath is accessible for the given operations in
 * an OS independent way. In case it is not accessible, a suitable message is
 * also printed to stderr.
 */
bool accessible(const char *filepath, bool read, bool write, bool execute);


/* Join paths, like python's os.path.join, yes it's also OS independent. */
size_t pathnjoin(const size_t size, char *const dest, const int nargs, ...);

#endif /* CUTIL_UTIL_H */
