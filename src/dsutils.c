#define _GNU_SOURCE  // qsort_r

#include <dsutils.h>
#include <util.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

void growingArray_init(struct growingArray *const ga,
                       const size_t itemSize, const size_t initialCapacity) {
        ga->capacity = initialCapacity;
        ga->length = 0;
        ga->itemSize = itemSize;
        ga->data = smallocarray(initialCapacity, itemSize);
}

void *growingArray_append(struct growingArray *const ga) {
        while (ga->length >= ga->capacity) {
                ga->capacity *= 2;
                ga->data = sreallocarray(ga->data, ga->capacity, ga->itemSize);
        }
        void *const ptr = (void *const)((char *const)ga->data + (ga->length * ga->itemSize));
        ga->length++;
        return ptr;
}

void growingArray_pop(struct growingArray *const ga) {
        if (ga->length > 0) {
                ga->length -= 1;
        }
}

void growingArray_pack(struct growingArray *const ga) {
        ga->data = sreallocarray(ga->data, ga->length, ga->itemSize);
}

void growingArray_foreach(const struct growingArray *const ga,
                          const foreach_cb fun, void *const args) {
        for (size_t i=0; i<ga->length; i++) {
                if (!fun((void*)((char*)ga->data + i * ga->itemSize), args)) {
                        break;
                }
        }
}

void growingArray_sort(struct growingArray *const ga,
                       const cmp_cb cmp, void *const args) {
        qsort_r(ga->data, ga->length, ga->itemSize, cmp, args);
}

struct contain_args {
        cmp_cb cmp;
        const void *element;
        bool found;
};
__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
static bool contains(void *item, void *args) {
        struct contain_args *cont_args = args;
        if (cont_args->cmp(cont_args->element, item, NULL) == 0) {
                cont_args->found = true;
                return false;
        }
        return true;
}
bool growingArray_contains(const struct growingArray *const ga,
                           const cmp_cb cmp, const void *const element) {
        struct contain_args args = {
                .cmp = cmp,
                .element = element,
                .found = false,
        };
        growingArray_foreach(ga, contains, &args);
        return args.found;
}

void growingArray_clear(struct growingArray *const ga) {
        ga->length = 0;
}

void growingArray_destroy(struct growingArray *const ga) {
        free(ga->data);
        ga->capacity = 0;
        ga->length = 0;
        ga->itemSize = 0;
        ga->data = NULL;
}



void stack_init(struct stack *const s,
                const size_t capacity, const size_t itemSize) {
        s->capacity = capacity;
        s->itemSize = itemSize;
        s->ptr = 0;
        s->data = smallocarray(capacity, itemSize);
}

void *stack_push(struct stack *const s) {
        if (s->ptr < s->capacity) {
                void *ptr = (void *const)
                        ((char *const)s->data + s->ptr * s->itemSize);
                s->ptr += 1;
                return ptr;
        }
        return NULL;
}

void *stack_pop(struct stack *const s) {
        if (s->ptr > 0) {
                s->ptr -= 1;
                return (void *const)
                        ((char *const)s->data + s->ptr * s->itemSize);
        }
        return NULL;
}

const void *stack_peek(const struct stack *const s) {
        if (s->ptr > 0) {
                return (const void *const)
                        ((const char *const)
                         s->data + (s->ptr - 1) * s->itemSize);
        }
        return NULL;
}

void stack_destroy(struct stack *const s) {
        free(s->data);
        s->capacity = 0;
        s->itemSize = 0;
        s->ptr = 0;
        s->data = NULL;
}
