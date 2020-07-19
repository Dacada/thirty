#include <object.h>
#include <shader.h>
#include <material.h>
#include <geometry.h>
#include <camera.h>
#include <light.h>
#include <dsutils.h>
#include <util.h>
#include <cglm/struct.h>
#include <stdint.h>
#include <assert.h>

#define STARTING_OBJECT_COUNT 16
#define STARTING_LIGHT_COUNT 8
#define STARTING_SHADER_COUNT 1

void object_initFromFile(struct object *const object,
                         struct geometry *const geometries,
                         struct material *const materials,
                         struct light *const lights,
                         struct camera *const camera, FILE *const f) {
        uint8_t is_camera;
        sfread(&is_camera, sizeof(is_camera), 1, f);
        if (is_camera == 0) {
                object->camera = NULL;
        } else {
                object->camera = camera;
        }

        uint32_t geometry_idx;
        sfread(&geometry_idx, sizeof(geometry_idx), 1, f);
        if (geometry_idx == 0) {
                object->geometry = NULL;
        } else {
                object->geometry = geometries + geometry_idx - 1;
        }

        uint32_t material_idx;
        sfread(&material_idx, sizeof(material_idx), 1, f);
        if (material_idx == 0) {
                object->material = NULL;
                assert(object->geometry == NULL);
        } else {
                object->material = materials + material_idx - 1;
        }

        uint32_t light_idx;
        sfread(&light_idx, sizeof(light_idx), 1, f);
        if (light_idx == 0) {
                object->light = NULL;
        } else {
                object->light = lights + light_idx - 1;
        }
        
        vec3s translation;
        sfread(translation.raw, sizeof(float), 3, f);
        
        vec3s rotation_axis;
        sfread(rotation_axis.raw, sizeof(float), 3, f);
        
        float angle;
        sfread(&angle, sizeof(float), 1, f);
        
        vec3s scale;
        sfread(scale.raw, sizeof(float), 3, f);
        
        object->model =
                glms_scale(
                        glms_rotate(
                                glms_translate(
                                        glms_mat4_identity(),
                                        translation),
                                angle, rotation_axis),
                        scale);
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

void object_scale(struct object *const object, const vec3s scale) {
        object->model = glms_scale(object->model, scale);
}



struct objectModelAndDistance {
        const struct object *object;
        mat4s model;
        float distanceToCamera;
};

struct lightingInfo {
        struct growingArray *objects;
        struct growingArray *light_idxs;
        mat4s view;
};

enum renderType {
        RENDER_OPAQUE_OBJECTS,
        RENDER_TRANSPARENT_OBJECTS
};

struct renderArgs {
        enum renderType renderType;
        enum shaders shader;
        const struct material *material;
        const mat4s *projection;
        const mat4s *view;
};

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull (1, 2)))
static int cmpshdr(const void *const item1, const void *const item2,
                   void *const args) {
        const enum shaders *const shader1 = item1;
        const enum shaders *const shader2 = item2;
        (void)args;

        return (int)*shader1 - (int)*shader2;
}

__attribute__((access (read_write, 1)))
__attribute__((access (read_only, 2)))
__attribute__((access (write_only, 3)))
__attribute__((access (read_write, 4)))
__attribute__((access (read_write, 5)))
__attribute__((nonnull))
static void gather_object_tree(struct growingArray *const objects,
                               const struct object *const object,
                               unsigned *const camera_idx,
                               struct growingArray *const light_idxs,
                               struct growingArray *const shaders,
                               const mat4s parent_model) {
        const mat4s model = glms_mat4_mul(parent_model, object->model);

        if (object->camera != NULL) {
                *camera_idx = (unsigned)objects->length;
        }
        if (object->light != NULL) {
                unsigned *const light_idx = growingArray_append(light_idxs);
                *light_idx = (unsigned)objects->length;
        }

        if (object->material != NULL &&
            !growingArray_contains(shaders, cmpshdr,
                                   &object->material->shader)) {
                enum shaders *shader = growingArray_append(shaders);
                *shader = object->material->shader;
        }
        
        struct objectModelAndDistance *const obj_mod =
                growingArray_append(objects);
        obj_mod->object = object;
        obj_mod->model = model;
        
        for (unsigned i=0; i<object->nchildren; i++) {
                gather_object_tree(objects, object->children[i],
                                   camera_idx, light_idxs, shaders, model);
        }  
}

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static bool update_shader_lighting(void *const item, void *const args) {
        const enum shaders *const shader = item;
        const struct lightingInfo *const lighting = args;
        const unsigned nlights = (unsigned)lighting->light_idxs->length;

        shader_use(*shader);
        for (unsigned i=0; i<nlights; i++) {
                const unsigned *const light_idx =
                        growingArray_get(lighting->light_idxs, i);
                const struct objectModelAndDistance *const object =
                        growingArray_get(lighting->objects, *light_idx);
                const struct light *const light = object->object->light;
                assert(light != NULL);

                light_updateShader(light, i, lighting->view,
                                   object->model, *shader);
        }
        light_updateShaderDisabled(nlights, *shader);
        return true;
}

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static bool calculate_distance(void *const item, void *const args) {
        struct objectModelAndDistance *const object = item;
        const vec4s *const camera_position = args;

        if (object->object->camera != NULL || object->object->light != NULL) {
                return true; // No need to do it for camera and lights
        }

        vec4s object_position;
        mat4s r;
        vec3s s;
        glms_decompose(object->model, &object_position, &r, &s);
        object->distanceToCamera = glms_vec4_distance(*camera_position,
                                                      object_position);
        return true;
}

// Sort by:
//  * Transparency (opaque objects first)
//  * then by Shader (all objects with the same shader grouped)
//  * then by Material (all objects with the same material grouped)
//  * then by Distance to Camera (objects closer to the camera first)
__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull (1, 2)))
static int cmp_objs(const void *const item1, const void *const item2,
                    void *const args) {
        (void)args;
        
        const struct objectModelAndDistance *const obj1 = item1;
        const struct objectModelAndDistance *const obj2 = item2;

        // No need to do anything for objects without geometry which won't be
        // rendered. Just put them all at the end so we can finish early when
        // we find the first one.
        const bool hasgeo1 = obj1->object->geometry != NULL;
        const bool hasgeo2 = obj2->object->geometry != NULL;
        
        if (!hasgeo1 && !hasgeo2) {
                return 0;
        }
        if (!hasgeo1 && hasgeo2) {
                return 1;
        }
        if (hasgeo1 && !hasgeo2) {
                return -1;
        }
        
        const struct material *mat1 = obj1->object->material;
        const struct material *mat2 = obj2->object->material;

        const enum shaders shader1 = mat1->shader;
        const enum shaders shader2 = mat2->shader;

        const float dist1 = obj1->distanceToCamera;
        const float dist2 = obj2->distanceToCamera;

        const bool trans1 = material_isTransparent(mat1);
        const bool trans2 = material_isTransparent(mat2);

        // Non transparent objects first
        if (!trans1 && trans2) {
                return -1;
        }
        if (trans1 && !trans2) {
                return 1;
        }

        // Group by shader
        if (shader1 < shader2) {
                return -1;
        }
        if (shader1 > shader2) {
                return 1;
        }

        // Group by material
        if (mat1 < mat2) {
                return -1;
        }
        if (mat1 > mat2) {
                return 1;
        }

        // Objects closer to the camera first
        if (dist1 < dist2) {
                return -1;
        }
        if (dist1 > dist2) {
                return 1;
        }
        
        return 0;
}

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static bool render_object(void *const item,
                          void *const vargs) {
        const struct objectModelAndDistance *const obj_mod = item;
        struct renderArgs *const args = vargs;

        if (obj_mod->object->geometry == NULL) {
                // All objects without geometry are lumped at the end. If we
                // find one, we're done.
                return false;
        }

        if (obj_mod->object->material->shader != args->shader) {
                shader_use(obj_mod->object->material->shader);
                args->shader = obj_mod->object->material->shader;
        }
        if (obj_mod->object->material != args->material) {
                material_updateShader(obj_mod->object->material);
                material_bindTextures(obj_mod->object->material);
                args->material = obj_mod->object->material;
        }

        if (args->renderType == RENDER_OPAQUE_OBJECTS &&
            material_isTransparent(obj_mod->object->material)) {
                args->renderType = RENDER_TRANSPARENT_OBJECTS;
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else if (args->renderType == RENDER_TRANSPARENT_OBJECTS) {
                assert(material_isTransparent(obj_mod->object->material));
        }
        
        const mat4s modelView = glms_mat4_mul(*(args->view), obj_mod->model);
        const mat4s modelViewProjection =
                glms_mat4_mul(*(args->projection), modelView);
        shader_setMat4(obj_mod->object->material->shader,
                       "modelView", modelView);
        shader_setMat4(obj_mod->object->material->shader,
                       "modelViewProjection", modelViewProjection);

        geometry_draw(obj_mod->object->geometry);

        return true;
}

void object_draw(const struct object *const object) {
        static bool first = true;
        unsigned camera_idx = 0;
        static struct growingArray objects;
        static struct growingArray light_idxs;
        static struct growingArray shaders;
        if (first) {
                growingArray_init(&objects,
                                  sizeof(struct objectModelAndDistance),
                                  STARTING_OBJECT_COUNT);
                growingArray_init(&light_idxs,
                                  sizeof(unsigned),
                                  STARTING_LIGHT_COUNT);
                growingArray_init(&shaders,
                                  sizeof(unsigned),
                                  STARTING_SHADER_COUNT);
                first = false;
        }

        // First we gather all objects, with their model matrices, all shaders,
        // and learn which object is the camera and which are lights.
        gather_object_tree(&objects, object,
                           &camera_idx, &light_idxs, &shaders,
                           GLMS_MAT4_IDENTITY);
        
        // Get view and projection matrices.
        struct objectModelAndDistance *camera = growingArray_get(&objects,
                                                                 camera_idx);
        struct camera *camera_data = camera->object->camera;
        assert(camera_data != NULL);
        const mat4s view = camera_viewMatrix(camera_data, camera->model);
        const mat4s projection = camera_projectionMatrix(camera_data);

        // Update every shader with the new lighting information
        struct lightingInfo lighting = {
                .objects = &objects,
                .light_idxs = &light_idxs,
                .view = view
        };
        growingArray_foreach(&shaders, update_shader_lighting, &lighting);

        // Now we iterate all objects to calculate their distance to the
        // camera.
        vec4s camera_position;
        mat4s r;
        vec3s s;
        glms_decompose(camera->model, &camera_position, &r, &s);
        growingArray_foreach(&objects, calculate_distance, &camera_position);

        // Sort objects by render order
        growingArray_sort(&objects, cmp_objs, NULL);

        // Each time an object is rendered it checks this structure.
        // Projection and view are just passed to the shader. Material and
        // shader are the last rendered object's. So that it knows if it needs
        // to change material/shader. Similar with render type, to know if it
        // has to change GL to blend mode for rendering transparent
        // objects.
        struct renderArgs args;
        args.projection = &projection;
        args.view = &view;
        args.renderType = RENDER_OPAQUE_OBJECTS;
        args.material = NULL;
        args.shader = SHADERS_TOTAL; // will always be a non existing shader

        // Render everything
        growingArray_foreach(&objects, render_object, &args);

        // Cleanup
        glDisable(GL_BLEND);
        growingArray_clear(&objects);
        growingArray_clear(&light_idxs);
        growingArray_clear(&shaders);
}

void object_free(const struct object *object) {
        if (object->geometry != NULL) {
                geometry_free(object->geometry);
        }
        if (object->material != NULL) {
                material_free(object->material);
        }
}
