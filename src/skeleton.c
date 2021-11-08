#include <thirty/skeleton.h>
#include <thirty/util.h>

#define BUFFSIZE 256

__attribute__((access (read_only, 1, 2)))
__attribute__((nonnull))
static bool arrayContains(const size_t *const array, const size_t arrayLen,
                          const size_t element) {
        for (size_t i=0; i<arrayLen; i++) {
                if (array[i] == element) {
                        return true;
                }
        }
        return false;
}


// init boneOrder, determine an order to iterate bones that will always
// evaluate parents first
__attribute__((access (read_write, 1)))
__attribute__((nonnull))
static void calcBoneOrder(struct skeleton *const skel) {
        skel->boneOrder = smallocarray(skel->nbones, sizeof(*skel->boneOrder));
        size_t boneOrderLen = 0;
        while (boneOrderLen < skel->nbones) {
                for (size_t i=0; i<skel->nbones; i++) {
                        size_t parent = skel->bones[i].parent;
                        if (parent == 0 ||  // root
                            arrayContains(skel->boneOrder, boneOrderLen,
                                          parent-1)) {
                                skel->boneOrder[boneOrderLen] = i;
                                boneOrderLen++;
                                if (boneOrderLen >= skel->nbones) {
                                        break;
                                }
                        }
                }
        }
}

static void calcAbsoluteTransforms(struct skeleton *const skel) {
        for (size_t ii=0; ii<skel->nbones; ii++) {
                size_t i = skel->boneOrder[ii];
                struct bone *bone = &skel->bones[i];

                // Start with the parent's absolute transform
                if (bone->parent == 0) {
                        // For the root, the skeleton's transform, which is
                        // relative to the actual object we're animating.
                        bone->absoluteTransform = skel->model;
                } else {
                        bone->absoluteTransform =
                                skel->bones[bone->parent-1].absoluteTransform;
                }

                // Record current location, move to origin
                vec3s trans = glms_vec3(bone->absoluteTransform.col[3]);
                bone->absoluteTransform.col[3].x = 0;
                bone->absoluteTransform.col[3].y = 0;
                bone->absoluteTransform.col[3].z = 0;

                // Apply rotation relative to the parent's
                bone->absoluteTransform = glms_quat_rotate(
                        bone->absoluteTransform, bone->rotationRelative);

                // Set position back to what it was
                bone->absoluteTransform.col[3].x = trans.x;
                bone->absoluteTransform.col[3].y = trans.y;
                bone->absoluteTransform.col[3].z = trans.z;

                // Advance to the bone position relative to the parent
                bone->absoluteTransform = glms_translate(
                        bone->absoluteTransform, bone->positionRelative);
        }
}

static void calcBindPose(struct skeleton *const skel) {
        for (size_t i=0; i<skel->nbones; i++) {
                struct bone *bone = &skel->bones[i];
                bone->bindPoseInv = glms_mat4_inv(bone->absoluteTransform);
        }
}

void skeleton_initFromFile(struct skeleton *const skel, FILE *const f) {
        sfread(skel->model.raw, sizeof(float),
               sizeof(skel->model)/sizeof(float), f);
        
        uint32_t nbones;
        sfread(&nbones, 1, sizeof(nbones), f);
        skel->nbones = nbones;

        skel->bones = smallocarray(nbones, sizeof(*skel->bones));
        for (size_t i=0; i<nbones; i++) {
                bone_initFromFile(&skel->bones[i], f);
        }

        calcBoneOrder(skel);
        calcAbsoluteTransforms(skel);
        calcBindPose(skel);
}

void skeleton_initFromKeyframe(struct skeleton *const skel,
                               const struct skeleton *const base,
                               const struct keyframe *const keyframe) {
        // Copy data
        skel->model = base->model;
        skel->nbones = base->nbones;
        skel->bones = smallocarray(skel->nbones, sizeof(*skel->bones));
        skel->boneOrder = smallocarray(skel->nbones, sizeof(*skel->boneOrder));
        memcpy(skel->bones, base->bones,
               skel->nbones * sizeof(*skel->bones));
        memcpy(skel->boneOrder, base->boneOrder,
               skel->nbones * sizeof(*skel->boneOrder));

        // Apply rotations and root offset from the keyframe
        assert(skel->nbones == keyframe->nbones);
        for (size_t i=0; i<skel->nbones; i++) {
                skel->bones[i].rotationRelative =
                        glms_quat_mul(skel->bones[i].rotationRelative,
                                      keyframe->relativeBoneRotations[i]);
        }
        size_t rootIdx = skel->boneOrder[0];
        skel->bones[rootIdx].positionRelative = glms_vec3_add(
                skel->bones[rootIdx].positionRelative, keyframe->rootOffset);

        // Recalculate absolute transforms
        calcAbsoluteTransforms(skel);
}

void skeleton_bindBones(const struct skeleton *const skel,
                        const enum shaders shader) {
        for (size_t i=0; i<skel->nbones; i++) {
                struct bone *bone = skel->bones + i;
                mat4s skinningMatrix = glms_mat4_mul(bone->absoluteTransform,
                                                     bone->bindPoseInv);
                static char buff[BUFFSIZE];
                snprintf(buff, BUFFSIZE, "bones[%lu]", i);
                shader_setMat4(shader, buff, skinningMatrix);
        }
}

void skeleton_free(struct skeleton *skel) {
        free(skel->bones);
        free(skel->boneOrder);
}
