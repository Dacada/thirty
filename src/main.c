#include <scene.h>
#include <shader.h>
#include <camera.h>
#include <window.h>
#include <util.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TITLE_BUFFER_SIZE 256
#define FRAME_PERIOD_FPS_REFRESH 10
#define CAMERA_SENSITIVITY 0.01F

static struct scene scene;

static char title[TITLE_BUFFER_SIZE];
static bool freefly = false;

static void processKeyboardInput(void) {
        float deltaTime = window_timeDelta();
        
        if (window_keyPressed(GLFW_KEY_W)) {
                camera_move(&scene.camera, camera_FORWARD,
                            deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_S)) {
                camera_move(&scene.camera, camera_BACKWARD,
                            deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_A)) {
                camera_move(&scene.camera, camera_LEFT,
                            deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_D)) {
                camera_move(&scene.camera, camera_RIGHT,
                            deltaTime, freefly);
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
                camera_look(&scene.camera, offset.x, offset.y, true);
        }
}

static void processMouseScroll(const double offset) {
        camera_zoom(&scene.camera, (float)offset);
}

static void draw(void) {
        static unsigned count = 0;
        if (count == FRAME_PERIOD_FPS_REFRESH) {
                int fps = (int)(1.0F/window_timeDelta());
                snprintf(title, TITLE_BUFFER_SIZE, "[%d] Boxes (%s)", fps,
                         freefly ? "Freefly camera" : "FPS camera");
                window_updateTitle();
                count = 0;
        } else {
                count++;
        }

        scene_draw(&scene);

#ifndef NDEBUG
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
}

static void freeScene(void) {
        scene_free(&scene);
}

int main(void) {
        window_init(SCREEN_WIDTH, SCREEN_HEIGHT);
        window_title = title;
        
        scene_initFromFile(&scene, SCREEN_WIDTH, SCREEN_HEIGHT, "scene");
        scene.camera.look_sensitivity = CAMERA_SENSITIVITY;

        window_onKeyboardInput = processKeyboardInput;
        window_onKeyboardEvent = processKeyboardEvent;
        window_onMousePosition = processMousePosition;
        window_onMouseScroll = processMouseScroll;
        window_onDraw = draw;
        window_onTearDown = freeScene;
        
        window_run();
        
        return 0;
}
