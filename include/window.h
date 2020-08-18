#ifndef WINDOW_H
#define WINDOW_H

#include <cglm/struct.h>
#include <stdbool.h>

/*
 * Manage the window, OpenGL context, and everything else.
 */

/*
 * Change this value (effective next frame) to change the clear color.
 */
extern vec4s window_clearColor;

/*
 * Initialize window, GL context, etc.
 */
void window_init(int width, int height);

/*
 * Start main loop.
 */
void window_run(void);

/* 
 * Get the time that it took the latest frame to fully process.
 */
float window_timeDelta(void);

/*
 * Return whether a key is pressed.
 */
bool window_keyPressed(int key);

/*
 * Change window's title.
 */
void window_updateTitle(const char *title);

/*
 * Asynchronously close the window and terminate OpenGL, GLFW and everything
 * else at the end of the frame.
 */
void window_close(void);

#endif
