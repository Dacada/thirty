#include <thirty/object.h>
#include <thirty/util.h>

void object_initEmpty(struct object *const object, struct game *const game,
                      const size_t scene, const char *const name) {
        object->game = game;
        object->name = sstrdup(name);
        object->scene = scene;
        growingArray_init(&object->children, sizeof(size_t), 1);
        componentCollection_init(&object->components);
        object->onUpdate = NULL;

        struct transform *trans = componentCollection_create(
                game, COMPONENT_TRANSFORM);
        object_setComponent(object, &trans->base);
        transform_init(trans, GLMS_MAT4_IDENTITY);
}

static inline void assign_idx(struct object *const object,
                              const enum componentType component,
                              const unsigned offset,
                              FILE *const f) {
        uint32_t idx;
        sfread(&idx, sizeof(idx), 1, f);
        if (idx != 0) {
                componentCollection_set(&object->components, object->idx,
                                        component, offset + idx - 1);
        }
}

void object_initFromFile(struct object *const object,
                         struct game *const game,
                         const size_t scene, 
                         const unsigned ncams, const unsigned ngeos,
                         const unsigned nmats, const unsigned nlights,
                         const unsigned nanims,
                         FILE *const f) {
        char *name = strfile(f);
        object_initEmpty(object, game, scene, name);
        free(name);

        unsigned offset = 0;
        assign_idx(object, COMPONENT_CAMERA, offset, f);
        offset += ncams;
        assign_idx(object, COMPONENT_GEOMETRY, offset, f);
        offset += ngeos;
        assign_idx(object, COMPONENT_MATERIAL, offset, f);
        offset += nmats;
        assign_idx(object, COMPONENT_LIGHT, offset, f);
        offset += nlights;
        assign_idx(object, COMPONENT_ANIMATIONCOLLECTION, offset, f);
        //offset += nanims;
        (void)nanims;

        mat4s model;
        sfread(model.raw, sizeof(float), sizeof(model) / sizeof(float), f);
        struct transform *trans = object_getComponent(
                object, COMPONENT_TRANSFORM);
        trans->model = model;
}

void object_addChild(struct object *parent, struct object *child) {
        size_t *const child_idx_ptr = growingArray_append(&parent->children);
        *child_idx_ptr = child->idx;
        child->parent = parent->idx;
}

void object_setComponent(struct object *object, struct component *comp) {
        componentCollection_set(&object->components, object->idx,
                                comp->type, comp->idx);
}

void *object_getComponent(const struct object *object,
                          enum componentType type) {
        return componentCollection_get(&object->components, type);
}

void object_update(struct object *const object, const float timeDelta) {
        if (object->onUpdate != NULL) {
                struct eventBrokerUpdate args = {
                        .timeDelta = timeDelta,
                };
                object->onUpdate(object, &args);
        }
        componentCollection_update(&object->components, timeDelta);
}

bool object_draw(const struct object *const object, mat4s model,
                 mat4s view, const mat4s projection,
                 enum renderStage *const lastRenderStage,
                 const struct material **const lastMaterial,
                 enum shaders *const lastShader) {
        const struct geometry *const geometry = object_getComponent(
                object, COMPONENT_GEOMETRY);

        if (geometry == NULL) {
                // All objects without geometry are lumped at the end. If we
                // find one, we're done.
                return false;
        }

        const struct material *const material = object_getComponent(
                object, COMPONENT_MATERIAL);
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

        struct animationCollection *anims = object_getComponent(
                object, COMPONENT_ANIMATIONCOLLECTION);
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
