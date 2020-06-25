#include <cglm/struct.h>
#include <object.h>
#include <shader.h>
#include <geometry.h>
#include <util.h>
#include <string.h> // TODO: only temporary for strcmp

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

        struct geometry *geometry = NULL;
        if (!skip_geometry) {
                struct vertex *const vertices = scalloc(obj_header.vertlen,
                                                        sizeof(*vertices));
                unsigned *const indices = scalloc(obj_header.indlen,
                                                  sizeof(*indices));
        
                sfread(vertices, sizeof(*vertices), obj_header.vertlen, f);
                sfread(indices, sizeof(*indices), obj_header.indlen, f);

                geometry = smalloc(sizeof *geometry);
                geometry_initFromArray(geometry,
                                       vertices, obj_header.vertlen,
                                       indices, obj_header.indlen);

                free(vertices);
                free(indices);
        }
        object->geometry = geometry;
        
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
        
        // TODO: Shaders should be created once in some kind of initialization
        // and then used by reference
        static bool first = true;
        static unsigned int simple_shader;
        static unsigned int mixed_shader;
        static unsigned int color_shader;
        if (first) {
                first = false;
                
                simple_shader = shader_new("diffuse", "diffuse");
                shader_use(simple_shader);
                shader_setInt(simple_shader, "texture0", 0);
                
                mixed_shader = shader_new("mix_two_textures_difuse",
                                          "diffuse");
                shader_use(mixed_shader);
                shader_setInt(mixed_shader, "texture0", 0);
                shader_setInt(mixed_shader, "texture1", 1);
                
                color_shader = shader_new("color_difuse", "diffuse");
                shader_use(color_shader);
                shader_setVec4(color_shader, "color", color_shader_color);
        }
        
        // TODO: This should be also in the file but for now it's hard coded
        // depending on the object name.
        if (strcmp(object->name, "HumanF") == 0) {
                object->shader = simple_shader;
                const char *const humanfTexture = "HumanF";
                geometry_setTextures(object->geometry, &humanfTexture, 1);
        } else if (strncmp(object->name, "Cube", 4) == 0) {
                object->shader = mixed_shader;
                static const char *const cubeTextures[] = {
                        "container",
                        "awesomeface"
                };
                geometry_setTextures(object->geometry, cubeTextures, 2);
        } else if (strcmp(object->name, "FloorPlane") == 0) {
                object->shader = color_shader;
        } else {
                bail("Got an unexpected geometry name! %s\n",
                     object->name);
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

static void draw_tree(const struct object *const object,
                      const struct camera *const camera,
                      const struct light lights[LIGHTLIMIT],
                      const mat4s parent_model) {
        // TODO: All drawing operations should be done per shader, so as to
        // minimize the number of shader changes.
        const mat4s model = glms_mat4_mul(parent_model, object->model);
        if (object->geometry != NULL) {
                geometry_draw(object->geometry, model, camera, object->shader);
                light_update_shader(&lights[0], object->shader);
        }
        for (unsigned i=0; i<object->nchildren; i++) {
                draw_tree(object->children[i], camera, lights, model);
        }
}

void object_draw(const struct object *const object,
                 const struct camera *const camera,
                 const struct light lights[LIGHTLIMIT]) {
        draw_tree(object, camera, lights, object->model);
}

void object_free(const struct object *object) {
        if (object->geometry != NULL) {
                geometry_free(object->geometry);
                free(object->geometry);
        }
}
