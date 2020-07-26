#include <shader.h>
#include <util.h>
#include <cglm/struct.h>
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

static const int ambientTextureSampler = 0;
static const int emissiveTextureSampler = 1;
static const int diffuseTextureSampler = 2;
static const int specularTextureSampler = 3;
static const int specularPowerTextureSampler = 4;
static const int normalTextureSampler = 5;
static const int bumpTextureSampler = 6;
static const int opacityTextureSampler = 7;
static const int environmentTextureSampler = 8;

// Holds currently created shaders, or 0 if the shader hasn't been created yet.
static unsigned shaders[SHADER_TOTAL];

__attribute__((access (read_only, 1)))
__attribute__((nonnull))
__attribute__((returns_nonnull))
static char *readall(const char *const filename) {
        if (!accessible(filename, true, false, false)) {
                bail("Failed to access shader file.\n");
        }
        
        FILE *const f = sfopen(filename, "rb");

        sfseek(f, 0L, SEEK_END);
        size_t size = sftell(f);
        fseek(f, 0L, SEEK_SET);

        char *const buff = smallocarray(size+1, sizeof(char));
        sfread(buff, sizeof(char), size, f);
        buff[size] = '\0';
        sfclose(f);

        return buff;
}

// Careful! 'file' and 'extension' should never point to the same string!
__attribute__((access (write_only, 2, 1)))
__attribute__((access (read_only, 3)))
__attribute__((access (read_only, 5, 4)))
__attribute__((nonnull))
static void buildpath(const size_t destsize, char *const dest,
                      const char *const file, const size_t extsize,
                      const char *const extension) {
        const size_t len = pathjoin(destsize, dest, 3, ASSETSPATH,
                                     "shaders", file);
        if (len + extsize - 1 >= destsize) {
                die("Path to shader file too long.\n");
        }
        strcpy(dest+len-2, extension);
}

__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static void handle_compile_infolog(const unsigned int shader,
                                   const char *const which) {
        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == 0) {
                int length;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
                char *const info_log =
                        smallocarray((const size_t)length, sizeof(char));
                glGetShaderInfoLog(shader, length, NULL, info_log);
                fprintf(stderr, "Error compiling %s shader:\n%s\n",
                        which, info_log);
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
                char *const info_log =
                        smallocarray((const size_t)length, sizeof(char));
                glGetProgramInfoLog(program, length, NULL, info_log);
                fprintf(stderr, "Error linking shaders:\n%s\n", info_log);
                free(info_log);
                die(NULL);
        }
}

__attribute__((access (write_only, 1)))
__attribute__((access (read_only, 3)))
__attribute__((access (read_only, 5, 4)))
__attribute__((nonnull))
static unsigned int compile_shader(char *const path,
                                   const size_t nfilenames,
                                   const char *const filenames[],
                                   const size_t ext_len,
                                   const char *const ext,
                                   const GLenum shader_type) {
        char *sources[nfilenames];
        for (size_t i=0; i<nfilenames; i++) {
                const char *const filename = filenames[i];
                buildpath(PATH_MAX, path, filename, ext_len, ext);
                char *const src = readall(path);
                sources[i] = src;
        }
        
        const unsigned int shader = glCreateShader(shader_type);
        if (nfilenames > INT_MAX) {
                die("Too many files for shader\n");
        }
        glShaderSource(shader, (int)nfilenames,
                       (const char *const *)sources, NULL);
        glCompileShader(shader);
        handle_compile_infolog(shader,
                               shader_type == GL_VERTEX_SHADER ?
                               "vertex" : "fragment");

        for (size_t i=0; i<nfilenames; i++) {
                free(sources[i]);
        }
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

__attribute__((nonnull))
static unsigned int shader_new(const size_t nvertfiles,
                               const size_t nfragfiles,
                               const char *const vertfiles[nvertfiles],
                               const char *const fragfiles[nfragfiles]) {
        static const char *const vert_ext = ".vert";
        static const char *const frag_ext = ".frag";
        static const size_t vert_ext_len = strlen(vert_ext);
        static const size_t frag_ext_len = strlen(frag_ext);

        char *const path = smallocarray(PATH_MAX, sizeof(char));
        const unsigned int vert = compile_shader(
                path, nfragfiles, fragfiles, vert_ext_len, vert_ext,
                GL_VERTEX_SHADER);
        const unsigned int frag = compile_shader(
                path, nvertfiles, vertfiles, frag_ext_len, frag_ext,
                GL_FRAGMENT_SHADER);
        free(path);
        
        const unsigned int shader = link_shader(vert, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);

        return shader;
}

static void init_shader(const enum shaders shader) {
        shader_use(shader);

        switch (shader) {
        case SHADER_UBER:
                shader_setInt(shader, "ambientTexture", ambientTextureSampler);
                shader_setInt(shader, "emissiveTexture", emissiveTextureSampler);
                shader_setInt(shader, "diffuseTexture", diffuseTextureSampler);
                shader_setInt(shader, "specularTexture", specularTextureSampler);
                shader_setInt(shader, "specularPowerTexture", specularPowerTextureSampler);
                shader_setInt(shader, "normalTexture", normalTextureSampler);
                shader_setInt(shader, "bumpTexture", bumpTextureSampler);
                shader_setInt(shader, "opacityTexture", opacityTextureSampler);
                break;
        case SHADER_SKYBOX:
                shader_setInt(shader, "skybox", environmentTextureSampler);
                break;
        case SHADER_TOTAL:
        default:
                die("Unexpected Shader");
        }
}

static unsigned get_shader_id(const enum shaders shader) {
        if (shaders[shader] != 0) {
                return shaders[shader];
        }

#define SHADERFILES(what, which, ...)                                   \
        static const char *const which##_##what##files[] = __VA_ARGS__; \
        static const size_t which##_n##what##files =                    \
                sizeof(which##_##what##files) / sizeof(*which##_##what##files)

        SHADERFILES(vert, uber, {"header", "uber"});
        SHADERFILES(frag, uber, {"header", "uber"});
        SHADERFILES(vert, skybox, {"header", "skybox"});
        SHADERFILES(frag, skybox, {"header", "skybox"});

        size_t nvertfiles;
        size_t nfragfiles;
        const char *const *vertfiles;
        const char *const *fragfiles;

        switch (shader) {
        case SHADER_UBER:
                nvertfiles = uber_nvertfiles;
                nfragfiles = uber_nfragfiles;
                vertfiles = uber_vertfiles;
                fragfiles = uber_fragfiles;
                break;
        case SHADER_SKYBOX:
                nvertfiles = skybox_nvertfiles;
                nfragfiles = skybox_nfragfiles;
                vertfiles = skybox_vertfiles;
                fragfiles = skybox_fragfiles;
                break;
        case SHADER_TOTAL:
        default:
                die("Unexpected Shader");
        }

        unsigned id = shader_new(nvertfiles, nfragfiles, vertfiles, fragfiles);
        shaders[shader] = id;

        init_shader(shader);
        return id;
}

void shader_use(const enum shaders shader) {
        unsigned id = get_shader_id(shader);
        glUseProgram(id);
}

__attribute__((nonnull))
inline static int getloc(const enum shaders shader, const char *const name) {
        unsigned id = get_shader_id(shader);
        return glGetUniformLocation(id, name);
}

void shader_setBool(const enum shaders shader, const char *const name,
		    const bool value) {
        shader_setUInt(shader, name, value);
}
void shader_setInt(const enum shaders shader, const char *const name,
		   const int value) {
        glUniform1i(getloc(shader, name), value);
}
void shader_setUInt(const enum shaders shader, const char *const name,
                    const unsigned value) {
        glUniform1ui(getloc(shader, name), value);
}
void shader_setFloat(const enum shaders shader, const char *const name,
		     const float value) {
        glUniform1f(getloc(shader, name), value);
}
void shader_setVec3(const enum shaders shader, const char *const name,
                    const vec3s value) {
        glUniform3fv(getloc(shader, name), 1, value.raw);
}
void shader_setVec4(const enum shaders shader, const char *const name,
                    const vec4s value) {
        glUniform4fv(getloc(shader, name), 1, value.raw);
}
void shader_setMat3(const enum shaders shader, const char *const name,
                    const mat3s value) {
        glUniformMatrix3fv(getloc(shader, name), 1, GL_FALSE, value.raw[0]);
}
void shader_setMat4(const enum shaders shader, const char *const name,
                    const mat4s value) {
        glUniformMatrix4fv(getloc(shader, name), 1, GL_FALSE, value.raw[0]);
}
