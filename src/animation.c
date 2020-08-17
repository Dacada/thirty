#include <animation.h>
#include <keyframe.h>
#include <util.h>
#include <stdint.h>

void animation_initFromFile(struct animation *const anim,
                            FILE *const f, const size_t nbones) {
        anim->name = strfile(f);
        
        uint32_t nkeyframes;
        sfread(&nkeyframes, sizeof(nkeyframes), 1, f);
        anim->nkeyframes = nkeyframes;

        anim->keyframes = smallocarray(nkeyframes, sizeof(*anim->keyframes));
        for (size_t i=0; i<nkeyframes; i++) {
                keyframe_initFromFile(&anim->keyframes[i], f, nbones);
        }
}

void animation_bindBones(const struct animation *const anim,
                         const struct skeleton *const skel,
                         const float timestamp, const enum shaders shader) {
        if (anim == NULL) { // No animation: bind pose
                skeleton_bindBones(skel, shader);
                return;
        }
        
        struct keyframe *prev = NULL;
        struct keyframe *next = NULL;
        for (size_t i=0; i<anim->nkeyframes; i++) {
                struct keyframe *keyframe = anim->keyframes + i;
                if (keyframe->timestamp > timestamp) {
                        if (i == 0) {
                                // Animation's first frame takes place before
                                // the current timestamp, so don't begin
                                // playing the animation yet?
                                return;
                        }
                        prev = keyframe - 1;
                        next = keyframe;
                        break; 
                }
        }

        assert((prev == NULL && next == NULL) ||
               (prev != NULL && next != NULL));
        if (prev == NULL) {
                // No frames in this animation, don't do anything.
                skeleton_bindBones(skel, shader);
                return;
        }

        assert(prev->timestamp <= timestamp);
        assert(next->timestamp > timestamp);
        struct keyframe result;
        keyframe_initFromInterp(prev, next, &result, timestamp);

        struct skeleton posedSkeleton;
        skeleton_initFromKeyframe(&posedSkeleton, skel, &result);
        skeleton_bindBones(&posedSkeleton, shader);
        skeleton_free(&posedSkeleton);

        keyframe_free(&result);
}

void animation_free(struct animation *const anim) {
        free(anim->name);
        for (size_t i=0; i<anim->nkeyframes; i++) {
                keyframe_free(&anim->keyframes[i]);
        }
        free(anim->keyframes);
}
