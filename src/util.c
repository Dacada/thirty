#define _DEFAULT_SOURCE

#include <thirty/util.h>
#include <cglm/cglm.h>
#include <unistd.h> //TODO: Only for linux
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 256

__attribute__((const))
static bool is_safe_multiply(const size_t a, const size_t b) {
        if (a == 0 || b == 0) {
                return true;
        }
        
        static const int sizet_size_bits = sizeof(size_t) * 8;
        const int a_size_bits = sizet_size_bits - __builtin_clzl(a);
        const int b_size_bits = sizet_size_bits - __builtin_clzl(b);
        return a_size_bits + b_size_bits <= sizet_size_bits;
}

float clamp_angle(float angle, float minVal, float maxVal) {
        if (angle < -GLM_2_PIf) {
                angle += GLM_2_PIf * (-angle/GLM_2_PIf);
        }
        if (angle > GLM_2_PIf) {
                angle -= GLM_2_PIf * (angle/GLM_2_PIf);
        }
        return glm_clamp(angle, minVal, maxVal);
}

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
        if (size == 0) {
                return NULL;
        }
        
        void *const ptr = malloc(size);
        if (ptr == NULL) {
                perror("malloc");
                die(NULL);
        }
        return ptr;
}

void *smallocarray(const size_t nmemb, const size_t size) {
        if (is_safe_multiply(nmemb, size)) {
                return smalloc(nmemb * size);
        }
        die("calloc would overflow (%lu elements of size %lu)", nmemb, size);
}

void *srealloc(void *const ptr, const size_t size) {
        void *const new_ptr = realloc(ptr, size);
        if (new_ptr == NULL && size > 0) {
                perror("realloc");
                die(NULL);
        }
        return new_ptr;
}

void *sreallocarray(void *const ptr, const size_t nmemb, const size_t size) {
        void *const new_ptr = reallocarray(ptr, nmemb, size);
        if (new_ptr == NULL && nmemb > 0 && size > 0) {
                perror("reallocarray");
                die(NULL);
        }
        return new_ptr;
}

char *sstrdup(const char *const s) {
        char *ptr = strdup(s);
        if (ptr == NULL) {
                perror("strdup");
                die(NULL);
        }
        return ptr;
}

size_t strlenu(const unsigned char *s) {
        size_t len;
        for (len = 0;; len++) {
                if (s[len] == '\0') {
                        return len;
                }
        }
}

unsigned char *sstrdupu(const unsigned char *s) {
        size_t len = strlenu(s);

        unsigned char *ptr = smallocarray(len, sizeof(unsigned char));
        memcpy(ptr, s, len * sizeof(unsigned char));
        return ptr;
}

FILE *sfopen(const char *const pathname, const char *const mode) {
        FILE *const f = fopen(pathname, mode);
        if (f == NULL) {
                perror(pathname);
                die("Failed to open file.");
        }
        return f;
}

DIR *sopendir(const char *const pathname) {
        DIR *const d = opendir(pathname);
        if (d == NULL) {
                perror(pathname);
                die("Failed to open directory.");
        }
        return d;
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

void sfread(void *const ptr, const size_t size, const size_t nmemb,
            FILE *const stream) {
        if (!is_safe_multiply(nmemb, size)) {
                die("sfread would overflow (%lu elements of size %lu)",
                    nmemb, size);
        }
        
        if (fread(ptr, size, nmemb, stream) != size &&
            ferror(stream) != 0) {
                perror("fread");
                die(NULL);
        }
}

struct dirent *sreaddir(DIR *stream) {
        errno = 0;
        struct dirent *dirent = readdir(stream);
        if (dirent == NULL && errno != 0) {
                perror("readdir");
                die(NULL);
        }
        return dirent;
}

void sfclose(FILE *const stream) {
        if (fclose(stream) != 0) {
                perror("fclose");
                die(NULL);
        }
}

// TODO: Unittest this
bool accessible(const char *filepath, const bool read, const bool write,
                const bool execute) {
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
                static char msg[BUFFER_SIZE];
                snprintf(msg, BUFFER_SIZE, "Cannot access file %s", filepath);
                perror(msg);
        }
        return ret;
}

// TODO: windows version, copy from
// https://github.com/python/cpython/blob/master/Lib/ntpath.py
size_t pathjoin_va(const size_t size, char *const dest,
                   const int nargs, va_list ap) {
        size_t len = 0;
        char *pdest = dest;

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
        return len;
}

size_t pathjoin(const size_t size, char *const dest, const int nargs, ...) {
        va_list ap;
        va_start(ap, nargs);
        size_t r = pathjoin_va(size, dest, nargs, ap);
        va_end(ap);
        return r;
}

char *pathjoin_dyn(int nargs, ...) {
        va_list ap;
        
        va_start(ap, nargs);
        size_t size = 0;
        for (int i=0; i<nargs; i++) {
                const char *p = va_arg(ap, const char *);
                size += strlen(p) + 1;  // add 1 to account for slashes
        }
        size++;  // account for final null character
        va_end(ap);

        char *dest = smallocarray(size, sizeof(*dest));

        va_start(ap, nargs);
        pathjoin_va(size, dest, nargs, ap);
        va_end(ap);
        return dest;
}

char *strfile(FILE *const f) {
        uint32_t len;
        sfread(&len, sizeof(len), 1, f);
        char *const str = smallocarray(len+1, sizeof(char));
        sfread(str, sizeof(char), len, f);
        str[len] = '\0';
        return str;
}

void set_cwd(const char *const dir) {
        char buff[PATH_MAX];
        ssize_t ret = readlink("/proc/self/exe", buff, PATH_MAX);
        if (ret < 0) {
                perror("readlink");
                die("could not determine executable path from proc filesystem");
        }
        if (ret >= PATH_MAX) {
                die("executable path too long");
        }
        buff[ret] = '\0';

        char *absdir = pathjoin_dyn(2, buff, dir);
        chdir(absdir);
        free(absdir);
}
