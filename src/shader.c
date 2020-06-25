#include <shader.h>
#include <util.h>
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static char *readall(const char *const filename) {
        if (!accessible(filename, true, false, false)) {
                bail("Failed to access shader file.\n");
        }
        
        FILE *const f = sfopen(filename, "rb");

        sfseek(f, 0L, SEEK_END);
        size_t size = sftell(f);
        fseek(f, 0L, SEEK_SET);

        char *const buff = scalloc(size+1, sizeof(char));
        sfread(buff, sizeof(char), size, f);
        sfclose(f);

        return buff;
}

static void buildpath(const size_t destsize, char *const dest,
                      const char *const file, const size_t extsize,
                      const char *const extension) {
        const size_t len = pathnjoin(destsize, dest, 3, ASSETSPATH,
                                     "shaders", file);
        if (len + extsize - 1 >= destsize) {
                die("Path to shader file too long.\n");
        }
        strcpy(dest+len-2, extension);
}

static void handle_compile_infolog(const unsigned int shader) {
        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == 0) {
                int length;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
                char *const info_log = scalloc((const size_t)length,
                                               sizeof(char));
                glGetShaderInfoLog(shader, length, NULL, info_log);
                fprintf(stderr, "Error compiling shaders:\n%s\n", info_log);
                free(info_log);
                die(NULL);
        }
}

static void handle_link_infolog(const unsigned int program) {
        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success == 0) {
                int length;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
                char *const info_log = scalloc((const size_t)length,
                                               sizeof(char));
                glGetProgramInfoLog(program, length, NULL, info_log);
                fprintf(stderr, "Error linking shaders:\n%s\n", info_log);
                free(info_log);
                die(NULL);
        }
}

static unsigned int compile_shader(char *const path,
                                   const char *const filename,
                                   const size_t ext_len,
                                   const char *const ext,
                                   const GLenum shader_type) {
        buildpath(PATH_MAX, path, filename, ext_len, ext);
        char *const vertex_src = readall(path);
        const char *const const_vertex_src = vertex_src;
        const unsigned int shader = glCreateShader(shader_type);
        glShaderSource(shader, 1, &const_vertex_src, NULL);
        glCompileShader(shader);
        handle_compile_infolog(shader);
        free(vertex_src);
        return shader;
}

static unsigned int link_shader(const unsigned int vertex_shader,
                                const unsigned int fragment_shader) {
        const unsigned int shader = glCreateProgram();
        glAttachShader(shader, vertex_shader);
        glAttachShader(shader, fragment_shader);
        glLinkProgram(shader);
        handle_link_infolog(shader);
        return shader;
}

unsigned int shader_new(const char *const vertfile,
                        const char *const fragfile) {
        static const char *const vert_ext = ".vert";
        static const size_t vert_ext_len = strlen(vert_ext);
        static const char *const frag_ext = ".frag";
        static const size_t frag_ext_len = strlen(frag_ext);
        
        char *const path = scalloc(PATH_MAX, sizeof(char));
        const unsigned int vert = compile_shader(
                path, fragfile, vert_ext_len, vert_ext, GL_VERTEX_SHADER);
        const unsigned int frag = compile_shader(
                path, vertfile, frag_ext_len, frag_ext, GL_FRAGMENT_SHADER);
        free(path);
        
        const unsigned int shader = link_shader(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);

        return shader;
}

void shader_use(const unsigned int shader) {
        glUseProgram(shader);
}

inline static int getloc(const unsigned int shader, const char *const name) {
        return glGetUniformLocation(shader, name);
}

void shader_setBool(const unsigned int shader, const char *const name,
		    const bool value) {
        glUniform1i(getloc(shader, name), (int)value); 
}

void shader_setInt(const unsigned int shader, const char *const name,
		   const int value) {
        glUniform1i(getloc(shader, name), value); 
}
void shader_setFloat(const unsigned int shader, const char *const name,
		     const float value) {
        glUniform1f(getloc(shader, name), value); 
}
void shader_setVec3(const unsigned int shader, const char *const name,
                    const vec3s value) {
        glUniform3fv(getloc(shader, name), 1, value.raw);
}
void shader_setVec4(const unsigned int shader, const char *const name,
                    const vec4s value) {
        glUniform4fv(getloc(shader, name), 1, value.raw);
}
void shader_setMat4(const unsigned int shader, const char *const name,
                    const mat4s value) {
        glUniformMatrix4fv(getloc(shader, name), 1, GL_FALSE, value.raw[0]);
}
