#include <thirty/texture.h>
#include <thirty/util.h>

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static char *buildpathTex(const char *const file, const char *const ext) {
        char *path = pathjoin_dyn(2, "textures", file);
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
        if (name == NULL) {
                tex->name = NULL;
        } else {
                tex->name = sstrdup(name);
        }
}

__attribute__((access (read_only, 1)))
__attribute__((nonnull (1)))
static void loadImageIntoGl(const char *const filename,
                            const GLenum type, const bool flip) {
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
        
        stbi_image_free(data);
}

void texture_load(struct texture *const tex) {
        glGenTextures(1, &tex->idx);
        glActiveTexture(tex->slot);
        glBindTexture(tex->type, tex->idx);
        
        if (tex->type == GL_TEXTURE_2D) {
                char *filepath = buildpathTex(tex->name, ".png");
                loadImageIntoGl(filepath, tex->type, true);
                free(filepath);
                
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(tex->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(tex->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glGenerateMipmap(tex->type);
        } else if (tex->type == GL_TEXTURE_CUBE_MAP) {
                char *filepath;
                filepath = buildpathTex(tex->name, "_right.png");
                loadImageIntoGl(filepath, GL_TEXTURE_CUBE_MAP_POSITIVE_X, false);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_left.png");
                loadImageIntoGl(filepath, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, false);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_top.png");
                loadImageIntoGl(filepath, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, false);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_bottom.png");
                loadImageIntoGl(filepath, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, false);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_front.png");
                loadImageIntoGl(filepath, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, false);
                free(filepath);
                
                filepath = buildpathTex(tex->name, "_back.png");
                loadImageIntoGl(filepath, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, false);
                free(filepath);
                
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(tex->type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTexParameteri(tex->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(tex->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        
        tex->loaded = true;
}

void texture_bind(const struct texture *const tex) {
        if (tex->loaded) {
                glActiveTexture(tex->slot);
                glBindTexture(tex->type, tex->idx);
        }
}

void texture_free(struct texture *const tex) {
        if (tex->loaded) {
                glDeleteTextures(1, &tex->idx);
                tex->loaded = false;
        }
        free(tex->name);
}
