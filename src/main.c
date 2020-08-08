#include <inputHelpers.h>
#include <scene.h>
#include <componentCollection.h>
#include <camera.h>
#include <window.h>
#include <util.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TITLE_BUFFER_SIZE 256
#define FRAME_PERIOD_FPS_REFRESH 10
#define CAMERA_PITCH_LIMIT_OFFSET 0.1F

static const float movement_speed = 10.0F;
static const float look_sensitivity = 0.1F;

static struct scene *scene;

static char title[TITLE_BUFFER_SIZE];

static struct object *find_camera(struct object *const object) {
        struct camera *cam = (struct camera*)
                componentCollection_get(&object->components,
                                        COMPONENT_CAMERA);
        if (cam != NULL && cam->main) {
                return object;
        }

        growingArray_foreach_START(&object->children, size_t*, child)
                struct object *obj = scene_getObjectFromIdx(scene, *child);
                struct object *ret = find_camera(obj);
                if (ret != NULL) {
                        return ret;
                }
        growingArray_foreach_END

        return NULL;
}

static struct fpsCameraController cam_ctrl;
static void processMouseInput(double xpos, double ypos) {
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
static void processKeyboardInput(void) {
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

static void processKeyboardEvent(const int key, const int action,
                                 const int modifiers) {
        (void)modifiers;
        if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_ESCAPE) {
                        window_close();
                } else if (key == GLFW_KEY_F) {
                        cam_ctrl.freefly = !cam_ctrl.freefly;
                }
        }
}

static void draw(void) {
        static unsigned count = 0;
        if (count == FRAME_PERIOD_FPS_REFRESH) {
                const int fps = (const int)(1.0F/window_timeDelta());
                snprintf(title, TITLE_BUFFER_SIZE, "[%d] Temple (%s)", fps,
                         cam_ctrl.freefly ?
                         "Freefly camera" :
                         "FPS camera");
                window_updateTitle();
                count = 0;
        } else {
                count++;
        }

        scene_draw(scene);

#ifndef NDEBUG
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
}

static void freeScene(void) {
        scene_free(scene);
        free(scene);
        componentCollection_shutdown();
}

int main(void) {
        componentCollection_startup();
        window_init(SCREEN_WIDTH, SCREEN_HEIGHT);
        window_title = title;

        scene = smalloc(sizeof(*scene));
        scene_initFromFile(scene, "scene");
        scene_setSkybox(scene, "skybox");
        
        fpsCameraController_init(&cam_ctrl,
                                 movement_speed, look_sensitivity,
                                 find_camera(&scene->root));

        window_onKeyboardInput = processKeyboardInput;
        window_onKeyboardEvent = processKeyboardEvent;
        window_onMousePosition = processMouseInput;
        window_onDraw = draw;
        window_onTearDown = freeScene;
        
        window_run();
        
        return 0;
}
