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

unsigned int shader_new(const char *vertfile, const char *fragfile);
void shader_use(unsigned int shader);
void shader_setBool(unsigned int shader, const char *name, bool value);
void shader_setInt( unsigned int shader, const char *name, int value);
void shader_setFloat(unsigned int shader, const char *name, float value);
void shader_setVec3(unsigned int shader, const char *name, vec3s value);
void shader_setVec4(unsigned int shader, const char *name, vec4s value);
void shader_setMat4(unsigned int shader, const char *name, mat4s value);

#endif
