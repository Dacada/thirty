#include <material.h>
#include <texture.h>
#include <shader.h>
#include <glad/glad.h>
#include <cglm/struct.h>
#include <stdbool.h>

__attribute__((access (write_only, 1)))
__attribute__((nonnull))
static void initTexturesEmpty(struct material *const material) {
        material->ambientTexture.loaded = false;
        material->emissiveTexture.loaded = false;
        material->diffuseTexture.loaded = false;
        material->specularTexture.loaded = false;
        material->specularPowerTexture.loaded = false;
        material->normalTexture.loaded = false;
        material->bumpTexture.loaded = false;
        material->opacityTexture.loaded = false;
}

void material_initDefaults(struct material *const material,
                           const enum shaders shader) {
        material->shader = shader;

        const vec4s black = GLMS_VEC4_BLACK_INIT;
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
        material->alphaBlendingMode = false;

        initTexturesEmpty(material);
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

        uint8_t alphaBlendingMode;
        sfread(&alphaBlendingMode, sizeof(alphaBlendingMode), 1, f);
        material->alphaBlendingMode = alphaBlendingMode;
        
        initTexturesEmpty(material);

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

__attribute__((access (read_only, 1)))
__attribute__((nonnull))
static struct texture *getTexPtrAndSlot(struct material *const material,
                                        const enum material_textureType tex) {
        switch (tex) {
        case MATERIAL_TEXTURE_AMBIENT:
                return &material->ambientTexture;
        case MATERIAL_TEXTURE_EMISSIVE:
                return &material->emissiveTexture;
        case MATERIAL_TEXTURE_DIFFUSE:
                return &material->diffuseTexture;
        case MATERIAL_TEXTURE_SPECULAR:
                return &material->specularTexture;
        case MATERIAL_TEXTURE_SPECULARPOWER:
                return &material->specularPowerTexture;
        case MATERIAL_TEXTURE_NORMAL:
                return &material->normalTexture;
        case MATERIAL_TEXTURE_BUMP:
                return &material->bumpTexture;
        case MATERIAL_TEXTURE_OPACITY:
                return &material->opacityTexture;
        default:
                assert_fail();
                return NULL;
        }
}

void material_setTexture(struct material *const material,
                         const enum material_textureType tex,
                         const char *const name) {
        GLenum gltexture = GL_TEXTURE0 + tex;
        struct texture *texture = getTexPtrAndSlot(material, tex);

        if (texture->loaded) {
                texture_free(texture);
        }

        texture_init(texture, name, gltexture);
        texture_load(texture);
}

void material_unsetTexture(struct material *const material,
                           const enum material_textureType tex) {
        struct texture *texture = getTexPtrAndSlot(material, tex);
        texture_free(texture);
}

bool material_isTransparent(const struct material *const material) {
        return material->alphaBlendingMode;
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
                       material->ambientTexture.loaded);
        shader_setBool(material->shader, "material.hasEmissiveTexture",
                       material->emissiveTexture.loaded);
        shader_setBool(material->shader, "material.hasDiffuseTexture",
                       material->diffuseTexture.loaded);
        shader_setBool(material->shader, "material.hasSpecularTexture",
                       material->specularTexture.loaded);
        shader_setBool(material->shader, "material.hasNormalTexture",
                       material->normalTexture.loaded);
        shader_setBool(material->shader, "material.hasBumpTexture",
                       material->bumpTexture.loaded);
        shader_setBool(material->shader, "material.hasOpacityTexture",
                       material->opacityTexture.loaded);
        
        shader_setFloat(material->shader, "material.bumpIntensity",
                        material->bumpIntensity);
        shader_setFloat(material->shader, "material.specularScale",
                        material->specularScale);
        shader_setFloat(material->shader, "material.alphaThreshold",
                        material->alphaThreshold);
        shader_setBool(material->shader, "material.alphaBlendingMode",
                       material->alphaBlendingMode);
}

void material_bindTextures(struct material *const material) {
        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex <= MATERIAL_TEXTURE_OPACITY; tex++) {
                struct texture *texture = getTexPtrAndSlot(material, tex);
                texture_bind(texture);
        }
}

void material_free(struct material *const material) {
        for (enum material_textureType tex = MATERIAL_TEXTURE_AMBIENT;
             tex <= MATERIAL_TEXTURE_OPACITY; tex++) {
                material_unsetTexture(material, tex);
        }
}
