#include <object.h>
#include <material.h>
#include <shader.h>
#include <geometry.h>
#include <camera.h>
#include <light.h>
#include <dsutils.h>
#include <util.h>
#include <cglm/struct.h>
#include <assert.h>

#define STARTING_OBJECT_COUNT 16

void object_init_fromFile(struct object *const object, FILE *const f) {
        struct {
                unsigned vertlen, indlen;
        } obj_header;
        
        sfread(&obj_header.vertlen, 4, 1, f);
        sfread(&obj_header.indlen, 4, 1, f);

        if ((obj_header.vertlen == 0 && obj_header.indlen != 0) ||
            (obj_header.vertlen != 0 && obj_header.indlen == 0)) {
                bail("Number of vertices and indices make no sense. "
                     "Both should be 0 or both should be over 0.");
        }
        const bool skip_geometry = obj_header.vertlen == 0 &&
                obj_header.indlen == 0;

        sfread(object->name, sizeof(char), OBJECT_NAME_SIZE, f);

        if (skip_geometry) {
                object->geometry = NULL;
        } else {
                object->geometry = smalloc(sizeof *object->geometry);
                geometry_initFromFile(object->geometry,
                                      obj_header.vertlen, obj_header.indlen,
                                      f);
        }
        
        vec3s translation;
        vec3s axis;
        vec3s scale;
        float angle;
        
        sfread(translation.raw, sizeof(float), 3, f);
        sfread(axis.raw, sizeof(float), 3, f);
        sfread(&angle, sizeof(float), 1, f);
        sfread(scale.raw, sizeof(float), 3, f);

        object->model = glms_mat4_identity();
        object->model =
                glms_scale(
                        glms_rotate(
                                glms_translate(
                                        object->model,
                                        translation),
                                angle, axis),
                        scale);

        if (skip_geometry) {
                return;
        }
}

void object_translate(struct object *const object, const vec3s position) {
        object->model = glms_translate(object->model, position);
}

void object_rotate(struct object *const object, const float angle,
                   const vec3s axis) {
        object->model = glms_rotate(object->model, angle, axis);
}

void object_scale(struct object *const object, const vec3s scale) {
        object->model = glms_scale(object->model, scale);
}

struct objectModelAndDistance {
        const struct object *object;
        mat4s model;
        float distanceToCamera;
};

enum renderType {
        RENDER_OPAQUE_OBJECTS,
        RENDER_TRANSPARENT_OBJECTS
};

__attribute__((access (read_write, 1)))
__attribute__((access (read_only, 2)))
__attribute__((access (read_only, 3)))
__attribute__((nonnull))
static void gather_object_tree(struct growingArray *const objects,
                               const struct object *const object,
                               const struct camera *const camera,
                               const mat4s parent_model) {
        const mat4s model = glms_mat4_mul(parent_model, object->model);
        if (object->geometry != NULL) {
                vec4s t;
                mat4s r;
                vec3s s;
                glms_decompose(model, &t, &r, &s);
                const vec4s object_position = t;
                
                const vec4s camera_position = {
                        .x = camera->position.x,
                        .y = camera->position.y,
                        .z = camera->position.z,
                        .w = object_position.w,
                };

                const float distance = glms_vec4_distance(
                        object_position, camera_position);
                
                struct objectModelAndDistance *const obj_mod =
                        growingArray_append(objects);
                obj_mod->object = object;
                obj_mod->model = model;
                obj_mod->distanceToCamera = distance;
        }
        
        for (unsigned i=0; i<object->nchildren; i++) {
                gather_object_tree(
                        objects, object->children[i], camera, model);
        }  
}

struct renderArgs {
        enum renderType renderType;
        enum shaders shader;
        const struct material *material;
        const mat4s *projection;
        const mat4s *view;
        const struct light *lights;
        size_t nlights;
};

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static bool render_object(void *const item,
                          void *const vargs) {
        const struct objectModelAndDistance *const obj_mod = item;
        struct renderArgs *const args = vargs;

        if (obj_mod->object->material->shader != args->shader) {
                shader_use(obj_mod->object->material->shader);
                args->shader = obj_mod->object->material->shader;
                light_updateShaderView(args->lights, args->nlights,
                                       *(args->view),
                                       obj_mod->object->material->shader);
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


// TODO: Sort by material first or by shader first?
// Sort by:
//  * Transparency (opaque objects first)
//  * then by Shader (all objects with the same shader grouped)
//  * then by Material (all objects with the same material grouped)
//  * then by Distance to Camera (objects closer to the camera first)
__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull (1, 2)))
__attribute__((pure))
static int cmp_objs(const void *const item1, const void *const item2,
                    void *const args) {
        (void)args;
        
        const struct objectModelAndDistance *const obj1 = item1;
        const struct objectModelAndDistance *const obj2 = item2;
        
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

void object_draw(const struct object *const object,
                 const struct camera *const camera,
                 const size_t nlights, const struct light *const lights) {
        const mat4s projection = camera_projectionMatrix(camera);
        const mat4s view = camera_viewMatrix(camera);

        // Each time an object is rendered it checks this structure.
        // Projection and view are just passed to the shader. Material and
        // shader are the last rendered object's. So that it knows if it needs
        // to change material/shader. Lights and nlights are used to
        // recalculate the light positions and directions in viewspace for each
        // shader.
        struct renderArgs args;
        args.projection = &projection;
        args.view = &view;
        args.renderType = RENDER_OPAQUE_OBJECTS;
        args.material = NULL;
        args.shader = SHADERS_TOTAL;
        args.lights = lights;
        args.nlights = nlights;

        static bool first = true;
        static struct growingArray objects;
        if (first) {
                growingArray_init(&objects,
                                  sizeof(struct objectModelAndDistance),
                                  STARTING_OBJECT_COUNT);
                first = false;
        }
        
        gather_object_tree(&objects, object, camera, GLMS_MAT4_IDENTITY);
        growingArray_sort(&objects, cmp_objs, NULL);
        growingArray_foreach(&objects, render_object, &args);
        growingArray_clear(&objects);
        
        glDisable(GL_BLEND);
}

void object_free(const struct object *object) {
        material_free(object->material);
        if (object->geometry != NULL) {
                geometry_free(object->geometry);
                free(object->geometry);
        }
}
