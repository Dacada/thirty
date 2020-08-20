#include <inputHelpers.h>
#include <scene.h>
#include <componentCollection.h>
#include <animationCollection.h>
#include <camera.h>
#include <geometry.h>
#include <light.h>
#include <window.h>
#include <eventBroker.h>
#include <util.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TITLE_BUFFER_SIZE 256
#define FRAME_PERIOD_FPS_REFRESH 10
#define CAMERA_PITCH_LIMIT_OFFSET 0.1F

static const vec4s globalAmbientLight = {.x=0.3F, .y=0.3F, .z=0.3F, .w=1.0F};
static const float movement_speed = 10.0F;
static const float look_sensitivity = 0.1F;

static struct scene *scene;

static struct fpsCameraController cam_ctrl;
static void processMouseInput(void *vargs) {
        struct eventBrokerMousePosition *args = vargs;
        const double xpos = args->xpos;
        const double ypos = args->ypos;
        
        const vec2s curr = {
                .x = (float)xpos,
                .y = (float)ypos,
        };

        static bool first = true;
        static vec2s prev;
        if (first) {
                prev = curr;
                first = false;
        }

        const vec2s offset = glms_vec2_sub(prev, curr);
        fpsCameraController_look(&cam_ctrl, offset, window_timeDelta());

        prev = curr;
}

static void processKeyboardInput(void *args) {
        (void)args;
        
        vec2s movement = {.x=0, .y=0};
        
        if (window_keyPressed(GLFW_KEY_LEFT) ||
            window_keyPressed(GLFW_KEY_A)) {
                movement.x -= 1;
        }
        if (window_keyPressed(GLFW_KEY_RIGHT) ||
            window_keyPressed(GLFW_KEY_D)) {
                movement.x += 1;
        }
        
        if (window_keyPressed(GLFW_KEY_UP) ||
            window_keyPressed(GLFW_KEY_W)) {
                movement.y += 1;
        }
        if (window_keyPressed(GLFW_KEY_DOWN) ||
            window_keyPressed(GLFW_KEY_S)) {
                movement.y -= 1;
        }

        movement = glms_vec2_normalize(movement);
        fpsCameraController_move(&cam_ctrl, movement, window_timeDelta());
}

static void processKeyboardEvent(void *vargs) {
        struct eventBrokerKeyboardEvent *args = vargs;
        const int key = args->key;
        const int action = args->action;
        //const int modifiers = args->modifiers;
                        
        if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_ESCAPE) {
                        window_close();
                } else if (key == GLFW_KEY_F) {
                        cam_ctrl.freefly = !cam_ctrl.freefly;
                }
        }
}

static void updateScene(void *args) {
        (void)args;
        scene_update(scene);
}

static void updateTitle(void *args) {
        (void)args;
        static unsigned count = 0;
        if (count == FRAME_PERIOD_FPS_REFRESH) {
                const int fps = (const int)(1.0F/window_timeDelta());
                static char title[TITLE_BUFFER_SIZE];
                snprintf(title, TITLE_BUFFER_SIZE, "[%d] Temple (%s)", fps,
                         cam_ctrl.freefly ?
                         "Freefly camera" :
                         "FPS camera");
                window_updateTitle(title);
                count = 0;
        } else {
                count++;
        }
}

static void drawScene(void *args) {
        (void)args;
        scene_draw(scene);

#ifndef NDEBUG
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
}

static void freeScene(void *args) {
        (void)args;
        
        scene_free(scene);
        free(scene);
        componentCollection_shutdown();
}

int main(void) {
        componentCollection_startup();
        window_init(SCREEN_WIDTH, SCREEN_HEIGHT);

        scene = smalloc(sizeof(*scene));
        scene_init(scene, globalAmbientLight, 1);

        // Create sphere
        struct object *sphere = scene_createObject(scene, "sphere", 0);
        
        struct geometry *geo = componentCollection_create(COMPONENT_GEOMETRY);
        size_t geoIdx = geo->base.idx;
        geometry_initIcosphere(geo, "sphere_geo", 3);
        componentCollection_set(&sphere->components, COMPONENT_GEOMETRY, geoIdx);
        
        struct material_uber *mat = componentCollection_create(
                COMPONENT_MATERIAL_UBER);
        size_t matIdx = mat->base.base.idx;
        material_uber_initDefaults(mat, "sphere_material", SHADER_UBER);
        vec4s white = GLMS_VEC4_ONE_INIT;
        mat->emissiveColor = white;
        material_setTexture((struct material*)mat,
                            MATERIAL_TEXTURE_EMISSIVE, "wood");
        componentCollection_set(&sphere->components, COMPONENT_MATERIAL, matIdx);

        // Create camera
        struct object *camera = scene_createObject(scene, "camera", 0);
        object_translateZ(camera, 10.0F);

        struct camera *cam = componentCollection_create(COMPONENT_CAMERA_FPS);
        size_t camIdx = cam->base.idx;
        camera_init(cam, "camera", (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT,
                    0.1F, 100.0F, 45.0F, true, COMPONENT_CAMERA_FPS);
        componentCollection_set(&camera->components, COMPONENT_CAMERA, camIdx);

        fpsCameraController_init(&cam_ctrl, movement_speed, look_sensitivity,
                                 camera);

        // Register events
        eventBroker_register(processKeyboardInput, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_KEYBOARD_INPUT);
        eventBroker_register(processKeyboardEvent, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_KEYBOARD_EVENT);
        eventBroker_register(processMouseInput, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_MOUSE_POSITION);
        eventBroker_register(updateScene, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_UPDATE);
        eventBroker_register(updateTitle, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_UPDATE);
        eventBroker_register(drawScene, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_DRAW);
        eventBroker_register(freeScene, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_TEAR_DOWN);

        // Main loop
        window_run();
        return 0;
}
