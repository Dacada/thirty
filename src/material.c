#include <material.h>
#include <util.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <string.h>

void material_initDefaults(struct material *const material,
                           const enum shaders shader) {
        material->shader = shader;

        vec4s black = GLMS_VEC4_BLACK_INIT;
        material->ambientColor = black;
        material->emissiveColor = black;
        material->diffuseColor = black;
        material->specularColor = black;
        material->reflectance = black;

        material->opacity = 1.0F;
        material->specularPower = 100.0F;
        material->indexOfRefraction = 0.0F;

        material->ambientTexture = 0;
        material->emissiveTexture = 0;
        material->diffuseTexture = 0;
        material->specularTexture = 0;
        material->specularPowerTexture = 0;
        material->normalTexture = 0;
        material->bumpTexture = 0;
        material->opacityTexture = 0;

        material->bumpIntensity = 1.0F;
        material->specularScale = 1.0F;
        material->alphaThreshold = 0.95F;
}

static unsigned *getTexPtrAndSlot(struct material *const material,
                                  const enum material_textureType tex,
                                  GLenum *const gltexture) {
        switch (tex) {
        case MATERIAL_TEXTURE_AMBIENT:
                *gltexture = GL_TEXTURE0;
                return &material->ambientTexture;
        case MATERIAL_TEXTURE_EMISSIVE:
                *gltexture = GL_TEXTURE1;
                return &material->emissiveTexture;
        case MATERIAL_TEXTURE_DIFFUSE:
                *gltexture = GL_TEXTURE2;
                return &material->diffuseTexture;
        case MATERIAL_TEXTURE_SPECULAR:
                *gltexture = GL_TEXTURE3;
                return &material->specularTexture;
        case MATERIAL_TEXTURE_SPECULARPOWER:
                *gltexture = GL_TEXTURE4;
                return &material->specularPowerTexture;
        case MATERIAL_TEXTURE_NORMAL:
                *gltexture = GL_TEXTURE5;
                return &material->normalTexture;
        case MATERIAL_TEXTURE_BUMP:
                *gltexture = GL_TEXTURE6;
                return &material->bumpTexture;
        case MATERIAL_TEXTURE_OPACITY:
                *gltexture = GL_TEXTURE7;
                return &material->opacityTexture;
        }
}

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

void material_setTexture(struct material *const material,
                         const enum material_textureType tex,
                         const char *const name) {
        stbi_set_flip_vertically_on_load(true);

        GLenum gltexture;
        unsigned *texture = getTexPtrAndSlot(material, tex, &gltexture);

        if (*texture != 0) {
                material_unsetTexture(material, tex);
        }

        glGenTextures(1, texture);
        glActiveTexture(gltexture);
        glBindTexture(GL_TEXTURE_2D, *texture);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        static char path[PATH_MAX];
        buildpathTex(PATH_MAX, path, name);
        if (!accessible(path, true, false, false)) {
                bail("Can't read texture file.\n");
        }

        int width;
        int height;
        int nrChannels;
        unsigned char *const data __attribute__ ((nonstring)) =
                stbi_load(path, &width, &height, &nrChannels, 0);
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
}

void material_unsetTexture(struct material *const material,
                           const enum material_textureType tex) {
        GLenum tmp;
        unsigned *texture = getTexPtrAndSlot(material, tex, &tmp);
        if (*texture != 0) {
                glDeleteTextures(1, texture);
                *texture = 0;
        }
}

bool material_isTransparent(const struct material *const material) {
        return material->opacity < 1.0F || material->opacityTexture != 0;
}

void material_updateShader(const struct material *const material) {
        shader_use(material->shader);
        
        shader_setVec4(material->shader, "material.ambientColor",
                       material->ambientColor);
        shader_setVec4(material->shader, "material.emissiveColor",
                       material->emissiveColor);
        shader_setVec4(material->shader, "material.diffuseColor",
                       material->diffuseColor);
        shader_setVec4(material->shader, "material.specularColor",
                       material->specularColor);
        shader_setVec4(material->shader, "material.reflectance",
                       material->reflectance);
        
        shader_setFloat(material->shader, "material.opacity",
                        material->opacity);
        shader_setFloat(material->shader, "material.specularPower",
                        material->specularPower);
        shader_setFloat(material->shader, "material.indexOfRefraction",
                        material->indexOfRefraction);

        shader_setBool(material->shader, "material.hasAmbientTexture",
                       material->ambientTexture != 0);
        shader_setBool(material->shader, "material.hasEmissiveTexture",
                       material->emissiveTexture != 0);
        shader_setBool(material->shader, "material.hasDiffuseTexture",
                       material->diffuseTexture != 0);
        shader_setBool(material->shader, "material.hasSpecularTexture",
                       material->specularTexture != 0);
        shader_setBool(material->shader, "material.hasNormalTexture",
                       material->normalTexture != 0);
        shader_setBool(material->shader, "material.hasBumpTexture",
                       material->bumpTexture != 0);
        shader_setBool(material->shader, "material.hasOpacityTexture",
                       material->opacityTexture != 0);
        
        shader_setFloat(material->shader, "material.bumpIntensity",
                        material->bumpIntensity);
        shader_setFloat(material->shader, "material.specularScale",
                        material->specularScale);
        shader_setFloat(material->shader, "material.alphaThreshold",
                        material->alphaThreshold);
}

void material_bindTextures(const struct material *const material) {
        /* TODO: This goes in another place now
        for (unsigned i=0; i<geometry->ntextures; i++) {
                glActiveTexture(GL_TEXTURE0+i);
                glBindTexture(GL_TEXTURE_2D, geometry->textures[i]);
        }
        */
}
