#include <texture.h>
#include <dsutils.h>
#include <util.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <stdbool.h>
#include <string.h>

struct loadedTextureInfo {
        const char *name;
        GLuint idx;
        unsigned refcount;
};

static bool loadedTexturesInit = false;
static struct growingArray loadedTextures;

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
static int nameCompare(const void *a, const void *b, void *args) {
        // a and b will wither be pointers to a char pointer or pointers to the
        // loadedTextureInfo, which can also be interpreted as a pointer to its
        // first element, a char pointer
        const char *const *const name1 = a;
        const char *const *const name2 = b;
        (void)args;
        
        return strcmp(*name1, *name2);
}

__attribute__((access (read_write, 1)))
static bool assignLoadedTexture(struct texture *const tex) {
        if (!loadedTexturesInit) {
                return false;
        }

        struct loadedTextureInfo *info = growingArray_bsearch(
                &loadedTextures, &tex->name, nameCompare, NULL);
        if (info == NULL) {
                return false;
        }

        tex->idx = info->idx;
        tex->loaded = true;
        info->refcount++;
        return true;
}

__attribute__((access (read_only, 1)))
static void setLoadedTexture(const struct texture *const tex) {
        if (!loadedTexturesInit) {
                growingArray_init(&loadedTextures,
                                  sizeof(struct loadedTextureInfo), 2);
                loadedTexturesInit = true;
        }
        struct loadedTextureInfo *info = growingArray_append(&loadedTextures);
        info->name = sstrdup(tex->name);
        info->idx = tex->idx;
        info->refcount = 1;

        growingArray_sort(&loadedTextures, nameCompare, NULL);
}

__attribute__((access (read_only, 1)))
static void unsetLoadedTexture(const struct texture *const tex) {
        if (!loadedTexturesInit) {
                return;
        }
        
        struct loadedTextureInfo *info = growingArray_bsearch(
                &loadedTextures, &tex->name, nameCompare, NULL);
        
        if (info == NULL) {
#ifndef NDEBUG
                assert(false);
#else
                return;
#endif
        }
        if (info->refcount == 0) {
#ifndef NDEBUG
                assert(false);
#else
                return;
#endif
        }

        info->refcount--;
        bool cleanup = true;
        growingArray_foreach_START(&loadedTextures,
                                   struct loadedTextureInfo*, inf)
                if (inf->refcount > 0) {
                        cleanup = false;
                        return;
                }
        growingArray_foreach_END;
        if (cleanup) {
                growingArray_destroy(&loadedTextures);
                loadedTexturesInit = false;
        }
}

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static char *buildpathTex(const char *const file, const char *const ext) {
        char *path = pathjoin_dyn(3, ASSETSPATH, "textures", file);
        size_t pathlen = strlen(path);
        size_t extlen = strlen(ext);
        
        if (path[pathlen-1] == '/') {
                pathlen -= 1;
        }
        
        path = sreallocarray(path, pathlen + extlen + 1, sizeof(char));
        strcpy(path+pathlen, ext);
        if (!accessible(path, true, false, false)) {
                die("Cannot read texture file");
        }
        return path;
}

void texture_init(struct texture *const tex, const char *const name,
                    const GLenum slot, const GLenum type) {
        tex->loaded = false;
        tex->slot = slot;
        tex->type = type;
        tex->name = sstrdup(name);
}

__attribute__((access (read_only, 1)))
__attribute__((nonnull (1)))
static void loadImageIntoGl(const char *const filename,
                            const GLenum type, const bool flip,
                            int *const outWidth, int *const outHeight) {
        // TODO: Remove this and use callbacks instead
        fprintf(stderr, "Loading texture %s ...\n", filename);
        
        stbi_set_flip_vertically_on_load(flip);

        int width;
        int height;
        int nrChannels;
        unsigned char *const data __attribute__ ((nonstring)) =
                stbi_load(filename, &width, &height, &nrChannels, 0);
        if (data == NULL) {
                bail("Can't read texture image data.\n");
        }

        GLenum format;
        if (nrChannels == 1) { // Monochrome PNG (mask)
                format = GL_RED;
        } else if (nrChannels == 3) { // PNG without transparency data
                format = GL_RGB;
        } else if (nrChannels == 4) { // PNG with transparency data
                format =  GL_RGBA;
        } else { // Abomination
                die("Failing to load png texture. I expected 3 or 4 "
                    "channels but this thing has %d?\n", nrChannels);
        }

        glTexImage2D(type, 0, GL_RGBA, width, height, 0,
                     format, GL_UNSIGNED_BYTE, data);

        if (outWidth != NULL) {
                *outWidth = width;
        }
        if (outHeight != NULL) {
                *outHeight = height;
        }
        
        stbi_image_free(data);
}

void texture_load(struct texture *const tex) {
        if (assignLoadedTexture(tex)) {
                texture_bind(tex);
                return;
        }
        
        glGenTextures(1, &tex->idx);
        glActiveTexture(tex->slot);
        glBindTexture(tex->type, tex->idx);
        
        if (tex->type == GL_TEXTURE_2D) {
                char *filepath = buildpathTex(tex->name, ".png");
                loadImageIntoGl(filepath, tex->type, true,
                                &tex->width, &tex->height);
                free(filepath);
                
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_S,
                                GL_REPEAT);
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_T,
                                GL_REPEAT);
                glTexParameteri(tex->type, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
                glTexParameteri(tex->type, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
                glGenerateMipmap(tex->type);
        } else if (tex->type == GL_TEXTURE_CUBE_MAP) {
                char *filepath;
                filepath = buildpathTex(tex->name, "_right.png");
                loadImageIntoGl(filepath,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X, false,
                                &tex->width, &tex->height);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_left.png");
                loadImageIntoGl(filepath,
                                GL_TEXTURE_CUBE_MAP_NEGATIVE_X, false,
                                NULL, NULL);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_top.png");
                loadImageIntoGl(filepath,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_Y, false,
                                NULL, NULL);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_bottom.png");
                loadImageIntoGl(filepath,
                                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, false,
                                NULL, NULL);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_front.png");
                loadImageIntoGl(filepath,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_Z, false,
                                NULL, NULL);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_back.png");
                loadImageIntoGl(filepath,
                                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, false,
                                NULL, NULL);
                free(filepath);
                
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_R,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(tex->type, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
                glTexParameteri(tex->type, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
        }
        
        tex->loaded = true;
        setLoadedTexture(tex);
}

void texture_bind(const struct texture *const tex) {
        if (tex->loaded) {
                glActiveTexture(tex->slot);
                glBindTexture(tex->type, tex->idx);
        }
}

void texture_free(struct texture *const tex) {
        if (tex->loaded) {
                unsetLoadedTexture(tex);
                glDeleteTextures(1, &tex->idx);
                free(tex->name);
                tex->loaded = false;
        }
}
