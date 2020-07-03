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
        vec3s tang;
        vec3s binorm;
};

struct geometry {
        GLuint vao, vbo, ibo;
        int nindices;
};

void geometry_initFromArray(struct geometry *restrict geometry,
                            const struct vertex *restrict vertices,
                            size_t nvertices,
                            const unsigned *restrict indices,
                            size_t nindices)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2, 3)))
        __attribute__((access (read_only, 4, 5)))
        __attribute__((leaf))
        __attribute__((nonnull (1)));

void geometry_draw(const struct geometry *restrict geometry, mat4s model)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void geometry_free(const struct geometry *restrict geometry)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* GEOMETRY_H */
