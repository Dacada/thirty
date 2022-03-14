#ifndef ANIMATION_COLLECTION_H
#define ANIMATION_COLLECTION_H

#include <thirty/component.h>
#include <thirty/shader.h>
#include <thirty/animation.h>
#include <thirty/dsutils.h>

/*
 * A collection of animations. This component handles playing, stopping,
 * posing, etc the model of the object it is attached to. As such, the model
 * must have well defined bone indices and weights that coincide with the
 * skeleton's.
 */

/*
 * Get the timestamp corresponding to an animation frame for a given framerate.
 */
#define ANIMATION_FRAME_TO_TIMESTAMP(frame, framerate)  \
        ((float)(frame)/(float)(framerate))

struct animationCollection {
        struct component base;
        
        struct skeleton skeleton;
        
        size_t nanimations;
        struct animation *animations;

        bool running;
        size_t current;
        float time;
};

/*
 * Initialize an animation collection from a BOGLE file positioned at the
 * correct offset.
 */
size_t animationCollection_initFromFile(struct animationCollection *col,
                                        FILE *f, enum componentType type,
                                        struct varSizeGrowingArray *components)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((access (read_write, 4)))
        __attribute__((nonnull));

/*
 * Get the idx corresponding to the animaton in this collection with the given
 * name. If it's not found, 0 is returned. Otherwise the idx+1 is returned. So
 * 1 should be substracted from it before using it.
 */
size_t animationCollection_idxByName(struct animationCollection *col,
                                     const char *name)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Play the given animation (by idx). The animation will play on a loop.
 */
void animationCollection_playAnimation(struct animationCollection *col,
                                       size_t anim)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Pose the given animation (by idx). The animation will stay in the frame
 * corresponding to the given timestamp without moving.
 */
void animationCollection_poseAnimation(struct animationCollection *col,
                                       size_t anima, float timestamp)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Set the animated object back to its bindpose, undoing any animation.
 */
void animationCollection_setBindPose(struct animationCollection *col)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Bind the skeleton's bones to the given shader such that an object will be
 * drawn with the correct pose, if any.
 */
void animationCollection_bindBones(const struct animationCollection *col,
                                   enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Run on each frame to update the running animation timestamp with the
 * timeDelta.
 */
void animationCollection_update(struct animationCollection *col,
                                float timeDelta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Free resources used by an animationCollection, deinitializing it.
 */
void animationCollection_free(struct animationCollection *col)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
