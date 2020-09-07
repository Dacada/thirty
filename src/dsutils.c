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

#define growingArrayAddress(ga, n)              \
        (void *const)((char *const)(ga)->data + ((n) * (ga)->itemSize))

void *growingArray_append(struct growingArray *const ga) {
        while (ga->length >= ga->capacity) {
                ga->capacity *= 2;
                ga->data = sreallocarray(ga->data, ga->capacity, ga->itemSize);
        }
        void *const ptr = growingArrayAddress(ga, ga->length);
        ga->length++;
        return ptr;
}

void *growingArray_get(const struct growingArray *const ga, const size_t n) {
        assert(n < ga->length);
        return growingArrayAddress(ga, n);
}

void growingArray_pack(struct growingArray *const ga) {
        ga->data = sreallocarray(ga->data, ga->length, ga->itemSize);
}

void growingArray_foreach(const struct growingArray *const ga,
                          const foreach_cb fun, void *const args) {
        for (size_t i=0; i<ga->length; i++) {
                if (!fun(growingArray_get(ga, i), args)) {
                        break;
                }
        }
}

void growingArray_sort(struct growingArray *const ga,
                       const cmp_cb cmp, void *const args) {
        qsort_r(ga->data, ga->length, ga->itemSize, cmp, args);
}

static void *bsearch_r(const void *key, void *base,
                       size_t num, size_t size, cmp_cb cmp, void *args) {
	char *pivot;
	int result;

	while (num > 0) {
		pivot = (char*)base + (num >> 1) * size;
		result = cmp(key, pivot, args);

		if (result == 0)
			return (void *)pivot;

		if (result > 0) {
			base = pivot + size;
			num--;
		}
		num >>= 1;
	}

	return NULL;
}

void *growingArray_bsearch(struct growingArray *ga, const void *key, cmp_cb cmp, void *args) {
        return bsearch_r(key, ga->data, ga->length, ga->itemSize, cmp, args);
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



void varSizeGrowingArray_init(struct varSizeGrowingArray *const vga,
                              const size_t alignment,
                              const size_t initialCapacity,
                              const size_t itemSizeHint) {
        size_t padding = alignment;
        while (sizeof(size_t) > padding) {
                padding *= 2;
        }
        padding -= sizeof(size_t);
        vga->padding = padding;
        
        vga->capacity = initialCapacity;
        vga->data = smalloc(initialCapacity);

        size_t cap;
        if (itemSizeHint == 0) {
                cap = 0;
        } else {
                cap = initialCapacity / itemSizeHint;
        }
        growingArray_init(&vga->offsets, sizeof(size_t), cap);
}

void *varSizeGrowingArray_append(struct varSizeGrowingArray *const vga,
                                 const size_t size) {
        size_t offset;
        if (vga->offsets.length == 0) {
                offset = 0;
        } else {
                const size_t *const off = growingArray_get(
                        &vga->offsets, vga->offsets.length-1);
                offset = *off;
        }
        
        size_t totalSize = sizeof(size_t) + vga->padding + size;

        bool change = false;
        while (offset + totalSize > vga->capacity) {
                if (vga->capacity == 0) {
                        vga->capacity = 1;
                }
                vga->capacity *= 2;
                change = true;
        }
        if (change) {
                vga->data = srealloc(vga->data, vga->capacity);
        }

        size_t *const off = growingArray_append(&vga->offsets);
        *off = offset + totalSize;

        void *const ptr = (void*)(((char *)(vga->data))+offset);
        *((size_t*)ptr) = size;
        return (void*)(((char*)ptr)+sizeof(size_t)+vga->padding);
}

void *varSizeGrowingArray_get(const struct varSizeGrowingArray *const vga,
                              const size_t n, size_t *const s) {
        assert(n < vga->offsets.length);
        
        size_t offset;
        if (n == 0) {
                offset = 0;
        } else {
                const size_t *const off = growingArray_get(&vga->offsets, n-1);
                offset = *off;
        }

        if (s != NULL) {
                *s = *((size_t*)((void*)(((char*)(vga->data))+offset)));
        }
        return ((void*)(((char*)(vga->data))+
                        offset+sizeof(size_t)+vga->padding));
}

void varSizeGrowingArray_foreach(const struct varSizeGrowingArray *const vga,
                                 const foreach_sized_cb fun,
                                 void *const args) {
        char *ptr = vga->data;
        for (size_t i=0; i<vga->offsets.length; i++) {
                size_t size = *((size_t*)((void*)ptr));
                ptr += sizeof(size_t) + vga->padding;
                void *dataPtr = (void*)ptr;
                if (!fun(dataPtr, size, args)) {
                        break;
                }
                ptr += size;
        }
}

void varSizeGrowingArray_destroy(struct varSizeGrowingArray *const vga) {
        vga->capacity = 0;
        free(vga->data);
        vga->data = NULL;
        growingArray_destroy(&vga->offsets);
}



void stack_init(struct stack *const s,
                const size_t capacity, const size_t itemSize) {
        s->capacity = capacity;
        s->itemSize = itemSize;
        s->ptr = 0;
        s->data = smallocarray(capacity, itemSize);
}

void *stack_push(struct stack *const s) {
        assert(!stack_full(s));
        
        void *ptr = (void *const)
                ((char *const)s->data + s->ptr * s->itemSize);
        s->ptr += 1;
        return ptr;
}

void *stack_pop(struct stack *const s) {
        assert(!stack_empty(s));
        
        s->ptr -= 1;
        return (void *const)
                ((char *const)s->data + s->ptr * s->itemSize);
}

const void *stack_peek(const struct stack *const s) {
        assert(!stack_empty(s));
        
        return (const void *const)
                ((const char *const)
                 s->data + (s->ptr - 1) * s->itemSize);
}

bool stack_full(const struct stack *const s) {
        return s->ptr >= s->capacity;
}

bool stack_empty(const struct stack *const s) {
        return s->ptr <= 0;
}

void stack_destroy(struct stack *const s) {
        free(s->data);
        s->capacity = 0;
        s->itemSize = 0;
        s->ptr = 0;
        s->data = NULL;
}
