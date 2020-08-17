#include <keyframe.h>
#include <util.h>

void keyframe_initFromFile(struct keyframe *const keyframe, FILE *const f,
                           const size_t nbones) {
        sfread(&keyframe->timestamp, sizeof(keyframe->timestamp), 1, f);
        sfread(keyframe->rootOffset.raw, sizeof(float), 3, f);
        keyframe->nbones = nbones;
        keyframe->relativeBoneRotations = smallocarray(nbones,
                                                       sizeof(versors));
        sfread(keyframe->relativeBoneRotations, sizeof(versors), nbones, f);
}

void keyframe_free(struct keyframe *const keyframe) {
        free(keyframe->relativeBoneRotations);
}
