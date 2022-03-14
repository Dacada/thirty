#include <thirty/asyncLoader.h>
#include <thirty/dsutils.h>
#include <thirty/util.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>

#define THREADS 2

struct loader {
        int fd;
        size_t size;
        void *buf;
        bool finished;
        asyncLoader_cb callback;
        void *callbackArgs;
};

static sem_t semaphore;
static pthread_mutex_t mutex;
static struct growingArray queue;
static size_t tail;
static size_t reapTail;
static size_t totalSize;

static pthread_t threads[THREADS];

static void *worker(void *args) {
        (void)args;
        for (;;) {
                struct loader loader;
                size_t idx;
                
                sem_wait(&semaphore);
                pthread_mutex_lock(&mutex);
                {
                        idx = tail;
                        tail++;
                
                        struct loader *ptr = growingArray_get(&queue, idx);
                        loader = *ptr;
                }
                pthread_mutex_unlock(&mutex);

                ssize_t s = read(loader.fd, loader.buf, loader.size);
                if (s < 0) {
                        perror("read");
                } else if ((size_t)s != loader.size) {
                        fprintf(stderr, "read: unexpected read size %ld (expected %lu)\n",
                                s, loader.size);
                }

                pthread_mutex_lock(&mutex);
                {
                        struct loader *ptr = growingArray_get(&queue, idx);
                        ptr->finished = true;
                }
                pthread_mutex_unlock(&mutex);
        }
        assert_fail();
}

void asyncLoader_init(void) {
        growingArray_init(&queue, sizeof(struct loader), 8);
        tail = 0;
        reapTail = 0;
        totalSize = 0;
        
        sem_init(&semaphore, 0, 0);
        pthread_mutex_init(&mutex, NULL);
        
        for (int i=0; i<THREADS; i++) {
                pthread_create(&threads[i], NULL, worker, NULL);
        }
}

void asyncLoader_enqueueRead(const char *const filepath, asyncLoader_cb callback,
                             void *const callbackArgs) {
        int fd = sopen(filepath, O_RDONLY);
        size_t size = (size_t)slseek(fd, 0, SEEK_END);
        slseek(fd, 0, SEEK_SET);
        void *buf = smalloc(size);

        totalSize += size;

        struct loader loader = {
                .fd = fd,
                .size = size,
                .buf = buf,
                .finished = false,
                .callback = callback,
                .callbackArgs = callbackArgs,
        };

        pthread_mutex_lock(&mutex);
        {
                struct loader *ptr = growingArray_append(&queue);
                *ptr = loader;
        }
        pthread_mutex_unlock(&mutex);
        sem_post(&semaphore);
}

bool asyncLoader_await(size_t *sizePtr) {
        size_t length;
        pthread_mutex_lock(&mutex);
        {
                length = queue.length;
        }
        pthread_mutex_unlock(&mutex);
        
        if (reapTail >= length) {
                return false;
        }
        
        asyncLoader_cb callback = NULL;
        void *callbackArgs = NULL;
        void *buffer = NULL;
        size_t size = 0;
        bool finished;
        pthread_mutex_lock(&mutex);
        {
                struct loader *ptr = growingArray_get(&queue, reapTail);
                finished = ptr->finished;
                if (finished) {
                        callback = ptr->callback;
                        callbackArgs = ptr->callbackArgs;
                        buffer = ptr->buf;
                        size = ptr->size;
                }
        }
        pthread_mutex_unlock(&mutex);
        
        if (finished) {
                callback(buffer, size, callbackArgs);
                reapTail++;
                *sizePtr = size;
        } else {
                *sizePtr = 0;
        }
        
        return true;
}

size_t asyncLoader_totalSize(void) {
        return totalSize;
}

void asyncLoader_destroy(void) {
        for (int i=0; i<THREADS; i++) {
                pthread_cancel(threads[i]);
        }
        for (int i=0; i<THREADS; i++) {
                void *ret;
                pthread_join(threads[i], &ret);
                assert(ret == PTHREAD_CANCELED);
        }

        pthread_mutex_destroy(&mutex);
        sem_destroy(&semaphore);

        growingArray_destroy(&queue);
}

void asyncLoader_copyBytes(void *restrict dest, const void *restrict src,
                           size_t nmemb, size_t size, size_t *offset) {
        if (!is_safe_multiply(nmemb, size)) {
                bail("multiplication would overflow: %lu * %lu", nmemb, size);
        }
        size_t total = nmemb * size;
        const void *offsetSrc = (const char*)src + *offset;
        memcpy(dest, offsetSrc, total);
        *offset += total;
        
}
