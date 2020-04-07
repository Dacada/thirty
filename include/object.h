#ifndef OBJECT_H
#define OBJECT_H

#include <camera.h>
#include <glad/glad.h>

/*
 * This module encapsulates OpenGL objects. Given the geometric data, a 3D
 * object is initialized. Its model matrix is also tracked. The draw method
 * uses a camera and a shader to draw the object. The module should be
 * initialized before doing anything else. Tearing down an object frees any
 * data used in the struct object as well as by OpenGL. Objects can be
 * initialized in two ways: From arrays with all the 3D coordinates, or from a
 * file containing this information. The only supported format is one I made up
 * myself, which pretty much directly maps to OpenGL data. The "expected"
 * parameter means that you already know how many object the file contains. If
 * it's false, an array of appropiate size will be allocated at *object. If
 * it's true then we asume that memory is already allocated at *object.
 */

struct vertex {
        vec3s vert;
        vec2s tex;
};

struct object {
        GLuint vao, vbo, ibo;
        int nindices;
        GLuint *textures;
        unsigned ntextures;
        mat4s model;
};

void object_initModule(void);

unsigned object_initFromFile(struct object **object, const char *filename,
                             bool expected);

void object_initFromArray(struct object *object,
                          const struct vertex *vertices, size_t nvertices,
                          const unsigned *indices, size_t nindices);

void object_setTextures(struct object *object,
                        const char *const textures[], unsigned ntextures);

void object_translate(struct object *object, vec3s position);
void object_rotate(struct object *object, float angle, vec3s axis);
void object_scale(struct object *object, vec3s scale);

void object_draw(const struct object *object,
                 const struct camera *camera,
                 unsigned int shader);

void object_tearDown(struct object *object);

#endif /* OBJECT_H */
