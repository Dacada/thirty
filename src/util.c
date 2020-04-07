#define _DEFAULT_SOURCE

#include <util.h>
#include <stdio.h>
#include <stdlib.h>

void bail(const char *const msg, ...) {
        if (msg != NULL) {
                va_list ap;
                va_start(ap, msg);
                vfprintf(stderr, msg, ap);
                va_end(ap);
        }

        exit(EXIT_FAILURE);
}

void die(const char *const msg, ...) {
        if (msg != NULL) {
                va_list ap;
                va_start(ap, msg);
                vfprintf(stderr, msg, ap);
                va_end(ap);
        }

        abort();
}

void *smalloc(const size_t size) {
        void *const ptr = malloc(size);
        if (ptr == NULL) {
                perror("malloc");
                die(NULL);
        }
        return ptr;
}

// TODO: Ensure that it does what the current glibc does
void *scalloc(const size_t nmemb, const size_t size) {
        void *const ptr = calloc(nmemb, size);
        if (ptr == NULL) {
                perror("calloc");
                die(NULL);
        }
        return ptr;
}

void *srealloc(void *const ptr, const size_t size) {
        void *const new_ptr = realloc(ptr, size);
        if (new_ptr == NULL) {
                perror("realloc");
                die(NULL);
        }
        return new_ptr;
}

// TODO: Ensure that it does what the current glibc does
void *sreallocarray(void *const ptr, const size_t nmemb, const size_t size) {
        void *const new_ptr = reallocarray(ptr, nmemb, size);
        if (new_ptr == NULL) {
                perror("reallocarray");
                die(NULL);
        }
        return new_ptr;
}

FILE *sfopen(const char *const pathname, const char *const mode) {
        FILE *const f = fopen(pathname, mode);
        if (f == NULL) {
                perror(pathname);
                die("Failed to open file.");
        }
        return f;
}

void sfseek(FILE *const stream, const long offset, const int whence) {
        if (fseek(stream, offset, whence) == -1) {
                perror("fseek");
                die(NULL);
        }
}

size_t sftell(FILE *const stream) {
        const long size = ftell(stream);
        if (size < 0) {
                perror("ftell");
                die(NULL);
        }
        return (size_t)size;
}

void sfread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
        if (fread(ptr, size, nmemb, stream) != size &&
            ferror(stream) != 0) {
                perror("fread");
                die(NULL);
        }
}

void sfclose(FILE *stream) {
        if (fclose(stream) != 0) {
                perror("fclose");
                die(NULL);
        }
}

// TODO: Unittest this
bool accessible(const char *filepath, bool read, bool write, bool execute) {
        // TODO: Windows version
        
        int mode = 0;
        if (read) {
                mode |= R_OK;
        }
        if (write) {
                mode |= W_OK;
        }
        if (execute) {
                mode |= X_OK;
        }
        if (!read && !write && !execute) {
                mode = F_OK;
        }

        const bool ret = access(filepath, mode) == 0;
        if (!ret) {
                static char msg[256];
                snprintf(msg, 256, "Cannot access file %s", filepath);
                perror(msg);
        }
        return ret;
}

// TODO: windows version, copy from
// https://github.com/python/cpython/blob/master/Lib/ntpath.py
size_t pathnjoin(const size_t size, char *const dest, const int nargs, ...) {
        size_t len = 0;
        char *pdest = dest;
        va_list ap;
        va_start(ap, nargs);
        for (int arg=0; arg<nargs; arg++) {
                const char *p = va_arg(ap, const char *);
                if (*p == '/') {
                        pdest = dest;
                }
                for (;;) {
                        *(pdest++) = *(p++);
                        len++;
                        if (len == size) {
                                goto end;
                        }
                        if (*p == '\0') {
                                break;
                        }
                }
                if (*(pdest-1) != '/') {
                        *(pdest++) = '/';
                        len++;
                        if (len == size) {
                                goto end;
                        }
                }
        }
        *pdest = '\0';
        len++;
end:
        va_end(ap);
        return len;
}
