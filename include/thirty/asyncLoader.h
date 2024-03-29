#ifndef ASYNCLOADER_H
#define ASYNCLOADER_H

#include <stddef.h>
#include <stdbool.h>

typedef void(*asyncLoader_cb)(void*, size_t, void*);

// Initialize async loading system
void asyncLoader_init(void);

// Enqueue a read to the async loading system. When the read finished, the
// callback will be called with the buffer with the data, the size of the data
// and the args pointer.
void asyncLoader_enqueueRead(const char *filepath, asyncLoader_cb callback,
                             void *callbackArgs)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (1)));

// Nonblocking. Should be called periodically. Reaps at most one sync load
// operation. Returns false if all operations have been reaped. If size is not
// null, writes the size of the reaped operation to *size if one was reaped or
// zero otherwise.
bool asyncLoader_await(size_t *sizePtr);

// Returns the total size of all enqueued reads
size_t asyncLoader_totalSize(void);

// Free all resources used by a collection of async loader system. Should be
// called if system won't be used for a while to free up resources.
void asyncLoader_destroy(void);

// Helper function to copy over nmemb elements of size size into dest from src
// at the offset indicated by *offset which is then shifted forward by
// nmemb*size bytes.
void asyncLoader_copyBytes(void *restrict dest, const void *restrict src,
                           size_t nmemb, size_t size, size_t *offset);

#endif /* ASYNCLOADER_H */
