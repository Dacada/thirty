#ifndef ASYNCLOADER_H
#define ASYNCLOADER_H

#include <stdbool.h>
#include <stddef.h>
#include <aio.h>

typedef void(*asyncLoader_cb)(void*, size_t, void*);

struct asyncLoader {
        struct aiocb aiocb;
        bool finished;
        void *buf;
        asyncLoader_cb onFinishCb;
        void *onFinishCbArgs;
};

// Check whether an async read finished. If true is returned, a callback will
// possibly be called.
bool asyncLoader_finished(struct asyncLoader *loader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

// Set an uninitialized async loader as finished. Nothing is called on
// asyncLoader_finished returning true.
void asyncLoader_setFinished(struct asyncLoader *loader)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Open a file and read it in its entirety asynchronously. When finished, a
 * call to asyncLoader_finish will return true and will also call the cb
 * function. The first argument is a pointer to the read data, the second the
 * size in bytes of the data and the third is the args parameter of this
 * function. The data pointer should not be freed during the call to cb.
 */
void asyncLoader_read(struct asyncLoader *loader, const char *filename,
                      asyncLoader_cb, void *args)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

// Helper function to copy bytes from a memory area
void asyncLoader_copyBytes(void *restrict dest, const void *restrict src, size_t nmemb, size_t size, size_t *ptr)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_write, 5)))
        __attribute__((nonnull));

#endif /* ASYNCLOADER_H */
