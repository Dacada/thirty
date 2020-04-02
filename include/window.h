#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

typedef void(*window_processInput_cb)(GLFWwindow *);
typedef void(*window_draw_cb)(void);

void window_setProcessInputCallback(window_processInput_cb fun);
void window_setDrawCallback(window_draw_cb fun);

GLFWwindow *window_init(int width, int height, const char *const title);
void window_run(GLFWwindow *window);

#endif
