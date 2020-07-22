#ifndef MATERIAL_H
#define MATERIAL_H

#include <shader.h>
#include <cglm/struct.h>
#include <stdbool.h>

/*
 * Represents a material. For now only one shader is used. In the future there
 * should be a generic material and then many materials that have different
 * fields depending on the shader.
 *
 * The material simply encapsulates all the lighting and color and texture
 * parameters of an object.
 */

struct material {
        enum shaders shader;
        
        vec4s ambientColor;
        vec4s emissiveColor;
        vec4s diffuseColor;
        vec4s specularColor;
        vec4s reflectance; // Unimplemented

        float opacity;
        float specularPower;
        float indexOfRefraction; // Unimplemented

        unsigned ambientTexture;
        unsigned emissiveTexture;
        unsigned diffuseTexture;
        unsigned specularTexture;
        unsigned specularPowerTexture;
        unsigned normalTexture;
        unsigned bumpTexture;
        unsigned opacityTexture;

        float bumpIntensity;
        float specularScale;
        float alphaThreshold;
        bool alphaBlendingMode;
};

enum material_textureType {
        MATERIAL_TEXTURE_AMBIENT,
        MATERIAL_TEXTURE_EMISSIVE,
        MATERIAL_TEXTURE_DIFFUSE,
        MATERIAL_TEXTURE_SPECULAR,
        MATERIAL_TEXTURE_SPECULARPOWER,
        MATERIAL_TEXTURE_NORMAL,
        MATERIAL_TEXTURE_BUMP,
        MATERIAL_TEXTURE_OPACITY,
};

/*
 * Initialize a material with default values: All colors black and no
 * associated textures. Must pass in the shader.
 */
void material_initDefaults(struct material *material,
                           enum shaders shader)
        __attribute__((access (write_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void material_initFromFile(struct material *material, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Associate a texture with the material. The texture can be one of many, as
 * defined in the material_textureType enum. If a texture was already set in
 * that slot, it is replaced.
 */
void material_setTexture(struct material *material,
                         enum material_textureType tex,
                         const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 3)))
        __attribute__((leaf))
        __attribute__((nonnull (1)));

/*
 * Unassociate a texture with the material, freeing any resources along the
 * way. This is already called automatically by setTexture if it's replacing a
 * texture.
 */
void material_unsetTexture(struct material *material,
                           enum material_textureType tex)
        __attribute__((access (write_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Return whether the material contains any transparencies.
 */
bool material_isTransparent(const struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Update the associated shader's parameters with the material. This should be
 * called after any modification of the material, including associating
 * textures or calling the init function, so that the changes can reflect on
 * the shader (can also make multiple changes and then call this function).
 */
void material_updateShader(const struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Bind the material's textures to the shader previous to drawing.
 */
void material_bindTextures(const struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Free any resources used by the material, specifically all textures.
 */
void material_free(struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* MATERIAL_H */
