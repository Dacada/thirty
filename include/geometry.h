#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <camera.h>
#include <glad/glad.h>

/*
 * This module encapsulates OpenGL geometry. Given the geometric data, a 3D
 * geometry is initialized. Its model matrix is also tracked. The draw method
 * uses a camera and a shader to draw the geometry. The module should be
 * initialized before doing anything else. Tearing down a geometry frees any
 * data used in the struct geometry as well as by OpenGL. Geometries can be
 * initialized from arrays with all the 3D coordinates but they'll usuallt be
 * initialized automatically by a scene processing a file containing this
 * information.
 */

struct vertex {
        vec3s vert;
        vec2s tex;
        vec3s norm;
};

struct geometry {
        GLuint vao, vbo, ibo;
        int nindices;
        GLuint *textures;
        unsigned ntextures;
};

void geometry_initFromArray(struct geometry *geometry,
                            const struct vertex *vertices, size_t nvertices,
                            const unsigned *indices, size_t nindices);

void geometry_setTextures(struct geometry *geometry,
                          const char *const textures[], unsigned ntextures);

void geometry_draw(const struct geometry *geometry, mat4s model,
                   const struct camera *camera, unsigned int shader);

void geometry_free(const struct geometry *geometry);

#endif /* GEOMETRY_H */
