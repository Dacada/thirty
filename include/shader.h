#ifndef SHADER_H
#define SHADER_H

#include <cglm/struct.h>
#include <stdbool.h>

/*
 * Creating a shader must be done by giving a filename without extension. It
 * will look in the assets directory, shaders subdirectory, for two files with
 * the given names and two different hardcoded extensions: one for the fragment
 * shader and another for the vertex shader. For example, shader_new("foo",
 * "bar") will load assets/shaders/foo.vert and assets/shaders/bar.frag then
 * compile them and link them and return the resulting program. And of course,
 * crash horribly on any kind of error.
 */

unsigned int shader_new(const char *restrict vertfile,
                        const char *restrict fragfile)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_use(unsigned int shader)
        __attribute__((leaf));

void shader_setBool(unsigned int shader,
                    const char *restrict name, bool value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setInt( unsigned int shader,
                    const char *restrict name, int value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setFloat(unsigned int shader,
                     const char *restrict name, float value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setVec3(unsigned int shader,
                    const char *restrict name, vec3s value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setVec4(unsigned int shader,
                    const char *restrict name, vec4s value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void shader_setMat4(unsigned int shader,
                    const char *restrict name, mat4s value)
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif
