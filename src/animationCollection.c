#include <thirty/animationCollection.h>
#include <thirty/util.h>

size_t animationCollection_initFromFile(struct animationCollection *const col,
                                        FILE *const f, const enum componentType type,
                                        struct varSizeGrowingArray *const components) {
        assert(type == COMPONENT_ANIMATIONCOLLECTION);
        (void)components;
        
        char *name = strfile(f);
        component_init(&col->base, name);
        free(name);

        uint32_t nanimations;
        sfread(&nanimations, sizeof(nanimations), 1, f);
        col->nanimations = nanimations;

        skeleton_initFromFile(&col->skeleton, f);

        col->animations = smallocarray(nanimations, sizeof(*col->animations));
        for (size_t i=0; i<nanimations; i++) {
                animation_initFromFile(&col->animations[i], f,
                                       col->skeleton.nbones);
        }

        col->running = false;
        col->current = 0;

        return sizeof(struct animationCollection);
}

size_t animationCollection_idxByName(struct animationCollection *const col,
                                     const char *const name) {
        assert(col->base.type == COMPONENT_ANIMATIONCOLLECTION);
        
        for (size_t i=0; i<col->nanimations; i++) {
                if (strcmp(col->animations[i].name, name) == 0) {
                        return i+1;
                }
        }
        return 0;
}

void animationCollection_playAnimation(struct animationCollection *const col,
                                       const size_t anim) {
        assert(col->base.type == COMPONENT_ANIMATIONCOLLECTION);
        
        col->running = true;
        col->current = anim+1;
        col->time = 0.0F;
}

void animationCollection_poseAnimation(struct animationCollection *const col,
                                       const size_t anim,
                                       const float timestamp) {
        assert(col->base.type == COMPONENT_ANIMATIONCOLLECTION);
        
        col->running = false;
        col->current = anim+1;
        col->time = timestamp;
}

void animationCollection_setBindPose(struct animationCollection *col) {
        assert(col->base.type == COMPONENT_ANIMATIONCOLLECTION);
        
        col->running = false;
        col->current = 0;
}

void animationCollection_bindBones(const struct animationCollection *const col,
                                   const enum shaders shader) {
        assert(col->base.type == COMPONENT_ANIMATIONCOLLECTION);
        
        if (col->current > 0) {
                animation_bindBones(&col->animations[col->current-1],
                                    &col->skeleton, col->time, shader);
        } else {
                animation_bindBones(NULL, &col->skeleton, 0.0F, shader);
        }
}

void animationCollection_update(struct animationCollection *const col,
                                const float timeDelta) {
        assert(col->base.type == COMPONENT_ANIMATIONCOLLECTION);
        
        if (col->running && col->current > 0) {
                col->time += timeDelta;
                
                struct animation *anim = &col->animations[col->current-1];
                size_t nkeyframes = anim->nkeyframes;
                if (nkeyframes > 0) {
                        float totalTime =
                                anim->keyframes[nkeyframes-1].timestamp;
                        if (col->time >= totalTime) {
                                col->time -= totalTime;
                        }
                }
        }
}

void animationCollection_free(struct animationCollection *col) {
        assert(col->base.type == COMPONENT_ANIMATIONCOLLECTION);
        
        component_free(&col->base);
        skeleton_free(&col->skeleton);
        for (size_t i=0; i<col->nanimations; i++) {
                animation_free(&col->animations[i]);
        }
        free(col->animations);
}
