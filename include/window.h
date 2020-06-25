#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>

/*
 * Create the window, OpenGL context, and everything else. After window_init,
 * call window_run to start the program's main loop.
 *
 * Use window_timeDelta to get the time that it took the latest frame to fully
 * process.
 *
 * Use window_close to asynchronously close the window and terminate OpenGL,
 * GLFW and everything else.
 *
 * The various window_on* variables are callbacks that will be called during
 * certain events. Probably the important one is window_onDraw which is called
 * when the next frame is ready to be drawn, where the code for the actual
 * drawing should go.
 *
 * onKeyboardInput is called every frame, even if no keyboard keys are
 * pressed. Use window_keyPressed to check if a key is pressed. This is useful
 * for movement keys and other situations when we care about what keys are
 * pressed on that particular frame.
 *
 * onKeyboardEvent is called whenever a new key is pressed, released or
 * repeated. Check https://www.glfw.org/docs/latest/input_guide.html#input_key
 * This is useful for when you want to react to a single keypress instead of a
 * key being pressed or not on that particular frame (e.g. press E to activate)
 *
 * onTearDown is called just before the GL context is torn down, for cleanup
 * and stuff.
 *
 * window_onKeyboardEvent: key, action, modifiers
 *
 * window_onKeyboardInput: No arguments.
 *
 * window_onMousePosition: X and Y mouse coordinates.
 *
 * window_onMouseScroll: Scroll offset.
 *
 * window_onDraw: No arguments.
 *
 * window_onTearDown: No arguments.
 *
 * You can edit window_clearColor (effective next frame) and window_title
 * (effective upon call to window_updateTitle).
 */

#include <cglm/struct.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

extern void(*window_onKeyboardEvent)(const int, const int, const int);
extern void(*window_onKeyboardInput)(void);
extern void(*window_onMousePosition)(const double, const double);
extern void(*window_onMouseScroll)(const double);
extern void(*window_onDraw)(void);
extern void(*window_onTearDown)(void);

extern vec4s window_clearColor;
extern char *window_title;

void window_init(int width, int height);
void window_run(void);
float window_timeDelta(void);
bool window_keyPressed(int key);
void window_updateTitle(void);
void window_close(void);

#endif
