#include <thirty/keyframe.h>
#include <thirty/util.h>

void keyframe_initFromFile(struct keyframe *const keyframe, FILE *const f,
                           const size_t nbones) {
        sfread(&keyframe->timestamp, sizeof(keyframe->timestamp), 1, f);
        sfread(keyframe->rootOffset.raw, sizeof(float), 3, f);
        keyframe->nbones = nbones;
        keyframe->relativeBoneRotations = smallocarray(nbones,
                                                       sizeof(versors));
        sfread(keyframe->relativeBoneRotations, sizeof(versors), nbones, f);
}

void keyframe_initFromInterp(const struct keyframe *const prev,
                             const struct keyframe *const next,
                             struct keyframe *const keyframe,
                             const float timestamp) {
        float interpolationPoint = (timestamp - prev->timestamp)/
                (next->timestamp - prev->timestamp);

        assert(prev->nbones == next->nbones);
        size_t nbones = prev->nbones;
        versors *interpRelBoneRots =
                smallocarray(nbones, sizeof(*interpRelBoneRots));
        for (size_t i=0; i<nbones; i++) {
                interpRelBoneRots[i] =
                        glms_quat_slerp(prev->relativeBoneRotations[i],
                                        next->relativeBoneRotations[i],
                                        interpolationPoint);
        }

        vec3s interpRootOffset = glms_vec3_lerp(prev->rootOffset,
                                                next->rootOffset,
                                                interpolationPoint);

        keyframe->timestamp = timestamp;
        keyframe->rootOffset = interpRootOffset;
        keyframe->nbones = prev->nbones;
        keyframe->relativeBoneRotations = interpRelBoneRots;
}

void keyframe_free(struct keyframe *const keyframe) {
        free(keyframe->relativeBoneRotations);
}
