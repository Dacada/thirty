#ifndef DSUTILS_H
#define DSUTILS_H

#include <stdbool.h>
#include <stddef.h>

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
        void *restrict data;
};

typedef bool (*const growingArray_foreach_cb)(void *const, void *const);

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
 * Deallocate everything in the growing array, reducing capacity to 0. Should
 * be reinitialized if it's going to be reused.
 */
void growingArray_destroy(struct growingArray *restrict ga)
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
                          growingArray_foreach_cb fun, void *args)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (1)));

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
