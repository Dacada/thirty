#ifndef VERTEX_H
#define VERTEX_H

#include <cglm/struct.h>

/*
 * A representation of a vertex. Each vertex contains a position (vert),
 * texture coordinates (tex), a normal, a tangent and a binormal (norm, rang,
 * binorm), and bone indices and the corresponding weights (bones, weights).
 */

struct vertex {
        vec3s vert;
        vec2s tex;
        vec3s norm;
        vec3s tang;
        vec3s binorm;
        vec3s bones;
        vec3s weights;
};

#endif /* VERTEX_H */
