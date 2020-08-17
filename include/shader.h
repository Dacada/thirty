#ifndef SHADER_H
#define SHADER_H

#include <cglm/struct.h>
#include <stdbool.h>

/*
 * Shaders are created automatically when shader_use is called. Shaders are
 * created from various file fragments in the assets directory.
 *
 * In practice, simply call shader_use(SHADER_UBER) to use a shader. What
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

enum shaders {
        SHADER_UBER,
        SHADER_SKYBOX,
        
        SHADER_TOTAL
};

/*
 * Use the given shader. This might collect, compile and link the shader.
 */
void shader_use(enum shaders shader);

/*
 * Set a boolean uniform to the given shader.
 */
void shader_setBool(enum shaders shader,
                    const char *name, bool value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Set a int uniform to the given shader.
 */
void shader_setInt(enum shaders shader,
                   const char *name, int value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Set a unsigned int uniform to the given shader.
 */
void shader_setUInt(enum shaders shader,
                   const char *name, unsigned value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Set a float uniform to the given shader.
 */
void shader_setFloat(enum shaders shader,
                     const char *name, float value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Set a vec3 uniform to the given shader.
 */
void shader_setVec3(enum shaders shader,
                    const char *name, vec3s value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Set a vec4 uniform to the given shader.
 */
void shader_setVec4(enum shaders shader,
                    const char *name, vec4s value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Set a mat3 uniform to the given shader.
 */
void shader_setMat3(enum shaders shader,
                    const char *name, mat3s value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Set a mat4 uniform to the given shader.
 */
void shader_setMat4(enum shaders shader,
                    const char *name, mat4s value)
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

#endif
