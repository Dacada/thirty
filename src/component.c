#include <thirty/component.h>
#include <thirty/util.h>

void component_init(struct component *const component, const char *const name) {
        component->name = sstrdup(name);
}

void component_free(struct component *const component) {
        free(component->name);
        component->name = NULL;
}
