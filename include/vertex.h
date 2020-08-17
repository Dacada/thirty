#ifndef VERTEX_H
#define VERTEX_H

#include <cglm/struct.h>

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
