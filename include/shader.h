#ifndef SHADER_H
#define SHADER_H

#include <cglm/struct.h>
#include <stdbool.h>

/*
 * Shaders are created automatically when shader_use is called. Shaders are
 * harcoded in the implementation of this file, and eventually created from
 * various file fragments in the assets directory.
 *
 * In practice, simply call shader_use(SHADER_PHONG) to use a shader. What
 * follows is the internal implementation:
 *
 * Creating a shader is internally done by using a hardcoded set of filenames
 * without extension for the vertex and for the fragment parts. It will look in
 * the assets directory, shaders subdirectory, for these files with two
 * different hardcoded extensions: one for the fragment shader and another for
 * the vertex shader. The two sets of files are each concatenated in order and
 * then compiled and linked. Crash on error.

 * For example, shader_new({"foo", "oof}, {"bar", "rab"}) would load
 * assets/shaders/foo.vert, assets/shaders/oof.vert, assets/shaders/bar.frag,
 * assets/shaders/rab.frag. Concatenate the contents of foo.vert and
 * oof.vert. Concatenate the contents of bar.frag and rab.frag. Then compile
 * the two resulting souorce codes and link them. Finally return the resulting
 * program.
 */

#define SHADERS_TOTAL 1

enum shaders {
        SHADER_UBER
};

void shader_use(enum shaders shader)
        __attribute__((leaf));

void shader_setBool(enum shaders shader,
                    const char *restrict name, bool value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setInt(enum shaders shader,
                   const char *restrict name, int value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setUInt(enum shaders shader,
                   const char *restrict name, unsigned value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setFloat(enum shaders shader,
                     const char *restrict name, float value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setVec3(enum shaders shader,
                    const char *restrict name, vec3s value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setVec4(enum shaders shader,
                    const char *restrict name, vec4s value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setMat4(enum shaders shader,
                    const char *restrict name, mat4s value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif
