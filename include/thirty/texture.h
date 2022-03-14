#ifndef TEXTURE_H
#define TEXTURE_H

#include <stb_image.h>
#include <glad/glad.h>
#include <stdbool.h>

/*
 * A module for dealing with textures. They can be loaded and bound to OpenGL
 * for use.
 */

struct texture {
        bool loaded;
        GLenum slot;
        GLuint idx;
        GLenum type;
};

/*
 * Initialize a texture from the given parameters.
 */
void texture_init(struct texture *tex, GLenum slot, GLenum type)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull (1)));

/*
 * Load a texture from a loaded png image buffer. Only for single image textures.
 */
void texture_load(struct texture *tex, void *buff, size_t size)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Load cubemap texture with six png image buffers. Order: right, left, top,
 * bottom, front, back.
 */
void texture_loadCubeMap(struct texture *text, void *buffs[6], size_t sizes[6]);

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
