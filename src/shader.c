#include <shader.h>
#include <util.h>
#include <cglm/struct.h>
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

// Holds currently created shaders, or 0 if the shader hasn't been created yet.
static unsigned shaders[SHADERS_TOTAL];

__attribute__((access (read_only, 3, 1)))
__attribute__((access (read_only, 4, 2)))
__attribute__((access (write_only, 5)))
__attribute__((nonnull (5)))
__attribute__((malloc))
__attribute__((returns_nonnull))
static char *strcat_new(const size_t lena, const size_t lenb,
                        char *const restrict a,
                        char *const restrict b,
                        size_t *const restrict lenc) {
        char *const restrict c = smallocarray(lena + lenb + 1, sizeof(char));
        if (a != NULL) {
                strncpy(c, a, lena);
        }
        if (b != NULL) {
                strncpy(c + lena, b, lenb);
        }
        c[lena + lenb] = '\0';
        *lenc = lena + lenb;
        free(a);
        free(b);
        return c;
}

__attribute__((access (read_only, 1)))
__attribute__((access (write_only, 2)))
__attribute__((nonnull))
__attribute__((malloc))
__attribute__((returns_nonnull))
static char *readall(const char *const restrict filename,
                     size_t *const restrict len) {
        if (!accessible(filename, true, false, false)) {
                bail("Failed to access shader file.\n");
        }
        
        FILE *const restrict f = sfopen(filename, "rb");

        sfseek(f, 0L, SEEK_END);
        size_t size = sftell(f);
        fseek(f, 0L, SEEK_SET);

        char *const restrict buff = smallocarray(size+1, sizeof(char));
        sfread(buff, sizeof(char), size, f);
        sfclose(f);

        *len = size;
        return buff;
}

// Careful! 'file' and 'extension' should never point to the same string!
__attribute__((access (write_only, 2, 1)))
__attribute__((access (read_only, 3)))
__attribute__((access (read_only, 5, 4)))
__attribute__((nonnull))
static void buildpath(const size_t destsize, char *const restrict dest,
                      const char *const restrict file, const size_t extsize,
                      const char *const restrict extension) {
        const size_t len = pathjoin(destsize, dest, 3, ASSETSPATH,
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
                char *const restrict info_log =
                        smallocarray((const size_t)length, sizeof(char));
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
                char *const restrict info_log =
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
static unsigned int compile_shader(char *const restrict path,
                                   const size_t nfilenames,
                                   const char *const filenames[],
                                   const size_t ext_len,
                                   const char *const restrict ext,
                                   const GLenum shader_type) {
        char *totalsrc = NULL;
        size_t totalsrc_len = 0;
        for (size_t i=0; i<nfilenames; i++) {
                const char *const filename = filenames[i];
                buildpath(PATH_MAX, path, filename, ext_len, ext);
                size_t fragsrc_len;
                char *const restrict fragsrc = readall(path, &fragsrc_len);
                size_t newlen;
                totalsrc = strcat_new(
                        fragsrc_len, totalsrc_len, fragsrc, totalsrc, &newlen);
                totalsrc_len = newlen;
        }
                
        const char *const const_src = totalsrc;
        const unsigned int shader = glCreateShader(shader_type);
        glShaderSource(shader, 1, &const_src, NULL);
        glCompileShader(shader);
        handle_compile_infolog(shader);
        free(totalsrc);
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
        static const char *const restrict vert_ext = ".vert";
        static const char *const restrict frag_ext = ".frag";
        static const size_t vert_ext_len = strlen(vert_ext);
        static const size_t frag_ext_len = strlen(frag_ext);

        char *const restrict path = smallocarray(PATH_MAX, sizeof(char));
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
                shader_setInt(shader, "ambientTexture", 0);
                shader_setInt(shader, "emissiveTexture", 1);
                shader_setInt(shader, "diffuseTexture", 2);
                shader_setInt(shader, "specularTexture", 3);
                shader_setInt(shader, "specularPowerTexture", 4);
                shader_setInt(shader, "normalTexture", 5);
                shader_setInt(shader, "bumpTexture", 6);
                shader_setInt(shader, "opacityTexture", 7);
                break;
        default:
                die("Unexpected Shader");
        }
}

static unsigned get_shader_id(const enum shaders shader) {
        if (shaders[shader] != 0) {
                return shaders[shader];
        }

        static const size_t uber_nvertfiles = 1;
        static const size_t uber_nfragfiles = 1;

        static const char *const uber_vertfiles[] =
                {"uber_forward"};
        static const char *const uber_fragfiles[] =
                {"uber_forward"};

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
void shader_setMat4(const enum shaders shader, const char *const name,
                    const mat4s value) {
        glUniformMatrix4fv(getloc(shader, name), 1, GL_FALSE, value.raw[0]);
}
