#ifndef DSUTILS_H
#define DSUTILS_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Callback for various foreach functions declared here.
 */
typedef bool (*foreach_cb)(const void *, void *);


/*
 * Callback for various sorting functions declared here.
 */
typedef int (*cmp_cb)(const void *, const void *, void *);


/*
 * A generic growing array. Can add and remove elements from the end.
 *
 * The "data" field is a restrict pointer. Accessing it directly (while holding
 * a pointer to a value obtained from one of the array's operations) is
 * undefined behavior.
 */
struct growingArray {
        size_t capacity;
        size_t length;
        size_t itemSize;
        void *data;
};

void growingArray_init(struct growingArray *restrict ga,
                       size_t itemSize, size_t initialCapacity)
        __attribute__((access (write_only, 1)))
        __attribute__((leaf))
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
void *growingArray_append(struct growingArray *restrict ga)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((malloc))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Remove the last element from the growing array. Will not deallocate memory
 * in any case.
 */
void growingArray_pop(struct growingArray *restrict ga)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Deallocate all the memory from the growing array that isn't being used so
 * that capacity == size.
 */
void growingArray_pack(struct growingArray *restrict ga)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
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
        __attribute__((nonnull (1)))
        __attribute__((pure));

/*
 * Deallocate everything in the growing array, reducing capacity to 0. Should
 * be reinitialized if it's going to be reused.
 */
void growingArray_destroy(struct growingArray *restrict ga)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

///////////////////////////////////////////////////////////////////////////////

/*
 * A generic stack. Can add or remove elements from the end. Unlike the generic
 * array, it only allocates data once, so it can't grow past its initial
 * capacity.
 *
 * The "data" field is a restrict pointer. Accessing it directly (while holding
 * a pointer to a value obtained from one of the stack's operations) is
 * undefined behavior.
 */
struct stack {
        size_t capacity;
        size_t itemSize;
        size_t ptr;
        void *restrict data;
};

void stack_init(struct stack *restrict s, size_t capacity, size_t itemSize)
        __attribute__((access (write_only, 1)))
        __attribute__((leaf))
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
void *stack_push(struct stack *restrict s)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((malloc))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Removes the element at the top of the stack and returns its address. Returns
 * NULL if stack is empty.
 */
void *stack_pop(struct stack *restrict s)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Returns the address of the element at the top of the stack or NULL if the
 * stack is empty.
 */
const void *stack_peek(const struct stack *restrict s)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Destroy stack and free memory. Should be reinitialized if reused.
 */
void stack_destroy(struct stack *restrict s)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* DSUTILS_H */
