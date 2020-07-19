#include <material.h>
#include <util.h>
#include <shader.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <cglm/struct.h>
#include <stdbool.h>
#include <string.h>

__attribute__((access (write_only, 1)))
__attribute__((nonnull))
static void initTexturesZero(struct material *const material) {
        material->ambientTexture = 0;
        material->emissiveTexture = 0;
        material->diffuseTexture = 0;
        material->specularTexture = 0;
        material->specularPowerTexture = 0;
        material->normalTexture = 0;
        material->bumpTexture = 0;
        material->opacityTexture = 0;
}

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

        material->bumpIntensity = 1.0F;
        material->specularScale = 1.0F;
        material->alphaThreshold = 1.0F;

        initTexturesZero(material);
}

void material_initFromFile(struct material *const material, FILE *const f) {
        uint32_t mat_type;
        sfread(&mat_type, sizeof(mat_type), 1, f);
        if (mat_type != 0) {
                bail("Error parsing scene: Nonzero material.");
        }

        uint32_t shader_type;
        sfread(&shader_type, sizeof(shader_type), 1, f);
        if (shader_type != SHADER_UBER) {
                bail("Error parsing scene: Unknown shader.");
        }
        material->shader = SHADER_UBER;

        sfread(material->ambientColor.raw, sizeof(float), 4, f);
        sfread(material->emissiveColor.raw, sizeof(float), 4, f);
        sfread(material->diffuseColor.raw, sizeof(float), 4, f);
        sfread(material->specularColor.raw, sizeof(float), 4, f);
        sfread(material->reflectance.raw, sizeof(float), 4, f);
        
        sfread(&material->opacity, sizeof(float), 1, f);
        sfread(&material->specularPower, sizeof(float), 1, f);
        sfread(&material->indexOfRefraction, sizeof(float), 1, f);
        
        sfread(&material->bumpIntensity, sizeof(float), 1, f);
        sfread(&material->specularScale, sizeof(float), 1, f);
        sfread(&material->alphaThreshold, sizeof(float), 1, f);
        
        initTexturesZero(material);

        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex <= MATERIAL_TEXTURE_OPACITY; tex++) {
                uint32_t nchars;
                sfread(&nchars, sizeof(nchars), 1, f);
                if (nchars > 0) {
                        char *const name =
                                smallocarray(nchars+1, sizeof(*name));
                        sfread(name, nchars, sizeof(*name), f);
                        name[nchars] = '\0';
                        material_setTexture(material, tex, name);
                        free(name);
                }
        }
}

#define TEXANDSLOT(ptr)                                                 \
        do {                                                            \
                switch (tex) {                                          \
                case MATERIAL_TEXTURE_AMBIENT:                          \
                        *gltexture = GL_TEXTURE0;                       \
                        return ptr material->ambientTexture;            \
                case MATERIAL_TEXTURE_EMISSIVE:                         \
                        *gltexture = GL_TEXTURE1;                       \
                        return ptr material->emissiveTexture;           \
                case MATERIAL_TEXTURE_DIFFUSE:                          \
                        *gltexture = GL_TEXTURE2;                       \
                        return ptr material->diffuseTexture;            \
                case MATERIAL_TEXTURE_SPECULAR:                         \
                        *gltexture = GL_TEXTURE3;                       \
                        return ptr material->specularTexture;           \
                case MATERIAL_TEXTURE_SPECULARPOWER:                    \
                        *gltexture = GL_TEXTURE4;                       \
                        return ptr material->specularPowerTexture;      \
                case MATERIAL_TEXTURE_NORMAL:                           \
                        *gltexture = GL_TEXTURE5;                       \
                        return ptr material->normalTexture;             \
                case MATERIAL_TEXTURE_BUMP:                             \
                        *gltexture = GL_TEXTURE6;                       \
                        return ptr material->bumpTexture;               \
                case MATERIAL_TEXTURE_OPACITY:                          \
                        *gltexture = GL_TEXTURE7;                       \
                        return ptr material->opacityTexture;            \
                default:                                                \
                        assert_fail();                                  \
                        break;                                          \
                }                                                       \
        } while (0)
__attribute__((access (read_only, 1)))
__attribute__((access (write_only, 3)))
__attribute__((nonnull))
static unsigned *getTexPtrAndSlot(
        struct material *const material,
        const enum material_textureType tex,
        GLenum *const gltexture) {
        TEXANDSLOT(&);
        return NULL;
}
__attribute__((access (read_only, 1)))
__attribute__((access (write_only, 3)))
__attribute__((nonnull))
static unsigned getTexAndSlot(const struct material *const material,
                                 const enum material_textureType tex,
                                 GLenum *const gltexture) {
        TEXANDSLOT();
        return 0;
}
#undef TEXANDSLOT

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
        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex <= MATERIAL_TEXTURE_OPACITY; tex++) {
                GLenum gltexture;
                unsigned texture = getTexAndSlot(material, tex, &gltexture);
                if (texture != 0) {
                        glActiveTexture(gltexture);
                        glBindTexture(GL_TEXTURE_2D, texture);
                }
        }
}

void material_free(struct material *const material) {
        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex <= MATERIAL_TEXTURE_OPACITY; tex++) {
                material_unsetTexture(material, tex);
        }
}
