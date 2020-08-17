#include <object.h>
#include <componentCollection.h>
#include <geometry.h>
#include <material.h>
#include <animationCollection.h>
#include <shader.h>
#include <dsutils.h>
#include <util.h>
#include <cglm/struct.h>
#include <stdint.h>
#include <string.h>

void object_initEmpty(struct object *const object, struct scene *const scene,
                      const char *const name) {
        object->name = sstrdup(name);
        object->scene = scene;
        growingArray_init(&object->children, sizeof(size_t), 1);
        object->model = GLMS_MAT4_IDENTITY;
        componentCollection_init(&object->components);
}

static inline void assign_idx(struct componentCollection *const collection,
                              const enum componentType component,
                              const unsigned offset,
                              FILE *const f) {
        uint32_t idx;
        sfread(&idx, sizeof(idx), 1, f);
        if (idx != 0) {
                componentCollection_set(
                        collection, component, offset + idx - 1);
        }
}

void object_initFromFile(struct object *const object,
                         struct scene *const scene, 
                         const unsigned ncams, const unsigned ngeos,
                         const unsigned nmats, const unsigned nlights,
                         const unsigned nanims,
                         FILE *const f) {
        char *name = strfile(f);
        object_initEmpty(object, scene, name);
        free(name);

        unsigned offset = 0;
        assign_idx(&object->components, COMPONENT_CAMERA, offset, f);
        offset += ncams;
        assign_idx(&object->components, COMPONENT_GEOMETRY, offset, f);
        offset += ngeos;
        assign_idx(&object->components, COMPONENT_MATERIAL, offset, f);
        offset += nmats;
        assign_idx(&object->components, COMPONENT_LIGHT, offset, f);
        offset += nlights;
        assign_idx(&object->components,
                   COMPONENT_ANIMATIONCOLLECTION, offset, f);
        offset += nanims;

        mat4s model;
        sfread(model.raw, sizeof(float), sizeof(model) / sizeof(float), f);
        object->model = model;
}

void object_setSkybox(struct object *const skybox,
                       const size_t geo, const size_t mat) {
        componentCollection_set(
                &skybox->components, COMPONENT_GEOMETRY, geo);

        componentCollection_set(
                &skybox->components, COMPONENT_MATERIAL, mat);
}

void object_translate(struct object *const object, const vec3s delta) {
        object->model = glms_translate(object->model, delta);
}

void object_translateX(struct object *const object, const float delta) {
        object->model = glms_translate_x(object->model, delta);
}

void object_translateY(struct object *const object, const float delta) {
        object->model = glms_translate_y(object->model, delta);
}

void object_translateZ(struct object *const object, const float delta) {
        object->model = glms_translate_z(object->model, delta);
}

void object_rotate(struct object *const object, const float angle,
                   const vec3s axis) {
        object->model = glms_rotate(object->model, angle, axis);
}

void object_rotateX(struct object *object, float angle) {
        object->model = glms_rotate_x(object->model, angle);
}

void object_rotateY(struct object *object, float angle) {
        object->model = glms_rotate_y(object->model, angle);
}

void object_rotateZ(struct object *object, float angle) {
        object->model = glms_rotate_z(object->model, angle);
}

void object_rotateMat(struct object *object, mat4s rotation) {
        object->model = glms_mat4_mul(object->model, rotation);
}

void object_scale(struct object *const object, const vec3s scale) {
        object->model = glms_scale(object->model, scale);
}

void object_addChild(struct object *parent, struct object *child) {
        size_t *const child_idx_ptr = growingArray_append(&parent->children);
        *child_idx_ptr = child->idx;
        child->parent = parent->idx;
}

void object_update(struct object *const object) {
        componentCollection_update(&object->components);
}

bool object_draw(const struct object *const object, mat4s model,
                 mat4s view, const mat4s projection,
                 enum renderStage *const lastRenderStage,
                 const struct material **const lastMaterial,
                 enum shaders *const lastShader) {

        const struct componentCollection *const comps = &object->components;

        const struct geometry *const geometry = componentCollection_get(
                comps, COMPONENT_GEOMETRY);

        if (geometry == NULL) {
                // All objects without geometry are lumped at the end. If we
                // find one, we're done.
                return false;
        }

        const struct material *const material = componentCollection_get(
                comps, COMPONENT_MATERIAL);
        assert(material != NULL);

        // Sanity checks, if objects are well sorted this should always pass
        assert(*lastRenderStage != RENDER_SKYBOX);
        assert(*lastRenderStage != RENDER_TRANSPARENT_OBJECTS ||
               material_isTransparent(material) ||
               material->base.type == COMPONENT_MATERIAL_SKYBOX);

        if (material->base.type == COMPONENT_MATERIAL_SKYBOX) {
                *lastRenderStage = RENDER_SKYBOX;
                glDepthFunc(GL_LEQUAL);

                model.col[3] = (vec4s){{0, 0, 0, 1}};
                model.m03 = 0;
                model.m13 = 0;
                model.m23 = 0;
                model.m33 = 1;
                
                view.col[3] = (vec4s){{0, 0, 0, 1}};
                view.m03 = 0;
                view.m13 = 0;
                view.m23 = 0;
                view.m33 = 1;
        }

        const enum shaders shader = material->shader;
        if (shader != *lastShader) {
                shader_use(shader);
                *lastShader = shader;
                shader_setMat4(shader, "invView", glms_mat4_inv(view));
        }
        if (material != *lastMaterial) {
                material_updateShader(material);
                material_bindTextures(material);
                *lastMaterial = material;
        }

        struct animationCollection *anims = componentCollection_get(
                &object->components, COMPONENT_ANIMATIONCOLLECTION);
        if (anims != NULL) {
                animationCollection_bindBones(anims, shader);
        }

        if (*lastRenderStage == RENDER_OPAQUE_OBJECTS &&
            material_isTransparent(material)) {
                *lastRenderStage = RENDER_TRANSPARENT_OBJECTS;
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        const mat4s modelView = glms_mat4_mul(view, model);
        const mat4s modelViewProjection =
                glms_mat4_mul(projection, modelView);
        
        shader_setMat4(shader, "modelView", modelView);
        shader_setMat4(shader, "modelViewProjection", modelViewProjection);

        geometry_draw(geometry);

        return true;
}

void object_free(struct object *object) {
        free(object->name);
        growingArray_destroy(&object->children);
        componentCollection_free(&object->components);
}
