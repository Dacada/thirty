#ifndef WINDOW_H
#define WINDOW_H

#include <cglm/struct.h>
#include <stdbool.h>

/*
 * Manage the window, OpenGL context, and everything else.
 */

/*
 * onKeyboardEvent is called whenever a new key is pressed, released or
 * repeated. Check https://www.glfw.org/docs/latest/input_guide.html#input_key
 * This is useful for when you want to react to a single keypress instead of a
 * key being pressed or not on that particular frame (e.g. press E to activate)
 */
extern void(*window_onKeyboardEvent)(const int, const int, const int);

/*
 * onKeyboardInput is called every frame, even if no keyboard keys are
 * pressed. Use window_keyPressed to check if a key is pressed. This is useful
 * for movement keys and other situations when we care about what keys are
 * pressed on that particular frame.
 */
extern void(*window_onKeyboardInput)(void);

/*
 * Called when the mouse position changes with the new coordinates
 */
extern void(*window_onMousePosition)(const double, const double);

/*
 * Called when the mouse is scrolled with the offset.
 */
extern void(*window_onMouseScroll)(const double);

/*
 * Called in the update step of the frame.
 */
extern void(*window_onUpdate)(void);

/*
 * Called in the draw step of the frame.
 */
extern void(*window_onDraw)(void);

/*
 * Called just before closing the window and terminating everything.
 */
extern void(*window_onTearDown)(void);

/*
 * Change this value (effective next frame) to change the clear color.
 */
extern vec4s window_clearColor;

/*
 * Change this value (effective upon call to window_updateTitle) to change the
 * window's title.
 */
extern char *window_title;

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
 * Read window_title and update the window's title.
 */
void window_updateTitle(void);

/*
 * Asynchronously close the window and terminate OpenGL, GLFW and everything
 * else.
 */
void window_close(void);

#endif
