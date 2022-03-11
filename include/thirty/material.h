#ifndef MATERIAL_H
#define MATERIAL_H

#include <thirty/shader.h>
#include <thirty/component.h>
#include <thirty/texture.h>
#include <thirty/asyncLoader.h>
#include <thirty/dsutils.h>

/*
 * Represents a material. There are two kinds of materials. Uber and
 * Skybox.
 */

/*
 * Ambient, Emissive, Diffuse, Specular, SpecularPower, Normal, Bump and
 * Opacity textures are used by the Uber material only. Environment textures
 * are used for Skybox material only, but are also used for environment
 * mapping (or will be).
 */
enum material_textureType {
        MATERIAL_TEXTURE_AMBIENT,
        MATERIAL_TEXTURE_EMISSIVE,
        MATERIAL_TEXTURE_DIFFUSE,
        MATERIAL_TEXTURE_SPECULAR,
        MATERIAL_TEXTURE_SPECULARPOWER,
        MATERIAL_TEXTURE_NORMAL,
        MATERIAL_TEXTURE_BUMP,
        MATERIAL_TEXTURE_OPACITY,
        
        MATERIAL_TEXTURE_ENVIRONMENT,

        MATERIAL_TEXTURE_TOTAL  // Useful only for loops
};

/*
 * The base material is used as a base of all materials and should not
 * be used directly, as it has no parameters and does nothing on its own.
 */
struct material {
        struct component base;
        enum shaders shader;
};

/*
 * Initialize the base of the material. Already called by the other material's
 * init functions.
 */
void material_init(struct material *material, const char *name,
                   enum shaders shader, enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/* 
 * Initialize material from a BOGLE file positioned at the correct offset.
 * File has the information about the material type so this assumes that the
 * pointer points to an area with enough space to fit any material type. Returns
 * the actual used size.
 */
size_t material_initFromFile(struct material *material, FILE *f,
                             enum componentType type, struct asyncLoader *loader,
                             struct varSizeGrowingArray *components)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((access (read_write, 4)))
        __attribute__((access (read_write, 5)))
        __attribute__((nonnull));

/*
 * Associate a texture with the material. The texture can be one of many, as
 * defined in the material_textureType enum. If a texture was already set in
 * that slot, it is replaced. Not all material types support all texture
 * types. Trying to associate a texture type with a material that doesn't
 * support it will be silently ignored.
 */
void material_setTexture(struct material *material,
                         enum material_textureType tex,
                         const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 3)))
        __attribute__((nonnull (1)));

/*
 * Unassociate a texture with the material, freeing any resources along the
 * way. This is already called automatically by setTexture if it's replacing a
 * texture. Unassociating a texture from a material that doesn't support it or
 * doesn't have any associated it silently ignored.
 */
void material_unsetTexture(struct material *material,
                           enum material_textureType tex)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Return whether the material is considered "transparent", meaning that it
 * should be rendered in the transparent pass.
 */
bool material_isTransparent(const struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Update the associated shader's parameters with the material.
 */
void material_updateShader(const struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Bind the material's textures to the shader.
 */
void material_bindTextures(const struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Free any resources used by the material, usually this means all textures.
 */
void material_free(struct material *material)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));


/*
 * The default material, with color, texture, lighting...
 */
struct material_uber {
        struct material base;
        
        vec4s ambientColor;
        vec4s emissiveColor;
        vec4s diffuseColor;
        vec4s specularColor;

        float opacity;
        float specularPower;
        float reflectance;
        float refraction;
        float indexOfRefraction;

        struct texture ambientTexture;
        struct texture emissiveTexture;
        struct texture diffuseTexture;
        struct texture specularTexture;
        struct texture specularPowerTexture;
        struct texture normalTexture;
        struct texture bumpTexture;
        struct texture opacityTexture;

        float bumpIntensity;
        float specularScale;
        float alphaThreshold;
        bool alphaBlendingMode;
};

/*
 * Initialize a material with default values: All colors black and no
 * associated textures. Must still pass in the shader.
 */
void material_uber_initDefaults(struct material_uber *material,
                                const char *name,
                                enum shaders shader)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Initialize the material from a Bogle file that already has been seeked to
 * the right offset.
 */
void material_uber_initFromFile(struct material_uber *material, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));


/*
 * This material is used for the skybox. As such it's very simple and only has
 * one texture.
 */
struct material_skybox {
        struct material base;
        struct texture skybox;
};

/*
 * Initialize the skybox material.
 */
void material_skybox_init(struct material_skybox *material,
                          const char *name,
                          enum shaders shader)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Initialize the skybox material from the base name of its texture.
 */
void material_skybox_initFromName(struct material_skybox *material,
                                  const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

#endif /* MATERIAL_H */
