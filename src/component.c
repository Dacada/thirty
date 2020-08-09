#include <component.h>
#include <util.h>
#include <stdlib.h>
#include <stddef.h>

void component_init(struct component *const component, const char *const name) {
        component->name = sstrdup(name);
}

void component_free(struct component *const component) {
        free(component->name);
        component->name = NULL;
}
