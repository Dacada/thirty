#include <thirty/texture.h>
#include <thirty/util.h>

void texture_init(struct texture *const tex, const GLenum slot, const GLenum type) {
        tex->loaded = false;
        tex->slot = slot;
        tex->type = type;
}

__attribute__((access (read_only, 1)))
__attribute__((nonnull (1)))
static void decodeImageIntoGl(void *buffer, int size, GLenum type, bool flip) {
        stbi_set_flip_vertically_on_load(flip);

        int width;
        int height;
        int nrChannels;
        unsigned char *const data __attribute__ ((nonstring)) =
                stbi_load_from_memory(buffer, size, &width, &height, &nrChannels, 0);
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

static inline void genGLtexture(struct texture *const tex) {
        glGenTextures(1, &tex->idx);
        glActiveTexture(tex->slot);
        glBindTexture(tex->type, tex->idx);
}
static inline void setGLtextureParams(struct texture *const tex) {
        glTexParameteri(tex->type, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(tex->type, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(tex->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(tex->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void texture_load(struct texture *const tex, void *buf, size_t size) {
        assert(tex->type == GL_TEXTURE_2D);
        genGLtexture(tex);

        assert(size <= INT_MAX);
        decodeImageIntoGl(buf, (int)size, tex->type, true);

        setGLtextureParams(tex);
        glGenerateMipmap(tex->type);

        tex->loaded = true;
}

void texture_loadCubeMap(struct texture *const tex, void *buf[6], size_t sizes[6]) {
        assert(tex->type == GL_TEXTURE_CUBE_MAP);
        genGLtexture(tex);

        for (int i=0; i<6; i++) {
                assert(sizes[i] <= INT_MAX);
        }

        decodeImageIntoGl(buf[0], (int)sizes[0], GL_TEXTURE_CUBE_MAP_POSITIVE_X, false);
        decodeImageIntoGl(buf[1], (int)sizes[1], GL_TEXTURE_CUBE_MAP_NEGATIVE_X, false);
        decodeImageIntoGl(buf[2], (int)sizes[2], GL_TEXTURE_CUBE_MAP_POSITIVE_Y, false);
        decodeImageIntoGl(buf[3], (int)sizes[3], GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, false);
        decodeImageIntoGl(buf[4], (int)sizes[4], GL_TEXTURE_CUBE_MAP_POSITIVE_Z, false);
        decodeImageIntoGl(buf[5], (int)sizes[5], GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, false);

        setGLtextureParams(tex);
        glTexParameteri(tex->type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
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
}
