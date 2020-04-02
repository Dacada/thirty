#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include <window.h>

static void framebufferSizeChanged(GLFWwindow *window, int width, int height) {
        (void)window;
        glViewport(0, 0, width, height);
}

static window_processInput_cb onProcessInput = NULL;
void window_setProcessInputCallback(window_processInput_cb fun) {
        onProcessInput = fun;
}

static window_draw_cb onDraw = NULL;
void window_setDrawCallback(window_draw_cb fun) {
        onDraw = fun;
}

GLFWwindow *window_init(int width, int height, const char *const title) {
        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (window == NULL) {
                fprintf(stderr, "Failed to create GLFW window\n");
                glfwTerminate();
                exit(EXIT_FAILURE);
        }
        glfwMakeContextCurrent(window);
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                fprintf(stderr, "Failed to initialize GLAD\n");
                glfwTerminate();
                exit(EXIT_FAILURE);
        }
        
        glViewport(0, 0, width, height);
        glfwSetFramebufferSizeCallback(window, framebufferSizeChanged);
        
        return window;
}

void window_run(GLFWwindow *window) {
        while (!glfwWindowShouldClose(window)) {
                if (onProcessInput != NULL) onProcessInput(window);
                
                glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                
                if (onDraw != NULL) onDraw();
                
                glfwSwapBuffers(window);
                glfwPollEvents();
        }
        
        glfwTerminate();
}
