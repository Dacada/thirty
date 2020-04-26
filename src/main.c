#include <object.h>
#include <shader.h>
#include <camera.h>
#include <window.h>
#include <util.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static unsigned int shader_simple;
static unsigned int shader_mix;
static unsigned int shader_color;

static unsigned nobjects = 0;
static struct object *objects = NULL;

static const char *const cubeTextures[] = {
        "container",
        "awesomeface"
};
static const char *const humanfTexture = "HumanF";
static const vec4s floorPlaneColor = {
        .x = 0.4f,
        .y = 0.3f,
        .z = 0.1f,
        .w = 1.0f,
};

static const vec3s startingCameraPosition = {
        .x=-0.0f, .y=1.2f, .z=-9.0f
};
static const float startingCameraYaw = GLM_PI_2f;
static struct camera cam;

static const vec3s lightPos = {
        .x=0.0f, .y=10.0f, .z=0.0f
};
static const vec4s ambientLightColor = {
        .x=1.0f, .y=1.0f, .z=1.0f
};
static const vec4s difuseLightColor = {
        .x=1.0f, .y=1.0f, .z=1.0f
};
static const vec4s specularLightColor = {
        .x=1.0f, .y=1.0f, .z=1.0f
};
static const float ambientLightPower = 0.15f;
static const float difuseLightPower = 50.0f;
static const float specularLightPower = 50.0f;

static char title[256];
static bool freefly = false;

static void processKeyboardInput(void) {
        float deltaTime = window_timeDelta();
        
        if (window_keyPressed(GLFW_KEY_W)) {
                camera_move(&cam, camera_FORWARD, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_S)) {
                camera_move(&cam, camera_BACKWARD, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_A)) {
                camera_move(&cam, camera_LEFT, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_D)) {
                camera_move(&cam, camera_RIGHT, deltaTime, freefly);
        }
}

static void processKeyboardEvent(const int key, const int action,
                                 const int modifiers) {
        (void)modifiers;
        if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_ESCAPE) {
                        window_close();
                } else if (key == GLFW_KEY_F) {
                        freefly = !freefly;
                }
        }
}

static void processMousePosition(const double xpos, const double ypos) {
        static bool first = true;
        static vec2s last;

        vec2s pos = {
                .x=(float)xpos,
                .y=(float)ypos
        };

        if (first) {
                last = pos;
                first = false;
        } else {
                vec2s offset = {
                        .x=pos.x - last.x,
                        .y=last.y - pos.y
                };
                last = pos;
                camera_look(&cam, offset.x, offset.y, true);
        }
}

static void processMouseScroll(const double offset) {
        camera_zoom(&cam, (float)offset);
}

static void draw(void) {
        static unsigned count = 0;
        if (count == 10) {
                int fps = (int)(1.0f/window_timeDelta());
                snprintf(title, 256, "[%d] Boxes (%s)", fps,
                         freefly ? "Freefly camera" : "FPS camera");
                window_updateTitle();
                count = 0;
        } else {
                count++;
        }

        for (unsigned i=0; i<nobjects; i++) {
                struct object *obj = &objects[i];
                unsigned shader;
                if (strcmp(obj->name, "HumanF") == 0) {
                        shader = shader_simple;
                } else if (strncmp(obj->name, "Cube", 4) == 0) {
                        shader = shader_mix;
                } else if (strcmp(obj->name, "FloorPlane") == 0) {
                        shader = shader_color;
                } else {
                        bail("Got an unexpected object name! %s\n", obj->name);
                }
                object_draw(obj, &cam, shader);
        }

#ifndef NDEBUG
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
}

int main(void) {
        object_initModule();
        
        camera_init(&cam, SCREEN_WIDTH, SCREEN_HEIGHT,
                    &startingCameraPosition, NULL, &startingCameraYaw, NULL);
        cam.look_sensitivity = 0.01f;

        window_init(SCREEN_WIDTH, SCREEN_HEIGHT);
        window_title = title;

        // Up next: material system
        // * Separate textures from objects
        // * Create "material" which is associated to object
        // * Material has data like texture, shaders, and various (difuse?)
        //   properties

        // After that: Export cameras and lights from blender
        // * Automate interaction and setting variables in shaders

        shader_simple = shader_new("diffuse", "diffuse");
        shader_use(shader_simple);
        shader_setInt(shader_simple, "texture0", 0);
        shader_setVec3(shader_simple, "light_pos", lightPos);
        shader_setVec4(shader_simple, "ambient_light_color",
                       ambientLightColor);
        shader_setVec4(shader_simple, "difuse_light_color",
                       difuseLightColor);
        shader_setVec4(shader_simple, "specular_light_color",
                       specularLightColor);
        shader_setFloat(shader_simple, "ambient_light_power",
                        ambientLightPower);
        shader_setFloat(shader_simple, "difuse_light_power",
                        difuseLightPower);
        shader_setFloat(shader_simple, "specular_light_power",
                        specularLightPower);
        
        shader_mix = shader_new("mix_two_textures_difuse", "diffuse");
        shader_use(shader_mix);
        shader_setInt(shader_mix, "texture0", 0);
        shader_setInt(shader_mix, "texture1", 1);
        shader_setVec3(shader_mix, "light_pos", lightPos);
        shader_setVec4(shader_mix, "ambient_light_color",
                       ambientLightColor);
        shader_setVec4(shader_mix, "difuse_light_color",
                       difuseLightColor);
        shader_setVec4(shader_mix, "specular_light_color",
                       specularLightColor);
        shader_setFloat(shader_mix, "ambient_light_power",
                        ambientLightPower);
        shader_setFloat(shader_mix, "difuse_light_power",
                        difuseLightPower);
        shader_setFloat(shader_mix, "specular_light_power",
                        specularLightPower);

        shader_color = shader_new("color_difuse", "diffuse");
        shader_use(shader_color);
        shader_setVec4(shader_color, "color", floorPlaneColor);
        shader_setVec3(shader_color, "light_pos", lightPos);
        shader_setVec4(shader_color, "ambient_light_color",
                       ambientLightColor);
        shader_setVec4(shader_color, "difuse_light_color",
                       difuseLightColor);
        shader_setVec4(shader_color, "specular_light_color",
                       specularLightColor);
        shader_setFloat(shader_color, "ambient_light_power",
                        ambientLightPower);
        shader_setFloat(shader_color, "difuse_light_power",
                        difuseLightPower);
        shader_setFloat(shader_color, "specular_light_power",
                        specularLightPower);

        nobjects = object_initFromFile(&objects, "scene", false);
        for (unsigned i=0; i<nobjects; i++) {
                struct object *obj = &objects[i];
                if (strcmp(obj->name, "HumanF") == 0) {
                        object_setTextures(obj, &humanfTexture, 1);
                } else if (strncmp(obj->name, "Cube", 4) == 0) {
                        object_setTextures(obj, cubeTextures, 2);
                } else if (strcmp(obj->name, "FloorPlane") == 0) {
                } else {
                        bail("Got an unexpected object name! %s\n", obj->name);
                }
        }

        window_onKeyboardInput = processKeyboardInput;
        window_onKeyboardEvent = processKeyboardEvent;
        window_onMousePosition = processMousePosition;
        window_onMouseScroll = processMouseScroll;
        window_onDraw = draw;
        
        window_run();

        for (unsigned i=0; i<nobjects; i++) {
                // TODO: This crashes, why?
                // object_tearDown(&objects[i]);
        }
        
        return 0;
}
