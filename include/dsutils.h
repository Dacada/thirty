#ifndef DSUTILS_H
#define DSUTILS_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Callback for various foreach functions declared here.
 */
typedef bool (*foreach_cb)(void *, void *);
typedef bool (*foreach_sized_cb)(void *, size_t, void *);


/*
 * Callback for various sorting functions declared here.
 */
typedef int (*cmp_cb)(const void *, const void *, void *);


/*
 * A generic growing array. Can add and remove elements from the end.
 */
struct growingArray {
        size_t capacity;
        size_t length;
        size_t itemSize;
        void *data;
};

void growingArray_init(struct growingArray *ga,
                       size_t itemSize, size_t initialCapacity)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Allocate space (if needed) for a new element for the array, then return a
 * pointer to the newly allocated element. The calling code can then write
 * whatever data it wants to this pointer.
 *
 * Data is allocated using malloc so the first element will be aligned for any
 * type. Next elements are accessed by advancing in increments of
 * 'itemSize'. Which means they should also be aligned.
 */
void *growingArray_append(struct growingArray *ga)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Return the address of the nth element in the array.
 */
void *growingArray_get(const struct growingArray *ga, size_t n)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Deallocate all the memory from the growing array that isn't being used so
 * that capacity == size.
 */
void growingArray_pack(struct growingArray *ga)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Run the given function for each element in the growing array. The function
 * will be called with the element for each element in the array. The args
 * parameter is also passed to the function. If the function returns false, the
 * iteration stops.
 *
 * Should not add or remove elements from the array during the iteration, but
 * looking at elements is allowed by passing in the array with args.
 */
void growingArray_foreach(const struct growingArray *ga,
                          foreach_cb fun, void *args)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (1)));

/*
 * Iterate growing array, but implemented as a macro instead of a
 * function. It's more convinient but it has some limitations, like that it
 * can't be nested.
 */
#define growingArray_foreach_START(ga, type, name)                      \
        for (unsigned growingArray_foreach_idx=0;                       \
             growingArray_foreach_idx<(ga)->length;                     \
             growingArray_foreach_idx++) {                              \
        type name = growingArray_get(ga, growingArray_foreach_idx);
#define growingArray_foreach_END                \
        }

/*
 * Sort in place the contents of the growing array. The cmp function acts
 * similar to strcmp: A pointer to an element, a pointer to a second element,
 * and a third pointer to "args"; return an integer. Negative if the first
 * element is smaller, positive if the second is smaller and zero if they're
 * equal. The "args" argument is the third argument to this function, which can
 * be used to avoid having to use global variables and such.
 */
void growingArray_sort(struct growingArray *ga, cmp_cb cmp, void *args)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_write, 3)))
        __attribute__((nonnull (1)));

/*
 * Return whether the growing array contains the value pointed to. The value
 * pointed to can be anything. It uses the given cmp_cb function to decide
 * whether a value in the array is equal to the given value. This is a naive
 * function, so it essentially just calls growingArray_foreach which in turn
 * uses the given cmp function to compare each element in the array with the
 * given pointer. The third argument will always be NULL. If any comparison
 * returns 0, it stops looping and this function returns true. Otherwise if
 * nothing is found it returns false.
 */
bool growingArray_contains(const struct growingArray *ga,
                           cmp_cb cmp, const void *element)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 3)))
        __attribute__((nonnull (1, 2)))
        __attribute__((pure));

/*
 * Reduce the array's length to 0, losing access to every object in the
 * array. But the capacity is maintained at the current value and nothing is
 * deallocated. They array shouldn't be reinitialized to be used again.
 */
void growingArray_clear(struct growingArray *ga)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Deallocate everything in the growing array, reducing capacity to 0. Should
 * be reinitialized if it's going to be reused.
 */
void growingArray_destroy(struct growingArray *ga)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

///////////////////////////////////////////////////////////////////////////////

/*
 * A growing array for elements of varying size. Each element is a cell
 * indicating the length in bytes, then data of that length.
 */

struct varSizeGrowingArray {
        size_t capacity;
        size_t padding;
        struct growingArray offsets;
        void *data;
};

/*
 * Initialize. Initial capacity in total bytes. Item size hint will be used as
 * a hint for the size of each individual item in the array. It's only used on
 * initialization and is generally of no consequence. Smaller values will be
 * "faster" but consume more memory. All memory returned by functions will be
 * aligned at least to the given alignment.
 */
void varSizeGrowingArray_init(struct varSizeGrowingArray *vga,
                              size_t alignment, size_t initialCapacity,
                              size_t itemSizeHint)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Similar to growingArray, but the size of the element to append must be
 * given.
 */
void *varSizeGrowingArray_append(struct varSizeGrowingArray *vga,
                                 size_t size)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Get nth element. If s is not NULL, it will hold its size. Return a pointer
 * to it.
 */
void *varSizeGrowingArray_get(const struct varSizeGrowingArray *vga,
                              size_t n, size_t *s)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (1)))
        __attribute__((returns_nonnull));

/*
 * Similar to growingArray's foreach, but the callback also receives the size
 * of the current element.
 */
void varSizeGrowingArray_foreach(const struct varSizeGrowingArray *vga,
                                 foreach_sized_cb fun, void *args)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (1)));

/*
 * Deallocate everything.
 */
void varSizeGrowingArray_destroy(struct varSizeGrowingArray *vga)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

///////////////////////////////////////////////////////////////////////////////

/*
 * A generic stack. Can add or remove elements from the end. Unlike the generic
 * array, it only allocates data once, so it can't grow past its initial
 * capacity.
 */
struct stack {
        size_t capacity;
        size_t itemSize;
        size_t ptr;
        void *data;
};

void stack_init(struct stack *s, size_t capacity, size_t itemSize)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Adds an element to the stack, returning the address of the element so that
 * the calling code can write the actual data to it. Returns NULL if stack is
 * full.
 *
 * Data is initially allocated using malloc so the first element will be
 * aligned for any type. Next elements are accessed by advancing in increments
 * of 'itemSize'. Which means they should also be aligned.
 */
void *stack_push(struct stack *s)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Removes the element at the top of the stack and returns its address. Returns
 * NULL if stack is empty.
 */
void *stack_pop(struct stack *s)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Returns the address of the element at the top of the stack or NULL if the
 * stack is empty.
 */
const void *stack_peek(const struct stack *s)
        __attribute__((access (read_only, 1)))
        __attribute__((pure))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Returns whether the stack is full.
 */
bool stack_full(const struct stack *s)
        __attribute__((access (read_only, 1)))
        __attribute__((pure))
        __attribute__((nonnull));

/*
 * Returns whether the stack is empty.
 */
bool stack_empty(const struct stack *s)
        __attribute__((access (read_only, 1)))
        __attribute__((pure))
        __attribute__((nonnull));

/*
 * Destroy stack and free memory. Should be reinitialized if reused.
 */
void stack_destroy(struct stack *s)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif /* DSUTILS_H */
