#include <thirty/asyncLoader.h>
#include <thirty/util.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

bool asyncLoader_finished(struct asyncLoader *const loader) {
        if (loader->finished) {
                return true;
        }

        switch (aio_error(&loader->aiocb)) {
        case EINPROGRESS:
                return false;
        case ECANCELED:
                bail("asynchronous IO request was cancelled");
        case 0:
                break;
        default:
                perror("aio");
                die(NULL);
        }

        loader->onFinishCb(loader->buf, loader->aiocb.aio_nbytes, loader->onFinishCbArgs);
        close(loader->aiocb.aio_fildes);
        free(loader->buf);
        
        loader->finished = true;
        return true;
}

void asyncLoader_setFinished(struct asyncLoader *const loader) {
        loader->finished = true;
        loader->onFinishCb = NULL;
        loader->onFinishCbArgs = NULL;
}

void asyncLoader_read(struct asyncLoader *const loader, const char *const filename,
                      asyncLoader_cb cb, void *const args) {
        int fd = open(filename, O_RDONLY);
        ssize_t sizeSign = lseek(fd, 0, SEEK_END);
        assert(sizeSign > 0);
        size_t size = (size_t)sizeSign;
        loader->buf = smalloc(size);
        
        loader->aiocb.aio_fildes = fd;
        loader->aiocb.aio_offset = 0;
        loader->aiocb.aio_buf = loader->buf;
        loader->aiocb.aio_nbytes = size;
        loader->aiocb.aio_reqprio = 0;
        loader->aiocb.aio_sigevent.sigev_notify = SIGEV_NONE;
        
        loader->finished = false;
        loader->onFinishCb = cb;
        loader->onFinishCbArgs = args;

        aio_read(&loader->aiocb);
}

void asyncLoader_copyBytes(void *const restrict dest, const void *const restrict src,
                           const size_t nmemb, const size_t size, size_t *const ptr) {
        if (!is_safe_multiply(nmemb, size)) {
                die("copyButes would overflow (%lu elements of size %lu)", nmemb, size);
        }
        size_t amount = nmemb * size;
        memcpy(dest, (const char*)src + *ptr, amount);
        *ptr += amount;
}
