#include <thirty/material.h>
#include <thirty/util.h>

#define getTextureInfo(cnst, material, tex, textureType)                \
        do {                                                            \
                if ((material)->base.type == COMPONENT_MATERIAL_UBER) { \
                        if ((textureType) != NULL) {                    \
                                *(textureType) = GL_TEXTURE_2D;         \
                        }                                               \
                                                                        \
                        cnst struct material_uber *mat =                \
                                (cnst struct material_uber*)(material); \
                        switch (tex) {                                  \
                        case MATERIAL_TEXTURE_AMBIENT:                  \
                                return &mat->ambientTexture;            \
                        case MATERIAL_TEXTURE_EMISSIVE:                 \
                                return &mat->emissiveTexture;           \
                        case MATERIAL_TEXTURE_DIFFUSE:                  \
                                return &mat->diffuseTexture;            \
                        case MATERIAL_TEXTURE_SPECULAR:                 \
                                return &mat->specularTexture;           \
                        case MATERIAL_TEXTURE_SPECULARPOWER:            \
                                return &mat->specularPowerTexture;      \
                        case MATERIAL_TEXTURE_NORMAL:                   \
                                return &mat->normalTexture;             \
                        case MATERIAL_TEXTURE_BUMP:                     \
                                return &mat->bumpTexture;               \
                        case MATERIAL_TEXTURE_OPACITY:                  \
                                return &mat->opacityTexture;            \
                                                                        \
                        case MATERIAL_TEXTURE_ENVIRONMENT:              \
                        case MATERIAL_TEXTURE_TOTAL:                    \
                        default:                                        \
                                return NULL;                            \
                        }                                               \
                } else if ((material)->base.type ==                     \
                           COMPONENT_MATERIAL_SKYBOX) {                 \
                        if ((textureType) != NULL) {                    \
                                *(textureType) = GL_TEXTURE_CUBE_MAP;   \
                        }                                               \
                                                                        \
                        cnst struct material_skybox *mat =              \
                                (cnst struct material_skybox*)(material); \
                        switch (tex) {                                  \
                        case MATERIAL_TEXTURE_ENVIRONMENT:              \
                                return &mat->skybox;                    \
                                                                        \
                        case MATERIAL_TEXTURE_AMBIENT:                  \
                        case MATERIAL_TEXTURE_EMISSIVE:                 \
                        case MATERIAL_TEXTURE_DIFFUSE:                  \
                        case MATERIAL_TEXTURE_SPECULAR:                 \
                        case MATERIAL_TEXTURE_SPECULARPOWER:            \
                        case MATERIAL_TEXTURE_NORMAL:                   \
                        case MATERIAL_TEXTURE_BUMP:                     \
                        case MATERIAL_TEXTURE_OPACITY:                  \
                        case MATERIAL_TEXTURE_TOTAL:                    \
                        default:                                        \
                                return NULL;                            \
                        }                                               \
                }                                                       \
                                                                        \
                return NULL;                                            \
        } while (0)
                
__attribute__((access (read_only, 1)))
__attribute__((access (write_only, 3)))
__attribute__((nonnull (1)))
static const struct texture *getConstTextureInfo(
        const struct material *const material,
        const enum material_textureType tex,
        GLenum *const textureType) {
        getTextureInfo(const, material, tex, textureType);
}

__attribute__((access (read_only, 1)))
__attribute__((access (write_only, 3)))
__attribute__((nonnull (1)))
static struct texture *getVarTextureInfo(
        struct material *const material,
        const enum material_textureType tex,
        GLenum *const textureType) {
        getTextureInfo(, material, tex, textureType);
}

#undef getTextureInfo

void material_init(struct material *material, const char *const name,
                   const enum shaders shader, const enum componentType type) {
        assert(type == COMPONENT_MATERIAL_SKYBOX ||
               type == COMPONENT_MATERIAL_UBER);
        
        component_init((struct component*)material, name);
        material->base.type = type;
        material->shader = shader;

        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex < MATERIAL_TEXTURE_TOTAL; tex++) {
                struct texture *texture = getVarTextureInfo(
                        material, tex, NULL);
                if (texture != NULL) {
                        texture_init(texture, NULL, 0, 0);
                }
        }
}

size_t material_initFromFile(struct material *const material, FILE *const f,
                             const enum componentType type) {
        assert(type == COMPONENT_MATERIAL_SKYBOX ||
               type == COMPONENT_MATERIAL_UBER);
        
        uint8_t shader_type;
        sfread(&shader_type, sizeof(shader_type), 1, f);

        char *name = strfile(f);
        material_init(material, name, shader_type, type);
        free(name);
        
        if (type == COMPONENT_MATERIAL_UBER) {
                material_uber_initFromFile((struct material_uber*)material, f);
                return sizeof(struct material_uber);
        }
        
        bail("Error parsing scene: Invalid material: %u\n", type);
}

void material_setTexture(struct material *const material,
                         const enum material_textureType tex,
                         const char *const name) {
        assert(material->base.type == COMPONENT_MATERIAL_SKYBOX ||
               material->base.type == COMPONENT_MATERIAL_UBER);
        
        GLenum textureSlot = GL_TEXTURE0 + tex;
        GLenum textureType;
        struct texture *texture = getVarTextureInfo(material, tex,
                                                    &textureType);
        if (texture == NULL) {
                return;
        }

        if (texture->loaded) {
                texture_free(texture);
        }

        texture_init(texture, name, textureSlot, textureType);
        texture_load(texture);
}

void material_unsetTexture(struct material *const material,
                           const enum material_textureType tex) {
        assert(material->base.type == COMPONENT_MATERIAL_SKYBOX ||
               material->base.type == COMPONENT_MATERIAL_UBER);
        
        struct texture *texture = getVarTextureInfo(material, tex, NULL);
        if (texture != NULL) {
                texture_free(texture);
        }
}

bool material_isTransparent(const struct material *const material) {
        assert(material->base.type == COMPONENT_MATERIAL_SKYBOX ||
               material->base.type == COMPONENT_MATERIAL_UBER);
        
        if (material->base.type == COMPONENT_MATERIAL_UBER) {
                return ((const struct material_uber*)material)->
                        alphaBlendingMode;
        }
        return false;
}

void material_updateShader(const struct material *const material) {
        assert(material->base.type == COMPONENT_MATERIAL_SKYBOX ||
               material->base.type == COMPONENT_MATERIAL_UBER);
        
        if (material->base.type == COMPONENT_MATERIAL_UBER) {
                const struct material_uber *const mat =
                        (const struct material_uber*)material;
                shader_setVec4(material->shader, "material.ambientColor",
                               mat->ambientColor);
                shader_setVec4(material->shader, "material.emissiveColor",
                               mat->emissiveColor);
                shader_setVec4(material->shader, "material.diffuseColor",
                               mat->diffuseColor);
                shader_setVec4(material->shader, "material.specularColor",
                               mat->specularColor);
        
                shader_setFloat(material->shader, "material.opacity",
                                mat->opacity);
                shader_setFloat(material->shader, "material.specularPower",
                                mat->specularPower);
                shader_setFloat(material->shader, "material.reflectance",
                               mat->reflectance);
                shader_setFloat(material->shader, "material.refraction",
                               mat->refraction);
                shader_setFloat(material->shader, "material.indexOfRefraction",
                                mat->indexOfRefraction);

                shader_setBool(material->shader, "material.hasAmbientTexture",
                               mat->ambientTexture.loaded);
                shader_setBool(material->shader, "material.hasEmissiveTexture",
                               mat->emissiveTexture.loaded);
                shader_setBool(material->shader, "material.hasDiffuseTexture",
                               mat->diffuseTexture.loaded);
                shader_setBool(material->shader, "material.hasSpecularTexture",
                               mat->specularTexture.loaded);
                shader_setBool(material->shader, "material.hasNormalTexture",
                               mat->normalTexture.loaded);
                shader_setBool(material->shader, "material.hasBumpTexture",
                               mat->bumpTexture.loaded);
                shader_setBool(material->shader, "material.hasOpacityTexture",
                               mat->opacityTexture.loaded);
        
                shader_setFloat(material->shader, "material.bumpIntensity",
                                mat->bumpIntensity);
                shader_setFloat(material->shader, "material.specularScale",
                                mat->specularScale);
                shader_setFloat(material->shader, "material.alphaThreshold",
                                mat->alphaThreshold);
                shader_setBool(material->shader, "material.alphaBlendingMode",
                               mat->alphaBlendingMode);
        }
}

void material_bindTextures(const struct material *const material) {
        assert(material->base.type == COMPONENT_MATERIAL_SKYBOX ||
               material->base.type == COMPONENT_MATERIAL_UBER);
        
        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex < MATERIAL_TEXTURE_TOTAL; tex++) {
                const struct texture *texture = getConstTextureInfo(
                        material, tex, NULL);
                if (texture != NULL) {
                        texture_bind(texture);
                }
        }
}

void material_free(struct material *const material) {
        assert(material->base.type == COMPONENT_MATERIAL_SKYBOX ||
               material->base.type == COMPONENT_MATERIAL_UBER);
        
        component_free((struct component*)material);
        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex < MATERIAL_TEXTURE_TOTAL; tex++) {
                material_unsetTexture(material, tex);
        }
}

__attribute__((access (write_only, 1)))
__attribute__((nonnull))
static void uberInitTexturesEmpty(struct material_uber *const material) {
        material->ambientTexture.loaded = false;
        material->emissiveTexture.loaded = false;
        material->diffuseTexture.loaded = false;
        material->specularTexture.loaded = false;
        material->specularPowerTexture.loaded = false;
        material->normalTexture.loaded = false;
        material->bumpTexture.loaded = false;
        material->opacityTexture.loaded = false;
}

void material_uber_initDefaults(struct material_uber *const material,
                                const char *const name,
                                const enum shaders shader) {
        material_init(&material->base, name, shader, COMPONENT_MATERIAL_UBER);

        static const vec4s white = GLMS_VEC4_ONE_INIT;
        static const vec4s black = GLMS_VEC4_BLACK_INIT;
        material->ambientColor = black;
        material->emissiveColor = black;
        material->diffuseColor = white;
        material->specularColor = black;

        material->opacity = 1.0F;
        material->specularPower = 100.0F;
        material->reflectance = 0.0F;
        material->refraction = 0.0F;
        material->indexOfRefraction = 1.0F;

        material->bumpIntensity = 1.0F;
        material->specularScale = 1.0F;
        material->alphaThreshold = 1.0F;
        material->alphaBlendingMode = false;

        uberInitTexturesEmpty(material);
}

void material_uber_initFromFile(struct material_uber *const material,
                                FILE *const f) {
        assert(material->base.base.type == COMPONENT_MATERIAL_UBER);
        
        sfread(material->ambientColor.raw, sizeof(float), 4, f);
        sfread(material->emissiveColor.raw, sizeof(float), 4, f);
        sfread(material->diffuseColor.raw, sizeof(float), 4, f);
        sfread(material->specularColor.raw, sizeof(float), 4, f);
        
        sfread(&material->opacity, sizeof(float), 1, f);
        sfread(&material->specularPower, sizeof(float), 1, f);
        sfread(&material->reflectance, sizeof(float), 1, f);
        sfread(&material->refraction, sizeof(float), 1, f);
        sfread(&material->indexOfRefraction, sizeof(float), 1, f);
        
        sfread(&material->bumpIntensity, sizeof(float), 1, f);
        sfread(&material->specularScale, sizeof(float), 1, f);
        sfread(&material->alphaThreshold, sizeof(float), 1, f);

        uint8_t alphaBlendingMode;
        sfread(&alphaBlendingMode, sizeof(alphaBlendingMode), 1, f);
        material->alphaBlendingMode = alphaBlendingMode;
        
        uberInitTexturesEmpty(material);

        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex < MATERIAL_TEXTURE_TOTAL; tex++) {
                if (getConstTextureInfo(&material->base, tex, NULL) == NULL) {
                        continue;
                }
                
                uint32_t nchars;
                sfread(&nchars, sizeof(nchars), 1, f);
                if (nchars > 0) {
                        char *const name =
                                smallocarray(nchars+1, sizeof(*name));
                        sfread(name, nchars, sizeof(*name), f);
                        name[nchars] = '\0';
                        material_setTexture((struct material*)material,
                                            tex, name);
                        free(name);
                }
        }
}

void material_skybox_init(struct material_skybox *const material,
                          const char *name, const enum shaders shader) {
        material_init(&material->base, name, shader,
                      COMPONENT_MATERIAL_SKYBOX);
        material->skybox.loaded = false;
}

void material_skybox_initFromName(struct material_skybox *const material,
                                  const char *const name) {
        material_skybox_init(material, name, SHADER_SKYBOX);
        material_setTexture((struct material *)material,
                            MATERIAL_TEXTURE_ENVIRONMENT, name);
}
