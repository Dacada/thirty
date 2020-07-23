#include <texture.h>
#include <util.h>
#include <stb_image.h>
#include <string.h>

__attribute__((access (write_only, 2, 1)))
__attribute__((access (read_only, 3)))
__attribute__((nonnull))
static void buildpathTex(const size_t destsize, char *const dest,
                         const char *const file) {
        const size_t len = pathjoin(destsize, dest, 3, ASSETSPATH,
                                     "textures", file);
        if (len + 3 - 1 >= destsize) {
                die("Path to texture file too long.\n");
        }
        strcpy(dest+len-2, ".png");
}

void texture_init(struct texture *tex, const char *name, GLenum slot) {
        tex->loaded = false;
        buildpathTex(PATH_MAX, tex->filepath, name);
        if (!accessible(tex->filepath, true, false, false)) {
                bail("Can't read texture file.\n");
        }
        tex->slot = slot;
}

void texture_load(struct texture *tex) {
        stbi_set_flip_vertically_on_load(true);

        glGenTextures(1, &tex->idx);
        glActiveTexture(tex->slot);
        glBindTexture(GL_TEXTURE_2D, tex->idx);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width;
        int height;
        int nrChannels;
        unsigned char *const data __attribute__ ((nonstring)) =
                stbi_load(tex->filepath, &width, &height, &nrChannels, 0);
        if (data == NULL) {
                bail("Can't read texture image data.\n");
        }
        
        GLenum internalFormat;
        if (nrChannels == 3) { // PNG without transparency data
                internalFormat = GL_RGB;
        } else if (nrChannels == 4) { // PNG with transparency data
                internalFormat =  GL_RGBA;
        } else { // Abomination
                die("Failing to load png texture. I expected 3 or 4 "
                    "channels but this thing has %d?\n", nrChannels);
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                     internalFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(data);
        tex->loaded = true;
}

void texture_bind(const struct texture *tex) {
        if (tex->loaded) {
                glActiveTexture(tex->slot);
                glBindTexture(GL_TEXTURE_2D, tex->idx);
        }
}

void texture_free(struct texture *tex) {
        if (tex->loaded) {
                glDeleteTextures(1, &tex->idx);
                tex->loaded = false;
        }
}
