#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <stdbool.h>

/*
 * A module for dealing with textures. They can be loaded and bound to OpenGL
 * for use.
 */

struct texture {
        bool loaded;
        char *name;
        GLenum slot;
        GLuint idx;
        GLenum type;
        int width;
        int height;
};

/*
 * Initialize a texture from the given parameters. Name is the file's name
 * without extension in the assets/textures directory. All textures are PNG.
 */
void texture_init(struct texture *tex, const char *name,
                  GLenum slot, GLenum type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Load a texture from its file.
 */
void texture_load(struct texture *tex)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Bind the texture to use for OpenGL
 */
void texture_bind(const struct texture *tex)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Free a texture, uninitializing it.
 */
void texture_free(struct texture *tex)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif /* TEXTURE_H */
