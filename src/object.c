#include <object.h>
#include <material.h>
#include <shader.h>
#include <geometry.h>
#include <camera.h>
#include <light.h>
#include <dsutils.h>
#include <util.h>
#include <cglm/struct.h>
#include <string.h> // TODO: only temporary for strcmp

#define STARTING_OBJECT_COUNT 16

static const vec4s color_shader_color = { // Only until shaders properly set up
        .x = 0.4F, .y = 0.3F, .z = 0.1F, .w = 1.0F
};

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

        object->geometry = NULL;
        if (!skip_geometry) {
                struct vertex *const restrict vertices =
                        smallocarray(obj_header.vertlen,
                                     sizeof(*vertices));
                unsigned *const restrict indices =
                        smallocarray(obj_header.indlen,
                                     sizeof(*indices));
        
                sfread(vertices, sizeof(*vertices), obj_header.vertlen, f);
                sfread(indices, sizeof(*indices), obj_header.indlen, f);

                object->geometry = smalloc(sizeof *object->geometry);
                geometry_initFromArray(object->geometry,
                                       vertices, obj_header.vertlen,
                                       indices, obj_header.indlen);

                free(vertices);
                free(indices);
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
        
        // TODO: This should be also in the file but for now it's hard coded
        // depending on the object name.
        static bool first = true;
        static struct material diagonalCeilingMaterial;
        static struct material diagonalFloorMaterial;
        static struct material diagonalWallMaterial;
        static struct material highStraightWallMaterial;
        static struct material straightCeilingMaterial;
        static struct material straightCeilingWithLightMaterial;
        static struct material straightFloorMaterial;
        static struct material straightWallMaterial;
        if (first) {
                material_initDefaults(&diagonalCeilingMaterial);
                material_setTexture(
                        &diagonalCeilingMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "DiagonalCeiling");
                material_updateShader(&diagonalCeilingMaterial);
                
                material_initDefaults(&diagonalFloorMaterial);
                material_setTexture(
                        &diagonalFloorMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "DiagonalFloor");
                material_updateShader(&diagonalFloorMaterial);
                
                material_initDefaults(&diagonalWallMaterial);
                material_setTexture(
                        &diagonalWallMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "DiagonalWall");
                material_updateShader(&diagonalWallMaterial);
                
                material_initDefaults(&highStraightWallMaterial);
                material_setTexture(
                        &highStraightWallMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "HighStraightWall");
                material_updateShader(&highStraightWallMaterial);
                
                material_initDefaults(&straightCeilingMaterial);
                material_setTexture(
                        &straightCeilingMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "StraightCeiling");
                material_updateShader(&straightCeilingMaterial);
                
                material_initDefaults(&straightCeilingWithLightMaterial);
                material_setTexture(
                        &straightCeilingWithLightMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "StraightCeilingWithLight");
                material_updateShader(&straightCeilingWithLightMaterial);
                
                material_initDefaults(&straightFloorMaterial);
                material_setTexture(
                        &straightFloorMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "StraightFloor");
                material_updateShader(&straightFloorMaterial);
                
                material_initDefaults(&straightWallMaterial);
                material_setTexture(
                        &straightWallMaterial,
                        MATERIAL_TEXTURE_DIFFUSE, "StraightWall");
                material_updateShader(&straightWallMaterial);
                
                first = false;
        }
        
        if (strncmp(object->name, "DiagonalCeiling.", 16) == 0) {
                object->material = &diagonalCeilingMaterial;
        } else if (strncmp(object->name, "DiagonalFloor.", 14) == 0) {
                object->material = &diagonalFloorMaterial;
        } else if (strncmp(object->name, "DiagonalWall.", 13) == 0) {
                object->material = &diagonalWallMaterial;
        } else if (strncmp(object->name, "HighStraightWall.", 17) == 0) {
                object->material = &highStraightWallMaterial;
        } else if (strncmp(object->name, "StraightCeiling.", 16) == 0) {
                object->material = &straightCeilingMaterial;
        } else if (strncmp(object->name,"StraightCeilingWithLight.",26) == 0) {
                object->material = &straightCeilingWithLightMaterial;
        } else if (strncmp(object->name, "StraightFloor.", 14) == 0) {
                object->material = &straightFloorMaterial;
        } else if (strncmp(object->name, "StraightWall.", 13) == 0) {
                object->material = &straightWallMaterial;
        } else {
                bail("Got an unexpected geometry name! %s\n", object->name);
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
static void gather_object_tree(struct growingArray *const restrict objects,
                               const struct object *const object,
                               const struct camera *const restrict camera,
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
                
                struct objectModelAndDistance *const restrict obj_mod =
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

struct renderTypeShaderCamera {
        enum renderType renderType;
        enum shaders shader;
        const mat4s *projection;
        const mat4s *view;
};

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static bool render_object(const void *const restrict item,
                          void *const restrict args) {
        const struct objectModelAndDistance *const obj_mod = item;
        const struct renderTypeShaderCamera *const type_shader_cam = args;
        const enum shaders shader = type_shader_cam->shader;

        if (obj_mod->object->material->shader != shader) {
                return true;
        }
        
        switch (type_shader_cam->renderType) {
        case RENDER_OPAQUE_OBJECTS:
                if (material_isTransparent(obj_mod->object->material)) {
                        return true;
                }
                break;
        case RENDER_TRANSPARENT_OBJECTS:
                if (!material_isTransparent(obj_mod->object->material)) {
                        return true;
                }
                break;
        default:
                bail("Unexpected render mode.\n");
        }
        
        const mat4s modelView = glms_mat4_mul(
                obj_mod->model, *(type_shader_cam->view));
        const mat4s modelViewProjection = glms_mat4_mul(
                modelView, *(type_shader_cam->projection));
        shader_setMat4(shader, "modelView", modelView);
        shader_setMat4(shader, "modelViewProjection", modelViewProjection);

        // Check geometry_draw, but more stuff needs to be added
        material_bindTextures(obj_mod->object->material);
        geometry_draw(obj_mod->object->geometry, obj_mod->model);

        return true;
}

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull (1, 2)))
__attribute__((pure))
static int cmp_objs(const void *const item1, const void *const item2,
                    void *const restrict args) {
        const struct objectModelAndDistance *const obj1 = item1;
        const struct objectModelAndDistance *const obj2 = item2;
        (void)args;

        if (obj1->distanceToCamera < obj2->distanceToCamera) {
                return -1;
        } else if (obj1->distanceToCamera > obj2->distanceToCamera) {
                return 1;
        } else {
                return 0;
        }
}

void object_draw(const struct object *const restrict object,
                 const struct camera *const restrict camera,
                 const size_t nlights, const size_t nshaders,
                 const struct light *const lights,
                 const enum shaders *const shaders) {
        const mat4s projection = camera_projectionMatrix(camera);
        const mat4s view = camera_viewMatrix(camera);
        
        struct renderTypeShaderCamera type_shader_cam;
        type_shader_cam.projection = &projection;
        type_shader_cam.view = &view;
        
        struct growingArray objects;
        growingArray_init(&objects, sizeof(struct objectModelAndDistance),
                          STARTING_OBJECT_COUNT);
        gather_object_tree(&objects, object, camera, GLMS_MAT4_IDENTITY);
        growingArray_sort(&objects, cmp_objs, NULL);

        for (size_t i=0; i<nshaders; i++) {
                shader_use(shaders[i]); // TODO: Remove any other shader_use
                
                light_updateShader(lights, nlights, shaders[i]);

                type_shader_cam.shader = shaders[i];
        
                type_shader_cam.renderType = RENDER_OPAQUE_OBJECTS;
                growingArray_foreach(&objects, render_object,
                                     &type_shader_cam);

                // TODO: First check one pass opaque only is ok
                //type_shader_cam.renderType = RENDER_TRANSPARENT_OBJECTS;
                //growingArray_foreach(&objects, render_object,
                //                     &type_shader_cam);
        }

        growingArray_destroy(&objects);
}

void object_free(const struct object *object) {
        material_free(object->material);
        if (object->geometry != NULL) {
                geometry_free(object->geometry);
                free(object->geometry);
        }
}
