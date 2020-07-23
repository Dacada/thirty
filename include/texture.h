#ifndef TEXTURE_H
#define TEXTURE_H

#include <util.h>
#include <glad/glad.h>
#include <stdbool.h>

struct texture {
        bool loaded;
        char filepath[PATH_MAX];
        GLenum slot;
        GLuint idx;
};

void texture_init(struct texture *tex, const char *name, GLenum slot)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void texture_load(struct texture *tex)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void texture_bind(const struct texture *tex)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void texture_free(struct texture *tex)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* TEXTURE_H */