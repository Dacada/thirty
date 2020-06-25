#ifndef DSUTILS_H
#define DSUTILS_H

#include <stdbool.h>
#include <stddef.h>

/*
 * A generic growing array. Can add and remove elements from the end.
 */
struct growingArray {
        size_t capacity;
        size_t length;
        size_t itemSize;
        void *data;
};

typedef bool (*const growingArray_foreach_cb)(void *const, void *const);

void growingArray_init(struct growingArray *ga,
                       size_t itemSize, size_t initialCapacity);

/*
 * Allocate space (if needed) for a new element for the array, then return a
 * pointer to the newly allocated element. The calling code can then write
 * whatever data it wants to this pointer.
 */
void *growingArray_append(struct growingArray *ga);

/*
 * Remove the last element from the growing array. Will not deallocate memory
 * in any case.
 */
void growingArray_pop(struct growingArray *ga);

/*
 * Deallocate all the memory from the growing array that isn't being used so
 * that capacity == size.
 */
void growingArray_pack(struct growingArray *ga);

/*
 * Deallocate everything in the growing array, reducing capacity to 0. Should
 * be reinitialized if it's going to be reused.
 */
void growingArray_destroy(struct growingArray *ga);

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
                          growingArray_foreach_cb fun, void *args);

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

void stack_init(struct stack *s, size_t capacity, size_t itemSize);

/*
 * Adds an element to the stack, returning the address of the element so that
 * the calling code can write the actual data to it. Returns NULL if stack is
 * full.
 */
void *stack_push(struct stack *s);

/*
 * Removes the element at the top of the stack and returns its address. Returns
 * NULL if stack is empty.
 */
void *stack_pop(struct stack *s);

/*
 * Returns the address of the element at the top of the stack or NULL if the
 * stack is empty.
 */
const void *stack_peek(const struct stack *s);

/*
 * Destroy stack and free memory. Should be reinitialized if reused.
 */
void stack_destroy(struct stack *s);

#endif /* DSUTILS_H */
