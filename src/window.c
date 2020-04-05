#include <window.h>
#include <util.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void(*window_onKeyboardInput)(void) = NULL;
void(*window_onKeyboardEvent)(int, int, int) = NULL;
void(*window_onMousePosition)(double, double) = NULL;
void(*window_onMouseScroll)(double) = NULL;
void(*window_onDraw)(void) = NULL;

vec3s window_clearColor = { .x=0.2f, .y=0.3f, .z=0.3f };
char *window_title = NULL;

static void onFramebufferSizeChanged(GLFWwindow *const w,
                                     const int width, const int height) {
        (void)w;
        glViewport(0, 0, width, height);
}

static void onKeyboardEvent(GLFWwindow *const w, const int key,
                            const int scancode, const int action,
                            const int mods) {
        (void)w;
        (void)scancode;
        window_onKeyboardEvent(key, action, mods);
}

static void onMousePosition(GLFWwindow *const w, const double xpos,
                            const double ypos) {
        (void)w;
        if (window_onMousePosition != NULL) {
                window_onMousePosition(xpos, ypos);
        }
}

static void onMouseScroll(GLFWwindow *const w, const double xoff,
                          const double yoff) {
        (void)w;
        (void)xoff;
        if (window_onMouseScroll != NULL) {
                window_onMouseScroll(yoff);
        }
}

static void onDraw(void) {
        if (window_onDraw != NULL) {
                window_onDraw();
        }
}

static GLFWwindow *window = NULL;
static float timeDelta = 1.0f/30.0f; // Initialize with a sane value
                                     // just in case

void window_init(const int width, const int height) {
        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, "", NULL, NULL);
        if (window == NULL) {
                glfwTerminate();
                die("Failed to create GLFW window.");
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                glfwTerminate();
                die("Failed to initialize GLAD.");
        }

        glViewport(0, 0, width, height);
        glfwSetFramebufferSizeCallback(window, onFramebufferSizeChanged);
        glfwSetKeyCallback(window, onKeyboardEvent);
        glfwSetCursorPosCallback(window, onMousePosition);
        glfwSetScrollCallback(window, onMouseScroll);
        
        glEnable(GL_DEPTH_TEST);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void window_run(void) {
        while (!glfwWindowShouldClose(window)) {
                // Update deltatime
                timeDelta = (float)glfwGetTime();
                glfwSetTime(0);

                // Clear screen
                glClearColor(window_clearColor.x, window_clearColor.y,
                             window_clearColor.z, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Draw stuff
                onDraw();

                glfwSwapBuffers(window);

                // Process inputs
                glfwPollEvents();
                window_onKeyboardInput();
        }
        glfwTerminate();
}

float window_timeDelta(void) {
        return timeDelta;
}

bool window_keyPressed(const int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
}

void window_updateTitle(void) {
        if (window_title == NULL) {
                glfwSetWindowTitle(window, "");
        } else {
                glfwSetWindowTitle(window, window_title);
        }
}

void window_close(void) {
        glfwSetWindowShouldClose(window, 1);
}
